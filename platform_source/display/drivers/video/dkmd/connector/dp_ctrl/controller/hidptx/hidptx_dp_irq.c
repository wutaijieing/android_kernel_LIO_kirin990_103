/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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

#include <linux/time.h>

#include "dp_ctrl.h"
#include "hidptx_dp_avgen.h"
#include "hidptx_dp_core.h"
#include "hidptx_dp_irq.h"
#include "hidptx_reg.h"
#include "dp_core_interface.h"
#include "dp_irq.h"
#include "dp_aux.h"
#include "drm_dp_helper_additions.h"
#include "dp_drv.h"

/*lint -save -e* */
int handle_test_link_phy_pattern(struct dp_ctrl *dptx)
{
	int retval;
	uint8_t pattern;
	uint32_t phy_pattern;

	retval = 0;
	pattern = 0;
	phy_pattern = 0;

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] null pointer");

	retval = dptx_read_dpcd(dptx, DP_TEST_PHY_PATTERN, &pattern);
	if (retval)
		return retval;

	pattern &= DPTX_PHY_TEST_PATTERN_SEL_MASK;

	switch (pattern) {
	case DPTX_NO_TEST_PATTERN_SEL:
		dpu_pr_info("[DP] DPTX_NO_TEST_PATTERN_SEL");
		phy_pattern = DPTX_PHYIF_CTRL_TPS_NONE;
		break;
	case DPTX_D102_WITHOUT_SCRAMBLING:
		dpu_pr_info("[DP] DPTX_D102_WITHOUT_SCRAMBLING");
		phy_pattern = DPTX_PHYIF_CTRL_TPS_1;
		break;
	case DPTX_SYMBOL_ERROR_MEASUREMENT_COUNT:
		dpu_pr_info("[DP] DPTX_SYMBOL_ERROR_MEASUREMENT_COUNT");
		phy_pattern = DPTX_PHYIF_CTRL_TPS_SYM_ERM;
		break;
	case DPTX_TEST_PATTERN_PRBS7:
		dpu_pr_info("[DP] DPTX_TEST_PATTERN_PRBS7");
		phy_pattern = DPTX_PHYIF_CTRL_TPS_PRBS7;
		break;
	case DPTX_80BIT_CUSTOM_PATTERN_TRANS:
		dpu_pr_info("[DP] DPTX_80BIT_CUSTOM_PATTERN_TRANS");
		dptx_writel(dptx, DPTX_PHYIF_CUSTOM80B_0, 0x3e0f83e0);
		dptx_writel(dptx, DPTX_PHYIF_CUSTOM80B_1, 0x3e0f83e0);
		dptx_writel(dptx, DPTX_PHYIF_CUSTOM80B_2, 0xf83e0);
		phy_pattern = DPTX_PHYIF_CTRL_TPS_CUSTOM80;
		break;
	case DPTX_CP2520_HBR2_COMPLIANCE_EYE_PATTERN:
		dpu_pr_info("[DP] DPTX_CP2520_HBR2_COMPLIANCE_EYE_PATTERN");
		phy_pattern = DPTX_PHYIF_CTRL_TPS_CP2520_1;
		break;
	case DPTX_CP2520_HBR2_COMPLIANCE_PATTERN_2:
		dpu_pr_info("[DP] DPTX_CP2520_HBR2_COMPLIANCE_EYE_PATTERN_2");
		phy_pattern = DPTX_PHYIF_CTRL_TPS_CP2520_2;
		break;
	case DPTX_CP2520_HBR2_COMPLIANCE_PATTERN_3:
		dpu_pr_info("[DP] DPTX_CP2520_HBR2_COMPLIANCE_EYE_PATTERN_3");
		phy_pattern = DPTX_PHYIF_CTRL_TPS_4;
		break;
	default:
		dpu_pr_info("[DP] Invalid TEST_PATTERN");
		return -EINVAL;
	}

	if (dptx->dptx_phy_set_pattern)
		dptx->dptx_phy_set_pattern(dptx, phy_pattern);

	return 0;
}

void dptx_hpd_handler(struct dp_ctrl *dptx, bool plugin, uint8_t dp_lanes)
{
	int retval = 0;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	dpu_pr_info("[DP] DP Plug Type: %d, Lanes: %d", plugin, dp_lanes);

	/* need to check dp lanes */
	dptx->max_lanes = dp_lanes;
	if (plugin && dptx->handle_hotplug) {
		retval = dptx->handle_hotplug(dptx);
		if (retval)
			dpu_pr_err("[DP] hotplug error");
	}

	if (!plugin && dptx->handle_hotunplug) {
		retval = dptx->handle_hotunplug(dptx);
		if (retval)
			dpu_pr_err("[DP] hotunplug error");
	}
}

void dptx_hpd_irq_handler(struct dp_ctrl *dptx)
{
	int retval = 0;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	dpu_pr_info("[DP] DP Short Plug!");

	if (!dptx->video_transfer_enable) {
		dpu_pr_err("[DP] DP never link train success, shot plug is useless!");
		return;
	}

	dptx_notify(dptx);

	retval = handle_sink_request(dptx);
	if (retval)
		dpu_pr_err("[DP] Unable to handle sink request %d", retval);
}

/*lint -restore*/
