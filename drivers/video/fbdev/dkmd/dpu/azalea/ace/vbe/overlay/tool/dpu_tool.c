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

struct dpu_fb_to_dma_pixel_fmt {
	uint32_t fb_pixel_format;
	uint32_t dma_pixel_format;
};

bool hal_format_has_alpha(uint32_t format)
{
	switch (format) {
	case DPU_FB_PIXEL_FORMAT_RGBA_4444:
	case DPU_FB_PIXEL_FORMAT_RGBA_5551:
	case DPU_FB_PIXEL_FORMAT_RGBA_8888:

	case DPU_FB_PIXEL_FORMAT_BGRA_4444:
	case DPU_FB_PIXEL_FORMAT_BGRA_5551:
	case DPU_FB_PIXEL_FORMAT_BGRA_8888:

	case DPU_FB_PIXEL_FORMAT_RGBA_1010102:
	case DPU_FB_PIXEL_FORMAT_BGRA_1010102:
		return true;

	default:
		return false;
	}
}

bool is_pixel_10bit2dma(int format)
{
	switch (format) {
	case DMA_PIXEL_FORMAT_RGBA_1010102:
	case DMA_PIXEL_FORMAT_Y410_10BIT:
	case DMA_PIXEL_FORMAT_YUV422_10BIT:
	case DMA_PIXEL_FORMAT_YUV420_SP_10BIT:
	case DMA_PIXEL_FORMAT_YUV422_SP_10BIT:
	case DMA_PIXEL_FORMAT_YUV420_P_10BIT:
	case DMA_PIXEL_FORMAT_YUV422_P_10BIT:
		return true;

	default:
		return false;
	}
}

bool is_pixel_10bit2dfc(int format)
{
	switch (format) {
	case DFC_PIXEL_FORMAT_BGRA_1010102:
	case DFC_PIXEL_FORMAT_YUVA_1010102:
	case DFC_PIXEL_FORMAT_UYVA_1010102:
	case DFC_PIXEL_FORMAT_VUYA_1010102:
	case DFC_PIXEL_FORMAT_YUYV_10:
	case DFC_PIXEL_FORMAT_UYVY_10:
		return true;

	default:
		return false;
	}
}

bool is_yuv_package(uint32_t format)
{
	switch (format) {
	case DPU_FB_PIXEL_FORMAT_YUV_422_I:
	case DPU_FB_PIXEL_FORMAT_YUYV_422_PKG:
	case DPU_FB_PIXEL_FORMAT_YVYU_422_PKG:
	case DPU_FB_PIXEL_FORMAT_UYVY_422_PKG:
	case DPU_FB_PIXEL_FORMAT_VYUY_422_PKG:
	case DPU_FB_PIXEL_FORMAT_YUV422_10BIT:
		return true;

	default:
		return false;
	}
}

bool is_yuv_semiplanar(uint32_t format)
{
	switch (format) {
	case DPU_FB_PIXEL_FORMAT_YCBCR_422_SP:
	case DPU_FB_PIXEL_FORMAT_YCRCB_422_SP:
	case DPU_FB_PIXEL_FORMAT_YCBCR_420_SP:
	case DPU_FB_PIXEL_FORMAT_YCRCB_420_SP:
	case DPU_FB_PIXEL_FORMAT_YCRCB420_SP_10BIT:
	case DPU_FB_PIXEL_FORMAT_YCBCR420_SP_10BIT:
	case DPU_FB_PIXEL_FORMAT_YCBCR422_SP_10BIT:
		return true;

	default:
		return false;
	}
}

bool is_yuv_plane(uint32_t format)
{
	switch (format) {
	case DPU_FB_PIXEL_FORMAT_YCBCR_422_P:
	case DPU_FB_PIXEL_FORMAT_YCRCB_422_P:
	case DPU_FB_PIXEL_FORMAT_YCBCR_420_P:
	case DPU_FB_PIXEL_FORMAT_YCRCB_420_P:
	case DPU_FB_PIXEL_FORMAT_YCBCR420_P_10BIT:
	case DPU_FB_PIXEL_FORMAT_YCBCR422_P_10BIT:
		return true;

	default:
		return false;
	}
}

bool is_yuv(uint32_t format)
{
	return (is_yuv_package(format) || is_yuv_semiplanar(format) || is_yuv_plane(format));
}

