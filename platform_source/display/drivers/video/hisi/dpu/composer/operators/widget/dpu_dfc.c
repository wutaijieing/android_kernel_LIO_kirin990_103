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

#include "dpu_dfc.h"
#include "hisi_operator_tool.h"

struct dpu_to_dfc_pixel_fmt {
	uint32_t dpu_pixel_format;
	uint32_t dfc_pixel_format;
};

int dpu_dfc_rb_swap(int format)
{
	switch (format) {
	case HISI_FB_PIXEL_FORMAT_BGRA_1010102:
		return 1;

	default:
		return 0;
	}
}

int dpu_dfc_uv_swap(int format)
{
	switch (format) {
	case HISI_FB_PIXEL_FORMAT_YCRCB_422_SP:
	case HISI_FB_PIXEL_FORMAT_YCRCB_420_SP:
	case HISI_FB_PIXEL_FORMAT_YCRCB_422_P:
	case HISI_FB_PIXEL_FORMAT_YCRCB_420_P:
	case HISI_FB_PIXEL_FORMAT_YCRCB420_SP_10BIT:
	case HISI_FB_PIXEL_FORMAT_YCRCB422_SP_10BIT:
		return 1;

	default:
		return 0;
	}
}

int dpu_dfc_ax_swap(int format)
{
	switch (format) {
	case HISI_FB_PIXEL_FORMAT_RGBA_8888:
	case HISI_FB_PIXEL_FORMAT_BGRA_8888:
	case HISI_FB_PIXEL_FORMAT_RGBX_8888:
	case HISI_FB_PIXEL_FORMAT_BGRX_8888:
	case HISI_FB_PIXEL_FORMAT_RGBA_4444:
	case HISI_FB_PIXEL_FORMAT_BGRA_4444:
	case HISI_FB_PIXEL_FORMAT_RGBA_5551:
	case HISI_FB_PIXEL_FORMAT_BGRA_5551:
	case HISI_FB_PIXEL_FORMAT_RGBX_4444:
	case HISI_FB_PIXEL_FORMAT_BGRX_4444:
	case HISI_FB_PIXEL_FORMAT_RGBX_5551:
	case HISI_FB_PIXEL_FORMAT_BGRX_5551:
	case HISI_FB_PIXEL_FORMAT_BGRA_1010102:
		return 1;

	default:
		return 0;
	}
}

static struct dpu_to_dfc_pixel_fmt dpu_to_dfc_fmt_static[] = {
	{DPU_SDMA_PIXEL_FORMAT_RGB_565, DPU_DFC_STATIC_PIXEL_FORMAT_RGB_565},
	{DPU_SDMA_PIXEL_FORMAT_XBGR_4444, DPU_DFC_STATIC_PIXEL_FORMAT_XBGR_4444},
	{DPU_SDMA_PIXEL_FORMAT_ABGR_4444, DPU_DFC_STATIC_PIXEL_FORMAT_ABGR_4444},
	{DPU_SDMA_PIXEL_FORMAT_XBGR_1555, DPU_DFC_STATIC_PIXEL_FORMAT_XBGR_1555},
	{DPU_SDMA_PIXEL_FORMAT_ABGR_1555, DPU_DFC_STATIC_PIXEL_FORMAT_ABGR_1555},
	{DPU_SDMA_PIXEL_FORMAT_XBGR_8888, DPU_DFC_STATIC_PIXEL_FORMAT_XBGR_8888},
	{DPU_SDMA_PIXEL_FORMAT_ABGR_8888, DPU_DFC_STATIC_PIXEL_FORMAT_ABGR_8888},
	{DPU_SDMA_PIXEL_FORMAT_RGBA_1010102, DPU_DFC_STATIC_PIXEL_FORMAT_RGBA_1010102},
	{DPU_SDMA_PIXEL_FORMAT_BGR_565, DPU_DFC_STATIC_PIXEL_FORMAT_BGR_565},
	{DPU_SDMA_PIXEL_FORMAT_XRGB_4444, DPU_DFC_STATIC_PIXEL_FORMAT_XRGB_4444},
	{DPU_SDMA_PIXEL_FORMAT_ARGB_4444, DPU_DFC_STATIC_PIXEL_FORMAT_ARGB_4444},
	{DPU_SDMA_PIXEL_FORMAT_XRGB_1555, DPU_DFC_STATIC_PIXEL_FORMAT_XRGB_1555},
	{DPU_SDMA_PIXEL_FORMAT_ARGB_1555, DPU_DFC_STATIC_PIXEL_FORMAT_ARGB_1555},
	{DPU_SDMA_PIXEL_FORMAT_XRGB_8888, DPU_DFC_STATIC_PIXEL_FORMAT_XRGB_8888},
	{DPU_SDMA_PIXEL_FORMAT_ARGB_8888, DPU_DFC_STATIC_PIXEL_FORMAT_ARGB_8888},

