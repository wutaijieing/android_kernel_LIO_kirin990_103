/*
 * Copyright (c) 2013-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#include "../../dpu_ovl_online_wb.h"
#include "../../dpu_mmbuf_manager.h"
#include "../../spr/dpu_spr.h"
#include "../../dpu_frame_rate_ctrl.h"

#if defined(CONFIG_ASYNCHRONOUS_PLAY)
#include "../../dpu_overlay_asynchronous_play.h"
#endif

#define OV_COMPOSE_PRE 0
#define OV_COMPOSE_RDMA_IN 1
#define OV_COMPOSE_RDMA_OUT 2
#define OV_COMPOSE_RDFC_OUT 3
#define OV_COMPOSE_SCF_OUT 4

#ifdef SUPPORT_DSI_VER_2_0
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
	uint32_t temp = 0;

	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is null\n");
	dpu_check_and_no_retval(!pov_req, ERR, "pov_req is null\n");

	if (pov_req->ovl_idx == DSS_OVL0) {
		unflow_mipi_ldi_int_handler(dpufd->mipi_dsi0_base, unmask);
		if (is_dual_mipi_panel(dpufd))
			unflow_mipi_ldi_int_handler(dpufd->mipi_dsi1_base, unmask);
	} else if (pov_req->ovl_idx == DSS_OVL1) {
		if (is_dp_panel(dpufd)) {
			temp = inp32(dpufd->dss_base + DSS_LDI_DP_OFFSET + LDI_CPU_ITF_INT_MSK);
			if (unmask)
				temp &= ~BIT_LDI_UNFLOW;
			else
				temp |= BIT_LDI_UNFLOW;

			outp32(dpufd->dss_base + DSS_LDI_DP_OFFSET + LDI_CPU_ITF_INT_MSK, temp);

			temp = inp32(dpufd->dss_base + DSS_LDI_DP1_OFFSET + LDI_CPU_ITF_INT_MSK);
			if (unmask)
				temp &= ~BIT_LDI_UNFLOW;
			else
				temp |= BIT_LDI_UNFLOW;

			outp32(dpufd->dss_base + DSS_LDI_DP1_OFFSET + LDI_CPU_ITF_INT_MSK, temp);
		} else {
			unflow_mipi_ldi_int_handler(dpufd->mipi_dsi1_base, unmask);
		}
	} else {
		;  /* do nothing */
	}
}
#else
void dpu_unflow_handler(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req, bool unmask)
{
	uint32_t tmp = 0;
	char __iomem *ldi_base = NULL;

	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is null\n");
	dpu_check_and_no_retval(!pov_req, ERR, "pov_req is null\n");

#ifdef CONFIG_DPU_FB_V501
	if (is_dp_panel(dpufd))
		ldi_base = dpufd->dss_base + DSS_LDI_DP_OFFSET;
	else
		ldi_base = dpufd->dss_base + DSS_LDI1_OFFSET;
#else
	ldi_base = dpufd->dss_base + DSS_LDI1_OFFSET;
#endif

	if (pov_req->ovl_idx == DSS_OVL0) {
		tmp = inp32(dpufd->dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INT_MSK);
		if (unmask)
			tmp &= ~BIT_LDI_UNFLOW;
		else
			tmp |= BIT_LDI_UNFLOW;

		outp32(dpufd->dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INT_MSK, tmp);
	} else if (pov_req->ovl_idx == DSS_OVL1) {
		tmp = inp32(ldi_base + LDI_CPU_ITF_INT_MSK);
		if (unmask)
			tmp &= ~BIT_LDI_UNFLOW;
		else
			tmp |= BIT_LDI_UNFLOW;

		outp32(ldi_base + LDI_CPU_ITF_INT_MSK, tmp);
	} else {
		; /* do nothing */
	}
}
#endif

static void dpu_evs_switch(struct dpu_fb_data_type *dpufd, const void __user *argp)
{
	int ret = 0;
	int evs_enable = 0;

	ret = (int)copy_from_user(&evs_enable, argp, sizeof(int));
	if (ret) {
		DPU_FB_ERR("copy arg fail\n");
		return;
	}
	dpufd->evs_enable = evs_enable ? true : false;

	DPU_FB_INFO("DPUFB_EVS_SWITCH, evs enable:%d, evs fb index %d\n",
		evs_enable, dpufd->index);
}

#if defined(CONFIG_CDC_MULTI_DISPLAY)
static int dpu_fd_evs_is_enable(const struct dpu_fb_data_type *dpufd)
{
	return ((dpufd->evs_enable) ? 1 : 0);
}

