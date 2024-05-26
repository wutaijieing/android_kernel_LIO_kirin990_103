/* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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
#include "../../dpu_display_effect.h"
#include "../../dpu_dpe_utils.h"
#include "../../dpu_mmbuf_manager.h"

struct rect_pad {
	uint32_t left;
	uint32_t right;
	uint32_t top;
	uint32_t bottom;
};

struct dpu_fb_to_dfc_pixel_fmt {
	uint32_t fb_pixel_format;
	uint32_t dfc_pixel_format;
};

struct dfc_config {
	int dfc_fmt;
	int dfc_bpp;
	int dfc_pix_in_num;
	int dfc_aligned;
	uint32_t size_hrz;
	uint32_t size_vrt;
	int dfc_hrz_clip;
	bool need_clip;
};

struct wdfc_internal_data {
	uint32_t size_hrz;
	uint32_t size_vrt;
	int dfc_pix_in_num;
	uint32_t dfc_w;
	int aligned_pixel;
	uint32_t dfc_aligned;
};

void dpu_dfc_init(const char __iomem *dfc_base, dss_dfc_t *s_dfc)
{
	if (dfc_base == NULL) {
		DPU_FB_ERR("dfc_base is NULL\n");
		return;
	}
	if (s_dfc == NULL) {
		DPU_FB_ERR("s_dfc is NULL\n");
		return;
	}

	memset(s_dfc, 0, sizeof(dss_dfc_t));

	s_dfc->disp_size = inp32(dfc_base + DFC_DISP_SIZE);
	s_dfc->pix_in_num = inp32(dfc_base + DFC_PIX_IN_NUM);
	s_dfc->disp_fmt = inp32(dfc_base + DFC_DISP_FMT);
	s_dfc->clip_ctl_hrz = inp32(dfc_base + DFC_CLIP_CTL_HRZ);
	s_dfc->clip_ctl_vrz = inp32(dfc_base + DFC_CLIP_CTL_VRZ);
	s_dfc->ctl_clip_en = inp32(dfc_base + DFC_CTL_CLIP_EN);
	s_dfc->icg_module = inp32(dfc_base + DFC_ICG_MODULE);
	s_dfc->dither_enable = inp32(dfc_base + DFC_DITHER_ENABLE);
	s_dfc->padding_ctl = inp32(dfc_base + DFC_PADDING_CTL);
}

void dpu_dfc_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *dfc_base, dss_dfc_t *s_dfc)
{
	if (dpufd == NULL) {
		DPU_FB_DEBUG("dpufd is NULL!\n");
		return;
	}

	if (dfc_base == NULL) {
		DPU_FB_DEBUG("dfc_base is NULL!\n");
		return;
	}

	if (s_dfc == NULL) {
		DPU_FB_DEBUG("s_dfc is NULL!\n");
		return;
	}

	dpufd->set_reg(dpufd, dfc_base + DFC_DISP_SIZE, s_dfc->disp_size, 32, 0);
	dpufd->set_reg(dpufd, dfc_base + DFC_PIX_IN_NUM, s_dfc->pix_in_num, 32, 0);
	dpufd->set_reg(dpufd, dfc_base + DFC_DISP_FMT, s_dfc->disp_fmt, 32, 0);
	dpufd->set_reg(dpufd, dfc_base + DFC_CLIP_CTL_HRZ, s_dfc->clip_ctl_hrz, 32, 0);
	dpufd->set_reg(dpufd, dfc_base + DFC_CLIP_CTL_VRZ, s_dfc->clip_ctl_vrz, 32, 0);
	dpufd->set_reg(dpufd, dfc_base + DFC_CTL_CLIP_EN, s_dfc->ctl_clip_en, 32, 0);
	dpufd->set_reg(dpufd, dfc_base + DFC_ICG_MODULE, s_dfc->icg_module, 32, 0);
	dpufd->set_reg(dpufd, dfc_base + DFC_DITHER_ENABLE, s_dfc->dither_enable, 32, 0);
	dpufd->set_reg(dpufd, dfc_base + DFC_PADDING_CTL, s_dfc->padding_ctl, 32, 0);

	if (dpufd->index != EXTERNAL_PANEL_IDX)
		dpufd->set_reg(dpufd, dfc_base + DFC_BITEXT_CTL, s_dfc->bitext_ctl, 32, 0);
}

static struct dpu_fb_to_dfc_pixel_fmt dpu_dfc_fmt[] = {
	{ DPU_FB_PIXEL_FORMAT_RGB_565, DFC_PIXEL_FORMAT_RGB_565 },
	{ DPU_FB_PIXEL_FORMAT_RGBX_4444, DFC_PIXEL_FORMAT_XBGR_4444 },
	{ DPU_FB_PIXEL_FORMAT_RGBA_4444, DFC_PIXEL_FORMAT_ABGR_4444 },
	{ DPU_FB_PIXEL_FORMAT_RGBX_5551, DFC_PIXEL_FORMAT_XBGR_5551 },
	{ DPU_FB_PIXEL_FORMAT_RGBA_5551, DFC_PIXEL_FORMAT_ABGR_5551 },
	{ DPU_FB_PIXEL_FORMAT_RGBX_8888, DFC_PIXEL_FORMAT_XBGR_8888 },
	{ DPU_FB_PIXEL_FORMAT_RGBA_8888, DFC_PIXEL_FORMAT_ABGR_8888 },
	{ DPU_FB_PIXEL_FORMAT_RGBA_1010102, DFC_PIXEL_FORMAT_BGRA_1010102 },
	{ DPU_FB_PIXEL_FORMAT_BGR_565, DFC_PIXEL_FORMAT_BGR_565 },
	{ DPU_FB_PIXEL_FORMAT_BGRX_4444, DFC_PIXEL_FORMAT_XRGB_4444 },
	{ DPU_FB_PIXEL_FORMAT_BGRA_4444, DFC_PIXEL_FORMAT_ARGB_4444 },
	{ DPU_FB_PIXEL_FORMAT_BGRX_5551, DFC_PIXEL_FORMAT_XRGB_5551 },
	{ DPU_FB_PIXEL_FORMAT_BGRA_5551, DFC_PIXEL_FORMAT_ARGB_5551 },
	{ DPU_FB_PIXEL_FORMAT_BGRX_8888, DFC_PIXEL_FORMAT_XRGB_8888 },
	{ DPU_FB_PIXEL_FORMAT_BGRA_8888, DFC_PIXEL_FORMAT_ARGB_8888 },
	{ DPU_FB_PIXEL_FORMAT_BGRA_1010102, DFC_PIXEL_FORMAT_BGRA_1010102 },
	{ DPU_FB_PIXEL_FORMAT_YUV_422_I, DFC_PIXEL_FORMAT_YUYV422 },
	{ DPU_FB_PIXEL_FORMAT_YUYV_422_PKG, DFC_PIXEL_FORMAT_YUYV422 },
	{ DPU_FB_PIXEL_FORMAT_YVYU_422_PKG, DFC_PIXEL_FORMAT_YVYU422 },
	{ DPU_FB_PIXEL_FORMAT_UYVY_422_PKG, DFC_PIXEL_FORMAT_UYVY422 },
	{ DPU_FB_PIXEL_FORMAT_VYUY_422_PKG, DFC_PIXEL_FORMAT_VYUY422 },
	{ DPU_FB_PIXEL_FORMAT_YCBCR_422_SP, DFC_PIXEL_FORMAT_YUYV422 },
	{ DPU_FB_PIXEL_FORMAT_YCRCB_422_SP, DFC_PIXEL_FORMAT_YVYU422 },
	{ DPU_FB_PIXEL_FORMAT_YCBCR_420_SP, DFC_PIXEL_FORMAT_YUYV422 },
	{ DPU_FB_PIXEL_FORMAT_YCRCB_420_SP, DFC_PIXEL_FORMAT_YVYU422 },
	{ DPU_FB_PIXEL_FORMAT_YCBCR_422_P, DFC_PIXEL_FORMAT_YUYV422 },
	{ DPU_FB_PIXEL_FORMAT_YCBCR_420_P, DFC_PIXEL_FORMAT_YUYV422 },
	{ DPU_FB_PIXEL_FORMAT_YCRCB_422_P, DFC_PIXEL_FORMAT_YVYU422 },
	{ DPU_FB_PIXEL_FORMAT_YCRCB_420_P, DFC_PIXEL_FORMAT_YVYU422 },
	{ DPU_FB_PIXEL_FORMAT_Y410_10BIT, DFC_PIXEL_FORMAT_UYVA_1010102 },
	{ DPU_FB_PIXEL_FORMAT_YCRCB420_SP_10BIT, DFC_PIXEL_FORMAT_YUYV_10 },
	{ DPU_FB_PIXEL_FORMAT_YUV422_10BIT, DFC_PIXEL_FORMAT_YUYV_10 },
	{ DPU_FB_PIXEL_FORMAT_YCBCR420_SP_10BIT, DFC_PIXEL_FORMAT_YUYV_10 },
	{ DPU_FB_PIXEL_FORMAT_YCBCR422_SP_10BIT, DFC_PIXEL_FORMAT_YUYV_10 },
	{ DPU_FB_PIXEL_FORMAT_YCBCR420_P_10BIT, DFC_PIXEL_FORMAT_YUYV_10 },
	{ DPU_FB_PIXEL_FORMAT_YCBCR422_P_10BIT, DFC_PIXEL_FORMAT_YUYV_10 },
};

static int dpu_pixel_format_hal2dfc(int format)
{
	int i;
	int size = sizeof(dpu_dfc_fmt) / sizeof(dpu_dfc_fmt[0]);

	for (i = 0; i < size; i++) {
		if ((int)(dpu_dfc_fmt[i].fb_pixel_format) == format)
			break;
	}

	if (i >= size) {
		DPU_FB_ERR("not support format-%d!\n", format);
		return -1;
	}

	return dpu_dfc_fmt[i].dfc_pixel_format;
}

static int dpu_rb_swap(int format)
{
	switch (format) {
	case DPU_FB_PIXEL_FORMAT_BGR_565:
	case DPU_FB_PIXEL_FORMAT_BGRX_4444:
	case DPU_FB_PIXEL_FORMAT_BGRA_4444:
	case DPU_FB_PIXEL_FORMAT_BGRX_5551:
	case DPU_FB_PIXEL_FORMAT_BGRA_5551:
	case DPU_FB_PIXEL_FORMAT_BGRX_8888:
	case DPU_FB_PIXEL_FORMAT_BGRA_8888:
	case DPU_FB_PIXEL_FORMAT_BGRA_1010102:
		return 1;
	default:
		return 0;
	}
}

static int dpu_uv_swap(int format)
{
	switch (format) {
	case DPU_FB_PIXEL_FORMAT_YCRCB_422_SP:
	case DPU_FB_PIXEL_FORMAT_YCRCB_420_SP:
	case DPU_FB_PIXEL_FORMAT_YCRCB_422_P:
	case DPU_FB_PIXEL_FORMAT_YCRCB_420_P:
	case DPU_FB_PIXEL_FORMAT_YCRCB420_SP_10BIT:
	case DPU_FB_PIXEL_FORMAT_YCRCB422_SP_10BIT:
		return 1;

	default:
		return 0;
	}
}

static int dpu_dfc_get_bpp(int dfc_format)
{
	int ret = 0;

	switch (dfc_format) {
	case DFC_PIXEL_FORMAT_RGB_565:
	case DFC_PIXEL_FORMAT_XRGB_4444:
	case DFC_PIXEL_FORMAT_ARGB_4444:
	case DFC_PIXEL_FORMAT_XRGB_5551:
	case DFC_PIXEL_FORMAT_ARGB_5551:

	case DFC_PIXEL_FORMAT_BGR_565:
	case DFC_PIXEL_FORMAT_XBGR_4444:
	case DFC_PIXEL_FORMAT_ABGR_4444:
	case DFC_PIXEL_FORMAT_XBGR_5551:
	case DFC_PIXEL_FORMAT_ABGR_5551:
		ret = 2;  /* dfc_bpp format */
		break;

	case DFC_PIXEL_FORMAT_XRGB_8888:
	case DFC_PIXEL_FORMAT_ARGB_8888:
	case DFC_PIXEL_FORMAT_XBGR_8888:
	case DFC_PIXEL_FORMAT_ABGR_8888:
	case DFC_PIXEL_FORMAT_BGRA_1010102:
		ret = 4;  /* dfc_bpp format */
		break;

	case DFC_PIXEL_FORMAT_YUV444:
	case DFC_PIXEL_FORMAT_YVU444:
		ret = 3;  /* dfc_bpp format */
		break;

	case DFC_PIXEL_FORMAT_YUYV422:
	case DFC_PIXEL_FORMAT_YVYU422:
	case DFC_PIXEL_FORMAT_VYUY422:
	case DFC_PIXEL_FORMAT_UYVY422:
	case DFC_PIXEL_FORMAT_YUYV_10:
		ret = 2;  /* dfc_bpp format */
		break;
	case DFC_PIXEL_FORMAT_UYVA_1010102:
		ret = 4;  /* dfc_bpp format */
		break;
	default:
		DPU_FB_ERR("not support format[%d]!\n", dfc_format);
		ret = -1;
		break;
	}

	return ret;
}

