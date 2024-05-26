/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2021. All rights reserved.
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

#include "dpu_comp_mgr.h"
#include "dpu_isr.h"

static void dpu_comp_alsc_handle_work(struct kthread_work *work)
{
	struct comp_online_present *present = NULL;
	struct dpu_composer *dpu_comp = NULL;

	present = container_of(work, struct comp_online_present, alsc_handle_work);
	dpu_check_and_no_retval(!present, err, "preset is null!");

	dpu_comp = present->dpu_comp;
	down(&dpu_comp->comp.blank_sem);

	/* According to the need to deal with hiace interrupts */
	dpu_pr_debug("handle scene=%u alsc interrupts message!",
		present->frames[present->displaying_idx].in_frame.scene_id);

	up(&dpu_comp->comp.blank_sem);
}

static int32_t dpu_comp_alsc_handle_isr_notify(struct notifier_block *self, unsigned long action, void *data)
{
	struct dpu_composer *dpu_comp = (struct dpu_composer *)data;
	struct comp_online_present *present = (struct comp_online_present *)dpu_comp->present_data;

	dpu_pr_debug("action=%#x, enter", action);
	kthread_queue_work(&dpu_comp->handle_worker, &present->alsc_handle_work);

	return 0;
}

static struct notifier_block alsc_handle_isr_notifier = {
	.notifier_call = dpu_comp_alsc_handle_isr_notify,
};

void dpu_comp_alsc_handle_init(struct dkmd_isr *isr_ctrl, struct dpu_composer *dpu_comp, uint32_t listening_bit)
{
	struct comp_online_present *present = (struct comp_online_present *)dpu_comp->present_data;

	kthread_init_work(&present->alsc_handle_work, dpu_comp_alsc_handle_work);

	dkmd_isr_register_listener(isr_ctrl, &alsc_handle_isr_notifier, listening_bit, dpu_comp);
}

void dpu_comp_alsc_handle_deinit(struct dkmd_isr *isr_ctrl, uint32_t listening_bit)
{
	dkmd_isr_unregister_listener(isr_ctrl, &alsc_handle_isr_notifier, listening_bit);
}