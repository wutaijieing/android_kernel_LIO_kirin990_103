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
#include "hisi_dss.h"
#include "hisi_disp.h"
#include "hisi_fb_pm.h"
#include "hisi_drv_test.h"
#include "hisi_comp_utils.h"
#include "scl/dpu_scl.h"
#include "hisi_dssv700_utils.h"
#include "hisi_disp_rm_debug.h"
#include "vsync/hisi_disp_vactive.h"
#include "hisi_disp_config.h"
#include "hisi_disp_iommu.h"
#include "hisi_operator_tool.h"
#include "hdmirx_init.h"
#include "../../../dprx/dprx.h"
static int arsr_post_en = 0;
static inline uint32_t set_operator(union operator_id *operator, uint32_t type, uint32_t id, uint32_t ops_idx)
{
	operator[ops_idx].info.type = type;
	operator[ops_idx].info.idx = id;

	disp_pr_ops(debug, operator[ops_idx]);
	disp_pr_debug("ops_idx = %d", ops_idx);
	return ++ops_idx;
}

static uint32_t hisi_fb_layer_fmt(struct dss_layer *src_layer)
{
	// FIXME :fpga test plan, not last version
	uint32_t src_type;
	src_type = get_dss_layer_src_type(src_layer->layer_idx);
	if (src_type == LAYER_SRC_TYPE_HDMIRX)
		return hdmirx_get_layer_fmt();
	if (src_type == LAYER_SRC_TYPE_DPRX || src_type == LAYER_SRC_TYPE_EDPRX)
		return dprx_get_layer_fmt(src_layer->img.format);
	return src_layer->img.format;
}
static void demura_hebc_lut_addr(struct pipeline_src_layer *demua_lut_layer, int plane_idx,
	struct dss_layer *src_layer)
{
	demua_lut_layer->img.hebc_planes[plane_idx].header_addr = src_layer->img.hebc_header_addr0;
	demua_lut_layer->img.hebc_planes[plane_idx].header_offset = src_layer->img.hebc_header_offset0;
	demua_lut_layer->img.hebc_planes[plane_idx].header_stride = src_layer->img.hebc_header_stride0;
	demua_lut_layer->img.hebc_planes[plane_idx].payload_addr = src_layer->img.hebc_payload_addr0;
	demua_lut_layer->img.hebc_planes[plane_idx].payload_offset = src_layer->img.hebc_payload_offset0;
	demua_lut_layer->img.hebc_planes[plane_idx].payload_stride = src_layer->img.hebc_payload_stride0;
}

static void set_4_plane_demura_lut_addr(uint32_t layer_idx, struct pipeline_src_layer *demua_lut_layer0,
	struct dss_layer *src_layer){
	int plane_idx = 0;
	if (demua_lut_layer0->need_caps & CAP_HEBCD) { // R normal G B HEBC D3_128
		if (layer_idx == 0)
			plane_idx = 0;
		else if (layer_idx == 2 || layer_idx == 3)
			plane_idx = layer_idx - 1;
		else
			return;
		demura_hebc_lut_addr(demua_lut_layer0, plane_idx, src_layer);
	} else { // R normal G B linear D3_128
		if (layer_idx == 2)
			demua_lut_layer0->img.offset_plane1 = src_layer->img.phy_addr;
		else if (layer_idx == 3)
			demua_lut_layer0->img.offset_plane2 = src_layer->img.phy_addr;
	}
}

static void set_8_plane_demura_lut_addr(uint32_t layer_idx, struct hisi_display_frame *frames,
	struct dss_layer *src_layer)
{
	struct pipeline_src_layer *demua_lut_layer = NULL;
	int lut_idx = 0;
	int plane_idx = 0;
	if (layer_idx < 2) { // R1 R2 normal G1 G2 B1 B2
		lut_idx = layer_idx % 2;
		plane_idx = 0;
	} else if (layer_idx > 2){
		lut_idx = (layer_idx - 1) % 2;
		plane_idx = (layer_idx - 1) / 2;
	} else {
		return;
	}
	demua_lut_layer = &(frames->pre_pipelines[lut_idx].src_layer);
	demura_hebc_lut_addr(demua_lut_layer, plane_idx, src_layer);
}

