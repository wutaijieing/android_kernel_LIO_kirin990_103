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

#include "hisi_drv_dp.h"
#include <linux/time.h>

#include "hidptx_dp_avgen.h"
#include "hidptx_dp_core.h"
#include "hidptx_dp_irq.h"
#include "hidptx_reg.h"
#include "../dp_core_interface.h"
#include "link/dp_irq.h"
#include <linux/arm-smccc.h>
#include "hisi_disp_debug.h"

uint32_t g_vbp_cnt;
uint32_t g_vfp_cnt;
uint32_t g_vactive0_cnt;
uint32_t g_vactive1_cnt;
uint32_t g_stream0_cnt;
uint32_t last_cnt = 0;

/*lint -save -e* */
int handle_test_link_phy_pattern(struct dp_ctrl *dptx)
{
	int retval;
	uint8_t pattern;
	uint32_t phy_pattern;

	retval = 0;
	pattern = 0;
	phy_pattern = 0;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] NULL Pointer\n");

	retval = dptx_read_dpcd(dptx, DP_TEST_PHY_PATTERN, &pattern);
	if (retval)
		return retval;

	pattern &= DPTX_PHY_TEST_PATTERN_SEL_MASK;

	switch (pattern) {
	case DPTX_NO_TEST_PATTERN_SEL:
		disp_pr_info("[DP] DPTX_NO_TEST_PATTERN_SEL\n");
		phy_pattern = DPTX_PHYIF_CTRL_TPS_NONE;
		break;
	case DPTX_D102_WITHOUT_SCRAMBLING:
		disp_pr_info("[DP] DPTX_D102_WITHOUT_SCRAMBLING\n");
		phy_pattern = DPTX_PHYIF_CTRL_TPS_1;
		break;
	case DPTX_SYMBOL_ERROR_MEASUREMENT_COUNT:
		disp_pr_info("[DP] DPTX_SYMBOL_ERROR_MEASUREMENT_COUNT\n");
		phy_pattern = DPTX_PHYIF_CTRL_TPS_SYM_ERM;
		break;
	case DPTX_TEST_PATTERN_PRBS7:
		disp_pr_info("[DP] DPTX_TEST_PATTERN_PRBS7\n");
		phy_pattern = DPTX_PHYIF_CTRL_TPS_PRBS7;
		break;
	case DPTX_80BIT_CUSTOM_PATTERN_TRANS:
		disp_pr_info("[DP] DPTX_80BIT_CUSTOM_PATTERN_TRANS\n");
		dptx_writel(dptx, DPTX_PHYIF_CUSTOM80B_0, 0x3e0f83e0);
		dptx_writel(dptx, DPTX_PHYIF_CUSTOM80B_1, 0x3e0f83e0);
		dptx_writel(dptx, DPTX_PHYIF_CUSTOM80B_2, 0xf83e0);
		phy_pattern = DPTX_PHYIF_CTRL_TPS_CUSTOM80;
		break;
	case DPTX_CP2520_HBR2_COMPLIANCE_EYE_PATTERN:
		disp_pr_info("[DP] DPTX_CP2520_HBR2_COMPLIANCE_EYE_PATTERN\n");
		phy_pattern = DPTX_PHYIF_CTRL_TPS_CP2520_1;
		break;
	case DPTX_CP2520_HBR2_COMPLIANCE_PATTERN_2:
		disp_pr_info("[DP] DPTX_CP2520_HBR2_COMPLIANCE_EYE_PATTERN_2\n");
		phy_pattern = DPTX_PHYIF_CTRL_TPS_CP2520_2;
		break;
	case DPTX_CP2520_HBR2_COMPLIANCE_PATTERN_3:
		disp_pr_info("[DP] DPTX_CP2520_HBR2_COMPLIANCE_EYE_PATTERN_3\n");
		phy_pattern = DPTX_PHYIF_CTRL_TPS_4;
		break;
	default:
		disp_pr_info("[DP] Invalid TEST_PATTERN\n");
		return -EINVAL;
	}

	if (dptx->dptx_phy_set_pattern)
		dptx->dptx_phy_set_pattern(dptx, phy_pattern);

	return 0;
}

