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
#include "dpp_cmd_data.h"
#include "config/dpu_config_utils.h"
#include "cmdlist_interface.h"
#include "dkmd_log.h"

static uint32_t g_dpp_offset[OPR_DPP_NUM] = {
	DPU_DPP0_OFFSET,
	DPU_DPP1_OFFSET,
};

struct opr_cmd_data *init_dpp_cmd_data(union dkmd_opr_id id)
{
	struct opr_cmd_data *cmd_data = NULL;
	struct dpp_cmd_data *dpp_data = NULL;

	cmd_data = kzalloc(sizeof(*cmd_data), GFP_KERNEL);
	if (unlikely(!cmd_data)) {
		dpu_pr_err("alloc cmd_data mem fail");
		return NULL;
	}

	dpp_data = kzalloc(sizeof(*dpp_data), GFP_KERNEL);
	if (unlikely(!dpp_data)) {
		dpu_pr_err("alloc dpp_data mem fail");
		kfree(cmd_data);
		return NULL;
	}

	cmd_data->data = (struct opr_cmd_data_base *)dpp_data;
	cmd_data->set_data = opr_set_dpp_data;

	dpp_data->base_data.opr_id = id;
	dpp_data->base_data.module_offset = g_dpp_offset[id.info.ins];

	dpu_alsc_init(&dpp_data->alsc);
	dpp_data->alsc.alsc_base = dpu_config_get_ip_base(DISP_IP_BASE_DPU) + dpp_data->base_data.module_offset;

	return cmd_data;
}

static void opr_set_dpp_cfg_data(struct opr_cmd_data_base *data)
{
	dpu_alsc_set_reg(data);

	dkmd_set_reg(data->scene_id, data->reg_cmdlist_id, DPU_DPP_REG_CTRL_ADDR(data->module_offset), data->scene_id);
	dkmd_set_reg(data->scene_id, data->reg_cmdlist_id, DPU_DPP_REG_CTRL_FLUSH_EN_ADDR(data->module_offset), 1);
}

int32_t opr_set_dpp_data(struct opr_cmd_data *cmd_data, const struct dkmd_base_layer *base_layer,
	const struct opr_cmd_data *pre_cmd_data, const struct opr_cmd_data **next_cmd_datas, uint32_t next_oprs_num)
{
	struct dpu_dm_param *dm_param = NULL;
	struct dpu_dm_dpp_info *dpp_info = NULL;
	uint32_t width;
	uint32_t height;

	dpu_assert(!cmd_data);
	dpu_assert(!pre_cmd_data);
	dpu_assert(!next_cmd_datas);
	dpu_assert(next_oprs_num < 1);

	(void)base_layer;
	dm_param = cmd_data->data->dm_param;
	dpu_assert(!dm_param);
	dpp_info = &dm_param->dpp_info;

	set_common_cmd_data(cmd_data, pre_cmd_data);
	width = rect_width(&cmd_data->data->in_common_data.rect);
	height = rect_height(&cmd_data->data->in_common_data.rect);
	if (width == 0 || height == 0) {
		dpu_pr_err("width=%u or height=%u is zero", width, height);
		return -1;
	}

	++dm_param->scene_info.ddic_number.reg.dpp_number;

	dpp_info->dpp_input_img_width.reg.dpp_input_img_width = width - 1;
	dpp_info->dpp_input_img_width.reg.dpp_input_img_height = height - 1;
	dpp_info->dpp_output_img_width.reg.dpp_output_img_width = width - 1;
	dpp_info->dpp_output_img_width.reg.dpp_output_img_height = height - 1;

	dpp_info->dpp_sel.reg.dpp_sel = BIT(cmd_data->data->opr_id.info.ins);
	dpp_info->dpp_sel.reg.dpp_order1 = OPR_INVALID;
	dpp_info->dpp_sel.reg.dpp_order0 = (uint32_t)opr_dpu_to_soc_type(next_cmd_datas[0]->data->opr_id.info.type);
	dpp_info->dpp_sel.reg.dpp_layer_id = POST_LAYER_ID;

	dpp_info->dpp_reserved.reg.dpp_output_fmt = SDMA_FMT_ARGB_10101010;
	dpp_info->dpp_reserved.reg.dpp_input_fmt = SDMA_FMT_ARGB_10101010;

	opr_set_dpp_cfg_data(cmd_data->data);

	return 0;
}