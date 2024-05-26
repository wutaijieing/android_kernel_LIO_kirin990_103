/*
 *
 * Analyze the occupancy of ddr bandwidth
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include "dfx_perf_stat64.h"
#include <asm/memory.h>
#include <asm/cacheflush.h>
#include <asm/irq_regs.h>
#include <asm/irqflags.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/mm.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/platform_device.h>
#include <linux/perf_event.h>
#include <linux/bitmap.h>
#include <linux/vmalloc.h>
#include <linux/gfp.h>
#include <linux/dma-direction.h>
#include <linux/ptrace.h>
#include <linux/mutex.h>
#include <linux/printk.h>
#include <linux/pm_wakeup.h>
#include <linux/regulator/consumer.h>
#include <linux/syscalls.h>
#include <linux/workqueue.h>
#include <linux/atomic.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#include <soc_pmctrl_interface.h>
#include <soc_acpu_baseaddr_interface.h>
#include <soc_crgperiph_interface.h>
#include <soc_perfstat_interface.h>
#include <soc_pctrl_interface.h>
#include <trace/events/power.h>
#include <securec.h>

/*
 * static char *perfstat_master_name[PERF_PORT_NUM] = {
 * chicago   boston ES   boston CS
 * "perf-dsssubsys0",   0:dss0   0:dss0
 * "perf-dsssubsys1",   1:dss1   1:dss1
 * "perf-ccisubsys0",   2:cci0   2:cpu_l
 * "perf-ccisubsys1",   3:cci1   3:cpu_b
 * "perf-g3d",   4:gpu0   4:media_common
 * "perf-modemsubsys0",   5:modem0   5:modem0
 * "perf-modemsubsys1",   6:modem1   6:modem1
 * "perf-ispsubsys0",   7:isp0   7:isp0
 * "perf-ispsubsys1",   8:isp1   8:isp1
 * "perf-vdec",   9:vdec   9:vdec
 * "perf-venc",   10:venc   10:venc
 * "perf-ivp",   11:ivp   11:ivp
 * "perf-system noc",   12:system noc   12:system noc
 * "perf-asp",   13:asp   13:asp
 * "perf-emmc",   14:cnn   14:ics
 * "perf-usbotg",   15:usbotg   15:usbotg
 * "perf-ufs",   16:ufs   16:ufs
 * "perf-pcie",   17:pcie   17:pcie
 * "perf-ispsrtdrm",   18:isp_srt_drm   18:isp_srt_drm
 * "perf-ccisubsys2",   19:cci2   19:gpu
 * "perf-ccisubsys3",   20:cci3   20:reserved
 * "perf-dmsssubsys1",   21:dmss1   21:dmss1
 * "perf-dmsssubsys0",   22:dmss0   22:dmss0
 * "perf-gpu1",   23:gpu1   23:jpeg
 * "perf-ipf",   24:ipf   24:ipf
 * "perf-dmca",   25:dmc a   25:dmc a
 * "perf-dmcb",   26:dmc b   26:dmc b
 * "perf-dmcc",   27:dmc c   27:dmc c
 * "perf-dmcd",   28:dmc d   28:dmc d
 * "pcie1",   29:pcie1   29:reserved
 * "dp",   30:dp   30:dp
 * "reserved"   31:reserved   31:reserved
 */

static struct perfstat_dev_info *perfstat_info;
static void __iomem *perfstat_base;
static void __iomem *pericrg_base;
static void __iomem *pctrl_base;
static unsigned long g_sample_time;
static bool perf_file_saved; /* default value is false */
struct completion perf_comp;
static bool is_probe_comp;
static struct mutex perfstat_transaction_mutex;
static atomic_t is_recording = ATOMIC_INIT(0);
static unsigned int suggest_clk;
static unsigned int suggest_aclk;
static unsigned int g_fpga_flag;
static unsigned int ics2_dp_sel;
static struct perf_event *current_event;

#ifdef CONFIG_DFX_PERF_STAT_LAT_MON
static struct perfstat_subsystem_info *g_subsys_info;
static unsigned int g_subsys_len;
static unsigned int g_unregister_def_port;

unsigned int perfstat_latency_monitor_register(unsigned int id, perfstat_get_sub_period_cb cb)
{
	if (g_subsys_len == 0)
		return PERF_ERROR;

	if (id >= g_subsys_len) {
		pr_err("[%s]: invalid parameter id=%d\n", __func__, id);
		return PERF_ERROR;
	}

	if (!cb)
		return PERF_ERROR;

	if (g_subsys_info[id].cb) {
		pr_err("[%s]: %s callback has been registered\n", __func__, g_subsys_info[id].name);
		return PERF_ERROR;
	}

	g_subsys_info[id].cb = cb;
	g_subsys_info[id].is_valid = true;

	return PERF_OK;
}

static int latency_monitor_period_check(int monitors)
{
	int period = PERF_ERROR;
	int i = 0;

	if (g_subsys_len == 0)
		return PERF_ERROR;

	for (; i < g_subsys_len; i++) {
		if ((1 << i) & monitors) {
			pr_err("[%s]: %s period is %d\n", __func__, g_subsys_info[i].name,
			       g_subsys_info[i].period);

			if (g_subsys_info[i].period != period && period != PERF_ERROR) {
				pr_err("[%s]: subsys latency monitors period are different\n", __func__);
				return PERF_ERROR;
			}
			period = g_subsys_info[i].period;
		}
	}

	return period;
}

static int latency_monitor_period_get(unsigned long sample_port_low,
				      unsigned long sample_port_high)
{
	int i = 0;
	int period = PERF_ERROR;
	unsigned int mon_id_rd;
	unsigned int mon_id_wr;
	unsigned int monitors = 0;
	unsigned long flag;

	if (g_subsys_len == 0)
		return PERF_ERROR;

	for (; i < g_subsys_len; i++) {
		flag = 0;
		mon_id_rd = g_subsys_info[i].read;
		mon_id_wr = g_subsys_info[i].write;
		if (mon_id_rd < BITS_PER_LONG) {
			flag = ((1UL << mon_id_rd) & sample_port_low);
			if (mon_id_wr != PERF_ERROR)
				flag |= ((1UL << mon_id_wr) & sample_port_low);
		} else {
			flag = ((1UL << (mon_id_rd - BITS_PER_LONG)) & sample_port_high);
			if (mon_id_wr != PERF_ERROR)
				flag |= ((1UL << (mon_id_wr - BITS_PER_LONG)) & sample_port_high);
		}

		if (flag == 0)
			continue;

		monitors |= (1 << i);
		if ((1 << i) & g_unregister_def_port) { /* VDEC VENC DMSS ports */
			g_subsys_info[i].period = UNREGISTER_DEF_PERIOD;
			continue;
		}

		/* DSS ISP ports */
		if (!g_subsys_info[i].is_valid) {
			pr_err("[%s]: %s is not registered\n", __func__, g_subsys_info[i].name);
			return PERF_ERROR;
		}

		g_subsys_info[i].period = g_subsys_info[i].cb();
		if (g_subsys_info[i].period < SUBSYS_PERIOD_MIN ||
		    g_subsys_info[i].period > SUBSYS_PERIOD_MAX) {
			pr_err("[%s]: %s period=%d is invalid\n", __func__,
			       g_subsys_info[i].name, g_subsys_info[i].period);
			return PERF_ERROR;
		}
	}

	period = latency_monitor_period_check(monitors);
	if (period == PERF_ERROR) {
		pr_err("[%s]: period check error, %d\n", __func__, period);
		return PERF_ERROR;
	}

	return period;
}
#endif

struct record {
	int ret;
	int size;
	int offset;
	int size_l;
	int size_h;
	int page_idx;
	int sample_total_size;
	int addr_mod;
	int i;
};
struct event {
	unsigned int allocate_size;
	int page_n;
	int i;
	int ret;
	int sample_port_num;
	u64 list_head_phyaddr;
};

static struct of_device_id of_perfstat_match_tbl[] = {
	{
		.compatible = "hisilicon,perfstat-driver",
	},
	/* end */
	{ }
};

static void perfstat_reset_en(void)
{
	writel(1 << SOC_CRGPERIPH_PERRSTEN0_ip_rst_perf_stat_START,
	       SOC_CRGPERIPH_PERRSTEN0_ADDR(pericrg_base));
}

static void perfstat_reset_dis(void)
{
	writel(1 << SOC_CRGPERIPH_PERRSTDIS0_ip_rst_perf_stat_START,
	       SOC_CRGPERIPH_PERRSTDIS0_ADDR(pericrg_base));
}

