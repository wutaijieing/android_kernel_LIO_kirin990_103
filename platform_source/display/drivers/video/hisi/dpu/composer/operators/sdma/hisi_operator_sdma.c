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
#include <linux/dma-mapping.h>

#include "hisi_operators_manager.h"
#include "hisi_drv_utils.h"
#include "hisi_disp.h"
#include "hisi_disp_config.h"
#include "hisi_disp_composer.h"
#include "dm/hisi_dm.h"
#include "sdma/hisi_operator_sdma.h"
#include "hisi_dpu_module.h"
#include "hdr/dpu_mitm.h"
#include "gralloc/hisi_disp_iommu.h"
#include "hisi_operator_tool.h"
#include "widget/hebc/dpu_hebcd.h"
#include "widget/dpu_dfc.h"
#include "wlt/disp_dpu_wlt.h"
#include "hisi_operator_tool.h"
#include "hisi_disp_debug.h"

static void set_demura_layer(struct hisi_comp_operator *operator, uint32_t layer_id,
	struct hisi_dm_layer_info *layer_info, char __iomem *dm_base, struct pipeline_src_layer *layer) {
	if (layer->layer_id == 0)
		layer_info->layer_bot_crop_union.reg.layer_nxt_order = DPU_DEMURA0_OP_ID;
	else
		layer_info->layer_bot_crop_union.reg.layer_nxt_order = DPU_DEMURA1_OP_ID;
	g_demura_lut_fmt = layer_info->layer_dma_sel_union.reg.layer_dma_fmt;
	disp_pr_debug(" g_demura_lut_fmt = %d\n", g_demura_lut_fmt);
	if (layer->need_caps & CAP_HEBCD) {
		layer_info->layer_ov_dfc_cfg_union.value = layer->img.hebc_planes[2].header_stride << 16;
		layer_info->layer_ov_starty_union.value = layer->img.hebc_planes[2].payload_addr;
		layer_info->layer_ov_endy_union.value = layer->img.hebc_planes[2].header_addr;
		layer_info->layer_ov_pattern_a_union.value = layer->img.hebc_planes[2].payload_stride;
	} else {
		layer_info->layer_start_addr1_l_union.value = layer->img.offset_plane1;
		layer_info->layer_stride1_union.value = layer->img.stride;
		layer_info->layer_start_addr2_l_union.value = layer->img.offset_plane2;
		layer_info->layer_stride2_union.value = layer->img.stride;
	}
	layer_info->layer_ov_clip_left_union.value = 0;
	layer_info->layer_mask_y0_union.value = 0;
	layer_info->layer_mask_y1_union.value = 0;
	layer_info->layer_ov_mode_union.value = 0;
	layer_info->layer_ov_alpha_union.value = 0;
	layer_info->layer_stretch3_line_rsv_union.value = 0;
}

static int hisi_sdma_get_sdma_addr(struct pipeline_src_layer *layer, uint32_t *sdma_addr)
{
	disp_pr_debug(" ++++ \n");

	if (*sdma_addr & (DMA_ADDR_ALIGN - 1)) {
		disp_pr_err("layer%d sdma_addr(0x%x) is not %d bytes aligned.\n", layer->layer_id, *sdma_addr, DMA_ADDR_ALIGN);
		return -EINVAL;
	}

	if (layer->img.stride & (DMA_STRIDE_ALIGN - 1)) {
		disp_pr_err("layer%d stride(0x%x) is not %d bytes aligned.\n",
			layer->layer_id, layer->img.stride, DMA_STRIDE_ALIGN);
		return -EINVAL;
	}

	*sdma_addr = layer->img.mmu_enable? layer->img.vir_addr : layer->img.phy_addr;
	disp_pr_debug(" ---- mmu_enable:%d\n", layer->img.mmu_enable);
	return 0;
}

static int hisi_mcu_get_layer_dmaaddr(struct pipeline_src_layer *layer, char __iomem **rdma_addr)
{
	disp_pr_debug("vir_addr = %x, phy_addr: %x\n", layer->img.vir_addr, layer->img.phy_addr);

	*rdma_addr = layer->img.vir_addr;

	if (g_local_dimming_en) {
		set_ld_wch_vaddr(layer->img.vir_addr + layer->img.buff_size);
		disp_pr_debug("g_ld_wch_vaddr:0x%x\n", get_ld_wch_vaddr());
	}

	return 0;
}

