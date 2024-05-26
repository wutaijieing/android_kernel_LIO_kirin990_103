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
#include <soc_dte_define.h>
#include "dpu_hebce.h"
#include "hisi_disp_debug.h"
#include "hisi_disp_gadgets.h"
#include "hisi_operator_tool.h"
#include "hisi_disp_config.h"

static struct hal_to_hebce_pixel_fmt hebce_fmt[] = {
	{HEBCE_RGB565, HISI_FB_PIXEL_FORMAT_RGB_565},
	{HEBCE_RGBA4444, HISI_FB_PIXEL_FORMAT_RGBA_4444},
	{HEBCE_RGBX4444, HISI_FB_PIXEL_FORMAT_RGBX_4444},
	{HEBCE_RGBA5551, HISI_FB_PIXEL_FORMAT_RGBA_5551},
	{HEBCE_RGBX5551, HISI_FB_PIXEL_FORMAT_RGBX_5551},
	{HEBCE_RGBA8888, HISI_FB_PIXEL_FORMAT_RGBA_8888},
	{HEBCE_RGBX8888, HISI_FB_PIXEL_FORMAT_RGBX_8888},
	{HEBCE_RGBA1010102, HISI_FB_PIXEL_FORMAT_RGBA_1010102},
	{HEBCE_NV12_8BIT, HISI_FB_PIXEL_FORMAT_YCBCR_420_SP},
	{HEBCE_NV12_10BIT, HISI_FB_PIXEL_FORMAT_YCBCR420_SP_10BIT},
	{HEBCE_RGB565_ROT, HISI_FB_PIXEL_FORMAT_RGB_565},
	{HEBCE_RGBA4444_ROT, HISI_FB_PIXEL_FORMAT_RGBA_4444},
	{HEBCE_RGBX4444_ROT, HISI_FB_PIXEL_FORMAT_RGBX_4444},
	{HEBCE_RGBA5551_ROT, HISI_FB_PIXEL_FORMAT_RGBA_5551},
	{HEBCE_RGBX5551_ROT, HISI_FB_PIXEL_FORMAT_RGBX_5551},
	{HEBCE_RGBA8888_ROT, HISI_FB_PIXEL_FORMAT_RGBA_8888},
	{HEBCE_RGBX8888_ROT, HISI_FB_PIXEL_FORMAT_RGBX_8888},
	{HEBCE_RGBA1010102_ROT, HISI_FB_PIXEL_FORMAT_RGBA_1010102},
	{HEBCE_NV12_8BIT_ROT, HISI_FB_PIXEL_FORMAT_YCBCR_420_SP},
	{HEBCE_NV12_10BIT_ROT, HISI_FB_PIXEL_FORMAT_YCBCR420_SP_10BIT}
};

static int dpu_hebce_get_format_type(uint32_t format, uint32_t transform)
{
	disp_pr_info(" ++++ \n");
	int i;
	struct hal_to_hebce_pixel_fmt *p_hebce_fmt;

	for (i = 0; i < HEBCE_TYPE_MAX; ++i) {
		if (transform & DPU_TRANSFORM_ROT)
			p_hebce_fmt = hebce_fmt + HEBCE_TYPE_NOROT_MAX;
		else
			p_hebce_fmt = hebce_fmt;

		if (p_hebce_fmt[i].hal_pixel_format == format) {
			disp_pr_info(" p_hebce_fmt[i].hebce_pixel_format:%d \n", p_hebce_fmt[i].hebce_pixel_format);
			return p_hebce_fmt[i].hebce_pixel_format;
		}
	}
	disp_pr_err("hal input format:%d not support!\n", format);

	return -1;
}

static int dpu_init_hebce_info(struct disp_wb_layer *layer, struct hebce_params *hebce_para)
{
	disp_pr_info(" ++++ \n");
	hebce_para->tag_mode = HEBC_NO_TAG;
	hebce_para->scramble = layer->dst.hebc_scramble_mode;
	hebce_para->yuv_trans = HEBC_YUV_TRANS;
	hebce_para->transform = dpu_transform_hal2dma(layer->transform, layer->wchn_idx);
	hebce_para->wdma_format = dpu_pixel_format_hal2dma(layer->dst.format, DPU_HAL2WDMA_PIXEL_FORMAT);
	if (hebce_para->wdma_format < 0) {
		disp_pr_err("layer format %d not support!\n", layer->dst.format);
		return -EINVAL;
	}
	return 0;
}

enum {
	HEBCE_SBLOCK_RGB_WIDTH_ALIGN_INFO = HEBC_SBLOCK_RGB_WIDTH,
	HEBCE_SBLOCK_RGB_HEIGHT_ALIGN_INFO = HEBC_SBLOCK_RGB_HEIGHT,

	HEBCE_SBLOCK_NV12_WIDTH_ALIGN_INFO = HEBC_SBLOCK_Y8_WIDTH,
	HEBCE_SBLOCK_NV12_HEIGHT_ALIGN_INFO = 2 * HEBC_SBLOCK_Y8_HEIGHT,