static void build_pre_pipeline_operator(struct hisi_display_frame *frames, struct dss_overlay_block *ov_h_block_infos,
	uint32_t index, uint32_t *ops_count, uint32_t *cld_layer_count)
{
	struct dss_layer *src_layer = NULL;
	struct pipeline *p_pipeline = NULL;
	struct pipeline_src_layer *demua_lut_layer0 = NULL;
	struct pipeline_src_layer *demua_lut_layer1 = NULL;
	uint32_t ops_cnt = *ops_count;
	uint32_t cld_layer_cnt = *cld_layer_count;

	src_layer = &ov_h_block_infos[0].layer_infos[index];
	p_pipeline = &(frames->pre_pipelines[index].pipeline);
	p_pipeline->pipeline_id = index;

	if (get_dss_layer_src_type(src_layer->layer_idx) == LAYER_SRC_TYPE_LOCAL) {
		if (g_debug_en_sdma1 == 0)
			ops_cnt = set_operator(p_pipeline->operators, COMP_OPS_SDMA, 0, ops_cnt);
		else
			ops_cnt = set_operator(p_pipeline->operators, COMP_OPS_SDMA, 1, ops_cnt);
	} else {
		disp_pr_info("src_layer->dsd_index=%d\n", get_dss_layer_dsd_index(src_layer->layer_idx));
		ops_cnt = set_operator(p_pipeline->operators, COMP_OPS_DSD,
			get_dss_layer_dsd_index(src_layer->layer_idx), ops_cnt);
	}

	disp_pr_debug("src_layer cap = %d\n", src_layer->need_cap);
	disp_pr_debug("src_layer->is_cld_layer=%u ", src_layer->is_cld_layer);

	if (src_layer->is_cld_layer) {
		ops_cnt = set_operator(p_pipeline->operators, COMP_OPS_CLD, (src_layer->is_cld_layer >> 1), ops_cnt);
		cld_layer_cnt++;
	}

	if (src_layer->need_cap & CAP_UVUP)
		ops_cnt = set_operator(p_pipeline->operators, COMP_OPS_UVUP, 0, ops_cnt);

	if (src_layer->need_cap & CAP_HIGH_SCL)
		ops_cnt = set_operator(p_pipeline->operators, COMP_OPS_ARSR, 0, ops_cnt);

	if (src_layer->need_cap & CAP_ARSR1_PRE) {
		ops_cnt = set_operator(p_pipeline->operators, COMP_OPS_ARSR, 1, ops_cnt);
		dpu_arsr_cnt();
	}

	if (src_layer->need_cap & CAP_HDR)
		ops_cnt = set_operator(p_pipeline->operators, COMP_OPS_HDR, 0, ops_cnt);

	if (src_layer->need_cap & CAP_ARSR0_POST) {
		dpu_arsr_cnt();
		arsr_post_en = 1;
		disp_pr_debug(" arsr_post_en = %d\n", arsr_post_en);
	}

	if (is_d3_128(src_layer->img.format) || (src_layer->need_cap & CAP_DEMA)) {
		ops_cnt = set_operator(p_pipeline->operators, COMP_OPS_DEMURA, 0, ops_cnt);
		g_debug_en_demura = 1;
		g_debug_en_ddic = 1;
		demua_lut_layer0 = &(frames->pre_pipelines[0].src_layer);
		if (ov_h_block_infos->layer_nums == 4 || ov_h_block_infos->layer_nums == 2) { // 4 plane
			g_demura_plane_num = 4;
			set_4_plane_demura_lut_addr(index, demua_lut_layer0, src_layer);
			frames->pre_pipeline_count = 2; // 1lut & 1layer
		} else if (ov_h_block_infos->layer_nums == 7 || ov_h_block_infos->layer_nums == 3){ // 8 plane
			g_demura_plane_num = 8;
			set_8_plane_demura_lut_addr(index, frames, src_layer);
			frames->pre_pipeline_count = 3; // 2luts & 1layer
		}
		disp_pr_debug(" enter demura en %d \n", g_debug_en_demura);
		disp_pr_info(" i =%d,  demua_lut_layer0 = %p", index, demua_lut_layer0);
	} else {
		ops_cnt = set_operator(p_pipeline->operators, COMP_OPS_OVERLAY, 0, ops_cnt);
	}

	*ops_count = ops_cnt;
	*cld_layer_count = cld_layer_cnt;
	disp_pr_debug("ops_count:%d\n", ops_cnt);
}