void hisi_set_dm_addr_reg(struct hisi_comp_operator *operator, char __iomem *dpu_base_addr)
{
	static unsigned int p_flag = 0; /* 0: PING  1:PONG */
	uint32_t scend_id = operator->scene_id;

	if (is_offline_scene(scend_id)) {
		disp_pr_info("offline scene no need set ping pong\n");
		return;
	}

	disp_pr_debug(" ++++ \n");

	/* 0: PING, 1:PONG */
	if (p_flag == 0)
		dpu_set_reg(SOC_DACC_CTL_SW_EN_REG_ADDR(dpu_base_addr + DACC_OFFSET + DMC_OFFSET, scend_id), 0x0, 32, 0);
	else
		dpu_set_reg(SOC_DACC_CTL_SW_EN_REG_ADDR(dpu_base_addr + DACC_OFFSET + DMC_OFFSET, scend_id), 0x1, 32, 0);
}

uint32_t get_ovl_blending_mode(struct pipeline_src_layer *layer)
{
	uint32_t blend_mode = 0;
	bool has_per_pixel_alpha = false;

	has_per_pixel_alpha = hal_format_has_alpha(layer->img.format);

	if (layer->blending_mode == BLENDING_PREMULT) {
		if (has_per_pixel_alpha)
			blend_mode = (layer->glb_alpha < 0xFF) ? DSS_BLEND_FIX_PER12 : DSS_BLEND_SRC_OVER_DST;
		else
			blend_mode = (layer->glb_alpha < 0xFF) ? DSS_BLEND_FIX_PER8 : DSS_BLEND_SRC;
	} else if (layer->blending_mode == BLENDING_COVERAGE) {
		if (has_per_pixel_alpha)
			blend_mode = (layer->glb_alpha < 0xFF) ? DSS_BLEND_FIX_PER13 : DSS_BLEND_FIX_OVER;
		else
			blend_mode = (layer->glb_alpha < 0xFF) ? DSS_BLEND_FIX_PER8 : DSS_BLEND_SRC;
	} else {
		if (has_per_pixel_alpha)
			blend_mode = DSS_BLEND_SRC;
		else
			blend_mode = DSS_BLEND_FIX_PER17;
	}

	if (layer->is_cld_layer == 1)
		blend_mode = DSS_BLEND_FIX_OVER;

	disp_pr_debug("blending=%d, fomat=%d, has_per_pixel_alpha=%d, blend_mode=%d.\n",
		layer->blending_mode, layer->img.format, has_per_pixel_alpha, blend_mode);

	return blend_mode;
}

void set_layer_mirr(struct hisi_dm_layer_info *layer_info, struct pipeline_src_layer *layer)
{
	layer_info->layer_ov_clip_left_union.value = 0;
	if ((layer->transform == HISI_FB_TRANSFORM_ROT_180) || (layer->transform == HISI_FB_TRANSFORM_ROT_270) ||
		(layer->transform == HISI_FB_TRANSFORM_FLIP_V))
		layer_info->layer_ov_clip_left_union.reg.layer_vmirr_en = 1;

	if ((layer->transform == HISI_FB_TRANSFORM_ROT_180) || (layer->transform == HISI_FB_TRANSFORM_ROT_90) ||
		(layer->transform == HISI_FB_TRANSFORM_FLIP_H))
		layer_info->layer_ov_clip_left_union.reg.layer_hmirr_en = 1;
}

