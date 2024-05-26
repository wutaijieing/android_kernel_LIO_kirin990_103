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
#include "../../dpu_mmbuf_manager.h"

static int dpu_hfbcd_mmbuf_calc(struct dpu_fb_data_type *dpufd, dss_layer_t *layer,
	bool is_pixel_10bit, int chn_idx, uint32_t mmbuf_line_num)
{
	struct dss_mmbuf_info *mm_info = dpufd->mmbuf_info;
	int32_t rect_left;
	int32_t rect_right;

	if (is_pixel_10bit) {
		if (layer->transform & DPU_FB_TRANSFORM_ROT_90) {
			rect_left = ALIGN_DOWN((uint32_t)layer->src_rect.y, MMBUF_ADDR_ALIGN);
			rect_right = ALIGN_UP(ALIGN_UP((uint32_t)(layer->src_rect.y - rect_left + layer->src_rect.h),
				MMBUF_ADDR_ALIGN) / 4, MMBUF_ADDR_ALIGN);
		} else {
			rect_left = ALIGN_DOWN((uint32_t)layer->src_rect.x, MMBUF_ADDR_ALIGN);
			rect_right = ALIGN_UP(ALIGN_UP((uint32_t)(layer->src_rect.x - rect_left + layer->src_rect.w),
				MMBUF_ADDR_ALIGN) / 4, MMBUF_ADDR_ALIGN);
		}

		mm_info->mm_size2_y2[chn_idx] = rect_right * mmbuf_line_num;
		mm_info->mm_size3_c2[chn_idx] = mm_info->mm_size2_y2[chn_idx] / 2;
	}

	mm_info->mm_size[chn_idx] = mm_info->mm_size0_y8[chn_idx] + mm_info->mm_size1_c8[chn_idx] +
		mm_info->mm_size2_y2[chn_idx] + mm_info->mm_size3_c2[chn_idx];

	mm_info->mm_base[chn_idx] = dpu_mmbuf_alloc(g_mmbuf_gen_pool, mm_info->mm_size[chn_idx]);
	mm_info->mm_base0_y8[chn_idx] = mm_info->mm_base[chn_idx];
	mm_info->mm_base1_c8[chn_idx] = mm_info->mm_base0_y8[chn_idx] + mm_info->mm_size0_y8[chn_idx];
	mm_info->mm_base2_y2[chn_idx] = mm_info->mm_base1_c8[chn_idx] + mm_info->mm_size1_c8[chn_idx];
	mm_info->mm_base3_c2[chn_idx] = mm_info->mm_base2_y2[chn_idx] + mm_info->mm_size2_y2[chn_idx];

	if ((mm_info->mm_base0_y8[chn_idx] < MMBUF_BASE) || (mm_info->mm_base1_c8[chn_idx] < MMBUF_BASE)) {
		DPU_FB_ERR("fb%d, chn%d failed to alloc mmbuf, mm_base0_y8=0x%x, mm_base1_c8=0x%x.\n",
			dpufd->index, chn_idx, mm_info->mm_base0_y8[chn_idx], mm_info->mm_base1_c8[chn_idx]);
			return -EINVAL;
	}

	if (is_pixel_10bit) {
		if ((mm_info->mm_base2_y2[chn_idx] < MMBUF_BASE) || (mm_info->mm_base3_c2[chn_idx] < MMBUF_BASE)) {
			DPU_FB_ERR("fb%d, chn%d failed to alloc mmbuf, mm_base2_y2=0x%x, mm_base3_c2=0x%x.\n",
				dpufd->index, chn_idx, mm_info->mm_base2_y2[chn_idx], mm_info->mm_base3_c2[chn_idx]);
			return -EINVAL;
		}
	}

	return 0;
}

static int dpu_hfbcd_mmbuf_config(struct dpu_fb_data_type *dpufd, int ovl_idx,
	dss_layer_t *layer, uint32_t hfbcd_block_type, bool is_pixel_10bit)
{
	int chn_idx;
	dss_rect_t src_rect;
	bool mm_alloc_needed = false;

	dss_rect_ltrb_t hfbcd_rect;
	uint32_t mmbuf_line_num = 0;

	chn_idx = layer->chn_idx;
	src_rect = layer->src_rect;

	if (ovl_idx <= DSS_OVL1) {
		mm_alloc_needed = true;
	} else {
		if (dpufd->mmbuf_info->mm_used[chn_idx] == 1)
			mm_alloc_needed = false;
		else
			mm_alloc_needed = true;
	}

	if (mm_alloc_needed) {
		hfbcd_rect.left = ALIGN_DOWN((uint32_t)src_rect.x, MMBUF_ADDR_ALIGN);
		hfbcd_rect.right = ALIGN_UP((uint32_t)(src_rect.x - hfbcd_rect.left + src_rect.w), MMBUF_ADDR_ALIGN);

		if (hfbcd_block_type == 0) {
			if (layer->transform & DPU_FB_TRANSFORM_ROT_90) {
				mmbuf_line_num = MMBUF_BLOCK0_ROT_LINE_NUM;
				hfbcd_rect.left = ALIGN_DOWN((uint32_t)src_rect.y, MMBUF_ADDR_ALIGN);
				hfbcd_rect.right = ALIGN_UP((uint32_t)(src_rect.y - hfbcd_rect.left + src_rect.h),
					MMBUF_ADDR_ALIGN);
			} else {
				mmbuf_line_num = MMBUF_BLOCK0_LINE_NUM;
			}
		} else if (hfbcd_block_type == 1) {
			mmbuf_line_num = MMBUF_BLOCK1_LINE_NUM;
		} else {
			DPU_FB_ERR("hfbcd_block_type=%d no support!\n", layer->img.hfbcd_block_type);
			return -EINVAL;
		}

		dpufd->mmbuf_info->mm_size0_y8[chn_idx] = hfbcd_rect.right * mmbuf_line_num;
		dpufd->mmbuf_info->mm_size1_c8[chn_idx] = dpufd->mmbuf_info->mm_size0_y8[chn_idx] / 2;

		if (dpu_hfbcd_mmbuf_calc(dpufd, layer, is_pixel_10bit, chn_idx, mmbuf_line_num) != 0)
			return -EINVAL;
	}

	dpufd->mmbuf_info->mm_used[chn_idx] = 1;

	return 0;
}

static int dpu_hfbcd_config_check_mm_base0(
	struct dpu_fb_data_type *dpufd, uint32_t mm_base0_y8,
	uint32_t mm_base1_c8, int chn_idx, int32_t layer_idx)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL, return!\n");
		return -EINVAL;
	}

	if ((mm_base0_y8 & (MMBUF_ADDR_ALIGN - 1)) ||
		(dpufd->mmbuf_info->mm_size0_y8[chn_idx] & (MMBUF_ADDR_ALIGN - 1)) ||
		(mm_base1_c8 & (MMBUF_ADDR_ALIGN - 1)) ||
		(dpufd->mmbuf_info->mm_size1_c8[chn_idx] & (MMBUF_ADDR_ALIGN - 1))) {
		DPU_FB_ERR("layer%d mm_base0_y8 0x%x or mm_size0_y8 0x%x or "
			"mm_base1_c8 0x%x or mm_size1_c8 0x%x is not %d bytes aligned!\n",
			layer_idx, mm_base0_y8, dpufd->mmbuf_info->mm_size0_y8[chn_idx],
			mm_base1_c8, dpufd->mmbuf_info->mm_size1_c8[chn_idx],
			MMBUF_ADDR_ALIGN);
		return -EINVAL;
	}

	return 0;
}

static int dpu_hfbcd_config_check_mm_base2(
	struct dpu_fb_data_type *dpufd, uint32_t mm_base2_y2,
	uint32_t mm_base3_c2, int chn_idx, int32_t layer_idx)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL, return!\n");
		return -EINVAL;
	}

	if ((mm_base2_y2 & (MMBUF_ADDR_ALIGN - 1)) ||
		(dpufd->mmbuf_info->mm_size2_y2[chn_idx] & (MMBUF_ADDR_ALIGN - 1)) ||
		(mm_base3_c2 & (MMBUF_ADDR_ALIGN - 1)) ||
		(dpufd->mmbuf_info->mm_size3_c2[chn_idx] & (MMBUF_ADDR_ALIGN - 1))) {
		DPU_FB_ERR("layer%d mm_base2_y2 0x%x or mm_size2_y2 0x%x or "
			"mm_base3_c2 0x%x or mm_size3_c2 0x%x is not %d bytes aligned!\n",
			layer_idx, mm_base2_y2, dpufd->mmbuf_info->mm_size2_y2[chn_idx],
			mm_base3_c2, dpufd->mmbuf_info->mm_size3_c2[chn_idx],
			MMBUF_ADDR_ALIGN);
		return -EINVAL;
	}

	return 0;
}

static int dpu_hfbcd_calc_base_para(dss_layer_t *layer, struct dss_hfbcd_para *hfbcd_para)
{
	int chn_idx = layer->chn_idx;

	hfbcd_para->mmu_enable = (layer->img.mmu_enable == 1) ? true : false;
	hfbcd_para->is_yuv_semi_planar = is_yuv_semiplanar(layer->img.format);
	hfbcd_para->is_yuv_planar = is_yuv_plane(layer->img.format);

	hfbcd_para->rdma_format = dpu_pixel_format_hal2dma(layer->img.format);
	if (hfbcd_para->rdma_format < 0) {
		DPU_FB_ERR("layer format %d not support!\n", layer->img.format);
		return -EINVAL;
	}

	hfbcd_para->rdma_transform = dpu_transform_hal2dma(layer->transform, chn_idx);
	if (hfbcd_para->rdma_transform < 0) {
		DPU_FB_ERR("layer transform %d not support!\n", layer->transform);
		return -EINVAL;
	}

	hfbcd_para->hfbcd_block_type = layer->img.hfbcd_block_type;

	return 0;
}

static int dpu_hfbcd_using_base_para(struct dpu_fb_data_type *dpufd, int ovl_idx, dss_layer_t *layer,
	struct dss_hfbcd_para *hfbcd_para)
{
	bool is_pixel_10bit = false;
	int ret;
	int chn_idx;

	chn_idx = layer->chn_idx;
	is_pixel_10bit = is_pixel_10bit2dma(hfbcd_para->rdma_format);
	if ((layer->img.hfbc_mmbuf_base0_y8 > 0) &&
		(layer->img.hfbc_mmbuf_base1_c8 > 0)) {
		hfbcd_para->mm_base0_y8 = layer->img.hfbc_mmbuf_base0_y8;
		hfbcd_para->mm_base1_c8 = layer->img.hfbc_mmbuf_base1_c8;
		hfbcd_para->mm_base2_y2 = layer->img.hfbc_mmbuf_base2_y2;
		hfbcd_para->mm_base3_c2 = layer->img.hfbc_mmbuf_base3_c2;
	} else {
		ret = dpu_hfbcd_mmbuf_config(dpufd, ovl_idx, layer, layer->img.hfbcd_block_type, is_pixel_10bit);
		if (ret < 0) {
			DPU_FB_ERR("dpu_hfbcd_mmbuf_config fail!\n");
			return -EINVAL;
		}

		hfbcd_para->mm_base0_y8 = dpufd->mmbuf_info->mm_base0_y8[chn_idx];
		hfbcd_para->mm_base1_c8 = dpufd->mmbuf_info->mm_base1_c8[chn_idx];
		if (is_pixel_10bit) {
			hfbcd_para->mm_base2_y2 = dpufd->mmbuf_info->mm_base2_y2[chn_idx];
			hfbcd_para->mm_base3_c2 = dpufd->mmbuf_info->mm_base3_c2[chn_idx];
		}
	}

	hfbcd_para->mm_base0_y8 -= MMBUF_BASE;
	hfbcd_para->mm_base1_c8 -= MMBUF_BASE;

	ret = dpu_hfbcd_config_check_mm_base0(dpufd, hfbcd_para->mm_base0_y8, hfbcd_para->mm_base1_c8,
		chn_idx, layer->layer_idx);
	if (ret == -EINVAL)
		return ret;

	if (is_pixel_10bit) {
		hfbcd_para->mm_base2_y2 -= MMBUF_BASE;
		hfbcd_para->mm_base3_c2 -= MMBUF_BASE;

		ret = dpu_hfbcd_config_check_mm_base2(dpufd, hfbcd_para->mm_base2_y2, hfbcd_para->mm_base3_c2,
			chn_idx, layer->layer_idx);
		if (ret == -EINVAL)
			return ret;
	}

	return 0;
}

static int dpu_hfbcd_get_align_info(struct dss_hfbcd_para *hfbcd_para)
{
	if (hfbcd_para->hfbcd_block_type == 0) {
		hfbcd_para->hfbcd_block_width_align = HFBC_BLOCK0_WIDTH_ALIGN;
		hfbcd_para->hfbcd_block_height_align = HFBC_BLOCK0_HEIGHT_ALIGN;
	} else if (hfbcd_para->hfbcd_block_type == 1) {
		hfbcd_para->hfbcd_block_width_align = HFBC_BLOCK1_WIDTH_ALIGN;
		hfbcd_para->hfbcd_block_height_align = HFBC_BLOCK1_HEIGHT_ALIGN;
	} else {
		DPU_FB_ERR("hfbcd_block_type=%d no support!\n", hfbcd_para->hfbcd_block_type);
		return -EINVAL;
	}

	return 0;
}

