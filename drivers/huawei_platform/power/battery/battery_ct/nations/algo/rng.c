/* SPDX-License-Identifier: GPL-2.0 */
/*
 * rng.c
 *
 * rng function
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
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
#include <linux/err.h>
#include <linux/random.h>
#include "common.h"

static u32 rand_u32(void)
{
	return get_random_u32();
}

int u32_get_rand(u32 *n, u32 len)
{
	u32 i;

	if (!n) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	if (len > MAXWORDLEN)
		return -EINVAL;

	for (i = 0; i < len; i++)
		n[i] = rand_u32();
	return 0;
}
