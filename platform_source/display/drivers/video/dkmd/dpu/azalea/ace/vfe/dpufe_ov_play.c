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

#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#include "dpufe_dbg.h"
#include "dpufe_ov_utils.h"
#include "dpufe_ov_play.h"
#include "dpufe_fb_buf_sync.h"
#include "dpufe_enum.h"
#include "../include/dpu_communi_def.h"
#include "dpufe_virtio_fb.h"
#include "dpufe_pan_display.h"
#include "dpufe_iommu.h"
#include "dpufe_queue.h"
#include "dpufe_async_play.h"

#define MAGIC_NUM_FOR_SEND (0x12345678)

static bool dpufe_online_play_need_continue(
	struct dpufe_data_type *dfd, void __user *argp)
{
	if (!dfd->panel_power_on) {
		DPUFE_INFO("fb%d, panel is power off!\n", dfd->index);
		return false;
	}

	if (g_dpufe_debug_ldi_underflow) {
		if (g_dpufe_err_status & (DSS_PDP_LDI_UNDERFLOW | DSS_SDP_LDI_UNDERFLOW)) {
			mdelay(DPU_COMPOSER_HOLD_TIME);
			return false;
		}
	}

	return true;
}

static int dpufe_check_layer_buff(dss_layer_t *layer)
{
	struct dma_buf *buf = NULL;
	uint64_t iova = 0;

	if (layer->need_cap & (CAP_BASE | CAP_DIM | CAP_PURE_COLOR))
		return 0;

	buf = dpufe_get_buffer_by_sharefd(&iova, layer->img.shared_fd);
	if (IS_ERR_OR_NULL(buf) || (iova == 0)) {
		DPUFE_ERR("buf or iova is error\n");
		return -1;
	}

	if (layer->img.vir_addr != iova) {
		layer->img.vir_addr = iova;
		layer->img.afbc_header_addr = iova + layer->img.afbc_header_offset;
		layer->img.afbc_payload_addr = iova + layer->img.afbc_payload_offset;
	}

	dpufe_put_dmabuf(buf);

	return 0;
}

static int dpufe_get_base_layer_info(dss_overlay_block_t *block_info, struct dpu_core_disp_data *ov_info)
{
	uint32_t i;
	dss_layer_t *layer = NULL;
	struct layer_info *dpufe_layer_info = NULL;
	dss_rect_t *dpufe_ov_block_rect = NULL;

	for (i = 0; i < block_info->layer_nums; i++) {
		layer = &(block_info->layer_infos[i]);
		dpufe_layer_info = &(ov_info->ov_block_infos[0].layer_infos[i]);
		dpufe_check_and_return(dpufe_check_layer_buff(layer) != 0, -1, "layer[%u] check_layer_buff failed", i);
		dpufe_layer_info->format = layer->img.format;
		dpufe_layer_info->width = layer->img.width;
		dpufe_layer_info->height = layer->img.height;
		dpufe_layer_info->bpp = layer->img.bpp;
		dpufe_layer_info->buf_size = layer->img.buf_size;
		dpufe_layer_info->stride = layer->img.stride;
		dpufe_layer_info->shared_fd = layer->img.shared_fd;
		dpufe_layer_info->afbc_header_addr = layer->img.afbc_header_addr;
		dpufe_layer_info->afbc_header_offset = layer->img.afbc_header_offset;
		dpufe_layer_info->afbc_header_stride = layer->img.afbc_header_stride;
		dpufe_layer_info->afbc_payload_addr = layer->img.afbc_payload_addr;
		dpufe_layer_info->afbc_payload_offset = layer->img.afbc_payload_offset;
		dpufe_layer_info->afbc_payload_stride = layer->img.afbc_payload_stride;
		dpufe_layer_info->afbc_scramble_mode = layer->img.afbc_scramble_mode;
		dpufe_layer_info->mmbuf_base = layer->img.mmbuf_base;
		dpufe_layer_info->mmbuf_size = layer->img.mmbuf_size;
		dpufe_layer_info->mmu_enable = layer->img.mmu_enable;
		dpufe_layer_info->stride_plane1 = layer->img.stride_plane1;
		dpufe_layer_info->stride_plane2 = layer->img.stride_plane2;
		dpufe_layer_info->offset_plane1 = layer->img.offset_plane1;
		dpufe_layer_info->offset_plane2 = layer->img.offset_plane2;
		dpufe_layer_info->csc_mode = layer->img.csc_mode;

		dpufe_layer_info->src_rect = layer->src_rect;
		dpufe_layer_info->dst_rect = layer->dst_rect;
		dpufe_layer_info->transform = layer->transform;
		dpufe_layer_info->blending = layer->blending;
		dpufe_layer_info->glb_alpha = layer->glb_alpha;
		dpufe_layer_info->layer_idx = layer->layer_idx;
		dpufe_layer_info->chn_idx = layer->chn_idx;
		dpufe_layer_info->need_cap = layer->need_cap;
		dpufe_layer_info->vir_addr = layer->img.vir_addr;
		dpufe_layer_info->disp_source = NON_RDA_DISP;
	}

	dpufe_ov_block_rect = &(ov_info->ov_block_infos[0].ov_block_rect);
	memcpy(dpufe_ov_block_rect, &block_info->ov_block_rect, sizeof(dss_rect_t));
	return 0;
}

