/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#include <linux/mutex.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/kernel.h>

#include "dpu_disp_merger_mgr.h"
#include "dpu_fb_def.h"
#include "dpu_fb_struct.h"
#include "../include/dpu_communi_def.h"
#include "overlay/dump/dpu_dump.h"
#include "dpu_iommu.h"
#include "dpu_fb.h"
#include "merger_mgr/dpu_frame_buffer_mgr.h"
#include "dpu_fb_panel.h"

#define MAX_LAYER_SUP_FOR_RDA 1
#define MAX_LAYER_SUP_FOR_NON_RDA 1
#define GLB_ALPHA_NON_TRANS 255
#define USE_MMU 1
#define FRAME_BUF_SIZE_ALIGN 64
#define MERGE_PLAY_TIMEOUT_THRESHOLD 300 /* ms */
#define WAIT_TIMES_NUM 50
#define TIME_EACH_WAIT 10 /* ms */
#define DUMP_PERF_FRAME_NUM 10

static uint32_t rchs_for_rda[MAX_LAYER_SUP_FOR_RDA] = {DSS_RCHN_V1};

bool is_support_disp_merge(struct dpu_fb_data_type *dpufd)
{
	return dpufd->index == EXTERNAL_PANEL_IDX;
}

static inline bool is_support_block_sync(struct dpu_core_disp_data *disp_data)
{
	return is_from_rda(disp_data);
}

static struct dpu_fb_data_type *to_dpufd(struct disp_merger_mgr *merger_mgr)
{
	return container_of(merger_mgr, struct dpu_fb_data_type, merger_mgr);
}

static void dpu_get_newest_merge_disp_data(
	struct disp_merger_mgr *merger_mgr, struct dpu_core_disp_data *rda_info, struct dpu_core_disp_data *non_rda_info)
{
	struct timeval tv_start;

	dpu_trace_ts_begin(&tv_start);
	if (need_rda_disp(merger_mgr)) {
		mutex_lock(&merger_mgr->rda_disp_lock);
		memcpy(rda_info, &merger_mgr->rda_overlay, sizeof(struct dpu_core_disp_data));
		mutex_unlock(&merger_mgr->rda_disp_lock);
	}
	if (need_non_rda_disp(merger_mgr)) {
		mutex_lock(&merger_mgr->non_rda_disp_lock);
		memcpy(non_rda_info, &merger_mgr->non_rda_overlay, sizeof(struct dpu_core_disp_data));
		mutex_unlock(&merger_mgr->non_rda_disp_lock);
	}
	dpu_trace_ts_end(&tv_start, g_online_comp_timeout, "newest merge data need rda:%d, non-rda:%d\n",
		need_rda_disp(merger_mgr), need_non_rda_disp(merger_mgr));
}

static void dpu_revise_merged_info(struct dpu_core_disp_data *merged_info)
{
	uint32_t i;
	struct layer_info *layer = NULL;
	struct dpu_fb_data_type *dpufd = dpufb_get_dpufd(EXTERNAL_PANEL_IDX);

	merged_info->ov_block_nums = DPU_OV_BLOCK_NUMS;
	merged_info->disp_id = EXTERNAL_PANEL_IDX;
	merged_info->ovl_idx = DSS_OVL1;
	merged_info->magic_num = SND_INFO_MAGIC_NUM;
	merged_info->frame_no = 0;

	merged_info->ov_block_infos[0].ov_block_rect.x = 0;
	merged_info->ov_block_infos[0].ov_block_rect.y = 0;
	merged_info->ov_block_infos[0].ov_block_rect.w = get_panel_xres(dpufd);
	merged_info->ov_block_infos[0].ov_block_rect.h = get_panel_yres(dpufd);

	for (i = 0; i < merged_info->ov_block_infos[0].layer_nums; i++) {
		dpu_check_and_no_retval(i >= MAX_DSS_SRC_NUM, ERR, "merged layer num is larger than %d", MAX_DSS_SRC_NUM);
		layer = &(merged_info->ov_block_infos[0].layer_infos[i]);
		if (layer->disp_source == RDA_DISP) {
			layer->chn_idx = rchs_for_rda[0]; /* maintained by by display drv */
			layer->stride = ALIGN_UP(layer->width, FRAME_BUF_SIZE_ALIGN) * layer->bpp;
			layer->buf_size = layer->stride * layer->height;
			layer->color = 0;
			layer->transform = 0;
			layer->need_cap = 0;
			layer->afbc_header_addr = 0;
			layer->afbc_header_offset = 0;
			layer->afbc_payload_addr = 0;
			layer->afbc_payload_offset = 0;
			layer->afbc_header_stride = 0;
			layer->afbc_payload_stride = 0;
			layer->afbc_scramble_mode = 0;
			layer->stride_plane1 = 0;
			layer->stride_plane2 = 0;
			layer->offset_plane1 = 0;
			layer->offset_plane2 = 0;
			layer->csc_mode = 0;
			layer->mmbuf_base = 0;
			layer->mmbuf_size = 0;
			layer->mmu_enable = USE_MMU;
		}
	}
}

