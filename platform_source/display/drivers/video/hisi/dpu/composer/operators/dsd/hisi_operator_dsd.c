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
#include "dsd/hisi_operator_dsd.h"
#include "sdma/hisi_operator_sdma.h"
#include "hisi_dpu_module.h"
#include "gralloc/hisi_disp_iommu.h"
#include "hisi_operator_tool.h"
#include "video/hdmirx_video.h"
#include "hisi_disp_debug.h"
#include <securec.h>

static void dpu_set_ld_wch_addr(struct pipeline_src_layer *layer)
{
	disp_pr_debug("vir_addr = 0x%x, size: %d\n", layer->img.vir_addr, layer->img.buff_size);

	if (g_local_dimming_en) {
		set_ld_wch_vaddr(layer->img.vir_addr + layer->img.buff_size);
		disp_pr_debug("g_ld_wch_vaddr:0x%x\n", get_ld_wch_vaddr());
	}
}

static int hisi_dsd_set_dm_layerinfo_dsd_section(struct hisi_composer_device *composer_device,
		struct hisi_comp_operator *operator, struct pipeline_src_layer *layer, struct hisi_dm_layer_info *layer_info)
{
	uint32_t blending_mode = get_ovl_blending_mode(layer);
	disp_pr_info(" ++++ \n");

	dpu_set_ld_wch_addr(layer);

	// DPU_DM_LAYER_HEIGHT_UNION 0x7800438
	layer_info->layer_height_union.value = 0;

	layer_info->layer_height_union.reg.layer_width = DPU_WIDTH(layer->img.width);
	layer_info->layer_height_union.reg.layer_height = DPU_HEIGHT(layer->img.height);
	disp_pr_debug("[DATA] dsd in w = %d, h = %d\n", layer_info->layer_height_union.reg.layer_width,
		layer_info->layer_height_union.reg.layer_height);

	// DPU_DM_LAYER_MASK_Y0_UNION
	layer_info->layer_mask_y0_union.value = 0;
	// DPU_DM_LAYER_MASK_Y1_UNION
	layer_info->layer_mask_y1_union.value = 0;
	// DPU_DM_LAYER_DMA_SEL_UNION 0x600
	layer_info->layer_dma_sel_union.value = 0;

	// for rx
	layer_info->layer_dma_sel_union.reg.layer_dma_fmt = operator->out_data->format;
	layer_info->layer_dma_sel_union.reg.layer_fbc_type = layer->img.compress_type;
	layer_info->layer_dma_sel_union.reg.layer_dma_sel = (layer->dma_sel == 1) ? 0x10 : 0x20; // dsd0 dsd1

	disp_pr_info(" layer%d_dma_fmt:0x%x,0x%x, layer_fbc_type %d, layer_dma_sel 0x%x \n",
		layer->layer_id, layer->img.format, operator->out_data->format,
		layer->img.compress_type,
		layer->dma_sel);

	layer_info->layer_sblk_type_union.value = 0;

	// for rx
	layer_info->layer_sblk_type_union.reg.layer_id = layer->layer_id;

	layer_info->layer_sblk_type_union.reg.layer_stretch_en = 0;
	layer_info->layer_sblk_type_union.reg.layer_sblk_type = 0;

	disp_pr_info(" operator->in_data->next_order:%u \n", operator->in_data->next_order);
	layer_info->layer_bot_crop_union.reg.layer_nxt_order = hisi_get_op_id_by_op_type(operator->in_data->next_order);
	disp_pr_info(" operator->in_data->format:%x \n", operator->in_data->format);
	layer_info->layer_bot_crop_union.reg.layer_top_crop = 0;
	layer_info->layer_bot_crop_union.reg.layer_bot_crop = 0;

	set_layer_mirr(layer_info, layer);

	layer_info->layer_ov_dfc_cfg_union.value = 0;

