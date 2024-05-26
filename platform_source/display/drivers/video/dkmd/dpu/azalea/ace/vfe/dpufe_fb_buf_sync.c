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

#include <linux/syscalls.h>
#include <linux/file.h>
#include "dpufe_fb_buf_sync.h"
#include "dpufe_enum.h"
#include "dpufe_dbg.h"
#include "dpufe_panel_def.h"
#include "dpufe_iommu.h"
#include "../include/dpu_communi_def.h"

#define DPUFE_LAYERBUF_FREE "dpufe%u-layerbuf-free"

/* buf sync fence */
#define BUF_SYNC_TIMEOUT_MSEC (1 * MSEC_PER_SEC)

static void dpufe_put_sync_fence(struct dpufe_fence *fence)
{
	if (!fence) {
		DPUFE_ERR("invalid parameters\n");
		return;
	}
	dma_fence_put((struct dma_fence *) fence);
}

static int dpufe_buf_sync_wait_fence(struct dpufe_fence *fence)
{
	int ret;

	ret = dpufe_wait_sync_fence(fence, BUF_SYNC_TIMEOUT_MSEC);
	if (ret < 0)
		DPUFE_ERR("Waiting on fence=%pK failed, ret=%d\n", fence, ret);

	dpufe_put_sync_fence(fence);

	return ret;
}

static int dpufe_buf_sync_wait(int fence_fd)
{
	struct dpufe_fence *fence = NULL;

	if (fence_fd < 0)
		return 0;

	fence = dpufe_get_fd_sync_fence(fence_fd);
	if (!fence) {
		DPUFE_ERR("fence_fd=%d, sync_fence_fdget failed!\n", fence_fd);
		return -EINVAL;
	}

	return dpufe_buf_sync_wait_fence(fence);
}

static bool dpufe_check_parameter_valid(struct dpufe_data_type *dfd,
	dss_overlay_t *pov_req, struct list_head *plock_list)
{
	if (!dfd) {
		DPUFE_ERR("dfd is NULL!\n");
		return false;
	}
	if (!pov_req) {
		DPUFE_ERR("pov_req is NULL!\n");
		return false;
	}
	if (!plock_list) {
		DPUFE_ERR("plock_list is NULL!\n");
		return false;
	}

	return true;
}

static int dpufe_layerbuf_block_lock(dss_overlay_t *pov_req, struct list_head *plock_list,
	dss_layer_t *layer)
{
	struct dpufe_layerbuf *node = NULL;
	struct dma_buf *buf_handle = NULL;

	buf_handle = dpufe_get_dmabuf(layer->img.shared_fd);
	if (!buf_handle)
		return -EINVAL;

	node = (struct dpufe_layerbuf *)kzalloc(sizeof(struct dpufe_layerbuf), GFP_KERNEL);
	if (!node) {
		DPUFE_ERR("layer_idx%d, failed to kzalloc!\n", layer->layer_idx);
		dpufe_put_dmabuf(buf_handle);
		buf_handle = NULL;
		return -ENOMEM;
	}

	node->shared_fd = layer->img.shared_fd;
	node->buffer_handle = buf_handle;
	node->frame_no = pov_req->frame_no;
	node->timeline = 0;

	node->vir_addr = layer->img.vir_addr;
	node->chn_idx = layer->chn_idx;

	list_add_tail(&node->list_node, plock_list);

	return 0;
}

static void dpufe_block_lock_handle(dss_overlay_block_t *pov_h_block, dss_layer_t *layer,
		struct dpufe_data_type *dfd, dss_overlay_t *pov_req, struct list_head *plock_list)
{
	int i;

	for (i = 0; i < pov_h_block->layer_nums; i++) {
		layer = &(pov_h_block->layer_infos[i]);

		if (layer->dst_rect.y < pov_h_block->ov_block_rect.y)
			continue;

		if (layer->need_cap & (CAP_DIM | CAP_BASE | CAP_PURE_COLOR))
			continue;

		if (dpufe_buf_sync_wait(layer->acquire_fence) != 0) {
			DPUFE_INFO("fb%u wait fence[%d] timeout!\n", dfd->index, layer->acquire_fence);
			dpufe_reset_all_buf_fence(dfd);
			dump_dbg_queues(dfd->index);
		}

		if (dpufe_layerbuf_block_lock(pov_req, plock_list, layer)) {
			if (g_dpufe_debug_layerbuf_sync)
				DPUFE_INFO("fb%u, layer_idx = %d.\n", dfd->index, i);
		}
	}
}

