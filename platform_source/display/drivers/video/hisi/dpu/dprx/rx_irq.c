/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#include "rx_irq.h"
#include "rx_core.h"
#include "rx_reg.h"
#include "../device/hisi_disp.h"

static void dprx_irq_power_ctl(uint32_t irq_sts, struct dprx_ctrl *dprx, uint8_t *dpcd_value)
{
	uint32_t reg;
	uint32_t phy_value;

	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_TRAINING_INTR_ORI_STATUS, DPRX_IRQ_AUX_WR_600);
	*dpcd_value = dprx_read_dpcd(dprx, 0x600);
	dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_600: powerstate cur:%d, last:%d \n", *dpcd_value, dprx->dpcds.power_state);
	if ((*dpcd_value == 0x2 || *dpcd_value == 0x5) && (dprx->dpcds.power_state == 1)) {  // enter P3
		dpu_pr_info("[DPRX] enter Low power consumption\n");
		// 1、close video
		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_VIDEO_CTRL);
		reg &= ~VIDEO_OUT_EN;
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_VIDEO_CTRL, reg);
		// 2、close hs_det en
		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_HS_DET_CTRL0);
		reg &= ~DPRX_HS_DET_EN;
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_HS_DET_CTRL0, reg);
		// 3、soft ctl lane
		phy_value = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RST_CTRL);
		phy_value |= LANE_POWERDOWN_SEL;
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RST_CTRL, phy_value);
		// 4、enter P3
		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RWR_CTRL);
		reg &= ~DPRX_PHY_LANE_POWERDOWN;
		reg |= DPRX_4LANE_ENTER_P3; // 0:P0   1:P1   2:P2   3:P3, 0~3 lane
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RWR_CTRL, reg);
		// 5、todo : wait 200um
		// 6、restore hardware ctl lane
		phy_value &= ~LANE_POWERDOWN_SEL;
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RST_CTRL, phy_value);

		dprx->local_event_notify(dprx, DPRX_IRQ_ATC_TRAIN_LOST);
	}
	if (*dpcd_value == 0x1) {
		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RWR_CTRL);
		dpu_pr_info("[DPRX] reg : 0x%x\n", reg);
		if ((dprx->dpcds.power_state == 2 || dprx->dpcds.power_state == 5) ||
			((reg & DPRX_PHY_LANE_POWERDOWN) == DPRX_4LANE_ENTER_P3)) { // if P3 then exist P3
			dpu_pr_info("[DPRX] Exit Low Power Consumption\n", reg);
			// 1、soft ctl lane
			phy_value = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RST_CTRL);
			phy_value |= LANE_POWERDOWN_SEL;
			dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RST_CTRL, phy_value);
			// 2、enter P0
			reg &= ~DPRX_PHY_LANE_POWERDOWN;
			dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RWR_CTRL, reg);
			// 3、todo : wait 50um
			// 4、restore hardware ctl lane
			phy_value &= ~LANE_POWERDOWN_SEL;
			dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RST_CTRL, phy_value);
			// 5、open hs_det en
			reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_HS_DET_CTRL0);
			reg |= DPRX_HS_DET_EN;
			dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_HS_DET_CTRL0, reg);
		}
	}
}