/* Attention : rda layers should always be on the top, otherwise no blending effect */
static int dpu_merge_layer_info(struct dpu_core_disp_data *merged_info,
	struct dpu_core_disp_data *rda_info, struct dpu_core_disp_data *non_rda_info)
{
	uint32_t i;
	uint32_t layer_index = 0;
	struct layer_info *layer = NULL;
	struct block_info *merged_blk_info = &merged_info->ov_block_infos[0];
	struct block_info *rda_blk_info = &rda_info->ov_block_infos[0];
	struct block_info *non_rda_blk_info = &non_rda_info->ov_block_infos[0];

	for (i = 0; i < non_rda_blk_info->layer_nums; i++) {
		dpu_check_and_return(i >= MAX_LAYER_SUP_FOR_NON_RDA, -1, ERR,
			"non rda layer num is larger than %d", MAX_LAYER_SUP_FOR_NON_RDA);
		layer = &merged_blk_info->layer_infos[i];
		memcpy(layer, &non_rda_blk_info->layer_infos[i], sizeof(struct layer_info));
		layer->layer_idx = layer_index;
		layer_index ++;
	}

	for (i = 0; i < rda_blk_info->layer_nums; i++) {
		dpu_check_and_return((i >= MAX_LAYER_SUP_FOR_RDA) || (layer_index + i >= MAX_DSS_SRC_NUM), -1, ERR,
			"rda layer num is larger than %d", MAX_LAYER_SUP_FOR_RDA);
		layer = &(merged_blk_info->layer_infos[layer_index + i]);
		memcpy(layer, &rda_blk_info->layer_infos[i], sizeof(struct layer_info));
		layer->layer_idx = layer_index;
		layer_index ++;
	}

	merged_blk_info->layer_nums = rda_blk_info->layer_nums + non_rda_blk_info->layer_nums;

	/* some vars are not needed to be fill by rda usr, but need fill in this architecture */
	dpu_revise_merged_info(merged_info);

	if (g_debug_ovl_online_composer != 0) {
		dump_core_disp_info(rda_info);
		dump_core_disp_info(non_rda_info);
		dump_core_disp_info(merged_info);
	}

	return 0;
}

/* copy layers to dpufd->ov_req, then layers will be composed automatically */
static int dpu_load_merged_disp_info(struct disp_merger_mgr *merger_mgr,
	struct dpu_core_disp_data *merged_info)
{
	int ret;
	dss_overlay_t *pov_req = NULL;
	struct dpu_fb_data_type *dpufd = to_dpufd(merger_mgr);

	pov_req = &(dpufd->ov_req);

	pov_req->ov_block_nums = merged_info->ov_block_nums;
	pov_req->frame_no = merged_info->frame_no;
	pov_req->ovl_idx = merged_info->ovl_idx;
	pov_req->ov_block_infos[0].layer_nums = merged_info->ov_block_infos[0].layer_nums;
	pov_req->ov_block_infos[0].ov_block_rect = merged_info->ov_block_infos[0].ov_block_rect;

