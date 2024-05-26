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

#include <linux/platform_device.h>
#include <linux/types.h>
#include <soc_dte_define.h>

#include "hisi_operators_manager.h"
#include "hisi_disp_gadgets.h"
#include "dpu_disp_srot.h"
#include "hisi_disp_config.h"
#include "hisi_disp_composer.h"
#include "hisi_operator_tool.h"
#include "dm/hisi_dm.h"

static int dpu_dm_srot_set_params(const struct hisi_comp_operator *operator, const struct pipeline_src_layer *src_layer,
	struct hisi_dm_param *dm_param)
{
	int srot_sel = operator->id_desc.info.idx;
	struct dpu_dm_srot0_info *p_srot_info;

	if (srot_sel >= DM_SROT_NUM) {
		disp_pr_err("[SROT] srot_sel:%d overflow\n", srot_sel);
		return -1;
	}

	p_srot_info = &dm_param->srot_info[srot_sel];
	p_srot_info->srot0_input_img_width_union.reg.srot0_input_img_height = operator->in_data->rect.h - 1;
	p_srot_info->srot0_input_img_width_union.reg.srot0_input_img_width = operator->in_data->rect.w - 1;
	p_srot_info->srot0_output_img_width_union.reg.srot0_output_img_height = operator->out_data->rect.h - 1;
	p_srot_info->srot0_output_img_width_union.reg.srot0_output_img_width = operator->out_data->rect.w - 1;
	p_srot_info->srot0_input_fmt_union.reg.srot0_layer_id = src_layer->layer_id;
	p_srot_info->srot0_input_fmt_union.reg.srot0_order0 = hisi_get_op_id_by_op_type(operator->out_data->next_order);
	p_srot_info->srot0_input_fmt_union.reg.srot0_sel = BIT(srot_sel);
	p_srot_info->srot0_input_fmt_union.reg.srot0_input_fmt = operator->in_data->format;
	p_srot_info->srot0_reserved_union.reg.srot0_output_fmt = operator->out_data->format;

	return 0;
}

static int dpu_dm_srot_set_reg(struct hisi_comp_operator *operator, const struct hisi_dm_param *dm_param, char __iomem *dpu_base_addr)
{
	int srot_sel = operator->id_desc.info.idx;
	char __iomem *dm_base = dpu_base_addr + dpu_get_dm_offset(operator->scene_id);
	const struct dpu_dm_srot0_info *p_srot_info = &dm_param->srot_info[srot_sel];
	uint32_t srot1_offset = srot_sel * 0x10;

	hisi_module_set_reg(&operator->module_desc, DPU_DM_SROT0_INPUT_IMG_WIDTH_ADDR(dm_base + srot1_offset), p_srot_info->srot0_input_img_width_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_SROT0_OUTPUT_IMG_WIDTH_ADDR(dm_base + srot1_offset), p_srot_info->srot0_output_img_width_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_SROT0_INPUT_FMT_ADDR(dm_base + srot1_offset), p_srot_info->srot0_input_fmt_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_SROT0_RESERVED_ADDR(dm_base + srot1_offset), p_srot_info->srot0_reserved_union.value);

	return 0;
}

static int dpu_srot_set_cmd_item(struct hisi_comp_operator *operator, struct hisi_composer_device *composer_device, void *layer)
{
	int ret;
	struct dpu_operator_srot *operator_srot = NULL;
	struct pipeline_src_layer *src_layer = (struct pipeline_src_layer *)layer;
	struct hisi_dm_param *dm_param = composer_device->ov_data.dm_param;
	char __iomem *dpu_base_addr = composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU];

	operator_srot = (struct dpu_operator_srot *)operator;

	ret = dpu_dm_srot_set_params(operator, src_layer, dm_param);
	if (ret)
		return -1;

	dpu_dm_srot_set_reg(operator, dm_param, dpu_base_addr);

	return 0;
}

static int dpu_srot_build_data(struct hisi_comp_operator *operator, void *layer, struct hisi_pipeline_data *_pre_out_data,
	struct hisi_comp_operator *next_operator)
{
	struct pipeline_src_layer *src_layer;

	if (!layer) {
		disp_pr_err("[SROT] layer is NULL pointer\n");
		return -1;
	}

	src_layer = (struct pipeline_src_layer *)layer;

	if (_pre_out_data) {
		operator->in_data->format = _pre_out_data->format;
		operator->in_data->rect = _pre_out_data->rect;
	}

	memcpy(operator->out_data, operator->in_data, sizeof(*(operator->in_data)));

	dpu_operator_build_data(operator, layer, _pre_out_data, next_operator);

	return 0;
}

void dpu_srot_init(struct hisi_operator_type *type_operator, struct dpu_module_ops ops, void *cookie)
{
	struct dpu_operator_srot **pp_srot = NULL;
	struct hisi_comp_operator *base = NULL;
	uint32_t i;

	disp_pr_info("[SROT] ++++\n");
	pp_srot = kzalloc(sizeof(*pp_srot) * type_operator->operator_count, GFP_KERNEL);
	if (!pp_srot)
		return;

	for (i = 0; i < type_operator->operator_count; i++) {
		pp_srot[i] = kzalloc(sizeof(*(pp_srot[i])), GFP_KERNEL);
		if (!pp_srot[i])
			continue;

		hisi_operator_init(&pp_srot[i]->base, ops, cookie);

		base = &pp_srot[i]->base;
		base->id_desc.info.idx = i;
		base->id_desc.info.type = type_operator->type;
		base->set_cmd_item = dpu_srot_set_cmd_item;
		base->build_data = dpu_srot_build_data;

		sema_init(&pp_srot[i]->base.operator_sem, 1);

		base->out_data = kzalloc(sizeof(struct dpu_srot_out_data), GFP_KERNEL);
		if (!base->out_data) {
			disp_pr_err("alloc out_data failed\n");
			kfree(pp_srot[i]);
			pp_srot[i] = NULL;
			continue;
		}

		base->in_data = kzalloc(sizeof(struct dpu_srot_in_data), GFP_KERNEL);
		if (!base->in_data) {
			disp_pr_err("alloc in_data failed\n");
			kfree(base->out_data);
			base->out_data = NULL;

			kfree(pp_srot[i]);
			pp_srot[i] = NULL;
			continue;
		}

		pp_srot[i]->base.be_dm_counted = false;
	}

	type_operator->operators = (struct hisi_comp_operator **)(pp_srot);
	disp_pr_info("[SROT] ----\n");
}
