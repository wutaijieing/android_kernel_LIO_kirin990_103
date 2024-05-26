/*
 * hercules_max_power.c
 *
 * es middle cluster max power buffer
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
#include "hercules_max_power_wfe.h"
#include <linux/io.h>
#include <linux/types.h>
#include <max_power.h>

#define DATA_POOL_THREE_LOOP 1024
#define DATA_POOL_TWO_LOOP 368
#define DATA_POOL_ZERO_LOOP1 112
#define DATA_POOL_ZERO_LOOP2 128
#define array_size(a) (sizeof(a) / sizeof((a)[0]))

uintptr_t g_hercules_data_pool_03; /* 1M */
uintptr_t g_hercules_data_pool_00; /* 2M */
uintptr_t g_hercules_data_pool_02; /* 1M */

uintptr_t g_hercules_data_pool_13; /* 1M */
uintptr_t g_hercules_data_pool_10; /* 2M */
uintptr_t g_hercules_data_pool_12; /* 1M */

uintptr_t g_hercules_data_pool_23; /* 2M */
uintptr_t g_hercules_data_pool_20; /* 1M */
uintptr_t g_hercules_data_pool_22; /* 1M */

u64 g_hercules_outer_loop_count = 1;
u64 g_hercules_inter_loop_count = 1;
static void hercules_three_init(u32 *addr)
{
	u32 i, j;
	u32 arry[] = {
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA
	};

	for (i = 0; i < DATA_POOL_THREE_LOOP; i++) {
		for (j = 0; j < array_size(arry); j++) {
			*addr = arry[j];
			addr++;
		}
	}
}

static void hercules_zero_init(u32 *addr)
{
	u32 i, j;
	u32 arry[] = {
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555
	};
	u32 arry_two[] = {
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA
	};

	for (i = 0; i < DATA_POOL_ZERO_LOOP1; i++) { /* 28k */
		for (j = 0; j < array_size(arry); j++) {
			*addr = arry[j];
			addr++;
		}
	}
	for (i = 0; i < DATA_POOL_ZERO_LOOP2; i++) { /* 8K */
		for (j = 0; j < array_size(arry_two); j++) {
			*addr = arry_two[j];
			addr++;
		}
	}
}

static void hercules_two_init(u32 *addr)
{
	u32 i, j;
	u32 arry[] = {
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA
	};

	for (i = 0; i < DATA_POOL_TWO_LOOP; i++) {
		for (j = 0; j < array_size(arry); j++) {
			*addr = arry[j];
			addr++;
		}
	}
}

void hercules_data_pool_init(u32 *addr3, u32 *addr0, u32 *addr2)
{
	hercules_three_init(addr3); /* 64K */
	hercules_zero_init(addr0); /* 36k */
	hercules_two_init(addr2); /* 92K */
}

void hercules_max_power(u32 outer_count, u32 inter_count, u32 gpio_addr)
{
	void __iomem *gpio_virtul_addr = NULL;

	if (outer_count == 0 || inter_count == 0 || gpio_addr == 0) {
		pr_err("%s arg error:outer%d,inter:%d,gpio:%d\n", __func__,
		       outer_count, inter_count, gpio_addr);
		return;
	}

	gpio_virtul_addr = ioremap((phys_addr_t)gpio_addr,
				   (size_t)sizeof(long));
	if (gpio_virtul_addr == NULL) {
		pr_err("%s remap err.addr:%x\n", __func__, gpio_addr);
		return;
	}

	g_hercules_outer_loop_count = (u64)outer_count;
	g_hercules_inter_loop_count = (u64)inter_count;
	hercules_max_power_pi_wfe((u64)gpio_virtul_addr);

	iounmap(gpio_virtul_addr);
}

void hercules_max_power_init(uintptr_t pool_addr)
{
	g_hercules_data_pool_03 = pool_addr;
	g_hercules_data_pool_00 = g_hercules_data_pool_03 + HERA_ONE_M;
	g_hercules_data_pool_02 = g_hercules_data_pool_00 + HERA_TWO_M;
	hercules_data_pool_init((u32 *)g_hercules_data_pool_03,
				(u32 *)g_hercules_data_pool_00,
				(u32 *)g_hercules_data_pool_02);

	g_hercules_data_pool_13 = g_hercules_data_pool_02 + HERA_ONE_M;
	g_hercules_data_pool_10 = g_hercules_data_pool_13 + HERA_ONE_M;
	g_hercules_data_pool_12 = g_hercules_data_pool_10 + HERA_TWO_M;
	hercules_data_pool_init((u32 *)g_hercules_data_pool_13,
				(u32 *)g_hercules_data_pool_10,
				(u32 *)g_hercules_data_pool_12);

	g_hercules_data_pool_23 = g_hercules_data_pool_12 + HERA_ONE_M;
	g_hercules_data_pool_20 = g_hercules_data_pool_23 + HERA_TWO_M;
	g_hercules_data_pool_22 = g_hercules_data_pool_20 + HERA_ONE_M;
	hercules_data_pool_init((u32 *)g_hercules_data_pool_23,
				(u32 *)g_hercules_data_pool_20,
				(u32 *)g_hercules_data_pool_22);
}
