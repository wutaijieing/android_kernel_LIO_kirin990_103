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

#include "hisi_drv_utils.h"
#include "hisi_disp.h"
#include "hisi_disp_composer.h"
#include "dm/hisi_dm.h"
#include "dpu_offline_define.h"
#include "hisi_operator_tool.h"
#include "block/dpu_block_algorithm.h"
#include "dpu_operator_wch.h"
#include "hisi_disp_smmu.h"
#include "wch/dpu_wch_wlt.h"

static int dpu_wch_config_dm_wchinfo(struct post_offline_info *in_data, struct hisi_dm_wch_info *wch_info)
{
	disp_pr_info("enter++++");
	disp_pr_info("[DATA] wch input size w = %d h = %d x = %d y = %d", in_data->wb_ov_block_rect.w,
		in_data->wb_ov_block_rect.h, in_data->wb_ov_block_rect.x, in_data->wb_ov_block_rect.y);
	wch_info->wch_input_img_width_union.value = 0;
	wch_info->wch_input_img_width_union.reg.wch_input_img_width = DPU_WIDTH(in_data->wb_ov_block_rect.w);
	wch_info->wch_input_img_width_union.reg.wch_input_img_height = DPU_HEIGHT(in_data->wb_ov_block_rect.h);

	wch_info->wch_input_img_stry_union.value = 0;
	wch_info->wch_input_img_stry_union.reg.wch_input_img_strx = 0;
	wch_info->wch_input_img_stry_union.reg.wch_input_img_stry = 0;

	wch_info->wch_reserved_0_union.value = 0;
	wch_info->wch_reserved_0_union.reg.wch_layer_id = POST_LAYER_ID;
	wch_info->wch_reserved_0_union.reg.wch_sel = BIT(in_data->wb_layer.wchn_idx);
	if (g_debug_wch_yuv_in) {
		wch_info->wch_reserved_0_union.reg.wch_input_fmt = DPU_RDFC_OUT_FORMAT_AYUV_10101010;
	} else {
		wch_info->wch_reserved_0_union.reg.wch_input_fmt = DPU_RDFC_OUT_FORMAT_ARGB_10101010;
	}

	disp_pr_info("wch sel %d format %d", wch_info->wch_reserved_0_union.reg.wch_sel,
		wch_info->wch_reserved_0_union.reg.wch_input_fmt);
	disp_pr_info("need_cap 0x%x, wch_input_fmt %u", in_data->wb_layer.need_caps,
		wch_info->wch_reserved_0_union.reg.wch_input_fmt);
	disp_pr_info("enter----");
	return 0;
}

static void dpu_wch_set_dm_regs(struct hisi_comp_operator *operator,
	struct hisi_dm_wch_info *wch_info, char __iomem *dpu_base_addr)
{
	disp_pr_info("enter++++");
	char __iomem *dm_base = dpu_base_addr + dpu_get_dm_offset(operator->scene_id);

	hisi_module_set_reg(&operator->module_desc, DPU_DM_WCH_INPUT_IMG_WIDTH_ADDR(dm_base),
		wch_info->wch_input_img_width_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_WCH_INPUT_IMG_STRY_ADDR(dm_base),
		wch_info->wch_input_img_stry_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_WCH_RESERVED_0_ADDR(dm_base),
		wch_info->wch_reserved_0_union.value);
	disp_pr_info("enter----");
}

static void dpu_wch_flush_en(struct hisi_comp_operator *operator, char __iomem *wch_base)
{
	disp_check_and_no_retval((wch_base == NULL), err, "wrap_ctl_base is NULL!\n");
	disp_pr_info("enter++++ wrap_ctl_base = 0x%x", wch_base);

	hisi_module_set_reg(&operator->module_desc, DPU_WCH_REG_CTRL_FLUSH_EN_ADDR(wch_base), 0x1);
	dpu_set_reg(DPU_WCH_REG_CTRL_DEBUG_ADDR(wch_base), 0x400, 32, 0);
}