void set_ov_mode(struct hisi_dm_layer_info *layer_info, uint32_t blending_mode)
{
	layer_info->layer_ov_mode_union.reg.layer_ov_alpha_smode = g_ovl_alpha[blending_mode].alpha_smode;
	layer_info->layer_ov_mode_union.reg.layer_ov_fix_mode = g_ovl_alpha[blending_mode].fix_mode;
	layer_info->layer_ov_mode_union.reg.layer_ov_dst_pmode = g_ovl_alpha[blending_mode].dst_pmode;
	layer_info->layer_ov_mode_union.reg.layer_ov_dst_gmode = g_ovl_alpha[blending_mode].dst_gmode;
	layer_info->layer_ov_mode_union.reg.layer_ov_dst_amode = g_ovl_alpha[blending_mode].dst_amode;
	layer_info->layer_ov_mode_union.reg.layer_ov_src_mmode = 0;
	layer_info->layer_ov_mode_union.reg.layer_ov_src_pmode = g_ovl_alpha[blending_mode].src_pmode;
	layer_info->layer_ov_mode_union.reg.layer_ov_src_gmode = g_ovl_alpha[blending_mode].src_gmode;
	layer_info->layer_ov_mode_union.reg.layer_ov_src_amode = g_ovl_alpha[blending_mode].src_amode;
	layer_info->layer_ov_mode_union.reg.layer_ov_auto_nosrc = 0;
	layer_info->layer_ov_mode_union.reg.layer_ov_src_cfg = 0;
	layer_info->layer_ov_mode_union.reg.layer_ov_trop_code = 0;
	layer_info->layer_ov_mode_union.reg.layer_ov_rop_code = 0;
	layer_info->layer_ov_mode_union.reg.layer_clip_en = 1; // FIXME: 1
	layer_info->layer_ov_mode_union.reg.layer_bitext_en = 1; // FIXME: 1
}

static int hisi_sdma_set_dm_layerinfo_sdma_section(struct hisi_composer_device *composer_device,
	struct hisi_comp_operator *operator, struct pipeline_src_layer *layer, struct hisi_dm_layer_info *layer_info)
{
	int ret;

	uint32_t blending_mode = get_ovl_blending_mode(layer);
	char __iomem *rdma_addr;

	ret = hisi_mcu_get_layer_dmaaddr(layer, &rdma_addr);
	if (ret != 0)
		return ret;

	disp_pr_info(" ++++ \n");

	// DPU_DM_LAYER_HEIGHT_UNION 0x7800438
	layer_info->layer_height_union.value = 0;

	layer_info->layer_height_union.reg.layer_width = DPU_WIDTH(layer->img.width);
	layer_info->layer_height_union.reg.layer_height = DPU_HEIGHT(layer->img.height);
	disp_pr_debug("[DATA] sdma in w = %d, h = %d\n", layer_info->layer_height_union.reg.layer_width,
		layer_info->layer_height_union.reg.layer_height);

	if (layer->is_cld_layer) {
		layer_info->layer_height_union.reg.layer_width = layer->cld_width - 1;
		layer_info->layer_height_union.reg.layer_height = layer->cld_height - 1;
	}

	// DPU_DM_LAYER_MASK_Y0_UNION
	layer_info->layer_mask_y0_union.value = 0;
	// DPU_DM_LAYER_MASK_Y1_UNION
	layer_info->layer_mask_y1_union.value = 0;
	// DPU_DM_LAYER_DMA_SEL_UNION 0x600
	layer_info->layer_dma_sel_union.value = 0;
	layer_info->layer_dma_sel_union.reg.layer_dma_fmt = dpu_pixel_format_hal2dma(layer->img.format, DPU_HAL2SDMA_PIXEL_FORMAT);
	layer_info->layer_dma_sel_union.reg.layer_fbc_type = layer->img.compress_type;
	layer_info->layer_dma_sel_union.reg.layer_dma_sel = layer->dma_sel;

	layer_info->layer_sblk_type_union.value = 0;
	layer_info->layer_sblk_type_union.reg.layer_id = layer->layer_id;
	layer_info->layer_sblk_type_union.reg.layer_stretch_en = 0;
	layer_info->layer_sblk_type_union.reg.layer_sblk_type = 0;

	disp_pr_debug(" operator->in_data->next_order:%u \n", operator->in_data->next_order);

	layer_info->layer_bot_crop_union.reg.layer_nxt_order = hisi_get_op_id_by_op_type(operator->in_data->next_order);
	disp_pr_debug(" operator->in_data->next_order:%u \n", layer_info->layer_bot_crop_union.reg.layer_nxt_order);
	disp_pr_debug(" operator->in_data->format:%x \n", operator->in_data->format);

