/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: initialization and save trace log of pattern trace
 * Create: 2021-10-28
 */

#define pr_fmt(fmt) "dmsspt: " fmt
#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/sched/clock.h>
#include <linux/err.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/bitops.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <platform_include/basicplatform/linux/util.h>
#include <soc_qice_interface.h>
#include <ddr_define.h>
#include <mntn_public_interface.h>
#include "dmss_pt_dump.h"
#include "dmss_pt_drv.h"
#include "securec.h"
#include "hisi_ddr_autofsgt_proxy_kernel.h"

#define TRACE0 0
#define TRACE1 1

static u64 dmsspt_addr_chans2phy(unsigned int dmis, unsigned int gbl_trace_stat0, u32 intlv_gran, int num);
static inline u16 dmsspt_addr_phy2chans(u64 phy_addr);

struct dmsspt_devices {
	spinlock_t	lock;
	void __iomem 	*qice_base;
	u32 irq;
	u32 irq_cpu_core;
	u32 init_completed;
	u32 save_cnt; /* Number of times that dmsspt_save_trace is invoked in the current recording process */
	struct dmsspt_region pt_region;
	struct pattern_trace_info pt_info;
	struct pattern_trace_stat pt_stat;
};
static struct dmsspt_devices *g_pt0_devp;
static struct dmsspt_devices *g_pt1_devp;

struct dmsspt_cfg {
	u8 roll_en;
	u8 unaligned_mode; /* 0/1 Identifies the 32B/64B address non-aligned write command */
	u8 rec_ts_max_intrvl; /* insert timestamp interval forcibly */
	u8 rec_pri;
	u8 rec_mid;
	u8 rec_intlv_gran;
	u8 cur_freq;
	u16 rec_top_addr; /* Upper limit of the address in the trace space channel - MB */
	u16 rec_base_addr; /* low limit of the address in the trace space channel - MB */

	/* stop and interrupt */
	u32 max_pattern_num;
	u32 trace_prd;
	u16 int_trigger;
	u8 trace_int_en;

	/* filter conditions */
	u16 filter_top_addr; /* upper limit of the address that is filtered - MB */
	u16 filter_base_addr; /* low limit of the address that is filtered - MB */
#ifdef DDRC_FILTER_SRC_PLATFORM
	u8 filter_src; /* Monitoring source end of the TRACE command */
#endif
	u8 filter_ch;
	u8 filter_type_wr;
	u8 filter_type_rd;
	u32 filter_asi; /* filtered by condition for TRACE command's ASI, 0 - don't record */
	u32 filter_mid[MID_GROUP_SIZE]; /* Indicates the MID filtering. */
};

/* pattern trace's default configurations */
static struct dmsspt_cfg g_pt_cfg;
static struct dmsspt_cfg g_pt1_cfg;

static int dmsspt_is_stop(int num)
{
	int is_stop;
	SOC_QICE_TBED_TRACE_STAT0_UNION ctrl0;

	if (num == 0)
		ctrl0 = (SOC_QICE_TBED_TRACE_STAT0_UNION)readl(QICE_TBED_TRACE_STAT0_PTR(g_pt0_devp->qice_base + TBED_DMC0));
	else if (num == 1)
		ctrl0 = (SOC_QICE_TBED_TRACE_STAT0_UNION)readl(QICE_TBED_TRACE_STAT0_PTR(g_pt1_devp->qice_base + TBED_DMC1));

	is_stop = ctrl0.reg.trace_running ? 0 : 1;

	return is_stop;
}

static void dmsspt_enable(int enable, int num)
{
	volatile SOC_QICE_TBED_ABNML_NS_INT_CLR_STR *int_clear_ptr = NULL;

	if (num == 0) {
		int_clear_ptr = QICE_TBED_ABNML_NS_INT_CLR_PTR(g_pt0_devp->qice_base + TBED_DMC0);
		g_pt0_devp->save_cnt = 0;
	} else if (num == 1) {
		int_clear_ptr = QICE_TBED_ABNML_NS_INT_CLR_PTR(g_pt1_devp->qice_base + TBED_DMC1);
		g_pt1_devp->save_cnt = 0;
	}

	int_clear_ptr->reg.abnml_ns_int_clr = 1;
}

static u32 intlv_to_bytes(u32 intlv_gran)
{
	u32 intlv_bytes = 128;
	u32 intlv_gran_tmp;

#ifdef INTLV_GRAN_START_VALUE0
	intlv_gran_tmp = intlv_gran;
#else
	intlv_gran_tmp = intlv_gran - 1;
#endif

	if (intlv_gran >= 0 && intlv_gran <= INTLV_GRAN_MAX_VAL) {
		intlv_bytes = DMSS_INTLV_BYTES(intlv_gran_tmp);
	} else {
		pr_err("ddr intlv_gran is invalid!\n");
		WARN_ON(1);
	}

	return intlv_bytes;
}

static void dmsspt_get_intlv(void)
{
	SOC_QICE_TBED_GLB_ADDR_CTRL_UNION trace0_intlv, trace1_intlv;
	u32 trace0_intlv_gran_tmp, trace1_intlv_gran_tmp;

	trace0_intlv = (SOC_QICE_TBED_GLB_ADDR_CTRL_UNION)readl(QICE_TBED_GLB_ADDR_PTR(g_pt0_devp->qice_base + TBED_DMC0));
	g_pt0_devp->pt_info.intlv = intlv_to_bytes(trace0_intlv.reg.intlv_gran);

	trace1_intlv = (SOC_QICE_TBED_GLB_ADDR_CTRL_UNION)readl(QICE_TBED_GLB_ADDR_PTR(g_pt1_devp->qice_base + TBED_DMC1));
	g_pt1_devp->pt_info.intlv = intlv_to_bytes(trace1_intlv.reg.intlv_gran);

#ifdef INTLV_GRAN_START_VALUE0
	trace0_intlv_gran_tmp = trace0_intlv.reg.intlv_gran + 1;
	trace1_intlv_gran_tmp = trace1_intlv.reg.intlv_gran + 1;
#else
	trace0_intlv_gran_tmp = trace0_intlv.reg.intlv_gran;
	trace1_intlv_gran_tmp = trace1_intlv.reg.intlv_gran;
#endif

	g_pt_cfg.rec_intlv_gran = trace0_intlv_gran_tmp;
	g_pt1_cfg.rec_intlv_gran = trace1_intlv_gran_tmp;
}