int dpufe_layerbuf_lock(struct dpufe_data_type *dfd, dss_overlay_t *pov_req, struct list_head *plock_list)
{
	dss_overlay_block_t *pov_h_block_infos = NULL;
	dss_overlay_block_t *pov_h_block = NULL;
	dss_layer_t *layer = NULL;
	bool parameter_valid = false;
	uint32_t m;

	dpufe_check_and_return(!dfd, -EINVAL, "dfd is nullptr!\n");
	DPUFE_DEBUG("fb%u+\n", dfd->index);

	parameter_valid = dpufe_check_parameter_valid(dfd, pov_req, plock_list);
	if (!parameter_valid)
		return -EINVAL;

	pov_h_block_infos = (dss_overlay_block_t *)(uintptr_t)(pov_req->ov_block_infos_ptr);
	for (m = 0; m < pov_req->ov_block_nums; m++) {
		pov_h_block = &(pov_h_block_infos[m]);
		dpufe_block_lock_handle(pov_h_block, layer, dfd, pov_req, plock_list);
	}

	DPUFE_DEBUG("fb%u-", dfd->index);
	return 0;
}

void dpufe_layerbuf_unlock(struct dpufe_data_type *dfd,
	struct list_head *pfree_list)
{
	struct dpufe_layerbuf *node = NULL;
	struct dpufe_layerbuf *_node_ = NULL;

	dpufe_check_and_no_retval(!dfd, "dfd is nullptr!\n");
	dpufe_check_and_no_retval(!pfree_list, "pfree_list is nullptr!\n");

	list_for_each_entry_safe(node, _node_, pfree_list, list_node) {
		list_del(&node->list_node);

		if (g_dpufe_debug_layerbuf_sync)
			DPUFE_INFO("fb%u, frame_no=%u, share_fd=%d, buffer_handle=%pK, "
				"timeline=%u, mmbuf(0x%x, %u), vir_addr = 0x%llx, chn_idx = %d\n",
				dfd->index, node->frame_no, node->shared_fd, node->buffer_handle,
				node->timeline, node->mmbuf.addr, node->mmbuf.size, node->vir_addr, node->chn_idx);

		node->timeline = 0;
		if (node->buffer_handle) {
			dpufe_put_dmabuf(node->buffer_handle);
			node->buffer_handle = NULL;
		}
		kfree(node);
	}
}

void dpufe_layerbuf_lock_exception(struct dpufe_data_type *dfd,
	struct list_head *plock_list)
{
	unsigned long flags = 0;

	dpufe_check_and_no_retval(!dfd, "dfd is nullptr!\n");
	dpufe_check_and_no_retval(!plock_list, "plock_list is nullptr!\n");

	spin_lock_irqsave(&(dfd->buf_sync_ctrl.layerbuf_spinlock), flags);
	dfd->buf_sync_ctrl.layerbuf_flushed = false;
	spin_unlock_irqrestore(&(dfd->buf_sync_ctrl.layerbuf_spinlock), flags);

	dpufe_layerbuf_unlock(dfd, plock_list);
}

static void dpufe_layerbuf_unlock_work(struct work_struct *work)
{
	struct dpufe_buf_sync *pbuf_sync = NULL;
	struct dpufe_data_type *dfd = NULL;
	struct dpufe_layerbuf *node = NULL;
	struct dpufe_layerbuf *_node_ = NULL;
	struct list_head free_list;
	unsigned long flags = 0;

	pbuf_sync = container_of(work, struct dpufe_buf_sync, free_layerbuf_work);
	dpufe_check_and_no_retval(!pbuf_sync, "pbuf_sync is nullptr!\n");

	dfd = container_of(pbuf_sync, struct dpufe_data_type, buf_sync_ctrl);
	dpufe_check_and_no_retval(!dfd, "dfd is nullptr!\n");

	INIT_LIST_HEAD(&free_list);
	down(&pbuf_sync->layerbuf_sem);
	spin_lock_irqsave(&pbuf_sync->layerbuf_spinlock, flags);
	list_for_each_entry_safe(node, _node_, &pbuf_sync->layerbuf_list, list_node) {
		if (node->timeline >= 2) {
			list_del(&node->list_node);
			list_add_tail(&node->list_node, &free_list);
		}
	}
	spin_unlock_irqrestore(&pbuf_sync->layerbuf_spinlock, flags);
	up(&pbuf_sync->layerbuf_sem);
	dpufe_layerbuf_unlock(dfd, &free_list);
}