static int dpu_hfbcd_get_hreg_pic_info(struct dss_hfbcd_para *hfbcd_para,
	const dss_layer_t *layer, dss_rect_ltrb_t *aligned_rect)
{
	dss_rect_t src_rect = layer->src_rect;
	uint32_t src_bottom = (uint32_t)src_rect.y + src_rect.h;
	uint32_t src_right = (uint32_t)src_rect.x + src_rect.w;
	/* aligned rect */
	aligned_rect->left = ALIGN_DOWN((uint32_t)src_rect.x, hfbcd_para->hfbcd_block_width_align);
	aligned_rect->right = ALIGN_UP(src_right, hfbcd_para->hfbcd_block_width_align) - 1;
	aligned_rect->top = ALIGN_DOWN((uint32_t)src_rect.y, hfbcd_para->hfbcd_block_height_align);
	aligned_rect->bottom = ALIGN_UP(src_bottom, hfbcd_para->hfbcd_block_height_align) - 1;

	hfbcd_para->hfbcd_hreg_pic_width = aligned_rect->right - aligned_rect->left;
	hfbcd_para->hfbcd_hreg_pic_height = aligned_rect->bottom - aligned_rect->top;

	if (layer->transform & DPU_FB_TRANSFORM_ROT_90) {
		if ((hfbcd_para->hfbcd_hreg_pic_width > HFBC_PIC_WIDTH_ROT_MAX) ||
			(hfbcd_para->hfbcd_hreg_pic_height > HFBC_PIC_HEIGHT_ROT_MAX)) {
			DPU_FB_ERR("layer%d hreg_pic_width %d, hreg_pic_height %d is larger than 4096*2160!\n",
				layer->layer_idx, hfbcd_para->hfbcd_hreg_pic_width, hfbcd_para->hfbcd_hreg_pic_height);
			return -EINVAL;
		}
	}

	return 0;
}

static int dpu_hfbcd_get_payload_info(struct dss_hfbcd_para *hfbcd_para,
	const dss_layer_t *layer, dss_rect_ltrb_t *aligned_rect, bool is_pixel_10bit)
{
	/* hfbcd payload */
	if (is_pixel_10bit) {
		hfbcd_para->hfbcd_payload0_align = HFBC_PAYLOAD_ALIGN_10BIT;
		hfbcd_para->hfbcd_payload1_align = HFBC_PAYLOAD_ALIGN_10BIT;
	} else {
		hfbcd_para->hfbcd_payload0_align = HFBC_PAYLOAD0_ALIGN_8BIT;
		hfbcd_para->hfbcd_payload1_align = HFBC_PAYLOAD1_ALIGN_8BIT;
	}

	hfbcd_para->hfbcd_payload_stride0 = layer->img.hfbc_payload_stride0;
	hfbcd_para->hfbcd_payload_stride1 = layer->img.hfbc_payload_stride1;

	hfbcd_para->hfbcd_payload_addr0 = layer->img.hfbc_payload_addr0 +
		(aligned_rect->top / hfbcd_para->hfbcd_block_height_align) * hfbcd_para->hfbcd_payload_stride0 +
		(aligned_rect->left / hfbcd_para->hfbcd_block_width_align) * hfbcd_para->hfbcd_payload0_align;

	hfbcd_para->hfbcd_payload_addr1 = layer->img.hfbc_payload_addr1 +
		(aligned_rect->top / hfbcd_para->hfbcd_block_height_align) * hfbcd_para->hfbcd_payload_stride1 +
		(aligned_rect->left / hfbcd_para->hfbcd_block_width_align) * hfbcd_para->hfbcd_payload1_align;

	if (is_pixel_10bit) {
		if ((hfbcd_para->hfbcd_payload_addr0 != hfbcd_para->hfbcd_payload_addr1) ||
			(hfbcd_para->hfbcd_payload_stride0 != hfbcd_para->hfbcd_payload_stride1)) {
			DPU_FB_ERR("layer%d 10bit hfbcd_payload_addr0 is not equal "
				"to hfbcd_payload_addr1 or hfbcd_payload_stride0 is "
				"not equal to hfbcd_payload_stride1!\n", layer->layer_idx);
			return -EINVAL;
		}
	}

	dpu_check_and_return((hfbcd_para->hfbcd_payload_addr0 & (hfbcd_para->hfbcd_payload0_align - 1)),
		-EINVAL, ERR, "hfbcd_payload_addr0 and hfbcd_payload0_align - 1 not match!\n");
	dpu_check_and_return((hfbcd_para->hfbcd_payload_stride0 & (hfbcd_para->hfbcd_payload0_align - 1)),
		-EINVAL, ERR, "hfbcd_payload_stride0 and hfbcd_payload0_align not match!\n");
	dpu_check_and_return((hfbcd_para->hfbcd_payload_addr1 & (hfbcd_para->hfbcd_payload1_align - 1)),
		-EINVAL, ERR, "hfbcd_payload_addr1 and hfbcd_payload1_align - 1 not match!\n");
	dpu_check_and_return((hfbcd_para->hfbcd_payload_stride1 & (hfbcd_para->hfbcd_payload1_align - 1)),
		-EINVAL, ERR, "hfbcd_payload_stride1 and hfbcd_payload1_align - 1 not match!\n");

	return 0;
}

static int dpu_hfbcd_get_encoded_para(struct dss_hfbcd_para *hfbcd_para,
	const dss_layer_t *layer, dss_rect_ltrb_t *aligned_rect)
{
	int bpp;
	int aligned_pixel;
	bool is_pixel_10bit = false;

	is_pixel_10bit = is_pixel_10bit2dma(hfbcd_para->rdma_format);
	bpp = (hfbcd_para->is_yuv_semi_planar || hfbcd_para->is_yuv_planar) ? 1 : layer->img.bpp;
	if (is_pixel_10bit)
		bpp = layer->img.bpp;

	aligned_pixel = DMA_ALIGN_BYTES / bpp;

	if (dpu_hfbcd_get_hreg_pic_info(hfbcd_para, layer, aligned_rect) != 0)
		return -EINVAL;

	dpu_check_and_return((layer->img.width & (hfbcd_para->hfbcd_block_width_align - 1)), -EINVAL, ERR,
		"layer->img.width and hfbcd_block_width_align - 1 not match!\n");
	dpu_check_and_return((layer->img.height & (hfbcd_para->hfbcd_block_height_align - 1)), -EINVAL, ERR,
		"layer->img.height and hfbcd_block_height_align - 1 not match!\n");

	if (hfbcd_para->hfbcd_block_type == 0) {
		if (layer->transform & DPU_FB_TRANSFORM_ROT_90)
			hfbcd_para->hfbcd_crop_num_max = HFBCD_BLOCK0_ROT_CROP_MAX;
		else
			hfbcd_para->hfbcd_crop_num_max = HFBCD_BLOCK0_CROP_MAX;
	} else {
		hfbcd_para->hfbcd_crop_num_max = HFBCD_BLOCK1_CROP_MAX;
	}

	hfbcd_para->rdma_oft_x0 = aligned_rect->left / aligned_pixel;
	hfbcd_para->rdma_oft_x1 = aligned_rect->right / aligned_pixel;

	hfbcd_para->hfbcd_header_stride0 = layer->img.hfbc_header_stride0;
	hfbcd_para->hfbcd_header_offset =
		(aligned_rect->top / hfbcd_para->hfbcd_block_height_align) * hfbcd_para->hfbcd_header_stride0 +
		(aligned_rect->left / hfbcd_para->hfbcd_block_width_align) * HFBC_HEADER_STRIDE_BLOCK;

	hfbcd_para->hfbcd_header_addr0 = layer->img.hfbc_header_addr0 + hfbcd_para->hfbcd_header_offset;
	hfbcd_para->hfbcd_header_addr1 = layer->img.hfbc_header_addr1 + hfbcd_para->hfbcd_header_offset;
	hfbcd_para->hfbcd_header_stride1 = hfbcd_para->hfbcd_header_stride0;
	dpu_check_and_return((hfbcd_para->hfbcd_header_addr0 & (HFBC_HEADER_ADDR_ALIGN - 1)), -EINVAL, ERR,
		"hfbcd_para->hfbcd_header_addr0 and HFBC_HEADER_ADDR_ALIGN - 1 not match!\n");
	dpu_check_and_return((hfbcd_para->hfbcd_header_stride0 & (HFBC_HEADER_STRIDE_ALIGN - 1)), -EINVAL, ERR,
		"hfbcd_para->hfbcd_header_stride0 and HFBC_HEADER_STRIDE_ALIGN - 1 not match!\n");
	dpu_check_and_return((hfbcd_para->hfbcd_header_addr1 & (HFBC_HEADER_ADDR_ALIGN - 1)), -EINVAL, ERR,
		"hfbcd_para->hfbcd_header_addr1 and HFBC_HEADER_ADDR_ALIGN - 1 not match!\n");

	if (dpu_hfbcd_get_payload_info(hfbcd_para, layer, aligned_rect, is_pixel_10bit) != 0)
		return -EINVAL;

	return 0;
}

static int dpu_hfbcd_get_out_rect(struct dss_hfbcd_para *hfbcd_para, dss_layer_t *layer,
	dss_rect_ltrb_t *aligned_rect, dss_rect_ltrb_t *clip_rect, dss_rect_t *out_aligned_rect)
{
	dss_rect_t src_rect;
	int chn_idx;
	uint32_t src_bottom;
	uint32_t src_right;

	chn_idx = layer->chn_idx;
	src_rect = layer->src_rect;
	src_bottom = (uint32_t)src_rect.y + src_rect.h;
	src_right = (uint32_t)src_rect.x + src_rect.w;

	if (layer->transform & DPU_FB_TRANSFORM_ROT_90) {
		aligned_rect->left = ALIGN_DOWN((uint32_t)src_rect.y, hfbcd_para->hfbcd_block_height_align);
		aligned_rect->right = ALIGN_UP(src_bottom, hfbcd_para->hfbcd_block_height_align) - 1;
		aligned_rect->top = ALIGN_DOWN((uint32_t)src_rect.x, hfbcd_para->hfbcd_block_width_align);
		aligned_rect->bottom = ALIGN_UP(src_right, hfbcd_para->hfbcd_block_width_align) - 1;
	}

	/* out_aligned_rect */
	out_aligned_rect->x = 0;
	out_aligned_rect->y = 0;
	out_aligned_rect->w = aligned_rect->right - aligned_rect->left + 1;
	out_aligned_rect->h = aligned_rect->bottom - aligned_rect->top + 1;

	/* rdfc clip_rect */
	if (layer->transform & DPU_FB_TRANSFORM_ROT_90) {
		clip_rect->left = src_rect.y - aligned_rect->left;
		clip_rect->right = aligned_rect->right - DSS_WIDTH(src_bottom);
		clip_rect->top = src_rect.x - aligned_rect->top;
		clip_rect->bottom = aligned_rect->bottom - DSS_HEIGHT(src_right);
	} else {
		clip_rect->left = src_rect.x - aligned_rect->left;
		clip_rect->right = aligned_rect->right - DSS_WIDTH(src_right);
		clip_rect->top = src_rect.y - aligned_rect->top;
		clip_rect->bottom = aligned_rect->bottom - DSS_HEIGHT(src_bottom);
	}
	if (dpu_adjust_clip_rect(layer, clip_rect) < 0) {
		DPU_FB_ERR("clip rect invalid => layer_idx=%d, chn_idx=%d, clip_rect %d, %d, %d, %d.\n",
			layer->layer_idx, chn_idx, clip_rect->left, clip_rect->right,
			clip_rect->top, clip_rect->bottom);
		return -EINVAL;
	}

	/* hfbcd crop */
	hfbcd_para->hfbcd_top_crop_num = ((uint32_t)(clip_rect->top) > hfbcd_para->hfbcd_crop_num_max) ?  /*lint !e574*/
		hfbcd_para->hfbcd_crop_num_max : (uint32_t)(clip_rect->top);
	hfbcd_para->hfbcd_bottom_crop_num = ((uint32_t)(clip_rect->bottom) > hfbcd_para->hfbcd_crop_num_max) ?  /*lint !e574*/
		hfbcd_para->hfbcd_crop_num_max : (uint32_t)(clip_rect->bottom);

	clip_rect->top -= (int32_t)(hfbcd_para->hfbcd_top_crop_num);
	clip_rect->bottom -= (int32_t)(hfbcd_para->hfbcd_bottom_crop_num);

	/* adjust out_aligned_rect */
	out_aligned_rect->h -= (hfbcd_para->hfbcd_top_crop_num + hfbcd_para->hfbcd_bottom_crop_num);
	hfbcd_para->stretch_size_vrt = DSS_HEIGHT(out_aligned_rect->h);

	return 0;
}

