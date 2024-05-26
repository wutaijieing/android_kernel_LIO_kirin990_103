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
#include <linux/notifier.h>
#include <linux/spinlock.h>
#include "hisi_disp_composer.h"
#include "hisi_disp_timeline.h"
#include "isr/hisi_isr_utils.h"

static int hisi_timeline_step_pt_value(struct hisi_disp_timeline *timeline)
{
	spin_lock(&timeline->value_lock);
	//disp_pr_info("timeline pt_value=%u, inc_step=%u", timeline->pt_value, timeline->inc_step);
	if (timeline->inc_step == 0) {
		spin_unlock(&timeline->value_lock);
		return -1;
	}

	timeline->pt_value += timeline->inc_step;
	timeline->inc_step = 0;

	spin_unlock(&timeline->value_lock);
	return 0;
}

static int hisi_timeline_isr_notify(struct notifier_block *self, unsigned long action, void *data)
{
	struct hisi_composer_device *comp_dev = (struct hisi_composer_device *)data;
	struct hisi_disp_timeline *tl = &comp_dev->timeline;
	struct hisi_timeline_listener *listener_node;
	struct hisi_timeline_listener *_node_;
	bool is_signaled = false;

	if (action != tl->isr_unmask_bit) {
		disp_pr_info("action 0x%x is not equal to isr_unmask_bit 0x%x", action, tl->isr_unmask_bit);
		return 0;
	}

	/* pt_value doesn't need step */
	if (hisi_timeline_step_pt_value(tl) < 0)
		return 0;

	list_for_each_entry_safe(listener_node, _node_, &tl->listener_list, list_node) {
		// disp_pr_info(" pt_value:%u", listener_node->pt_value);
		if (!listener_node->ops || !listener_node->ops->is_signaled)
			continue;

		is_signaled = listener_node->ops->is_signaled(listener_node, tl->pt_value);
		if (!is_signaled)
			continue;

		if (listener_node->ops->handle_signal)
			listener_node->ops->handle_signal(listener_node);

		if (listener_node->ops->release)
			listener_node->ops->release(listener_node);

		hisi_timeline_del_listener(tl, listener_node);
		kfree(listener_node);
	}

	return 0;
}

static struct notifier_block timeline_isr_notifier = {
	.notifier_call = hisi_timeline_isr_notify,
};

void hisi_disp_timline_init(struct hisi_composer_device *parent, struct hisi_disp_timeline *tl, const char *name)
{
	snprintf(tl->name, sizeof(tl->name), "%s", name);
	disp_pr_info(" ++++ ");
	kref_init(&tl->kref);
	spin_lock_init(&tl->value_lock);
	spin_lock_init(&tl->list_lock);

	tl->pt_value = 0;
	tl->next_value = 0;
	tl->parent = parent;
	// tl->isr_unmask_bit = DSI_INT_VSYNC
	tl->isr_unmask_bit = hisi_disp_isr_get_vsync_bit(parent->ov_data.fix_panel_info->type);

	INIT_LIST_HEAD(&tl->listener_list);
	disp_pr_info("init call hisi_disp_isr_register_listener, isr_unmask_bit = %d", tl->isr_unmask_bit);
	hisi_disp_isr_register_listener(&parent->ov_data.isr_ctrl[COMP_ISR_POST], &timeline_isr_notifier, IRQ_UNMASK_GLB, tl->isr_unmask_bit);
}