	if (is_offline_scene(operator->scene_id)) {
		layer_info->layer_ov_starty_union.reg.layer_ov_startx = layer->dst_rect.x;
		layer_info->layer_ov_starty_union.reg.layer_ov_starty = layer->dst_rect.y;
		layer_info->layer_ov_endy_union.reg.layer_ov_endx = DPU_WIDTH(layer->wb_block_rect.w);
		layer_info->layer_ov_endy_union.reg.layer_ov_endy = DPU_HEIGHT(layer->wb_block_rect.h);
	} else {
		layer_info->layer_ov_starty_union.reg.layer_ov_startx = layer->dst_rect.x;
		layer_info->layer_ov_starty_union.reg.layer_ov_starty = layer->dst_rect.y;

		layer_info->layer_ov_endy_union.reg.layer_ov_endx =
			get_ov_endx(&layer->dst_rect, composer_device->ov_data.fix_panel_info->xres);
		layer_info->layer_ov_endy_union.reg.layer_ov_endy =
			get_ov_endy(&layer->dst_rect, composer_device->ov_data.fix_panel_info->yres);
		disp_pr_info(" xres,yres:%d %d\n", composer_device->ov_data.fix_panel_info->xres,
			composer_device->ov_data.fix_panel_info->yres);
	}

	layer_info->layer_ov_pattern_a_union.value = 0;

	if (!g_debug_wch_yuv_in)
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_enable = 1;

	layer_info->layer_ov_pattern_a_union.reg.layer_ov_dfc_en = 1; // FIXME: 1

	if (is_yuv_format(layer->img.format) && !g_debug_wch_yuv_in) {
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_csc_en = 1;
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_csc_reg_sel = 4;
	}
	layer_info->layer_ov_pattern_rgb_union.value = 0;

	set_ov_mode(layer_info, blending_mode);

	if (operator->in_data->format == DPU_DSD_WRAP_FORMAT_YUV422_10BIT) {
		layer_info->layer_ov_mode_union.reg.layer_bitext_en = 0;
		disp_pr_info(" dsd in format yuv422 10bit bitext disable.\n");
	} else {
		layer_info->layer_ov_mode_union.reg.layer_bitext_en = 1; // FIXME: 1
	}

	layer_info->layer_ov_alpha_union.value = 0;
	layer_info->layer_ov_alpha_union.reg.layer_ov_alpha = 0;
	layer_info->layer_ov_alpha_union.reg.layer_ov_alpha_offsrc = g_ovl_alpha[blending_mode].alpha_offsrc;
	layer_info->layer_ov_alpha_union.reg.layer_ov_alpha_offdst = g_ovl_alpha[blending_mode].alpha_offdst;
	layer_info->layer_ov_alpha_union.reg.layer_ov_dst_global_alpha = 0xFF * 4;
	layer_info->layer_ov_alpha_union.reg.layer_ov_src_global_alpha = 0xFF * 4;

	layer_info->layer_stretch3_line_rsv_union.value = 0;
	layer_info->layer_start_addr3_h_union.value = 0;

	// for rx
	layer_info->layer_start_addr0_l_union.value = 0;

	layer_info->layer_start_addr2_l_union.value = 0;
	layer_info->layer_start_addr3_l_union.value = 0;
	layer_info->layer_stride0_union.value = layer->img.stride;
	layer_info->layer_stride2_union.value = 0;

	// for rx
	layer_info->layer_start_addr1_l_union.value = 0;
	layer_info->layer_stride1_union.value = 0;
	disp_pr_info(" ---- \n");

	return 0;
}

static void hisi_dsd_layer_set_regs(struct hisi_comp_operator *operator, uint32_t layer_id,
	struct hisi_dm_layer_info *layer_info, char __iomem *dpu_base_addr)
{
	char __iomem *dm_base = dpu_base_addr + dpu_get_dm_offset(operator->scene_id);