static void build_dest_layer_img(struct dss_layer *src_layer, struct pipeline_src_layer *dest_layer)
{
	dest_layer->img.bpp = src_layer->img.bpp;
	dest_layer->img.buff_size = src_layer->img.buf_size;
	dest_layer->img.compress_type = COMPRESS_TYPE_NONE;
	dest_layer->img.csc_mode = src_layer->img.csc_mode;
	dest_layer->img.format = hisi_fb_layer_fmt(src_layer);
	dest_layer->img.hebcd_block_type = src_layer->img.hebcd_block_type;
	if (dest_layer->need_caps & CAP_HEBCD) {
		dest_layer->img.compress_type = COMPRESS_TYPE_HEBC;
		dest_layer->img.hebc_planes[HEBC_PANEL_Y].header_addr = src_layer->img.hebc_header_addr0;
		dest_layer->img.hebc_planes[HEBC_PANEL_Y].header_offset = src_layer->img.hebc_header_offset0;
		dest_layer->img.hebc_planes[HEBC_PANEL_Y].header_stride = src_layer->img.hebc_header_stride0;
		dest_layer->img.hebc_planes[HEBC_PANEL_Y].payload_addr = src_layer->img.hebc_payload_addr0;
		dest_layer->img.hebc_planes[HEBC_PANEL_Y].payload_offset = src_layer->img.hebc_payload_offset0;
		dest_layer->img.hebc_planes[HEBC_PANEL_Y].payload_stride = src_layer->img.hebc_payload_stride0;
		dest_layer->img.hebc_planes[HEBC_PANEL_UV].header_addr = src_layer->img.hebc_header_addr1;
		dest_layer->img.hebc_planes[HEBC_PANEL_UV].header_offset = src_layer->img.hebc_header_offset1;
		dest_layer->img.hebc_planes[HEBC_PANEL_UV].header_stride = src_layer->img.hebc_header_stride1;
		dest_layer->img.hebc_planes[HEBC_PANEL_UV].payload_addr = src_layer->img.hebc_payload_addr1;
		dest_layer->img.hebc_planes[HEBC_PANEL_UV].payload_offset = src_layer->img.hebc_payload_offset1;
		dest_layer->img.hebc_planes[HEBC_PANEL_UV].payload_stride = src_layer->img.hebc_payload_stride1;

		disp_pr_debug(" hebc_header_addr0:0x%x %d %d hebc_payload_addr0:0x%x %d %d ",
					src_layer->img.hebc_header_addr0, src_layer->img.hebc_header_offset0, src_layer->img.hebc_header_stride0,
					src_layer->img.hebc_payload_addr0, src_layer->img.hebc_payload_offset0,src_layer->img.hebc_payload_stride0);
		disp_pr_debug(" hebc_header_addr1:0x%x %d %d hebc_payload_addr1:0x%x %d %d ",
					src_layer->img.hebc_header_addr1, src_layer->img.hebc_header_offset1, src_layer->img.hebc_header_stride1,
					src_layer->img.hebc_payload_addr1, src_layer->img.hebc_payload_offset1, src_layer->img.hebc_payload_stride1);
	} else if (dest_layer->need_caps & CAP_AFBCD) {
		dest_layer->img.compress_type = COMPRESS_TYPE_HEBC; // AFBCD reuse HEBCD code process
		dest_layer->img.hebc_planes[HEBC_PANEL_Y].header_addr = src_layer->img.afbc_header_addr;
		dest_layer->img.hebc_planes[HEBC_PANEL_Y].header_offset = src_layer->img.afbc_header_offset;
		dest_layer->img.hebc_planes[HEBC_PANEL_Y].header_stride = src_layer->img.afbc_header_stride;
		dest_layer->img.hebc_planes[HEBC_PANEL_Y].payload_addr = src_layer->img.afbc_payload_addr;
		dest_layer->img.hebc_planes[HEBC_PANEL_Y].payload_offset = src_layer->img.afbc_payload_offset;
		dest_layer->img.hebc_planes[HEBC_PANEL_Y].payload_stride = src_layer->img.afbc_payload_stride;
		dest_layer->img.hebc_planes[HEBC_PANEL_UV].header_addr = src_layer->img.afbc_header_addr;
		dest_layer->img.hebc_planes[HEBC_PANEL_UV].header_offset = src_layer->img.afbc_header_offset;
		dest_layer->img.hebc_planes[HEBC_PANEL_UV].header_stride = src_layer->img.afbc_header_stride;
		dest_layer->img.hebc_planes[HEBC_PANEL_UV].payload_addr = src_layer->img.afbc_payload_addr;
		dest_layer->img.hebc_planes[HEBC_PANEL_UV].payload_offset = src_layer->img.afbc_payload_offset;
		dest_layer->img.hebc_planes[HEBC_PANEL_UV].payload_stride = src_layer->img.afbc_payload_stride;
		disp_pr_debug(" afbc_header_addr0:0x%x %d %d afbc_payload_addr0:0x%x %d %d ",
					src_layer->img.afbc_header_addr, src_layer->img.afbc_header_offset, src_layer->img.afbc_header_stride,
					src_layer->img.afbc_payload_addr, src_layer->img.afbc_payload_offset,src_layer->img.afbc_payload_stride);
		disp_pr_debug(" afbc_header_addr1:0x%x %d %d afbc_payload_addr1:0x%x %d %d ",
					src_layer->img.afbc_header_addr, src_layer->img.afbc_header_offset, src_layer->img.afbc_header_stride,
					src_layer->img.afbc_payload_addr, src_layer->img.afbc_payload_offset, src_layer->img.afbc_payload_stride);
	}

	disp_pr_info(" compress_type(%d) format(%d) block_type(%d) scramble(%d) width(%d) height(%d) bpp(%d) transform(%d) stride(%d) buf_size(%d)",
				dest_layer->img.compress_type, src_layer->img.format, src_layer->img.hebcd_block_type,
				src_layer->img.hebc_scramble_mode, src_layer->img.width, src_layer->img.height,
				src_layer->img.bpp, src_layer->transform, src_layer->img.stride, src_layer->img.buf_size);
	disp_pr_debug(" DST: dst_rect:%d %d %d %d src_rect:%d %d %d %d src_rect_mask:%d %d %d %d ",
				dest_layer->dst_rect.x, dest_layer->dst_rect.y, dest_layer->dst_rect.w, dest_layer->dst_rect.h,
				dest_layer->src_rect.x, dest_layer->src_rect.y, dest_layer->src_rect.w, dest_layer->src_rect.h,
				dest_layer->src_rect_mask.x, dest_layer->src_rect_mask.y, dest_layer->src_rect_mask.w, dest_layer->src_rect_mask.h);
	disp_pr_debug(" SRC: dst_rect:%d %d %d %d src_rect:%d %d %d %d src_rect_mask:%d %d %d %d ",
				src_layer->dst_rect.x, src_layer->dst_rect.y, src_layer->dst_rect.w, src_layer->dst_rect.h,
				src_layer->src_rect.x, src_layer->src_rect.y, src_layer->src_rect.w, src_layer->src_rect.h,
				src_layer->src_rect_mask.x, src_layer->src_rect_mask.y, src_layer->src_rect_mask.w, src_layer->src_rect_mask.h);
	dest_layer->img.hebc_scramble_mode = src_layer->img.hebc_scramble_mode;
	dest_layer->img.height = src_layer->img.height;
	dest_layer->img.mmu_enable = src_layer->img.mmu_enable;
	dest_layer->img.phy_addr = src_layer->img.phy_addr;
	dest_layer->img.secure_mode = src_layer->img.secure_mode;

	disp_pr_debug("src_layer share_fd = %d\n", src_layer->img.shared_fd);
	dest_layer->img.shared_fd = src_layer->img.shared_fd;
	dest_layer->img.stride = src_layer->img.stride;
	dest_layer->img.stride_plane1 = src_layer->img.stride_plane1;
	dest_layer->img.stride_plane2 = src_layer->img.stride_plane2;
	dest_layer->img.offset_plane1 = src_layer->img.offset_plane1;
	dest_layer->img.offset_plane2 = src_layer->img.offset_plane2;

	dest_layer->img.vir_addr = src_layer->img.vir_addr;
	dest_layer->img.width = src_layer->img.width;
}

