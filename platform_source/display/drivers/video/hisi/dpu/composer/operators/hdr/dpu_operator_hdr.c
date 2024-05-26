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

#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/types.h>

#include "hisi_operators_manager.h"
#include "hisi_drv_utils.h"
#include "dpu_operator_hdr.h"
#include "hdr/dpu_hihdr.h"
#include "hdr/hdr_test.h"
#include "widget/dpu_csc.h"
#include "hisi_disp.h"
#include "hisi_dpu_module.h"
#include "hisi_operator_tool.h"

#define CSC_ROW 3
#define CSC_COL 5
#define CSC_NUM 5

unsigned hisi_fb_display_yuvmode = 0x0;
module_param_named(debug_display_yuvmode, hisi_fb_display_yuvmode, int, 0644);
MODULE_PARM_DESC(debug_display_yuvmode, "hisi_fb_display_yuvmode");

static int g_csc_coe_yuv2rgb2020[CSC_ROW][CSC_COL] = {
	{0x04A85, 0x00000, 0x06B6F, 0x007C0, 0x00000},
	{0x04A85, 0x1F402, 0x1D65F, 0x00600, 0x00000},
	{0x04A85, 0x08912, 0x00000, 0x00600, 0x00000},
};
static int g_csc_coe_yuv2rgb601_wide[CSC_ROW][CSC_COL] = {
	{0x4000, 0x00000, 0x059ba, 0x000, 0x000},
	{0x4000, 0x1e9fa, 0x1d24c, 0x600, 0x000},
	{0x4000, 0x07168, 0x00000, 0x600, 0x000},
};
static int g_csc_coe_yuv2rgb601_narrow[CSC_ROW][CSC_COL] = {
	{0x4a85, 0x00000, 0x06625, 0x7c0, 0x000},
	{0x4a85, 0x1e6ed, 0x1cbf8, 0x600, 0x000},
	{0x4a85, 0x0811a, 0x00000, 0x600, 0x000},
};

static int g_csc_coe_yuv2rgb709_wide[CSC_ROW][CSC_COL] = {
	{0x4000, 0x00000, 0x064ca, 0x000, 0x000},
	{0x4000, 0x1f403, 0x1e20a, 0x600, 0x000},
	{0x4000, 0x076c2, 0x00000, 0x600, 0x000},
};

static int g_csc_coe_yuv2rgb709_narrow[CSC_ROW][CSC_COL] = {
	{0x4a85, 0x00000, 0x072bc, 0x7c0, 0x000},
	{0x4a85, 0x1f25a, 0x1dde5, 0x600, 0x000},
	{0x4a85, 0x08732, 0x00000, 0x600, 0x000},
};

