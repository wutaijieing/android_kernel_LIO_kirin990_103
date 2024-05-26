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

static void dpu_comp_hiace_handle_work(struct kthread_work *work)
{
	uint64_t tv0;
	struct comp_online_present *present = NULL;
	struct dpu_composer *dpu_comp = NULL;

	present = container_of(work, struct comp_online_present, hiace_handle_work);
	dpu_check_and_no_retval(!present, err, "preset is null!");

	dpu_trace_ts_begin(&tv0);
	dpu_comp = present->dpu_comp;
	down(&dpu_comp->comp.blank_sem);

	/* According to the need to deal with hiace interrupts */
	dpu_pr_debug("handle scene=%u hiace interrupts message!",
		present->frames[present->displaying_idx].in_frame.scene_id);

	up(&dpu_comp->comp.blank_sem);
	dpu_trace_ts_end(&tv0, "hiace handle finished!");
}

static int32_t dpu_comp_hiace_handle_isr_notify(struct notifier_block *self, unsigned long action, void *data)
{
	struct dpu_composer *dpu_comp = (struct dpu_composer *)data;
	struct comp_online_present *present = (struct comp_online_present *)dpu_comp->present_data;

	dpu_pr_debug("action=%#x, enter", action);
	kthread_queue_work(&dpu_comp->handle_worker, &present->hiace_handle_work);

	return 0;
}

static struct notifier_block hiace_handle_isr_notifier = {
	.notifier_call = dpu_comp_hiace_handle_isr_notify,
};

void dpu_comp_hiace_handle_init(struct dkmd_isr *isr_ctrl, struct dpu_composer *dpu_comp, uint32_t listening_bit)
{
	struct comp_online_present *present = NULL;

	if (!isr_ctrl || !dpu_comp) {
		dpu_pr_err("isr_ctrl or dpu_comp is null ptr");
		return;
	}

	if (isr_ctrl->irq_no < 0) {
		dpu_pr_warn("%s error irq_no: %d", isr_ctrl->irq_name, isr_ctrl->irq_no);
		return;
	}
	present = (struct comp_online_present *)dpu_comp->present_data;

	kthread_init_work(&present->hiace_handle_work, dpu_comp_hiace_handle_work);

	dkmd_isr_register_listener(isr_ctrl, &hiace_handle_isr_notifier, listening_bit, dpu_comp);
}

void dpu_comp_hiace_handle_deinit(struct dkmd_isr *isr_ctrl, uint32_t listening_bit)
{
	if (!isr_ctrl) {
		dpu_pr_err("isr_ctrl is null ptr");
		return;
	}

	if (isr_ctrl->irq_no < 0) {
		dpu_pr_warn("%s error irq_no: %d", isr_ctrl->irq_name, isr_ctrl->irq_no);
		return;
	}

	dkmd_isr_unregister_listener(isr_ctrl, &hiace_handle_isr_notifier, listening_bit);
}