static void dprx_handle_training_irq(struct dprx_ctrl *dprx)
{
	uint32_t irq_sts;
	uint32_t reg;
	uint8_t dpcd_value;
	uint32_t phyifctrl;

	irq_sts = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_TRAINING_INTR_MASKED_STATUS);
	if (irq_sts & DPRX_IRQ_ATC_TRAIN_TIMEOUT) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_TRAINING_INTR_ORI_STATUS, DPRX_IRQ_ATC_TRAIN_TIMEOUT);
		dpu_pr_info("[DPRX] DPRX_IRQ_ATC_TRAIN_TIMEOUT\n");
		// todo
	}

	if (irq_sts & DPRX_IRQ_ATC_TRAIN_DONE) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_TRAINING_INTR_ORI_STATUS, DPRX_IRQ_ATC_TRAIN_DONE);
		dpu_pr_info("[DPRX] DPRX_IRQ_ATC_TRAIN_DONE\n");
		// todo
	}

	if (irq_sts & DPRX_IRQ_DESKEW_DONE) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_TRAINING_INTR_ORI_STATUS, DPRX_IRQ_DESKEW_DONE);
		dpu_pr_info("[DPRX] DPRX_IRQ_DESKEW_DONE\n");
		// todo
	}

	if (irq_sts & DPRX_IRQ_DESKEW_TIMEOUT) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_TRAINING_INTR_ORI_STATUS, DPRX_IRQ_DESKEW_TIMEOUT);
		dpu_pr_info("[DPRX] DPRX_IRQ_DESKEW_TIMEOUT\n");
		// todo
	}

	if (irq_sts & DPRX_IRQ_ATC_PWR_DONE_LOST) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_TRAINING_INTR_ORI_STATUS, DPRX_IRQ_ATC_PWR_DONE_LOST);
		dpu_pr_info("[DPRX] DPRX_IRQ_ATC_PWR_DONE_LOST\n");
		// todo
	}

	if (irq_sts & DPRX_IRQ_AUX_WR_100) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_TRAINING_INTR_ORI_STATUS, DPRX_IRQ_AUX_WR_100);
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_100\n");
		// todo
	}

	if (irq_sts & DPRX_IRQ_AUX_WR_101) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_TRAINING_INTR_ORI_STATUS, DPRX_IRQ_AUX_WR_101);
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_101\n");
		// todo
	}

	if (irq_sts & DPRX_IRQ_AUX_WR_102) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_TRAINING_INTR_ORI_STATUS, DPRX_IRQ_AUX_WR_102);
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_102\n");
		// for test
		reg = dprx_readl(dprx, 0x100);
		dpcd_value = (reg & GENMASK(23, 16)) >> 16;
		dpu_pr_info("[DPRX] DPCD103-100 = 0x%x, last102 = 0x%x\n", reg, dprx->dpcds.link_cfgs[2]);
		if ((dprx->dpcds.link_cfgs[2] & GENMASK(3, 0)) != (dpcd_value & GENMASK(3, 0))) {
			if ((dpcd_value & GENMASK(3, 0)) == 1) {
				phyifctrl = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RST_CTRL);
				phyifctrl &= ~DPRX_PHY_LANE_RESET;
				dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RST_CTRL, phyifctrl);
				phyifctrl |= DPRX_PHY_LANE_RESET;
				dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RST_CTRL, phyifctrl);
				dpu_pr_info("[DPRX] TPS1 Sel, Phy reset\n");
			}
		}
		dprx->dpcds.link_cfgs[2] = dpcd_value;
	}

	if (irq_sts & DPRX_IRQ_AUX_WR_103) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_TRAINING_INTR_ORI_STATUS, DPRX_IRQ_AUX_WR_103);
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_103\n");
		// todo
	}

	if (irq_sts & DPRX_IRQ_AUX_WR_104) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_TRAINING_INTR_ORI_STATUS, DPRX_IRQ_AUX_WR_104);
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_104\n");
		// todo
	}

	if (irq_sts & DPRX_IRQ_AUX_WR_105) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_TRAINING_INTR_ORI_STATUS, DPRX_IRQ_AUX_WR_105);
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_105\n");
		// todo
	}

	if (irq_sts & DPRX_IRQ_AUX_WR_106) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_TRAINING_INTR_ORI_STATUS, DPRX_IRQ_AUX_WR_106);
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_106\n");
		// todo
	}

	if (irq_sts & DPRX_IRQ_AUX_WR_600) {
		dprx_irq_power_ctl(irq_sts, dprx, &dpcd_value);
		dprx->dpcds.power_state = dpcd_value;
	}

	if (irq_sts & DPRX_IRQ_MAIN_LINK_STS_DET_LOST) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_TRAINING_INTR_ORI_STATUS, DPRX_IRQ_MAIN_LINK_STS_DET_LOST);
		dpu_pr_info("[DPRX] DPRX_IRQ_MAIN_LINK_STS_DET_LOST\n");
		// todo
	}

	if (irq_sts & DPRX_IRQ_TPS234_DET) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_TRAINING_INTR_ORI_STATUS, DPRX_IRQ_TPS234_DET);
		dpu_pr_info("[DPRX] DPRX_IRQ_TPS234_DET\n");
		// todo
	}

	if (irq_sts & DPRX_IRQ_ATC_TRAIN_LOST) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_TRAINING_INTR_ORI_STATUS, DPRX_IRQ_ATC_TRAIN_LOST);
		dpu_pr_info("[DPRX] DPRX_IRQ_ATC_TRAIN_LOST\n");
		phyifctrl = dprx_readl(dprx, DPRX_SCTRL_OFFSET + SDP_FIFO_CLR);
		phyifctrl |= ICFG_SDP_FIFO_CLR;
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + SDP_FIFO_CLR, phyifctrl);
		phyifctrl &= ~ICFG_SDP_FIFO_CLR;
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + SDP_FIFO_CLR, phyifctrl);
		dpu_pr_info("[DPRX] fifo reset\n");

		dprx->local_event_notify(dprx, DPRX_IRQ_ATC_TRAIN_LOST);
	}
}

