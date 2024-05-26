/* Copyright (c) 2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#include <platform_include/display/linux/dpu_drmdriver.h>
#include "hidptx_dp_core.h"
#include "hidptx_dp_avgen.h"
#include "hidptx_dp_aux.h"
#include "hidptx_dp_irq.h"
#include "hidptx_reg.h"
#include "../dp_core_interface.h"
#include "../../phy/dp_phy_config_interface.h"
#include "hisi_drv_dp.h"
#include "../dsc/dsc_config_base.h"
#include <linux/arm-smccc.h>

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
	irq_ien |= ((DPTX_IRQ_ALL_TIMING_GEN_INTR | DPTX_IRQ_ADAPTIVE_SYNC_SDP) & (~DPTX_IRQ_HPRI_SDP) & (~DPTX_IRQ_LPRI_SDP));
	dptx_writel(dptx, DPTX_INTR_ACPU_TIMING_GEN_ENABLE, irq_ien);

	/* enable hpd interrupts */
	irq_ien = 0;
	irq_ien |= (DPTX_IRQ_ALL_INTR & (~DPTX_IRQ_VIDEO_FIFO_FULL) & (~DPTX_IRQ_AUDIO_FIFO_FULL));
	dptx_writel(dptx, DPTX_INTR_ENABLE, irq_ien);

	/* enable sdp second level irq */
	irq_ien = 0;
	irq_ien |= (DPTX_IRQ_STRAME0_INFOFRAME_SDP | DPTX_IRQ_STRAME1_INFOFRAME_SDP | DPTX_IRQ_STREAM0_PPS_SDP);
	dptx_writel(dptx, DPTX_SDP_INTR_ENABLE, irq_ien);

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
	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");

	dptx_writel(dptx, DPTX_INTR_ENABLE, 0);
	dptx_writel(dptx, DPTX_SDP_INTR_ENABLE, 0);
	dptx_writel(dptx, DPTX_INTR_ACPU_TIMING_GEN_ENABLE, 0);
}

/**
 * dptx_clear_intr() - cleat top-level interrupts
 * @dptx: The dptx struct
 *
 * clear all top-level interrupts.
 */
void dptx_global_intr_clear(struct dp_ctrl *dptx)
{
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

	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");

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

	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");

	retval = dptx_readl(dptx, DPTX_RST_CTRL);
	retval |= DPTX_GOBAL_RST_N_MASK;
	dptx_writel(dptx, DPTX_RST_CTRL, retval);
}

void dptx_typec_reset_ack(struct dp_ctrl *dptx)
{
}

/**
 * dptx_check_dptx_id() - Check value of DPTX_ID register
 * @dptx: The dptx struct
 *
 * Returns True if DPTX core correctly identifyed.
 */
bool dptx_check_dptx_id(struct dp_ctrl *dptx)
{
	return true;
}

void dptx_init_hwparams(struct dp_ctrl *dptx)
{
}

static void dptx_hdcp_enable_video_transform(struct dp_ctrl *dptx, bool enable)
{
	struct arm_smccc_res res = {0};

	disp_pr_info("[Dp] test hdcp 600+++\n");

	if (dptx->id == DPTX_DP0)
		arm_smccc_smc(ACCESS_REGISTER_FN_MAIN_ID_HDCP,
			DSS_HDCP_DP_ENABLE, enable, (u64)ACCESS_REGISTER_FN_SUB_ID_HDCP_CTRL,
			0, 0, 0, 0, &res);

	disp_pr_info("[Dp] test hdcp 600---\n");
}

/**
 * dptx_core_init() - Initializes the DP TX core
 * @dptx: The dptx struct
 *
 * Initialize the DP TX core and put it in a known state.
 */
int dptx_core_init(struct dp_ctrl *dptx)
{
	uint32_t phyifctrl;
	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] NULL Pointer\n");

	/* soft reset the core */
	dptx_soft_reset_all(dptx);

	phyifctrl = dptx_readl(dptx, DPTX_AUX_REQ);
	if (dptx->mode == DP_MODE) {
		phyifctrl &= ~DPTX_CFG_AUX_SYNC_LEN_SEL;
	} else {
		phyifctrl |= DPTX_CFG_AUX_SYNC_LEN_SEL;
	}
	dptx_writel(dptx, DPTX_AUX_REQ, phyifctrl);

	/* config security reg, enable video transform */
	dptx_hdcp_enable_video_transform(dptx, true);

	/* reset video ctrl */
	dptx_writel(dptx, DPTX_VIDEO_CTRL_STREAM(0), 0x0);
	dptx_writel(dptx, DPTX_VIDEO_CTRL_STREAM(1), 0x0);

	return 0;
}