static void dpu_hdr_set_csc_info(struct hisi_comp_operator *operator,
	char __iomem *dpu_base_addr) {

	struct hihdr_csc_info csc_coef;
	char __iomem *hdr_base = dpu_base_addr + DSS_HDR_OFFSET;
	if (is_rgb(operator->in_data->format)) { // rgb in
		dpu_set_reg(DPU_HDR_CSC_CTRL_ADDR(hdr_base), 2, 2, 0);
		disp_pr_info(" hdr in format %d\n", operator->in_data->format);
		return;
	} else { // yuv in
		dpu_set_reg(DPU_HDR_CSC_CTRL_ADDR(hdr_base), 1, 2, 0);
		disp_pr_info(" hdr in format %d, yuv2rgb\n", operator->in_data->format);
	}

	/* FPGA TEST use 601 this mode maybe different */
	csc_coef.hdr_csc_idc0_union.value =
		(uint32_t)g_csc_coe_yuv2rgb601_wide[0][3] |
		((uint32_t)g_csc_coe_yuv2rgb601_wide[1][3] << 16);
	csc_coef.hdr_csc_idc2_union.value = g_csc_coe_yuv2rgb601_wide[2][3];
	csc_coef.hdr_csc_odc0_union.value =
		(uint32_t)g_csc_coe_yuv2rgb601_wide[0][4] |
		((uint32_t)g_csc_coe_yuv2rgb601_wide[1][4] << 16);
	csc_coef.hdr_csc_odc2_union.value = g_csc_coe_yuv2rgb601_wide[2][4];
	csc_coef.hdr_csc_p00_union.value = g_csc_coe_yuv2rgb601_wide[0][0];
	csc_coef.hdr_csc_p01_union.value = g_csc_coe_yuv2rgb601_wide[0][1];
	csc_coef.hdr_csc_p02_union.value = g_csc_coe_yuv2rgb601_wide[0][2];
	csc_coef.hdr_csc_p10_union.value = g_csc_coe_yuv2rgb601_wide[1][0];
	csc_coef.hdr_csc_p11_union.value = g_csc_coe_yuv2rgb601_wide[1][1];
	csc_coef.hdr_csc_p12_union.value = g_csc_coe_yuv2rgb601_wide[1][2];
	csc_coef.hdr_csc_p20_union.value = g_csc_coe_yuv2rgb601_wide[2][0];
	csc_coef.hdr_csc_p21_union.value = g_csc_coe_yuv2rgb601_wide[2][1];
	csc_coef.hdr_csc_p22_union.value = g_csc_coe_yuv2rgb601_wide[2][2];

	outp32(DPU_HDR_CSC_IDC0_ADDR(hdr_base), csc_coef.hdr_csc_idc0_union.value);
	outp32(DPU_HDR_CSC_IDC2_ADDR(hdr_base), csc_coef.hdr_csc_idc2_union.value);
	outp32(DPU_HDR_CSC_ODC0_ADDR(hdr_base), csc_coef.hdr_csc_odc0_union.value);
	outp32(DPU_HDR_CSC_ODC2_ADDR(hdr_base), csc_coef.hdr_csc_odc2_union.value);
	outp32(DPU_HDR_CSC_P00_ADDR(hdr_base), csc_coef.hdr_csc_p00_union.value);
	outp32(DPU_HDR_CSC_P01_ADDR(hdr_base), csc_coef.hdr_csc_p01_union.value);
	outp32(DPU_HDR_CSC_P02_ADDR(hdr_base), csc_coef.hdr_csc_p02_union.value);
	outp32(DPU_HDR_CSC_P10_ADDR(hdr_base), csc_coef.hdr_csc_p10_union.value);
	outp32(DPU_HDR_CSC_P11_ADDR(hdr_base), csc_coef.hdr_csc_p11_union.value);
	outp32(DPU_HDR_CSC_P12_ADDR(hdr_base), csc_coef.hdr_csc_p12_union.value);
	outp32(DPU_HDR_CSC_P20_ADDR(hdr_base), csc_coef.hdr_csc_p20_union.value);
	outp32(DPU_HDR_CSC_P21_ADDR(hdr_base), csc_coef.hdr_csc_p21_union.value);
	outp32(DPU_HDR_CSC_P22_ADDR(hdr_base), csc_coef.hdr_csc_p22_union.value);
}

static void dpu_hdr_set_dm_hdrinfo(struct hisi_comp_operator *operator,
	struct hisi_dm_hdr_info *hdr_info,
	struct pipeline_src_layer *layer) {
	disp_pr_info(
		" ++++layer format %d, in fomat %d h = %d, w = %d \n",
		layer->img.format, operator->in_data->format, operator->in_data->rect.h,
		operator->in_data->rect.w);

	hdr_info->hdr_input_img_width_union.reg.hdr_input_img_height =
	operator->in_data->rect.h - 1;
	hdr_info->hdr_input_img_width_union.reg.hdr_input_img_width =
	operator->in_data->rect.w - 1;

	hdr_info->hdr_output_img_width_union.reg.hdr_output_img_height =
	operator->in_data->rect.h - 1;
	hdr_info->hdr_output_img_width_union.reg.hdr_output_img_width =
	operator->in_data->rect.w - 1;

	hdr_info->hdr_input_fmt_union.reg.hdr_layer_id = layer->layer_id;
	hdr_info->hdr_input_fmt_union.reg.hdr_order0 = DPU_OV_OP_ID;
	hdr_info->hdr_input_fmt_union.reg.hdr_sel = 1;
	hdr_info->hdr_input_fmt_union.reg.hdr_input_fmt = operator->in_data->format;

	hdr_info->hdr_reservied.reg.hdr_output_fmt = XRGB10101010;
	disp_pr_info(" ++++hdr_output_fmt format %d, in fomat %d\n",
		hdr_info->hdr_reservied.reg.hdr_output_fmt,
		operator->in_data->format);
}

