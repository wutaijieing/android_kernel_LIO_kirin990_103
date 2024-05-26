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
#include "dpu_hebcd.h"
#include "hisi_disp_debug.h"
#include "hisi_disp_gadgets.h"
#include "hisi_operator_tool.h"
#include "hisi_disp_config.h"

static bool is_dma_pixel_format_10bit(int format)
{
	switch (format) {
	case DPU_SDMA_PIXEL_FORMAT_RGBA_1010102:
	case DPU_SDMA_PIXEL_FORMAT_YUYV_420_10BIT_SP:
		return true;
	default:
		return false;
	}
}

static int dpu_hebcd_get_aligned_pixel(struct hebcd_params *hebcd_para, const struct pipeline_src_layer *layer)
{
	int bpp;
	int aligned_pixel;
	bool is_pixel_10bit = false;

	is_pixel_10bit = is_dma_pixel_format_10bit(hebcd_para->dma_format);
	bpp = hebcd_para->is_yuv_semi_planar ? 1 : layer->img.bpp;
	if (is_pixel_10bit)
		bpp = layer->img.bpp;

	aligned_pixel = DMA_ALIGN_BYTES / bpp;

	return aligned_pixel;
}

static uint32_t dpu_init_hebcd_info(struct pipeline_src_layer *layer, struct hebcd_params *hebcd_para)
{
	hebcd_para->is_yuv_semi_planar = is_yuv_semiplanar(layer->img.format);
	hebcd_para->is_d3_128 = is_d3_128(layer->img.format);
	hebcd_para->hebcd_sblock_type = layer->img.hebcd_block_type;
	hebcd_para->tag_mode = HEBC_TAG_MODE;
	hebcd_para->scramble = layer->img.hebc_scramble_mode;
	hebcd_para->yuv_trans = HEBC_YUV_TRANS;
	hebcd_para->dma_format = dpu_pixel_format_hal2dma(layer->img.format, DPU_HAL2SDMA_PIXEL_FORMAT);
	if (hebcd_para->dma_format < 0) {
		disp_pr_err("layer format %d not support!\n", layer->img.format);
		return -EINVAL;
	}
	return 0;
}

static int dpu_hebcd_calc_aligned_rect(struct hebcd_params *hebcd_para,
			const struct pipeline_src_layer *layer, struct dpu_edit_rect *aligned_rect)
{
	uint32_t src_left = (uint32_t)layer->src_rect.x;
	uint32_t src_top = (uint32_t)layer->src_rect.y;
	uint32_t src_right = (uint32_t)layer->src_rect.x + layer->src_rect.w;
	uint32_t src_bottom = (uint32_t)layer->src_rect.y + layer->src_rect.h;

	aligned_rect->left = ALIGN_DOWN(src_left, hebcd_para->hebcd_block_width_align);
	aligned_rect->right = ALIGN_UP(src_right, hebcd_para->hebcd_block_width_align) - 1;
	aligned_rect->top = ALIGN_DOWN(src_top, hebcd_para->hebcd_block_height_align);
	aligned_rect->bottom = ALIGN_UP(src_bottom, hebcd_para->hebcd_block_height_align) - 1;

	if (layer->transform & HISI_FB_TRANSFORM_ROT_90) {
		aligned_rect->left = ALIGN_DOWN(src_top, hebcd_para->hebcd_block_height_align);
		aligned_rect->right = ALIGN_UP(src_bottom, hebcd_para->hebcd_block_height_align) - 1;
		aligned_rect->top = ALIGN_DOWN(src_left, hebcd_para->hebcd_block_width_align);
		aligned_rect->bottom = ALIGN_UP(src_right, hebcd_para->hebcd_block_width_align) - 1;
	}
	disp_pr_debug("[aligned_rect] left:%u right:%u top:%u bottom:%u \n", aligned_rect->left, aligned_rect->right,
				aligned_rect->top, aligned_rect->bottom);
	return 0;
}