static int dpu_evs_play_bypass_check(const struct dpu_fb_data_type *dpufd, const void __user *argp)
{
	int ret = 0;
	dss_overlay_t ovreq_evs_or_sf;

	if (dpu_fd_evs_is_enable(dpufd) == 0)
		return 0;

	ret = (int)copy_from_user(&ovreq_evs_or_sf, argp, sizeof(dss_overlay_t));
	if (ret) {
		DPU_FB_ERR("copy arg fail for check ovreq");
		return 0;
	}

	if (ovreq_evs_or_sf.is_evs_request != 1)
		return 1;

	return 0;
}
#endif

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
	if (g_debug_ovl_online_composer || g_debug_ovl_offline_composer) {
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

#if defined(CONFIG_RCH_CLD)
	ret = dpu_cld_config(dpufd, layer);
	dpu_check_and_return((ret != 0), ret, ERR, "dpu_cld_config failed! ret = %d\n", ret);
#endif

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

int dpu_wb_compose_handler(struct dpu_fb_data_type *dpufd,
	dss_wb_layer_t *wb_layer,
	dss_rect_t *wb_ov_block_rect,
	bool last_block,
	struct dpu_ov_compose_flag ov_compose_flag)
{
	int ret = 0;
	dss_rect_t aligned_rect;
	dss_overlay_t *pov_req = NULL;

	memset(&aligned_rect, 0, sizeof(aligned_rect));

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL Point!\n");
	dpu_check_and_return(!wb_layer, -EINVAL, ERR, "wb_layer is NULL Point!\n");

	pov_req = &(dpufd->ov_req);

	if (ov_compose_flag.csc_needed) {
		ret = dpu_csc_config(dpufd, NULL, wb_layer);
		dpu_check_and_return((ret != 0), ret, ERR, "dpu_csc_config failed! ret = %d\n", ret);
	}

	if (g_debug_ovl_online_composer || g_debug_ovl_offline_composer)
		DPU_FB_INFO("fb%d, wdfc input, src_rect:%d,%d,%d,%d, dst_rect:%d,%d,%d,%d.\n",
			dpufd->index,
			wb_layer->src_rect.x, wb_layer->src_rect.y, wb_layer->src_rect.w, wb_layer->src_rect.h,
			wb_layer->dst_rect.x, wb_layer->dst_rect.y, wb_layer->dst_rect.w, wb_layer->dst_rect.h);

#if !defined(CONFIG_DPU_FB_V320) && !defined(CONFIG_DPU_FB_V330)
	ret = dpu_wb_scl_config(dpufd, wb_layer);
#endif
	dpu_check_and_return((ret != 0), ret, ERR, "dpu_wb_scl_config failed, ret = %d\n", ret);

	ret = dpu_wdfc_config(dpufd, wb_layer, &aligned_rect, wb_ov_block_rect);
	dpu_check_and_return((ret != 0), ret, ERR, "dpu_wdfc_config failed, ret = %d\n", ret);

	if (g_debug_ovl_online_composer || g_debug_ovl_offline_composer)
		DPU_FB_INFO("fb%d, wdfc output, aligned_rect:%d,%d,%d,%d, dst_rect:%d,%d,%d,%d.\n",
			dpufd->index, aligned_rect.x, aligned_rect.y, aligned_rect.w, aligned_rect.h,
			wb_layer->dst_rect.x, wb_layer->dst_rect.y, wb_layer->dst_rect.w, wb_layer->dst_rect.h);

	ret = dpu_wdma_config(dpufd, wb_layer, aligned_rect, wb_ov_block_rect, last_block);
	dpu_check_and_return((ret != 0), ret, ERR, "dpu_wdma_config failed, ret = %d\n", ret);

	if (g_debug_ovl_online_composer || g_debug_ovl_offline_composer)
		DPU_FB_INFO("fb%d, wdma output, aligned_rect:%d,%d,%d,%d, dst_rect:%d,%d,%d,%d.\n",
			dpufd->index, aligned_rect.x, aligned_rect.y, aligned_rect.w, aligned_rect.h,
			wb_layer->dst_rect.x, wb_layer->dst_rect.y, wb_layer->dst_rect.w, wb_layer->dst_rect.h);

	ret = dpu_mif_config(dpufd, NULL, wb_layer, false);
	dpu_check_and_return((ret != 0), ret, ERR, "dpu_mif_config failed! ret = %d\n", ret);

	ret = dpu_aif1_ch_config(dpufd, pov_req, NULL, wb_layer, pov_req->ovl_idx);
	dpu_check_and_return((ret != 0), ret, ERR, "dpu_aif1_ch_config failed! ret = %d\n", ret);

	ret = dpu_aif_ch_config(dpufd, pov_req, NULL, NULL, wb_layer);
	dpu_check_and_return((ret != 0), ret, ERR, "dpu_aif_ch_config failed! ret = %d\n", ret);

	ret = dpu_smmu_ch_config(dpufd, NULL, wb_layer);
	dpu_check_and_return((ret != 0), ret, ERR, "dpu_smmu_ch_config failed! ret = %d\n", ret);

	ret = dpu_mctl_ch_config(dpufd, NULL, wb_layer, wb_ov_block_rect, 0);
	dpu_check_and_return((ret != 0), ret, ERR, "dpu_mctl_ch_config failed! ret = %d\n", ret);

	return 0;
}

int dpu_wb_ch_module_set_regs(struct dpu_fb_data_type *dpufd, uint32_t wb_type,
	dss_wb_layer_t *wb_layer, bool enable_cmdlist)
{
	int32_t mctl_idx;
	int ret;
	dss_overlay_t *pov_req_h_v = NULL;

	dpu_check_and_return((!dpufd || !wb_layer), -EINVAL, ERR, "dpufd or wb_layer is NULL Point!\n");

	pov_req_h_v = &(dpufd->ov_req);

	/* dpuv400 copybit */
	mctl_idx = (wb_layer->chn_idx == DSS_WCHN_W2) ? DSS_MCTL5 : pov_req_h_v->ovl_idx;
	ret = dpu_ch_module_set_regs(dpufd, mctl_idx, wb_layer->chn_idx, wb_type, enable_cmdlist);
	dpu_check_and_return(ret != 0, ret, ERR, "fb%d, set reg fail, ret = %d\n", dpufd->index, ret);

	return 0;
}

#ifndef CONFIG_DPU_FB_V320
static void dpu_vactive0_start_disable_ldi(struct dpu_fb_data_type *dpufd) {
	if (dpu_check_panel_product_type(dpufd->panel_info.product_type))
		disable_ldi(dpufd);
	else if ((dpufd->secure_ctrl.secure_status == dpufd->secure_ctrl.secure_event) &&
		(dpufd->secure_ctrl.secure_status == DSS_SEC_IDLE))
		disable_ldi(dpufd);
}
#endif

void dpu_vactive0_start_isr_handler(struct dpu_fb_data_type *dpufd)
{
	struct dpufb_vsync *vsync_ctrl = NULL;
	ktime_t pre_vactive_timestamp;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL\n");
		return;
	}

	if (is_mipi_cmd_panel(dpufd) && (dpufd->frame_update_flag == 0)) {
		dpufd->vactive0_start_flag = 1;

#if defined(CONFIG_DPU_FB_V320)
		if ((dpufd->secure_ctrl.secure_status == dpufd->secure_ctrl.secure_event) &&
			(dpufd->secure_ctrl.secure_status == DSS_SEC_IDLE) &&
			!dpufb_display_effect_is_need_ace(dpufd))
			disable_ldi(dpufd);
#else
		dpu_vactive0_start_disable_ldi(dpufd);
#endif
	} else {
		dpufd->vactive0_start_flag++;
	}

	wake_up_interruptible_all(&dpufd->vactive0_start_wq);

#if defined(CONFIG_ASYNCHRONOUS_PLAY)
	if (dpu_is_async_play(dpufd)) {
		dpufd->asynchronous_vactive0_start_flag = 1;
		wake_up_interruptible_all(&dpufd->asynchronous_play_wq);
	}
#endif

	if (g_debug_online_vactive) {
		vsync_ctrl = &(dpufd->vsync_ctrl);

		pre_vactive_timestamp = vsync_ctrl->vactive_timestamp;
		vsync_ctrl->vactive_timestamp = ktime_get();

		DPU_FB_INFO("fb%d, VACTIVE =%llu, time_diff=%llu.\n", dpufd->index,
			ktime_to_ns(vsync_ctrl->vactive_timestamp),
			(ktime_to_ns(vsync_ctrl->vactive_timestamp) - ktime_to_ns(pre_vactive_timestamp)));
	}
}