	ret = dpu_load_layer_info(&pov_req->ov_block_infos[0], &merged_info->ov_block_infos[0], dpufd->index);
	dpu_check_and_return(ret != 0, -1, ERR, "load layer info failed\n");

	if (g_debug_ovl_online_composer)
		dump_dss_overlay(dpufd, pov_req);
	return 0;
}

static void dpu_notify_disp_task_submitter(struct disp_merger_mgr *merger_mgr,
	bool has_new_rda_task, bool has_new_non_rda_task)
{
	if (has_new_non_rda_task)
		merger_mgr->is_non_rda_task_consumed = true;
	if (has_new_rda_task)
		merger_mgr->is_rda_task_consumed = true;
	wake_up_interruptible_all(&(merger_mgr->wait_consume));
}

static int dpu_merge_disp_data(struct disp_merger_mgr *merger_mgr)
{
	int ret;
	struct dpu_core_disp_data merged_info = {0};
	struct dpu_core_disp_data rda_info = {0};
	struct dpu_core_disp_data non_rda_info = {0};
	struct timeval tv_start;

	dpu_trace_ts_begin(&tv_start);
	dpu_get_newest_merge_disp_data(merger_mgr, &rda_info, &non_rda_info);

	ret = dpu_merge_layer_info(&merged_info, &rda_info, &non_rda_info);
	dpu_check_and_return(ret != 0, -1, ERR, "merge layer info failed\n");

	ret = dpu_load_merged_disp_info(merger_mgr, &merged_info);
	dpu_check_and_return(ret != 0, -1, ERR, "load merged disp info failed\n");

	dpu_trace_ts_end(&tv_start, g_online_comp_timeout, "dpu_merge_disp_data");

	return 0;
}

static int dpu_merge_play(struct dpu_fb_data_type *dpufd)
{
	int ret;
	struct timeval tv_start;

	dpu_trace_ts_begin(&tv_start);

	down(&dpufd->blank_sem0);
	ret = dpu_online_play_config_locked(dpufd);
	up(&dpufd->blank_sem0);
	dpu_check_and_return(ret != 0, -1, ERR, "fb%u dpu merge play failed\n", dpufd->index);

	dpu_online_play_post_handle(dpufd);

	dpu_trace_ts_end(&tv_start, g_online_comp_timeout, "dpu_merge_play");
	return 0;
}

static void dump_merge_play_perf(struct dpu_fb_data_type *dpufd, struct disp_merger_mgr *merger_mgr)
{
	if ((merger_mgr->is_non_rda_task_consumed == false) &&
		((dpufd->online_play_count < ONLINE_PLAY_LOG_PRINTF) || (g_dump_each_non_rda_frame != 0))) {
		DPU_FB_INFO("non_rda frame[%u] is displaying\n", merger_mgr->non_rda_overlay.frame_no);
		dpufd->online_play_count++;
	}

	if ((merger_mgr->is_rda_task_consumed == false) && (g_dump_each_rda_frame != 0))
		DPU_FB_INFO("rda frame[%u] is displaying\n", merger_mgr->rda_frame_no);
}

