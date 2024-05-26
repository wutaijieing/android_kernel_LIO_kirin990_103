/*
 * drv_venc_dpm.c
 *
 * This is for venc dpm.
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

#include "drv_venc_dpm.h"
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

#define VENC_DPM_TYPE 1
#define DPM_LOAD_NUM 16

#define venc_dpm_apb_clk_en(base) ((base) + 0x084C)
#define vcpi_dpm_softrst(base) ((base) + 0x0024)
#define venc_dpm_freq_sel(base) ((base) + 0x0)

#define VENC_SIGNAL_MODE 0x9D56

#define VENC_FREQ_CLEAR_MASK 0xFFFFFFF8

enum {
	ACCUMULATE_DISABLE,
	ACCUMULATE_ENABLE,
};

static DEFINE_MUTEX(g_dpm_venc_state_lock);

static struct venc_dpm_regulator {
	void __iomem *venc_base;
	void __iomem *dpm_venc_base;
	void __iomem *venc_smmu_pre_base;
} g_venc_dpm_regulator;

static unsigned int g_dpm_venc_monitor_param[] = {
	0x5C5C, 0x157, 0x210, 0x1419, 0x50E, 0x282D, 0x31C, 0x564,
	0x418, 0x17846, 0x1, 0x157, 0x2DC, 0x34713, 0x1371B, 0x389,
	0,
	0x242, 0x242, 0x200, 0x1C2, 0x188, 0x152, 0x120, 0xF2,
	0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200
};

static bool g_dpm_venc_inited = false;

static int vcodec_dpm_venc_update_counter(void);
static int vcodec_dpm_venc_update_power(void);
#ifdef CONFIG_DPM_HWMON_DEBUG
static int vcodec_dpm_venc_get_counter_for_fitting(int mode);
#endif

static struct dpm_hwmon_ops g_dpm_venc_ops = {
	.dpm_module_id = DPM_VENC_ID,
	.dpm_type = DPM_PERI_MODULE,
	.hi_dpm_update_counter = vcodec_dpm_venc_update_counter,
	.dpm_cnt_len = DPM_LOAD_NUM * VENC_DPM_TYPE,
	.dpm_power_len = VENC_DPM_TYPE,
	.hi_dpm_update_power = vcodec_dpm_venc_update_power,
#ifdef CONFIG_DPM_HWMON_DEBUG
	.hi_dpm_get_counter_for_fitting = vcodec_dpm_venc_get_counter_for_fitting,
#endif
};

static int dpm_venc_enable(const struct venc_config *venc_config)
{
	unsigned int val;
	struct venc_config_priv venc_cfg_reg_info = venc_config->venc_conf_priv[VENC_CORE_0];
	struct venc_config_priv venc_dpm_reg_info = venc_config->venc_conf_dpm[DP_MONIT_MOUDLE];
	struct venc_config_priv venc_smmu_reg_info = venc_config->venc_conf_dpm[SMMU_PRE_MODULE];

	g_venc_dpm_regulator.venc_base = ioremap(venc_cfg_reg_info.reg_base_addr, venc_cfg_reg_info.reg_range);
	if (g_venc_dpm_regulator.venc_base == NULL) {
		VCODEC_ERR_VENC("venc_base is NULL");
		return -EIO;
	}

	g_venc_dpm_regulator.dpm_venc_base = ioremap(venc_dpm_reg_info.reg_base_addr, venc_dpm_reg_info.reg_range);
	if (g_venc_dpm_regulator.dpm_venc_base == NULL) {
		iounmap(g_venc_dpm_regulator.venc_base);
		g_venc_dpm_regulator.venc_base = NULL;
		VCODEC_ERR_VENC("dpm_venc_base is NULL");
		return -EIO;
	}

	g_venc_dpm_regulator.venc_smmu_pre_base = ioremap(venc_smmu_reg_info.reg_base_addr, venc_smmu_reg_info.reg_range);
	if (g_venc_dpm_regulator.venc_smmu_pre_base == NULL) {
		iounmap(g_venc_dpm_regulator.venc_base);
		g_venc_dpm_regulator.venc_base = NULL;
		iounmap(g_venc_dpm_regulator.dpm_venc_base);
		g_venc_dpm_regulator.dpm_venc_base = NULL;
		VCODEC_ERR_VENC("venc_smmu_pre is NULL");
		return -EIO;
	}

	val = readl(venc_dpm_apb_clk_en(g_venc_dpm_regulator.venc_base));
	writel(val | BIT(22), venc_dpm_apb_clk_en(g_venc_dpm_regulator.venc_base));

	val = readl(vcpi_dpm_softrst(g_venc_dpm_regulator.venc_base));
	writel(val & ~BIT(0), vcpi_dpm_softrst(g_venc_dpm_regulator.venc_base));

	val = readl(venc_dpm_freq_sel(g_venc_dpm_regulator.venc_smmu_pre_base));
	writel(val | VENC_CLK_RATE_LOWER, venc_dpm_freq_sel(g_venc_dpm_regulator.venc_smmu_pre_base));

	if (g_dpm_venc_ops.module_enabled)
		dpm_monitor_enable(g_venc_dpm_regulator.dpm_venc_base, VENC_SIGNAL_MODE,
			g_dpm_venc_monitor_param, ARRAY_SIZE(g_dpm_venc_monitor_param));

	VCODEC_DBG_VENC("enable dpm_venc success");
	return 0;
}

static void dpm_venc_disable(void)
{
	unsigned int val;

	dpm_monitor_disable(g_venc_dpm_regulator.dpm_venc_base);

	val = readl(vcpi_dpm_softrst(g_venc_dpm_regulator.venc_base));
	writel(val | BIT(0), vcpi_dpm_softrst(g_venc_dpm_regulator.venc_base));

	val = readl(venc_dpm_apb_clk_en(g_venc_dpm_regulator.venc_base));
	writel(val & ~BIT(22), venc_dpm_apb_clk_en(g_venc_dpm_regulator.venc_base));

	iounmap(g_venc_dpm_regulator.venc_base);
	g_venc_dpm_regulator.venc_base = NULL;
	iounmap(g_venc_dpm_regulator.dpm_venc_base);
	g_venc_dpm_regulator.dpm_venc_base = NULL;
	iounmap(g_venc_dpm_regulator.venc_smmu_pre_base);
	g_venc_dpm_regulator.venc_smmu_pre_base = NULL;
	VCODEC_DBG_VENC("disable dpm_venc success");
}

static int vcodec_dpm_venc_update_power(void)
{
	int len;
	unsigned long long *dpm_ptable = g_dpm_venc_ops.dpm_power_table;

	if (!dpm_ptable) {
		VCODEC_ERR_VENC("dpm_power_table is NULL");
		return -EINVAL;
	}

	mutex_lock(&g_dpm_venc_state_lock);
	if (!g_dpm_venc_inited) {
		mutex_unlock(&g_dpm_venc_state_lock);
		return 0;
	}

	writel(BIT(SOC_DPMONITOR_SOFT_PULSE_soft_pulse_START),
		SOC_DPMONITOR_SOFT_PULSE_ADDR(g_venc_dpm_regulator.dpm_venc_base));

	udelay(1);

	len = dpm_hwmon_update_power(g_venc_dpm_regulator.dpm_venc_base, dpm_ptable, ACCUMULATE_ENABLE);

	mutex_unlock(&g_dpm_venc_state_lock);

	return len;
}

static int vcodec_dpm_venc_get_counter_power(int mode)
{
	int len;
	unsigned long long *dpm_ctable = g_dpm_venc_ops.dpm_counter_table;
	unsigned long long *dpm_ptable = g_dpm_venc_ops.dpm_power_table;

	if (!dpm_ctable || !dpm_ptable) {
		VCODEC_ERR_VENC("dpm_counter_table is NULL");
		return -EINVAL;
	}

	mutex_lock(&g_dpm_venc_state_lock);
	if (!g_dpm_venc_inited) {
		VCODEC_INFO_VENC("venc dpm is disabled");
		mutex_unlock(&g_dpm_venc_state_lock);
		return 0;
	}

	writel(BIT(SOC_DPMONITOR_SOFT_PULSE_soft_pulse_START),
		SOC_DPMONITOR_SOFT_PULSE_ADDR(g_venc_dpm_regulator.dpm_venc_base));

	udelay(1);

	len = dpm_hwmon_update_counter_power(g_venc_dpm_regulator.dpm_venc_base,
		dpm_ctable, DPM_LOAD_NUM, dpm_ptable, mode);

	mutex_unlock(&g_dpm_venc_state_lock);

	return len;
}

static int vcodec_dpm_venc_update_counter()
{
	return vcodec_dpm_venc_get_counter_power(ACCUMULATE_ENABLE);
}

#ifdef CONFIG_DPM_HWMON_DEBUG
static int vcodec_dpm_venc_get_counter_for_fitting(int mode)
{
	return vcodec_dpm_venc_get_counter_power(mode);
}
#endif

void venc_dpm_freq_select(venc_clk_t clk_type)
{
	unsigned int val;
	if (!g_dpm_venc_inited) {
		VCODEC_DBG_VENC("dpm_venc is disabled");
		return;
	}

	val = readl(venc_dpm_freq_sel(g_venc_dpm_regulator.venc_smmu_pre_base));
	writel((val & VENC_FREQ_CLEAR_MASK) | clk_type, venc_dpm_freq_sel(g_venc_dpm_regulator.venc_smmu_pre_base));
}

void venc_dpm_init(struct venc_config *venc_config)
{
	int ret;

	if (!g_dpm_venc_ops.module_enabled) {
		VCODEC_DBG_VENC("dpm_venc is not enabled");
		return;
	}

	if (g_dpm_venc_inited) {
		VCODEC_INFO_VENC("dpm_venc is already init");
		return;
	}

	ret = dpm_venc_enable(venc_config);
	if (ret != 0) {
		VCODEC_ERR_VENC("dpm_venc enable failed");
		return;
	}

	mutex_lock(&g_dpm_venc_state_lock);
	g_dpm_venc_inited = true;
	mutex_unlock(&g_dpm_venc_state_lock);
	VCODEC_DBG_VENC("dpm_venc is successfully init");
}

void venc_dpm_deinit(void)
{
	if (!g_dpm_venc_inited) {
		VCODEC_DBG_VENC("dpm_venc is not init");
		return;
	}

	vcodec_dpm_venc_update_power();

	mutex_lock(&g_dpm_venc_state_lock);
	g_dpm_venc_inited = false;
	mutex_unlock(&g_dpm_venc_state_lock);

	dpm_venc_disable();

	VCODEC_DBG_VENC("dpm_venc is successfully deinit");
}

static int __init dpm_venc_init(void)
{
	dpm_hwmon_register(&g_dpm_venc_ops);
	return 0;
}

static void __exit dpm_venc_exit(void)
{
	dpm_hwmon_unregister(&g_dpm_venc_ops);
}

module_init(dpm_venc_init);
module_exit(dpm_venc_exit);
