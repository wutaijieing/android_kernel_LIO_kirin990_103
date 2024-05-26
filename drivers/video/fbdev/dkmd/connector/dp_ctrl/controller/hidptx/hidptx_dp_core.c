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
#include <linux/arm-smccc.h>
#include <soc_hsdt1_crg_interface.h>
#include <soc_crgperiph_interface.h>
#include "dp_drv.h"
#include "dp_ctrl.h"
#include "hidptx_dp_core.h"
#include "hidptx_dp_avgen.h"
#include "hidptx_dp_aux.h"
#include "hidptx_dp_irq.h"
#include "hidptx_reg.h"
#include "dp_core_interface.h"
#include "drm_dp_helper_additions.h"
#include "dp_aux.h"
#include "phy/dp_phy_config_interface.h"
#include "dsc/dsc_config_base.h"
#include "dkmd_log.h"

/**
 * dptx_global_intr_en() - Enables top-level interrupts
 * @dptx: The dptx struct
 *
 * Enables (unmasks) all top-level interrupts.
 */
void dptx_global_intr_en(struct dp_ctrl *dptx)
{
	uint32_t val;
	uint32_t irq_ien = 0;

	/* enable all timingGen interrupts */
	irq_ien = DPTX_IRQ_VSYNC | DPTX_IRQ_STREAM0_UNDERFLOW;
	dptx_writel(dptx, DPTX_INTR_ACPU_TIMING_GEN_ENABLE, irq_ien);

	/* enable hpd interrupts */
	irq_ien = DPTX_IRQ_STREAM0_FRAME;
	dptx_writel(dptx, DPTX_INTR_ENABLE, irq_ien);

	val = dptx_readl(dptx, DPTX_ACTIVE_CNT_NUM_CFG);
	val &= ~(DPTX_ACTIVE0_NUM_MASK | DPTX_ACTIVE1_NUM_MASK);
	val |= 0x10;
	val |= (0x60 << DPTX_ACTIVE1_NUM_SHIFT);
	dptx_writel(dptx, DPTX_ACTIVE_CNT_NUM_CFG, val);
}

/**
 * dptx_global_intr_dis() - Disables top-level interrupts
 * @dptx: The dptx struct
 *
 * Disables (masks) all top-level interrupts.
 */
void dptx_global_intr_dis(struct dp_ctrl *dptx)
{
	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	dptx_writel(dptx, DPTX_INTR_ENABLE, 0);
	dptx_writel(dptx, DPTX_SDP_INTR_ENABLE, 0);
	dptx_writel(dptx, DPTX_INTR_ACPU_TIMING_GEN_ENABLE, 0);
}

/**
 * dptx_clear_intr() - clear top-level interrupts
 * @dptx: The dptx struct
 *
 * clear all top-level interrupts.
 */
void dptx_global_intr_clear(struct dp_ctrl *dptx)
{
	dptx_writel(dptx, DPTX_INTR_ORIGINAL_STATUS, ~0);
	dptx_writel(dptx, DPTX_SDP_INTR_ORIGINAL_STATUS, ~0);
	dptx_writel(dptx, DPTX_INTR_ACPU_TIMING_GEN_ORIGINAL_STATUS, ~0);
}

/**
 * dptx_soft_reset() - Performs a core soft reset
 * @dptx: The dptx struct
 * @bits: The components to reset
 *
 * Resets specified parts of the core by writing @bits into the core
 * soft reset control register and clearing them 10-20 microseconds
 * later.
 */
void dptx_soft_reset(struct dp_ctrl *dptx, uint32_t bits)
{
	uint32_t rst;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	rst = dptx_readl(dptx, DPTX_RST_CTRL);
	rst &= ~bits;
	dptx_writel(dptx, DPTX_RST_CTRL, rst);

	usleep_range(10, 20);

	rst = dptx_readl(dptx, DPTX_RST_CTRL);
	rst |= bits;
	dptx_writel(dptx, DPTX_RST_CTRL, rst);
}

/**
 * dptx_soft_reset_all() - Reset all core modules
 * @dptx: The dptx struct
 */
void dptx_soft_reset_all(struct dp_ctrl *dptx)
{
	uint32_t retval;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	retval = dptx_readl(dptx, DPTX_RST_CTRL);
	retval |= DPTX_GLOBAL_RST_N_MASK;
	dptx_writel(dptx, DPTX_RST_CTRL, retval);
}