/* TODO: just support gpu compose now */
static int hisi_fb_adaptor_build_pre_pipeline(struct hisi_display_frame *frames,
		struct dss_overlay_block *ov_h_block_infos, uint32_t count)
{
	struct pipeline_src_layer *dest_layer = NULL;
	struct dss_layer *src_layer = NULL;
	struct pipeline *p_pipeline = NULL;
	int i;
	uint32_t ops_count = 0;
	uint32_t cld_layer_cnt = 0;

	disp_pr_info(" ++++ ");

	if (count != 1)
		return -1;

	disp_pr_debug("ov_h_block_infos->layer_nums=%d\n", ov_h_block_infos->layer_nums);
	if (ov_h_block_infos->layer_nums > SRC_LAYERS_MAX_COUNT) {
		return -1;
	}

	frames->pre_pipeline_count = ov_h_block_infos->layer_nums;
	for (i = 0; i < ov_h_block_infos->layer_nums; ++i) {
		src_layer = &ov_h_block_infos[0].layer_infos[i];
		p_pipeline = &(frames->pre_pipelines[i].pipeline);
		disp_pr_debug("frames->pre_pipelines[%d].pipeline:0x%p\n", i, p_pipeline);

		p_pipeline->pipeline_id = i;
		ops_count = 0;

		disp_pr_info("src_layer->src_type=%u ", get_dss_layer_src_type(src_layer->layer_idx));

		build_pre_pipeline_operator(frames, ov_h_block_infos, i, &ops_count, &cld_layer_cnt);

		p_pipeline->operator_count = ops_count;

		dest_layer = &(frames->pre_pipelines[i].src_layer);

		dest_layer->acquire_fence = src_layer->acquire_fence;
		dest_layer->blending_mode = src_layer->blending;
		dest_layer->dma_sel = 1 << p_pipeline->operators[0].info.idx;
		memcpy(&dest_layer->dst_rect, &src_layer->dst_rect, sizeof(src_layer->dst_rect));
		dest_layer->glb_alpha = src_layer->glb_alpha;
		dest_layer->glb_color = src_layer->color;
		dest_layer->layer_id = src_layer->layer_idx;
		dest_layer->need_caps = src_layer->need_cap | (src_layer->is_cld_layer == 0 ? 0 : CAP_CLD);
		memcpy(&dest_layer->src_rect_mask, &src_layer->src_rect_mask, sizeof(src_layer->src_rect_mask));
		memcpy(&dest_layer->src_rect, &src_layer->src_rect, sizeof(src_layer->src_rect));
		dest_layer->transform = src_layer->transform;

		/* build dest layer img */
		build_dest_layer_img(src_layer, dest_layer);

		dest_layer->is_cld_layer    = src_layer->is_cld_layer;
		dest_layer->cld_width       = src_layer->cld_width;
		dest_layer->cld_height      = src_layer->cld_height;
		dest_layer->cld_data_offset = src_layer->cld_data_offset;
		dest_layer->cld_reuse = cld_layer_cnt > 1 ? true : false;

		dest_layer->src_type    = get_dss_layer_src_type(src_layer->layer_idx);
		dest_layer->slice_num = WLT_SLICE_MAX_COUNT;
		dest_layer->dsd_index = get_dss_layer_dsd_index(src_layer->layer_idx);
	}

