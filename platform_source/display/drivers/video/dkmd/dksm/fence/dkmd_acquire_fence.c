/**
 * @file fence_acquire_sync.c
 * @brief Provide fence synchronization mechanism
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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
#include "dkmd_log.h"

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/slab.h>
#include <linux/sync_file.h>

#include "dkmd_acquire_fence.h"
#include "dkmd_fence_utils.h"

#define ACQUIRE_SYNC_FENCE_DRIVER_NAME "acquire_sync"

static const char *acquire_sync_fence_get_driver_name(struct dma_fence *fence)
{
	return ACQUIRE_SYNC_FENCE_DRIVER_NAME;
}

static bool acquire_sync_fence_enable_signaling(struct dma_fence *fence)
{
	return true;
}

static void acquire_sync_fence_release(struct dma_fence *fence)
{
	dpu_pr_debug("release fence %s seqno:%d\n",
		fence->ops->get_driver_name(fence),
		fence->seqno);

	kfree_rcu(fence, rcu);
}

static bool acquire_sync_fence_signaled(struct dma_fence *fence)
{
	dpu_pr_debug("drv:%s timeline:%s seqno:%d signaled",
		fence->ops->get_driver_name(fence),
		fence->ops->get_timeline_name(fence),
		fence->seqno);
	return true;
}

static void acquire_sync_fence_value_str(struct dma_fence *fence, char *str, int32_t size)
{
	snprintf(str, size, "%u", fence->seqno);
}

static struct dma_fence_ops acquire_sync_ops = {
	.get_driver_name = acquire_sync_fence_get_driver_name,
	.get_timeline_name = acquire_sync_fence_get_driver_name,
	.enable_signaling = acquire_sync_fence_enable_signaling,
	.signaled = acquire_sync_fence_signaled,
	.wait = dma_fence_default_wait,
	.release = acquire_sync_fence_release,
	.fence_value_str = acquire_sync_fence_value_str,
	.timeline_value_str = acquire_sync_fence_value_str,
};

int32_t dkmd_acquire_fence_create_fd(struct dma_fence *fence, spinlock_t *lock, int32_t value)
{
	int32_t fence_fd = -1;

	if (!fence)
		return -1;

	dma_fence_init(fence, &acquire_sync_ops, lock, dma_fence_context_alloc(1), value);

	fence_fd = dkmd_fence_get_fence_fd(fence);
	if (fence_fd < 0) {
		dpu_pr_err("get_unused_fd_flags failed error:%d\n", fence_fd);
		dma_fence_put(fence);
		return -1;
	}

	return fence_fd;
}
EXPORT_SYMBOL(dkmd_acquire_fence_create_fd);

struct dma_fence *dkmd_alloc_acquire_fence(spinlock_t *lock, int32_t value)
{
	struct dma_fence *fence = kzalloc(sizeof(struct dma_fence), GFP_KERNEL);
	if (!fence)
		return NULL;

	dma_fence_init(fence, &acquire_sync_ops, lock, dma_fence_context_alloc(1), value);

	return fence;
}
EXPORT_SYMBOL(dkmd_alloc_acquire_fence);

int32_t dkmd_acquire_fence_wait(struct dma_fence *fence, long timeout)
{
	int32_t rc = 0;

	if (!fence)
		return rc;

	rc = dma_fence_wait_timeout(fence, false, msecs_to_jiffies(timeout));
	if (rc > 0) {
		dpu_pr_debug("signaled drv:%s timeline:%s seqno:%d\n",
			fence->ops->get_driver_name(fence),
			fence->ops->get_timeline_name(fence),
			fence->seqno);
		rc = 0;
	} else if (rc == 0) {
		dpu_pr_err("timeout drv:%s timeline:%s seqno:%d.\n",
			fence->ops->get_driver_name(fence),
			fence->ops->get_timeline_name(fence),
			fence->seqno);
		rc = -ETIMEDOUT;
	}

	return rc;
}
EXPORT_SYMBOL(dkmd_acquire_fence_wait);

/**
 * @brief This function is used to wait effective fence fd,
 *  such as the GPU graphics buffer
 *
 * @param fence_fd Must be the same thread of fd
 * @param timeout
 * @return int32_t
 */
int32_t dkmd_acquire_fence_wait_fd(int32_t fence_fd, long timeout)
{
	int32_t rc = 0;
	struct dma_fence *fence = NULL;

	if (fence_fd < 0)
		return 0;

	fence = sync_file_get_fence(fence_fd);
	if (!fence) {
		dpu_pr_err("fence_fd=%d sync_file_get_fence failed!\n", fence_fd);
		return -EINVAL;
	}

	dpu_pr_debug("wait for fence_fd=%d", fence_fd);
	rc = dkmd_acquire_fence_wait(fence, timeout);
	if (rc != 0)
		dpu_pr_err("wait for fence_fd=%d fail", fence_fd);

	dma_fence_put(fence);
	return rc;
}
EXPORT_SYMBOL(dkmd_acquire_fence_wait_fd);

void dkmd_acquire_fence_signal_release(struct dma_fence *fence)
{
	if (!fence)
		return;

	if (!dma_fence_is_signaled_locked(fence))
		dpu_pr_info("no signal drv:%s timeline:%s seqno:%d, but release!\n",
			fence->ops->get_driver_name(fence),
			fence->ops->get_timeline_name(fence),
			fence->seqno);

	/* ref match fence_create_init */
	dma_fence_put(fence);
}
EXPORT_SYMBOL(dkmd_acquire_fence_signal_release);

int32_t dkmd_acquire_fence_signal(int32_t fence_fd)
{
	int32_t rc = 0;
	struct dma_fence *fence = NULL;

	if (fence_fd < 0)
		return 0;

	fence = sync_file_get_fence(fence_fd);
	if (!fence) {
		dpu_pr_err("fence_fd=%d, sync_file_get_fence failed!\n", fence_fd);
		return -EINVAL;
	}

	if (dma_fence_is_signaled_locked(fence)) {
		/* ref match fence_create_init */
		dma_fence_put(fence);
	} else {
		dpu_pr_info("signal err drv:%s timeline:%s seqno:%d\n",
			fence->ops->get_driver_name(fence),
			fence->ops->get_timeline_name(fence),
			fence->seqno);
		rc = -1;
	}
	dma_fence_put(fence);

	return rc;
}
EXPORT_SYMBOL(dkmd_acquire_fence_signal);

MODULE_LICENSE("GPL");