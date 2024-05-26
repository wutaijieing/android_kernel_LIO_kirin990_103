/* Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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
#include "dpu_dp.h"
#include "dpu_fb.h"
#include "dpu_fb_def.h"
#include <linux/platform_drivers/usb/chip_usb.h>

/* PLL */
#define PLL1UPDATECTRL             (0xc00 + 0x10)
#define PLL1UPDATECTRL_RATE_MASK   GENMASK(11, 8)
#define PLL1UPDATECTRL_RATE_OFFSET 8

#define PLL1CKGCTRLR0              (0xc00 + 0x14)
#define PLL1CKGCTRLR1              (0xc00 + 0x1c)
#define PLL1CKGCTRLR2              (0xc00 + 0x24)
#define PLL_LPF_CS_R_MASK          GENMASK(7, 6)
#define PLL_LPF_RS_R_MASK          GENMASK(11, 8)

/* ssc */
#define PLL1SSCG_CTRL              (0xc00 + 0x80)
#define PLL1_SSCG_CNT_STEPSIZE_FORCE_0 BIT(1)

#define pll1sscg_ctrl_initial_r(n) (0xc00 + 0x10 * (n) + 0x84)
#define pll1sscg_cnt_r(n)          (0xc00 + 0x10 * (n) + 0x88)
#define pll1sscg_cnt2_r(n)         (0xc00 + 0x10 * (n) + 0x8c)

/* pre sw */
#define txdrvctrl(n)               (0x2000 + 0x0400 * (n) + 0x314)
#define txeqcoeff(n)               (0x2000 + 0x0400 * (n) + 0x318)

#define AUX_EYE_LEVEL              0x1
#define AUX_HYS_TUNE_VAL           0x1
#define AUX_CTRL_VAL               0x3
#define DPTX_DP_AUX_CTRL           0x20

/* aux disreset */
#define REG_DP_AUX_PWDNB           BIT(9)

/* eye diagram */
#define AUX_RTERMSEL_MASK             GENMASK(2, 0)
#define AUX_NEN_RTERM_MASK            BIT(3)
#define REG_DP_AUX_AUX_CTRL_MASK      GENMASK(7, 4)
#define AUX_CTRL_OFFSET               4
#define REG_DP_AUX_AUX_HYS_TUNE_MASK  GENMASK(11, 10)
#define AUX_HYS_TUNE_OFFSET           10
#define REG_DP_AUX_RONSELN_MASK       GENMASK(14, 12)
#define AUX_RONSELN_OFFSET            12
#define REG_DP_AUX_RONSELP_MASK       GENMASK(18, 16)
#define AUX_RONSELP_OFFSET            16

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
		DPU_FB_ERR("[DP] lane number error!\n");
		return -EINVAL;
	}

	*txdrvctrl = txdrvctrl(phylane);
	*txeqcoeff = txeqcoeff(phylane);

	return 0;
}

static int dptx_config_swing_and_preemphasis_on_sw_level_0(struct dpu_fb_data_type *dpufd, struct dp_ctrl *dptx,
	uint32_t pe_level, uint16_t txdrvctrl, uint16_t txeqcoeff)
{
	switch (pe_level) {
	case 0:
		outp32(dpufd->usb_tca_base + txdrvctrl, dptx->combophy_pree_swing[0]);
		outp32(dpufd->usb_tca_base + txeqcoeff, dptx->combophy_pree_swing[1]);
		break;
	case 1:
		outp32(dpufd->usb_tca_base + txdrvctrl, dptx->combophy_pree_swing[2]);
		outp32(dpufd->usb_tca_base + txeqcoeff, dptx->combophy_pree_swing[3]);
		break;
	case 2:
		outp32(dpufd->usb_tca_base + txdrvctrl, dptx->combophy_pree_swing[4]);
		outp32(dpufd->usb_tca_base + txeqcoeff, dptx->combophy_pree_swing[5]);
		break;
	case 3:
		outp32(dpufd->usb_tca_base + txdrvctrl, dptx->combophy_pree_swing[6]);
		outp32(dpufd->usb_tca_base + txeqcoeff, dptx->combophy_pree_swing[7]);
		break;
	default:
		DPU_FB_ERR("[DP] preemphasis_ level error\n");
		return -EINVAL;
	}

	return 0;
}

static int dptx_config_swing_and_preemphasis_on_sw_level_1(struct dpu_fb_data_type *dpufd, struct dp_ctrl *dptx,
	uint32_t pe_level, uint16_t txdrvctrl, uint16_t txeqcoeff)
{
	switch (pe_level) {
	case 0:
		outp32(dpufd->usb_tca_base + txdrvctrl, dptx->combophy_pree_swing[8]);
		outp32(dpufd->usb_tca_base + txeqcoeff, dptx->combophy_pree_swing[9]);
		break;
	case 1:
		outp32(dpufd->usb_tca_base + txdrvctrl, dptx->combophy_pree_swing[10]);
		outp32(dpufd->usb_tca_base + txeqcoeff, dptx->combophy_pree_swing[11]);
		break;
	case 2:
		outp32(dpufd->usb_tca_base + txdrvctrl, dptx->combophy_pree_swing[12]);
		outp32(dpufd->usb_tca_base + txeqcoeff, dptx->combophy_pree_swing[13]);
		break;
	case 3:
		DPU_FB_WARNING("[DP] Don't support sw 1 & ps level 3\n");
		break;
	default:
		DPU_FB_ERR("[DP] preemphasis_ level error\n");
		return -EINVAL;
	}

	return 0;
}