	disp_pr_debug(" dm_base:0x%p, layer_id:%u \n", dm_base, layer_id);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_HEIGHT_ADDR(dm_base, layer_id),
						layer_info->layer_height_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_DMA_SEL_ADDR(dm_base, layer_id),
						layer_info->layer_dma_sel_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_SBLK_TYPE_ADDR(dm_base, layer_id),
						layer_info->layer_sblk_type_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_BOT_CROP_ADDR(dm_base, layer_id),
						layer_info->layer_bot_crop_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_START_ADDR0_L_ADDR(dm_base, layer_id),
						layer_info->layer_start_addr0_l_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_STRIDE0_ADDR(dm_base, layer_id),
						layer_info->layer_stride0_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_OV_CLIP_LEFT_ADDR(dm_base, layer_id),
						layer_info->layer_ov_clip_left_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_OV_DFC_CFG_ADDR(dm_base, layer_id),
						layer_info->layer_ov_dfc_cfg_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_OV_STARTY_ADDR(dm_base, layer_id),
						layer_info->layer_ov_starty_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_OV_ENDY_ADDR(dm_base, layer_id),
						layer_info->layer_ov_endy_union.value);

	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_OV_MODE_ADDR(dm_base, layer_id),
						layer_info->layer_ov_mode_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_OV_ALPHA_ADDR(dm_base, layer_id),
						layer_info->layer_ov_alpha_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_OV_PATTERN_A_ADDR(dm_base, layer_id),
						layer_info->layer_ov_pattern_a_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_START_ADDR1_L_ADDR(dm_base, layer_id),
						layer_info->layer_start_addr1_l_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_STRIDE1_ADDR(dm_base, layer_id),
						layer_info->layer_stride1_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_STRETCH3_LINE_ADDR(dm_base, layer_id),
						layer_info->layer_stretch3_line_rsv_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_START_ADDR2_L_ADDR(dm_base, layer_id),
						layer_info->layer_start_addr2_l_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_LAYER_STRIDE2_ADDR(dm_base, layer_id),
						layer_info->layer_stride2_union.value);

	hisi_set_dm_addr_reg(operator, dpu_base_addr);
}

unsigned int dpu_get_hblank(struct pipeline_src_layer *layer)
{
	unsigned int hblank = 280;

	if (layer->src_type == LAYER_SRC_TYPE_HDMIRX)
		hblank = hdmirx_video_get_hblank();

	return hblank;
}

static void dpu_dsd_config(char __iomem *dpu_base_addr, struct hisi_comp_operator *operator,
	struct pipeline_src_layer *layer)
{
	char __iomem *dsd_base; // for test, dsd
	char __iomem *glb_base = dpu_base_addr + 0x13000;
	struct dpu_dsd_params dsd_params;

	disp_pr_info("dsd_index =  %d\n", layer->dsd_index);
	if (layer->dsd_index == 0) {
		dsd_base = dpu_base_addr + DSS_DSD0_OFFSET;
	} else {
		dsd_base = dpu_base_addr + DSS_DSD1_OFFSET;
	}
	disp_pr_info("dsd_base =  0x%x\n", dsd_base);

	dsd_params.dscd_ctrl.value = 0;
	dsd_params.dscd_ctrl.reg.ice_dscd_en = 0; // dsd bypass
	dsd_params.dscd_ctrl.reg.ice_dscd_cfg_sel = 1; // software config

	dsd_params.pic_reso.value = 0;
	dsd_params.pic_reso.reg.pic_width = layer->img.width;
	dsd_params.pic_reso.reg.pic_height = layer->img.height;

	disp_pr_info("layer->img.pic_width %d, layer->img.height %d\n", layer->img.width, layer->img.height);

	dpu_set_reg(DPU_DSD_WRAP_ICE_DSCD_CTRL_ADDR(dsd_base), dsd_params.dscd_ctrl.value, 32, 0);
	dpu_set_reg(DPU_DSD_WRAP_ICE_DSCD_PIC_RESO_ADDR(dsd_base), dsd_params.pic_reso.value, 32, 0);

	dsd_params.reg_ctrl.value = 0;
	dsd_params.reg_ctrl.reg.reg_ctrl_scene_id = operator->scene_id; // 4; //offline
	disp_pr_info("operator->scene_id %d\n", operator->scene_id);

	dpu_set_reg(DPU_DSD_WRAP_REG_CTRL_ADDR(dsd_base), dsd_params.reg_ctrl.value, 32, 0);

	dsd_params.hblank_num.value = 0;
	dsd_params.hblank_num.reg.format = operator->in_data->format;
	dsd_params.hblank_num.reg.dsd_hblank_num = dpu_get_hblank(layer);
	dsd_params.hblank_num.reg.dsd_slice_h = layer->img.height;
	dpu_set_reg(DPU_DSD_WRAP_HBLANK_NUM_ADDR(dsd_base), dsd_params.hblank_num.value, 32, 0);

	dsd_params.pic_size.value = 0;
	dsd_params.pic_size.reg.dsd_bypass_en = 1;
	dsd_params.pic_size.reg.dsd_pic_w = layer->img.width;
	dsd_params.pic_size.reg.dsd_pic_h = layer->img.height;
	dpu_set_reg(DPU_DSD_WRAP_PIC_SIZE_ADDR(dsd_base), dsd_params.pic_size.value, 32, 0);

