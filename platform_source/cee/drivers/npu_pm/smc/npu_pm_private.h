/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2020. All rights reserved.
 * Description: npu debugfs
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef __NPU_PM_PRIVATE_H__
#define __NPU_PM_PRIVATE_H__

#define KHZ	1000U
#define MAX_MODULE_NAME_LEN	10

struct npu_pm_dvfs_data {
	bool dvfs_enable;
	unsigned int current_freq;
	struct thermal_zone_device *tz_dev;
	int *tzone;
	int tz_num;
	int tz_cur;
	unsigned int test;
	int temp_test;
	unsigned long max_dvfs_time;
};

enum {
	NPU_LOW_TEMP_DEBUG,
	NPU_LOW_TEMP_STATE
};

struct npu_pm_device {
	struct mutex mutex;
	struct device *dev;
	struct devfreq *devfreq;
	unsigned long last_freq;
	unsigned long target_freq;
	int pm_qos_class;
	bool power_on;
	unsigned int power_on_count;
	unsigned int power_off_count;
	struct npu_pm_dvfs_data *dvfs_data;
#ifdef CONFIG_DEVFREQ_THERMAL
	struct thermal_cooling_device *devfreq_cooling;
	unsigned long cur_state;
#endif
	unsigned long max_pwron_time;
	unsigned long max_pwroff_time;
};

#ifdef CONFIG_NPUFREQ
int npu_pm_devfreq_init(struct npu_pm_device *pmdev);
void npu_pm_devfreq_term(struct npu_pm_device *pmdev);
int npu_pm_devfreq_suspend(struct devfreq *devfreq);
int npu_pm_devfreq_resume(struct devfreq *devfreq);
int npu_pm_dvfs_hal(unsigned long target_freq);
int npu_pm_dvfs_init(struct npu_pm_device *pmdev);
int npu_pm_pwr_on(unsigned long target_freq);
int npu_pm_pwr_off(void);
#else
static inline int npu_pm_devfreq_init(struct npu_pm_device *pmdev)
{
	(void)pmdev;

	return 0;
}

static inline void npu_pm_devfreq_term(struct npu_pm_device *pmdev)
{
	(void)pmdev;
}

static inline int npu_pm_devfreq_suspend(struct devfreq *devfreq)
{
	(void)devfreq;

	return 0;
}

static inline int npu_pm_devfreq_resume(struct devfreq *devfreq)
{
	(void)devfreq;

	return 0;
}

static inline int npu_pm_dvfs_hal(unsigned long target_freq)
{
	(void)target_freq;

	return 0;
}

static inline int npu_pm_dvfs_init(struct npu_pm_device *pmdev)
{
	(void)pmdev;

	return 0;
}

static inline int npu_pm_pwr_on(unsigned long target_freq)
{
	(void)target_freq;

	return 0;
}

static inline int npu_pm_pwr_off(void)
{
	return 0;
}
#endif

#ifdef CONFIG_NPU_PM_DEBUG
void npu_pm_debugfs_init(struct npu_pm_device *pmdev);
void npu_pm_debugfs_exit(void);
#else
static inline void npu_pm_debugfs_init(struct npu_pm_device *pmdev)
{
	(void)pmdev;
}

static inline void npu_pm_debugfs_exit(void)
{
}
#endif

#endif /* __NPU_PM_PRIVATE_H__ */
