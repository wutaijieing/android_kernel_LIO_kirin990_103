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

#include <linux/kernel.h>
#include "dp_phy_config_interface.h"
#include "dp_ctrl.h"
#include "dp_drv.h"
#include "dpu_dp_dbg.h"
#include "drm_dp_helper_additions.h"
#include <linux/platform_drivers/usb/chip_usb.h>

/* PLL */
#define PLL1UPDATECTRL							(0xc00 + 0x10)
#define PLL1UPDATECTRL_RATE_MASK				GENMASK(11, 8)
#define PLL1UPDATECTRL_RATE_OFFSET				8

#define PLL1CKGCTRLR0							(0xc00 + 0x14)
#define PLL1CKGCTRLR1							(0xc00 + 0x1c)
#define PLL1CKGCTRLR2							(0xc00 + 0x24)
#define PLL_LPF_CS_R_MASK						GENMASK(7, 6)
#define PLL_LPF_RS_R_MASK						GENMASK(11, 8)

/* ssc */
#define PLL1SSCG_CTRL							(0xc00 + 0x80)
#define PLL1_SSCG_CNT_STEPSIZE_FORCE_0			BIT(1)

#define pll1sscg_ctrl_initial_r(n)				(0xc00 + 0x10 * (n) + 0x84)
#define pll1sscg_cnt_r(n)						(0xc00 + 0x10 * (n) + 0x88)
#define pll1sscg_cnt2_r(n)						(0xc00 + 0x10 * (n) + 0x8c)

/* pre sw */
#define txdrvctrl(n)							(0x2000 + 0x0400 * (n) + 0x314)
#define txeqcoeff(n)							(0x2000 + 0x0400 * (n) + 0x318)

#define AUX_EYE_LEVEL		0x3
#define AUX_HYS_TUNE_VAL	0x1
#define AUX_CTRL_VAL		0x3
#define DPTX_DP_AUX_CTRL						0x20
/* aux disreset */
#define REG_DP_AUX_PWDNB						BIT(9)
/* eye diagram */
#define AUX_RTERMSEL_MASK						GENMASK(2, 0)
#define AUX_NEN_RTERM_MASK						BIT(3)
#define REG_DP_AUX_AUX_CTRL_MASK				GENMASK(7, 4)
#define AUX_CTRL_OFFSET							4
#define REG_DP_AUX_AUX_HYS_TUNE_MASK			GENMASK(11, 10)
#define AUX_HYS_TUNE_OFFSET						10
#define REG_DP_AUX_RONSELN_MASK					GENMASK(14, 12)
#define AUX_RONSELN_OFFSET						12
#define REG_DP_AUX_RONSELP_MASK					GENMASK(18, 16)
#define AUX_RONSELP_OFFSET						16

/* (plug_type, lane_num) -> (phylane) mapping */
static const struct phylane_mapping g_phylane_mappings[] = {
	{ DP_PLUG_TYPE_NORMAL, 0, 3 },
	{ DP_PLUG_TYPE_NORMAL, 1, 1 },
	{ DP_PLUG_TYPE_NORMAL, 2, 0 },
	{ DP_PLUG_TYPE_NORMAL, 3, 2 },
	{ DP_PLUG_TYPE_FLIPPED, 0, 2 },
	{ DP_PLUG_TYPE_FLIPPED, 1, 0 },
	{ DP_PLUG_TYPE_FLIPPED, 2, 1 },
	{ DP_PLUG_TYPE_FLIPPED, 3, 3 },
};

static int dptx_mapping_phylane(uint32_t lane, int plug_type,
	uint16_t *txdrvctrl, uint16_t *txeqcoeff)
{
	uint32_t i;
	uint8_t phylane = 0;

	for (i = 0; i < ARRAY_SIZE(g_phylane_mappings); i++) {
		if (plug_type == g_phylane_mappings[i].plug_type && lane == g_phylane_mappings[i].lane) {
			phylane = g_phylane_mappings[i].phylane;
			break;
		}
	}

	if (i == ARRAY_SIZE(g_phylane_mappings)) {
		dpu_pr_err("[DP] lane number error!\n");
		return -EINVAL;
	}

	*txdrvctrl = txdrvctrl(phylane);
	*txeqcoeff = txeqcoeff(phylane);

	return 0;
}