static void dprx_handle_timing_irq(struct dprx_ctrl *dprx)
{
	uint32_t irq_sts;

	irq_sts = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_TIMING_INTR_MASKED_STATUS);
	if (irq_sts & DPRX_IRQ_VSYNC)
		dpu_pr_debug("[DPRX] DPRX_IRQ_VSYNC\n");

	if (irq_sts & DPRX_IRQ_HSYNC)
		dpu_pr_debug("[DPRX] DPRX_IRQ_HSYNC\n");

	if (irq_sts & DPRX_IRQ_VBS)
		dpu_pr_debug("[DPRX] DPRX_IRQ_VBS\n");

	if (irq_sts & DPRX_IRQ_VSTART)
		dpu_pr_debug("[DPRX] DPRX_IRQ_VSTART\n");

	if (irq_sts & DPRX_IRQ_VSYNC_TO_DSS)
		dpu_pr_debug("[DPRX] DPRX_IRQ_VSYNC_TO_DSS\n");

	if (irq_sts & DPRX_IRQ_HSYNC_TO_DSS)
		dpu_pr_debug("[DPRX] DPRX_IRQ_HSYNC_TO_DSS\n");

	if (irq_sts & DPRX_IRQ_VBS_TO_DSS)
		dpu_pr_debug("[DPRX] DPRX_IRQ_VBS_TO_DSS\n");

	if (irq_sts & DPRX_IRQ_VSTART_TO_DSS)
		dpu_pr_debug("[DPRX] DPRX_IRQ_VSTART_TO_DSS\n");

	if (irq_sts & DPRX_IRQ_VFP)
		dpu_pr_debug("[DPRX] DPRX_IRQ_VFP\n");

	if (irq_sts & DPRX_IRQ_VACTIVE1)
		dpu_pr_debug("[DPRX] DPRX_IRQ_VACTIVE1\n");

	if (irq_sts & DPRX_IRQ_VBS_DPRX)
		dpu_pr_debug("[DPRX] DPRX_IRQ_VBS_DPRX\n");

	if (irq_sts & DPRX_IRQ_TIME_COUNT)
		dpu_pr_info("[DPRX] DPRX_IRQ_TIME_COJNT\n");

	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_TIMING_INTR_ORI_STATUS, irq_sts);
}

static const struct pixel_enc_info {
	enum pixel_enc_type pix_enc;
	char desc[10];
} enc_info_list[PIXEL_ENC_INVALID] = {
	{ RGB, "RGB" },
	{ YCBCR420, "YUV420" },
	{ YCBCR422, "YUV422" },
	{ YCBCR444, "YUV444" },
	{ YONLY, "YONLY" },
	{ RAW, "RAW" },
};

static void dprx_handle_video_parmas(struct dprx_ctrl *dprx)
{
	if (dprx->v_params.video_format_valid == false || dprx->msa.msa_valid == false)
		return;

	dprx->video_input_params_ready = true;
	dpu_pr_info("Get Input Parameters:");
	if (dprx->v_params.pix_enc >= PIXEL_ENC_INVALID)
		dpu_pr_info("ColorSpace: Invalid\n");
	else
		dpu_pr_info("ColorSpace: %s\n", enc_info_list[dprx->v_params.pix_enc].desc);
	dpu_pr_info("Bpc: %u\n", dprx->v_params.bpc);

	dpu_pr_info("**************************************************************");
	dpu_pr_info("MSA MVID, NVID: %u, %u\n", dprx->msa.mvid, dprx->msa.nvid);
	dpu_pr_info("MSA h_total, h_width, h_start, hsw, hsp: %u, %u, %u, %u, %u\n",
				dprx->msa.h_total, dprx->msa.h_width, dprx->msa.h_start, dprx->msa.hsw, dprx->msa.hsp);
	dpu_pr_info("MSA v_total, v_height, v_start, vsw, vsp: %u, %u, %u, %u, %u\n",
				dprx->msa.v_total, dprx->msa.v_height, dprx->msa.v_start, dprx->msa.vsw, dprx->msa.vsp);
	dpu_pr_info("MSA misc0, misc1: 0x%x, 0x%x\n", dprx->msa.misc0, dprx->msa.misc1);

	dprx->local_event_notify(dprx, DPRX_IRQ_FORMAT_CHANGE | DPRX_IRQ_TIMING_CHANGE);
}