	layer_info->layer_bot_crop_union.reg.layer_top_crop = 0;
	layer_info->layer_bot_crop_union.reg.layer_bot_crop = 0;

	set_layer_mirr(layer_info, layer);

	layer_info->layer_ov_dfc_cfg_union.value = 0;
	if (is_offline_scene(operator->scene_id)) {
		layer_info->layer_ov_starty_union.reg.layer_ov_startx = 0;
		layer_info->layer_ov_starty_union.reg.layer_ov_starty = 0;
		layer_info->layer_ov_endy_union.reg.layer_ov_endx = DPU_WIDTH(layer->wb_block_rect.w);
		layer_info->layer_ov_endy_union.reg.layer_ov_endy = DPU_HEIGHT(layer->wb_block_rect.h);
	} else {
		layer_info->layer_ov_starty_union.reg.layer_ov_startx = layer->dst_rect.x;
		layer_info->layer_ov_starty_union.reg.layer_ov_starty = layer->dst_rect.y;
		layer_info->layer_ov_endy_union.reg.layer_ov_endx =
			get_ov_endx(&layer->dst_rect, composer_device->ov_data.fix_panel_info->xres);
		layer_info->layer_ov_endy_union.reg.layer_ov_endy =
			get_ov_endy(&layer->dst_rect, composer_device->ov_data.fix_panel_info->yres);
	}
	layer_info->layer_ov_pattern_a_union.value = 0;
	if (!g_debug_wch_yuv_in)
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_enable = 1;

	if (layer->need_caps & (CAP_UVUP | CAP_HDR))
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_dfc_en = 0;
	else
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_dfc_en = 1; // FIXME: 1

#ifndef LOCAL_DIMMING_GOLDEN_TEST
	if (is_yuv_format(layer->img.format) && !g_debug_wch_yuv_in && !(layer->need_caps & CAP_HDR)) {
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_csc_en = 1;
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_csc_reg_sel = 4;
	}
#endif

	layer_info->layer_ov_pattern_rgb_union.value = 0;

#ifndef LOCAL_DIMMING_GOLDEN_TEST
	if (layer->need_caps & CAP_MITM)
		dpu_ov_mitm_sel(layer, layer_info);

	set_ov_mode(layer_info, blending_mode);
#endif

	layer_info->layer_ov_alpha_union.value = 0;
#ifndef LOCAL_DIMMING_GOLDEN_TEST
	layer_info->layer_ov_alpha_union.reg.layer_ov_alpha = 0;
	layer_info->layer_ov_alpha_union.reg.layer_ov_alpha_offsrc = g_ovl_alpha[blending_mode].alpha_offsrc;
	layer_info->layer_ov_alpha_union.reg.layer_ov_alpha_offdst = g_ovl_alpha[blending_mode].alpha_offdst;
	layer_info->layer_ov_alpha_union.reg.layer_ov_dst_global_alpha = 0xFF * 4;
	layer_info->layer_ov_alpha_union.reg.layer_ov_src_global_alpha = 0xFF * 4;
#endif

	layer_info->layer_stretch3_line_rsv_union.value = 0;
	layer_info->layer_start_addr3_h_union.value = 0;
	if (is_offline_scene(operator->scene_id))
		layer_info->layer_start_addr0_l_union.value = rdma_addr + layer->wb_block_rect.x * layer->img.bpp;
	else
		layer_info->layer_start_addr0_l_union.value = rdma_addr;

	layer_info->layer_start_addr2_l_union.value = 0;
	layer_info->layer_start_addr3_l_union.value = 0;
	layer_info->layer_stride0_union.value = layer->img.stride;
	layer_info->layer_stride2_union.value = 0;
	if (is_yuv_semiplanar(layer->img.format)) {
		layer_info->layer_start_addr1_l_union.value = rdma_addr + layer->img.offset_plane1 +
			layer->wb_block_rect.x * layer->img.bpp;
		layer_info->layer_stride1_union.value = layer->img.stride_plane1;
	}
	if (is_yuv_plane(layer->img.format)) { // FIXME
		layer_info->layer_start_addr2_l_union.value = rdma_addr + layer->img.offset_plane2;
		layer_info->layer_stride2_union.value = layer->img.stride_plane2;
	}