static void dmsspt0_get_stat(void)
{
	unsigned int i;
	u64 pt_buf_sta, pt_buf_end, wptr, step;

	pt_buf_sta = g_pt0_devp->pt_region.base;
	pt_buf_end = g_pt0_devp->pt_region.base + g_pt0_devp->pt_region.size;
	step = (u64)g_pt0_devp->pt_info.intlv * 2;

	for (i = 0; i < g_pt0_devp->pt_info.dmi_num; i++) {
		g_pt0_devp->pt_stat.trace_cur_address[i] = readl(QICE_TBED_TRACE_STAT1_PTR(g_pt0_devp->qice_base + TBED_DMC0, i));
		g_pt0_devp->pt_stat.trace_pattern_cnt[i] = readl(QICE_TBED_TRACE_STAT2_PTR(g_pt0_devp->qice_base + TBED_DMC0, i));
		g_pt0_devp->pt_stat.trace_roll_cnt[i] = readl(QICE_TBED_TRACE_STAT3_PTR(g_pt0_devp->qice_base + TBED_DMC0, i));

		wptr = dmsspt_addr_chans2phy(i, g_pt0_devp->pt_stat.trace_cur_address[i],
									g_pt_cfg.rec_intlv_gran, TRACE0);
		g_pt0_devp->pt_info.pt_rptr[i] = 0;
		if (wptr >= pt_buf_end) {
			pr_info("Rollback wptr%u:[%pK]->[%pK]\n", i, (void *)wptr, (void *)(wptr - 2 * step));
			wptr = wptr - 2 * step;
		}

		if (g_pt0_devp->pt_stat.trace_roll_cnt[i] &&
		    g_pt0_devp->save_cnt == 0) {
			if (wptr + 2 * step < pt_buf_end)
				g_pt0_devp->pt_info.pt_rptr[i] = wptr + 2 * step;

			pr_info("Move rptr%u->%pK\n", i, (void *)g_pt0_devp->pt_info.pt_rptr[i]);
		}

		g_pt0_devp->pt_info.pt_wptr[i] = wptr;

		pr_info("pt_rptr%u:%pK\n", i, (void *)g_pt0_devp->pt_info.pt_rptr[i]);
		pr_info("pt_wptr%u:%pK\n", i, (void *)g_pt0_devp->pt_info.pt_wptr[i]);
		pr_info("trace_cur_addr%u:0x%x\n", i, g_pt0_devp->pt_stat.trace_cur_address[i]);
		pr_info("trace_pattern_cnt%u:0x%x\n", i, g_pt0_devp->pt_stat.trace_pattern_cnt[i]);
		pr_info("trace_roll_cnt%u:0x%x\n", i, g_pt0_devp->pt_stat.trace_roll_cnt[i]);
	}

	g_pt0_devp->pt_stat.trace_end_time = sched_clock();
}

static void dmsspt1_get_stat(void)
{
	unsigned int i;
	u64 pt_buf_sta, pt_buf_end, wptr, step;

	pt_buf_sta = g_pt1_devp->pt_region.base;
	pt_buf_end = g_pt1_devp->pt_region.base + g_pt1_devp->pt_region.size;
	step = (u64)g_pt1_devp->pt_info.intlv * 2;

	for (i = 0; i < g_pt1_devp->pt_info.dmi_num; i++) {
		g_pt1_devp->pt_stat.trace_cur_address[i] = readl(QICE_TBED_TRACE_STAT1_PTR(g_pt1_devp->qice_base + TBED_DMC1, i));
		g_pt1_devp->pt_stat.trace_pattern_cnt[i] = readl(QICE_TBED_TRACE_STAT2_PTR(g_pt1_devp->qice_base + TBED_DMC1, i));
		g_pt1_devp->pt_stat.trace_roll_cnt[i] = readl(QICE_TBED_TRACE_STAT3_PTR(g_pt1_devp->qice_base + TBED_DMC1, i));

		wptr = dmsspt_addr_chans2phy(i, g_pt1_devp->pt_stat.trace_cur_address[i],
									g_pt1_cfg.rec_intlv_gran, TRACE1);

		g_pt1_devp->pt_info.pt_rptr[i] = 0;
		if (wptr >= pt_buf_end) {
			pr_info("Rollback wptr%u:[%pK]->[%pK]\n", i, (void *)wptr, (void *)(wptr - 2*step));
			wptr = wptr - 2*step;
		}

		if (g_pt1_devp->pt_stat.trace_roll_cnt[i] &&
		    g_pt1_devp->save_cnt == 0) {
			if (wptr + 2 * step < pt_buf_end)
				g_pt1_devp->pt_info.pt_rptr[i] = wptr + 2*step;

			pr_info("Move rptr%u->%pK\n", i, (void *)g_pt1_devp->pt_info.pt_rptr[i]);
		}

		g_pt1_devp->pt_info.pt_wptr[i] = wptr;

		pr_info("pt_rptr%u:%pK\n", i, (void *)g_pt1_devp->pt_info.pt_rptr[i]);
		pr_info("pt_wptr%u:%pK\n", i, (void *)g_pt1_devp->pt_info.pt_wptr[i]);
		pr_info("trace_cur_addr%u:0x%x\n", i, g_pt1_devp->pt_stat.trace_cur_address[i]);
		pr_info("trace_pattern_cnt%u:0x%x\n", i, g_pt1_devp->pt_stat.trace_pattern_cnt[i]);
		pr_info("trace_roll_cnt%u:0x%x\n", i, g_pt1_devp->pt_stat.trace_roll_cnt[i]);
	}

	g_pt1_devp->pt_stat.trace_end_time = sched_clock();
}

