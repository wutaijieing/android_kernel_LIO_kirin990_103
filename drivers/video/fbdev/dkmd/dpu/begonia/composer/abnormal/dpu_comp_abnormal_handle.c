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
#include <dpu/dpu_dm.h>
#include <soc_media1_crg_interface.h>
#include <linux/interrupt.h>
#include "dpu_comp_mgr.h"
#include "dpu_comp_vactive.h"
#include "dpu_comp_abnormal_handle.h"
#include "dpu_comp_smmu.h"
#include "dpu_opr_config.h"
#include "dpu_dacc.h"

static void dpu_comp_handle_underflow_clear(struct dpu_composer *dpu_comp)
{
	uint32_t ret = 0, clear_bit = 0;
	uint32_t displaying_scene_id, displayed_scene_id;
	struct dkmd_connector_info *pinfo = dpu_comp->conn_info;
	struct comp_online_present *present = (struct comp_online_present *)dpu_comp->present_data;

	/* 1. after disable ldi and set clear, wait 16ms */
	dpu_pr_info("underflow clear start!");
	clear_bit = BIT(pinfo->sw_post_chn_idx[PRIMARY_CONNECT_CHN_IDX]);
	if (pinfo->sw_post_chn_num > 1)
		clear_bit = clear_bit | BIT(pinfo->sw_post_chn_idx[EXTERNAL_CONNECT_CHN_IDX]);
	outp32(DPU_GLB_RS_CLEAR_ADDR(dpu_comp->comp_mgr->dpu_base + DPU_GLB0_OFFSET), clear_bit);

	mdelay(16);

	/* 2. enable dacc_crg clr_rsicv_start */
	displaying_scene_id = (uint32_t)present->frames[present->displaying_idx].in_frame.scene_id;
	displayed_scene_id = (uint32_t)present->frames[present->displayed_idx].in_frame.scene_id;
	ret |= dpu_dacc_handle_clear(dpu_comp->comp_mgr->dpu_base, displaying_scene_id);
	if (displaying_scene_id != displayed_scene_id)
		ret |= dpu_dacc_handle_clear(dpu_comp->comp_mgr->dpu_base, displayed_scene_id);

	outp32(DPU_GLB_RS_CLEAR_ADDR(dpu_comp->comp_mgr->dpu_base + DPU_GLB0_OFFSET), 0);

	/* 3. check it in vactive_end intr */
	if (ret == 0)
		ret = (uint32_t)pipeline_next_ops_handle(pinfo->conn_device, pinfo,
												 DO_CLEAR, (void *)dpu_comp->comp_mgr->dpu_base);

	if (ret != 0) {
		dpu_pr_info("underflow clear failed, reset hardware!!");
		/* to balance isr_enable in reset hardware, we need enable isr here */
		dpu_comp->isr_ctrl.handle_func(&dpu_comp->isr_ctrl, DKMD_ISR_ENABLE);
		composer_manager_reset_hardware(dpu_comp);
		dpu_comp->isr_ctrl.handle_func(&dpu_comp->isr_ctrl, DKMD_ISR_DISABLE);
	} else {
		dpu_pr_info("underflow clear successfully!");
	}
	if (!is_offline_panel(&dpu_comp->conn_info->base))
		present->vactive_start_flag = 1;
}

static void dpu_comp_abnormal_dump_dacc(char __iomem *dpu_base, uint32_t scene_id)
{
	char __iomem *module_base = dpu_base + DACC_OFFSET + DMC_OFFSET;

	dpu_pr_info("DACC:\n\n"
		"CORE_STATUS_REG=%#x\t\tDACC_CORE_MON_PC_REG=%#x\t\tDBG_DEMURA_RLST_CNT=%#x\t\tDBG_SCL_RLST_CNT_01=%#x\n"
		"DBG_SCL_RLST_CNT_23=%#x\t\tDBG_UVUP_RLST_YCNT=%#x\t\tDBG_CST_ALL=%#x\t\tDBG_FSM_MISC=%#x\n"
		"DBG_BUSY_REQ_MISC=%#x\t\tDBG_RESERVED_0=%#x\t\tDBG_RESERVED_1=%#x\n\n",
		inp32(SOC_DACC_CORE_STATUS_REG_ADDR(module_base)), inp32(SOC_DACC_CORE_MON_PC_REG_ADDR(module_base)),
		inp32(SOC_DACC_DBG_DEMURA_RLST_CNT_ADDR(module_base, scene_id)),
		inp32(SOC_DACC_DBG_SCL_RLST_CNT_01_ADDR(module_base, scene_id)),
		inp32(SOC_DACC_DBG_SCL_RLST_CNT_23_ADDR(module_base, scene_id)),
		inp32(SOC_DACC_DBG_UVUP_RLST_YCNT_ADDR(module_base, scene_id)),
		inp32(SOC_DACC_DBG_CST_ALL_ADDR(module_base, scene_id)),
		inp32(SOC_DACC_DBG_FSM_MISC_ADDR(module_base, scene_id)),
		inp32(SOC_DACC_DBG_BUSY_REQ_MISC_ADDR(module_base, scene_id)),
		inp32(SOC_DACC_DBG_RESERVED_0_ADDR(module_base, scene_id)),
		inp32(SOC_DACC_DBG_RESERVED_1_ADDR(module_base, scene_id)));
}

