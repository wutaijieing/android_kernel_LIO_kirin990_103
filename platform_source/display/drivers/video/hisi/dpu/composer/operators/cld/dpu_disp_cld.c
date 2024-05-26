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

#include <linux/types.h>
#include <soc_dte_define.h>

#include "dpu_disp_cld.h"
#include "hisi_operators_manager.h"
#include "hisi_disp_gadgets.h"
#include "hisi_disp_config.h"
#include "hisi_disp_composer.h"
#include "cmdlist.h"
#include "dm/hisi_dm.h"

#define CLT_CLD_REG_SIZE 256
#define CLT_CLD_REG_RANGE_SIZE 0

static uint32_t g_cld_offset[] = { DSS_CLD0_OFFSET, DSS_CLD1_OFFSET };

static int dpu_dm_cld_set_params(const struct dpu_operator_cld *operator, const struct pipeline_src_layer *src_layer,
	struct hisi_dm_param *dm_param)
{
	int cld_idx = operator->base.id_desc.info.idx;
	int cld_sel = 0; // use CLD0 hardware
	struct dpu_dm_cld0_info *p_cld_info;
	if (src_layer->cld_reuse)
		cld_idx = 1;
	disp_pr_info("cld_idx:%d, layer_id:%d\n",cld_idx, src_layer->layer_id);

	if (cld_idx >= DM_CLD_NUM) {
		disp_pr_err("[CLD] cld_idx:%d overflow\n", cld_idx);
		return -1;
	}

	if (cld_idx == 0)
		dm_param->cmdlist_addr.cmdlist_cld0_addr.value = operator->client->cmd_header_addr;
	else
		dm_param->cmdlist_addr.cmdlist_cld1_addr.value = operator->client->cmd_header_addr;

	p_cld_info = &dm_param->cld_info[cld_idx];
	p_cld_info->cld0_input_img_width_union.reg.cld0_input_img_height = operator->base.in_data->rect.h - 1;
	p_cld_info->cld0_input_img_width_union.reg.cld0_input_img_width = operator->base.in_data->rect.w - 1;
	p_cld_info->cld0_output_img_width_union.reg.cld0_output_img_height = operator->base.out_data->rect.h - 1;
	p_cld_info->cld0_output_img_width_union.reg.cld0_output_img_width = operator->base.out_data->rect.w - 1;
	p_cld_info->cld0_input_fmt_union.reg.cld0_layer_id = src_layer->layer_id;
	p_cld_info->cld0_input_fmt_union.reg.cld0_order0 = hisi_get_op_id_by_op_type(operator->base.out_data->next_order);
	p_cld_info->cld0_input_fmt_union.reg.cld0_sel = 1 << cld_sel;
	operator->client->list_cmd_header->total_items.bits.dma_dst_addr = g_cld_offset[cld_sel] >> 2;
	p_cld_info->cld0_input_fmt_union.reg.cld0_input_fmt = operator->base.in_data->format;
	p_cld_info->cld0_reserved_union.reg.cld0_output_fmt = operator->base.out_data->format;

	return 0;
}

static int dpu_dm_cld_set_reg(struct dpu_operator_cld *operator, const struct hisi_dm_param *dm_param,
	char __iomem *dpu_base_addr, const struct pipeline_src_layer *src_layer)
{
	int cld_idx = operator->base.id_desc.info.idx;
	char __iomem *dm_base = dpu_base_addr + dpu_get_dm_offset(operator->base.scene_id);
	if (src_layer->cld_reuse)
		cld_idx = 1;
	const struct dpu_dm_cld0_info *p_cld_info = &(dm_param->cld_info[cld_idx]);
	uint32_t cld1_offset = cld_idx * 0x10;

	disp_pr_info(" ++++ cld_idx=%d\n", cld_idx);

	hisi_module_set_reg(&operator->base.module_desc, DPU_DM_CLD0_INPUT_IMG_WIDTH_ADDR(dm_base + cld1_offset), p_cld_info->cld0_input_img_width_union.value);
	hisi_module_set_reg(&operator->base.module_desc, DPU_DM_CLD0_OUTPUT_IMG_WIDTH_ADDR(dm_base + cld1_offset), p_cld_info->cld0_output_img_width_union.value);
	hisi_module_set_reg(&operator->base.module_desc, DPU_DM_CLD0_INPUT_FMT_ADDR(dm_base + cld1_offset), p_cld_info->cld0_input_fmt_union.value);
	hisi_module_set_reg(&operator->base.module_desc, DPU_DM_CLD0_RESERVED_ADDR(dm_base + cld1_offset), p_cld_info->cld0_reserved_union.value);

	return 0;
}