	HEBCE_SBLOCK_NV12_10BIT_WIDTH_ALIGN_INFO = HEBC_SBLOCK_Y10_WIDTH,
	HEBCE_SBLOCK_NV12_10BIT_HEIGHT_ALIGN_INFO = 2 * HEBC_SBLOCK_Y10_HEIGHT,

	HEBCE_SBLOCK_RGB_WIDTH_ROT_ALIGN_INFO = HEBC_SBLOCK_RGB_WIDTH_ROT,
	HEBCE_SBLOCK_RGB_HEIGHT_ROT_ALIGN_INFO = HEBC_SBLOCK_RGB_HEIGHT_ROT,

	HEBCE_SBLOCK_NV12_WIDTH_ROT_ALIGN_INFO = HEBC_SBLOCK_Y8_WIDTH_ROT,
	HEBCE_SBLOCK_NV12_HEIGHT_ROT_ALIGN_INFO = HEBC_SBLOCK_Y8_HEIGHT_ROT,

	HEBCE_SBLOCK_NV12_10BIT_WIDTH_ROT_ALIGN_INFO = HEBC_SBLOCK_Y10_WIDTH_ROT,
	HEBCE_SBLOCK_NV12_10BIT_HEIGHT_ROT_ALIGN_INFO = HEBC_SBLOCK_Y10_HEIGHT_ROT,
};

static const struct hebce_rect_align_info g_hebce_rect_align_info[HEBCE_TYPE_MAX] = {
	// HEBCE_RGB565
	{HEBCE_SBLOCK_RGB_WIDTH_ALIGN_INFO, HEBCE_SBLOCK_RGB_HEIGHT_ALIGN_INFO},
	// HEBCE_RGBA4444
	{HEBCE_SBLOCK_RGB_WIDTH_ALIGN_INFO, HEBCE_SBLOCK_RGB_HEIGHT_ALIGN_INFO},
	// HEBCE_RGBX4444
	{HEBCE_SBLOCK_RGB_WIDTH_ALIGN_INFO, HEBCE_SBLOCK_RGB_HEIGHT_ALIGN_INFO},
	// HEBCE_RGBA5551
	{HEBCE_SBLOCK_RGB_WIDTH_ALIGN_INFO, HEBCE_SBLOCK_RGB_HEIGHT_ALIGN_INFO},
	// HEBCE_RGBX5551
	{HEBCE_SBLOCK_RGB_WIDTH_ALIGN_INFO, HEBCE_SBLOCK_RGB_HEIGHT_ALIGN_INFO},
	// HEBCE_RGBA8888
	{HEBCE_SBLOCK_RGB_WIDTH_ALIGN_INFO, HEBCE_SBLOCK_RGB_HEIGHT_ALIGN_INFO},
	// HEBCE_RGBX8888
	{HEBCE_SBLOCK_RGB_WIDTH_ALIGN_INFO, HEBCE_SBLOCK_RGB_HEIGHT_ALIGN_INFO},
	// HEBCE_RGBA1010102
	{HEBCE_SBLOCK_RGB_WIDTH_ALIGN_INFO, HEBCE_SBLOCK_RGB_HEIGHT_ALIGN_INFO},
	// HEBCE_NV12_8BIT
	{HEBCE_SBLOCK_NV12_WIDTH_ALIGN_INFO, HEBCE_SBLOCK_NV12_HEIGHT_ALIGN_INFO},
	// HEBCE_NV12_10BIT
	{HEBCE_SBLOCK_NV12_10BIT_WIDTH_ALIGN_INFO, HEBCE_SBLOCK_NV12_10BIT_HEIGHT_ALIGN_INFO},
	// HEBCE_RGB565_ROT
	{HEBCE_SBLOCK_RGB_WIDTH_ROT_ALIGN_INFO, HEBCE_SBLOCK_RGB_HEIGHT_ROT_ALIGN_INFO},
	// HEBCE_RGBA4444_ROT
	{HEBCE_SBLOCK_RGB_WIDTH_ROT_ALIGN_INFO, HEBCE_SBLOCK_RGB_HEIGHT_ROT_ALIGN_INFO},
	// HEBCE_RGBX4444_ROT
	{HEBCE_SBLOCK_RGB_WIDTH_ROT_ALIGN_INFO, HEBCE_SBLOCK_RGB_HEIGHT_ROT_ALIGN_INFO},
	// HEBCE_RGBA5551_ROT
	{HEBCE_SBLOCK_RGB_WIDTH_ROT_ALIGN_INFO, HEBCE_SBLOCK_RGB_HEIGHT_ROT_ALIGN_INFO},
	// HEBCE_RGBX5551_ROT
	{HEBCE_SBLOCK_RGB_WIDTH_ROT_ALIGN_INFO, HEBCE_SBLOCK_RGB_HEIGHT_ROT_ALIGN_INFO},
	// HEBCE_RGBA8888_ROT
	{HEBCE_SBLOCK_RGB_WIDTH_ROT_ALIGN_INFO, HEBCE_SBLOCK_RGB_HEIGHT_ROT_ALIGN_INFO},
	// HEBCE_RGBX8888_ROT
	{HEBCE_SBLOCK_RGB_WIDTH_ROT_ALIGN_INFO, HEBCE_SBLOCK_RGB_HEIGHT_ROT_ALIGN_INFO},
	// HEBCE_RGBA1010102_ROT
	{HEBCE_SBLOCK_RGB_WIDTH_ROT_ALIGN_INFO, HEBCE_SBLOCK_RGB_HEIGHT_ROT_ALIGN_INFO},
	// HEBCE_NV21_8BIT_ROT
	{HEBCE_SBLOCK_NV12_WIDTH_ROT_ALIGN_INFO, HEBCE_SBLOCK_NV12_HEIGHT_ROT_ALIGN_INFO},
	// HEBCE_NV21_10BIT_ROT
	{HEBCE_SBLOCK_NV12_10BIT_WIDTH_ROT_ALIGN_INFO, HEBCE_SBLOCK_NV12_10BIT_HEIGHT_ROT_ALIGN_INFO}
};

