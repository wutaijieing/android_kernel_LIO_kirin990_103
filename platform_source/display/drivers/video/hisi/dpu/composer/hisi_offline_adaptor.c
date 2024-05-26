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
#include <linux/uaccess.h>

#include "hisi_fb.h"
#include "adaptor/hisi_dss.h"
#include "hisi_disp.h"
#include "hisi_fb_pm.h"
#include "hisi_drv_test.h"
#include "hisi_comp_utils.h"
#include "hisi_disp_rm_debug.h"
#include "hisi_operator_tool.h"
static uint32_t g_wch_idx;
static uint32_t g_csc_mode;
static uint32_t g_wb_layer_caps;
static uint32_t g_src_layer_format;
static uint32_t g_src_transform; /* wblayer transform from src_layer*/
static struct disp_rect g_src_rect;
static uint32_t g_need_clip;
#define OLD_SCL_BIT BIT(4)

static int hisi_offline_adaptor_build_post_offline_pipeline(struct hisi_display_frame *frames)
{
	struct post_offline_pipeline *post_pipeline = NULL;
	struct post_offline_info *offline_info = NULL;

	disp_pr_info("enter++++");
	frames->post_offline_count = 1;
	frames->post_online_count = 0;
	post_pipeline = &frames->post_offline_pipelines[0];

	disp_pr_info(" post_pipeline:0x%p ", post_pipeline);
	post_pipeline->pipeline.pipeline_id = 0;
	post_pipeline->pipeline.operator_count = 1;
	post_pipeline->pipeline.operators[0].info.type = COMP_OPS_WCH;
	post_pipeline->pipeline.operators[0].info.idx = g_wch_idx;
	disp_pr_info("exit---- wch seq %d", post_pipeline->pipeline.operators[0].info.idx);
	return 0;
}

static int hisi_offline_adaptor_alloc_block_infos(struct ov_req_infos *ov_req)
{
	uint32_t ov_block_size;
	struct disp_overlay_block *pov_h_block_infos = NULL;

	disp_pr_info("enter+++");
	ov_block_size = ov_req->ov_block_nums * sizeof(struct disp_overlay_block);
	pov_h_block_infos = (struct disp_overlay_block *)kmalloc(ov_block_size, GFP_ATOMIC);
	if (!pov_h_block_infos) {
			disp_pr_err("pov_h_block_infos alloc failed!");
			ov_req->ov_block_infos_ptr = 0;
			return -EINVAL;
	}
	memset(pov_h_block_infos, 0, ov_block_size);
	ov_req->ov_block_infos_ptr = (uint64_t)(uintptr_t)pov_h_block_infos;

	disp_pr_info("pov_h_block_infos addr =0x%p", pov_h_block_infos);

	disp_pr_info("enter---");
	return 0;
}

static int hisi_offline_adaptor_ovidx_to_dmaid (int32_t ov_idx)
{
	int32_t dma_id;

	switch(ov_idx) {
	case DSS_OVL0:
		dma_id = DPU_SDMA0;
		break;
	case DSS_OVL1:
		dma_id = DPU_SDMA1;
		break;
	case DSS_OVL2:
		dma_id = DPU_SDMA2;
		break;
	case DSS_OVL3:
		dma_id = DPU_SDMA3;
		break;
	default:
		disp_pr_err("ov[%d] is not surport", ov_idx);
		break;
	}
	disp_pr_err("dma_id = %d", dma_id);
	return dma_id;
}

static void hisi_offline_adaptor_cal_clip_value(struct pipeline_src_layer *src_layer)
{
	g_debug_wch_clip_left = src_layer->src_rect.x - src_layer->dst_rect.x;
	int32_t clip_hrz = src_layer->img.width- src_layer->src_rect.w;
	g_debug_wch_clip_right = clip_hrz - g_debug_wch_clip_left;
	g_debug_wch_clip_top = src_layer->src_rect.y - src_layer->dst_rect.y;
	int32_t clip_vrz = src_layer->img.height- src_layer->src_rect.h;
	g_debug_wch_clip_bot = clip_vrz - g_debug_wch_clip_top;

	if (g_debug_wch_clip_left || g_debug_wch_clip_right || g_debug_wch_clip_top || g_debug_wch_clip_bot) {
		src_layer->src_rect.w = src_layer->img.width;
		src_layer->src_rect.h = src_layer->img.height;
		src_layer->src_rect.x = src_layer->dst_rect.x;
		src_layer->src_rect.y = src_layer->dst_rect.y;
		g_need_clip = 1;
	}

	disp_pr_info("[DATA] clip l = %d, r = %d, t = %d, b = %d", g_debug_wch_clip_left, g_debug_wch_clip_right,
		g_debug_wch_clip_top, g_debug_wch_clip_bot);
}

