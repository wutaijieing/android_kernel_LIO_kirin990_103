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

#include <linux/platform_device.h>
#include <linux/types.h>
#include <securec.h>
#include "hisi_operators_manager.h"
#include "hisi_drv_utils.h"
#include "hisi_disp_composer.h"
#include "hisi_disp.h"
#include "uvup/hisi_operator_uvup.h"
#include "hisi_disp_gadgets.h"
#include "wlt/disp_dpu_wlt.h"
#include "hisi_operator_tool.h"
#include "widget/dpu_csc.h"

static void hisi_uvup_post_set_dm_uvup_info(struct post_offline_info *offline_info,
	struct hisi_dm_param *dm_param)
{
	disp_pr_info(" ++++ \n");

	struct hisi_dm_uvup0_info *uvup0_info = &(dm_param->uvup0_info);
	struct hisi_dm_uvup1_info *uvup1_info = &(dm_param->uvup1_info);

	uvup0_info->uvup0_input_img_width_union.value = 0;
	uvup0_info->uvup0_input_img_width_union.reg.uvup0_input_img_height =
		DPU_HEIGHT(offline_info->wb_ov_block_rect.h);
	uvup0_info->uvup0_input_img_width_union.reg.uvup0_input_img_width =
		DPU_WIDTH(offline_info->wb_ov_block_rect.w);

	uvup0_info->uvup0_output_img_width_union.value = 0;
	uvup0_info->uvup0_output_img_width_union.reg.uvup0_output_img_height =
		DPU_HEIGHT(offline_info->wb_ov_block_rect.h);
	uvup0_info->uvup0_output_img_width_union.reg.uvup0_output_img_width =
		DPU_WIDTH(offline_info->wb_ov_block_rect.w);

	uvup0_info->uvup0_input_fmt_union.value = 0;
	uvup0_info->uvup0_input_fmt_union.reg.uvup0_layer_id = POST_LAYER_ID;
	uvup0_info->uvup0_input_fmt_union.reg.uvup0_order0 = DPU_WCH_OP_ID;

	if (g_debug_en_uvup1 == 0) {
		uvup0_info->uvup0_input_fmt_union.reg.uvup0_sel = 1;
	} else {
		uvup0_info->uvup0_input_fmt_union.reg.uvup0_sel = 2;
	}

	uvup0_info->uvup0_input_fmt_union.reg.uvup0_input_fmt =
		DPU_RDFC_OUT_FORMAT_ARGB_10101010;

	disp_pr_info("[DATA] uvup in w = %d, h = %d out w = %d, h = %d\n",
		uvup0_info->uvup0_input_img_width_union.reg.uvup0_input_img_width,
		uvup0_info->uvup0_input_img_width_union.reg.uvup0_input_img_height,
		uvup0_info->uvup0_output_img_width_union.reg.uvup0_output_img_height,
		uvup0_info->uvup0_output_img_width_union.reg.uvup0_output_img_height);

	uvup0_info->uvup0_threshold_union.value = 0;
	uvup0_info->uvup0_threshold_union.reg.uvup0_output_fmt =
		DPU_RDFC_OUT_FORMAT_AYUV_10101010; // YUV444;
	uvup0_info->uvup0_threshold_union.reg.uvup0_threshold = UVUP_THREHOLD_DEFAULT;

	uvup1_info->uvup1_input_fmt_union.value = 0;
	uvup1_info->uvup1_input_fmt_union.reg.uvup1_layer_id = INVALID_LAYER_ID;

	disp_pr_info(" --- \n");
}

static void hisi_uvup_post_set_uvsampinfo(struct post_offline_info *offline_info,
	struct dpu_uvsamp *uvsamp_info)
{
	disp_pr_info(" ++++ \n");

	uvsamp_info->uvsamp_size.value = 0;
	uvsamp_info->uvsamp_size.reg.uvsamp_width =
		DPU_WIDTH(offline_info->wb_ov_block_rect.w);
	uvsamp_info->uvsamp_size.reg.uvsamp_height =
		DPU_HEIGHT(offline_info->wb_ov_block_rect.h);
	disp_pr_info("[DATA] uvsamp in w = %d, h = %d\n",
		uvsamp_info->uvsamp_size.reg.uvsamp_width,
		uvsamp_info->uvsamp_size.reg.uvsamp_height);

	uvsamp_info->uvsamp_ctrl.value = 0;
	uvsamp_info->uvsamp_ctrl.reg.uvsamp_en = 1;
	uvsamp_info->uvsamp_ctrl.reg.uvsamp_mode = DOWN_SAMPLING;