static bool is_need_rect_clip(dss_rect_ltrb_t clip_rect)
{
	return ((clip_rect.left > 0) || (clip_rect.top > 0) ||
		(clip_rect.right > 0) || (clip_rect.bottom > 0));
}

static void dpu_config_dfc_value(dss_layer_t *layer, dss_dfc_t *dfc, struct dfc_config rdfc_config,
		dss_rect_ltrb_t clip_rect, bool need_bitext)
{
	/* disp-size valve */
	dfc->disp_size = set_bits32(dfc->disp_size, (rdfc_config.size_vrt | (rdfc_config.size_hrz << 16)), 29, 0);
	dfc->pix_in_num = set_bits32(dfc->pix_in_num, rdfc_config.dfc_pix_in_num, 1, 0); /* pix_num valve */
	dfc->disp_fmt = set_bits32(dfc->disp_fmt,
		(((uint32_t)rdfc_config.dfc_fmt << 1) | ((uint32_t)dpu_uv_swap(layer->img.format) << 6) |
		((uint32_t)dpu_rb_swap(layer->img.format) << 7)), 8, 0);

	if (rdfc_config.need_clip) {
		dfc->clip_ctl_hrz = set_bits32(dfc->clip_ctl_hrz,
			((uint32_t)clip_rect.right | ((uint32_t)clip_rect.left << 16)), 32, 0);
		dfc->clip_ctl_vrz = set_bits32(dfc->clip_ctl_vrz,
			((uint32_t)clip_rect.bottom | ((uint32_t)clip_rect.top << 16)), 32, 0);
		dfc->ctl_clip_en = set_bits32(dfc->ctl_clip_en, 0x1, 1, 0);
	} else {
		dfc->clip_ctl_hrz = set_bits32(dfc->clip_ctl_hrz, 0x0, 32, 0);
		dfc->clip_ctl_vrz = set_bits32(dfc->clip_ctl_vrz, 0x0, 32, 0);
		dfc->ctl_clip_en = set_bits32(dfc->ctl_clip_en, 0x1, 1, 0);
	}
	dfc->icg_module = set_bits32(dfc->icg_module, 0x1, 1, 0);
	dfc->dither_enable = set_bits32(dfc->dither_enable, 0x0, 1, 0);
	dfc->padding_ctl = set_bits32(dfc->padding_ctl, 0x0, 17, 0);

	if (need_bitext)
		dfc->bitext_ctl = set_bits32(dfc->bitext_ctl, 0x3, 32, 0);
}