static void dmsspt_pre_hw_init(struct dmsspt_cfg *cfg)
{
	int ret;
	u16 chans_addr;
	chans_addr = dmsspt_addr_phy2chans(g_pt0_devp->pt_region.base);
	cfg->rec_base_addr = chans_addr;

	chans_addr = dmsspt_addr_phy2chans(g_pt0_devp->pt_region.base + g_pt0_devp->pt_region.size - 1);
	cfg->rec_top_addr = chans_addr;

	ret = irq_set_affinity(g_pt0_devp->irq, cpumask_of(g_pt0_devp->irq_cpu_core));
	if (ret < 0) {
		WARN_ON(1); /*lint !e730*/
		pr_warn("[%s] irq affinity fail,%d\n", __func__, g_pt0_devp->irq_cpu_core);
	}
}

void dmsspt_pt0_hw_init(struct dmsspt_cfg *cfg)
{
	unsigned int mid_grps;
	volatile SOC_QICE_TBED_TRACE_CTRL0_UNION *trace0_ctrl1_ptr =
															QICE_TBED_TRACE_CTRL0_PTR(g_pt0_devp->qice_base + TBED_DMC0);
	volatile SOC_QICE_TBED_TRACE_CTRL1_UNION *trace0_ctrl2_ptr =
															QICE_TBED_TRACE_CTRL1_PTR(g_pt0_devp->qice_base + TBED_DMC0);
	volatile SOC_QICE_TBED_TRACE_REC0_UNION *trace0_rec0_ptr =
															QICE_TBED_TRACE_REC0_PTR(g_pt0_devp->qice_base + TBED_DMC0);
	volatile SOC_QICE_TBED_TRACE_REC1_UNION *trace0_rec1_ptr =
															QICE_TBED_TRACE_REC1_PTR(g_pt0_devp->qice_base + TBED_DMC0);
	volatile SOC_QICE_TBED_TRACE_INT_EN_UNION *trace0_int_en_ptr =
															QICE_TBED_TRACE_INT_EN_PTR(g_pt0_devp->qice_base + TBED_DMC0);
	volatile SOC_QICE_TBED_TRACE_FILTER0_UNION *trace0_filter0_ptr =
															QICE_TBED_TRACE_FILTER0_PTR(g_pt0_devp->qice_base + TBED_DMC0);
	volatile SOC_QICE_TBED_TRACE_FILTER1_UNION *trace0_filter1_ptr =
															QICE_TBED_TRACE_FILTER1_PTR(g_pt0_devp->qice_base + TBED_DMC0);
	volatile SOC_QICE_TBED_TRACE_FILTER2_UNION *trace0_filter2_ptr =
															QICE_TBED_TRACE_FILTER2_PTR(g_pt0_devp->qice_base + TBED_DMC0, 0);

	trace0_ctrl1_ptr->reg.max_pattern_num = cfg->max_pattern_num;
	trace0_ctrl1_ptr->reg.unaligned_mode = cfg->unaligned_mode;
	trace0_ctrl1_ptr->reg.roll_en = cfg->roll_en;

	trace0_ctrl2_ptr->reg.trace_prd = cfg->trace_prd;

	trace0_rec0_ptr->reg.rec_base_addr = cfg->rec_base_addr;
	trace0_rec0_ptr->reg.rec_top_addr = cfg->rec_top_addr;

	trace0_rec1_ptr->reg.rec_mid = cfg->rec_mid;
	trace0_rec1_ptr->reg.rec_pri = cfg->rec_pri;
	trace0_rec1_ptr->reg.rec_ts_max_intrvl = cfg->rec_ts_max_intrvl;

	trace0_int_en_ptr->reg.trace_int_en = cfg->trace_int_en;
	trace0_int_en_ptr->reg.int_trigger = cfg->int_trigger;

	trace0_filter0_ptr->reg.filter_base_addr = cfg->filter_base_addr;
	trace0_filter0_ptr->reg.filter_top_addr = cfg->filter_top_addr;

	trace0_filter1_ptr->reg.filter_ch = cfg->filter_ch;
	trace0_filter1_ptr->reg.filter_type_wr = cfg->filter_type_wr;
	trace0_filter1_ptr->reg.filter_type_rd = cfg->filter_type_rd;

#ifdef DDRC_FILTER_SRC_PLATFORM
	trace0_filter1_ptr->reg.filter_src = cfg->filter_src;
#endif

	for (mid_grps = 0; mid_grps < MID_GROUP_SIZE; mid_grps++) {
		trace0_filter2_ptr = QICE_TBED_TRACE_FILTER2_PTR(g_pt0_devp->qice_base + TBED_DMC0, mid_grps);
		trace0_filter2_ptr->value = cfg->filter_mid[mid_grps];
	}

	g_pt0_devp->pt_stat.trace_unaligned_mode = cfg->unaligned_mode;
}