static void dprx_handle_timing_change(struct dprx_ctrl *dprx)
{
	uint32_t reg;

	reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_MSA_STATUS);
	if (reg & DPRX_MSA_VALID_BIT) {
		/* for test, change to dpu_pr_debug */
		dpu_pr_info("MSA Valid\n");
		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_MSA_MVID);
		dprx->msa.mvid = reg & DPRX_MSA_MVID_MASK;

		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_MSA_NVID);
		dprx->msa.nvid = reg & DPRX_MSA_NVID_MASK;

		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_MISC);
		dprx->msa.misc0 = (reg & DPRX_MISC0_MASK) >> DPRX_MISC0_OFFSET;
		dprx->msa.misc1 = (reg & DPRX_MISC1_MASK) >> DPRX_MISC1_OFFSET;

		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_MSA_TIMING0);
		dprx->msa.v_total = (reg & DPRX_MSA_VTOTAL_MASK) >> DPRX_MSA_VTOTAL_OFFSET;
		dprx->msa.h_total = (reg & DPRX_MSA_HTOTAL_MASK) >> DPRX_MSA_HTOTAL_OFFSET;

		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_MSA_TIMING1);
		dprx->msa.v_start = (reg & DPRX_MSA_VSTART_MASK) >> DPRX_MSA_VSTART_OFFSET;
		dprx->msa.h_start = (reg & DPRX_MSA_HSTART_MASK) >> DPRX_MSA_HSTART_OFFSET;

		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_MSA_TIMING2);
		dprx->msa.vsw = (reg & DPRX_MSA_VSW_MASK) >> DPRX_MSA_VSW_OFFSET;
		dprx->msa.vsp= (reg & DPRX_MSA_VSP_MASK) >> DPRX_MSA_VSP_OFFSET;
		dprx->msa.hsw = (reg & DPRX_MSA_HSW_MASK) >> DPRX_MSA_HSW_OFFSET;
		dprx->msa.hsp= (reg & DPRX_MSA_HSP_MASK) >> DPRX_MSA_HSP_OFFSET;

		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_MSA_TIMING3);
		dprx->msa.v_height = (reg & DPRX_MSA_VHEIGHT_MASK) >> DPRX_MSA_VHEIGHT_OFFSET;
		dprx->msa.h_width = (reg & DPRX_MSA_HWIDTH_MASK) >> DPRX_MSA_HWIDTH_OFFSET;

		dprx->msa.msa_valid = true;
	} else {
		dpu_pr_info("MSA Invalid\n");
		dprx->msa.msa_valid = false;
	}
}

static const enum color_depth color_depth_maping[DPRX_COLOR_DEPTH_INDEX_MAX] = {
	COLOR_DEPTH_6, COLOR_DEPTH_7, COLOR_DEPTH_8, COLOR_DEPTH_10,
	COLOR_DEPTH_12, COLOR_DEPTH_14, COLOR_DEPTH_16
};

static const enum pixel_enc_type pixel_enc_maping[DPRX_PIXEL_ENC_INDEX_MAX] = {
	RGB, YCBCR422, YCBCR420, YONLY, RAW, YCBCR444
};

static void dprx_handle_format_change(struct dprx_ctrl *dprx)
{
	uint32_t reg;
	uint32_t value;

	reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_LINK_STATUS);
	if (reg & DPRX_VIDEO_FMT_VALID) {
		/* for test, change to dpu_pr_debug */
		dpu_pr_info("Format Valid\n");

		value = (reg & DPRX_COLOR_SPACE_MASK) >> DPRX_COLOR_SPACE_OFFSET;
		if (value >= DPRX_PIXEL_ENC_INDEX_MAX)
			dprx->v_params.pix_enc = PIXEL_ENC_INVALID;
		else
			dprx->v_params.pix_enc = pixel_enc_maping[value];

		value = (reg & DPRX_BPC_NUM_MASK) >> DPRX_BPC_NUM_OFFSET;
		if (value >= DPRX_COLOR_DEPTH_INDEX_MAX)
			dprx->v_params.bpc = COLOR_DEPTH_INVALID;
		else
			dprx->v_params.bpc = color_depth_maping[value];

		if ((dprx->v_params.bpc == COLOR_DEPTH_8) ||
			(dprx->v_params.bpc == COLOR_DEPTH_10) ||
			(dprx->v_params.bpc == COLOR_DEPTH_6 && dprx->v_params.pix_enc == RGB)) {
			dprx->v_params.video_format_valid = true;
		} else {
			dpu_pr_err("Format Invalid\n");
			dprx->v_params.video_format_valid = false;
		}
	} else {
		dpu_pr_info("Format Invalid\n");
		dprx->v_params.video_format_valid = false;
	}
}