	disp_pr_info(" ---- \n");

	return ret;
}

static void hisi_sdma_set_regs(struct hisi_comp_operator *operator, uint32_t layer_id,
	struct hisi_dm_layer_info *layer_info, char __iomem *dpu_base_addr, struct pipeline_src_layer *src_layer)
{
	char __iomem *dm_base = dpu_base_addr + dpu_get_dm_offset(operator->scene_id);
	disp_pr_debug(" dm_base:0x%p, layer_id:%u \n", dm_base, layer_id);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_HEIGHT_ADDR(dm_base, layer_id), layer_info->layer_height_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_DMA_SEL_ADDR(dm_base, layer_id), layer_info->layer_dma_sel_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_SBLK_TYPE_ADDR(dm_base, layer_id), layer_info->layer_sblk_type_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_BOT_CROP_ADDR(dm_base, layer_id), layer_info->layer_bot_crop_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_STRIDE0_ADDR(dm_base, layer_id), layer_info->layer_stride0_union.value);
	if (is_d3_128(src_layer->img.format) || (src_layer->need_caps & CAP_DEMA))
		set_demura_layer(operator, layer_id, layer_info, dm_base, src_layer);

	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_OV_CLIP_LEFT_ADDR(dm_base, layer_id), layer_info->layer_ov_clip_left_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_MASK_Y0_ADDR(dm_base, layer_id),
		layer_info->layer_mask_y0_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_MASK_Y1_ADDR(dm_base, layer_id),
		layer_info->layer_mask_y1_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_OV_MODE_ADDR(dm_base, layer_id), layer_info->layer_ov_mode_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_OV_ALPHA_ADDR(dm_base, layer_id), layer_info->layer_ov_alpha_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_STRETCH3_LINE_ADDR(dm_base, layer_id), layer_info->layer_stretch3_line_rsv_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_OV_DFC_CFG_ADDR(dm_base, layer_id), layer_info->layer_ov_dfc_cfg_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_OV_STARTY_ADDR(dm_base, layer_id), layer_info->layer_ov_starty_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_OV_ENDY_ADDR(dm_base, layer_id), layer_info->layer_ov_endy_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_OV_PATTERN_A_ADDR(dm_base, layer_id), layer_info->layer_ov_pattern_a_union.value);

	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_STRIDE1_ADDR(dm_base, layer_id), layer_info->layer_stride1_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_STRIDE2_ADDR(dm_base, layer_id), layer_info->layer_stride2_union.value);

	if (!(src_layer->need_caps & CAP_WLT)) {
		disp_pr_info("no wlt mode");
		hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_START_ADDR0_L_ADDR(dm_base, layer_id), layer_info->layer_start_addr0_l_union.value);
		hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_START_ADDR1_L_ADDR(dm_base, layer_id), layer_info->layer_start_addr1_l_union.value);
		hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_START_ADDR2_L_ADDR(dm_base, layer_id), layer_info->layer_start_addr2_l_union.value);
	} else {
		disp_pr_info("wlt mode no need set dm sdma addr");
	}

	hisi_set_dm_addr_reg(operator, dpu_base_addr);
}