/**
 * dptx_core_deinit() - Deinitialize the core
 * @dptx: The dptx struct
 *
 * Disable the core in preparation for module shutdown.
 */
int dptx_core_deinit(struct dp_ctrl *dptx)
{
	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] NULL Pointer\n");

	//dptx_global_intr_dis(dptx);
	dptx_soft_reset_all(dptx);
	/* config security reg, disenable video transform */
	dptx_hdcp_enable_video_transform(dptx, false);

	if (dptx->dptx_aux_disreset)
		dptx->dptx_aux_disreset(dptx, false);

	return 0;
}

static int dp_dis_reset(struct dp_ctrl *dptx, bool benable)
{
	uint32_t reg;
	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	if (g_fpga_flag == 1) {
		if (benable) {
			reg = inp32(dptx->connector_dev->base_addr.hsdt1_crg_base + DPTX_APB_DIS_RESET_OFFSET); /* 0xEB045000 + 0x84 */
			if (dptx->id == DPTX_DP0) {
				reg |= DPTX_APB_DIS_RESET_BIT;
				reg |= DPTX_DPC_DIS_RESET_BIT;
			} else if (dptx->id == DPTX_DP1) {
				reg |= DPTX1_APB_DIS_RESET_BIT;
				reg |= DPTX1_DPC_DIS_RESET_BIT;
			}
			outp32(dptx->connector_dev->base_addr.hsdt1_crg_base + DPTX_APB_DIS_RESET_OFFSET, reg);
			disp_pr_info("[Dp] test: disreset dp\n");
		} else {
			reg = inp32(dptx->connector_dev->base_addr.hsdt1_crg_base + DPTX_APB_RESET_OFFSET); /* 0xEB045000 + 0x80 */
			if (dptx->id == DPTX_DP0)
				reg |= DPTX_APB_RESET_BIT;
			else if (dptx->id == DPTX_DP1)
				reg |= DPTX1_APB_RESET_BIT;
			outp32(dptx->connector_dev->base_addr.hsdt1_crg_base + DPTX_APB_RESET_OFFSET, reg);
		}
	}

	return 0;
}

int dptx_core_on(struct dp_ctrl *dptx)
{
	int ret = 0;
	uint32_t phyifctrl;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] NULL Pointer\n");

	if (!dptx_check_dptx_id(dptx)) {
		disp_pr_err("[DP] DPTX_ID not match !\n");
		return -EINVAL;
	}

	disp_pr_err("[Dp] test708: %x\n", dptx_readl(dptx, DPTX_GCTL0));
	phyifctrl = dptx_readl(dptx, DPTX_GCTL0);
	phyifctrl &= ~DPTX_CFG_MODE_SELECT_MASK;
	phyifctrl |= DPTX_HPD_POLARITY_SEL;
	dptx_writel(dptx, DPTX_GCTL0, phyifctrl);

	dptx_writel(dptx, DPTX_PHYIF_CTRL1, 0x0);

	disp_pr_err("[Dp] test708: %x\n", dptx_readl(dptx, DPTX_GCTL0));

	dptx_init_hwparams(dptx);

	/* set lowpower mode: dptx_core_set_low_power_mode(dptx); */

	ret = dptx_core_init(dptx);
	if (ret) {
		disp_pr_err("[DP] DPTX core init failed!\n");
		return ret;
	}

	/* FIXME: clear intr */
	dptx_global_intr_dis(dptx);
	enable_irq(dptx->irq);
	/* Enable all top-level interrupts */
	dptx_global_intr_en(dptx);

	return 0;
}

void dptx_core_off(struct dp_ctrl *dptx)
{
	disp_check_and_no_retval((dptx == NULL), err, "[DP] dptx is NULL\n");

	/* FIXME: clear intr */
	dptx_global_intr_dis(dptx);

	disable_irq(dptx->irq);

	dptx_core_deinit(dptx);
}

void dptx_core_remove(struct dp_ctrl *dptx)
{
	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");

	dptx_core_deinit(dptx);
}