static void dpu_merge_play_handler(struct work_struct *work)
{
	int ret;
	struct disp_merger_mgr *merger_mgr = NULL;
	struct dpu_fb_data_type *dpufd = NULL;
	struct timeval tv_start;
	bool has_new_rda_task = false;
	bool has_new_non_rda_task = false;

	dpu_trace_ts_begin(&tv_start);

	dpu_check_and_no_retval(!work, ERR, "work is NULL\n");
	merger_mgr = container_of(work, struct disp_merger_mgr, disp_merge_work);

	dpufd = to_dpufd(merger_mgr);
	DPU_FB_DEBUG("fb%u +\n", dpufd->index);

	/* in case wq scheduling is very slow */
	if (!dpu_check_vsync_valid(dpufd))
		return;

	dump_merge_play_perf(dpufd, merger_mgr);

	merger_mgr->is_playing = true;
	if (merger_mgr->is_rda_task_consumed == false)
		has_new_rda_task = true;
	if (merger_mgr->is_non_rda_task_consumed == false)
		has_new_non_rda_task = true;

	ret = dpu_merge_disp_data(merger_mgr);
	if (ret != 0) {
		DPU_FB_ERR("fb%u merge disp data failed\n", dpufd->index);
		goto end;
	}

	ret = dpu_merge_play(dpufd);
	if (ret != 0) {
		DPU_FB_ERR("fb%u dpu merge play failed\n", dpufd->index);
		goto end;
	}

end:
	merger_mgr->is_playing = false;
	if (has_new_rda_task)
		dpu_lock_buf_sync(merger_mgr);
	dpu_notify_disp_task_submitter(merger_mgr, has_new_rda_task, has_new_non_rda_task);

	dpu_trace_ts_end(&tv_start, g_online_comp_timeout, "dpu_merge_play_handler");
	DPU_FB_DEBUG("fb%u -\n", dpufd->index);
}

static void dpu_exec_disp_merger_task(struct disp_merger_mgr *merger_mgr)
{
	struct dpu_fb_data_type *dpufd = NULL;
	dpu_check_and_no_retval(!merger_mgr, ERR, "merger_mgr is null\n");

	dpufd = to_dpufd(merger_mgr);
	if (!is_support_disp_merge(dpufd))
		return;

	if (!has_disp_merge_task(merger_mgr))
		return;

	if (!dpufd->panel_power_on) {
		DPU_FB_INFO("fb%u is power off\n", dpufd->index);
		return;
	}

	if (merger_mgr->is_playing) {
		DPU_FB_DEBUG("last frame is playing\n");
		return;
	}

	if (merger_mgr->disp_merge_wq)
		queue_work(merger_mgr->disp_merge_wq, &merger_mgr->disp_merge_work);
}

/* share fd cannot across processes context, it's safer to use dma buf */
static int dpu_save_layer_dmabuf(struct dpu_core_disp_data *ov_info)
{
	uint32_t i;
	struct dma_buf *buf = NULL;
	struct block_info *ov_block_infos = &(ov_info->ov_block_infos[0]);

	if (!is_from_rda(ov_info))
		return 0;

	for (i = 0; i < ov_block_infos->layer_nums; i++) {
		buf = dpu_get_dmabuf(ov_block_infos->layer_infos[i].shared_fd);
		dpu_check_and_return(!buf, -1, ERR, "dpu_get_dmabuf failed\n");
		ov_block_infos->layer_infos[i].buf = buf;
		ov_block_infos->layer_infos[i].shared_fd = -1;
	}

	return 0;
}

static int dpu_wait_disp_task_consumed(struct disp_merger_mgr *merger_mgr, struct dpu_core_disp_data *tmp_ov_info)
{
	int ret = 0;
	int times = 0;
	struct timeval tv_start;

	if (!is_support_block_sync(tmp_ov_info))
		return 0;

	dpu_trace_ts_begin(&tv_start);

	while (true) {
		ret = wait_event_interruptible_timeout(merger_mgr->wait_consume,
			merger_mgr->is_rda_task_consumed, msecs_to_jiffies(MERGE_PLAY_TIMEOUT_THRESHOLD));
		if ((ret == -ERESTARTSYS) && (times++ < WAIT_TIMES_NUM))
			mdelay(TIME_EACH_WAIT);
		else
			break;
	}

	ret = (ret <= 0) ? -1 : 0;
	dpu_trace_ts_end(&tv_start, g_online_comp_timeout, "dpu_wait_disp_task_consumed");

	return ret;
}

static bool dpu_check_disp_data_valid(struct dpu_core_disp_data *tmp_ov_info)
{
	if (tmp_ov_info->magic_num != SND_INFO_MAGIC_NUM) {
		DPU_FB_ERR("wrong magic num : %u\n", tmp_ov_info->magic_num);
		return false;
	}

	if (tmp_ov_info->ov_block_infos[0].layer_nums >= MAX_DSS_SRC_NUM) {
		DPU_FB_ERR("invalid layer_nums : %u\n", tmp_ov_info->ov_block_infos[0].layer_nums);
		return false;
	}

	return true;
}

