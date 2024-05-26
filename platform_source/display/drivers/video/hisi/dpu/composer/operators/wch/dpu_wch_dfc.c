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

#include "hisi_operator_tool.h"
#include "hisi_disp_gadgets.h"
#include "hisi_disp_composer.h"
#include "dpu_wch_dfc.h"
#include "../widget/hebc/dpu_hebc.h"

int g_debug_ovl_offline_composer = 1;

struct rect_pad {
	uint32_t left;
	uint32_t right;
	uint32_t top;
	uint32_t bottom;
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

void dpu_wdfc_init(const char __iomem *wch_base, struct dpu_wdfc *s_dfc)
{
	if (wch_base == NULL) {
		disp_pr_err("wch_base is NULL\n");
		return;
	}
	if (s_dfc == NULL) {
		disp_pr_err("s_dfc is NULL\n");
		return;
	}

	memset(s_dfc, 0, sizeof(struct dpu_wdfc));

	s_dfc->disp_size.value = inp32(DPU_WCH_DFC_DISP_SIZE_ADDR(wch_base));
	s_dfc->pix_in_num.value = inp32(DPU_WCH_DFC_PIX_IN_NUM_ADDR(wch_base));
	s_dfc->disp_fmt.value = inp32(DPU_WCH_DFC_DISP_FMT_ADDR(wch_base));
	s_dfc->clip_ctl_hrz.value = inp32(DPU_WCH_DFC_CLIP_CTL_HRZ_ADDR(wch_base));
	s_dfc->clip_ctl_vrz.value = inp32(DPU_WCH_DFC_CLIP_CTL_VRZ_ADDR(wch_base));
	s_dfc->ctl_clip_en.value = inp32(DPU_WCH_DFC_CTL_CLIP_EN_ADDR(wch_base));
	s_dfc->icg_module.value = inp32(DPU_WCH_DFC_ICG_MODULE_ADDR(wch_base));
	s_dfc->dither_enable.value = inp32(DPU_WCH_DFC_DITHER_ENABLE_ADDR(wch_base));
	s_dfc->padding_ctl.value = inp32(DPU_WCH_DFC_PADDING_CTL_ADDR(wch_base));
}

void dpu_wdfc_set_reg(struct hisi_comp_operator *operator, char __iomem *wch_base, struct dpu_wdfc *s_dfc)
{
	if (wch_base == NULL) {
		disp_pr_err("wch_base is NULL!\n");
		return;
	}

	if (s_dfc == NULL) {
		disp_pr_err("s_dfc is NULL!\n");
		return;
	}

	disp_pr_info("enter+++ wch_base = 0x%x", wch_base);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_DFC_DISP_SIZE_ADDR(wch_base), s_dfc->disp_size.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_DFC_PIX_IN_NUM_ADDR(wch_base), s_dfc->pix_in_num.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_DFC_DISP_FMT_ADDR(wch_base), s_dfc->disp_fmt.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_DFC_CLIP_CTL_HRZ_ADDR(wch_base), s_dfc->clip_ctl_hrz.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_DFC_CLIP_CTL_VRZ_ADDR(wch_base), s_dfc->clip_ctl_vrz.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_DFC_CTL_CLIP_EN_ADDR(wch_base), s_dfc->ctl_clip_en.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_DFC_ICG_MODULE_ADDR(wch_base), s_dfc->icg_module.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_DFC_DITHER_ENABLE_ADDR(wch_base), s_dfc->dither_enable.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_DFC_PADDING_CTL_ADDR(wch_base), s_dfc->padding_ctl.value);

#if !defined(CONFIG_DPU_FB_V320) && !defined(CONFIG_DPU_FB_V330)
	// if (offline_pipeline->index != EXTERNAL_PANEL_IDX)
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_BITEXT_CTL_ADDR(wch_base), s_dfc->bitext_ctl.value);
#endif
	disp_pr_info("enter---");
}

static bool is_need_rect_clip(struct disp_rect_ltrb clip_rect)
{
	return ((clip_rect.left > 0) || (clip_rect.top > 0) ||
		(clip_rect.right > 0) || (clip_rect.bottom > 0));
}

static bool is_need_dither(int fmt)
{
	return (fmt == DPU_DFC_STATIC_PIXEL_FORMAT_RGB_565) || (fmt == DPU_DFC_STATIC_PIXEL_FORMAT_BGR_565);
}