void dptx_free_lanes(struct dp_ctrl *dptx)
{
	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");

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

	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");

	disp_pr_info("[DP] DPTX set lanes =%d\n", lanes);

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
		disp_pr_err("[DP] Invalid number of lanes %d\n", lanes);
		return;
	}

	phyifctrl = dptx_readl(dptx, DPTX_PHYIF_CTRL0);
	phyifctrl &= ~DPTX_LANE_DATA_EN_MASK;
	phyifctrl |= val << DPTX_LANE_DATA_EN_SHIFT;
	dptx_writel(dptx, DPTX_PHYIF_CTRL0, phyifctrl);

	val = dptx_readl(dptx, DPTX_GCTL0);
	disp_pr_err("[Dp] test708: %x\n", val);
	val &= ~GENMASK(2, 1);
	val |= ((lanes / 2) << 1);
	dptx_writel(dptx, DPTX_GCTL0, val);

	disp_pr_err("[Dp] test708: %x\n", dptx_readl(dptx, DPTX_GCTL0));
}

static void dptx_phy_disable_lane_data(struct dp_ctrl *dptx)
{
	uint32_t phyifctrl;

	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");

	phyifctrl = dptx_readl(dptx, DPTX_PHYIF_CTRL0);
	phyifctrl &= ~DPTX_LANE_DATA_EN_MASK;
	dptx_writel(dptx, DPTX_PHYIF_CTRL0, phyifctrl);
}

void dptx_phy_set_rate(struct dp_ctrl *dptx, uint32_t rate)
{
	uint32_t phyifctrl;
	int retval;
	bool rate_changed = false;

	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");

	disp_pr_info("[DP] DPTX set rate=%d\n", rate);

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
		disp_pr_err("[DP] Invalid PHY rate %d\n", rate);
		return;
	}

	phyifctrl = rate;
	dptx_writel(dptx, DPTX_PHYIF_CTRL2, phyifctrl);

	if (rate_changed) {
		retval = dptx_phy_wait_busy(dptx, dptx->link.lanes);
		if (retval)
			disp_pr_err("[DP] set_rate fail\n");
	}
}

void dptx_phy_data_lane_clear(struct dp_ctrl *dptx)
{
	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");

	/* set 0xA0 to 0 */
	dptx_writel(dptx, DPTX_PHYIF_CTRL0, 0x0);
}

int dptx_phy_wait_busy(struct dp_ctrl *dptx, uint32_t lanes)
{
	unsigned long dw_jiffies = 0;
	uint32_t phyifctrl;
	uint32_t mask = 0;

	disp_pr_info("[DP] lanes=%d\n", lanes);
	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] NULL Pointer\n");

	/* FPGA don't need config phy */
	if (g_fpga_flag == 1)
		return 0;

	switch (lanes) {
	/* This case (value 4) is not terminated by a 'break' statement */
	case 4:
		mask |= DPTX_PHYIF_CTRL_BUSY(3);
		mask |= DPTX_PHYIF_CTRL_BUSY(2);
	/* This case (value 2) is not terminated by a 'break' statement */
	case 2:
		mask |= DPTX_PHYIF_CTRL_BUSY(1);
	case 1:
		mask |= DPTX_PHYIF_CTRL_BUSY(0);
		break;
	default:
		disp_pr_err("[DP] Invalid number of lanes %d\n", lanes);
		break;
	}

	dw_jiffies = jiffies + HZ / 10;

	do {
		phyifctrl = dptx_readl(dptx, DPTX_PHYIF_STATUS0);
		if ((phyifctrl & mask) == mask)
			return 0;
		udelay(1);
	} while (time_after(dw_jiffies, jiffies));

	disp_pr_err("[DP] PHY BUSY timed out\n");

	return -EBUSY;
}