void dptx_typec_reset_ack(struct dp_ctrl *dptx)
{
}

#define ACCESS_REGISTER_FN_MAIN_ID_HDCP       (0xc500aa01)
#define ACCESS_REGISTER_FN_SUB_ID_HDCP_CTRL   (0x55bbccf1)
static void dptx_hdcp_enable_video_transform(bool enable)
{
	struct arm_smccc_res res = {0};
	arm_smccc_smc((u64)ACCESS_REGISTER_FN_MAIN_ID_HDCP,
		DSS_HDCP_DP_ENABLE, enable, (u64)ACCESS_REGISTER_FN_SUB_ID_HDCP_CTRL,
		0, 0, 0, 0, &res);
}

/**
 * dptx_core_deinit() - Deinitialize the core
 * @dptx: The dptx struct
 *
 * Disable the core in preparation for module shutdown.
 */
int dptx_core_deinit(struct dp_ctrl *dptx)
{
	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] null pointer");

	dptx_soft_reset_all(dptx);

	/* config security reg, disenable video transform */
	dptx_hdcp_enable_video_transform(false);

	if (dptx->dptx_aux_disreset)
		dptx->dptx_aux_disreset(dptx, false);

	return 0;
}

static int dp_dis_reset(struct dp_ctrl *dptx, bool benable)
{
	struct dpu_connector *connector = NULL;
	struct dp_private *dp_pri = NULL;

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is NULL!");
	connector = dptx->connector;
	dp_pri = to_dp_private(connector->conn_info);

	if (benable) {
		set_reg(SOC_HSDT1_CRG_PERRSTDIS0_ADDR(dp_pri->hsdt1_crg_base), 1, 1, 3); /* 0xEFB02000 + 0x24 bit3 */
		set_reg(SOC_CRGPERIPH_PERRSTDIS0_ADDR(connector->peri_crg_base), 1, 1, 0); /* 0xFFB85000 + 0x64 bit0 */
		set_reg(SOC_HSDT1_CRG_PEREN0_ADDR(dp_pri->hsdt1_crg_base), 1, 1, 3); /* 0xEFB02000 + 0x0 bit3 */
		set_reg(SOC_CRGPERIPH_PEREN12_ADDR(connector->peri_crg_base), 1, 1, 22); /* 0xFFB85000 + 0x470 bit22 */
		if (dptx->mst) {
			set_reg(SOC_HSDT1_CRG_PERRSTDIS0_ADDR(dp_pri->hsdt1_crg_base), 1, 1, 24); /* 0xEFB02000 + 0x24 bit24 for mst */
			set_reg(SOC_HSDT1_CRG_PEREN1_ADDR(dp_pri->hsdt1_crg_base), 1, 1, 19); /* 0xEFB02000 + 0x10 bit19 for mst */
		}
		set_reg(SOC_HSDT1_CRG_PEREN0_ADDR(dp_pri->hsdt1_crg_base), 1, 1, 7);  /* 0xEFB02000 + 0x0 bit7 */
		set_reg(SOC_CRGPERIPH_PEREN12_ADDR(connector->peri_crg_base), 1, 1, 21); /* 0xFFB85000 + 0x470 bit21 */
	} else {
		set_reg(SOC_HSDT1_CRG_PERDIS0_ADDR(dp_pri->hsdt1_crg_base), 1, 1, 7); /* 0xEFB02000 + 0x4 bit7 */
		set_reg(SOC_HSDT1_CRG_PERRSTEN0_ADDR(dp_pri->hsdt1_crg_base), 1, 1, 3); /* 0xEFB02000 + 0x20 bit3 */
		set_reg(SOC_CRGPERIPH_PERRSTEN0_ADDR(connector->peri_crg_base), 1, 1, 0); /* 0xFFB85000 + 0x60 bit0 */
	}
	dpu_pr_info("--> PERRSTSTAT0=0x%x, PERRSTSTAT0=0x%x, PERSTAT0=0x%x",
		inp32(SOC_HSDT1_CRG_PERRSTSTAT0_ADDR(dp_pri->hsdt1_crg_base)),
		inp32(SOC_CRGPERIPH_PERRSTSTAT0_ADDR(connector->peri_crg_base)),
		inp32(SOC_HSDT1_CRG_PERSTAT0_ADDR(dp_pri->hsdt1_crg_base)));

	return 0;
}

