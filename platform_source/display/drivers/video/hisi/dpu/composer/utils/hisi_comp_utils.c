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
#include "hisi_composer_core.h"
#include "hisi_disp_composer.h"
#include "hisi_operators_manager.h"
#include "hisi_composer_operator.h"
#include "hisi_operator_tool.h"
#include "hisi_disp_smmu.h"
#include "fence/hisi_disp_fence.h"
#include "scl/dpu_scl.h"
#include "widget/dpu_dfc.h"

static void dpu_operator_count_numbers(struct hisi_comp_operator *operator, struct hisi_dm_scene_info *scene_info)
{
	if (operator->be_dm_counted)
		return;

	switch (operator->id_desc.info.type) {
	case COMP_OPS_HDR:
		scene_info->srot_number_union.reg.hdr_number++;
		break;
	case COMP_OPS_UVUP:
		scene_info->srot_number_union.reg.uvup_number++;
		break;
	case COMP_OPS_SROT:
		scene_info->srot_number_union.reg.srot_number++;
		break;
	case COMP_OPS_ARSR:
	case COMP_OPS_VSCF:
		scene_info->ddic_number_union.reg.scl_number++;
		break;
	case COMP_OPS_CLD:
		scene_info->ddic_number_union.reg.cld_number++;
		break;
	case COMP_OPS_DDIC:
		scene_info->ddic_number_union.reg.ddic_number++;
		break;
	case COMP_OPS_DPP:
		scene_info->ddic_number_union.reg.dpp_number++;
		break;
	case COMP_OPS_DSC:
		scene_info->layer_number_union.reg.dsc_number++;
		break;
	case COMP_OPS_WCH:
		scene_info->layer_number_union.reg.wch_number++;
		break;
	case COMP_OPS_ITF:
		scene_info->layer_number_union.reg.itf_number++;
		break;
	default:
		disp_pr_debug("unsupport operator type=%u", operator->id_desc.info.type);
		return;
	}

	//operator->be_dm_counted = true;
}

/* TODO: 如果overlay后面多条后级pipeline，需要重构这个函数 */
static uint32_t _get_post_pipeline_head_ops(struct hisi_display_frame *frame)
{
	struct pipeline *pipeline = NULL;

	if (frame->post_offline_count > 0)
		pipeline = &frame->post_offline_pipelines[0].pipeline;

	if (frame->post_online_count > 0)
		pipeline = &frame->post_online_pipelines[0].pipeline;

	if (pipeline && pipeline->operator_count > 0)
		return pipeline->operators[0].id;

	return build_operator_id(COMP_OPS_INVALID, 0);
}

static uint32_t _get_pre_pipeline_tail_ops(struct hisi_display_frame *frame)
{
	struct pipeline *pipeline = NULL;

	if (frame->pre_pipeline_count > 0)
		pipeline = &frame->pre_pipelines[0].pipeline;

	if (pipeline && pipeline->operator_count > 0)
		return pipeline->operators[pipeline->operator_count - 1].id;

	return build_operator_id(COMP_OPS_INVALID, 0);
}

static void dpu_comp_set_layer_ov_format(struct hisi_composer_device *composer_device, void *layer)
{
	struct hisi_dm_layer_info *dm_layer_info = NULL;
	struct hisi_composer_data *ov_data = &composer_device->ov_data;
	struct pipeline_src_layer *src_layer = (struct pipeline_src_layer *)layer;
	uint32_t scend_id = ov_data->scene_id;
	char __iomem *dpu_dm_base = ov_data->ip_base_addrs[DISP_IP_BASE_DPU] + dpu_get_dm_offset(scend_id);
	struct dpu_module_desc module_desc;
	module_desc.client = composer_device->client;

	dm_layer_info = &(ov_data->dm_param->layer_info[src_layer->layer_id]);

	dm_layer_info->layer_bot_crop_union.reg.layer_ov_fmt = ov_data->layer_ov_format;

	if (is_yuv(ov_data->layer_ov_format)) {
		dm_layer_info->layer_ov_pattern_a_union.reg.layer_ov_csc_en = 1;
		dm_layer_info->layer_ov_pattern_a_union.reg.layer_ov_csc_reg_sel = 4;
		disp_pr_debug("yuv in need csc\n");
	} else {
		dm_layer_info->layer_ov_pattern_a_union.reg.layer_ov_csc_en = 0;
		disp_pr_debug("rgb in no need csc\n");
	}

	disp_pr_debug("ov_data->layer_ov_format: %x\n", ov_data->layer_ov_format);

	if (has_40bit(ov_data->layer_ov_format))
		dm_layer_info->layer_ov_pattern_a_union.reg.layer_ov_dfc_en = 0;

	disp_pr_debug("layer_id: %d, layer_bot_crop_union.value: %x\n", src_layer->layer_id, dm_layer_info->layer_bot_crop_union.value);

	if (!is_d3_128(src_layer->img.format)) {
		hisi_module_set_reg(&module_desc, DPU_DM_LAYER_OV_PATTERN_A_ADDR(dpu_dm_base, src_layer->layer_id),
		dm_layer_info->layer_ov_pattern_a_union.value);
	} else {
		disp_pr_debug(" demura no need set this\n");
	}
	hisi_module_set_reg(&module_desc, DPU_DM_LAYER_BOT_CROP_ADDR(dpu_dm_base, src_layer->layer_id),
		dm_layer_info->layer_bot_crop_union.value);
}