static int dptx_phy_wait_changed_lane_busy(struct dp_ctrl *dptx, uint32_t phyifctrl, uint32_t orig_phyifctrl)
{
	unsigned long dw_jiffies = 0;
	uint32_t val;
	uint32_t mask = 0;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] NULL Pointer\n");

	/* FPGA don't need config phy */
	if (g_fpga_flag == 1)
		return 0;

	if ((phyifctrl & DPTX_LANE3_PHY_STATUS_MASK) != (orig_phyifctrl & DPTX_LANE3_PHY_STATUS_MASK))
		mask |= DPTX_PHYIF_CTRL_BUSY(3);
	if ((phyifctrl & DPTX_LANE2_PHY_STATUS_MASK) != (orig_phyifctrl & DPTX_LANE2_PHY_STATUS_MASK))
		mask |= DPTX_PHYIF_CTRL_BUSY(2);
	if ((phyifctrl & DPTX_LANE1_PHY_STATUS_MASK) != (orig_phyifctrl & DPTX_LANE1_PHY_STATUS_MASK))
		mask |= DPTX_PHYIF_CTRL_BUSY(1);
	if ((phyifctrl & DPTX_LANE0_PHY_STATUS_MASK) != (orig_phyifctrl & DPTX_LANE0_PHY_STATUS_MASK))
		mask |= DPTX_PHYIF_CTRL_BUSY(0);

	dw_jiffies = jiffies + HZ / 10;

	do {
		val = dptx_readl(dptx, DPTX_PHYIF_STATUS0);
		if ((val & mask) == mask)
			return 0;
		udelay(1);
	} while (time_after(dw_jiffies, jiffies));

	disp_pr_err("[DP] PHY BUSY timed out\n");

	return -EBUSY;
}

void dptx_phy_set_pattern(struct dp_ctrl *dptx, uint32_t pattern)
{
	uint32_t phyifctrl = 0;

	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");

	disp_pr_info("[DP] Setting PHY pattern=0x%x\n", pattern);

	phyifctrl = dptx_readl(dptx, DPTX_PHYIF_CTRL0);
	phyifctrl &= ~DPTX_PATTERN_SEL_MASK;
	phyifctrl |= ((pattern << DPTX_PATTERN_SEL_SHIFT) &
			  DPTX_PATTERN_SEL_MASK);
	dptx_writel(dptx, DPTX_PHYIF_CTRL0, phyifctrl);
}

void dptx_phy_lane_reset(struct dp_ctrl *dptx, bool bopen)
{
	uint32_t phyifctrl;

	disp_pr_info("[DP] dptx%d enter++++\n", dptx->id);

	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");

	// FIXME: DPRX & DPTX PHY coupled in power-supply, this is a work-around
	if (dptx->source_is_dprx) {
		disp_pr_warn("skip dptx%d phy reset, as RX & TX coupled in phy power\n", dptx->id);
		return;
	}

	phyifctrl = dptx_readl(dptx, DPTX_PHYIF_CTRL1);

	if (bopen) /* P0 mode, disreset lanes */
		phyifctrl = CFG_PHY_LANES(DPTX_MAX_LINK_LANES);
	else
		phyifctrl &= ~CFG_PHY_LANES(DPTX_MAX_LINK_LANES);

	dptx_writel(dptx, DPTX_PHYIF_CTRL1, phyifctrl);

	disp_pr_info("[DP] dptx%d exit----\n", dptx->id);
}

void dptx_phy_set_lanes_power(struct dp_ctrl *dptx, bool bopen)
{
	uint32_t phyifctrl;
	uint32_t lanes;

	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");

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
		disp_pr_err("[DP] Invalid number of lanes %d\n", lanes);
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
			disp_pr_err("[DP] Invalid number of lanes %d\n", lanes);
			return;
		}
	}

	dptx_writel(dptx, DPTX_PHYIF_CTRL3, phyifctrl);
}

void dptx_phy_set_ssc(struct dp_ctrl *dptx, bool bswitchphy)
{
	int retval;
	uint8_t byte = 0;
	bool sink_support_ssc = false;
	bool open_ssc = false;

	/* FPGA don't need config phy */
	if (g_fpga_flag == 1)
		return;

	if (dptx == NULL) {
		disp_pr_err("[DP] NULL Pointer\n");
		return;
	}

	retval = dptx_read_dpcd(dptx, DP_MAX_DOWNSPREAD, &byte);
	if (retval) {
		disp_pr_err("[DP] Read DPCD error\n");
		return;
	}

	sink_support_ssc = byte & DP_MAX_DOWNSPREAD_0_5;

	disp_pr_info("[DP] SSC setting:  %x\n", byte);

	open_ssc = sink_support_ssc && dptx->ssc_en;

	if (dptx->dptx_combophy_set_ssc_dis)
		dptx->dptx_combophy_set_ssc_dis(dptx, !open_ssc);

	retval = dptx_read_dpcd(dptx, DP_DOWNSPREAD_CTRL, &byte);
	if (retval) {
		disp_pr_err("[DP] Read DPCD error\n");
		return;
	}

	if (open_ssc)
		byte |= DP_SPREAD_AMP_0_5;
	else
		byte &= ~DP_SPREAD_AMP_0_5;

	retval = dptx_write_dpcd(dptx, DP_DOWNSPREAD_CTRL, byte);
	if (retval) {
		disp_pr_err("[DP] write DPCD error\n");
		return;
	}
}

