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

#ifndef COMPOSER_OFFLINE_H
#define COMPOSER_OFFLINE_H

#include <linux/kthread.h>
#include <linux/dma-fence.h>
#include <linux/spinlock.h>
#include "scene/dpu_comp_scene.h"

struct dpu_comp_fence {
	struct dma_fence *fence;
	spinlock_t lock;
};

struct comp_offline_present {
	/**
	 * @brief frame is each record input inframe data
	 */
	struct dpu_comp_frame frame;

	/**
	 * @brief record each scene offline compose isq status
	 */
	uint32_t wb_done;
	uint32_t wb_has_presented;
	struct semaphore wb_sem;
	wait_queue_head_t wb_wq;

	struct dpu_comp_fence *acquire_fence;
	struct kthread_work compose_work;
	struct dpu_composer *dpu_comp;
};

void composer_offline_setup(struct dpu_composer *dpu_comp, struct comp_offline_present *present);
void composer_offline_release(struct dpu_composer *dpu_comp, struct comp_offline_present *present);

#endif