static void dpu_hfbcd_generate_reg_value(struct dpu_fb_data_type *dpufd, dss_layer_t *layer,
	const struct dss_hfbcd_para *hfbcd_para, dss_rect_ltrb_t aligned_rect, bool rdma_stretch_enable)
{
	dss_rdma_t *dma = NULL;
	int chn_idx;

	chn_idx = layer->chn_idx;

	dma = &(dpufd->dss_module.rdma[chn_idx]);
	dpufd->dss_module.dma_used[chn_idx] = 1;

	dma->hfbcd_used = 1;
	dma->oft_x0 = set_bits32(dma->oft_x0, hfbcd_para->rdma_oft_x0, 16, 0);
	dma->oft_x1 = set_bits32(dma->oft_x1, hfbcd_para->rdma_oft_x1, 16, 0);
	dma->stretch_size_vrt = set_bits32(dma->stretch_size_vrt,
		(hfbcd_para->stretch_size_vrt | (hfbcd_para->stretched_line_num << 13)), 19, 0);
	dma->ctrl = set_bits32(dma->ctrl, hfbcd_para->rdma_format, 5, 3);
	dma->ctrl = set_bits32(dma->ctrl, (hfbcd_para->mmu_enable ? 0x1 : 0x0), 1, 8);
	dma->ctrl = set_bits32(dma->ctrl, hfbcd_para->rdma_transform, 3, 9);
	dma->ctrl = set_bits32(dma->ctrl, (rdma_stretch_enable ? 1 : 0), 1, 12);
	dma->ch_ctl = set_bits32(dma->ch_ctl, 0x1, 1, 0);
	dma->ch_ctl = set_bits32(dma->ch_ctl, 0x1, 1, 2);

	dma->hfbcd_hreg_hdr_ptr_l0 = set_bits32(dma->hfbcd_hreg_hdr_ptr_l0, hfbcd_para->hfbcd_header_addr0, 32, 0);
	dma->hfbcd_hreg_pic_width = set_bits32(dma->hfbcd_hreg_pic_width, hfbcd_para->hfbcd_hreg_pic_width, 16, 0);
	dma->hfbcd_hreg_pic_height = set_bits32(dma->hfbcd_hreg_pic_height, hfbcd_para->hfbcd_hreg_pic_height, 16, 0);
	dma->hfbcd_line_crop = set_bits32(dma->hfbcd_line_crop,
		((hfbcd_para->hfbcd_top_crop_num << 8) | hfbcd_para->hfbcd_bottom_crop_num), 16, 0);
	dma->hfbcd_input_header_stride0 = set_bits32(dma->hfbcd_input_header_stride0,
		hfbcd_para->hfbcd_header_stride0, 14, 0);
	dma->hfbcd_hreg_hdr_ptr_l1 = set_bits32(dma->hfbcd_hreg_hdr_ptr_l1, hfbcd_para->hfbcd_header_addr1, 32, 0);
	dma->hfbcd_header_stride1 = set_bits32(dma->hfbcd_header_stride1, hfbcd_para->hfbcd_header_stride1, 14, 0);
	dma->hfbcd_mm_base0_y8 = set_bits32(dma->hfbcd_mm_base0_y8, hfbcd_para->mm_base0_y8, 32, 0);
	dma->hfbcd_mm_base1_c8 = set_bits32(dma->hfbcd_mm_base1_c8, hfbcd_para->mm_base1_c8, 32, 0);
	dma->hfbcd_mm_base2_y2 = set_bits32(dma->hfbcd_mm_base2_y2, hfbcd_para->mm_base2_y2, 32, 0);
	dma->hfbcd_mm_base3_c2 = set_bits32(dma->hfbcd_mm_base3_c2, hfbcd_para->mm_base3_c2, 32, 0);
	dma->hfbcd_payload_pointer = set_bits32(dma->hfbcd_payload_pointer, hfbcd_para->hfbcd_payload_addr0, 32, 0);
	dma->hfbcd_payload_stride0 = set_bits32(dma->hfbcd_payload_stride0, hfbcd_para->hfbcd_payload_stride0, 20, 0);
	dma->hfbcd_hreg_pld_ptr_l1 = set_bits32(dma->hfbcd_hreg_pld_ptr_l1, hfbcd_para->hfbcd_payload_addr1, 32, 0);
	dma->hfbcd_payload_stride1 = set_bits32(dma->hfbcd_payload_stride1, hfbcd_para->hfbcd_payload_stride1, 20, 0);
	dma->hfbcd_creg_fbcd_ctrl_mode = set_bits32(dma->hfbcd_creg_fbcd_ctrl_mode, 1, 2, 0);
	/* hfbcd_scramble_mode */
	dma->hfbcd_scramble_mode = set_bits32(dma->hfbcd_scramble_mode, layer->img.hfbc_scramble_mode, 4, 2);
	dma->hfbcd_block_type = set_bits32(dma->hfbcd_block_type, hfbcd_para->hfbcd_block_type, 2, 0);

	if ((g_debug_ovl_online_composer > 0) || (g_debug_ovl_offline_composer > 0))
		DPU_FB_INFO("fb%d, mm_base0_y8=0x%x, mm_base2_y2=0x%x, "
			"mm_base1_c8=0x%x, mm_base3_c2=0x%x, mm_base0_y8_size=%d, "
			"mm_base2_y2_size=%d,mm_base1_c8_size=%d,mm_base3_c2_size=%d,"
			"aligned_rect %d,%d,%d,%d, hfbcd_block_type=%d!\n",
			dpufd->index, hfbcd_para->mm_base0_y8, hfbcd_para->mm_base2_y2, hfbcd_para->mm_base1_c8,
			hfbcd_para->mm_base3_c2, layer->img.hfbc_mmbuf_size0_y8,
			layer->img.hfbc_mmbuf_size2_y2, layer->img.hfbc_mmbuf_size1_c8, layer->img.hfbc_mmbuf_size3_c2,
			aligned_rect.left, aligned_rect.top, aligned_rect.right,
			aligned_rect.bottom, hfbcd_para->hfbcd_block_type);
}

int dpu_hfbcd_config(struct dpu_fb_data_type *dpufd,
	int ovl_idx, dss_layer_t *layer, struct dpu_ov_compose_rect *ov_compose_rect,
	struct dpu_ov_compose_flag *ov_compose_flag)
{
	struct dss_hfbcd_para hfbcd_para = {0};
	dss_rect_ltrb_t aligned_rect = { 0, 0, 0, 0 };
	int ret;

	if (!dpufd || !layer)
		return -EINVAL;

	if (!ov_compose_rect || !ov_compose_flag)
		return -EINVAL;

	if (!ov_compose_rect->aligned_rect || !ov_compose_rect->clip_rect)
		return -EINVAL;

	hfbcd_para.stretched_line_num = is_need_rdma_stretch_bit(dpufd, layer);
	ov_compose_flag->rdma_stretch_enable = (hfbcd_para.stretched_line_num > 0) ? true : false;

	ret = dpu_hfbcd_calc_base_para(layer, &hfbcd_para);
	if (ret)
		return -EINVAL;

	ret = dpu_hfbcd_using_base_para(dpufd, ovl_idx, layer, &hfbcd_para);
	if (ret)
		return -EINVAL;

	ret = dpu_hfbcd_get_align_info(&hfbcd_para);
	if (ret)
		return -EINVAL;

	ret = dpu_hfbcd_get_encoded_para(&hfbcd_para, layer, &aligned_rect);
	if (ret)
		return -EINVAL;

	ret = dpu_hfbcd_get_out_rect(&hfbcd_para, layer, &aligned_rect, ov_compose_rect->clip_rect,
		ov_compose_rect->aligned_rect);
	if (ret)
		return -EINVAL;

	dpu_hfbcd_generate_reg_value(dpufd, layer, &hfbcd_para, aligned_rect,
		ov_compose_flag->rdma_stretch_enable);

	return 0;
}

void dpu_hfbcd_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *dma_base, dss_rdma_t *s_dma)
{
	if (!dpufd || !dma_base || !s_dma) {
		DPU_FB_DEBUG("dpufd or dma_base, s_dma is NULL!\n");
		return;
	}

	dpufd->set_reg(dpufd, dma_base + AFBCD_HREG_HDR_PTR_LO,
		s_dma->hfbcd_hreg_hdr_ptr_l0, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_HREG_PIC_WIDTH,
		s_dma->hfbcd_hreg_pic_width, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_HREG_PIC_HEIGHT,
		s_dma->hfbcd_hreg_pic_height, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_LINE_CROP,
		s_dma->hfbcd_line_crop, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_INPUT_HEADER_STRIDE,
		s_dma->hfbcd_input_header_stride0, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_PAYLOAD_STRIDE,
		s_dma->hfbcd_payload_stride0, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_AFBCD_PAYLOAD_POINTER,
		s_dma->hfbcd_payload_pointer, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_SCRAMBLE_MODE,
		s_dma->hfbcd_scramble_mode, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_CREG_FBCD_CTRL_MODE,
		s_dma->hfbcd_creg_fbcd_ctrl_mode, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_HREG_HDR_PTR_L1,
		s_dma->hfbcd_hreg_hdr_ptr_l1, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_HREG_PLD_PTR_L1,
		s_dma->hfbcd_hreg_pld_ptr_l1, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_HEADER_SRTIDE_1,
		s_dma->hfbcd_header_stride1, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_PAYLOAD_SRTIDE_1,
		s_dma->hfbcd_payload_stride1, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_BLOCK_TYPE,
		s_dma->hfbcd_block_type, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_MM_BASE_0,
		s_dma->hfbcd_mm_base0_y8, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_MM_BASE_1,
		s_dma->hfbcd_mm_base1_c8, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_MM_BASE_2,
		s_dma->hfbcd_mm_base2_y2, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_MM_BASE_3,
		s_dma->hfbcd_mm_base3_c2, 32, 0);
}

#ifdef SUPPORT_DSS_HFBCE
static int dpu_hfbce_get_aligned_boundary(const dss_wb_layer_t *layer, struct dss_hfbce_para *hfbce_para)
{
	if (layer->transform & DPU_FB_TRANSFORM_ROT_90) {
		hfbce_para->hfbce_block_width_align = HFBC_BLOCK1_WIDTH_ALIGN;
		hfbce_para->hfbce_block_height_align = HFBC_BLOCK1_HEIGHT_ALIGN;
	} else {
		hfbce_para->hfbce_block_width_align = HFBC_BLOCK0_WIDTH_ALIGN;
		hfbce_para->hfbce_block_height_align = HFBC_BLOCK0_HEIGHT_ALIGN;
	}

	if ((layer->dst.width & (hfbce_para->hfbce_block_width_align - 1)) ||
		(layer->dst.height & (hfbce_para->hfbce_block_height_align - 1))) {
		DPU_FB_ERR("wb_layer dst width %d is not %d bytes aligned, "
			"or img heigh %d is not %d bytes aligned!\n",
			layer->dst.width, hfbce_para->hfbce_block_width_align,
			layer->dst.height, hfbce_para->hfbce_block_height_align);
		return -EINVAL;
	}

	return 0;
}

static int dpu_hfbce_check_aligned_rect(const dss_rect_t in_rect, dss_wb_layer_t *layer)
{
	if (layer->transform & DPU_FB_TRANSFORM_ROT_90) {
		if ((in_rect.w < HFBC_PIC_WIDTH_ROT_MIN) ||
			(in_rect.w > HFBC_PIC_WIDTH_MAX) ||
			(in_rect.h < HFBC_PIC_HEIGHT_ROT_MIN) ||
			(in_rect.h > HFBC_PIC_HEIGHT_MAX) ||
			((uint32_t)in_rect.w & (HFBC_BLOCK1_HEIGHT_ALIGN - 1)) ||
			((uint32_t)in_rect.h & (HFBC_BLOCK1_WIDTH_ALIGN - 1))) {
			DPU_FB_ERR("hfbce in_rect %d,%d, %d,%d is out of range!\n",
				in_rect.x, in_rect.y, in_rect.w, in_rect.h);
			return -EINVAL;
		}
	} else {
		if ((in_rect.w < HFBC_PIC_WIDTH_MIN) ||
			(in_rect.w > HFBC_PIC_WIDTH_MAX) ||
			(in_rect.h < HFBC_PIC_HEIGHT_MIN) ||
			(in_rect.h > HFBC_PIC_HEIGHT_MAX) ||
			((uint32_t)in_rect.w & (HFBC_BLOCK0_WIDTH_ALIGN - 1)) ||
			((uint32_t)in_rect.h & (HFBC_BLOCK0_HEIGHT_ALIGN - 1))) {
			DPU_FB_ERR("hfbce in_rect %d,%d, %d,%d is out of range!\n",
				in_rect.x, in_rect.y, in_rect.w, in_rect.h);
			return -EINVAL;
		}
	}

	return 0;
}