static struct dpufe_fence *dpufe_sync_get_fence(struct dpufe_timeline *timeline,
		const char *fence_name, int val)
{
	struct dpufe_fence *fence = NULL;

	fence = dpufe_get_sync_fence(timeline, fence_name, NULL, val);
	if (IS_ERR_OR_NULL(fence)) {
		DPUFE_ERR("%s: cannot create fence\n", fence_name);
		return NULL;
	}

	return fence;
}

static struct dpufe_fence *dpufe_buf_create_fence(struct dpufe_data_type *dfd,
	struct dpufe_buf_sync *buf_sync_ctrl, u32 fence_type,
	int *fence_fd, int value)
{
	struct dpufe_fence *sync_fence = NULL;
	char fence_name[32]; /* fence name length */

	if (fence_type == DPU_RETIRE_FENCE) {
		snprintf(fence_name, sizeof(fence_name), "fb%u_retire", dfd->index);
		sync_fence = dpufe_sync_get_fence(
					buf_sync_ctrl->timeline_retire,
					fence_name, value);
	} else {
		snprintf(fence_name, sizeof(fence_name), "fb%u_release", dfd->index);
		sync_fence = dpufe_sync_get_fence(
					buf_sync_ctrl->timeline,
					fence_name, value);
	}

	if (IS_ERR_OR_NULL(sync_fence)) {
		DPUFE_ERR("%s: unable to retrieve release fence\n", fence_name);
		return sync_fence;
	}

	/* get fence fd */
	*fence_fd = dpufe_get_sync_fence_fd(sync_fence);
	if (*fence_fd < 0) {
		DPUFE_ERR("%s: get_unused_fd_flags failed error:0x%x\n", fence_name, *fence_fd);
		dpufe_put_sync_fence(sync_fence);
		sync_fence = NULL;
		return sync_fence;
	}

	return sync_fence;
}

int dpufe_buf_sync_create_fence(struct dpufe_data_type *dfd,
	int32_t *release_fence_fd, int32_t *retire_fence_fd)
{
	int ret = 0;
	int value;
	unsigned long flags = 0;
	struct dpufe_fence *release_fence = NULL;
	struct dpufe_fence *retire_fence = NULL;
	struct dpufe_buf_sync *buf_sync_ctrl = NULL;

	dpufe_check_and_return(!dfd, -EINVAL, "dfd is nullptr!\n");
	DPUFE_DEBUG("fb%d+\n", dfd->index);

	buf_sync_ctrl = &dfd->buf_sync_ctrl;

	spin_lock_irqsave(&buf_sync_ctrl->refresh_lock, flags);
	value = buf_sync_ctrl->timeline_max + buf_sync_ctrl->threshold + 1;
	spin_unlock_irqrestore(&buf_sync_ctrl->refresh_lock, flags);

	release_fence = dpufe_buf_create_fence(dfd, buf_sync_ctrl,
		DPU_RELEASE_FENCE, release_fence_fd, value);
	if (IS_ERR_OR_NULL(release_fence)) {
		DPUFE_ERR("unable to create release fence\n");
		ret = PTR_ERR(release_fence);
		goto release_fence_err;
	}

	value += buf_sync_ctrl->retire_threshold;
	retire_fence = dpufe_buf_create_fence(dfd, buf_sync_ctrl,
		DPU_RETIRE_FENCE, retire_fence_fd, value);
	if (IS_ERR_OR_NULL(retire_fence)) {
		DPUFE_ERR("unable to create retire fence\n");
		ret = PTR_ERR(retire_fence);
		goto retire_fence_err;
	}

	if (g_dpufe_debug_fence_timeline)
		DPUFE_INFO("dpufe%u frame_no[%u] create fence timeline_max[%d], %s[%d],"
			"%s[%d], timeline[%u], timeline_retire[%u], release_fence_fd[%d], retire_fence_fd[%d]!\n",
			dfd->index, dfd->ov_req.frame_no,
			buf_sync_ctrl->timeline_max,
			release_fence->name, release_fence->base.seqno,
			retire_fence->name, retire_fence->base.seqno,
			buf_sync_ctrl->timeline->value,
			buf_sync_ctrl->timeline_retire->value, *release_fence_fd, *retire_fence_fd);

	DPUFE_DEBUG("fb%d-\n", dfd->index);

	return ret;

retire_fence_err:
	put_unused_fd(*release_fence_fd);
	dpufe_put_sync_fence(release_fence);
release_fence_err:
	*retire_fence_fd = -1;
	*release_fence_fd = -1;

	return ret;
}

