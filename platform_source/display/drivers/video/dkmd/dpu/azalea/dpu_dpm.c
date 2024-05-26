/* Copyright (c) 2019-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include "dpu_dpm.h"
#include <asm/compiler.h>
#include <linux/compiler.h>
#include <linux/mutex.h>
#include <linux/of_device.h>
#include <linux/platform_drivers/dpm_hwmon.h>

#define DSS_DPM_TAG "[dss_dpm]"
/*
 * dpm load has 3 types: low load, nor load, high load
 * each type load have 16 registers from LOAD0 to LOAD15
 * dpm also has 4 pclk load
 */
#define DSS_DPM_TYPE 2 /* disp and part0 */
#define DPM_LOAD_NUM 16
#define DPM_LOAD_TYPE_NUM 3
#define DPM_PCLK_LOAD_NUM 4
#define DISP_READ_REG_NUM (DPM_LOAD_TYPE_NUM * DPM_LOAD_NUM)
#define PART0_READ_REG_NUM (DPM_LOAD_TYPE_NUM * DPM_LOAD_NUM)

#define PERI_VOLTAGE_060V 600
#define PERI_VOLTAGE_065V 650
#define PERI_VOLTAGE_070V 700
#define PERI_VOLTAGE_080V 800

#define DSS_DPM_TIMER_SEC (DSS_DPM_CHECK_PERIOD / 1000)
#define DSS_DPM_TIMER_NSEC ((DSS_DPM_CHECK_PERIOD % 1000) * 1000000)

/* DPMONITOR_SOFT_SAMPLE_PULSE_ADDR */
#define soft_sample_pulse_addr(base) ((base) + 0x0E8)

/* DPMONITOR_LOW_LOAD0_ADDR */
#define low_load0_addr(base, offset) (((base) + 4 * (offset)) + 0x014)

#define core_clk_sel_addr(base) ((base) + 0x34)
#define core_clk_en_addr(base) ((base) + 0x38)
#define dss_glb_clk_sel(base) ((base) + 0x300)
#define dss_glb_clk_en(base) ((base) + 0x304)

#if defined(CONFIG_DPU_FB_V600)
#define SOC_ACPU_DSS_DISP_BASE 0xE89A1000
#define SOC_ACPU_DPM_DSS_DISP_BASE 0xE89BF000
#define SOC_ACPU_DSS_PART0_BASE 0xE8912000
#define SOC_ACPU_DPM_DSS_PART0_BASE 0xE8961000
#define DSS_DISP_SIGNAL_MODE 0x3043
#define DSS_PART0_SIGNAL_MODE 0xE52D
#else
#define SOC_ACPU_DSS_DISP_BASE 0xE84A1000
#define SOC_ACPU_DPM_DSS_DISP_BASE 0xE84BF000
#define SOC_ACPU_DSS_PART0_BASE 0xE8412000
#define SOC_ACPU_DPM_DSS_PART0_BASE 0xE8461000
/* DSS_DISP_DPMONITOR_SIGNAL_MODE */
#define DSS_DISP_SIGNAL_MODE 0x158D
/* DSS_PART0_DPMONITOR_SIGNAL_MODE */
#define DSS_PART0_SIGNAL_MODE 0x1532
#endif

void __iomem *g_dss_disp_base; /* virtual address */
void __iomem *g_dpm_dss_disp_base; /* virtual address */
void __iomem *g_dss_part0_base; /* virtual address */
void __iomem *g_dpm_dss_part0_base; /* virtual address */

static DEFINE_MUTEX(g_dpm_dss_state_lock);

enum {
	NO_ACCUMULATION = 0,
	ACCUMULATION = 1,
};

static struct dpu_dpm g_dpm_ctrl = {
	.dpm_inited = false
};

#ifdef CONFIG_DPM_HWMON_V2
static unsigned int g_disp_dpm_monitor_param[] = {
	0xB8, 0x471, 0xC2, 0x1E6, 0x6A2, 0x1CAA, 0x2DB, 0x17B4,
	0x5, 0x1611, 0xC6, 0x25AA, 0x12B, 0x0, 0x11F, 0x0,
	0,
	0x242, 0x242, 0x200, 0x1C2, 0x188, 0x152, 0x120, 0xF2,
	0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200
};
static unsigned int g_part0_dpm_monitor_param[] = {
	0xA6, 0x350, 0xB9, 0x1E2, 0x744, 0x1600, 0x313, 0xF70,
	0x5, 0x17C1, 0xCF, 0x23A3, 0x138, 0x2BA0, 0x125, 0x1D0A,
	0,
	0x242, 0x242, 0x200, 0x1C2, 0x188, 0x152, 0x120, 0xF2,
	0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200
};

