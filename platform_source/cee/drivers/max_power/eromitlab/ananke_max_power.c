/*
 * ananke_max_power.c
 *
 * little cluster max power buffer
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
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
#include <linux/types.h>
#include <linux/io.h>
#include "ananke_max_power_wfe.h"

u64 g_l1_memory_addr; /* ddr array 32K */
u64 g_ananke_outer_loop_count = 1;
u64 g_ananke_inter_loop_count = 1;

void ananke_max_power(u32 outer_count, u32 inter_count, u32 gpio_addr)
{
	void __iomem *gpio_virtul_addr = NULL;

	if (outer_count == 0 || inter_count == 0 || gpio_addr == 0) {
		pr_err("%s arg error:outer%d,inter:%d,gpio:%d\n", __func__,
		       outer_count, inter_count, gpio_addr);
		return;
	}

	gpio_virtul_addr = ioremap((phys_addr_t)gpio_addr, (size_t)sizeof(long));
	if (gpio_virtul_addr == NULL) {
		pr_err("%s remap err.addr:%x\n", __func__, gpio_addr);
		return;
	}

	g_ananke_outer_loop_count = (u64)outer_count;
	g_ananke_inter_loop_count = (u64)inter_count;
	ananke_max_power_pi_wfe((u64)gpio_virtul_addr);

	iounmap(gpio_virtul_addr);
}

void ananke_max_power_init(u64 base_addr)
{
	g_l1_memory_addr = base_addr;
}