static int dpu_hebcd_calc_basic_align_info(struct hebcd_params *hebcd_para)
{
	hebcd_para->hebcd_crop_num_max = HEBC_CROP_MAX;
	uint32_t width_align = 0;
	uint32_t height_align = 0;
	if ((hebcd_para->hebcd_sblock_type != SUPER_BLOCK_TYPE_NORMAL) && 
		(hebcd_para->hebcd_sblock_type != SUPER_BLOCK_TYPE_ROT)) {
			disp_pr_err("Not supported hebcd_sblock_type!\n");
			return -EINVAL;
	}
	dpu_hebc_get_basic_align_info(hebcd_para->is_yuv_semi_planar, hebcd_para->hebcd_sblock_type, hebcd_para->dma_format,
									&width_align, &height_align);
	hebcd_para->hebcd_block_width_align = width_align;
	hebcd_para->hebcd_block_height_align = height_align;
	
	disp_pr_debug("[calc_basic_align_info] width_align:%u height_align:%u \n", width_align, height_align);

	return 0;
}

static int dpu_hebcd_check_rot_constraint(struct pipeline_src_layer *layer, struct dpu_edit_rect *aligned_rect)
{
	uint32_t aligned_width;
	uint32_t aligned_height;

	if (aligned_rect->right < aligned_rect->left || aligned_rect->bottom < aligned_rect->top) {
		disp_pr_err("Wrong aligned rect!\n");
		return -EINVAL;
	}

	aligned_width = (uint32_t)(aligned_rect->right - aligned_rect->left);
	aligned_height = (uint32_t)(aligned_rect->bottom - aligned_rect->top);

	if (layer->transform & HISI_FB_TRANSFORM_ROT_90) {
		if (is_yuv_format(layer->img.format)) {
			disp_check_and_return((aligned_width > (HEBC_PIC_WIDTH_ROT_MAX_YUV- 1)), -EINVAL, err,
				"hebcd aligned width <= HEBC_PIC_WIDTH_ROT_MAX_YUV - 1!\n");
			disp_check_and_return((aligned_height > (HEBC_PIC_HEIGHT_ROT_MAX_YUV - 1)), -EINVAL, err,
				"hebcd aligned height <= HEBC_PIC_HEIGHT_ROT_MAX_YUV - 1!\n");
		} else {
			disp_check_and_return((aligned_width > (HEBC_PIC_WIDTH_ROT_MAX_RGB - 1)), -EINVAL, err,
				"hebcd aligned width <= HEBC_PIC_WIDTH_ROT_MAX_RGB - 1!\n");
			disp_check_and_return((aligned_height > (HEBC_PIC_HEIGHT_ROT_MAX_RGB - 1)), -EINVAL, err,
				"hebcd aligned height <= HEBC_PIC_HEIGHT_ROT_MAX_RGB - 1!\n");
		}
	}

	return 0;
}

static int dpu_hebcd_check_aligned_para(struct hebcd_params *hebcd_para)
{
	disp_check_and_return((hebcd_para->hebcd_planes[0].header_addr % HEBC_HEADER_ADDR_ALIGN), -EINVAL, err,
				"hebcd_header_addr0 and HEBC_HEADER_ADDR_ALIGN not aligned!\n");
	disp_check_and_return((hebcd_para->hebcd_planes[0].header_stride % HEBC_HEADER_STRIDE_ALIGN), -EINVAL, err,
				"hebcd_header_stride0 and HEBC_HEADER_STRIDE_ALIGN not aligned!\n");
	disp_check_and_return((hebcd_para->hebcd_planes[0].payload_addr % HEBC_PAYLOAD_ADDR_ALIGN), -EINVAL, err,
				"hebcd_payload_addr0 and HEBC_PAYLOAD_ADDR_ALIGN not aligned!\n");
	disp_check_and_return((hebcd_para->hebcd_planes[0].payload_stride % HEBC_PAYLOAD_STRIDE_ALIGN), -EINVAL, err,
				"hebcd_payload_stride0 and HEBC_PAYLOAD_STRIDE_ALIGN not aligned!\n");
	if (hebcd_para->is_yuv_semi_planar) {
		disp_check_and_return((hebcd_para->hebcd_planes[1].header_addr % HEBC_HEADER_ADDR_ALIGN), -EINVAL, err,
				"hebcd_header_addr1 and HEBC_HEADER_ADDR_ALIGN not aligned!\n");
        disp_check_and_return((hebcd_para->hebcd_planes[1].header_stride % HEBC_HEADER_STRIDE_ALIGN), -EINVAL, err,
				"hebcd_header_stride1 and HEBC_HEADER_STRIDE_ALIGN not aligned!\n");
		disp_check_and_return((hebcd_para->hebcd_planes[1].payload_addr % HEBC_PAYLOAD_ADDR_ALIGN), -EINVAL, err,
				"hebcd_payload_addr1 and HEBC_PAYLOAD_ADDR_ALIGN not aligned!\n");
		disp_check_and_return((hebcd_para->hebcd_planes[1].payload_stride % HEBC_PAYLOAD_STRIDE_ALIGN), -EINVAL, err,
				"hebcd_payload_stride1 and HEBC_PAYLOAD_STRIDE_ALIGN not aligned!\n");
	}
	return 0;
}