	uvsamp_info->reg_ctrl_flush_en.value = 0;
	uvsamp_info->reg_ctrl_flush_en.reg.reg_ctrl_flush_en = 1;

	disp_pr_info(" config uvsamp coef\n");

	uvsamp_info->uvsamp_coef_0.value = 0;
	uvsamp_info->uvsamp_coef_1.value = 0;
	uvsamp_info->uvsamp_coef_1.reg.uvsamp_coeff_2 = 46;
	uvsamp_info->uvsamp_coef_1.reg.uvsamp_coeff_3 = 164;
	uvsamp_info->uvsamp_coef_2.value = 0;
	uvsamp_info->uvsamp_coef_2.reg.uvsamp_coeff_4 = 46;
	uvsamp_info->uvsamp_coef_3.value = 0;

	uvsamp_info->csc_icg_module.value = 0;
	uvsamp_info->csc_icg_module.reg.csc_module_en = 1;
}

static void hisi_uvup_post_set_uvsamp_regs(struct hisi_comp_operator *operator,
	struct dpu_uvsamp *uvsamp_info, char __iomem *dpu_base_addr)
{
	disp_pr_info(" ++++ \n");

	char __iomem *uvsamp_base = NULL;
	uint32_t uvup_offset = 0;

	uvup_offset = g_debug_en_uvup1 ? DSS_UVSAMP1_OFFSET : DSS_UVSAMP0_OFFSET;
	uvsamp_base = dpu_base_addr + uvup_offset;

	hisi_module_set_reg(&operator->module_desc,
		DPU_UVSAMP_REG_DEFAULT_ADDR(uvsamp_base), 0x1);
	hisi_module_set_reg(&operator->module_desc,
		DPU_UVSAMP_REG_DEFAULT_ADDR(uvsamp_base), 0x0);

	hisi_module_set_reg(&operator->module_desc, DPU_UVSAMP_SIZE_ADDR(uvsamp_base),
		uvsamp_info->uvsamp_size.value); // height & width
	hisi_module_set_reg(&operator->module_desc, DPU_UVSAMP_CTRL_ADDR(uvsamp_base),
		uvsamp_info->uvsamp_ctrl.value); // uvsamp enable & uvsamp_mode

	hisi_module_set_reg(&operator->module_desc, DPU_UVSAMP_COEF_0_ADDR(uvsamp_base),
		uvsamp_info->uvsamp_coef_0.value);
	hisi_module_set_reg(&operator->module_desc, DPU_UVSAMP_COEF_1_ADDR(uvsamp_base),
		uvsamp_info->uvsamp_coef_1.value);
	hisi_module_set_reg(&operator->module_desc, DPU_UVSAMP_COEF_2_ADDR(uvsamp_base),
		uvsamp_info->uvsamp_coef_2.value);
	hisi_module_set_reg(&operator->module_desc, DPU_UVSAMP_COEF_3_ADDR(uvsamp_base),
		uvsamp_info->uvsamp_coef_3.value);

	hisi_module_set_reg(&operator->module_desc,
		DPU_UVSAMP_CSC_ICG_MODULE_ADDR(uvsamp_base),
		uvsamp_info->csc_icg_module.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_UVSAMP_REG_CTRL_FLUSH_EN_ADDR(uvsamp_base),
		uvsamp_info->reg_ctrl_flush_en.value);
}

static void hisi_uvup_set_dm_uvup_regs(struct hisi_comp_operator *operator,
	struct hisi_dm_param *dm_param, char __iomem *dpu_base_addr)
{
	disp_pr_info(" ++++ , operator->scene_id %u\n", operator->scene_id);

	char __iomem *dm_base = dpu_base_addr + dpu_get_dm_offset(operator->scene_id);
	struct hisi_dm_uvup0_info *uvup0_info = &(dm_param->uvup0_info);
	struct hisi_dm_uvup1_info *uvup1_info = &(dm_param->uvup1_info);

	hisi_module_set_reg(&operator->module_desc,
		DPU_DM_UVUP0_INPUT_IMG_WIDTH_ADDR(dm_base),
		uvup0_info->uvup0_input_img_width_union.value);

	hisi_module_set_reg(&operator->module_desc,
		DPU_DM_UVUP0_OUTPUT_IMG_WIDTH_ADDR(dm_base),
		uvup0_info->uvup0_output_img_width_union.value);