	disp_pr_info(" ---- ");
	return 0;
}

static void hisi_fb_adaptor_build_post_online_pipeline(struct hisi_display_frame *frames, struct dss_overlay *overlay_frame)
{
	struct post_online_pipeline *post_pipeline = NULL;
	struct pipeline *pipeline = NULL;
	uint32_t ops_count = 0;

	disp_pr_debug(" ++++ ");

	frames->post_offline_count = 0;
	frames->post_online_count = 1;

	post_pipeline = &frames->post_online_pipelines[0];
	disp_pr_debug(" post_pipeline:0x%p ", post_pipeline);


	pipeline = &post_pipeline->pipeline;
	pipeline->pipeline_id = 0;

#ifdef CONFIG_DPP_SCENE0
	if (arsr_post_en == 1) {
		disp_pr_debug("en arsr0 \n");
		ops_count = set_operator(pipeline->operators, COMP_OPS_ARSR, 0, ops_count);
	}

	ops_count = set_operator(pipeline->operators, COMP_OPS_DPP, 0, ops_count);
#ifdef CONFIG_DKMD_DPU_V720
	if(g_debug_en_ddic) {
		ops_count = set_operator(pipeline->operators, COMP_OPS_DDIC, 0, ops_count);
		disp_pr_debug(" en DDIC \n");
	}

	ops_count = set_operator(pipeline->operators, COMP_OPS_DSC, 0, ops_count);
#endif
	ops_count = set_operator(pipeline->operators, COMP_OPS_ITF, 0, ops_count);
#elif defined(CONFIG_DEBUG_DPU_SCENE_1)
	ops_count = set_operator(pipeline->operators, COMP_OPS_DPP, 0, ops_count);
	ops_count = set_operator(pipeline->operators, COMP_OPS_ITF, 0, ops_count);
#else
	ops_count = set_operator(pipeline->operators, COMP_OPS_ITF, DEBUG_ONLINE_SCENE_ID, ops_count);
#endif
	pipeline->operator_count = ops_count;

	memcpy(&post_pipeline->online_info.dirty_rect, &overlay_frame->dirty_rect, sizeof(overlay_frame->dirty_rect));
	post_pipeline->online_info.frame_no = overlay_frame->frame_no;
	disp_pr_debug(" ---- ");
}