int dpu_rdfc_config(struct dpu_fb_data_type *dpufd,
	dss_layer_t *layer,
	dss_rect_t *aligned_rect,
	dss_rect_ltrb_t clip_rect,
	struct dpu_ov_compose_flag *ov_compose_flag)
{
	dss_dfc_t *dfc = NULL;
	int chn_idx;
	bool need_bitext = false;
	struct dfc_config rdfc_config;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");
	dpu_check_and_return(!layer, -EINVAL, ERR, "layer is NULL\n");
	dpu_check_and_return(!aligned_rect, -EINVAL, ERR, "aligned_rect is NULL\n");
	dpu_check_and_return(!ov_compose_flag, -EINVAL, ERR, "ov_compose_flag is NULL\n");

	chn_idx = layer->chn_idx;

	dfc = &(dpufd->dss_module.dfc[chn_idx]);
	dpufd->dss_module.dfc_used[chn_idx] = 1;

	rdfc_config.dfc_fmt = dpu_pixel_format_hal2dfc(layer->img.format);
	if (rdfc_config.dfc_fmt < 0) {
		DPU_FB_ERR("layer format-%d not support!\n", layer->img.format);
		return -EINVAL;
	}
	need_bitext = !is_pixel_10bit2dfc(rdfc_config.dfc_fmt);

