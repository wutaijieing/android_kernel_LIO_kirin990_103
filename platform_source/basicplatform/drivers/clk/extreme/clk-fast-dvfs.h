/*
 * Copyright (c) 2019-2020 Huawei Technologies Co., Ltd.
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

#ifndef __CLK_FAST_DVFS_H_
#define __CLK_FAST_DVFS_H_

#include "clk.h"

#define FREQ_CONVERSION 1000
#define ADD_FREQ_OFFSET 1000000  /* For 184.444M to 185M */

#define HIMASKEN_SHIFT 16
#define himask_set(mask) ((BIT(mask) << HIMASKEN_SHIFT) | BIT(mask))
#define himask_unset(mask) (BIT(mask) << HIMASKEN_SHIFT)

/* member of struct clksw_offset and struct clkdiv_offset */
enum {
	CFG_OFFSET = 0,
	CFG_MASK,
	SHIFT,
};

struct hi3xxx_fastclk {
	struct clk_hw hw;
	u32 clksw_offset[SW_DIV_CFG_CNT]; /* offset mask start_bit */
	u32 clkdiv_offset[SW_DIV_CFG_CNT]; /* offset mask start_bit */
	u32 clkgt_cfg[GATE_CFG_CNT]; /* offset value */
	u32 clkgate_cfg[GATE_CFG_CNT]; /* offset value */
	struct media_pll_info **pll_info;
	u32 pll_num;
	u32 profile_value[PROFILE_CNT]; /* profile value */
	u32 profile_pll_id[PROFILE_CNT]; /* profile pll id */
	u32 p_sw_cfg[PROFILE_CNT]; /* profile sw cfg */
	u32 div_val[PROFILE_CNT]; /* profile div val */
	u32 p_div_cfg[PROFILE_CNT]; /* profile div cfg */
	int profile_level;
	void __iomem *base_addr;
	spinlock_t *lock;
	int en_count;
	u32 always_on;
	unsigned long rate;
};
#endif