void dmsspt_pt1_hw_init(struct dmsspt_cfg *cfg)
{
	unsigned int mid_grps;
	volatile SOC_QICE_TBED_TRACE_CTRL0_UNION *trace1_ctrl1_ptr =
															QICE_TBED_TRACE_CTRL0_PTR(g_pt1_devp->qice_base + TBED_DMC1);
	volatile SOC_QICE_TBED_TRACE_CTRL1_UNION *trace1_ctrl2_ptr =
															QICE_TBED_TRACE_CTRL1_PTR(g_pt1_devp->qice_base + TBED_DMC1);
	volatile SOC_QICE_TBED_TRACE_REC0_UNION *trace1_rec0_ptr =
															QICE_TBED_TRACE_REC0_PTR(g_pt1_devp->qice_base + TBED_DMC1);
	volatile SOC_QICE_TBED_TRACE_REC1_UNION *trace1_rec1_ptr =
															QICE_TBED_TRACE_REC1_PTR(g_pt1_devp->qice_base + TBED_DMC1);
	volatile SOC_QICE_TBED_TRACE_INT_EN_UNION *trace1_int_en_ptr =
															QICE_TBED_TRACE_INT_EN_PTR(g_pt1_devp->qice_base + TBED_DMC1);
	volatile SOC_QICE_TBED_TRACE_FILTER0_UNION *trace1_filter0_ptr =
															QICE_TBED_TRACE_FILTER0_PTR(g_pt1_devp->qice_base + TBED_DMC1);
	volatile SOC_QICE_TBED_TRACE_FILTER1_UNION *trace1_filter1_ptr =
															QICE_TBED_TRACE_FILTER1_PTR(g_pt1_devp->qice_base + TBED_DMC1);
	volatile SOC_QICE_TBED_TRACE_FILTER2_UNION *trace1_filter2_ptr =
															QICE_TBED_TRACE_FILTER2_PTR(g_pt1_devp->qice_base + TBED_DMC1, 0);

	trace1_ctrl1_ptr->reg.max_pattern_num = cfg->max_pattern_num;
	trace1_ctrl1_ptr->reg.unaligned_mode = cfg->unaligned_mode;
	trace1_ctrl1_ptr->reg.roll_en = cfg->roll_en;

	trace1_ctrl2_ptr->reg.trace_prd = cfg->trace_prd;

	trace1_rec0_ptr->reg.rec_base_addr = cfg->rec_base_addr;
	trace1_rec0_ptr->reg.rec_top_addr = cfg->rec_top_addr;

	trace1_rec1_ptr->reg.rec_mid = cfg->rec_mid;
	trace1_rec1_ptr->reg.rec_pri = cfg->rec_pri;
	trace1_rec1_ptr->reg.rec_ts_max_intrvl = cfg->rec_ts_max_intrvl;

	trace1_int_en_ptr->reg.trace_int_en = cfg->trace_int_en;
	trace1_int_en_ptr->reg.int_trigger = cfg->int_trigger;

	trace1_filter0_ptr->reg.filter_base_addr = cfg->filter_base_addr;
	trace1_filter0_ptr->reg.filter_top_addr = cfg->filter_top_addr;

	trace1_filter1_ptr->reg.filter_ch = cfg->filter_ch;
	trace1_filter1_ptr->reg.filter_type_wr = cfg->filter_type_wr;
	trace1_filter1_ptr->reg.filter_type_rd = cfg->filter_type_rd;

#ifdef DDRC_FILTER_SRC_PLATFORM
	trace1_filter1_ptr->reg.filter_src = cfg->filter_src;
#endif

	for (mid_grps = 0; mid_grps < MID_GROUP_SIZE; mid_grps++) {
		trace1_filter2_ptr = QICE_TBED_TRACE_FILTER2_PTR(g_pt1_devp->qice_base + TBED_DMC1, mid_grps);
		trace1_filter2_ptr->value = cfg->filter_mid[mid_grps];
	}

	g_pt1_devp->pt_stat.trace_unaligned_mode = cfg->unaligned_mode;
}

/* Convert dmc channel address to phy address */
static u64 dmsspt_addr_chans2phy(unsigned int dmis, unsigned int gbl_trace_stat0, u32 intlv_gran, int num)
{
/*
segment             high addr  | chsel(2bit) | low addr(size=intlv gran)
phyaddr bit def     zz...zzz   |     yy      | xx...xx
local variable  addr_high_bits |    chsel    | addr_low_bits
*/
	u32 chsel, chsel_start;
	u32 dmi_num, channel_bw;
	u64 stat_cur_addr = ((u64)(gbl_trace_stat0 & 0x7fffffff)) << 7;
	u64 phy_addr, addr_high_bits, addr_low_bits;

	dmi_num = g_pt0_devp->pt_info.dmi_num;
	channel_bw = dmi_num;

	if (dmis >= g_pt0_devp->pt_info.dmi_num ||
	    intlv_gran == 0 ||
	    intlv_gran >= INTLV_GRAN_MAX_VAL) {
		pr_err("%s param error: dmis=%u, intlv_gran=%u", __func__, dmis, intlv_gran);
		return 0;
	}

	/*
	        DMI1        DMI0
		     |           |
	      -------  or  -------
	      |     |      |     |
	     DMC3  DMC1   DMC2  DMC0
	*/
	if (num == 0)
		chsel = dmis * 2;
	else if (num == 1)
		chsel = dmis * 2 + 1;
	/*
	intlv_gran |  chsel     | chsel_start
	0: 128byte | addr[8:7]  | 7
	1: 256byte | addr[9:8]  | 8
	2: 512byte | addr[10:9] | 9
	...
	*/

	chsel_start = 6 + intlv_gran;

	addr_low_bits = ((u64)stat_cur_addr & ((1UL<<chsel_start) - 1));
	addr_high_bits = stat_cur_addr >> chsel_start;
	phy_addr = (addr_high_bits<<(chsel_start + channel_bw)) | ((u64)chsel<<chsel_start) | addr_low_bits;

	phy_addr += g_pt0_devp->pt_region.fama_offset;

	return phy_addr;
}

static u16 dmsspt_addr_phy2chans(u64 phy_addr)
{
	u16 chans_addr = 0xffff;
	u64 tmp;
	u32 channel_bw;

	channel_bw = g_pt0_devp->pt_info.dmi_num; /* number of bits occupied by the channel ID in the physical address */

	if (phy_addr < g_pt0_devp->pt_region.fama_offset) {
		pr_err("%s: phy_addr=%pK, fail!!!\n", __func__, (void *)phy_addr);
		WARN_ON(1);
	} else {
		phy_addr -= g_pt0_devp->pt_region.fama_offset;
	}

	tmp = phy_addr >> (channel_bw + 20); /* 4 channel(2bit) - MB(20bit) */

	if (tmp & (~0xffffUL)) {
		pr_err("%s: phy_addr=%pK, fail!!!\n", __func__, (void *)phy_addr);
		WARN_ON(1);
	} else {
		chans_addr = (u16)tmp;
	}

	return chans_addr;
}

