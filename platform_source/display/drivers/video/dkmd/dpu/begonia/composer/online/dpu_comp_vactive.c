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

#include <linux/delay.h>

#include "dpu_comp_mgr.h"
#include "dpu_comp_vactive.h"

static int32_t dpu_comp_vactive_isr_notify(struct notifier_block *self, unsigned long action, void *data)
{
	struct dpu_composer *dpu_comp = (struct dpu_composer *)data;
	struct comp_online_present *present = (struct comp_online_present *)dpu_comp->present_data;

	++present->vactive_start_flag;
	if (is_mipi_cmd_panel(&dpu_comp->conn_info->base)) {
		/* disable mipi ldi */
		dpu_comp->conn_info->disable_ldi(dpu_comp->conn_info);
		dpu_pr_debug("vactive start disable ldi!");
	}
	wake_up_interruptible_all(&present->vactive_start_wq);

	return 0;
}

static struct notifier_block vactive_isr_notifier = {
	.notifier_call = dpu_comp_vactive_isr_notify,
};

void dpu_comp_vactive_init(struct dkmd_isr *isr, struct dpu_composer *dpu_comp, uint32_t listening_bit)
{
	struct comp_online_present *present = (struct comp_online_present *)dpu_comp->present_data;

	init_waitqueue_head(&present->vactive_start_wq);
	present->vactive_start_flag = 1;

	dkmd_isr_register_listener(isr, &vactive_isr_notifier, listening_bit, dpu_comp);
}

void dpu_comp_vactive_deinit(struct dkmd_isr *isr, uint32_t listening_bit)
{
	dkmd_isr_unregister_listener(isr, &vactive_isr_notifier, listening_bit);
}

int32_t dpu_comp_vactive_wait_event(struct comp_online_present *present)
{
	int32_t ret = 0;
	int32_t timeout = ASIC_EVENT_TIMEOUT_MS;
	uint32_t prev_vactive_start_flag;
	uint64_t vactive_tv;
	struct dpu_composer *dpu_comp = present->dpu_comp;

	dpu_trace_ts_begin(&vactive_tv);
	if (dpu_comp->conn_info->base.fpga_flag)
		timeout = FPGA_EVENT_TIMEOUT_MS;

	present = (struct comp_online_present *)dpu_comp->present_data;
	while (true) {
		if (is_mipi_cmd_panel(&dpu_comp->conn_info->base)) {
			ret = wait_event_interruptible_timeout(present->vactive_start_wq,
				present->vactive_start_flag,
				msecs_to_jiffies(timeout));
		} else {
			prev_vactive_start_flag = present->vactive_start_flag;
			ret = (int32_t)wait_event_interruptible_timeout(present->vactive_start_wq,
				(prev_vactive_start_flag != present->vactive_start_flag) ||
				(present->vactive_start_flag == 1),
				msecs_to_jiffies(timeout));
		}
		if (ret == -ERESTARTSYS)
			continue;

		if (ret > 0) {
			if (is_mipi_cmd_panel(&dpu_comp->conn_info->base))
				present->vactive_start_flag = 0;
		} else {
			dpu_pr_warn("pipe_sw_itfch_idx=%d online compose wait vactive_start timeout!",
				dpu_comp->conn_info->base.pipe_sw_itfch_idx);
			if (g_debug_dpu_clear_enable)
				kthread_queue_work(&dpu_comp->handle_worker, &present->abnormal_handle_work);
		}

		break;
	}
	dpu_trace_ts_end(&vactive_tv, "online compose wait vactive");

	return (ret > 0) ? 0 : -1;
}
