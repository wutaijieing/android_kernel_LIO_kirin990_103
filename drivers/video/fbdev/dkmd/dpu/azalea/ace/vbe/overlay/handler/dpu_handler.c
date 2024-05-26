/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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
#include "../../dpu_display_effect.h"
#include "../../dpu_dpe_utils.h"
#include "../../dpu_mmbuf_manager.h"
#include "../../dpu_fb_panel.h"
#include "../../dpu_smmu_base.h"
#include "../../dpu_fb_panel.h"
#include "../../merger_mgr/dpu_disp_merger_mgr.h"

#define OV_COMPOSE_PRE 0
#define OV_COMPOSE_RDMA_IN 1
#define OV_COMPOSE_RDMA_OUT 2
#define OV_COMPOSE_RDFC_OUT 3
#define OV_COMPOSE_SCF_OUT 4

static void unflow_mipi_ldi_int_handler(char __iomem *mipi_ldi_base, bool unmask)
{
	uint32_t temp;

	temp = inp32(mipi_ldi_base + MIPI_LDI_CPU_ITF_INT_MSK);
	if (unmask)
		temp &= ~BIT_LDI_UNFLOW;
	else
		temp |= BIT_LDI_UNFLOW;

	outp32(mipi_ldi_base + MIPI_LDI_CPU_ITF_INT_MSK, temp);
}

void dpu_unflow_handler(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req, bool unmask)
{
	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is null\n");
	dpu_check_and_no_retval(!pov_req, ERR, "pov_req is null\n");

	if (pov_req->ovl_idx == DSS_OVL0) {
		unflow_mipi_ldi_int_handler(dpufd->mipi_dsi0_base, unmask);
		if (is_dual_mipi_panel(dpufd))
			unflow_mipi_ldi_int_handler(dpufd->mipi_dsi1_base, unmask);
	} else if (pov_req->ovl_idx == DSS_OVL1) {
		unflow_mipi_ldi_int_handler(dpufd->mipi_dsi1_base, unmask);
	} else {
		;  /* do nothing */
	}
}

static void dpu_evs_switch(struct dpu_fb_data_type *dpufd, const void __user *argp)
{
	int ret = 0;
	int evs_enable = 0;

	ret = (int)copy_from_user(&evs_enable, argp, sizeof(int));
	if (ret) {
		DPU_FB_ERR("copy arg fail\n");
		return;
	}
	dpufd->evs_enable = (evs_enable > 0) ? true : false;

	DPU_FB_INFO("DPUFB_EVS_SWITCH, evs enable:%d, evs fb index %d\n",
		evs_enable, dpufd->index);
}

static bool is_base_layer(dss_layer_t *layer, dss_overlay_block_t *pov_h_block, bool *has_base)
{
	if (layer->need_cap & (CAP_BASE | CAP_DIM | CAP_PURE_COLOR)) {
		if (layer->need_cap & CAP_BASE)
			*has_base = true;

		return true;
	}

	if (layer->dst_rect.y < pov_h_block->ov_block_rect.y) {
		if (g_debug_ovl_block_composer)
			DPU_FB_INFO("%s=%d, %s=%d, %s=%d, %s=%d!!!!\n",
				"layer->dst_rect.y", layer->dst_rect.y,
				"pov_h_block->ov_block_rect.y", pov_h_block->ov_block_rect.y,
				"layer->chn_idx", layer->chn_idx,
				"layer->layer_idx", layer->layer_idx);

		return true;
	}

	return false;
}

static int dpu_ov_compose_base_config(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req,
	dss_layer_t *layer, dss_rect_t *wb_ov_block_rect, bool *has_base)
{
	int ret;

	ret = dpu_ovl_layer_config(dpufd, pov_req, layer, wb_ov_block_rect, *has_base);
	if (ret != 0) {
		DPU_FB_ERR("dpu_ovl_config failed! need_cap=0x%x, ret=%d\n", layer->need_cap, ret);
		return ret;
	}

	ret = dpu_mctl_ch_config(dpufd, layer, NULL, wb_ov_block_rect, *has_base);
	if (ret != 0) {
		DPU_FB_ERR("dpu_mctl_ch_config failed! ret = %d\n", ret);
		return ret;
	}

	return ret;
}