static int dpu_hebce_get_aligned_info(struct hebce_params *hebce_para, struct disp_wb_layer *layer)
{
	disp_pr_info(" ++++ \n");
	uint32_t hebce_type = dpu_hebce_get_format_type(layer->dst.format, hebce_para->transform);
	if ((layer->transform & HISI_FB_TRANSFORM_ROT_90) && (hebce_type >= HEBCE_TYPE_MAX)
		|| (layer->transform == HISI_FB_TRANSFORM_NOP) && (hebce_type >= HEBCE_TYPE_NOROT_MAX)) {
		 disp_pr_err("hebce_type(%u) error!\n", hebce_para->transform);
		 return -1;
	}

	hebce_para->hebce_block_width_align = g_hebce_rect_align_info[hebce_type].width_align_info;
	hebce_para->hebce_block_height_align = g_hebce_rect_align_info[hebce_type].height_align_info;

	return 0;
}

static void dpu_hebce_calc_rect(struct hebce_params *hebce_para, struct disp_wb_layer *layer,
				struct disp_rect *ov_block_rect, struct disp_rect *aligned_rect)
{
	disp_pr_info(" ++++ \n");
	struct dpu_edit_rect hebce_rect = {0, 0, 0, 0};
	uint32_t left_used = 0;
	uint32_t top_used = 0;
	if (layer->transform & HISI_FB_TRANSFORM_ROT_90) {
		hebce_rect.left = ALIGN_DOWN(layer->dst_rect.x, hebce_para->hebce_block_width_align);
		hebce_rect.top = ALIGN_DOWN(layer->dst_rect.y +
			(ov_block_rect->x - layer->dst_rect.x), hebce_para->hebce_block_height_align);
		left_used = hebce_rect.left / hebce_para->hebce_block_height_align;
		top_used = hebce_rect.top / hebce_para->hebce_block_width_align;
	} else {
		hebce_rect.left = ALIGN_DOWN(aligned_rect->x, hebce_para->hebce_block_width_align);
		hebce_rect.top = ALIGN_DOWN(aligned_rect->y, hebce_para->hebce_block_height_align);
		left_used = hebce_rect.left / hebce_para->hebce_block_width_align;
		top_used = hebce_rect.top / hebce_para->hebce_block_height_align;
	}

	hebce_para->hebce_planes[0].header_stride = layer->dst.hebc_planes[0].header_stride;
	hebce_para->hebce_planes[0].header_offset = top_used * hebce_para->hebce_planes[0].header_stride +
												left_used * (uint64_t)HEBC_HEADER_SBLOCK_SIZE;
	hebce_para->hebce_planes[0].header_addr = layer->dst.hebc_planes[0].header_addr +
			hebce_para->hebce_planes[0].header_offset;

	hebce_para->hebce_planes[0].payload_stride = layer->dst.hebc_planes[0].payload_stride;
	uint32_t super_block_size;
	if (hebce_para->wdma_format == DPU_WDMA_PIXEL_FORMAT_RGB_565) {
		super_block_size = HEBC_PLD_SBLOCK_SIZE_RGB565;
	} else {
		super_block_size = HEBC_PLD_SBLOCK_SIZE;
	}
	hebce_para->hebce_planes[0].payload_offset = top_used * hebce_para->hebce_planes[0].payload_stride +
												left_used * (uint64_t)super_block_size;

	hebce_para->hebce_planes[0].payload_addr = layer->dst.hebc_planes[0].payload_addr +
		hebce_para->hebce_planes[0].payload_offset;

