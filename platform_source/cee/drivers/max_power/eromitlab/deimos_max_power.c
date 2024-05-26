/*
 * deimos_max_power.c
 *
 * cs middle&big cluster max power buffer
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
#include <linux/io.h>
#include <linux/types.h>
#include <linux/printk.h>
#include <securec.h>
#include "deimos_max_power_wfe.h"

#define DATA_POOL_THREE_LOOP 1024
#define DATA_POOL_ZERO_LOOP1 112
#define DATA_POOL_ZERO_LOOP2 128
#define DATA_POOL_TWO_LOOP 368

#define POOL_1M (1 * 1024 * 1024)
#define POOL_2M (2 * 1024 * 1024)
#define TOTAL_POOL_SIZE (POOL_1M + POOL_2M * 2)
#define array_size(arr) (sizeof(arr) / sizeof((arr)[0]))

uintptr_t g_deimos_data_pool_03; /* 1M */
uintptr_t g_deimos_data_pool_00; /* 2M */
uintptr_t g_deimos_data_pool_02; /* 2M */

uintptr_t g_deimos_data_pool_13; /* 1M */
uintptr_t g_deimos_data_pool_10; /* 2M */
uintptr_t g_deimos_data_pool_12; /* 2M */

uintptr_t g_deimos_data_pool_23; /* 1M */
uintptr_t g_deimos_data_pool_20; /* 2M */
uintptr_t g_deimos_data_pool_22; /* 2M */

uintptr_t g_deimos_data_pool_33; /* 1M */
uintptr_t g_deimos_data_pool_30; /* 2M */
uintptr_t g_deimos_data_pool_32; /* 2M */

u64 g_deimos_outer_loop_count;

static void deimos_data_pool_three_init(u32 *addr)
{
	u32 i, j;

	u32 array[] = {
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA
	};

	for (i = 0; i < DATA_POOL_THREE_LOOP; i++) { /* 64k */
		for (j = 0; j < array_size(array); j++) {
			*addr = array[j];
			addr++;
		}
	}
}

/* 36K */;
static void deimos_data_pool_zero_init(u32 *addr)
{
	u32 i;
	u32 j;

	u32 array[] = {
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555
	};
	u32 array_two[] = {
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA
	};

	for (i = 0; i < DATA_POOL_ZERO_LOOP1; i++) { /* 28K */
		for (j = 0; j < array_size(array); j++) {
			*addr = array[j];
			addr++;
		}
	}

	for (i = 0; i < DATA_POOL_ZERO_LOOP2; i++) { /* 8K */
		for (j = 0; j < array_size(array_two); j++) {
			*addr = array_two[j];
			addr++;
		}
	}
}

static void deimos_data_pool_two_init(u32 *addr)
{
	u32 i;
	u32 j;

	u32 array[] = {
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

	for (i = 0; i < DATA_POOL_TWO_LOOP; i++) { /* 92K */
		for (j = 0; j < array_size(array); j++) {
			*addr = array[j];
			addr++;
		}
	}
}

void deimos_max_power_init(uintptr_t pool_addr)
{
	g_deimos_data_pool_03 = pool_addr;
	g_deimos_data_pool_00 = g_deimos_data_pool_03 + POOL_1M;
	g_deimos_data_pool_02 = g_deimos_data_pool_00 + POOL_2M;
	deimos_data_pool_three_init((u32 *)g_deimos_data_pool_03);
	deimos_data_pool_zero_init((u32 *)g_deimos_data_pool_00);
	deimos_data_pool_two_init((u32 *)g_deimos_data_pool_02);

	g_deimos_data_pool_13 = g_deimos_data_pool_03 + TOTAL_POOL_SIZE;
	g_deimos_data_pool_10 = g_deimos_data_pool_00 + TOTAL_POOL_SIZE;
	g_deimos_data_pool_12 = g_deimos_data_pool_02 + TOTAL_POOL_SIZE;
	deimos_data_pool_three_init((u32 *)g_deimos_data_pool_13);
	deimos_data_pool_zero_init((u32 *)g_deimos_data_pool_10);
	deimos_data_pool_two_init((u32 *)g_deimos_data_pool_12);

	g_deimos_data_pool_23 = g_deimos_data_pool_13 + TOTAL_POOL_SIZE;
	g_deimos_data_pool_20 = g_deimos_data_pool_10 + TOTAL_POOL_SIZE;
	g_deimos_data_pool_22 = g_deimos_data_pool_12 + TOTAL_POOL_SIZE;
	deimos_data_pool_three_init((u32 *)g_deimos_data_pool_23);
	deimos_data_pool_zero_init((u32 *)g_deimos_data_pool_20);
	deimos_data_pool_two_init((u32 *)g_deimos_data_pool_22);

	g_deimos_data_pool_33 = g_deimos_data_pool_23 + TOTAL_POOL_SIZE;
	g_deimos_data_pool_30 = g_deimos_data_pool_20 + TOTAL_POOL_SIZE;
	g_deimos_data_pool_32 = g_deimos_data_pool_22 + TOTAL_POOL_SIZE;
	deimos_data_pool_three_init((u32 *)g_deimos_data_pool_33);
	deimos_data_pool_zero_init((u32 *)g_deimos_data_pool_30);
	deimos_data_pool_two_init((u32 *)g_deimos_data_pool_32);
}

void deimos_max_power(u32 outer_count, u32 inter_count, u32 gpio_addr)
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

	g_deimos_outer_loop_count = (u64)outer_count;
	deimos_max_power_pi_wfe((u64)gpio_virtul_addr);

	iounmap(gpio_virtul_addr);
}