#ifdef CONFIG_DFX_DEBUG_FS
static void dmsspt_init_store(unsigned int val)
{
	unsigned long flags;
	volatile SOC_QICE_GLB_TB_COMMON_UNION *glb_common_ptr = QICE_GLB_TB_COMMON_PTR(g_pt0_devp->qice_base);

	if (!val || (!dmsspt_is_stop(TRACE0))) {
		pr_warn("pattern trace is running\n");
		return;
	}
	if (!val || (!dmsspt_is_stop(TRACE1))) {
		pr_warn("pattern trace is running\n");
		return;
	}

	spin_lock_irqsave(&g_pt0_devp->lock, flags);
	glb_common_ptr->reg.trace_clk_on = 1;
	dmsspt_pre_hw_init(&g_pt_cfg);
	dmsspt_pt0_hw_init(&g_pt_cfg);
	dmsspt_pt1_hw_init(&g_pt_cfg);
	g_pt0_devp->init_completed = 1;
	g_pt1_devp->init_completed = 1;
	spin_unlock_irqrestore(&g_pt0_devp->lock, flags);
}

static void dmsspt_start_store(unsigned int val)
{
	unsigned long flags;
	volatile SOC_QICE_GLB_TB_TRACE_START_UNION *trace_start_ptr = QICE_GLB_TB_TRACE_START_PTR(g_pt0_devp->qice_base);

	if (!dmsspt_is_finished()) {
		pr_err("the last trace save is not finished!\n");
		return;
	}

#if defined(CONFIG_DDR_AUTO_FSGT)
	(void)ddr_autofsgt_ctrl(DDR_AUTOFSGT_PROXY_CLIENT_DMSSPT, DDR_AUTOFSGT_LOGIC_DIS);
#endif
	if (val && dmsspt_is_stop(TRACE0) && dmsspt_is_stop(TRACE1) && g_pt0_devp->init_completed) {
		spin_lock_irqsave(&g_pt0_devp->lock, flags);
		g_pt0_devp->pt_stat.trace_begin_time = sched_clock();
		g_pt1_devp->pt_stat.trace_begin_time = sched_clock();
		dmsspt_enable(1, TRACE0);
		dmsspt_enable(1, TRACE1);
		trace_start_ptr->reg.trace_start = 1;
		spin_unlock_irqrestore(&g_pt0_devp->lock, flags);
	}
}

void dmsspt_stop_store(unsigned int val)
{
	unsigned long flags;
	volatile SOC_QICE_GLB_TB_TRACE_STOP_UNION *trace_stop_ptr = QICE_GLB_TB_TRACE_STOP_PTR(g_pt0_devp->qice_base);

	trace_stop_ptr->reg.trace_stop = 1;
	if (val && !dmsspt_is_stop(TRACE0)) {
		spin_lock_irqsave(&g_pt0_devp->lock, flags);
		dmsspt_enable(0, TRACE0);
		pr_err("manual stop pattern trace.\n");
		g_pt0_devp->pt_stat.trace_end_time = sched_clock();
		dmsspt0_get_stat();
		(void)dmsspt_save_trace(&g_pt0_devp->pt_info, &g_pt0_devp->pt_stat, 1, TRACE0);
		spin_unlock_irqrestore(&g_pt0_devp->lock, flags);
	}

	if (val && !dmsspt_is_stop(TRACE1)) {
		spin_lock_irqsave(&g_pt1_devp->lock, flags);
		dmsspt_enable(0, TRACE1);
		pr_err("manual stop pattern trace.\n");
		g_pt1_devp->pt_stat.trace_end_time = sched_clock();
		dmsspt1_get_stat();
		(void)dmsspt_save_trace(&g_pt1_devp->pt_info, &g_pt1_devp->pt_stat, 1, TRACE1);
		spin_unlock_irqrestore(&g_pt1_devp->lock, flags);
	}

#if defined(CONFIG_DDR_AUTO_FSGT)
	(void)ddr_autofsgt_ctrl(DDR_AUTOFSGT_PROXY_CLIENT_DMSSPT, DDR_AUTOFSGT_LOGIC_EN);
#endif
}

static void dmsspt_irq_core_store(unsigned int val)
{
	if (val < CPU_CORE_NUM_MAX) {
		g_pt0_devp->irq_cpu_core = val;
		g_pt1_devp->irq_cpu_core = val;
	} else {
		WARN_ON(1);
	}
}

static ssize_t dmsspt_finished_show(char *kbuf)
{
	int cnt;

	if (dmsspt_is_finished()) {
		cnt = snprintf_s(kbuf,
				 (ssize_t)DMSSPT_SHOW_LEN_MAX,
				 (ssize_t)DMSSPT_SHOW_LEN_MAX,
				 "#finished#\n");
	} else {
		cnt = snprintf_s(kbuf,
				 (ssize_t)DMSSPT_SHOW_LEN_MAX,
				 (ssize_t)DMSSPT_SHOW_LEN_MAX,
				 "#saving#\n");
	}
	return cnt;
}

static ssize_t dmsspt_region_show(char *kbuf)
{
	int cnt;

	cnt = snprintf_s(kbuf, (ssize_t)DMSSPT_SHOW_LEN_MAX, (ssize_t)DMSSPT_SHOW_LEN_MAX, "base=0x%pK, size=0x%llx\n",\
		(void *)g_pt0_devp->pt_region.base, g_pt0_devp->pt_region.size);

	return cnt;
}

