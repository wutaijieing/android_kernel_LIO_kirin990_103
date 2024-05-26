/*
 * freqdump_k.h
 *
 * freqdump header file
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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

#ifndef __FREQDUMP_K_H__
#define __FREQDUMP_K_H__

#include <freqdump_plat.h>
#include <loadmonitor_common_kernel.h>

#define OK					0
#define USER_BUFFER_SIZE	128

enum PLAT_FLAG {
	ES = 0,
	CS,
};

struct file_str_ops {
	char *str;
	const struct file_operations *file_ops;
};

enum FREQ_SMC_SUB_ID {
	FREQ_READ_DATA,
	NODE_DUMP_CLUSTER0,
	NODE_DUMP_CLUSTER1,
	NODE_DUMP_CLUSTER2,
	NODE_DUMP_GPU,
	NODE_DUMP_L3,
	NODE_DUMP_DDR,
	NODE_DUMP_NPU,
	NODE_DUMP_PERI,
};

extern void __iomem *g_freqdump_virt_addr;
extern phys_addr_t freqdump_phy_addr;
extern int g_loadmonitor_status;
extern struct semaphore g_freqdump_sem;
extern struct semaphore g_loadmonitor_sem;
extern u32 g_monitor_en_flags;
extern struct dentry *g_freqdump_debugfs_root;

int peri_monitor_clk_init(const char *clk_name, unsigned int clk_freq);
int peri_monitor_clk_disable(const char *clk_name);
#endif
