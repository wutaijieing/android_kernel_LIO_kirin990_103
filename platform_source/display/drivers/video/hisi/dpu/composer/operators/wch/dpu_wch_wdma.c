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

#include "hisi_comp_utils.h"
#include "hisi_disp_gadgets.h"
#include "hisi_composer_operator.h"
#include "dpu_offline_define.h"
#include "hisi_operator_tool.h"
#include "widget/hebc/dpu_hebce.h"

#define BITS_PER_BYTE 8
#define DMA_ALIGN_BYTES (128 / BITS_PER_BYTE)

struct dpu_wdma_config_param {
	int wdma_format;
	int wdma_transform;
	uint32_t data_num;
	uint32_t wdma_addr;
	uint32_t wdma_stride;
	uint32_t wdma_img_width;
	uint32_t wdma_img_height;
	struct disp_rect in_rect;
	bool mmu_enable;
	struct disp_rect *ov_block;
};

static uint32_t calculate_display_addr_wb(bool mmu_enable, struct disp_wb_layer *wb_layer,
	struct disp_rect aligned_rect, struct disp_rect *ov_block_rect, int add_type)
{
	uint32_t addr;
	uint32_t dst_addr = 0;
	uint32_t stride = 0;
	uint32_t offset;
	int left;
	int top;
	int bpp;

	if (wb_layer->transform & HISI_FB_TRANSFORM_ROT_90) {
		left = wb_layer->dst_rect.x;
		top = ov_block_rect->x - wb_layer->dst_rect.x + wb_layer->dst_rect.y;
	} else {
		left = aligned_rect.x;
		top = aligned_rect.y;
	}

	if (add_type == DPU_ADDR_PLANE0) {
		stride = wb_layer->dst.stride;
		dst_addr = mmu_enable ? wb_layer->dst.vir_addr : wb_layer->dst.phy_addr;
	} else if (add_type == DPU_ADDR_PLANE1) {
		stride = wb_layer->dst.stride_plane1;
		offset = wb_layer->dst.offset_plane1;
        disp_pr_info("offset = %d", offset);
		dst_addr = mmu_enable ? (wb_layer->dst.vir_addr + offset) : (wb_layer->dst.phy_addr + offset);
		top /= 2;  /* half valve of top */
	} else {
		disp_pr_err("NOT SUPPORT this add_type[%d].\n", add_type);
	}

	bpp = wb_layer->dst.bpp;
	addr = dst_addr + left * bpp + top * stride;

	disp_pr_info("addr=0x%x,dst_addr=0x%x,left=%d,top=%d,stride=%d,bpp=%d\n",
				addr, dst_addr, left, top, wb_layer->dst.stride, bpp);
	return addr;
}

void dpu_wdma_init(const char __iomem *wdma_base, struct dpu_wch_wdma *s_wdma)
{
	if (!wdma_base) {
		disp_pr_err("wdma_base is NULL\n");
		return;
	}
	if (!s_wdma) {
		disp_pr_err("s_wdma is NULL\n");
		return;
	}

	memset(s_wdma, 0, sizeof(struct dpu_wch_wdma));
	s_wdma->dma_ctrl0.value = inp32(DPU_WCH_DMA_CTRL0_ADDR(wdma_base));
	s_wdma->dma_ctrl1.value = inp32(DPU_WCH_DMA_CTRL1_ADDR(wdma_base));
	s_wdma->dma_img_size.value = inp32(DPU_WCH_DMA_IMG_SIZE_ADDR(wdma_base));
	s_wdma->dma_ptr0.value = inp32(DPU_WCH_DMA_PTR0_ADDR(wdma_base));
	s_wdma->dma_stride0.value = inp32(DPU_WCH_DMA_STRIDE0_ADDR(wdma_base));
	s_wdma->dma_ptr1.value = inp32(DPU_WCH_DMA_PTR1_ADDR(wdma_base));
	s_wdma->dma_stride1.value = inp32(DPU_WCH_DMA_STRIDE1_ADDR(wdma_base));
	s_wdma->dma_ptr2.value = inp32(DPU_WCH_DMA_PTR2_ADDR(wdma_base));
	s_wdma->dma_stride2.value = inp32(DPU_WCH_DMA_STRIDE2_ADDR(wdma_base));
	s_wdma->ch_ctl.value = inp32(DPU_WCH_CH_CTL_ADDR(wdma_base));
}