	if (is_yuv_semiplanar(layer->dst.format)) {
		disp_pr_info(" yuv420 \n");
		hebce_para->hebce_planes[1].header_stride = layer->dst.hebc_planes[1].header_stride;
		hebce_para->hebce_planes[1].header_offset = top_used / 2 * hebce_para->hebce_planes[1].header_stride +
													left_used * (uint64_t)HEBC_HEADER_SBLOCK_SIZE;

		hebce_para->hebce_planes[1].header_addr = layer->dst.hebc_planes[1].header_addr +
				hebce_para->hebce_planes[1].header_offset;

		hebce_para->hebce_planes[1].payload_stride = layer->dst.hebc_planes[1].payload_stride;
		hebce_para->hebce_planes[1].payload_offset = top_used / 2 * hebce_para->hebce_planes[1].payload_stride +
													left_used * (uint64_t)super_block_size;

		hebce_para->hebce_planes[1].payload_addr = layer->dst.hebc_planes[1].payload_addr +
			hebce_para->hebce_planes[1].payload_offset;
		hebce_para->hebce_block_height_align /= 2;
	}

	disp_pr_info(" hebce_rect.left:%u hebce_rect.top:%u \n", hebce_rect.left, hebce_rect.top);
	disp_pr_info(" header_addr[0]:0x%x payload_addr[0]:0x%x \n", hebce_para->hebce_planes[0].header_addr,
				hebce_para->hebce_planes[0].payload_addr);
	disp_pr_info(" header_addr[1]:0x%x payload_addr[1]:0x%x \n", hebce_para->hebce_planes[1].header_addr,
				hebce_para->hebce_planes[1].payload_addr);
	disp_pr_info(" header_stride[0]:0x%x payload_stride[0]:0x%x \n", hebce_para->hebce_planes[0].header_stride,
				hebce_para->hebce_planes[0].payload_stride);
	disp_pr_info(" header_stride[1]:0x%x payload_stride[1]:0x%x \n", hebce_para->hebce_planes[1].header_stride,
				hebce_para->hebce_planes[1].payload_stride);
}

static int dpu_hebce_calc(struct hebce_params *hebce_para, struct disp_wb_layer *layer, struct disp_rect aligned_rect,
				struct disp_rect *ov_block_rect)
{
	disp_pr_info(" ++++ \n");
	if (dpu_init_hebce_info(layer, hebce_para))
		return -EINVAL;

	if (dpu_hebce_get_aligned_info(hebce_para, layer))
		return -EINVAL;

	dpu_hebce_calc_rect(hebce_para, layer, ov_block_rect, &aligned_rect);

	disp_pr_info(" ---- \n");
	return 0;
}

static const struct hebce_rect_boundary g_hebce_rect_boundary[HEBCE_TYPE_MAX] = {
	// HEBCE_RGB565
	{HEBCE_WIDTH_RGB_MIN, HEBCE_HEIGHT_RGB_MIN, HEBCE_WIDTH_MAX_1, HEBCE_HEIGHT_MAX},
	// HEBCE_RGBA4444
	{HEBCE_WIDTH_RGB_MIN, HEBCE_HEIGHT_RGB_MIN, HEBCE_WIDTH_MAX_1, HEBCE_HEIGHT_MAX},
	// HEBCE_RGBX4444
	{HEBCE_WIDTH_RGB_MIN, HEBCE_HEIGHT_RGB_MIN, HEBCE_WIDTH_MAX_1, HEBCE_HEIGHT_MAX},
	// HEBCE_RGBA5551
	{HEBCE_WIDTH_RGB_MIN, HEBCE_HEIGHT_RGB_MIN, HEBCE_WIDTH_MAX_1, HEBCE_HEIGHT_MAX},
	// HEBCE_RGBX5551
	{HEBCE_WIDTH_RGB_MIN, HEBCE_HEIGHT_RGB_MIN, HEBCE_WIDTH_MAX_1, HEBCE_HEIGHT_MAX},
	// HEBCE_RGBA8888
	{HEBCE_WIDTH_RGB_MIN, HEBCE_HEIGHT_RGB_MIN, HEBCE_WIDTH_MAX_2, HEBCE_HEIGHT_MAX},
	// HEBCE_RGBX8888
	{HEBCE_WIDTH_RGB_MIN, HEBCE_HEIGHT_RGB_MIN, HEBCE_WIDTH_MAX_2, HEBCE_HEIGHT_MAX},
	// HEBCE_RGBA1010102
	{HEBCE_WIDTH_RGB_MIN, HEBCE_HEIGHT_RGB_MIN, HEBCE_WIDTH_MAX_2, HEBCE_HEIGHT_MAX},
	// HEBCE_NV12_8BIT
	{HEBCE_WIDTH_NV12_MIN, HEBCE_HEIGHT_NV12_MIN, HEBCE_WIDTH_MAX_1, HEBCE_HEIGHT_MAX},
	// HEBCE_NV12_10BIT
	{HEBCE_WIDTH_NV12_10BIT_MIN, HEBCE_HEIGHT_NV12_10BIT_MIN, HEBCE_WIDTH_MAX_1, HEBCE_HEIGHT_MAX},
	// HEBCE_RGB565_ROT
	{HEBCE_WIDTH_RGB_ROT_MIN, HEBCE_HEIGHT_RGB_ROT_MIN, HEBCE_WIDTH_MAX_ROT, HEBCE_HEIGHT_MAX_ROT},
	// HEBCE_RGBA4444_ROT
	{HEBCE_WIDTH_RGB_ROT_MIN, HEBCE_HEIGHT_RGB_ROT_MIN, HEBCE_WIDTH_MAX_ROT, HEBCE_HEIGHT_MAX_ROT},
	// HEBCE_RGBX4444_ROT
	{HEBCE_WIDTH_RGB_ROT_MIN, HEBCE_HEIGHT_RGB_ROT_MIN, HEBCE_WIDTH_MAX_ROT, HEBCE_HEIGHT_MAX_ROT},
	// HEBCE_RGBA5551_ROT
	{HEBCE_WIDTH_RGB_ROT_MIN, HEBCE_HEIGHT_RGB_ROT_MIN, HEBCE_WIDTH_MAX_ROT, HEBCE_HEIGHT_MAX_ROT},
	// HEBCE_RGBX5551_ROT
	{HEBCE_WIDTH_RGB_ROT_MIN, HEBCE_HEIGHT_RGB_ROT_MIN, HEBCE_WIDTH_MAX_ROT, HEBCE_HEIGHT_MAX_ROT},
	// HEBCE_RGBA8888_ROT
	{HEBCE_WIDTH_RGB_ROT_MIN, HEBCE_HEIGHT_RGB_ROT_MIN, HEBCE_WIDTH_MAX_ROT, HEBCE_HEIGHT_MAX_ROT},
	// HEBCE_RGBX8888_ROT
	{HEBCE_WIDTH_RGB_ROT_MIN, HEBCE_HEIGHT_RGB_ROT_MIN, HEBCE_WIDTH_MAX_ROT, HEBCE_HEIGHT_MAX_ROT},
	// HEBCE_RGBA1010102_ROT
	{HEBCE_WIDTH_RGB_ROT_MIN, HEBCE_HEIGHT_RGB_ROT_MIN, HEBCE_WIDTH_MAX_ROT, HEBCE_HEIGHT_MAX_ROT},
	// HEBCE_NV12_8BIT_ROT
	{HEBCE_WIDTH_NV12_ROT_MIN, HEBCE_HEIGHT_NV12_ROT_MIN,	HEBCE_WIDTH_MAX_ROT, HEBCE_HEIGHT_MAX_ROT},
	// HEBCE_NV12_10BIT_ROT
	{HEBCE_WIDTH_NV12_10BIT_ROT_MIN, HEBCE_HEIGHT_NV12_10BIT_ROT_MIN, HEBCE_WIDTH_MAX_ROT, HEBCE_HEIGHT_MAX_ROT},
};

