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
#include <linux/dma-mapping.h>

#include "hisi_drv_utils.h"
#include "hisi_disp.h"
#include "hisi_disp_composer.h"
#include "hisi_dpu_module.h"
#include "hisi_operators_manager.h"

#include "scl/dpu_scl.h"
#include "scl/vscf/dpu_operator_vscf.h"
#include "widget/dpu_csc.h"
#include "widget/dpu_dfc.h"
#include "widget/dpu_post_clip.h"
#include "widget/dpu_scf.h"
#include "widget/dpu_scf_lut.h"
#include "hisi_operator_tool.h"

static uint32_t g_vscf_offset[] = {
	DPU_VSCF0_OFFSET, DPU_VSCF1_OFFSET
};

static void dpu_vscf_set_dm_scl_section(struct dpu_operator_vscf *vscf, struct pipeline_src_layer *layer,
		struct hisi_dm_scl_info *scl_info, char __iomem *dpu_base_addr)
{
	struct dpu_module_desc *module = &vscf->base.module_desc;
	char __iomem *dm_scl_base = dpu_base_addr + dpu_get_dm_offset(vscf->base.scene_id) + vscf->base.dm_offset;

	/* scl input and output information must be calculated by build_data() */
	scl_info->scl0_input_img_width_union.reg.scl0_input_img_height = DPU_HEIGHT(vscf->base.in_data->rect.h);
	scl_info->scl0_input_img_width_union.reg.scl0_input_img_width = DPU_WIDTH(vscf->base.in_data->rect.w);

	scl_info->scl0_output_img_width_union.reg.scl0_output_img_height = DPU_HEIGHT(vscf->base.out_data->rect.h);
	scl_info->scl0_output_img_width_union.reg.scl0_output_img_width = DPU_WIDTH(vscf->base.out_data->rect.w);

	scl_info->scl0_type_union.reg.scl0_layer_id = layer->layer_id;
	scl_info->scl0_type_union.reg.scl0_order0 = hisi_get_op_id_by_op_type(vscf->base.in_data->next_order);
	scl_info->scl0_type_union.reg.scl0_sel = BIT(vscf->base.id_desc.info.idx % dpu_config_get_scl_count());
	scl_info->scl0_type_union.reg.scl0_type = SCL_TYPE_VSCF;

	scl_info->scl0_threshold_union.reg.scl0_input_fmt = vscf->base.in_data->format;
	scl_info->scl0_threshold_union.reg.scl0_output_fmt = vscf->base.out_data->format;
	scl_info->scl0_threshold_union.reg.scl0_pre_post_sel = 0; // scl must be before overlay
	scl_info->scl0_threshold_union.reg.scl0_threshold = SCL_THRESHOLD_DEFAULT_LEVEL;

	/* DM SCL Section */
	hisi_module_set_reg(module, DPU_DM_SCL0_INPUT_IMG_WIDTH_ADDR(dm_scl_base), scl_info->scl0_input_img_width_union.value);
	hisi_module_set_reg(module, DPU_DM_SCL0_OUTPUT_IMG_WIDTH_ADDR(dm_scl_base), scl_info->scl0_output_img_width_union.value);
	hisi_module_set_reg(module, DPU_DM_SCL0_TYPE_ADDR(dm_scl_base), scl_info->scl0_type_union.value);
	hisi_module_set_reg(module, DPU_DM_SCL0_THRESHOLD_ADDR(dm_scl_base), scl_info->scl0_threshold_union.value);
}