void dpu_wdma_set_reg(struct hisi_comp_operator *operator, char __iomem *wdma_base, struct dpu_wch_wdma *s_wdma)
{
	disp_check_and_no_retval((wdma_base == NULL), err, "wdma_base is NULL!\n");
	disp_check_and_no_retval((s_wdma == NULL), err, "s_wdma is NULL!\n");

	disp_pr_info("enter++++");
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_REG_DEFAULT_ADDR(wdma_base), 0x1);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_REG_DEFAULT_ADDR(wdma_base), 0x0);

	hisi_module_set_reg(&operator->module_desc, DPU_WCH_DMA_CTRL0_ADDR(wdma_base), s_wdma->dma_ctrl0.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_DMA_CTRL1_ADDR(wdma_base), s_wdma->dma_ctrl1.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_DMA_IMG_SIZE_ADDR(wdma_base), s_wdma->dma_img_size.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_DMA_PTR0_ADDR(wdma_base), s_wdma->dma_ptr0.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_DMA_STRIDE0_ADDR(wdma_base), s_wdma->dma_stride0.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_DMA_PTR1_ADDR(wdma_base), s_wdma->dma_ptr1.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_DMA_STRIDE1_ADDR(wdma_base), s_wdma->dma_stride1.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_DMA_PTR2_ADDR(wdma_base), s_wdma->dma_ptr2.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_DMA_STRIDE2_ADDR(wdma_base), s_wdma->dma_stride2.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_CH_CTL_ADDR(wdma_base), s_wdma->ch_ctl.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_ROT_SIZE_ADDR(wdma_base), s_wdma->rot_size.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_DMA_CMP_CTRL_ADDR(wdma_base), s_wdma->dma_cmp_ctrl.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_DMA_PTR3_ADDR(wdma_base), s_wdma->dma_ptr3.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_DMA_STRIDE3_ADDR(wdma_base), s_wdma->dma_stride3.value);

	disp_pr_info("exit----");
}

static int prepare_wdma_common_param(struct disp_wb_layer *layer, struct disp_rect aligned_rect,
	struct dpu_wdma_config_param *param)
{
	param->wdma_format = dpu_pixel_format_hal2dma(layer->dst.format, DPU_HAL2WDMA_PIXEL_FORMAT);
	disp_check_and_return((param->wdma_format < 0), -EINVAL, err, "hisi_pixel_format_hal2dma failed!\n");

	param->in_rect = aligned_rect;

	param->wdma_transform = dpu_transform_hal2dma(layer->transform, layer->wchn_idx);
	disp_check_and_return((param->wdma_transform < 0), -EINVAL, err, "hisi_transform_hal2dma failed!\n");

	param->mmu_enable = (layer->dst.mmu_enable == 1) ? true : false;
	disp_pr_debug("mmu_enable:%d", param->mmu_enable);
	param->wdma_addr = param->mmu_enable ? layer->dst.vir_addr : layer->dst.phy_addr;

	return 0;
}

static void adjust_wdma_rect(struct disp_wb_layer *layer, struct dpu_wdma_config_param *param)
{
	int temp;

	if (layer->transform & HISI_FB_TRANSFORM_ROT_90) {
		temp = param->in_rect.w;
		param->in_rect.w = param->in_rect.h;
		param->in_rect.h = temp;
	}
}

static void dpu_set_wdma(struct disp_wb_layer *layer, struct disp_rect aligned_rect,
	struct dpu_wdma_config_param *param, bool last_block, struct dpu_wch_wdma *wdma)
{
	if (layer->transform & HISI_FB_TRANSFORM_ROT_90) {
		wdma->rot_size.value = 0;
		wdma->rot_size.reg.rot_img_width = DPU_WIDTH(param->ov_block->w);
		wdma->rot_size.reg.rot_img_height = DPU_HEIGHT(aligned_rect.h);
		disp_pr_info("[DATA] in rot w = %d h = %d", param->ov_block->w, aligned_rect.h);
	}

	disp_pr_info("[DATA] in wdma w = %d h = %d", param->ov_block->w, param->ov_block->h);
	disp_pr_info("[DATA] align   w = %d h = %d", aligned_rect.w, aligned_rect.h);
	param->wdma_img_width = DPU_WIDTH(aligned_rect.w);
	param->wdma_img_height = DPU_HEIGHT(aligned_rect.h);
	/* Resolving the problem of writing back RGBA8888 */
	wdma->dma_ctrl1.value = set_bits32(wdma->dma_ctrl1.value, 0x10, 6, 7);
	wdma->dma_ctrl1.value = set_bits32(wdma->dma_ctrl1.value, param->wdma_format, 5, 0);
	if (layer->transform & HISI_FB_TRANSFORM_ROT_90)
		wdma->dma_ctrl0.value = set_bits32(wdma->dma_ctrl0.value, 1, 1, 4); /* enable rot mirror */
	else
		wdma->dma_ctrl0.value = set_bits32(wdma->dma_ctrl0.value, 0, 1, 4);