static int hisi_comp_traverse_pipeline(struct hisi_composer_device *composer_device, struct pipeline *pipeline,
			void *src_layer, enum PIPELINE_TYPE pipeline_type, uint32_t head_ops_id, uint32_t tail_ops_id)
{
	struct hisi_comp_operator *operator = NULL;
	struct hisi_comp_operator *next_operator = NULL;
	struct hisi_pipeline_data *pre_operator_data = NULL;
	struct hisi_comp_operator *tail_operator = NULL;
	union operator_id id;
	uint32_t i;
	int ret;

	id.id = tail_ops_id;
	tail_operator = hisi_disp_get_operator(id);

	id.id = head_ops_id;
	operator = hisi_disp_get_operator(id);
	if (operator)
		pre_operator_data = operator->out_data;

	disp_pr_debug("src_layer:0x%p, operator_count:%u", src_layer, pipeline->operator_count);
	for (i = 0; i < pipeline->operator_count; i++) {
		operator = hisi_disp_get_operator(pipeline->operators[i]);
		if (!operator)
			continue;

		if (!operator->set_cmd_item)
			continue;

		operator->operator_pos = pipeline_type;
		operator->scene_id = composer_device->ov_data.scene_id;
		operator->module_desc.client = composer_device->client;

		if (operator->build_data) {
			if ((i + 1) < pipeline->operator_count)
				next_operator = hisi_disp_get_operator(pipeline->operators[i + 1]);
			else
				next_operator = tail_operator;

			operator->build_data(operator, src_layer, pre_operator_data, next_operator);
		}

		dpu_operator_count_numbers(operator, &composer_device->ov_data.dm_param->scene_info);
		ret = operator->set_cmd_item(operator, composer_device, src_layer);
		if (ret)
			return -1;

		if ((pipeline_type == PIPELINE_TYPE_PRE) && (operator->id_desc.info.type != COMP_OPS_DEMURA)) // SDMA->DEMA no ov
			dpu_comp_set_layer_ov_format(composer_device, src_layer);

		pre_operator_data = operator->out_data;
		disp_pr_debug("operator->module_desc.list_node:0x%p ", &operator->module_desc.list_node);
	}

	disp_pr_debug(" ---- ");
	return 0;
}

int hisi_comp_traverse_src_layer(struct hisi_composer_device *composer_device, struct hisi_display_frame *frame)
{
	struct pipeline_src_layer *src_layer = NULL;
	struct pipeline *layer_pipeline = NULL;
	struct hisi_dm_param *dm_param = NULL;
	uint32_t i;
	int ret;

	disp_pr_debug(" ++++ ");

	dm_param = composer_device->ov_data.dm_param;
	dm_param->scene_info.layer_number_union.value = 0;
	dm_param->scene_info.layer_number_union.reg.layer_number = frame->pre_pipeline_count;

	disp_pr_debug(" pre_pipeline_count:%u ", frame->pre_pipeline_count);
	for (i = 0; i < frame->pre_pipeline_count; i++) {
		src_layer = &frame->pre_pipelines[i].src_layer;

		layer_pipeline = &frame->pre_pipelines[i].pipeline;

		ret = hisi_comp_traverse_pipeline(composer_device, layer_pipeline, src_layer, PIPELINE_TYPE_PRE,
				build_operator_id(COMP_OPS_INVALID, 0), _get_post_pipeline_head_ops(frame));
		if (ret)
			return -1;
	}

	disp_pr_debug(" ---- ");
	return 0;
}

