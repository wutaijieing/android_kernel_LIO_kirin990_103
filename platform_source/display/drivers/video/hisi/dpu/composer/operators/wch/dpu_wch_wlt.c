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

#include "hisi_disp_composer.h"
#include "hisi_disp_gadgets.h"
#include "hisi_composer_operator.h"
#include "hisi_operator_tool.h"
#include "wlt/disp_dpu_wlt.h"

#ifdef DEBUG_BBIT_WITH_VCODEC
#include "vcodec_type.h"
#endif

static void config_wlt_line_threshold(struct disp_wb_layer *layer, struct dpu_wch_wlt *wlt)
{
	int slice_row;
	int slice_num;
	int height;

	height =  layer->src_rect.h;
	slice_num = layer->slice_num == 0 ? 4 : layer->slice_num;

	slice_row = ALIGN_UP(height / slice_num, 64);

	disp_pr_debug("wlt rect h %u, slice_row %d\n",  height, slice_row);

	int cur_row = slice_row;
	wlt->wlt_line_threshold1.value = 0xFFFFFFFF;
	wlt->wlt_line_threshold3.value = 0xFFFFFFFF;

	wlt->wlt_line_threshold1.reg.wlt_line_threshold0 = count_from_zero(cur_row);
	if (slice_num == 1)
		return;

	cur_row += slice_row;
	cur_row = MIN(cur_row, height);
	wlt->wlt_line_threshold1.reg.wlt_line_threshold1 = count_from_zero(cur_row);
	if (slice_num == 2)
		return;

	cur_row += slice_row;
	cur_row = MIN(cur_row, height);
	wlt->wlt_line_threshold3.reg.wlt_line_threshold2 = count_from_zero(cur_row);
	if (slice_num == 3)
		return;

	cur_row += slice_row;
	cur_row = MIN(cur_row, height);
	wlt->wlt_line_threshold3.reg.wlt_line_threshold3 = count_from_zero(cur_row);
}

void dpu_wch_wlt_set_reg(struct hisi_comp_operator *operator, char __iomem *wlt_base, struct dpu_wch_wlt *wlt)
{
	disp_check_and_no_retval((operator == NULL), err, "operator is NULL!\n");
	disp_check_and_no_retval((wlt_base == NULL), err, "wlt_base is NULL!\n");
	disp_check_and_no_retval((wlt == NULL), err, "wlt is NULL!\n");

	disp_pr_info("[CHECK] wlt_en is %u.\n", wlt->wlt_ctrl.reg.wlt_en);
	hisi_module_set_reg(&operator->module_desc,
		DPU_WCH_WTL_CTRL_ADDR(wlt_base), wlt->wlt_ctrl.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_WCH_WLT_LINE_THRESHOLD1_ADDR(wlt_base), wlt->wlt_line_threshold1.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_WCH_WLT_LINE_THRESHOLD3_ADDR(wlt_base), wlt->wlt_line_threshold3.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_WCH_WLT_BASE_LINE_OFFSET_ADDR(wlt_base),
		wlt->wlt_base_line_offset.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_WCH_WLT_DMA_ADDR_ADDR(wlt_base), wlt->wlt_dma_addr.value);
}

int dpu_wch_wlt_config(struct post_offline_info *offline_info,
	struct hisi_composer_data *ov_data, struct disp_wb_layer *layer,
	struct disp_rect aligned_rect)
{
	struct dpu_wch_wlt *wlt = NULL;
	int ret;
	uint64_t y_addr = 0;
	uint64_t uv_addr = 0;
	uint64_t y_header_addr = 0;
	uint64_t uv_header_addr = 0;

	disp_check_and_return((offline_info == NULL), -EINVAL, err, "NULL ptr.\n");
	disp_check_and_return((layer == NULL), -EINVAL, err, "NULL ptr.\n");
	disp_check_and_return((ov_data == NULL), -EINVAL, err, "NULL ptr.\n");

	wlt = &(ov_data->offline_module.wlt[layer->wchn_idx]);
	wlt->wlt_ctrl.value = 0;

#ifndef CONFIG_DKMD_DPU_V720
	if (layer->wchn_idx != DPU_WCHN_W1) {
		disp_pr_info("wch[%d] not surport wlt\n", layer->wchn_idx);
		return 0;
	}
#endif

	if (!(layer->need_caps & CAP_WLT)) {
		disp_pr_debug("not need wlt, 0x%x\n", layer->need_caps);
		return 0;
	}

	disp_pr_debug("need wlt, 0x%x\n", layer->need_caps);

	ov_data->offline_module.wlt_used[layer->wchn_idx] = 1;
	wlt->wlt_ctrl.reg.wlt_en = 1;

	disp_pr_info("wlt slice_num = %u\n", layer->slice_num);

	config_wlt_line_threshold(layer, wlt);

	wlt->wlt_base_line_offset.value = 0;
	wlt->wlt_base_line_offset.reg.wlt_base_line_offset = aligned_rect.line_offset;
	disp_pr_info("line_offset, %u\n", wlt->wlt_base_line_offset.reg.wlt_base_line_offset);

	disp_dpu_wlt_set_writeback_addr(layer);
	wlt->wlt_dma_addr.value = 0;
	wlt->wlt_dma_addr.reg.wlt_dma_addr = layer->dst.wlt_dma_addr;

	disp_pr_info("compress_type = %u need_caps = %u\n",
		layer->dst.compress_type, layer->need_caps);
	if (layer->dst.compress_type == COMPRESS_TYPE_HEBC) {
		y_addr = layer->dst.hebc_planes[HEBC_PANEL_Y].payload_addr;
		uv_addr = layer->dst.hebc_planes[HEBC_PANEL_UV].payload_addr;
		y_header_addr = layer->dst.hebc_planes[HEBC_PANEL_Y].header_addr;
		uv_header_addr = layer->dst.hebc_planes[HEBC_PANEL_UV].header_addr;
	} else {
		y_addr = (layer->dst.mmu_enable == 1) ? layer->dst.vir_addr : layer->dst.phy_addr;
		uv_addr = y_addr + layer->dst.offset_plane1;
	}

	disp_pr_info("bbit with venc: y_addr=%lx uv=%lx slice_addr=%lx \
		y_header_addr=%lx uv_header_addr=%lx\n", y_addr, uv_addr,
		layer->dst.wlt_dma_addr, y_header_addr, uv_header_addr);

#ifdef DEBUG_BBIT_WITH_VCODEC
	if (g_debug_bbit_venc == 1)
		venc_set_ldy_addr(y_addr, uv_addr, y_header_addr, uv_header_addr, \
		layer->dst.wlt_dma_addr);
#endif

	return 0;
}