static void dpu_comp_abnormal_dump_lbuf(char __iomem *dpu_base)
{
	char __iomem *module_base = dpu_base + DPU_LBUF_OFFSET;

	dpu_pr_info("LBUF:\n"
		"\tLBUF_DCPT_STATE=%#x\t\tLBUF_DCPT_DMA_STATE=%#x\t\tLBUF_CTL_STATE=%#08x\t\tLBUF_RF_STATE=%#x\n"
		"\tLBUF_PRE_USE_STATE=%#x\t\tLBUF_PPST_USE_STATE=%#x\t\tLBUF_PRE_CHECK_STATE=%#x\t\tLBUF_PPST_CHECK_STATE=%#x\n"
		"\tLBUF_SCN_STATE=%#08x\t\tLBUF_SCN_DMA_STATE=%#x\t\tLBUF_RUN_STATE=%#08x\t\tLBUF_RUN_MST_STATE=%#x\n"
		"\tLBUF_RUN_CTL_STATE=%#x\t\tLBUF_RUN_WR_STATE=%#x\t\tLBUF_RUN_RD_STATE=%#x\t\tLBUF_RUN_RD2_STATE=%#x\n"
		"\tLBUF_PART0_RMST_PASS=%#x\tLBUF_PART0_WMST_PASS=%#x\tLBUF_PART0_IDX_DATA=%#x\tLBUF_PART0_RUN_DAT=%#x\n"
		"\tLBUF_PART1_RMST_PASS=%#x\tLBUF_PART1_WMST_PASS=%#x\tLBUF_PART1_IDX_DATA=%#x\tLBUF_PART1_RUN_DATA=%#x\n"
		"\tLBUF_PART2_RMST_PASS=%#x\tLBUF_PART2_WMST_PASS=%#x\tLBUF_PART2_IDX_DATA=%#x\tLBUF_PART2_RUN_DATA=%#x\n",
		inp32(DPU_LBUF_DCPT_STATE_ADDR(module_base)),
		inp32(DPU_LBUF_DCPT_DMA_STATE_ADDR(module_base)),
		inp32(DPU_LBUF_CTL_STATE_ADDR(module_base)),
		inp32(DPU_LBUF_RF_STATE_ADDR(module_base)),
		inp32(DPU_LBUF_PRE_USE_STATE_ADDR(module_base)),
		inp32(DPU_LBUF_PPST_USE_STATE_ADDR(module_base)),
		inp32(DPU_LBUF_PRE_CHECK_STATE_ADDR(module_base)),
		inp32(DPU_LBUF_PPST_CHECK_STATE_ADDR(module_base)),
		inp32(DPU_LBUF_SCN_STATE_ADDR(module_base)),
		inp32(DPU_LBUF_SCN_DMA_STATE_ADDR(module_base)),
		inp32(DPU_LBUF_RUN_STATE_ADDR(module_base)),
		inp32(DPU_LBUF_RUN_MST_STATE_ADDR(module_base)),
		inp32(DPU_LBUF_RUN_CTL_STATE_ADDR(module_base)),
		inp32(DPU_LBUF_RUN_WR_STATE_ADDR(module_base)),
		inp32(DPU_LBUF_RUN_RD_STATE_ADDR(module_base)),
		inp32(DPU_LBUF_RUN_RD2_STATE_ADDR(module_base)),
		inp32(DPU_LBUF_PART0_RMST_PASS_ADDR(module_base)),
		inp32(DPU_LBUF_PART0_WMST_PASS_ADDR(module_base)),
		inp32(DPU_LBUF_PART0_IDX_DATA_ADDR(module_base)),
		inp32(DPU_LBUF_PART0_RUN_DAT_ADDR(module_base)),
		inp32(DPU_LBUF_PART1_RMST_PASS_ADDR(module_base)),
		inp32(DPU_LBUF_PART1_WMST_PASS_ADDR(module_base)),
		inp32(DPU_LBUF_PART1_IDX_DATA_ADDR(module_base)),
		inp32(DPU_LBUF_PART1_RUN_DATA_ADDR(module_base)),
		inp32(DPU_LBUF_PART2_RMST_PASS_ADDR(module_base)),
		inp32(DPU_LBUF_PART2_WMST_PASS_ADDR(module_base)),
		inp32(DPU_LBUF_PART2_IDX_DATA_ADDR(module_base)),
		inp32(DPU_LBUF_PART2_RUN_DATA_ADDR(module_base)));
}