static void dpu_check_blending_data(struct dpu_core_disp_data *tmp_ov_info)
{
	uint32_t i;
	struct layer_info *layer = NULL;

	if (!is_from_rda(tmp_ov_info))
		return;

	for (i = 0; i < tmp_ov_info->ov_block_infos[0].layer_nums; i++) {
		layer = &(tmp_ov_info->ov_block_infos[0].layer_infos[i]);
		if (layer->blending != DPU_FB_BLENDING_COVERAGE)
			layer->blending = DPU_FB_BLENDING_COVERAGE;
		if (layer->glb_alpha != GLB_ALPHA_NON_TRANS)
			layer->glb_alpha = GLB_ALPHA_NON_TRANS;
	}
}

static int dpu_preprocess_disp_data(struct dpu_core_disp_data *tmp_ov_info)
{
	int ret;

	if (!dpu_check_disp_data_valid(tmp_ov_info)) {
		DPU_FB_ERR("disp data is invalid\n");
		return -1;
	}

	ret = dpu_save_layer_dmabuf(tmp_ov_info);
	if (ret != 0) {
		DPU_FB_ERR("save layer dmabuf failed\n");
		return -1;
	}

	dpu_check_blending_data(tmp_ov_info);

	return 0;
}

static void dump_disp_commit_task_perf(struct disp_merger_mgr *merger_mgr)
{
	merger_mgr->rda_frame_no++;
	if ((merger_mgr->rda_frame_no < DUMP_PERF_FRAME_NUM) || (g_dump_each_rda_frame != 0))
		DPU_FB_INFO("frame[%u] : save rda task\n", merger_mgr->rda_frame_no);
}

static int dpu_save_disp_data(struct disp_merger_mgr *merger_mgr, void __user *argp)
{
	int ret;
	struct dpu_core_disp_data tmp_ov_info = {0};
	struct dpu_fb_data_type *dpufd = to_dpufd(merger_mgr);
	struct timeval tv_start;

	ret = copy_from_user(&tmp_ov_info, argp, sizeof(struct dpu_core_disp_data));
	if (ret) {
		DPU_FB_ERR("fb%d, copy_from_user failed!\n", dpufd->index);
		return -1;
	}

	if (dpu_preprocess_disp_data(&tmp_ov_info) != 0) {
		DPU_FB_ERR("fb%u dpu_preprocess_disp_data failed\n");
		return -1;
	}

	if (dpu_wait_disp_task_consumed(merger_mgr, &tmp_ov_info) != 0) {
		DPU_FB_ERR("fb%u merge play timeout\n", dpufd->index);
		return -1;
	}

	dpu_trace_ts_begin(&tv_start);

	if (is_from_rda(&tmp_ov_info)) {
		mutex_lock(&merger_mgr->rda_disp_lock);
		memcpy(&merger_mgr->rda_overlay, &tmp_ov_info, sizeof(struct dpu_core_disp_data));
		merger_mgr->merge_disp_status[RDA_DISP] = DISP_OPEN;
		merger_mgr->is_rda_task_consumed = false;
		dump_disp_commit_task_perf(merger_mgr);
		DPU_FB_DEBUG("save rda task\n");
		mutex_unlock(&merger_mgr->rda_disp_lock);
	} else {
		mutex_lock(&merger_mgr->non_rda_disp_lock);
		memcpy(&merger_mgr->non_rda_overlay, &tmp_ov_info, sizeof(struct dpu_core_disp_data));
		merger_mgr->merge_disp_status[NON_RDA_DISP] = DISP_OPEN;
		merger_mgr->is_non_rda_task_consumed = false;
		DPU_FB_DEBUG("save non-rda task\n");
		mutex_unlock(&merger_mgr->non_rda_disp_lock);
	}
	dpu_trace_ts_end(&tv_start, g_online_comp_timeout, "copy ovinfo from %s\n",
		is_from_rda(&tmp_ov_info) ? "rda" : "non-rda");

	dpu_trace_ts_begin(&tv_start);
	if (is_support_block_sync(&tmp_ov_info))
		wait_buffer_available(merger_mgr);

	dpu_trace_ts_end(&tv_start, g_online_comp_timeout, "wait_buffer_available");
	return 0;
}