	rdfc_config.dfc_bpp = dpu_dfc_get_bpp(rdfc_config.dfc_fmt);
	if (rdfc_config.dfc_bpp <= 0) {
		DPU_FB_ERR("dfc_bpp-%d not support !\n", rdfc_config.dfc_bpp);
		return -EINVAL;
	}

	rdfc_config.dfc_pix_in_num = (rdfc_config.dfc_bpp <= 2) ? 0x1 : 0x0;  /* dfc pix nums */
	rdfc_config.dfc_aligned = (rdfc_config.dfc_bpp <= 2) ? 4 : 2;  /* dfc aligned valve */

	rdfc_config.need_clip = is_need_rect_clip(clip_rect);

	rdfc_config.size_hrz = DSS_WIDTH(aligned_rect->w);
	rdfc_config.size_vrt = DSS_HEIGHT(aligned_rect->h);

	if (((rdfc_config.size_hrz + 1) % rdfc_config.dfc_aligned) != 0) {
		rdfc_config.size_hrz -= 1;
		DPU_FB_ERR("SIZE_HRT=%d mismatch!bpp=%d\n", rdfc_config.size_hrz, layer->img.bpp);

		DPU_FB_ERR("layer_idx%d, format=%d, transform=%d, original_src_rect:%d,%d,%d,%d, "
			"rdma_out_rect:%d,%d,%d,%d, dst_rect:%d,%d,%d,%d!\n",
			layer->layer_idx, layer->img.format, layer->transform,
			layer->src_rect.x, layer->src_rect.y, layer->src_rect.w, layer->src_rect.h,
			aligned_rect->x, aligned_rect->y, aligned_rect->w, aligned_rect->h,
			layer->dst_rect.x, layer->dst_rect.y, layer->dst_rect.w, layer->dst_rect.h);
	}

	rdfc_config.dfc_hrz_clip = (rdfc_config.size_hrz + 1) % rdfc_config.dfc_aligned;
	if (rdfc_config.dfc_hrz_clip) {
		clip_rect.right += rdfc_config.dfc_hrz_clip;
		rdfc_config.size_hrz += rdfc_config.dfc_hrz_clip;
		rdfc_config.need_clip = true;
	}

	if (!ov_compose_flag->csc_needed)
		need_bitext = false;

	dpu_config_dfc_value(layer, dfc, rdfc_config, clip_rect, need_bitext);

	/* update */
	if (rdfc_config.need_clip) {
		aligned_rect->w -= (clip_rect.left + clip_rect.right);
		aligned_rect->h -= (clip_rect.top + clip_rect.bottom);
	}

	return 0;
}