void dpu_comp_abnormal_debug_dump(char __iomem *dpu_base, uint32_t scene_id)
{
	uint32_t i;
	char __iomem *module_base = dpu_base + DPU_GLB0_OFFSET;

	dpu_pr_info("scene_id=%u ip_status: DBG_IP_STATE0=%#x DBG_IP_STATE1=%#x", scene_id,
		inp32(DPU_GLB_DBG_IP_STATE0_ADDR(module_base)), inp32(DPU_GLB_DBG_IP_STATE1_ADDR(module_base)));

	dpu_pr_info("OV:\n"
		"\tDBG_OV_YCNT0=%#x\tDBG_OV_YCNT1=%#x\tDBG_OV_YCNT2=%#x\tDBG_OV_YCNT3=%#x\n\n",
		inp32(DPU_GLB_DBG_OV_YCNT0_ADDR(module_base)), inp32(DPU_GLB_DBG_OV_YCNT1_ADDR(module_base)),
		inp32(DPU_GLB_DBG_OV_YCNT2_ADDR(module_base)), inp32(DPU_GLB_DBG_OV_YCNT3_ADDR(module_base)));

	for (i = 0; i < OPR_SDMA_NUM; ++i)
		dpu_pr_info("SDMA[%u]:\n"
			"\tSDMA_DBG_MONITOR0=%#x\t\tSDMA_DBG_MONITOR1=%#x\n\tSDMA_DBG_MONITOR2=%#x\t\tSDMA_DBG_MONITOR3=%#x\n"
			"\tSDMA_DBG_MONITOR4=%#x\t\tSDMA_DBG_MONITOR5=%#x\n\tSDMA_W_DBG0=%#x\t\tSDMA_W_DBG1=%#x\n"
			"\tSDMA_W_DBG2=%#x\t\tSDMA_W_DBG3=%#x\n\n", i,
			inp32(DPU_GLB_SDMA_DBG_MONITOR0_ADDR(module_base, i)), inp32(DPU_GLB_SDMA_DBG_MONITOR1_ADDR(module_base, i)),
			inp32(DPU_GLB_SDMA_DBG_MONITOR2_ADDR(module_base, i)), inp32(DPU_GLB_SDMA_DBG_MONITOR3_ADDR(module_base, i)),
			inp32(DPU_GLB_SDMA_DBG_MONITOR4_ADDR(module_base, i)), inp32(DPU_GLB_SDMA_DBG_MONITOR5_ADDR(module_base, i)),
			inp32(DPU_GLB_SDMA_W_DBG0_ADDR(module_base, i)), inp32(DPU_GLB_SDMA_W_DBG1_ADDR(module_base, i)),
			inp32(DPU_GLB_SDMA_W_DBG2_ADDR(module_base, i)), inp32(DPU_GLB_SDMA_W_DBG3_ADDR(module_base, i)));

	for (i = 0; i < OPR_CLD_NUM; ++i) {
		module_base = dpu_base + g_cld_offset[i];
		dpu_pr_info("CLD[%u]:\n"
			"\tR_LB_DEBUG0=%#x\t\tR_LB_DEBUG1=%#x\t\tR_LB_DEBUG2=%#x\t\tR_LB_DEBUG3=%#x\n"
			"\tW_LB_DEBUG0=%#x\t\tW_LB_DEBUG1=%#x\t\tW_LB_DEBUG2=%#x\t\tW_LB_DEBUG3=%#x\n\n", i,
			inp32(DPU_CLD_R_LB_DEBUG0_ADDR(module_base)), inp32(DPU_CLD_R_LB_DEBUG0_ADDR(module_base)),
			inp32(DPU_CLD_R_LB_DEBUG0_ADDR(module_base)), inp32(DPU_CLD_R_LB_DEBUG0_ADDR(module_base)),
			inp32(DPU_CLD_W_LB_DEBUG0_ADDR(module_base)), inp32(DPU_CLD_W_LB_DEBUG0_ADDR(module_base)),
			inp32(DPU_CLD_W_LB_DEBUG0_ADDR(module_base)), inp32(DPU_CLD_W_LB_DEBUG0_ADDR(module_base)));
	}

	module_base = dpu_base + DPU_DBCU_OFFSET;
	dpu_pr_info("DBCU:\n"
		"\tMONITOR_OS_R0=%#x\t\tMONITOR_OS_R1=%#x\t\tMONITOR_OS_R2=%#x\t\tMONITOR_OS_R3=%#x\t\tMONITOR_OS_R4=%#x\n"
		"\tMONITOR_OS_W0=%#x\t\tMONITOR_OS_W1=%#x\t\tMONITOR_OS_W2=%#x\t\tMONITOR_OS_W3=%#x\n"
		"\tAIF_STAT_0=%#x\t\tAIF_STAT_1=%#x\t\tAIF_STAT_2=%#x\t\tAIF_STAT_3=%#x\n"
		"\tAIF_STAT_4=%#x\t\tAIF_STAT_5=%#x\t\tAIF_STAT_6=%#x\t\tAIF_STAT_7=%#x\n"
		"\tAIF_STAT_8=%#x\t\tAIF_STAT_9=%#x\t\tAIF_STAT_10=%#x\t\tAIF_STAT_11=%#x\n"
		"\tAIF_STAT_12=%#x\t\tAIF_STAT_13=%#x\t\tAIF_STAT_14=%#x\t\tAIF_STAT_15=%#x\n\n",
		inp32(DPU_DBCU_MONITOR_OS_R0_ADDR(module_base)), inp32(DPU_DBCU_MONITOR_OS_R1_ADDR(module_base)),
		inp32(DPU_DBCU_MONITOR_OS_R2_ADDR(module_base)), inp32(DPU_DBCU_MONITOR_OS_R3_ADDR(module_base)),
		inp32(DPU_DBCU_MONITOR_OS_R4_ADDR(module_base)),
		inp32(DPU_DBCU_MONITOR_OS_W0_ADDR(module_base)), inp32(DPU_DBCU_MONITOR_OS_W1_ADDR(module_base)),
		inp32(DPU_DBCU_MONITOR_OS_W2_ADDR(module_base)), inp32(DPU_DBCU_MONITOR_OS_W3_ADDR(module_base)),
		inp32(DPU_DBCU_AIF_STAT_0_ADDR(module_base)), inp32(DPU_DBCU_AIF_STAT_1_ADDR(module_base)),
		inp32(DPU_DBCU_AIF_STAT_2_ADDR(module_base)), inp32(DPU_DBCU_AIF_STAT_3_ADDR(module_base)),
		inp32(DPU_DBCU_AIF_STAT_4_ADDR(module_base)), inp32(DPU_DBCU_AIF_STAT_5_ADDR(module_base)),
		inp32(DPU_DBCU_AIF_STAT_6_ADDR(module_base)), inp32(DPU_DBCU_AIF_STAT_7_ADDR(module_base)),
		inp32(DPU_DBCU_AIF_STAT_8_ADDR(module_base)), inp32(DPU_DBCU_AIF_STAT_9_ADDR(module_base)),
		inp32(DPU_DBCU_AIF_STAT_10_ADDR(module_base)), inp32(DPU_DBCU_AIF_STAT_11_ADDR(module_base)),
		inp32(DPU_DBCU_AIF_STAT_12_ADDR(module_base)), inp32(DPU_DBCU_AIF_STAT_13_ADDR(module_base)),
		inp32(DPU_DBCU_AIF_STAT_14_ADDR(module_base)), inp32(DPU_DBCU_AIF_STAT_15_ADDR(module_base)));

	if (scene_id <= DPU_SCENE_ONLINE_3) {
		module_base = dpu_base + g_itfsw_offset[scene_id];
		dpu_pr_info("ITFSW:\n"
			"\tR_LB_DEBUG0=%#x\t\tR_LB_DEBUG1=%#x\t\tR_LB_DEBUG2=%#x\t\tR_LB_DEBUG3=%#x\n\n",
			inp32(DPU_ITF_CH_R_LB_DEBUG0_ADDR(module_base)), inp32(DPU_ITF_CH_R_LB_DEBUG1_ADDR(module_base)),
			inp32(DPU_ITF_CH_R_LB_DEBUG2_ADDR(module_base)), inp32(DPU_ITF_CH_R_LB_DEBUG3_ADDR(module_base)));
	}

	dpu_comp_abnormal_dump_lbuf(dpu_base);
	dpu_comp_abnormal_dump_dacc(dpu_base, scene_id);
}