	hisi_module_set_reg(&operator->module_desc,
		DPU_DM_UVUP0_INPUT_FMT_ADDR(dm_base),
		uvup0_info->uvup0_input_fmt_union.value);

	hisi_module_set_reg(&operator->module_desc,
		DPU_DM_UVUP0_THRESHOLD_ADDR(dm_base),
		uvup0_info->uvup0_threshold_union.value);

	hisi_module_set_reg(&operator->module_desc,
		DPU_DM_UVUP1_INPUT_FMT_ADDR(dm_base),
		uvup1_info->uvup1_input_fmt_union.value);

	disp_pr_info(" ---- \n");
}

static void hisi_uvup_pre_set_dm_uvup_info(struct hisi_comp_operator *operator,
	struct pipeline_src_layer *src_layer, struct hisi_dm_param *dm_param)
{
	disp_pr_info(" ++++ \n");

	struct hisi_dm_uvup0_info *uvup0_info = &(dm_param->uvup0_info);
	struct hisi_dm_uvup1_info *uvup1_info = &(dm_param->uvup1_info);

	uvup0_info->uvup0_input_img_width_union.value = 0;
	uvup0_info->uvup0_input_img_width_union.reg.uvup0_input_img_height =
		DPU_HEIGHT(operator->in_data->rect.h);
	uvup0_info->uvup0_input_img_width_union.reg.uvup0_input_img_width =
		DPU_WIDTH(operator->in_data->rect.w);

	uvup0_info->uvup0_output_img_width_union.value = 0;
	uvup0_info->uvup0_output_img_width_union.reg.uvup0_output_img_height =
		DPU_HEIGHT(operator->in_data->rect.h);
	uvup0_info->uvup0_output_img_width_union.reg.uvup0_output_img_width =
		DPU_WIDTH(operator->in_data->rect.w);

	uvup0_info->uvup0_input_fmt_union.value = 0;
	uvup0_info->uvup0_input_fmt_union.reg.uvup0_layer_id = src_layer->layer_id; // src_layer->layer_id;
	uvup0_info->uvup0_input_fmt_union.reg.uvup0_order0 =
		hisi_get_op_id_by_op_type(operator->in_data->next_order);
	disp_pr_info("next order %d", uvup0_info->uvup0_input_fmt_union.reg.uvup0_order0);

	if (g_debug_en_uvup1 == 0) {
		uvup0_info->uvup0_input_fmt_union.reg.uvup0_sel = 1; /* enable uvup0 */
	} else {
		uvup0_info->uvup0_input_fmt_union.reg.uvup0_sel = 2; /* enable uvup1 */
	}

	uvup0_info->uvup0_input_fmt_union.reg.uvup0_input_fmt = operator->in_data->format;

	uvup0_info->uvup0_threshold_union.value = 0;
	uvup0_info->uvup0_threshold_union.reg.uvup0_output_fmt =
		DPU_RDFC_OUT_FORMAT_AYUV_10101010;
	uvup0_info->uvup0_threshold_union.reg.uvup0_threshold = UVUP_THREHOLD_DEFAULT;

	uvup1_info->uvup1_input_fmt_union.value = 0;
	uvup1_info->uvup1_input_fmt_union.reg.uvup1_layer_id = INVALID_LAYER_ID; // src_layer->layer_id;

	disp_pr_info(" ---- \n");
}

static void hisi_uvup_pre_set_uvsampinfo(struct pipeline_src_layer *src_layer,
	struct dpu_uvsamp *uvsamp_info)
{
	disp_pr_info(" ++++ \n");

	uvsamp_info->csc_icg_module.value = 0;
	uvsamp_info->csc_icg_module.reg.csc_module_en = 0;

	uvsamp_info->uvsamp_size.value = 0;
	uvsamp_info->uvsamp_size.reg.uvsamp_width = src_layer->src_rect.w - 1;
	uvsamp_info->uvsamp_size.reg.uvsamp_height = src_layer->src_rect.h - 1;

	uvsamp_info->uvsamp_ctrl.value = 0;
	uvsamp_info->uvsamp_ctrl.reg.uvsamp_en = 1;
	uvsamp_info->uvsamp_ctrl.reg.uvsamp_mode = UP_SAMPLING;

	if (is_8bit(src_layer->img.format))
		uvsamp_info->uvsamp_ctrl.reg.uvsamp_bitext_mode = 1;