int hisi_comp_traverse_online_post(struct hisi_composer_device *composer_device, struct hisi_display_frame *frame)
{
	struct pipeline *post_pipeline = NULL;
	struct post_online_info *post_info = NULL;
	union operator_id pre_tail_ops;
	int ret;
	uint32_t i;

	disp_pr_debug(" ++++ ");

	pre_tail_ops.id = _get_pre_pipeline_tail_ops(frame);

	for (i = 0; i < frame->post_online_count; i++) {
		post_pipeline = &frame->post_online_pipelines[i].pipeline;
		post_info  = &frame->post_online_pipelines[i].online_info;

		disp_pr_debug("post_info.dirty_rect[0x%x 0x%x 0x%x 0x%x]\n", post_info->dirty_rect.x, post_info->dirty_rect.y,
			post_info->dirty_rect.w, post_info->dirty_rect.h);

		post_info->dirty_rect.x = 0;
		post_info->dirty_rect.y = 0;
		post_info->dirty_rect.w = composer_device->ov_data.fix_panel_info->xres;
		post_info->dirty_rect.h = composer_device->ov_data.fix_panel_info->yres;

		disp_pr_debug("post_info.dirty_rect[0x%x 0x%x 0x%x 0x%x]\n", post_info->dirty_rect.x, post_info->dirty_rect.y,
			post_info->dirty_rect.w, post_info->dirty_rect.h);
		ret = hisi_comp_traverse_pipeline(composer_device, post_pipeline, post_info, PIPELINE_TYPE_POST,
				pre_tail_ops.id, build_operator_id(COMP_OPS_INVALID, 0));
		if (ret)
			return -1;

		// post_info->release_fence = hisi_disp_create_fence(&composer_device->timeline, DISP_RELEASE_FENCE);
		// post_info->release_fence = hisi_disp_create_fence(&composer_device->timeline, DISP_RETIRE_FENCE);
		disp_pr_debug("release_fence:%d, retire_fence:%d", post_info->release_fence, post_info->retire_fence);
	}

	disp_pr_debug(" ---- ");

	return 0;
}

void dpu_comp_set_dm_section_info(struct hisi_composer_device *composer_device)
{
	struct hisi_dm_scene_info *scene_info = NULL;
	struct hisi_composer_data *ov_data = &composer_device->ov_data;
	uint32_t scene_id = ov_data->scene_id;
	char __iomem *dpu_dm_base = ov_data->ip_base_addrs[DISP_IP_BASE_DPU] + dpu_get_dm_offset(scene_id);
	struct dpu_module_desc module_desc;
	struct dpu_module_ops *module_ops = hisi_module_get_context_ops();

	hisi_module_init(&module_desc, *module_ops, NULL);

	scene_info = &ov_data->dm_param->scene_info;
	scene_info->srot_number_union.reg.scene_id = scene_id;
	disp_pr_info(" layer_number_union.value:%u ", scene_info->layer_number_union.value);
	hisi_module_set_reg(&module_desc, DPU_DM_SROT_NUMBER_ADDR(dpu_dm_base), scene_info->srot_number_union.value);
	hisi_module_set_reg(&module_desc, DPU_DM_DDIC_NUMBER_ADDR(dpu_dm_base), scene_info->ddic_number_union.value);
	hisi_module_set_reg(&module_desc, DPU_DM_LAYER_NUMBER_ADDR(dpu_dm_base), scene_info->layer_number_union.value);
	hisi_module_set_reg(&module_desc, DPU_DM_SCENE_RESERVED_ADDR(dpu_dm_base), scene_info->scene_reserved_union.value);
#ifndef DEBUG_SMMU_BYPASS
	hisi_dss_smmu_ch_set_reg(ov_data->ip_base_addrs[DISP_IP_BASE_DPU], scene_id, scene_info->layer_number_union.reg.layer_number);
#endif
	dpu_scl_free_dm_node(); // TODO: free scl at scl file
#ifndef DEBUG_SMMU_BYPASS
	hisi_dss_smmu_tlb_flush(ov_data->scene_id);
#endif
}

int hisi_comp_traverse_offline_post(struct hisi_composer_device *composer_device, struct hisi_display_frame *frame)
{
	struct pipeline *post_pipeline = NULL;
	struct post_offline_info *post_info = NULL;
	union operator_id post_head_ops;
	int ret;
	uint32_t i;

	disp_pr_info(" ++++ ");

	post_head_ops.id = _get_post_pipeline_head_ops(frame);

	for (i = 0; i < frame->post_offline_count; i++) {
		post_pipeline = &frame->post_offline_pipelines[i].pipeline;
		post_info  = &frame->post_offline_pipelines[i].offline_info;

		ret = hisi_comp_traverse_pipeline(composer_device, post_pipeline, post_info, PIPELINE_TYPE_POST,
				post_head_ops.id, build_operator_id(COMP_OPS_INVALID, 0));
		if (ret)
			return -1;
	}

	disp_pr_info(" ---- ");
	return 0;
}

