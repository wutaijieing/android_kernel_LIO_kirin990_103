/*
 * freqdump_kernel.h
 *
 * freqdump
 *
 * Copyright (C) 2018-2020 Huawei Technologies Co., Ltd.
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
#ifndef __FREQDUMP_KERNEL_H__
#define __FREQDUMP_KERNEL_H__

#include <linux/types.h>
#include <freqdump_plat.h>

#define FREQDUMP_BUFF_SIZE	(1024 * 3)
#define OK	0

enum ENABLE_FLAGS {
	PERI_MONITOR_EN = 0,
	AO_MONITOR_EN,
	ALL_MONITOR_EN,
};

enum PLAT_FLAG {
	ES = 0,
	CS,
};

void sec_freqdump_read(void);

int ao_loadmonitor_read(u64 *addr);
void sec_chip_type_read(void);
void sec_loadmonitor_data_read(unsigned int enable_flags);
void sec_loadmonitor_switch_enable(unsigned int delay_value_peri,
					unsigned int delay_value_ao,
					unsigned int enable_flags);

int atfd_service_freqdump_smc(u64 function_id, u64 arg0, u64 arg1,
				   u64 arg2);
int peri_monitor_clk_init(const char *clk_name, unsigned int clk_freq);
int peri_monitor_clk_disable(const char *clk_name);
void sec_loadmonitor_switch_disable(void);
#ifdef CONFIG_NPU_PM_DEBUG
/*
 * data[6][2] used to store NPU module freq & voltage info
 * name[6][10] used to store NPU module name, max length is 9bytes
 */
int get_npu_freq_info(void *data, int size, char (*name)[10], int size_name);
#endif

#endif
