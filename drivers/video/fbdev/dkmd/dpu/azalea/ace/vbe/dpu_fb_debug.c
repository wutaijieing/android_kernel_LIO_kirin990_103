/* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#include "dpu_fb.h"
#include "overlay/init/dpu_init.h"
#include "overlay/dump/dpu_dump.h"

/*
 * for debug, S_IRUGO
 * /sys/module/dpufb/parameters
 */

unsigned int g_dpu_fb_msg_level = 7;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_msg_level, g_dpu_fb_msg_level, int, 0644);
MODULE_PARM_DESC(debug_msg_level, "dpu fb msg level");
#endif

uint32_t g_dump_each_rda_frame;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(dump_each_rda_frame, g_dump_each_rda_frame, int, 0644);
MODULE_PARM_DESC(dump_each_rda_frame, "dump_each_rda_frame");
#endif

uint32_t g_dump_each_non_rda_frame;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(dump_each_non_rda_frame, g_dump_each_non_rda_frame, int, 0644);
MODULE_PARM_DESC(dump_each_non_rda_frame, "dump_each_non_rda_frame");
#endif

uint32_t g_dump_iova_page;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(dump_iova_page, g_dump_iova_page, int, 0644);
MODULE_PARM_DESC(dump_iova_page, "g_dump_iova_page");
#endif

uint32_t g_enable_ov_base;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(enable_ov_base, g_enable_ov_base, int, 0644);
MODULE_PARM_DESC(g_enable_ov_base, "g_enable_ov_base debug");
#endif

uint32_t g_dsi_pipe_switch_connector;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_dsi_pipe_switch_connector, g_dsi_pipe_switch_connector, int, 0644);
MODULE_PARM_DESC(g_dsi_pipe_switch_connector, "g_dsi_pipe_switch_connector debug");
#endif

int g_dss_perf_debug;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(dss_perf_debug, g_dss_perf_debug, int, 0644);
MODULE_PARM_DESC(dss_perf_debug, "dss_perf_debug");
#endif

uint32_t g_dss_dfr_debug;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(dss_dfr_debug, g_dss_dfr_debug, int, 0644);
MODULE_PARM_DESC(dss_dfr_debug, "dss_dfr_debug");
#endif

int g_debug_mmu_error;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_mmu_error, g_debug_mmu_error, int, 0644);
MODULE_PARM_DESC(debug_mmu_error, "dpu mmu error debug");
#endif

int g_debug_underflow_error = 1;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_underflow_error, g_debug_underflow_error, int, 0644);
MODULE_PARM_DESC(debug_underflow_error, "dpu underflow error debug");
#endif

int g_debug_ldi_underflow;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_ldi_underflow, g_debug_ldi_underflow, int, 0644);
MODULE_PARM_DESC(debug_ldi_underflow, "dpu ldi_underflow debug");
#endif

int g_debug_ldi_underflow_clear = 1;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_ldi_underflow_clear, g_debug_ldi_underflow_clear, int, 0644);
MODULE_PARM_DESC(debug_ldi_underflow_clear, "dpu ldi_underflow_clear debug");
#endif

int g_debug_panel_mode_switch;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_panel_mode_switch, g_debug_panel_mode_switch, int, 0644);
MODULE_PARM_DESC(debug_panel_mode_switch, "dpu panel_mode_switch debug");
#endif

int g_debug_set_reg_val;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_set_reg_val, g_debug_set_reg_val, int, 0644);
MODULE_PARM_DESC(debug_set_reg_val, "dpu set reg val debug");
#endif

int g_debug_online_vsync;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_online_vsync, g_debug_online_vsync, int, 0644);
MODULE_PARM_DESC(debug_online_vsync, "dpu online vsync debug");
#endif

int g_debug_online_vactive;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_online_vactive, g_debug_online_vactive, int, 0644);
MODULE_PARM_DESC(debug_online_vactive, "dpu online vactive debug");
#endif

int g_debug_ovl_online_composer;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_ovl_online_composer, g_debug_ovl_online_composer, int, 0644);
MODULE_PARM_DESC(debug_ovl_online_composer, "dpu overlay online composer debug");
#endif

