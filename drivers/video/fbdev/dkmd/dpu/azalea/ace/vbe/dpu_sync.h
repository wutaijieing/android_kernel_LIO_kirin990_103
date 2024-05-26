/* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#endif

