/* Copyright (c) 2013-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#include "../dpu_overlay_utils.h"
#include "../../dpu_mmbuf_manager.h"

/*
 * DSS CSC
 */
#define CSC_ROW 3
#define CSC_COL 5

/*
 * [ p00 p01 p02 idc2 odc2 ]
 * [ p10 p11 p12 idc1 odc1 ]
 * [ p20 p21 p22 idc0 odc0 ]
 */
static int g_csc_coe_yuv2rgb601_wide[CSC_ROW][CSC_COL] = {
	{0x4000, 0x00000, 0x059ba, 0x000, 0x000},
	{0x4000, 0x1e9fa, 0x1d24c, 0x600, 0x000},
	{0x4000, 0x07168, 0x00000, 0x600, 0x000},
};

static int g_csc_coe_rgb2yuv601_wide[CSC_ROW][CSC_COL] = {
	{0x01323, 0x02591, 0x0074c, 0x000, 0x000},
	{0x1f533, 0x1eacd, 0x02000, 0x000, 0x200},
	{0x02000, 0x1e534, 0x1facc, 0x000, 0x200},
};

static int g_csc_coe_yuv2rgb601_narrow[CSC_ROW][CSC_COL] = {
	{0x4a85, 0x00000, 0x06625, 0x7c0, 0x000},
	{0x4a85, 0x1e6ed, 0x1cbf8, 0x600, 0x000},
	{0x4a85, 0x0811a, 0x00000, 0x600, 0x000},
};

static int g_csc_coe_rgb2yuv601_narrow[CSC_ROW][CSC_COL] = {
	{0x0106f, 0x02044, 0x00644, 0x000, 0x040},
	{0x1f684, 0x1ed60, 0x01c1c, 0x000, 0x200},
	{0x01c1c, 0x1e876, 0x1fb6e, 0x000, 0x200},
};

static int g_csc_coe_yuv2rgb709_wide[CSC_ROW][CSC_COL] = {
	{0x4000, 0x00000, 0x064ca, 0x000, 0x000},
	{0x4000, 0x1f403, 0x1e20a, 0x600, 0x000},
	{0x4000, 0x076c2, 0x00000, 0x600, 0x000},
};

static int g_csc_coe_rgb2yuv709_wide[CSC_ROW][CSC_COL] = {
	{0x00d9b, 0x02dc6, 0x0049f, 0x000, 0x000},
	{0x1f8ab, 0x1e755, 0x02000, 0x000, 0x200},
	{0x02000, 0x1e2ef, 0x1fd11, 0x000, 0x200},
};

static int g_csc_coe_yuv2rgb709_narrow[CSC_ROW][CSC_COL] = {
	{0x4a85, 0x00000, 0x072bc, 0x7c0, 0x000},
	{0x4a85, 0x1f25a, 0x1dde5, 0x600, 0x000},
	{0x4a85, 0x08732, 0x00000, 0x600, 0x000},
};

static int g_csc_coe_rgb2yuv709_narrow[CSC_ROW][CSC_COL] = {
	{0x00baf, 0x02750, 0x003f8, 0x000, 0x040},
	{0x1f98f, 0x1ea55, 0x01c1c, 0x000, 0x200},
	{0x01c1c, 0x1e678, 0x1fd6c, 0x000, 0x200},
};

static int g_csc_coe_rgb2yuv2020[CSC_ROW][CSC_COL] = {
	{0x04000, 0x00000, 0x00000, 0x00000, 0x00000},
	{0x00000, 0x04000, 0x00000, 0x00600, 0x00000},
	{0x00000, 0x00000, 0x04000, 0x00600, 0x00000},
};

static int g_csc_coe_yuv2rgb2020[CSC_ROW][CSC_COL] = {
	{0x04A85, 0x00000, 0x06B6F, 0x007C0, 0x00000},
	{0x04A85, 0x1F402, 0x1D65F, 0x00600, 0x00000},
	{0x04A85, 0x08912, 0x00000, 0x00600, 0x00000},
};

void dpu_csc_init(const char __iomem *csc_base, dss_csc_t *s_csc)
{
	if (!csc_base) {
		DPU_FB_ERR("csc_base is NULL\n");
		return;
	}
	if (!s_csc) {
		DPU_FB_ERR("s_csc is NULL\n");
		return;
	}

	memset(s_csc, 0, sizeof(dss_csc_t));

	s_csc->idc0 = inp32(csc_base + CSC_IDC0);
	s_csc->idc2 = inp32(csc_base + CSC_IDC2);
	s_csc->odc0 = inp32(csc_base + CSC_ODC0);
	s_csc->odc2 = inp32(csc_base + CSC_ODC2);

	s_csc->p00 = inp32(csc_base + CSC_P00);
	s_csc->p01 = inp32(csc_base + CSC_P01);
	s_csc->p02 = inp32(csc_base + CSC_P02);
	s_csc->p10 = inp32(csc_base + CSC_P10);
	s_csc->p11 = inp32(csc_base + CSC_P11);
	s_csc->p12 = inp32(csc_base + CSC_P12);
	s_csc->p20 = inp32(csc_base + CSC_P20);
	s_csc->p21 = inp32(csc_base + CSC_P21);
	s_csc->p22 = inp32(csc_base + CSC_P22);
	s_csc->icg_module = inp32(csc_base + CSC_ICG_MODULE);
}

