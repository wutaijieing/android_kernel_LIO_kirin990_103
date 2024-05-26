/*
 * hera_max_power.c
 *
 * es big cluster max power buffer
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
#include "hera_max_power_wfe.h"
#include <linux/io.h>
#include <linux/types.h>
#include <linux/printk.h>
#include <max_power.h>

#define DATA_POOL_THREE_LOOP 1024
#define DATA_POOL_ZERO_LOOP1 512
#define DATA_POOL_ZERO_LOOP2 128
#define DATA_POOL_TWO_LOOP 368
#define BIG_CORE_INDEX 7
#define array_size(a) (sizeof(a) / sizeof((a)[0]))

uintptr_t g_hera_data_pool_33; /* 1M */
uintptr_t g_hera_data_pool_30; /* 2M */
uintptr_t g_hera_data_pool_32; /* 2M */
uintptr_t g_hera_data_pool_34; /* 2M */
u64 g_hera_outer_loop_count = 1;
u64 g_hera_inter_loop_count = 1;

static void hera_data_pool_three_init(u32 *addr)
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

static void hera_data_pool_zero_init(u32 *addr)
{
	u32 i, j;
	u32 arry[] = {
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
	u32 arry_two[] = {
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA
	};

	for (i = 0; i < DATA_POOL_ZERO_LOOP1; i++) {
		for (j = 0; j < array_size(arry); j++) {
			*addr = arry[j];
			addr++;
		}
	}
	for (i = 0; i < DATA_POOL_ZERO_LOOP2; i++) {
		for (j = 0; j < array_size(arry_two); j++) {
			*addr = arry_two[j];
			addr++;
		}
	}
}

static void hera_data_pool_two_init(u32 *addr)
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

static void hera_data_pool_four_init(u32 *addr)
{
	u32 i;
	u32 arry[] = {
		0xE89EF61A, 0x3F91EB64, 0xD1C4227D, 0x3FB8291C,
		0xE44A0D93, 0x3F2C2086, 0x96D0BDC4, 0x3F64410D,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA,
		0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA
	};

	for (i = 0; i < array_size(arry); i++) {
		*addr = arry[i];
		addr++;
	}
}

void hera_data_pool_init(u32 *pool33, u32 *pool30,
			 u32 *pool32, u32 *pool34)
{
	hera_data_pool_three_init((u32 *)pool33);
	hera_data_pool_zero_init((u32 *)pool30);
	hera_data_pool_two_init((u32 *)pool32);
	hera_data_pool_four_init((u32 *)pool34);
}

void hera_max_power(u32 outer_count, u32 inter_count, u32 gpio_addr)
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

	g_hera_outer_loop_count = (u64)outer_count;
	g_hera_inter_loop_count = (u64)inter_count;
	hera_max_power_pi_wfe((u64)gpio_virtul_addr);

	iounmap(gpio_virtul_addr);
}

void hera_max_power_init(uintptr_t pool_addr)
{
	g_hera_data_pool_33 = pool_addr;
	g_hera_data_pool_30 = g_hera_data_pool_33 + HERA_ONE_M;
	g_hera_data_pool_32 = g_hera_data_pool_30 + HERA_TWO_M;
	g_hera_data_pool_34 = g_hera_data_pool_32 + HERA_TWO_M;
	hera_data_pool_init((u32 *)g_hera_data_pool_33,
			    (u32 *)g_hera_data_pool_30,
			    (u32 *)g_hera_data_pool_32,
			    (u32 *)g_hera_data_pool_34);
}