static ssize_t dmsspt_intlv_show(char *kbuf)
{
	int cnt;

	cnt = snprintf_s(kbuf, (ssize_t)DMSSPT_SHOW_LEN_MAX, (ssize_t)DMSSPT_SHOW_LEN_MAX, "intlv=%u",\
		g_pt0_devp->pt_info.intlv);

	return cnt;
}

/* debugfs node for demand dmsspt, such as init/start/stop .etc */
struct dmsspt_op_node{
	char *name;
	void (*store)(unsigned int);
	ssize_t (*show)(char *kbuf);
};

static const struct dmsspt_op_node g_pt_op_nodes[] = {
	{"init", dmsspt_init_store, NULL},
	{"start", dmsspt_start_store, NULL},
	{"stop", dmsspt_stop_store, NULL},
	{"irq_core", dmsspt_irq_core_store, NULL},
	{"finished", NULL, dmsspt_finished_show},
	{"rec_region", NULL, dmsspt_region_show},
	{"intlv", NULL, dmsspt_intlv_show},
};

static const struct dmsspt_op_node *find_op_node(struct file *filp)
{
	int ret;
	unsigned int i;
	const struct dmsspt_op_node *np = NULL;

	for (i = 0; i < ARRAY_SIZE(g_pt_op_nodes); i++) {
		ret = strncmp(g_pt_op_nodes[i].name, filp->private_data, strnlen(g_pt_op_nodes[i].name, NAME_LEN_MAX));
		if (!ret) {
			np = &g_pt_op_nodes[i];
			break;
		}
	}

	return np;
}

static ssize_t dmsspt_comm_store(struct file *filp, const char __user *ubuf, size_t cnt, loff_t *ppos)
{
	int ret;
	unsigned int val = 0;
	char kbuf[4] = {'\0'};
	const struct dmsspt_op_node *np;

	if (NULL == ubuf || 0 == cnt) {
		pr_err("buf is null !\n");
		return -EINVAL;
	}

	if (cnt >= sizeof(kbuf)) {
		pr_err("input char too many! \n");
		return -ENOMEM;
	}

	if (copy_from_user(kbuf, ubuf, cnt)) {
		return -EINVAL;
	}

	ret = kstrtouint(kbuf, 10, &val);
	if (ret < 0) {
		pr_err("input error: %x %x %x!\n", kbuf[0], kbuf[1], kbuf[2]);
		return -EINVAL;
	}

	np = find_op_node(filp);
	if (np && np->store)
		np->store(val);

	return (ssize_t)cnt;
}

ssize_t dmsspt_comm_show(struct file *filp, char __user *ubuf, size_t size, loff_t *ppos)
{
	int cnt = 0;
	char kbuf[DMSSPT_SHOW_LEN_MAX] = {0};
	const struct dmsspt_op_node *np;

	if (!access_ok(VERIFY_WRITE, ubuf, DMSSPT_SHOW_LEN_MAX)) {
		pr_err("ubuf is invalid@%s\n", (char *)filp->private_data);
		return -EINVAL;
	}

	np = find_op_node(filp);
	if (np && np->show) {
		cnt = np->show(kbuf);
		return simple_read_from_buffer(ubuf, size, ppos, kbuf, cnt);
	}

	return 0;
}

static int dmsspt_open(struct inode *inode, struct file *filp)
{
	filp->private_data = inode->i_private;
	return 0;
}

static const struct file_operations dmss_op_fops = {
	.open		= dmsspt_open,
	.write		= dmsspt_comm_store,
	.read		= dmsspt_comm_show,
};

/* debugfs node for config dmsspt */
struct dmsspt_mod_cfg {
	char *name;
	unsigned int bitwidth;
	void *val_p;
};

static const struct dmsspt_mod_cfg g_pt_mod_cfg[] = {
	{"roll_en",		1, &g_pt_cfg.roll_en},
	{"unaligned_mode",	1, &g_pt_cfg.unaligned_mode},
	{"rec_ts_max_intrvl",	4, &g_pt_cfg.rec_ts_max_intrvl},
	{"rec_pri",		3, &g_pt_cfg.rec_pri},
	{"rec_mid",		8, &g_pt_cfg.rec_mid},
	{"rec_intlv_gran",	3, &g_pt_cfg.rec_intlv_gran},
	{"cur_freq",		4, &g_pt_cfg.cur_freq},

	{"max_pattern_num",	28, &g_pt_cfg.max_pattern_num},
	{"trace_prd",		32, &g_pt_cfg.trace_prd},
	{"int_trigger",		16, &g_pt_cfg.int_trigger},
	{"trace_int_en",	1, &g_pt_cfg.trace_int_en},

	{"filter_ch",		4, &g_pt_cfg.filter_ch},
	{"filter_type_wr",	1, &g_pt_cfg.filter_type_wr},
	{"filter_type_rd",	1, &g_pt_cfg.filter_type_rd},
#ifdef DDRC_FILTER_SRC_PLATFORM
	{"filter_src",		1, &g_pt_cfg.filter_src},
#endif
	{"filter_asi",		32, &g_pt_cfg.filter_asi},
	{"filter_mid0",		32, &g_pt_cfg.filter_mid[0]},
	{"filter_mid1",		32, &g_pt_cfg.filter_mid[1]},
	{"filter_mid2",		32, &g_pt_cfg.filter_mid[2]},
	{"filter_mid3",		32, &g_pt_cfg.filter_mid[3]},
	{"filter_top_addr",	16, &g_pt_cfg.filter_top_addr},
	{"filter_base_addr",	16, &g_pt_cfg.filter_base_addr},
};