static int hisi_offline_adaptor_build_block_infos(struct ov_req_infos *ov_req, struct dss_overlay *overlay_frame, struct hisi_display_frame *frames)
{
	int i;
	int j;
	int ret;
	struct disp_overlay_block *dst_block = NULL;
	struct disp_overlay_block *dst_block_infos = NULL;
	struct dss_overlay_block *src_block = NULL;
	struct dss_overlay_block *src_block_infos = NULL;
	struct pipeline_src_layer *dst_layer = NULL;
	struct dss_layer *src_layer = NULL;

	/* build disp_overlay_block layer_info */
	ov_req->frame_no = overlay_frame->frame_no;
	ov_req->ovl_idx = overlay_frame->ovl_idx;
	ov_req->ov_block_nums = overlay_frame->ov_block_nums;

	ret = hisi_offline_adaptor_alloc_block_infos(ov_req);
	if (ret)
		return ret;

	dst_block_infos = (struct disp_overlay_block *)(uintptr_t)ov_req->ov_block_infos_ptr;
	src_block_infos = (struct dss_overlay_block *)(uintptr_t)overlay_frame->ov_block_infos_ptr;

	disp_pr_info("ov_block_nums = %d", ov_req->ov_block_nums);
	for (i = 0; i < ov_req->ov_block_nums; i++) {
		src_block = &(src_block_infos[i]);
		dst_block = &(dst_block_infos[i]);
		disp_pr_info("block addr = 0x%p", dst_block);
		disp_pr_info("layer_number = %d", src_block->layer_nums);
		dst_block->layer_nums = src_block->layer_nums;

		for (j = 0; j < src_block->layer_nums; j++) {
			dst_layer = &dst_block->layer_infos[j];
			src_layer = &src_block->layer_infos[j];

			dst_layer->acquire_fence = src_layer->acquire_fence;
			dst_layer->blending_mode = src_layer->blending;

			memcpy(&dst_layer->dst_rect, &src_layer->dst_rect, sizeof(src_layer->dst_rect));
			dst_layer->glb_alpha = src_layer->glb_alpha;
			dst_layer->glb_color = src_layer->color;
			dst_layer->layer_id = src_layer->layer_idx;
			dst_layer->need_caps = src_layer->need_cap;
			dst_layer->slice_num = WLT_SLICE_MAX_COUNT;

			dst_layer->src_type = get_dss_layer_src_type(src_layer->layer_idx);
			disp_pr_info("src_type %d", get_dss_layer_src_type(src_layer->layer_idx));

			dst_layer->dsd_index = get_dss_layer_dsd_index(src_layer->layer_idx);
			disp_pr_info("dsd_index %d\n", get_dss_layer_dsd_index(src_layer->layer_idx));

			memcpy(&dst_layer->src_rect_mask, &src_layer->src_rect_mask, sizeof(src_layer->src_rect_mask));
			memcpy(&dst_layer->src_rect, &src_layer->src_rect, sizeof(src_layer->src_rect));

			dst_layer->transform = src_layer->transform;

			g_src_transform = dst_layer->transform;
			/* build img */
			dst_layer->img.bpp = src_layer->img.bpp;
			dst_layer->img.buff_size = src_layer->img.buf_size;
			dst_layer->img.compress_type = (dst_layer->need_caps & CAP_HEBCE) ? COMPRESS_TYPE_HEBC : COMPRESS_TYPE_NONE;
			dst_layer->img.csc_mode = src_layer->img.csc_mode;
			g_csc_mode = dst_layer->img.csc_mode; /* use for wb_layer */
			disp_pr_info("src_layer format %d", src_layer->img.format);
			dst_layer->img.format = src_layer->img.format;
			g_src_layer_format = dst_layer->img.format;
			dst_layer->img.hebcd_block_type = src_layer->img.hebcd_block_type;

			dst_layer->img.hebc_planes[HEBC_PANEL_Y].header_addr = src_layer->img.hebc_header_addr0;
			dst_layer->img.hebc_planes[HEBC_PANEL_Y].header_offset = src_layer->img.hebc_header_offset0;
			dst_layer->img.hebc_planes[HEBC_PANEL_Y].header_stride = src_layer->img.hebc_header_stride0;
			dst_layer->img.hebc_planes[HEBC_PANEL_Y].payload_addr = src_layer->img.hebc_payload_addr0;
			dst_layer->img.hebc_planes[HEBC_PANEL_Y].payload_offset = src_layer->img.hebc_payload_offset0;
			dst_layer->img.hebc_planes[HEBC_PANEL_Y].payload_stride = src_layer->img.hebc_payload_stride0;
			dst_layer->img.hebc_planes[HEBC_PANEL_UV].header_addr = src_layer->img.hebc_header_addr1;
			dst_layer->img.hebc_planes[HEBC_PANEL_UV].header_offset = src_layer->img.hebc_header_offset1;
			dst_layer->img.hebc_planes[HEBC_PANEL_UV].header_stride = src_layer->img.hebc_header_stride1;
			dst_layer->img.hebc_planes[HEBC_PANEL_UV].payload_addr = src_layer->img.hebc_payload_addr1;
			dst_layer->img.hebc_planes[HEBC_PANEL_UV].payload_offset = src_layer->img.hebc_payload_offset1;
			dst_layer->img.hebc_planes[HEBC_PANEL_UV].payload_stride = src_layer->img.hebc_payload_stride1;

			dst_layer->img.hebc_scramble_mode = src_layer->img.hebc_scramble_mode;
			dst_layer->img.height = src_layer->img.height;
			dst_layer->img.mmu_enable = src_layer->img.mmu_enable;
			dst_layer->img.phy_addr = src_layer->img.phy_addr;
			dst_layer->img.secure_mode = src_layer->img.secure_mode;
			dst_layer->img.shared_fd = src_layer->img.shared_fd;
			dst_layer->img.stride = src_layer->img.stride;
			dst_layer->img.stride_plane1 = src_layer->img.stride_plane1;
			dst_layer->img.stride_plane2 = src_layer->img.stride_plane2;
			dst_layer->img.offset_plane1 = src_layer->img.offset_plane1;
			dst_layer->img.offset_plane2 = src_layer->img.offset_plane2;
			dst_layer->img.vir_addr = src_layer->img.vir_addr;
			dst_layer->img.width = src_layer->img.width;

			hisi_offline_adaptor_cal_clip_value(dst_layer);
			memcpy(&g_src_rect, &dst_layer->src_rect, sizeof(src_layer->src_rect));

			disp_pr_debug("dst_layer->img.mmu_enable:%d", dst_layer->img.mmu_enable);
			disp_pr_info("dst_layer->img.shared_fd:%d", dst_layer->img.shared_fd);
			disp_pr_info("src_layer vir_addr phy_addr 0x%p", src_layer->img.vir_addr, src_layer->img.phy_addr);
			disp_pr_info("dst_layer vir_addr phy_addr 0x%p", dst_layer->img.vir_addr, dst_layer->img.phy_addr);
		}
	}
	return 0;
}