static int dptx_config_swing_and_preemphasis(struct dp_ctrl *dptx, uint32_t sw_level, uint32_t pe_level,
	uint16_t txdrvctrl, uint16_t txeqcoeff)
{
	struct dpu_connector *connector = NULL;
	struct dp_private *dp = NULL;
	uint32_t offset = 0;
	uint32_t index0, index1;

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is NULL!\n");

	connector = dptx->connector;
	dpu_check_and_return(!connector, -EINVAL, err, "[DP] dptx is NULL!\n");
	dp = to_dp_private(connector->conn_info);

	/**
	 * @brief The index value is in line with the following rules:
	 * (sw_level, pe_level) --> (0, 0 ~ 3) --> index0=0,2,4,6 index1=1,3,5,7
	 * (sw_level, pe_level) --> (1, 0 ~ 2) --> index0=8,10,12 index1=9,11,13
	 * (sw_level, pe_level) --> (2, 0 ~ 1) --> index0=14,16 index1=15,17
	 * (sw_level, pe_level) --> (3, 0    ) --> index0=18 index1=19
	 */
	if ((sw_level > 3) || (pe_level > 3) || (pe_level > 4 - sw_level)) {
		dpu_pr_err("[DP] Don't support sw %d & ps level %d\n", sw_level, pe_level);
		return -1;
	}

	index0 = sw_level * 8 + pe_level * 2;
	index1 = sw_level * 8 + pe_level * 2 + 1;
	while (sw_level-- > 0)
		offset += sw_level * 2;
	index0 = index0 - offset;
	index1 = index1 - offset;
	outp32(dp->combo_phy_base + txdrvctrl, dptx->combophy_pree_swing[index0]);
	outp32(dp->combo_phy_base + txeqcoeff, dptx->combophy_pree_swing[index1]);

	dpu_pr_info("combophy_pree_swing[%d]: 0x%x, combophy_pree_swing[%d]: 0x%x \n",
		index0, dptx->combophy_pree_swing[index0], index1, dptx->combophy_pree_swing[index1]);

	return 0;
}

void dptx_combophy_set_preemphasis_swing(struct dp_ctrl *dptx,
	uint32_t lane, uint32_t sw_level, uint32_t pe_level)
{
	uint16_t txdrvctrl = 0;
	uint16_t txeqcoeff = 0;
	int ret;

	dpu_check_and_no_retval(!dptx, err, "[DP] dptx is NULL!\n");

	ret = dptx_mapping_phylane(lane, dptx->dptx_plug_type, &txdrvctrl, &txeqcoeff);
	if (ret)
		return;
	ret = dptx_config_swing_and_preemphasis(dptx, sw_level, pe_level, txdrvctrl, txeqcoeff);
	if (ret)
		return;
}

void dptx_aux_disreset(struct dp_ctrl *dptx, bool enable)
{
	uint32_t reg;
	struct dpu_connector *connector = NULL;
	struct dp_private *dp = NULL;

	dpu_check_and_no_retval(!dptx, err, "[DP] NULL Pointer\n");
	connector = dptx->connector;
	dpu_check_and_no_retval(!connector, err, "[DP] connector is NULL!\n");
	dp = to_dp_private(connector->conn_info);

	/* Enable AUX Block */
	if (g_dp_debug_mode_enable) {
		reg = (uint32_t)g_dp_aux_ctrl;
	} else {
		reg = (uint32_t)inp32(dp->combo_phy_base + DPTX_DP_AUX_CTRL);
		if (enable)
			reg |= REG_DP_AUX_PWDNB;
		else
			reg &= ~REG_DP_AUX_PWDNB;

		reg &= ~REG_DP_AUX_RONSELN_MASK;
		reg &= ~REG_DP_AUX_RONSELP_MASK;
		reg |=  AUX_EYE_LEVEL << AUX_RONSELN_OFFSET;
		reg |=  AUX_EYE_LEVEL << AUX_RONSELP_OFFSET;
		reg &= ~REG_DP_AUX_AUX_HYS_TUNE_MASK;
		reg |= (AUX_HYS_TUNE_VAL << AUX_HYS_TUNE_OFFSET);
		reg &= ~REG_DP_AUX_AUX_CTRL_MASK;
		reg |= (AUX_CTRL_VAL << AUX_CTRL_OFFSET);
		reg &= ~AUX_NEN_RTERM_MASK;
		reg &= ~AUX_RTERMSEL_MASK;
	}
	dpu_pr_info("[DP] DPTX_DP_AUX_CTRL config value: 0x%x\n", reg);
	outp32(dp->combo_phy_base + DPTX_DP_AUX_CTRL, reg);
	dpu_pr_info("[DP] DPTX_DP_AUX_CTRL get value: 0x%x\n", inp32(dp->combo_phy_base + DPTX_DP_AUX_CTRL));
	mdelay(1);
}