#define PRIV_AUTH       (S_IRUSR|S_IWUSR|S_IRGRP)
static int dmsspt_debugfs_init(void)
{
	unsigned int i;
	struct dentry *pt_dir, *mode_cfg;

	pt_dir = debugfs_create_dir("pattern_trace", NULL);
	if (!pt_dir)
		return -ENOMEM;

	mode_cfg = debugfs_create_dir("mode_cfg", pt_dir);
	if (!mode_cfg)
		return -ENOMEM;

	for (i = 0; i < ARRAY_SIZE(g_pt_op_nodes); i++)
		debugfs_create_file(g_pt_op_nodes[i].name, PRIV_AUTH, pt_dir,
				    g_pt_op_nodes[i].name, &dmss_op_fops);

	for (i = 0; i < ARRAY_SIZE(g_pt_mod_cfg); i++) {
		if (g_pt_mod_cfg[i].bitwidth == NODE_WIDTH_1_BITS) {
			debugfs_create_bool(g_pt_mod_cfg[i].name,
					    PRIV_AUTH, mode_cfg,
					    g_pt_mod_cfg[i].val_p);
		} else if (g_pt_mod_cfg[i].bitwidth <= NODE_WIDTH_8_BITS) {
			debugfs_create_x8(g_pt_mod_cfg[i].name,
					  PRIV_AUTH, mode_cfg,
					  g_pt_mod_cfg[i].val_p);
		} else if (g_pt_mod_cfg[i].bitwidth <= NODE_WIDTH_16_BITS) {
			debugfs_create_x16(g_pt_mod_cfg[i].name,
					   PRIV_AUTH, mode_cfg,
					   g_pt_mod_cfg[i].val_p);
		} else if (g_pt_mod_cfg[i].bitwidth <= NODE_WIDTH_32_BITS) {
			debugfs_create_x32(g_pt_mod_cfg[i].name,
					   PRIV_AUTH, mode_cfg,
					   g_pt_mod_cfg[i].val_p);
		} else {
			pr_err("create debugfs node fail\n");
			return -EINVAL;
		}
	}
	return 0;
}
#else
static int dmsspt_debugfs_init(void)
{
	return 0;
}
#endif

static irqreturn_t dmsspt_interrupt(int irq, void *dev_id)
{
	unsigned long flags;
	int ret, is_stop;

	volatile SOC_QICE_TBED_ABNML_NS_INT_ST_STR *trace0_int_stat_ptr =
																QICE_TBED_ABNML_NS_INT_ST_PTR(g_pt0_devp->qice_base + TBED_DMC0);
	volatile SOC_QICE_TBED_ABNML_NS_INT_CLR_STR *trace0_int_clear_ptr =
																QICE_TBED_ABNML_NS_INT_CLR_PTR(g_pt0_devp->qice_base + TBED_DMC0);

	volatile SOC_QICE_TBED_ABNML_NS_INT_ST_STR *trace1_int_stat_ptr =
																QICE_TBED_ABNML_NS_INT_ST_PTR(g_pt1_devp->qice_base + TBED_DMC1);
	volatile SOC_QICE_TBED_ABNML_NS_INT_CLR_STR *trace1_int_clear_ptr =
																QICE_TBED_ABNML_NS_INT_CLR_PTR(g_pt1_devp->qice_base + TBED_DMC1);

	pr_err("%s@%llu\n", __func__, sched_clock());

	if (!trace0_int_stat_ptr->reg.abnml_ns_int_trace_err_status) {
		pr_err("non pattern trace interrupt\n");
		WARN_ON(1);
		return IRQ_NONE;
	}

	if (!trace1_int_stat_ptr->reg.abnml_ns_int_trace_err_status) {
		pr_err("non pattern trace interrupt\n");
		WARN_ON(1);
		return IRQ_NONE;
	}

	spin_lock_irqsave(&g_pt0_devp->lock, flags);

	is_stop = dmsspt_is_stop(TRACE0);
	dmsspt0_get_stat();
	pr_err("is_stop=%d, cnt=%u\n", is_stop, g_pt0_devp->save_cnt);
	ret = dmsspt_save_trace(&g_pt0_devp->pt_info,
				&g_pt0_devp->pt_stat,
				(bool)is_stop, TRACE0);
	g_pt0_devp->save_cnt++;
	if (ret == 1) {
		pr_err("dmsspt0_save_trace fail, ret=%d!\n", ret);
		dmsspt_enable(0, TRACE0);
	}

	spin_unlock_irqrestore(&g_pt0_devp->lock, flags);

	spin_lock_irqsave(&g_pt1_devp->lock, flags);

	is_stop = dmsspt_is_stop(TRACE1);
	dmsspt1_get_stat();
	pr_err("is_stop=%d, cnt=%u\n", is_stop, g_pt1_devp->save_cnt);
	ret = dmsspt_save_trace(&g_pt1_devp->pt_info,
				&g_pt1_devp->pt_stat,
				(bool)is_stop, TRACE1);
	g_pt1_devp->save_cnt++;
	if (ret == 1) {
		pr_err("dmsspt1_save_trace fail, ret=%d!\n", ret);
		dmsspt_enable(0, TRACE1);
	}

	spin_unlock_irqrestore(&g_pt1_devp->lock, flags);

#if defined(CONFIG_DDR_AUTO_FSGT)
	if (is_stop || ret)
		(void)ddr_autofsgt_ctrl(DDR_AUTOFSGT_PROXY_CLIENT_DMSSPT, DDR_AUTOFSGT_LOGIC_EN);
#endif

	trace0_int_clear_ptr->reg.abnml_ns_int_clr = 1;
	trace1_int_clear_ptr->reg.abnml_ns_int_clr = 1;

	return IRQ_HANDLED;
}