static void hisi_offline_adaptor_adjust_wchnidx(struct dss_wb_layer *wb_layer)
{
	if (wb_layer->chn_idx == DPU_WCHN_W0)
		wb_layer->chn_idx = DPU_WCHN_W2; /* wch0 use for online */
}

static uint32_t hisi_offline_adaptor_adjust_caps(uint32_t old_caps)
{
	if (old_caps & OLD_SCL_BIT)
		return CAP_SCL;

	return old_caps;
}

static void hisi_offline_adaptor_yuv_in(struct disp_wb_layer *dst_layer)
{
	if (is_yuv_format(g_src_layer_format) && g_src_transform == 0 && dst_layer->need_caps == 0)
		g_debug_wch_yuv_in = 1;

	if(g_debug_wch_yuv_in && !is_yuv_format(dst_layer->dst.format)) {
		dst_layer->wchn_idx = DPU_WCHN_W1; /* only wch1 surport pcsc */
		disp_pr_info("[DATA] enter pcsc cases");
	}

	disp_pr_info("[GLOBAL] g_debug_wch_yuv_in = %d, format = %d", g_debug_wch_yuv_in, dst_layer->dst.format);
}

static void hisi_offline_adaptor_adjust_wb_layer(struct disp_wb_layer *wb_layer)
{
	if (g_need_clip) {
		memcpy(&wb_layer->src_rect, &g_src_rect, sizeof(wb_layer->src_rect));
		memcpy(&wb_layer->dst_rect, &g_src_rect, sizeof(wb_layer->dst_rect));
		disp_pr_info("[DATA] clip wb_layer->src_rect.w = %d, h = %d, x = %d, y = %d", wb_layer->src_rect.w, wb_layer->src_rect.h,
		wb_layer->src_rect.x, wb_layer->src_rect.y);
		wb_layer->wchn_idx = DPU_WCHN_W1; /* rdfc cases default use wch0 */
	}
}

