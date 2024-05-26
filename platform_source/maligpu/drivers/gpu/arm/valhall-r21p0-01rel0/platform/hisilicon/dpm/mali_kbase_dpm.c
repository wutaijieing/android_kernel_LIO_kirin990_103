/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: This file describe HISI GPU DPM&PCR feature.
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2019-10-1
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/io.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/platform_drivers/dpm_hwmon.h>
#include "mali_kbase_dpm.h"
#include "mali_kbase_pcr.h"
#include "mali_kbase_hisilicon.h"
#include "mali_kbase_config_platform.h"

void __iomem *g_dpm_reg;
struct hisi_gpu_dpm *g_dpm_ctrl;

static int dpm_update_power(void);
static int dpm_update_counter_and_power(void);
static void read_counter_and_power(unsigned long long *counter_table,
	unsigned long long *power_table, bool is_cumulative);
#ifdef CONFIG_DPM_HWMON_DEBUG
static int dpm_get_counter_for_fitting(int mode);
#endif

/* calculate dpm module virtual address according to core_id */
static void __iomem *calculate_dpm_addr(struct kbase_device *kbdev,
		unsigned int core_id)
{
	void __iomem *module_dpm_addr = NULL;
	/*
	 * for borr platform: top dpm:core_id=0; core dpm: core_id=1~24;
	 * for trym platform: top dpm:core_id=0; core dpm: core_id=1~8;
	 */
	if (core_id != 0) /* core dpm virtual address */
		module_dpm_addr = kbdev->hisi_dev_data.dpm_ctrl.dpmreg +
			DPM_GPU_CORE_OFFSET +
				(core_id - 1) * DPM_GPU_CORE_OFFSET_STEP;
	else /* top dpm virtual address */
		module_dpm_addr = kbdev->hisi_dev_data.dpm_ctrl.dpmreg +
			DPM_GPU_TOP_OFFSET;
	return module_dpm_addr;
}

static void dpm_gpu_monitor_enable(struct kbase_device *kbdev,
	unsigned int core_id)
{
	unsigned int i;
	void __iomem *module_dpm_addr = NULL;

	if (core_id > GPU_DPM_NUM_MAX) {
		pr_err("%s: wrong core_id\n", __func__);
		return;
	}
	module_dpm_addr = calculate_dpm_addr(kbdev, core_id);
	/* step 5: no use */
	/* step 6: sync_cg_off bit17 set 1, bit1 set 0 */
	writel(SYNC_CG_OFF_ENABLE, DPM_CTRL_EN_ADDR(module_dpm_addr));

	/* step 7: config sig 0~15_power_param[23:0] */
	for (i = 0; i < DPM_SIG_AND_CONST_POWER_PARA_NUM; i++)
		writel(POWER_PARAM, DPM_POWER_PARAM0_ADDR(
			module_dpm_addr + NUM_OF_BYTES * i));

	/* step 8:  config volt0~7_param[9:0],freq0~7_param[9:0] */
	for (i = 0; i < DPM_VOLT_AND_FREQ_PARA_NUM; i++)
		writel(VOLT_PARAM, DPM_VOLT_PARAM_ADDR(module_dpm_addr +
			NUM_OF_BYTES * i));

	for (i = 0; i < DPM_VOLT_AND_FREQ_PARA_NUM; i++)
		writel(FREQ_PARAM, DPM_FREQ_PARAM_ADDR(module_dpm_addr +
			NUM_OF_BYTES * i));

	/* step 9: config sig_lev_mode */
	if (core_id == 0)
		writel(SIG_LEV_MODE_LEVEL_TOP,
			DPM_SIG_LEV_MODE_ADDR(module_dpm_addr));
	else
		writel(SIG_LEV_MODE_LEVEL_CORE,
			DPM_SIG_LEV_MODE_ADDR(module_dpm_addr));

	/* step 10,11 no use */

	/* step 12:monitor_sensor enable */
	writel(ALL_MONITOR_SENSOR_ENABLE, DPM_SENSOR_EN_ADDR(module_dpm_addr));

	/* step 13: monitor_ctrl enable */
	writel(MONITOR_CTRL_ENABLE, DPM_CTRL_EN_ADDR(module_dpm_addr));
}

static void dpm_gpu_monitor_disable(struct kbase_device *kbdev,
	unsigned int core_id)
{
	void __iomem *module_dpm_addr = NULL;

	if (core_id > GPU_DPM_NUM_MAX) {
		pr_err("%s: wrong core_id\n", __func__);
		return;
	}

	module_dpm_addr = calculate_dpm_addr(kbdev, core_id);
	/* step end: monitor_ctrl_en disable bit16 set1, bit0 set 0 */
	writel(MONITOR_CTRL_DISABLE, DPM_CTRL_EN_ADDR(module_dpm_addr));
}

/*
 * dpm_gpu_read_register_value - Read dpm counter registers and power registers.
 *
 * @core_id:     Gpu shader core ID(0,top core; 1~24,shader core)
 * @counter_buff:The address to write counter register values when it's not NULL
 * @power_buff:  The address to write power register values when it's not NULL
 *
 * @is_cumulative:True, add counter and power register values to counter_buff
 *                      and power_buff
 *                False, update counter_buff and power_buff with counter and
 *                       power register values
 */