static void ov_compose_print(uint8_t type, struct dpu_fb_data_type *dpufd, dss_layer_t *layer,
	dss_rect_ltrb_t *clip_rect, dss_rect_t *aligned_rect)
{
	if ((g_debug_ovl_online_composer >0) || (g_debug_ovl_offline_composer > 0)) {
		switch (type) {
		case OV_COMPOSE_RDMA_IN:
			DPU_FB_INFO("fb%d, rdma input, %s:%d,%d,%d,%d, %s%d,%d,%d,%d, %s:%d,%d,%d,%d\n",
				dpufd->index,
				"src_rect", layer->src_rect.x, layer->src_rect.y, layer->src_rect.w, layer->src_rect.h,
				"src_rect_mask", layer->src_rect_mask.x, layer->src_rect_mask.y,
				layer->src_rect_mask.w, layer->src_rect_mask.h,
				"dst_rect", layer->dst_rect.x, layer->dst_rect.y, layer->dst_rect.w, layer->dst_rect.h);

			break;
		case OV_COMPOSE_RDMA_OUT:
			DPU_FB_INFO("fb%d, rdma output, %s:%d,%d,%d,%d, %s:%d,%d,%d,%d, %s:%d,%d,%d,%d\n",
				dpufd->index,
				"clip_rect", clip_rect->left, clip_rect->right, clip_rect->top, clip_rect->bottom,
				"aligned_rect", aligned_rect->x, aligned_rect->y, aligned_rect->w, aligned_rect->h,
				"dst_rect", layer->dst_rect.x, layer->dst_rect.y, layer->dst_rect.w, layer->dst_rect.h);

			break;
		case OV_COMPOSE_RDFC_OUT:
			DPU_FB_INFO("fb%d, rdfc output, %s:%d,%d,%d,%d, %s:%d,%d,%d,%d, %s:%d,%d,%d,%d\n",
				dpufd->index,
				"clip_rect", clip_rect->left, clip_rect->right, clip_rect->top, clip_rect->bottom,
				"aligned_rect", aligned_rect->x, aligned_rect->y, aligned_rect->w, aligned_rect->h,
				"dst_rect", layer->dst_rect.x, layer->dst_rect.y, layer->dst_rect.w, layer->dst_rect.h);

			break;
		case OV_COMPOSE_SCF_OUT:
			DPU_FB_INFO("fb%d, scf output, %s:%d,%d,%d,%d, %s:%d,%d,%d,%d, %s:%d,%d,%d,%d\n",
				dpufd->index,
				"clip_rect", clip_rect->left, clip_rect->right, clip_rect->top, clip_rect->bottom,
				"aligned_rect", aligned_rect->x, aligned_rect->y, aligned_rect->w, aligned_rect->h,
				"dst_rect", layer->dst_rect.x, layer->dst_rect.y, layer->dst_rect.w, layer->dst_rect.h);

			break;
		default:
			break;
		}
	}
}

static int dpu_ov_composer_spec_config(struct dpu_fb_data_type *dpufd,
	dss_layer_t *layer,
	struct dpu_ov_compose_rect *ov_compose_rect,
	struct dpu_ov_compose_flag *ov_compose_flag)
{
	int ret;
	dss_overlay_t *pov_req = &(dpufd->ov_req);

	ret = dpu_smmu_ch_config(dpufd, layer, NULL);
	dpu_check_and_return((ret != 0), ret, ERR, "dpu_smmu_ch_config failed! ret = %d\n", ret);

	ov_compose_print(OV_COMPOSE_RDMA_IN, dpufd, layer, ov_compose_rect->clip_rect, ov_compose_rect->aligned_rect);
	ret = dpu_rdma_config(dpufd, layer, ov_compose_rect, ov_compose_flag);
	dpu_check_and_return((ret != 0), ret, ERR, "dpu_rdma_config failed! ret = %d\n", ret);
	ov_compose_print(OV_COMPOSE_RDMA_OUT, dpufd, layer, ov_compose_rect->clip_rect, ov_compose_rect->aligned_rect);

	ret = dpu_aif_ch_config(dpufd, pov_req, layer, ov_compose_rect->wb_dst_rect, NULL);
	dpu_check_and_return((ret != 0), ret, ERR, "dpu_aif_ch_config failed! ret = %d\n", ret);

	ret = dpu_aif1_ch_config(dpufd, pov_req, layer, NULL, pov_req->ovl_idx);
	dpu_check_and_return((ret != 0), ret, ERR, "dpu_aif1_ch_config failed! ret = %d\n", ret);

	ret = dpu_mif_config(dpufd, layer, NULL, ov_compose_flag->rdma_stretch_enable);
	dpu_check_and_return((ret != 0), ret, ERR, "dpu_mif_config failed! ret = %d\n", ret);

	ret = dpu_rdfc_config(dpufd, layer, ov_compose_rect->aligned_rect, *ov_compose_rect->clip_rect, ov_compose_flag);
	dpu_check_and_return((ret != 0), ret, ERR, "dpu_rdfc_config failed! ret = %d\n", ret);
	ov_compose_print(OV_COMPOSE_RDFC_OUT, dpufd, layer, ov_compose_rect->clip_rect, ov_compose_rect->aligned_rect);

	ret = dpu_scl_config(dpufd, layer, ov_compose_rect->aligned_rect, ov_compose_flag->rdma_stretch_enable);
	dpu_check_and_return((ret != 0), ret, ERR, "dpu_scl_config failed! ret = %d\n", ret);

