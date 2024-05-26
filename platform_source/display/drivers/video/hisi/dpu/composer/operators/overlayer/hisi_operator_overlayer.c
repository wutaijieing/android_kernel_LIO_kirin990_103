/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/types.h>
#include <securec.h>
#include "hisi_operators_manager.h"
#include "hisi_drv_utils.h"
#include "hisi_disp_composer.h"
#include "hisi_disp.h"
#include "overlayer/hisi_operator_overlayer.h"
#include "scl/dpu_scl.h"
#include "hisi_dpu_module.h"
#include "hisi_disp_gadgets.h"
#include "hisi_operator_tool.h"
#include "widget/dpu_csc.h"
#include "hdr/dpu_mitm.h"
#include "hdr/hdr_test.h"

static void hisi_overlayer_set_dm_ovinfo(struct hisi_comp_operator *operator, struct hisi_composer_data *ov_data, struct hisi_dm_ov_info *ov_info,
	struct pipeline_src_layer *src_layer)

{
	disp_pr_debug(" ++++ \n");

	if (is_offline_scene(operator->scene_id)) {
		ov_info->ov_img_width_union.reg.ov_img_width = DPU_WIDTH(src_layer->wb_block_rect.w);
		ov_info->ov_img_width_union.reg.ov_img_height = DPU_HEIGHT(src_layer->wb_block_rect.h);

		ov_info->ov_blayer_endx_union.reg.ov_blayer_startx = 0;
		ov_info->ov_blayer_endy_union.reg.ov_blayer_starty = 0;
		ov_info->ov_blayer_endx_union.reg.ov_blayer_endx = DPU_WIDTH(src_layer->wb_block_rect.w);
		ov_info->ov_blayer_endy_union.reg.ov_blayer_endy = DPU_HEIGHT(src_layer->wb_block_rect.h);
		disp_pr_debug("[DATA] ov w = %d, h = %d, endx = %d, endy = %d",
			ov_info->ov_img_width_union.reg.ov_img_width,
			ov_info->ov_img_width_union.reg.ov_img_height,
			ov_info->ov_blayer_endx_union.reg.ov_blayer_endx,
			ov_info->ov_blayer_endy_union.reg.ov_blayer_endy);
	} else {
		ov_info->ov_img_width_union.reg.ov_img_width = ov_data->fix_panel_info->xres - 1;
		ov_info->ov_img_width_union.reg.ov_img_height = ov_data->fix_panel_info->yres - 1;

		ov_info->ov_blayer_endx_union.reg.ov_blayer_endx = ov_data->fix_panel_info->xres - 1;
		ov_info->ov_blayer_endy_union.reg.ov_blayer_endy = ov_data->fix_panel_info->yres - 1;
	}

	ov_info->ov_bg_color_rgb_union.value = 0; //0x000ffc00;//blue 0x3FF00000;red
#ifndef LOCAL_DIMMING_GOLDEN_TEST
	ov_info->ov_order0_union.reg.ov_bg_color_cfg = 0xFFFF; // enbale ov base
#endif
	ov_info->ov_order0_union.reg.ov_sel = BIT(operator->id_desc.info.idx); //BIT(operator->scene_id);
	ov_info->ov_order0_union.reg.ov_order0 = hisi_get_op_id_by_op_type(operator->in_data->next_order); /* 52 means ITFSW0*/

	disp_pr_debug("ov_sel=%u, ov_order0=%u", ov_info->ov_order0_union.reg.ov_sel, ov_info->ov_order0_union.reg.ov_order0);

	ov_info->ov_reserved_0_union.reg.ov_order1 = DPU_OP_INVALID_ID;
}

static void hisi_overlayer_set_regs(struct hisi_comp_operator *operator, struct hisi_dm_ov_info *ov_info, char __iomem *dpu_base_addr)
{
	char __iomem *dm_base = dpu_base_addr + dpu_get_dm_offset(operator->scene_id);

	hisi_module_set_reg(&operator->module_desc, DPU_DM_OV_IMG_WIDTH_ADDR(dm_base), ov_info->ov_img_width_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_OV_BLAYER_ENDX_ADDR(dm_base), ov_info->ov_blayer_endx_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_OV_BLAYER_ENDY_ADDR(dm_base), ov_info->ov_blayer_endy_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_OV_BG_COLOR_RGB_ADDR(dm_base), ov_info->ov_bg_color_rgb_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_OV_ORDER0_ADDR(dm_base), ov_info->ov_order0_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_OV_RESERVED_0_ADDR(dm_base), ov_info->ov_reserved_0_union.value);
}