static int dpu_wch_config_inner_operators(
	struct post_offline_info *offline_info,
	struct hisi_composer_data *ov_data,
	struct disp_wb_layer *wb_layer,
	uint32_t input_format)
{
	int ret = 0;
	struct disp_rect aligned_rect;

	memset(&aligned_rect, 0, sizeof(aligned_rect));

	disp_check_and_return(!offline_info, -EINVAL, err, "offline_info is NULL Point!\n");
	disp_check_and_return(!wb_layer, -EINVAL, err, "wb_layer is NULL Point!\n");

	disp_pr_info("enter+++");

	ret = dpu_wch_csc_config(ov_data, wb_layer, input_format); /* pcsc need set yuv_in, csc need set rgb in */
	disp_check_and_return((ret != 0), ret, err, "hisi_csc_config failed! ret = %d\n", ret);

	ret = dpu_wb_scf_config(offline_info, ov_data, wb_layer);
	disp_check_and_return((ret != 0), ret, err, "hisi_wb_scl_config failed! ret = %d\n", ret);

	ret = dpu_wb_post_clip_config(offline_info, ov_data, wb_layer);
	disp_check_and_return((ret != 0), ret, err, "hisi_wb_post_clip_config failed! ret = %d\n", ret);

	ret = dpu_wdfc_dither_config(offline_info, ov_data, wb_layer, HISI_FB_PIXEL_FORMAT_RGBA_8888);
	disp_check_and_return((ret != 0), ret, err, "hisi_wb_dither_config failed! ret = %d\n", ret);

	ret = dpu_wdfc_config(offline_info, ov_data, wb_layer, &aligned_rect);
	disp_check_and_return((ret != 0), ret, err, "hisi_wch_wdfc_config failed, ret = %d\n", ret);

	ret = dpu_wdma_config(offline_info, ov_data, wb_layer, aligned_rect);
	disp_check_and_return((ret != 0), ret, err, "hisi_wch_wdma_config failed, ret = %d\n", ret);

	ret = dpu_wch_wlt_config(offline_info, ov_data, wb_layer, aligned_rect);
	disp_check_and_return((ret != 0), ret, err, "hisi_wch_wlt_config failed, ret = %d\n", ret);

	disp_pr_info("exit---");
	return 0;
}

static int dpu_wch_set_inner_operators(struct hisi_comp_operator *operator, struct hisi_composer_data *ov_data,
	int32_t chn_index)
{
	struct dpu_offline_module *offline_module = NULL;

	offline_module = &(ov_data->offline_module);

	disp_pr_info("offline_module addr = 0x%p chn_idx = %d", offline_module, chn_index);

	if (offline_module->dma_used[chn_index] == 1)
		dpu_wdma_set_reg(operator, offline_module->wch_base[chn_index], &(offline_module->wdma[chn_index]));

	if (offline_module->dfc_used[chn_index] == 1)
		dpu_wdfc_set_reg(operator, offline_module->wch_base[chn_index], &(offline_module->dfc[chn_index]));

	if (offline_module->wlt_used[chn_index] == 1)
		dpu_wch_wlt_set_reg(operator, offline_module->wch_base[chn_index], &(offline_module->wlt[chn_index]));

	if (offline_module->wdfc_dither_used[chn_index] == 1)
		dpu_wdfc_dither_set_reg(operator, offline_module->wch_base[chn_index], &(offline_module->dither[chn_index]));

	if (offline_module->pcsc_used[chn_index] == 1)
		dpu_wch_pcsc_set_reg(operator, offline_module->wch_base[chn_index], &(offline_module->pcsc[chn_index]));

	if (offline_module->post_clip_used[chn_index] == 1)
		dpu_wb_post_clip_set_reg(operator, offline_module->wch_base[chn_index], &(offline_module->clip[chn_index]));

	if (offline_module->scf_used[chn_index] == 1 && offline_module->has_scf[chn_index])
		dpu_wb_scf_set_reg(operator, offline_module->wch_base[chn_index], &(offline_module->scf[chn_index]));

	if (offline_module->csc_used[chn_index] == 1)
		dpu_wch_csc_set_reg(operator, offline_module->wch_base[chn_index], &(offline_module->csc[chn_index]));

	dpu_wch_flush_en(operator, offline_module->wch_base[chn_index]);

	if (ov_data->ov_block_infos[0].layer_infos[0].src_type != LAYER_SRC_TYPE_LOCAL) {
		dpu_set_reg(DPU_WCH_REG_CTRL_DEBUG_ADDR(offline_module->wch_base[chn_index]), 1,1,10);
		disp_pr_info("wch continuos frame\n");
	}

	return 0;
}

static int dpu_wch_compose(struct hisi_comp_operator *operator, struct post_offline_info *offline_info,
	struct hisi_composer_data *ov_data, uint32_t input_format)
{
	uint32_t i;
	int ret;
	struct disp_wb_layer *wb_layer = NULL;
	struct ov_req_infos *pov_req_h_v = &(ov_data->ov_req);

