/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#ifndef HISI_DISP_FENCE_H
#define HISI_DISP_FENCE_H

#include <linux/types.h>
#include <linux/dma-fence.h>

#include "timeline/hisi_disp_timeline.h"

enum disp_fence_type {
	DISP_RELEASE_FENCE = 0,
	DISP_RETIRE_FENCE,
};

struct hisi_disp_fence {
	struct dma_fence base;
	struct hisi_disp_timeline *timeline;
	enum disp_fence_type fence_type;

	void (*dump)(struct hisi_disp_fence *fence);
};

static inline struct hisi_disp_timeline *hisi_fence_get_timeline(struct dma_fence *fence)
{
	struct hisi_disp_fence *disp_fence = (struct hisi_disp_fence *)fence;

	return disp_fence->timeline;
}

int hisi_disp_create_fence(struct hisi_disp_timeline *tl, enum disp_fence_type type);

#endif /* HISI_DISP_FENCE_H */