static void dpu_vscf_set_regs(struct dpu_operator_vscf *vscf, struct pipeline_src_layer *src_layer,
		char __iomem *dpu_base_addr)
{
	char __iomem *vscf_base = dpu_base_addr + vscf->base.operator_offset;
	struct dpu_module_desc *module = &vscf->base.module_desc;
	DPU_VSCF_REG_CTRL_UNION vscf_reg_ctrl;
	struct dpu_dfc_data dfc_data = {0};
	struct dpu_csc_data csc_data = {0};
	struct dpu_scf_data scf_data = {0};
	struct dpu_post_clip_data post_clip_data = {0};

	/*
	 * DFC
	 */
	dfc_data.clip_rect.bottom = 0;
	dfc_data.clip_rect.top = 0;
	dfc_data.clip_rect.left = 0;
	dfc_data.clip_rect.right = 0;
	dfc_data.dfc_base = vscf_base;
	dfc_data.enable_dither = 0;
	dfc_data.format = vscf->base.in_data->format;
	dfc_data.glb_alpha = 0;
	dfc_data.src_rect = vscf->base.in_data->rect;
	dfc_data.dst_rect = vscf->base.out_data->rect;
	dfc_data.enable_icg_submodule = has_same_dim(&(dfc_data.src_rect), &(dfc_data.dst_rect)) ? 0 : 1;
	dpu_dfc_set_cmd_item(module, &dfc_data);

	/*
	 * CSC
	 */
	csc_data.base = vscf_base;
	csc_data.csc_mode = DEFAULT_CSC_MODE;
	csc_data.direction = dpu_csc_get_direction(dfc_data.format, vscf->base.out_data->format);
	csc_data.out_format = vscf->base.out_data->format;
	dpu_csc_set_cmd_item(module, &csc_data);

	/*
	 * SCF
	 */
	scf_data.base = vscf_base;
	scf_data.has_alpha = has_alpha(csc_data.out_format);
	scf_data.acc_hscl = 0;
	scf_data.acc_vscl = 0;
	scf_data.src_rect = dfc_data.dst_rect;
	scf_data.dst_rect = vscf->base.out_data->rect;
	dpu_scf_set_cmd_item(module, &scf_data);

	/*
	 * TODO: post clip
	 */
	dpu_post_clip_set_cmd_item(module, &post_clip_data);

	/*
	 * TODO: scl lut must be move to power on
	 */
	dpu_scf_lut_set_cmd(module, vscf_base);

	/* scf top reg */
	vscf_reg_ctrl.reg.reg_ctrl_scene_id = vscf->base.scene_id;
	hisi_module_set_reg(module, DPU_VSCF_REG_CTRL_ADDR(vscf_base), vscf_reg_ctrl.value);
	hisi_module_set_reg(module, DPU_VSCF_REG_CTRL_FLUSH_EN_ADDR(vscf_base), 0x1);

	disp_pr_info("scene_id=%d, layer_id=%d", vscf->base.scene_id, src_layer->layer_id);
}

static int dpu_vscf_build_data(struct hisi_comp_operator *operator, void *layer, struct hisi_pipeline_data *pre_out_data,
		 struct hisi_comp_operator *next_operator) // 使用id
{
	struct dpu_operator_vscf *vscf = (struct dpu_operator_vscf *)operator;
	struct pipeline_src_layer *src_layer = (struct pipeline_src_layer *)layer;
	struct dpu_vscf_in_data *in_data = NULL;
	struct dpu_vscf_out_data *out_data = NULL;

	disp_pr_info(" ++++ \n");

	in_data = (struct dpu_vscf_in_data *)vscf->base.in_data;
	disp_assert_if_cond(in_data == NULL);

	out_data = (struct dpu_vscf_out_data *)vscf->base.out_data;
	disp_assert_if_cond(out_data == NULL);

	if (next_operator) {
		disp_pr_info(" next_operator->id_desc.info.type:%u \n", next_operator->id_desc.info.type);
		in_data->base.next_order = next_operator->id_desc.info.type;
	}

	if (pre_out_data) {
		in_data->base.format = pre_out_data->format;
		in_data->base.rect = pre_out_data->rect;
	} else {
		in_data->base.format = dpu_pixel_format_hal2dma(src_layer->img.format, DPU_HAL2SDMA_PIXEL_FORMAT);
		in_data->base.rect = src_layer->src_rect;
	}