static int dpu_commit_merger_task(struct disp_merger_mgr *merger_mgr, void __user *argp)
{
	int ret;
	struct dpu_fb_data_type *dpufd = NULL;
	struct timeval tv_start;

	dpu_check_and_return(unlikely(!merger_mgr), -1, ERR, "merger_mgr is null\n");
	dpu_check_and_return(unlikely(!argp), -1, ERR, "argp is null\n");

	dpufd = to_dpufd(merger_mgr);

	DPU_FB_DEBUG("fb%u +\n", dpufd->index);
	dpu_trace_ts_begin(&tv_start);

	if (is_support_disp_merge(dpufd) == false) {
		DPU_FB_ERR("fb%d not support disp merge\n", dpufd->index);
		return 0;
	}

	if (dpufd->panel_power_on == false) {
		DPU_FB_INFO("fb%d, panel is power off!\n", dpufd->index);
		return 0;
	}

	ret = dpu_save_disp_data(merger_mgr, argp);
	dpu_check_and_return(ret != 0, ret, ERR, "fb%d merge task commit failed\n", dpufd->index);

	dpu_trace_ts_end(&tv_start, g_online_comp_timeout, "commit_merger_task");

	DPU_FB_DEBUG("fb%u -\n", dpufd->index);
	return 0;
}

static void dpu_init_merger_lock(struct disp_merger_mgr *merger_mgr)
{
	mutex_init(&merger_mgr->rda_disp_lock);
	mutex_init(&merger_mgr->non_rda_disp_lock);
}

static int dpu_init_merger_workqueue(struct disp_merger_mgr *merger_mgr)
{
	struct dpu_fb_data_type *dpufd = to_dpufd(merger_mgr);

	merger_mgr->disp_merge_wq = alloc_ordered_workqueue("multi_disp_merge", WQ_HIGHPRI | WQ_MEM_RECLAIM);
	dpu_check_and_return(merger_mgr->disp_merge_wq == NULL, -1, ERR,
		"fb%u alloc disp_merge_wq failed\n",dpufd->index);

	INIT_WORK(&merger_mgr->disp_merge_work, dpu_merge_play_handler);
	return 0;
}

static int dpu_init_merger_mgr(struct disp_merger_mgr *merger_mgr)
{
	struct dpu_fb_data_type *dpufd = NULL;

	dpu_check_and_return(unlikely(!merger_mgr), -1, ERR, "merger_mgr is null\n");

	dpufd = to_dpufd(merger_mgr);
	DPU_FB_DEBUG("fb%u +\n", dpufd->index);

	dpu_check_and_return(!is_support_disp_merge(dpufd), 0, ERR, "fb%d not support disp merge\n", dpufd->index);

	dpu_init_merger_lock(merger_mgr);
	dpu_check_and_return((dpu_init_merger_workqueue(merger_mgr) != 0), -1, ERR, "merge_init_workqueue\n");
	init_waitqueue_head(&merger_mgr->wait_consume);
	merger_mgr->is_non_rda_task_consumed = true;
	merger_mgr->is_rda_task_consumed = true;
	merger_mgr->is_playing = false;
	merger_mgr->rda_frame_no = 0;
	memset(merger_mgr->merge_disp_status, DISP_NONE, sizeof(merger_mgr->merge_disp_status));

	init_buf_sync_mgr();

	DPU_FB_DEBUG("fb%u -\n", dpufd->index);
	return 0;
}

