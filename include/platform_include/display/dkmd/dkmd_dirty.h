/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _DKMD_DIRTY_H_
#define _DKMD_DIRTY_H_

#include <linux/types.h>

/**
 * @brief if the left_align is 8,right_align is 8,and w_min is larger than 802,then w_min should be set to 808,
 * make sure that it is 8 align,if w_min is set to 802,there will be an error.left_align,right_align,top_align
 * bottom_align,w_align,h_align,w_min and h_min's valid value should be larger than 0,top_start and bottom_start
 * maybe equal to 0. if it's not surpport partial update, these value should set to invalid value(-1).
 *
 */
typedef struct lcd_dirty_region_info {
	int32_t left_align;
	int32_t right_align;
	int32_t top_align;
	int32_t bottom_align;

	int32_t w_align;
	int32_t h_align;
	int32_t w_min;
	int32_t h_min;

	int32_t top_start;
	int32_t bottom_start;
	int32_t spr_overlap;
	int32_t reserved;
} lcd_dirty_region_info_t;

#endif