	{DPU_SDMA_PIXEL_FORMAT_YVYU_422_8BIT_PKG, DPU_DFC_STATIC_PIXEL_FORMAT_YVYU422},
	{DPU_SDMA_PIXEL_FORMAT_YUYV_422_8BIT_PKG, DPU_DFC_STATIC_PIXEL_FORMAT_YUYV422},
	{DPU_SDMA_PIXEL_FORMAT_YUYV_422_8BIT_SP, DPU_DFC_STATIC_PIXEL_FORMAT_YUYV422},
	{DPU_SDMA_PIXEL_FORMAT_YUYV_422_8BIT_P, DPU_DFC_STATIC_PIXEL_FORMAT_YUYV422},
	{DPU_SDMA_PIXEL_FORMAT_YUYV_420_8BIT_P, DPU_DFC_STATIC_PIXEL_FORMAT_YUYV420_SP_8BIT},
	{DPU_SDMA_PIXEL_FORMAT_YUYV_420_8BIT_SP, DPU_DFC_STATIC_PIXEL_FORMAT_YUYV420_SP_8BIT},
	{DPU_SDMA_PIXEL_FORMAT_YUYV_422_10BIT_PKG, DPU_DFC_STATIC_PIXEL_FORMAT_YUYV_10},
	{DPU_SDMA_PIXEL_FORMAT_YUYV_422_10BIT_SP, DPU_DFC_STATIC_PIXEL_FORMAT_YUYV_10},
	{DPU_SDMA_PIXEL_FORMAT_YUYV_422_10BIT_P, DPU_DFC_STATIC_PIXEL_FORMAT_YUYV_10},
	{DPU_SDMA_PIXEL_FORMAT_YUYV_420_10BIT_P, DPU_DFC_STATIC_PIXEL_FORMAT_YUYV420_SP_10BIT},
	{DPU_SDMA_PIXEL_FORMAT_YUYV_420_10BIT_SP, DPU_DFC_STATIC_PIXEL_FORMAT_YUYV420_SP_10BIT},
	{DPU_SDMA_PIXEL_FORMAT_YUV444, DPU_DFC_STATIC_PIXEL_FORMAT_YUV444},
	{DPU_SDMA_PIXEL_FORMAT_YVU444, DPU_DFC_STATIC_PIXEL_FORMAT_YVU444},
	{DPU_SDMA_PIXEL_FORMAT_YUVA_1010102, DPU_DFC_STATIC_PIXEL_FORMAT_YUVA_1010102},
	{DPU_SDMA_PIXEL_FORMAT_UYVA_1010102, DPU_DFC_STATIC_PIXEL_FORMAT_UYVA_1010102},
	{DPU_SDMA_PIXEL_FORMAT_VUYA_1010102, DPU_DFC_STATIC_PIXEL_FORMAT_VUYA_1010102},
};

static struct dpu_to_dfc_pixel_fmt dpu_to_dfc_fmt_dynamic[] = {
	{DPU_SDMA_PIXEL_FORMAT_RGB_565, DPU_DFC_DYNAMIC_PIXEL_FORMAT_RGB_565},
	{DPU_SDMA_PIXEL_FORMAT_XBGR_4444, DPU_DFC_DYNAMIC_PIXEL_FORMAT_XBGR_4444},
	{DPU_SDMA_PIXEL_FORMAT_ABGR_4444, DPU_DFC_DYNAMIC_PIXEL_FORMAT_ABGR_4444},
	{DPU_SDMA_PIXEL_FORMAT_XBGR_1555, DPU_DFC_DYNAMIC_PIXEL_FORMAT_XBGR_1555},
	{DPU_SDMA_PIXEL_FORMAT_ABGR_1555, DPU_DFC_DYNAMIC_PIXEL_FORMAT_ABGR_1555},
	{DPU_SDMA_PIXEL_FORMAT_XBGR_8888, DPU_DFC_DYNAMIC_PIXEL_FORMAT_XBGR_8888},
	{DPU_SDMA_PIXEL_FORMAT_ABGR_8888, DPU_DFC_DYNAMIC_PIXEL_FORMAT_ABGR_8888},
	{DPU_SDMA_PIXEL_FORMAT_RGBA_1010102, DPU_DFC_DYNAMIC_PIXEL_FORMAT_RGBA_1010102},
	{DPU_SDMA_PIXEL_FORMAT_BGR_565, DPU_DFC_DYNAMIC_PIXEL_FORMAT_BGR_565},
	{DPU_SDMA_PIXEL_FORMAT_XRGB_4444, DPU_DFC_DYNAMIC_PIXEL_FORMAT_XRGB_4444},
	{DPU_SDMA_PIXEL_FORMAT_ARGB_4444, DPU_DFC_DYNAMIC_PIXEL_FORMAT_ARGB_4444},
	{DPU_SDMA_PIXEL_FORMAT_XRGB_1555, DPU_DFC_DYNAMIC_PIXEL_FORMAT_XRGB_1555},
	{DPU_SDMA_PIXEL_FORMAT_ARGB_1555, DPU_DFC_DYNAMIC_PIXEL_FORMAT_ARGB_1555},
	{DPU_SDMA_PIXEL_FORMAT_XRGB_8888, DPU_DFC_DYNAMIC_PIXEL_FORMAT_XRGB_8888},
	{DPU_SDMA_PIXEL_FORMAT_ARGB_8888, DPU_DFC_DYNAMIC_PIXEL_FORMAT_ARGB_8888},

