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
#ifndef __CLK_HVC_H_
#define __CLK_HVC_H_

#include <linux/types.h>

/*
 * clk smc handler x0 value, every sec clk cmd needs unique
 * to prevent impact on other commands
 */
enum {
	CLK_PREPARE = 0x0,
	CLK_UNPREPARE,
	CLK_ENABLE,
	CLK_DISABLE,
	CLK_GET_RATE,
	CLK_SET_RATE,
};

#define CLK_HVC_FN_ID	0xc500f0f0
noinline int uvmm_service_clk_hvc(u64 _arg1,
	u64 _arg2, u64 _arg3, u64 _arg4);
#endif