	wdma->dma_ctrl0.value = set_bits32(wdma->dma_ctrl0.value, (param->mmu_enable ? 0x1 : 0x0), 1, 3);
	wdma->dma_ctrl0.value = set_bits32(wdma->dma_ctrl0.value, layer->dst.compress_type, 3, 0);

	wdma->dma_ptr0.value = set_bits32(wdma->dma_ptr0.value, param->wdma_addr, 32, 0);
	wdma->dma_stride0.value = set_bits32(wdma->dma_stride0.value, param->wdma_stride, 23, 0);

	if (is_yuv_sp_420(layer->dst.format) || is_yuv_sp_422(layer->dst.format)) {
		param->wdma_addr = calculate_display_addr_wb(param->mmu_enable,
			layer, param->in_rect, param->ov_block, DPU_ADDR_PLANE1);
		param->wdma_stride = layer->dst.stride_plane1;
		wdma->dma_ptr1.value = set_bits32(wdma->dma_ptr1.value, param->wdma_addr, 32, 0);
		wdma->dma_stride1.value = set_bits32(wdma->dma_stride1.value, param->wdma_stride, 13, 0);
	}

	if (last_block)
		wdma->ch_ctl.value = set_bits32(wdma->ch_ctl.value, 1, 1, 4);
	else
		wdma->ch_ctl.value = set_bits32(wdma->ch_ctl.value, 0, 1, 4);

	wdma->ch_ctl.value = set_bits32(wdma->ch_ctl.value, 1, 1, 3);
	wdma->ch_ctl.value = set_bits32(wdma->ch_ctl.value, 1, 1, 0);

	wdma->dma_img_size.value = set_bits32(wdma->dma_img_size.value,
		param->wdma_img_width | (param->wdma_img_height << 16), 32, 0);
}

static void print_wdma_info(struct disp_rect aligned_rect, struct dpu_wdma_config_param *param)
{
	// TODO:
}

int dpu_wdma_config(struct post_offline_info *offline_info, struct hisi_composer_data *ov_data,
	struct disp_wb_layer *layer, struct disp_rect aligned_rect)
{
	struct dpu_wdma_config_param param = {0};
	struct dpu_wch_wdma *wdma = NULL;
	struct disp_rect *ov_block_rect = NULL;
	int ret;
	bool last_block;

	disp_check_and_return((offline_info == NULL || layer == NULL), -EINVAL, err, "NULL ptr.\n");
	disp_check_and_return((layer->dst.bpp == 0), -EINVAL, err, "zero bpp.\n");

	disp_pr_info("enter+++");
	last_block = offline_info->last_block;
	ov_block_rect = &offline_info->wb_ov_block_rect;
	param.ov_block = ov_block_rect;

	/* ov block now is the wb block dst which is after scaler and rotation */
	if (offline_info->wb_type == WB_COMPOSE_COPYBIT && g_debug_block_scl_rot)
		param.ov_block = &(layer->wb_block.dst_rect);

	if (g_debug_wch_scf && g_debug_wch_scf_rot) {
		param.ov_block->w = layer->dst_rect.h;
		param.ov_block->h = layer->dst_rect.w;
	} else if (g_debug_wch_scf)
		param.ov_block = &(layer->dst_rect);

	wdma = &(ov_data->offline_module.wdma[layer->wchn_idx]);
	ov_data->offline_module.dma_used[layer->wchn_idx] = 1;

	ret = prepare_wdma_common_param(layer, aligned_rect, &param);
	disp_check_and_return((ret < 0), -EINVAL, err, "prepare_wdma_common_param failed!\n");

	adjust_wdma_rect(layer, &param);

	param.wdma_addr = calculate_display_addr_wb(param.mmu_enable, layer, param.in_rect,
		param.ov_block, DPU_ADDR_PLANE0);
	param.wdma_stride = layer->dst.stride;

	dpu_set_wdma(layer, aligned_rect, &param, last_block, wdma);

	if (layer->dst.compress_type == COMPRESS_TYPE_HEBC)
		dpu_hebce_config(layer, aligned_rect, wdma, ov_block_rect);

	print_wdma_info(aligned_rect, &param);
	disp_pr_info("exit+++");
	return 0;
}