static int dptx_core_on(struct dp_ctrl *dptx)
{
	uint32_t phyifctrl;

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] null pointer");

	/* soft reset the core */
	dptx_soft_reset_all(dptx);

	dptx_writel(dptx, DPTX_PHYIF_CTRL1, 0x0);

	/* config security reg, enable video transform config DPTX_HDCP_CFG val: 0xA2 */
	dptx_hdcp_enable_video_transform(true);

	phyifctrl = dptx_readl(dptx, DPTX_GCTL0);
	phyifctrl &= ~DPTX_CFG_MODE_SELECT_MASK;
	phyifctrl |= DPTX_CFG_DP_ENABLE_BIT;
	dptx_writel(dptx, DPTX_GCTL0, phyifctrl);

	/* sel dp */
	set_reg(dptx->base + DPTX_AUX_REQ, 0, 1, 1);

	/* reset video ctrl */
	dptx_writel(dptx, dptx_video_ctrl_stream(0), 0x0);
	dptx_writel(dptx, dptx_video_ctrl_stream(1), 0x0);

	/* clear intr */
	dptx_global_intr_dis(dptx);
	dptx_global_intr_clear(dptx);

	/* Enable all top-level interrupts */
	dptx_global_intr_en(dptx);

	return 0;
}

void dptx_core_off(struct dp_ctrl *dptx)
{
	dpu_check_and_no_retval(!dptx, err, "[DP] dptx is NULL");

	dptx_global_intr_dis(dptx);
	dptx_global_intr_clear(dptx);

	dptx_core_deinit(dptx);
}

void dptx_free_lanes(struct dp_ctrl *dptx)
{
	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	if (dptx->dptx_disable_default_video_stream)
		dptx->dptx_disable_default_video_stream(dptx, 0);

	dptx_typec_reset_ack(dptx);
	dptx_phy_set_lanes_power(dptx, false);
}

/*
 * PHYIF core access functions
 */
void dptx_phy_set_lanes(struct dp_ctrl *dptx, uint32_t lanes)
{
	uint32_t val;
	uint32_t phyifctrl;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	dpu_pr_info("[DP] DPTX set lanes=%d", lanes);

	switch (lanes) {
	case 1:
		val = 0x1;
		break;
	case 2:
		val = 0x3;
		break;
	case 4:
		val = 0xf;
		break;
	default:
		dpu_pr_err("[DP] Invalid number of lanes %d", lanes);
		return;
	}

	phyifctrl = dptx_readl(dptx, DPTX_PHYIF_CTRL0);
	phyifctrl &= ~DPTX_LANE_DATA_EN_MASK;
	phyifctrl |= val << DPTX_LANE_DATA_EN_SHIFT;
	dptx_writel(dptx, DPTX_PHYIF_CTRL0, phyifctrl);

	val = dptx_readl(dptx, DPTX_GCTL0);
	val &= ~GENMASK(2, 1);
	val |= ((lanes / 2) << 1);
	dptx_writel(dptx, DPTX_GCTL0, val);
}

static void dptx_phy_disable_lane_data(struct dp_ctrl *dptx)
{
	uint32_t phyifctrl;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	phyifctrl = dptx_readl(dptx, DPTX_PHYIF_CTRL0);
	phyifctrl &= ~DPTX_LANE_DATA_EN_MASK;
	dptx_writel(dptx, DPTX_PHYIF_CTRL0, phyifctrl);
}

void dptx_phy_set_rate(struct dp_ctrl *dptx, uint32_t rate)
{
	uint32_t phyifctrl;
	int retval;
	bool rate_changed = false;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	dpu_pr_info("[DP] DPTX set rate=%d", rate);

	phyifctrl = dptx_readl(dptx, DPTX_PHYIF_CTRL2);
	if ((phyifctrl & DPTX_LANE_RATE_MASK) != rate)
		rate_changed = true;

	switch (rate) {
	case DPTX_PHYIF_CTRL_RATE_RBR:
	case DPTX_PHYIF_CTRL_RATE_HBR:
	case DPTX_PHYIF_CTRL_RATE_HBR2:
	case DPTX_PHYIF_CTRL_RATE_HBR3:
		break;
	default:
		dpu_pr_err("[DP] Invalid PHY rate %d", rate);
		return;
	}

	phyifctrl = rate;
	dptx_writel(dptx, DPTX_PHYIF_CTRL2, phyifctrl);

	if (rate_changed) {
		retval = dptx_phy_wait_busy(dptx, dptx->link.lanes);
		if (retval)
			dpu_pr_err("[DP] set_rate fail");
	}
}