static int dpu_hfbce_calculate_parameter(const dss_wb_layer_t *layer,
	struct dss_hfbce_para *hfbce_para,
	dss_rect_ltrb_t hfbce_header_rect, dss_rect_ltrb_t hfbce_payload_rect)
{
	bool is_pixel_10bit = false;

	is_pixel_10bit = is_pixel_10bit2dma(hfbce_para->wdma_format);
	/* hfbc header */
	hfbce_para->hfbce_header_stride0 = layer->dst.hfbc_header_stride0;
	/* width_align, height_align from dpu_hfbce_get_aligned_boundary(), not 0 */
	hfbce_para->hfbce_header_offset = (hfbce_header_rect.top /
		hfbce_para->hfbce_block_height_align) * hfbce_para->hfbce_header_stride0 +
		(hfbce_header_rect.left / hfbce_para->hfbce_block_width_align) * HFBC_HEADER_STRIDE_BLOCK;

	hfbce_para->hfbce_header_addr0 = layer->dst.hfbc_header_addr0 + hfbce_para->hfbce_header_offset;
	hfbce_para->hfbce_header_addr1 = layer->dst.hfbc_header_addr1 + hfbce_para->hfbce_header_offset;
	hfbce_para->hfbce_header_stride1 = hfbce_para->hfbce_header_stride0;
	dpu_check_and_return((hfbce_para->hfbce_header_addr0 & (HFBC_HEADER_ADDR_ALIGN - 1)), -EINVAL, ERR,
		"hfbce_header_addr0 and HFBC_HEADER_ADDR_ALIGN - 1 not match!\n");
	dpu_check_and_return((hfbce_para->hfbce_header_stride0 & (HFBC_HEADER_STRIDE_ALIGN - 1)), -EINVAL, ERR,
		"hfbce_para->hfbce_header_stride0 and HFBC_HEADER_STRIDE_ALIGN - 1 not match!\n");
	dpu_check_and_return((hfbce_para->hfbce_header_addr1 & (HFBC_HEADER_ADDR_ALIGN - 1)), -EINVAL, ERR,
		"hfbcd_para->hfbce_header_addr1 and HFBC_HEADER_ADDR_ALIGN - 1 not match!\n");
	/* hfbc payload */
	hfbce_para->hfbce_payload0_align = is_pixel_10bit ? HFBC_PAYLOAD_ALIGN_10BIT : HFBC_PAYLOAD0_ALIGN_8BIT;
	hfbce_para->hfbce_payload1_align = is_pixel_10bit ? HFBC_PAYLOAD_ALIGN_10BIT : HFBC_PAYLOAD1_ALIGN_8BIT;
	hfbce_para->hfbce_payload_stride0 = layer->dst.hfbc_payload_stride0;
	hfbce_para->hfbce_payload_stride1 = layer->dst.hfbc_payload_stride1;

	hfbce_para->hfbce_payload_addr0 = layer->dst.hfbc_payload_addr0 +
		(hfbce_payload_rect.top / hfbce_para->hfbce_block_height_align) * hfbce_para->hfbce_payload_stride0 +
		(hfbce_payload_rect.left / hfbce_para->hfbce_block_width_align) * hfbce_para->hfbce_payload0_align;

	hfbce_para->hfbce_payload_addr1 = layer->dst.hfbc_payload_addr1 +
		(hfbce_payload_rect.top / hfbce_para->hfbce_block_height_align) * hfbce_para->hfbce_payload_stride1 +
		(hfbce_payload_rect.left / hfbce_para->hfbce_block_width_align) * hfbce_para->hfbce_payload1_align;

	if (is_pixel_10bit) {
		if ((hfbce_para->hfbce_payload_addr0 != hfbce_para->hfbce_payload_addr1) ||
			(hfbce_para->hfbce_payload_stride0 != hfbce_para->hfbce_payload_stride1)) {
			DPU_FB_ERR("ch%d 10bit hfbce_payload_addr0 is not equal to "
				"hfbce_payload_addr1 or hfbce_payload_stride0 is "
				"not equal to hfbce_payload_stride1!\n", layer->chn_idx);
			return -EINVAL;
		}
	}

	dpu_check_and_return((hfbce_para->hfbce_payload_addr0 & (hfbce_para->hfbce_payload0_align - 1)), -EINVAL, ERR,
		"hfbce_payload_addr0 and hfbce_para->hfbce_payload0_align - 1 not match!\n");
	dpu_check_and_return((hfbce_para->hfbce_payload_stride0 & (hfbce_para->hfbce_payload0_align - 1)), -EINVAL, ERR,
		"hfbce_payload_stride0 and hfbce_para->hfbce_payload0_align - 1 not match!\n");
	dpu_check_and_return((hfbce_para->hfbce_payload_addr1 & (hfbce_para->hfbce_payload1_align - 1)), -EINVAL, ERR,
		"hfbce_payload_addr1 and hfbce_para->hfbce_payload1_align - 1 not match!\n");
	dpu_check_and_return((hfbce_para->hfbce_payload_stride1 & (hfbce_para->hfbce_payload1_align - 1)), -EINVAL, ERR,
		"hfbce_payload_stride1 and hfbce_para->hfbce_payload1_align - 1 not match!\n");

	return 0;
}

static void dpu_hfbce_generate_reg_value(struct dpu_fb_data_type *dpufd, dss_wb_layer_t *layer,
	const struct dss_hfbce_para *hfbce_para, dss_rect_t in_rect, bool last_block)
{
	dss_wdma_t *wdma = NULL;
	int chn_idx;

	chn_idx = layer->chn_idx;
	wdma = &(dpufd->dss_module.wdma[chn_idx]);
	dpufd->dss_module.dma_used[chn_idx] = 1;

	wdma->hfbce_used = 1;
	wdma->ctrl = set_bits32(wdma->ctrl, hfbce_para->wdma_format, 5, 3);
	wdma->ctrl = set_bits32(wdma->ctrl, (hfbce_para->mmu_enable ? 0x1 : 0x0), 1, 8);
	wdma->ctrl = set_bits32(wdma->ctrl, hfbce_para->wdma_transform, 3, 9);
	if (last_block)
		wdma->ch_ctl = set_bits32(wdma->ch_ctl, 0x1d, 5, 0);
	else
		wdma->ch_ctl = set_bits32(wdma->ch_ctl, 0xd, 5, 0);

	wdma->rot_size = set_bits32(wdma->rot_size,
		(DSS_WIDTH((uint32_t)in_rect.w) |
		(DSS_HEIGHT((uint32_t)in_rect.h) << 16)), 32, 0);

	wdma->hfbce_hreg_pic_blks = set_bits32(
		wdma->hfbce_hreg_pic_blks, hfbce_para->hfbce_hreg_pic_blks, 24, 0);
	wdma->hfbce_hreg_hdr_ptr_l0 = set_bits32(
		wdma->hfbce_hreg_hdr_ptr_l0, hfbce_para->hfbce_header_addr0, 32, 0);
	wdma->hfbce_hreg_pld_ptr_l0 = set_bits32(
		wdma->hfbce_hreg_pld_ptr_l0, hfbce_para->hfbce_payload_addr0, 32, 0);
	wdma->hfbce_picture_size = set_bits32(wdma->hfbce_picture_size,
		((DSS_WIDTH((uint32_t)in_rect.w) << 16) |
		DSS_HEIGHT((uint32_t)in_rect.h)), 32, 0);
	wdma->hfbce_header_stride0 = set_bits32(
		wdma->hfbce_header_stride0, hfbce_para->hfbce_header_stride0, 14, 0);
	wdma->hfbce_payload_stride0 = set_bits32(
		wdma->hfbce_payload_stride0, hfbce_para->hfbce_payload_stride0, 20, 0);
	wdma->hfbce_scramble_mode = set_bits32(wdma->hfbce_scramble_mode, layer->dst.hfbc_scramble_mode, 4, 2);
	wdma->hfbce_header_pointer_offset = set_bits32(
		wdma->hfbce_header_pointer_offset, hfbce_para->hfbce_header_offset, 32, 0);
	wdma->fbce_creg_fbce_ctrl_mode = set_bits32(
		wdma->fbce_creg_fbce_ctrl_mode, 1, 32, 0);
	wdma->hfbce_hreg_hdr_ptr_l1 = set_bits32(
		wdma->hfbce_hreg_hdr_ptr_l1, hfbce_para->hfbce_header_addr1, 32, 0);
	wdma->hfbce_hreg_pld_ptr_l1 = set_bits32(
		wdma->hfbce_hreg_pld_ptr_l1, hfbce_para->hfbce_payload_addr1, 32, 0);
	wdma->hfbce_header_stride1 = set_bits32(
		wdma->hfbce_header_stride1, hfbce_para->hfbce_header_stride1, 14, 0);
	wdma->hfbce_payload_stride1 = set_bits32(
		wdma->hfbce_payload_stride1, hfbce_para->hfbce_payload_stride1, 20, 0);

	if (g_debug_ovl_offline_composer)
		DPU_FB_INFO("aligned_rect %d,%d,%d,%d!\n",
			in_rect.x, in_rect.y, DSS_WIDTH(in_rect.x + in_rect.w),
			DSS_WIDTH(in_rect.y + in_rect.h));
}

int dpu_hfbce_config(struct dpu_fb_data_type *dpufd, dss_wb_layer_t *layer, dss_rect_t aligned_rect,
	dss_rect_t *ov_block_rect, bool last_block)
{
	struct dss_hfbce_para hfbce_para = {0};
	dss_rect_ltrb_t hfbce_header_rect = {0};
	dss_rect_ltrb_t hfbce_payload_rect = {0};
	dss_rect_t in_rect;
	int chn_idx;
	int ret;

	dpu_check_and_return((!dpufd || !layer || !ov_block_rect), -EINVAL, ERR, "NULL ptr\n");

	chn_idx = layer->chn_idx;
	hfbce_para.wdma_format = dpu_pixel_format_hal2dma(layer->dst.format);
	dpu_check_and_return(hfbce_para.wdma_format < 0, -EINVAL, ERR, "dpu_pixel_format_hal2dma failed!\n");

	in_rect = aligned_rect;
	hfbce_para.wdma_transform = dpu_transform_hal2dma(layer->transform, chn_idx);
	dpu_check_and_return(hfbce_para.wdma_transform < 0, -EINVAL, ERR, "dpu_transform_hal2dma failed!\n");

	hfbce_para.mmu_enable = true;
	hfbce_para.wdma_addr = layer->dst.vir_addr;

	ret = dpu_hfbce_get_aligned_boundary(layer, &hfbce_para);
	if (ret)
		return -EINVAL;
	ret = dpu_hfbce_check_aligned_rect(in_rect, layer);
	if (ret)
		return -EINVAL;

	if (layer->transform & DPU_FB_TRANSFORM_ROT_90) {
		hfbce_header_rect.left = ALIGN_DOWN((uint32_t)layer->dst_rect.x, hfbce_para.hfbce_block_width_align);
		hfbce_header_rect.top = ALIGN_DOWN((uint32_t)(layer->dst_rect.y +
			(ov_block_rect->x - layer->dst_rect.x)), hfbce_para.hfbce_block_height_align);

		hfbce_payload_rect.left = ALIGN_DOWN((uint32_t)layer->dst_rect.x, hfbce_para.hfbce_block_width_align);
		hfbce_payload_rect.top = hfbce_header_rect.top;

		hfbce_para.hfbce_hreg_pic_blks = (in_rect.h / hfbce_para.hfbce_block_width_align) *
			(in_rect.w / hfbce_para.hfbce_block_height_align) - 1;
	} else {
		hfbce_header_rect.left = ALIGN_DOWN((uint32_t)in_rect.x, hfbce_para.hfbce_block_width_align);
		hfbce_header_rect.top = ALIGN_DOWN((uint32_t)in_rect.y, hfbce_para.hfbce_block_height_align);

		hfbce_payload_rect.left = ALIGN_DOWN((uint32_t)in_rect.x, hfbce_para.hfbce_block_width_align);
		hfbce_payload_rect.top = hfbce_header_rect.top;

		hfbce_para.hfbce_hreg_pic_blks = (in_rect.w / hfbce_para.hfbce_block_width_align) *
			(in_rect.h / hfbce_para.hfbce_block_height_align) - 1;
	}

	ret = dpu_hfbce_calculate_parameter(layer, &hfbce_para, hfbce_header_rect, hfbce_payload_rect);
	if (ret)
		return -EINVAL;

	dpu_hfbce_generate_reg_value(dpufd, layer, &hfbce_para, in_rect, last_block);
	if (g_debug_ovl_offline_composer)
		DPU_FB_INFO("hfbce_rect %d,%d,%d,%d!\n",
			hfbce_payload_rect.left, hfbce_payload_rect.top,
			hfbce_payload_rect.right, hfbce_payload_rect.bottom);

	return 0;
}

void dpu_hfbce_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *wdma_base, dss_wdma_t *s_wdma)
{
	if (!dpufd || !wdma_base || !s_wdma) {
		DPU_FB_DEBUG("dpufd or wdma_base, s_wdma is NULL!\n");
		return;
	}

	dpufd->set_reg(dpufd, wdma_base + AFBCE_HREG_PIC_BLKS,
		s_wdma->hfbce_hreg_pic_blks, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + AFBCE_HREG_HDR_PTR_L0,
		s_wdma->hfbce_hreg_hdr_ptr_l0, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + AFBCE_HREG_PLD_PTR_L0,
		s_wdma->hfbce_hreg_pld_ptr_l0, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + AFBCE_PICTURE_SIZE,
		s_wdma->hfbce_picture_size, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + AFBCE_HEADER_SRTIDE,
		s_wdma->hfbce_header_stride0, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + AFBCE_PAYLOAD_STRIDE,
		s_wdma->hfbce_payload_stride0, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + AFBCE_SCRAMBLE_MODE,
		s_wdma->hfbce_scramble_mode, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + AFBCE_HEADER_POINTER_OFFSET,
		s_wdma->hfbce_header_pointer_offset, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + AFBCE_CREG_FBCE_CTRL_MODE,
		s_wdma->fbce_creg_fbce_ctrl_mode, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + AFBCE_HREG_HDR_PTR_L1,
		s_wdma->hfbce_hreg_hdr_ptr_l1, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + AFBCE_HREG_PLD_PTR_L1,
		s_wdma->hfbce_hreg_pld_ptr_l1, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + AFBCE_HEADER_SRTIDE_1,
		s_wdma->hfbce_header_stride1, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + AFBCE_PAYLOAD_SRTIDE_1,
		s_wdma->hfbce_payload_stride1, 32, 0);
}
#endif