irqreturn_t dptx_threaded_irq(int irq, void *dev)
{
	struct dp_ctrl *dptx = NULL;

	disp_check_and_return(!dev, IRQ_HANDLED, err, "[DP] dev is NULL\n");

	return IRQ_HANDLED;
}

void set_dp_irq_cnt_clear(void)
{
	set_dptx_vsync_cnt(0);
	g_vbp_cnt = 0;
	g_vfp_cnt = 0;
	g_vactive0_cnt = 0;
	g_vactive1_cnt = 0;
	g_stream0_cnt = 0;
	last_cnt = 0;
}

void dptx_hpd_handler(struct dp_ctrl *dptx, bool plugin, uint8_t dp_lanes)
{
	int retval = 0;

	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");

	disp_pr_info("[DP] DP Plug Type: %d, Lanes: %d\n", plugin, dp_lanes);

	/* need to check dp lanes */
	dptx->max_lanes = dp_lanes;

	set_dptx_vsync_cnt(0);
	g_vbp_cnt = 0;
	g_vfp_cnt = 0;
	g_vactive0_cnt = 0;
	g_vactive1_cnt = 0;
	g_stream0_cnt = 0;
	last_cnt = 0;

	if (plugin && dptx->handle_hotplug) {
		retval = dptx->handle_hotplug(dptx);
		if (retval)
			disp_pr_err("[DP] hotplug error\n");
	}

	if (!plugin && dptx->handle_hotunplug) {
		retval = dptx->handle_hotunplug(dptx);
		if (retval)
			disp_pr_err("[DP] hotunplug error\n");
	}
}

void dptx_hpd_irq_handler(struct dp_ctrl *dptx)
{
	int retval = 0;

	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");

	disp_pr_info("[DP] DP Short Plug!\n");

	if (!dptx->video_transfer_enable) {
		disp_pr_err("[DP] DP never link train success, shot plug is useless!\n");
		return;
	}

	dptx_notify(dptx);
	retval = handle_sink_request(dptx);
	if (retval)
		disp_pr_err("[DP] Unable to handle sink request %d\n", retval);
}

uint32_t g_dptx_inte_print_level = DPTX_INT_MAX;
void dptx_up_inte_level(uint32_t print_level)
{
	if (print_level > DPTX_INT_MAX)
		g_dptx_inte_print_level = DPTX_INT_MAX;
	else
		g_dptx_inte_print_level = print_level;
}
EXPORT_SYMBOL_GPL(dptx_up_inte_level);