static int hisi_fb_adaptor_build_frame(struct hisi_display_frame *frames, struct dpu_source_layers *source_layers)
{
	struct dss_overlay *overlay_frame;
	struct dss_overlay_block *ov_blocks;

	disp_pr_info(" ++++ ");

	overlay_frame = &source_layers->ov_frame;
	ov_blocks = source_layers->ov_blocks;

	if (hisi_fb_adaptor_build_pre_pipeline(frames, ov_blocks, overlay_frame->ov_block_nums)) {
		disp_pr_err("build pre pipeline fail");
		goto free_block_ov_info;
	}

	hisi_fb_adaptor_build_post_online_pipeline(frames, overlay_frame);

	disp_pr_info(" ------ ");
	return 0;

free_block_ov_info:
	disp_pr_info(" ---- ");
	return -1;
}

static void hisi_fb_adaptor_get_out_fence(struct hisi_display_frame *frames, struct dss_overlay *overlay_frame)
{
	if (frames->post_online_count != 1) {
		disp_pr_err("post online count %d is not 1", frames->post_online_count);
		return;
	}

#ifdef CONFIG_DKMD_DPU_V720
	overlay_frame->release_fence = -1;
	overlay_frame->retire_fence = -1;
#else
	overlay_frame->release_fence = frames->post_online_pipelines[0].online_info.release_fence;
	overlay_frame->retire_fence = frames->post_online_pipelines[0].online_info.retire_fence;
#endif
	disp_pr_debug("release_fence:%d, retire_fence:%d", overlay_frame->release_fence, overlay_frame->retire_fence);
}

