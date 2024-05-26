/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef DPU_HEBCE_H
#define DPU_HEBCE_H

#include "hisi_composer_operator.h"
#include "dpu_offline_define.h"
#include "dm/hisi_dm.h"
#include "hisi_disp.h"
#include "dpu_hebc.h"

enum {
	BPP_RGB565  = 2,
	BPP_RGBA4444 = 2,
	BPP_RGBX4444 = 2,
	BPP_RGBA5551 = 2,
	BPP_RGBX5551 = 2,
	BPP_RGBA8888 = 4,
	BPP_RGBX8888 = 4,
	BPP_RGBA1010102 = 4,
	BPP_NV12_10BIT = 2,
	BPP_NV12_8BIT = 1
};

enum hebce_pixel_format {
	HEBCE_RGB565 = 0,
	HEBCE_RGBA4444,
	HEBCE_RGBX4444,
	HEBCE_RGBA5551,
	HEBCE_RGBX5551,
	HEBCE_RGBA8888,
	HEBCE_RGBX8888,
	HEBCE_RGBA1010102,
	HEBCE_NV12_8BIT,
	HEBCE_NV12_10BIT,
	HEBCE_TYPE_NOROT_MAX,
	HEBCE_RGB565_ROT = HEBCE_TYPE_NOROT_MAX,
	HEBCE_RGBA4444_ROT,
	HEBCE_RGBX4444_ROT,
	HEBCE_RGBA5551_ROT,
	HEBCE_RGBX5551_ROT,
	HEBCE_RGBA8888_ROT,
	HEBCE_RGBX8888_ROT,
	HEBCE_RGBA1010102_ROT,
	HEBCE_NV12_8BIT_ROT,
	HEBCE_NV12_10BIT_ROT,
	HEBCE_TYPE_MAX
};

// HEBCE picture width x height max / min
enum {
	HEBCE_WIDTH_MAX_1 = 3840,
	HEBCE_WIDTH_MAX_2 = 3840,
	HEBCE_HEIGHT_MAX = 4096,
	HEBCE_WIDTH_MAX_ROT = 2560,
	HEBCE_HEIGHT_MAX_ROT = 3840
};

// HEBCE picture width x height max / min
enum {
	HEBCE_WIDTH_RGB_MIN = 32,
	HEBCE_HEIGHT_RGB_MIN = 4,

	HEBCE_WIDTH_NV12_MIN = 64,
	HEBCE_HEIGHT_NV12_MIN = 16,
	HEBCE_WIDTH_NV12_10BIT_MIN = 32,
	HEBCE_HEIGHT_NV12_10BIT_MIN = 16,

	HEBCE_WIDTH_RGB_ROT_MIN = 8,
	HEBCE_HEIGHT_RGB_ROT_MIN = 16,

	HEBCE_WIDTH_NV12_ROT_MIN = 16,
	HEBCE_HEIGHT_NV12_ROT_MIN = 64,
	HEBCE_WIDTH_NV12_10BIT_ROT_MIN = 16,
	HEBCE_HEIGHT_NV12_10BIT_ROT_MIN = 32
};

struct hebce_rect_boundary {
	uint32_t width_min;
	uint32_t height_mix;
	uint32_t width_max;
	uint32_t height_max;
};

struct hal_to_hebce_pixel_fmt {
	uint32_t hebce_pixel_format;
	uint32_t hal_pixel_format;
};

struct hebce_rect_align_info {
	uint32_t width_align_info;
	uint32_t height_align_info;
};

struct hebce_align_info {
	uint32_t hebce_header_addr_align;
	uint32_t hebce_header_stride_align;
	uint32_t hebce_pld_addr_align;
	uint32_t hebce_pld_stride_align;
};

struct hebce_params {
	int32_t wdma_format;
	uint32_t wdma_transform;
	uint32_t color_transform;

	uint32_t sblk_layout;
	uint32_t sblk_layout_switch_en;
	uint32_t tag_mode;
	uint32_t yuv_trans;
	uint32_t scramble;
	uint32_t transform;

	uint32_t hebce_block_width_align;
	uint32_t hebce_block_height_align;

	struct dpu_edit_rect clip_rect;
	struct disp_rect aligned_rect;
	uint32_t hebce_payload_align[HEBC_PANEL_MAX_COUNT];
	struct hebc_plane_info hebce_planes[HEBC_PANEL_MAX_COUNT];
};

int dpu_hebce_config(struct disp_wb_layer *layer, struct disp_rect aligned_rect, struct dpu_wch_wdma *wdma,
				struct disp_rect *ov_block_rect);
#endif
