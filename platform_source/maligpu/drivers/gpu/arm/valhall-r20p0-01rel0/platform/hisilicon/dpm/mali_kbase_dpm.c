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

#include "mali_kbase_dpm.h"
#include "mali_kbase_pcr.h"
#include "mali_kbase_hisilicon.h"
#include "mali_kbase_config_platform.h"
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/platform_drivers/dpm_hwmon.h>

void __iomem *g_dpm_reg;
struct hisi_gpu_dpm *g_dpm_ctrl;

static int dpm_update_acc_energy(void);

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
		dev_err("%s: wrong core_id\n", __func__);
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
	writel(SIG_LEV_MODE_LEVEL, DPM_SIG_LEV_MODE_ADDR(module_dpm_addr));

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
		dev_err("%s: wrong core_id\n", __func__);
		return;
	}

	module_dpm_addr = calculate_dpm_addr(kbdev, core_id);
	/* step end: monitor_ctrl_en disable bit16 set1, bit0 set 0 */
	writel(MONITOR_CTRL_DISABLE, DPM_CTRL_EN_ADDR(module_dpm_addr));
}

static unsigned int dpm_gpu_get_reg_val(unsigned int core_id,
	unsigned long long *data_buff, bool is_update)
{
	unsigned int i;
	unsigned int val;
	unsigned int ret;
	void __iomem *module_dpm_addr = NULL;

	if (core_id > GPU_DPM_NUM_MAX) {
		dev_err("%s: wrong core_id\n", __func__);
		return 0;
	}
	if (core_id != 0)
		module_dpm_addr = g_dpm_reg + DPM_GPU_CORE_OFFSET +
			(core_id - 1) * DPM_GPU_CORE_OFFSET_STEP;
	else
		module_dpm_addr = g_dpm_reg + DPM_GPU_TOP_OFFSET;

	mutex_lock(&g_dpm_ctrl->gpu_power_lock);

	if (g_dpm_ctrl->dpm_init_status != true) {
		dev_err("%s: read dpm because gpu power down!\n", __func__);
		mutex_unlock(&g_dpm_ctrl->gpu_power_lock);
		return 0;
	}

	writel(SOFT_SAMPLE_PULSE, DPM_SOFT_SAMPLE_PULSE_ADDR(module_dpm_addr));
	udelay(PULSE_DELAY_TIME);
	if (is_update == false) {
		/* read dpm counters for dpm fitting */
		for (i = 0; i < DPM_BUSY_COUNT_NUM; i++) {
			val = (unsigned int)readl(DPM_BUSY_CNT0_ADDR(
				module_dpm_addr + NUM_OF_BYTES * i));
			data_buff[i] = val;
		}
		mutex_unlock(&g_dpm_ctrl->gpu_power_lock);
		ret = DPM_BUSY_COUNT_NUM;
	} else {
		/* read dpm acc_energy for energy reporting */
		val = (unsigned int)readl(
			DPM_ACC_ENERGY_ADDR(module_dpm_addr));
		data_buff += val;
		mutex_unlock(&g_dpm_ctrl->gpu_power_lock);
		ret = DPM_ACC_ENERGY_NUM;
	}
	return ret;
}

#ifdef CONFIG_DPM_HWMON_DEBUG
static int dpm_get_counter_for_fitting(void)
{
	unsigned int i;
	unsigned int ret;
	unsigned int len = 0;
	unsigned int counter = 0;

	for (i = 0; i <= GPU_DPM_NUM_MAX; i++) {
		if ((1U << i)&(REPORT_CORE_MASK)) {
			ret = dpm_gpu_get_reg_val(i, g_dpm_buffer_for_fitting +
				DPM_BUSY_COUNT_NUM * counter, false);
			counter++;
			if (ret == 0) /* gpu power down */
				return 0;
			else
				len += ret;
		}
	}
	return len;
}
#endif

struct dpm_hwmon_ops gpu_hwmon_op = {
	.name = "gpu",
	.hi_dpm_update_counter = dpm_update_acc_energy,
#ifdef CONFIG_DPM_HWMON_DEBUG
	.hi_dpm_get_counter_for_fitting = dpm_get_counter_for_fitting,
#endif
};

static int dpm_update_acc_energy(void)
{
	unsigned int i;
	unsigned int ret;
	unsigned int len = 0;
	unsigned int counter = 0;
	unsigned long long *dpm_gpu_counter_table =
		gpu_hwmon_op.dpm_counter_table;
	/* dpm_report_enable is a global parameter
	 * which is used to choose reporting dpm data or fitting dpm data.
	 */
	if (!g_dpm_report_enabled)
		return 0;

	if (dpm_gpu_counter_table == NULL) {
		dev_err("%s: dpm_counter_table is NULL\n", __func__);
		return 0;
	}

	for (i = 0; i <= GPU_DPM_NUM_MAX; i++) {
		if ((1U << i)&(REPORT_CORE_MASK)) {
			ret = dpm_gpu_get_reg_val(i, dpm_gpu_counter_table +
				DPM_ACC_ENERGY_NUM * counter, true);
			counter++;
			if (ret == 0) /* gpu power down */
				return 0;
			else
				len += ret;
		}
	}

	return len;
}

static enum hrtimer_restart hisi_dpm_hrtimer_callback(struct hrtimer *timer)
{
	ktime_t period;
	struct hisi_gpu_dpm *dpm_ctrl = NULL;

	dpm_ctrl = container_of(timer, struct hisi_gpu_dpm, dpm_hrtimer);
	if (dpm_ctrl == NULL) {
		dev_err("%s: dpm_ctrl is NULL", __func__);
		return HRTIMER_NORESTART;
	}