static void dptx_timegen_irq_handle(uint32_t timing_gen_irq_status, struct dp_ctrl *dptx)
{
	if (timing_gen_irq_status & DPTX_IRQ_ADAPTIVE_SYNC_SDP) {
		disp_pr_info("[DP-IRQ] DPTX_IRQ_ADAPTIVE_SYNC_SDP.\n");
		dptx_writel(dptx, DPTX_INTR_ACPU_TIMING_GEN_ORIGINAL_STATUS, DPTX_IRQ_ADAPTIVE_SYNC_SDP);
	}

	if (timing_gen_irq_status & DPTX_IRQ_ADAPTIVE_SYNC_SDP) {
		disp_pr_info("[DP-IRQ] DPTX_IRQ_ADAPTIVE_SYNC_SDP.\n");
		dptx_writel(dptx, DPTX_INTR_ACPU_TIMING_GEN_ORIGINAL_STATUS, DPTX_IRQ_ADAPTIVE_SYNC_SDP);
	}

	if (timing_gen_irq_status & DPTX_IRQ_VSYNC) {
		set_dptx_vsync_cnt(get_dptx_vsync_cnt() + 1);
		if (g_dptx_inte_print_level == VSYNC_INT)
			disp_pr_info("[DP-IRQ] DPTX_IRQ_VSYNC.\n");
		disp_pr_debug("[DP-IRQ] DPTX_IRQ_VSYNC.\n");
		dptx_writel(dptx, DPTX_INTR_ACPU_TIMING_GEN_ORIGINAL_STATUS, DPTX_IRQ_VSYNC);
#ifdef LOCAL_DIMMING_GOLDEN_TEST
		char __iomem *dpu_base_addr = hisi_config_get_ip_base(DISP_IP_BASE_DPU);
		disp_pr_info("ld_irq_status:0x%x", inp32(dpu_base_addr + DSS_GLB0_OFFSET + 0x4C0));
		disp_pr_debug("g_vsync_cnt:%d\n", get_dptx_vsync_cnt());
		uint32_t status = dptx_readl(dptx, DPTX_INTR_ACPU_TIMING_GEN_ENABLE);
		if (get_dptx_vsync_cnt() == get_ld_frm_cnt())
			dptx_writel(dptx, DPTX_INTR_ACPU_TIMING_GEN_ENABLE, (status & (~0x400)));
#endif
	}

	if (timing_gen_irq_status & DPTX_IRQ_VBP_END) {
		set_dptx_vsync_cnt(get_dptx_vsync_cnt() + 1);
		if (g_dptx_inte_print_level == VBP_INT)
			disp_pr_info("[DP-IRQ] DPTX_IRQ_VBP_END.\n");
		disp_pr_debug("[DP-IRQ] DPTX_IRQ_VBP_END.\n");
		dptx_writel(dptx, DPTX_INTR_ACPU_TIMING_GEN_ORIGINAL_STATUS, DPTX_IRQ_VBP_END);
	}

	if (timing_gen_irq_status & DPTX_IRQ_STREAM0_UNDERFLOW) {
		disp_pr_err("[DP-IRQ] DPTX_IRQ_STREAM0_UNDERFLOW.\n");
		dptx_writel(dptx, DPTX_INTR_ACPU_TIMING_GEN_ORIGINAL_STATUS, DPTX_IRQ_STREAM0_UNDERFLOW);
		dptx->local_notify_func(dptx, DISCONNECTED);
	}

	if (timing_gen_irq_status & DPTX_IRQ_STREAM1_UNDERFLOW) {
		disp_pr_err("[DP-IRQ] DPTX_IRQ_STREAM1_UNDERFLOW.\n");
		dptx_writel(dptx, DPTX_INTR_ACPU_TIMING_GEN_ORIGINAL_STATUS, DPTX_IRQ_STREAM1_UNDERFLOW);
	}

	if (timing_gen_irq_status & DPTX_IRQ_VFP_END) {
		g_vfp_cnt++;
		if (g_dptx_inte_print_level == VFP_INT)
			disp_pr_info("[DP-IRQ] DPTX_IRQ_VFP_END.\n");
		disp_pr_debug("[DP-IRQ] DPTX_IRQ_VFP_END.\n");
		dptx_writel(dptx, DPTX_INTR_ACPU_TIMING_GEN_ORIGINAL_STATUS, DPTX_IRQ_VFP_END);
	}

	if (timing_gen_irq_status & DPTX_IRQ_VACTIVE0) {
		g_vactive0_cnt++;
		hisi_vactive0_event_increase();
		if (g_dptx_inte_print_level == ACTIVE0_INT)
			disp_pr_info("[DP-IRQ] DPTX_IRQ_VACTIVE0.\n");
		disp_pr_debug("[DP-IRQ] DPTX_IRQ_VACTIVE0.\n");
		char __iomem *dpu_base_addr = hisi_config_get_ip_base(DISP_IP_BASE_DPU);
#ifdef LOCAL_DIMMING_GOLDEN_TEST
		if (get_dptx_vsync_cnt() <= get_ld_frm_cnt()) {
			set_ld_wch_last_vaddr(get_ld_wch_last_vaddr() + ALIGN_UP(2 * get_ld_led_numh(), 16) * get_ld_led_numv());
			disp_pr_debug("g_ld_wch_last_vaddr:0x%x\n", get_ld_wch_last_vaddr());
			dpu_set_reg(DPU_LD_DIM_ADDR2D_REG1_ADDR(dpu_base_addr + 0xCA000), get_ld_wch_last_vaddr(), 32, 0);
			outp32(dpu_base_addr + DSS_GLB0_OFFSET + 0x4C0, 0xF); // clear ld interrupt
		}
#endif
		dpu_set_reg(dpu_base_addr + 0x0D1000 + 0x5C, 1, 1, 0); // ITF_CH0 flush en
		dpu_set_reg(dpu_base_addr + 0xD9000 + 0x0254, 1, 1, 0); // DSC0 flush en
		dpu_set_reg(dpu_base_addr + 0xB4000 + 0x60, 1, 1, 0); // DPP0 flush en
		dptx_writel(dptx, DPTX_INTR_ACPU_TIMING_GEN_ORIGINAL_STATUS, DPTX_IRQ_VACTIVE0);

		if (g_vactive0_cnt == 1)
			dptx->local_notify_func(dptx, VIDEO_ACTIVATED);
	}

	if (timing_gen_irq_status & DPTX_IRQ_VACTIVE1) {
		g_vactive1_cnt++;
		if (g_dptx_inte_print_level == ACTIVE1_INT)
			disp_pr_info("[DP-IRQ] DPTX_IRQ_VACTIVE1.\n");
		disp_pr_debug("[DP-IRQ] DPTX_IRQ_VACTIVE1.\n");
		dptx_writel(dptx, DPTX_INTR_ACPU_TIMING_GEN_ORIGINAL_STATUS, DPTX_IRQ_VACTIVE1);
	}
}