static unsigned int dpm_gpu_read_register_value(unsigned int core_id,
	unsigned long long *counter_buff, unsigned long long *power_buff,
	bool is_cumulative)
{
	unsigned int i;
	unsigned int val;
	void __iomem *module_dpm_addr = NULL;

	if (core_id > GPU_DPM_NUM_MAX) {
		pr_err("%s: wrong core_id\n", __func__);
		return DPM_FALSE;
	}
	if (core_id != 0)
		module_dpm_addr = g_dpm_reg + DPM_GPU_CORE_OFFSET +
			(core_id - 1) * DPM_GPU_CORE_OFFSET_STEP;
	else
		module_dpm_addr = g_dpm_reg + DPM_GPU_TOP_OFFSET;

	mutex_lock(&g_dpm_ctrl->gpu_power_lock);
	if (g_dpm_ctrl->dpm_init_status != true) {
		mutex_unlock(&g_dpm_ctrl->gpu_power_lock);
		return DPM_FALSE;
	}

	writel(SOFT_SAMPLE_PULSE, DPM_SOFT_SAMPLE_PULSE_ADDR(module_dpm_addr));
	udelay(PULSE_DELAY_TIME);

	if (counter_buff != NULL) {    /* read dpm counter registers */
		for (i = 0; i < DPM_BUSY_COUNT_NUM; i++) {
			val = (unsigned int)readl(DPM_BUSY_CNT0_ADDR(
				module_dpm_addr + NUM_OF_BYTES * i));

			if (is_cumulative)
				counter_buff[i] += val;
			else
				counter_buff[i] = val;
		}
	}

	if (power_buff != NULL) {    /* read dpm power registers */
		val = (unsigned int)readl(
			DPM_ACC_ENERGY_ADDR(module_dpm_addr));
		if (is_cumulative)
			*power_buff += val;
		else
			*power_buff = val;
	}
	mutex_unlock(&g_dpm_ctrl->gpu_power_lock);
	return DPM_OK;
}

struct dpm_hwmon_ops gpu_hwmon_op = {
	.dpm_module_id = DPM_GPU_ID,
	.dpm_type = DPM_GPU_MODULE,
	.hi_dpm_update_power = dpm_update_power,
	.hi_dpm_update_counter = dpm_update_counter_and_power,
#ifdef CONFIG_DPM_HWMON_DEBUG
	.hi_dpm_get_counter_for_fitting = dpm_get_counter_for_fitting,
#endif
};

#ifdef CONFIG_DPM_HWMON_DEBUG
static int dpm_get_counter_for_fitting(int mode)
{
	unsigned long long *dpm_gpu_counter_table =
		gpu_hwmon_op.dpm_counter_table;
	unsigned long long *dpm_gpu_power_table =
		gpu_hwmon_op.dpm_power_table;

	if (dpm_gpu_counter_table == NULL || dpm_gpu_power_table == NULL) {
		pr_err("%s: dpm_gpu_counter_table or dpm_gpu_power_table is NULL\n",
				__func__);
		return DPM_FALSE;
	}

	if (mode)
		read_counter_and_power(
			dpm_gpu_counter_table, dpm_gpu_power_table, true);
	else
		read_counter_and_power(
			dpm_gpu_counter_table, dpm_gpu_power_table, false);

	return DPM_OK;
}
#endif

/* base function to read counter and power */
static void read_counter_and_power(unsigned long long *counter_table,
	unsigned long long *power_table, bool is_cumulative)
{
	unsigned int i;
	unsigned int ret;

	for (i = 0; i <= GPU_DPM_NUM_MAX; i++) {
		ret = dpm_gpu_read_register_value(i, counter_table,
			power_table, is_cumulative);
		if (!ret) /* gpu power down */
			return;

		if (counter_table != NULL)
			counter_table += DPM_BUSY_COUNT_NUM;

		if (power_table != NULL)
			power_table += DPM_ACC_ENERGY_NUM;
	}
	return;
}

/* interface for framework calling */
static int dpm_update_power(void)
{
	unsigned long long *dpm_gpu_power_table =
		gpu_hwmon_op.dpm_power_table;

	if (!g_dpm_report_enabled)
		return DPM_FALSE;

	if (dpm_gpu_power_table == NULL) {
		pr_err("%s: dpm_power_table is NULL\n", __func__);
		return DPM_FALSE;
	}
	read_counter_and_power(NULL, dpm_gpu_power_table, true);
	return DPM_OK;
}

/* interface for framework calling */
static int dpm_update_counter_and_power(void)
{
	unsigned long long *dpm_gpu_counter_table =
		gpu_hwmon_op.dpm_counter_table;
	unsigned long long *dpm_gpu_power_table =
		gpu_hwmon_op.dpm_power_table;

	if (dpm_gpu_counter_table == NULL || dpm_gpu_power_table == NULL) {
		pr_err("%s: dpm_gpu_counter_table or dpm_gpu_power_table is NULL\n",
			__func__);
		return DPM_FALSE;
	}
	read_counter_and_power(dpm_gpu_counter_table,
		dpm_gpu_power_table, true);
	return DPM_OK;
}

