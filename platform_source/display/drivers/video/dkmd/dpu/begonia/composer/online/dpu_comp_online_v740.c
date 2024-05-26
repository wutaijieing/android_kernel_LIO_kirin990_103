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
#include <dpu/soc_dpu_define.h>
#include <linux/interrupt.h>

#include "dkmd_acquire_fence.h"
#include "dkmd_release_fence.h"
#include "cmdlist_interface.h"
#include "dpu_comp_mgr.h"
#include "dpu_comp_online.h"
#include "dpu_comp_vactive.h"
#include "dpu_cmdlist.h"
#include "dpu_isr.h"
#include "dpu_comp_abnormal_handle.h"

static void dpu_comp_switch_present_index(struct comp_online_present *present)
{
	present->displayed_idx = present->displaying_idx;
	present->displaying_idx = present->incoming_idx;
	present->incoming_idx = (present->incoming_idx + 1) % COMP_FRAME_MAX;
}

static void composer_online_preprocess(struct comp_online_present *present, struct disp_frame *frame)
{
	int32_t i;
	uint64_t wait_fence_tv;
	struct disp_layer *layer = NULL;
	struct dpu_comp_frame *using_frame = &present->frames[present->incoming_idx];

	dpu_trace_ts_begin(&wait_fence_tv);
	/* lock dma buf and wait layer acquired fence */
	using_frame->in_frame = *frame;
	for (i = 0; i < (int32_t)frame->layer_count; ++i) {
		layer = &frame->layer[i];
		dkmd_buf_sync_lock_dma_buf(&present->timeline, layer->share_fd);
		dkmd_acquire_fence_wait_fd(layer->acquired_fence, ACQUIRE_FENCE_TIMEOUT_MSEC);
		/* fd need be close by HAL */
		layer->acquired_fence = -1;
	}
	/**
	 * @brief increase timeline step value, normally inc step is 1
	 * vsync isr will increase the step with pt_value
	 */
	dkmd_timeline_inc_step(&present->timeline);

	/* check cmdlist_id and lock */
	dpu_cmdlist_sync_lock(&present->timeline, frame);

	/**
	 * @brief After three frames, free logo buffer,
	 * g_dpu_complete_start need set true after booting complete
	 */
	if ((frame->frame_index > 3) && !g_dpu_complete_start) {
		g_dpu_complete_start = true; /* executed only once */
		composer_dpu_free_logo_buffer(present->dpu_comp);
	}
	dpu_trace_ts_end(&wait_fence_tv, "online compose wait gpu fence");
}

static int32_t composer_online_overlay(struct dpu_composer *dpu_comp, struct disp_frame *frame)
{
	struct composer_scene *scene = NULL;
	uint32_t scene_id = (uint32_t)frame->scene_id;
	struct comp_online_present *present = (struct comp_online_present *)dpu_comp->present_data;

	if (unlikely(scene_id > DPU_SCENE_ONLINE_3)) {
		dpu_pr_err("invalid scene_id=%u", scene_id);
		return -1;
	}

	scene = dpu_comp->comp_mgr->scene[scene_id];
	if (unlikely(!scene)) {
		dpu_pr_err("unsupport scene_id=%u", scene_id);
		return -1;
	}
	dpu_comp_active_vsync(dpu_comp);

	down(&dpu_comp->comp_mgr->power_sem);
	dpu_comp->comp_mgr->hardware_reseted &= ~BIT(dpu_comp->comp.index);
	if (dpu_comp->comp_mgr->hardware_reseted != 0) {
		dpu_pr_warn("some error happend, recovery this frame commit!");
		composer_dpu_power_off_sub(dpu_comp);
		composer_dpu_power_on_sub(dpu_comp);
		dpu_comp->comp_mgr->hardware_reseted = 0; /* only online need right now */
		present->vactive_start_flag = 1;
	}
	dpu_pr_info("frame_index=%u: vactive_start_flag=%u enter", frame->frame_index, present->vactive_start_flag);

	/* wait vactive isr */
	if (dpu_comp_vactive_wait_event(present) != 0) {
		dpu_pr_err("scene_id=%u wait vactive timeout! please check config and resync timeline!", scene_id);
		composer_online_timeline_resync(dpu_comp, present);
		up(&dpu_comp->comp_mgr->power_sem);
		dpu_comp_deactive_vsync(dpu_comp);
		return -1;
	}
	composer_online_preprocess(present, frame);

	/* commit to hardware, but sometimes hardware is handling exceptions,
	 * so need acquire comp_mgr power_sem
	 */
	if (likely(scene->present)) {
		scene->frame_index = frame->frame_index;
		dpu_comp_scene_switch(dpu_comp->conn_info, scene);
		scene->present(scene, frame->cmdlist_id);
		if (dpu_comp->conn_info->enable_ldi)
			dpu_comp->conn_info->enable_ldi(dpu_comp->conn_info);
		dpu_comp->isr_ctrl.unmask &= ~DSI_INT_UNDER_FLOW;
	}
	up(&dpu_comp->comp_mgr->power_sem);
	dpu_comp_switch_present_index(present);
	dpu_comp_deactive_vsync(dpu_comp);

	// set backlight before enter ulps
	pipeline_next_ops_handle(dpu_comp->conn_info->conn_device, dpu_comp->conn_info, SET_BACKLIGHT, NULL);
	dpu_pr_info("vactive_start_flag=%d exit", present->vactive_start_flag);
	return 0;
}

