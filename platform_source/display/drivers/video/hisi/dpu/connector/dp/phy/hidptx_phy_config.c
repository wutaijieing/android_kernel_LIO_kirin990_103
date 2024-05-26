/* Copyright (c) 2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#include "dp_phy_config_interface.h"
#include "hisi_drv_dp.h"
#include <linux/platform_drivers/usb/chip_usb.h>

#define AUX_EYE_LEVEL	0x1

#define PLL1UPDATECTRL							(0xc00 + 0x10)
#define PLL1UPDATECTRL_RATE_MASK				GENMASK(11, 8)
#define PLL1UPDATECTRL_RATE_OFFSET				8

/* PLL */
#define PLL1CKGCTRLR0							(0xc00 + 0x14)
#define PLL1CKGCTRLR1							(0xc00 + 0x1c)
#define PLL1CKGCTRLR2							(0xc00 + 0x24)
#define PLL_LPF_RS_R_MASK						GENMASK(11, 8)

/* ssc */
#define PLL1SSCG_CTRL							(0xc00 + 0x80)
#define PLL1_SSCG_CNT_STEPSIZE_FORCE_0			BIT(1)

#define PLL1SSCG_CTRL_INITIAL_R(n)				(0xc00 + 0x10 * (n) + 0x84)
#define PLL1SSCG_CNT_R(n)						(0xc00 + 0x10 * (n) + 0x88)
#define PLL1SSCG_CNT2_R(n)						(0xc00 + 0x10 * (n) + 0x8c)

/* pre sw */
#define TXDRVCTRL(n)							(0x2000 + 0x0400 * (n) + 0x314)
#define TXEQCOEFF(n)							(0x2000 + 0x0400 * (n) + 0x318)

#define DPTX_DP_AUX_CTRL						0X20
/* aux disreset */
#define REG_DP_AUX_PWDNB						BIT(9)
/* eye diagram */
#define REG_DP_AUX_RONSELN_MASK					GENMASK(14, 12)
#define REG_DP_AUX_RONSELP_MASK					GENMASK(18, 16)
#define AUX_RONSELN_OFFSET					12
#define AUX_RONSELP_OFFSET					16
#define AUX_NEN_RTERM_MASK					BIT(3)
#define AUX_RTERMSEL_MASK					GENMASK(2, 0)

/* (plug_type, lane_num) -> (phylane) mapping */
static const struct phylane_mapping phylane_mappings[] = {
	{DP_PLUG_TYPE_NORMAL, 0, 3},
	{DP_PLUG_TYPE_NORMAL, 1, 1},
	{DP_PLUG_TYPE_NORMAL, 2, 0},
	{DP_PLUG_TYPE_NORMAL, 3, 2},
	{DP_PLUG_TYPE_FLIPPED, 0, 2},
	{DP_PLUG_TYPE_FLIPPED, 1, 0},
	{DP_PLUG_TYPE_FLIPPED, 2, 1},
	{DP_PLUG_TYPE_FLIPPED, 3, 3},
};

static int dptx_mapping_phylane(uint32_t lane, int plug_type,
	uint16_t *txdrvctrl, uint16_t *txeqcoeff)
{
	uint32_t i;
	uint8_t phylane = 0;

	for (i = 0; i < ARRAY_SIZE(phylane_mappings); i++) {
		if (plug_type == phylane_mappings[i].plug_type && lane == phylane_mappings[i].lane) {
			phylane = phylane_mappings[i].phylane;
			break;
		}
	}

	if (i == ARRAY_SIZE(phylane_mappings)) {
		disp_pr_err("[DP] lane number error!\n");
		return -EINVAL;
	}

	*txdrvctrl = TXDRVCTRL(phylane);
	*txeqcoeff = TXEQCOEFF(phylane);

	return 0;
}

static int dptx_config_swing_and_preemphasis(struct dp_ctrl *dptx, uint32_t sw_level, uint32_t pe_level,
	uint16_t txdrvctrl, uint16_t txeqcoeff)
{
	return 0;
}

void dptx_combophy_set_preemphasis_swing(struct dp_ctrl *dptx,
	uint32_t lane, uint32_t sw_level, uint32_t pe_level)
{
	uint16_t txdrvctrl = 0;
	uint16_t txeqcoeff = 0;
	int ret;

	disp_check_and_no_retval((dptx == NULL), err, "[DP] dptx is NULL!\n");

	ret = dptx_mapping_phylane(lane, dptx->dptx_plug_type, &txdrvctrl, &txeqcoeff);
	if (ret)
		return;
	ret = dptx_config_swing_and_preemphasis(dptx, sw_level, pe_level, txdrvctrl, txeqcoeff);
	if (ret)
		return;
}

void dptx_aux_disreset(struct dp_ctrl *dptx, bool enable)
{
}

void dptx_combophy_set_ssc_dis(struct dp_ctrl *dptx, bool ssc_disable)
{
}

void dptx_combophy_set_rate(struct dp_ctrl *dptx, int rate)
{
}

void dptx_phy_layer_init(struct dp_ctrl *dptx)
{
	disp_check_and_no_retval((dptx == NULL), err, "[DP] dptx is NULL!\n");

	dptx->dptx_combophy_set_preemphasis_swing = dptx_combophy_set_preemphasis_swing;
	dptx->dptx_change_physetting_hbr2 = NULL;
	dptx->dptx_combophy_set_lanes_power = NULL;
	dptx->dptx_combophy_set_ssc_addtions = NULL;
	dptx->dptx_combophy_set_ssc_dis = dptx_combophy_set_ssc_dis;
	dptx->dptx_aux_disreset = dptx_aux_disreset;
	dptx->dptx_combophy_set_rate = dptx_combophy_set_rate;
}