static void dprx_handle_psr_irq(struct dprx_ctrl *dprx)
{
	uint32_t irq_sts;
	uint32_t reg;

	irq_sts = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_PSR_INTR_MASKED_STATUS);
	if (irq_sts & DPRX_PSR_ENTRY)
		dpu_pr_info("[DPRX] psr entry success\n");

	if (irq_sts & DPRX_PSR_PRE_ENTRY)
		dpu_pr_info("[DPRX] psr pre entry\n");

	if (irq_sts & DPRX_PSR_UPDATE)
		dpu_pr_info("[DPRX] psr update success\n");

	if (irq_sts & DPRX_PSR_SU_UPDATE)
		dpu_pr_info("[DPRX] psr su update success\n");

	if (irq_sts & DPRX_PSR_EXIT)
		dpu_pr_info("[DPRX] psr exit success\n");

	if (irq_sts & DPRX_SU_INFO)
		dpu_pr_info("[DPRX] psr su info\n");

	if (irq_sts & DPRX_AUX_WR_00170H) {
		dpu_pr_info("[DPRX] wr DPCD 170h\n");
		reg = dprx_read_dpcd(dprx, 0x170);
		dpu_pr_err("[DPRX] DPCD 170h value:%d\n", reg);
		// todo
	}

	if (irq_sts & DPRX_PSR_ABORT)
		dpu_pr_info("[DPRX] psr abort\n");

	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_PSR_INTR_ORI_STATUS, irq_sts);
}

static void dprx_handle_sdp_and_format_irq(struct dprx_ctrl *dprx)
{
	uint32_t irq_sts;
	uint32_t reg;

	irq_sts = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_SDP_VIDEOFORMAT_MASKED_STATUS);
	if (irq_sts & DPRX_IRQ_VSYNC_REPORT_SDP) {
		dpu_pr_info("[DPRX] DPRX_IRQ_VSYNC_REPORT_SDP\n");
		if (dprx->dprx_sdp_wq != NULL)
			queue_work(dprx->dprx_sdp_wq, &(dprx->dprx_sdp_work));
	}

	if (irq_sts & DPRX_IRQ_SDP_DB_FIFO_AFULL)
		dpu_pr_info("[DPRX] DPRX_IRQ_SDP_DB_FIFO_AFULL\n");

	if (irq_sts & DPRX_IRQ_SDP_HB_FIFO_AFULL)
		dpu_pr_info("[DPRX] DPRX_IRQ_SDP_HB_FIFO_AFULL\n");

	if (irq_sts & DPRX_IRQ_SDP_ERR) {
		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_ATC_STATUS);
		if (((reg >> 4) & ATC_TRAIN_STATUS) == ATC_STATUS_TRAIN_DONE)
			dpu_pr_info("[DPRX] DPRX_IRQ_SDP_ERR\n");
		// todo
	}

	if (irq_sts & (DPRX_IRQ_FORMAT_CHANGE | DPRX_IRQ_TIMING_CHANGE)) {
		dpu_pr_info("[DPRX] DPRX_IRQ_TIMING_FORMAT_CHANGE\n");
		dprx_handle_format_change(dprx);
		dprx_handle_timing_change(dprx);
		dprx_handle_video_parmas(dprx);
	}

	if (irq_sts & DPRX_IRQ_COMP_CHANGE)
		dpu_pr_info("[DPRX] DPRX_IRQ_COMP_CHANGE\n");

	if (irq_sts & DPRX_IRQ_VIDEO2IDLE)
		dpu_pr_info("[DPRX] DPRX_IRQ_VIDEO2IDLE\n");

	if (irq_sts & DPRX_IRQ_IDLE2VIDEO)
		dpu_pr_info("[DPRX] DPRX_IRQ_IDLE2VIDEO\n");

	if (irq_sts & DPRX_IRQ_SDP_DB_W_ERROR)
		dpu_pr_info("[DPRX] DPRX_IRQ_SDP_DB_W_ERROR\n");

	if (irq_sts & DPRX_IRQ_SDP_HB_W_ERROR)
		dpu_pr_info("[DPRX] DPRX_IRQ_SDP_HB_W_ERROR\n");

	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_SDP_VIDEOFORMAT_ORI_STATUS, irq_sts);
}

irqreturn_t dprx_threaded_irq_handler(int irq, void *dev)
{
	dpu_check_and_return(!dev, IRQ_HANDLED, err, "[DPRX] dev is NULL\n");

	return IRQ_HANDLED;
}

