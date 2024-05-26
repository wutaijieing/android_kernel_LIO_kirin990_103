/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#include <linux/slab.h>

#include <dpu_format.h>
#include <dpu/dpu_dm.h>
#include "ov_cmd_data.h"
#include "../opr_cmd_data.h"
#include "../opr_format.h"
#include "dkmd_log.h"

struct opr_cmd_data *init_ov_cmd_data(union dkmd_opr_id id)
{
	struct opr_cmd_data *cmd_data = NULL;

	cmd_data = kzalloc(sizeof(*cmd_data), GFP_KERNEL);
	if (unlikely(!cmd_data)) {
		dpu_pr_err("alloc cmd_data mem fail");
		return NULL;
	}

	cmd_data->data = kzalloc(sizeof(*cmd_data->data), GFP_KERNEL);
	if (unlikely(!cmd_data->data)) {
		dpu_pr_err("alloc cmd_data->data mem fail");
		kfree(cmd_data);
		return NULL;
	}

	cmd_data->set_data = opr_set_ov_data;
	cmd_data->data->opr_id = id;

	return cmd_data;
}

static void ov_set_layer_info(struct opr_cmd_data *cmd_data, const struct dkmd_base_layer *base_layer,
	struct dpu_dm_layer_info *layer_info)
{
	layer_info->layer_bot_crop.reg.layer_ov_fmt =
		(uint32_t)dpu_fmt_to_dynamic_dfc(cmd_data->data->in_common_data.format);
	layer_info->layer_ov_starty.reg.layer_ov_startx = base_layer->dst_rect.left;
	layer_info->layer_ov_starty.reg.layer_ov_starty = base_layer->dst_rect.top;
	layer_info->layer_ov_endy.reg.layer_ov_endx =
		(rect_width(&base_layer->dst_rect) > 0) ? rect_width(&base_layer->dst_rect) - 1 : 0;
	layer_info->layer_ov_endy.reg.layer_ov_endy =
		(rect_height(&base_layer->dst_rect) > 0) ? rect_height(&base_layer->dst_rect) - 1 : 0;

	layer_info->layer_ov_pattern_a.reg.layer_ov_enable = 1;
	layer_info->layer_ov_pattern_a.reg.layer_ov_dfc_en = 1;

	// OV_BLEND_SRC
	layer_info->layer_ov_mode.value = 0;
	layer_info->layer_ov_mode.reg.layer_ov_dst_amode = 1;
	layer_info->layer_ov_mode.reg.layer_clip_en = 1;
	layer_info->layer_ov_mode.reg.layer_bitext_en = 1;

	layer_info->layer_ov_alpha.value = 0;
	layer_info->layer_ov_alpha.reg.layer_ov_dst_global_alpha = 0xFF * 4;
	layer_info->layer_ov_alpha.reg.layer_ov_src_global_alpha = 0xFF * 4;
}

int32_t opr_set_ov_data(struct opr_cmd_data *cmd_data, const struct dkmd_base_layer *base_layer,
	const struct opr_cmd_data *pre_cmd_data, const struct opr_cmd_data **next_cmd_datas, uint32_t next_oprs_num)
{
	struct dpu_dm_param *dm_param = NULL;
	struct dpu_dm_ov_info *ov_info = NULL;
	struct dpu_dm_layer_info *layer_info = NULL;
	uint32_t width;
	uint32_t height;

	dpu_assert(!cmd_data);
	dpu_assert(!next_cmd_datas);
	dpu_assert(next_oprs_num < 1);

	set_common_cmd_data(cmd_data, pre_cmd_data);
	dm_param = cmd_data->data->dm_param;
	dpu_assert(!dm_param);
	ov_info = &dm_param->ov_info;
	layer_info = &dm_param->layer_info[0];

	cmd_data->data->out_common_data.rect = next_cmd_datas[0]->data->in_common_data.rect;
	cmd_data->data->out_common_data.format = DPU_FMT_ARGB_10101010;

	width = rect_width(&next_cmd_datas[0]->data->in_common_data.rect);
	height = rect_height(&next_cmd_datas[0]->data->in_common_data.rect);
	if (width == 0 || height == 0) {
		dpu_pr_err("width=%u or height=%u is zero", width, height);
		return -1;
	}

	ov_info->ov_img_width.reg.ov_img_width = width - 1;
	ov_info->ov_img_width.reg.ov_img_height = height - 1;
	ov_info->ov_blayer_endx.reg.ov_blayer_startx = 0;
	ov_info->ov_blayer_endy.reg.ov_blayer_starty = 0;
	ov_info->ov_blayer_endx.reg.ov_blayer_endx = width - 1;
	ov_info->ov_blayer_endy.reg.ov_blayer_endy = height - 1;
	ov_info->ov_order0.reg.ov_bg_color_cfg = 0xFFFF;
	ov_info->ov_order0.reg.ov_sel = BIT(cmd_data->data->opr_id.info.ins);
	ov_info->ov_order0.reg.ov_order0 = (uint32_t)opr_dpu_to_soc_type(next_cmd_datas[0]->data->opr_id.info.type);
	ov_info->ov_reserved_0.reg.ov_order1 = OPR_INVALID;
	if (next_oprs_num > 1)
		ov_info->ov_reserved_0.reg.ov_order1 = (uint32_t)opr_dpu_to_soc_type(next_cmd_datas[1]->data->opr_id.info.type);

	ov_set_layer_info(cmd_data, base_layer, layer_info);

	return 0;
}