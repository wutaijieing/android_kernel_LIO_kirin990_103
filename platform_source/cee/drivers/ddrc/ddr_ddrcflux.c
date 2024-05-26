/*
 * ddr_ddrcflux.c
 *
 * ddrc flux
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
#include <linux/version.h>
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
#include <linux/err.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/bitops.h>
#include <linux/dma-mapping.h>
#include <linux/sched/clock.h>
#include <soc_dmss_interface.h>
#include <soc_acpu_baseaddr_interface.h>
#include <soc_ddrc_dmc_interface.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/arm-smccc.h>
#include <linux/time64.h>
#include <global_ddr_map.h>
#include <platform_include/display/linux/dpu_drmdriver.h>
#include "hisi_ddr_ddrcflux_kirin.h"
#include "hisi_ddr_autofsgt_proxy_kernel.h"
#include "securec.h"
#ifdef DDRC_QOSB_PLATFORM
#include <soc_ddrc_qosb_interface.h>
#ifdef DDRC_EXMBIST_PCLK_GATE
#include <soc_exmbist_interface.h>
#endif
#endif

#define FREQ_INDEX_SHIFT		8
#define CLK_MODULUS		10
#define RATE_SCALE		1000000
#define FLUX_TIMER_CLK		4800000
#define DDRFLUX_INFO_NUM		12
#define INTERVAL_CHECK		10000
#define SUM_TIME_CHECK		500000
#define REG_BASE_DMSS_LEN		0x20
#define DDRFLUX_REG_DMSS_LEN		8
#define REG_QOSBUF_LEN		1
#define REG_EXMBIST_LEN		1
#define REG_DMC_LEN		1
#define DMSS_ENABLE_MSK		BIT(0)
#define QOSBUF_ENABLE_MSK		BIT(1)
#define DMC_ENABLE_MSK		BIT(2)
#define DMCCMD_ENABLE_MSK		BIT(3)
#define DMCDATA_ENABLE_MSK		BIT(4)
#define DMCLATENCY_ENABLE_MSK		BIT(5)
#define FLUX_EN_DMA_PA_NUM		2
#define BEGIN_TIME_DIV		1000000000
#define BEGIN_TIME_MODULUS		1000
#define RTC_YEAR_BASE		1900
#define DDRFLUX_INIT_NUM		2
#define DMSS_GLB_STAT_CTRL_MSK		0xFFF
#define SMEM_SPOT		2
#define ACPU_TIMER5_BASE_LEN		1
#define ACPU_SCTRL_BASE_LEN		1
#define FLUX_DATA_FILE_MOD		0444

#ifdef CONFIG_DFX_DEBUG_FS
#define PRIV_AUTH		(S_IRUSR | S_IWUSR | S_IRGRP)
#endif

#ifndef min
#define min(a, b)	((a) < (b) ? (a) : (b))
#endif

struct ddrc_flux_device *g_dfdev;
static struct ddrflux_data *g_ddrc_datas;
static enum hisi_plat g_hisi_plat_ddr = PLAT_MAX;
static u32 g_count;
static u32 g_usr[DDRC_FLUX_MAX];
static u32 g_flag;
static struct dentry *g_ddrc_flux_dir;
static struct dentry *g_flux_range;
int g_stop;
static unsigned char g_dmc_start;

static const struct ddrflux_pull *g_ddrflux_lookups;
static u32 g_ddrflux_list_len;

static u32 g_max_dmss_asi_num;
static u32 g_max_dmc_num;
static u32 g_max_qos_num;

static const struct ddrflux_pull g_ddrflux_lookups_info[] = {
	DDR_FLUX_DMSS_ASI_DFX_OSTD
	DDR_FLUX_DMSS_ASI_STAT_FLUX_WR
	DDR_FLUX_DMSS_ASI_STAT_FLUX_RD
	DDR_QOSB

	DDR_FLUX_DMC
};

#define MAX_FLUX_REG_NUM (QOSB_REG_NUM_PER_QOSB * g_max_qos_num + \
			  DMC_REG_NUM_PER_DMC * g_max_dmc_num + \
			  DMSS_REG_NUM_PER_ASI * g_max_dmss_asi_num)

static void dmss_flux_enable_ctrl(int en);
static void dmc_flux_enable_ctrl(int en);
#ifdef DDRC_QOSB_PLATFORM
static void qosbuf_flux_enable_ctrl(int en);
#endif

static void hisi_bw_timer_clk_disable(void)
{
	clk_disable_unprepare(g_dfdev->ddrc_flux_timer->clk);
}

void ddrc_flux_data_pull(void)
{
	u32 i, addr;
	u32 list_asi_num, slice_length;
	u32 j = 0;
	void __iomem *base = NULL;
	struct ddrflux_data *ddrc_data = NULL;

	if (g_ddrc_datas == NULL) {
		ddrflux_err("g_ddrc_datas=NULL");
		return;
	}

	ddrc_data = (struct ddrflux_data *)((u64)g_ddrc_datas + g_count *
					    (sizeof(struct ddrflux_data) +
					     sizeof(u32) * MAX_FLUX_REG_NUM));

	ddrc_data->ddrc_time = sched_clock();

	slice_length = min((unsigned long)(SLICE_LEN - 1),
			   strlen(g_dfdev->slice.name));
	(void)strncpy_s(ddrc_data->slice, SLICE_LEN, g_dfdev->slice.name,
			slice_length);
	ddrc_data->slice[slice_length] = '\0';

	list_asi_num = (g_ddrflux_list_len - g_max_dmc_num *
			DMC_REG_NUM_PER_DMC - g_max_qos_num *
			QOSB_REG_NUM_PER_QOSB) / DMSS_REG_NUM_PER_ASI;

	for (i = 0; i < g_ddrflux_list_len; i++) {
		if ((i >= g_max_dmss_asi_num &&
		     i < (list_asi_num * ASI_REG_GROUP1)) ||
		    (i >= (list_asi_num * ASI_REG_GROUP1 + g_max_dmss_asi_num) &&
		     i < (list_asi_num * ASI_REG_GROUP2)) ||
		    (i >= (list_asi_num * ASI_REG_GROUP2 + g_max_dmss_asi_num) &&
		     i < (list_asi_num * ASI_REG_GROUP3))) {
			continue;
		}
		addr = g_ddrflux_lookups[i].addr;

		switch (addr) {
		case DMSS:
			if (g_usr[DMSS_ENABLE] != 0)
				base = g_dfdev->dmss_base;
			break;
#ifdef DDRC_QOSB_PLATFORM
		case QOSB0:
		case QOSB1:
			if (g_usr[QOSBUF_ENABLE] != 0)
				base = g_dfdev->qosb_base[addr - QOSB0];
			break;
#endif
		case DMC00:
		case DMC01:
		case DMC10:
		case DMC11:
			if (g_usr[DMC_ENABLE] != 0)
				base = g_dfdev->dmc_base[addr - DMC00];
			break;
		default:
			ddrflux_err("ddr mode error!");
		}

		if (base != NULL) {
			ddrc_data->ddrflux_data[j] =
				readl(base + g_ddrflux_lookups[i].offset);
			base = NULL;
			j++;
		} else {
			ddrc_data->ddrflux_data[j] = 0;
			j++;
		}
	}
}

static irqreturn_t hisi_bw_timer_interrupt(int irq, void *dev_id)
{
	u32 freq_index, slice_length;
	unsigned long flags;
	struct ddrflux_data *ddrc_data = NULL;
	struct arm_smccc_res res = {0};

	if (hisi_bw_timer_interrput_status(g_dfdev) != 0) {
		/* clear the interrupt */
		hisi_bw_timer_clr_interrput(g_dfdev);
		ddrflux_dbg("[0]cnt =%d", g_count);

		spin_lock_irqsave(&g_dfdev->ddrc_flux_timer->lock, flags);
		if (g_ddrc_datas == NULL) {
			spin_unlock_irqrestore(&g_dfdev->ddrc_flux_timer->lock,
					       flags);
			ddrflux_err("g_ddrc_datas=NULL");
			return IRQ_HANDLED;
		}

		freq_index = readl(g_dfdev->sctrl + SCBAKDATA4);
		freq_index &= MASK_DDR;
		freq_index = freq_index >> FREQ_INDEX_SHIFT;

		ddrc_data = (struct ddrflux_data *)((u64)g_ddrc_datas + g_count *
						    (sizeof(struct ddrflux_data) +
						     sizeof(u32) * MAX_FLUX_REG_NUM));
		ddrc_data->ddr_tclk_ns = freq_index;
		ddrc_data->ddrc_time = sched_clock();

		ddrc_flux_data_pull();

		spin_unlock_irqrestore(&g_dfdev->ddrc_flux_timer->lock, flags);

		if (g_count < (g_usr[SUM_TIME] / g_usr[INTERVAL]) - 1) {
			g_count++;
		} else if (g_count == (g_usr[SUM_TIME] / g_usr[INTERVAL] - 1)) {
			ddrflux_dbg("[1]cnt =%d", g_count);
			hisi_bw_timer_disable(g_dfdev, &g_stop);
			ddrflux_dbg("[2]cnt =%d", g_count);
		}
	}

	return IRQ_HANDLED;
}

