/* Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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

#include "dpu_dp.h"
#include "dpu_fb.h"
#include "dpu_fb_def.h"

#include "hidptx_dp_avgen.h"
#include "hidptx_dp_irq.h"
#include "hidptx_reg.h"
#include "../dp_core_interface.h"
#include "../../link/dp_irq.h"

static int phy_pattern_get(struct dp_ctrl *dptx, const uint8_t pattern, uint32_t *phy_pattern)
{
	switch (pattern) {
	case DPTX_NO_TEST_PATTERN_SEL:
		DPU_FB_INFO("[DP] DPTX_NO_TEST_PATTERN_SEL\n");
		*phy_pattern = DPTX_PHYIF_CTRL_TPS_NONE;
		break;
	case DPTX_D102_WITHOUT_SCRAMBLING:
		DPU_FB_INFO("[DP] DPTX_D102_WITHOUT_SCRAMBLING\n");
		*phy_pattern = DPTX_PHYIF_CTRL_TPS_1;
		break;
	case DPTX_SYMBOL_ERROR_MEASUREMENT_COUNT:
		DPU_FB_INFO("[DP] DPTX_SYMBOL_ERROR_MEASUREMENT_COUNT\n");
		*phy_pattern = DPTX_PHYIF_CTRL_TPS_SYM_ERM;
		break;
	case DPTX_TEST_PATTERN_PRBS7:
		DPU_FB_INFO("[DP] DPTX_TEST_PATTERN_PRBS7\n");
		*phy_pattern = DPTX_PHYIF_CTRL_TPS_PRBS7;
		break;
	case DPTX_80BIT_CUSTOM_PATTERN_TRANS:
		DPU_FB_INFO("[DP] DPTX_80BIT_CUSTOM_PATTERN_TRANS\n");
		dptx_writel(dptx, DPTX_PHYIF_CUSTOM80B_0, 0x3e0f83e0);
		dptx_writel(dptx, DPTX_PHYIF_CUSTOM80B_1, 0x3e0f83e0);
		dptx_writel(dptx, DPTX_PHYIF_CUSTOM80B_2, 0xf83e0);
		*phy_pattern = DPTX_PHYIF_CTRL_TPS_CUSTOM80;
		break;
	case DPTX_CP2520_HBR2_COMPLIANCE_EYE_PATTERN:
		DPU_FB_INFO("[DP] DPTX_CP2520_HBR2_COMPLIANCE_EYE_PATTERN\n");
		*phy_pattern = DPTX_PHYIF_CTRL_TPS_CP2520_1;
		break;
	case DPTX_CP2520_HBR2_COMPLIANCE_PATTERN_2:
		DPU_FB_INFO("[DP] DPTX_CP2520_HBR2_COMPLIANCE_EYE_PATTERN_2\n");
		*phy_pattern = DPTX_PHYIF_CTRL_TPS_CP2520_2;
		break;
	case DPTX_CP2520_HBR2_COMPLIANCE_PATTERN_3:
		DPU_FB_INFO("[DP] DPTX_CP2520_HBR2_COMPLIANCE_EYE_PATTERN_3\n");
		*phy_pattern = DPTX_PHYIF_CTRL_TPS_4;
		break;
	default:
		DPU_FB_INFO("[DP] Invalid TEST_PATTERN\n");
		return -EINVAL;
	}

	return 0;
}

int handle_test_link_phy_pattern(struct dp_ctrl *dptx)
{
	int retval;
	uint8_t pattern;
	uint32_t phy_pattern;

	retval = 0;
	pattern = 0;
	phy_pattern = 0;

	dpu_check_and_return((dptx == NULL), -EINVAL, ERR, "[DP] NULL Pointer\n");

	retval = dptx_read_dpcd(dptx, DP_TEST_PHY_PATTERN, &pattern);
	if (retval != 0)
		return retval;

	pattern &= DPTX_PHY_TEST_PATTERN_SEL_MASK;

	if (phy_pattern_get(dptx, pattern, &phy_pattern) != 0)
		return -EINVAL;

	if (dptx->dptx_phy_set_pattern)
		dptx->dptx_phy_set_pattern(dptx, phy_pattern);

	return 0;
}

irqreturn_t dptx_threaded_irq(int irq, void *dev)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dp_ctrl *dptx = NULL;

	dpu_check_and_return(!dev, IRQ_HANDLED, ERR, "[DP] dev is NULL\n");

	dpufd = dev;

	dptx = &(dpufd->dp);

	return IRQ_HANDLED;
}

void dptx_hpd_handler(struct dp_ctrl *dptx, bool plugin, uint8_t dp_lanes)
{
	int retval;

	dpu_check_and_no_retval((dptx == NULL), ERR, "[DP] NULL Pointer\n");

	DPU_FB_INFO("[DP] DP Plug Type: %d, Lanes: %d\n", plugin, dp_lanes);

	/* need to check dp lanes */
	dptx->max_lanes = dp_lanes;

	if (plugin && dptx->handle_hotplug) {
		retval = dptx->handle_hotplug(dptx->dpufd);
		if (retval != 0)
			DPU_FB_ERR("[DP] hotplug error\n");
	}

	if (!plugin && dptx->handle_hotunplug) {
		retval = dptx->handle_hotunplug(dptx->dpufd);
		if (retval != 0)
			DPU_FB_ERR("[DP] hotunplug error\n");
	}
}

void dptx_hpd_irq_handler(struct dp_ctrl *dptx)
{
	int retval;

	dpu_check_and_no_retval((dptx == NULL), ERR, "[DP] NULL Pointer\n");

	DPU_FB_INFO("[DP] DP Shot Plug!\n");

	if (!dptx->video_transfer_enable) {
		DPU_FB_ERR("[DP] DP never link train success, shot plug is useless!\n");
		return;
	}

	dptx_notify(dptx);
	retval = handle_sink_request(dptx);
	if (retval != 0)
		DPU_FB_ERR("[DP] Unable to handle sink request %d\n", retval);
}

irqreturn_t dptx_irq(int irq, void *dev)
{
	irqreturn_t retval = IRQ_HANDLED;
	(void)irq;
	(void)dev;

	/* disable intr */
	return retval;
}