void dprx_handle_linkcfg_irq(struct dprx_ctrl *dprx)
{
	uint32_t irq_sts;
	irq_sts = dprx_readl(dprx, DPRX_SCTRL_OFFSET + 0x00B1C);
	if (irq_sts & BIT(0)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB20, BIT(0));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_107 \n");
	}
	if (irq_sts & BIT(1)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB20, BIT(1));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_108 \n");
	}
	if (irq_sts & BIT(2)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB20, BIT(2));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_109 \n");
	}
	if (irq_sts & BIT(3)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB20, BIT(3));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_10a \n");
	}
	if (irq_sts & BIT(4)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB20, BIT(4));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_10b \n");
	}
	if (irq_sts & BIT(5)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB20, BIT(5));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_10c \n");
	}
	if (irq_sts & BIT(6)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB20, BIT(6));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_10d \n");
	}
	if (irq_sts & BIT(7)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB20, BIT(7));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_10e \n");
	}
	if (irq_sts & BIT(8)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB20, BIT(8));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_111 \n");
	}
	if (irq_sts & BIT(9)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB20, BIT(9));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_112 \n");
	}
	if (irq_sts & BIT(10)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB20, BIT(10));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_113 \n");
	}
	if (irq_sts & BIT(11)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB20, BIT(11));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_114 \n");
	}
	if (irq_sts & BIT(12)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB20, BIT(12));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_115 \n");
	}
	if (irq_sts & BIT(13)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB20, BIT(13));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_116 \n");
	}
	if (irq_sts & BIT(14)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB20, BIT(14));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_117 \n");
	}
	if (irq_sts & BIT(15)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB20, BIT(15));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_118 \n");
	}
	if (irq_sts & BIT(16)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB20, BIT(16));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_119 \n");
	}
	if (irq_sts & BIT(17)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB20, BIT(17));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_120 \n");
	}
}

void dprx_handle_gtc_irg(struct dprx_ctrl *dprx)
{
	uint32_t irq_sts;
	irq_sts = dprx_readl(dprx, DPRX_SCTRL_OFFSET + 0x00B28);
	if (irq_sts & BIT(1)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB2C, BIT(1));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_154 \n");
	}
	if (irq_sts & BIT(2)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB2C, BIT(2));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_155 \n");
	}
	if (irq_sts & BIT(3)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB2C, BIT(3));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_156 \n");
	}
	if (irq_sts & BIT(4)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB2C, BIT(4));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_157 \n");
	}
	if (irq_sts & BIT(5)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB2C, BIT(5));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_158 \n");
	}
	if (irq_sts & BIT(6)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB2C, BIT(6));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_159 \n");
	}
	if (irq_sts & BIT(7)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB2C, BIT(7));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_15a \n");
	}
	if (irq_sts & BIT(8)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB2C, BIT(8));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_15b \n");
	}
	if (irq_sts & BIT(9)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB2C, BIT(9));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_15c \n");
	}
	if (irq_sts & BIT(10)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB2C, BIT(10));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_15d \n");
	}
	if (irq_sts & BIT(11)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB2C, BIT(11));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_15e \n");
	}
	if (irq_sts & BIT(12)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB2C, BIT(12));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_15f \n");
	}
	if (irq_sts & BIT(13)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB2C, BIT(13));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_160 \n");
	}
}

void dprx_rsv_intr_wr30_3f_irq(uint32_t irq_sts, struct dprx_ctrl *dprx)
{
	if (irq_sts & BIT(1))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_30 \n");

	if (irq_sts & BIT(2))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_31 \n");

	if (irq_sts & BIT(3))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_32 \n");

	if (irq_sts & BIT(4))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_33 \n");

	if (irq_sts & BIT(5))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_34 \n");

	if (irq_sts & BIT(6))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_35 \n");

	if (irq_sts & BIT(7))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_36 \n");

	if (irq_sts & BIT(8))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_37 \n");

	if (irq_sts & BIT(9))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_38 \n");

	if (irq_sts & BIT(10))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_39 \n");

	if (irq_sts & BIT(11))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_3a \n");

	if (irq_sts & BIT(12))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_3b \n");

	if (irq_sts & BIT(13))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_3c \n");

	if (irq_sts & BIT(14))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_3d \n");

	if (irq_sts & BIT(15))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_3e \n");

	if (irq_sts & BIT(16))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_3f \n");
}

void dprx_handle_rsv_irq(struct dprx_ctrl *dprx)
{
	uint32_t irq_sts;
	irq_sts = dprx_readl(dprx, DPRX_SCTRL_OFFSET + 0x00B70);
	if (irq_sts & BIT(0))
		dpu_pr_info("[DPRX] DPRX_IRQ_ADAPTIVE_SYNC \n");

	dprx_rsv_intr_wr30_3f_irq(irq_sts, dprx);

	if (irq_sts & BIT(17))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_720 \n");

	if (irq_sts & BIT(18))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_721 \n");

	if (irq_sts & BIT(19))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_722 \n");

	if (irq_sts & BIT(20))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_723 \n");

	if (irq_sts & BIT(21))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_724 \n");

	if (irq_sts & BIT(22))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_202 \n");

	if (irq_sts & BIT(23))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_203 \n");

	if (irq_sts & BIT(24))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_20f \n");

	if (irq_sts & BIT(25))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_280 \n");

	if (irq_sts & BIT(26))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_200c \n");

	if (irq_sts & BIT(27))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_200d \n");

	if (irq_sts & BIT(28))
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_2011 \n");

	dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB74, irq_sts);
}