static int hisi_bw_timer_clk_enable(void)
{
	int ret = clk_prepare_enable(g_dfdev->ddrc_flux_timer->clk);
	if (ret != 0)
		ddrflux_err("clk prepare enable failed!");

	return ret;
}

/*
 * func: used for wakeup on timer for S/R
 * decs: set timer2 count value = (seconds*1000+milliseconds)*32.768 ms
 * mode is used for user to set timer work in periodic or oneshot mode
 * mode: 0 for periodic
 *       1 for oneshot
 */
static void hisi_pm_bw_on_timer(u32 useconds)
{
	u32 set_time, rate, max_timing;

	if (useconds == 0) {
		ddrflux_err("hisi_pm_bw_on_timer: input time error!");
		return;
	}

	/* time to be useconds format */
	rate = clk_get_rate(g_dfdev->ddrc_flux_timer->clk);
	if (rate == 0) {
		ddrflux_err("hisi_pm_bw_on_timer: clk_get_rate error!");
		return;
	}

	set_time = useconds * (rate * CLK_MODULUS / RATE_SCALE) / CLK_MODULUS;
	ddrflux_dbg("us=%d,rate=%d,settime=%d", useconds, rate, set_time);
	max_timing = (MAX_DATA_OF_32BIT / rate) * RATE_SCALE;
	ddrflux_dbg("set_time=%d", set_time);

	if (set_time > max_timing) {
		ddrflux_err("hisi_pm_bw_on_timer: input timing overflow!");
		return;
	}

	/* enable clk */
	if (hisi_bw_timer_clk_enable() != 0)
		return;

	/*
	 * add for the case ICS4.0 system changed the timer clk to 6.5MHz
	 * here changed back to 4.8MHz.
	 */
	hisi_bw_timer_init_config(g_dfdev);
	hisi_bw_timer_set_time(set_time, g_dfdev, &g_stop);
}

int hisi_bw_timer_clk_is_ready(void)
{
	if (g_dfdev->ddrc_flux_timer->clk == NULL)
		return 1;

	return 0;
}

static int hisi_bw_timer_init(u32 irq_per_cpu)
{
	int ret;

	if (hisi_bw_timer_clk_is_ready() != 0) {
		ddrflux_err("clk is not ready");
		ret = -EINVAL;
		goto err_clk_get;
	}

	ret = clk_set_rate(g_dfdev->ddrc_flux_timer->clk, FLUX_TIMER_CLK);
	if (ret != 0) {
		ddrflux_err("set rate failed");
		goto err_set_rate;
	}

	ddrflux_dbg("clk rate=%ld",
		    clk_get_rate(g_dfdev->ddrc_flux_timer->clk));

	if (hisi_bw_timer_clk_enable() != 0)
		goto err_clk_enable;

	/*
	 * do timer init configs: disable timer ,mask interrupt,
	 * clear interrupt and set clk to 32.768KHz
	 */
	hisi_bw_timer_init_config(g_dfdev);

	/* register timer5 interrupt */
	if (request_irq(g_dfdev->ddrc_flux_timer->irq, hisi_bw_timer_interrupt,
			IRQF_NO_SUSPEND, MODULE_NAME, NULL) != 0) {
		ddrflux_err("request irq for timer error");
		goto err_irq;
	}

	ddrflux_dbg("timerirq=%d,irq_per_cpu=%d",
		    g_dfdev->ddrc_flux_timer->irq,
		    irq_per_cpu);
	ret = irq_set_affinity(g_dfdev->ddrc_flux_timer->irq,
			       cpumask_of(irq_per_cpu));
	if (ret < 0)
		ddrflux_err("irq affinity fail,irq_per_cpu=%d", irq_per_cpu);

	hisi_bw_timer_clk_disable();
	return ret;
err_irq:
	hisi_bw_timer_clk_disable();
err_clk_enable:
err_set_rate:
err_clk_get:
	return ret;
}

static int hisi_bw_timer_deinit(void)
{
	if (g_dfdev->ddrc_flux_timer->clk == NULL)
		return -EINVAL;

	free_irq(g_dfdev->ddrc_flux_timer->irq, NULL);

	return 0;
}