static int dptx_config_swing_and_preemphasis_on_sw_level_2(struct dpu_fb_data_type *dpufd, struct dp_ctrl *dptx,
	uint32_t pe_level, uint16_t txdrvctrl, uint16_t txeqcoeff)
{
	switch (pe_level) {
	case 0:
		outp32(dpufd->usb_tca_base + txdrvctrl, dptx->combophy_pree_swing[14]);
		outp32(dpufd->usb_tca_base + txeqcoeff, dptx->combophy_pree_swing[15]);
		break;
	case 1:
		outp32(dpufd->usb_tca_base + txdrvctrl, dptx->combophy_pree_swing[16]);
		outp32(dpufd->usb_tca_base + txeqcoeff, dptx->combophy_pree_swing[17]);
		break;
	case 2:
		DPU_FB_WARNING("[DP] Don't support sw 2 & ps level 2\n");
		break;
	case 3:
		DPU_FB_WARNING("[DP] Don't support sw 2 & ps level 3\n");
		break;
	default:
		DPU_FB_ERR("[DP] preemphasis_ level error\n");
		return -EINVAL;
	}

	return 0;
}

static int dptx_config_swing_and_preemphasis_on_sw_level_3(struct dpu_fb_data_type *dpufd, struct dp_ctrl *dptx,
	uint32_t pe_level, uint16_t txdrvctrl, uint16_t txeqcoeff)
{
	switch (pe_level) {
	case 0:
		outp32(dpufd->usb_tca_base + txdrvctrl, dptx->combophy_pree_swing[18]);
		outp32(dpufd->usb_tca_base + txeqcoeff, dptx->combophy_pree_swing[19]);
		break;
	case 1:
		DPU_FB_WARNING("[DP] Don't support sw 3 & ps level 1\n");
		break;
	case 2:
		DPU_FB_WARNING("[DP] Don't support sw 3 & ps level 2\n");
		break;
	case 3:
		DPU_FB_WARNING("[DP] Don't support sw 3 & ps level 3\n");
		break;
	default:
		DPU_FB_ERR("[DP] preemphasis_ level error\n");
		return -EINVAL;
	}

	return 0;
}

static int dptx_config_swing_and_preemphasis(struct dp_ctrl *dptx, uint32_t sw_level, uint32_t pe_level,
	uint16_t txdrvctrl, uint16_t txeqcoeff)
{
	struct dpu_fb_data_type *dpufd = NULL;
	int ret;

	dpu_check_and_return((dptx == NULL), -EINVAL, ERR, "[DP] dptx is NULL!\n");

	dpufd = dptx->dpufd;
	dpu_check_and_return((dpufd == NULL), -EINVAL, ERR, "[DP] dptx is NULL!\n");

	switch (sw_level) {
	case 0:
		ret = dptx_config_swing_and_preemphasis_on_sw_level_0(dpufd, dptx, pe_level, txdrvctrl, txeqcoeff);
		break;
	case 1:
		ret = dptx_config_swing_and_preemphasis_on_sw_level_1(dpufd, dptx, pe_level, txdrvctrl, txeqcoeff);
		break;
	case 2:
		ret = dptx_config_swing_and_preemphasis_on_sw_level_2(dpufd, dptx, pe_level, txdrvctrl, txeqcoeff);
		break;
	case 3:
		ret = dptx_config_swing_and_preemphasis_on_sw_level_3(dpufd, dptx, pe_level, txdrvctrl, txeqcoeff);
		break;
	default:
		DPU_FB_ERR("[DP] vswing level error\n");
		ret = -EINVAL;
	}

	return ret;
}