	ret = dpu_arsr2p_config(dpufd, layer, ov_compose_rect->aligned_rect,
		ov_compose_flag->rdma_stretch_enable);
	dpu_check_and_return((ret != 0), ret, ERR, "dpu_arsr2p_config failed! ret = %d\n", ret);

	ret = dpu_post_clip_config(dpufd, layer);
	dpu_check_and_return((ret != 0), ret, ERR, "dpu_post_clip_config failed! ret = %d\n", ret);

	ov_compose_print(OV_COMPOSE_SCF_OUT, dpufd, layer, ov_compose_rect->clip_rect, ov_compose_rect->aligned_rect);

	if (ov_compose_flag->csc_needed) {
		ret = dpu_csc_config(dpufd, layer, NULL);
		dpu_check_and_return((ret != 0), ret, ERR, "dpu_csc_config failed! ret = %d\n", ret);
	}

	return ret;
}

int dpu_ov_compose_handler(struct dpu_fb_data_type *dpufd,
	dss_overlay_block_t *pov_h_block,
	dss_layer_t *layer,
	struct dpu_ov_compose_rect *ov_compose_rect,
	struct dpu_ov_compose_flag *ov_compose_flag)
{
	int ret;
	int32_t mctl_idx;
	dss_overlay_t *pov_req = NULL;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");
	pov_req = &(dpufd->ov_req);

	dpu_check_and_return(!pov_h_block, -EINVAL, ERR, "pov_h_block is NULL\n");
	dpu_check_and_return(!layer, -EINVAL, ERR, "layer is NULL\n");
	dpu_check_and_return(!ov_compose_rect, -EINVAL, ERR, "ov_compose_rect is NULL\n");
	dpu_check_and_return((!ov_compose_rect->aligned_rect || !ov_compose_rect->clip_rect),
		-EINVAL, ERR, "aligned_rect or clip_rect is NULL\n");
	dpu_check_and_return(!ov_compose_flag, -EINVAL, ERR, "ov_compose_flag is NULL\n");

	if ((dpufd->index == MEDIACOMMON_PANEL_IDX) && g_enable_ovl_async_composer) {
		;
	} else {
		ret = dpufd->fb_pan_display ? 0 : dpu_check_layer_par(dpufd, layer);
		dpu_check_and_return((ret != 0), ret, ERR, "dpu_check_layer_par failed! ret = %d\n", ret);
	}

	/* assume next frame use online play. */
	dpufd->fb_pan_display = false;
	if (g_debug_ovl_block_composer)
		DPU_FB_INFO("%s=%d, %s=%d, %s=%d, %s=%d\n",
			"layer->dst_rect.y", layer->dst_rect.y,
			"pov_h_block->ov_block_rect.y", pov_h_block->ov_block_rect.y,
			"layer->chn_idx", layer->chn_idx,
			"layer->layer_idx", layer->layer_idx);

	ov_compose_print(OV_COMPOSE_PRE, dpufd, layer, ov_compose_rect->clip_rect, ov_compose_rect->aligned_rect);

	if (is_base_layer(layer, pov_h_block, &ov_compose_flag->has_base) == true) {
		ret = dpu_ov_compose_base_config(dpufd, pov_req, layer, ov_compose_rect->wb_ov_block_rect,
			&ov_compose_flag->has_base);
		return ret;
	}

	ret = dpu_ov_composer_spec_config(dpufd, layer, ov_compose_rect, ov_compose_flag);
	if (ret != 0)
		return ret;

	ret = dpu_ov_compose_base_config(dpufd, pov_req, layer, ov_compose_rect->wb_ov_block_rect,
		&ov_compose_flag->has_base);
	if (ret != 0)
		return ret;

	mctl_idx = pov_req->ovl_idx;
	ret = dpu_ch_module_set_regs(dpufd, mctl_idx, layer->chn_idx, 0, ov_compose_flag->enable_cmdlist);
	dpu_check_and_return((ret != 0), ret, ERR, "fb%d, dpu_ch_module_set_regs failed! ret = %d\n",
		dpufd->index, ret);

	return 0;
}

void dpu_vactive0_start_isr_handler(struct dpu_fb_data_type *dpufd)
{
	struct dpufb_vsync *vsync_ctrl = NULL;
	ktime_t pre_vactive_timestamp;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL\n");
		return;
	}

	vsync_ctrl = &(dpufd->vsync_ctrl);
	pre_vactive_timestamp = vsync_ctrl->vactive_timestamp;
	vsync_ctrl->vactive_timestamp = systime_get();

	if (is_mipi_cmd_panel(dpufd) && (dpufd->frame_update_flag == 0))
		dpufd->vactive0_start_flag = 1;
	else
		dpufd->vactive0_start_flag++;
	wake_up_interruptible_all(&dpufd->vactive0_start_wq);

	if (g_debug_online_vactive)
		DPU_FB_INFO("fb%d, VACTIVE =%llu, time_diff=%llu.\n", dpufd->index, \
			ktime_to_ns(vsync_ctrl->vactive_timestamp), \
			(ktime_to_ns(vsync_ctrl->vactive_timestamp) - ktime_to_ns(pre_vactive_timestamp)));
}