void ddrflux_info_cfg(u32 *info, u32 n)
{
	int i;

	if (info == NULL)
		return;

	if (g_usr[INTERVAL] == 0 || g_usr[SUM_TIME] == 0) {
		ddrflux_err("INTERVAL=%d, SUM_TIME=%d",
			    g_usr[INTERVAL], g_usr[SUM_TIME]);
		return;
	}

	if (n < DDRFLUX_INFO_NUM) {
		ddrflux_err("ddrflux cfg info error!");
		return;
	}

	for (i = 0; i < DDRC_FLUX_MAX; i++)
		g_usr[i] = info[i];
}
EXPORT_SYMBOL(ddrflux_info_cfg);

static void check_ddrflux_param(void)
{
	int interval_min;

	if (g_usr[INTERVAL] == 0) {
		g_usr[INTERVAL] = INTERVAL_CHECK;
		ddrflux_err("INTERVAL=0, fixed as %d", g_usr[INTERVAL]);
	}

	if (g_usr[SUM_TIME] == 0) {
		g_usr[SUM_TIME] = SUM_TIME_CHECK;
		ddrflux_err("SUM_TIME=0, fixed as %d", g_usr[SUM_TIME]);
	}

	if (g_usr[INTERVAL] > g_usr[SUM_TIME]) {
		ddrflux_err("INTERVAL[%d] > SUM_TIME[%d]!",
			    g_usr[INTERVAL], g_usr[SUM_TIME]);
		g_usr[SUM_TIME] = g_usr[INTERVAL];
	}


	interval_min = MIN_STATISTIC_INTERVAL_UNSEC;

	if (g_usr[INTERVAL] < interval_min) {
		ddrflux_err("interval can't too small(<%d)!", interval_min);
		g_usr[INTERVAL] = interval_min;
		g_usr[SUM_TIME] = g_usr[INTERVAL] * MULTI_INTERVAL;
	}
	if (g_usr[IRQ_AFFINITY] > DDRC_IRQ_CPU_CORE_NUM - 1) {
		ddrflux_err("input cpucore error:%d\n", g_usr[IRQ_AFFINITY]);
		g_usr[IRQ_AFFINITY] = DDRC_IRQ_CPU_CORE_NUM - 1;
	}
}

static int __ddrflux_reg_iomap(void __iomem **p, phys_addr_t reg_base,
			       u32 size)
{
	if (p != NULL && *p == NULL) {
		*p = (void __iomem *)virt(reg_base, size);
		if (*p == NULL)
			return -1;
	}

	return 0;
}

static int __ddrflux_reg_iomap_all(void)
{
	u32 ret = 0;

#ifndef DDRC_QOSB_PLATFORM
	if (g_usr[DMSS_ENABLE] != 0)
		ret |= (u32)__ddrflux_reg_iomap(&g_dfdev->dmss_base,
						REG_BASE_DMSS,
						REG_BASE_DMSS_LEN);
#else
	if (g_usr[DMSS_ENABLE] != 0)
		ret |= (u32)__ddrflux_reg_iomap(&g_dfdev->dmss_base,
						REG_BASE_DMSS,
						DDRFLUX_REG_DMSS_LEN);
	if (g_usr[QOSBUF_ENABLE] != 0) {
#ifdef DDRC_4CH_PLATFORM
		ret |= (u32)__ddrflux_reg_iomap(&g_dfdev->qosb_base[0],
						ddr_reg_qosbuf_addr(0),
						REG_QOSBUF_LEN);
		ret |= (u32)__ddrflux_reg_iomap(&g_dfdev->qosb_base[1],
						ddr_reg_qosbuf_addr(1),
						REG_QOSBUF_LEN);
#else
		ret |= (u32)__ddrflux_reg_iomap(&g_dfdev->qosb_base[0],
						ddr_reg_qosbuf_addr(0),
						REG_QOSBUF_LEN);
#endif
#ifdef DDRC_EXMBIST_PCLK_GATE
		ret |= (u32)__ddrflux_reg_iomap(&g_dfdev->exmbist_base,
						DDR_REG_EXMBIST,
						REG_EXMBIST_LEN);
#endif
	}
#endif

	if (g_usr[DMC_ENABLE] != 0) {
#ifdef DDRC_4CH_PLATFORM
		ret |= (u32)__ddrflux_reg_iomap(&g_dfdev->dmc_base[0],
						ddr_reg_dmc_addr(0, 0),
						REG_DMC_LEN);
		ret |= (u32)__ddrflux_reg_iomap(&g_dfdev->dmc_base[1],
						ddr_reg_dmc_addr(0, 1),
						REG_DMC_LEN);
		ret |= (u32)__ddrflux_reg_iomap(&g_dfdev->dmc_base[2],
						ddr_reg_dmc_addr(1, 0),
						REG_DMC_LEN);
		ret |= (u32)__ddrflux_reg_iomap(&g_dfdev->dmc_base[3],
						ddr_reg_dmc_addr(1, 1),
						REG_DMC_LEN);
#else
		/* triones's single channel handle */
		if (g_dmc_start == DDRC_1CH_FLAG) {
			ret |= (u32)__ddrflux_reg_iomap(&g_dfdev->dmc_base[1],
							ddr_reg_dmc_addr(0, 1),
							REG_DMC_LEN);
		} else {
			ret |= (u32)__ddrflux_reg_iomap(&g_dfdev->dmc_base[0],
							ddr_reg_dmc_addr(0, 0),
							REG_DMC_LEN);
			ret |= (u32)__ddrflux_reg_iomap(&g_dfdev->dmc_base[1],
							ddr_reg_dmc_addr(0, 1),
							REG_DMC_LEN);
		}
#endif
	}

	return (int)ret;
}

static void __ddrflux_reg_iounmap(void __iomem **p)
{
	if (p != NULL && *p != NULL) {
		iounmap(*p);
		*p = NULL;
	}
}

static void __ddrflux_reg_iounmap_all(void)
{
	__ddrflux_reg_iounmap(&g_dfdev->dmss_base);
#ifdef DDRC_QOSB_PLATFORM
#ifdef DDRC_4CH_PLATFORM
	__ddrflux_reg_iounmap(&g_dfdev->qosb_base[0]);
	__ddrflux_reg_iounmap(&g_dfdev->qosb_base[1]);
#else
	__ddrflux_reg_iounmap(&g_dfdev->qosb_base[0]);
#endif
#ifdef DDRC_EXMBIST_PCLK_GATE
	__ddrflux_reg_iounmap(&g_dfdev->exmbist_base);
#endif
#endif

#ifdef DDRC_4CH_PLATFORM
	__ddrflux_reg_iounmap(&g_dfdev->dmc_base[0]);
	__ddrflux_reg_iounmap(&g_dfdev->dmc_base[1]);
	__ddrflux_reg_iounmap(&g_dfdev->dmc_base[2]);
	__ddrflux_reg_iounmap(&g_dfdev->dmc_base[3]);
#else
	/* triones's single channel handle */
	if (g_dmc_start == DDRC_1CH_FLAG) {
		__ddrflux_reg_iounmap(&g_dfdev->dmc_base[1]);
	} else {
		__ddrflux_reg_iounmap(&g_dfdev->dmc_base[0]);
		__ddrflux_reg_iounmap(&g_dfdev->dmc_base[1]);
	}
#endif
}