static int dpu_hebcd_check_maximum(struct hebcd_params *hebcd_para)
{
	disp_check_and_return((hebcd_para->hebcd_planes[0].header_stride > HEBC_RGB_HEADER_STRIDE_MAXIMUM), -EINVAL, err,
				"hebcd_header_stride0 can not exceed %u!\n", HEBC_RGB_HEADER_STRIDE_MAXIMUM);
	disp_check_and_return((hebcd_para->hebcd_planes[0].payload_stride > HEBC_RGB_PLD_STRIDE_MAXIMUM), -EINVAL, err,
				"hebcd_payload_stride0 can not exceed %u!\n", HEBC_RGB_PLD_STRIDE_MAXIMUM);
	if (hebcd_para->is_yuv_semi_planar) {
		disp_check_and_return((hebcd_para->hebcd_planes[1].header_stride > HEBC_C_HEADER_STRIDE_MAXIMUM), -EINVAL, err,
				"hebcd_header_stride1 can not exceed %u!\n", HEBC_C_HEADER_STRIDE_MAXIMUM);
        disp_check_and_return((hebcd_para->hebcd_planes[1].payload_stride > HEBC_C_PLD_STRIDE_MAXIMUM), -EINVAL, err,
				"hebcd_payload_stride1 can not exceed %u!\n", HEBC_C_PLD_STRIDE_MAXIMUM);
    }
	return 0;
}

static int dpu_hebcd_get_aligned_para(struct hebcd_params *hebcd_para,
									  const struct pipeline_src_layer *layer, struct dpu_edit_rect *aligned_rect)
{
	hebcd_para->hebcd_planes[0].header_stride = layer->img.hebc_planes[0].header_stride;
	hebcd_para->hebcd_planes[0].header_offset = (aligned_rect->top / hebcd_para->hebcd_block_height_align) *
				(uint64_t)hebcd_para->hebcd_planes[0].header_stride +
				(aligned_rect->left / hebcd_para->hebcd_block_width_align) *
				(uint64_t)HEBC_HEADER_SBLOCK_SIZE;
	hebcd_para->hebcd_planes[0].header_addr = layer->img.hebc_planes[0].header_addr +
			hebcd_para->hebcd_planes[0].header_offset;

	hebcd_para->hebcd_payload_align[0] = HEBC_PLD_SBLOCK_SIZE;
	hebcd_para->hebcd_planes[0].payload_stride = layer->img.hebc_planes[0].payload_stride;
    hebcd_para->hebcd_planes[0].payload_offset = (aligned_rect->top / hebcd_para->hebcd_block_height_align) *
				(uint64_t)hebcd_para->hebcd_planes[0].payload_stride +
				(aligned_rect->left / hebcd_para->hebcd_block_width_align) *
				(uint64_t)hebcd_para->hebcd_payload_align[0];
	hebcd_para->hebcd_planes[0].payload_addr = layer->img.hebc_planes[0].payload_addr +
			hebcd_para->hebcd_planes[0].payload_offset;

	if (hebcd_para->is_yuv_semi_planar || hebcd_para->is_d3_128) {
		/* plane 2's header is the same with plane 1 for NV12 & NV12_10bit */
		hebcd_para->hebcd_planes[1].header_stride = hebcd_para->hebcd_planes[0].header_stride;
		hebcd_para->hebcd_planes[1].header_addr = layer->img.hebc_planes[1].header_addr +
				hebcd_para->hebcd_planes[0].header_offset;

		hebcd_para->hebcd_payload_align[1] = HEBC_PLD_SBLOCK_SIZE;
		hebcd_para->hebcd_planes[1].payload_stride = layer->img.hebc_planes[1].payload_stride;
		hebcd_para->hebcd_planes[1].payload_addr = layer->img.hebc_planes[1].payload_addr +
		 		(aligned_rect->top / hebcd_para->hebcd_block_height_align) *
				(uint64_t)hebcd_para->hebcd_planes[1].payload_stride +
				(aligned_rect->left / hebcd_para->hebcd_block_width_align) *
				(uint64_t)hebcd_para->hebcd_payload_align[1];
	}
	return 0;
}