	/* TODO: vscf output format maybe is not ARGB, and out rect is not layer dest rect, such as offline block segment */
	out_data->base.rect = src_layer->dst_rect;
	out_data->base.format = has_same_dim(&(in_data->base.rect), &(out_data->base.rect)) ? in_data->base.format : AYUV10101010;

	dpu_operator_build_data(operator, layer, pre_out_data, next_operator);

	dpu_disp_print_pipeline_data(&in_data->base);
	dpu_disp_print_pipeline_data(&out_data->base);

	return 0;
}

static int dpu_vscf_set_cmd_item(struct hisi_comp_operator *operator, struct hisi_composer_device *composer_device,
	void *layer)
{
	struct hisi_dm_param *dm_param = NULL;
	struct dpu_operator_vscf *vscf = NULL;
	struct pipeline_src_layer *src_layer = NULL;
	struct hisi_dm_scl_info *scl_info = NULL;
	char __iomem *dpu_base_addr = NULL;

	disp_pr_info(" ++++ \n");

	vscf = (struct dpu_operator_vscf *)operator;
	dm_param = composer_device->ov_data.dm_param;
	src_layer = (struct pipeline_src_layer *)layer;
	scl_info = &dm_param->scl_info[vscf->base.id_desc.info.idx];
	dpu_base_addr = composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU];
	vscf->base.dm_offset = dpu_scl_alloc_dm_node();

	/* calculate dm parament */
	dpu_vscf_set_dm_scl_section(vscf, src_layer, scl_info, dpu_base_addr);

	/* set dm regs and vscf regs */
	dpu_vscf_set_regs(vscf, src_layer, dpu_base_addr);

	disp_pr_info(" ---- \n");
	return 0;
}

void dpu_vscf_init(struct hisi_operator_type *type_operator, struct dpu_module_ops ops, void *cookie)
{
	struct dpu_operator_vscf **vscfs = NULL;
	struct hisi_comp_operator *base = NULL;
	uint32_t i;

	disp_pr_info(" ++++ \n");
	disp_pr_info(" type_operator->operator_count:%u \n", type_operator->operator_count);

	disp_assert_if_cond(type_operator->operator_count != DPU_VSCF_COUNT);

	vscfs = kzalloc(sizeof(*vscfs) * type_operator->operator_count, GFP_KERNEL);
	if (!vscfs)
		return;

	for (i = 0; i < type_operator->operator_count; i++) {
		disp_pr_info(" size of struct dpu_operator_vscf:%u \n", sizeof(*(vscfs[i])));
		vscfs[i] = kzalloc(sizeof(*(vscfs[i])), GFP_KERNEL);
		if (!vscfs[i])
			continue;

		base = &vscfs[i]->base;

		/* TODO: error check */
		hisi_operator_init(base, ops, cookie);
		base->id_desc.info.idx = i;
		base->id_desc.info.type = type_operator->type;
		base->set_cmd_item = dpu_vscf_set_cmd_item;
		base->build_data = dpu_vscf_build_data;

		/* idx is bigger than count, idx will be 0,1,2,3， count is 2 */
		base->operator_offset = g_vscf_offset[base->id_desc.info.idx];

		sema_init(&base->operator_sem, 1);
		disp_pr_ops(info, base->id_desc);

		base->out_data = kzalloc(sizeof(struct dpu_vscf_out_data), GFP_KERNEL);
		if (!base->out_data) {
			kfree(vscfs[i]);
			vscfs[i] = NULL;
			continue;
		}

		base->in_data = kzalloc(sizeof(struct dpu_vscf_in_data), GFP_KERNEL);
		if (!base->in_data) {
			kfree(base->out_data);
			base->out_data = NULL;

			kfree(vscfs[i]);
			vscfs[i] = NULL;
			continue;
		}

		/* TODO: init other rmda operators */

		base->be_dm_counted = false;
	}

	type_operator->operators = (struct hisi_comp_operator **)(vscfs);
}
