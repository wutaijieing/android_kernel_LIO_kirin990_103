/*
 * loadmonitor_common.h
 *
 * common monitor device
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
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

#ifndef _LOADMONITOR_COMMON_H_
#define _LOADMONITOR_COMMON_H_

#include <linux/types.h>
#include <loadmonitor_plat.h>

#define LOADMONITOR_OFF		0
#define LOADMONITOR_ON		1

extern void __iomem *g_loadmonitor_virt_addr;
extern phys_addr_t loadmonitor_phy_addr;
extern struct loadmonitor_data g_loadmonitor_data_kernel;
extern struct delay_time_with_freq g_monitor_delay_value;
int read_format_cs_loadmonitor_data(u32 flags);
void sec_loadmonitor_switch_disable(u32 en_flags, bool open_sr_ao_monitor);
void sec_loadmonitor_switch_enable(bool open_sr_ao_monitor);
void sec_loadmonitor_data_read(unsigned int enable_flags);
noinline int atfd_service_freqdump_smc(u64 _function_id, u64 _arg0,
					    u64 _arg1, u64 _arg2);

#endif