static int dpu_hebce_check_rect_boundry(struct hebce_params *hebce_para, struct disp_rect *aligned_rect,
				struct disp_wb_layer *layer)
{
	disp_pr_info(" ++++ \n");
	uint32_t hebce_type = dpu_hebce_get_format_type(layer->dst.format, hebce_para->transform);
	if (aligned_rect->w < g_hebce_rect_boundary[hebce_type].width_min ||
		aligned_rect->w > g_hebce_rect_boundary[hebce_type].width_max) {
		disp_pr_err("aligned_rect width[x:%u y:%u w:%u h:%u] out of boundry!\n",
			aligned_rect->x, aligned_rect->y, aligned_rect->w, aligned_rect->h);
		return -1;
	}

	if (aligned_rect->h < g_hebce_rect_boundary[hebce_type].height_mix ||
		aligned_rect->h > g_hebce_rect_boundary[hebce_type].height_max) {
		disp_pr_err("aligned_rect height[x:%u y:%u w:%u h:%u] out of boundry!\n",
			aligned_rect->x, aligned_rect->y, aligned_rect->w, aligned_rect->h);
		return -1;
	}

	return 0;
}

static int dpu_hebce_check_rect_align_info(struct hebce_params *hebce_para, struct disp_rect *aligned_rect,
				struct disp_wb_layer *layer)
{
	disp_pr_info(" ++++ \n");
	uint32_t hebce_type = dpu_hebce_get_format_type(layer->dst.format, hebce_para->transform);
	if ((aligned_rect->w % g_hebce_rect_align_info[hebce_type].width_align_info) ||
		(aligned_rect->h % g_hebce_rect_align_info[hebce_type].height_align_info)) {
		disp_pr_err("aligned_rect[w:%u h:%u] is not aligned!\n", aligned_rect->w, aligned_rect->h);
		return -1;
	}

	return 0;
}

