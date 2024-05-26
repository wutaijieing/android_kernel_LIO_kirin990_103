/*
 * npu_dpm.h
 *
 * about npu dpm
 *
 * Copyright (c) 2021-2022 Huawei Technologies Co., Ltd.
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
#ifndef __NPU_DMP_H
#define __NPU_DMP_H

#include <linux/types.h>

#include <asm/compiler.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/version.h>

#include "npu_platform_register.h"
#include "npu_log.h"
#include "npu_adapter.h"

#if defined(CONFIG_DPM_HWMON_V1)
#include "npu_pm.h"
#endif

#define SOC_NPU_CRG_SIZE     0x1000
#define SOC_NPU_DPM_SIZE     0x1000
#define BUSINESSFIT          0
#define DATAREPORTING        1
#define DPM_REG_BASE_OFFSET  4

#if defined(CONFIG_DPM_HWMON_V1)
int npu_dpm_fitting_coff(void);
int npu_dpm_get_peri_volt(int *curr_volt);
#elif defined(CONFIG_DPM_HWMON_V2)
bool npu_dpm_enable_flag(void);
#elif defined(CONFIG_DPM_HWMON_V3)
bool npu_dpm_enable_flag(void);
#endif

int npu_dpm_get_counter_for_fitting(int mode);
void npu_dpm_init(void);
void npu_dpm_exit(void);

#endif /* __NPU_DMP_H */