void dprx_handle_debug_irq(struct dprx_ctrl *dprx)
{
	uint32_t irq_sts;
	irq_sts = dprx_readl(dprx, DPRX_SCTRL_OFFSET + 0x00B64);
	if (irq_sts & BIT(0)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB68, BIT(0));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_PHY_WAKE \n");
	}
	if (irq_sts & BIT(1)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB68, BIT(1));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_ACCES_INT \n");
	}
	if (irq_sts & BIT(2)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB68, BIT(2));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_1ST_LINE_INT \n");
	}
	if (irq_sts & BIT(3)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB68, BIT(3));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_HPD_INT \n");
	}
	if (irq_sts & BIT(4)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB68, BIT(4));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_WR_270 \n");
	}
}

void dprx_aux_access_irq_mon0_15(uint32_t irq_sts, struct dprx_ctrl *dprx)
{
	if (irq_sts & BIT(0)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB80, BIT(0));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_MON0 \n");
	}
	if (irq_sts & BIT(1)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB80, BIT(1));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_MON1 \n");
	}
	if (irq_sts & BIT(2)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB80, BIT(2));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_MON2 \n");
	}
	if (irq_sts & BIT(3)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB80, BIT(3));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_MON3 \n");
	}
	if (irq_sts & BIT(4)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB80, BIT(4));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_MON4 \n");
	}
	if (irq_sts & BIT(5)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB80, BIT(5));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_MON5 \n");
	}
	if (irq_sts & BIT(6)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB80, BIT(6));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_MON6 \n");
	}
	if (irq_sts & BIT(7)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB80, BIT(7));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_MON7 \n");
	}
	if (irq_sts & BIT(8)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB80, BIT(8));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_MON8 \n");
	}
	if (irq_sts & BIT(9)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB80, BIT(9));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_MON0 \n");
	}
	if (irq_sts & BIT(10)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB80, BIT(10));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_MON10 \n");
	}
	if (irq_sts & BIT(11)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB80, BIT(11));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_MON11 \n");
	}
	if (irq_sts & BIT(12)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB80, BIT(12));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_MON12 \n");
	}
	if (irq_sts & BIT(13)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB80, BIT(13));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_MON13 \n");
	}
	if (irq_sts & BIT(14)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB80, BIT(14));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_MON14 \n");
	}
	if (irq_sts & BIT(15)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB80, BIT(15));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_MON15 \n");
	}
}

void dprx_aux_access_irq_mon16_23(uint32_t irq_sts, struct dprx_ctrl *dprx)
{
	if (irq_sts & BIT(16)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB80, BIT(16));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_MON16 \n");
	}
	if (irq_sts & BIT(17)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB80, BIT(17));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_MON17 \n");
	}
	if (irq_sts & BIT(18)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB80, BIT(18));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_MON18 \n");
	}
	if (irq_sts & BIT(19)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB80, BIT(19));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_MON19 \n");
	}
	if (irq_sts & BIT(20)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB80, BIT(20));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_MON20 \n");
	}
	if (irq_sts & BIT(21)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB80, BIT(21));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_MON21 \n");
	}
	if (irq_sts & BIT(22)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB80, BIT(22));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_MON22 \n");
	}
	if (irq_sts & BIT(23)) {
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB80, BIT(23));
		dpu_pr_info("[DPRX] DPRX_IRQ_AUX_MON23 \n");
	}
}

void dprx_handle_aux_access_int_irq(struct dprx_ctrl *dprx)
{
	uint32_t irq_sts;
	irq_sts = dprx_readl(dprx, DPRX_SCTRL_OFFSET + 0x00B7C);
	dprx_aux_access_irq_mon0_15(irq_sts, dprx);
	dprx_aux_access_irq_mon16_23(irq_sts, dprx);
}

void dprx_handle_err_irq(struct dprx_ctrl *dprx)
{
	uint32_t irq_sts;
	irq_sts = dprx_readl(dprx, DPRX_SCTRL_OFFSET + 0x00B58);
	if (irq_sts & BIT(0))
		dpu_pr_info("[DPRX] DPRX_IRQ_DDC_TIMEOUT \n");

	if (irq_sts & BIT(1))
		dpu_pr_info("[DPRX] DPRX_IRQ_CHUNK_LEN_ERR \n");

	dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB5C, irq_sts);
}