static int dpu_cld_set_params(struct dpu_operator_cld *cld, struct dpu_cld_params *cld_params, const struct pipeline_src_layer *src_layer)
{
	disp_pr_info("[CLD] ++++\n");
	cld_params->cld_rgb_union.value = src_layer->glb_color;
	cld_params->cld_en_union.value = 1;
	cld_params->cld_size_union.reg.cld_size_hrz = src_layer->dst_rect.w - 1;
	cld_params->cld_size_union.reg.cld_size_vrt = src_layer->dst_rect.h - 1;
	cld_params->cld_reg_ctrl_union.reg.reg_ctrl_scene_id = cld->base.scene_id;
	cld_params->cld_reg_ctrl_flush_en_union.value = 1;

	disp_pr_info("[CLD] [cld_rgb_union cld_size_hrz cld_size_vrt]=[0x%x 0x%x 0x%x]\n", cld_params->cld_rgb_union.value,
		cld_params->cld_size_union.reg.cld_size_hrz, cld_params->cld_size_union.reg.cld_size_vrt);
	return 0;
}

static void dpu_cld_set_regs(struct dpu_operator_cld *operator,
	const struct dpu_cld_params *cld_params, char __iomem *dpu_base_addr)
{
	char __iomem *cld_base;
	int cld_idx = operator->base.id_desc.info.idx;

	cld_base = dpu_base_addr + g_cld_offset[0]; // reuse cld0, must use g_cld_offset[0]

	disp_pr_info("[CLD] cld_idx:%d cld_base:0x%p\n", cld_idx, cld_base);
	disp_pr_info("[CLD] [cld_size_hrz cld_size_vrt]=[0x%x 0x%x]\n",
		cld_params->cld_size_union.reg.cld_size_hrz, cld_params->cld_size_union.reg.cld_size_vrt);

	hisi_module_set_reg(&operator->base.module_desc, DPU_CLD_RGB_ADDR(cld_base), cld_params->cld_rgb_union.value);
	hisi_module_set_reg(&operator->base.module_desc, DPU_CLD_EN_ADDR(cld_base), cld_params->cld_en_union.value);
	hisi_module_set_reg(&operator->base.module_desc, DPU_CLD_SIZE_ADDR(cld_base), cld_params->cld_size_union.value);
	hisi_module_set_reg(&operator->base.module_desc, DPU_CLD_REG_CTRL_ADDR(cld_base), cld_params->cld_reg_ctrl_union.value);
	hisi_module_set_reg(&operator->base.module_desc, DPU_CLD_REG_CTRL_FLUSH_EN_ADDR(cld_base), cld_params->cld_reg_ctrl_flush_en_union.value);
}

static int dpu_cld_alloc_cmdlist_client(uint32_t i, struct dpu_operator_cld *operator)
{
	if (!operator->client)
		operator->client = dpu_cmdlist_create_client(DPU_SCENE_INITAIL, TRANSPORT_MODE);

	if (!operator->client) {
		disp_pr_err("[CLD] cmdlist client is NULL\n");
		return -1;
	}

	operator->cld_params = (struct dpu_cld_params *)operator->client->list_cmd_item;
	operator->client->list_cmd_header->total_items.bits.items_count = sizeof(*(operator->cld_params)) >> 4; /* 128 bit */
	operator->client->list_cmd_header->cmd_flag.bits.task_end = 1;
	operator->client->list_cmd_header->cmd_flag.bits.last = 1;

	return 0;
}

