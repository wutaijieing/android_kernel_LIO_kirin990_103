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

/* application: mode 2 is used in rgb2yuv, mode 0 is used in yuv2rgb */
#define CSC_MPREC_MODE_0 0
#define CSC_MPREC_MODE_1 1  /* never used for ES */
#define CSC_MPREC_MODE_2 2  /* yuv2rgb is not supported by mode 2 */

#define CSC_MPREC_MODE_RGB2YUV CSC_MPREC_MODE_2
#define CSC_MPREC_MODE_YUV2RGB CSC_MPREC_MODE_0

static int g_csc_coe_yuv2rgb601_narrow_mprec0[CSC_ROW][CSC_COL] = {
	{0x4a8, 0x000, 0x662, 0x7f0, 0x000},
	{0x4a8, 0x1e6f, 0x1cc0, 0x780, 0x000},
	{0x4a8, 0x812, 0x000, 0x780, 0x000}
};

static int g_csc_coe_rgb2yuv601_narrow_mprec2[CSC_ROW][CSC_COL] = {
	{0x41C, 0x811, 0x191, 0x000, 0x010},
	{0x1DA1, 0x1B58, 0x707, 0x000, 0x080},
	{0x707, 0x1A1E, 0x1EDB, 0x000, 0x080}
};

static int g_csc_coe_yuv2rgb709_narrow_mprec0[CSC_ROW][CSC_COL] = {
	{0x4a8, 0x000, 0x72c, 0x7f0, 0x000},
	{0x4a8, 0x1f26, 0x1dde, 0x77f, 0x000},
	{0x4a8, 0x873, 0x000, 0x77f, 0x000}
};

static int g_csc_coe_rgb2yuv709_narrow_mprec2[CSC_ROW][CSC_COL] = {
	{0x2EC, 0x9D4, 0x0FE, 0x000, 0x010},
	{0x1E64, 0x1A95, 0x707, 0x000, 0x081},
	{0x707, 0x199E, 0x1F5B, 0x000, 0x081}
};

static int g_csc_coe_yuv2rgb601_wide_mprec0[CSC_ROW][CSC_COL] = {
	{0x400, 0x000, 0x59c, 0x000, 0x000},
	{0x400, 0x1ea0, 0x1d25, 0x77f, 0x000},
	{0x400, 0x717, 0x000, 0x77f, 0x000}
};

static int g_csc_coe_rgb2yuv601_wide_mprec2[CSC_ROW][CSC_COL] = {
	{0x4C9, 0x964, 0x1d3, 0x000, 0x000},
	{0x1D4D, 0x1AB3, 0x800, 0x000, 0x081},
	{0x800, 0x194D, 0x1EB3, 0x000, 0x081},
};

static int g_csc_coe_yuv2rgb709_wide_mprec0[CSC_ROW][CSC_COL] = {
	{0x400, 0x000, 0x64d, 0x000, 0x000},
	{0x400, 0x1f40, 0x1e21, 0x77f, 0x000},
	{0x400, 0x76c, 0x000, 0x77f, 0x000}
};

static int g_csc_coe_rgb2yuv709_wide_mprec2[CSC_ROW][CSC_COL] = {
	{0x367, 0xB71, 0x128, 0x000, 0x000},
	{0x1E2B, 0x19D5, 0x800, 0x000, 0x081},
	{0x800, 0x18BC, 0x1F44, 0x000, 0x081},
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

	s_csc->p0 = inp32(csc_base + CSC_P0);
	s_csc->p1 = inp32(csc_base + CSC_P1);
	s_csc->p2 = inp32(csc_base + CSC_P2);
	s_csc->p3 = inp32(csc_base + CSC_P3);
	s_csc->p4 = inp32(csc_base + CSC_P4);
	s_csc->icg_module = inp32(csc_base + CSC_ICG_MODULE);
	s_csc->mprec = inp32(csc_base + CSC_MPREC);
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

	dpufd->set_reg(dpufd, csc_base + CSC_P0, s_csc->p0, 32, 0);
	dpufd->set_reg(dpufd, csc_base + CSC_P1, s_csc->p1, 32, 0);
	dpufd->set_reg(dpufd, csc_base + CSC_P2, s_csc->p2, 32, 0);
	dpufd->set_reg(dpufd, csc_base + CSC_P3, s_csc->p3, 32, 0);
	dpufd->set_reg(dpufd, csc_base + CSC_P4, s_csc->p4, 32, 0);
	dpufd->set_reg(dpufd, csc_base + CSC_ICG_MODULE, s_csc->icg_module, 32, 0);
	dpufd->set_reg(dpufd, csc_base + CSC_MPREC, s_csc->mprec, 32, 0);
}

static bool is_pcsc_needed(const dss_layer_t *layer)
{
	if (layer->chn_idx != DSS_RCHN_V1)
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
		csc_coe->csc_coe_yuv2rgb = g_csc_coe_yuv2rgb601_wide_mprec0;
		csc_coe->csc_coe_rgb2yuv = g_csc_coe_rgb2yuv601_wide_mprec2;
		break;
	case DSS_CSC_601_NARROW:
		csc_coe->csc_coe_yuv2rgb = g_csc_coe_yuv2rgb601_narrow_mprec0;
		csc_coe->csc_coe_rgb2yuv = g_csc_coe_rgb2yuv601_narrow_mprec2;
		break;
	case DSS_CSC_709_WIDE:
		csc_coe->csc_coe_yuv2rgb = g_csc_coe_yuv2rgb709_wide_mprec0;
		csc_coe->csc_coe_rgb2yuv = g_csc_coe_rgb2yuv709_wide_mprec2;
		break;
	case DSS_CSC_709_NARROW:
		csc_coe->csc_coe_yuv2rgb = g_csc_coe_yuv2rgb709_narrow_mprec0;
		csc_coe->csc_coe_rgb2yuv = g_csc_coe_rgb2yuv709_narrow_mprec2;
		break;
	default:
		/* TBD  add csc mprec mode 1 and mode 2 */
		DPU_FB_ERR("not support this csc_mode [%d]!\n", csc_mode);
		csc_coe->csc_coe_yuv2rgb = g_csc_coe_yuv2rgb601_wide_mprec0;
		csc_coe->csc_coe_rgb2yuv = g_csc_coe_rgb2yuv601_wide_mprec2;
		break;
	}
}