	{DPU_SDMA_PIXEL_FORMAT_YVYU_422_8BIT_PKG, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YVYU422},
	{DPU_SDMA_PIXEL_FORMAT_YUYV_422_8BIT_PKG, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUYV422},
	{DPU_SDMA_PIXEL_FORMAT_YUYV_422_8BIT_SP, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUYV422},
	{DPU_SDMA_PIXEL_FORMAT_YUYV_422_8BIT_P, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUYV422},
	{DPU_SDMA_PIXEL_FORMAT_YUYV_420_8BIT_P, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUV420_P_8BIT},
	{DPU_SDMA_PIXEL_FORMAT_YUYV_420_8BIT_SP, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUV420_SP_8BIT},
	{DPU_SDMA_PIXEL_FORMAT_YUYV_422_10BIT_PKG, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUYV_10},
	{DPU_SDMA_PIXEL_FORMAT_YUYV_422_10BIT_SP, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUYV_10},
	{DPU_SDMA_PIXEL_FORMAT_YUYV_422_10BIT_P, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUYV_10},
	{DPU_SDMA_PIXEL_FORMAT_YUYV_420_10BIT_P, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUV420_P_10BIT},
	{DPU_SDMA_PIXEL_FORMAT_YUYV_420_10BIT_SP, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUV420_SP_10BIT},
	{DPU_SDMA_PIXEL_FORMAT_YUV444, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUV444},
	{DPU_SDMA_PIXEL_FORMAT_YVU444, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YVU444},
	{DPU_SDMA_PIXEL_FORMAT_YUVA_1010102, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUVA_1010102},
	{DPU_SDMA_PIXEL_FORMAT_UYVA_1010102, DPU_DFC_DYNAMIC_PIXEL_FORMAT_UYVA_1010102},
	{DPU_SDMA_PIXEL_FORMAT_VUYA_1010102, DPU_DFC_DYNAMIC_PIXEL_FORMAT_VUYA_1010102},
};

int dpu_pixel_format_dpu2dfc(int format, int dfc_type)
{
	int i;
	int size;
	struct dpu_to_dfc_pixel_fmt *p_dpu_fmt = NULL;

	if (dfc_type == DFC_STATIC) {
		size = ARRAY_SIZE(dpu_to_dfc_fmt_static);
		p_dpu_fmt = dpu_to_dfc_fmt_static;
	} else if (dfc_type == DFC_DYNAMIC) {
		size = ARRAY_SIZE(dpu_to_dfc_fmt_dynamic);
		p_dpu_fmt = dpu_to_dfc_fmt_dynamic;
	} else {
		disp_pr_err("DFC type(%d) error!\n", dfc_type);
		return -1;
	}

	for (i = 0; i < size; i++) {
		if (p_dpu_fmt[i].dpu_pixel_format == format)
			return p_dpu_fmt[i].dfc_pixel_format;
	}

	disp_pr_err("dpu input format(%d) not support!\n", format);
	return -1;
}

/* WCH, ARSR, VSCF, HDR support DFC */