static int32_t composer_online_create_fence(struct dpu_composer *dpu_comp)
{
	struct comp_online_present *present = (struct comp_online_present *)dpu_comp->present_data;
	int32_t out_fence_fd = dkmd_release_fence_create(&present->timeline);

	dkmd_timeline_resync(&present->timeline);
	dpu_pr_debug("out_fence_fd:%d", out_fence_fd);

	return out_fence_fd;
}

void composer_online_setup(struct dpu_composer *dpu_comp, struct comp_online_present *present)
{
	uint32_t vsync_bit;
	char tmp_name[256] = {0};
	struct dkmd_isr *isr_ctrl = &dpu_comp->isr_ctrl;
	struct dkmd_connector_info *pinfo = dpu_comp->conn_info;
	struct dkmd_isr *m1_qic_isr_ctrl = &dpu_comp->comp_mgr->m1_qic_isr_ctrl;

	present->dpu_comp = dpu_comp;
	/* NOTICE: dp_fake_panel will be different from dsi panel */
	vsync_bit = (is_mipi_cmd_panel(&pinfo->base) && !g_debug_force_update) ? DSI_INT_LCD_TE0 : DSI_INT_VSYNC;
	if (present->timeline.listening_isr_bit == 0) {
		present->timeline.listening_isr_bit = vsync_bit;
		snprintf(tmp_name, sizeof(tmp_name), "online_composer_%s", dpu_comp->comp.base.name);
		dkmd_timline_init(&present->timeline, tmp_name, dpu_comp);
		present->timeline.present_handle_worker = &dpu_comp->handle_worker;
	} else {
		present->timeline.listening_isr_bit = vsync_bit;
	}
	dkmd_isr_register_listener(isr_ctrl, present->timeline.notifier,
		present->timeline.listening_isr_bit, &present->timeline);

	/* register vsync event to vsync isr */
	if (present->vsync_ctrl.listening_isr_bit == 0) {
		present->vsync_ctrl.listening_isr_bit = vsync_bit;
		present->vsync_ctrl.dpu_comp = dpu_comp;
		dpu_vsync_init(&present->vsync_ctrl, &dpu_comp->attrs);
	} else {
		present->vsync_ctrl.listening_isr_bit = vsync_bit;
	}
	dkmd_isr_register_listener(isr_ctrl, present->vsync_ctrl.notifier,
		present->vsync_ctrl.listening_isr_bit, &present->vsync_ctrl);

	dpu_comp_vactive_init(isr_ctrl, dpu_comp, DSI_INT_VACT0_START);
	dpu_comp_abnormal_handle_init(isr_ctrl, dpu_comp, DSI_INT_UNDER_FLOW);

	if (!is_dp_panel(&pinfo->base)) {
		dpu_comp_alsc_handle_init(isr_ctrl, dpu_comp, DSI_INT_VACT0_START);
		dpu_comp_alsc_handle_init(isr_ctrl, dpu_comp, DSI_INT_VACT0_END);
		dpu_comp_hiace_handle_init(&dpu_comp->comp_mgr->sdp_isr_ctrl, dpu_comp, DPU_DPP0_HIACE_NS_INT);
	}
	dpu_comp_m1_qic_handle_init(m1_qic_isr_ctrl, dpu_comp, DPU_M1_QIC_INTR_SAFETY_TB_DSS_CFG);
	dpu_comp_m1_qic_handle_init(m1_qic_isr_ctrl, dpu_comp, DPU_M1_QIC_INTR_SAFETY_ERR_TB_DSS_CFG);
	dpu_comp_m1_qic_handle_init(m1_qic_isr_ctrl, dpu_comp, DPU_M1_QIC_INTR_SAFETY_IB_DSS_RT0_DATA_RD);
	dpu_comp_m1_qic_handle_init(m1_qic_isr_ctrl, dpu_comp, DPU_M1_QIC_INTR_SAFETY_ERR_IB_DSS_RT0_DATA_RD);
	dpu_comp_m1_qic_handle_init(m1_qic_isr_ctrl, dpu_comp, DPU_M1_QIC_INTR_SAFETY_IB_DSS_RT0_DATA_WR);
	dpu_comp_m1_qic_handle_init(m1_qic_isr_ctrl, dpu_comp, DPU_M1_QIC_INTR_SAFETY_ERR_IB_DSS_RT0_DATA_WR);
	dpu_comp_m1_qic_handle_init(m1_qic_isr_ctrl, dpu_comp, DPU_M1_QIC_INTR_SAFETY_IB_DSS_RT1_DATA_RD);
	dpu_comp_m1_qic_handle_init(m1_qic_isr_ctrl, dpu_comp, DPU_M1_QIC_INTR_SAFETY_ERR_IB_DSS_RT1_DATA_RD);
	dpu_comp_m1_qic_handle_init(m1_qic_isr_ctrl, dpu_comp, DPU_M1_QIC_INTR_SAFETY_IB_DSS_RT2_DATA_RD);
	dpu_comp_m1_qic_handle_init(m1_qic_isr_ctrl, dpu_comp, DPU_M1_QIC_INTR_SAFETY_ERR_IB_DSS_RT2_DATA_RD);
	dpu_comp_m1_qic_handle_init(m1_qic_isr_ctrl, dpu_comp, DPU_M1_QIC_INTR_SAFETY_IB_DSS_RT2_DATA_WR);
	dpu_comp_m1_qic_handle_init(m1_qic_isr_ctrl, dpu_comp, DPU_M1_QIC_INTR_SAFETY_ERR_IB_DSS_RT2_DATA_WR);
	dpu_comp_m1_qic_handle_init(m1_qic_isr_ctrl, dpu_comp, DPU_M1_QIC_INTR_SAFETY_IB_DSS_RT3_DATA_RD);
	dpu_comp_m1_qic_handle_init(m1_qic_isr_ctrl, dpu_comp, DPU_M1_QIC_INTR_SAFETY_ERR_IB_DSS_RT3_DATA_RD);
	dpu_comp_m1_qic_handle_init(m1_qic_isr_ctrl, dpu_comp, DPU_M1_QIC_INTR_SAFETY_TB_QCP_DSS);
	dpu_comp_m1_qic_handle_init(m1_qic_isr_ctrl, dpu_comp, DPU_M1_QIC_INTR_SAFETY_ERR_TB_QCP_DSS);
	dpu_comp->overlay = composer_online_overlay;
	dpu_comp->create_fence = composer_online_create_fence;
}