static int dmsspt_platform_adapt(struct device_node *np)
{
	int cplen;
	const char *compatible;
	unsigned long cmplen;

	compatible = of_get_property(np, "compatible", &cplen);
	if (!compatible || cplen < 0) {
		pr_err("can find platform compatible");
		return -ENODEV;
	}
	pr_err("compatible = %s\n", compatible);
	cmplen = strlen(compatible);
	if (strncmp(compatible, "hisilicon,dmsspt", cmplen + 1) == 0) {
		g_pt0_devp->pt_info.dmi_num = 1;
		g_pt1_devp->pt_info.dmi_num = 1;
	}
	else {
		g_pt0_devp->pt_info.dmi_num = DDRC_DMI_NUM;
		g_pt1_devp->pt_info.dmi_num = DDRC_DMI_NUM;
	}

	return 0;
}

static void dmsspt_cpu_addr_shift(u64 phys)
{
	u32 addr_shift_mode = readl(QICE_TBED_GLB_ADDR_PTR(g_pt0_devp->qice_base + TBED_DMC0)) &
				    ADDR_SHIFT_MODE_MASK;

	pr_info("[%s]addr_shift_mode is %d\n", __func__, addr_shift_mode);
	if ((addr_shift_mode == ADDR_SHIFT_MODE_1 &&
	     phys >= ADDR_SHIFT_MODE_1_START_ADDR &&
	     phys < ADDR_SHIFT_MODE_1_END_ADDR) ||
	    (addr_shift_mode == ADDR_SHIFT_MODE_2 &&
	     phys >= ADDR_SHIFT_MODE_2_START_ADDR &&
	     phys < ADDR_SHIFT_MODE_2_END_ADDR)) {
		g_pt0_devp->pt_region.fama_offset = phys - DDR_SIZE_3G512M;
		g_pt1_devp->pt_region.fama_offset = phys - DDR_SIZE_3G512M;
	} else {
		g_pt0_devp->pt_region.fama_offset = 0;
		g_pt1_devp->pt_region.fama_offset = 0;
	}
	pr_err("%s, fama_offset:[0x%llx]\n", __func__, g_pt0_devp->pt_region.fama_offset);
}

static int dmsspt_probe_allocate_kernel_memory(struct device_node *node_ptr,
					     struct platform_device *plat_dev)
{
	if (check_mntn_switch(MNTN_DMSSPT) == 0) {
		pr_err("%s, MNTN_DMSSPT is closed!\n", __func__);
		return ERR;
	}

	if (node_ptr == NULL) {
		dev_err(&plat_dev->dev, "%s, no dev node\n", __func__);
		return -ENODEV;
	}

	g_pt0_devp = devm_kzalloc(&plat_dev->dev,
				 sizeof(struct dmsspt_devices), GFP_KERNEL);
	if (g_pt0_devp == NULL) {
		dev_warn(&plat_dev->dev, "Kzalloc failed.\n");
		return -ENOMEM;
	}

	g_pt1_devp = devm_kzalloc(&plat_dev->dev,
				 sizeof(struct dmsspt_devices), GFP_KERNEL);
	if (g_pt1_devp == NULL) {
		dev_warn(&plat_dev->dev, "Kzalloc failed.\n");
		return -ENOMEM;
	}

	return 0;
}

int dmsspt_probe(struct platform_device *pdev)
{
	int irq;
	int ret;
	struct device_node *np = pdev->dev.of_node;
	struct resource *res;

	pr_err("%s start\n", __func__);

	ret = dmsspt_probe_allocate_kernel_memory(np, pdev);
	if (ret == ERR)
		return 0;
	else if (ret == -ENODEV || ret == -ENOMEM)
		goto err;

	ret = dmsspt_platform_adapt(np);
	if (ret == -ENODEV)
		goto err;

	g_pt0_devp->pt_region = get_dmsspt_buffer_region();

	if (!g_pt0_devp->pt_region.base) {
		dev_err(&pdev->dev, "Trace0 buffer reserved failed\n");
		ret = -ENOMEM;
		goto err;
	}
	g_pt1_devp->pt_region = get_dmsspt_buffer_region();

	if (!g_pt1_devp->pt_region.base) {
		dev_err(&pdev->dev, "Trace1 buffer reserved failed\n");
		ret = -ENOMEM;
		goto err;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	g_pt0_devp->qice_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(g_pt0_devp->qice_base)) {
		ret = -ENOMEM;
		dev_err(&pdev->dev, "fail to ioremap !\n");
		goto err;
	}
	g_pt1_devp->qice_base = g_pt0_devp->qice_base;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		ret = -ENXIO;
		dev_err(&pdev->dev, "fail to get irq !\n");
		goto err;
	}
	ret = devm_request_irq(&pdev->dev, (unsigned int)irq, dmsspt_interrupt,
		(unsigned long)0, "hisi_dmsspt", pdev);
	if (ret < 0) {
		dev_err(&pdev->dev, "fail to request irq !\n");
		goto err;
	}
	g_pt0_devp->irq = irq;
	g_pt1_devp->irq = irq;
	dmsspt_cpu_addr_shift(g_pt0_devp->pt_region.base);
	dmsspt_get_intlv();
	ret = dmsspt_debugfs_init();
	if (ret != 0) {
		pr_err("%s debugfs init failed\n", __func__);
		goto err;
	}
	spin_lock_init(&g_pt0_devp->lock);
	spin_lock_init(&g_pt1_devp->lock);
	pr_err("%s success\n", __func__);
err:
	return ret;
}
/* lint -e785 */
static const struct of_device_id hisi_dmsspt_of_match[] = {
	PLATFORM_LIST
};
MODULE_DEVICE_TABLE(of, hisi_dmsspt_of_match);

static struct platform_driver hisi_dmsspt_driver = {
	.probe = dmsspt_probe,
	.driver = {
		.name  = "hisi-dmsspt",
		.of_match_table = of_match_ptr(hisi_dmsspt_of_match),
	},
};
/* lint +e785 */
/*lint -e528 -esym(528,*)*/
module_platform_driver(hisi_dmsspt_driver);/*lint   !e64*/
/*lint -e528 +esym(528,*)*/
/*lint -e753 -esym(753,*)*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("dmss pattern trace driver");
/*lint -e753 +esym(753,*)*/