bool is_yuv_sp_420(uint32_t format)
{
	switch (format) {
	case DPU_FB_PIXEL_FORMAT_YCBCR_420_SP:
	case DPU_FB_PIXEL_FORMAT_YCRCB_420_SP:
	case DPU_FB_PIXEL_FORMAT_YCBCR420_SP_10BIT:
	case DPU_FB_PIXEL_FORMAT_YCRCB420_SP_10BIT:
		return true;

	default:
		return false;
	}
}

bool is_yuv_sp_422(uint32_t format)
{
	switch (format) {
	case DPU_FB_PIXEL_FORMAT_YCBCR_422_SP:
	case DPU_FB_PIXEL_FORMAT_YCRCB_422_SP:
		return true;

	default:
		return false;
	}
}

bool is_yuv_p_420(uint32_t format)
{
	switch (format) {
	case DPU_FB_PIXEL_FORMAT_YCBCR_420_P:
	case DPU_FB_PIXEL_FORMAT_YCRCB_420_P:
	case DPU_FB_PIXEL_FORMAT_YCBCR420_P_10BIT:
		return true;

	default:
		return false;
	}
}

bool is_yuv_p_422(uint32_t format)
{
	switch (format) {
	case DPU_FB_PIXEL_FORMAT_YCBCR_422_P:
	case DPU_FB_PIXEL_FORMAT_YCRCB_422_P:
	case DPU_FB_PIXEL_FORMAT_YCBCR422_P_10BIT:
		return true;

	default:
		return false;
	}
}

bool is_rgbx(uint32_t format)
{
	switch (format) {
	case DPU_FB_PIXEL_FORMAT_RGBX_4444:
	case DPU_FB_PIXEL_FORMAT_BGRX_4444:
	case DPU_FB_PIXEL_FORMAT_RGBX_5551:
	case DPU_FB_PIXEL_FORMAT_BGRX_5551:
	case DPU_FB_PIXEL_FORMAT_RGBX_8888:
	case DPU_FB_PIXEL_FORMAT_BGRX_8888:
		return true;

	default:
		return false;
	}
}

uint32_t is_need_rdma_stretch_bit(struct dpu_fb_data_type *dpufd, dss_layer_t *layer)
{
	int v_stretch_ratio_threshold = 0;
	uint32_t v_stretch_ratio = 0;
	void_unused(dpufd);

	if (layer == NULL) {
		DPU_FB_ERR("layer is NULL\n");
		return 0;
	}

	if ((layer->need_cap & CAP_AFBCD) || (layer->need_cap & CAP_HFBCD) || (layer->need_cap & CAP_HEBCD)) {
		v_stretch_ratio = 0;
	} else {
		if (is_yuv_sp_420(layer->img.format) || is_yuv_p_420(layer->img.format)) {
			v_stretch_ratio_threshold = ((layer->src_rect.h + layer->dst_rect.h - 1) / layer->dst_rect.h);
			v_stretch_ratio = ((layer->src_rect.h / layer->dst_rect.h) / 2) * 2;
		} else {
			v_stretch_ratio_threshold = ((layer->src_rect.h + layer->dst_rect.h - 1) / layer->dst_rect.h);
			v_stretch_ratio = (layer->src_rect.h / layer->dst_rect.h);
		}

		if (v_stretch_ratio_threshold <= g_rdma_stretch_threshold)
			v_stretch_ratio = 0;
	}

	return v_stretch_ratio;
}