static const struct hebce_align_info g_hebce_align_info[HEBCE_TYPE_MAX] = {
	// HEBCE_RGB565
	{HEBC_HEADER_ADDR_ALIGN, HEBC_HEADER_STRIDE_ALIGN, BPP_RGB565 * HEBC_SBLOCK_RGB_WIDTH * HEBC_SBLOCK_RGB_HEIGHT,
		BPP_RGB565 * HEBC_SBLOCK_RGB_WIDTH * HEBC_SBLOCK_RGB_HEIGHT},
	// HEBCE_RGBA4444
	{HEBC_HEADER_ADDR_ALIGN, HEBC_HEADER_STRIDE_ALIGN, BPP_RGBA4444 * HEBC_SBLOCK_RGB_WIDTH * HEBC_SBLOCK_RGB_HEIGHT,
		BPP_RGBA4444 * HEBC_SBLOCK_RGB_WIDTH * HEBC_SBLOCK_RGB_HEIGHT},
	// HEBCE_RGBX4444
	{HEBC_HEADER_ADDR_ALIGN, HEBC_HEADER_STRIDE_ALIGN, BPP_RGBX4444 * HEBC_SBLOCK_RGB_WIDTH * HEBC_SBLOCK_RGB_HEIGHT,
		BPP_RGBX4444 * HEBC_SBLOCK_RGB_WIDTH * HEBC_SBLOCK_RGB_HEIGHT},
	// HEBCE_RGBA5551
	{HEBC_HEADER_ADDR_ALIGN, HEBC_HEADER_STRIDE_ALIGN, BPP_RGBA5551 * HEBC_SBLOCK_RGB_WIDTH * HEBC_SBLOCK_RGB_HEIGHT,
		BPP_RGBA5551 * HEBC_SBLOCK_RGB_WIDTH * HEBC_SBLOCK_RGB_HEIGHT},
	// HEBCE_RGBX5551
	{HEBC_HEADER_ADDR_ALIGN, HEBC_HEADER_STRIDE_ALIGN, BPP_RGBX5551 * HEBC_SBLOCK_RGB_WIDTH * HEBC_SBLOCK_RGB_HEIGHT,
		BPP_RGBX5551 * HEBC_SBLOCK_RGB_WIDTH * HEBC_SBLOCK_RGB_HEIGHT},
	// HEBCE_RGBA8888
	{HEBC_HEADER_ADDR_ALIGN, HEBC_HEADER_STRIDE_ALIGN, BPP_RGBA8888 * HEBC_SBLOCK_RGB_WIDTH * HEBC_SBLOCK_RGB_HEIGHT,
		BPP_RGBA8888 * HEBC_SBLOCK_RGB_WIDTH * HEBC_SBLOCK_RGB_HEIGHT},
	// HEBCE_RGBX8888
	{HEBC_HEADER_ADDR_ALIGN, HEBC_HEADER_STRIDE_ALIGN, BPP_RGBX8888 * HEBC_SBLOCK_RGB_WIDTH * HEBC_SBLOCK_RGB_HEIGHT,
		BPP_RGBX8888 * HEBC_SBLOCK_RGB_WIDTH * HEBC_SBLOCK_RGB_HEIGHT},
	// HEBCE_RGBA1010102
	{HEBC_HEADER_ADDR_ALIGN, HEBC_HEADER_STRIDE_ALIGN, BPP_RGBA1010102 * HEBC_SBLOCK_RGB_WIDTH *
		HEBC_SBLOCK_RGB_HEIGHT, BPP_RGBA1010102 * HEBC_SBLOCK_RGB_WIDTH * HEBC_SBLOCK_RGB_HEIGHT},
	// HEBCE_NV12_8BIT
	{HEBC_HEADER_ADDR_ALIGN, HEBC_HEADER_STRIDE_ALIGN, BPP_NV12_8BIT * HEBC_SBLOCK_Y8_WIDTH * HEBC_SBLOCK_Y8_HEIGHT,
		BPP_NV12_8BIT * HEBC_SBLOCK_Y8_WIDTH * HEBC_SBLOCK_Y8_HEIGHT},
	// HEBCE_NV12_10BIT
	{HEBC_HEADER_ADDR_ALIGN, HEBC_HEADER_STRIDE_ALIGN, BPP_NV12_10BIT * HEBC_SBLOCK_Y10_WIDTH * HEBC_SBLOCK_Y10_HEIGHT,
		BPP_NV12_10BIT * HEBC_SBLOCK_Y10_WIDTH * HEBC_SBLOCK_Y10_HEIGHT}
};