static void compress_set_rect_align(struct disp_rect *aligned_rect, struct disp_rect in_rect, uint32_t dfc_w,
	uint32_t xw_align, uint32_t yh_aligh)
{
	aligned_rect->x = ALIGN_DOWN((uint32_t)in_rect.x, xw_align);
	aligned_rect->w = ALIGN_UP((uint32_t)(in_rect.x - aligned_rect->x + in_rect.w + dfc_w), xw_align);
	aligned_rect->y = ALIGN_DOWN((uint32_t)in_rect.y, yh_aligh);
	aligned_rect->h = ALIGN_UP((uint32_t)(in_rect.y - aligned_rect->y + in_rect.h), yh_aligh);
}

static void compress_set_rect_pad(
	struct disp_rect *aligned_rect, struct disp_rect in_rect, uint32_t dfc_w, struct rect_pad *pad)
{
	pad->left = in_rect.x - aligned_rect->x;
	pad->right = aligned_rect->w - (in_rect.x - aligned_rect->x + in_rect.w + dfc_w);
	pad->top = in_rect.y - aligned_rect->y;
	pad->bottom = aligned_rect->h - (in_rect.y - aligned_rect->y + in_rect.h);
}

static void no_compress_rot90_set_rect_align(struct disp_rect *aligned_rect, struct disp_rect in_rect,
	struct disp_wb_layer *layer, struct wdfc_internal_data data, struct rect_pad *pad)
{
	uint32_t aligned_line;
	uint32_t addr;
	uint32_t dst_addr;
	uint32_t bpp;
	bool mmu_enable;

	aligned_line = (layer->dst.bpp <= 2) ? 32 : 16; /* yuv:32bytes ; rgbs:16bytes */
	mmu_enable = (layer->dst.mmu_enable == 1) ? true : false;
	dst_addr = mmu_enable ? layer->dst.vir_addr : layer->dst.phy_addr;
	bpp = layer->dst.bpp;
	addr = dst_addr + layer->dst_rect.x * bpp +
		(in_rect.x - layer->dst_rect.x + layer->dst_rect.y) * layer->dst.stride; /* for block */

	if (is_yuv_sp_420(layer->dst.format) || is_yuv_sp_422(layer->dst.format))
		pad->top = (addr & 0x1F) / bpp; /* bit[0~4] */
	else
		pad->top = (addr & 0x3F) / bpp; /* bit[0~5] */

	aligned_rect->x = in_rect.x;
	aligned_rect->y = in_rect.y;
	aligned_rect->w = ALIGN_UP(data.size_hrz + 1, data.dfc_aligned);
	aligned_rect->h = ALIGN_UP((uint32_t)in_rect.h + pad->top, aligned_line);

	pad->left = 0;
	pad->right = aligned_rect->w - data.size_hrz - 1;
	pad->bottom = aligned_rect->h - in_rect.h - pad->top;
    disp_pr_info("pad left = %d, right = %d, bottom = %d, top = %d", pad->left, pad->right, pad->bottom, pad->top);
}

static void hebce_set_rect_align(struct disp_rect *aligned_rect, struct disp_rect in_rect, uint32_t dfc_w,
	struct disp_wb_layer *layer, struct rect_pad *pad)
{
	bool is_yuv420 = is_yuv_sp_420(layer->dst.format);
	uint32_t xw_align;
	uint32_t yh_align;
	uint32_t is_super_block_rot = 0;
	uint32_t format = dpu_pixel_format_hal2dma(layer->dst.format, DPU_HAL2SDMA_PIXEL_FORMAT);

	is_super_block_rot = layer->transform & HISI_FB_TRANSFORM_ROT_90 ? SUPER_BLOCK_TYPE_ROT : SUPER_BLOCK_TYPE_NORMAL;
	dpu_hebc_get_basic_align_info(is_yuv420, is_super_block_rot, format, &xw_align, &yh_align);
	disp_pr_info("hebce_set_rect_align xw_align = %d, yh_align = %d, transform = %d", xw_align, yh_align,
		layer->transform);
	compress_set_rect_align(aligned_rect, in_rect, dfc_w, xw_align, yh_align);

	compress_set_rect_pad(aligned_rect, in_rect, dfc_w, pad);
}