#ifdef SUPPORT_DSS_HEBCE
void dpu_hebcd_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *dma_base, dss_rdma_t *s_dma)
{
	if (!dpufd || !dma_base || !s_dma) {
		DPU_FB_DEBUG("dpufd or dma_base, s_dma is NULL!\n");
		return;
	}

	dpufd->set_reg(dpufd, dma_base + AFBCD_HREG_HDR_PTR_LO,
		s_dma->hebcd_hreg_hdr_ptr_l0, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_HREG_PIC_WIDTH,
		s_dma->hebcd_hreg_pic_width, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_HREG_PIC_HEIGHT,
		s_dma->hebcd_hreg_pic_height, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_HREG_FORMAT,
		s_dma->hebcd_hreg_format, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_STR, s_dma->hebcd_str, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_LINE_CROP,
		s_dma->hebcd_line_crop, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_INPUT_HEADER_STRIDE,
		s_dma->hebcd_input_header_stride0, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_PAYLOAD_STRIDE,
		s_dma->hebcd_payload_stride0, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_MM_BASE_0,
		s_dma->hebcd_mm_base0_y8, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_AFBCD_PAYLOAD_POINTER,
		s_dma->hebcd_payload_pointer, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_HEIGHT_BF_STR,
		s_dma->hebcd_height_bf_str, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_SCRAMBLE_MODE,
		s_dma->hebcd_scramble_mode, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_CREG_FBCD_CTRL_MODE,
		s_dma->hebcd_creg_fbcd_ctrl_mode, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_HREG_HDR_PTR_L1,
		s_dma->hebcd_hreg_hdr_ptr_l1, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_HREG_PLD_PTR_L1,
		s_dma->hebcd_hreg_pld_ptr_l1, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_HEADER_SRTIDE_1,
		s_dma->hebcd_header_stride1, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_PAYLOAD_SRTIDE_1,
		s_dma->hebcd_payload_stride1, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_BLOCK_TYPE,
		s_dma->hebcd_block_type, 32, 0);
	dpufd->set_reg(dpufd, dma_base + AFBCD_MM_BASE_1,
		s_dma->hebcd_mm_base1_c8, 32, 0);
}

static int dpu_hebcd_config_check_mm_base0(
	struct dpu_fb_data_type *dpufd, uint32_t mm_base0_y8,
	uint32_t mm_base1_c8, dss_layer_t *layer, bool is_yuv_semi_planar)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL, return!\n");
		return -EINVAL;
	}

	if ((mm_base0_y8 & (HEBC_MMBUF_ADDR_ALIGN - 1)) ||
		(dpufd->mmbuf_info->mm_size0_y8[layer->chn_idx] & (HEBC_MMBUF_ADDR_ALIGN - 1))) {
		DPU_FB_ERR("layer%d mm_base0_y8 0x%x or mm_size0_y80x%x or "
			"mm_base1_c8 0x%x or mm_size1_c8 0x%x is not %d bytes aligned!\n",
			layer->layer_idx, mm_base0_y8, dpufd->mmbuf_info->mm_size0_y8[layer->chn_idx],
			mm_base1_c8, dpufd->mmbuf_info->mm_size1_c8[layer->chn_idx],
			HEBC_MMBUF_ADDR_ALIGN);
		return -EINVAL;
	}

	if (is_yuv_semi_planar) {
		if ((mm_base1_c8 & (HEBC_MMBUF_ADDR_ALIGN - 1)) ||
			(dpufd->mmbuf_info->mm_size1_c8[layer->chn_idx] & (HEBC_MMBUF_ADDR_ALIGN - 1))) {
			DPU_FB_ERR("layer%d mm_base0_y8 0x%x or mm_size0_y8 0x%x or "
				"mm_base1_c8 0x%x or mm_size1_c8 0x%x is not %d bytes aligned!\n",
				layer->layer_idx, mm_base0_y8, dpufd->mmbuf_info->mm_size0_y8[layer->chn_idx],
				mm_base1_c8, dpufd->mmbuf_info->mm_size1_c8[layer->chn_idx],
				HEBC_MMBUF_ADDR_ALIGN);
			return -EINVAL;
		}
	}

	return 0;
}

static int get_mmbuf_y8c8_line_num(dss_layer_t *layer, uint32_t hebcd_block_type, bool is_yuv_semi_planar,
	uint32_t *mmbuf_y8_line_num, uint32_t *mmbuf_c8_line_num)
{
	if (hebcd_block_type == 0) {
		if (layer->transform & DPU_FB_TRANSFORM_ROT_90) { /* read rotation */
			if (is_yuv_semi_planar) {
				*mmbuf_y8_line_num = 64;  /* Y8: 64 Y10: 32 */
				*mmbuf_c8_line_num = 32;  /* C8: 32 C10: 16 */
			} else {
				*mmbuf_y8_line_num = 16;  /* C8: 16 C10: 8 */
			}
		} else {
			if (is_yuv_semi_planar) {
				*mmbuf_y8_line_num = HEBC_MMBUF_BLOCK0_Y8_LINE_NUM;
				*mmbuf_c8_line_num = HEBC_MMBUF_BLOCK0_C8_LINE_NUM;
			} else {
				*mmbuf_y8_line_num = HEBC_MMBUF_BLOCK0_RGB_LINE_NUM;
			}
		}
	} else if (hebcd_block_type == 1) {
		if (is_yuv_semi_planar) {
			*mmbuf_y8_line_num = HEBC_MMBUF_BLOCK1_Y8_LINE_NUM;
			*mmbuf_c8_line_num = HEBC_MMBUF_BLOCK1_C8_LINE_NUM;
		} else {
			*mmbuf_y8_line_num = HEBC_MMBUF_BLOCK1_RGB_LINE_NUM;
		}
	} else {
		DPU_FB_ERR("hebcd_block_type=%d no support!\n", layer->img.hebcd_block_type);
		return -EINVAL;
	}

	return 0;
}

static bool is_mm_alloc_needed(struct dpu_fb_data_type *dpufd, int ovl_idx, int chn_idx)
{
	bool mm_alloc_needed = false;

	if (ovl_idx <= DSS_OVL1) {
		mm_alloc_needed = true;
	} else {
		if (dpufd->mmbuf_info->mm_used[chn_idx] == 1)
			mm_alloc_needed = false;
		else
			mm_alloc_needed = true;
	}

	return mm_alloc_needed;
}

static int dpu_hebcd_mmbuf_config(
	struct dpu_fb_data_type *dpufd, int ovl_idx,
	dss_layer_t *layer, uint32_t hebcd_block_type,
	bool is_yuv_semi_planar)
{
	int chn_idx;
	dss_rect_t src_rect;
	dss_rect_ltrb_t hebcd_rect;
	uint32_t mmbuf_y8_line_num = 0;
	uint32_t mmbuf_c8_line_num = 0;

	if (!dpufd || !layer)
		return -EINVAL;

	chn_idx = layer->chn_idx;
	src_rect = layer->src_rect;

	if (is_mm_alloc_needed(dpufd, ovl_idx, chn_idx)) {
		hebcd_rect.left = ALIGN_DOWN((uint32_t)src_rect.x, HEBC_MMBUF_ADDR_ALIGN);
		hebcd_rect.right = ALIGN_UP((uint32_t)(src_rect.x - hebcd_rect.left + src_rect.w),
			HEBC_MMBUF_ADDR_ALIGN);

		if (get_mmbuf_y8c8_line_num(layer, hebcd_block_type, is_yuv_semi_planar,
			&mmbuf_y8_line_num, &mmbuf_c8_line_num) != 0)
			return -EINVAL;

		/* Y8: W*1*line_num, Y10: W*2*line_num, RGB: W*4*line_num */
		hebcd_rect.right = is_yuv_semi_planar ? (hebcd_rect.right * 2) : (hebcd_rect.right * 4);

		dpufd->mmbuf_info->mm_size0_y8[chn_idx] = hebcd_rect.right * mmbuf_y8_line_num;
		dpufd->mmbuf_info->mm_size1_c8[chn_idx] =
			is_yuv_semi_planar ? (hebcd_rect.right * mmbuf_c8_line_num) : 0;

		dpufd->mmbuf_info->mm_size[chn_idx] = dpufd->mmbuf_info->mm_size0_y8[chn_idx] +
			dpufd->mmbuf_info->mm_size1_c8[chn_idx];

		dpufd->mmbuf_info->mm_base[chn_idx] = dpu_mmbuf_alloc(g_mmbuf_gen_pool,
			dpufd->mmbuf_info->mm_size[chn_idx]);
		dpufd->mmbuf_info->mm_base0_y8[chn_idx] = dpufd->mmbuf_info->mm_base[chn_idx];
		if (dpufd->mmbuf_info->mm_base0_y8[chn_idx] < MMBUF_BASE) {
			DPU_FB_ERR("fb%d, chn%d failed to alloc mmbuf, mm_base0_y8=0x%x\n",
				dpufd->index, chn_idx, dpufd->mmbuf_info->mm_base0_y8[chn_idx]);
			return -EINVAL;
		}

		if (is_yuv_semi_planar) {
			dpufd->mmbuf_info->mm_base1_c8[chn_idx] = dpufd->mmbuf_info->mm_base0_y8[chn_idx] +
				dpufd->mmbuf_info->mm_size0_y8[chn_idx];
			if (dpufd->mmbuf_info->mm_base1_c8[chn_idx] < MMBUF_BASE) {
				DPU_FB_ERR("fb%d, chn%d failed to alloc mmbuf, mm_base1_c8=0x%x\n",
					dpufd->index, chn_idx, dpufd->mmbuf_info->mm_base1_c8[chn_idx]);
				return -EINVAL;
			}
		}
	}

	dpufd->mmbuf_info->mm_used[chn_idx] = 1;

	return 0;
}

static int dpu_hebcd_calc_base_para(struct dpu_fb_data_type *dpufd, dss_layer_t *layer,
	struct dss_hebcd_para *hebcd_para)
{
	int chn_idx = layer->chn_idx;

	hebcd_para->color_transform = 1;
	hebcd_para->is_yuv_semi_planar = is_yuv_semiplanar(layer->img.format);
	hebcd_para->hebcd_block_type = layer->img.hebcd_block_type;
	hebcd_para->stretched_line_num = is_need_rdma_stretch_bit(dpufd, layer);
	hebcd_para->mmu_enable = (layer->img.mmu_enable == 1) ? true : false;
	hebcd_para->rdma_format = dpu_pixel_format_hal2dma(layer->img.format);
	if (hebcd_para->rdma_format < 0) {
		DPU_FB_ERR("layer format %d not support!\n", layer->img.format);
		return -EINVAL;
	}

	hebcd_para->rdma_transform = dpu_transform_hal2dma(layer->transform, chn_idx);
	if (hebcd_para->rdma_transform < 0) {
		DPU_FB_ERR("layer transform %d not support!\n", layer->transform);
		return -EINVAL;
	}

	hebcd_para->mm_base0_y8 = layer->img.hebc_mmbuf_base0_y8;
	if (layer->img.hebc_mmbuf_base0_y8 <= 0)
		hebcd_para->mm_base0_y8 = dpufd->mmbuf_info->mm_base0_y8[chn_idx];
	hebcd_para->mm_base0_y8 -= MMBUF_BASE;

	if (hebcd_para->is_yuv_semi_planar) {
		hebcd_para->mm_base1_c8 = layer->img.hebc_mmbuf_base1_c8;
		if (layer->img.hebc_mmbuf_base1_c8 <= 0)
			hebcd_para->mm_base1_c8 = dpufd->mmbuf_info->mm_base1_c8[chn_idx];
		hebcd_para->mm_base1_c8 -= MMBUF_BASE;

		hebcd_para->color_transform = 0;
	}

	return 0;
}

static int dpu_hebcd_using_base_para(struct dpu_fb_data_type *dpufd, int ovl_idx,
	dss_layer_t *layer, const struct dss_hebcd_para *hebcd_para)
{
	int ret;

	if (layer->img.hebc_mmbuf_base0_y8 <= 0) {
		ret = dpu_hebcd_mmbuf_config(dpufd, ovl_idx, layer,
			hebcd_para->hebcd_block_type, hebcd_para->is_yuv_semi_planar);
		if (ret < 0) {
			DPU_FB_ERR("dpu_hebcd_mmbuf_config fail!\n");
			return -EINVAL;
		}
	}

	ret = dpu_hebcd_config_check_mm_base0(dpufd, hebcd_para->mm_base0_y8, hebcd_para->mm_base1_c8,
		layer, hebcd_para->is_yuv_semi_planar);
	if (ret == -EINVAL)
		return ret;

	return 0;
}