#else
/* dss dpm disp/part0 fitting ratio, raw_data * 1000 */
static int g_dpm_disp_ratio_table[DPM_LOAD_NUM] = {
	0, 15672, 0, 0, 173351, 11944, 82298, 0,
	0, 9756, 0, 58192, 0, 124984, 38967, 42997
};

static int g_dpm_part0_ratio_table[DPM_LOAD_NUM] = {
	409204, 0, 32488, 318405, 0, 0, 74434, 96224,
	0, 34132, 0, 117426, 0, 58810, 51883, 228737
};
#endif

#ifdef CONFIG_DPM_HWMON_DEBUG
static int dpm_dss_debug_get_counter(int mode);
#endif
static int dpm_dss_update_counter(void);

#ifdef CONFIG_DPM_HWMON_V2
static int dpm_dss_update_power(void);
#else
static int dpm_dss_fitting_coff(void);
static int dpm_dss_get_voltage(void);
#endif

static struct dpm_hwmon_ops dpm_dss_disp_ops = {
	.dpm_module_id = DPM_DSS_ID,
	.dpm_type = DPM_PERI_MODULE,
#ifdef CONFIG_DPM_HWMON_V2
	.dpm_cnt_len = DPM_LOAD_NUM * DSS_DPM_TYPE,
	.hi_dpm_update_counter = dpm_dss_update_counter,
	.dpm_power_len = DSS_DPM_TYPE,
	.hi_dpm_update_power = dpm_dss_update_power,
#else /* CONFIG_DPM_HWMON_V1 */
	/* dpm_counter_table size */
	.dpm_cnt_len = (DISP_READ_REG_NUM + PART0_READ_REG_NUM),
	/* dpm_fitting_table size */
	.dpm_fit_len = (DISP_READ_REG_NUM + PART0_READ_REG_NUM),
	.hi_dpm_fitting_coff = dpm_dss_fitting_coff,
	.hi_dpm_update_counter = dpm_dss_update_counter,
	/* compensate_coff: 67791 * 1000000 / (809 * 809) */
	.compensate_coff = 103580,
	.hi_dpm_get_voltage = dpm_dss_get_voltage,
#endif
#ifdef CONFIG_DPM_HWMON_DEBUG
	.hi_dpm_get_counter_for_fitting = dpm_dss_debug_get_counter,
#endif
};

static void dss_disp_clk_open(void)
{
	unsigned int val;

	/* module clk enable */
	val = readl(core_clk_sel_addr(g_dss_disp_base));
	writel(val | BIT(10), core_clk_sel_addr(g_dss_disp_base));
	val = readl(core_clk_en_addr(g_dss_disp_base));
	writel(val | BIT(10), core_clk_en_addr(g_dss_disp_base));
}

static void dss_disp_clk_close(void)
{
	unsigned int val;

	/* module clk off */
	val = readl(core_clk_sel_addr(g_dss_disp_base));
	writel(val & ~BIT(10), core_clk_sel_addr(g_dss_disp_base));
	val = readl(core_clk_en_addr(g_dss_disp_base));
	writel(val & ~BIT(10), core_clk_en_addr(g_dss_disp_base));
}

static void dpm_dss_disp_on(void)
{
	dss_disp_clk_open();

#ifdef CONFIG_DPM_HWMON_V2
	dpm_monitor_enable(g_dpm_dss_disp_base, DSS_DISP_SIGNAL_MODE,
		g_disp_dpm_monitor_param, ARRAY_SIZE(g_disp_dpm_monitor_param));
#else
	dpm_monitor_enable(g_dpm_dss_disp_base, DSS_DISP_SIGNAL_MODE);
#endif

	return;
}

static void dpm_dss_disp_off(void)
{
	dpm_monitor_disable(g_dpm_dss_disp_base);
	dss_disp_clk_close();
}