	uvsamp_info->uvsamp_coef_0.value = 0;
	uvsamp_info->uvsamp_coef_0.reg.uvsamp_coeff_0 = 0;
	uvsamp_info->uvsamp_coef_0.reg.uvsamp_coeff_1 = 0;
	uvsamp_info->uvsamp_coef_1.value = 0;
	uvsamp_info->uvsamp_coef_1.reg.uvsamp_coeff_2 = 256;
	uvsamp_info->uvsamp_coef_1.reg.uvsamp_coeff_3 = 0;
	uvsamp_info->uvsamp_coef_2.value = 0;
	uvsamp_info->uvsamp_coef_2.reg.uvsamp_coeff_4 = -48;
	uvsamp_info->uvsamp_coef_2.reg.uvsamp_coeff_5 = 176;
	uvsamp_info->uvsamp_coef_3.value = 0;
	uvsamp_info->uvsamp_coef_3.reg.uvsamp_coeff_6 = 176;
	uvsamp_info->uvsamp_coef_3.reg.uvsamp_coeff_7 = -48;

	uvsamp_info->reg_ctrl_flush_en.value = 0;
	uvsamp_info->reg_ctrl_flush_en.reg.reg_ctrl_flush_en = 1;

	// set dblk
	uvsamp_info->dblk_size.value = 0;
	uvsamp_info->dblk_size.reg.dblk_width = src_layer->src_rect.w - 1;
	uvsamp_info->dblk_size.reg.dblk_height = src_layer->src_rect.h - 1;

	uvsamp_info->dblk_ctrl.value = 0;
	uvsamp_info->dblk_ctrl.reg.dblk_unit_size = 1;
	if (is_8bit(src_layer->img.format))
		uvsamp_info->dblk_ctrl.reg.dblk_bitdepth = 0;
	else
		uvsamp_info->dblk_ctrl.reg.dblk_bitdepth = 1;

	int height =  src_layer->src_rect.h;
	int slice_num = src_layer->slice_num == 0 ? 4 : src_layer->slice_num;
	int slice_row = ALIGN_UP(height / slice_num, 64);

	uvsamp_info->dblk_ctrl.reg.dblk_slice_height = count_from_zero(slice_row);

	uvsamp_info->dblk_slice0_frame_info.value = 0;
	uvsamp_info->dblk_slice0_frame_info.reg.dblk_slice0_bs = 2;
	uvsamp_info->dblk_slice1_frame_info.value = 0;
	uvsamp_info->dblk_slice1_frame_info.reg.dblk_slice1_bs = 2;
	uvsamp_info->dblk_slice2_frame_info.value = 0;
	uvsamp_info->dblk_slice2_frame_info.reg.dblk_slice2_bs = 2;
	uvsamp_info->dblk_slice3_frame_info.value = 0;
	uvsamp_info->dblk_slice3_frame_info.reg.dblk_slice3_bs = 2;

	uvsamp_info->dblk_slice0_info.value = 0;
	uvsamp_info->dblk_slice0_info.reg.dblk_slice0_enable = 1;
	uvsamp_info->dblk_slice0_info.reg.dblk_slice0_qp = 47;

	disp_pr_debug("wlt rect h %u, slice_row %d\n",  height, slice_row);

	disp_pr_info(" dblk_width = %d dblk_height = %d dblk_slice_height = %d \n",
		uvsamp_info->dblk_size.reg.dblk_width, uvsamp_info->dblk_size.reg.dblk_height,
		uvsamp_info->dblk_ctrl.reg.dblk_slice_height);
}

static void hisi_uvup_pre_set_uvsamp_regs(struct hisi_comp_operator *operator,
	struct dpu_uvsamp *uvsamp_info, char __iomem *dpu_base_addr)
{
	disp_pr_info(" ++++ \n");

	char __iomem *uvsamp_base = NULL;
	if (g_debug_en_uvup1 == 0) {
		uvsamp_base = dpu_base_addr + DSS_UVSAMP0_OFFSET;
	} else {
		uvsamp_base = dpu_base_addr + DSS_UVSAMP1_OFFSET;
	}

	hisi_module_set_reg(&operator->module_desc,
		DPU_UVSAMP_REG_DEFAULT_ADDR(uvsamp_base), 0x1);
	hisi_module_set_reg(&operator->module_desc,
		DPU_UVSAMP_REG_DEFAULT_ADDR(uvsamp_base), 0x0);

	hisi_module_set_reg(&operator->module_desc,
		DPU_UVSAMP_SIZE_ADDR(uvsamp_base), uvsamp_info->uvsamp_size.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_UVSAMP_CTRL_ADDR(uvsamp_base), uvsamp_info->uvsamp_ctrl.value);