static void perfstat_reg_show(void)
{
	pr_err("0x0 clk_gate is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_CLK_GATE_ADDR(perfstat_base)));
	pr_err("0x4 cmd is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_CMD_START_ADDR(perfstat_base)));
	pr_err("0x8 sample_cnt is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_SAMPLE_CNT_REG_ADDR(perfstat_base)));
	pr_err("0xc sample_num is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_SAMPLE_NUM_REG_ADDR(perfstat_base)));
	pr_err("0x10 sample_stop is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_SAMPLE_STOP_ADDR(perfstat_base)));
#ifndef SOC_PERFSTAT_REAL_PORT_NUM_L_ADDR
	pr_err("0x14 REAL_PORT_NUM is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_REAL_PORT_NUM_ADDR(perfstat_base)));
	pr_err("0x18 axi_addr_mode is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_AXI_ADDR_MODE_ADDR(perfstat_base)));
#else
	pr_err("0x14 REAL_PORT_NUM_L is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_REAL_PORT_NUM_ADDR(perfstat_base)));
	pr_err("0x18 REAL_PORT_NUM_H is 0x%x\n", *(volatile unsigned int *)(DRV_SOC_PERFSTAT_REAL_PORT_NUM_H_ADDR(perfstat_base)));
	pr_err("0x1c axi_addr_mode is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_AXI_ADDR_MODE_ADDR(perfstat_base)));
#endif
	pr_err("0x20 SEQ_ADDR_LEN is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_STAT_RST_CNT_ADDR(perfstat_base)));
	pr_err("0x2c STAT_RST_CNT is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_STAT_RST_CNT_ADDR(perfstat_base)));
	pr_err("0x30 INT_EN is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_INT_EN_ADDR(perfstat_base)));
	pr_err("0x34 INT_CLR is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_INT_CLR_ADDR(perfstat_base)));
	pr_err("0x38 RAW_INT_STAT is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_RAW_INT_STAT_ADDR(perfstat_base)));
	pr_err("0x3c MASK_INT_STAT is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_MASK_INT_STAT_ADDR(perfstat_base)));
	pr_err("0x40 ALL_SAMPLE_NUM is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_ALL_SAMPLE_NUM_ADDR(perfstat_base)));
#ifndef SOC_PERFSTAT_MAX_PORT_NUM_ADDR
	pr_err("0x50 DEBUG_MONITOR is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_DEBUG_MONITOR_ADDR(perfstat_base)));
#endif
	pr_err("0x54 MONITOR_RESET is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_MONITOR_RESET_ADDR(perfstat_base)));
	pr_err("0x58 SEQ_ADDR_LOW is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_SEQ_ADDR_LOW_ADDR(perfstat_base)));
	pr_err("0x5c SEQ_ADDR_HIGH is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_SEQ_ADDR_HIGH_ADDR(perfstat_base)));

	pr_err("0x60 DESCRIPTOR_ADDR_LOW is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_DESCRIPTOR_ADDR_LOW_ADDR(perfstat_base)));
	pr_err("0x64 DESCRIPTOR_ADDR_HIGH is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_DESCRIPTOR_ADDR_HIGH_ADDR(perfstat_base)));
	pr_err("0x68 LAST_DESC_ADDR_LOW is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_LAST_DESC_ADDR_LOW_ADDR(perfstat_base)));
	pr_err("0x6c LAST_DESC_ADDR_HIGH is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_LAST_DESC_ADDR_HIGH_ADDR(perfstat_base)));
	pr_err("0x70 LAST_SAMPLE_AXI_ADDR_LOW is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_LAST_SAMPLE_AXI_ADDR_LOW_ADDR(perfstat_base)));

	pr_err("0x74 LAST_SAMPLE_AXI_ADDR_HIGH is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_LAST_SAMPLE_AXI_ADDR_HIGH_ADDR(perfstat_base)));
	pr_err("0x78 PDRST_TMO_CNT_CFG is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_PDRST_TMO_CNT_CFG_ADDR(perfstat_base)));

#ifdef SOC_PERFSTAT_MAX_PORT_NUM_ADDR
	pr_err("0x7c PORT_INDEX_0 is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_PORT_INDEX_0_ADDR(perfstat_base)));
	pr_err("0x80 PORT_INDEX_1 is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_PORT_INDEX_1_ADDR(perfstat_base)));
	pr_err("0x84 PORT_INDEX_2 is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_PORT_INDEX_2_ADDR(perfstat_base)));
	pr_err("0x88 PORT_INDEX_3 is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_PORT_INDEX_3_ADDR(perfstat_base)));
	pr_err("0x8c PORT_INDEX_4 is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_PORT_INDEX_4_ADDR(perfstat_base)));
	pr_err("0x90 PORT_INDEX_5 is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_PORT_INDEX_5_ADDR(perfstat_base)));
	pr_err("0x94 PORT_INDEX_6 is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_PORT_INDEX_6_ADDR(perfstat_base)));
	pr_err("0x98 PORT_INDEX_7 is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_PORT_INDEX_7_ADDR(perfstat_base)));

	pr_err("0x9C MAX_PORT_NUM is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_MAX_PORT_NUM_ADDR(perfstat_base)));
	pr_err("0x120 REAL_PORT_NUM_2 is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_REAL_PORT_NUM_2_ADDR(perfstat_base)));
	pr_err("0x124 REAL_PORT_NUM_3 is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_REAL_PORT_NUM_3_ADDR(perfstat_base)));
#else
#ifdef PERFSTAT_PORT_CAPACITY_128
	pr_err("0x7c RS_ENABLE1_L is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_RS_ENABLE1_L_ADDR(perfstat_base)));
	pr_err("0x80 RS_ENABLE1_H is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_RS_ENABLE1_H_ADDR(perfstat_base)));
	pr_err("0x84 RS_ENABLE2_L is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_RS_ENABLE2_L_ADDR(perfstat_base)));
	pr_err("0x88 RS_ENABLE2_H is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_RS_ENABLE2_H_ADDR(perfstat_base)));
	pr_err("0x8c RS_ENABLE3_L is 0x%x\n", *(volatile unsigned int *)(SOC_PERFSTAT_RS_ENABLE3_L_ADDR(perfstat_base)));
#endif
#endif
}

static void perfstat_set_sample_cnt(int cnt)
{
	writel(cnt, SOC_PERFSTAT_SAMPLE_CNT_REG_ADDR(perfstat_base));
}

static void perfstat_set_sample_period(int usec)
{
	int value;
	struct perfstat_dev_info *dev_info = perfstat_info;

	value = usec * (int)(dev_info->clk_perf_stat_rate / PERF_MILLION);
	pr_err("[%s] clk_perf_stat_rate is 0x%lx\n", __func__,
	       dev_info->clk_perf_stat_rate);
	perfstat_set_sample_cnt(value);
}

static void perfstat_set_sample_num(u32 times)
{
	writel(times, SOC_PERFSTAT_SAMPLE_NUM_REG_ADDR(perfstat_base));
}

#ifdef SOC_PERFSTAT_MAX_PORT_NUM_ADDR
static void perfstat_set_clk_gate(u32 status)
{
	writel(status, SOC_PERFSTAT_CLK_GATE_ADDR(perfstat_base));
}

static void perfstat_set_port_num(unsigned int num)
{
	writel(num, SOC_PERFSTAT_MAX_PORT_NUM_ADDR(perfstat_base));
}

static void perfstat_set_port_index(unsigned long perf_port_low64, unsigned long perf_port_high64)
{
	unsigned int i;
	unsigned int port_reg_index = 0;
	unsigned int reg_8bit = 0;
	unsigned int port_index[PERF_PORT_INDEX_NUM] = {0, 0, 0, 0, 0, 0, 0, 0};

	for (i = 0; i < sizeof(perf_port_low64) * BIT_SIZE; i++) {
		if (perf_port_low64 & PERF_PORT_BIT(i)) {
			port_index[port_reg_index] += i << (reg_8bit * BIT_SIZE);
			if (++reg_8bit >= sizeof(port_index[0])) {
				reg_8bit = 0;
				port_reg_index++;
			}
		}
	}

	for (i = 0; i < sizeof(perf_port_high64) * BIT_SIZE; i++) {
		if (perf_port_high64 & PERF_PORT_BIT(i)) {
			port_index[port_reg_index] += (i + sizeof(perf_port_low64) * BIT_SIZE) << (reg_8bit * BIT_SIZE);
			if (++reg_8bit >= sizeof(port_index[0])) {
				reg_8bit = 0;
				port_reg_index++;
			}
		}
	}

	for (i = 0; i < PERF_PORT_INDEX_NUM; i++)
		writel(port_index[i], SOC_PERFSTAT_PORT_INDEX_0_ADDR(perfstat_base) + i * PORT_INDEX_ADDR_OFFSET);
}
#endif


#ifdef PERFSTAT_PORT_CAPACITY_128
static void perfstat_set_sample_port(unsigned long port, unsigned long port_extend0_64)
#else
static void perfstat_set_sample_port(unsigned long port)
#endif
{
	unsigned int perf_port_low;
	unsigned int perf_port_high;

#ifdef PERFSTAT_PORT_CAPACITY_128
	unsigned int perf_port_high_low;
	unsigned int perf_port_high_high;

	unsigned int port_h_32 = (port & PERF_PORT_H_MASK) >> OFFSET_32;
	unsigned int port_high_h_32 = (port_extend0_64 & PERF_PORT_H_MASK) >> OFFSET_32;

	perf_port_low = port & PERF_PORT_L_MASK;
	perf_port_high_low = port_extend0_64 & PERF_PORT_L_MASK;

	if (port_h_32 > 0) {
		writel(perf_port_low, SOC_PERFSTAT_REAL_PORT_NUM_ADDR(perfstat_base));
		perf_port_high = (port & PERF_PORT_H_MASK) >> OFFSET_32;
		writel(perf_port_high, DRV_SOC_PERFSTAT_REAL_PORT_NUM_H_ADDR(perfstat_base));
		pr_err("====[perfstat_set_sample_port_low hi32 lo=%x hi=%x]====\n", perf_port_low, perf_port_high);
	} else {
		writel(perf_port_low, SOC_PERFSTAT_REAL_PORT_NUM_ADDR(perfstat_base));
		pr_err("====[perfstat_set_sample_port_low l32 lo=%x]====\n", perf_port_low);
	}

	if (port_high_h_32 > 0) {
		writel(perf_port_high_low, SOC_PERFSTAT_REAL_PORT_NUM2_ADDR(perfstat_base));
		perf_port_high_high = (port_extend0_64 & PERF_PORT_H_MASK) >> OFFSET_32;
		writel(perf_port_high_high, DRV_SOC_PERFSTAT_REAL_PORT_NUM_3_ADDR(perfstat_base));
		pr_err("====[perfstat_set_sample_port_high hi32 lo=%x hi=%x]====\n", perf_port_high_low, perf_port_high_high);
	} else {
		writel(perf_port_high_low, SOC_PERFSTAT_REAL_PORT_NUM2_ADDR(perfstat_base));
		pr_err("====[perfstat_set_sample_port_high l32 lo=%x]====\n", perf_port_high_low);
	}
#else
	unsigned int port_h_32 = (port & PERF_PORT_H_MASK) >> OFFSET_32;

	perf_port_low = port & PERF_PORT_L_MASK;

	if (port_h_32 > 0) {
		writel(perf_port_low, SOC_PERFSTAT_REAL_PORT_NUM_ADDR(perfstat_base));
		perf_port_high = (port & PERF_PORT_H_MASK) >> OFFSET_32;
		writel(perf_port_high, DRV_SOC_PERFSTAT_REAL_PORT_NUM_H_ADDR(perfstat_base));
		pr_err("====[perfstat_set_sample_port_low hi32 lo=%x hi=%x]====\n", perf_port_low, perf_port_high);
	} else {
		writel(perf_port_low, SOC_PERFSTAT_REAL_PORT_NUM_ADDR(perfstat_base));
		pr_err("====[perfstat_set_sample_port_low l32 lo=%x]====\n", perf_port_low);
	}
#endif
}

static void perfstat_set_addr_mode(int mode)
{
	writel(mode, SOC_PERFSTAT_AXI_ADDR_MODE_ADDR(perfstat_base));
}

/* bit[0]: 0 -ics2; 1-dp;  bit[1-31]: reserve */
static void perfstat_set_ics2_dp_sel(int mode)
{
	writel(mode, SOC_PCTRL_PERI_CTRL4_ADDR(pctrl_base));
}

static int perfstat_set_list_mode_addr(u64 addr)
{
	if (!PERF_IS_ALIGNED(addr, LIST_ADDR_BIT)) {
		pr_err("[%s] 8 bytes align needed\n", __func__);
		return PERF_ERROR;
	}

	writel((u32) addr, SOC_PERFSTAT_DESCRIPTOR_ADDR_LOW_ADDR(perfstat_base));
	writel((u32) (addr >> LIST_ADDR_OFFSET), SOC_PERFSTAT_DESCRIPTOR_ADDR_HIGH_ADDR(perfstat_base));

	return PERF_OK;
}

static void perfstat_set_timeout_cnt(int cnt)
{
	writel(cnt, SOC_PERFSTAT_OVERTIME_CFG_CNT_ADDR(perfstat_base));
}

static void perfstat_set_timeout_duration(int msec)
{
	int value;
	struct perfstat_dev_info *dev_info = perfstat_info;

	value = msec * (int)(dev_info->clk_perf_stat_rate / SET_TIME);
	perfstat_set_timeout_cnt(value);
}

static void perfstat_set_monitor_reset(void)
{
	writel(0x1, SOC_PERFSTAT_MONITOR_RESET_ADDR(perfstat_base));
}

static void perfstat_trigger_sample_start(void)
{
	writel(0x1, SOC_PERFSTAT_CMD_START_ADDR(perfstat_base));
}

static void perfstat_trigger_sample_stop(void)
{
	writel(0x1, SOC_PERFSTAT_SAMPLE_STOP_ADDR(perfstat_base));
}

static void perfstat_enable_interrupt(unsigned int type)
{
	unsigned int value;

	value = readl(SOC_PERFSTAT_INT_EN_ADDR(perfstat_base));
	value |= type;
	writel(value, SOC_PERFSTAT_INT_EN_ADDR(perfstat_base));
}

#ifndef SOC_PERFSTAT_MAX_PORT_NUM_ADDR
static void perfstat_enable_rs(struct perfstat_dev_info *dev_info)
{
	unsigned int i;

	for (i = 0; i < dev_info->rs_enable_support; i++)
		writel(dev_info->rs_data[RS_DATA_NUM * i + 1], perfstat_base + dev_info->rs_data[RS_DATA_NUM * i]);
}
#endif

static void perfstat_clear_interrupt(int type)
{
	writel(type, SOC_PERFSTAT_INT_CLR_ADDR(perfstat_base));
}

static unsigned int perfstat_get_masked_int_stat(void)
{
	return
	    readl(SOC_PERFSTAT_MASK_INT_STAT_ADDR(perfstat_base));
}

static int perfstat_get_cur_sample_cnt(void)
{
	return
	    readl(SOC_PERFSTAT_ALL_SAMPLE_NUM_ADDR(perfstat_base));
}

static u64 perfstat_get_cur_data_addr(void)
{
	u64 val_l, val_h, val;

	val_l =
	    readl(SOC_PERFSTAT_LAST_SAMPLE_AXI_ADDR_LOW_ADDR
			      (perfstat_base));
	val_h =
	    readl(SOC_PERFSTAT_LAST_SAMPLE_AXI_ADDR_HIGH_ADDR
			      (perfstat_base));
	val = (val_h << CUR_ADDR_H_OFFSET) | (val_l & CUR_ADDR_L_BIT);
	return val;
}

static u64 perfstat_get_cur_list_addr(void)
{
	u64 val_l, val_h, val;

	val_l =
	    readl(SOC_PERFSTAT_LAST_DESC_ADDR_LOW_ADDR
			      (perfstat_base));
	val_h =
	    readl(SOC_PERFSTAT_LAST_DESC_ADDR_HIGH_ADDR
			      (perfstat_base));
	val = (val_h << CUR_ADDR_H_OFFSET) | (val_l & CUR_ADDR_L_BIT);
	return val;
}

static int perfstat_get_fifo_status(void)
{
	return
	    readl(SOC_PERFSTAT_DEBUG_FIFO_FULL_ADDR(perfstat_base));
}

#ifndef SOC_PERFSTAT_MAX_PORT_NUM_ADDR
static int perfstat_get_block_port(void)
{
	return
	    readl(SOC_PERFSTAT_DEBUG_MONITOR_ADDR(perfstat_base));
}
#endif

static u64 perfstat_get_list_mode_addr(void)
{
	u64 val;

	val = readl(SOC_PERFSTAT_DESCRIPTOR_ADDR_HIGH_ADDR(perfstat_base));
	val |= (u64)((val << CUR_ADDR_H_OFFSET) + (u32)readl(SOC_PERFSTAT_LAST_DESC_ADDR_LOW_ADDR(perfstat_base)));
	return val;
}

static int perfstat_get_reset_cnt(void)
{
	return readl(SOC_PERFSTAT_STAT_RST_CNT_ADDR(perfstat_base));
}

static int perfstat_get_is_sampling(void)
{
	return readl(SOC_PERFSTAT_CMD_START_ADDR(perfstat_base));
}

static u32 perfstat_get_sample_cnt(void)
{
	return readl(SOC_PERFSTAT_SAMPLE_CNT_REG_ADDR(perfstat_base));
}

static int perfstat_set_memory_sample_cnt(void *pos)
{
	void *cnt_reg_memory_pos = NULL;
	u32 perf_sample_cnt_reg_value;
#ifdef PERFSTAT_PORT_CAPACITY_128
	struct perfstat_dev_info *dev_info = perfstat_info;
	u64 *pdrst_timeout = NULL;
	u64 *port_num = NULL;
	u64 tmp;
#endif
	int ret;

	if (pos == NULL)
		return PERF_ERROR;

	/* get value from reg,then clean sample cnt reg position context */
	perf_sample_cnt_reg_value = perfstat_get_sample_cnt();

	cnt_reg_memory_pos = pos + sizeof(u64);

	/* according spec. we need memset 8 bytes */
	ret = memset_s(cnt_reg_memory_pos, sizeof(u64), 0x00, sizeof(u64));
	if (ret != EOK) {
		pr_err("[%s] perfstat set memory sample cnt memset error\n", __func__);
		return PERF_ERROR;
	}

	/*
	 * set the sample cnt reg value from reg to the memory
	 * for new platform, perf modify the value
	 */
	ret = memcpy_s(cnt_reg_memory_pos, sizeof(u64), &perf_sample_cnt_reg_value, sizeof(u32));
	if (ret != EOK) {
		pr_err("[%s] perfstat set memory sample cnt memcpy error\n", __func__);
		return PERF_ERROR;
	}

#ifdef PERFSTAT_PORT_CAPACITY_128
	if (dev_info->samp_type == PERF_SAMPLE_HSCYCLE) {
		pdrst_timeout = (u64 *)pos + POS_OFFSET_4;
		port_num = (u64 *)pos + POS_OFFSET_6;
		pr_err("pdrst_timeout is %llx\n", *pdrst_timeout);
		pr_err("port_num is %llx\n", *port_num);
		tmp = *pdrst_timeout;
		*pdrst_timeout = *port_num;
		*port_num = tmp;
		pr_err("pdrst_timeout is %llx\n", *pdrst_timeout);
		pr_err("port_num is %llx\n", *port_num);
	}
#endif
	return PERF_OK;
}

static int perfstat_pwr_enable(void)
{
	int ret, i;
	perfstat_reset_dis();
	for (i = 0; i < perfstat_info->clk_data->num; i++) {
		ret = clk_prepare_enable(perfstat_info->clk_data->perfstat_clk[i]);
		if (ret) {
			pr_err("[%s] enable %s fails, value is=%d\n", __func__, perfstat_info->clk_data->name[i], ret);
			for (i = i - 1; i >= 0; i--)
				clk_disable_unprepare(perfstat_info->clk_data->perfstat_clk[i]);
			return PERF_ERROR;
		}
		pr_info("[%s] succeed to enable clock %s\n", __func__, perfstat_info->clk_data->name[i]);
	}
	return PERF_OK;
}

static void perfstat_pwr_disable(void)
{
	int i;
	perfstat_reset_en();
	for (i = 0; i < perfstat_info->clk_data->num; i++) {
		clk_disable_unprepare(perfstat_info->clk_data->perfstat_clk[i]);
		pr_info("[%s] succeed to disable clock %s\n", __func__, perfstat_info->clk_data->name[i]);
	}
}

static void perfstat_debug_info_show(void)
{
	int cur_samp_cnt;
	int dbg_fifo_stat;
#ifndef SOC_PERFSTAT_MAX_PORT_NUM_ADDR
	int dbg_block_port;
#endif
	u64 cur_data_addr;
	u64 cur_list_addr;

	cur_samp_cnt = perfstat_get_cur_sample_cnt();
	pr_err("[%s]CurSampCnt   : 0x%x\n", __func__, cur_samp_cnt);
	cur_data_addr = perfstat_get_cur_data_addr();
	pr_err("[%s]CurDataAddr  : 0x%llx\n", __func__, cur_data_addr);
	cur_list_addr = perfstat_get_cur_list_addr();
	pr_err("[%s]CurListAddr  : 0x%llx\n", __func__, cur_list_addr);
	dbg_fifo_stat = perfstat_get_fifo_status();
	pr_err("[%s]DbgFIFOStat  : 0x%x\n", __func__, dbg_fifo_stat);
#ifndef SOC_PERFSTAT_MAX_PORT_NUM_ADDR
	dbg_block_port = perfstat_get_block_port();
	pr_err("[%s]DbgBlockPort : 0x%x\n", __func__, dbg_block_port);
#endif
}

static void perfstat_list_done_handler(void)
{
	struct perfstat_dev_info *dev_info = perfstat_info;

	pr_err("[%s]perfstat list done.\n", __func__);
	perfstat_debug_info_show();

	if (dev_info->samp_type == PERF_SAMPLE_HIGHSPEED) {
		if (!perf_file_saved && !atomic_read(&is_recording)) {
			pr_err("%s: save perf data...\n", __func__);
			schedule_delayed_work(&dev_info->hs_record_data_work, 0);
		}
	}
}

static void perfstat_sample_done_handler(void)
{
	struct perfstat_dev_info *dev_info = perfstat_info;

	pr_err("[%s]perf sample done\n", __func__);
	perfstat_debug_info_show();
	if (!dev_info->list) {
		pr_err("[%s]no list exits\n", __func__);
		return;
	}

	if (dev_info->samp_type == PERF_SAMPLE_HSCYCLE &&
	    !perf_file_saved && !atomic_read(&is_recording)) {
		pr_err("%s: save perf data...\n", __func__);
		schedule_delayed_work(&dev_info->hs_record_data_work, 0);
	}
}

static void perfstat_list_error_handler(void)
{
	struct perfstat_dev_info *dev_info = perfstat_info;
	perf_list_descriptor *list_cfg = NULL;
	u64 val;

	pr_err("[%s]perf list error\n", __func__);
	perfstat_debug_info_show();

	val = perfstat_get_list_mode_addr();
	pr_err("[%s]list addr inReg is 0x%llx\n", __func__, val);
	list_cfg = dev_info->list;
	if (list_cfg == NULL)
		pr_err("[%s]list addr inSof is null\n", __func__);
	else
		pr_err("[%s]list addr inSof is 0x%lx\n", __func__,
		       (uintptr_t) __virt_to_phys((uintptr_t)list_cfg));
}

static void perfstat_port_overtime_handler(void)
{
	pr_err("[%s]perf port overtime\n", __func__);
	perfstat_debug_info_show();
}

static irqreturn_t perfstat_interrupt_handler(int irq, void *data)
{
	unsigned int val;
	struct perfstat_dev_info *dev_info = (struct perfstat_dev_info *)data;

	if (dev_info == NULL) {
		pr_err("[%s] dev_info is NULL\n", __func__);
		return IRQ_HANDLED;
	}

	spin_lock(&dev_info->synclock);
	if (dev_info->status == PERF_TASK_DONE) {
		pr_err("[%s]perfstat is in TASK_DONE mode!\n", __func__);
		perfstat_clear_interrupt(PERF_INT_ALL);
		goto out;
	}

	val = perfstat_get_masked_int_stat();
	if (val & PERF_INT_SAMP_DONE) {
		perfstat_clear_interrupt(PERF_INT_ALL);
		perfstat_sample_done_handler();
	}

	if (val & PERF_INT_LIST_DONE) {
		perfstat_clear_interrupt(PERF_INT_LIST_DONE);
		perfstat_list_done_handler();
	}

	if (val & PERF_INT_LIST_ERR) {
		perfstat_clear_interrupt(PERF_INT_LIST_ERR);
		perfstat_list_error_handler();
	}

	if (val & PERF_INT_OVER_TIME) {
		perfstat_clear_interrupt(PERF_INT_OVER_TIME);
		perfstat_port_overtime_handler();
	}

out:
	spin_unlock(&dev_info->synclock);
	return IRQ_HANDLED;
}

static int perfstat_check_ready(void)
{
	volatile int val;
	volatile int count;

	count = PERF_CHECK_READY_TIMEOUT;
	do {
		/* wait reset cnt to 0 or timeout */
		val = perfstat_get_reset_cnt();
		count--;
	} while ((val != 0) && (count != 0));

	if (!count)
		return PERF_ERROR;

	perfstat_set_monitor_reset();
	return PERF_OK;
}


#ifdef PERFSTAT_PORT_CAPACITY_128
static int perfstat_master_is_valid(unsigned long *sprt, unsigned long *port_extend0_64)
#else
static int perfstat_master_is_valid(unsigned long *sprt)
#endif
{
	unsigned int i;
	struct perfstat_dev_info *dev_info = perfstat_info;

	if (sprt == NULL) {
		pr_err("[%s] sample_port is NULL\n", __func__);
		return PERF_ERROR;
	}
	if (dev_info == NULL) {
		pr_err("[%s] dev_info is NULL\n", __func__);
		return PERF_ERROR;
	}

	pr_err("[%s]port is 0x%lx\n", __func__, *sprt);

#ifdef PERFSTAT_PORT_CAPACITY_128
	for (i = 0; i < PERF_PORT_NUM; i++) {
		if (*sprt & PERF_PORT_BIT(i)) {
			if (!(dev_info->vldmsk_of_sprt & PERF_PORT_BIT(i))) {
				pr_err("[%s] port %d is not valid!\n",
				       __func__, i);
				*sprt &= (unsigned long)(~PERF_PORT_BIT(i)); /* clear this port */
			}
		}
	}

	for (i = 0; i < PERF_PORT_NUM; i++) {
		if (*port_extend0_64 & PERF_PORT_BIT(i)) {
			if (!(dev_info->vldmsk_of_sprt_h & PERF_PORT_BIT(i))) {
				pr_err("[%s] port %d is not valid!\n",
				       __func__, i);
				*port_extend0_64 &= (unsigned long)(~PERF_PORT_BIT(i)); /* clear this port */
			}
		}
	}
#else
	for (i = 0; i < PERF_PORT_NUM; i++) {
		if (*sprt & PERF_PORT_BIT(i)) {
			if (!(dev_info->vldmsk_of_sprt & PERF_PORT_BIT(i))) {
				pr_err("[%s] port %d is not valid!\n",
				       __func__, i);
				*sprt &= (unsigned long)(~PERF_PORT_BIT(i)); /* clear this port */
			}
		}
	}
#endif

	/* If no master is powered up, not need to start perfstat. */
#ifdef PERFSTAT_PORT_CAPACITY_128
	if (*sprt == 0 && *port_extend0_64 == 0) {
		pr_err("[%s]no master is valid\n", __func__);
		return PERF_ERROR;
	}
#else
	if (*sprt == 0) {
		pr_err("[%s]no master is valid\n", __func__);
		return PERF_ERROR;
	}
#endif
	return PERF_OK;
}

static void perfstat_record_sample_data(struct work_struct *work)
{
	struct perfstat_dev_info *dev_info = perfstat_info;
	void *start = NULL;
	struct record record_data = {0};
	long fd;
	u64 cur_list_addr;
	u64 list_addr;
	u64 cur_data_addr;

	if (work == NULL) {
		pr_err("[%s] work_struct is NULL\n", __func__);
		return;
	}

	if (dev_info == NULL) {
		pr_err("[%s] dev_info is NULL\n", __func__);
		return;
	}

	if (!atomic_add_unless(&is_recording, 1, 1)) {
		pr_err("[%s] is recording, failed to record\n", __func__);
		return;
	}

	pr_err("[%s]perf sample done\n", __func__);
	perfstat_debug_info_show();
	if (!dev_info->list) {
		pr_err("[%s]no list exits\n", __func__);
		goto record_err;
	}

	start = dev_info->virt_addr;
	record_data.size = PAGE_SIZE * dev_info->pages_n;
	/* invalid data buffer to be copied */
	__dma_map_area((const void *)start, record_data.size, DMA_FROM_DEVICE);

	record_data.sample_total_size =
	    dev_info->sample_per_sz * perfstat_get_cur_sample_cnt() +
	    (int)sizeof(struct perf_sample_data_head);
	pr_err("[%s]record_data.sample_total_size is 0x%x\n", __func__, record_data.sample_total_size);
	pr_err("[%s]sample_per_sz is 0x%x,perfstat_get_cur_sample_cnt() is 0x%x\n",
	       __func__, dev_info->sample_per_sz, perfstat_get_cur_sample_cnt());
	pr_err("[%s]record_data.size is 0x%x, pages_n is 0x%x\n", __func__, record_data.size,
	       dev_info->pages_n);

	if (dev_info->samp_type == PERF_SAMPLE_HSCYCLE
	    && record_data.sample_total_size > (int)PAGE_SIZE * dev_info->pages_n) {
		/* find out the current page_idx in pages */
		cur_list_addr = perfstat_get_cur_list_addr();
		cur_data_addr = perfstat_get_cur_data_addr();
		list_addr = (uintptr_t)__virt_to_phys((uintptr_t)dev_info->list);

		pr_err("[%s] cur_list_addr is 0x%llx\n", __func__, cur_list_addr);
		pr_err("[%s] cur_data_addr is 0x%llx\n", __func__, cur_data_addr);
		pr_err("[%s] list_addr is 0x%llx\n", __func__, list_addr);

		for (record_data.i = 0; record_data.i < dev_info->pages_n; record_data.i++) {
			if (list_addr == cur_list_addr)
				break;
			list_addr = list_addr + sizeof(perf_list_descriptor);
		}

		if (unlikely(dev_info->pages_n == record_data.i)) {
			pr_err("[%s]no list addr matched,dev_info->pagns_n is 0x%x\n",
			     __func__, dev_info->pages_n);
			goto record_err;
		}

		record_data.page_idx = record_data.i;
		/* count the data start_addr & size */

#ifdef PERFSTAT_PORT_CAPACITY_128
		record_data.addr_mod =
		    (dev_info->pages_n * (int)PAGE_SIZE) % dev_info->sample_per_sz;
		if (record_data.addr_mod == 0)
			record_data.addr_mod = dev_info->sample_per_sz;

		pr_info("[%s] record_data.addr_mode=%d", __func__, record_data.addr_mod);
		record_data.offset =
		    (int)(cur_data_addr - dev_info->list[record_data.page_idx].bit_config.address);
		pr_info("[%s] record_data.offset=%d", __func__, record_data.offset);
		start =
		    (void *)((char *)(dev_info->virt_addr) +
			     PAGE_SIZE * record_data.page_idx + record_data.offset + record_data.addr_mod) - SAMPLE_START_OFFSET;
		record_data.size_l =
		    PAGE_SIZE * (dev_info->pages_n - record_data.page_idx) - record_data.offset -
		    record_data.addr_mod + SAMPLE_START_OFFSET;
		record_data.size_h = PAGE_SIZE * record_data.page_idx + record_data.offset + record_data.addr_mod - SAMPLE_START_OFFSET;
#else
		record_data.addr_mod =
		    (dev_info->pages_n * (int)PAGE_SIZE) % dev_info->sample_per_sz;
		if (record_data.addr_mod == 0)
			record_data.addr_mod = dev_info->sample_per_sz;

		pr_info("[%s] record_data.addr_mode=%d", __func__, record_data.addr_mod);
		record_data.offset =
		    (int)(cur_data_addr - dev_info->list[record_data.page_idx].bit_config.address);
		pr_info("[%s] record_data.offset=%d", __func__, record_data.offset);
		start =
		    (void *)((char *)(dev_info->virt_addr) +
			     PAGE_SIZE * record_data.page_idx + record_data.offset + record_data.addr_mod);
		record_data.size_l =
		    PAGE_SIZE * (dev_info->pages_n - record_data.page_idx) - record_data.offset -
		    record_data.addr_mod;
		record_data.size_h = PAGE_SIZE * record_data.page_idx + record_data.offset + record_data.addr_mod;
#endif

		pr_err("[%s] end\n", __func__);
	}

	/* according phoenix new feature,soft adapt modify */
	record_data.ret = perfstat_set_memory_sample_cnt(start);
	if (record_data.ret < 0) {
		pr_err("[%s]get menory sample failed, record_data.ret is %d\n", __func__, record_data.ret);
		return;
	}

	fd = SYS_OPEN(PERF_HIGHSPEED_DATA_PATH, O_CREAT | O_WRONLY | O_TRUNC,
		      PERF_FILE_MASK);
	if (fd < 0) {
		pr_err("[%s]open file failed, fd is %ld\n", __func__, fd);
		goto record_err;
	}

	/* write time info */
	record_data.ret = SYS_WRITE(fd, (const char *)&g_sample_time, sizeof(g_sample_time));
	if (record_data.ret < 0) {
		pr_err("[%s]write file failed, ret is %d\n", __func__, record_data.ret);
		goto writing_err;
	}

	if (dev_info->samp_type == PERF_SAMPLE_HSCYCLE
	    && (unsigned int)record_data.sample_total_size > PAGE_SIZE * dev_info->pages_n) {
		/* write perfstat raw data */
		record_data.ret = SYS_WRITE(fd, start, record_data.size_l);
		if (record_data.ret < 0) {
			pr_err("[%s]write file failed, record_data.ret is %d\n", __func__,
			       record_data.ret);
			goto writing_err;
		}
		/* write perfstat raw data */
		record_data.ret = SYS_WRITE(fd, dev_info->virt_addr, record_data.size_h);
		if (record_data.ret < 0) {
			pr_err("[%s]write file failed, record_data.ret is %d\n", __func__,
			       record_data.ret);
			goto writing_err;
		}
	} else {
		/* write perfstat raw data */
		record_data.ret = SYS_WRITE(fd, start, record_data.size);
		if (record_data.ret < 0) {
			pr_err("[%s]write file failed, record_data.ret is %d\n", __func__,
			       record_data.ret);
			goto writing_err;
		}
	}

	record_data.ret = SYS_FSYNC(fd);
	if (record_data.ret < 0)
		pr_err("[%s]sys_fsync failed, ret is %d\n", __func__, record_data.ret);
	perf_file_saved = true;
writing_err:
	record_data.ret = SYS_CLOSE(fd);
	if (record_data.ret < 0)
		pr_err("[%s]sys_close failed, ret is %d\n", __func__, record_data.ret);

	pr_err("[%s]perf data saved complete!\n", __func__);

record_err:
	atomic_dec(&is_recording);
	complete(&perf_comp);
}
static void perfstat_event_destroy(struct perf_event *event)
{
	struct perfstat_dev_info *dev_info = perfstat_info;
	struct page **pages;
	int i;

	if (event == NULL) {
		pr_err("[%s] perfstat_probe is not intialized\n", __func__);
		return;
	}

	mutex_lock(&perfstat_transaction_mutex);
	if (dev_info->status != PERF_DEL_DONE) {
		pr_err("[%s] perfstat is not running\n", __func__);
		goto err_end;
	}
	pr_err("[%s] start running.\n", __func__);
	if (perfstat_get_is_sampling())
		perfstat_trigger_sample_stop();

	wait_for_completion(&perf_comp);

	pages = dev_info->pages;
	vunmap((const void *)dev_info->virt_addr);
	for (i = 0; i < dev_info->pages_n; i++)
		__free_pages(pages[i], 0);

	kfree(dev_info->list);
	kfree(dev_info->pages);
	dev_info->list = NULL;
	dev_info->pages = NULL;

	perf_file_saved = false;
	dev_info->event_bk = NULL;
	dev_info->virt_addr = NULL;

	perfstat_pwr_disable();

	spin_lock(&dev_info->synclock);
	dev_info->status = PERF_TASK_DONE;
	spin_unlock(&dev_info->synclock);
err_end:
	mutex_unlock(&perfstat_transaction_mutex);
}

static int perfstat_check_perm(struct perf_event *event,
				struct perfstat_dev_info *dev_info)
{
	int sample_period;

	if (event->attr.type != PERF_TYPE_PERF_STAT)
		goto check_err;

	/* get the period from attr.sample_period. */
	sample_period = event->attr.sample_period & SAMPLE_PERIOD_MASK;
	if (sample_period <= 0) {
		pr_err("[%s]sample_period %d<=0\n", __func__, sample_period);
		goto check_err;
	}

	return 0;
check_err:
	return -1;
}

static int perfstat_calculate_pages(struct perfstat_dev_info *dev_info, int *page_n,
				int sample_period, unsigned int allocate_size)
{
	int sz;

	if ((dev_info == NULL) || (sample_period == 0)) {
		pr_err("[%s] dev_info or sample_period is 0", __func__);
		return PERF_ERROR;
	}

	if (allocate_size == 0) {
		pr_err("%s: timing not specified, use default time\n",
		       __func__);
		*page_n = PERF_MALLOC_PAGES_100M;
	} else {
		if (dev_info->samp_type == PERF_SAMPLE_HSCYCLE) {
			/* when in this mode, sptm means sample space size. */
			*page_n = (int)allocate_size * PERF_SIZE_1M / (int)PAGE_SIZE;
		} else {
			sz = dev_info->sample_per_sz * DIV_ROUND_UP((int)allocate_size * PERF_TIME_1US, sample_period) +
			    (int)sizeof(struct perf_sample_data_head);
			if (sz <= 0) {
				pr_err("[%s] allocate_size:0x%x is too big and sz = %d!", __func__, allocate_size, sz);
				return PERF_ERROR;
			}

			*page_n = DIV_ROUND_UP((u32)sz, PAGE_SIZE);
		}
	}
	pr_info("[%s] page_n=%d, sample_period=%d, allocate_size=%x\n", __func__, *page_n, sample_period, allocate_size);
	return 0;
}

static int perfstat_initialize_list_cfg(struct perfstat_dev_info *dev_info,
				perf_list_descriptor *list_cfg,
				u64 *list_head_phyaddr)
{
	signed int i;
	u64 phy_addr;

	if (list_cfg == NULL) {
		pr_err("[%s]list_cfg is not intialized\n", __func__);
		return -1;
	}

	if (dev_info == NULL) {
		pr_err("[%s]dev_info is not intialized\n", __func__);
		return -1;
	}
	*list_head_phyaddr = (u64) __virt_to_phys((uintptr_t)list_cfg);
	pr_info("[%s] list_head_phyaddr is 0x%llx\n", __func__,
		*list_head_phyaddr);

	for (i = 0; i < dev_info->pages_n; i++) {
		dev_info->pages[i] = alloc_page(GFP_KERNEL);
		if (!dev_info->pages[i]) {
			pr_err("[%s] alloc page error\n", __func__);
			return -1;
		}

		phy_addr =
		    (uintptr_t) __virt_to_phys((uintptr_t)page_address(dev_info->pages[i]));
		if (dev_info->samp_type == PERF_SAMPLE_HIGHSPEED
		    && i == dev_info->pages_n - 1) {
			list_cfg[i].value.value0 = PERF_LIST_CFG_INT;
		} else {
			list_cfg[i].value.value0 = PERF_LIST_CFG_NORMAL;
		}
		list_cfg[i].bit_config.address = phy_addr;
		list_cfg[i].bit_config.length = PAGE_SIZE;
	}
	/* last descriptor is a link to the head */
	if (dev_info->samp_type == PERF_SAMPLE_HSCYCLE) {
		list_cfg[dev_info->pages_n].value.value0 = PERF_LIST_CFG_LINK;
		list_cfg[dev_info->pages_n].bit_config.address =
		    *list_head_phyaddr;
	}

	return 0;
}

static struct clk *perfstat_find_clk(struct perfstat_dev_info *dev_info, const char *clk_name)
{
	int i;
	for (i = 0; i < dev_info->clk_data->num; i++) {
		if (strcmp(dev_info->clk_data->name[i], clk_name) == 0)
			return dev_info->clk_data->perfstat_clk[i];
	}
	return NULL;
}

static int perfstat_clk_init(struct platform_device *pdev, unsigned int fpga_flag)
{
	struct perfstat_dev_info *dev_info = perfstat_info;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct clk *clk_perf_stat = NULL;
	struct clk *aclk_perf_stat = NULL;
	int ret, i;

	dev_info->clk_data = kzalloc(sizeof(*dev_info->clk_data), GFP_KERNEL);
	if (!dev_info->clk_data) {
		pr_err("[%s] clk_data kzalloc error\n", __func__);
		return -ENOMEM;
	}

	dev_info->clk_data->num = of_property_count_strings(np, "clock-names");
	if (dev_info->clk_data->num <= 0) {
		pr_err("[%s] clk_data->num is %d\n", __func__, dev_info->clk_data->num);
		kfree(dev_info->clk_data);
		return PERF_ERROR;
	}

	dev_info->clk_data->name = kcalloc(dev_info->clk_data->num, sizeof(char *), GFP_KERNEL);
	if (!dev_info->clk_data->name) {
		pr_err("[%s] clk_data name kzalloc error\n", __func__);
		kfree(dev_info->clk_data);
		return -ENOMEM;
	}

	dev_info->clk_data->perfstat_clk = kcalloc(dev_info->clk_data->num, sizeof(struct clk *), GFP_KERNEL);
	if (!dev_info->clk_data->perfstat_clk) {
		pr_err("[%s] clk_data clk kzalloc error\n", __func__);
		kfree(dev_info->clk_data->name);
		kfree(dev_info->clk_data);
		return -ENOMEM;
	}

	for (i = 0; i < dev_info->clk_data->num; i++) {
		ret = of_property_read_string_index(np, "clock-names", i,
						    &dev_info->clk_data->name[i]);
		if (ret) {
			pr_err("[%s] failed to get clock name at idx %d\n", __func__, i);
			return PERF_ERROR;
		}

		dev_info->clk_data->perfstat_clk[i] = devm_clk_get(dev, dev_info->clk_data->name[i]);
		if (IS_ERR_OR_NULL(dev_info->clk_data->perfstat_clk[i])) {
			pr_err("[%s] failed to get %s from dts\n", __func__, dev_info->clk_data->name[i]);
			return -ENODEV;
		}
		pr_info("[%s] succeed to get clock name at idx %d, name is %s\n", __func__, i, dev_info->clk_data->name[i]);
	}

	clk_perf_stat = perfstat_find_clk(dev_info, "clk_perf_stat");
	if (!clk_perf_stat) {
		pr_err("[%s] cannot find clk_perf_stat\n", __func__);
		return PERF_ERROR;
	}
	aclk_perf_stat = perfstat_find_clk(dev_info, "aclk_perf_stat");
	if (!aclk_perf_stat) {
		pr_err("[%s] cannot find aclk_perf_stat\n", __func__);
		return PERF_ERROR;
	}

	if (!fpga_flag) { /* When fpga mode, no need to set clk_perf_stat to 120MHz */
		ret = clk_set_rate(clk_perf_stat, suggest_clk);
		if (ret) {
			pr_err("[%s] set clk_perf_stat error\n", __func__);
			return -ENODEV;
		}
	}

	dev_info->clk_perf_stat_rate = clk_get_rate(clk_perf_stat);
	pr_err("[%s] clk_perf_stat is %lu\n", __func__, dev_info->clk_perf_stat_rate);

	dev_info->aclk_perf_stat_rate = clk_get_rate(aclk_perf_stat);
	pr_err("[%s] aclk_perf_stat is %lu\n", __func__, dev_info->aclk_perf_stat_rate);
	return PERF_OK;
}

#ifdef PERFSTAT_PORT_CAPACITY_128
static int perfstat_get_edata(struct perf_event *event, int *sample_period,
				unsigned long *sample_port_low, unsigned long *sample_port_high, unsigned int *allocate_size)
#else
static int perfstat_get_edata(struct perf_event *event, int *sample_period,
				unsigned long *sample_port_low, unsigned int *allocate_size)
#endif
{
	int ret;
	unsigned int event_id, event_base;
	unsigned long sample_port_l;
	unsigned long sample_port_h;
#ifdef PERFSTAT_PORT_CAPACITY_128
	unsigned long sample_port_h_l;
	unsigned long sample_port_h_h;
#endif

	if (event == NULL) {
		pr_err("[%s]perf_event is not intialized\n", __func__);
		goto intialize_err;
	}

	if (sample_port_low == NULL) {
		pr_err("[%s]sample_port_low is not intialized\n", __func__);
		goto intialize_err;
	}

	if (sample_period == NULL) {
		pr_err("[%s]sample_period is not intialized\n", __func__);
		goto intialize_err;
	}

	if (allocate_size == NULL) {
		pr_err("[%s]sample_time is not intialized\n", __func__);
		goto intialize_err;
	}

	event_id = (event->attr.config >> OFFSET_48) & EVENT_ID_BIT;
	event_base = (perfstat_info->event_id >> OFFSET_16) & EVENT_BASE_BIT;
	pr_err("[%s] event_id=%x, event_base=%x", __func__, event_id, event_base);
	if (event_id == event_base + PERF_SAMPLE_HSCYCLE) {
		perfstat_info->samp_type = PERF_SAMPLE_HSCYCLE;
		pr_err("[%s]perfstat use high speed cycle sample type\n",
		       __func__);
	} else if (event_id == event_base + PERF_SAMPLE_HIGHSPEED) {
		perfstat_info->samp_type = PERF_SAMPLE_HIGHSPEED;
		pr_err("[%s]perfstat use high speed sample type\n", __func__);
	} else {
		pr_err("[%s] use mode error\n", __func__);
		goto intialize_err;
	}
	/* count buffer size to store data,then figure out
	 * the pages size needed, and also the page size
	 * reported once a interrupt
	 */
	/* get the period from attr.sample_period. */
	*sample_period = event->attr.sample_period & SAMPLE_PERIOD_BIT;
	/* get the low 32 bit from attr.config as sample port. */
#ifdef PERFSTAT_PORT_CAPACITY_128
	sample_port_l = event->attr.config & PERF_PORT_ALL;
	sample_port_h = event->attr.config1 & PERF_PORT_ALL;
	sample_port_h_l = (event->attr.config1 & PERF_PORT_H_MASK) >> OFFSET_32;
	sample_port_h_h = event->attr.config2 & PERF_PORT_ALL;
	*sample_port_low = (sample_port_h << OFFSET_32) + sample_port_l;
	*sample_port_high = (sample_port_h_h << OFFSET_32) + sample_port_h_l;
	pr_err("[%s]sample_port_l is %lx, sample_port_h is %lx, sample_port_h_l is %lx, sample_port_h_h is %lx, sample_port_low is %lx, sample_port_high is %lx!\n",
	       __func__, sample_port_l, sample_port_h, sample_port_h_l, sample_port_h_h, *sample_port_low, *sample_port_high);
#else
	sample_port_l = event->attr.config & PERF_PORT_ALL;
	sample_port_h = event->attr.config1 & PERF_PORT_ALL;

	*sample_port_low = (sample_port_h << OFFSET_32) + sample_port_l;
	pr_err("[%s]sample_port_l is %lx, sample_port_h is %lx, sample_port_low is %lx!\n", __func__,
		sample_port_l, sample_port_h, *sample_port_low);
#endif

	/* get the third low 16 bit from attr.config as sample time or perf data space size. */
	*allocate_size = (int)((event->attr.config >> OFFSET_32) & ALLOCATE_BIT);

#ifdef PERFSTAT_PORT_CAPACITY_128
	ret = perfstat_master_is_valid(sample_port_low, sample_port_high);
#else
	ret = perfstat_master_is_valid(sample_port_low);
#endif
	if (ret != PERF_OK) {
		pr_err("[%s]no master is valid!\n", __func__);
		goto intialize_err;
	}

	event->attr.sample_type = PERF_SAMPLE_RAW;
	event->attr.mmap = 0;
	event->attr.comm = 0;
	event->attr.task = 0;
	event->destroy = perfstat_event_destroy;
	event->attr.disabled = 0;
	event->state = PERF_EVENT_STATE_INACTIVE;

	return 0;
intialize_err:
	return -ENOENT;
}


static void perfstat_fill_devinfo(struct perfstat_dev_info *dev_info, int page_n,
				unsigned long sample_port_low, perf_list_descriptor *list_cfg)
{
	pr_err("[%s] dev_info->per_data_sz is 0x%x\n", __func__,
	       dev_info->per_data_sz);
	pr_err("[%s] perf_sample_data_head size is %d,perf_sample_data_end sz is %d\n",
	     __func__, (int) sizeof(struct perf_sample_data_head),
	     (int) sizeof(struct perf_sample_data_end));

	dev_info->pages_n = page_n;
	dev_info->sprt = sample_port_low;

	dev_info->list = list_cfg;
}


static int perfstat_do_check(struct perf_event *event,
				struct perfstat_dev_info *dev_info)
{
	int ret;

	ret = perfstat_check_ready();
	if (ret < 0) {
		pr_err("[%s]perfstat is not ready\n", __func__);
		return ret;
	}

	ret = perfstat_check_perm(event, dev_info);
	if (ret < 0)
		return ret;

	return ret;
}

static int perfstat_event_init(struct perf_event *event)
{
	struct perfstat_dev_info *dev_info = perfstat_info;
	struct page **pages = NULL;
	perf_list_descriptor *list_cfg = NULL;
	struct event event_var;
	struct clk *aclk_perf_stat = NULL;
	int sample_period;
	unsigned long sample_port_low;
#ifdef PERFSTAT_PORT_CAPACITY_128
	unsigned long sample_port_high;
#endif
#ifdef CONFIG_DFX_PERF_STAT_LAT_MON
	int subsys_period = PERF_ERROR;
#endif

	mutex_lock(&perfstat_transaction_mutex);

	if (event == NULL) {
		pr_err("[%s] event is null!\n", __func__);
		goto err_end;
	}

	if (event->attr.type != PERF_TYPE_PERF_STAT)
		goto err_end;

	if (dev_info->status != PERF_TASK_DONE) {
		pr_err("[%s] Perf sampling, please wait for it finished!\n",
		       __func__);
		goto err_end;
	}

	aclk_perf_stat = perfstat_find_clk(dev_info, "aclk_perf_stat");
	if (!aclk_perf_stat) {
		pr_err("[%s] cannot find aclk_perf_stat\n", __func__);
		goto err_end;
	}

	if (!g_fpga_flag) { /* When fpga mode, no need to set aclk_perf_stat to 228MHz */
		event_var.ret = clk_set_rate(aclk_perf_stat, suggest_aclk);
		if (event_var.ret)  {
			pr_err("[%s] set aclk_perf_stat error\n", __func__);
			goto err_end;
		}
	}

	event_var.ret = perfstat_pwr_enable();
	if (event_var.ret < 0) {
		pr_err("[%s] enable perfstat failed", __func__);
		goto err_end;
	}

	event_var.ret = perfstat_do_check(event, dev_info);
	if (event_var.ret < 0)
		goto perf_supply_err;
#ifdef PERFSTAT_PORT_CAPACITY_128
	event_var.ret = perfstat_get_edata(event, &sample_period, &sample_port_low, &sample_port_high, &event_var.allocate_size);
#else
	event_var.ret = perfstat_get_edata(event, &sample_period, &sample_port_low, &event_var.allocate_size);
#endif
	if (event_var.ret < 0) {
		pr_err("[%s] initialize data fail\n", __func__);
		goto perf_supply_err;
	}
	pr_info("[%s] perfstat event_var.allocate_size:%d\n", __func__, event_var.allocate_size);
	pr_info("[%s] perfstat sample_period:%d\n", __func__, sample_period);
	pr_info("[%s] perfstat sample_port_low:%lx\n", __func__, sample_port_low);
#ifdef PERFSTAT_PORT_CAPACITY_128
	pr_info("[%s] perfstat sample_port_high:%lx\n", __func__, sample_port_high);
#endif
	pr_err("==============[dfx perf prepare +]================\n");

	dev_info->sample_per_sz =
	    (dev_info->sample_port_num * (int)dev_info->per_data_sz) + (int)sizeof(struct perf_sample_data_end);
	pr_err("[%s] dev_info->sample_port_num is 0x%x, dev_info->sample_per_sz is 0x%x\n", __func__,
	       dev_info->sample_port_num, dev_info->sample_per_sz);

	event_var.ret = perfstat_calculate_pages(dev_info, &event_var.page_n, sample_period, event_var.allocate_size); /* false alarm]:fortify */
	if (event_var.ret < 0) {
		pr_err("[%s] calculate pages fail.\n", __func__);
		goto err;
	}
	pr_info("[%s] event_var.page_n : %d\n", __func__, event_var.page_n);
	pages = kzalloc((event_var.page_n * sizeof(struct page *)), GFP_KERNEL | GFP_DMA);
	if (!pages) {
		pr_err("[%s] cannot allocate pages ptr buffer\n", __func__);
		goto err;
	}
	dev_info->pages = pages;
	list_cfg = kzalloc(((event_var.page_n + 1) * sizeof(perf_list_descriptor)),
			   GFP_KERNEL | GFP_DMA);
	pr_info("[%s]  kzalloc list_cfg is 0x%lx\n", __func__, (uintptr_t) list_cfg);
	if (!list_cfg) {
		pr_err("[%s] cannot allocate list_cfg buffer\n", __func__);
		goto err;
	}

	perfstat_fill_devinfo(dev_info, event_var.page_n, sample_port_low, list_cfg);
	pr_err("[%s] port low          : 0x%lx\n", __func__, sample_port_low);
#ifdef PERFSTAT_PORT_CAPACITY_128
	pr_err("[%s] port high         : 0x%lx\n", __func__, sample_port_high);
#endif
	pr_err("[%s] period            : %dus\n", __func__, sample_period);
	pr_err("[%s] event_var.page_n  : %d\n", __func__, event_var.page_n);

	event_var.ret = perfstat_initialize_list_cfg(dev_info, list_cfg, &event_var.list_head_phyaddr);
	if (event_var.ret < 0) {
		pr_err("[%s] init list config error\n", __func__);
		goto err;
	}
	pr_info("[%s] initialize list_cfg done\n", __func__);

	/* flush list_cfg into ddr */
	__dma_map_area((const void *)list_cfg,
		       ((event_var.page_n + 1) * sizeof(perf_list_descriptor)),
		       DMA_TO_DEVICE);

	/* map pages to a continuous virtual address */
	dev_info->virt_addr = vmap(pages, event_var.page_n, VM_MAP, PAGE_KERNEL); /*lint !e446*/
	if (!dev_info->virt_addr) {
		pr_err("[%s] vmap pages error\n", __func__);
		goto err;
	}

	(void)memset_s(dev_info->virt_addr, event_var.page_n * PAGE_SIZE, 0, event_var.page_n * PAGE_SIZE);
	__dma_map_area((const void *)dev_info->virt_addr, (event_var.page_n * PAGE_SIZE),
		       DMA_FROM_DEVICE);

	pr_info("[%s] vmap is %pK\n", __func__, dev_info->virt_addr);

#ifdef CONFIG_DFX_PERF_STAT_LAT_MON
	subsys_period = latency_monitor_period_get(sample_port_low, sample_port_high);
	if (subsys_period == PERF_ERROR) {
		pr_err("[%s]: subsys period=%d and config period=%d is different\n",
		       __func__, subsys_period, sample_period);
		if (sample_period <= SAMPLE_PERIOD_MIN)
			return PERF_ERROR;
	} else {
		sample_period = subsys_period;
	}
#endif
	perfstat_set_sample_period(sample_period);
#ifdef PERFSTAT_PORT_CAPACITY_128
	perfstat_set_sample_port(sample_port_low, sample_port_high);
#else
	perfstat_set_sample_port(sample_port_low);
#endif

#ifdef SOC_PERFSTAT_MAX_PORT_NUM_ADDR
	perfstat_set_clk_gate(PERF_CLK_GATE_EN);
	perfstat_set_port_index(sample_port_low, sample_port_high);
	perfstat_set_port_num(dev_info->sample_port_num);
#endif

	if (dev_info->samp_type == PERF_SAMPLE_HSCYCLE) {
		perfstat_set_sample_num(PERF_SAMPLE_NUM_CYCLE);
	} else {
#ifdef SOC_PERFSTAT_MAX_PORT_NUM_ADDR
		u32 hispeed_sample_num = DIV_ROUND_UP(event_var.allocate_size * PERF_TIME_1US, (u32)sample_period);
		perfstat_set_sample_num(hispeed_sample_num);
#else
		perfstat_set_sample_num(PERF_SAMPLE_NUM_CYCLE);
#endif
	}

	perfstat_set_addr_mode(PERF_LIST_ADDR_MODE);
	event_var.ret = perfstat_set_list_mode_addr(event_var.list_head_phyaddr);
	if (event_var.ret != PERF_OK) {
		pr_err("[%s] set list addr error\n", __func__);
		goto err;
	}

	perfstat_clear_interrupt(PERF_INT_ALL);
	perfstat_enable_interrupt(PERF_INT_ALL);

#ifndef SOC_PERFSTAT_MAX_PORT_NUM_ADDR
	if (dev_info->rs_enable_support)
		perfstat_enable_rs(dev_info);
#endif

	if (ics2_dp_sel == PERF_STAT_ICS2_PORT_SEL)
		perfstat_set_ics2_dp_sel(0);
	else if (ics2_dp_sel == PERF_STAT_DP_PORT_SEL)
		perfstat_set_ics2_dp_sel(1);

	perfstat_set_timeout_duration(SET_TIME_IN);
	dev_info->status = PERF_INIT_DONE;
	mutex_unlock(&perfstat_transaction_mutex);
	pr_err("==============[dfx perf prepare -]================\n");

	dev_info->event_bk = event;

	return 0;

err:
	if (dev_info->virt_addr) {
		vunmap(dev_info->virt_addr);
		dev_info->virt_addr = NULL;
	}
	if (dev_info->list) {
		kfree(dev_info->list);
		dev_info->list = NULL;
	}
	if (dev_info->pages) {
		for (event_var.i = dev_info->pages_n - 1; event_var.i >= 0; event_var.i--)
			if (dev_info->pages[event_var.i])
				__free_pages(dev_info->pages[event_var.i], 0);
		kfree(dev_info->pages);
		dev_info->pages = NULL;
	}
	dev_info->status = PERF_TASK_DONE;
perf_supply_err:
	perfstat_pwr_disable();
err_end:
	mutex_unlock(&perfstat_transaction_mutex);
	return -ENOENT;
}

u64 hisi_get_curuptime(void)
{
	u64 pcurtime;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0))
	struct timespec uptime;
	get_monotonic_boottime(&uptime);
#else
	struct timespec64 uptime;
	ktime_get_boottime_ts64(&uptime);
#endif
	pcurtime = uptime.tv_sec;
	pcurtime = pcurtime * NSECS_IN_SEC + uptime.tv_nsec;

	return pcurtime;
}

static int perfstat_add(struct perf_event *bp, int flags)
{
	struct perfstat_dev_info *dev_info = perfstat_info;

	if (bp == NULL) {
		pr_err("[%s] perf_event is NULL\n", __func__);
		return -ENOENT;
	}

	if (dev_info == NULL) {
		pr_err("[%s] perfstat_info is NULL\n", __func__);
		return -ENOENT;
	}
	mutex_lock(&perfstat_transaction_mutex);

	if (dev_info->status != PERF_INIT_DONE) {
		pr_err("[%s] perf is sampling cannot add event\n", __func__);
		goto err_end;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
	__pm_stay_awake(&dev_info->wakelock);
#else
	__pm_stay_awake(dev_info->wakelock);
#endif

	g_sample_time = (unsigned long)hisi_get_curuptime();
	pr_err("[%s] sample time is %lu\n", __func__, g_sample_time);
	pr_info("[%s][dfx perf start sample...]\n", __func__);
	perfstat_trigger_sample_start();
	dev_info->status = PERF_ADD_DONE;
	mutex_unlock(&perfstat_transaction_mutex);
	return 0;
err_end:

	mutex_unlock(&perfstat_transaction_mutex);
	return -ENOENT;
}

static void perfstat_del(struct perf_event *bp, int flags)
{
	struct perfstat_dev_info *dev_info = perfstat_info;

	if (bp == NULL) {
		pr_err("[%s] perf_event is NULL\n", __func__);
		return;
	}

	if (dev_info == NULL) {
		pr_err("[%s] perfstat_info is NULL\n", __func__);
		return;
	}

	mutex_lock(&perfstat_transaction_mutex);
	if (dev_info->status != PERF_ADD_DONE) {
		pr_err("[%s] perf is sampling cannot del event\n", __func__);
		goto err_end;
	}

	if ((dev_info->samp_type == PERF_SAMPLE_HIGHSPEED) && (!perf_file_saved)
	    && !atomic_read(&is_recording)) {
		pr_err("%s: save perf data...\n", __func__);
		schedule_delayed_work(&dev_info->hs_record_data_work, 0);
	}

	pr_info("[%s][dfx perf stop sample]\n", __func__);
	perfstat_trigger_sample_stop();

	pr_err("==============[dfx perf finish +]================\n");
	perfstat_debug_info_show();
	pr_err("==============[dfx perf finish -]================\n");

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
	__pm_relax(&dev_info->wakelock);
#else
	__pm_relax(dev_info->wakelock);
#endif
	dev_info->status = PERF_DEL_DONE;
err_end:
	mutex_unlock(&perfstat_transaction_mutex);
}

static void perfstat_start(struct perf_event *bp, int flags)
{
	bp->hw.state = 0;
}

static void perfstat_stop(struct perf_event *bp, int flags)
{
	bp->hw.state = PERF_HES_STOPPED;
}

static void perfstat_read(struct perf_event *bp)
{
	return;
}

static int perfstat_event_idx(struct perf_event *bp)
{
	return 0;
}

static struct pmu perf_stat_pmu = {
	.task_ctx_nr = perf_sw_context,
	.event_init = perfstat_event_init,
	.add = perfstat_add,
	.del = perfstat_del,
	.start = perfstat_start,
	.stop = perfstat_stop,
	.read = perfstat_read,
	.event_idx = perfstat_event_idx,
};

#ifdef CONFIG_DFX_PERF_STAT_LAT_MON
static int init_subsys_info(struct device *dev)
{
	int i, j, ret, len;
	u32 data;
	char *data_str = NULL;

	/* get g_unregister_def_port from dts */
	ret = of_property_read_u32(dev->of_node, "unregister_def_port", &g_unregister_def_port);
	if (ret < 0)
		pr_err("[%s] cannot get unregister_def_port %d\n", __func__, ret);

	/* get length of g_subsys_info from dts */
	if (!of_find_property(dev->of_node, "subsys_mon_rw", &len))
		return PERF_ERROR;

	if ((len % (sizeof(u32) * 2)) != 0 || len == 0)
		return PERF_ERROR;

	len /= sizeof(u32) * 2;

	/* get g_subsys_info from dts */
	g_subsys_info = devm_kzalloc(dev, len * sizeof(struct perfstat_subsystem_info),
				     GFP_KERNEL);
	if (!g_subsys_info) {
		pr_err("[%s] perfstat_subsystem_info kzalloc fail.\n", __func__);
		return PERF_ERROR;
	}

	for (i = 0, j = 0; i < len; i++, j += 2) {
		ret = of_property_read_u32_index(dev->of_node, "subsys_mon_rw", j,
						 &data);
		if (ret < 0)
			return PERF_ERROR;
		g_subsys_info[i].read = data == 0 ? -1 : data;

		ret = of_property_read_u32_index(dev->of_node, "subsys_mon_rw", j + 1,
						 &data);
		if (ret < 0)
			return PERF_ERROR;
		g_subsys_info[i].write = data == 0 ? -1 : data;

		ret = of_property_read_string_index(dev->of_node, "subsys_mon_name", i,
						    &data_str);
		if (ret < 0)
			return PERF_ERROR;
		strcpy_s(g_subsys_info[i].name, SUBSYS_NAME_LEN, data_str);

		pr_info("[%s] read:%d write:%d, name:%s\n", __func__, g_subsys_info[i].read,
			g_subsys_info[i].write, g_subsys_info[i].name);
	}
	g_subsys_len = len;
	pr_info("[%s] g_subsys_len:%d\n", __func__, g_subsys_len);
	return PERF_OK;
}
#endif

static int fake_of_get_perfstat_attribute(const struct device *dev,
					  struct perfstat_dev_info *dev_info)
{
	int ret;
	struct device_node *np = dev->of_node;
#ifdef SOC_PERFSTAT_MAX_PORT_NUM_ADDR
	unsigned int vldmsk_of_sprt_h128 = 0;
#endif
	unsigned int vldmsk_of_sprt_h96 = 0;
	unsigned int vldmsk_of_sprt_h64 = 0;
	unsigned int vldmsk_of_sprt_l32 = 0;

	ret = of_property_read_u32(np, "perf-event-id",
				   &dev_info->event_id);
	if (ret < 0) {
		pr_err("[%s] cannot get perf-event-id\n", __func__);
		return -ENOENT;
	}

	ret = of_property_read_u32(np, "per-data-size",
				   &dev_info->per_data_sz);
	if (ret < 0) {
		pr_err("[%s] cannot get per-data-size\n", __func__);
		return -ENOENT;
	}

	ret = of_property_read_u32(np, "vldmsk-of-sprt", &vldmsk_of_sprt_l32);
	if (ret < 0) {
		pr_err("[%s] cannot get vldmsk-of-sprt %d\n", __func__, ret);
		return -ENOENT;
	}

	ret = of_property_read_u32(np, "vldmsk-of-sprt-h64", &vldmsk_of_sprt_h64);
	if (ret < 0)
		pr_err("[%s] cannot get vldmsk-of-sprt-h64 %d\n", __func__, ret);

	ret = of_property_read_u32(np, "vldmsk-of-sprt-h96", &vldmsk_of_sprt_h96);
	if (ret < 0)
		pr_err("[%s] cannot get vldmsk-of-sprt-h96 %d\n", __func__, ret);

#ifdef SOC_PERFSTAT_MAX_PORT_NUM_ADDR
	ret = of_property_read_u32(np, "vldmsk-of-sprt-h128", &vldmsk_of_sprt_h128);
	if (ret < 0)
		pr_err("[%s] cannot get vldmsk-of-sprt-h128 %d\n", __func__, ret);
#endif

	dev_info->vldmsk_of_sprt = ((u64)vldmsk_of_sprt_h64 << OFFSET_32) + vldmsk_of_sprt_l32;
	pr_err("[%s] get vldmsk-of-sprt %llx\n", __func__, dev_info->vldmsk_of_sprt);

#ifdef PERFSTAT_PORT_CAPACITY_128
#ifdef SOC_PERFSTAT_MAX_PORT_NUM_ADDR
	dev_info->vldmsk_of_sprt_h = ((u64)vldmsk_of_sprt_h128 << OFFSET_32) + vldmsk_of_sprt_h96;
#else
	dev_info->vldmsk_of_sprt_h = vldmsk_of_sprt_h96;
#endif
	pr_err("[%s] get vldmsk-of-sprt-h %llx\n", __func__, dev_info->vldmsk_of_sprt_h);
#endif

	ret = of_property_read_u32(np, "suggest_clk", &suggest_clk);
	if (ret < 0)
		pr_err("[%s] cannot get suggest_clk\n", __func__);

	ret = of_property_read_u32(np, "suggest_aclk", &suggest_aclk);
	if (ret < 0)
		pr_err("[%s] cannot get suggest_aclk\n", __func__);

#ifndef SOC_PERFSTAT_MAX_PORT_NUM_ADDR
	ret = of_property_read_u32(np, "rs_enable_support",
				   &dev_info->rs_enable_support);
	if ((ret < 0) || (dev_info->rs_enable_support > RSDATA_MAX / GET_ENABLE_NUM)) {
		dev_info->rs_enable_support = 0;
		pr_err("[%s] cannot get rs_enable_support\n", __func__);
	}

	if (dev_info->rs_enable_support > 0) {
		/* get rs_support_port regester */
		ret = of_property_read_u32_array(np, "rs_support_port", dev_info->rs_data, (size_t)dev_info->rs_enable_support * GET_ENABLE_NUM);
		if (ret < 0) {
			pr_err("[%s] enable perfstat rs fails, value is=%d\n", __func__, ret);
			dev_info->rs_enable_support = 0;
		}
	}
#endif
#ifdef CONFIG_DFX_PERF_STAT_LAT_MON
	ret = init_subsys_info(dev);
	if (ret < 0) {
		pr_err("[%s] cannot init subsystem info.\n", __func__);
		return -ENXIO;
	}
#endif
	return 0;
}

#ifndef SOC_PERFSTAT_MAX_PORT_NUM_ADDR
static int perfstat_set_qos(const struct device_node *np)
{
	int ret;
	static void __iomem *dma_noc_service_target_addr;
	unsigned int qos_support = 0;

	ret = of_property_read_u32(np, "qos_support",
				   &qos_support);
	if ((ret < 0) || (!qos_support))
		return -ENOENT;

	pr_info("[%s] qos_support\n", __func__);

	dma_noc_service_target_addr = ioremap(SOC_ACPU_DMA_NOC_Service_Target_BASE_ADDR, 0x1000);
	if (!dma_noc_service_target_addr) {
		pr_err("[%s] DMA_NOC_Service ioremap fail\n", __func__);
		return -ENOMEM;
	}

	writew(RF_STAT_QOS_PRIORI_VALUE, dma_noc_service_target_addr + RF_STAT_QOS_PRIORI);
	writew(PERF_STAT_QOS_MODE_VALUE, dma_noc_service_target_addr + PERF_STAT_QOS_MODE);

	return 0;
}
#endif

static int perfstat_get_fpga_flag(unsigned int *fpga)
{
	struct device_node *np = NULL;
	unsigned int fpga_flag;
	int ret;

	np = of_find_compatible_node(NULL, NULL, "hisilicon,dpufb");
	if (!np) {
		pr_err("NOT FOUND device node 'fb'!\n");
		return -ENXIO;
	}
	ret = of_property_read_u32(np, "fpga_flag", &fpga_flag);
	if (ret) {
		pr_err("failed to get fpga_flag resource\n");
		return -ENXIO;
	}
	*fpga = fpga_flag;
	return 0;
}

/*
 * Function: perfstat_probe
 * Description: initialize perfstat driver, it's called when this platform_device
 * and this platform_driver match.
 * Input: platform_device: an abstraction of perfstat device
 * Output: NA
 * Return: 0: perfstat_probe successfully
 * <0:	perfstat_probe failed
 * Other: NA
 */
static int perfstat_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
#ifndef SOC_PERFSTAT_MAX_PORT_NUM_ADDR
	struct device_node *np = dev->of_node;
#endif
	struct perfstat_dev_info *dev_info = NULL;
	const struct of_device_id *match = NULL;
	int ret;

	pr_err("================[dfx perf probe s]================\n");

	/* Get fpga flag first */
	ret = perfstat_get_fpga_flag(&g_fpga_flag);
	if (ret < 0)
		pr_err("[%s]: perfstat get fpga flag fail\n", __func__);

	/* to check which type of regulator this is */
	match = of_match_device(of_perfstat_match_tbl, dev);
	if (match == NULL) {
		pr_err("[%s]: mismatch of perf_stat driver\n\r", __func__);
		return -ENODEV;
	}

	dev_info =
	    devm_kzalloc(dev, sizeof(struct perfstat_dev_info), GFP_KERNEL);
	if (!dev_info) {
		pr_err("[%s] cannot alloc perfstat desc\n", __func__);
		return -ENOMEM;
	}
	perfstat_info = dev_info;

	dev_info->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!dev_info->res) {
		pr_err("[%s] platform_get_resource err\n", __func__);
		return -ENOENT;
	}

	if (!devm_request_mem_region(dev, dev_info->res->start,
				     resource_size(dev_info->res),
				     pdev->name)) {
		pr_err("[%s] cannot claim register memory\n", __func__);
		return -ENOMEM;
	}

	pericrg_base = ioremap(SOC_ACPU_PERI_CRG_BASE_ADDR, PERICRG_SIZE); /*lint !e446*/
	if (!pericrg_base) {
		pr_err("[%s] pericrg_base ioremap fail\n", __func__);
		return -ENOMEM;
	}

	pctrl_base = ioremap(SOC_ACPU_PCTRL_BASE_ADDR, SZ_4K); /*lint !e446*/
	if (!pctrl_base) {
		pr_err("[%s] pctrl_base ioremap fail\n", __func__);
		return -ENOMEM;
	}

	perfstat_base = ioremap(dev_info->res->start, resource_size(dev_info->res));  /*lint !e446*/
	if (!perfstat_base) {
		pr_err("[%s] base address ioremap fail\n", __func__);
		return -ENOMEM;
	}

	dev_info->irq = platform_get_irq(pdev, 0);
	if (dev_info->irq < 0) {
		pr_err("[%s] platform_get_irq err\n", __func__);
		return -ENXIO;
	}

	ret =
	    request_threaded_irq(dev_info->irq, perfstat_interrupt_handler,
				 NULL, IRQF_NO_SUSPEND, "pertstat-irq",
				 dev_info);
	if (ret < 0) {
		pr_err("[%s] requset irq error\n", __func__);
		return -ENODEV;
	}

	suggest_clk = PERF_SUGGEST_CLK;
	suggest_aclk = PERF_SUGGEST_ACLK;
	ret = fake_of_get_perfstat_attribute(dev, dev_info);
	if (ret < 0) {
		pr_err("[%s] get dts fail\n", __func__);
		return -ENOENT;
	}

	ret = perfstat_clk_init(pdev, g_fpga_flag);
	if (ret < 0) {
		pr_err("[%s] perfstat clk init failed\n", __func__);
		return -ENODEV;
	}

#ifndef SOC_PERFSTAT_MAX_PORT_NUM_ADDR
	ret = perfstat_set_qos(np);
	if (ret < 0)
		pr_err("[%s] perfstat_set_qos not support!continue probe\n", __func__);
#endif

	ret = perf_pmu_register(&perf_stat_pmu, "HiPERFSTAT", PERF_TYPE_PERF_STAT);
	if (ret < 0) {
		pr_err("[%s] register perf pmu fail\n", __func__);
		return -ENODEV;
	}

	INIT_DELAYED_WORK(&dev_info->hs_record_data_work,
			  perfstat_record_sample_data);
	init_completion(&perf_comp);
	mutex_init(&perfstat_transaction_mutex);
	spin_lock_init(&dev_info->synclock);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
	wakeup_source_init(&dev_info->wakelock,
		       "perf-stat-wakelock");
#else
	dev_info->wakelock = wakeup_source_register(NULL, "perf-stat-wakelock");
#endif
	dev_info->status = PERF_TASK_DONE;
	is_probe_comp = true;
	pr_err("================[dfx perf probe e]================\n");

	return 0;
}

static int perfstat_remove(struct platform_device *pdev)
{
	return 0;
}

/*
 * Function: create_perf_event
 * Description: a function to create perf_event
 * Input: int sample_period: sample period
 * unsigned long sample_port: sample port
 * int sample_time: sample time(mode 1 means monitoring
 * sample_time seconds, mode 2 means monitoring sample_time Mb)
 * int mode: monitor mode, 1 is high speed mode, 2 is cycle mode
 * Output: NA
 * Return: perf_event: NULL means creating perf_event failed
 * Other: NA
 */
#ifdef PERFSTAT_PORT_CAPACITY_128
static struct perf_event *create_perf_event(int sample_period,
				unsigned long sample_port_low, unsigned long sample_port_high,
				int sample_time, int mode)
#else
static struct perf_event *create_perf_event(int sample_period,
				unsigned long sample_port_low, int sample_time, int mode)
#endif
{
	struct perf_event *event = NULL;

	if (sample_period <= 0) {
		pr_err("[%s] sample_period is negative: %d\n", __func__,
		       sample_period);
		goto err;
	}
	if (sample_time < 0 || sample_time >= 1 << SAMPLE_TIME_RANGE) {
		pr_err("[%s] sample_time is out of range: %d\n", __func__,
		       sample_time);
		goto err;
	}
	if (mode != PERF_SAMPLE_HSCYCLE && mode != PERF_SAMPLE_HIGHSPEED) {
		pr_err("[%s] use mode error. mode=%d\n", __func__, mode);
		goto err;
	}
	event = kzalloc(sizeof(struct perf_event), GFP_KERNEL);
	if (event == NULL)
		goto err;

	event->attr.type = PERF_TYPE_PERF_STAT;
	event->attr.sample_period = sample_period;
#ifdef PERFSTAT_PORT_CAPACITY_128
	event->attr.config =
		(u64)(sample_port_low & PERF_PORT_L_MASK) + ((u64)(u32)sample_time << OFFSET_32) +
		((u64)(u32)(MODE_OFFSET + mode) << OFFSET_48);
	pr_err("config is %llx\n", event->attr.config);
	pr_err("sample_port_low is 0x%lx\n", sample_port_low);

	/* get the H_32 bit */
	event->attr.config1 = (sample_port_low & PERF_PORT_H_MASK) >> OFFSET_32;
	pr_err("config1.0 is 0x%llx\n", event->attr.config1);
	event->attr.config1 = ((u64)(sample_port_high & PERF_PORT_L_MASK)) << OFFSET_32;
	pr_err("config1.1 is 0x%llx\n", event->attr.config1);
	event->attr.config1 = (((u64)(sample_port_high & PERF_PORT_L_MASK)) << OFFSET_32) + ((sample_port_low & PERF_PORT_H_MASK) >> OFFSET_32);
	pr_err("config1 is 0x%llx\n", event->attr.config1);
	pr_err("sample_port_high is 0x%lx\n", sample_port_high);
	event->attr.config2 = (sample_port_high & PERF_PORT_H_MASK) >> OFFSET_32;
	pr_err("config2 is 0x%llx\n", event->attr.config2);
#else
	event->attr.config =
		(u64) (sample_port_low & PERF_PORT_L_MASK) + ((u64)(u32)sample_time << OFFSET_32) +
		((u64)(u32)(MODE_OFFSET + mode) << OFFSET_48);
	pr_err("config is %llx\n", event->attr.config);
	pr_err("sample_port_low is %lx\n", sample_port_low);
	/* get the H_32 bit */
	event->attr.config1 = (sample_port_low & PERF_PORT_H_MASK) >> OFFSET_32;
#endif
	return event;

err:
	return NULL;
}

/*
 * Function: perfstat_start_sampling
 * Description: an interface for other module use, which is used to start
 * Input: int sample_period: sample period
 * unsigned long sample_port: sample port
 * int sample_time: sample time(mode 1 means monitoring
 * sample_time seconds, mode 2 means monitoring sample_time Mb)
 * int mode: monitor mode, 1 is high speed mode, 2 is cycle mode
 * this driver. Moreover, it's a couple with perfstat_stop_sampling
 * Output: NA
 * Return: PERF_ERROR: start failed
 * PERF_OK: start successfully
 * Other: NA
 */
#ifdef PERFSTAT_PORT_CAPACITY_128
int perfstat_start_sampling(int sample_period, unsigned long sample_port_low, unsigned long sample_port_high,
				int sample_time, uintptr_t mode)
#else
int perfstat_start_sampling(int sample_period, unsigned long sample_port_low, int sample_time, uintptr_t mode)
#endif
{
	int ret;
	struct perfstat_dev_info *dev_info = perfstat_info;

	pr_err("[%s] perfstat_start_sampling begin\n", __func__);
	ret = PERF_ERROR;
	if (current_event != NULL) {
		pr_err("[%s] Perf is sampling, please wait for it finished\n", __func__);
		return PERF_ERROR;
	}
	if (is_probe_comp == false || dev_info == NULL) {
		pr_err("[%s] perfstat is not intialized", __func__);
		goto out_err;
	}

	if (sample_period <= 0) {
		pr_err("[%s] sample_period is negative: %d\n", __func__,
		       sample_period);
		goto out_err;
	}
	/* use platform setting to judge */
	if (sample_port_low > dev_info->vldmsk_of_sprt) {
		pr_err("[%s] sample_port_low is out of range: 0x%lx\n", __func__,
		       sample_port_low);
		goto out_err;
	}
	/* check max port num */
#ifdef PERFSTAT_PORT_CAPACITY_128
	dev_info->sample_port_num = bitmap_weight((const unsigned long *)&sample_port_low, BIT_SIZE * sizeof(sample_port_low));
	dev_info->sample_port_num += bitmap_weight((const unsigned long *)&sample_port_high, BIT_SIZE * sizeof(sample_port_high));
	if (dev_info->sample_port_num > MAX_PORT_NUM) {
		pr_err("[%s] sample_port_num is out of range: %d\n", __func__, MAX_PORT_NUM);
		goto out_err;
	}
#else
	dev_info->sample_port_num = bitmap_weight((const unsigned long *)&sample_port_low, BIT_SIZE * sizeof(sample_port_low));
#endif

	if (sample_time < 0 || sample_time >= 1 << SAMPLE_TIME_RANGE) {
		pr_err("[%s] sample_time is out of range: %d\n", __func__,
		       sample_time);
		goto out_err;
	}

	/* atlanta cs ics2 and dp discriminated by mode bit16~31 */
	/* ics2_dp_sel: 1-ics2; 2-dp */
	ics2_dp_sel = (mode >> OFFSET_16) & 0xff;
	mode &= 0xffff;

	if (mode != PERF_SAMPLE_HSCYCLE && mode != PERF_SAMPLE_HIGHSPEED) {
		pr_err("[%s] use mode error. mode=%lu\n", __func__, mode);
		goto out_err;
	}

#ifdef PERFSTAT_PORT_CAPACITY_128
	current_event =
	    create_perf_event(sample_period, sample_port_low, sample_port_high, sample_time, mode);
#else
	current_event =
	    create_perf_event(sample_period, sample_port_low, sample_time, mode);
#endif
	if (current_event == NULL) {
		pr_err("[%s] creating perf_event fails\n", __func__);
		goto out_err;
	}

	ret = perfstat_event_init(current_event);
	if (ret < 0) {
		pr_err("[%s] perfstat init fails\n", __func__);
		ret = -ENOENT;
		goto event_err;
	}

	ret = perfstat_add(current_event, 0);

	perfstat_reg_show();

	pr_err("[%s] perfstat_start_sampling end\n", __func__);
	return ret;

event_err:
	if (current_event != NULL) {
		kfree(current_event);
		current_event = NULL;
	}
out_err:
	pr_err("[%s] perfstat_start_sampling fails\n", __func__);
	return ret;
}
EXPORT_SYMBOL_GPL(perfstat_start_sampling); /*lint !e580*/

/*
 * Function: perfstat_stop_sampling
 * Description: an interface for other module use, which is used to stop
 * this driver. Moreover, it's a couple with perfstat_start_sampling
 * and is allowed to use only after perfstat_start_sampling being called
 * Input: NA
 * Output: NA
 * Return: PERF_ERROR: stop failed
 * PERF_OK: stop successfully
 * Other: NA
 */
int perfstat_stop_sampling(void)
{
#ifdef CONFIG_DFX_PERF_STAT_LAT_MON
	int i = 0;
#endif
	struct perfstat_dev_info *dev_info = perfstat_info;

	pr_err("[%s] perfstat_stop_sampling begin\n", __func__);
	if (is_probe_comp == false || dev_info == NULL) {
		pr_err("[%s] perfstat is not intialized", __func__);
		return PERF_ERROR;
	}
	if (dev_info->status == PERF_TASK_DONE) {
		pr_err("[%s] perfstat is not running\n", __func__);
		return PERF_ERROR;
	}
	if (current_event == NULL) {
		pr_err("[%s] perf_event is not intialized\n", __func__);
		return PERF_ERROR;
	}
	perfstat_del(current_event, 0);
	perfstat_event_destroy(current_event);

	kfree(current_event);
	current_event = NULL;

#ifdef CONFIG_DFX_PERF_STAT_LAT_MON
	for (; i < g_subsys_len; i++)
		g_subsys_info[i].is_valid = false;
#endif

	pr_err("[%s] perfstat_stop_sampling end\n", __func__);
	return PERF_OK;
}
EXPORT_SYMBOL_GPL(perfstat_stop_sampling); /*lint !e580*/

static struct platform_driver perfstat_driver = {
	.driver = {
		   .name = "HiPERFSTAT",
		   .owner = THIS_MODULE,
		   .of_match_table = of_perfstat_match_tbl,
		   },
	.probe = perfstat_probe,
	.remove = perfstat_remove,
};

static __init int perfstat_init(void)
{
	int ret = PERF_OK;

	ret = platform_driver_register(&perfstat_driver);
	if (ret) {
		pr_err("[%s] platform_driver_register failed %d\n",
		       __func__, ret);
	}

	return ret;
}

static void __exit perfstat_exit(void)
{
	platform_driver_unregister(&perfstat_driver);
}

fs_initcall(perfstat_init);
module_exit(perfstat_exit);