	dsd_params.clk_sel.value = 0;
	dsd_params.clk_sel.reg.dsd_clk_sel = 1;
	dsd_params.clk_sel.reg.wdbuf_clk_sel = 1;
	dpu_set_reg(DPU_DSD_WRAP_CLK_SEL_ADDR(dsd_base), dsd_params.clk_sel.value, 32, 0);

	dsd_params.clk_en.value = 0;
	dsd_params.clk_en.reg.dsd_clk_en = 1;
	dsd_params.clk_en.reg.wdbuf_clk_en = 1;
	dpu_set_reg(DPU_DSD_WRAP_CLK_EN_ADDR(dsd_base), dsd_params.clk_en.value, 32, 0);

	/* DCPT */
	/* WDBUF */
	dpu_set_reg(DPU_DSD_WRAP_WDBUF_INTE_ADDR(dsd_base), 0x81, 32, 0);
	dpu_set_reg(dsd_base + 0x328, 0x10008, 32, 0); // wdbuf debug
	dpu_set_reg(DPU_DSD_WRAP_WDBUF_DFS_THD_ADDR(dsd_base), 0x345F, 32, 0);
	dpu_set_reg(DPU_DSD_WRAP_FLUX_REQ_H_THD_0_ADDR(dsd_base), 0x04790147, 32, 0);
	dpu_set_reg(DPU_DSD_WRAP_FLUX_REQ_H_THD_1_ADDR(dsd_base), 0x00180010, 32, 0);
	dpu_set_reg(DPU_DSD_WRAP_FLUX_REQ_L_THD_0_ADDR(dsd_base),  0x0ADC07AA, 32, 0);
	dpu_set_reg(DPU_DSD_WRAP_FLUX_REQ_L_THD_1_ADDR(dsd_base),  0x00180010, 32, 0);
	dpu_set_reg(DPU_DSD_WRAP_WQOS_THD_ADDR(dsd_base),  0x160D0E0D, 32, 0);
	dpu_set_reg(DPU_DSD_WRAP_WARN_KEEP_TIME_ADDR(dsd_base), 0x400, 32, 0);

	dsd_params.reg_ctrl_flush_en.value = 0;
	dsd_params.reg_ctrl_flush_en.reg.reg_ctrl_flush_en = 1;
	dpu_set_reg(DPU_DSD_WRAP_REG_CTRL_FLUSH_EN_ADDR(dsd_base), dsd_params.reg_ctrl_flush_en.value, 32, 0);

	dpu_set_reg(DPU_DSD_WRAP_REG_CTRL_DEBUG_ADDR(dsd_base), 1, 1, 10);
	disp_pr_info("continuous frame\n");

	/* frame start config */
	if (is_offline_scene(operator->scene_id)) {
		if (layer->dsd_index == 0) {
			dpu_set_reg(glb_base + 0xA00, 0x1C20, 32, 0);
			dpu_set_reg(glb_base + 0xA08, 0x19, 32, 0);
			dpu_set_reg(glb_base + 0xA1C, 0x6, 32, 0); // dprx0->wch1
			disp_pr_info("config offline rx0 frame start, wch1 end\n");
		} else {
			dpu_set_reg(glb_base + 0x9FC, 0x1C20, 32, 0);
			dpu_set_reg(glb_base + 0xA0C, 0x22, 32, 0);
			dpu_set_reg(glb_base + 0xA20, 0x7, 32, 0); // dprx1-->wch2
			disp_pr_info("config offline rx1 frame start, wch1 end\n");
		}
	} else {
		dpu_set_reg(glb_base + 0xA04, 0, 32, 0);
		if (layer->dsd_index == 0) {
			dpu_set_reg(glb_base + 0xA1C, 0x3, 32, 0); // dprx0-->dptx0
			disp_pr_info("config online rx0 itf frame start, itf end\n");
		} else {
			dpu_set_reg(glb_base + 0xA20, 0x3, 32, 0); // dprx1-->dptx0
			disp_pr_info("config online rx1 itf frame start, itf end\n");
		}
	}
}