static int dpu_hebcd_get_out_rect(struct hebcd_params *hebcd_para, struct pipeline_src_layer *layer,
								  struct dpu_edit_rect *aligned_rect)
{
	struct disp_rect src_rect;
	uint32_t src_left;
	uint32_t src_top;
	uint32_t src_bottom;
	uint32_t src_right;
	struct disp_rect *out_aligned_rect = &hebcd_para->aligned_rect;
	struct dpu_edit_rect *clip_rect = &hebcd_para->clip_rect;

	src_rect = layer->src_rect;
	src_left = (uint32_t)src_rect.x;
	src_top = (uint32_t)src_rect.y;
	src_bottom = (uint32_t)src_rect.y + src_rect.h;
	src_right = (uint32_t)src_rect.x + src_rect.w;

	/* out_aligned_rect */
	out_aligned_rect->x = 0;
	out_aligned_rect->y = 0;
	out_aligned_rect->w = aligned_rect->right - aligned_rect->left + 1;
	out_aligned_rect->h = aligned_rect->bottom - aligned_rect->top + 1;

	/* dfc clip_rect */
	if (layer->transform & HISI_FB_TRANSFORM_ROT_90) {
		clip_rect->left = src_top - aligned_rect->left;
		clip_rect->right = aligned_rect->right - DPU_WIDTH(src_bottom);
		clip_rect->top = src_left - aligned_rect->top;
		clip_rect->bottom = aligned_rect->bottom - DPU_HEIGHT(src_right);
	} else {
		clip_rect->left = src_left - aligned_rect->left;
		clip_rect->right = aligned_rect->right - DPU_WIDTH(src_right);
		clip_rect->top = src_top - aligned_rect->top;
		clip_rect->bottom = aligned_rect->bottom - DPU_HEIGHT(src_bottom);
	}

	hebcd_para->hebcd_top_crop_num = (clip_rect->top > hebcd_para->hebcd_crop_num_max) ?
				hebcd_para->hebcd_crop_num_max : clip_rect->top;
	hebcd_para->hebcd_bottom_crop_num = (clip_rect->bottom > hebcd_para->hebcd_crop_num_max) ?
				hebcd_para->hebcd_crop_num_max : clip_rect->bottom;
	disp_pr_debug("[clip_rect] left:%u right:%u top:%u bottom:%u \n", clip_rect->left, clip_rect->right,
				clip_rect->top, clip_rect->bottom);
	clip_rect->top -= hebcd_para->hebcd_top_crop_num;
	clip_rect->bottom -= hebcd_para->hebcd_bottom_crop_num;

	disp_pr_debug("[crop_num] top:%u bottom:%u max:%u \n", hebcd_para->hebcd_top_crop_num,
				hebcd_para->hebcd_bottom_crop_num,
				hebcd_para->hebcd_crop_num_max);
	disp_pr_debug("[src_rect] x:%u y:%u w:%u h:%u \n", src_rect.x, src_rect.y, src_rect.w, src_rect.h);
	disp_pr_debug("[out_aligned_rect] x:%u y:%u w:%u h:%u \n", out_aligned_rect->x, out_aligned_rect->y,
				out_aligned_rect->w, out_aligned_rect->h);
	disp_pr_debug("[clip_rect] left:%u right:%u top:%u bottom:%u \n", clip_rect->left, clip_rect->right,
				clip_rect->top, clip_rect->bottom);
	/* adjust out_aligned_rect */
	out_aligned_rect->h -= (hebcd_para->hebcd_top_crop_num + hebcd_para->hebcd_bottom_crop_num);
	disp_pr_debug("[out_aligned_rect] x:%u y:%u w:%u h:%u \n", out_aligned_rect->x, out_aligned_rect->y,
				out_aligned_rect->w, out_aligned_rect->h);
	return 0;
}