static int dpu_cld_set_cmd_item(struct hisi_comp_operator *operator, struct hisi_composer_device *composer_device,
	void *layer)
{
	int ret;
	struct dpu_operator_cld *operator_cld = NULL;
	struct dpu_cld_params *cld_params = NULL;
	struct pipeline_src_layer *src_layer = (struct pipeline_src_layer *)layer;
	struct hisi_dm_param *dm_param = composer_device->ov_data.dm_param;
	char __iomem *dpu_base_addr = composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU];

	if (!(src_layer->need_caps & CAP_CLD))
		return 0;

	disp_pr_info("[CLD] set cld layer params\n");

	operator_cld = (struct dpu_operator_cld *)operator;

	cld_params = operator_cld->cld_params;
	cld_params->cld_reg_ctrl_union.reg.reg_ctrl_scene_id = operator->scene_id;

	ret = dpu_cld_set_params(operator_cld, cld_params, src_layer);
	if (ret)
		return -1;

	ret = dpu_dm_cld_set_params(operator_cld, src_layer, dm_param);
	if (ret)
		return -1;

	dpu_cld_set_regs(operator_cld, cld_params, dpu_base_addr);
	dpu_dm_cld_set_reg(operator_cld, dm_param, dpu_base_addr, src_layer);

	dpu_cmdlist_dump_client(operator_cld->client);

	return 0;
}

static int dpu_cld_build_data(struct hisi_comp_operator *operator, void *layer, struct hisi_pipeline_data *_pre_out_data,
	struct hisi_comp_operator *next_operator)
{
	struct pipeline_src_layer *src_layer;

	if (!layer) {
		disp_pr_err("[CLD] layer is NULL pointer\n");
		return -1;
	}
	disp_pr_info("[CLD] ++++\n");
	src_layer = (struct pipeline_src_layer *)layer;
	if (!(src_layer->need_caps & CAP_CLD)) {
		return 0;
	}

	disp_pr_info("[CLD] find a cld layer\n");

	operator->out_data->rect.x = src_layer->dst_rect.x;
	operator->out_data->rect.y = src_layer->dst_rect.y;
	operator->out_data->rect.w = src_layer->dst_rect.w;
	operator->out_data->rect.h = src_layer->dst_rect.h;
	operator->out_data->format = ARGB8888; // src_layer->img.format;

	dpu_operator_build_data(operator, layer, _pre_out_data, next_operator);

	return 0;
}

void dpu_cld_init(struct hisi_operator_type *type_operator, struct dpu_module_ops ops, void *cookie)
{
	struct dpu_operator_cld **pp_cld = NULL;
	struct hisi_comp_operator *base = NULL;
	uint32_t i;

	disp_pr_info("[CLD] ++++\n");

	pp_cld = kzalloc(sizeof(*pp_cld) * type_operator->operator_count, GFP_KERNEL);
	if (!pp_cld)
		return;

	for (i = 0; i < type_operator->operator_count; i++) {
		pp_cld[i] = kzalloc(sizeof(*(pp_cld[i])), GFP_KERNEL);
		if (!pp_cld[i])
			continue;

		hisi_operator_init(&pp_cld[i]->base, ops, cookie);

		if (dpu_cld_alloc_cmdlist_client(i, pp_cld[i])) {
			kfree(pp_cld[i]);
			pp_cld[i] = NULL;
			continue;
		}

		base = &pp_cld[i]->base;
		base->id_desc.info.idx = i;
		base->id_desc.info.type = type_operator->type;
		base->set_cmd_item = dpu_cld_set_cmd_item;
		base->build_data = dpu_cld_build_data;

		sema_init(&pp_cld[i]->base.operator_sem, 1);

		base->out_data = kzalloc(sizeof(struct dpu_cld_out_data), GFP_KERNEL);
		if (!base->out_data) {
			disp_pr_err("alloc out_data failed\n");
			kfree(pp_cld[i]);
			pp_cld[i] = NULL;
			continue;
		}

		base->in_data = kzalloc(sizeof(struct dpu_cld_in_data), GFP_KERNEL);
		if (!base->in_data) {
			disp_pr_err("alloc in_data failed\n");
			kfree(base->out_data);
			base->out_data = NULL;

			kfree(pp_cld[i]);
			pp_cld[i] = NULL;
			continue;
		}

		pp_cld[i]->base.be_dm_counted = false;
	}

	type_operator->operators = (struct hisi_comp_operator **)(pp_cld);
	disp_pr_info("[CLD] ----\n");
}
