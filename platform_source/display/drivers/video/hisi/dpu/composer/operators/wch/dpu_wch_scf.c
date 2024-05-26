/* Copyright (c) 2013-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#include "hisi_operator_tool.h"

struct wb_scf_info {
	bool en_hscl;
	bool en_vscl;
	bool en_mmp;
	uint32_t h_ratio;
	uint32_t v_ratio;
	uint32_t h_v_order;
	uint32_t acc_hscl;
	uint32_t acc_vscl;
	uint32_t scf_en_vscl;
};

static void wb_scf_rect_set(struct wb_block *pblock_info, struct disp_wb_layer *wb_layer,
	struct disp_rect *src_rect, struct disp_rect *dst_rect)
{
	struct disp_rect temp;

	if (!g_debug_wch_scf) {
		if (pblock_info && (pblock_info->src_rect.w != 0) && (pblock_info->dst_rect.w != 0)) {
			*src_rect = pblock_info->src_rect;
			*dst_rect = pblock_info->dst_rect;
		} else {
			*src_rect = wb_layer->src_rect;
			*dst_rect = wb_layer->dst_rect;

			if (wb_layer->transform ==
				(HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_V)) {
				temp.w = dst_rect->w;
				dst_rect->w = dst_rect->h;
				dst_rect->h = temp.w;
			}
		}
	} else {
		*src_rect = wb_layer->src_rect;
		if (g_debug_wch_scf_rot) {
			dst_rect->w = wb_layer->dst_rect.h;
			dst_rect->h = wb_layer->dst_rect.w;
		} else {
			*dst_rect = wb_layer->dst_rect;
		}

		disp_pr_info("src_rect w = %d h = %d", src_rect->w, src_rect->h);
		disp_pr_info("dst_rect w = %d h = %d", dst_rect->w, dst_rect->h);
	}
}

static int wb_scf_h_ratio_set(struct wb_block *pblock_info, struct wb_scf_info *scf_info,
	struct disp_wb_layer *wb_layer, struct disp_rect src_rect, struct disp_rect dst_rect)
{
	if (!g_debug_wch_scf) {
		if (pblock_info && (pblock_info->h_ratio != 0) && (pblock_info->h_ratio != SCF_INC_FACTOR)) {
			scf_info->h_ratio = pblock_info->h_ratio;
			scf_info->en_hscl = true;
			return 0;
		}
	}

	if ((src_rect.w == dst_rect.w) || (DPU_HEIGHT(dst_rect.w) == 0))
		return 0;

	scf_info->en_hscl = true;

	if ((src_rect.w < SCF_MIN_INPUT) || (dst_rect.w < SCF_MIN_OUTPUT)) {
		disp_pr_err("src_rect.w %d small than 16, or dst_rect.w %d small than 16\n",
			src_rect.w, dst_rect.w);
		return -EINVAL;
	}

	scf_info->h_ratio = (DPU_HEIGHT(src_rect.w) * SCF_INC_FACTOR +
		SCF_INC_FACTOR / 2 - scf_info->acc_hscl) / DPU_HEIGHT(dst_rect.w);

	if ((dst_rect.w > (src_rect.w * SCF_UPSCALE_MAX)) || (src_rect.w > (dst_rect.w * SCF_DOWNSCALE_MAX))) {
		disp_pr_err("width out of range, original_src_rec %d, %d, %d, %d, "
			"src_rect %d, %d, %d, %d, dst_rect %d, %d, %d, %d\n",
			wb_layer->src_rect.x, wb_layer->src_rect.y,
			wb_layer->src_rect.w, wb_layer->src_rect.h,
			src_rect.x, src_rect.y, src_rect.w, src_rect.h,
			dst_rect.x, dst_rect.y, dst_rect.w, dst_rect.h);

			return -EINVAL;
	}

	return 0;
}

static int wb_scf_v_ratio_set(struct wb_block *pblock_info, struct wb_scf_info *scf_info,
	struct disp_wb_layer *wb_layer, struct disp_rect src_rect, struct disp_rect dst_rect)
{
	if ((src_rect.h == dst_rect.h) || (DPU_HEIGHT(dst_rect.h) == 0))
		return 0;

	disp_pr_info("src_rect w = %d h = %d", src_rect.w, src_rect.h);
	disp_pr_info("dst_rect w = %d h = %d", dst_rect.w, dst_rect.h);
	scf_info->en_vscl = true;
	scf_info->scf_en_vscl = 1;

	scf_info->v_ratio = (DPU_HEIGHT(src_rect.h) * SCF_INC_FACTOR +
		SCF_INC_FACTOR / 2 - scf_info->acc_vscl) / DPU_HEIGHT(dst_rect.h);

	if ((dst_rect.h > (src_rect.h * SCF_UPSCALE_MAX)) || (src_rect.h > (dst_rect.h * SCF_DOWNSCALE_MAX))) {
		disp_pr_err("height out of range, original_src_rec %d, %d, %d, %d, "
			"src_rect %d, %d, %d, %d, dst_rect %d, %d, %d, %d.\n",
			wb_layer->src_rect.x, wb_layer->src_rect.y,
			wb_layer->src_rect.w, wb_layer->src_rect.h,
			src_rect.x, src_rect.y, src_rect.w, src_rect.h,
			dst_rect.x, dst_rect.y, dst_rect.w, dst_rect.h);
		return -EINVAL;
	}

	return 0;
}

static void wb_scf_param_config(struct hisi_composer_data *ov_data, struct disp_wb_layer *wb_layer,
	struct wb_scf_info *scf_info, struct disp_rect src_rect, struct disp_rect dst_rect)
{
	struct dpu_scf *scf = NULL;
	bool has_pixel_alpha = false;

	scf = &(ov_data->offline_module.scf[wb_layer->wchn_idx]);
	ov_data->offline_module.scf_used[wb_layer->wchn_idx] = 1;

	has_pixel_alpha = hal_format_has_alpha(wb_layer->dst.format);

	scf->en_hscl_str.value = set_bits32(scf->en_hscl_str.value, 0x0, 1, 0); /* enbale h lvbo */

	if (src_rect.h > dst_rect.h * 4) { /* when dst < (1/4)*src need enbale v diudian */
		if (has_pixel_alpha)
			scf->en_vscl_str.value = set_bits32(scf->en_vscl_str.value, 0x3, 2, 0); /* enable v alpha diudian */
		else
			scf->en_vscl_str.value = set_bits32(scf->en_vscl_str.value, 0x1, 2, 0); /* enable v diudian*/
	} else {
		scf->en_vscl_str.value = set_bits32(scf->en_vscl_str.value, 0x0, 1, 0); /* enable v lvbo */
	}

	if (src_rect.h > dst_rect.h)
		scf_info->scf_en_vscl = 0x1;

	scf_info->en_mmp = 0x1;

	scf->h_v_order.value = set_bits32(scf->h_v_order.value, scf_info->h_v_order, 1, 0);
	scf->input_width_height.value = set_bits32(scf->input_width_height.value,
		DPU_HEIGHT(src_rect.h), 13, 0);
	scf->input_width_height.value = set_bits32(scf->input_width_height.value,
		DPU_WIDTH(src_rect.w), 13, 16);
	scf->output_width_height.value = set_bits32(scf->output_width_height.value,
		DPU_HEIGHT(dst_rect.h), 13, 0);
	scf->output_width_height.value = set_bits32(scf->output_width_height.value,
		DPU_WIDTH(dst_rect.w), 13, 16);
	scf->en_hscl.value = set_bits32(scf->en_hscl.value, (scf_info->en_hscl ? 0x1 : 0x0), 1, 0);
	scf->en_vscl.value = set_bits32(scf->en_vscl.value, scf_info->scf_en_vscl, 1, 0);
	scf->acc_hscl.value = set_bits32(scf->acc_hscl.value, scf_info->acc_hscl, 31, 0);
	scf->inc_hscl.value = set_bits32(scf->inc_hscl.value, scf_info->h_ratio, 24, 0);
	scf->inc_vscl.value = set_bits32(scf->inc_vscl.value, scf_info->v_ratio, 24, 0);
	scf->en_mmp.value = set_bits32(scf->en_mmp.value, scf_info->en_mmp, 1, 0);

	disp_pr_info("[DATA] scf in w = %d, h = %d, out w = %d, h = %d", src_rect.w, src_rect.h, dst_rect.w, dst_rect.h);
}

