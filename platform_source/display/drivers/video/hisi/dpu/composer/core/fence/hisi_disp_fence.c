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
#include <linux/types.h>
#include <linux/dma-fence.h>
#include <linux/file.h>
#include <linux/sync_file.h>
#include <linux/spinlock.h>
#include "timeline/hisi_disp_timeline.h"
#include "hisi_disp_fence.h"
#include "hisi_disp_debug.h"

#define HISI_DISP_FENCE_DRIVER_NAME "hisi_disp_fence"

static const char *hisi_fence_get_driver_name(struct dma_fence *fence)
{
	return HISI_DISP_FENCE_DRIVER_NAME;
}

static const char *hisi_fence_get_timeline_name(struct dma_fence *fence)
{
	struct hisi_disp_timeline *timeline = hisi_fence_get_timeline(fence);

	if (timeline)
		return timeline->name;

	return NULL;
}

// gt: greater than
static bool hisi_fence_is_signaled_gt(struct dma_fence *fence)
{
	struct hisi_disp_timeline *timeline = hisi_fence_get_timeline(fence);

	if (!timeline)
		return false;

	return ((s32)(timeline->pt_value - fence->seqno)) > 0;
}

// ge: greater than or equal to
static bool hisi_fence_is_signaled_ge(struct dma_fence *fence)
{
	struct hisi_disp_timeline *timeline = hisi_fence_get_timeline(fence);

	if (!timeline)
		return false;

	return ((s32)(timeline->pt_value - fence->seqno)) >= 0;
}

static void hisi_fence_release(struct dma_fence *fence)
{
	kfree_rcu(fence, rcu);
}

static void hisi_fence_value_str(struct dma_fence *fence, char *str, int size)
{
	snprintf(str, size, "%u", fence->seqno);
}

static void hisi_fence_timeline_value_str(struct dma_fence *fence, char *str,
		int size)
{
	struct hisi_disp_timeline *timeline = hisi_fence_get_timeline(fence);

	if (timeline)
		snprintf(str, size, "%u", timeline->pt_value);
}

static void hisi_fence_dump(struct hisi_disp_fence *fence)
{
	// TODO:
}

static bool hisi_fence_enable_signaling(struct dma_fence *fence)
{
	return true;
}

static struct dma_fence_ops g_hisi_release_fence_ops = {
	.get_driver_name = hisi_fence_get_driver_name,
	.get_timeline_name = hisi_fence_get_timeline_name,
	.enable_signaling = hisi_fence_enable_signaling,
	.signaled = hisi_fence_is_signaled_gt,
	.wait = dma_fence_default_wait,
	.release = hisi_fence_release,
	.fence_value_str = hisi_fence_value_str,
	.timeline_value_str = hisi_fence_timeline_value_str,
};

static struct dma_fence_ops g_hisi_retire_fence_ops = {
	.get_driver_name = hisi_fence_get_driver_name,
	.get_timeline_name = hisi_fence_get_timeline_name,
	.enable_signaling = hisi_fence_enable_signaling,
	.signaled = hisi_fence_is_signaled_ge,
	.wait = dma_fence_default_wait,
	.release = hisi_fence_release,
	.fence_value_str = hisi_fence_value_str,
	.timeline_value_str = hisi_fence_timeline_value_str,
};


static const char *hisi_fence_listener_get_name(struct hisi_timeline_listener *listener)
{
	struct hisi_disp_fence *fence = (struct hisi_disp_fence *)listener->listener_data;

	if (!fence || !fence->base.ops->get_driver_name)
		return NULL;

	return fence->base.ops->get_driver_name(&fence->base);
}

static bool hisi_fence_listener_is_signaled(struct hisi_timeline_listener *listener, uint32_t tl_val)
{
	struct hisi_disp_fence *fence = (struct hisi_disp_fence *)listener->listener_data;

	if (!fence)
		return false;

	return dma_fence_is_signaled_locked(&fence->base);
}

void hisi_fence_listener_release(struct hisi_timeline_listener *listener)
{
	struct hisi_disp_fence *fence = (struct hisi_disp_fence *)listener->listener_data;

	if (!fence)
		return;

	dma_fence_put(&fence->base);
}

static struct hisi_timeline_listener_ops g_hisi_listener_fence_ops = {
	.get_listener_name = hisi_fence_listener_get_name,
	.enable_signaling = NULL,
	.disable_signaling = NULL,
	.is_signaled = hisi_fence_listener_is_signaled,
	.handle_signal = NULL,
	.release = hisi_fence_listener_release,
};

static u64 g_fence_context = 0;
static int hisi_fence_get_sync_fd(struct dma_fence *fence)
{
	int fd;
	struct sync_file *sync_file = NULL;

	fd = get_unused_fd_flags(O_CLOEXEC);
	if (fd < 0) {
		disp_pr_err("fail to get unused fd\n");
		return fd;
	}

	sync_file = sync_file_create(fence);
	if (!sync_file) {
		put_unused_fd(fd);
		disp_pr_err("failed to create sync file\n");
		return -ENOMEM;
	}

	fd_install(fd, sync_file->file);
	return fd;
}

static void hisi_fence_init(struct hisi_disp_fence *fence, struct hisi_disp_timeline *tl, enum disp_fence_type type, uint64_t pt_value)
{
	struct dma_fence_ops *fence_ops = NULL;

	if (g_fence_context == 0)
		g_fence_context = dma_fence_context_alloc(1);

	fence->dump = hisi_fence_dump;
	fence->fence_type = type;
	fence->timeline = tl;
	disp_pr_info(" type:%d ", type);
	if (type == DISP_RETIRE_FENCE)
		fence_ops = &g_hisi_retire_fence_ops;
	else
		fence_ops = &g_hisi_release_fence_ops;

	dma_fence_init(&fence->base, fence_ops, &tl->value_lock, g_fence_context, pt_value);
}

int hisi_disp_create_fence(struct hisi_disp_timeline *tl, enum disp_fence_type type)
{
	struct hisi_disp_fence *fence;
	uint64_t pt_value;
	struct hisi_timeline_listener *listener = NULL;
	int fd;

	fence = kzalloc(sizeof(*fence), GFP_KERNEL);
	if (!fence)
		return -1;

	pt_value = hisi_timeline_get_pt_value(tl);
	hisi_fence_init(fence, tl, type, pt_value + 1);

	fd = hisi_fence_get_sync_fd(&fence->base);
	if (fd < 0)
		goto alloc_fence_fail;

	listener = hisi_timeline_alloc_listener(&g_hisi_listener_fence_ops, fence, pt_value);
	if (!listener)
		goto alloc_fence_listener_fail;

	hisi_timeline_add_listener(tl, listener);
	return fd;

alloc_fence_listener_fail:
	put_unused_fd(fd);
alloc_fence_fail:
	kfree(fence);
	return -1;
}