static void dss_part0_clk_open(void)
{
	unsigned int val;

	/* module clk enable */
	val = readl(dss_glb_clk_sel(g_dss_part0_base));
	writel(val | BIT(29), dss_glb_clk_sel(g_dss_part0_base));
	val = readl(dss_glb_clk_en(g_dss_part0_base));
	writel(val | BIT(29), dss_glb_clk_en(g_dss_part0_base));
}

static void dss_part0_clk_close(void)
{
	unsigned int val;

	/* module clk off */
	val = readl(dss_glb_clk_sel(g_dss_part0_base));
	writel(val & ~BIT(29), dss_glb_clk_sel(g_dss_part0_base));
	val = readl(dss_glb_clk_en(g_dss_part0_base));
	writel(val & ~BIT(29), dss_glb_clk_en(g_dss_part0_base));
}

static void dpm_dss_part0_on(void)
{
	dss_part0_clk_open();
#ifdef CONFIG_DPM_HWMON_V2
	dpm_monitor_enable(g_dpm_dss_part0_base, DSS_PART0_SIGNAL_MODE,
		g_part0_dpm_monitor_param, ARRAY_SIZE(g_part0_dpm_monitor_param));
#else
	dpm_monitor_enable(g_dpm_dss_part0_base, DSS_PART0_SIGNAL_MODE);
#endif

	return;
}

static void dpm_dss_part0_off(void)
{
	dpm_monitor_disable(g_dpm_dss_part0_base);
	dss_part0_clk_close();
}

static bool is_dpm_dss_on(void)
{
	bool state = false;

	mutex_lock(&g_dpm_dss_state_lock);
	state = g_dpm_ctrl.dpm_inited;
	mutex_unlock(&g_dpm_dss_state_lock);
	return state;
}

static void set_dpm_dss_on(bool state)
{
	mutex_lock(&g_dpm_dss_state_lock);
	g_dpm_ctrl.dpm_inited = state;
	mutex_unlock(&g_dpm_dss_state_lock);
}

#ifdef CONFIG_DPM_HWMON_V2

typedef int (*call_interface_handle)(bool only_power, int mode);

static int dpm_dss_update_counter_power(bool only_power, int mode)
{
	unsigned int len;
	unsigned long long *dpm_ctable = dpm_dss_disp_ops.dpm_counter_table;
	unsigned long long *dpm_ptable = dpm_dss_disp_ops.dpm_power_table;

	dpu_check_and_return(!dpm_ctable || !dpm_ptable, 0, ERR, "dpm_table is nullptr\n");

	if (only_power) {
		len = dpm_hwmon_update_power(g_dpm_dss_disp_base,
			dpm_ptable, mode);

		len += dpm_hwmon_update_power(g_dpm_dss_part0_base,
			dpm_ptable + 1, mode);
	} else {
		len = dpm_hwmon_update_counter_power(g_dpm_dss_disp_base,
			dpm_ctable, DPM_LOAD_NUM, dpm_ptable, mode);

		len += dpm_hwmon_update_counter_power(g_dpm_dss_part0_base,
			dpm_ctable + DPM_LOAD_NUM, DPM_LOAD_NUM, dpm_ptable + 1, mode);
	}

	return len;
}

