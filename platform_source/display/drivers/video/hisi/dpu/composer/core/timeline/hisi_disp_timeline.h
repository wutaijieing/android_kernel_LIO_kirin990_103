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
#ifndef HISI_DISP_TIMELINE_H
#define HISI_DISP_TIMELINE_H

#include <linux/types.h>
#include <linux/list.h>

#include "hisi_disp_timeline_listener.h"
#include "hisi_disp_debug.h"

#define HISI_DISP_SYNC_NAME_SIZE 64

struct hisi_composer_device;
struct hisi_disp_timeline {
	struct kref kref;
	char name[HISI_DISP_SYNC_NAME_SIZE];
	uint32_t isr_unmask_bit;
	void *parent;

	spinlock_t value_lock;
	uint32_t next_value;
	uint64_t pt_value;
	uint32_t inc_step;

	spinlock_t list_lock;
	struct list_head listener_list; /* struct hisi_timeline_listener */
};

static inline void hisi_timeline_add_listener(struct hisi_disp_timeline *timeline, struct hisi_timeline_listener *node)
{
	spin_lock(&timeline->list_lock);
	list_add_tail(&node->list_node, &timeline->listener_list);
	spin_unlock(&timeline->list_lock);
}

static inline void hisi_timeline_del_listener(struct hisi_disp_timeline *timeline, struct hisi_timeline_listener *node)
{
	spin_lock(&timeline->list_lock);
	list_del(&node->list_node);
	spin_unlock(&timeline->list_lock);
}

static inline void hisi_timeline_inc_step(struct hisi_disp_timeline *timeline)
{
	spin_lock(&timeline->list_lock);
	++timeline->inc_step;
	spin_unlock(&timeline->list_lock);
}

static inline struct hisi_timeline_listener *hisi_timeline_alloc_listener(struct hisi_timeline_listener_ops *listener_ops, void *listener_data, uint64_t value)
{
	struct hisi_timeline_listener *listener = NULL;

	listener = kzalloc(sizeof(*listener), GFP_KERNEL);
	if (!listener)
		return NULL;

	listener->listener_data = listener_data;
	listener->ops = listener_ops;
	listener->pt_value = value;
	return listener;
}

static inline uint64_t hisi_timeline_get_pt_value(struct hisi_disp_timeline *timeline)
{
	uint64_t value;
	unsigned long flags;
	disp_pr_info(" ++++ \n");
	spin_lock_irqsave(&timeline->value_lock, flags);
	value = timeline->pt_value;
	spin_unlock_irqrestore(&timeline->value_lock, flags);

	return value;
}

void hisi_disp_timline_init(struct hisi_composer_device *parent, struct hisi_disp_timeline *tl, const char *name);


#endif /* HISI_DISP_TIMELINE_H */