void dpufe_append_layer_list(struct dpufe_data_type *dfd, struct list_head *plock_list)
{
	struct dpufe_layerbuf *node = NULL;
	struct dpufe_layerbuf *_node_ = NULL;
	unsigned long flags = 0;

	dpufe_check_and_no_retval(!dfd, "dfd is nullptr!\n");
	dpufe_check_and_no_retval(!plock_list, "plock_list is nullptr!\n");

	spin_lock_irqsave(&(dfd->buf_sync_ctrl.layerbuf_spinlock), flags);
	list_for_each_entry_safe(node, _node_, plock_list, list_node) {
		list_del(&node->list_node);
		list_add_tail(&node->list_node, &(dfd->buf_sync_ctrl.layerbuf_list));
	}
	spin_unlock_irqrestore(&(dfd->buf_sync_ctrl.layerbuf_spinlock), flags);
}

void dpufe_layerbuf_flush(struct dpufe_data_type *dfd, struct list_head *plock_list)
{
	struct dpufe_layerbuf *node = NULL;
	struct dpufe_layerbuf *_node_ = NULL;
	unsigned long flags = 0;

	dpufe_check_and_no_retval(!dfd, "dfd is nullptr!\n");
	dpufe_check_and_no_retval(!plock_list, "plock_list is nullptr!\n");

	spin_lock_irqsave(&(dfd->buf_sync_ctrl.layerbuf_spinlock), flags);
	dfd->buf_sync_ctrl.layerbuf_flushed = true;
	list_for_each_entry_safe(node, _node_, plock_list, list_node) {
		list_del(&node->list_node);
		list_add_tail(&node->list_node, &(dfd->buf_sync_ctrl.layerbuf_list));
	}
	spin_unlock_irqrestore(&(dfd->buf_sync_ctrl.layerbuf_spinlock), flags);
}

void dpufe_reset_all_buf_fence(struct dpufe_data_type *dfd)
{
	struct dpufe_buf_sync *buf_sync_ctrl = NULL;
	unsigned long flags = 0;

	dpufe_check_and_no_retval(!dfd, "dfd is nullptr!\n");
	DPUFE_INFO("fb%u +\n", dfd->index);
	buf_sync_ctrl = &dfd->buf_sync_ctrl;
	dpufe_check_and_no_retval(!buf_sync_ctrl->timeline, "timeline is nullptr!\n");

	spin_lock_irqsave(&buf_sync_ctrl->refresh_lock, flags);

	dpufe_resync_timeline(buf_sync_ctrl->timeline);
	dpufe_resync_timeline(buf_sync_ctrl->timeline_retire);

	buf_sync_ctrl->timeline_max = (int)(buf_sync_ctrl->timeline->next_value) + 1;
	buf_sync_ctrl->refresh = 0;

	spin_unlock_irqrestore(&buf_sync_ctrl->refresh_lock, flags);
	if (g_dpufe_debug_fence_timeline)
		DPUFE_INFO("fb%u frame_no[%u] timeline_max[%d], TL[Nxt %u , Crnt %u]!\n",
			dfd->index, dfd->ov_req.frame_no, buf_sync_ctrl->timeline_max,
			buf_sync_ctrl->timeline->next_value, buf_sync_ctrl->timeline->value);
	DPUFE_INFO("fb%u -\n", dfd->index);
}

void dpufe_buf_sync_close_fence(int32_t *release_fence, int32_t *retire_fence)
{
	if (*release_fence >= 0) {
		ksys_close(*release_fence);
		*release_fence = -1;
	}

	if (*retire_fence >= 0) {
		ksys_close(*retire_fence);
		*retire_fence = -1;
	}
}