static int hisi_drv_composer_turn_on(uint32_t ov_id, struct platform_device *pdev)
{
	struct hisi_composer_device *overlay_dev = NULL;
	struct hisi_composer_priv_ops *overlay_priv_ops = NULL;
	struct hisi_pipeline_ops *next_dev_ops = NULL;
	struct hisi_composer_connector_device *next_connector_dev = NULL;
	int ret;
	uint32_t i;

	disp_pr_info(" enter ++++");

	overlay_dev = platform_get_drvdata(pdev);
	overlay_priv_ops = &overlay_dev->ov_priv_ops;

	/* 1, call overlay core function for different platform */
	dpu_check_and_call_func(ret, overlay_priv_ops->prepare_on, ov_id, pdev);
	dpu_check_and_call_func(ret, overlay_priv_ops->overlay_on, ov_id, pdev);
	dpu_check_and_call_func(ret, overlay_priv_ops->post_ov_on, ov_id, pdev);

	/* 2, call next device function */
	disp_pr_info(" next_dev_count:%u ", overlay_dev->next_dev_count);
	for (i = 0; i < overlay_dev->next_dev_count; i++) {
		next_connector_dev = overlay_dev->next_device[i];
		next_dev_ops = next_connector_dev->next_dev_ops;

		dpu_check_and_call_func(ret, next_dev_ops->turn_on, next_connector_dev->next_id, next_connector_dev->next_pdev);
	}

	return 0;
}

static int hisi_drv_composer_turn_off(uint32_t ov_id, struct platform_device *pdev)
{
	struct hisi_composer_device *overlay_dev = NULL;
	struct hisi_composer_priv_ops *overlay_priv_ops = NULL;
	struct hisi_pipeline_ops *next_dev_ops = NULL;
	struct hisi_composer_connector_device *next_connector_dev = NULL;
	int ret;
	uint32_t i;

	overlay_dev = platform_get_drvdata(pdev);
	overlay_priv_ops = &overlay_dev->ov_priv_ops;

	/* 1, call overlay core function for different platform */
	dpu_check_and_call_func(ret, overlay_priv_ops->prepare_off, ov_id, pdev);

	/* 2, call next device function */
	for (i = 0; i < overlay_dev->next_dev_count; i++) {
		next_connector_dev = overlay_dev->next_device[i];
		next_dev_ops = next_connector_dev->next_dev_ops;

		dpu_check_and_call_func(ret, next_dev_ops->turn_off, next_connector_dev->next_id, next_connector_dev->next_pdev);
	}

	dpu_check_and_call_func(ret, overlay_priv_ops->overlay_off, ov_id, pdev);
	dpu_check_and_call_func(ret, overlay_priv_ops->post_ov_off, ov_id, pdev);

	return 0;
}

static int hisi_drv_composer_present(uint32_t ov_id, struct platform_device *pdev, void *data)
{
	struct hisi_composer_device *overlay_dev = NULL;
	struct hisi_composer_priv_ops *overlay_priv_ops = NULL;
	struct hisi_pipeline_ops *next_dev_ops = NULL;
	struct hisi_composer_connector_device *next_connector_dev = NULL;
	int ret = 0;
	uint32_t i;
	disp_pr_debug(" ++++ ");
	overlay_dev = platform_get_drvdata(pdev);
	overlay_priv_ops = &overlay_dev->ov_priv_ops;

	/* 1, call overlay core function for different platform */
	if (overlay_priv_ops->overlay_prepare) {
		disp_pr_debug(" overlay_prepare ");
		ret = overlay_priv_ops->overlay_prepare(overlay_dev, data);
		if (ret) {
			return ret;
		}
	}

	/* 2, call overlay core function for different platform */
	if (overlay_priv_ops->overlay_present) {
		disp_pr_debug(" overlay_present ");
		ret = overlay_priv_ops->overlay_present(overlay_dev, data);
	}

	if (overlay_priv_ops->overlay_post) {
		disp_pr_debug(" overlay_post ");
		ret = overlay_priv_ops->overlay_post(overlay_dev, data);
	}
	disp_pr_debug(" ret:%d ", ret);
	if (ret)
		return -1;

	/* 3, call next device function */
	for (i = 0; i < overlay_dev->next_dev_count; i++) {
		next_connector_dev = overlay_dev->next_device[i];
		next_dev_ops = next_connector_dev->next_dev_ops;

		dpu_check_and_call_func(ret, next_dev_ops->present,
			next_connector_dev->next_id, next_connector_dev->next_pdev, data);
	}
	disp_pr_debug(" ---- ");
	return 0;
}

static struct hisi_composer_ops g_hisi_composer_ops = {
	.base = {
		.turn_on = hisi_drv_composer_turn_on,
		.turn_off = hisi_drv_composer_turn_off,
		.present = hisi_drv_composer_present,
	}
};

struct hisi_pipeline_ops *hisi_composer_get_public_ops(void)
{
	return (struct hisi_pipeline_ops *)(&g_hisi_composer_ops);
}

