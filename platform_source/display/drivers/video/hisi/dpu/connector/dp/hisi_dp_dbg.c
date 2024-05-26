/* Copyright (c) 2019-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#include <linux/module.h>
#include "hisi_dp_dbg.h"
#include "hisi_drv_dp.h"

/*******************************************************************************
 * DP GRAPHIC DEBUG TOOL
 */

/*
 * for debug, S_IRUGO
 * /sys/module/hisifb/parameters
 */
int g_dp_debug_mode_enable = 0;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(dp_debug_mode_enable, g_dp_debug_mode_enable, int, 0644);
MODULE_PARM_DESC(dp_debug_mode_enable, "dp_debug_mode_enable");
#endif

int g_dp_lane_num_debug = 4;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(dp_lane_num, g_dp_lane_num_debug, int, 0644);
MODULE_PARM_DESC(dp_lane_num, "dp_lane_num");
#endif

int g_dp_lane_rate_debug = 2;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(dp_lane_rate, g_dp_lane_rate_debug, int, 0644);
MODULE_PARM_DESC(dp_lane_rate, "dp_lane_rate");
#endif

int g_dp_ssc_enable_debug = 0;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(dp_ssc_enable, g_dp_ssc_enable_debug, int, 0644);
MODULE_PARM_DESC(dp_ssc_enable, "dp_ssc_enable");
#endif

int g_dp_fec_enable_debug = 0;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(dp_fec_enable, g_dp_fec_enable_debug, int, 0644);
MODULE_PARM_DESC(dp_fec_enable, "dp_fec_enable");
#endif

int g_dp_dsc_enable_debug = 0;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(dp_dsc_enable, g_dp_dsc_enable_debug, int, 0644);
MODULE_PARM_DESC(dp_dsc_enable, "dp_dsc_enable");
#endif

int g_dp_mst_enable_debug = 0;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(dp_mst_enable, g_dp_mst_enable_debug, int, 0644);
MODULE_PARM_DESC(dp_mst_enable, "dp_mst_enable");
#endif

int g_dp_efm_enable_debug = 1;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(dp_efm_enable, g_dp_efm_enable_debug, int, 0644);
MODULE_PARM_DESC(dp_efm_enable, "dp_efm_enable");
#endif

int g_dp_aux_ronselp = 1;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(dp_aux_ronselp, g_dp_aux_ronselp, int, 0644);
MODULE_PARM_DESC(dp_aux_ronselp, "dp_aux_ronselp");
#endif

int g_dp_user_format_debug = 0;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(dp_user_timing_format, g_dp_user_format_debug, int, 0644);
MODULE_PARM_DESC(dp_user_timing_format, "dp_user_timing_format");
#endif

int g_dp_user_code_debug = 3;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(dp_user_timing_code, g_dp_user_code_debug, int, 0644);
MODULE_PARM_DESC(dp_user_timing_code, "dp_user_timing_code");
#endif

#ifdef CONFIG_DP_GRAPHIC_DEBUG_TOOL

static uint32_t g_dp_preemphasis_swing[DPTX_COMBOPHY_PARAM_NUM];
int g_dp_pe_sw_length = 0;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_array_named(dp_preemphasis_swing, g_dp_preemphasis_swing, uint, &g_dp_pe_sw_length, 0644);
MODULE_PARM_DESC(dp_preemphasis_swing, "dp_preemphasis_swing");
#endif

static struct dp_ctrl *g_dptx_debug = NULL;

void dp_graphic_debug_node_init(struct dp_ctrl *dptx)
{
	g_dptx_debug = dptx;
	g_dp_debug_mode_enable = 0;
}

static bool dp_lane_num_valid(int dp_lane_num_debug)
{
	if (dp_lane_num_debug == 1 || dp_lane_num_debug == 2 || dp_lane_num_debug == 4)
		return true;

	return false;
}

static bool dp_lane_rate_valid(int dp_lane_rate_debug)
{
	if (dp_lane_rate_debug >= 0 && dp_lane_rate_debug <= 3)
		return true;

	return false;
}