irqreturn_t dprx_irq_handler(int irq, void *dev)
{
	struct dpu_dprx_device *dpu_dprx_dev = NULL;
	struct dprx_ctrl *dprx = NULL;
	uint32_t irq_status;

	dpu_check_and_return(!dev, IRQ_HANDLED, err, "[DPRX] dev is NULL\n");
	dpu_dprx_dev = (struct dpu_dprx_device *)dev;
	dpu_check_and_return((dpu_dprx_dev == NULL), IRQ_HANDLED, err, "[DPRX] dpu_dprx_dev is NULL!\n");

	dprx = &dpu_dprx_dev->dprx;

	irq_status = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_INTR_MASKED_STATUS);
	if (irq_status & TIMING_INTR_ENABLE) {
		dpu_pr_debug("[DPRX] TIMING_IRQ\n");
		dprx_handle_timing_irq(dprx);
	}

	if (irq_status & TRAINING_INTR_ENABLE) {
		dpu_pr_debug("[DPRX] TRAINING_IRQ\n");
		dprx_handle_training_irq(dprx);
	}

	if (irq_status & LINK_CFG_INTR_ENABLE) {
		dpu_pr_debug("[DPRX] LINK_CFG_IRQ\n");
		dprx_handle_linkcfg_irq(dprx);
		// todo
	}

	if (irq_status & GTC_INTR_ENABLE) {
		dpu_pr_debug("[DPRX] GTC_IRQ\n");
		dprx_handle_gtc_irg(dprx);
		// todo
	}

	if (irq_status & PSR_INTR_ENABLE) {
		dpu_pr_debug("[DPRX] PSR_IRQ\n");
		dprx_handle_psr_irq(dprx);
	}

	if (irq_status & SDP_VIDEO_FORMAT_INTR_ENABLE) {
		dpu_pr_debug("[DPRX] SDP_VIDEO_FORMAT_IRQ\n");
		dprx_handle_sdp_and_format_irq(dprx);
	}

	if (irq_status & AUDIO_INTR_ENABLE)
		dpu_pr_debug("[DPRX] AUDIO_IRQ\n");

	if (irq_status & ERR_INTR_ENABLE) {
		dpu_pr_debug("[DPRX] ERR_IRQ\n");
		dprx_handle_err_irq(dprx);
		// todo
	}

	if (irq_status & DEBUG_INTR_ENABLE) {
		dpu_pr_debug("[DPRX] DEBUG_IRQ\n");
		dprx_handle_debug_irq(dprx);
		// todo
	}

	if (irq_status & RSV_INTR_ENABLE) {
		dpu_pr_debug("[DPRX] RSV_IRQ\n");
		dprx_handle_rsv_irq(dprx);
		// todo
	}

	if (irq_status & AUX_RSV_INTR_ENABLE) {
		dpu_pr_debug("[DPRX] AUX_RSV_IRQ\n");
		dprx_handle_aux_access_int_irq(dprx);
		// todo
	}

	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_INTR_ORI_STATUS, irq_status);

	return IRQ_HANDLED;
}

uint32_t  dprx_get_color_space(struct dprx_ctrl *dprx)
{
	dpu_pr_info("ColorSpace: %s\n", enc_info_list[dprx->v_params.pix_enc].desc);
	return enc_info_list[dprx->v_params.pix_enc].pix_enc;
}

static struct dprx_to_layer_format g_dp2layer_fmt[] = {
	{RGB,      COLOR_DEPTH_6,  HISI_FB_PIXEL_FORMAT_RGBA_8888},
	{RGB,      COLOR_DEPTH_8,  HISI_FB_PIXEL_FORMAT_RGBA_8888},
	{RGB,      COLOR_DEPTH_10, HISI_FB_PIXEL_FORMAT_RGBA_1010102},
	{YCBCR420, COLOR_DEPTH_8,  HISI_FB_PIXEL_FORMAT_YUYV_420_PKG},
	{YCBCR420, COLOR_DEPTH_10, HISI_FB_PIXEL_FORMAT_YUYV_420_PKG_10BIT},
	{YCBCR422, COLOR_DEPTH_8,  HISI_FB_PIXEL_FORMAT_YUYV_422_PKG},
	{YCBCR422, COLOR_DEPTH_10, HISI_FB_PIXEL_FORMAT_YUV422_10BIT},
	{YCBCR444, COLOR_DEPTH_8,  HISI_FB_PIXEL_FORMAT_YUV444},
	{YCBCR444, COLOR_DEPTH_10, HISI_FB_PIXEL_FORMAT_YUVA444},
};

uint32_t dprx_ctrl_dp2layer_fmt(uint32_t color_space, uint32_t bit_width)
{
	int i;
	int size;
	struct dprx_to_layer_format *p_dp2layer_fmt;

	size = sizeof(g_dp2layer_fmt) / sizeof(g_dp2layer_fmt[0]);
	p_dp2layer_fmt = g_dp2layer_fmt;

	for (i = 0; i < size; i++) {
		if ((p_dp2layer_fmt[i].color_space == color_space) &&
			(p_dp2layer_fmt[i].bit_width == bit_width))
			return p_dp2layer_fmt[i].layer_format;
	}
	dpu_pr_info("no find fmt, return default rgba8888\n");
	return HISI_FB_PIXEL_FORMAT_RGBA_8888;
}