void dptx_phy_data_lane_clear(struct dp_ctrl *dptx)
{
	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	/* set 0xA0 to 0 */
	dptx_writel(dptx, DPTX_PHYIF_CTRL0, 0x0);
}

int dptx_phy_wait_busy(struct dp_ctrl *dptx, uint32_t lanes)
{
	unsigned long dw_jiffies = 0;
	uint32_t phyifctrl;
	uint32_t mask = 0;

	dpu_pr_info("[DP] lanes=%d", lanes);
	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] null pointer");

	switch (lanes) {
	/* This case (value 4) is not terminated by a 'break' statement */
	case 4:
		mask |= dptx_phyif_ctrl_busy(3);
		mask |= dptx_phyif_ctrl_busy(2);
	/* This case (value 2) is not terminated by a 'break' statement */
	case 2:
		mask |= dptx_phyif_ctrl_busy(1);
	case 1:
		mask |= dptx_phyif_ctrl_busy(0);
		break;
	default:
		dpu_pr_err("[DP] Invalid number of lanes %d", lanes);
		break;
	}

	dw_jiffies = jiffies + HZ / 10;

	do {
		phyifctrl = dptx_readl(dptx, DPTX_PHYIF_STATUS0);
		if ((phyifctrl & mask) == mask)
			return 0;
		udelay(1);
	} while (time_after(dw_jiffies, jiffies));

	dpu_pr_err("[DP] PHY BUSY timed out");

	return -EBUSY;
}

static int dptx_phy_wait_changed_lane_busy(struct dp_ctrl *dptx, uint32_t phyifctrl, uint32_t orig_phyifctrl)
{
	unsigned long dw_jiffies = 0;
	uint32_t val;
	uint32_t mask = 0;

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] null pointer");

	if ((phyifctrl & DPTX_LANE3_PHY_STATUS_MASK) != (orig_phyifctrl & DPTX_LANE3_PHY_STATUS_MASK))
		mask |= dptx_phyif_ctrl_busy(3);
	if ((phyifctrl & DPTX_LANE2_PHY_STATUS_MASK) != (orig_phyifctrl & DPTX_LANE2_PHY_STATUS_MASK))
		mask |= dptx_phyif_ctrl_busy(2);
	if ((phyifctrl & DPTX_LANE1_PHY_STATUS_MASK) != (orig_phyifctrl & DPTX_LANE1_PHY_STATUS_MASK))
		mask |= dptx_phyif_ctrl_busy(1);
	if ((phyifctrl & DPTX_LANE0_PHY_STATUS_MASK) != (orig_phyifctrl & DPTX_LANE0_PHY_STATUS_MASK))
		mask |= dptx_phyif_ctrl_busy(0);

	dw_jiffies = jiffies + HZ / 10;

	do {
		val = dptx_readl(dptx, DPTX_PHYIF_STATUS0);
		if ((val & mask) == mask)
			return 0;
		udelay(1);
	} while (time_after(dw_jiffies, jiffies));

	dpu_pr_err("[DP] PHY BUSY timed out");

	return -EBUSY;
}

void dptx_phy_set_pattern(struct dp_ctrl *dptx, uint32_t pattern)
{
	uint32_t phyifctrl = 0;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	dpu_pr_info("[DP] Setting PHY pattern=0x%x", pattern);

	phyifctrl = dptx_readl(dptx, DPTX_PHYIF_CTRL0);
	phyifctrl &= ~DPTX_PATTERN_SEL_MASK;
	phyifctrl |= ((pattern << DPTX_PATTERN_SEL_SHIFT) & DPTX_PATTERN_SEL_MASK);
	dptx_writel(dptx, DPTX_PHYIF_CTRL0, phyifctrl);
}

void dptx_phy_lane_reset(struct dp_ctrl *dptx, bool bopen)
{
	uint32_t phyifctrl;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	phyifctrl = dptx_readl(dptx, DPTX_PHYIF_CTRL1);

	if (bopen) /* P0 mode, disreset lanes */
		phyifctrl = (uint32_t)cfg_phy_lanes(DPTX_MAX_LINK_LANES);
	else
		phyifctrl &= ~cfg_phy_lanes(DPTX_MAX_LINK_LANES);

	dptx_writel(dptx, DPTX_PHYIF_CTRL1, phyifctrl);
}