static int hisi_dsd_set_cmd_item(struct hisi_comp_operator *operator,
	struct hisi_composer_device *composer_device, void *layer)
{
	int ret;
	struct hisi_dm_param *dm_param = NULL;
	struct pipeline_src_layer *src_layer = (struct pipeline_src_layer *)layer;

	disp_pr_info(" ++++ \n");
	dm_param = composer_device->ov_data.dm_param;

	ret = hisi_dsd_set_dm_layerinfo_dsd_section(composer_device, operator, src_layer,
		&(dm_param->layer_info[src_layer->layer_id]));
	if (ret)
		return -1;
	hisi_dsd_layer_set_regs(operator, src_layer->layer_id, &(dm_param->layer_info[src_layer->layer_id]),
		composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU]);

	dpu_dsd_config(composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU], operator, src_layer);

	composer_device->ov_data.layer_ov_format = dpu_pixel_format_hal2dfc(operator->out_data->format, DFC_DYNAMIC);
	disp_pr_info(" ---- \n");
	return 0;
}

static int hisi_dsd_build_data(struct hisi_comp_operator *operator, void *layer,
	struct hisi_pipeline_data *pre_out_data, struct hisi_comp_operator *next_operator)
{
	struct pipeline_src_layer *src_layer = (struct pipeline_src_layer *)layer;

	disp_pr_debug(" ++++ \n");

	memcpy_s(&operator->in_data->rect, sizeof(operator->in_data->rect),
			&src_layer->src_rect, sizeof(src_layer->src_rect));

	// from rx
	operator->in_data->format = dpu_pixel_format_hal2dsdwrap(src_layer->img.format);
	disp_pr_info("rx input format: hal 0x%x, dsd 0x%x\n",
		src_layer->img.format, operator->in_data->format);

	memcpy_s(operator->out_data, sizeof(*(operator->out_data)),
			operator->in_data, sizeof(*(operator->in_data)));
	operator->out_data->format = dpu_pixel_format_hal2dma(src_layer->img.format, DPU_HAL2DSD_PIXEL_FORMAT);
	disp_pr_info("dsd output format: 0x%x\n", operator->out_data->format);

	dpu_operator_build_data(operator, layer, pre_out_data, next_operator);

	return 0;
}

void hisi_dsd_init(struct hisi_operator_type *type_operator,
	struct dpu_module_ops ops, void *cookie)
{
	struct hisi_operator_dsd **dsd = NULL;
	struct hisi_comp_operator *base = NULL;
	uint32_t i;

	disp_pr_info(" ++++ \n");
	dsd = kzalloc(sizeof(*dsd) * type_operator->operator_count, GFP_KERNEL);
	if (!dsd)
		return;

	disp_pr_info(" type_operator->operator_count:%u \n", type_operator->operator_count);
	for (i = 0; i < type_operator->operator_count; i++) {
		disp_pr_info(" size of struct hisi_operator_dsd:%u \n", sizeof(*(dsd[i])));
		dsd[i] = kzalloc(sizeof(*(dsd[i])), GFP_KERNEL);
		if (!dsd[i])
			continue;

		base = &dsd[i]->base;

		/* TODO: error check */
		hisi_operator_init(base, ops, cookie);
		base->id_desc.info.idx = i;
		base->id_desc.info.type = type_operator->type;
		base->set_cmd_item = hisi_dsd_set_cmd_item;
		base->build_data = hisi_dsd_build_data;
		sema_init(&dsd[i]->base.operator_sem, 1);
		disp_pr_info(" base->id_desc.info.idx:%u, base->id_desc.info.type:%u \n",
			base->id_desc.info.idx, base->id_desc.info.type);

		base->out_data = kzalloc(sizeof(struct hisi_dsd_out_data), GFP_KERNEL);
		if (!base->out_data) {
			disp_pr_err("alloc out_data failed\n");
			kfree(dsd[i]);
			dsd[i] = NULL;
			continue;
		}

		base->in_data = kzalloc(sizeof(struct hisi_dsd_in_data), GFP_KERNEL);
		if (!base->in_data) {
			disp_pr_err("alloc in_data failed\n");
			kfree(base->out_data);
			base->out_data = NULL;

			kfree(dsd[i]);
			dsd[i] = NULL;
			continue;
		}

		/* TODO: init other rmda operators */
		dsd[i]->base.be_dm_counted = false;
	}
	type_operator->operators = (struct hisi_comp_operator **)(dsd);
}