static void dpu_deinit_merger_mgr(struct disp_merger_mgr *merger_mgr)
{
	struct dpu_fb_data_type *dpufd = NULL;

	dpu_check_and_no_retval(unlikely(!merger_mgr), ERR, "merger_mgr is null\n");

	dpufd = to_dpufd(merger_mgr);
	DPU_FB_DEBUG("fb%u +\n", dpufd->index);

	dpu_check_and_no_retval(!is_support_disp_merge(dpufd), ERR, "fb%d not support disp merge\n", dpufd->index);

	destroy_workqueue(merger_mgr->disp_merge_wq);
	memset(merger_mgr, 0, sizeof(struct disp_merger_mgr));

	DPU_FB_DEBUG("fb%u -\n", dpufd->index);
}

static int dpu_exec_disp_merger_enable(struct disp_merger_mgr *merger_mgr, struct disp_merger_ctl *merger_ctl)
{
	struct timeval tv_start;

	dpu_check_and_return(unlikely(!merger_mgr), -1, ERR, "merger_mgr is null\n");
	dpu_check_and_return(unlikely(!merger_ctl), -1, ERR, "merger_ctl is null\n");

	if (merger_ctl->disp_source >= MAX_SRC_NUM) {
		DPU_FB_ERR("disp_source:%u not suport\n", merger_ctl->disp_source);
		return -1;
	}
	dpu_trace_ts_begin(&tv_start);
	merger_ctl->enable = (merger_ctl->enable == 0) ? DISP_CLOSE : DISP_OPEN;

	mutex_lock(&merger_mgr->rda_disp_lock);
	merger_mgr->merge_disp_status[merger_ctl->disp_source] = merger_ctl->enable;
	mutex_unlock(&merger_mgr->rda_disp_lock);
	dpu_trace_ts_end(&tv_start, g_online_comp_timeout, "merger_enable disp_source: %d\n",
		merger_ctl->disp_source);

	return 0;
}

static int dpu_ctl_disp_merger_enable_for_usr(struct disp_merger_mgr *merger_mgr, void __user *argp)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct disp_merger_ctl merger_ctl = {0};
	unsigned long ret;
	int res;

	dpu_check_and_return(unlikely(!merger_mgr), -1, ERR, "merger_mgr is null\n");
	dpu_check_and_return(unlikely(!argp), -1, ERR, "argp is null\n");

	dpufd = to_dpufd(merger_mgr);
	dpu_check_and_return(!is_support_disp_merge(dpufd), -1, ERR, "fb%d not support disp merge\n", dpufd->index);

	ret = copy_from_user(&merger_ctl, argp, sizeof(struct disp_merger_ctl));
	dpu_check_and_return(ret != 0, -1, ERR, "fb%d disp merge ctl ioctl failed!\n", dpufd->index);

	res = dpu_exec_disp_merger_enable(merger_mgr, &merger_ctl);
	dpu_check_and_return(res != 0, -1, ERR, "fb%d dpu_exec_disp_merger_enable failed!\n", dpufd->index);

	DPU_FB_INFO("disp_source:%u disp merge ctl : %u\n", merger_ctl.disp_source, merger_ctl.enable);
	return 0;
}

static void dpu_disp_merger_suspend(struct disp_merger_mgr *merger_mgr)
{
	struct dpu_fb_data_type *dpufd = NULL;

	dpu_check_and_no_retval(unlikely(!merger_mgr), ERR, "merger_mgr is null\n");
	dpufd = to_dpufd(merger_mgr);

	merger_mgr->rda_frame_no = 0;
	dpufd->online_play_count = 0;
	dpu_init_buf_timeline();
}

static struct disp_merge_ops merge_ops = {
	.init = dpu_init_merger_mgr,
	.deinit = dpu_deinit_merger_mgr,
	.commit_task = dpu_commit_merger_task,
	.exec_task = dpu_exec_disp_merger_task,
	.ctl_enable = dpu_exec_disp_merger_enable,
	.ctl_enable_for_usr = dpu_ctl_disp_merger_enable_for_usr,
	.suspend = dpu_disp_merger_suspend,
};

void register_disp_merger_mgr(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_no_retval(unlikely(!dpufd), ERR, "dpufd is null\n");

	if (is_support_disp_merge(dpufd))
		dpufd->merger_mgr.ops = &merge_ops;
}