static int __ddrflux_init_unsec(void)
{
	int ret = __ddrflux_reg_iomap_all();
	if (ret != 0) {
		__ddrflux_reg_iounmap_all();
		return ret;
	}

	dmss_flux_enable_ctrl(1);
#ifdef DDRC_QOSB_PLATFORM
	qosbuf_flux_enable_ctrl(1);
#endif
	dmc_flux_enable_ctrl(1);

	return 0;
}

void __ddrflux_init(void)
{
	unsigned long mem_size, flags;
	int ret;

	ddrflux_dbg("%s start\n",  __func__);
	check_ddrflux_param();
	mem_size = (g_usr[SUM_TIME] / g_usr[INTERVAL] + 1) *
		   (sizeof(struct ddrflux_data) + sizeof(u32) *
		    MAX_FLUX_REG_NUM);

	if (mem_size > MAX_DDRFLUX_PULL_DATA) {
		ddrflux_dbg("mem_size[%ld] too big", mem_size);
		return;
	}
	spin_lock_irqsave(&g_dfdev->ddrc_flux_timer->lock, flags);
	g_ddrc_datas = (struct ddrflux_data *)vmalloc(mem_size);
	spin_unlock_irqrestore(&g_dfdev->ddrc_flux_timer->lock, flags);

	if (g_ddrc_datas == NULL) {
		ddrflux_err("vmalloc size =%ld failed!", mem_size);
		return;
	}

	ddrflux_dbg("sum = %u,INTERVAL = %u\n",
		    g_usr[SUM_TIME], g_usr[INTERVAL]);

	ddrflux_dbg("Current pass is unsec kernel!\n");
	ret = __ddrflux_init_unsec();
	if (ret != 0) {
		spin_lock_irqsave(&g_dfdev->ddrc_flux_timer->lock,
					 flags);
		vfree(g_ddrc_datas);
		g_ddrc_datas = NULL;
		spin_unlock_irqrestore(&g_dfdev->ddrc_flux_timer->lock,
					    flags);
		return;
	}

	g_count = 0;
	g_flag = 1;
	hisi_bw_timer_init(g_usr[IRQ_AFFINITY]);
	ddrflux_dbg("%s success\n",  __func__);
}

static void __ddrflux_exit(void)
{
	unsigned long flags;
	struct arm_smccc_res res = {0};

	if (g_ddrc_datas != NULL) {
		g_flag = 0;
		hisi_bw_timer_deinit();
		spin_lock_irqsave(&g_dfdev->ddrc_flux_timer->lock, flags);
		vfree(g_ddrc_datas);
		g_ddrc_datas = NULL;
		spin_unlock_irqrestore(&g_dfdev->ddrc_flux_timer->lock, flags);
	}

	dmss_flux_enable_ctrl(0);
#ifdef DDRC_QOSB_PLATFORM
	qosbuf_flux_enable_ctrl(0);
#endif
	dmc_flux_enable_ctrl(0);
	__ddrflux_reg_iounmap_all();
	ddrflux_dbg("unsec");
}

void ddrflux_exit(void)
{
	__ddrflux_exit();

#ifdef CONFIG_DDR_AUTO_FSGT
	(void)ddr_autofsgt_ctrl(DDR_AUTOFSGT_PROXY_CLIENT_DDRFLUX,
				DDR_AUTOFSGT_LOGIC_EN);
#endif
}
EXPORT_SYMBOL(ddrflux_exit);

void __ddrflux_start(void)
{
	__ddrflux_init();
	hisi_pm_bw_on_timer(g_usr[INTERVAL]);
}

void ddrflux_start(void)
{
#ifdef CONFIG_DDR_AUTO_FSGT
	(void)ddr_autofsgt_ctrl(DDR_AUTOFSGT_PROXY_CLIENT_DDRFLUX,
				DDR_AUTOFSGT_LOGIC_DIS);
#endif
	__ddrflux_start();
}
EXPORT_SYMBOL(ddrflux_start);

void __ddrflux_stop(void)
{
	if (g_stop == 0) {
		hisi_bw_timer_disable(g_dfdev, &g_stop);
		ddrflux_dbg("success!");
	} else {
		ddrflux_err("ddrflux already take over!");
	}
}

void ddrflux_stop(void)
{
	__ddrflux_stop();
}
EXPORT_SYMBOL(ddrflux_stop);

static void ddrflux_dram_type_list(char *dram_type_msg, struct seq_file *mm)
{
	u32 ret;

	switch (g_dfdev->dram_type) {
	case LPDDR:
		ret = strcpy_s(dram_type_msg, DRAM_TYPE_MSG_LEN, "LPDDR");
		break;
	case LPDDR2:
		ret = strcpy_s(dram_type_msg, DRAM_TYPE_MSG_LEN, "LPDDR2");
		break;
	case LPDDR3:
		ret = strcpy_s(dram_type_msg, DRAM_TYPE_MSG_LEN, "LPDDR3");
		break;
	case DDR:
		ret = strcpy_s(dram_type_msg, DRAM_TYPE_MSG_LEN, "DDR");
		break;
	case DDR2:
		ret = strcpy_s(dram_type_msg, DRAM_TYPE_MSG_LEN, "DDR2");
		break;
	case DDR3:
		ret = strcpy_s(dram_type_msg, DRAM_TYPE_MSG_LEN, "DDR3");
		break;
	case DDR4:
		ret = strcpy_s(dram_type_msg, DRAM_TYPE_MSG_LEN, "DDR4");
		break;
	case LPDDR4:
		ret = strcpy_s(dram_type_msg, DRAM_TYPE_MSG_LEN, "LPDDR4");
		break;
	case LPDDR5:
		ret = strcpy_s(dram_type_msg, DRAM_TYPE_MSG_LEN, "LPDDR5");
		break;
	default:
		ret = strcpy_s(dram_type_msg, DRAM_TYPE_MSG_LEN, "RESERVED");
	}

	if (ret != 0)
		seq_printf(mm, "strcpy_s fail");
}

static void *ddrflux_data_seq_start(struct seq_file *m, loff_t *pos)
{
	return (*pos >= g_count) ? NULL : pos;
}

