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

#include <linux/delay.h>
#include <dpu/soc_dpu_define.h>
#include "dkmd_release_fence.h"
#include "dkmd_acquire_fence.h"
#include "dpu_comp_mgr.h"
#include "scene/dpu_comp_scene.h"
#include "dpu_comp_vactive.h"
#include "dpu_cmdlist.h"
#include "cmdlist_interface.h"
#include "dpu_comp_offline.h"
#include "dpu_comp_online.h"
#include "dpu_comp_sysfs.h"
#include "dpu_config_utils.h"

void composer_online_timeline_resync(struct dpu_composer *dpu_comp, struct comp_online_present *present)
{
	dkmd_timeline_inc_step_by_val(&present->timeline, 2);
	dkmd_isr_notify_listener(&dpu_comp->isr_ctrl, present->timeline.listening_isr_bit);
	present->vactive_start_flag = 1;
}

int32_t composer_manager_present(struct composer *comp, void *in_frame)
{
	int ret = 0;
	struct disp_frame *frame = (struct disp_frame *)in_frame;
	struct dpu_composer *dpu_comp = to_dpu_composer(comp);

	if (!dpu_comp) {
		dpu_pr_err("dpu_comp is nullptr");
		dkmd_cmdlist_release_locked(frame->scene_id, frame->cmdlist_id);
		return -1;
	}

	ret = dpu_comp->overlay(dpu_comp, frame);
	if (ret != 0) {
		dpu_pr_err("scene_id=%d free cmdlist_id=%d buffer", frame->scene_id, frame->cmdlist_id);
		dkmd_cmdlist_release_locked(frame->scene_id, frame->cmdlist_id);
	}

	return ret;
}

void composer_present_data_setup(struct dpu_composer *dpu_comp, bool inited)
{
	struct dkmd_connector_info *pinfo = dpu_comp->conn_info;
	struct comp_online_present *present = NULL;

	if (is_offline_panel(&pinfo->base)) {
		if (!inited)
			composer_offline_setup(dpu_comp, (struct comp_offline_present *)dpu_comp->present_data);
		return;
	}

	present = (struct comp_online_present *)dpu_comp->present_data;
	present->vactive_start_flag = 1;

	/* Only used for CMD mode and VIDEO mode switching debugging */
	if ((g_debug_force_update && is_mipi_cmd_panel(&pinfo->base)) || inited) {
		pipeline_next_ops_handle(pinfo->conn_device, pinfo, SETUP_ISR, (void *)&dpu_comp->isr_ctrl);
		dkmd_isr_setup(&dpu_comp->isr_ctrl);
		list_add_tail(&dpu_comp->isr_ctrl.list_node, &dpu_comp->comp_mgr->isr_list);
		composer_online_setup(dpu_comp, present);
		dpu_comp_add_attrs(&dpu_comp->attrs);
	}

	/* NOTICE: stub max perf config when dptx connect */
	if (is_dp_panel(&pinfo->base) && !inited) {
		dpu_comp_active_vsync(dpu_comp);
		dpu_legacy_inter_frame_dvfs_vote(4);
	}
}

void composer_present_data_release(struct dpu_composer *dpu_comp)
{
	struct dkmd_connector_info *pinfo = dpu_comp->conn_info;
	struct comp_online_present *present = NULL;

	if (is_offline_panel(&pinfo->base)) {
		composer_offline_release(dpu_comp, (struct comp_offline_present *)dpu_comp->present_data);
		return;
	}

	present = (struct comp_online_present *)dpu_comp->present_data;

	/* Only used for CMD mode and VIDEO mode switching debugging */
	if (g_debug_force_update && is_mipi_cmd_panel(&pinfo->base)) {
		dpu_comp->isr_ctrl.handle_func(&dpu_comp->isr_ctrl, DKMD_ISR_RELEASE);
		composer_online_release(dpu_comp, present);
		list_del(&dpu_comp->isr_ctrl.list_node);
	}

	/* NOTICE: stub max perf config when dptx connect */
	if (is_dp_panel(&pinfo->base)) {
		dpu_legacy_inter_frame_dvfs_vote(1);
		dpu_comp_deactive_vsync(dpu_comp);
	}

	/* resync timeline when power off */
	composer_online_timeline_resync(dpu_comp, present);
}