static int hisi_offline_adaptor_build_wb_infos(struct ov_req_infos *ov_req, struct dss_overlay *overlay_frame)
{
	int i;
	struct disp_wb_layer *dst_layer = NULL;
	struct dss_wb_layer *src_layer = NULL;
	disp_pr_info("enter++++");

	if (!ov_req) {
		disp_pr_err("ov_req is null ptr");
		return -1;
	}

	if (!overlay_frame) {
		disp_pr_err("overlay_frame is null ptr");
		return -1;
	}

	/* build wb info */
	ov_req->wb_type = overlay_frame->wb_compose_type;
	ov_req->wb_layer_nums = overlay_frame->wb_layer_nums;
	ov_req->wb_enable = overlay_frame->wb_enable;
	memcpy(&ov_req->wb_ov_rect, &overlay_frame->wb_ov_rect, sizeof(overlay_frame->wb_ov_rect));

	disp_pr_info("wb_layer_num = %d", ov_req->wb_layer_nums);
	for (i = 0; i < ov_req->wb_layer_nums; i++) {
			dst_layer = &ov_req->wb_layers[i];
			src_layer = &overlay_frame->wb_layer_infos[i];

			dst_layer->layer_id = i;
			dst_layer->acquire_fence = src_layer->acquire_fence;
			hisi_offline_adaptor_adjust_wchnidx(src_layer);
			g_wch_idx = src_layer->chn_idx;
			dst_layer->wchn_idx = src_layer->chn_idx;
			dst_layer->transform = src_layer->transform;
			dst_layer->release_fence = src_layer->release_fence;
			dst_layer->retire_fence = src_layer->retire_fence;
			dst_layer->need_caps = hisi_offline_adaptor_adjust_caps(src_layer->need_cap);
			dst_layer->slice_num = WLT_SLICE_MAX_COUNT;
			g_wb_layer_caps = dst_layer->need_caps;
			memcpy(&dst_layer->dst_rect, &src_layer->dst_rect, sizeof(src_layer->dst_rect));
			memcpy(&dst_layer->src_rect, &src_layer->src_rect, sizeof(src_layer->src_rect));
			memcpy(&dst_layer->wb_block, &src_layer->wb_block_info, sizeof(src_layer->wb_block_info));

			/* build img */
			dst_layer->dst.bpp = src_layer->dst.bpp;
			dst_layer->dst.buff_size = src_layer->dst.buf_size;
			dst_layer->dst.compress_type = (src_layer->need_cap & CAP_HEBCE) ? COMPRESS_TYPE_HEBC : COMPRESS_TYPE_NONE;

			dst_layer->dst.csc_mode = g_csc_mode;
			dst_layer->dst.format = src_layer->dst.format;

			hisi_offline_adaptor_yuv_in(dst_layer);
			hisi_offline_adaptor_adjust_wb_layer(dst_layer);

			dst_layer->dst.hebcd_block_type = src_layer->dst.hebcd_block_type;
			dst_layer->dst.hebc_planes[HEBC_PANEL_Y].header_addr = src_layer->dst.hebc_header_addr0;
			dst_layer->dst.hebc_planes[HEBC_PANEL_Y].header_offset = src_layer->dst.hebc_header_offset0;
			dst_layer->dst.hebc_planes[HEBC_PANEL_Y].header_stride = src_layer->dst.hebc_header_stride0;
			dst_layer->dst.hebc_planes[HEBC_PANEL_Y].payload_addr = src_layer->dst.hebc_payload_addr0;
			dst_layer->dst.hebc_planes[HEBC_PANEL_Y].payload_offset = src_layer->dst.hebc_payload_offset0;
			dst_layer->dst.hebc_planes[HEBC_PANEL_Y].payload_stride = src_layer->dst.hebc_payload_stride0;
			dst_layer->dst.hebc_planes[HEBC_PANEL_UV].header_addr = src_layer->dst.hebc_header_addr1;
			dst_layer->dst.hebc_planes[HEBC_PANEL_UV].header_offset = src_layer->dst.hebc_header_offset1;
			dst_layer->dst.hebc_planes[HEBC_PANEL_UV].header_stride = src_layer->dst.hebc_header_stride1;
			dst_layer->dst.hebc_planes[HEBC_PANEL_UV].payload_addr = src_layer->dst.hebc_payload_addr1;
			dst_layer->dst.hebc_planes[HEBC_PANEL_UV].payload_offset = src_layer->dst.hebc_payload_offset1;
			dst_layer->dst.hebc_planes[HEBC_PANEL_UV].payload_stride = src_layer->dst.hebc_payload_stride1;
			dst_layer->dst.hebc_scramble_mode = src_layer->dst.hebc_scramble_mode;
			dst_layer->dst.height = src_layer->dst.height;
			dst_layer->dst.mmu_enable = src_layer->dst.mmu_enable;
			dst_layer->dst.phy_addr = src_layer->dst.phy_addr;
			dst_layer->dst.secure_mode = src_layer->dst.secure_mode;
			dst_layer->dst.shared_fd = src_layer->dst.shared_fd;
			dst_layer->dst.stride = src_layer->dst.stride;
			dst_layer->dst.vir_addr = src_layer->dst.vir_addr;
			dst_layer->dst.width = src_layer->dst.width;
			dst_layer->dst.stride_plane1 = src_layer->dst.stride_plane1;
			dst_layer->dst.stride_plane2 = src_layer->dst.stride_plane2;
			dst_layer->dst.offset_plane1 = src_layer->dst.offset_plane1;
			dst_layer->dst.offset_plane2 = src_layer->dst.offset_plane2;
			disp_pr_info("dst_layer->dst.shared_fd:%d", dst_layer->dst.shared_fd);
			disp_pr_info("dst_layer caps = %d csc_mode = %d", dst_layer->need_caps, dst_layer->dst.csc_mode);
			disp_pr_info("src_layer vir_addr phy_addr 0x%p", src_layer->dst.vir_addr, src_layer->dst.phy_addr);
			disp_pr_info("dst_layer stride_plane1 = %d stride_plane2 = %d offset_plane1  = %d offset_plane2 = %d",
                dst_layer->dst.stride_plane1, dst_layer->dst.stride_plane2, dst_layer->dst.offset_plane1 , dst_layer->dst.offset_plane2);
	}

	disp_pr_info("enter---");
	return 0;
}