int g_debug_ovl_online_composer_hold;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_ovl_online_composer_hold, g_debug_ovl_online_composer_hold, int, 0644);
MODULE_PARM_DESC(debug_ovl_online_composer_hold, "dpu overlay online composer hold debug");
#endif

int g_debug_ovl_online_composer_return;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_ovl_online_composer_return, g_debug_ovl_online_composer_return, int, 0644);
MODULE_PARM_DESC(debug_ovl_online_composer_return, "dpu overlay online composer return debug");
#endif

uint32_t g_debug_ovl_online_composer_timediff;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_ovl_online_composer_timediff, g_debug_ovl_online_composer_timediff, int, 0644);
MODULE_PARM_DESC(debug_ovl_online_composer_timediff, "dpu overlay online composer timediff debug");
#endif

uint32_t g_online_comp_timeout = 20000;  /* us */
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(online_comp_timeout, g_online_comp_timeout, int, 0644);
MODULE_PARM_DESC(debug_ovl_online_composer_time_threshold, "dpu overlay online composer time threshold debug");
#endif

uint32_t g_exec_online_threshold = 20000;  /* us */
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(online_exec_online_threshold, g_exec_online_threshold, int, 0644);
MODULE_PARM_DESC(online_exec_online_threshold, "dpu online exec online threshold debug");
#endif

int g_debug_ovl_offline_composer;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_ovl_offline_composer, g_debug_ovl_offline_composer, int, 0644);
MODULE_PARM_DESC(debug_ovl_offline_composer, "dpu overlay offline composer debug");
#endif

int g_debug_ovl_block_composer;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_ovl_block_composer, g_debug_ovl_block_composer, int, 0644);
MODULE_PARM_DESC(debug_ovl_block_composer, "dpu overlay block composer debug");
#endif

int g_debug_ovl_offline_composer_hold;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_ovl_offline_composer_hold, g_debug_ovl_offline_composer_hold, int, 0644);
MODULE_PARM_DESC(debug_ovl_offline_composer_hold, "dpu overlay offline composer hold debug");
#endif

uint32_t g_debug_ovl_offline_composer_timediff;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_ovl_offline_composer_timediff, g_debug_ovl_offline_composer_timediff, int, 0644);
MODULE_PARM_DESC(debug_ovl_offline_composer_timediff, "dpu overlay offline composer timediff debug");
#endif

int g_debug_ovl_offline_composer_time_threshold = 12000;  /* us */
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_ovl_offline_composer_time_threshold, g_debug_ovl_offline_composer_time_threshold, int, 0644);
MODULE_PARM_DESC(debug_ovl_offline_composer_time_threshold, "dpu overlay offline composer time threshold debug");
#endif

int g_debug_ovl_offline_block_num = -1;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_ovl_offline_block_num, g_debug_ovl_offline_block_num, int, 0644);
MODULE_PARM_DESC(debug_ovl_offline_block_num, "dpu overlay offline composer block debug");
#endif

int g_enable_ovl_async_composer = 1;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(enable_ovl_async_composer, g_enable_ovl_async_composer, int, 0644);
MODULE_PARM_DESC(enable_ovl_async_composer, "dpu overlay async composer debug");
#endif

int g_debug_ovl_mediacommon_composer;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_ovl_mediacommon_composer, g_debug_ovl_mediacommon_composer, int, 0644);
MODULE_PARM_DESC(debug_ovl_mediacommon_composer, "dpu overlay media common composer debug");
#endif

int g_debug_ovl_cmdlist;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_ovl_cmdlist, g_debug_ovl_cmdlist, int, 0644);
MODULE_PARM_DESC(debug_ovl_cmdlist, "dpu overlay cmdlist debug");
#endif

int g_dump_cmdlist_content;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(dump_cmdlist_content, g_dump_cmdlist_content, int, 0644);
MODULE_PARM_DESC(dump_cmdlist_content, "dpu overlay dump cmdlist content");
#endif

int g_enable_ovl_cmdlist_online;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(enable_ovl_cmdlist_online, g_enable_ovl_cmdlist_online, int, 0644);
MODULE_PARM_DESC(enable_ovl_cmdlist_online, "dpu overlay cmdlist online enable");
#endif