static int hisifb_dss_get_platform_type(struct fb_info *info, void __user *argp)
{
	int type;
	int ret;

	type = HISIFB_DSS_PLATFORM_TYPE;
	if (hisi_config_get_fpga_flag() == 1)
		type = HISIFB_DSS_PLATFORM_TYPE | FB_ACCEL_PLATFORM_TYPE_FPGA;

	if (!argp) {
		disp_pr_err("NULL Pointer!\n");
		return -EINVAL;
	}
	ret = copy_to_user(argp, &type, sizeof(type));
	if (ret) {
		disp_pr_err("copy to user failed! ret=%d\n", ret);
		ret = -EFAULT;
	}

	return ret;
}

int hisi_mmbuf_request(struct fb_info *info, void __user *argp)
{
	int ret;
	dss_mmbuf_t mmbuf_info;

	disp_check_and_return(!info, -EINVAL, err, "dss mmbuf alloc info NULL Pointer\n");

	disp_check_and_return(!argp, -EINVAL, err, "dss mmbuf alloc argp NULL Pointer\n");

	ret = copy_from_user(&mmbuf_info, argp, sizeof(dss_mmbuf_t));
	if (ret != 0) {
		disp_pr_err("copy for user failed!ret=%d\n", ret);
		return -EINVAL;
	}

	//mmbuf_info.addr = 0x40;//tmp addr
	mmbuf_info.addr = 0x40000000;//fastboot logo addr

	ret = copy_to_user(argp, &mmbuf_info, sizeof(dss_mmbuf_t));
	if (ret != 0) {
		disp_pr_err("copy to user failed!ret=%d\n", ret);
		return -EINVAL;
	}

	return 0;
}

static int hisi_fb_get_sourcelayers(dpu_source_layers_t *source_layers, void __user *argp)
{
	struct dss_overlay *overlay_frame;
	struct dss_overlay_block *ov_blocks;
	uint32_t size;

	overlay_frame = &source_layers->ov_frame;
	if (copy_from_user(overlay_frame, argp, sizeof(*overlay_frame)) != 0) {
		disp_pr_err("copy from user fail");
		return -1;
	}

	if (overlay_frame->ov_block_nums != 1) {
		disp_pr_err("ov_block_nums is %d", overlay_frame->ov_block_nums);
		return -1;
	}

	size = overlay_frame->ov_block_nums * sizeof(*ov_blocks);
	ov_blocks = kmalloc(size, GFP_ATOMIC);
	if (!ov_blocks) {
		disp_pr_err("ov_h_block_infos is null");
		return -1;
	}

	if (copy_from_user(ov_blocks, (void *)(uintptr_t)(overlay_frame->ov_block_infos_ptr), size)) {
		disp_pr_err("copy from user ov_h_block_infos fail");
		goto free_block_ov_info;
	}
	source_layers->ov_blocks = ov_blocks;

	return 0;

free_block_ov_info:
	kfree(ov_blocks);
	return -1;
}

static int hisi_fb_free_sourcelayers(dpu_source_layers_t *source_layers, void __user *argp)
{
	struct dss_overlay *overlay_frame;

	overlay_frame = &source_layers->ov_frame;

	if (copy_to_user(argp, &overlay_frame, sizeof(overlay_frame)) != 0)
		disp_pr_err("copy to user fail");

	kfree(source_layers->ov_blocks);
	source_layers->ov_blocks = NULL;

	return 0;
}