void dpu_wb_scf_init(const char __iomem *wch_base, struct dpu_scf *s_scl)
{
	disp_check_and_no_retval(!wch_base, err, "wch_base is NULL\n");
	disp_check_and_no_retval(!s_scl, err, "s_scl is NULL\n");

	memset(s_scl, 0, sizeof(struct dpu_scf));

	s_scl->en_hscl_str.value = inp32(DPU_WCH_SCF_EN_HSCL_STR_ADDR(wch_base));
	s_scl->en_vscl_str.value = inp32(DPU_WCH_SCF_EN_VSCL_STR_ADDR(wch_base));
	s_scl->h_v_order.value = inp32(DPU_WCH_SCF_H_V_ORDER_ADDR(wch_base));
	s_scl->input_width_height.value = inp32(DPU_WCH_SCF_INPUT_WIDTH_HEIGHT_ADDR(wch_base));
	s_scl->output_width_height.value = inp32(DPU_WCH_SCF_OUTPUT_WIDTH_HEIGHT_ADDR(wch_base));
	s_scl->en_hscl.value = inp32(DPU_WCH_SCF_EN_HSCL_ADDR(wch_base));
	s_scl->en_vscl.value = inp32(DPU_WCH_SCF_EN_VSCL_ADDR(wch_base));
	s_scl->acc_hscl.value = inp32(DPU_WCH_SCF_ACC_HSCL_ADDR(wch_base));
	s_scl->inc_hscl.value = inp32(DPU_WCH_SCF_INC_HSCL_ADDR(wch_base));
	s_scl->inc_vscl.value = inp32(DPU_WCH_SCF_INC_VSCL_ADDR(wch_base));
	s_scl->en_mmp.value = inp32(DPU_WCH_SCF_EN_MMP_ADDR(wch_base));
}