static int dpufb_ov_online_play(struct dpu_fb_data_type *dpufd, void __user *argp)
{
	uint32_t timediff;
	struct timeval tv0;
	struct timeval tv1;
	int ret = 0;
	DPU_FB_DEBUG("fb%u +\n", dpufd->index);

	if (dpufd->online_play_count < ONLINE_PLAY_LOG_PRINTF)
		DPU_FB_INFO("[online_play_count = %d] fb%d DPUFB_OV_ONLINE_PLAY\n",
			dpufd->online_play_count, dpufd->index);

	dpufd->online_play_count++;

	if (dpufd->ov_online_play != NULL) {
		if (g_debug_ovl_online_composer_timediff & 0x1)
			dpufb_get_timestamp(&tv0);

		down(&dpufd->blank_sem);
		ret = dpufd->ov_online_play(dpufd, argp);
		if (ret != 0)
			DPU_FB_ERR("fb%d ov_online_play failed!\n", dpufd->index);

		up(&dpufd->blank_sem);

		if (g_debug_ovl_online_composer_timediff & 0x1) {
			dpufb_get_timestamp(&tv1);
			timediff = dpufb_timestamp_diff(&tv0, &tv1);
			if (timediff >= g_online_comp_timeout) //lint !e737 !e574
				DPU_FB_ERR("ONLING_IOCTL_TIMEDIFF is %u us!\n", timediff);
		}
	}

	DPU_FB_DEBUG("fb%u -\n", dpufd->index);
	return ret;
}

static int dpu_exec_online_play(struct dpu_fb_data_type *dpufd, void __user *argp)
{
	int ret = 0;
	struct dpu_core_disp_data ov_info;
	struct timeval tv_start;

	dpu_trace_ts_begin(&tv_start);
	ret = (int)(copy_from_user(&ov_info, argp, sizeof(struct dpu_core_disp_data)));
	if (ret != 0) {
		DPU_FB_ERR("fb%d, copy_from_user failed!\n", dpufd->index);
		return -1;
	}

	if (is_support_disp_merge(dpufd)) {
		if (dpufd->merger_mgr.ops && dpufd->merger_mgr.ops->commit_task)
			ret = dpufd->merger_mgr.ops->commit_task(&dpufd->merger_mgr, argp);
	} else {
		ret = dpufb_ov_online_play(dpufd, argp);
	}

	dpu_check_and_return(ret != 0, -1, ERR, "fb%u online play exec failed\n", dpufd->index);
	dpu_trace_ts_end(&tv_start, g_exec_online_threshold, "fb[%u] frame_no[%u] exec online run",
		dpufd->index, ov_info.frame_no);
	return 0;
}

int dpu_overlay_ioctl_handler(struct dpu_fb_data_type *dpufd,
	uint32_t cmd, void __user *argp)
{
	int ret = 0;

	if ((dpufd == NULL) || (argp == NULL)) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	switch (cmd) {
	case DPUFB_OV_ONLINE_PLAY:
		ret = dpu_exec_online_play(dpufd, argp);
		break;
	case DPUFB_OV_OFFLINE_PLAY:
		if (dpufd->ov_offline_play != NULL) {
			ret = dpufd->ov_offline_play(dpufd, argp);
			if (ret < 0)
				DPU_FB_ERR("fb%d ov_offline_play failed!\n", dpufd->index);
		}
		break;
	case DPUFB_ONLINE_PLAY_BYPASS:
		ret = dpu_online_play_bypass(dpufd, argp);
		if (ret != 0)
			DPU_FB_ERR("fb%d online_play_bypass failed!\n", dpufd->index);
		break;
	case DPUFB_EVS_SWITCH:
		dpu_evs_switch(dpufd, argp);
		break;
	case DPUFB_DISP_MERGE_ENABLE:
		if (dpufd->merger_mgr.ops && dpufd->merger_mgr.ops->ctl_enable_for_usr) {
			ret = dpufd->merger_mgr.ops->ctl_enable_for_usr(&dpufd->merger_mgr, argp);
			if (ret != 0)
				DPU_FB_ERR("fb%d ctl disp merger enable failed!\n", dpufd->index);
		}
		break;
	default:
		DPU_FB_INFO("ioctl unsupport cmd:%u!\n", cmd);
		break;
	}

	return ret;
}
