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

#include <dpu/dpu_dm.h>
#include <dpu/soc_dpu_format.h>
#include <dpu_format.h>
#include "sdma_cmd_data.h"
#include "../opr_cmd_data.h"
#include "../opr_format.h"
#include "dkmd_log.h"

struct opr_cmd_data *init_sdma_cmd_data(union dkmd_opr_id id)
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

	cmd_data->set_data = opr_set_sdma_data;
	cmd_data->data->opr_id = id;

	return cmd_data;
}

int32_t opr_set_sdma_data(struct opr_cmd_data *cmd_data, const struct dkmd_base_layer *base_layer,
	const struct opr_cmd_data *pre_cmd_data, const struct opr_cmd_data **next_cmd_datas, uint32_t next_oprs_num)
{
	struct dpu_dm_param *dm_param = NULL;
	struct dpu_dm_layer_info *layer_info = NULL;
	uint64_t iova;
	uint32_t width;
	uint32_t height;

	dpu_assert(!cmd_data);
	dpu_assert(!base_layer);
	dpu_assert(!next_cmd_datas);
	dpu_assert(next_oprs_num < 1);

	(void)pre_cmd_data;
	dm_param = cmd_data->data->dm_param;
	dpu_assert(!dm_param);
	layer_info = &dm_param->layer_info[0];

	width = rect_width(&base_layer->src_rect);
	height = rect_height(&base_layer->src_rect);
	if (width == 0 || height == 0) {
		dpu_pr_err("width=%u or height=%u is zero", width, height);
		return -1;
	}

	dm_param->scene_info.layer_number.reg.layer_number = 1;
	layer_info->layer_sblk_type.reg.layer_id = 0;
	layer_info->layer_height.reg.layer_width = width - 1;
	layer_info->layer_height.reg.layer_height = height - 1;

	if (get_bpp_by_dpu_format(base_layer->format) == -1) {
		dpu_pr_err("base_layer->format=%d not support", base_layer->format);
		return -1;
	}
	iova = base_layer->iova + base_layer->src_rect.top * (uint64_t)base_layer->stride +
		base_layer->src_rect.left * (uint64_t)get_bpp_by_dpu_format(base_layer->format);
	layer_info->layer_start_addr0_l.value = iova & 0xFFFFFFFF;
	layer_info->layer_stride0.value = base_layer->stride;
	layer_info->layer_dma_sel.reg.layer_dma_fmt = (uint32_t)dpu_fmt_to_sdma(base_layer->format);
	layer_info->layer_dma_sel.reg.layer_fbc_type = base_layer->compress_type;
	layer_info->layer_dma_sel.reg.layer_dma_sel = BIT(cmd_data->data->opr_id.info.ins);

	layer_info->layer_bot_crop.reg.layer_nxt_order =
		(uint32_t)opr_dpu_to_soc_type(next_cmd_datas[0]->data->opr_id.info.type);
	layer_info->layer_bot_crop.reg.layer_ov_fmt = (uint32_t)dpu_fmt_to_dynamic_dfc(base_layer->format);
	cmd_data->data->out_common_data.format = base_layer->format;

	return 0;
}