void dptx_combophy_set_ssc_dis(struct dp_ctrl *dptx, bool ssc_disable)
{
	uint32_t reg;
	struct dpu_connector *connector = NULL;
	struct dp_private *dp = NULL;

	dpu_check_and_no_retval(!dptx, err, "[DP] dptx is NULL!\n");

	connector = dptx->connector;
	dpu_check_and_no_retval(!connector, err, "[DP] connector is NULL!\n");
	dp = to_dp_private(connector->conn_info);

	reg = 0x7ff0e0d; // default value for chip spec
	if (!ssc_disable) {
		reg &= ~PLL1_SSCG_CNT_STEPSIZE_FORCE_0;
		dpu_pr_info("[DP] SSC enable\n");
	} else {
		reg |= PLL1_SSCG_CNT_STEPSIZE_FORCE_0;
		dpu_pr_info("[DP] SSC disable\n");
	}
	if (g_dp_debug_mode_enable)
		reg = g_hbr2_pll1_sscg[3];

	dpu_pr_info("[DP] PLL1SSCG_CTRL config value: 0x%x\n", reg);
	outp32(dp->combo_phy_base + PLL1SSCG_CTRL, reg);
}

void dptx_combophy_set_rate(struct dp_ctrl *dptx, int rate)
{
	uint32_t reg;
	struct dpu_connector *connector = NULL;
	struct dp_private *dp = NULL;

	dpu_check_and_no_retval(!dptx, err, "[DP] dptx is NULL!\n");

	connector = dptx->connector;
	dpu_check_and_no_retval(!connector, err, "[DP] connector is NULL!\n");
	dp = to_dp_private(connector->conn_info);

	switch (rate) {
	case DPTX_PHYIF_CTRL_RATE_RBR:
		if (g_dp_debug_mode_enable)
			reg = (uint32_t)g_pll1ckgctrlr0;
		else
			reg = 0x054610df; // default value for chip spec
		dpu_pr_info("[DP] PLL1CKGCTRLR0 config value: 0x%x\n", reg);
		outp32(dp->combo_phy_base + PLL1CKGCTRLR0, reg);
		dpu_pr_info("[DP] PLL1CKGCTRLR0 get value: 0x%x\n", inp32(dp->combo_phy_base + PLL1CKGCTRLR0));
		break;
	case DPTX_PHYIF_CTRL_RATE_HBR:
		if (g_dp_debug_mode_enable)
			reg = (uint32_t)g_pll1ckgctrlr1;
		else
			reg = 0x046510da; // default value for chip spec
		dpu_pr_info("[DP] PLL1CKGCTRLR1 config value: 0x%x\n", reg);
		outp32(dp->combo_phy_base + PLL1CKGCTRLR1, reg);
		dpu_pr_info("[DP] PLL1CKGCTRLR1 get value: 0x%x\n", inp32(dp->combo_phy_base + PLL1CKGCTRLR1));
		break;
	case DPTX_PHYIF_CTRL_RATE_HBR2:
		if (g_dp_debug_mode_enable) {
			reg = (uint32_t)g_pll1ckgctrlr2;
			outp32(dp->combo_phy_base + pll1sscg_ctrl_initial_r(2), g_hbr2_pll1_sscg[0]);
			outp32(dp->combo_phy_base + pll1sscg_cnt_r(2), g_hbr2_pll1_sscg[1]);
			outp32(dp->combo_phy_base + pll1sscg_cnt2_r(2), g_hbr2_pll1_sscg[2]);
		} else {
			reg = 0x046500d5; // default value for chip spec
			outp32(dp->combo_phy_base + pll1sscg_ctrl_initial_r(2), 0x8CA00000); // default value for chip spec
			outp32(dp->combo_phy_base + pll1sscg_cnt_r(2), 0x011c0119); // default value for chip spec
			outp32(dp->combo_phy_base + pll1sscg_cnt2_r(2), 0x000131); // default value for chip spec
		}
		dpu_pr_info("[DP] PLL1CKGCTRLR2 config value: 0x%x\n", reg);
		outp32(dp->combo_phy_base + PLL1CKGCTRLR2, reg);
		dpu_pr_info("[DP] PLL1CKGCTRLR2 get value: 0x%x\n", inp32(dp->combo_phy_base + PLL1CKGCTRLR2));
		break;
	case DPTX_PHYIF_CTRL_RATE_HBR3:
		break;
	default:
		dpu_pr_err("[DP] Invalid PHY rate %d\n", rate);
		return;
	}
}

void dptx_phy_layer_init(struct dp_ctrl *dptx)
{
	dpu_check_and_no_retval(!dptx, err, "[DP] dptx is NULL!\n");

	dptx->dptx_combophy_set_preemphasis_swing = dptx_combophy_set_preemphasis_swing;
	dptx->dptx_change_physetting_hbr2 = NULL;
	dptx->dptx_combophy_set_ssc_addtions = NULL;
	dptx->dptx_combophy_set_lanes_power = NULL;
	dptx->dptx_combophy_set_ssc_dis = dptx_combophy_set_ssc_dis;
	dptx->dptx_aux_disreset = dptx_aux_disreset;
	dptx->dptx_combophy_set_rate = dptx_combophy_set_rate;
}