void dptx_phy_set_lanes_power(struct dp_ctrl *dptx, bool bopen)
{
	uint32_t phyifctrl;
	uint32_t lanes;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	lanes = dptx->link.lanes;

	phyifctrl = dptx_readl(dptx, DPTX_PHYIF_CTRL3);

	phyifctrl &= ~(DPTX_LANE2_PHY_STATUS_MASK | DPTX_LANE3_PHY_STATUS_MASK);
	phyifctrl &= ~(DPTX_LANE1_PHY_STATUS_MASK | DPTX_LANE0_PHY_STATUS_MASK);
	switch (lanes) {
	case 4:
	case 2:
	case 1:
		phyifctrl |= ((LANE_POWER_MODE_P2 << DPTX_LANE3_PHY_STATUS_SHIFT) |
			(LANE_POWER_MODE_P2 << DPTX_LANE2_PHY_STATUS_SHIFT));
		phyifctrl |= LANE_POWER_MODE_P2 << DPTX_LANE1_PHY_STATUS_SHIFT;
		phyifctrl |= LANE_POWER_MODE_P2;
		break;
	default:
		dpu_pr_err("[DP] Invalid number of lanes %d", lanes);
		return;
	}

	if (bopen) { /* set lanes to P0 mode */
		switch (lanes) {
		case 4:
			phyifctrl &= ~(DPTX_LANE2_PHY_STATUS_MASK | DPTX_LANE3_PHY_STATUS_MASK);
		case 2:
			phyifctrl &= ~DPTX_LANE1_PHY_STATUS_MASK;
		case 1:
			phyifctrl &= ~DPTX_LANE0_PHY_STATUS_MASK;
			break;
		default:
			dpu_pr_err("[DP] Invalid number of lanes %d", lanes);
			return;
		}
	}

	dptx_writel(dptx, DPTX_PHYIF_CTRL3, phyifctrl);
}

void dptx_phy_set_ssc(struct dp_ctrl *dptx, bool bswitchphy)
{
	uint8_t byte = 0;
	bool sink_support_ssc = false;
	bool open_ssc = false;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	/* FPGA don't need config phy */
	if (dptx->connector->conn_info->base.fpga_flag == 1)
		return;

	dpu_check_and_no_retval(dptx_read_dpcd(dptx, DP_MAX_DOWNSPREAD, &byte), err, "[DP] Read DPCD error");

	sink_support_ssc = byte & DP_MAX_DOWNSPREAD_0_5;
	dpu_pr_info("[DP] SSC setting: %x", byte);

	open_ssc = sink_support_ssc && dptx->ssc_en;
	if (dptx->dptx_combophy_set_ssc_dis)
		dptx->dptx_combophy_set_ssc_dis(dptx, !open_ssc);

	dpu_check_and_no_retval(dptx_read_dpcd(dptx, DP_DOWNSPREAD_CTRL, &byte), err, "[DP] Read DPCD error");

	if (open_ssc)
		byte |= DP_SPREAD_AMP_0_5;
	else
		byte &= ~DP_SPREAD_AMP_0_5;

	dpu_check_and_no_retval(dptx_write_dpcd(dptx, DP_DOWNSPREAD_CTRL, byte), err, "[DP] write DPCD error");
}

static void dptx_default_params_from_core(struct dp_ctrl *dptx)
{
	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	if (dptx->connector->conn_info->base.fpga_flag == 1)
		dptx->max_rate = DPTX_PHYIF_CTRL_RATE_HBR;
	else
		dptx->max_rate = DPTX_PHYIF_CTRL_RATE_HBR2;

	dptx->multipixel = DPTX_MP_DUAL_PIXEL;
	dptx->max_lanes = DPTX_DEFAULT_LINK_LANES;
}

int dptx_set_enhanced_framing_mode(struct dp_ctrl *dptx)
{
	int retval;
	uint32_t cctl;
	uint8_t byte;

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] null pointer");

	byte = dptx->link.lanes;
	cctl = dptx_readl(dptx, dptx_video_ctrl_stream(0));

	/* enhance frame */
	if (dptx->efm_en && drm_dp_enhanced_frame_cap(dptx->rx_caps)) {
		byte |= DP_ENHANCED_FRAME_CAP;
		cctl |= DPTX_STREAM_FRAME_MODE;
	} else {
		cctl &= ~DPTX_STREAM_FRAME_MODE;
	}

	dptx_writel(dptx, dptx_video_ctrl_stream(0), cctl);

	retval = dptx_write_dpcd(dptx, DP_LANE_COUNT_SET, byte);
	if (retval)
		return retval;

	return 0;
}

