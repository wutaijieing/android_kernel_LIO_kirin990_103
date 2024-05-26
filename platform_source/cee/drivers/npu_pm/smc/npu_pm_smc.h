/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2020. All rights reserved.
 * Description: npu pm smc interface
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
#ifndef __NPU_PM_SMC_H__
#define __NPU_PM_SMC_H__

#ifdef CONFIG_NPUFREQ
int npu_pm_pwron_smc_request(unsigned long freq, int temp);
int npu_pm_pwroff_smc_request(int temp);
int npu_pm_dvfs_request_smc_request(unsigned long tar_freq, int temp);
int npu_pm_set_voltage_smc_request(unsigned int voltage);
int npu_pm_get_voltage_smc_request(void);
void npu_pm_dbg_smc_request(unsigned int enable);
#else
static inline int npu_pm_pwron_request(unsigned long freq, int temp)
{
	(void)freq;
	(void)temp;

	return 0;
}

static inline int npu_pm_pwroff_smc_request(int temp)
{
	(void)temp;

	return 0;
}

static inline int npu_pm_dvfs_request_smc_request(unsigned long tar_freq,
						       int temp)
{
	(void)tar_freq;
	(void)temp;

	return 0;
}

static inline int npu_pm_set_voltage_smc_request(unsigned int voltage)
{
	(void)voltage;

	return 0;
}

static inline int npu_pm_get_voltage_smc_request(void)
{
	return 0;
}
#endif

#endif /* __NPU_PM_SMC_H__ */
