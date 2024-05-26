/*
 * freqdump_k.h
 *
 * freqdump header file
 *
 * Copyright (C) 2017-2020 Huawei Technologies Co., Ltd.
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
#include <loadmonitor_common.h>

#define OK	0

enum PLAT_FLAG {
	ES = 0,
	CS,
};

struct file_str_ops {
	char *str;
	const struct file_operations *file_ops;
};

extern void __iomem *g_freqdump_virt_addr;
extern phys_addr_t freqdump_phy_addr;
extern int g_loadmonitor_status;

int peri_monitor_clk_init(const char *clk_name, unsigned int clk_freq);
int peri_monitor_clk_disable(const char *clk_name);
#endif