void dp_graphic_debug_set_debug_params(struct dp_ctrl *dptx)
{
	int i;

	disp_check_and_no_retval(!dptx, err, "[DP] dptx is NULL\n");
	if (g_dp_debug_mode_enable) {
		if (dp_lane_num_valid(g_dp_lane_num_debug))
			dptx->max_lanes = g_dp_lane_num_debug;
		if (dp_lane_rate_valid(g_dp_lane_rate_debug))
			dptx->max_rate = g_dp_lane_rate_debug;
		dptx->mst = (bool)g_dp_mst_enable_debug;
		dptx->ssc_en = (bool)g_dp_ssc_enable_debug;
		dptx->fec = (bool)g_dp_fec_enable_debug;
		dptx->dsc = (bool)g_dp_dsc_enable_debug;
		dptx->efm_en = (bool)g_dp_efm_enable_debug;
		if (g_dp_pe_sw_length == DPTX_COMBOPHY_PARAM_NUM) {
			for (i = 0; i < g_dp_pe_sw_length; i++)
				dptx->combophy_pree_swing[i] = g_dp_preemphasis_swing[i];
		}
	}
}

int dp_debug_dptx_connected(void)
{
	if (g_dptx_debug == NULL)
		return 0;

	if (g_dptx_debug->video_transfer_enable)
		return 1;

	return 0;
}
EXPORT_SYMBOL_GPL(dp_debug_dptx_connected);

int dp_debug_choose_timing(void)
{
	if (g_dp_debug_mode_enable) {
		if (g_dp_user_format_debug >= VCEA && g_dp_user_format_debug <= DMT
			&& g_dp_user_code_debug >= 0)
			hisi_dptx_switch_source(g_dp_user_code_debug, g_dp_user_format_debug);
	}
	return 0;
}
EXPORT_SYMBOL_GPL(dp_debug_choose_timing);
#endif

/*******************************************************************************
 * Interface provided by cbg, include data monitor, event report, and etc.
 */
#ifdef HUAWEI_DP_DSM_ENABLE
void huawei_dp_imonitor_set_param(enum dp_imonitor_param param, void *data)
{
	dp_imonitor_set_param(param, data);
}

void huawei_dp_imonitor_set_param_aux_rw(bool rw,
	bool i2c,
	uint32_t addr,
	uint32_t len,
	int retval)
{
	dp_imonitor_set_param_aux_rw(rw, i2c, addr, len, retval);
}

void huawei_dp_imonitor_set_param_timing(uint16_t h_active,
	uint16_t v_active,
	uint32_t pixel_clock,
	uint8_t vesa_id)
{
	dp_imonitor_set_param_timing(h_active, v_active, pixel_clock, vesa_id);
}

void huawei_dp_imonitor_set_param_err_count(uint16_t lane0_err,
	uint16_t lane1_err,
	uint16_t lane2_err,
	uint16_t lane3_err)
{
	dp_imonitor_set_param_err_count(lane0_err, lane1_err, lane2_err, lane3_err);
}

void huawei_dp_imonitor_set_param_vs_pe(int index, uint8_t *vs, uint8_t *pe)
{
	dp_imonitor_set_param_vs_pe(index, vs, pe);
}

void huawei_dp_imonitor_set_param_resolution(uint8_t *user_mode, uint8_t *user_format)
{
	dp_imonitor_set_param_resolution(user_mode, user_format);
}

void huawei_dp_set_dptx_vr_status(bool dptx_vr)
{
	dp_set_dptx_vr_status(dptx_vr);
}

bool huawei_dp_factory_mode_is_enable(void)
{
	return dp_factory_mode_is_enable();
}

void huawei_dp_factory_link_cr_or_ch_eq_fail(bool is_cr)
{
	dp_factory_link_cr_or_ch_eq_fail(is_cr);
}

bool huawei_dp_factory_is_4k_60fps(uint8_t rate,
	uint8_t lanes,
	uint16_t h_active,
	uint16_t v_active,
	uint8_t fps)
{
	return dp_factory_is_4k_60fps(rate, lanes, h_active, v_active, fps);
}

int huawei_dp_get_current_dp_source_mode(void)
{
	return get_current_dp_source_mode();
}

int huawei_dp_update_external_display_timming_info(uint32_t width,
	uint32_t high,
	uint32_t fps)
{
	return update_external_display_timming_info(width, high, fps);
}

void huawei_dp_send_event(enum dp_event_type event)
{
	dp_send_event(event);
}

void huawei_dp_debug_init_combophy_pree_swing(uint32_t *pv, int count)
{
	dp_debug_init_combophy_pree_swing(pv, count);
}
#endif /* HUAWEI_DP_DSM_ENABLE */