	disp_pr_info("enter+++");

	for (i = 0; i < pov_req_h_v->wb_layer_nums; i++) {
		wb_layer = &(pov_req_h_v->wb_layers[i]);

		ret = dpu_wch_config_inner_operators(offline_info, ov_data, wb_layer, input_format);
		if (ret != 0) {
			disp_pr_err("write back config failed! ret=%d\n", ret);
			return ret;
		}

		dpu_wch_set_inner_operators(operator, ov_data, wb_layer->wchn_idx);
	}

#ifndef DEBUG_SMMU_BYPASS
	hisi_dss_smmu_wch_set_reg(ov_data->ip_base_addrs[DISP_IP_BASE_DPU], ov_data->scene_id);
	hisi_dss_smmu_tlb_flush(ov_data->scene_id);
#endif
	disp_pr_info("enter---");
	return 0;
}

int dpu_wch_set_cmd_item(struct hisi_comp_operator *operator, struct hisi_composer_device *ov_dev,
	void *layer)
{
	int ret;
	int32_t layer_id;
	struct hisi_dm_param *dm_param = NULL;
	struct hisi_composer_data *ov_data = NULL;
	struct post_offline_info *src = NULL;

	if (!operator) {
		disp_pr_err("operator is null ptr");
		return -1;
	}

	if (!ov_dev) {
		disp_pr_err("ov_dev is null ptr");
		return -1;
	}

	if (!layer) {
		disp_pr_err("layer is null ptr");
		return -1;
	}

	ov_data = &ov_dev->ov_data;
	src = (struct post_offline_info *)layer;
	layer_id = src->wb_layer.layer_id;
	dm_param = ov_data->dm_param;

	disp_pr_info("wb_layer id = %d", layer_id);
	ret = dpu_wch_config_dm_wchinfo(src, &(dm_param->wch_info[layer_id]));
	if (ret)
		return -1;

	dpu_wch_set_dm_regs(operator, &(dm_param->wch_info[layer_id]), ov_data->ip_base_addrs[DISP_IP_BASE_DPU]);

	ret = dpu_wch_compose(operator, src, ov_data, dm_param->wch_info[layer_id].wch_reserved_0_union.reg.wch_input_fmt);
	if (ret)
		return -1;

	return 0;
}

static int dpu_wch_build_data(struct hisi_comp_operator *operator, void *layer, struct hisi_pipeline_data *pre_out_data,
	struct hisi_comp_operator *next_operator)
{
	return 0;
}

void dpu_wch_init(struct hisi_operator_type *type_operator, struct dpu_module_ops ops, void *cookie)
{
	struct dpu_operator_wch **wchs = NULL;
	struct hisi_comp_operator *base = NULL;
	uint32_t i;

	disp_pr_info(" ++++ \n");
	wchs = kzalloc(sizeof(*wchs) * type_operator->operator_count, GFP_KERNEL);
	if (!wchs)
		return;

	for (i = 0; i < type_operator->operator_count; i++) {
		wchs[i] = kzalloc(sizeof(*(wchs[i])), GFP_KERNEL);
		if (!wchs[i])
			continue;

		base = &wchs[i]->base;
		hisi_operator_init(base, ops, cookie);

		base->id_desc.info.idx = i;
		base->id_desc.info.type = type_operator->type;
		base->set_cmd_item = dpu_wch_set_cmd_item;
		base->build_data = dpu_wch_build_data;

		disp_pr_info(" base->id_desc.info.seq %u \n", base->id_desc.info.idx);
		disp_pr_info(" base->id_desc.info.type %u \n", base->id_desc.info.type);

		base->out_data = kzalloc(sizeof(struct dpu_wch_out_data), GFP_KERNEL);
		if (!base->out_data) {
			kfree(wchs[i]);
			wchs[i] = NULL;
			continue;
		}

		base->in_data = kzalloc(sizeof(struct dpu_wch_in_data), GFP_KERNEL);
		if (!base->in_data) {
			kfree(base->in_data);
			base->in_data = NULL;

			kfree(wchs[i]);
			wchs[i] = NULL;
			continue;
		}

		/* TODO: init other ov operators */
		wchs[i]->base.be_dm_counted = false;
	}
	type_operator->operators = (struct hisi_comp_operator **)(wchs);
}
