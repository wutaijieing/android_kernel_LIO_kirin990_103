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
#include "dsc_debug.h"

void vesa_dsc_info_print(struct dsc_info *dsc_info)
{
	dpu_check_and_no_retval((dsc_info == NULL), err, "NULL POINTER");

	dpu_pr_info("[DSC] %d, %d, %d, %d, %d, %d, %d, %d\n",
		dsc_info->chunk_size,
		dsc_info->initial_dec_delay,
		dsc_info->final_offset,
		dsc_info->scale_increment_interval,
		dsc_info->initial_scale_value,
		dsc_info->scale_decrement_interval,
		dsc_info->nfl_bpg_offset,
		dsc_info->slice_bpg_offset
	);

	dpu_pr_info("[DSC] %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
		dsc_info->dsc_bpc,
		dsc_info->dsc_bpp,
		dsc_info->slice_width,
		dsc_info->slice_height,
		dsc_info->initial_xmit_delay,
		dsc_info->first_line_bpg_offset,
		dsc_info->block_pred_enable,
		dsc_info->linebuf_depth,
		dsc_info->initial_offset,
		dsc_info->flatness_min_qp,
		dsc_info->flatness_max_qp,
		dsc_info->rc_edge_factor,
		dsc_info->rc_model_size,
		dsc_info->rc_tgt_offset_lo,
		dsc_info->rc_tgt_offset_hi,
		dsc_info->rc_quant_incr_limit1,
		dsc_info->rc_quant_incr_limit0
	);
}