static int dpm_dss_get_counter_power(call_interface_handle call_interface, bool only_power, int mode)
{
	unsigned int len;
	struct dpufb_vsync *vsync_ctrl = NULL;

	/* if power is off, direct return
	 * if power is on, dss should wait dpm done before power off
	 */
	mutex_lock(&g_dpm_dss_state_lock);
	if (!g_dpm_ctrl.dpm_inited) {
		mutex_unlock(&g_dpm_dss_state_lock);
		return 0;
	}

	if (!g_dpm_ctrl.dpufd) {
		DPU_FB_ERR("%s dpufd is null\n", DSS_DPM_TAG);
		mutex_unlock(&g_dpm_dss_state_lock);
		return 0;
	}

	vsync_ctrl = &(g_dpm_ctrl.dpufd->vsync_ctrl);
	if (mutex_trylock(&(vsync_ctrl->vsync_lock)) == 0) {
		DPU_FB_INFO("%s dss is doing vsync\n", DSS_DPM_TAG);
		mutex_unlock(&g_dpm_dss_state_lock);
		return 0;
	}

	if (g_dpm_ctrl.dpufd->enter_idle) {
		DPU_FB_NOTICE("%s dss has entered idle\n", DSS_DPM_TAG);
		mutex_unlock(&(vsync_ctrl->vsync_lock));
		mutex_unlock(&g_dpm_dss_state_lock);
		return 0;
	}

	/* write pulse */
	writel(BIT(SOC_DPMONITOR_SOFT_PULSE_soft_pulse_START),
		SOC_DPMONITOR_SOFT_PULSE_ADDR(g_dpm_dss_disp_base));
	writel(BIT(SOC_DPMONITOR_SOFT_PULSE_soft_pulse_START),
		SOC_DPMONITOR_SOFT_PULSE_ADDR(g_dpm_dss_part0_base));

	udelay(2);  /* 2us */

	len = call_interface(only_power, mode);
	mutex_unlock(&(vsync_ctrl->vsync_lock));
	mutex_unlock(&g_dpm_dss_state_lock);

	DPU_FB_INFO("%s get reg value successfully\n", DSS_DPM_TAG);

	return len;
}

#ifdef CONFIG_DPM_HWMON_DEBUG
static int dpm_dss_debug_get_counter(int mode)
{
	return dpm_dss_get_counter_power(dpm_dss_update_counter_power, false, mode);
}
#endif

static int dpm_dss_update_counter(void)
{
	return dpm_dss_get_counter_power(dpm_dss_update_counter_power, false, ACCUMULATION);
}

static int dpm_dss_update_power(void)
{
	return dpm_dss_get_counter_power(dpm_dss_update_counter_power, true, ACCUMULATION);
}

#else  /* CONFIG_DPM_HWMON_V1 */

/* needn't input buffer_size for efficiency */
static void dpm_dss_fill_val_table(unsigned long long *buffer,
	unsigned int *offset, unsigned int val, bool is_update)
{
	if (is_update)
		buffer[*offset] += val;
	else
		buffer[*offset] = val;

	(*offset)++;
}

static unsigned int dpm_dss_get_reg_val(unsigned long long *buffer,
	unsigned int buffer_size, bool is_update)
{
	unsigned int i;
	unsigned int val;
	unsigned int len = 0;
	struct dpufb_vsync *vsync_ctrl = NULL;

	/* check register size ready to read */
	if (buffer_size < (DISP_READ_REG_NUM + PART0_READ_REG_NUM)) {
		DPU_FB_ERR("%s buffer will overflow\n", DSS_DPM_TAG);
		return 0;
	}

	/* if power is off, direct return
	 * if power is on, dss should wait dpm done before power off
	 */
	mutex_lock(&g_dpm_dss_state_lock);
	if (!g_dpm_ctrl.dpm_inited) {
		mutex_unlock(&g_dpm_dss_state_lock);
		return 0;
	}

	if (!g_dpm_ctrl.dpufd) {
		DPU_FB_ERR("%s dpufd is null\n", DSS_DPM_TAG);
		mutex_unlock(&g_dpm_dss_state_lock);
		return 0;
	}

	vsync_ctrl = &(g_dpm_ctrl.dpufd->vsync_ctrl);
	if (mutex_trylock(&(vsync_ctrl->vsync_lock)) == 0) {
		DPU_FB_INFO("%s dss is doing vsync\n", DSS_DPM_TAG);
		mutex_unlock(&g_dpm_dss_state_lock);
		return 0;
	}

	if (g_dpm_ctrl.dpufd->enter_idle) {
		DPU_FB_NOTICE("%s dss has entered idle\n", DSS_DPM_TAG);
		mutex_unlock(&(vsync_ctrl->vsync_lock));
		mutex_unlock(&g_dpm_dss_state_lock);
		return 0;
	}

	/* low normal high voltage 0-15 */
	writel(0x1, soft_sample_pulse_addr(g_dpm_dss_disp_base));
	writel(0x1, soft_sample_pulse_addr(g_dpm_dss_part0_base));

	/* delay for write address take effect */
	udelay(1);

	/* read load data */
	for (i = 0; i < DISP_READ_REG_NUM; i++) {
		val = readl(low_load0_addr(g_dpm_dss_disp_base, i));
		dpm_dss_fill_val_table(buffer, &len, val, is_update);
	}

	for (i = 0; i < PART0_READ_REG_NUM; i++) {
		val = readl(low_load0_addr(g_dpm_dss_part0_base, i));
		dpm_dss_fill_val_table(buffer, &len, val, is_update);
	}
	mutex_unlock(&(vsync_ctrl->vsync_lock));
	mutex_unlock(&g_dpm_dss_state_lock);
	DPU_FB_NOTICE("%s get reg value successfully\n", DSS_DPM_TAG);

	return len;
}

