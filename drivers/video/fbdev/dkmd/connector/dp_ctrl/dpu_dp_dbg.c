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

#include <linux/module.h>
#include "dpu_dp_dbg.h"
#include "dp_ctrl.h"
#include "dp_aux.h"

/*******************************************************************************
 * DP GRAPHIC DEBUG TOOL
 */

/*
 * for debug, S_IRUGO
 * /sys/module/dp_ctrl/parameters
 */
int g_dp_debug_mode_enable = 0;
module_param_named(dp_debug_mode_enable, g_dp_debug_mode_enable, int, 0644);
MODULE_PARM_DESC(dp_debug_mode_enable, "dp_debug_mode_enable");

int g_dp_lane_num_debug = 4;
module_param_named(dp_lane_num, g_dp_lane_num_debug, int, 0644);
MODULE_PARM_DESC(dp_lane_num, "dp_lane_num");

int g_dp_lane_rate_debug = 2;
module_param_named(dp_lane_rate, g_dp_lane_rate_debug, int, 0644);
MODULE_PARM_DESC(dp_lane_rate, "dp_lane_rate");

int g_dp_ssc_enable_debug = 1;
module_param_named(dp_ssc_enable, g_dp_ssc_enable_debug, int, 0644);
MODULE_PARM_DESC(dp_ssc_enable, "dp_ssc_enable");

int g_dp_fec_enable_debug = 0;
module_param_named(dp_fec_enable, g_dp_fec_enable_debug, int, 0644);
MODULE_PARM_DESC(dp_fec_enable, "dp_fec_enable");

int g_dp_dsc_enable_debug = 0;
module_param_named(dp_dsc_enable, g_dp_dsc_enable_debug, int, 0644);
MODULE_PARM_DESC(dp_dsc_enable, "dp_dsc_enable");

int g_dp_mst_enable_debug = 0;
module_param_named(dp_mst_enable, g_dp_mst_enable_debug, int, 0644);
MODULE_PARM_DESC(dp_mst_enable, "dp_mst_enable");

int g_dp_efm_enable_debug = 1;
module_param_named(dp_efm_enable, g_dp_efm_enable_debug, int, 0644);
MODULE_PARM_DESC(dp_efm_enable, "dp_efm_enable");

uint g_dp_aux_ctrl = 0x33630; // 0x77630;
module_param_named(dp_aux_ctrl, g_dp_aux_ctrl, uint, 0644);
MODULE_PARM_DESC(dp_aux_ctrl, "dp_aux_ctrl");

uint g_pll1ckgctrlr0 = 0x054610df; // 0x054610df
module_param_named(pll1ckgctrlr0, g_pll1ckgctrlr0, uint, 0644);
MODULE_PARM_DESC(pll1ckgctrlr0, "pll1ckgctrlr0");

uint g_pll1ckgctrlr1 = 0x046510da; // 0x046510da
module_param_named(pll1ckgctrlr1, g_pll1ckgctrlr1, uint, 0644);
MODULE_PARM_DESC(pll1ckgctrlr1, "pll1ckgctrlr1");

uint g_pll1ckgctrlr2 = 0x046500d5; // 0x046500d5
module_param_named(pll1ckgctrlr2, g_pll1ckgctrlr2, uint, 0644);
MODULE_PARM_DESC(pll1ckgctrlr2, "pll1ckgctrlr2");

int g_dp_same_source_debug = 1;
module_param_named(dp_same_source_debug, g_dp_same_source_debug, int, 0644);
MODULE_PARM_DESC(dp_same_source_debug, "dp_same_source_debug");

static uint32_t g_dp_preemphasis_swing[DPTX_COMBOPHY_PARAM_NUM];
int g_dp_pe_sw_length = 0;
module_param_array_named(dp_preemphasis_swing, g_dp_preemphasis_swing, uint, &g_dp_pe_sw_length, 0644);
MODULE_PARM_DESC(dp_preemphasis_swing, "dp_preemphasis_swing");

/* pll1sscg_ctrl_initial_r(2), pll1sscg_cnt_r(2), pll1sscg_cnt2_r(2), PLL1SSCG_CTRL */
uint32_t g_hbr2_pll1_sscg[4] = {0x8CA00000, 0x011c0119, 0x000131, 0x7ff0e0d};
int g_hbr2_pll1_sscg_length = 0;
module_param_array_named(hbr2_pll1_sscg, g_hbr2_pll1_sscg, uint, &g_hbr2_pll1_sscg_length, 0644);
MODULE_PARM_DESC(hbr2_pll1_sscg, "hbr2_pll1_sscg");

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

void dptx_debug_set_params(struct dp_ctrl *dptx)
{
	int i;

	dpu_check_and_no_retval(!dptx, err, "[DP] dptx is null pointor!\n");

	if (g_dp_debug_mode_enable == 0)
		return;

	if (dp_lane_num_valid(g_dp_lane_num_debug))
		dptx->max_lanes = (uint8_t)g_dp_lane_num_debug;

	if (dp_lane_rate_valid(g_dp_lane_rate_debug))
		dptx->max_rate = (uint8_t)g_dp_lane_rate_debug;

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

int dp_debug_dptx_connected(void)
{
	if (!g_dptx_debug)
		return 0;

	if (g_dptx_debug->video_transfer_enable)
		return 1;

	return 0;
}
EXPORT_SYMBOL_GPL(dp_debug_dptx_connected);

int dp_debug_read_dpcd(uint32_t addr)
{
	uint8_t byte = 0;
	int retval;

	dpu_check_and_return(!g_dptx_debug, -EINVAL, err, "[DP] dptx is NULL!\n");

	retval = dptx_read_dpcd(g_dptx_debug, addr, &byte);
	if (retval) {
	       dpu_pr_err("[DP] read dpcd fail\n");
	       return retval;
	}
	return (int)byte;
}
EXPORT_SYMBOL_GPL(dp_debug_read_dpcd);

int dp_debug_write_dpcd(uint32_t addr, uint8_t byte)
{
	int retval;

	dpu_check_and_return(!g_dptx_debug, -EINVAL, err, "[DP] dptx is NULL!\n");

	retval = dptx_write_dpcd(g_dptx_debug, addr, byte);
	if (retval) {
	       dpu_pr_err("[DP] write dpcd fail\n");
	       return retval;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(dp_debug_write_dpcd);