static int dpu_hebce_check_aligned_para(struct hebce_params *hebce_para, struct disp_wb_layer *layer)
{
	disp_pr_info(" ++++ \n");
	uint32_t hebc_hdr_addr_align;
	uint32_t hebc_hdr_stride_align;
	uint32_t hebc_pld_addr_align;
	uint32_t hebc_pld_stride_align;
	uint32_t hebce_type = dpu_hebce_get_format_type(layer->dst.format, 0);
	if ((hebce_para->transform == HISI_FB_TRANSFORM_NOP) && (hebce_type >= HEBCE_TYPE_MAX)) {
		 disp_pr_err("hebce_type(%u) error!\n", hebce_para->transform);
		 return -1;
	}

	hebc_hdr_addr_align = g_hebce_align_info[hebce_type].hebce_header_addr_align;
	hebc_hdr_stride_align = g_hebce_align_info[hebce_type].hebce_header_stride_align;
	hebc_pld_addr_align = g_hebce_align_info[hebce_type].hebce_pld_addr_align;
	hebc_pld_stride_align = g_hebce_align_info[hebce_type].hebce_pld_stride_align;

	disp_check_and_return((hebce_para->hebce_planes[0].header_addr % hebc_hdr_addr_align), -EINVAL, err,
				"hebce_header_addr0 and %u not aligned!\n", hebc_hdr_addr_align);
	disp_check_and_return((hebce_para->hebce_planes[0].header_stride % hebc_hdr_stride_align), -EINVAL, err,
				"hebce_header_stride0 and %u not aligned!\n", hebc_hdr_stride_align);
	disp_check_and_return((hebce_para->hebce_planes[0].payload_addr % hebc_pld_addr_align), -EINVAL, err,
				"hebce_payload_addr0 and %u not aligned!\n", hebc_pld_addr_align);
	disp_check_and_return((hebce_para->hebce_planes[0].payload_stride % hebc_pld_stride_align), -EINVAL, err,
				"hebce_payload_stride0 and %u not aligned!\n", hebc_pld_stride_align);
	if (is_yuv_semiplanar(layer->dst.format)) {
		disp_check_and_return((hebce_para->hebce_planes[1].header_addr % hebc_hdr_addr_align), -EINVAL, err,
				"hebce_header_addr1 and %u not aligned!\n", hebc_hdr_addr_align);
		disp_check_and_return((hebce_para->hebce_planes[1].header_stride % hebc_hdr_stride_align), -EINVAL, err,
				"hebce_header_stride1 and %u not aligned!\n", hebc_hdr_stride_align);
		disp_check_and_return((hebce_para->hebce_planes[1].payload_addr % hebc_pld_addr_align), -EINVAL, err,
				"hebce_payload_addr1 and %u not aligned!\n", hebc_pld_addr_align);
		disp_check_and_return((hebce_para->hebce_planes[1].payload_stride % hebc_pld_stride_align), -EINVAL, err,
				"hebce_payload_stride1 and %u not aligned!\n", hebc_pld_stride_align);
	}

	return 0;
}

static int dpu_hebce_check_maximum(struct hebce_params *hebce_para, struct disp_wb_layer *layer)
{
	disp_pr_info(" ++++ \n");

	disp_check_and_return((hebce_para->hebce_planes[0].header_stride > HEBC_RGB_HEADER_STRIDE_MAXIMUM), -EINVAL, err,
				"hebcd_header_stride0 can not exceed %u!\n", HEBC_RGB_HEADER_STRIDE_MAXIMUM);
	disp_check_and_return((hebce_para->hebce_planes[0].payload_stride > HEBC_RGB_PLD_STRIDE_MAXIMUM), -EINVAL, err,
				"hebcd_payload_stride0 can not exceed %u!\n", HEBC_RGB_PLD_STRIDE_MAXIMUM);
	if (is_yuv_semiplanar(layer->dst.format)) {
		disp_check_and_return((hebce_para->hebce_planes[1].header_stride > HEBC_C_HEADER_STRIDE_MAXIMUM), -EINVAL, err,
				"hebcd_header_stride1 can not exceed %u!\n", HEBC_C_HEADER_STRIDE_MAXIMUM);
		disp_check_and_return((hebce_para->hebce_planes[1].payload_stride > HEBC_C_PLD_STRIDE_MAXIMUM), -EINVAL, err,
				"hebcd_payload_stride1 can not exceed %u!\n", HEBC_C_PLD_STRIDE_MAXIMUM);
	}

	return 0;
}

static int dpu_hebce_check(struct hebce_params *hebce_para, struct disp_wb_layer *layer, struct disp_rect aligned_rect)
{
	disp_pr_info(" ++++ \n");

	if (dpu_hebce_check_rect_boundry(hebce_para, &aligned_rect, layer))
		return -EINVAL;

	if (dpu_hebce_check_rect_align_info(hebce_para, &aligned_rect, layer))
		return -EINVAL;

	if (dpu_hebce_check_aligned_para(hebce_para, layer))
		return -EINVAL;

	if (dpu_hebce_check_maximum(hebce_para, layer))
		return -EINVAL;

	disp_pr_info(" ---- \n");
	return 0;
}

static void dpu_hebce_set_info(struct hebce_params *hebce_para, struct disp_wb_layer *layer, struct dpu_wch_wdma *wdma,
									struct disp_rect aligned_rect)
{
	disp_pr_info(" ++++ \n");

	wdma->dma_cmp_ctrl.value = 0;
	wdma->dma_cmp_ctrl.reg.dma_cmp_yuv_trans = 0;
	wdma->dma_cmp_ctrl.reg.dma_cmp_tag_en = hebce_para->tag_mode;
	wdma->dma_cmp_ctrl.reg.dma_cmp_spblk_layout = 1;
	wdma->dma_cmp_ctrl.reg.dma_cmp_spblk_layout_switch_en = 1;
	wdma->dma_cmp_ctrl.reg.dma_cmp_scramble_mode = hebce_para->scramble;

