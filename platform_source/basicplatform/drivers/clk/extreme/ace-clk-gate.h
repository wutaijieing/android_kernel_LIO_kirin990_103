/*
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
 */
#ifndef __ACE_CLK_GATE_H_
#define __ACE_CLK_GATE_H_
#include <linux/clk.h>
#include <securec.h>
#include "clk.h"

struct ace_periclk {
	struct clk_hw hw;
	spinlock_t *lock;
	const char *name;
};

#define MAX_NAME_COUNT	25

#endif