static void dpu_comp_abnormal_handle_work(struct kthread_work *work)
{
	uint64_t tv0;
	struct comp_online_present *present = NULL;
	struct dpu_composer *dpu_comp = NULL;

	present = container_of(work, struct comp_online_present, abnormal_handle_work);
	if (!present || !present->dpu_comp) {
		dpu_pr_err("present_data or dpu_comp is null");
		return;
	}
	dpu_comp = present->dpu_comp;
	dpu_trace_ts_begin(&tv0);

	down(&dpu_comp->comp.blank_sem);
	if (dpu_comp->comp_mgr->power_status.status == 0) {
		dpu_pr_info("already power off, do not need handle underflow clear!");
		up(&dpu_comp->comp.blank_sem);
		return;
	}
	dpu_comp->isr_ctrl.handle_func(&dpu_comp->isr_ctrl, DKMD_ISR_DISABLE);
	if (dpu_comp->conn_info->disable_ldi)
		dpu_comp->conn_info->disable_ldi(dpu_comp->conn_info);

	dpu_comp_handle_underflow_clear(dpu_comp);

	dpu_comp->isr_ctrl.handle_func(&dpu_comp->isr_ctrl, DKMD_ISR_ENABLE);

	up(&dpu_comp->comp.blank_sem);

	dpu_trace_ts_end(&tv0, "handle underflow clear");
}