	queue_work(system_unbound_wq, &dpm_ctrl->dpm_work);

	period = ktime_set(GPU_DPM_TIMER_SEC, GPU_DPM_TIMER_NSEC);
	hrtimer_start(&dpm_ctrl->dpm_hrtimer, period, HRTIMER_MODE_REL);
	return HRTIMER_NORESTART;
}
static void dpm_update_work(struct work_struct *work)
{
	dpm_update_acc_energy();
}

/* map dpmpcr related registers */
int dpm_pcr_register_map(struct kbase_device *kbdev)
{
	int err = 0;

	kbdev->hisi_dev_data.dpm_ctrl.dpmreg =
		ioremap(SYS_REG_GPUPCR_BASE_ADDR, DPM_PCR_REMAP_SIZE);
	if (!kbdev->hisi_dev_data.dpm_ctrl.dpmreg) {
		dev_err(kbdev->dev,
			"Can't remap dpm register window on platform \n");
		err = -EINVAL;
		goto out_dpm_ioremap;
	}
#ifdef CONFIG_MALI_BORR
	kbdev->hisi_dev_data.dpm_ctrl.peri_pcr_reg =
		ioremap(SYS_REG_PERRIPCR_BASE_ADDR, PERRI_PCR_REMAP_SIZE);
	if (!kbdev->hisi_dev_data.dpm_ctrl.peri_pcr_reg) {
		dev_err(kbdev->dev,
			"Can't remap peri_pcr register window on platform \n");
		err = -EINVAL;
		goto out_peri_pcr_ioremap;
	}
#endif
	g_dpm_reg = kbdev->hisi_dev_data.dpm_ctrl.dpmreg;

	return err;

#ifdef CONFIG_MALI_BORR
out_peri_pcr_ioremap:
	iounmap(kbdev->hisi_dev_data.dpm_ctrl.dpmreg);
#endif
out_dpm_ioremap:
	return err;
}

/* unmap dpmpcr related registers */
void dpm_pcr_register_unmap(struct kbase_device *kbdev)
{
	if (kbdev->hisi_dev_data.dpm_ctrl.dpmreg != NULL)
		iounmap(kbdev->hisi_dev_data.dpm_ctrl.dpmreg);

#ifdef CONFIG_MALI_BORR
	if (kbdev->hisi_dev_data.dpm_ctrl.peri_pcr_reg != NULL)
		iounmap(kbdev->hisi_dev_data.dpm_ctrl.peri_pcr_reg);
#endif
}

/* init dpmpcr related resources */
void dpm_pcr_resource_init(struct kbase_device *kbdev)
{
	unsigned int i;
	unsigned int num_of_report_cores = 0;

	if (dpm_pcr_register_map(kbdev) != 0)
		dev_err("%s: dpm_pcr_register map failed\n", __func__);
	g_dpm_ctrl = &kbdev->hisi_dev_data.dpm_ctrl;
	/*
	 * calculate how many dpm cores to report
	 * according to macro REPORT_CORE_MASK.
	 */
	for (i = 0; i <= GPU_DPM_NUM_MAX; i++) {
		if ((1U << i)&(REPORT_CORE_MASK))
			num_of_report_cores++;
		}
	/* specify the length of energy reporting buffer */
	gpu_hwmon_op.dpm_cnt_len = num_of_report_cores;

	INIT_WORK(&kbdev->hisi_dev_data.dpm_ctrl.dpm_work, dpm_update_work);

	/* dpm_hrtimer init */
	hrtimer_init(&kbdev->hisi_dev_data.dpm_ctrl.dpm_hrtimer,
	CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	kbdev->hisi_dev_data.dpm_ctrl.dpm_hrtimer.function =
	hisi_dpm_hrtimer_callback;

	mutex_init(&kbdev->hisi_dev_data.dpm_ctrl.gpu_power_lock);

	/* register struct gpu_hwmon_op to top level framework */
	dpm_hwmon_register(&gpu_hwmon_op);

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
	ktime_t period;
	bool flag_for_bypass = false;

	gpu_pcr_enable(kbdev, flag_for_bypass);

	for (i = 0; i <= GPU_DPM_NUM_MAX; i++) {
		if ((1U << i)&(REPORT_CORE_MASK))
			dpm_gpu_monitor_enable(kbdev, i);
	}

	period = ktime_set(GPU_DPM_TIMER_SEC, GPU_DPM_TIMER_NSEC);
	hrtimer_start(&kbdev->hisi_dev_data.dpm_ctrl.dpm_hrtimer,
		period, HRTIMER_MODE_REL);

	mutex_lock(&kbdev->hisi_dev_data.dpm_ctrl.gpu_power_lock);
	kbdev->hisi_dev_data.dpm_ctrl.dpm_init_status = true;
	mutex_unlock(&kbdev->hisi_dev_data.dpm_ctrl.gpu_power_lock);
}

void dpm_pcr_disable(struct kbase_device *kbdev)
{
	unsigned int i;

	/* cancel dpm timer */
	hrtimer_cancel(&kbdev->hisi_dev_data.dpm_ctrl.dpm_hrtimer);

	/* update dpm counter, then disable dpm */
	dpm_update_acc_energy();

	mutex_lock(&kbdev->hisi_dev_data.dpm_ctrl.gpu_power_lock);
	kbdev->hisi_dev_data.dpm_ctrl.dpm_init_status = false;
	mutex_unlock(&kbdev->hisi_dev_data.dpm_ctrl.gpu_power_lock);

	for (i = 0; i <= GPU_DPM_NUM_MAX; i++) {
		if ((1U << i)&(REPORT_CORE_MASK))
			dpm_gpu_monitor_disable(kbdev, i);
	}
	gpu_pcr_disable(kbdev);
}

