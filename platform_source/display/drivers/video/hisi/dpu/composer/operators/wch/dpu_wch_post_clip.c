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

static void config_clip_rect(struct disp_rect_ltrb *clip_rect, struct disp_wb_layer *wb_layer, struct post_offline_info *off_info)
{
	clip_rect->left = g_debug_wch_clip_left;
	clip_rect->right = g_debug_wch_clip_right;
	clip_rect->top = g_debug_wch_clip_top;
	clip_rect->bottom = g_debug_wch_clip_bot;
	disp_pr_info("[DATA] clip l = %d, r = %d, t = %d, b = %d", clip_rect->left, clip_rect->right, clip_rect->top, clip_rect->bottom);
}

static void adjust_wb_ov_block (struct disp_rect *ov_block_rect, struct disp_rect_ltrb *clip_rect)
{
	ov_block_rect->w = ov_block_rect->w - (clip_rect->left + clip_rect->right);
	ov_block_rect->h = ov_block_rect->h - (clip_rect->top + clip_rect->bottom);
}

void dpu_wb_post_clip_init(const char __iomem *wch_base, struct dpu_post_clip *s_clip)
{
	if (!wch_base) {
		disp_pr_err("wch_base is NULL\n");
		return;
	}
	if (!s_clip) {
		disp_pr_err("s_clip is NULL\n");
		return;
	}

	memset(s_clip, 0, sizeof(struct dpu_post_clip));
	disp_pr_info("wch_base = 0x%p", wch_base);
	s_clip->disp_size.value = inp32(DPU_WCH_POST_CLIP_DISP_SIZE_ADDR(wch_base));
	s_clip->clip_ctl_hrz.value = inp32(DPU_WCH_POST_CLIP_CTL_HRZ_ADDR(wch_base));
	s_clip->clip_ctl_vrz.value = inp32(DPU_WCH_POST_CLIP_CTL_VRZ_ADDR(wch_base));
	s_clip->ctl_clip_en.value = inp32(DPU_WCH_POST_CLIP_EN_ADDR(wch_base));
}

void dpu_wb_post_clip_set_reg(struct hisi_comp_operator *operator, char __iomem *wch_base, struct dpu_post_clip *s_clip)
{
	disp_check_and_no_retval((wch_base == NULL), err, "wch_base is NULL!\n");
	disp_check_and_no_retval((s_clip == NULL), err, "s_clip is NULL!\n");
	disp_pr_info("enter++++ wch_base = 0x%x", wch_base);

	hisi_module_set_reg(&operator->module_desc, DPU_WCH_POST_CLIP_DISP_SIZE_ADDR(wch_base), s_clip->disp_size.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_POST_CLIP_CTL_HRZ_ADDR(wch_base), s_clip->clip_ctl_hrz.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_POST_CLIP_CTL_VRZ_ADDR(wch_base), s_clip->clip_ctl_vrz.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_POST_CLIP_EN_ADDR(wch_base), s_clip->ctl_clip_en.value);

	disp_pr_info("exit----");
}

static bool is_post_clip_needed (struct disp_rect_ltrb *clip_rect)
{
	if (clip_rect->left != 0 || clip_rect->right != 0 || clip_rect->bottom != 0 || clip_rect->top != 0) {
		disp_pr_info("clip left = %d, right = %d, bot = %d, top = %d",
			clip_rect->left, clip_rect->right, clip_rect->bottom, clip_rect->top);
		return true;
	}

	return false;
}

int dpu_wb_post_clip_config(struct post_offline_info *off_info, struct hisi_composer_data *ov_data, struct disp_wb_layer *layer)
{
	struct dpu_post_clip *post_clip = NULL;
	struct disp_rect *ov_block_rect = NULL;
	int chn_idx;
	struct disp_rect post_clip_rect;

	disp_check_and_return(!ov_data, -EINVAL, err, "off_info is nullptr!\n");
	disp_check_and_return(!off_info, -EINVAL, err, "off_info is nullptr!\n");
	disp_check_and_return(!layer, -EINVAL, err, "layer is nullptr!\n");
	chn_idx = layer->wchn_idx;

	if (chn_idx != DPU_WCHN_W1) {
		disp_pr_err("wch[%d] do not surport post_clip\n", chn_idx);
		return 0;
	}
	post_clip_rect = layer->dst_rect;
	struct disp_rect_ltrb clip_rect;
	memset(&clip_rect, 0, sizeof(struct disp_rect_ltrb));

	config_clip_rect(&clip_rect, layer, off_info);
	ov_block_rect = &off_info->wb_ov_block_rect;
	disp_pr_info("[DATA] in post_clip w = %d h = %d", ov_block_rect->w, ov_block_rect->h);

	post_clip = &(ov_data->offline_module.clip[chn_idx]);
	if (is_post_clip_needed(&clip_rect)) {
		ov_data->offline_module.post_clip_used[chn_idx] = 1;
	}

	post_clip->disp_size.value = 0;
	post_clip->disp_size.reg.post_clip_size_vrt = DPU_HEIGHT(ov_block_rect->h);
	post_clip->disp_size.reg.post_clip_size_hrz = DPU_HEIGHT(ov_block_rect->w);

	adjust_wb_ov_block(ov_block_rect, &clip_rect);
	disp_pr_info("[DATA] out post_clip w = %d h = %d", ov_block_rect->w, ov_block_rect->h);
	post_clip->clip_ctl_hrz.value = 0;
	post_clip->clip_ctl_hrz.reg.post_clip_right = clip_rect.right;
	post_clip->clip_ctl_hrz.reg.post_clip_left = clip_rect.left;

	post_clip->clip_ctl_vrz.value = 0;
	post_clip->clip_ctl_vrz.reg.post_clip_top = clip_rect.top;
	post_clip->clip_ctl_vrz.reg.post_clip_bot = clip_rect.bottom;

	post_clip->ctl_clip_en.value = 0;
	post_clip->ctl_clip_en.reg.post_clip_enable = 0x1;
	return 0;
}