/* map dpmpcr related registers */
int dpm_pcr_register_map(struct kbase_device *kbdev)
{
	int err = 0;

	kbdev->hisi_dev_data.dpm_ctrl.dpmreg =
		ioremap(SYS_REG_GPUPCR_BASE_ADDR, DPM_PCR_REMAP_SIZE);
	if (!kbdev->hisi_dev_data.dpm_ctrl.dpmreg) {
		pr_err("%s: Can't remap dpm register window on platform\n",
			__func__);
		err = -EINVAL;
		return err;
	}
	g_dpm_reg = kbdev->hisi_dev_data.dpm_ctrl.dpmreg;

	return err;
}

/* unmap dpmpcr related registers */
void dpm_pcr_register_unmap(struct kbase_device *kbdev)
{
	if (kbdev->hisi_dev_data.dpm_ctrl.dpmreg != NULL)
		iounmap(kbdev->hisi_dev_data.dpm_ctrl.dpmreg);
}

/* init dpmpcr related resources */
void dpm_pcr_resource_init(struct kbase_device *kbdev)
{
	unsigned int i;
	unsigned int num_of_report_cores = 0;

	if (dpm_pcr_register_map(kbdev) != 0)
		pr_err("%s: dpm_pcr_register map failed\n", __func__);
	g_dpm_ctrl = &kbdev->hisi_dev_data.dpm_ctrl;
	/*
	 * calculate how many dpm cores to report
	 * according to macro REPORT_CORE_MASK.
	 */
	for (i = 0; i <= GPU_DPM_NUM_MAX; i++) {
		if ((1U << i) & (REPORT_CORE_MASK))
			num_of_report_cores++;
	}
	/* specify dpm_counter_table_size */
	gpu_hwmon_op.dpm_cnt_len = num_of_report_cores * DPM_BUSY_COUNT_NUM;

	/* specify dpm_power_table size */
	gpu_hwmon_op.dpm_power_len = num_of_report_cores;

	mutex_init(&kbdev->hisi_dev_data.dpm_ctrl.gpu_power_lock);

	/* register struct gpu_hwmon_op to top level framework */
	if (dpm_hwmon_register(&gpu_hwmon_op)) {
		kbdev->hisi_dev_data.dpm_ctrl.dpm_register_status = false;
		pr_err("%s: dpm_pcr struct gpu_hwmon_op register failed\n",
			__func__);
	} else {
		kbdev->hisi_dev_data.dpm_ctrl.dpm_register_status = true;
	}

	/* dpm module has no been inited yet */
	kbdev->hisi_dev_data.dpm_ctrl.dpm_init_status = false;
}

void dpm_pcr_resource_exit(struct kbase_device *kbdev)
{
	/* unregister struct gpu_hwmon_op to top level framework */
	dpm_hwmon_unregister(&gpu_hwmon_op);
	dpm_pcr_register_unmap(kbdev);
}

void dpm_pcr_enable(struct kbase_device *kbdev)
{
	unsigned int i;
	bool flag_for_bypass = true;

	if (!gpu_hwmon_op.module_enabled ||
		!kbdev->hisi_dev_data.dpm_ctrl.dpm_register_status)
		return;

	for (i = 0; i <= GPU_DPM_NUM_MAX; i++) {
		if ((1U << i) & (REPORT_CORE_MASK))
			dpm_gpu_monitor_enable(kbdev, i);
	}
	gpu_pcr_enable(kbdev, flag_for_bypass);

	mutex_lock(&kbdev->hisi_dev_data.dpm_ctrl.gpu_power_lock);
	kbdev->hisi_dev_data.dpm_ctrl.dpm_init_status = true;
	mutex_unlock(&kbdev->hisi_dev_data.dpm_ctrl.gpu_power_lock);
}

void dpm_pcr_disable(struct kbase_device *kbdev)
{
	unsigned int i;

	mutex_lock(&kbdev->hisi_dev_data.dpm_ctrl.gpu_power_lock);
	if (kbdev->hisi_dev_data.dpm_ctrl.dpm_init_status == false) {
		mutex_unlock(&kbdev->hisi_dev_data.dpm_ctrl.gpu_power_lock);
		return;
	}
	/*
	 * unlock before dpm_update_power
	 * because dpm_update_power will get lock
	 */
	mutex_unlock(&kbdev->hisi_dev_data.dpm_ctrl.gpu_power_lock);
	/* update dpm counter, then disable dpm */
	dpm_update_power();
	kbdev->hisi_dev_data.dpm_ctrl.dpm_init_status = false;

	for (i = 0; i <= GPU_DPM_NUM_MAX; i++) {
		if ((1U << i) & (REPORT_CORE_MASK))
			dpm_gpu_monitor_disable(kbdev, i);
	}
	gpu_pcr_disable(kbdev);
}

