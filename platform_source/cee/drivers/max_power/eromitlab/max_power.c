/*
 * max_power.c
 *
 * plaform max power
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
#include <linux/slab.h>
#include <linux/stddef.h>
#include <linux/printk.h>
#include <linux/io.h>
#include <asm/cputype.h>
#include <securec.h>
#include "ananke_max_power.h"
#include "hercules_max_power.h"
#include "hera_max_power.h"
#include "deimos_max_power.h"

#define MAX_POOL_SIZE (23 * 1024 * 1024)
#define LITTLE_CLUSTER_POOL_SIZE (1 * 1024 * 1024)
#define MIDDLE_CLUSTER_POOL_SIZE (15 * 1024 * 1024)
#define MAX_POWER_ES 0
#define MAX_POWER_CS 1
#define CPU_CORE_ID_SHIFT 8
static char g_data[MAX_POOL_SIZE];

void max_power_middle_and_big_init(char *base_addr)
{
	char *freqdump_virt_addr = NULL;

	/* middle cluslter */
	freqdump_virt_addr = base_addr;
	hercules_max_power_init((uintptr_t)freqdump_virt_addr); /* 15M */

	/* big cluslter */
	freqdump_virt_addr = base_addr + MIDDLE_CLUSTER_POOL_SIZE;
	hera_max_power_init((uintptr_t)freqdump_virt_addr); /* 7M */
}

void max_power_int(int type)
{
	errno_t err;

	err = memset_s(g_data, sizeof(g_data), 0, sizeof(g_data));
	if (err != EOK)
		pr_err("%s,memset_s error!\n", __func__);

	/* little cluster */
	ananke_max_power_init((u64)g_data); /* buffer 32K */
	if (type == MAX_POWER_ES)
		max_power_middle_and_big_init(g_data +
					      LITTLE_CLUSTER_POOL_SIZE);
	else
		deimos_max_power_init((uintptr_t)g_data + LITTLE_CLUSTER_POOL_SIZE);
	pr_err("%s init success,plat type:%d\n", __func__, type);
}

void max_power(u32 outer_count, u32 inter_count, u32 gpio_addr, int type)
{
	u32 cpu_core_id;

	cpu_core_id = (read_cpuid_mpidr() >> CPU_CORE_ID_SHIFT) & 0xFF;
	pr_err("%s cpu_core_id:%u, type:%d\n", __func__, cpu_core_id, type);
	if (type == MAX_POWER_ES) {
		switch (cpu_core_id) {
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
			ananke_max_power(outer_count, inter_count, gpio_addr);
			break;
		case 0x4:
		case 0x5:
		case 0x6:
			hercules_max_power(outer_count, inter_count, gpio_addr);
			break;
		case 0x7:
			hera_max_power(outer_count, inter_count, gpio_addr);
			break;
		default:
			pr_err("%s fail,outer:%u,inter:%u,gpio:0x%x,type:%d\n",
			       __func__, outer_count,
			       inter_count, gpio_addr, type);
			return;
		}
	} else {
		switch (cpu_core_id) {
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
			ananke_max_power(outer_count, inter_count, gpio_addr);
			break;
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7:
			deimos_max_power(outer_count, inter_count, gpio_addr);
			break;
		default:
			pr_err("%s fail,outer:%u,inter:%u,gpio:0x%x,type:%d\n",
			       __func__, outer_count,
			       inter_count, gpio_addr, type);
			return;
		}
	}
	pr_err("%s success,outer:%u,inter:%u,gpio:0x%x,type:%d\n", __func__,
	       outer_count, inter_count, gpio_addr, type);
}
