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

#include <linux/file.h>
#include <linux/sync_file.h>

#include "dkmd_release_fence.h"
#include "dkmd_fence_utils.h"

static const char *release_fence_get_driver_name(struct dma_fence *fence)
{
	struct dkmd_release_fence *dkmd_fence = container_of(fence, struct dkmd_release_fence, base);

	return dkmd_fence->name;
}

static const char *release_fence_get_timeline_name(struct dma_fence *fence)
{
	struct dkmd_timeline *timeline = release_fence_get_timeline(fence);

	return timeline->name;
}

static bool release_fence_enable_signaling(struct dma_fence *fence)
{
	return true;
}

static bool release_fence_is_signaled_gt(struct dma_fence *fence)
{
	struct dkmd_timeline *timeline = release_fence_get_timeline(fence);
	bool ret = ((s32)(timeline->pt_value - fence->seqno)) > 0;

	if (g_debug_fence_timeline)
		dpu_pr_info("fence signal=%d at timeline:%s timeline_value=%llu fence->seqno:%llu",
			ret, timeline->name, timeline->pt_value, fence->seqno);

	return ret;
}

static void release_fence_release(struct dma_fence *fence)
{
	if (g_debug_fence_timeline)
		dpu_pr_info("timeline %s release for fence %s\n",
			fence->ops->get_timeline_name(fence),
			fence->ops->get_driver_name(fence));

	kfree_rcu(fence, rcu);
}

static void release_fence_value_str(struct dma_fence *fence, char *str, int32_t size)
{
	snprintf(str, size, "%u", fence->seqno);
}

static void release_fence_timeline_value_str(struct dma_fence *fence, char *str, int32_t size)
{
	struct dkmd_timeline *timeline = release_fence_get_timeline(fence);

	snprintf(str, size, "%u", timeline->pt_value);
}

static struct dma_fence_ops g_release_fence_ops = {
	.get_driver_name = release_fence_get_driver_name,
	.get_timeline_name = release_fence_get_timeline_name,
	.enable_signaling = release_fence_enable_signaling,
	.signaled = release_fence_is_signaled_gt,
	.wait = dma_fence_default_wait,
	.release = release_fence_release,
	.fence_value_str = release_fence_value_str,
	.timeline_value_str = release_fence_timeline_value_str,
};

static void release_fence_init(struct dkmd_release_fence *fence,
	struct dkmd_timeline *timeline, uint64_t pt_value)
{
	fence->timeline = timeline;

	dma_fence_init(&fence->base, &g_release_fence_ops,
		 &timeline->value_lock, dma_fence_context_alloc(1), pt_value);

	snprintf(fence->name, DKMD_SYNC_NAME_SIZE, "dkmd_release_fence_%u", pt_value);
}

int32_t dkmd_release_fence_create(struct dkmd_timeline *timeline)
{
	int32_t fd;
	uint64_t next_value;
	struct dkmd_release_fence *fence = NULL;

	if (!timeline) {
		dpu_pr_err("timeline input error is null");
		return -1;
	}

	fence = kzalloc(sizeof(*fence), GFP_KERNEL);
	if (!fence) {
		dpu_pr_err("kzalloc fence fail");
		return -1;
	}
	next_value = dkmd_timeline_get_next_value(timeline) + 1;
	release_fence_init(fence, timeline, next_value);

	fd = dkmd_fence_get_fence_fd(&fence->base);
	if (fd < 0)
		goto alloc_fence_fail;

	if (g_debug_fence_timeline)
		dpu_pr_info("fence created at val=%llu timeline:%s timeline_value=%llu",
			next_value, timeline->name, timeline->pt_value);

	return fd;

alloc_fence_fail:
	kfree(fence);
	return -1;
}
EXPORT_SYMBOL(dkmd_release_fence_create);