int hisi_fb_adaptor_online_play(struct hisi_fb_info *hfb_info, struct dpu_source_layers *source_layers)
{
	struct dss_overlay *overlay_frame;
	struct hisi_display_frame *using_frame;
	struct hisi_fb_priv_ops *fb_ops = NULL;
	uint32_t time_diff = 0;
	int ret = 0;

	disp_pr_info("present for fb%d, ov%d\n", hfb_info->index, hfb_info->id);
	arsr_post_en = 0;
	overlay_frame = &source_layers->ov_frame;
	fb_ops = &hfb_info->fb_priv_ops;

	using_frame = hisi_fb_get_using_frame(&hfb_info->present_data);

	if (hfb_info->fix_display_info.fix_panel_info->type == PANEL_DP) {
		ret = hisi_vactive0_wait_event(VIDEO_MODE, &time_diff);
	} else {
		ret = hisi_vactive0_wait_event(CMD_MODE, &time_diff);
		hisi_vactive0_set_event(0);
	}

	disp_pr_info(" ret:%d, time_diff:%u", ret, time_diff);

	if (hisi_fb_adaptor_build_frame(using_frame, source_layers) != 0) {
		disp_pr_err("failed to build frame\n");
		return -EINVAL;
	}

	/*	debug for kernel request resource */
	ret = hisi_rm_debug_request_frame_resource(using_frame);
	if (ret) {
		disp_pr_err("debug request resource fail");
		return ret;
	}

	if (!fb_ops->base.present) {
		disp_pr_err("base present is null");
		return -1;
	}

	ret = fb_ops->base.present(hfb_info->id, hfb_info->pdev, using_frame);
	if (ret) {
		disp_pr_err("present fail, using_idx = %u", hfb_info->present_data.using_idx);
		return ret;
	}

	hisi_fb_adaptor_get_out_fence(using_frame, overlay_frame);

	hisi_fb_switch_frame_index(&hfb_info->present_data);

	return ret;
}

int hisi_fb_adaptor_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	struct hisi_fb_info *hfb_info = NULL;
	struct hisi_fb_priv_ops *fb_ops = NULL;
	void __user *argp = (void __user *)(uintptr_t)arg;
	struct dpu_source_layers source_layers;
	int ret = 0;

	hfb_info = (struct hisi_fb_info *)info->par;
	fb_ops = &hfb_info->fb_priv_ops;
	char __iomem *dpu_base_addr = hisi_config_get_ip_base(DISP_IP_BASE_DPU);

	disp_pr_info(" using_idx:%u ", hfb_info->present_data.using_idx);
	g_debug_en_demura = 0; // memset global value
	g_demura_plane_num = 0; // memset global value

	switch (cmd) {
	case HISIFB_OV_ONLINE_PLAY:
		disp_pr_info("HISIFB_OV_ONLINE_PLAY:fb%d, ov%d enter\n", hfb_info->index, hfb_info->id);
		if (hisi_fb_get_sourcelayers(&source_layers, argp))
			return -1;

		ret = hisi_fb_adaptor_online_play(hfb_info, &source_layers);
		hisi_fb_free_sourcelayers(&source_layers, argp);
		disp_pr_info("HISIFB_OV_ONLINE_PLAY:fb%d, ov%d exit\n", hfb_info->index, hfb_info->id);
		break;
	case HISIFB_GRALLOC_MAP_IOVA:
		disp_pr_debug("HISIFB_GRALLOC_MAP_IOVA:%u", HISIFB_GRALLOC_MAP_IOVA);
		ret = hisi_dss_buffer_map_iova(info, argp);
		break;
	case HISIFB_GRALLOC_UNMAP_IOVA:
		disp_pr_debug("HISIFB_GRALLOC_UNMAP_IOVA:%u", HISIFB_GRALLOC_UNMAP_IOVA);
		ret = hisi_dss_buffer_unmap_iova(info, argp);
		break;
	case HISIFB_PLATFORM_TYPE_GET:
		disp_pr_debug("HISIFB_PLATFORM_TYPE_GET:%u", HISIFB_PLATFORM_TYPE_GET);
		ret = hisifb_dss_get_platform_type(info, argp);
		break;
	case HISIFB_VSYNC_CTRL:
		disp_pr_debug("HISIFB_VSYNC_CTRL:%u", HISIFB_VSYNC_CTRL);
		if (fb_ops->vsync_ops.vsync_ctrl_fnc != NULL)
			ret = fb_ops->vsync_ops.vsync_ctrl_fnc(info, argp);
		break;
	case HISIFB_DSS_MMBUF_ALLOC:
		disp_pr_debug("HISIFB_DSS_MMBUF_ALLOC:%u", HISIFB_DSS_MMBUF_ALLOC);
		ret = hisi_mmbuf_request(info, argp);
		break;
	default:
		disp_pr_debug("default cmd");
		return 0;
	}

	return ret;
}