void dpu_csc_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *csc_base, dss_csc_t *s_csc)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd is null\n");
		return;
	}

	if (!csc_base) {
		DPU_FB_ERR("csc_base is null\n");
		return;
	}

	if (!s_csc) {
		DPU_FB_ERR("s_csc is null\n");
		return;
	}

	dpufd->set_reg(dpufd, csc_base + CSC_IDC0, s_csc->idc0, 32, 0);
	dpufd->set_reg(dpufd, csc_base + CSC_IDC2, s_csc->idc2, 32, 0);
	dpufd->set_reg(dpufd, csc_base + CSC_ODC0, s_csc->odc0, 32, 0);
	dpufd->set_reg(dpufd, csc_base + CSC_ODC2, s_csc->odc2, 32, 0);

	dpufd->set_reg(dpufd, csc_base + CSC_P00, s_csc->p00, 32, 0);
	dpufd->set_reg(dpufd, csc_base + CSC_P01, s_csc->p01, 32, 0);
	dpufd->set_reg(dpufd, csc_base + CSC_P02, s_csc->p02, 32, 0);
	dpufd->set_reg(dpufd, csc_base + CSC_P10, s_csc->p10, 32, 0);
	dpufd->set_reg(dpufd, csc_base + CSC_P11, s_csc->p11, 32, 0);
	dpufd->set_reg(dpufd, csc_base + CSC_P12, s_csc->p12, 32, 0);
	dpufd->set_reg(dpufd, csc_base + CSC_P20, s_csc->p20, 32, 0);
	dpufd->set_reg(dpufd, csc_base + CSC_P21, s_csc->p21, 32, 0);
	dpufd->set_reg(dpufd, csc_base + CSC_P22, s_csc->p22, 32, 0);
	dpufd->set_reg(dpufd, csc_base + CSC_ICG_MODULE, s_csc->icg_module, 32, 0);
}

static bool is_pcsc_needed(const dss_layer_t *layer)
{
	if (layer->chn_idx != DSS_RCHN_V0)
		return false;

	if (layer->need_cap & CAP_2D_SHARPNESS)
		return true;

	/* horizental shrink is not supported by arsr2p */
	if ((layer->dst_rect.h != layer->src_rect.h) || (layer->dst_rect.w > layer->src_rect.w))
		return true;

	return false;
}

static void csc_get_regular_coef(uint32_t csc_mode, struct dss_csc_coe *csc_coe)
{
	switch (csc_mode) {
	case DSS_CSC_601_WIDE:
		csc_coe->csc_coe_yuv2rgb = g_csc_coe_yuv2rgb601_wide;
		csc_coe->csc_coe_rgb2yuv = g_csc_coe_rgb2yuv601_wide;
		break;
	case DSS_CSC_601_NARROW:
		csc_coe->csc_coe_yuv2rgb = g_csc_coe_yuv2rgb601_narrow;
		csc_coe->csc_coe_rgb2yuv = g_csc_coe_rgb2yuv601_narrow;
		break;
	case DSS_CSC_709_WIDE:
		csc_coe->csc_coe_yuv2rgb = g_csc_coe_yuv2rgb709_wide;
		csc_coe->csc_coe_rgb2yuv = g_csc_coe_rgb2yuv709_wide;
		break;
	case DSS_CSC_709_NARROW:
		csc_coe->csc_coe_yuv2rgb = g_csc_coe_yuv2rgb709_narrow;
		csc_coe->csc_coe_rgb2yuv = g_csc_coe_rgb2yuv709_narrow;
		break;
	case DSS_CSC_2020:
		csc_coe->csc_coe_yuv2rgb = g_csc_coe_yuv2rgb2020;
		csc_coe->csc_coe_rgb2yuv = g_csc_coe_rgb2yuv2020;
		break;
	default:
		csc_coe->csc_coe_yuv2rgb = g_csc_coe_yuv2rgb601_narrow;
		csc_coe->csc_coe_rgb2yuv = g_csc_coe_rgb2yuv601_narrow;
		break;
	}
}