void dpu_vactive0_end_isr_handler(struct dpu_fb_data_type *dpufd)
{
	uint32_t delaycount = 0;
	uint32_t task_name = 0;
	bool timeout = false;

	if ((dpufd == NULL) || (dpufd->index != PRIMARY_PANEL_IDX))
		return;

	if (dpufd->panel_info.mode_switch_state == PARA_UPDT_DOING)
		task_name = BIT(1);

	if (task_name) {
		while ((delaycount++) <= 5000) {  /* wait timers */
			if (task_name & BIT(1)) {
				if (dpufd->panel_info.mode_switch_state == PARA_UPDT_END)
					break;
			}
			udelay(10);
		}

		if (delaycount > 5000)  /* wait timers */
			timeout = true;
	}

	if (timeout)
		DPU_FB_ERR("wait task[0x%x] execution end timeout.\n", task_name);
}

#if defined(CONFIG_ASYNCHRONOUS_PLAY)
static void dpu_set_ov_async_play(struct dpu_fb_data_type *dpufd, void __user *argp)
{
	int ret;
	uint32_t timediff;
	timeval_compatible tv0;
	timeval_compatible tv1;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL Pointer\n");
		return;
	}

#if defined(CONFIG_CDC_MULTI_DISPLAY)
	if (dpu_evs_play_bypass_check(dpufd, argp) == 1)
		return;