static void dprx_stream_irq_handle(uint32_t intr_status, struct dp_ctrl *dptx)
{
	if (intr_status & DPTX_IRQ_STREAM0_FRAME) {
		g_stream0_cnt++;
		if (g_dptx_inte_print_level == STREAM0_FRAME_int)
			disp_pr_info("[DP-IRQ] DPTX_IRQ_STREAM0_FRAME.\n");
		disp_pr_debug("[DP-IRQ] DPTX_IRQ_STREAM0_FRAME.\n");
		dptx_writel(dptx, DPTX_INTR_ORIGINAL_STATUS, DPTX_IRQ_STREAM0_FRAME);
	}

	if (intr_status & DPTX_IRQ_HPD_PLUG) {
		disp_pr_info("[DP-IRQ] DPTX_IRQ_HPD_PLUG.\n");
		dptx_writel(dptx, DPTX_INTR_ORIGINAL_STATUS, DPTX_IRQ_HPD_PLUG);
	}

	if (intr_status & DPTX_IRQ_HPD_UNPLUG) {
		disp_pr_info("[DP-IRQ] DPTX_IRQ_HPD_UNPLUG.\n");
		dptx_writel(dptx, DPTX_INTR_ORIGINAL_STATUS, DPTX_IRQ_HPD_UNPLUG);
	}

	if (intr_status & DPTX_IRQ_HPD_SHORT) {
		disp_pr_info("[DP-IRQ] DPTX_IRQ_HPD_SHORT.\n");
		dptx_writel(dptx, DPTX_INTR_ORIGINAL_STATUS, DPTX_IRQ_HPD_SHORT);
		//dptx_hpd_irq_handler(dptx);
	}

	if (intr_status & DPTX_IRQ_AUX_REPLY) {
		if (g_dptx_inte_print_level == AUX_REPLY_INT)
			disp_pr_info("[DP-IRQ] DPTX_IRQ_AUX_REPLY.\n");
		disp_pr_debug("[DP-IRQ] DPTX_IRQ_AUX_REPLY.\n");
		dptx_writel(dptx, DPTX_INTR_ORIGINAL_STATUS, DPTX_IRQ_AUX_REPLY);
	}

	if (intr_status & DPTX_IRQ_STREAM0_SDP) {
		disp_pr_info("[DP-IRQ] DPTX_IRQ_STREAM0_SDP.\n");
		dptx_writel(dptx, DPTX_INTR_ORIGINAL_STATUS, DPTX_IRQ_STREAM0_SDP);
	}
}

static void dptx_sdp_irq_handle(uint32_t sdp_intr_status, struct dp_ctrl *dptx)
{
	if (sdp_intr_status & DPTX_IRQ_STRAME0_INFOFRAME_SDP) {
		disp_pr_info("[DP-IRQ] DPTX_IRQ_STRAME0_INFOFRAME_SDP, sdp_intr_status = 0x%08x\n", sdp_intr_status);
		dptx_writel(dptx, DPTX_SDP_INTR_ORIGINAL_STATUS, DPTX_IRQ_STRAME0_INFOFRAME_SDP);
	}

	if (sdp_intr_status & DPTX_IRQ_STRAME1_INFOFRAME_SDP) {
		disp_pr_info("[DP-IRQ] DPTX_IRQ_STRAME1_INFOFRAME_SDP, sdp_intr_status = 0x%08x\n", sdp_intr_status);
		dptx_writel(dptx, DPTX_SDP_INTR_ORIGINAL_STATUS, DPTX_IRQ_STRAME1_INFOFRAME_SDP);
	}

	if (sdp_intr_status & DPTX_IRQ_STREAM0_PPS_SDP) {
		if (g_dptx_inte_print_level == MANUAL_PPS_SDP_INT || g_dptx_inte_print_level == AUTO_PPS_SDP_INT)
			disp_pr_info("[DP-IRQ] DPTX_IRQ_STREAM0_PPS_SDP");
		disp_pr_debug("[DP-IRQ] DPTX_IRQ_STREAM0_PPS_SDP");
		dptx_writel(dptx, DPTX_SDP_INTR_ORIGINAL_STATUS, DPTX_IRQ_STREAM0_PPS_SDP);
	}
}