static void *ddrflux_data_seq_next(struct seq_file *m, void *v, loff_t *pos)
{
	(*pos)++;
	return (*pos >= g_count) ? NULL : pos;
}

static void ddrflux_data_seq_stop(struct seq_file *m, void *v)
{
}

static void ddrflux_title_show(u32 index, struct seq_file *mm)
{
	u32 list_asi_num, len;
	char dram_type_display[DRAM_TYPE_MSG_LEN];

	if (index == 0) {
		ddrflux_dram_type_list(dram_type_display, mm);
		seq_printf(mm, "STA_ID:%d,STA_ID_MASK:%d,dram_type:%s,INTERVAL:%dus\n",
			   g_usr[STA_ID], g_usr[STA_ID_MASK],
			   dram_type_display, g_usr[INTERVAL]);
		seq_printf(mm, "index,time,rtc_time,slice,ddrfreq(khz),");

		list_asi_num = (g_ddrflux_list_len - g_max_dmc_num *
				DMC_REG_NUM_PER_DMC - g_max_qos_num *
				QOSB_REG_NUM_PER_QOSB) / DMSS_REG_NUM_PER_ASI;
		for (len = 0; len < g_ddrflux_list_len; len++) {
			if ((len >= g_max_dmss_asi_num &&
			     len < (list_asi_num * ASI_REG_GROUP1)) ||
			    (len >= (list_asi_num * ASI_REG_GROUP1 +
				     g_max_dmss_asi_num) &&
			     len < (list_asi_num * ASI_REG_GROUP2)) ||
			    (len >= (list_asi_num * ASI_REG_GROUP2 +
				     g_max_dmss_asi_num) &&
			     len < (list_asi_num * ASI_REG_GROUP3)))
				continue;

			seq_printf(mm, "%s", g_ddrflux_lookups[len].head);
		}
	}
}

static void ddrflux_data_show(u32 index, struct seq_file *mm)
{
	u32 len;
	struct ddrflux_data *ddrc_data = NULL;
	u64 begin_time_s, begin_time_ns;
	struct tm tm_rtc;

	if (g_ddrc_datas != NULL) {
		ddrc_data = (struct ddrflux_data *)((u64)g_ddrc_datas + index *
						    (sizeof(struct ddrflux_data) +
						     sizeof(u32) * MAX_FLUX_REG_NUM));
		begin_time_s = ddrc_data->ddrc_time;
		begin_time_ns = do_div(begin_time_s, BEGIN_TIME_DIV);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		time64_to_tm((time64_t)begin_time_s, 0, &tm_rtc);
#else
		time_to_tm((time_t)begin_time_s, 0, &tm_rtc);
#endif
		seq_printf(mm, "%d,%05lu.%06lu,", index,
			   (u64)begin_time_s,
			   begin_time_ns / BEGIN_TIME_MODULUS);
		seq_printf(mm, "[%lu:%.2d:%.2d %.2d:%.2d:%.2d],",
			   RTC_YEAR_BASE + tm_rtc.tm_year, tm_rtc.tm_mon + 1,
			   tm_rtc.tm_mday, tm_rtc.tm_hour,
			   tm_rtc.tm_min, tm_rtc.tm_sec);
		seq_printf(mm, "%s,", ddrc_data->slice);
		seq_printf(mm, "%d,", ddrc_data->ddr_tclk_ns);

		for (len = 0; len < MAX_FLUX_REG_NUM - 1; len++)
			seq_printf(mm, "0x%08x,", ddrc_data->ddrflux_data[len]);

		len = (len > 1) ? (len - 1) : 0;
		seq_printf(mm, "0x%08x\n", ddrc_data->ddrflux_data[len]);
	} else {
		ddrflux_err("g_ddrc_datas=NULL");
	}
}

static int ddrflux_data_seq_show(struct seq_file *m, void *v)
{
	u32 i;

	i = *(loff_t *)v;
	ddrflux_title_show(i, m);
	ddrflux_data_show(i, m);

	return 0;
}

static ssize_t ddrflux_stop_store(struct file *filp, const char __user *ubuf,
				  size_t cnt, loff_t *ppos)
{
	__ddrflux_stop();
	return (ssize_t)cnt;
}

static ssize_t ddrflux_copy_user(const char __user *ubuf, size_t cnt,
				 char *init, int init_size)
{
	if (ubuf == NULL || cnt == 0) {
		ddrflux_err("buf is null!");
		return -EINVAL;
	}

	if (cnt > init_size) {
		ddrflux_err("input count larger!");
		return -ENOMEM;
	}

	/* copy init from user space. */
	spin_lock(&g_dfdev->lock);
	if (copy_from_user(init, ubuf, cnt - 1) != 0) {
		spin_unlock(&g_dfdev->lock);
		return (ssize_t)cnt;
	}
	spin_unlock(&g_dfdev->lock);

	return 0;
}

static ssize_t ddrflux_start_store(struct file *filp, const char __user *ubuf,
				   size_t cnt, loff_t *ppos)
{
	int ret;
	char init_tmp[DDRFLUX_INIT_NUM] = {0};

	ret = ddrflux_copy_user(ubuf, cnt, init_tmp, sizeof(init_tmp));
	if (ret != 0)
		return ret;

	if (strncmp(init_tmp, "0", 0x1) == 0) {
		return (ssize_t)cnt;
	} else if (strncmp(init_tmp, "1", 0x1) == 0) {
		if (g_flag != 0) {
			__ddrflux_stop();
			hisi_pm_bw_on_timer(g_usr[INTERVAL]);
			ddrflux_dbg("success!");
		} else {
			ddrflux_err("ddrflux start exception[not init_tmp]!");
		}
	} else {
		ddrflux_err("ubuf data must be 0 or 1!");
	}

	return (ssize_t)cnt;
}

static ssize_t ddrflux_exit_store(struct file *filp, const char __user *ubuf,
				  size_t cnt, loff_t *ppos)
{
	__ddrflux_exit();
#ifdef CONFIG_DDR_AUTO_FSGT
	(void)ddr_autofsgt_ctrl(DDR_AUTOFSGT_PROXY_CLIENT_DDRFLUX,
				DDR_AUTOFSGT_LOGIC_EN);
	ddrflux_err("DDR_AUTOFSGT_LOGIC_EN!");
#endif
	return (ssize_t)cnt;
}