	disp_pr_info(" uvsamp uvsamp_coef_x config\n");
	hisi_module_set_reg(&operator->module_desc,
		DPU_UVSAMP_COEF_0_ADDR(uvsamp_base), uvsamp_info->uvsamp_coef_0.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_UVSAMP_COEF_1_ADDR(uvsamp_base), uvsamp_info->uvsamp_coef_1.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_UVSAMP_COEF_2_ADDR(uvsamp_base), uvsamp_info->uvsamp_coef_2.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_UVSAMP_COEF_3_ADDR(uvsamp_base), uvsamp_info->uvsamp_coef_3.value);

	disp_pr_info(" uvsamp dblk config\n");
	hisi_module_set_reg(&operator->module_desc,
		DPU_UVSAMP_DBLK_SIZE_ADDR(uvsamp_base), uvsamp_info->dblk_size.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_UVSAMP_DBLK_CTRL_ADDR(uvsamp_base), uvsamp_info->dblk_ctrl.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_UVSAMP_DBLK_SLICE0_FRAME_INFO_ADDR(uvsamp_base),
		uvsamp_info->dblk_slice0_frame_info.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_UVSAMP_DBLK_SLICE1_FRAME_INFO_ADDR(uvsamp_base),
		uvsamp_info->dblk_slice1_frame_info.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_UVSAMP_DBLK_SLICE2_FRAME_INFO_ADDR(uvsamp_base),
		uvsamp_info->dblk_slice2_frame_info.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_UVSAMP_DBLK_SLICE3_FRAME_INFO_ADDR(uvsamp_base),
		uvsamp_info->dblk_slice3_frame_info.value);

	hisi_module_set_reg(&operator->module_desc,
		DPU_UVSAMP_DBLK_SLICE0_INFO_ADDR(uvsamp_base),
		uvsamp_info->dblk_slice0_info.value);

	hisi_module_set_reg(&operator->module_desc,
		DPU_UVSAMP_REG_CTRL_FLUSH_EN_ADDR(uvsamp_base),
		uvsamp_info->reg_ctrl_flush_en.value);
}

static int hisi_uvup_pre_build_in_data(struct hisi_comp_operator *operator, void *layer,
	void *pre_out_data, struct hisi_comp_operator *next_operator)
{
	errno_t err_ret;
	struct pipeline_src_layer *src_layer;

	if (!layer) {
		disp_pr_err("[uvup pre] layer is NULL pointer\n");
		return -1;
	}
	disp_pr_info("[uvup pre] ++++\n");
	src_layer = (struct pipeline_src_layer *)layer;

	err_ret = memcpy_s(&operator->in_data->rect, sizeof(operator->in_data->rect),
		&src_layer->src_rect, sizeof(src_layer->src_rect));
	if (err_ret != EOK) {
		disp_pr_err("secure func call error\n");
		return -1;
	}

	err_ret = memcpy_s(&operator->out_data->rect, sizeof(operator->out_data->rect),
		&src_layer->src_rect, sizeof(src_layer->src_rect));
	if (err_ret != EOK) {
		disp_pr_err("secure func call error\n");
		return -1;
	}

	operator->in_data->format = dpu_pixel_format_hal2dma(src_layer->img.format,
		DPU_HAL2SDMA_PIXEL_FORMAT);
	operator->out_data->format = DPU_RDFC_OUT_FORMAT_AYUV_10101010;
	dpu_operator_build_data(operator, layer, pre_out_data, next_operator);

	return 0;
}

int hisi_uvup_pre_set_cmd_item(struct hisi_comp_operator *operator, struct hisi_composer_device *composer_device,
	void *layer)
{
	disp_pr_info(" ++++ \n");

	int ret;
	struct hisi_dm_param *dm_param = NULL;
	struct dpu_uvsamp uvsamp_info = {0};
	struct dpu_dacc_wlt dacc_wlt = {0};
	struct pipeline_src_layer *src_layer = (struct pipeline_src_layer *)layer;
	dm_param = composer_device->ov_data.dm_param;
	char __iomem *dpu_base = composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU];

	hisi_uvup_pre_set_dm_uvup_info(operator, src_layer, dm_param);

	hisi_uvup_pre_set_uvsampinfo(src_layer, &uvsamp_info);

	hisi_uvup_set_dm_uvup_regs(operator, dm_param, dpu_base);

	hisi_uvup_pre_set_uvsamp_regs(operator, &uvsamp_info, dpu_base);

	disp_pr_info(" ---- \n");

	return 0;
}