static void hisi_offline_adaptor_scf_rot()
{
	if (g_src_transform == HISI_FB_TRANSFORM_ROT_90 && g_wb_layer_caps == CAP_SCL) {
		g_debug_wch_scf_rot = 1;
		g_debug_wch_scf = 1;
	} else if (g_wb_layer_caps == CAP_SCL)
		g_debug_wch_scf = 1;

	disp_pr_info("[GLOBAL] g_debug_wch_scf_rot = %d scf = %d", g_debug_wch_scf_rot, g_debug_wch_scf);
}

static void hisi_offline_adaptor_memset_global_value()
{
	g_debug_wch_scf_rot = 0;
	g_debug_wch_scf = 0;
	g_debug_wch_yuv_in = 0;
	g_debug_wch_clip_left = 0;
	g_debug_wch_clip_right = 0;
	g_debug_wch_clip_top = 0;
	g_debug_wch_clip_bot = 0;
	g_need_clip = 0;
	g_debug_block_scl_rot = 0;
	memset(&g_src_rect, 0, sizeof(g_src_rect));
}

static int hisi_offline_adaptor_build_frame(struct hisi_display_frame *frames, struct dss_overlay *overlay_frame)
{
	struct ov_req_infos *ov_req = NULL;

	int ret;
	disp_pr_info("enter++++");
	if (!frames) {
		disp_pr_err("frames is null ptr");
		return -1;
	}

	if (!overlay_frame) {
		disp_pr_err("overlay_frame is null ptr");
		return -1;
	}

	if (overlay_frame->ov_block_nums != 1) {
		disp_pr_err("ov_block_nums is %d", overlay_frame->ov_block_nums);
		return -1;
	}

	hisi_offline_adaptor_memset_global_value();

	ov_req = &frames->ov_req;

	ret = hisi_offline_adaptor_build_block_infos(ov_req, overlay_frame, frames);
	if (ret)
		return ret;

	ret = hisi_offline_adaptor_build_wb_infos(ov_req, overlay_frame);
	if (ret)
		return ret;

	hisi_offline_adaptor_scf_rot();
	disp_pr_info("enter----");
	return 0;
free_block_ov_info:
	// kfree(ov_h_block_infos);
	return ret;
}

