/*
 * max_power.h
 *
 * max power interface
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
#ifndef _MAX_POWER_H_
#define _MAX_POWER_H_

#include <linux/types.h>

#define HERA_ONE_M (1 * 1024 * 1024)
#define HERA_TWO_M (2 * 1024 * 1024)

void max_power_int(int type);
void max_power(u32 outer_count, u32 inter_count, u32 gpio_addr, int type);
#endif