static int hisi_sdma_set_cmd_item(struct hisi_comp_operator *operator, struct hisi_composer_device *composer_device,
	void *layer)
{
	int ret;
	struct hisi_dm_param *dm_param = NULL;
	struct pipeline_src_layer *src_layer = (struct pipeline_src_layer *)layer;

	disp_pr_debug(" ++++ \n");
	dm_param = composer_device->ov_data.dm_param;

	ret = hisi_sdma_set_dm_layerinfo_sdma_section(composer_device, operator, src_layer,
		&(dm_param->layer_info[src_layer->layer_id]));
	if (ret)
		return -1;

	if (src_layer->img.compress_type == COMPRESS_TYPE_HEBC) {
		dpu_hebcd_calc(src_layer, &(dm_param->layer_info[src_layer->layer_id]));
		hisi_sdma_hebcd_set_regs(operator, src_layer->layer_id, &(dm_param->layer_info[src_layer->layer_id]),
				composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU], src_layer);
	}

	hisi_sdma_set_regs(operator, src_layer->layer_id, &(dm_param->layer_info[src_layer->layer_id]),
		composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU], src_layer);
	composer_device->ov_data.layer_ov_format = dpu_pixel_format_hal2dfc(src_layer->img.format, DFC_DYNAMIC);
	if (src_layer->need_caps & CAP_WLT) {
		struct dpu_dacc_wlt dacc_wlt = {0};
		ret = disp_dpu_wlt_set_dacc_cfg_wltinfo(src_layer, &dacc_wlt);
		if (ret)
			return -1;

		disp_dpu_wlt_set_dacc_cfg_wlt_regs(composer_device, &dacc_wlt,
			composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU]);
	}

	disp_pr_debug(" ---- \n");
	return 0;
}

static int hisi_sdma_build_data(struct hisi_comp_operator *operator, void *layer,
	struct hisi_pipeline_data *pre_out_data, struct hisi_comp_operator *next_operator)
{
	struct pipeline_src_layer *src_layer = (struct pipeline_src_layer *)layer;

	memcpy(&operator->in_data->rect, &src_layer->src_rect, sizeof(src_layer->src_rect));
	if (src_layer->is_cld_layer) {
		operator->in_data->rect.w = src_layer->cld_width;
		operator->in_data->rect.h = src_layer->cld_height;
	}

	operator->in_data->format = dpu_pixel_format_hal2dma(src_layer->img.format, DPU_HAL2SDMA_PIXEL_FORMAT);

	memcpy(operator->out_data, operator->in_data, sizeof(*(operator->in_data)));

	dpu_operator_build_data(operator, layer, pre_out_data, next_operator);

	return 0;
}

void hisi_sdma_init(struct hisi_operator_type *type_operator, struct dpu_module_ops ops, void *cookie)
{
	struct hisi_operator_sdma **sdmas = NULL;
	struct hisi_comp_operator *base = NULL;
	uint32_t i;

	disp_pr_info(" ++++ \n");
	sdmas = kzalloc(sizeof(*sdmas) * type_operator->operator_count, GFP_KERNEL);
	if (!sdmas)
		return;

	disp_pr_info(" type_operator->operator_count:%u \n", type_operator->operator_count);
	for (i = 0; i < type_operator->operator_count; i++) {
		disp_pr_info(" size of struct hisi_operator_sdma:%u \n", sizeof(*(sdmas[i])));
		sdmas[i] = kzalloc(sizeof(*(sdmas[i])), GFP_KERNEL);
		if (!sdmas[i])
			continue;

		base = &sdmas[i]->base;

		/* TODO: error check */
		hisi_operator_init(base, ops, cookie);
		base->id_desc.info.idx = i;
		base->id_desc.info.type = type_operator->type;
		base->set_cmd_item = hisi_sdma_set_cmd_item;
		base->build_data = hisi_sdma_build_data;
		sema_init(&sdmas[i]->base.operator_sem, 1);
		disp_pr_info(" base->id_desc.info.idx:%u, base->id_desc.info.type:%u \n", base->id_desc.info.idx, base->id_desc.info.type);

		base->out_data = kzalloc(sizeof(struct hisi_sdma_out_data), GFP_KERNEL);
		if (!base->out_data) {
			disp_pr_err("alloc out_data failed\n");
			kfree(sdmas[i]);
			sdmas[i] = NULL;
			continue;
		}

		base->in_data = kzalloc(sizeof(struct hisi_sdma_in_data), GFP_KERNEL);
		if (!base->in_data) {
			disp_pr_err("alloc in_data failed\n");
			kfree(base->out_data);
			base->out_data = NULL;

			kfree(sdmas[i]);
			sdmas[i] = NULL;
			continue;
		}

		/* TODO: init other rmda operators */
		sdmas[i]->base.be_dm_counted = false;
	}
	type_operator->operators = (struct hisi_comp_operator **)(sdmas);
}