int dpufe_play_pack_data(struct dpufe_data_type *dfd, struct dpu_core_disp_data *ov_info)
{
	dss_overlay_t *pov_req = NULL;
	dss_overlay_block_t *block_info = NULL;

	DPUFE_DEBUG("+\n");

	pov_req = &(dfd->ov_req);
	block_info = (dss_overlay_block_t *)(uintptr_t)(pov_req->ov_block_infos_ptr);
	dpufe_check_and_return(unlikely(!block_info), -1, "block_info is null\n");

	ov_info->frame_no = pov_req->frame_no;
	ov_info->ov_block_infos[0].layer_nums = block_info->layer_nums;
	ov_info->disp_id = dfd->index;
	ov_info->ovl_idx = pov_req->ovl_idx;
	ov_info->magic_num = MAGIC_NUM_FOR_SEND;
	ov_info->ov_block_nums = DPU_OV_BLOCK_NUMS;
	dpufe_check_and_return(dpufe_get_base_layer_info(block_info, ov_info) != 0, -1,
		"fb%u get layer info failed\n", dfd->index);

	DPUFE_DEBUG("-\n");
	return 0;
}

static void dpufe_dump_overlay_info(struct dpu_core_disp_data *ov_info)
{
	struct block_info *block = ov_info->ov_block_infos;
	struct layer_info *layer_info = &block->layer_infos[0];
	uint32_t i;

	DPUFE_INFO("dss_overlay_t = {\n"
			".frame_no = %d\n"
			".ovl_idx = %d\n"
			".ov_block_nums = %d\n"
			".disp_id = %d\n"
			".magic_num = %d\n"
			".ov_block_rect = {x=%d, y=%d, w=%d, h=%d}\n"
			".layer_nums = %d\n"
			"}\n",
			ov_info->frame_no,
			ov_info->ovl_idx,
			ov_info->ov_block_nums,
			ov_info->disp_id,
			ov_info->magic_num,
			block->ov_block_rect.x, block->ov_block_rect.y,
			block->ov_block_rect.w,block->ov_block_rect.h,
			block->layer_nums);

	for (i = 0; i < block->layer_nums; i++) {
		layer_info = &(block->layer_infos[i]);

		DPUFE_INFO("\n"
			".layer_infos[%d] = {\n"
			"		.format = %d\n"
			"		.width = %d\n"
			"		.height = %d\n"
			"		.bpp = %d\n"
			"		.stride = %d\n"
			"		.buf_size = %d\n"
			"		.vir_addr = %lx\n"
			"		.shared_fd = %d\n"
			"		.src_rect = {x=%d, y=%d, w=%d, h=%d}\n"
			"		.dst_rect = {x=%d, y=%d, w=%d, h=%d}\n"
			"		.transform = %d\n"
			"		.blending = %d\n"
			"		.glb_alpha = %d\n"
			"		.color = %d\n"
			"		.layer_idx = %d\n"
			"		.chn_idx = %d\n"
			"		.need_cap = %d\n"
			"		.mmu_enable = %d\n"
			"	}\n",
			i,
			layer_info->format,
			layer_info->width,
			layer_info->height,
			layer_info->bpp,
			layer_info->stride,
			layer_info->buf_size,
			layer_info->vir_addr,
			layer_info->shared_fd,
			layer_info->src_rect.x, layer_info->src_rect.y,
			layer_info->src_rect.w, layer_info->src_rect.h,
			layer_info->dst_rect.x,layer_info->dst_rect.y,
			layer_info->dst_rect.w, layer_info->dst_rect.h,
			layer_info->transform,
			layer_info->blending,
			layer_info->glb_alpha,
			layer_info->color,
			layer_info->layer_idx,
			layer_info->chn_idx,
			layer_info->need_cap,
			layer_info->mmu_enable);
	}
}

int dpufe_play_send_info(struct dpufe_data_type *dfd, struct dpu_core_disp_data *ov_info)
{
	int ret;
	timeval_compatible tv_begin;

	if (g_debug_overlay_info)
		dpufe_dump_overlay_info(ov_info);

	dpufe_trace_ts_begin(&tv_begin);
	ret = vfb_send_buffer(ov_info);
	if (ret != 0) {
		DPUFE_ERR("send buffer failed\n");
		return ret;
	}
	dpufe_trace_ts_end(&tv_begin, g_dpufe_debug_time_threshold, "fb[%u] frame_no[%u] vfb send buffer info timediff=",
		dfd->index, ov_info->frame_no);

	return 0;
}
static void dpufe_online_play_post_handle(struct dpufe_data_type *dfd, dss_overlay_t *pov_req)
{
	/* pass to hwcomposer handle, driver doesn't use it no longer */
	pov_req->release_fence = -1;
	pov_req->retire_fence = -1;

	dfd->frame_count++;
}