int dpu_adjust_clip_rect(dss_layer_t *layer, dss_rect_ltrb_t *clip_rect)
{
	int ret = 0;
	uint32_t temp;

	dpu_check_and_return((layer == NULL), -EINVAL, ERR, "layer is NULL\n");
	dpu_check_and_return((clip_rect == NULL), -EINVAL, ERR, "clip_rect is NULL\n");

	if ((clip_rect->left < 0 || clip_rect->left > DFC_MAX_CLIP_NUM) ||
		(clip_rect->right < 0 || clip_rect->right > DFC_MAX_CLIP_NUM) ||
		(clip_rect->top < 0 || clip_rect->top > DFC_MAX_CLIP_NUM) ||
		(clip_rect->bottom < 0 || clip_rect->bottom > DFC_MAX_CLIP_NUM))
		return -EINVAL;

	switch (layer->transform) {
	case DPU_FB_TRANSFORM_NOP:
		/* do nothing */
		break;
	case DPU_FB_TRANSFORM_FLIP_H:
	case DPU_FB_TRANSFORM_ROT_90:
		{
			temp = clip_rect->left;
			clip_rect->left = clip_rect->right;
			clip_rect->right = temp;
		}
		break;
	case DPU_FB_TRANSFORM_FLIP_V:
	case DPU_FB_TRANSFORM_ROT_270:
		{
			temp = clip_rect->top;
			clip_rect->top = clip_rect->bottom;
			clip_rect->bottom = temp;
		}
		break;
	case DPU_FB_TRANSFORM_ROT_180:
		{
			temp = clip_rect->left;
			clip_rect->left =  clip_rect->right;
			clip_rect->right = temp;

			temp = clip_rect->top;
			clip_rect->top =  clip_rect->bottom;
			clip_rect->bottom = temp;
		}
		break;
	default:
		DPU_FB_ERR("not supported this transform:%d!\n", layer->transform);
		break;
	}

	return ret;
}

static struct dpu_fb_to_dma_pixel_fmt dpu_dma_fmt[] = {
	{ DPU_FB_PIXEL_FORMAT_RGB_565, DMA_PIXEL_FORMAT_RGB_565 },
	{ DPU_FB_PIXEL_FORMAT_BGR_565, DMA_PIXEL_FORMAT_RGB_565 },
	{ DPU_FB_PIXEL_FORMAT_RGBX_4444, DMA_PIXEL_FORMAT_XRGB_4444 },
	{ DPU_FB_PIXEL_FORMAT_BGRX_4444, DMA_PIXEL_FORMAT_XRGB_4444 },
	{ DPU_FB_PIXEL_FORMAT_RGBA_4444, DMA_PIXEL_FORMAT_ARGB_4444 },
	{ DPU_FB_PIXEL_FORMAT_BGRA_4444, DMA_PIXEL_FORMAT_ARGB_4444 },
	{ DPU_FB_PIXEL_FORMAT_RGBX_5551, DMA_PIXEL_FORMAT_XRGB_5551 },
	{ DPU_FB_PIXEL_FORMAT_BGRX_5551, DMA_PIXEL_FORMAT_XRGB_5551 },
	{ DPU_FB_PIXEL_FORMAT_RGBA_5551, DMA_PIXEL_FORMAT_ARGB_5551 },
	{ DPU_FB_PIXEL_FORMAT_BGRA_5551, DMA_PIXEL_FORMAT_ARGB_5551 },
	{ DPU_FB_PIXEL_FORMAT_RGBX_8888, DMA_PIXEL_FORMAT_XRGB_8888 },
	{ DPU_FB_PIXEL_FORMAT_BGRX_8888, DMA_PIXEL_FORMAT_XRGB_8888 },
	{ DPU_FB_PIXEL_FORMAT_RGBA_8888, DMA_PIXEL_FORMAT_ARGB_8888 },
	{ DPU_FB_PIXEL_FORMAT_BGRA_8888, DMA_PIXEL_FORMAT_ARGB_8888 },
	{ DPU_FB_PIXEL_FORMAT_YUV_422_I, DMA_PIXEL_FORMAT_YUYV_422_Pkg },
	{ DPU_FB_PIXEL_FORMAT_YUYV_422_PKG, DMA_PIXEL_FORMAT_YUYV_422_Pkg },
	{ DPU_FB_PIXEL_FORMAT_YVYU_422_PKG, DMA_PIXEL_FORMAT_YUYV_422_Pkg },
	{ DPU_FB_PIXEL_FORMAT_UYVY_422_PKG, DMA_PIXEL_FORMAT_YUYV_422_Pkg },
	{ DPU_FB_PIXEL_FORMAT_VYUY_422_PKG, DMA_PIXEL_FORMAT_YUYV_422_Pkg },
	{ DPU_FB_PIXEL_FORMAT_YCBCR_422_P, DMA_PIXEL_FORMAT_YUV_422_P_HP },
	{ DPU_FB_PIXEL_FORMAT_YCRCB_422_P, DMA_PIXEL_FORMAT_YUV_422_P_HP },
	{ DPU_FB_PIXEL_FORMAT_YCBCR_420_P, DMA_PIXEL_FORMAT_YUV_420_P_HP },
	{ DPU_FB_PIXEL_FORMAT_YCRCB_420_P, DMA_PIXEL_FORMAT_YUV_420_P_HP },
	{ DPU_FB_PIXEL_FORMAT_YCBCR_422_SP, DMA_PIXEL_FORMAT_YUV_422_SP_HP },
	{ DPU_FB_PIXEL_FORMAT_YCRCB_422_SP, DMA_PIXEL_FORMAT_YUV_422_SP_HP },
	{ DPU_FB_PIXEL_FORMAT_YCBCR_420_SP, DMA_PIXEL_FORMAT_YUV_420_SP_HP },
	{ DPU_FB_PIXEL_FORMAT_YCRCB_420_SP, DMA_PIXEL_FORMAT_YUV_420_SP_HP },
	{ DPU_FB_PIXEL_FORMAT_RGBA_1010102, DMA_PIXEL_FORMAT_RGBA_1010102 },
	{ DPU_FB_PIXEL_FORMAT_BGRA_1010102, DMA_PIXEL_FORMAT_RGBA_1010102 },
	{ DPU_FB_PIXEL_FORMAT_Y410_10BIT, DMA_PIXEL_FORMAT_Y410_10BIT },
	{ DPU_FB_PIXEL_FORMAT_YUV422_10BIT, DMA_PIXEL_FORMAT_YUV422_10BIT },
	{ DPU_FB_PIXEL_FORMAT_YCRCB420_SP_10BIT, DMA_PIXEL_FORMAT_YUV420_SP_10BIT },
	{ DPU_FB_PIXEL_FORMAT_YCBCR420_SP_10BIT, DMA_PIXEL_FORMAT_YUV420_SP_10BIT },
	{ DPU_FB_PIXEL_FORMAT_YCBCR422_SP_10BIT, DMA_PIXEL_FORMAT_YUV422_SP_10BIT },
	{ DPU_FB_PIXEL_FORMAT_YCBCR420_P_10BIT, DMA_PIXEL_FORMAT_YUV420_P_10BIT },
	{ DPU_FB_PIXEL_FORMAT_YCBCR422_P_10BIT, DMA_PIXEL_FORMAT_YUV422_P_10BIT },
};