struct timespec64 start;
irqreturn_t dptx_irq(int irq, void *dev)
{
	irqreturn_t retval = IRQ_HANDLED;
	struct hisi_connector_device *connector_dev = NULL;
	struct hisi_dp_info *dp_info = NULL;
	struct dp_ctrl *dptx = NULL;
	uint32_t timing_gen_irq_status;
	uint32_t intr_status;
	uint32_t sdp_intr_status;
	uint32_t ien;
	struct timespec64 end;
	int interval;
	int cnt_int, cnt_frac;

	disp_check_and_return(!dev, IRQ_HANDLED, err, "[DP] dev is NULL\n");
	connector_dev = (struct hisi_connector_device *)dev;
	disp_check_and_return((connector_dev == NULL), IRQ_HANDLED, err, "[DP] connector_dev is NULL!\n");
	dp_info = (struct hisi_dp_info *)connector_dev->master.connector_info;
	disp_check_and_return((dp_info == NULL), IRQ_HANDLED, err, "[DP] dp_info is NULL!\n");

	dptx = &(dp_info->dp);

	timing_gen_irq_status = dptx_readl(dptx, DPTX_INTR_ACPU_TIMING_GEN_MASKED_STATUS);
	ien = dptx_readl(dptx, DPTX_INTR_ACPU_TIMING_GEN_ENABLE);
	disp_pr_debug("[DP] DPTX_ISTS=0x%08x, DPTX_IEN=0x%08x.\n",timing_gen_irq_status, ien);

	intr_status = dptx_readl(dptx, DPTX_INTR_MASKED_STATUS);
	sdp_intr_status = dptx_readl(dptx, DPTX_SDP_INTR_ORIGINAL_STATUS);

	if (g_vbp_cnt == 0)
		ktime_get_ts64(&start);

	if ((g_vbp_cnt % 60 == 0) && (g_vbp_cnt > last_cnt)) {
		ktime_get_ts64(&end);
		interval = 1000 * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000;
		cnt_int = g_vbp_cnt * 1000 / interval;
		cnt_frac = (g_vbp_cnt * 1000 % interval) * 1000 / interval;
		disp_pr_info("[DP] interval = %d s, sec_average_cnt = %d.%03d .\n", interval / 1000, cnt_int, cnt_frac);
		disp_pr_info("[DP] g_vsync_cnt = %u, g_vbp_cnt = %u, g_vfp_cnt = %u, g_vactive0_cnt = %u, g_vactive1_cnt = %u, g_stream0_cnt = %u.\n",
			get_dptx_vsync_cnt(), g_vbp_cnt, g_vfp_cnt, g_vactive0_cnt, g_vactive1_cnt, g_stream0_cnt);
		last_cnt = g_vbp_cnt;
	}

	if (!(timing_gen_irq_status & (DPTX_IRQ_ALL_TIMING_GEN_INTR | DPTX_IRQ_ADAPTIVE_SYNC_SDP)) \
		&& !(intr_status & DPTX_IRQ_ALL_INTR)) {
		disp_pr_info("[DP] IRQ_NONE, DPTX_ISTS=0x%08x.\n", timing_gen_irq_status);
		retval = IRQ_NONE;
		return retval;
	}

	dptx_timegen_irq_handle(timing_gen_irq_status, dptx);

	dprx_stream_irq_handle(intr_status, dptx);

	dptx_sdp_irq_handle(sdp_intr_status, dptx);

	return IRQ_HANDLED;
}

/*lint -restore*/