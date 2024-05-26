/*
 * hisi_max_power_stub.c
 *
 * max power driver stub
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
#include "max_power.h"
#include <linux/types.h>
#include <linux/printk.h>

__attribute__((weak)) void max_power_int(int type)
{
	pr_err("%s stub,type:%d\n", __func__, type);
}

__attribute__((weak)) void max_power(u32 outer_count, u32 inter_count,
				     u32 gpio_addr, int type)
{
	pr_err("%s stub,outer:%u,inter:%u,gpio:0x%x,type:%d\n", __func__,
	       outer_count, inter_count, gpio_addr, type);
}