void dpu_wb_scf_set_reg(struct hisi_comp_operator *operator,
	char __iomem *wch_base, struct dpu_scf *s_scl)
{
	disp_check_and_no_retval(!wch_base, err, "wch_base is NULL\n");
	disp_check_and_no_retval(!s_scl, err, "s_scl is NULL\n");
	disp_pr_info("enter+++");

	hisi_module_set_reg(&operator->module_desc, DPU_WCH_SCF_EN_HSCL_STR_ADDR(wch_base), s_scl->en_hscl_str.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_SCF_EN_VSCL_STR_ADDR(wch_base), s_scl->en_vscl_str.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_SCF_H_V_ORDER_ADDR(wch_base), s_scl->h_v_order.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_SCF_INPUT_WIDTH_HEIGHT_ADDR(wch_base), s_scl->input_width_height.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_SCF_OUTPUT_WIDTH_HEIGHT_ADDR(wch_base), s_scl->output_width_height.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_SCF_EN_HSCL_ADDR(wch_base), s_scl->en_hscl.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_SCF_EN_VSCL_ADDR(wch_base), s_scl->en_vscl.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_SCF_ACC_HSCL_ADDR(wch_base), s_scl->acc_hscl.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_SCF_INC_HSCL_ADDR(wch_base), s_scl->inc_hscl.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_SCF_INC_VSCL_ADDR(wch_base), s_scl->inc_vscl.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_SCF_EN_MMP_ADDR(wch_base), s_scl->en_mmp.value);

	disp_pr_info("exit---");
}

int dpu_wb_scf_config(struct post_offline_info *off_info,
	struct hisi_composer_data *ov_data,
	struct disp_wb_layer *wb_layer)
{
	struct disp_rect src_rect;
	struct disp_rect dst_rect;
	struct wb_block *pblock_info = NULL;
	struct wb_scf_info scf_info = {0};

	if (!off_info || !wb_layer)
		return -EINVAL;

	if (wb_layer->wchn_idx != DPU_WCHN_W1) {
		disp_pr_err("wch[%d] do not surport scf\n", wb_layer->wchn_idx);
		return 0;
	}

	disp_pr_info("enter+++");
	pblock_info = &(wb_layer->wb_block);

	wb_scf_rect_set(pblock_info, wb_layer, &src_rect, &dst_rect);

	wb_scf_h_ratio_set(pblock_info, &scf_info, wb_layer, src_rect, dst_rect);
	wb_scf_v_ratio_set(pblock_info, &scf_info, wb_layer, src_rect, dst_rect);

	if (!scf_info.en_hscl && !scf_info.en_vscl) {
		disp_pr_info("no en_hscl && no en_vscl");
		return 0;
	}

	/* scale down, do hscl first set 0; scale up, do vscl first set 1 */
	scf_info.h_v_order = (src_rect.w > dst_rect.w) ? 0 : 1;

	if (pblock_info && (pblock_info->acc_hscl != 0))
		scf_info.acc_hscl = pblock_info->acc_hscl;

	wb_scf_param_config(ov_data, wb_layer, &scf_info, src_rect, dst_rect);
	disp_pr_info("exit---");
	return 0;
}