static int dpu_hebcd_calc_sblk_type(struct hebcd_params *hebcd_para, const struct pipeline_src_layer *layer)
{
	if (!is_yuv_semiplanar(layer->img.format)) {
		hebcd_para->sblk_type = SBLOCK_TYPE_32X4;
		if (layer->transform & HISI_FB_TRANSFORM_ROT_90)
			hebcd_para->sblk_type = SBLOCK_TYPE_16X8_UP_DOWN;
		return 0;
	}

	if (layer->img.format == HISI_FB_PIXEL_FORMAT_YCBCR_420_SP) {
		hebcd_para->sblk_type = SBLOCK_TYPE_64X8;
		return 0;
	}

	if (layer->img.format == HISI_FB_PIXEL_FORMAT_YCBCR420_SP_10BIT) {
		hebcd_para->sblk_type = SBLOCK_TYPE_32X8;
		return 0;
	}
	disp_pr_err("Wrong pixel format(%u)!", layer->img.format);
	return -EINVAL;
}

static void dpu_sdma_set_hebc_info(struct hebcd_params *hebcd_para, struct hisi_dm_layer_info *layer_info)
{
	layer_info->layer_sblk_type_union.reg.layer_tag_mode_en = hebcd_para->tag_mode;
	layer_info->layer_sblk_type_union.reg.layer_yuv_trans = hebcd_para->yuv_trans;
	layer_info->layer_sblk_type_union.reg.layer_sblk_type = hebcd_para->sblk_type;
	layer_info->layer_sblk_type_union.reg.layer_scramble_mode = hebcd_para->scramble;
	disp_pr_debug("scramble:%u \n", layer_info->layer_sblk_type_union.reg.layer_scramble_mode);
	layer_info->layer_start_addr3_h_union.value = 0;
	layer_info->layer_start_addr3_h_union.reg.layer_start_addr0_h =
				(hebcd_para->hebcd_planes[0].payload_addr >> 32) & 0xF;
	layer_info->layer_start_addr3_h_union.reg.layer_start_addr1_h =
				(hebcd_para->hebcd_planes[0].header_addr >> 32) & 0xF;
	layer_info->layer_start_addr3_h_union.reg.layer_start_addr2_h =
				(hebcd_para->hebcd_planes[1].payload_addr >> 32) & 0xF;
	layer_info->layer_start_addr3_h_union.reg.layer_start_addr3_h =
				(hebcd_para->hebcd_planes[1].header_addr >> 32) & 0xF;
	disp_pr_debug(" header_addr_addr_h[0]:0x%x payload_addr_addr_h[0]:0x%x \n",
				layer_info->layer_start_addr3_h_union.reg.layer_start_addr1_h,
				layer_info->layer_start_addr3_h_union.reg.layer_start_addr0_h);
	disp_pr_debug(" header_addr_addr_h[1]:0x%x payload_addr_addr_h[1]:0x%x \n",
				layer_info->layer_start_addr3_h_union.reg.layer_start_addr3_h,
				layer_info->layer_start_addr3_h_union.reg.layer_start_addr2_h);
	layer_info->layer_start_addr0_l_union.value = hebcd_para->hebcd_planes[0].payload_addr & 0xFFFFFFFF;
	layer_info->layer_start_addr1_l_union.value = hebcd_para->hebcd_planes[0].header_addr & 0xFFFFFFFF;
	layer_info->layer_start_addr2_l_union.value = hebcd_para->hebcd_planes[1].payload_addr & 0xFFFFFFFF;
	layer_info->layer_start_addr3_l_union.value = hebcd_para->hebcd_planes[1].header_addr & 0xFFFFFFFF;
	layer_info->layer_stride0_union.value = hebcd_para->hebcd_planes[0].payload_stride;
	layer_info->layer_stride1_union.value = hebcd_para->hebcd_planes[0].header_stride;
	layer_info->layer_stride2_union.value = hebcd_para->hebcd_planes[1].payload_stride;
	layer_info->layer_rsv0_union.value = 0;
	layer_info->layer_rsv0_union.reg.layer_stride3 = hebcd_para->hebcd_planes[1].header_stride;
	disp_pr_debug(" header_addr[0]:0x%x payload_addr[0]:0x%x \n", hebcd_para->hebcd_planes[0].header_addr,
																 hebcd_para->hebcd_planes[0].payload_addr);
	disp_pr_debug(" header_addr[1]:0x%x payload_addr[1]:0x%x \n", hebcd_para->hebcd_planes[1].header_addr,
																 hebcd_para->hebcd_planes[1].payload_addr);
	disp_pr_debug(" header_stride[0]:0x%x payload_stride[0]:0x%x \n", hebcd_para->hebcd_planes[0].header_stride,
																 	 hebcd_para->hebcd_planes[0].payload_stride);
	disp_pr_debug(" header_stride[1]:0x%x payload_stride[1]:0x%x \n", hebcd_para->hebcd_planes[1].header_stride,
																 	 hebcd_para->hebcd_planes[1].payload_stride);
	/* wi and h are related to the calculation of stride during decoding and must be the same as that in WCH. */
	layer_info->layer_height_union.reg.layer_width = hebcd_para->aligned_rect.w - 1;
	layer_info->layer_height_union.reg.layer_height = hebcd_para->aligned_rect.h - 1;

	disp_pr_debug("set_hebc_info:layer_width=%d, layer_height = %d \n",
		layer_info->layer_height_union.reg.layer_width, layer_info->layer_height_union.reg.layer_height);
}