static int hisi_uvup_post_build_in_data(struct hisi_comp_operator *operator, void *layer,
	void *pre_out_data, struct hisi_comp_operator *next_operator)
{
	return 0;
}

int hisi_uvup_post_set_cmd_item(struct hisi_comp_operator *operator,
	struct hisi_composer_device *composer_device,
	void *layer)
{
	disp_pr_info(" ++++ \n");

	uint32_t operator_offset;
	struct hisi_dm_param *dm_param = NULL;
	struct dpu_uvsamp uvsamp_info = {0};
	struct post_offline_info *offline_info = (struct post_offline_info *)layer;
	dm_param = composer_device->ov_data.dm_param;
	char __iomem *dpu_base = composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU];

	hisi_uvup_post_set_dm_uvup_info(offline_info, dm_param);

	hisi_uvup_post_set_uvsampinfo(offline_info, &uvsamp_info);

	hisi_uvup_set_dm_uvup_regs(operator, dm_param, dpu_base);

	hisi_uvup_post_set_uvsamp_regs(operator, &uvsamp_info, dpu_base);

	if (g_debug_en_uvup1 == 0) {
		operator_offset = DSS_UVSAMP0_OFFSET;
	} else {
		operator_offset = DSS_UVSAMP1_OFFSET;
	}
	dpu_csc_set_coef(&operator->module_desc,
		GET_CSC_HEAD_BASE(DPU_UVSAMP_CSC_IDC0_ADDR(dpu_base + operator_offset)),
		CSC_RGB2YUV, CSC_709_WIDE);

	disp_pr_info(" ---- \n");

	return 0;
}

static int hisi_uvup_build_data(struct hisi_comp_operator *operator, void *layer,
	struct hisi_pipeline_data *pre_out_data, struct hisi_comp_operator *next_operator)
{
	if (operator->operator_pos == PIPELINE_TYPE_POST)
		return hisi_uvup_post_build_in_data(operator, layer, pre_out_data, next_operator);

	return hisi_uvup_pre_build_in_data(operator, layer, pre_out_data, next_operator);
}

int hisi_uvup_set_cmd_item(struct hisi_comp_operator *operator,
	struct hisi_composer_device *composer_device, void *layer)
{
	if (operator->operator_pos == PIPELINE_TYPE_POST)
		return hisi_uvup_post_set_cmd_item(operator, composer_device, layer);

	return hisi_uvup_pre_set_cmd_item(operator, composer_device, layer);
}

void hisi_uvup_init(struct hisi_operator_type *type_operator, struct dpu_module_ops ops, void *cookie)
{
	struct hisi_operator_uvup **uvups = NULL;
	struct hisi_comp_operator *base = NULL;
	uint32_t i;

	disp_pr_info(" ++++ \n");
	uvups = kzalloc(sizeof(*uvups) * type_operator->operator_count, GFP_KERNEL);
	if (!uvups)
		return;

	for (i = 0; i < type_operator->operator_count; i++) {
		uvups[i] = kzalloc(sizeof(*(uvups[i])), GFP_KERNEL);
		if (!uvups[i])
			continue;

		hisi_operator_init(&uvups[i]->base, ops, cookie);

		base = &uvups[i]->base;
		base->id_desc.info.idx = i;
		base->id_desc.info.type = type_operator->type;
		base->set_cmd_item = hisi_uvup_set_cmd_item;
		base->build_data = hisi_uvup_build_data;

		disp_pr_info(" base->id_desc.info.idx %u \n", base->id_desc.info.idx);
		disp_pr_info(" base->id_desc.info.type %u \n", base->id_desc.info.type);
		sema_init(&uvups[i]->base.operator_sem, 1);

		base->out_data = kzalloc(sizeof(struct hisi_uvup_out_data), GFP_KERNEL);
		if (!base->out_data) {
			kfree(uvups[i]);
			uvups[i] = NULL;
			continue;
		}

		base->in_data = kzalloc(sizeof(struct hisi_uvup_in_data), GFP_KERNEL);
		if (!base->in_data) {
			kfree(base->in_data);
			base->in_data = NULL;

			kfree(uvups[i]);
			uvups[i] = NULL;
			continue;
		}

		uvups[i]->base.be_dm_counted = false;
	}

	type_operator->operators = (struct hisi_comp_operator **)(uvups);
}