static void hisi_offline_adaptor_dss_overlay_info_init(struct dss_overlay *ov_req)
{
	if (ov_req == NULL)
		return;

	memset(ov_req, 0, sizeof(struct dss_overlay));
	ov_req->release_fence = -1;
	ov_req->retire_fence = -1;
}

static struct dss_overlay *hisi_offline_adaptor_alloc_data(void)
{
	struct dss_overlay *pov_req = NULL;

	pov_req = (struct dss_overlay *)kmalloc(sizeof(struct dss_overlay), GFP_ATOMIC);
	if (pov_req)
		hisi_offline_adaptor_dss_overlay_info_init(pov_req);

	return pov_req;
}

static int hisi_offline_adaptor_get_ov_data_from_user(struct dss_overlay *pov_req, const void __user *argp)
{
	int ret;
	struct dss_overlay_block *pov_h_block_infos = NULL;
	uint32_t ov_block_size;

	ret = copy_from_user(pov_req, argp, sizeof(struct dss_overlay));
	if (ret) {
		disp_pr_err("copy_from_user failed");
		return -EINVAL;
	}

	pov_req->release_fence = -1;
	if ((pov_req->ov_block_nums <= 0) || (pov_req->ov_block_nums > HISI_DSS_OV_BLOCK_NUMS)) {
		disp_pr_err("ov_block_nums is out of range", pov_req->ov_block_nums);
		return -EINVAL;
	}

	ov_block_size = pov_req->ov_block_nums * sizeof(struct dss_overlay_block);
	pov_h_block_infos = (struct dss_overlay_block *)kmalloc(ov_block_size, GFP_ATOMIC);
	if (!pov_h_block_infos) {
		disp_pr_err("offline, pov_h_block_infos alloc failed!\n");
		pov_req->ov_block_infos_ptr = 0;
		return -EINVAL;
	}
	memset(pov_h_block_infos, 0, ov_block_size);

	ret = copy_from_user(pov_h_block_infos, (struct dss_overlay_block *)(uintptr_t)pov_req->ov_block_infos_ptr,
		ov_block_size);
	if (ret) {
		disp_pr_err("offline, dss_overlay_block_t copy_from_user failed!\n");
		kfree(pov_h_block_infos);
		pov_h_block_infos = NULL;
		pov_req->ov_block_infos_ptr = 0;
		return -EINVAL;
	}

	pov_req->ov_block_infos_ptr = (uint64_t)(uintptr_t)pov_h_block_infos;

	return ret;
}

static void hisi_offline_adaptor_free_data(struct dss_overlay *pov_req)
{
	struct dss_overlay_block *pov_h_block_infos = NULL;

	if (pov_req) {
		pov_h_block_infos = (struct dss_overlay_block *)(uintptr_t)(pov_req->ov_block_infos_ptr);
		if (pov_h_block_infos) {
			kfree(pov_h_block_infos);
			pov_h_block_infos = NULL;
		}
		kfree(pov_req);
	}
	pov_req = NULL;
}