static void no_compress_set_rect_align(struct disp_rect *aligned_rect, struct disp_rect in_rect,
	struct wdfc_internal_data data, struct disp_wb_layer *layer, struct rect_pad *pad)
{
	aligned_rect->x = ALIGN_DOWN((uint32_t)in_rect.x, (uint32_t)data.aligned_pixel);
	aligned_rect->w = ALIGN_UP((uint32_t)(in_rect.x - aligned_rect->x + in_rect.w + data.dfc_w),
		(uint32_t)data.aligned_pixel);
	aligned_rect->y = in_rect.y;

	if (is_yuv_sp_420(layer->dst.format) || is_yuv_sp_422(layer->dst.format))
		aligned_rect->h = ALIGN_UP((uint32_t)(in_rect.h), 2); /* even alignment */
	else
		aligned_rect->h = in_rect.h;

	pad->left = in_rect.x - aligned_rect->x;
	pad->right = aligned_rect->w - (pad->left + in_rect.w + data.dfc_w);
	pad->top = 0;
	pad->bottom = aligned_rect->h - in_rect.h;
}

static void wdfc_set_rect_align(struct disp_rect *aligned_rect, struct disp_rect in_rect,
	struct wdfc_internal_data data, struct disp_wb_layer *layer, struct rect_pad *pad)
{
	if (layer->dst.compress_type == COMPRESS_TYPE_HEBC) {
		hebce_set_rect_align(aligned_rect, in_rect, data.dfc_w, layer, pad);
	} else if (layer->transform & HISI_FB_TRANSFORM_ROT_90) {
		no_compress_rot90_set_rect_align(aligned_rect, in_rect, layer, data, pad);
	} else {
		no_compress_set_rect_align(aligned_rect, in_rect, data, layer, pad);
	}
}

static int wdfc_set_input_rect(struct post_offline_info *offline_info, struct disp_wb_layer *layer,
	struct disp_rect *in_rect)
{
	struct disp_rect *ov_block_rect = NULL;
	ov_block_rect = &offline_info->wb_ov_block_rect;

	if (ov_block_rect != NULL) {
		memcpy(in_rect, ov_block_rect, sizeof(struct disp_rect));

		/* dfc size is after scaler, so must equal to wb block dst size */
		if (offline_info->wb_type == WB_COMPOSE_COPYBIT && g_debug_block_scl_rot) {
			in_rect->x = layer->wb_block.dst_rect.x;
			in_rect->y = layer->wb_block.dst_rect.y;
			in_rect->w = layer->wb_block.dst_rect.w;
			in_rect->h = layer->wb_block.dst_rect.h;
		}

		if (g_debug_wch_scf && g_debug_wch_scf_rot) {
			in_rect->w = layer->dst_rect.h;
			in_rect->h = layer->dst_rect.w;
		} else if (g_debug_wch_scf)
			*in_rect = layer->dst_rect; /* wb_layer src need from src_layer's dst */
	} else {
		*in_rect = layer->src_rect;
	}
	disp_pr_info("in_rect w = %d, h = %d, x = %d, y = %d", in_rect->w, in_rect->h, in_rect->x, in_rect->y);
	return 0;
}

static int wdfc_set_internel_data(
	struct disp_wb_layer *layer, struct disp_rect in_rect, struct wdfc_internal_data *data)
{
	data->aligned_pixel = DMA_ALIGN_BYTES / layer->dst.bpp;
    disp_pr_info("aligned_pixel to %d", data->aligned_pixel);

	data->size_hrz = DPU_WIDTH(in_rect.w);
	data->size_vrt = DPU_HEIGHT(in_rect.h);

	if ((data->size_hrz + 1) % 2 == 1) {  /* size_hrz + 1 must be integral multiple of 4 or 2 */
		data->size_hrz += 1;
		data->dfc_w = 1;
	}

	data->dfc_aligned = 0x2; /* aligned to 2pixel */
	if (layer->dst.bpp <= 2) { /* bytes per pixel */
		data->dfc_pix_in_num = 0x1;
		data->dfc_aligned = 0x4; /* aligned to 4pixel */
	}

	return 0;
}