void dpufe_resync_timeline_fence(struct dpufe_data_type *dfd)
{
	struct dpufe_buf_sync *buf_sync_ctrl = NULL;
	int val = 0;
	unsigned long flags = 0;

	dpufe_check_and_no_retval(!dfd, "dfd is nullptr!\n");
	DPUFE_DEBUG("fb%u +\n", dfd->index);

	buf_sync_ctrl = &dfd->buf_sync_ctrl;
	if (g_dpufe_debug_fence_timeline)
		DPUFE_INFO("dpufe%u refresh=%d!\n", dfd->index, buf_sync_ctrl->refresh);

	spin_lock_irqsave(&buf_sync_ctrl->refresh_lock, flags);
	if (buf_sync_ctrl->refresh == 0)
		buf_sync_ctrl->refresh = 1;

	val = buf_sync_ctrl->refresh;
	dpufe_inc_timeline(buf_sync_ctrl->timeline, val);
	dpufe_inc_timeline(buf_sync_ctrl->timeline_retire, val);

	buf_sync_ctrl->timeline_max += val;
	buf_sync_ctrl->refresh = 0;
#if defined(CONFIG_ASYNCHRONOUS_PLAY)
	dfd->asynchronous_play_flag = 1;
#endif
	if (g_dpufe_debug_fence_timeline)
		DPUFE_INFO("dpufe%u frame_no=%u timeline_max=%d, timeline=%d, timeline_retire=%d!\n",
			dfd->index, dfd->ov_req.frame_no,
			buf_sync_ctrl->timeline_max,
			buf_sync_ctrl->timeline->value,
			buf_sync_ctrl->timeline_retire->value);
	spin_unlock_irqrestore(&buf_sync_ctrl->refresh_lock, flags);
}

void dpufe_update_frame_refresh_state(struct dpufe_data_type *dfd)
{
	unsigned long flags = 0;

	dpufe_check_and_no_retval(!dfd, "dfd is nullptr!\n");

	DPUFE_DEBUG("fb%u +\n", dfd->index);

	spin_lock_irqsave(&dfd->buf_sync_ctrl.refresh_lock, flags);
	dfd->buf_sync_ctrl.refresh++;
	dfd->buf_sync_ctrl.timeline_max++;
	spin_unlock_irqrestore(&dfd->buf_sync_ctrl.refresh_lock, flags);

	spin_lock_irqsave(&(dfd->buf_sync_ctrl.layerbuf_spinlock), flags);
	dfd->buf_sync_ctrl.layerbuf_flushed = true;
	spin_unlock_irqrestore(&(dfd->buf_sync_ctrl.layerbuf_spinlock), flags);

	DPUFE_DEBUG("fb%u -\n", dfd->index);
}

void dpufe_buf_sync_signal(struct dpufe_data_type *dfd)
{
	struct dpufe_layerbuf *node = NULL;
	struct dpufe_layerbuf *_node_ = NULL;
	struct dpufe_buf_sync *buf_sync_ctrl = NULL;
	int val = 0;

	dpufe_check_and_no_retval(!dfd, "dfd is nullptr!\n");
	DPUFE_DEBUG("fb%u +\n", dfd->index);

	buf_sync_ctrl = &dfd->buf_sync_ctrl;
	if (dfd->index == AUXILIARY_PANEL_IDX) {
		queue_work(buf_sync_ctrl->free_layerbuf_queue, &(buf_sync_ctrl->free_layerbuf_work));
		return;
	}
	if (g_dpufe_debug_fence_timeline)
		DPUFE_INFO("dpufe%u refresh=%d!\n", dfd->index, buf_sync_ctrl->refresh);

	spin_lock(&buf_sync_ctrl->refresh_lock);
	if (buf_sync_ctrl->refresh) {
		val = buf_sync_ctrl->refresh;
		dpufe_inc_timeline(buf_sync_ctrl->timeline, val);
		dpufe_inc_timeline(buf_sync_ctrl->timeline_retire, val);

#if defined(CONFIG_ASYNCHRONOUS_PLAY)
		dfd->asynchronous_play_flag = 1;
		wake_up_interruptible_all(&dfd->asynchronous_play_wq);
#endif
		buf_sync_ctrl->refresh = 0;

		if (g_dpufe_debug_fence_timeline)
			DPUFE_INFO("dpufe%u frame_no=%u timeline_max=%d, timeline=%d, timeline_retire=%d!\n",
				dfd->index, dfd->ov_req.frame_no,
				buf_sync_ctrl->timeline_max,
				buf_sync_ctrl->timeline->value,
				buf_sync_ctrl->timeline_retire->value);
	}
	spin_unlock(&buf_sync_ctrl->refresh_lock);

	spin_lock(&(buf_sync_ctrl->layerbuf_spinlock));
	list_for_each_entry_safe(node, _node_, &(buf_sync_ctrl->layerbuf_list), list_node) {
		if (buf_sync_ctrl->layerbuf_flushed)
			node->timeline++;
	}
	buf_sync_ctrl->layerbuf_flushed = false;
	spin_unlock(&(buf_sync_ctrl->layerbuf_spinlock));

	queue_work(buf_sync_ctrl->free_layerbuf_queue, &(buf_sync_ctrl->free_layerbuf_work));
	DPUFE_DEBUG("fb%u -\n", dfd->index);
}