static void dmss_flux_enable_ctrl(int en)
{
	u32 asi_base, val, ckg_byp;

	if (g_usr[DMSS_ENABLE] == 0)
		return;

	if (g_dfdev == NULL || g_dfdev->dmss_base == NULL)
		return;

	if (en != 0) {
		val = readl(SOC_DMSS_GLB_STAT_CTRL_ADDR(g_dfdev->dmss_base));
		val |= DMSS_GLB_STAT_CTRL_MSK;
		writel(val, SOC_DMSS_GLB_STAT_CTRL_ADDR(g_dfdev->dmss_base));
	} else {
		val = readl(SOC_DMSS_GLB_STAT_CTRL_ADDR(g_dfdev->dmss_base));
		val &= ~DMSS_GLB_STAT_CTRL_MSK;
		writel(val, SOC_DMSS_GLB_STAT_CTRL_ADDR(g_dfdev->dmss_base));

		/* ckg_byp_asi SOC_DMSS_ASI_DYN_CKG_ADDR */
		for (asi_base = 0; asi_base < g_max_dmss_asi_num; asi_base++) {
			val = readl(SOC_DMSS_ASI_DYN_CKG_ADDR(g_dfdev->dmss_base,
							      asi_base));
			ckg_byp = val;
			val |= BIT(SOC_DMSS_ASI_DYN_CKG_ckg_byp_asi_START);
			writel(val, SOC_DMSS_ASI_DYN_CKG_ADDR(g_dfdev->dmss_base,
							      asi_base));
			writel(ckg_byp, SOC_DMSS_ASI_DYN_CKG_ADDR(g_dfdev->dmss_base,
								  asi_base));
		}
	}
}

#ifdef DDRC_QOSB_PLATFORM
static void qosbuf_flux_enable_ctrl(int en)
{
	u32 i, reg_val;

	if (g_usr[QOSBUF_ENABLE] == 0 ||
	    g_dfdev == NULL)
		return;
#ifdef DDRC_EXMBIST_PCLK_GATE
	if (g_dfdev->exmbist_base == NULL)
		return;

	reg_val = readl(SOC_EXMBIST_PCLK_GATE_ADDR(g_dfdev->exmbist_base));
	reg_val |= BIT(SOC_EXMBIST_PCLK_GATE_apb_gt_en_START);
	writel(reg_val, SOC_EXMBIST_PCLK_GATE_ADDR(g_dfdev->exmbist_base));
	reg_val = BIT(SOC_EXMBIST_CLKEN0_apb_clken_0_START) |
		  BIT(SOC_EXMBIST_CLKEN0_apb_clken_0_START +
		      SOC_EXMBIST_CLKEN0_clk_wr_en_START) |
		  BIT(SOC_EXMBIST_CLKEN0_apb_clken_1_START) |
		  BIT(SOC_EXMBIST_CLKEN0_apb_clken_1_START +
		      SOC_EXMBIST_CLKEN0_clk_wr_en_START);
	writel(reg_val, SOC_EXMBIST_CLKEN0_ADDR(g_dfdev->exmbist_base));
#endif
	for (i = 0; i < g_max_qos_num; i++) {
		if (g_dfdev->qosb_base[i] == NULL)
			return;

		reg_val = readl(SOC_DDRC_QOSB_QOSB_CFG_PERF_ADDR(g_dfdev->qosb_base[i]));
		reg_val = en ? (reg_val | BIT(BIT_QOSB_PERF_EN)) :
			  (reg_val & ~(BIT(BIT_QOSB_PERF_EN)));
		writel(reg_val, SOC_DDRC_QOSB_QOSB_CFG_PERF_ADDR(g_dfdev->qosb_base[i]));
	}
#ifdef DDRC_EXMBIST_PCLK_GATE
	reg_val = readl(SOC_EXMBIST_PCLK_GATE_ADDR(g_dfdev->exmbist_base));
	reg_val &= ~(BIT(SOC_EXMBIST_PCLK_GATE_apb_gt_en_START));
	writel(reg_val, SOC_EXMBIST_PCLK_GATE_ADDR(g_dfdev->exmbist_base));
#endif
}
#endif

static void dmc_flux_enable_ctrl(int en)
{
	u32 i;
#ifndef DDRC_QOSB_PLATFORM
	volatile SOC_DDRC_DMC_DDRC_CTRL_STADAT_UNION *ddrc_ctrl_stadat = NULL;
	volatile SOC_DDRC_DMC_DDRC_CTRL_STACMD_UNION *ddrc_ctrl_stacmd = NULL;
#else
	u32 reg_val;
#endif

	if (g_usr[DMC_ENABLE] == 0 || g_dfdev == NULL)
		return;

	/* triones's single channel handle */
	if (g_dmc_start == DDRC_1CH_FLAG)
		g_dfdev->dmc_base[0] = g_dfdev->dmc_base[1];

	for (i = 0; i < g_max_dmc_num; i++) {
		if (g_dfdev->dmc_base[i] == NULL)
			return;

		writel(g_usr[STA_ID],
		       SOC_DDRC_DMC_DDRC_CFG_STAID_ADDR(g_dfdev->dmc_base[i]));
		writel(g_usr[STA_ID_MASK],
		       SOC_DDRC_DMC_DDRC_CFG_STAIDMSK_ADDR(g_dfdev->dmc_base[i]));

#ifndef DDRC_QOSB_PLATFORM
		ddrc_ctrl_stadat =
			(volatile SOC_DDRC_DMC_DDRC_CTRL_STADAT_UNION *)SOC_DDRC_DMC_DDRC_CTRL_STADAT_ADDR(g_dfdev->dmc_base[i]);
		ddrc_ctrl_stadat->reg.dat_stat_en = en;
		ddrc_ctrl_stacmd =
			(volatile SOC_DDRC_DMC_DDRC_CTRL_STACMD_UNION *)SOC_DDRC_DMC_DDRC_CTRL_STACMD_ADDR(g_dfdev->dmc_base[i]);
		ddrc_ctrl_stacmd->reg.load_stat_en = en;
#else
		reg_val = readl(SOC_DDRC_DMC_DDRC_STADAT_ADDR(g_dfdev->dmc_base[i]));
		reg_val = en ? (reg_val | BIT(BIT_DMC_EN)) :
			  (reg_val & ~(BIT(BIT_DMC_EN)));
		writel(reg_val, SOC_DDRC_DMC_DDRC_STADAT_ADDR(g_dfdev->dmc_base[i]));

		reg_val = readl(SOC_DDRC_DMC_DDRC_STACMD_ADDR(g_dfdev->dmc_base[i]));
		reg_val = en ? (reg_val | BIT(BIT_DMC_EN)) :
			  (reg_val & ~(BIT(BIT_DMC_EN)));
		writel(reg_val, SOC_DDRC_DMC_DDRC_STACMD_ADDR(g_dfdev->dmc_base[i]));
#endif
	}

	/* triones's single channel handle */
	if (g_dmc_start == DDRC_1CH_FLAG)
		g_dfdev->dmc_base[0] = NULL;
}