static void dptx_default_params_from_core(struct dp_ctrl *dptx)
{
	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");

	if (g_fpga_flag == 1)
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

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] NULL Pointer\n");

	byte = dptx->link.lanes;
	cctl = dptx_readl(dptx, DPTX_VIDEO_CTRL_STREAM(0));
	/* enhance frame */
	if (dptx->efm_en && drm_dp_enhanced_frame_cap(dptx->rx_caps)) {
		byte |= DP_ENHANCED_FRAME_CAP;
		cctl |= DPTX_STREAM_FRAME_MODE;
	} else {
		cctl &= ~DPTX_STREAM_FRAME_MODE;
	}

	dptx_writel(dptx, DPTX_VIDEO_CTRL_STREAM(0), cctl);

	retval = dptx_write_dpcd(dptx, DP_LANE_COUNT_SET, byte);
	if (retval)
		return retval;

	/* ASSR config */
	if (dptx->mode == EDP_MODE) {
		byte = 0;
		retval = dptx_read_dpcd(dptx, 0xD, &byte);
		if (retval) {
			disp_pr_err("[DP] read 0xD fail\n");
			return retval;
		}

		if (byte & BIT(0) != 0) {
			disp_pr_info("[DP] edp support ASSR\n");
			byte = 0;
			retval = dptx_read_dpcd(dptx, 0x10A, &byte);
			if (retval) {
				disp_pr_err("[DP] read 0x10A fail\n");
				return retval;
			}
			byte |= BIT(0);
			retval = dptx_write_dpcd(dptx, 0x10A, byte);
			if (retval) {
				disp_pr_err("[DP] write 0x10A fail\n");
				return retval;
			}
			uint32_t phyifctrl;
			phyifctrl = dptx_readl(dptx, DPTX_GCTL0);
			phyifctrl |= BIT(7);
			dptx_writel(dptx, DPTX_GCTL0, phyifctrl);
		} else {
			disp_pr_err("[DP] edp don't support ASSR\n");
		}
	}

	return 0;
}

static bool dptx_sink_device_connected(struct dp_ctrl *dptx)
{
	disp_check_and_return((dptx == NULL), false, err, "[DP] dptx is NULL\n");

	return dptx->dptx_enable;
}

void dptx_link_core_reset(struct dp_ctrl *dptx)
{
	int retval;

	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");

	dptx_soft_reset_all(dptx);

	if (dptx->dptx_aux_disreset)
		dptx->dptx_aux_disreset(dptx, true);

	dptx_phy_lane_reset(dptx, false);
	mdelay(1);
	dptx_phy_lane_reset(dptx, true);
	/* Wait for PHY busy */
	retval = dptx_phy_wait_busy(dptx, dptx->max_lanes);
	if (retval)
		disp_pr_err("[DP] phy release fail\n");
}