void hisi_sdma_hebcd_set_regs(struct hisi_comp_operator *operator, uint32_t layer_id,
									 struct hisi_dm_layer_info * layer_info, char __iomem * dpu_base_addr, struct pipeline_src_layer *layer)
{
	char __iomem *dm_base = dpu_base_addr + dpu_get_dm_offset(operator->scene_id);
	disp_pr_debug(" dm_base:0x%p, layer_id:%u \n", dm_base, layer_id);

	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_SBLK_TYPE_ADDR(dm_base, layer_id),
				layer_info->layer_sblk_type_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_START_ADDR3_H_ADDR(dm_base, layer_id),
				layer_info->layer_start_addr3_h_union.value);

	if (!(layer->need_caps & CAP_WLT)) {
		hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_START_ADDR0_L_ADDR(dm_base, layer_id),
				layer_info->layer_start_addr0_l_union.value);
		hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_START_ADDR1_L_ADDR(dm_base, layer_id),
				layer_info->layer_start_addr1_l_union.value);
		hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_START_ADDR2_L_ADDR(dm_base, layer_id),
				layer_info->layer_start_addr2_l_union.value);
		hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_START_ADDR3_L_ADDR(dm_base, layer_id),
				layer_info->layer_start_addr3_l_union.value);
	} else {
		disp_pr_info("wlt mode no need set dm sdma addr");
	}
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_STRIDE0_ADDR(dm_base, layer_id),
				layer_info->layer_stride0_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_STRIDE1_ADDR(dm_base, layer_id),
				layer_info->layer_stride1_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_STRIDE2_ADDR(dm_base, layer_id),
				layer_info->layer_stride2_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_RSV0_ADDR(dm_base, layer_id),
				layer_info->layer_rsv0_union.value);
}

int dpu_hebcd_calc(struct pipeline_src_layer *layer, struct hisi_dm_layer_info *layer_info)
{
	struct hebcd_params hebcd_para = {0};
	struct dpu_edit_rect aligned_rect = {0, 0, 0, 0};

	disp_pr_debug(" ++++ \n");
	if (dpu_init_hebcd_info(layer, &hebcd_para))
		return -EINVAL;

	if (dpu_hebcd_calc_basic_align_info(&hebcd_para))
		return -EINVAL;

	if (dpu_hebcd_calc_aligned_rect(&hebcd_para, layer, &aligned_rect))
		return -EINVAL;

	if (dpu_hebcd_check_rot_constraint(layer, &aligned_rect))
		return -EINVAL;

	dpu_hebcd_get_aligned_para(&hebcd_para, layer, &aligned_rect);

	if (dpu_hebcd_check_aligned_para(&hebcd_para))
		return -EINVAL;

	if (dpu_hebcd_check_maximum(&hebcd_para))
		return -EINVAL;

	if (dpu_hebcd_calc_sblk_type(&hebcd_para, layer))
		return -EINVAL;

	dpu_hebcd_get_out_rect(&hebcd_para, layer, &aligned_rect);
	dpu_sdma_set_hebc_info(&hebcd_para, layer_info);

	disp_pr_debug(" ---- \n");
	return 0;
}