static bool dptx_sink_device_connected(struct dp_ctrl *dptx)
{
	dpu_check_and_return(!dptx, false, err, "[DP] dptx is NULL");

	return dptx->dptx_enable;
}

void dptx_link_core_reset(struct dp_ctrl *dptx)
{
	int retval;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	dptx_soft_reset_all(dptx);
	if (dptx->dptx_aux_disreset)
		dptx->dptx_aux_disreset(dptx, true);

	dptx_phy_lane_reset(dptx, false);
	mdelay(1);
	dptx_phy_lane_reset(dptx, true);

	/* Wait for PHY busy */
	retval = dptx_phy_wait_busy(dptx, dptx->max_lanes);
	if (retval)
		dpu_pr_err("[DP] phy release fail");
}

void dptx_link_set_preemp_vswing(struct dp_ctrl *dptx)
{
	uint32_t i;
	uint8_t pe;
	uint8_t vs;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	/* FPGA don't need config phy */
	if (dptx->connector->conn_info->base.fpga_flag == 1)
		return;

	for (i = 0; i < dptx->link.lanes; i++) {
		pe = dptx->link.preemp_level[i];
		vs = dptx->link.vswing_level[i];
		dpu_pr_info("[DP] lane: %d, vs: %d, pe: %d", i, vs, pe);
		if (dptx->dptx_combophy_set_preemphasis_swing)
			dptx->dptx_combophy_set_preemphasis_swing(dptx, i, vs, pe);
	}
}

static int dptx_link_set_lane_status(struct dp_ctrl *dptx)
{
	int retval;
	uint8_t byte;
	uint32_t phyifctrl;
	uint32_t orig_phyifctrl;

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is NULL");

	dptx_phy_disable_lane_data(dptx);

	orig_phyifctrl = dptx_readl(dptx, DPTX_PHYIF_CTRL3);
	dptx_phy_set_lanes_power(dptx, true);
	phyifctrl = dptx_readl(dptx, DPTX_PHYIF_CTRL3);

	/* Wait for PHY busy */
	retval = dptx_phy_wait_changed_lane_busy(dptx, phyifctrl, orig_phyifctrl);
	if (retval != 0)
		dpu_pr_err("[DP] set power P0 fail");

	/* Initialize PHY */
	dptx_phy_set_rate(dptx, dptx->link.rate);

	if (dptx->dptx_combophy_set_rate)
		dptx->dptx_combophy_set_rate(dptx, dptx->link.rate);

	dptx_phy_set_lanes(dptx, dptx->link.lanes);
	if (dptx->dptx_phy_set_ssc)
		dptx->dptx_phy_set_ssc(dptx, false);

	/* Set PHY_TX_EQ */
	if (dptx->dptx_link_set_preemp_vswing)
		dptx->dptx_link_set_preemp_vswing(dptx);

	if (dptx->dptx_phy_set_pattern)
		dptx->dptx_phy_set_pattern(dptx, DPTX_PHYIF_CTRL_TPS_NONE);

	retval = dptx_write_dpcd(dptx, DP_TRAINING_PATTERN_SET, DP_TRAINING_PATTERN_DISABLE);
	if (retval != 0)
		return retval;

	retval = dptx_phy_rate_to_bw(dptx->link.rate);
	if (retval < 0)
		return retval;

	byte = (uint8_t)retval;
	retval = dptx_write_dpcd(dptx, DP_LINK_BW_SET, byte);
	if (retval != 0)
		return retval;

	retval = dptx_set_enhanced_framing_mode(dptx);
	if (retval != 0)
		return retval;

	byte = DP_SPREAD_AMP_0_5;
	retval = dptx_write_dpcd(dptx, DP_DOWNSPREAD_CTRL, byte);
	if (retval != 0)
		return retval;

	byte = 1;
	retval = dptx_write_dpcd(dptx, DP_MAIN_LINK_CHANNEL_CODING_SET, byte);
	if (retval != 0)
		return retval;

	return 0;
}