void dpufe_buf_sync_register(struct dpufe_data_type *dfd)
{
	char tmp_name[256] = {0}; /* release fence time line name */
	struct dpufe_buf_sync *buf_sync_ctrl = NULL;

	dpufe_check_and_no_retval(!dfd, "dfd is nullptr!\n");

	DPUFE_DEBUG("dfd%u, +\n", dfd->index);

	buf_sync_ctrl = &dfd->buf_sync_ctrl;
	buf_sync_ctrl->timeline = NULL;
	buf_sync_ctrl->timeline_retire = NULL;

	spin_lock_init(&buf_sync_ctrl->refresh_lock);
	spin_lock(&buf_sync_ctrl->refresh_lock);
	buf_sync_ctrl->refresh = 0;
	buf_sync_ctrl->timeline_max = 1;
	buf_sync_ctrl->threshold = 0;
	buf_sync_ctrl->retire_threshold = 0;
	spin_unlock(&buf_sync_ctrl->refresh_lock);

	if (dfd->index != AUXILIARY_PANEL_IDX) {
		snprintf(tmp_name, sizeof(tmp_name), "dfd%u", dfd->index);
		buf_sync_ctrl->timeline = dpufe_create_timeline(tmp_name);
		dpufe_check_and_no_retval(!buf_sync_ctrl->timeline, "cannot create release fence time line\n");

		snprintf(tmp_name, sizeof(tmp_name), "dfd%u_retire", dfd->index);
		buf_sync_ctrl->timeline_retire = dpufe_create_timeline(tmp_name);
		dpufe_check_and_no_retval(!buf_sync_ctrl->timeline_retire, "cannot create retire fence time line\n");
	}

	spin_lock_init(&(buf_sync_ctrl->layerbuf_spinlock));
	INIT_LIST_HEAD(&(buf_sync_ctrl->layerbuf_list));
	spin_lock(&(buf_sync_ctrl->layerbuf_spinlock));
	buf_sync_ctrl->layerbuf_flushed = false;
	spin_unlock(&(buf_sync_ctrl->layerbuf_spinlock));
	sema_init(&(buf_sync_ctrl->layerbuf_sem), 1);

	(void)snprintf(tmp_name, sizeof(tmp_name), DPUFE_LAYERBUF_FREE, dfd->index);
	INIT_WORK(&(buf_sync_ctrl->free_layerbuf_work), dpufe_layerbuf_unlock_work);
	buf_sync_ctrl->free_layerbuf_queue = create_singlethread_workqueue(tmp_name);
	if (!buf_sync_ctrl->free_layerbuf_queue) {
		DPUFE_ERR("failed to create free_layerbuf_queue!\n");
		return;
	}

	DPUFE_DEBUG("dfd%u, -\n", dfd->index);
}

void dpufe_buf_sync_unregister(struct dpufe_data_type *dfd)
{
	struct dpufe_buf_sync *buf_sync_ctrl = NULL;

	dpufe_check_and_no_retval(!dfd, "dfd is nullptr!\n");

	DPUFE_DEBUG("dfd%u, +\n", dfd->index);

	buf_sync_ctrl = &dfd->buf_sync_ctrl;

	if (buf_sync_ctrl->timeline) {
		dpufe_destroy_timeline(buf_sync_ctrl->timeline);
		buf_sync_ctrl->timeline = NULL;
	}

	if (buf_sync_ctrl->timeline_retire) {
		dpufe_destroy_timeline(buf_sync_ctrl->timeline_retire);
		buf_sync_ctrl->timeline_retire = NULL;
	}

	if (buf_sync_ctrl->free_layerbuf_queue) {
		destroy_workqueue(buf_sync_ctrl->free_layerbuf_queue);
		buf_sync_ctrl->free_layerbuf_queue = NULL;
	}

	DPUFE_DEBUG("dfd%d, -\n", dfd->index);
}
