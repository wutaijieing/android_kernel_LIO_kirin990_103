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
 *  Description: dubai its data file
 *  Author: duyouxuan
 *  Create: 2020-07-08
 */
#ifndef __DUBAI_ITS__
#define __DUBAI_ITS__

#include <linux/kernel.h>
#include <linux/lpm_thermal.h>

int ioctrl_get_cpu_power(void __user *argp);
int ioctrl_reset_cpu_power(void __user *argp);
void free_its_cpu_power_mem(void);
bool create_its_cpu_power_mem(void);

#endif
