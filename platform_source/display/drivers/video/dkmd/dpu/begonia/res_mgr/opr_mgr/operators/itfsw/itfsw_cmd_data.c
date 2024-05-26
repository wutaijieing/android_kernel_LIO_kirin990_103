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
#include "itfsw_cmd_data.h"
#include "config/dpu_opr_config.h"
#include "../opr_cmd_data.h"
#include "cmdlist_interface.h"
#include "smmu/dpu_comp_smmu.h"
#include "dkmd_log.h"

struct opr_cmd_data *init_itfsw_cmd_data(union dkmd_opr_id id)
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

	cmd_data->set_data = opr_set_itfsw_data;
	cmd_data->data->opr_id = id;

	return cmd_data;
}

static void opr_set_itfsw_cfg_data(const struct opr_cmd_data_base *data)
{
	dkmd_set_reg(data->scene_id, data->reg_cmdlist_id,
		DPU_ITF_CH_DPP_CLIP_EN_ADDR(g_itfsw_offset[data->scene_id]), 0);

	dkmd_set_reg(data->scene_id, data->reg_cmdlist_id,
		DPU_ITF_CH_DPP_CLRBAR_CTRL_ADDR(g_itfsw_offset[data->scene_id]), 0);

	dkmd_set_reg(data->scene_id, data->reg_cmdlist_id,
		DPU_ITF_CH_REG_CTRL_ADDR(g_itfsw_offset[data->scene_id]), data->scene_id);

	dkmd_set_reg(data->scene_id, data->reg_cmdlist_id,
		DPU_ITF_CH_ITFSW_DATA_SEL_ADDR(g_itfsw_offset[data->scene_id]), 0);

	dkmd_set_reg(data->scene_id, data->reg_cmdlist_id,
		DPU_ITF_CH_REG_CTRL_FLUSH_EN_ADDR(g_itfsw_offset[data->scene_id]), 1);
}

int32_t opr_set_itfsw_data(struct opr_cmd_data *cmd_data, const struct dkmd_base_layer *base_layer,
	const struct opr_cmd_data *pre_cmd_data, const struct opr_cmd_data **next_cmd_datas, uint32_t next_oprs_num)
{
	struct dpu_dm_param *dm_param = NULL;
	struct dpu_dm_itfsw_info *itfsw_info = NULL;
	struct opr_cmd_data_base *data = NULL;
	uint32_t width;
	uint32_t height;

	dpu_assert(!cmd_data);
	dpu_assert(!pre_cmd_data);
	dpu_assert(next_oprs_num > 0);

	(void)base_layer;
	(void)next_cmd_datas;
	data = cmd_data->data;
	dm_param = data->dm_param;
	dpu_assert(!dm_param);

	data->in_common_data = pre_cmd_data->data->out_common_data;

	itfsw_info = &dm_param->itfsw_info;
	++dm_param->scene_info.layer_number.reg.itf_number;

	width = rect_width(&data->in_common_data.rect);
	height = rect_height(&data->in_common_data.rect);
	if (width == 0 || height == 0) {
		dpu_pr_err("width=%u or height=%u is zero", width, height);
		return -1;
	}
	itfsw_info->itf_input_img_width.reg.itf_input_img_width = width - 1;
	itfsw_info->itf_input_img_width.reg.itf_input_img_height = height - 1;

	itfsw_info->itf_input_fmt.reg.itf_input_fmt = SDMA_FMT_ARGB_10101010;
	itfsw_info->itf_input_fmt.reg.itf_layer_id = POST_LAYER_ID; /* 31 means no relationship to layer */
	itfsw_info->itf_input_fmt.reg.itf_sel = BIT(data->scene_id);
	itfsw_info->itf_input_fmt.reg.itf_hidic_en = 0;

	opr_set_itfsw_cfg_data(data);
	dpu_comp_smmu_ch_set_reg(data->reg_cmdlist_id, data->scene_id, g_pan_display_frame_index);

	return 0;
}