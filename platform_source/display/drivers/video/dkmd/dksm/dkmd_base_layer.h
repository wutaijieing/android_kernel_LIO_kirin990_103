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

#ifndef DKMD_BASE_LAYER_H
#define DKMD_BASE_LAYER_H

#include <linux/types.h>
#include "dkmd_rect.h"

enum LAYER_TYPE {
	SOURCE_LAYER = 0,
	WB_LAYER,
	POST_LAYER,
};

enum TRANSFORM {
	TRANSFORM_NONE = 0,
	TRANSFORM_FLIP_H = 1,
	TRANSFORM_FLIP_V = 2,
	TRANSFORM_ROT_180 = 3,
	TRANSFORM_ROT_90 = 4,
	TRANSFORM_ROT_270 = 7,
	TRANSFORM_FLIP_H_ROT_90 = TRANSFORM_FLIP_H | TRANSFORM_ROT_90,
	TRANSFORM_FLIP_V_ROT_90 = TRANSFORM_FLIP_V | TRANSFORM_ROT_90,
};

enum COMPRESS_TYPE {
	COMPRESS_NONE = 0x000,
	COMPRESS_HEBC = 0x001,
	COMPRESS_HEMC = 0x002,
	COMPRESS_AFBC = 0x003
};

enum CSC_MODE {
	CSC_601_WIDE = 0,
	CSC_601_NARROW,
	CSC_709_WIDE,
	CSC_709_NARROW,
	CSC_2020,
	CSC_MODE_MAX,
};

enum COLOR_SPACE {
	COLOR_SPACE_SRGB = 0,
	COLOR_SPACE_DISPLAY_P3,
	COLOR_SPACE_BT2020,
	COLOR_SPACE_MAX,
};

struct dkmd_base_layer {
	int32_t layer_type;
	int32_t format;
	int32_t transform;
	int32_t compress_type;
	uint64_t iova;
	uint64_t phyaddr;
	uint32_t stride;
	int32_t acquire_fence;
	uint32_t dm_index; // Dm layer idx has range of [0, maxLayerNum)
	uint32_t csc_mode;
	uint32_t color_space;
	struct dkmd_rect_coord src_rect;
	struct dkmd_rect_coord dst_rect;
};

#endif