static int dpu_hebcd_get_align_info(struct dss_hebcd_para *hebcd_para)
{
	if (hebcd_para->hebcd_block_type == 0) {
		hebcd_para->hebcd_block_width_align = HEBC_BLOCK0_RGB_WIDTH_ALIGN;
		hebcd_para->hebcd_block_height_align = HEBC_BLOCK0_RGB_HEIGHT_ALIGN;
		hebcd_para->hebcd_crop_num_max = HEBC_BLOCK0_RGB_CROP_MAX;

		if (hebcd_para->is_yuv_semi_planar) {
			if (hebcd_para->rdma_format == DMA_PIXEL_FORMAT_YUV420_SP_10BIT) {
				hebcd_para->hebcd_block_width_align = 32; /* HEBC_BLOCK0_YUV_WIDTH_ALIGN */
				hebcd_para->hebcd_block_height_align = 16; /* HEBC_BLOCK0_YUV_HEIGHT_ALIGN */
			} else {
				hebcd_para->hebcd_block_width_align = HEBC_BLOCK0_YUV_WIDTH_ALIGN;
				hebcd_para->hebcd_block_height_align = HEBC_BLOCK0_YUV_HEIGHT_ALIGN;
			}
			hebcd_para->hebcd_crop_num_max = HEBC_BLOCK0_YUV_CROP_MAX;
		}

		return 0;
	}

	if (hebcd_para->hebcd_block_type == 1) {
		hebcd_para->hebcd_block_width_align = HEBC_BLOCK1_RGB_WIDTH_ALIGN;
		hebcd_para->hebcd_block_height_align = HEBC_BLOCK1_RGB_HEIGHT_ALIGN;
		hebcd_para->hebcd_crop_num_max = HEBC_BLOCK1_RGB_CROP_MAX;

		if (hebcd_para->is_yuv_semi_planar) {
			hebcd_para->hebcd_block_width_align = HEBC_BLOCK1_YUV_WIDTH_ALIGN;
			hebcd_para->hebcd_block_height_align = HEBC_BLOCK1_YUV_HEIGHT_ALIGN;
			hebcd_para->hebcd_crop_num_max = HEBC_BLOCK1_YUV_CROP_MAX;
		}

		return 0;
	}

	DPU_FB_ERR("hebcd_block_type=%d no support!\n", hebcd_para->hebcd_block_type);
	return -EINVAL;
}

static int dpu_hebcd_get_aligned_pixel(struct dss_hebcd_para *hebcd_para, const dss_layer_t *layer)
{
	int bpp;
	int aligned_pixel;
	bool is_pixel_10bit = false;

	is_pixel_10bit = is_pixel_10bit2dma(hebcd_para->rdma_format);
	bpp = hebcd_para->is_yuv_semi_planar ? 1 : layer->img.bpp;
	if (is_pixel_10bit)
		bpp = layer->img.bpp;

	aligned_pixel = DMA_ALIGN_BYTES / bpp;

	return aligned_pixel;
}

static int dpu_hebcd_get_hreg_pic_size(struct dss_hebcd_para *hebcd_para,
	const dss_layer_t *layer, dss_rect_ltrb_t *aligned_rect)
{
	dss_rect_t src_rect = layer->src_rect;
	uint32_t src_bottom = (uint32_t)src_rect.y + src_rect.h;
	uint32_t src_right = (uint32_t)src_rect.x + src_rect.w;

	aligned_rect->left = ALIGN_DOWN((uint32_t)src_rect.x, hebcd_para->hebcd_block_width_align);
	aligned_rect->right = ALIGN_UP(src_right, hebcd_para->hebcd_block_width_align) - 1;
	aligned_rect->top = ALIGN_DOWN((uint32_t)src_rect.y, hebcd_para->hebcd_block_height_align);
	aligned_rect->bottom = ALIGN_UP(src_bottom, hebcd_para->hebcd_block_height_align) - 1;

	hebcd_para->hebcd_hreg_pic_width = aligned_rect->right - aligned_rect->left;
	hebcd_para->hebcd_hreg_pic_height = aligned_rect->bottom - aligned_rect->top;

	if (layer->transform & DPU_FB_TRANSFORM_ROT_90) {
		dpu_check_and_return((hebcd_para->hebcd_hreg_pic_width > (HFBC_PIC_WIDTH_ROT_MAX - 1)),
			-EINVAL, ERR, "hebcd_hreg_pic_width <= HFBC_PIC_WIDTH_ROT_MAX - 1!\n");
		dpu_check_and_return((hebcd_para->hebcd_hreg_pic_height > (HFBC_PIC_HEIGHT_ROT_MAX - 1)),
			-EINVAL, ERR, "hebcd_hreg_pic_height <= HFBC_PIC_HEIGHT_ROT_MAX - 1!\n");
	}

	dpu_check_and_return((layer->img.width & (hebcd_para->hebcd_block_width_align - 1)),
		-EINVAL, ERR, "layer->img.width and hebcd_block_width_align - 1 not match!\n");
	dpu_check_and_return((layer->img.height & (hebcd_para->hebcd_block_height_align - 1)),
		-EINVAL, ERR, "layer->img.height and hebcd_block_height_align - 1 not match!\n");

	return 0;
}

static int dpu_hebcd_get_encoded_para(struct dss_hebcd_para *hebcd_para,
	const dss_layer_t *layer, dss_rect_ltrb_t *aligned_rect)
{
	int aligned_pixel;

	aligned_pixel = dpu_hebcd_get_aligned_pixel(hebcd_para, layer);
	dpu_check_and_return((aligned_pixel == 0), -1, ERR, "aligned_pixel is zero\n");
	/* aligned rect */
	if (dpu_hebcd_get_hreg_pic_size(hebcd_para, layer, aligned_rect) != 0)
		return -EINVAL;

	hebcd_para->rdma_oft_x0 = aligned_rect->left / aligned_pixel;
	hebcd_para->rdma_oft_x1 = aligned_rect->right / aligned_pixel;

	hebcd_para->hebcd_header_stride0 = layer->img.hebc_header_stride0;
	hebcd_para->hebcd_header_offset = (aligned_rect->top / hebcd_para->hebcd_block_height_align) *
		hebcd_para->hebcd_header_stride0 + (aligned_rect->left / hebcd_para->hebcd_block_width_align) *
		HEBC_HEADER_STRIDE_BLOCK;

	hebcd_para->hebcd_header_addr0 = layer->img.hebc_header_addr0 + hebcd_para->hebcd_header_offset;

	dpu_check_and_return((hebcd_para->hebcd_header_addr0 & (HEBC_HEADER_ADDR_ALIGN - 1)), -EINVAL, ERR,
		"hebcd_header_addr0 and HEBC_HEADER_ADDR_ALIGN - 1 not match!\n");
	dpu_check_and_return((hebcd_para->hebcd_header_stride0 & (HEBC_HEADER_STRIDE_ALIGN - 1)), -EINVAL, ERR,
		"hebcd_header_stride0 and HEBC_HEADER_STRIDE_ALIGN - 1 not match!\n");

	if (hebcd_para->is_yuv_semi_planar) {
		hebcd_para->hebcd_header_addr1 = layer->img.hebc_header_addr1 + hebcd_para->hebcd_header_offset;
		hebcd_para->hebcd_header_stride1 = hebcd_para->hebcd_header_stride0;

		dpu_check_and_return((hebcd_para->hebcd_header_addr1 & (HEBC_HEADER_ADDR_ALIGN - 1)), -EINVAL, ERR,
			"hebcd_header_addr1 and HEBC_HEADER_ADDR_ALIGN - 1 not match!\n");
	}

	hebcd_para->hebcd_payload0_align = HEBC_PAYLOAD_512B_ALIGN;
	hebcd_para->hebcd_payload_stride0 = layer->img.hebc_payload_stride0;
	hebcd_para->hebcd_payload_addr0 = layer->img.hebc_payload_addr0 +
		(aligned_rect->top / hebcd_para->hebcd_block_height_align) * hebcd_para->hebcd_payload_stride0 +
		(aligned_rect->left / hebcd_para->hebcd_block_width_align) * hebcd_para->hebcd_payload0_align;

	dpu_check_and_return((hebcd_para->hebcd_payload_addr0 & (hebcd_para->hebcd_payload0_align - 1)), -EINVAL, ERR,
		"hebcd_payload_addr0 and hebce_payload0_align - 1 not match!\n");
	dpu_check_and_return((hebcd_para->hebcd_payload_stride0 & (hebcd_para->hebcd_payload0_align - 1)),
		-EINVAL, ERR, "hebcd_payload_stride0 and hebce_payload0_align - 1 not match!\n");

	if (hebcd_para->is_yuv_semi_planar) {
		hebcd_para->hebcd_payload1_align = HEBC_PAYLOAD_512B_ALIGN;
		hebcd_para->hebcd_payload_stride1 = layer->img.hebc_payload_stride1;
		hebcd_para->hebcd_payload_addr1 = layer->img.hebc_payload_addr1 +
			(aligned_rect->top / hebcd_para->hebcd_block_height_align) * hebcd_para->hebcd_payload_stride1 +
			(aligned_rect->left / hebcd_para->hebcd_block_width_align) * hebcd_para->hebcd_payload1_align;

		dpu_check_and_return((hebcd_para->hebcd_payload_addr1 & (hebcd_para->hebcd_payload1_align - 1)),
			-EINVAL, ERR, "hebcd_payload_addr1 and hebcd_payload1_align - 1 not match!\n");
		dpu_check_and_return((hebcd_para->hebcd_payload_stride1 & (hebcd_para->hebcd_payload1_align - 1)),
			-EINVAL, ERR, "hebcd_payload_stride1 and hebcd_payload1_align - 1 not match!\n");
	}

	return 0;
}

static int dpu_hebcd_get_out_rect(struct dss_hebcd_para *hebcd_para,
	dss_layer_t *layer, dss_rect_ltrb_t *aligned_rect,
	dss_rect_ltrb_t *clip_rect, dss_rect_t *out_aligned_rect)
{
	dss_rect_t src_rect;
	uint32_t src_bottom;
	uint32_t src_right;

	src_rect = layer->src_rect;
	src_bottom = (uint32_t)src_rect.y + src_rect.h;
	src_right = (uint32_t)src_rect.x + src_rect.w;

	if (layer->transform & DPU_FB_TRANSFORM_ROT_90) {
		aligned_rect->left = ALIGN_DOWN((uint32_t)src_rect.y, hebcd_para->hebcd_block_height_align);
		aligned_rect->right = ALIGN_UP(src_bottom, hebcd_para->hebcd_block_height_align) - 1;
		aligned_rect->top = ALIGN_DOWN((uint32_t)src_rect.x, hebcd_para->hebcd_block_width_align);
		aligned_rect->bottom = ALIGN_UP(src_right, hebcd_para->hebcd_block_width_align) - 1;
	}

	/* out_aligned_rect */
	out_aligned_rect->x = 0;
	out_aligned_rect->y = 0;
	out_aligned_rect->w = aligned_rect->right - aligned_rect->left + 1;
	out_aligned_rect->h = aligned_rect->bottom - aligned_rect->top + 1;
	hebcd_para->hebcd_height_bf_str = aligned_rect->bottom - aligned_rect->top + 1;

	/* rdfc clip_rect */
	if (layer->transform & DPU_FB_TRANSFORM_ROT_90) {
		clip_rect->left = src_rect.y - aligned_rect->left;
		clip_rect->right = aligned_rect->right - DSS_WIDTH(src_bottom);
		clip_rect->top = src_rect.x - aligned_rect->top;
		clip_rect->bottom = aligned_rect->bottom - DSS_HEIGHT(src_right);
	} else {
		clip_rect->left = src_rect.x - aligned_rect->left;
		clip_rect->right = aligned_rect->right - DSS_WIDTH(src_right);
		clip_rect->top = src_rect.y - aligned_rect->top;
		clip_rect->bottom = aligned_rect->bottom - DSS_HEIGHT(src_bottom);
	}

	if (dpu_adjust_clip_rect(layer, clip_rect) < 0) {
		DPU_FB_ERR("clip rect invalid => layer_idx=%d, chn_idx=%d, clip_rect %d, %d, %d, %d.\n",
			layer->layer_idx, layer->chn_idx, clip_rect->left, clip_rect->right,
			clip_rect->top, clip_rect->bottom);
		return -EINVAL;
	}

	hebcd_para->hebcd_top_crop_num = ((uint32_t)clip_rect->top > hebcd_para->hebcd_crop_num_max) ?  /*lint !e574*/
		hebcd_para->hebcd_crop_num_max : (uint32_t)clip_rect->top;
	hebcd_para->hebcd_bottom_crop_num = ((uint32_t)clip_rect->bottom > hebcd_para->hebcd_crop_num_max) ?  /*lint !e574*/
		hebcd_para->hebcd_crop_num_max : (uint32_t)clip_rect->bottom;

	clip_rect->top -= (int32_t)(hebcd_para->hebcd_top_crop_num);
	clip_rect->bottom -= (int32_t)(hebcd_para->hebcd_bottom_crop_num);

	/* adjust out_aligned_rect */
	out_aligned_rect->h -= (hebcd_para->hebcd_top_crop_num + hebcd_para->hebcd_bottom_crop_num);
	hebcd_para->stretch_size_vrt = DSS_HEIGHT(out_aligned_rect->h);

	return 0;
}