void dptx_initiate_mst_act(struct dp_ctrl *dptx)
{
	uint32_t reg;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	reg = dptx_readl(dptx, DPTX_MST_CTRL);
	reg |= DPTX_CFG_SOFT_ACT_TRG;
	dptx_writel(dptx, DPTX_MST_CTRL, reg);
}

void dptx_clear_vcpid_table(struct dp_ctrl *dptx)
{
	int i;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	for (i = 0; i < 8; i++)
		dptx_writel(dptx, dptx_mst_vcp_table_reg_n(i), 0);
}

void dptx_set_vcpid_table_slot(struct dp_ctrl *dptx,
	uint32_t slot, uint32_t stream)
{
	uint32_t offset;
	uint32_t reg;
	uint32_t lsb;
	uint32_t mask;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");
	dpu_check_and_no_retval((slot > 63), err, "Invalid slot number > 63");

	offset = dptx_mst_vcp_table_reg_n(slot >> 3);
	reg = dptx_readl(dptx, offset);

	lsb = (slot & 0x7) * 4;
	mask = (uint32_t)GENMASK(lsb + 3, lsb);

	reg &= ~mask;
	reg |= (stream << lsb) & mask;

	dpu_pr_debug("[DP] Writing 0x%08x val=0x%08x", offset, reg);
	dptx_writel(dptx, offset, reg);
}

void dptx_ctrl_layer_init(struct dp_ctrl *dptx)
{
	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	dptx->aux_rw = dptx_aux_rw;

	dptx->dptx_hpd_handler = dptx_hpd_handler;
	dptx->dptx_hpd_irq_handler = dptx_hpd_irq_handler;

	dptx->dp_dis_reset = dp_dis_reset;
	dptx->dptx_core_on = dptx_core_on;
	dptx->dptx_core_off = dptx_core_off;

	dptx->dptx_free_lanes = dptx_free_lanes;
	dptx->dptx_link_core_reset = dptx_link_core_reset;
	dptx->dptx_default_params_from_core = dptx_default_params_from_core;
	dptx->dptx_sink_device_connected = dptx_sink_device_connected;

	dptx->dptx_hdr_infoframe_set_reg = dptx_hdr_infoframe_set_reg;

	dptx->dptx_audio_config = dptx_audio_config;
	dptx->dptx_video_core_config = dptx_video_core_config;
	dptx->dptx_sdp_config = dptx_sdp_config;
	dptx->dptx_link_timing_config = dptx_link_timing_config;
	dptx->dptx_video_ts_change = dptx_video_ts_change;
	dptx->dptx_video_ts_calculate = dptx_video_ts_calculate;
	dptx->dptx_link_close_stream = dptx_link_close_stream;
	dptx->dptx_fec_enable = dptx_fec_enable;

	dptx->dptx_link_set_lane_status = dptx_link_set_lane_status;
	dptx->dptx_link_set_lane_after_training = dptx_link_set_lane_after_training;
	dptx->dptx_phy_set_pattern = dptx_phy_set_pattern;
	dptx->dptx_link_set_preemp_vswing = dptx_link_set_preemp_vswing;

	dptx->dptx_enable_default_video_stream = dptx_enable_default_video_stream;
	dptx->dptx_disable_default_video_stream = dptx_disable_default_video_stream;
	dptx->dptx_triger_media_transfer = dptx_triger_media_transfer;
	dptx->dptx_resolution_switch = dptx_resolution_switch;
	dptx->dptx_phy_enable_xmit = NULL;

	dptx->dptx_dsc_cfg = dptx_dsc_cfg;
	dptx->dptx_slice_height_limit = dptx_slice_height_limit;
	dptx->dptx_line_buffer_depth_limit = dptx_line_buffer_depth_limit;
	dptx->dptx_dsc_sdp_manul_send = dptx_dsc_sdp_manul_send;

	/* for test */
	dptx->dptx_audio_num_ch_change = NULL;
	dptx->dptx_video_timing_change = dptx_video_timing_change;
	dptx->dptx_video_bpc_change = dptx_video_bpc_change;
	dptx->dptx_audio_infoframe_sdp_send = dptx_audio_infoframe_sdp_send;
	dptx->handle_test_link_phy_pattern = handle_test_link_phy_pattern;
	dptx->dptx_phy_set_ssc = dptx_phy_set_ssc;

	dptx_phy_layer_init(dptx);
}
/*lint -restore*/