void composer_online_release(struct dpu_composer *dpu_comp, struct comp_online_present *present)
{
	struct dkmd_isr *isr_ctrl = &dpu_comp->isr_ctrl;

	if (present->timeline.listening_isr_bit != 0) {
		/* resync timeline when power off */
		composer_online_timeline_resync(dpu_comp, present);
		dkmd_isr_unregister_listener(isr_ctrl,
			present->timeline.notifier, present->timeline.listening_isr_bit);
	}

	if (present->vsync_ctrl.listening_isr_bit != 0)
		dkmd_isr_unregister_listener(isr_ctrl,
			present->vsync_ctrl.notifier, present->vsync_ctrl.listening_isr_bit);

	dpu_comp_vactive_deinit(isr_ctrl, DSI_INT_VACT0_START);
	dpu_comp_abnormal_handle_deinit(isr_ctrl, DSI_INT_UNDER_FLOW);

	if (!is_dp_panel(&dpu_comp->conn_info->base)) {
		dpu_comp_alsc_handle_deinit(isr_ctrl, DSI_INT_VACT0_START);
		dpu_comp_alsc_handle_deinit(isr_ctrl, DSI_INT_VACT0_END);
		dpu_comp_hiace_handle_deinit(&dpu_comp->comp_mgr->sdp_isr_ctrl, DPU_DPP0_HIACE_NS_INT);
	}
	dpu_comp_m1_qic_handle_deinit(&dpu_comp->comp_mgr->m1_qic_isr_ctrl, DPU_M1_QIC_INTR_SAFETY_TB_DSS_CFG);
	dpu_comp_m1_qic_handle_deinit(&dpu_comp->comp_mgr->m1_qic_isr_ctrl, DPU_M1_QIC_INTR_SAFETY_ERR_TB_DSS_CFG);
	dpu_comp_m1_qic_handle_deinit(&dpu_comp->comp_mgr->m1_qic_isr_ctrl, DPU_M1_QIC_INTR_SAFETY_IB_DSS_RT0_DATA_RD);
	dpu_comp_m1_qic_handle_deinit(&dpu_comp->comp_mgr->m1_qic_isr_ctrl, DPU_M1_QIC_INTR_SAFETY_ERR_IB_DSS_RT0_DATA_RD);
	dpu_comp_m1_qic_handle_deinit(&dpu_comp->comp_mgr->m1_qic_isr_ctrl, DPU_M1_QIC_INTR_SAFETY_IB_DSS_RT0_DATA_WR);
	dpu_comp_m1_qic_handle_deinit(&dpu_comp->comp_mgr->m1_qic_isr_ctrl, DPU_M1_QIC_INTR_SAFETY_ERR_IB_DSS_RT0_DATA_WR);
	dpu_comp_m1_qic_handle_deinit(&dpu_comp->comp_mgr->m1_qic_isr_ctrl, DPU_M1_QIC_INTR_SAFETY_IB_DSS_RT1_DATA_RD);
	dpu_comp_m1_qic_handle_deinit(&dpu_comp->comp_mgr->m1_qic_isr_ctrl, DPU_M1_QIC_INTR_SAFETY_ERR_IB_DSS_RT1_DATA_RD);
	dpu_comp_m1_qic_handle_deinit(&dpu_comp->comp_mgr->m1_qic_isr_ctrl, DPU_M1_QIC_INTR_SAFETY_IB_DSS_RT2_DATA_RD);
	dpu_comp_m1_qic_handle_deinit(&dpu_comp->comp_mgr->m1_qic_isr_ctrl, DPU_M1_QIC_INTR_SAFETY_ERR_IB_DSS_RT2_DATA_RD);
	dpu_comp_m1_qic_handle_deinit(&dpu_comp->comp_mgr->m1_qic_isr_ctrl, DPU_M1_QIC_INTR_SAFETY_IB_DSS_RT2_DATA_WR);
	dpu_comp_m1_qic_handle_deinit(&dpu_comp->comp_mgr->m1_qic_isr_ctrl, DPU_M1_QIC_INTR_SAFETY_ERR_IB_DSS_RT2_DATA_WR);
	dpu_comp_m1_qic_handle_deinit(&dpu_comp->comp_mgr->m1_qic_isr_ctrl, DPU_M1_QIC_INTR_SAFETY_IB_DSS_RT3_DATA_RD);
	dpu_comp_m1_qic_handle_deinit(&dpu_comp->comp_mgr->m1_qic_isr_ctrl, DPU_M1_QIC_INTR_SAFETY_ERR_IB_DSS_RT3_DATA_RD);
	dpu_comp_m1_qic_handle_deinit(&dpu_comp->comp_mgr->m1_qic_isr_ctrl, DPU_M1_QIC_INTR_SAFETY_TB_QCP_DSS);
	dpu_comp_m1_qic_handle_deinit(&dpu_comp->comp_mgr->m1_qic_isr_ctrl, DPU_M1_QIC_INTR_SAFETY_ERR_TB_QCP_DSS);
}