void dptx_combophy_set_preemphasis_swing(struct dp_ctrl *dptx,
	uint32_t lane, uint32_t sw_level, uint32_t pe_level)
{
	uint16_t txdrvctrl = 0;
	uint16_t txeqcoeff = 0;
	int ret;

	dpu_check_and_no_retval((dptx == NULL), ERR, "[DP] dptx is NULL!\n");

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
	struct dpu_fb_data_type *dpufd = NULL;

	dpu_check_and_no_retval((dptx == NULL), ERR, "[DP] NULL Pointer\n");

	dpufd = dptx->dpufd;
	dpu_check_and_no_retval((dpufd == NULL), ERR, "[DP] dpufd is NULL!\n");

	if (g_fpga_flag == 1)
		return;

	/* Enable AUX Block */
	reg = (uint32_t)inp32(dpufd->usb_tca_base + DPTX_DP_AUX_CTRL);

	if (enable)
		reg |= REG_DP_AUX_PWDNB;
	else
		reg &= ~REG_DP_AUX_PWDNB;

	reg &= ~REG_DP_AUX_RONSELN_MASK;
	reg &= ~REG_DP_AUX_RONSELP_MASK;
	if (g_dp_debug_mode_enable) {
		if (g_dp_aux_ronselp < 0 || g_dp_aux_ronselp > 7)
			g_dp_aux_ronselp = 3;
		reg |= (uint32_t)g_dp_aux_ronselp << AUX_RONSELN_OFFSET;
		reg |= (uint32_t)g_dp_aux_ronselp << AUX_RONSELP_OFFSET;
	} else {
		reg |=  AUX_EYE_LEVEL << AUX_RONSELN_OFFSET;
		reg |=  AUX_EYE_LEVEL << AUX_RONSELP_OFFSET;
	}
	reg &= ~REG_DP_AUX_AUX_HYS_TUNE_MASK;
	reg |= (AUX_HYS_TUNE_VAL << AUX_HYS_TUNE_OFFSET);
	reg &= ~REG_DP_AUX_AUX_CTRL_MASK;
	reg |= (AUX_CTRL_VAL << AUX_CTRL_OFFSET);
	reg &= ~AUX_NEN_RTERM_MASK;
	reg &= ~AUX_RTERMSEL_MASK;

	outp32(dpufd->usb_tca_base + DPTX_DP_AUX_CTRL, reg);
	mdelay(1);
}

void dptx_combophy_set_ssc_dis(struct dp_ctrl *dptx, bool ssc_disable)
{
	uint32_t reg;
	struct dpu_fb_data_type *dpufd = NULL;

	dpu_check_and_no_retval((dptx == NULL), ERR, "[DP] dptx is NULL!\n");

	dpufd = dptx->dpufd;
	dpu_check_and_no_retval((dpufd == NULL), ERR, "[DP] dpufd is NULL!\n");

	reg = (uint32_t)inp32(dpufd->usb_tca_base + PLL1SSCG_CTRL);
	if (!ssc_disable) {
		reg &= ~PLL1_SSCG_CNT_STEPSIZE_FORCE_0;
		DPU_FB_DEBUG("[DP] SSC enable\n");
	} else {
		reg |= PLL1_SSCG_CNT_STEPSIZE_FORCE_0;
		DPU_FB_DEBUG("[DP] SSC disable\n");
	}
	outp32(dpufd->usb_tca_base + PLL1SSCG_CTRL, reg);
}

void dptx_combophy_set_rate(struct dp_ctrl *dptx, int rate)
{
	uint32_t reg;
	struct dpu_fb_data_type *dpufd = NULL;

	dpu_check_and_no_retval((dptx == NULL), ERR, "[DP] dptx is NULL!\n");

	dpufd = dptx->dpufd;
	dpu_check_and_no_retval((dpufd == NULL), ERR, "[DP] dpufd is NULL!\n");

	if (g_fpga_flag == 1)
		return;

	switch (rate) {
	case DPTX_PHYIF_CTRL_RATE_RBR:
		reg = (uint32_t)inp32(dptx->dpufd->usb_tca_base + PLL1CKGCTRLR0);
		reg &= ~PLL_LPF_RS_R_MASK;
		reg |= PLL_LPF_CS_R_MASK;
		outp32(dpufd->usb_tca_base + PLL1CKGCTRLR0, reg);
		break;
	case DPTX_PHYIF_CTRL_RATE_HBR:
		reg = (uint32_t)inp32(dptx->dpufd->usb_tca_base + PLL1CKGCTRLR1);
		reg &= ~PLL_LPF_RS_R_MASK;
		reg |= PLL_LPF_CS_R_MASK;
		outp32(dpufd->usb_tca_base + PLL1CKGCTRLR1, reg);
		break;
	case DPTX_PHYIF_CTRL_RATE_HBR2:
		reg = (uint32_t)inp32(dptx->dpufd->usb_tca_base + PLL1CKGCTRLR2);
		reg &= ~PLL_LPF_RS_R_MASK;
		reg |= PLL_LPF_CS_R_MASK;
		outp32(dpufd->usb_tca_base + PLL1CKGCTRLR2, reg);
		break;
	case DPTX_PHYIF_CTRL_RATE_HBR3:
		break;
	default:
		DPU_FB_ERR("[DP] Invalid PHY rate %d\n", rate);
		return;
	}
}

void dptx_phy_layer_init(struct dp_ctrl *dptx)
{
	dpu_check_and_no_retval((dptx == NULL), ERR, "[DP] dptx is NULL!\n");

	dptx->dptx_combophy_set_preemphasis_swing = dptx_combophy_set_preemphasis_swing;
	dptx->dptx_change_physetting_hbr2 = NULL;
	dptx->dptx_combophy_set_lanes_power = NULL;
	dptx->dptx_combophy_set_ssc_addtions = NULL;
	dptx->dptx_combophy_set_ssc_dis = dptx_combophy_set_ssc_dis;
	dptx->dptx_aux_disreset = dptx_aux_disreset;
	dptx->dptx_combophy_set_rate = dptx_combophy_set_rate;
}