static void dpu_csc_coef_array_config(dss_csc_t *csc, int **p, int j)
{
	csc->icg_module = set_bits32(csc->icg_module, 0x1, 1, 0);

	csc->idc0 = set_bits32(csc->idc0, (uint32_t)(*((int *)p + 2 * j + 3)) |
		((uint32_t)(*((int *)p + 1 * j + 3)) << 16), 27, 0);
	csc->idc2 = set_bits32(csc->idc2, (uint32_t)(*((int *)p + 0 * j + 3)), 11, 0);

	csc->odc0 = set_bits32(csc->odc0, (uint32_t)(*((int *)p + 2 * j + 4)) |
		((uint32_t)(*((int *)p + 1 * j + 4)) << 16), 27, 0);
	csc->odc2 = set_bits32(csc->odc2, (uint32_t)(*((int *)p + 0 * j + 4)), 11, 0);

	csc->p0 = set_bits32(csc->p0, (uint32_t)(*((int *)p + 0 * j + 0)), 13, 0);
	csc->p0 = set_bits32(csc->p0, (uint32_t)(*((int *)p + 0 * j + 1)), 13, 16);

	csc->p1 = set_bits32(csc->p1, (uint32_t)(*((int *)p + 0 * j + 2)), 13, 0);
	csc->p1 = set_bits32(csc->p1, (uint32_t)(*((int *)p + 1 * j + 0)), 13, 16);

	csc->p2 = set_bits32(csc->p2, (uint32_t)(*((int *)p + 1 * j + 1)), 13, 0);
	csc->p2 = set_bits32(csc->p2, (uint32_t)(*((int *)p + 1 * j + 2)), 13, 16);

	csc->p3 = set_bits32(csc->p3, (uint32_t)(*((int *)p + 2 * j + 0)), 13, 0);
	csc->p3 = set_bits32(csc->p3, (uint32_t)(*((int *)p + 2 * j + 1)), 13, 16);

	csc->p4 = set_bits32(csc->p4, (uint32_t)(*((int *)p + 2 * j + 2)), 13, 0);
}

static void dpu_csc_get_coef(struct dpu_fb_data_type *dpufd, const struct dss_csc_info *csc_info,
	const dss_layer_t *layer, const dss_wb_layer_t *wb_layer)
{
	dss_csc_t *csc = NULL;
	struct dss_csc_coe csc_coe;

	csc_get_regular_coef(csc_info->csc_mode, &csc_coe);

	if (layer) {
		/* config rch csc */
		if (dpufd->dss_module.csc_used[csc_info->chn_idx]) {
			csc = &(dpufd->dss_module.csc[csc_info->chn_idx]);
			csc->mprec = CSC_MPREC_MODE_YUV2RGB;
			dpu_csc_coef_array_config(csc, (int **)(csc_coe.csc_coe_yuv2rgb), CSC_COL);
		}

		/* config rch pcsc */
		if (dpufd->dss_module.pcsc_used[csc_info->chn_idx]) {
			csc = &(dpufd->dss_module.pcsc[csc_info->chn_idx]);
			csc->mprec = CSC_MPREC_MODE_RGB2YUV;
			dpu_csc_coef_array_config(csc, (int **)(csc_coe.csc_coe_rgb2yuv), CSC_COL);
		}
	}

	/* config wch csc */
	if (wb_layer) {
		csc = &(dpufd->dss_module.csc[csc_info->chn_idx]);
		csc->mprec = CSC_MPREC_MODE_RGB2YUV;
		dpu_csc_coef_array_config(csc, (int **)(csc_coe.csc_coe_rgb2yuv), CSC_COL);
	}
}

/* only for csc8b */
int dpu_csc_config(struct dpu_fb_data_type *dpufd,
	dss_layer_t *layer, dss_wb_layer_t *wb_layer)
{
	struct dss_csc_info csc_info = {0};

	/* layer and wb_layer maybe is NULL */
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");

	if (wb_layer) {
		csc_info.chn_idx = wb_layer->chn_idx;
		csc_info.format = wb_layer->dst.format;
		csc_info.csc_mode = wb_layer->dst.csc_mode;
	} else if (layer) {
		csc_info.chn_idx = layer->chn_idx;
		csc_info.format = layer->img.format;
		csc_info.csc_mode = layer->img.csc_mode;
	}

	if (csc_info.chn_idx != DSS_RCHN_V1) {
		if (!is_yuv(csc_info.format))
			return 0;
		dpufd->dss_module.csc_used[csc_info.chn_idx] = 1;
	} else if ((csc_info.chn_idx == DSS_RCHN_V1) && (!is_yuv(csc_info.format))) {  /* v1, rgb format */
		if (layer) {
			if (!is_pcsc_needed(layer))
				return 0;
		}
		dpufd->dss_module.csc_used[DSS_RCHN_V1] = 1;
		dpufd->dss_module.pcsc_used[DSS_RCHN_V1] = 1;
	} else {  /* v1, yuv format */
		dpufd->dss_module.csc_used[csc_info.chn_idx] = 1;
	}

	dpu_csc_get_coef(dpufd, &csc_info, layer, wb_layer);

	return 0;
}