#endif

	if (dpufd->online_play_count < ONLINE_PLAY_LOG_PRINTF) {
		DPU_FB_INFO("[online_play_count = %d] fb%d online_play_count\n",
			dpufd->online_play_count, dpufd->index);
	}
	dpufd->online_play_count++;

	if (dpufd->ov_asynchronous_play != NULL) {
		if (g_debug_ovl_online_composer_timediff & 0x1)
			dpufb_get_timestamp(&tv0);

		down(&dpufd->blank_sem);
		ret = dpufd->ov_asynchronous_play(dpufd, argp);
		if (ret != 0)
			DPU_FB_ERR("fb%d ov_asynchronous_play failed!\n", dpufd->index);

		up(&dpufd->blank_sem);

		if (g_debug_ovl_online_composer_timediff & 0x1) {
			dpufb_get_timestamp(&tv1);
			timediff = dpufb_timestamp_diff(&tv0, &tv1);
			if (timediff >= g_debug_ovl_online_composer_time_threshold)
				DPU_FB_ERR("ONLING_IOCTL_TIMEDIFF is %u us!\n", timediff);
		}

		if (ret == 0) {
			if (dpufd->bl_update != NULL)
				dpufd->bl_update(dpufd);
		}
	}
}
#endif

static int dpufb_ov_online_play(struct dpu_fb_data_type *dpufd, void __user *argp)
{
	uint32_t timediff;
	timeval_compatible tv0;
	timeval_compatible tv1;
	int ret = 0;

#if defined(CONFIG_CDC_MULTI_DISPLAY)
	if (dpu_evs_play_bypass_check(dpufd, argp) == 1)
		return ret;
#endif

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
			if (timediff >= g_debug_ovl_online_composer_time_threshold) //lint !e737 !e574
				DPU_FB_ERR("ONLING_IOCTL_TIMEDIFF is %u us!\n", timediff);
		}

		if (ret == 0) {
			if (dpufd->bl_update != NULL)
				dpufd->bl_update(dpufd);
#if defined(CONFIG_DPU_FB_V410) || defined(CONFIG_DPU_FB_V501) || defined(CONFIG_DPU_FB_V320)
			dpufb_display_effect_blc_cabc_update(dpufd);
#endif
		}
	}
	return ret;
}

int dpu_overlay_ioctl_handler(struct dpu_fb_data_type *dpufd,
	uint32_t cmd, void __user *argp)
{
	int ret = 0;
	struct dpu_panel_info *pinfo = NULL;

	if ((dpufd == NULL) || (argp == NULL)) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}
	pinfo = &(dpufd->panel_info);

	DPU_FB_DEBUG("fb%d, ioctl cmd type:%d, online_play_count = %d!\n",
			dpufd->index, cmd, dpufd->online_play_count);

	switch (cmd) {
	case DPUFB_OV_ONLINE_PLAY:
		ret = dpufb_ov_online_play(dpufd, argp);
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
#if defined(CONFIG_ASYNCHRONOUS_PLAY)
	case DPUFB_GET_RELEASE_AND_RETIRE_FENCE:
#if defined(CONFIG_CDC_MULTI_DISPLAY)
		if (dpu_fd_evs_is_enable(dpufd) == 1)
			return ret;
#endif
		ret = dpu_get_release_and_retire_fence(dpufd, argp);
		if (ret != 0)
			DPU_FB_INFO("fb%d get_release_and_retire_fence exit!\n", dpufd->index);
		break;
	case DPUFB_OV_ASYNC_PLAY:
		dpu_set_ov_async_play(dpufd, argp);
		break;
#endif
	case DPUFB_EVS_SWITCH:
		dpu_evs_switch(dpufd, argp);
		break;
	default:
		break;
	}

	return ret;
}