static ssize_t ddrflux_init_store(struct file *filp, const char __user *ubuf,
				  size_t cnt, loff_t *ppos)
{
	int ret;
	char init[DDRFLUX_INIT_NUM] = {0};

	ret = ddrflux_copy_user(ubuf, cnt, init, sizeof(init));
	if (ret != 0)
		return ret;

	if (strncmp(init, "0", 0x1UL) == 0) {
		return (ssize_t)cnt;
	} else if (strncmp(init, "1", 0x1UL) == 0) {
#ifdef CONFIG_DDR_AUTO_FSGT
		(void)ddr_autofsgt_ctrl(DDR_AUTOFSGT_PROXY_CLIENT_DDRFLUX,
					DDR_AUTOFSGT_LOGIC_DIS);
		ddrflux_err("DDR_AUTOFSGT_LOGIC_DIS!");
#endif
		__ddrflux_stop();
		__ddrflux_exit();
		__ddrflux_init();
	} else {
		ddrflux_err("ubuf data must be 0 or 1");
	}

	return (ssize_t)cnt;
}

static const struct seq_operations g_ddrflux_data_seq_ops = {
	.start = ddrflux_data_seq_start,
	.next = ddrflux_data_seq_next,
	.stop = ddrflux_data_seq_stop,
	.show = ddrflux_data_seq_show
};

static int ddrflux_data_open(struct inode *inode, struct file *file)
{
	__ddrflux_stop();
	return seq_open(file, &g_ddrflux_data_seq_ops);
}

static const struct file_operations g_ddrflux_data_fops = {
	.open = ddrflux_data_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int ddrflux_open(struct inode *inode, struct file *filp)
{
	filp->private_data = inode->i_private;
	return 0;
}

#define MODULE_STORE_DEFINE(func_name) \
	static const struct file_operations func_name##_fops = { \
	.open = ddrflux_open, \
	.write = func_name##_store, \
	.llseek = seq_lseek, \
}

MODULE_STORE_DEFINE(ddrflux_init);
MODULE_STORE_DEFINE(ddrflux_exit);
MODULE_STORE_DEFINE(ddrflux_start);
MODULE_STORE_DEFINE(ddrflux_stop);

static int ddrc_flux_dir_init(void)
{
#ifdef CONFIG_DFX_DEBUG_FS
	g_ddrc_flux_dir = debugfs_create_dir("ddrflux", NULL);
	if (g_ddrc_flux_dir == NULL)
		return -ENOMEM;

	g_flux_range = debugfs_create_dir("enable_range", g_ddrc_flux_dir);
	if (g_flux_range == NULL)
		return -ENOMEM;

	debugfs_create_u32("STA_ID", PRIV_AUTH, g_ddrc_flux_dir,
			   &g_usr[STA_ID]);
	debugfs_create_u32("STA_ID_MASK", PRIV_AUTH,
			   g_ddrc_flux_dir, &g_usr[STA_ID_MASK]);
	debugfs_create_u32("INTERVAL", PRIV_AUTH,
			   g_ddrc_flux_dir, &g_usr[INTERVAL]);
	debugfs_create_u32("SUM_TIME", PRIV_AUTH,
			   g_ddrc_flux_dir, &g_usr[SUM_TIME]);
	debugfs_create_u32("IRQ_AFFINITY", PRIV_AUTH,
			   g_ddrc_flux_dir, &g_usr[IRQ_AFFINITY]);

	debugfs_create_u32("DMSS_ENABLE", PRIV_AUTH,
			   g_flux_range, &g_usr[DMSS_ENABLE]);
#ifdef DDRC_QOSB_PLATFORM
	debugfs_create_u32("QOSBUF_ENABLE", PRIV_AUTH,
			   g_flux_range, &g_usr[QOSBUF_ENABLE]);
#endif
	debugfs_create_u32("DMC_ENABLE", PRIV_AUTH,
			   g_flux_range, &g_usr[DMC_ENABLE]);
	debugfs_create_u32("DMCCMD_ENABLE", PRIV_AUTH,
			   g_flux_range, &g_usr[DMCCMD_ENABLE]);
	debugfs_create_u32("DMCDATA_ENABLE", PRIV_AUTH,
			   g_flux_range, &g_usr[DMCDATA_ENABLE]);
	debugfs_create_u32("DMCLATENCY_ENABLE", PRIV_AUTH,
			   g_flux_range, &g_usr[DMCLATENCY_ENABLE]);
	debugfs_create_file("ddrflux_init", PRIV_AUTH, g_ddrc_flux_dir,
			    NULL, &ddrflux_init_fops);
	debugfs_create_file("ddrflux_exit", PRIV_AUTH, g_ddrc_flux_dir,
			    NULL, &ddrflux_exit_fops);
	debugfs_create_file("ddrflux_start", PRIV_AUTH, g_ddrc_flux_dir,
			    NULL, &ddrflux_start_fops);
	debugfs_create_file("ddrflux_stop", PRIV_AUTH, g_ddrc_flux_dir,
			    NULL, &ddrflux_stop_fops);
	debugfs_create_file("ddrflux_data", FLUX_DATA_FILE_MOD,
			    g_ddrc_flux_dir, NULL,
			    &g_ddrflux_data_fops);
#endif
	return 0;
}

static int hisi_get_platform(struct device_node *np)
{
	const char *compatible = NULL;
	int cplen;
	u64 cmplen;

	compatible = of_get_property(np, "compatible", &cplen);
	if (compatible == NULL || cplen < 0) {
		ddrflux_err("can find platform compatible");
		return -1;
	}

	ddrflux_dbg("compatible=%s", compatible);
	cmplen = strlen(compatible);

	if (strncmp(compatible, "hisilicon,ddrc-flux", cmplen + 1) == 0)
		g_hisi_plat_ddr = PLAT_MAX;
	else
		g_hisi_plat_ddr = DDRC_PLATFORM_NAME;

	return 0;
}

static int hisi_get_platform_info(struct device_node *np)
{
	if (hisi_get_platform(np) != 0)
		return -ENODEV;

	g_max_dmc_num = DDRC_DMC_NUM;
	g_max_dmss_asi_num = DDRC_ASI_PORT_NUM;
	g_max_qos_num = DDRC_QOSB_NUM;
	g_ddrflux_lookups = &g_ddrflux_lookups_info[0];
	g_ddrflux_list_len = sizeof(g_ddrflux_lookups_info) /
				sizeof(g_ddrflux_lookups_info[0]);

	return 0;
}

static void ddrc_flux_base_addr_init(void)
{
	int i;

	g_dfdev->dmss_base =  NULL;

#ifdef DDRC_QOSB_PLATFORM
#ifdef DDRC_4CH_PLATFORM
	g_dfdev->qosb_base[0] = NULL;
	g_dfdev->qosb_base[1] = NULL;
#else
	g_dfdev->qosb_base[0] = NULL;
#endif

#ifdef DDRC_EXMBIST_PCLK_GATE
	g_dfdev->exmbist_base = NULL;
#endif
#endif
	for (i = 0; i < DDRC_DMC_NUM; i++)
		g_dfdev->dmc_base[i] = NULL;
}

