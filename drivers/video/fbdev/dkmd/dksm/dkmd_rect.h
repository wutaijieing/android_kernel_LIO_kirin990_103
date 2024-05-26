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

#ifndef DKMD_RECT_H
#define DKMD_RECT_H

#include <linux/types.h>

struct dkmd_rect {
	int32_t x;
	int32_t y;
	uint32_t w;
	uint32_t h;
};

struct dkmd_rect_coord {
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;
};

static inline uint32_t rect_width(const struct dkmd_rect_coord *rect)
{
	return rect->right > rect->left ? (uint32_t)(rect->right - rect->left) : 0;
}

static inline uint32_t rect_height(const struct dkmd_rect_coord *rect)
{
	return rect->bottom > rect->top ? (uint32_t)(rect->bottom - rect->top) : 0;
}

/* Check if includee rect is included by includer rect */
static inline bool is_rect_included(const struct dkmd_rect_coord *includee, const struct dkmd_rect_coord *includer)
{
	return (includee->left >= includer->left && includee->top >= includer->top &&
		includee->right <= includer->right && includee->bottom <= includer->bottom);
}

#endif