static void wdfc_config_module_struct(
	struct dpu_wdfc *dfc, struct wdfc_internal_data data,
	bool need_dither, struct rect_pad pad, int dfc_fmt)
{
    disp_pr_info("[DATA] in dfc w = %d h = %d", data.size_hrz, data.size_vrt);
	dfc->disp_size.value = set_bits32(dfc->disp_size.value, (data.size_vrt | (data.size_hrz << 16)), 32, 0);
	dfc->pix_in_num.value = set_bits32(dfc->pix_in_num.value, data.dfc_pix_in_num, 1, 0);
	dfc->disp_fmt.value = set_bits32(dfc->disp_fmt.value, dfc_fmt, 5, 1);
	dfc->clip_ctl_hrz.value = set_bits32(dfc->clip_ctl_hrz.value, 0x0, 12, 0);
	dfc->clip_ctl_vrz.value = set_bits32(dfc->clip_ctl_vrz.value, 0x0, 12, 0);
	dfc->ctl_clip_en.value = set_bits32(dfc->ctl_clip_en.value, 0x0, 1, 0);
	dfc->icg_module.value = set_bits32(dfc->icg_module.value, 0x1, 1, 0);
	if (need_dither) {
		dfc->dither_enable.value = set_bits32(dfc->dither_enable.value, 0x1, 1, 0);
		dfc->bitext_ctl.value = set_bits32(dfc->bitext_ctl.value, 0x3, 32, 0);
	} else {
		dfc->dither_enable.value = set_bits32(dfc->dither_enable.value, 0x0, 1, 0);
		dfc->bitext_ctl.value = set_bits32(dfc->bitext_ctl.value, 0x0, 32, 0);
	}

	if (pad.left || pad.right || pad.top || pad.bottom) {
		dfc->padding_ctl.value = set_bits32(dfc->padding_ctl.value, (pad.left |
			(pad.right << 8) | (pad.top << 16) | (pad.bottom << 24) | (0x1 << 31)), 32, 0);
	} else {
		dfc->padding_ctl.value = set_bits32(dfc->padding_ctl.value, 0x0, 32, 0);
	}
}

int dpu_wdfc_config(struct post_offline_info *offline_info, struct hisi_composer_data *ov_data,
	struct disp_wb_layer *layer, struct disp_rect *aligned_rect)
{
	struct dpu_wdfc *dfc = NULL;
	struct disp_rect in_rect = {0};
	bool need_dither = false;
	int dfc_fmt;
	struct wdfc_internal_data data = {0};
	struct rect_pad pad = {0};

	disp_check_and_return((!offline_info), -EINVAL, err, "offline_info is NULL\n");
	disp_check_and_return((!layer), -EINVAL, err, "layer is NULL\n");
	disp_check_and_return((!aligned_rect), -EINVAL, err, "aligned_rect is NULL\n");

	disp_pr_info("enter+++ wch_index = %d", layer->wchn_idx);

	dfc = &(ov_data->offline_module.dfc[layer->wchn_idx]);
	ov_data->offline_module.dfc_used[layer->wchn_idx] = 1;

	dfc_fmt = dpu_pixel_format_hal2dfc(layer->dst.format, DFC_STATIC);
	disp_check_and_return((dfc_fmt < 0), -EINVAL, err, "layer format [%d] not support!\n", layer->dst.format);

	disp_pr_info("dfc_fmt = %d", dfc_fmt);

	need_dither = is_need_dither(dfc_fmt); /* when 888->565 */
	wdfc_set_input_rect(offline_info, layer, &in_rect);
	wdfc_set_internel_data(layer, in_rect, &data);

	wdfc_set_rect_align(aligned_rect, in_rect, data, layer, &pad);

	wdfc_config_module_struct(dfc, data, need_dither, pad, dfc_fmt);

	if (g_debug_ovl_offline_composer)
		disp_pr_info("in_rect[x_y_w_h][%d:%d:%d:%d],align_rect[x_y_w_h][%d:%d:%d:%d], "
			"pad[l_r_t_b][%d:%d:%d:%d],bpp=%d\n",
			in_rect.x, in_rect.y, in_rect.w, in_rect.h,
			aligned_rect->x, aligned_rect->y, aligned_rect->w, aligned_rect->h,
			pad.left, pad.right, pad.top, pad.bottom, layer->dst.bpp);

	disp_pr_info("exit---");
	return 0;
}
