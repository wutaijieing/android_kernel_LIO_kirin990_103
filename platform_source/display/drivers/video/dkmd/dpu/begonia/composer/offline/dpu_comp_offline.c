/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "dkmd_log.h"

#include <linux/dma-buf.h>
#include <linux/delay.h>
#include <linux/syscalls.h>
#include "dpu_comp_mgr.h"
#include "dpu_comp_offline.h"
#include "dkmd_acquire_fence.h"
#include "dpu_comp_smmu.h"
#include "dpu_dacc.h"
#include "cmdlist_interface.h"
#include "dpu_comp_abnormal_handle.h"
#include "dvfs.h"

static void composer_offline_postprocess(struct comp_offline_present *present, bool timeout)
{
	uint32_t i;
	struct dkmd_dma_buf *buffer = NULL;
	struct disp_frame *frame = &present->frame.in_frame;
	struct composer *comp = NULL;

	dpu_comp_smmu_offline_tlb_flush(frame->scene_id, frame->frame_index);

	if (present->acquire_fence)
		dkmd_acquire_fence_signal_release(present->acquire_fence->fence);

	dkmd_cmdlist_release_locked(frame->scene_id, frame->cmdlist_id);

	for (i = 0; i < frame->layer_count; ++i) {
		buffer = &(present->frame.layer_dma_buf[i]);
		if (buffer) {
			dpu_pr_debug("release %s", buffer->name);
			dma_buf_put(buffer->buf_handle);
			buffer->share_fd = -1;
			buffer->buf_handle = NULL;
		}
	}
	present->wb_done = 0;
	present->wb_has_presented = 0;

	comp = &present->dpu_comp->comp;
	down(&comp->blank_sem);

	if (unlikely(timeout))
		composer_manager_reset_hardware(present->dpu_comp);

	/* if there is only offline invokes (power)on interface, power_on will be false
	 * thus, we can invoke (power)off here cause only offline use power right now
	 */
	if (!comp->power_on)
		comp->off(comp);

	up(&comp->blank_sem);
}

static void composer_offline_finish_work(struct kthread_work *work)
{
	int32_t ret = 0;
	uint64_t frame_end_tv;
	uint32_t timeout_interval = ASIC_EVENT_TIMEOUT_MS;
	struct comp_offline_present *present = NULL;
	char __iomem *dpu_base = NULL;

	present = container_of(work, struct comp_offline_present, compose_work);
	dpu_check_and_no_retval(!present, err, "present is null");

	dpu_trace_ts_begin(&frame_end_tv);
	dpu_base = present->dpu_comp->comp_mgr->dpu_base;

	while (true) {
		ret = (int32_t)wait_event_interruptible_timeout(present->wb_wq,
			(present->wb_done == 1), /* wb type status is true */
			msecs_to_jiffies(timeout_interval));
		if (ret == -ERESTARTSYS)
			continue;

		if (ret <= 0) {
			dpu_pr_warn("offline compose timeout!!");
			dpu_comp_abnormal_debug_dump(dpu_base, present->frame.in_frame.scene_id);
			if (g_debug_dpu_clear_enable) {
				if (dpu_dacc_handle_clear(dpu_base, present->frame.in_frame.scene_id) != 0) {
					dpu_pr_warn("offline clear failed, reset hardware");
					composer_offline_postprocess(present, true);
					dpu_trace_ts_end(&frame_end_tv, "offline compose failed");
					return;
				}
			}
		}
		break;
	}

	composer_offline_postprocess(present, false);
	dpu_trace_ts_end(&frame_end_tv, "offline compose present");
}

static void composer_offline_preprocess(struct comp_offline_present *present)
{
	uint32_t i;
	struct disp_layer *layer = NULL;
	struct disp_frame *frame = &present->frame.in_frame;

	dpu_dvfs_intra_frame_vote(&present->frame.in_frame.dvfs_info);

	/* wait layer acquired fence and lock dma buf */
	for (i = 0; i < frame->layer_count; ++i) {
		layer = &frame->layer[i];
		if (layer->acquired_fence > 0) {
			dkmd_acquire_fence_wait_fd(layer->acquired_fence, ACQUIRE_FENCE_TIMEOUT_MSEC);
			layer->acquired_fence = -1;
		}

		if (layer->share_fd > 0) {
			present->frame.layer_dma_buf[i].share_fd = layer->share_fd;
			present->frame.layer_dma_buf[i].buf_handle = dma_buf_get(layer->share_fd);
			snprintf(present->frame.layer_dma_buf[i].name, DKMD_SYNC_NAME_SIZE, "buf_share_fd_%u", layer->share_fd);
		}
	}
}

