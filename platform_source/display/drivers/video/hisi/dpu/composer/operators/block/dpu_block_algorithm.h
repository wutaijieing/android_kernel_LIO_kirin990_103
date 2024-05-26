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

#ifndef __HISI_BLOCK_ALGORITHM_H__
#define __HISI_BLOCK_ALGORITHM_H__

#include "hisi_disp.h"
#include "dpu_offline_define.h"
#include "hisi_operator_tool.h"

struct block_layer_info {
	int input_startpos;
	int input_span;
	uint32_t output_startpos; /* relative to overlay plane */
	uint32_t output_span;
	uint32_t h_ratio;
	uint32_t acc_hscl;
	uint32_t scf_read_start;
	uint32_t scf_read_end;
	struct disp_rect rect_transform;
	struct disp_rect src_rect;
	int scf_int;
	int scf_rem;
	struct disp_rect dst_rect;
	int first_block;
	int last_block;
	int scf_in_start;
	int scf_in_end;
	int chn_idx_temp;
};

struct block_rect_info {
	uint32_t block_size;
	uint32_t current_offset;
	uint32_t last_offset;
	uint32_t fix_scf_span;
	int block_has_layer;
	int w;
	int h;
	bool copybit_wch_scl;
};

struct wb_block_layer_info {
	int input_startpos;
	int input_span;
	uint32_t output_startpos; /* relative to overlay plane */
	uint32_t output_span;
	uint32_t h_ratio;
	uint32_t acc_hscl;
	uint32_t scf_read_start;
	uint32_t scf_read_end;
	struct disp_rect rect_transform;
	int first_block;
	int last_block;
	int scf_in_start;
	int scf_in_end;
	int scf_dst_w;
};

#define ARSR2P_OVERLAPH 16

int get_ov_block_rect(struct ov_req_infos *pov_req_h, struct disp_overlay_block *pov_h_block,
	int *block_num, struct disp_rect *ov_block_rects[], bool has_wb_scl);
int get_block_layers(struct ov_req_infos *pov_req_h, struct disp_overlay_block *pov_h_block,
	struct disp_rect ov_block_rect, struct ov_req_infos *pov_req_v_block, int block_num);
int rect_across_rect(struct disp_rect rect1,struct disp_rect rect2, struct disp_rect *cross_rect);
int get_wb_layer_block_rect(struct disp_wb_layer *wb_layer, bool has_wb_scl, struct disp_rect *wb_layer_block_rect);
int get_wb_ov_block_rect(struct disp_rect ov_block_rect, struct disp_rect wb_src_rect,
	struct disp_rect *wb_ov_block_rect, struct ov_req_infos *pov_req_h_v, uint32_t wb_compose_type);

#endif