int g_smmu_global_bypass;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(smmu_global_bypass, g_smmu_global_bypass, int, 0644);
MODULE_PARM_DESC(smmu_global_bypass, "dpu smmu_global_bypass");
#endif

int g_enable_ovl_cmdlist_offline = 1;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(enable_ovl_cmdlist_offline, g_enable_ovl_cmdlist_offline, int, 0644);
MODULE_PARM_DESC(enable_ovl_cmdlist_offline, "dpu overlay cmdlist offline enable");
#endif

int g_rdma_stretch_threshold = RDMA_STRETCH_THRESHOLD;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(rdma_stretch_threshold, g_rdma_stretch_threshold, int, 0644);
MODULE_PARM_DESC(rdma_stretch_threshold, "dpu rdma stretch threshold");
#endif

int g_enable_alsc = 1;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(enable_alsc, g_enable_alsc, int, 0644);
MODULE_PARM_DESC(enable_alsc, "dpu dss alsc enable");
#endif

int g_enable_mmbuf_debug;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(enable_mmbuf_debug, g_enable_mmbuf_debug, int, 0644);
MODULE_PARM_DESC(enable_mmbuf_debug, "dpu dss mmbuf debug enable");
#endif

int g_ldi_data_gate_en;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(enable_ldi_data_gate, g_ldi_data_gate_en, int, 0644);
MODULE_PARM_DESC(enable_ldi_data_gate, "dpu dss ldi data gate enable");
#endif

int g_debug_ovl_credit_step;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_ovl_credit_step, g_debug_ovl_credit_step, int, 0644);
MODULE_PARM_DESC(debug_ovl_credit_step, "dpu overlay debug_ovl_credit_step");
#endif

int g_debug_layerbuf_sync;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_layerbuf_sync, g_debug_layerbuf_sync, int, 0644);
MODULE_PARM_DESC(debug_layerbuf_sync, "dpu dss debug_layerbuf_sync");
#endif

int g_debug_fence_timeline;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_fence_timeline, g_debug_fence_timeline, int, 0644);
MODULE_PARM_DESC(debug_fence_timeline, "dpu dss debug_fence_timeline");
#endif

int g_enable_dss_idle = 1;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(enable_dss_idle, g_enable_dss_idle, int, 0644);
MODULE_PARM_DESC(enable_dss_idle, "dpu dss enable_dss_idle");
#endif

unsigned int g_dss_smmu_outstanding = DSS_SMMU_OUTSTANDING_VAL + 1;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(dss_smmu_outstanding, g_dss_smmu_outstanding, int, 0644);
MODULE_PARM_DESC(dss_smmu_outstanding, "dpu dss smmu outstanding");
#endif

int g_debug_dump_mmbuf;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_dump_mmbuf, g_debug_dump_mmbuf, int, 0644);
MODULE_PARM_DESC(debug_dump_mmbuf, "dpu dump mmbuf debug");
#endif

int g_debug_dump_iova;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_dump_iova, g_debug_dump_iova, int, 0644);
MODULE_PARM_DESC(debug_dump_iova, "dpu dump iova debug");
#endif

uint32_t g_underflow_stop_perf_stat;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(underflow_stop_perf, g_underflow_stop_perf_stat, int, 0600);
MODULE_PARM_DESC(underflow_stop_perf, "dpu underflow stop perf stat");
#endif

uint32_t g_dss_min_bandwidth_inbusbusy = 200; /* 200M */
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(dss_min_bandwidth_inbusbusy, g_dss_min_bandwidth_inbusbusy, int, 0644);
MODULE_PARM_DESC(dss_min_bandwidth_inbusbusy, "dpu overlay dss_min_bandwidth_inbusbusy");
#endif

uint32_t g_mmbuf_addr_test;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(mmbuf_addr_test, g_mmbuf_addr_test, int, 0600);
MODULE_PARM_DESC(mmbuf_addr_test, "dpu mmbuf addr test");
#endif

uint32_t g_dump_sensorhub_aod_hwlock;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(dump_sensorhub_aod_hwlock, g_dump_sensorhub_aod_hwlock, int, 0644);
MODULE_PARM_DESC(dump_sensorhub_aod_hwlock, "dpu dump sensorhub aod hwlock");
#endif

