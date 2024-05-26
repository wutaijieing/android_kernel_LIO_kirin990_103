/*
 * vcodec_vdec_dpm.c
 *
 * This is for dpm vdec clk enable
 *
 * Copyright (c) 2009-2020 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "vcodec_vdec_dpm.h"
#include <asm/compiler.h>
#include <linux/compiler.h>
#include <linux/mutex.h>
#include <linux/of_device.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/platform_drivers/dpm_hwmon.h>
#include "platform_include/see/bl31_smc.h"
#include "vcodec_vdec_plat.h"

#define DPM_VDEC_TYPE 1
#define DPM_LOAD_NUM 16

#define VDEC_SIGNAL_MODE 0x0000
#define VDEC_FREQ_CLEAR_MASK 0xFFFFFFF8

#define vdec_dpm_freq_sel(base) ((base) + (0x0020))
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#define __asmeq(x, y)  ".ifnc " x "," y " ; .err ; .endif\n\t"
#endif

enum {
	ACCUMULATE_DISABLE,
	ACCUMULATE_ENABLE,
};

enum {
	PARAM_DISABLE,
	PARAM_ENABLE,
	PARAM_UNUSED,
};

static DEFINE_MUTEX(g_dpm_vdec_state_lock);

static void __iomem *g_vdec_base;
static void __iomem *g_dpm_vdec_base;

static unsigned int g_dpm_vdec_monitor_param[] = {
	0x98E, 0x2D31, 0xA4C, 0x4426, 0x1456, 0x1739, 0xF2CB, 0x7C11,
	0x725, 0x1C02, 0x15516, 0x10A5, 0x1E73, 0x61E, 0x3F6, 0x5C7,
	0,
	0x242, 0x242, 0x200, 0x1C2, 0x188, 0x152, 0x120, 0xF2,
	0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200
};

static vcodec_bool g_dpm_vdec_inited = VCODEC_FALSE;

static int vcodec_dpm_vdec_update_counter(void);
static int vcodec_dpm_vdec_update_power(void);
#ifdef CONFIG_DPM_HWMON_DEBUG
static int vcodec_dpm_vdec_get_counter_for_fitting(int mode);
#endif

static struct dpm_hwmon_ops g_dpm_vdec_ops = {
	.dpm_module_id = DPM_VDEC_ID,
	.dpm_type = DPM_PERI_MODULE,
	.hi_dpm_update_counter = vcodec_dpm_vdec_update_counter,
	.dpm_cnt_len = DPM_LOAD_NUM * DPM_VDEC_TYPE,
	.dpm_power_len = DPM_VDEC_TYPE,
	.hi_dpm_update_power = vcodec_dpm_vdec_update_power,
#ifdef CONFIG_DPM_HWMON_DEBUG
	.hi_dpm_get_counter_for_fitting = vcodec_dpm_vdec_get_counter_for_fitting,
#endif
};

noinline int atfd_vdec_dpm(u64 _function_id, u64 _arg0, u64 _arg1, u64 _arg2)
{
	register u64 function_id asm("x0") = _function_id;
	register u64 arg0 asm("x1") = _arg0;
	register u64 arg1 asm("x2") = _arg1;
	register u64 arg2 asm("x3") = _arg2;
	asm volatile (
		__asmeq("%0", "x0")
		__asmeq("%1", "x1")
		__asmeq("%2", "x2")
		__asmeq("%3", "x3")
		"smc #0\n"
		: "+r" (function_id)
		: "r" (arg0), "r" (arg1), "r" (arg2));

	return (int)function_id;
}

static int dpm_vdec_enable(const vdec_dts *dts_info)
{
	unsigned int val;
	vfmw_module_reg_info vdec_crg_reg_info = dts_info->module_reg[CRG_MOUDLE];
	vfmw_module_reg_info vdec_dpm_reg_info = dts_info->module_reg[DP_MONIT_MOUDLE];

	atfd_vdec_dpm(VDEC_FID_VALUE, PARAM_ENABLE,
		vdec_crg_reg_info.reg_phy_addr, PARAM_UNUSED);

	g_vdec_base = ioremap(vdec_crg_reg_info.reg_phy_addr, vdec_crg_reg_info.reg_range);
	if (!g_vdec_base) {
		dprint(PRN_FATAL, "g_vdec_base is NULL\n");
		return VCODEC_FAILURE;
	}

	g_dpm_vdec_base = ioremap(vdec_dpm_reg_info.reg_phy_addr, vdec_dpm_reg_info.reg_range);
	if (!g_dpm_vdec_base) {
		iounmap(g_vdec_base);
		g_vdec_base = NULL;
		dprint(PRN_FATAL, "g_dpm_vdec_base is NULL\n");
		return VCODEC_FAILURE;
	}

	val = readl(vdec_dpm_freq_sel(g_vdec_base));
	writel(val | VDEC_CLK_RATE_LOWER, vdec_dpm_freq_sel(g_vdec_base));

	if (g_dpm_vdec_ops.module_enabled)
		dpm_monitor_enable(g_dpm_vdec_base, VDEC_SIGNAL_MODE,
			g_dpm_vdec_monitor_param, ARRAY_SIZE(g_dpm_vdec_monitor_param));

	dprint(PRN_DBG, "enable dpm_vdec success\n");
	return 0;
}

static void dpm_vdec_disable(const vdec_dts *dts_info)
{
	vfmw_module_reg_info vdec_crg_reg_info = dts_info->module_reg[CRG_MOUDLE];

	dpm_monitor_disable(g_dpm_vdec_base);
	atfd_vdec_dpm(VDEC_FID_VALUE, PARAM_DISABLE,
		vdec_crg_reg_info.reg_phy_addr, PARAM_UNUSED);

	iounmap(g_vdec_base);
	g_vdec_base = NULL;
	iounmap(g_dpm_vdec_base);
	g_dpm_vdec_base = NULL;

	dprint(PRN_DBG, "disable dpm_vdec success\n");
}

static int vcodec_dpm_vdec_update_power(void)
{
	int len;
	unsigned long long *dpm_ptable = g_dpm_vdec_ops.dpm_power_table;

	if (!dpm_ptable) {
		dprint(PRN_FATAL, "dpm_power_table is NULL\n");
		return -EINVAL;
	}

	mutex_lock(&g_dpm_vdec_state_lock);
	if (!g_dpm_vdec_inited) {
		mutex_unlock(&g_dpm_vdec_state_lock);
		return 0;
	}

	writel(BIT(SOC_DPMONITOR_SOFT_PULSE_soft_pulse_START),
		SOC_DPMONITOR_SOFT_PULSE_ADDR(g_dpm_vdec_base));

	udelay(1);

	len = dpm_hwmon_update_power(g_dpm_vdec_base, dpm_ptable, ACCUMULATE_ENABLE);

	mutex_unlock(&g_dpm_vdec_state_lock);

	return len;
}

static int vcodec_dpm_vdec_get_counter_power(int mode)
{
	int len;
	unsigned long long *dpm_ctable = g_dpm_vdec_ops.dpm_counter_table;
	unsigned long long *dpm_ptable = g_dpm_vdec_ops.dpm_power_table;

	if (!dpm_ctable || !dpm_ptable) {
		dprint(PRN_FATAL, "dpm_counter_table is NULL\n");
		return -EINVAL;
	}

	mutex_lock(&g_dpm_vdec_state_lock);
	if (!g_dpm_vdec_inited) {
		dprint(PRN_ALWS, "vdec dpm is disabled\n");
		mutex_unlock(&g_dpm_vdec_state_lock);
		return 0;
	}

	writel(BIT(SOC_DPMONITOR_SOFT_PULSE_soft_pulse_START),
		SOC_DPMONITOR_SOFT_PULSE_ADDR(g_dpm_vdec_base));

	udelay(1);

	len = dpm_hwmon_update_counter_power(g_dpm_vdec_base,
		dpm_ctable, DPM_LOAD_NUM, dpm_ptable, mode);

	mutex_unlock(&g_dpm_vdec_state_lock);

	return len;
}

static int vcodec_dpm_vdec_update_counter(void)
{
	return vcodec_dpm_vdec_get_counter_power(ACCUMULATE_ENABLE);
}

#ifdef CONFIG_DPM_HWMON_DEBUG
static int vcodec_dpm_vdec_get_counter_for_fitting(int mode)
{
	return vcodec_dpm_vdec_get_counter_power(mode);
}
#endif

void vdec_dpm_freq_select(clk_rate_e clk_type)
{
	unsigned int val;

	if (!g_dpm_vdec_inited) {
		dprint(PRN_DBG, "vdec dpm is disabled\n");
		return;
	}

	val = readl(vdec_dpm_freq_sel(g_vdec_base));
	writel((val & VDEC_FREQ_CLEAR_MASK) | clk_type, vdec_dpm_freq_sel(g_vdec_base));
}

void vdec_dpm_init(void)
{
	int ret;
	vdec_plat *plt = vdec_plat_get_entry();

	if (!plt->plt_init) {
		dprint(PRN_ERROR, "vdec plt is not init\n");
		return;
	}

	if (!g_dpm_vdec_ops.module_enabled) {
		dprint(PRN_DBG, "module_enabled is false\n");
		return;
	}

	if (g_dpm_vdec_inited) {
		dprint(PRN_DBG, "dpm_vdec is already init\n");
		return;
	}

	ret = dpm_vdec_enable(&plt->dts_info);
	if (ret != 0) {
		dprint(PRN_FATAL, "dpm_vdec enable failed\n");
		return;
	}

	mutex_lock(&g_dpm_vdec_state_lock);
	g_dpm_vdec_inited = VCODEC_TRUE;
	mutex_unlock(&g_dpm_vdec_state_lock);

	dprint(PRN_DBG, "dpm_vdec is successfully init\n");
}

void vdec_dpm_deinit(void)
{
	vdec_plat *plt = vdec_plat_get_entry();

	if (!plt->plt_init) {
		dprint(PRN_ERROR, "vdec plt is not init\n");
		return;
	}

	if (!g_dpm_vdec_inited) {
		dprint(PRN_DBG, "dpm_vdec is not init\n");
		return;
	}

	(void)vcodec_dpm_vdec_update_power();

	mutex_lock(&g_dpm_vdec_state_lock);
	g_dpm_vdec_inited = VCODEC_FALSE;
	mutex_unlock(&g_dpm_vdec_state_lock);

	dpm_vdec_disable(&plt->dts_info);
	dprint(PRN_DBG, "dpm_vdec is successfully deinit\n");
}

static int __init dpm_vdec_init(void)
{
	dpm_hwmon_register(&g_dpm_vdec_ops);
	return 0;
}

static void __exit dpm_vdec_exit(void)
{
	dpm_hwmon_unregister(&g_dpm_vdec_ops);
}

module_init(dpm_vdec_init);
module_exit(dpm_vdec_exit);

