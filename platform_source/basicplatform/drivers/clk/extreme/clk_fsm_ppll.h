/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * clk_fsm_ppll.h
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __CLK_FSM_PPLL_H
#define __CLK_FSM_PPLL_H

#include "clk.h"

#define REG_MAX_BIT 32
#define FSM_PLL_MASK_OFFSET 16
#define AP_FSM_PPLL_STABLE_TIME 60
#define FSM_PLL_REG_NUM 2

struct hi3xxx_fsm_ppll_clk {
	struct clk_hw hw;
	u32 ref_cnt; /* reference count */
	u32 pll_id;
	u32 fsm_en_ctrl[FSM_PLL_REG_NUM];
	u32 fsm_stat_ctrl[FSM_PLL_REG_NUM];
	void __iomem *addr; /* base addr */
	spinlock_t *lock;
};

#endif
