/* Copyright (c) Huawei Technologies Co., Ltd. 2015-2021. All rights reserved.
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
#ifndef DPU_SYNC_H
#define DPU_SYNC_H

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/version.h>
#include <linux/dma-fence.h>

#define DPU_SYNC_NAME_SIZE 64

enum {
	DPU_RELEASE_FENCE = 0,
	DPU_RETIRE_FENCE,
};

/**
 * struct dpu_fence - sync fence context
 * @base: base sync fence object
 * @name: name of this sync fence
 * @fence_list: linked list of outstanding sync fence
 */
struct dpu_fence {
	struct dma_fence base;
	char name[DPU_SYNC_NAME_SIZE];
	struct list_head fence_list;
};

/**
 * struct dpu_timeline - sync timeline context
 * @kref: reference count of timeline
 * @lock: serialization lock for timeline and fence update
 * @name: name of timeline
 * @fence_name: fence name prefix
 * @next_value: next commit sequence number
 * @value: current retired sequence number
 * @context: fence context identifier
 * @fence_list_head: linked list of outstanding/active sync fence (unsignaled/errored)
 */

struct dpu_timeline {
	struct kref kref;
	spinlock_t lock;
	spinlock_t list_lock;
	char name[DPU_SYNC_NAME_SIZE];
	u32 next_value;
	u32 value;
	u64 context;
	struct list_head fence_list_head;
};

/*
 * dpu_create_timeline - Create timeline object with the given name
 * @name: Pointer to name character string.
 */
struct dpu_timeline *dpu_create_timeline(const char *name);

/*
 * dpu_destroy_timeline - Destroy the given timeline object
 * @tl: Pointer to timeline object.
 */
void dpu_destroy_timeline(struct dpu_timeline *tl);

/*
 * dpu_get_sync_fence - Create fence object from the given timeline
 * @tl: Pointer to timeline object
 * @timestamp: Pointer to timestamp of the returned fence. Null if not required.
 * Return: pointer fence created on give time line.
 */
struct dpu_fence *dpu_get_sync_fence(
		struct dpu_timeline *tl, const char *fence_name,
		u32 *timestamp, int value);

/*
 * dpu_put_sync_fence - Destroy given fence object
 * @fence: Pointer to fence object.
 */
void dpu_put_sync_fence(struct dpu_fence *fence);

/*
 * dpu_wait_sync_fence - Wait until fence signal or timeout
 * @fence: Pointer to fence object.
 * @timeout: maximum wait time, in msec, for fence to signal.
 */
int dpu_wait_sync_fence(struct dpu_fence *fence,
		long timeout);

/*
 * dpu_get_fd_sync_fence - Get fence object of given file descriptor
 * @fd: File description of fence object.
 */
struct dpu_fence *dpu_get_fd_sync_fence(int fd);

/*
 * dpu_get_sync_fence_fd - Get file descriptor of given fence object
 * @fence: Pointer to fence object.
 * Return: File descriptor on success, or error code on error
 */
int dpu_get_sync_fence_fd(struct dpu_fence *fence);

/*
 * dpu_get_sync_fence_name - get given fence object name
 * @fence: Pointer to fence object.
 * Return: fence name
 */
const char *dpu_get_sync_fence_name(struct dpu_fence *fence);

/*
 * dpu_inc_timeline - Increment timeline by given amount
 * @tl: Pointer to timeline object.
 * @increment: the amount to increase the timeline by.
 */
int dpu_inc_timeline(struct dpu_timeline *tl, int increment);

/*
 * dpu_resync_timeline - Resync timeline to last committed value
 * @tl: Pointer to timeline object.
 */
void dpu_resync_timeline(struct dpu_timeline *tl);

#endif