void dptx_link_set_preemp_vswing(struct dp_ctrl *dptx)
{
	uint32_t i;
	uint8_t pe;
	uint8_t vs;

	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");

	/* FPGA don't need config phy */
	if (g_fpga_flag == 1)
		return;

	for (i = 0; i < dptx->link.lanes; i++) {
		huawei_dp_imonitor_set_param_vs_pe(i, &(dptx->link.vswing_level[i]), &(dptx->link.preemp_level[i]));
		pe = dptx->link.preemp_level[i];
		vs = dptx->link.vswing_level[i];
		disp_pr_info("[DP] lane: %d, vs: %d, pe: %d\n", i, vs, pe);
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

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL\n");

	dptx_phy_disable_lane_data(dptx);

	orig_phyifctrl = dptx_readl(dptx, DPTX_PHYIF_CTRL3);
	dptx_phy_set_lanes_power(dptx, true);
	phyifctrl = dptx_readl(dptx, DPTX_PHYIF_CTRL3);
	/* Wait for PHY busy */
	retval = dptx_phy_wait_changed_lane_busy(dptx, phyifctrl, orig_phyifctrl);
	if (retval)
		disp_pr_err("[DP] set power P0 fail\n");

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
	if (retval)
		return retval;

	retval = dptx_phy_rate_to_bw(dptx->link.rate);
	if (retval < 0)
		return retval;

	byte = (uint8_t)retval;
	retval = dptx_write_dpcd(dptx, DP_LINK_BW_SET, byte);
	if (retval)
		return retval;

	retval = dptx_set_enhanced_framing_mode(dptx);
	if (retval)
		return retval;

	byte = DP_SPREAD_AMP_0_5;
	retval = dptx_write_dpcd(dptx, DP_DOWNSPREAD_CTRL, byte);
	if (retval)
		return retval;

	byte = 1;
	retval = dptx_write_dpcd(dptx, DP_MAIN_LINK_CHANNEL_CODING_SET, byte);
	if (retval)
		return retval;

	return 0;
}

void dptx_initiate_mst_act(struct dp_ctrl *dptx)
{
	uint32_t reg;

	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");

	reg = dptx_readl(dptx, DPTX_MST_CTRL);
	reg |= DPTX_CFG_SOFT_ACT_TRG;
	dptx_writel(dptx, DPTX_MST_CTRL, reg);

}

void dptx_clear_vcpid_table(struct dp_ctrl *dptx)
{
	int i;

	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");

	for (i = 0; i < 8; i++)
		dptx_writel(dptx, DPTX_MST_VCP_TABLE_REG_N(i), 0);
}

void dptx_set_vcpid_table_slot(struct dp_ctrl *dptx,
	uint32_t slot, uint32_t stream)
{
	uint32_t offset;
	uint32_t reg;
	uint32_t lsb;
	uint32_t mask;

	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");
	disp_check_and_no_retval((slot > 63), err, "Invalid slot number > 63");

	offset = DPTX_MST_VCP_TABLE_REG_N(slot >> 3);
	reg = dptx_readl(dptx, offset);

	lsb = (slot & 0x7) * 4;
	mask = GENMASK(lsb + 3, lsb);

	reg &= ~mask;
	reg |= (stream << lsb) & mask;

	disp_pr_debug("[DP] Writing 0x%08x val=0x%08x\n", offset, reg);
	dptx_writel(dptx, offset, reg);
}

void dptx_ctrl_layer_init(struct dp_ctrl *dptx)
{
	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");

	dptx->aux_rw = dptx_aux_rw;

	dptx->dptx_hpd_handler = dptx_hpd_handler;
	dptx->dptx_hpd_irq_handler = dptx_hpd_irq_handler;
	dptx->dptx_irq = dptx_irq;
	dptx->dptx_threaded_irq = dptx_threaded_irq;

	dptx->dp_dis_reset = dp_dis_reset;
	dptx->dptx_core_on = dptx_core_on;
	dptx->dptx_core_off = dptx_core_off;
	dptx->dptx_core_remove = dptx_core_remove;

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
	dptx->dptx_enable_dpu_pipeline = dptx_enable_dpu_pipeline;
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

void test_fast_link_training(struct dp_ctrl *dptx)
{
	uint32_t pattern;

	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");
	disp_pr_err("[DP] +\n");

	dptx_phy_set_pattern(dptx, DPTX_PHYIF_CTRL_TPS_1);
	udelay(500);

	switch (dptx->max_rate) {
	case DPTX_PHYIF_CTRL_RATE_HBR3:
		if (drm_dp_tps4_supported(dptx->rx_caps)) {
			pattern = DPTX_PHYIF_CTRL_TPS_4;
			break;
		}
		/* Fall through */
	case DPTX_PHYIF_CTRL_RATE_HBR2:
		if (drm_dp_tps3_supported(dptx->rx_caps)) {
			pattern = DPTX_PHYIF_CTRL_TPS_3;
			break;
		}
		/* Fall through */
	case DPTX_PHYIF_CTRL_RATE_RBR:
	case DPTX_PHYIF_CTRL_RATE_HBR:
		pattern = DPTX_PHYIF_CTRL_TPS_2;
		break;
	default:
		disp_pr_err("[DP] Invalid rate %d\n", dptx->link.rate);
		return;
	}

	dptx_phy_set_pattern(dptx, pattern);
	udelay(500);
	dptx_phy_set_pattern(dptx, DPTX_PHYIF_CTRL_TPS_NONE);
	disp_pr_err("[DP] -\n");
}
/*lint -restore*/
