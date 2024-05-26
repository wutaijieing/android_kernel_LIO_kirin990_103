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

#ifndef DPU_HEBCD_H
#define DPU_HEBCD_H

#include "hisi_composer_operator.h"
#include "dm/hisi_dm.h"
#include "hisi_disp.h"
#include "dpu_hebc.h"

struct hebcd_params {
	uint32_t dma_format;
	uint32_t sblk_type;
	uint32_t tag_mode;
	uint32_t yuv_trans;
	uint32_t scramble;
	uint32_t hebcd_sblock_type;
	uint32_t is_yuv_semi_planar;
	uint32_t hebcd_block_width_align;
	uint32_t hebcd_block_height_align;
	uint32_t hebcd_crop_num_max;

	uint32_t hebcd_payload_align[HEBC_PANEL_MAX_COUNT];
	uint32_t hebcd_top_crop_num;
	uint32_t hebcd_bottom_crop_num;

	struct dpu_edit_rect clip_rect;
	struct disp_rect aligned_rect;
	struct hebc_plane_info hebcd_planes[HEBC_PANEL_MAX_COUNT];
	uint32_t is_d3_128;
};

void hisi_sdma_hebcd_set_regs(struct hisi_comp_operator *operator, uint32_t layer_id,
	struct hisi_dm_layer_info * layer_info, char __iomem * dpu_base_addr, struct pipeline_src_layer *layer);
int dpu_hebcd_calc(struct pipeline_src_layer *layer, struct hisi_dm_layer_info *layer_info);

#endif