static void dpu_hdr_set_dfc_info(struct hisi_comp_operator *operator,
	struct pipeline_src_layer *src_layer,
	char __iomem *hdr_base) {
	disp_pr_info(" +++ \n");
	int formart;
	uint32_t temp;
	uint32_t val;

	disp_pr_info(" hdr_base:0x%p, rect.w:0x%x\n",
	hdr_base, operator->in_data->rect.w);

	/* hdr dfc info */
	val =
			((operator->in_data->rect.w - 1) << 16) | (operator->in_data->rect.h - 1);
	dpu_set_reg(DPU_HDR_DFC_DISP_SIZE_ADDR(hdr_base), val, 32, 0);

	formart = operator->in_data->format;
	disp_pr_info(" hdr formart:0x%x, src_layer formart 0x%x\n", formart,
	src_layer->img.format);

	dpu_set_reg(DPU_HDR_DFC_DISP_FMT_ADDR(hdr_base), formart, 5, 1);
	dpu_set_reg(DPU_HDR_DFC_ICG_MODULE_ADDR(hdr_base), 1, 1, 0);
	disp_pr_info(" --- \n");
}

static void dpu_hdr_set_regs(struct hisi_comp_operator *operator,
	struct pipeline_src_layer *src_layer,
	struct dpu_hdr_info *hdr_info,
	char __iomem *dpu_base_addr) {
	char __iomem *hdr_base = dpu_base_addr + DSS_HDR_OFFSET;

	if (!has_40bit(operator->in_data->format))
		dpu_hdr_set_dfc_info(operator, src_layer, hdr_base);

	dpu_hdr_set_csc_info(operator, dpu_base_addr);

	dpu_hihdr_gtm_set_reg(operator, &operator->module_desc, src_layer, hdr_base, hdr_info);

	dpu_hdr_degamma_set_lut(&operator->module_desc, src_layer, hdr_base,
	hdr_info); // TBD

	dpu_hdr_gmp_set_lut(&operator->module_desc, src_layer, hdr_base,
	hdr_info); // TBD

	dpu_hdr_gamma_set_lut(&operator->module_desc, src_layer, hdr_base,
	hdr_info); // TBD

	dpu_set_reg(DPU_HDR_REG_CTRL_DEBUG_ADDR(hdr_base), 1, 1, 10);

	dpu_set_reg(DPU_HDR_REG_CTRL_FLUSH_EN_ADDR(hdr_base), 1, 1, 0);
}

static void dpu_hdr_flush_regs(struct hisi_comp_operator *operator,
	struct hisi_dm_hdr_info *hdr_info,
	char __iomem *dpu_base_addr) {
	char __iomem *dm_base = dpu_base_addr + dpu_get_dm_offset(operator->scene_id);
	disp_pr_info(" HDR ++++ \n");
	dpu_set_reg(DPU_DM_HDR_INPUT_IMG_WIDTH_ADDR(dm_base),
		hdr_info->hdr_input_img_width_union.value, 32, 0);
	dpu_set_reg(DPU_DM_HDR_OUTPUT_IMG_WIDTH_ADDR(dm_base),
		hdr_info->hdr_output_img_width_union.value, 32, 0);
	dpu_set_reg(DPU_DM_HDR_INPUT_FMT_ADDR(dm_base),
		hdr_info->hdr_input_fmt_union.value, 32, 0);
	dpu_set_reg(DPU_DM_HDR_RESERVED_ADDR(dm_base),
		hdr_info->hdr_reservied.value, 32, 0);
}