static int dpm_dss_get_voltage(void)
{
	int voltage_value = 0;

	/*
	 * if power is off, direct return
	 * if power is on, dss should wait dpm done before power off
	 */
	mutex_lock(&g_dpm_dss_state_lock);
	if (!g_dpm_ctrl.dpm_inited) {
		mutex_unlock(&g_dpm_dss_state_lock);
		return 0;
	}

	dss_get_peri_volt(&voltage_value);
	mutex_unlock(&g_dpm_dss_state_lock);

	switch (voltage_value) {
	case PERI_VOLTAGE_LEVEL0:
		return PERI_VOLTAGE_060V;
	case PERI_VOLTAGE_LEVEL1:
		return PERI_VOLTAGE_065V;
	case PERI_VOLTAGE_LEVEL2:
		return PERI_VOLTAGE_070V;
	case PERI_VOLTAGE_LEVEL3:
		return PERI_VOLTAGE_080V;
	default:
		DPU_FB_ERR("%s unsupport vol is %d\n", DSS_DPM_TAG, voltage_value);
		return -1;
	}
}

#ifdef CONFIG_DPM_HWMON_DEBUG
static int dpm_dss_debug_get_counter(int mode)
{
	bool type = (mode == 0) ? false : true;
	unsigned long long *dpm_table = dpm_dss_disp_ops.dpm_counter_table;
	unsigned int table_size = dpm_dss_disp_ops.dpm_cnt_len;
	unsigned int len;

	DPU_FB_NOTICE("%s debug get counter enter\n", DSS_DPM_TAG);

	dpu_check_and_return(!dpm_table, 0, ERR, "%s dpm_counter_table is NULL\n", DSS_DPM_TAG);
	len = dpm_dss_get_reg_val(dpm_table, table_size, type);
	if (len < DPM_BUFFER_SIZE)
		memcpy(g_dpm_buffer_for_fitting, dpm_table, len * sizeof(unsigned long long));
	else
		DPU_FB_ERR("%s memcpy g_dpm_buffer_for_fitting fail\n", DSS_DPM_TAG);

	return len;
}
#endif

static unsigned int dpm_dss_fill_ratio_table(int *fitting_table, unsigned int fit_size,
	const int *ratio_table, unsigned int ratio_size)
{
	unsigned int offset = 0;
	const unsigned int copy_len = DPM_LOAD_NUM * DPM_LOAD_TYPE_NUM;
	unsigned int load;
	unsigned int load_type;

	if ((fit_size < copy_len) || (ratio_size < DPM_LOAD_NUM)) {
		DPU_FB_ERR("%s fit_size or ratio_size is invalid\n", DSS_DPM_TAG);
		return 0;
	}

	for (load_type = 0; load_type < DPM_LOAD_TYPE_NUM; load_type++) {
		for (load = 0; load < DPM_LOAD_NUM; load++) {
			*(fitting_table + offset) = *(ratio_table + load);
			offset++;
		}
	}

	return offset;
}

static int dpm_dss_fitting_coff(void)
{
	int *fitting_table = dpm_dss_disp_ops.dpm_fitting_table;
	unsigned int len = dpm_dss_disp_ops.dpm_fit_len;
	unsigned int offset;
	unsigned int i;

	if (!fitting_table) {
		DPU_FB_ERR("%s dpm_fitting_table is NULL\n", DSS_DPM_TAG);
		return 0;
	}

	for (i = 0; i < len; i++)
		fitting_table[i] = 1;

	/*
	 * fill dss dpm disp/part0 ratio
	 * fit table len is DISP_READ_REG_NUM + PART0_READ_REG_NUM
	 * copy len is DSS_DPM_TYPE*DPM_LOAD_TYPE_NUM*DPM_LOAD_NUM
	 * so fit table len > copy len, won't overflow
	 */
	offset = dpm_dss_fill_ratio_table(fitting_table, len, g_dpm_disp_ratio_table, DPM_LOAD_NUM);
	fitting_table += offset;
	dpm_dss_fill_ratio_table(fitting_table, (len - offset), g_dpm_part0_ratio_table, DPM_LOAD_NUM);

	return len;
}

