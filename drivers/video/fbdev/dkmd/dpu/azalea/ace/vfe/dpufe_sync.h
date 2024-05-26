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

#ifndef DPUFE_SYNC_H
#define DPUFE_SYNC_H

#include <linux/dma-fence.h>
#include <linux/types.h>

enum {
	DPU_RELEASE_FENCE = 0,
	DPU_RETIRE_FENCE,
};

struct dpufe_timeline {
	struct kref kref;
	spinlock_t lock;
	spinlock_t list_lock;
	char name[64];
	u32 next_value;
	u32 value;
	u64 context;
	struct list_head fence_list_head;
};

/**
 * struct dpu_fence - sync fence context
 * @base: base sync fence object
 * @name: name of this sync fence
 * @fence_list: linked list of outstanding sync fence
 */
struct dpufe_fence {
	struct dma_fence base;
	char name[256];
	struct list_head fence_list;
};

struct dpufe_timeline *dpufe_create_timeline(const char *name);
void dpufe_destroy_timeline(struct dpufe_timeline *tl);
struct dpufe_fence *dpufe_get_sync_fence(
	struct dpufe_timeline *tl, const char *fence_name, u32 *timestamp, int value);
int dpufe_inc_timeline(struct dpufe_timeline *tl, int increment);
int dpufe_wait_sync_fence(struct dpufe_fence *fence, long timeout);
void dpufe_resync_timeline(struct dpufe_timeline *tl);
struct dpufe_fence *dpufe_get_fd_sync_fence(int fd);
int dpufe_get_sync_fence_fd(struct dpufe_fence *fence);

#endif