static void ddrc_flux_set_dmc_start(void)
{
#ifdef DDRC_WITH_1CH
	g_dmc_start = (readl(SOC_SCTRL_SCBAKDATA7_ADDR(g_dfdev->sctrl)) &
		       BIT(SCBAKDATA7_CH_NUM_BIT)) >> SCBAKDATA7_CH_NUM_BIT;
	ddrflux_err("%s g_dmc_start = %u\n", __func__, g_dmc_start);
#else
	g_dmc_start = DDRC_2CH_FLAG;
#endif
}

static void ddrc_flux_get_timer(u32 val)
{
	g_dfdev->ddrc_flux_timer->irq = val;
	g_dfdev->ddrc_flux_timer->irq_per_cpu =  DDRC_IRQ_CPU_CORE_NUM - 1;
	g_dfdev->ddrc_flux_timer->pclk = NULL;

	spin_lock_init(&g_dfdev->ddrc_flux_timer->lock);
}

static void ddrc_flux_set_default_name(void)
{
	u32 slice_length = min((unsigned long)(SLICE_LEN - 1),
			       strlen(DEFAULT_SLICE_NAME));

	(void)strncpy_s(g_dfdev->slice.name, SLICE_LEN, DEFAULT_SLICE_NAME,
			slice_length);
	g_dfdev->slice.name[slice_length] = '\0';
}

static int ddrc_flux_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	int ret;
	u32 val;

	if (np == NULL) {
		ddrflux_err("%s: no dev node", MODULE_NAME);
		ret = -ENODEV;
		goto out;
	}
	ddrflux_dbg("%s, [%s][%s]start\n", __func__, dev_name(&pdev->dev), np->name);

	g_dfdev = kzalloc(sizeof(struct ddrc_flux_device), GFP_KERNEL);
	if (g_dfdev == NULL) {
		dev_warn(&pdev->dev, "Kzalloc failed\n");
		ret = -ENOMEM;
		goto no_mem;
	}

	g_dfdev->sctrl = virt(SOC_ACPU_SCTRL_BASE_ADDR, 1);
	if (g_dfdev->sctrl == NULL) {
		ddrflux_err("ioremap failed");
		goto no_iomap;
	}

	ddrc_flux_set_dmc_start();
	ret = hisi_get_platform_info(np);
	if (ret != 0)
		goto no_iomap;

	ddrc_flux_base_addr_init();
	g_dfdev->ddrc_freq_clk = of_clk_get(np, 0);

	if (IS_ERR_OR_NULL(g_dfdev->ddrc_freq_clk)) {
		dev_warn(&pdev->dev, "Clock not found\n");
		ret = PTR_ERR(g_dfdev->ddrc_freq_clk);
		goto no_ddrfreq;
	}

	g_dfdev->ddrc_flux_timer = kzalloc(sizeof(struct hisi_bw_timer),
					   GFP_KERNEL);
	if (g_dfdev->ddrc_flux_timer == NULL) {
		ddrflux_err("Kzalloc failed");
		ret = -ENOMEM;
		goto no_timer_mem;
	}

	g_dfdev->ddrc_flux_timer->clk = of_clk_get(np, 1);

	if (IS_ERR_OR_NULL(g_dfdev->ddrc_flux_timer->clk)) {
		ddrflux_err("timer clock get failed!");
		ret = PTR_ERR(g_dfdev->ddrc_flux_timer->clk);
		goto no_timerclk;
	}

	g_dfdev->ddrc_flux_timer->base = virt(SOC_ACPU_TIMER5_BASE_ADDR,
					      ACPU_TIMER5_BASE_LEN);

	if (g_dfdev->ddrc_flux_timer->base == NULL) {
		ddrflux_err("ioremap failed");
		goto err;
	}

#ifndef DDRC_QOSB_PLATFORM
	writel(TIMER_SCTRL_SCPEREN1_GT_CLOCK, SOC_SCTRL_SCPEREN1_ADDR(g_dfdev->sctrl));
#endif

	val = irq_of_parse_and_map(np, 0);
	if (val == 0) {
		dev_err(&pdev->dev, "cannot find IRQ\n");
		goto remap_fail;
	}

	ddrc_flux_get_timer(val);
	ddrc_flux_set_default_name();
	g_dfdev->dram_type = LPDDR;

	spin_lock_init(&g_dfdev->lock);
	platform_set_drvdata(pdev, g_dfdev);
	ret = ddrc_flux_dir_init();
	if (ret != 0) {
		ddrflux_err("create flux dir failed!\n");
		goto remap_fail;
	}

	ddrflux_err("[%s] probe success!\n", dev_name(&pdev->dev));
	return ret;
remap_fail:
	iounmap(g_dfdev->ddrc_flux_timer->base);
	g_dfdev->ddrc_flux_timer->base = NULL;
err:
	clk_put(g_dfdev->ddrc_flux_timer->clk);
no_timerclk:
	kfree(g_dfdev->ddrc_flux_timer);
	g_dfdev->ddrc_flux_timer = NULL;
no_timer_mem:
	clk_put(g_dfdev->ddrc_freq_clk);
no_ddrfreq:

	if (g_dfdev->ddrflux_base != NULL)
		iounmap(g_dfdev->ddrflux_base);

	g_dfdev->ddrflux_base = NULL;
no_iomap:
	kfree(g_dfdev);
	g_dfdev = NULL;
no_mem:
	of_node_put(np);
out:
	return ret;
}

static int ddrc_flux_remove(struct platform_device *pdev)
{
	platform_set_drvdata(pdev, NULL);
	clk_put(g_dfdev->ddrc_freq_clk);
	clk_put(g_dfdev->ddrc_flux_timer->clk);

	if (g_ddrc_flux_dir != NULL)
		debugfs_remove_recursive(g_ddrc_flux_dir);

	if (g_dfdev != NULL) {
		kfree(g_dfdev);
		g_dfdev = NULL;
	}

	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id g_ddrc_flux_of_device_ids[] = {
	PLATFORM_LIST
};

MODULE_DEVICE_TABLE(of, ddr_devfreq_of_match);
#endif

static struct platform_driver g_ddrc_flux_driver = {
	.driver = {
		.name = "ddrc-flux",
		.owner = THIS_MODULE,
		.of_match_table = g_ddrc_flux_of_device_ids,
	},
	.probe = ddrc_flux_probe,
	.remove = ddrc_flux_remove,
};

module_platform_driver(g_ddrc_flux_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ddrc flux statistics driver for hisi");