	disp_pr_info("scramble:%u \n", wdma->dma_cmp_ctrl.reg.dma_cmp_scramble_mode);
	wdma->dma_ptr0.value = hebce_para->hebce_planes[0].header_addr & 0xFFFFFFFF;
	wdma->dma_ptr1.value = hebce_para->hebce_planes[0].payload_addr & 0xFFFFFFFF;
	wdma->dma_ptr2.value = hebce_para->hebce_planes[1].header_addr & 0xFFFFFFFF;
	wdma->dma_ptr3.value = hebce_para->hebce_planes[1].payload_addr & 0xFFFFFFFF;

	wdma->dma_stride0.value = hebce_para->hebce_planes[0].header_stride;
	wdma->dma_stride1.value = hebce_para->hebce_planes[0].payload_stride;
	wdma->dma_stride2.value = hebce_para->hebce_planes[1].header_stride;
	wdma->dma_stride3.value = hebce_para->hebce_planes[1].payload_stride;

	wdma->rot_size.reg.rot_img_height = aligned_rect.h - 1;
	wdma->rot_size.reg.rot_img_width = aligned_rect.w - 1;

	disp_pr_info(" header_addr[0]:0x%x payload_addr[0]:0x%x \n", hebce_para->hebce_planes[0].header_addr,
				hebce_para->hebce_planes[0].payload_addr);
	disp_pr_info(" header_addr[1]:0x%x payload_addr[1]:0x%x \n", hebce_para->hebce_planes[1].header_addr,
				hebce_para->hebce_planes[1].payload_addr);

	disp_pr_info(" header_stride[0]:0x%x payload_stride[0]:0x%x \n", hebce_para->hebce_planes[0].header_stride,
				hebce_para->hebce_planes[0].payload_stride);
	disp_pr_info(" header_stride[1]:0x%x payload_stride[1]:0x%x \n", hebce_para->hebce_planes[1].header_stride,
				hebce_para->hebce_planes[1].payload_stride);
	disp_pr_info(" ---- \n");
}

int dpu_hebce_config(struct disp_wb_layer *layer, struct disp_rect aligned_rect, struct dpu_wch_wdma *wdma,
				struct disp_rect *ov_block_rect)
{
	disp_pr_info(" ++++ \n");
	struct hebce_params hebce_para = {0};

	if (dpu_hebce_calc(&hebce_para, layer, aligned_rect, ov_block_rect)) {
		disp_pr_err("dpu_hebce_calc error\n");
		return -EINVAL;
	}

	if (dpu_hebce_check(&hebce_para, layer, aligned_rect)) {
		disp_pr_err("dpu_hebce_check error\n");
		return -EINVAL;
	}

	dpu_hebce_set_info(&hebce_para, layer, wdma, aligned_rect);

	return 0;
}

/*
hebc width x height align:
yuv:64x16
rgb:32x4
if have rot then
yuv:16x64
rgb:4x32
*/
void dpu_hebc_get_basic_align_info(uint32_t is_yuv_semi_planar, uint32_t is_super_block_rot, uint32_t format,
									uint32_t *width_align, uint32_t *height_align)
{
	if (is_yuv_semi_planar) {
		if (format == DPU_SDMA_PIXEL_FORMAT_YUYV_420_10BIT_SP) {
			if (is_super_block_rot == SUPER_BLOCK_TYPE_NORMAL) {
				/* 32x16 */
			 	*width_align = HEBCE_SBLOCK_NV12_10BIT_WIDTH_ALIGN_INFO;
				*height_align = HEBCE_SBLOCK_NV12_10BIT_HEIGHT_ALIGN_INFO;
			} else {
				/* 16x32 */
				*width_align = HEBCE_SBLOCK_NV12_10BIT_WIDTH_ROT_ALIGN_INFO;
		 		*height_align = HEBCE_SBLOCK_NV12_10BIT_HEIGHT_ROT_ALIGN_INFO;
			}
		} else if (format == DPU_SDMA_PIXEL_FORMAT_YUYV_420_8BIT_SP) {
			if (is_super_block_rot == SUPER_BLOCK_TYPE_NORMAL) {
				/* 64x16 */
				*width_align = HEBCE_SBLOCK_NV12_WIDTH_ALIGN_INFO;
				*height_align = HEBCE_SBLOCK_NV12_HEIGHT_ALIGN_INFO;
			} else {
				/* 16x64 */
				*width_align = HEBCE_SBLOCK_NV12_WIDTH_ROT_ALIGN_INFO;
				*height_align = HEBCE_SBLOCK_NV12_HEIGHT_ROT_ALIGN_INFO;
			}
		} else {
		}
	} else {
		if (is_super_block_rot == SUPER_BLOCK_TYPE_NORMAL) {
			/* 32*4 */
			*width_align = HEBCE_SBLOCK_RGB_WIDTH_ALIGN_INFO;
			*height_align = HEBCE_SBLOCK_RGB_HEIGHT_ALIGN_INFO;
		} else {
			/* 8x16 */
			*width_align = HEBCE_SBLOCK_RGB_WIDTH_ROT_ALIGN_INFO;
			*height_align = HEBCE_SBLOCK_RGB_HEIGHT_ROT_ALIGN_INFO;
		}
	}
}
