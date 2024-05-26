/*
 *  Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  Description: dubai dpm data file
 *  Author: duyouxuan
 *  Create: 2020-07-08
 */
#ifndef __DUBAI_DPM__
#define __DUBAI_DPM__

#include <linux/kernel.h>
#include <linux/platform_drivers/dpm_hwmon_user.h>
#ifdef CONFIG_MODEM_DPM
#include <linux/dpm_hwmon_modem.h>
#endif

int ioctl_dpm_enable_module(const void __user *argp);
int ioctl_dpm_get_peri_num(void __user *argp);
int ioctl_dpm_get_peri_data(void __user *argp);
int ioctl_dpm_get_gpu_data(void __user *argp);
int ioctl_dpm_get_npu_data(void __user *argp);
#ifdef CONFIG_MODEM_DPM
int ioctl_dpm_get_mdm_num(void __user *argp);
int ioctl_dpm_get_mdm_data(void __user *argp);
#endif

#endif