static void dpu_hebcd_generate_reg_value(struct dpu_fb_data_type *dpufd, dss_layer_t *layer,
	const struct dss_hebcd_para *hebcd_para, bool rdma_stretch_enable)
{
	dss_rdma_t *dma = NULL;
	int chn_idx;

	chn_idx = layer->chn_idx;
	dma = &(dpufd->dss_module.rdma[chn_idx]);
	dpufd->dss_module.dma_used[chn_idx] = 1;

	dma->hebcd_used = 1;
	dma->oft_x0 = set_bits32(dma->oft_x0, hebcd_para->rdma_oft_x0, 16, 0);
	dma->oft_x1 = set_bits32(dma->oft_x1, hebcd_para->rdma_oft_x1, 16, 0);
	dma->stretch_size_vrt = set_bits32(dma->stretch_size_vrt, (hebcd_para->stretch_size_vrt | (0 << 13)), 19, 0);
	dma->ctrl = set_bits32(dma->ctrl, hebcd_para->rdma_format, 5, 3);
	dma->ctrl = set_bits32(dma->ctrl, (hebcd_para->mmu_enable ? 0x1 : 0x0), 1, 8);
	dma->ctrl = set_bits32(dma->ctrl, hebcd_para->rdma_transform, 3, 9);
	dma->ctrl = set_bits32(dma->ctrl, (rdma_stretch_enable ? 1 : 0), 1, 12);
	dma->ch_ctl = set_bits32(dma->ch_ctl, 0x1, 1, 0);
	dma->ch_ctl = set_bits32(dma->ch_ctl, 0x1, 1, 2);

	dma->hebcd_hreg_hdr_ptr_l0 = set_bits32(dma->hebcd_hreg_hdr_ptr_l0, hebcd_para->hebcd_header_addr0, 32, 0);
	dma->hebcd_hreg_pic_width = set_bits32(dma->hebcd_hreg_pic_width, hebcd_para->hebcd_hreg_pic_width, 16, 0);
	dma->hebcd_hreg_pic_height = set_bits32(dma->hebcd_hreg_pic_height, hebcd_para->hebcd_hreg_pic_height, 16, 0);
	dma->hebcd_hreg_format = set_bits32(dma->hebcd_hreg_format, hebcd_para->color_transform, 1, 21);
	dma->hebcd_line_crop = set_bits32(dma->hebcd_line_crop,
			((hebcd_para->hebcd_top_crop_num << 8) | hebcd_para->hebcd_bottom_crop_num), 16, 0);
	dma->hebcd_input_header_stride0 = set_bits32(dma->hebcd_input_header_stride0,
		hebcd_para->hebcd_header_stride0, 14, 0);
	dma->hebcd_payload_stride0 = set_bits32(dma->hebcd_payload_stride0, hebcd_para->hebcd_payload_stride0, 20, 0);
	dma->hebcd_mm_base0_y8 = set_bits32(dma->hebcd_mm_base0_y8, hebcd_para->mm_base0_y8, 32, 0);
	dma->hebcd_payload_pointer = set_bits32(dma->hebcd_payload_pointer, hebcd_para->hebcd_payload_addr0, 32, 0);
	dma->hebcd_height_bf_str = set_bits32(dma->hebcd_height_bf_str,
		DSS_HEIGHT(hebcd_para->hebcd_height_bf_str), 16, 0);
	dma->hebcd_scramble_mode = set_bits32(dma->hebcd_scramble_mode, layer->img.hebc_scramble_mode, 4, 8);
	dma->hebcd_creg_fbcd_ctrl_mode = set_bits32(dma->hebcd_creg_fbcd_ctrl_mode, 0x10, 32, 0);
	dma->hebcd_hreg_hdr_ptr_l1 = set_bits32(dma->hebcd_hreg_hdr_ptr_l1, hebcd_para->hebcd_header_addr1, 32, 0);
	dma->hebcd_hreg_pld_ptr_l1 = set_bits32(dma->hebcd_hreg_pld_ptr_l1, hebcd_para->hebcd_payload_addr1, 32, 0);
	dma->hebcd_header_stride1 = set_bits32(dma->hebcd_header_stride1, hebcd_para->hebcd_header_stride1, 14, 0);
	dma->hebcd_payload_stride1 = set_bits32(dma->hebcd_payload_stride1, hebcd_para->hebcd_payload_stride1, 20, 0);
	dma->hebcd_block_type = set_bits32(dma->hebcd_block_type, hebcd_para->hebcd_block_type, 2, 0);
	dma->hebcd_mm_base1_c8 = set_bits32(dma->hebcd_mm_base1_c8, hebcd_para->mm_base1_c8, 32, 0);

	if ((g_debug_ovl_online_composer > 0) || (g_debug_ovl_offline_composer > 0))
		DPU_FB_INFO("fb%d, base0_y8=0x%x, base1_c8=0x%x, "
			"base0_y8_size=%d, base1_c8_size=%d, block_type=%d!\n",
			dpufd->index, hebcd_para->mm_base0_y8, hebcd_para->mm_base1_c8,
			layer->img.hebc_mmbuf_size0_y8, layer->img.hebc_mmbuf_size1_c8, hebcd_para->hebcd_block_type);
}

int dpu_hebcd_config(struct dpu_fb_data_type *dpufd, int ovl_idx, dss_layer_t *layer,
	struct dpu_ov_compose_rect *ov_compose_rect,
	struct dpu_ov_compose_flag *ov_compose_flag)
{
	struct dss_hebcd_para hebcd_para = {0};
	dss_rect_ltrb_t aligned_rect = { 0, 0, 0, 0 };
	int ret;

	if (!dpufd || !layer)
		return -EINVAL;

	if (!ov_compose_rect || !ov_compose_flag)
		return -EINVAL;

	if (!ov_compose_rect->aligned_rect || !ov_compose_rect->clip_rect)
		return -EINVAL;

	ret = dpu_hebcd_calc_base_para(dpufd, layer, &hebcd_para);
	if (ret)
		return -EINVAL;

	ret = dpu_hebcd_using_base_para(dpufd, ovl_idx, layer, &hebcd_para);
	if (ret)
		return -EINVAL;

	ret = dpu_hebcd_get_align_info(&hebcd_para);
	if (ret)
		return -EINVAL;

	ret = dpu_hebcd_get_encoded_para(&hebcd_para, layer, &aligned_rect);
	if (ret)
		return -EINVAL;

	ret = dpu_hebcd_get_out_rect(&hebcd_para, layer, &aligned_rect, ov_compose_rect->clip_rect,
		ov_compose_rect->aligned_rect);
	if (ret)
		return -EINVAL;

	ov_compose_flag->rdma_stretch_enable = (hebcd_para.stretched_line_num > 0) ? true : false;

	dpu_hebcd_generate_reg_value(dpufd, layer, &hebcd_para, ov_compose_flag->rdma_stretch_enable);

	return 0;
}

void dpu_hebce_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *wdma_base, dss_wdma_t *s_wdma)
{
	if (!dpufd || !wdma_base || !s_wdma) {
		DPU_FB_DEBUG("dpufd or wdma_base, s_wdma is NULL!\n");
		return;
	}

	dpufd->set_reg(dpufd, wdma_base + AFBCE_HREG_PIC_BLKS,
		s_wdma->hebce_hreg_pic_blks, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + AFBCE_HREG_FORMAT,
		s_wdma->hebce_hreg_format, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + AFBCE_HREG_HDR_PTR_L0,
		s_wdma->hebce_hreg_hdr_ptr_l0, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + AFBCE_HREG_PLD_PTR_L0,
		s_wdma->hebce_hreg_pld_ptr_l0, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + AFBCE_PICTURE_SIZE,
		s_wdma->hebce_picture_size, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + AFBCE_HEADER_SRTIDE,
		s_wdma->hebce_header_stride0, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + AFBCE_PAYLOAD_STRIDE,
		s_wdma->hebce_payload_stride0, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + AFBCE_SCRAMBLE_MODE,
		s_wdma->hebce_scramble_mode, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + AFBCE_CREG_FBCE_CTRL_MODE,
		s_wdma->fbce_creg_fbce_ctrl_mode, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + AFBCE_HREG_HDR_PTR_L1,
		s_wdma->hebce_hreg_hdr_ptr_l1, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + AFBCE_HREG_PLD_PTR_L1,
		s_wdma->hebce_hreg_pld_ptr_l1, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + AFBCE_HEADER_SRTIDE_1,
		s_wdma->hebce_header_stride1, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + AFBCE_PAYLOAD_SRTIDE_1,
		s_wdma->hebce_payload_stride1, 32, 0);
	dpufd->set_reg(dpufd, wdma_base + HEBCE_SPBLOCK_MODE,
		s_wdma->hebce_spblock_mode, 32, 0);
}

static int dpu_hebce_get_aligned_boundary(const dss_wb_layer_t *layer, int wdma_format,
		bool is_yuv, struct dss_hebce_para *hebce_para)
{
	uint32_t hebce_block_width_align;
	uint32_t hebce_block_height_align;

	if (layer->transform & DPU_FB_TRANSFORM_ROT_90) {
		if (is_yuv) {
			if (wdma_format == DMA_PIXEL_FORMAT_YUV420_SP_10BIT) {
				hebce_block_width_align = HEBC_BLOCK1_YUV_WIDTH_ALIGN;
				hebce_block_height_align = HEBC_BLOCK1_YUV_HEIGHT_ALIGN;
			} else {
				hebce_block_width_align = HEBC_BLOCK1_YUV_WIDTH_ALIGN;
				hebce_block_height_align = HEBC_BLOCK1_YUV_HEIGHT_ALIGN;
			}
		} else {
			hebce_block_width_align = HEBC_BLOCK1_RGB_WIDTH_ALIGN;
			hebce_block_height_align = HEBC_BLOCK1_RGB_HEIGHT_ALIGN;
		}
	} else {
		if (is_yuv) {
			if (wdma_format == DMA_PIXEL_FORMAT_YUV420_SP_10BIT) {
				hebce_block_width_align = 32;
				hebce_block_height_align = 16;
			} else {
				hebce_block_width_align = HEBC_BLOCK0_YUV_WIDTH_ALIGN;
				hebce_block_height_align = HEBC_BLOCK0_YUV_HEIGHT_ALIGN;
			}
		} else {
			hebce_block_width_align = HEBC_BLOCK0_RGB_WIDTH_ALIGN;
			hebce_block_height_align = HEBC_BLOCK0_RGB_HEIGHT_ALIGN;
		}
	}

	if ((layer->dst.width & (hebce_block_width_align - 1)) ||
		(layer->dst.height & (hebce_block_height_align - 1))) {
		DPU_FB_ERR("wb_layer dst width %d is not %d bytes aligned, or "
			"img heigh%d is not %d bytes aligned!\n",
			layer->dst.width, hebce_block_width_align,
			layer->dst.height, hebce_block_height_align);
		return -EINVAL;
	}

	hebce_para->hebce_block_width_align = hebce_block_width_align;
	hebce_para->hebce_block_height_align = hebce_block_height_align;

	return 0;
}

static const struct hebce_rect_border g_hebce_rect_border[ALIGN_TYPE_MAX] = {
	/* ROT90_YUV */
	{ HEBC_YUV_WIDTH_ROT_MIN, HEBC_YUV_WIDTH_ROT_MAX, HEBC_BLOCK1_YUV_HEIGHT_ALIGN,
	 HEBC_YUV_HEIGHT_ROT_MIN, HEBC_YUV_HEIGHT_ROT_MAX, HEBC_BLOCK1_YUV_WIDTH_ALIGN },
	/* ROT90_NO_YUV */
	{ HEBC_RGB_WIDTH_ROT_MIN, HEBC_RGB_WIDTH_ROT_MAX, HEBC_BLOCK1_RGB_HEIGHT_ALIGN,
	HEBC_RGB_HEIGHT_ROT_MIN, HEBC_RGB_HEIGHT_ROT_MAX, HEBC_BLOCK1_RGB_WIDTH_ALIGN },
	/* ROT0_YUV */
	{ HEBC_YUV_WIDTH_MIN, HEBC_YUV_WIDTH_MAX, HEBC_BLOCK0_YUV_WIDTH_ALIGN,
	HEBC_YUV_HEIGHT_MIN, HEBC_YUV_HEIGHT_MAX, HEBC_BLOCK0_YUV_HEIGHT_ALIGN },
	/* ROT0_NO_YUV */
	{ HEBC_RGB_WIDTH_MIN, HEBC_RGB_WIDTH_MAX, HEBC_BLOCK0_RGB_WIDTH_ALIGN,
	HEBC_RGB_HEIGHT_MIN, HEBC_RGB_HEIGHT_MAX, HEBC_BLOCK0_RGB_HEIGHT_ALIGN }
};

static bool hebce_check_rect_border(const dss_rect_t in_rect, enum hebce_rect_align_type type)
{
	return ((in_rect.w < g_hebce_rect_border[type].width_min) ||
		(in_rect.w > g_hebce_rect_border[type].width_max) ||
		(in_rect.h < g_hebce_rect_border[type].heigth_mix) ||
		(in_rect.h > g_hebce_rect_border[type].heigth_max) ||
		((uint32_t)in_rect.w & (uint32_t)(g_hebce_rect_border[type].width_alion - 1)) ||
		((uint32_t)in_rect.h & (uint32_t)(g_hebce_rect_border[type].heigth_align - 1)));
}