static int32_t composer_offline_overlay(struct dpu_composer *dpu_comp, struct disp_frame *frame)
{
	uint32_t scene_id = (uint32_t)frame->scene_id;
	struct composer_scene *scene = dpu_comp->comp_mgr->scene[scene_id];
	struct comp_offline_present *present = (struct comp_offline_present *)dpu_comp->present_data;

	if (unlikely(scene_id > DPU_SCENE_OFFLINE_2 || scene_id < DPU_SCENE_ONLINE_3)) {
		dpu_pr_err("invalid scene_id=%u", scene_id);
		return -1;
	}

	if (unlikely(!scene)) {
		dpu_pr_err("unsupport scene_id=%u", scene_id);
		return -1;
	}
	frame->out_present_fence = -1;
	present->frame.in_frame = *frame;
	present->wb_has_presented = 1;
	composer_offline_preprocess(present);

	/* commit to hardware, but sometimes hardware is handling exceptions,
	 * so need acquire comp_mgr power_sem
	 */
	down(&dpu_comp->comp_mgr->power_sem);
	if (likely(scene->present != NULL))
		scene->present(scene, frame->cmdlist_id);
	up(&dpu_comp->comp_mgr->power_sem);

	frame->out_present_fence = dpu_comp->create_fence(dpu_comp);
	kthread_queue_work(&dpu_comp->handle_worker, &present->compose_work);

	/* NOTICE: offline HAL does not support asynchronous, so wait fence here */
	if (frame->out_present_fence > 0) {
		dkmd_acquire_fence_wait_fd(frame->out_present_fence, ACQUIRE_FENCE_TIMEOUT_MSEC);
		ksys_close(frame->out_present_fence);
		frame->out_present_fence = -1;
		kfree(present->acquire_fence);
		present->acquire_fence = NULL;
	}

	return 0;
}

static int32_t composer_offline_create_fence(struct dpu_composer *dpu_comp)
{
	int32_t out_present_fence = -1;
	struct dpu_comp_fence *acquire_fence = NULL;
	struct comp_offline_present *present = (struct comp_offline_present *)dpu_comp->present_data;

	if (present->acquire_fence != NULL) {
		dma_fence_put(present->acquire_fence->fence);
		kfree(present->acquire_fence);
		present->acquire_fence = NULL;
		dpu_pr_info("prev offline compose have not finished");
		return out_present_fence;
	}

	acquire_fence = (struct dpu_comp_fence *)kzalloc(sizeof(*acquire_fence), GFP_KERNEL);
	if (!acquire_fence) {
		dpu_pr_info("alloc acquire_fence failed");
		return -1;
	}
	spin_lock_init(&acquire_fence->lock);
	acquire_fence->fence = kzalloc(sizeof(struct dma_fence), GFP_KERNEL);
	if (acquire_fence->fence != NULL)
		out_present_fence = dkmd_acquire_fence_create_fd(acquire_fence->fence, &acquire_fence->lock,
			present->frame.in_frame.scene_id);

	present->acquire_fence = acquire_fence;

	return out_present_fence;
}

static int32_t dpu_offline_isr_notify(struct notifier_block *self, unsigned long action, void *data)
{
	struct dpu_composer *dpu_comp = (struct dpu_composer *)data;
	struct comp_offline_present *present = (struct comp_offline_present *)dpu_comp->present_data;

	if (present->wb_has_presented == 1) {
		present->wb_done = 1;
		wake_up_interruptible_all(&present->wb_wq);
	}

	return 0;
}

static struct notifier_block offline_isr_notifier = {
	.notifier_call = dpu_offline_isr_notify,
};

void composer_offline_setup(struct dpu_composer *dpu_comp, struct comp_offline_present *present)
{
	present->dpu_comp = dpu_comp;

	sema_init(&(present->wb_sem), 1);
	init_waitqueue_head(&(present->wb_wq));
	kthread_init_work(&present->compose_work, composer_offline_finish_work);

	pipeline_next_ops_handle(dpu_comp->conn_info->conn_device,
		dpu_comp->conn_info, SETUP_ISR, (void *)&dpu_comp->isr_ctrl);
	dkmd_isr_setup(&dpu_comp->isr_ctrl);
	list_add_tail(&dpu_comp->isr_ctrl.list_node, &dpu_comp->comp_mgr->isr_list);
	dkmd_isr_register_listener(&dpu_comp->isr_ctrl, &offline_isr_notifier, WCH_FRM_END_INTS, dpu_comp);

	dpu_comp_active_vsync(dpu_comp);

	dpu_comp->overlay = composer_offline_overlay;
	dpu_comp->create_fence = composer_offline_create_fence;
}

void composer_offline_release(struct dpu_composer *dpu_comp, struct comp_offline_present *present)
{
	dpu_comp->isr_ctrl.handle_func(&dpu_comp->isr_ctrl, DKMD_ISR_RELEASE);
	list_del(&dpu_comp->isr_ctrl.list_node);
	dpu_comp_deactive_vsync(dpu_comp);
}