int dpu_pixel_format_hal2dma(int format)
{
	int i;
	int size = sizeof(dpu_dma_fmt) / sizeof(dpu_dma_fmt[0]);

	for (i = 0; i < size; i++) {
		if ((int)(dpu_dma_fmt[i].fb_pixel_format) == format)
			break;
	}

	if (i >= size) {
		DPU_FB_ERR("not support format-%d!\n", format);
		return -1;
	}

	return dpu_dma_fmt[i].dma_pixel_format;
}
/*lint -e655*/
int dpu_transform_hal2dma(int transform, int chn_idx)
{
	int ret;

	if (chn_idx < DSS_WCHN_W0 || chn_idx == DSS_RCHN_V2) {
		switch (transform) {
		case DPU_FB_TRANSFORM_NOP:
			ret = DSS_TRANSFORM_NOP;
			break;
		case DPU_FB_TRANSFORM_FLIP_H:
			ret = DSS_TRANSFORM_FLIP_H;
			break;
		case DPU_FB_TRANSFORM_FLIP_V:
			ret = DSS_TRANSFORM_FLIP_V;
			break;
		case DPU_FB_TRANSFORM_ROT_180:
			ret = DSS_TRANSFORM_FLIP_V | DSS_TRANSFORM_FLIP_H;
			break;
		case DPU_FB_TRANSFORM_ROT_90:
			ret = DSS_TRANSFORM_ROT | DSS_TRANSFORM_FLIP_H;
			break;
		case DPU_FB_TRANSFORM_ROT_270:
			ret = DSS_TRANSFORM_ROT | DSS_TRANSFORM_FLIP_V;
			break;
		default:
			ret = -1;
			DPU_FB_ERR("Transform %d is not supported\n", transform);
			break;
		}
	} else {
		if (transform == DPU_FB_TRANSFORM_NOP) {
			ret = DSS_TRANSFORM_NOP;
		} else if (transform == (DPU_FB_TRANSFORM_ROT_90 | DPU_FB_TRANSFORM_FLIP_V)) {
			ret = DSS_TRANSFORM_ROT;
		} else {
			ret = -1;
			DPU_FB_ERR("Transform %d is not supported\n", transform);
		}
	}

	return ret;
}

