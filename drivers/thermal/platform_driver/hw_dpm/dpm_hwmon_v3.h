
/*
 * dpm_hwmon_v3.h
 *
 * dpm interface for component
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#ifndef __DPM_HWMON_V3_H
#define __DPM_HWMON_V3_H

#include <linux/platform_drivers/dpm_hwmon.h>
#include <linux/kernel.h>
#include <linux/mutex.h>

unsigned long long get_dpm_power(struct dpm_hwmon_ops *pos);
unsigned long long get_part_power(struct dpm_hwmon_ops *pos,
				  unsigned int start, unsigned int end);
void dpm_enable_module(struct dpm_hwmon_ops *pos, bool dpm_switch);
bool get_dpm_enabled(struct dpm_hwmon_ops *pos);
void dpm_sample(struct dpm_hwmon_ops *pos);

void dpm_hwmon_end(void);
int dpm_hwmon_prepare(void);

extern struct list_head g_dpm_hwmon_ops_list;
extern struct mutex g_dpm_hwmon_ops_list_lock;

#if defined(CONFIG_DPM_HWMON_DEBUG) && defined(CONFIG_DPM_HWMON_PWRSTAT)
void powerstat_config(struct dpm_hwmon_ops *pos, int timer_span,
		      int total_count, int mode);
#endif
#endif