static int32_t dpu_comp_abnormal_handle_isr_notify(struct notifier_block *self, unsigned long action, void *data)
{
	uint32_t displaying_scene_id, displayed_scene_id;
	struct comp_online_present *present = NULL;
	struct dpu_composer *dpu_comp = (struct dpu_composer *)data;

	dpu_pr_info("action=%#x, enter", action);
	dpu_comp->isr_ctrl.unmask |= DSI_INT_UNDER_FLOW;

	present = (struct comp_online_present *)dpu_comp->present_data;
	displaying_scene_id = (uint32_t)(present->frames[present->displaying_idx].in_frame.scene_id);
	displayed_scene_id = (uint32_t)(present->frames[present->displayed_idx].in_frame.scene_id);

	dpu_comp_abnormal_debug_dump(dpu_comp->comp_mgr->dpu_base, displaying_scene_id);
	if (displaying_scene_id != displayed_scene_id) {
		dpu_pr_info("scene_id has changed: %d --> %d", displayed_scene_id, displaying_scene_id);
		dpu_comp_abnormal_debug_dump(dpu_comp->comp_mgr->dpu_base, displayed_scene_id);
	}

	if (g_debug_dpu_clear_enable)
		kthread_queue_work(&dpu_comp->handle_worker, &present->abnormal_handle_work);

	return 0;
}

static struct notifier_block abnormal_handle_isr_notifier = {
	.notifier_call = dpu_comp_abnormal_handle_isr_notify,
};

void dpu_comp_abnormal_handle_init(struct dkmd_isr *isr, struct dpu_composer *dpu_comp, uint32_t listening_bit)
{
	struct comp_online_present *present = (struct comp_online_present *)dpu_comp->present_data;

	kthread_init_work(&present->abnormal_handle_work, dpu_comp_abnormal_handle_work);

	dkmd_isr_register_listener(isr, &abnormal_handle_isr_notifier, listening_bit, dpu_comp);
}

void dpu_comp_abnormal_handle_deinit(struct dkmd_isr *isr, uint32_t listening_bit)
{
	dkmd_isr_unregister_listener(isr, &abnormal_handle_isr_notifier, listening_bit);
}
