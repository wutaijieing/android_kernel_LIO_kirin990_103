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

#include <linux/module.h>
#include <linux/slab.h>
#include "dkmd_timeline.h"

static int32_t _timeline_step_pt_value(struct dkmd_timeline *timeline)
{
	spin_lock(&timeline->value_lock);
	if (timeline->inc_step == 0) {
		spin_unlock(&timeline->value_lock);
		return -1;
	}
	timeline->pt_value += timeline->inc_step;
	timeline->inc_step = 0;
	spin_unlock(&timeline->value_lock);

	return 0;
}

static void dkmd_timline_sync_workqueue(struct kthread_work *work)
{
	struct dkmd_timeline *timeline = NULL;
	struct dkmd_timeline_listener *listener_node = NULL;
	struct dkmd_timeline_listener *_node_ = NULL;
	int32_t ret = 0;

	dpu_check_and_no_retval(!work, err, "work is NULL!!!");

	timeline = container_of(work, typeof(*timeline), timeline_sync_work);
	dpu_check_and_no_retval(!timeline, err, "timeline is NULL!!!");

	mutex_lock(&timeline->list_lock);
	list_for_each_entry_safe(listener_node, _node_, &timeline->listener_list, list_node) {
		if (!listener_node->ops) {
			dpu_pr_err("listener_node ops is NULL!!!");
			continue;
		}

		if (g_debug_fence_timeline)
			dpu_pr_info("timeline:%s timeline_value=<%llu,%llu> listener_name:%s value:%llu",
				timeline->name, timeline->next_value, timeline->pt_value,
				listener_node->ops->get_listener_name(listener_node), listener_node->pt_value);

		if (!listener_node->ops->is_signaled(listener_node, timeline->pt_value))
			continue;

		if (listener_node->ops->handle_signal) {
			ret = listener_node->ops->handle_signal(listener_node);
			if (ret != 0)
				dpu_pr_warn("listener = 0x%p handle fail", listener_node);
		}

		if (listener_node->ops->release)
			listener_node->ops->release(listener_node);

		list_del(&listener_node->list_node);
		kfree(listener_node);
	}
	mutex_unlock(&timeline->list_lock);
}

static int32_t _timeline_isr_notify(struct notifier_block *self, unsigned long action, void *data)
{
	struct dkmd_timeline *timeline = (struct dkmd_timeline *)data;

	if (action != timeline->listening_isr_bit) {
		dpu_pr_info("action %#x is not equal to listening_isr_bit %#x", action, timeline->listening_isr_bit);
		return 0;
	}

	/* pt_value doesn't need step */
	if (_timeline_step_pt_value(timeline) < 0)
		return 0;

	kthread_queue_work(timeline->present_handle_worker, &timeline->timeline_sync_work);

	return 0;
}

static struct notifier_block timeline_isr_notifier = {
	.notifier_call = _timeline_isr_notify,
};

void dkmd_timline_init(struct dkmd_timeline *timeline, const char *name, void *parent)
{
	snprintf(timeline->name, sizeof(timeline->name), "%s", name);
	kref_init(&timeline->kref);
	spin_lock_init(&timeline->value_lock);
	mutex_init(&timeline->list_lock);
	INIT_LIST_HEAD(&timeline->listener_list);

	/* init timeline value, which fence create on */
	timeline->pt_value = 1;
	timeline->next_value = 1; /* for create release fence */
	timeline->parent = parent;
	timeline->notifier = &timeline_isr_notifier;
	kthread_init_work(&timeline->timeline_sync_work, dkmd_timline_sync_workqueue);

	dpu_pr_info("init timeline:%s listening_isr_bit=%#x", timeline->name, timeline->listening_isr_bit);
}
EXPORT_SYMBOL(dkmd_timline_init);

uint64_t dkmd_timeline_get_next_value(struct dkmd_timeline *timeline)
{
	uint64_t value;
	unsigned long flags;

	spin_lock_irqsave(&timeline->value_lock, flags);
	value = timeline->next_value;
	spin_unlock_irqrestore(&timeline->value_lock, flags);

	return value;
}
EXPORT_SYMBOL(dkmd_timeline_get_next_value);

struct dkmd_timeline_listener *dkmd_timeline_alloc_listener(struct dkmd_timeline_listener_ops *listener_ops,
	void *listener_data, uint64_t value)
{
	struct dkmd_timeline_listener *listener = NULL;

	listener = kzalloc(sizeof(*listener), GFP_KERNEL);
	if (!listener) {
		dpu_pr_err("listener kzalloc failed");
		return NULL;
	}

	listener->listener_data = listener_data;
	listener->ops = listener_ops;
	listener->pt_value = value;

	dpu_pr_debug("create listener %pK at pt_value %d", listener, value);

	return listener;
}
EXPORT_SYMBOL(dkmd_timeline_alloc_listener);

MODULE_LICENSE("GPL");