static int hisi_overlayer_set_cmd_item(struct hisi_comp_operator *operator, struct hisi_composer_device *composer_device,
			void *layer)
{
	struct hisi_dm_param *dm_param = NULL;
	struct pipeline_src_layer *src_layer = (struct pipeline_src_layer *)layer;
	char __iomem *dpu_base = composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU];

	disp_pr_debug(" layer:0x%p \n", layer);
	disp_pr_debug(" ++++ \n");
	disp_pr_debug(" src_layer:0x%p \n", src_layer);
	disp_pr_debug(" composer_device->ov_data.dm_param: 0x%p \n", composer_device->ov_data.dm_param);
	dm_param = composer_device->ov_data.dm_param;

	operator->out_data->rect.x = 0;
	operator->out_data->rect.y = 0;
	if (!(src_layer->need_caps & CAP_ARSR0_POST)) {
		if (!composer_device->ov_data.fix_panel_info) {
			operator->out_data->rect.w = DPU_WIDTH(src_layer->dst_rect.w);
			operator->out_data->rect.h = DPU_HEIGHT(src_layer->dst_rect.h);
		} else {
			operator->out_data->rect.w = composer_device->ov_data.fix_panel_info->xres - 1;
			operator->out_data->rect.h = composer_device->ov_data.fix_panel_info->yres - 1;
		}
	}
	if (src_layer->need_caps & CAP_MITM) {
		struct wcg_mitm_info mitm_param;
		disp_pr_debug(" MITM test \n");
		dpu_mitm_param_config(&mitm_param);
		dpu_set_dm_mitem_reg(&operator->module_desc, dpu_base, &mitm_param);
	}

	hisi_overlayer_set_dm_ovinfo(operator, &(composer_device->ov_data), &(dm_param->ov_info), src_layer);
	hisi_overlayer_set_regs(operator, &(dm_param->ov_info), dpu_base);

	composer_device->ov_data.layer_ov_format = operator->in_data->format;
	disp_pr_debug("composer_device->ov_data.layer_ov_format: 0x%x\n", composer_device->ov_data.layer_ov_format);
	disp_pr_debug(" ---- \n");
	return 0;
}

static int overlayer_build_data(struct hisi_comp_operator *operator, void *layer, struct hisi_pipeline_data *_pre_out_data,
	struct hisi_comp_operator *next_operator)
{
	struct pipeline_src_layer *src_layer;

	if (!layer) {
		disp_pr_err("[OV] layer is NULL pointer\n");
		return -1;
	}

	disp_pr_debug("[OV] ++++\n");
	src_layer = (struct pipeline_src_layer *)layer;
	operator->out_data->format = ARGB10101010;

	if (is_arsr1_arsr0_pipe())
		(void)memcpy_s(&operator->out_data->rect, sizeof(operator->out_data->rect),
			&src_layer->dst_rect, sizeof(src_layer->dst_rect));
	else
		(void)memcpy_s(&operator->out_data->rect, sizeof(operator->out_data->rect),
			&src_layer->src_rect, sizeof(src_layer->src_rect)); // sdma->ov->arsr

	dpu_operator_build_data(operator, layer, _pre_out_data, next_operator);

	return 0;
}

void hisi_overlayer_init(struct hisi_operator_type *type_operator, struct dpu_module_ops ops, void *cookie)
{
	struct hisi_operator_ov **ovs = NULL;
	struct hisi_comp_operator *base = NULL;
	uint32_t i;

	disp_pr_info(" ++++ \n");
	ovs = kzalloc(sizeof(*ovs) * type_operator->operator_count, GFP_KERNEL);
	if (!ovs)
		return;

	for (i = 0; i < type_operator->operator_count; i++) {
		ovs[i] = kzalloc(sizeof(*(ovs[i])), GFP_KERNEL);
		if (!ovs[i])
			continue;

		base = &ovs[i]->base;
		hisi_operator_init(base, ops, cookie);

		base->id_desc.info.idx = i;
		base->id_desc.info.type = type_operator->type;
		base->set_cmd_item = hisi_overlayer_set_cmd_item;
		base->build_data = overlayer_build_data;
		base->operator_offset = DPU_RCH_OV_OFFSET;

		disp_pr_info(" base->id_desc.info.idx：%u \n", base->id_desc.info.idx);
		disp_pr_info(" base->id_desc.info.type：%u \n", base->id_desc.info.type);

		base->out_data = kzalloc(sizeof(struct hisi_ov_out_data), GFP_KERNEL);
		if (!base->out_data) {
			kfree(ovs[i]);
			ovs[i] = NULL;
			continue;
		}

		base->in_data = kzalloc(sizeof(struct hisi_ov_in_data), GFP_KERNEL);
		if (!base->in_data) {
			kfree(base->in_data);
			base->in_data = NULL;

			kfree(ovs[i]);
			ovs[i] = NULL;
			continue;
		}

		/* TODO: init other ov operators */
		ovs[i]->base.be_dm_counted = false;
	}
	type_operator->operators = (struct hisi_comp_operator **)(ovs);
}