void dpu_dfc_set_cmd_item(struct dpu_module_desc *module, struct dpu_dfc_data *input)
{
	DPU_WCH_DFC_DISP_SIZE_UNION disp_size = INIT_UNION_ZERO;
	DPU_WCH_DFC_DISP_FMT_UNION disp_format = INIT_UNION_ZERO;
	DPU_ARSR_DFC_CLIP_CTL_HRZ_UNION clip_hrz = INIT_UNION_ZERO;
	DPU_ARSR_DFC_CLIP_CTL_VRZ_UNION clip_vrz = INIT_UNION_ZERO;
	DPU_ARSR_DFC_PADDING_CTL_UNION padding = INIT_UNION_ZERO;
	DPU_WCH_DFC_GLB_ALPHA23_UNION alpha23 = INIT_UNION_ZERO;
	uint32_t aligned_width;

	uint8_t bpp = dpu_get_bpp(input->format);
	uint8_t pixel_num = dpu_get_pixel_num(bpp);
	char __iomem *dfc_base = input->dfc_base;

	// disp_assert_if_cond(!DPU_IS_MULTIPLE_WITH(input->src_rect.w, pixel_num));
	aligned_width = ALIGN_UP(input->src_rect.w, pixel_num);
	input->clip_rect.right = aligned_width - input->src_rect.w;
	input->src_rect.w = aligned_width;
	disp_pr_dfc_input(input);

	disp_size.reg.dfc_size_hrz = DPU_WIDTH(input->src_rect.w);
	disp_size.reg.dfc_size_vrt = DPU_HEIGHT(input->src_rect.h);
	hisi_module_set_reg(module, DPU_WCH_DFC_DISP_SIZE_ADDR(dfc_base), disp_size.value);
	hisi_module_set_reg(module, DPU_WCH_DFC_PIX_IN_NUM_ADDR(dfc_base), (uint32_t)(pixel_num != 2));

	// TODO: if format is ARGB1555, need config this reg
	hisi_module_set_reg(module, DPU_WCH_DFC_GLB_ALPHA01_ADDR(dfc_base), 0);

	// TODO: for 10bit swap
	disp_format.value = 0;
	disp_format.reg.dfc_img_fmt = dpu_pixel_format_dpu2dfc(input->format, DFC_STATIC);
	hisi_module_set_reg(module, DPU_WCH_DFC_DISP_FMT_ADDR(dfc_base), disp_format.value);

	clip_hrz.reg.dfc_right_clip = input->clip_rect.right;
	clip_hrz.reg.dfc_left_clip = input->clip_rect.left;
	hisi_module_set_reg(module, DPU_WCH_DFC_CLIP_CTL_HRZ_ADDR(dfc_base), clip_hrz.value);

	clip_vrz.reg.dfc_bot_clip = input->clip_rect.bottom;
	clip_vrz.reg.dfc_top_clip = input->clip_rect.top;
	hisi_module_set_reg(module, DPU_WCH_DFC_CLIP_CTL_VRZ_ADDR(dfc_base), clip_vrz.value);
	hisi_module_set_reg(module, DPU_WCH_DFC_CTL_CLIP_EN_ADDR(dfc_base), 1); // enable clip

	/* TODO: dfc icg need confirm */
	hisi_module_set_reg(module, DPU_WCH_DFC_ICG_MODULE_ADDR(dfc_base), input->enable_icg_submodule);
	hisi_module_set_reg(module, DPU_WCH_DFC_DITHER_ENABLE_ADDR(dfc_base), input->enable_dither);

	padding.reg.dfc_left_pad = input->padding_rect.left;
	padding.reg.dfc_right_pad = input->padding_rect.right;
	padding.reg.dfc_top_pad = input->padding_rect.top;
	padding.reg.dfc_bot_pad = input->padding_rect.bottom;
	padding.reg.dfc_ctl_pad_enable = (padding.value == 0) ? 0 : 1;
	hisi_module_set_reg(module, DPU_WCH_DFC_PADDING_CTL_ADDR(dfc_base), padding.value);

	/* TODO: confirm glb alpha23 */
	alpha23.reg.dfc_glb_alpha3 = input->glb_alpha;
	alpha23.reg.dfc_glb_alpha2 = input->glb_alpha;
	hisi_module_set_reg(module, DPU_WCH_DFC_GLB_ALPHA23_ADDR(dfc_base), alpha23.value);

	input->dst_rect.w = input->src_rect.w - input->clip_rect.left - input->clip_rect.right +
		input->padding_rect.left + input->padding_rect.right;

	input->dst_rect.h = input->src_rect.h - input->clip_rect.top - input->clip_rect.bottom +
		input->padding_rect.top + input->padding_rect.bottom;
}