static int dpu_hdr_set_cmd_item(struct hisi_comp_operator *operator,
	struct hisi_composer_device *composer_device,
	void *layer) {
	struct hisi_dm_param *dm_param = NULL;
	struct pipeline_src_layer *src_layer = (struct pipeline_src_layer *)layer;
	dm_param = composer_device->ov_data.dm_param;
	struct dpu_hdr_info hdr_info;

	dpu_hdr_set_dm_hdrinfo(operator, &(dm_param->hdr_info[0]), src_layer);
	dpu_hdr_gtm_param_config(&hdr_info);
	dpu_hdr_set_regs(operator,
	src_layer, &hdr_info,
	composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU]);
	dpu_hdr_flush_regs(operator, &(dm_param->hdr_info[0]),
	composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU]);

	composer_device->ov_data.layer_ov_format = XRGB10101010;

	return 0;
}

static int dpu_hdr_build_in_data(struct hisi_comp_operator *operator,
	void *layer, struct hisi_pipeline_data *pre_out_data,
	struct hisi_comp_operator *next_operator) {
	struct pipeline_src_layer *src_layer = (struct pipeline_src_layer *)layer;

	/* in data */
	if (next_operator) {
		disp_pr_info(" next_operator->id_desc.info.type:%u \n", next_operator->id_desc.info.type);
		operator->in_data->next_order = next_operator->id_desc.info.type;
	}

	if (pre_out_data) {
		operator->in_data->format = pre_out_data->format;
		operator->in_data->rect = pre_out_data->rect;
	} else {
		operator->in_data->format = dpu_pixel_format_hal2dfc(src_layer->img.format, DFC_STATIC);
		operator->in_data->rect = src_layer->src_rect;
	}

	/* out data */
	operator->out_data->rect = src_layer->dst_rect;
	operator->out_data->format = XRGB10101010;

	dpu_operator_build_data(operator, layer, pre_out_data, next_operator);
	return 0;
}

void dpu_hdr_init(struct hisi_operator_type *type_operator,
	struct dpu_module_ops ops, void *cookie) {
	struct dpu_operator_hdr **hdrs = NULL;
	struct hisi_comp_operator *base = NULL;
	uint32_t i;

	disp_pr_info(" ++++ , type_operator->operator_count %d",
	type_operator->operator_count);
	hdrs = kzalloc(sizeof(*hdrs) * type_operator->operator_count, GFP_KERNEL);
	if (!hdrs)
		return;

	for (i = 0; i < type_operator->operator_count; i++) {
		hdrs[i] = kzalloc(sizeof(*(hdrs[i])), GFP_KERNEL);
		if (!hdrs[i])
			continue;

		hisi_operator_init(&hdrs[i]->base, ops, cookie);

		base = &hdrs[i]->base;
		base->id_desc.info.idx = i;
		base->id_desc.info.type = type_operator->type;
		base->set_cmd_item = dpu_hdr_set_cmd_item;
		base->build_data = dpu_hdr_build_in_data;
		disp_pr_info(" base->id_desc.info.idx: 0x%u\n", base->id_desc.info.idx);
		disp_pr_info(" base->id_desc.info.type: 0x%u\n", base->id_desc.info.type);
		sema_init(&hdrs[i]->base.operator_sem, 1);

		base->out_data = kzalloc(sizeof(struct dpu_hdr_out_data), GFP_KERNEL);
		if (!base->out_data) {
			kfree(hdrs[i]);
			hdrs[i] = NULL;
			continue;
		}

		base->in_data = kzalloc(sizeof(struct dpu_hdr_in_data), GFP_KERNEL);
		if (!base->in_data) {
			kfree(base->out_data);
			base->out_data = NULL;

			kfree(hdrs[i]);
			hdrs[i] = NULL;
			continue;
		}

		hdrs[i]->base.be_dm_counted = false;
	}

	type_operator->operators = (struct hisi_comp_operator **)(hdrs);
}