int g_dss_effect_sharpness1D_en = 1;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(dss_effect_sharpness1D_en, g_dss_effect_sharpness1D_en, int, 0644);
MODULE_PARM_DESC(dss_effect_sharpness1D_en, "dpu dss display effect sharpness1D");
#endif

int g_dss_effect_sharpness2D_en;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(dss_effect_sharpness2D_en, g_dss_effect_sharpness2D_en, int, 0644);
MODULE_PARM_DESC(dss_effect_sharpness2D_en, "dpu dss display effect sharpness2D");
#endif

int g_dss_effect_acm_ce_en = 1;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(dss_effect_acm_ce_en, g_dss_effect_acm_ce_en, int, 0644);
MODULE_PARM_DESC(dss_effect_acm_ce_en, "dpu dss display effect acm ce");
#endif

int g_debug_online_play_bypass;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_online_play_bypass, g_debug_online_play_bypass, int, 0644);
MODULE_PARM_DESC(debug_online_play_bypass, "dpu online play bypass debug");
#endif

/* wait asynchronous vactive0 start threshold 300ms */
uint32_t g_debug_wait_asy_vactive0_thr = 300;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_wait_asy_vactive0_start_thr, g_debug_wait_asy_vactive0_thr, int, 0644);
MODULE_PARM_DESC(debug_wait_asy_vactive0_start_thr, "dpu dss asynchronous vactive0 start threshold");
#endif

int g_debug_dpp_cmdlist_dump;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_dpp_cmdlist_dump, g_debug_dpp_cmdlist_dump, int, 0644);
MODULE_PARM_DESC(debug_dpp_cmdlist_dump, "dpu dpp cmdlist dump debug");
#endif

int g_debug_dpp_cmdlist_type = 1;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_dpp_cmdlist_type, g_debug_dpp_cmdlist_type, int, 0644);
MODULE_PARM_DESC(debug_dpp_cmdlist_type, "dpu dpp cmdlist type debug");
#endif

int g_debug_dpp_cmdlist_debug = 0;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_dpp_cmdlist_debug, g_debug_dpp_cmdlist_debug, int, 0644);
MODULE_PARM_DESC(debug_dpp_cmdlist_debug, "dpu dpp cmdlist debug config flow");
#endif

int g_debug_effect_hihdr = 0;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_effect_hihdr, g_debug_effect_hihdr, int, 0644);
MODULE_PARM_DESC(debug_effect_hihdr, "dpu hihdr debug config flow");
#endif

int g_debug_mipi_phy_config = 0;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_mipi_phy_config, g_debug_mipi_phy_config, int, 0644);
MODULE_PARM_DESC(debug_mipi_phy_config, "dpu mipi phy debug config switch");
#endif

uint32_t g_debug_isr_ioctl_timeout = 20000;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_isr_ioctl_timeout, g_debug_isr_ioctl_timeout, int, 0644);
MODULE_PARM_DESC(debug_isr_ioctl_timeout, "dpu isr ioctl timecost debug");
#endif

void dpu_underflow_dump_cmdlist(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req_prev, dss_overlay_t *pov_req_prev_prev)
{
	uint32_t cmdlist_idxs_prev = 0;
	uint32_t cmdlist_idxs_prev_prev = 0;

	if ((g_debug_underflow_error) && (g_underflow_count < DSS_UNDERFLOW_COUNT)) {
		if (pov_req_prev_prev != NULL) {
			(void)dpu_cmdlist_get_cmdlist_idxs(pov_req_prev_prev, &cmdlist_idxs_prev_prev, NULL);
			dump_dss_overlay(dpufd, pov_req_prev_prev);
			dpu_cmdlist_dump_all_node(dpufd, NULL, cmdlist_idxs_prev_prev);
		}

		if (pov_req_prev != NULL) {
			(void)dpu_cmdlist_get_cmdlist_idxs(pov_req_prev, &cmdlist_idxs_prev, NULL);
			dump_dss_overlay(dpufd, pov_req_prev);
			dpu_cmdlist_dump_all_node(dpufd, NULL, cmdlist_idxs_prev);
		}
	}
}