static int hisi_offline_adaptor_data_prepare(const void __user *argp, struct dss_overlay **pov_req_out)
{
	int ret;
	struct dss_overlay *pov_req = NULL;

	pov_req = hisi_offline_adaptor_alloc_data();
	if (!pov_req) {
		disp_pr_err("dss_overlay alloc failed");
		return -1;
	}

	ret = hisi_offline_adaptor_get_ov_data_from_user(pov_req, argp);
	if (ret != 0) {
		disp_pr_err("get_ov_data_from_user failed",ret);
		hisi_offline_adaptor_free_data(pov_req);
		return -1;
	}

	// TODO: check_panel_status_and_composer_type

	*pov_req_out = pov_req;

	return ret;
}

static int hisi_offline_adaptor_device_prepare(struct dss_overlay *pov_req)
{
	/* TODO: offline_handle_device_power */
	/* TODO: offline_layerbuf_lock */
	/* TODO: layerbuf_flush */

	return 0;
}

static int hisi_offline_adaptor_prepare(const void __user *argp, struct dss_overlay **pov_req_out)
{
	int ret;

	disp_pr_info(" enter+++");

	ret = hisi_offline_adaptor_data_prepare(argp, pov_req_out);
	if (ret != 0) {
		disp_pr_err("offline data prepare failed");
		return ret;
	}

	ret = hisi_offline_adaptor_device_prepare(*pov_req_out);
	if (ret != 0) {
		disp_pr_err("offline device prepare failed");
		hisi_offline_adaptor_free_data(*pov_req_out);
		return ret;
	}

	disp_pr_info(" enter---");

	return 0;
}

int hisi_offline_adaptor_play(struct hisi_composer_device *ov_dev, unsigned long arg)
{
	struct hisi_composer_priv_ops *overlay_priv_ops = NULL;
	void __user *argp = (void __user *)(uintptr_t)arg;
	struct hisi_display_frame *using_frame;
	struct dss_overlay *overlay_frame;
	int ret = 0;

	disp_pr_info(" enter ++++");

	using_frame = &ov_dev->frame;

	char __iomem *dpu_base_addr = hisi_config_get_ip_base(DISP_IP_BASE_DPU);
	ov_dev->ov_data.ip_base_addrs[DISP_IP_BASE_DPU] = dpu_base_addr;
	disp_pr_info("dpu_base addr = 0x%p", dpu_base_addr);
	disp_pr_info("using_frame addr = 0x%p", using_frame);
	if (hisi_offline_adaptor_prepare(argp, &overlay_frame) != 0) {
		disp_pr_err("hisi_offline_adaptor_prepare fail");
		return -EINVAL;
	}

	// update scene_id : wch1->scene4 wch2->scene5
	if (overlay_frame->wb_layer_infos[0].chn_idx == WB_TYPE_WCH1) {
		ov_dev->ov_data.scene_id = DPU_SCENE_OFFLINE_0;
	} else if (overlay_frame->wb_layer_infos[0].chn_idx == WB_TYPE_WCH2) {
		ov_dev->ov_data.scene_id = DPU_SCENE_OFFLINE_1;
	}

	disp_pr_info("update scene_id = %d by chn_ids = %d\n", ov_dev->ov_data.scene_id,
		overlay_frame->wb_layer_infos[0].chn_idx);

	if (hisi_offline_adaptor_build_frame(using_frame, overlay_frame) != 0) {
		disp_pr_err("hisi_offline_adaptor_build_frame fail");
		return -EINVAL;
	}

	overlay_priv_ops = &ov_dev->ov_priv_ops;

	/* 1, call overlay core function for different platform */
	if (overlay_priv_ops->overlay_prepare) {
		ret = overlay_priv_ops->overlay_prepare(ov_dev, using_frame);
		if (ret) {
			disp_pr_info("overlay_prepare failed");
			return ret;
		}
	}

	/* 2, call overlay core function for different platform */
	if (overlay_priv_ops->overlay_present) {
		ret = overlay_priv_ops->overlay_present(ov_dev, using_frame);
		if (ret) {
			disp_pr_info("overlay_present failed");
			return ret;
		}
	}

	/* 3, call overlay core function for different platform */
	if (overlay_priv_ops->overlay_post) {
		ret = overlay_priv_ops->overlay_post(ov_dev, using_frame);
		if (ret) {
			disp_pr_info("overlay_post failed");
			return ret;
		}
	}
	disp_pr_info(" exit ----");
	return 0;
}