static void dpu_csc_coef_array_config(dss_csc_t *csc, int **p, int j)
{
	csc->icg_module = set_bits32(csc->icg_module, 0x1, 1, 0);
	/* The follow code get one by one array value to calculate csc coef value
	 * It contains lots of fixed numbers
	 */
	csc->idc0 = set_bits32(csc->idc0, (uint32_t)(*((int *)p + 2 * j + 3)) |
		((uint32_t)(*((int *)p + 1 * j + 3)) << 16), 27, 0);
	csc->idc2 = set_bits32(csc->idc2, *((int *)p + 0 * j + 3), 11, 0);

	csc->odc0 = set_bits32(csc->odc0, (uint32_t)(*((int *)p + 2 * j + 4)) |
		((uint32_t)(*((int *)p + 1 * j + 4)) << 16), 27, 0);
	csc->odc2 = set_bits32(csc->odc2, *((int *)p + 0 * j + 4), 11, 0);

	csc->p00 = set_bits32(csc->p00, *((int *)p + 0 * j + 0), 17, 0);
	csc->p01 = set_bits32(csc->p01, *((int *)p + 0 * j + 1), 17, 0);
	csc->p02 = set_bits32(csc->p02, *((int *)p + 0 * j + 2), 17, 0);

	csc->p10 = set_bits32(csc->p10, *((int *)p + 1 * j + 0), 17, 0);
	csc->p11 = set_bits32(csc->p11, *((int *)p + 1 * j + 1), 17, 0);
	csc->p12 = set_bits32(csc->p12, *((int *)p + 1 * j + 2), 17, 0);

	csc->p20 = set_bits32(csc->p20, *((int *)p + 2 * j + 0), 17, 0);
	csc->p21 = set_bits32(csc->p21, *((int *)p + 2 * j + 1), 17, 0);
	csc->p22 = set_bits32(csc->p22, *((int *)p + 2 * j + 2), 17, 0);
}

static void dpu_csc_get_coef(struct dpu_fb_data_type *dpufd, const struct dss_csc_info *csc_info,
	const dss_layer_t *layer, const dss_wb_layer_t *wb_layer)
{
	dss_csc_t *csc = NULL;
	struct dss_csc_coe csc_coe;

	csc_get_regular_coef(csc_info->csc_mode, &csc_coe);

	/* config rch csc and pcsc */
	if (layer) {
		if (dpufd->dss_module.csc_used[csc_info->chn_idx]) {
			csc = &(dpufd->dss_module.csc[csc_info->chn_idx]);
			dpu_csc_coef_array_config(csc, (int **)(csc_coe.csc_coe_yuv2rgb), CSC_COL);
		}
		if (dpufd->dss_module.pcsc_used[csc_info->chn_idx]) {
			csc = &(dpufd->dss_module.pcsc[csc_info->chn_idx]);
			dpu_csc_coef_array_config(csc, (int **)(csc_coe.csc_coe_rgb2yuv), CSC_COL);
		}
	}

	/* config wch csc */
	if (wb_layer && dpufd->dss_module.csc_used[csc_info->chn_idx]) {
		csc = &(dpufd->dss_module.csc[csc_info->chn_idx]);
		dpu_csc_coef_array_config(csc, (int **)(csc_coe.csc_coe_rgb2yuv), CSC_COL);
	}
}

int dpu_csc_config(struct dpu_fb_data_type *dpufd,
	dss_layer_t *layer, dss_wb_layer_t *wb_layer)
{
	struct dss_csc_info csc_info = { 0, 0, 0, false};

	/* layer and wb_layer maybe is NULL */
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");

	if (wb_layer) {
		csc_info.chn_idx = wb_layer->chn_idx;
		csc_info.format = wb_layer->dst.format;
		csc_info.csc_mode = wb_layer->dst.csc_mode;
	} else {
		if (layer) {
			csc_info.chn_idx = layer->chn_idx;
			csc_info.format = layer->img.format;
			csc_info.csc_mode = layer->img.csc_mode;
		}
	}

	if ((csc_info.chn_idx == DSS_RCHN_V0) && (dpufd->index == MEDIACOMMON_PANEL_IDX))
		return 0;

	if (csc_info.chn_idx != DSS_RCHN_V0) {
		if (!is_yuv(csc_info.format))
			return 0;
		dpufd->dss_module.csc_used[csc_info.chn_idx] = 1;
	} else if ((csc_info.chn_idx == DSS_RCHN_V0) && (!is_yuv(csc_info.format))) {  /* v0, rgb format */
		if (layer) {
			if (!is_pcsc_needed(layer))
				return 0;
		}

		dpufd->dss_module.csc_used[DSS_RCHN_V0] = 1;
		dpufd->dss_module.pcsc_used[DSS_RCHN_V0] = 1;
	} else {  /* v0, yuv format */
		dpufd->dss_module.csc_used[csc_info.chn_idx] = 1;
	}

	dpu_csc_get_coef(dpufd, &csc_info, layer, wb_layer);

	return 0;
}