static int dpufe_online_play_config(struct dpufe_data_type *dfd, bool need_sync_fence)
{
	int ret;
	struct list_head lock_list;
	dss_overlay_t *pov_req = NULL;

	DPUFE_DEBUG("fb%d+\n", dfd->index);

	pov_req = &(dfd->ov_req);
	INIT_LIST_HEAD(&lock_list);
	dpufe_layerbuf_lock(dfd, pov_req, &lock_list);

	dpufe_buf_sync_create_fence(dfd, &pov_req->release_fence, &pov_req->retire_fence);

	ret = dpufe_play_pack_data(dfd, dfd->disp_data);
	if (ret) {
		DPUFE_ERR("online play block config failed!\n");
		goto err_return;
	}

	ret = dpufe_play_send_info(dfd, dfd->disp_data);
	if (ret) {
		DPUFE_ERR("online play commit failed!\n");
		goto err_return;
	}

	if (need_sync_fence)
		dpufe_update_frame_refresh_state(dfd);

	dpufe_append_layer_list(dfd, &lock_list);
	DPUFE_DEBUG("fb%d-\n", dfd->index);
	return ret;

err_return:
	dpufe_layerbuf_lock_exception(dfd, &lock_list);
	return ret;
}

int dpufe_ov_online_play(struct dpufe_data_type *dfd, void __user *argp)
{
	int ret;
	uint64_t ov_block_infos_ptr;
	dss_overlay_t *pov_req = NULL;
	timeval_compatible tv_begin;
	bool need_sync_fence = true;

	dpufe_check_and_return(!dfd, -EINVAL, "dfd is nullptr!\n");
	dpufe_check_and_return(!argp, -EINVAL, "argp is nullptr!\n");

	DPUFE_DEBUG("fb%d +\n", dfd->index);

	if (dfd->evs_enable)
		need_sync_fence = false;

	dfd->online_play_count++;
	if (dfd->online_play_count <= DSS_ONLINE_PLAY_PRINT_COUNT)
		DPUFE_INFO("[online_play_count = %d] fb%d dpufe_ov_online_play.\n", dfd->online_play_count, dfd->index);

	if (!dpufe_online_play_need_continue(dfd, argp)) {
		DPUFE_INFO("fb%d skip online play!\n", dfd->index);
		return 0;
	}

	pov_req = &(dfd->ov_req);
	dpufe_trace_ts_begin(&tv_begin);

	ret = dpufe_overlay_get_ov_data_from_user(dfd, &(dfd->ov_req), argp);
	if (ret != 0) {
		DPUFE_ERR("fb%d, dpufe_dss_get_ov_data_from_user failed! ret=%d\n", dfd->index, ret);
		return ret;
	}

	if (need_sync_fence)
		dpufe_check_last_frame_start_working(&dfd->vstart_ctl);
	else
		wait_last_frame_start_working(dfd);

	down(&dfd->blank_sem);
	ret = dpufe_online_play_config(dfd, need_sync_fence);
	if (ret == 0) {
		ov_block_infos_ptr = pov_req->ov_block_infos_ptr;
		pov_req->ov_block_infos_ptr = (uint64_t)0;
		if (copy_to_user((struct dss_overlay_t __user *)argp, pov_req, sizeof(dss_overlay_t))) {
			dpufe_buf_sync_close_fence(&pov_req->release_fence, &pov_req->retire_fence);
			DPUFE_ERR("fb%d, copy_to_user failed\n", dfd->index);
			ret = -EFAULT;
		} else {
			/* restore the original value from the variable ov_block_infos_ptr */
			pov_req->ov_block_infos_ptr = ov_block_infos_ptr;
			dpufe_online_play_post_handle(dfd, pov_req);
		}
	}

	up(&dfd->blank_sem);
	dpufe_trace_ts_end(&tv_begin, g_dpufe_debug_time_threshold, "fb[%u] config ov online play timediff=", dfd->index);
	dpufe_queue_push_tail(&g_dpu_dbg_queue[DBG_DPU1_PLAY], ktime_get());

	DPUFE_DEBUG("fb%d-\n", dfd->index);
	return ret;
}

int dpufe_evs_switch(struct dpufe_data_type *dfd, const void __user *argp)
{
	int ret = 0;
	int evs_enable = 0;

	ret = (int)copy_from_user(&evs_enable, argp, sizeof(int));
	if (ret) {
		DPUFE_ERR("copy arg fail\n");
		return -1;
	}
	dfd->evs_enable = evs_enable ? true : false;

	DPUFE_INFO("DPUFE_EVS_SWITCH, evs enable:%d, evs fb index %d\n",
		evs_enable, dfd->index);
	return 0;
}