static int dpu_hebce_check_aligned_rect(const dss_rect_t in_rect, uint32_t transform, bool is_yuv)
{
	if (transform & DPU_FB_TRANSFORM_ROT_90) {
		if (is_yuv) {
			if (hebce_check_rect_border(in_rect, ROT90_YUV))
				return -EINVAL;
		} else {
			if (hebce_check_rect_border(in_rect, ROT90_NO_YUV)) {
				DPU_FB_ERR("hebce in_rect %d,%d, %d,%d is out of range!\n",
					in_rect.x, in_rect.y, in_rect.w, in_rect.h);
				return -EINVAL;
			}
		}
	} else {
		if (is_yuv) {
			if (hebce_check_rect_border(in_rect, ROT0_YUV)) {
				DPU_FB_ERR("hebce in_rect %d,%d, %d,%d is out of range!\n",
					in_rect.x, in_rect.y, in_rect.w, in_rect.h);
				return -EINVAL;
			}
		} else {
			if (hebce_check_rect_border(in_rect, ROT0_NO_YUV)) {
				DPU_FB_ERR("hebce in_rect %d,%d, %d,%d is out of range!\n",
					in_rect.x, in_rect.y, in_rect.w, in_rect.h);
				return -EINVAL;
			}
		}
	}

	return 0;
}

static int dpu_hebce_calculate_parameter(struct dss_hebce_para *hebce_para,
	const dss_wb_layer_t *layer, bool is_yuv_format,
	dss_rect_ltrb_t hebce_header_rect, dss_rect_ltrb_t hebce_payload_rect)
{
	/* hebc payload */
	if (is_yuv_format) {
		hebce_para->hebce_payload0_align = HEBC_PAYLOAD_512B_ALIGN;
		hebce_para->hebce_payload1_align = HEBC_PAYLOAD_512B_ALIGN;
	} else {
		if (layer->dst.bpp == 4) /* 16bit RGB */
			hebce_para->hebce_payload0_align = HEBC_PAYLOAD_512B_ALIGN;
		else if (layer->dst.bpp == 2) /* 4bit RGB */
			hebce_para->hebce_payload0_align = HEBC_PAYLOAD_256B_ALIGN;
	}

	/* hebc header */
	hebce_para->hebce_header_stride0 = layer->dst.hebc_header_stride0;
	if (is_yuv_format) {
		/* width_align, height_align from dpu_hfbce_get_aligned_boundary(), not 0 */
		hebce_para->hebce_header_offset = (hebce_header_rect.top / hebce_para->hebce_block_height_align) *
			hebce_para->hebce_header_stride0 +
			(hebce_header_rect.left / hebce_para->hebce_block_width_align) * HEBC_HEADER_STRIDE_BLOCK;

		hebce_para->hebce_header_addr1 = layer->dst.hebc_header_addr1 + hebce_para->hebce_header_offset;
		hebce_para->hebce_header_stride1 = hebce_para->hebce_header_stride0;
		dpu_check_and_return((hebce_para->hebce_header_addr1 & (HEBC_HEADER_ADDR_ALIGN - 1)), -EINVAL, ERR,
			"hebce_header_addr1 and HEBC_HEADER_ADDR_ALIGN - 1 not match!\n");

		hebce_para->hebce_payload_stride1 = layer->dst.hebc_payload_stride1;
		hebce_para->hebce_payload_addr1 = layer->dst.hebc_payload_addr1 +
			(hebce_payload_rect.top / hebce_para->hebce_block_height_align) *
			hebce_para->hebce_payload_stride1 +
			(hebce_payload_rect.left / hebce_para->hebce_block_width_align) *
			hebce_para->hebce_payload1_align;

		dpu_check_and_return((hebce_para->hebce_payload_addr1 & (hebce_para->hebce_payload1_align - 1)),
			-EINVAL, ERR, "hebce_payload_addr1 and hebce_payload1_align - 1 not match!\n");
		dpu_check_and_return((hebce_para->hebce_payload_stride1 & (hebce_para->hebce_payload1_align - 1)),
			-EINVAL, ERR, "hebce_payload_stride1 and hebce_payload1_align - 1 not match!\n");

		hebce_para->hebce_block_height_align /= 2;
	}

	hebce_para->hebce_header_offset = (hebce_header_rect.top / hebce_para->hebce_block_height_align) *
		hebce_para->hebce_header_stride0 + (hebce_header_rect.left / hebce_para->hebce_block_width_align) *
		HEBC_HEADER_STRIDE_BLOCK;
	hebce_para->hebce_header_addr0 = layer->dst.hebc_header_addr0 + hebce_para->hebce_header_offset;

	dpu_check_and_return((hebce_para->hebce_header_addr0 & (HEBC_HEADER_ADDR_ALIGN - 1)), -EINVAL, ERR,
		"hebce_header_addr0 and HEBC_HEADER_ADDR_ALIGN - 1 not match!\n");
	dpu_check_and_return((hebce_para->hebce_header_stride0 & (HEBC_HEADER_STRIDE_ALIGN - 1)), -EINVAL, ERR,
		"hebce_header_stride0 and HEBC_HEADER_STRIDE_ALIGN - 1 not match!\n");

	hebce_para->hebce_payload_stride0 = layer->dst.hebc_payload_stride0;
	hebce_para->hebce_payload_addr0 = layer->dst.hebc_payload_addr0 +
		(hebce_payload_rect.top / hebce_para->hebce_block_height_align) *
		hebce_para->hebce_payload_stride0 + (hebce_payload_rect.left / hebce_para->hebce_block_width_align) *
		hebce_para->hebce_payload0_align;

	dpu_check_and_return((hebce_para->hebce_payload_addr0 & (hebce_para->hebce_payload0_align - 1)), -EINVAL, ERR,
		"hebce_header_addr0 and hebce_payload0_align - 1 not match!\n");
	dpu_check_and_return((hebce_para->hebce_payload_stride0 & (hebce_para->hebce_payload0_align - 1)),
		-EINVAL, ERR, "hebce_header_stride0 and hebce_payload0_align - 1 not match!\n");

	return 0;
}

static void dpu_hebce_generate_reg_value(struct dpu_fb_data_type *dpufd, const struct dss_hebce_para *hebce_para,
	const dss_wb_layer_t *layer, dss_rect_t in_rect, bool last_block)
{
	bool mmu_enable = false;
	dss_wdma_t *wdma = &(dpufd->dss_module.wdma[layer->chn_idx]);

	dpufd->dss_module.dma_used[layer->chn_idx] = 1;

	mmu_enable = (layer->dst.mmu_enable == 1) ? true : false;

	wdma->hebce_used = 1;
	wdma->ctrl = set_bits32(wdma->ctrl, hebce_para->wdma_format, 5, 3);
	wdma->ctrl = set_bits32(wdma->ctrl, (mmu_enable ? 0x1 : 0x0), 1, 8);
	wdma->ctrl = set_bits32(wdma->ctrl, hebce_para->wdma_transform, 3, 9);
	if (last_block)
		wdma->ch_ctl = set_bits32(wdma->ch_ctl, 0x1d, 5, 0);
	else
		wdma->ch_ctl = set_bits32(wdma->ch_ctl, 0xd, 5, 0);

	wdma->rot_size = set_bits32(wdma->rot_size, ((uint32_t)DSS_WIDTH(in_rect.w) |
		((uint32_t)DSS_HEIGHT(in_rect.h) << 16)), 32, 0);

	wdma->hebce_hreg_pic_blks = set_bits32(wdma->hebce_hreg_pic_blks, 0, 24, 0);

	/* color transform */
	wdma->hebce_hreg_format = set_bits32(wdma->hebce_hreg_format, hebce_para->color_transform, 1, 21);
	wdma->hebce_hreg_hdr_ptr_l0 = set_bits32(wdma->hebce_hreg_hdr_ptr_l0, hebce_para->hebce_header_addr0, 32, 0);
	wdma->hebce_hreg_pld_ptr_l0 = set_bits32(wdma->hebce_hreg_pld_ptr_l0, hebce_para->hebce_payload_addr0, 32, 0);
	wdma->hebce_picture_size = set_bits32(wdma->hebce_picture_size,
		(((uint32_t)DSS_WIDTH(in_rect.w) << 16) | (uint32_t)DSS_HEIGHT(in_rect.h)), 32, 0);
	wdma->hebce_header_stride0 = set_bits32(wdma->hebce_header_stride0, hebce_para->hebce_header_stride0, 14, 0);
	wdma->hebce_payload_stride0 = set_bits32(wdma->hebce_payload_stride0, hebce_para->hebce_payload_stride0, 20, 0);
	wdma->hebce_scramble_mode = set_bits32(wdma->hebce_scramble_mode, layer->dst.hebc_scramble_mode, 4, 6);

	/* HEBCE enable */
	wdma->fbce_creg_fbce_ctrl_mode = set_bits32(wdma->fbce_creg_fbce_ctrl_mode, 0x20, 32, 0);
	wdma->hebce_hreg_hdr_ptr_l1 = set_bits32(wdma->hebce_hreg_hdr_ptr_l1, hebce_para->hebce_header_addr1, 32, 0);
	wdma->hebce_hreg_pld_ptr_l1 = set_bits32(wdma->hebce_hreg_pld_ptr_l1, hebce_para->hebce_payload_addr1, 32, 0);
	wdma->hebce_header_stride1 = set_bits32(wdma->hebce_header_stride1, hebce_para->hebce_header_stride1, 14, 0);
	wdma->hebce_payload_stride1 = set_bits32(wdma->hebce_payload_stride1, hebce_para->hebce_payload_stride1, 20, 0);
	wdma->hebce_spblock_mode = set_bits32(wdma->hebce_spblock_mode, 0x1, 32, 0);

	if (g_debug_ovl_offline_composer)
		DPU_FB_INFO("aligned_rect %d,%d,%d,%d,hebce_header_offset=0x%x!\n",
			in_rect.x, in_rect.y, DSS_WIDTH(in_rect.x + in_rect.w), DSS_WIDTH(in_rect.y + in_rect.h),
			hebce_para->hebce_header_offset);
}

int dpu_hebce_config(struct dpu_fb_data_type *dpufd, dss_wb_layer_t *layer,
	dss_rect_t aligned_rect, dss_rect_t *ov_block_rect, bool last_block)
{
	struct dss_hebce_para hebce_para = {0};
	dss_rect_t in_rect = aligned_rect;
	dss_rect_ltrb_t hebce_header_rect = {0};
	dss_rect_ltrb_t hebce_payload_rect = {0};
	bool is_yuv_format = false;
	int ret;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is null\n");
	dpu_check_and_return(!layer, -EINVAL, ERR, "layer is null\n");
	dpu_check_and_return(!ov_block_rect, -EINVAL, ERR, "ov_block_rect is null\n");

	hebce_para.wdma_transform = dpu_transform_hal2dma(layer->transform, layer->chn_idx);
	dpu_check_and_return(hebce_para.wdma_transform < 0, -EINVAL, ERR, "dpu_transform_hal2dma failed!\n");

	hebce_para.wdma_format = dpu_pixel_format_hal2dma(layer->dst.format);
	dpu_check_and_return(hebce_para.wdma_format < 0, -EINVAL, ERR, "dpu_pixel_format_hal2dma failed!\n");

	hebce_para.color_transform = 1;
	if ((hebce_para.wdma_format == DMA_PIXEL_FORMAT_YUV_420_SP_HP) ||
		(hebce_para.wdma_format == DMA_PIXEL_FORMAT_YUV420_SP_10BIT)) {
		hebce_para.color_transform = 0;
		is_yuv_format = true;
	}

	ret = dpu_hebce_check_aligned_rect(in_rect, hebce_para.wdma_transform, is_yuv_format);
	if (ret)
		return -EINVAL;

	ret = dpu_hebce_get_aligned_boundary(layer, hebce_para.wdma_format, is_yuv_format, &hebce_para);
	if (ret)
		return -EINVAL;

	if (layer->transform & DPU_FB_TRANSFORM_ROT_90) {
		hebce_header_rect.left = ALIGN_DOWN((uint32_t)layer->dst_rect.x, hebce_para.hebce_block_width_align);
		hebce_header_rect.top = ALIGN_DOWN((uint32_t)(layer->dst_rect.y +
			(ov_block_rect->x - layer->dst_rect.x)), hebce_para.hebce_block_height_align);

		hebce_payload_rect.left = ALIGN_DOWN((uint32_t)layer->dst_rect.x, hebce_para.hebce_block_width_align);
		hebce_payload_rect.top = hebce_header_rect.top;
	} else {
		hebce_header_rect.left = ALIGN_DOWN((uint32_t)in_rect.x, hebce_para.hebce_block_width_align);
		hebce_header_rect.top = ALIGN_DOWN((uint32_t)in_rect.y, hebce_para.hebce_block_height_align);

		hebce_payload_rect.left = ALIGN_DOWN((uint32_t)in_rect.x, hebce_para.hebce_block_width_align);
		hebce_payload_rect.top = hebce_header_rect.top;
	}

	ret = dpu_hebce_calculate_parameter(&hebce_para, layer, is_yuv_format,
		hebce_header_rect, hebce_payload_rect);
	if (ret)
		return -EINVAL;

	dpu_hebce_generate_reg_value(dpufd, &hebce_para, layer, in_rect, last_block);

	return ret;
}

#endif