static int dpm_dss_update_counter(void)
{
	unsigned long long *dpm_table = dpm_dss_disp_ops.dpm_counter_table;
	unsigned int table_size = dpm_dss_disp_ops.dpm_cnt_len;
	unsigned int len;

	if (!dpm_table) {
		DPU_FB_ERR("%s dpm_counter_table is NULL\n", DSS_DPM_TAG);
		return 0;
	}

	len = dpm_dss_get_reg_val(dpm_table, table_size, true);

	return len;
}

#endif

/* when power on dss, do dpm_dss_enable() */
static void dpm_dss_enable(void)
{
	dpm_dss_disp_on();
	dpm_dss_part0_on();
}

/* when power off dss, do dpm_dss_disable() */
static void dpm_dss_disable(void)
{
	dpm_dss_disp_off();
	dpm_dss_part0_off();
}

static bool is_dss_dpm_addr_valid(void)
{
	return (g_dss_disp_base && g_dpm_dss_disp_base && g_dss_part0_base && g_dpm_dss_part0_base);
}

void dpu_dpm_init(struct dpu_fb_data_type *dpufd)
{
	if (!dpufd) {
		DPU_FB_ERR("%s dpufd is NULL\n", DSS_DPM_TAG);
		return;
	}

#ifdef CONFIG_DPM_HWMON_V2
	if (!dpm_dss_disp_ops.module_enabled) {
		DPU_FB_DEBUG("%s module_enabled is false\n", DSS_DPM_TAG);
		return;
	}
#endif

	if (is_dpm_dss_on()) {
		DPU_FB_DEBUG("%s dpm_dss is already init\n", DSS_DPM_TAG);
		return;
	}

	if (!is_dss_dpm_addr_valid()) {
		DPU_FB_ERR("%s dss dpm addr is null\n", DSS_DPM_TAG);
		return;
	}
	g_dpm_ctrl.dpufd = dpufd;

	/* enable dpm */
	dpm_dss_enable();

	set_dpm_dss_on(true);
	DPU_FB_NOTICE("%s dpm_dss is successfully init\n", DSS_DPM_TAG);
}

void dpu_dpm_deinit(struct dpu_fb_data_type *dpufd)
{
	if (!dpufd) {
		DPU_FB_ERR("%s dpufd is NULL\n", DSS_DPM_TAG);
		return;
	}

	if (!is_dpm_dss_on()) {
		DPU_FB_DEBUG("%s dpm_dss is not inited\n", DSS_DPM_TAG);
		return;
	}

	/* ready to disable dpm, set flag to be off to avoid read reg */
	set_dpm_dss_on(false);
	dpm_dss_disable();

	DPU_FB_NOTICE("%s dpm_dss is successfully deinit\n", DSS_DPM_TAG);
}

static int __init dpm_dss_init(void)
{
	g_dss_disp_base = ioremap(SOC_ACPU_DSS_DISP_BASE, 0xFFF);  /*lint !e446*/
	g_dpm_dss_disp_base = ioremap(SOC_ACPU_DPM_DSS_DISP_BASE, 0xFFF);  /*lint !e446*/
	g_dss_part0_base = ioremap(SOC_ACPU_DSS_PART0_BASE, 0xFFF);  /*lint !e446*/
	g_dpm_dss_part0_base = ioremap(SOC_ACPU_DPM_DSS_PART0_BASE, 0xFFF);  /*lint !e446*/
	if (!is_dss_dpm_addr_valid()) {
		DPU_FB_ERR("%s dss dpm addr is null\n", DSS_DPM_TAG);
		return -EIO;
	}

	dpm_hwmon_register(&dpm_dss_disp_ops);
	return 0;
}

static void __exit dpm_dss_exit(void)
{
	dpm_hwmon_unregister(&dpm_dss_disp_ops);
}

module_init(dpm_dss_init);
module_exit(dpm_dss_exit);

