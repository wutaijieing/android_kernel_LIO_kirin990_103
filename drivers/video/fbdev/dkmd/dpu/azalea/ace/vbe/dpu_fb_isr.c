/* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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
#include "dpu_fb_debug.h"
#include "dpu_fb.h"
#include "dpu_mipi_dsi.h"
#include "dpu_dpe_utils.h"
#include "dpu_fb_panel.h"
#include "overlay/dpu_overlay_utils_platform.h"
#include "overlay/dpu_overlay_utils.h"
#include "chrdev/dpu_chrdev.h"

/*******************************************************************************
 * handle isr
 */
static bool need_panel_mode_swtich(struct dpu_fb_data_type *dpufd, uint32_t isr_s2)
{
	if (dpufd->panel_mode_switch_isr_handler && dpufd->panel_info.panel_mode_swtich_support &&
		(dpufd->panel_info.mode_switch_to != dpufd->panel_info.current_mode)) {
		if (!(isr_s2 & BIT_LDI_UNFLOW))
			return true;
	}

	return false;
}

static bool need_dsi_bit_clk_upt(struct dpu_fb_data_type *dpufd)
{
	if (dpufd->mipi_dsi_bit_clk_upt_isr_handler && dpufd->panel_info.dsi_bit_clk_upt_support &&
		dpufd->panel_info.mipi.dsi_bit_clk_upt &&
		(dpufd->panel_info.mipi.dsi_bit_clk_upt != dpufd->panel_info.mipi.dsi_bit_clk))
		return true;

	return false;
}

static void dpufb_display_effect_flags_config(struct dpu_fb_data_type *dpufd)
{
	if (dpufd->color_temperature_flag > 0)
		dpufd->color_temperature_flag--;

	if (dpufd->display_effect_flag > 0)
		dpufd->display_effect_flag--;

	if (dpufd->panel_info.xcc_set_in_isr_support && dpufd->xcc_coef_set == 1) {
		dpe_set_xcc_csc_value(dpufd);
		dpufd->xcc_coef_set = 0;
	}
}

static bool need_fps_upt(struct dpu_fb_data_type *dpufd)
{
	if (dpufd->panel_info.fps_updt_support && dpufd->fps_upt_isr_handler &&
		(dpufd->panel_info.fps_updt != dpufd->panel_info.fps ||
		dpufd->panel_info.fps_updt_force_update))
		return true;

	return false;
}

static bool need_config_frm_rate_para(struct dpu_fb_data_type *dpufd)
{
	void_unused(dpufd);
	return false;
}

static void config_frm_rate_para(struct dpu_fb_data_type *dpufd)
{
	void_unused(dpufd);
}

static void dss_pdp_isr_vactive0_end_handle(struct dpu_fb_data_type *dpufd, uint32_t isr_s2)
{
	dpufd->vactive0_end_flag = 1;
	dpufb_display_effect_flags_config(dpufd);

	if (need_config_frm_rate_para(dpufd)) {
		config_frm_rate_para(dpufd);
	} else if (need_panel_mode_swtich(dpufd, isr_s2)) {
		if (((uint32_t)inp32(dpufd->dss_base + DSS_LDI0_OFFSET + LDI_CTRL) & 0x1) == 0)
			dpufd->panel_mode_switch_isr_handler(dpufd, dpufd->panel_info.mode_switch_to);
		dpufd->panel_info.mode_switch_state = PARA_UPDT_END;
	} else if (need_dsi_bit_clk_upt(dpufd)) {
		if (!(isr_s2 & BIT_VACTIVE0_START))
			dpufd->mipi_dsi_bit_clk_upt_isr_handler(dpufd);
	} else if (need_fps_upt(dpufd)) {
		dpufd->fps_upt_isr_handler(dpufd);
	}

	if ((g_debug_ldi_underflow == 0) && (g_err_status & DSS_PDP_LDI_UNDERFLOW))
		g_err_status &= ~DSS_PDP_LDI_UNDERFLOW;
}

static void dss_pdp_isr_vactive0_start_handle(struct dpu_fb_data_type *dpufd, uint32_t isr_s2)
{
	if (dpufd->ov_vactive0_start_isr_handler != NULL)
		dpufd->ov_vactive0_start_isr_handler(dpufd);

	if (need_panel_mode_swtich(dpufd, isr_s2)) {
		disable_ldi(dpufd);
		dpufd->panel_info.mode_switch_state = PARA_UPDT_DOING;
	}

	wake_up_interruptible_all(&(dpufd->vsync_ctrl.vactive_wait));
}

static void dss_debug_underflow_and_clear_base(struct dpu_fb_data_type *dpufd)
{
	g_err_status |= DSS_SDP_LDI_UNDERFLOW;

	if ((g_debug_ldi_underflow_clear != 0) && dpufd->ldi_underflow_wq != NULL)
		queue_work(dpufd->ldi_underflow_wq, &dpufd->ldi_underflow_work);

	if ((g_debug_ldi_underflow != 0) && (g_debug_ovl_online_composer != 0) && dpufd->dss_debug_wq)
		queue_work(dpufd->dss_debug_wq, &dpufd->dss_debug_work);
}

static void dss_te_vsync_isr_handle_base(struct dpu_fb_data_type *dpufd)
{
	if (dpufd->vsync_isr_handler)
		dpufd->vsync_isr_handler(dpufd);

	if (dpufd->buf_sync_signal)
		dpufd->buf_sync_signal(dpufd);
}

static void start_dss_clearing_work(struct dpu_fb_data_type *dpufd)
{
	if (g_debug_ldi_underflow_clear == 0)
		return;

	if (dpufd->ldi_underflow_wq)
		queue_work(dpufd->ldi_underflow_wq, &dpufd->ldi_underflow_work);
}

static void dss_isr_underflow_handle(struct dpu_fb_data_type *dpufd)
{
	start_dss_clearing_work(dpufd);

	if (g_debug_ldi_underflow && dpufd->dss_debug_wq)
		queue_work(dpufd->dss_debug_wq, &dpufd->dss_debug_work);

	g_err_status |= DSS_PDP_LDI_UNDERFLOW;
}

static void dss_dsi0_isr_for_te_vactive0_start(struct dpu_fb_data_type *dpufd, uint32_t isr_s2)
{
	uint32_t isr_te_vsync;

	if (is_mipi_cmd_panel(dpufd))
		isr_te_vsync = BIT_LCD_TE0_PIN;
	else
		isr_te_vsync = BIT_VSYNC;
	isr_te_vsync = BIT_VSYNC;

	if (dpu_is_async_play(dpufd)) {
		if (isr_s2 & isr_te_vsync) {
			dpufd->te_timestamp = ktime_get();
			dss_te_vsync_isr_handle_base(dpufd);
		}

		if (isr_s2 & BIT_VACTIVE0_START)
			dss_pdp_isr_vactive0_start_handle(dpufd, isr_s2);
	} else {
		if (isr_s2 & BIT_VACTIVE0_START)
			dss_pdp_isr_vactive0_start_handle(dpufd, isr_s2);

		if (isr_s2 & isr_te_vsync) {
			dpufd->te_timestamp = ktime_get();
			dss_te_vsync_isr_handle_base(dpufd);
		}
	}
}

irqreturn_t dss_dsi0_isr(int irq, void *ptr)
{
	struct dpu_fb_data_type *dpufd = NULL;
	uint32_t isr_s2;
	uint32_t mask;
	char __iomem *mipi_dsi_base = NULL;
	void_unused(irq);

	dpufd = (struct dpu_fb_data_type *)ptr;
	if (!dpufd) {
		DPU_FB_ISR_INFO("dpufd is NULL\n");
		return IRQ_NONE;
	}

	mipi_dsi_base = get_mipi_dsi_base(dpufd);
	dpu_check_and_return(!mipi_dsi_base, IRQ_NONE, ISR_INFO, "mipi_dsi_base is NULL\n");

	isr_s2 = inp32(mipi_dsi_base + MIPI_LDI_CPU_ITF_INTS);
	outp32(mipi_dsi_base + MIPI_LDI_CPU_ITF_INTS, isr_s2);

	isr_s2 &= ~((uint32_t)inp32(mipi_dsi_base + MIPI_LDI_CPU_ITF_INT_MSK));
	if (isr_s2 & BIT_VACTIVE0_END)
		dss_pdp_isr_vactive0_end_handle(dpufd, isr_s2);

	dss_dsi0_isr_for_te_vactive0_start(dpufd, isr_s2);

	if (isr_s2 & BIT_LDI_UNFLOW) {
		mask = inp32(mipi_dsi_base + MIPI_LDI_CPU_ITF_INT_MSK);
		mask |= BIT_LDI_UNFLOW;
		outp32(mipi_dsi_base + MIPI_LDI_CPU_ITF_INT_MSK, mask);
		if (is_dual_mipi_panel(dpufd))
			outp32(dpufd->mipi_dsi1_base + MIPI_LDI_CPU_ITF_INT_MSK, mask);

		DPU_FB_ISR_INFO("dsi0 underflow frame_no %d, isr 0x%x, dpp_dbg 0x%x, online_fill_level 0x%x\n",
			dpufd->ov_req.frame_no, isr_s2,
			inp32(dpufd->dss_base + DSS_DISP_CH0_OFFSET + DISP_CH_DBG_CNT),
			inp32(dpufd->dss_base + DSS_DBUF0_OFFSET + DBUF_ONLINE_FILL_LEVEL));
		DPU_FB_ISR_INFO("clk: edc[%llu] mmbuf[%llu]\n",
			(uint64_t)clk_get_rate(dpufd->dss_pri_clk), (uint64_t)clk_get_rate(dpufd->dss_mmbuf_clk));
		dss_isr_underflow_handle(dpufd);
	}

	return IRQ_HANDLED;
}

irqreturn_t dss_dsi1_isr(int irq, void *ptr)
{
	struct dpu_fb_data_type *dpufd = NULL;
	uint32_t isr_s2;
	uint32_t mask;
	uint32_t isr_te_vsync = BIT_VSYNC;
	void_unused(irq);

	dpufd = (struct dpu_fb_data_type *)ptr;
	dpu_check_and_return(!dpufd, IRQ_NONE, ISR_INFO, "dpufd is NULL!\n");

	isr_s2 = inp32(dpufd->mipi_dsi1_base + MIPI_LDI_CPU_ITF_INTS);

	outp32(dpufd->mipi_dsi1_base + MIPI_LDI_CPU_ITF_INTS, isr_s2);

	isr_s2 &= ~((uint32_t)inp32(dpufd->mipi_dsi1_base + MIPI_LDI_CPU_ITF_INT_MSK));

	if (is_mipi_cmd_panel(dpufd))
		isr_te_vsync = BIT_LCD_TE0_PIN;

	if (dpufd->panel_info.mipi.dsi_te_type == DSI1_TE1_TYPE)
		isr_te_vsync = BIT_LCD_TE1_PIN;

	if (isr_s2 & BIT_VACTIVE0_END)
		dss_pdp_isr_vactive0_end_handle(dpufd, isr_s2);

	if (isr_s2 & BIT_VACTIVE0_START)
		dss_pdp_isr_vactive0_start_handle(dpufd, isr_s2);

	isr_te_vsync = BIT_VSYNC;
	if (isr_s2 & isr_te_vsync)
		dss_te_vsync_isr_handle_base(dpufd);

	if (isr_s2 & BIT_LDI_UNFLOW) {
		mask = inp32(dpufd->mipi_dsi1_base + MIPI_LDI_CPU_ITF_INT_MSK);
		mask |= BIT_LDI_UNFLOW;
		outp32(dpufd->mipi_dsi1_base + MIPI_LDI_CPU_ITF_INT_MSK, mask);

		DPU_FB_ISR_INFO("dsi1 underflow frame_no %d, isr 0x%x, dpp_dbg 0x%x, online_fill_level 0x%x\n",
			dpufd->ov_req.frame_no, isr_s2,
			inp32(dpufd->dss_base + DSS_DISP_CH2_OFFSET + DISP_CH_DBG_CNT),
			inp32(dpufd->dss_base + DSS_DBUF1_OFFSET + DBUF_ONLINE_FILL_LEVEL));
		DPU_FB_ISR_INFO("clk: edc[%llu] mmbuf[%llu]\n",
			(uint64_t)clk_get_rate(dpufd->dss_pri_clk), (uint64_t)clk_get_rate(dpufd->dss_mmbuf_clk));

		dss_debug_underflow_and_clear_base(dpufd);
	}

	return IRQ_HANDLED;
}

irqreturn_t dss_sdp_isr_mipi_panel(int irq, void *ptr)
{
	struct dpu_fb_data_type *dpufd = NULL;
	uint32_t isr_s1;
	uint32_t isr_s2_smmu;
	void_unused(irq);

	dpufd = (struct dpu_fb_data_type *)ptr;
	if (!dpufd) {
		DPU_FB_ISR_INFO("dpufd is NULL\n");
		return IRQ_NONE;
	}

	isr_s1 = inp32(dpufd->dss_base + GLB_CPU_SDP_INTS);

	isr_s2_smmu = inp32(dpufd->dss_base + DSS_SMMU_OFFSET + SMMU_INTSTAT_NS);

	outp32(dpufd->dss_base + DSS_SMMU_OFFSET + SMMU_INTCLR_NS, isr_s2_smmu);
	outp32(dpufd->dss_base + GLB_CPU_SDP_INTS, isr_s1);

	isr_s1 &= ~((uint32_t)inp32(dpufd->dss_base + GLB_CPU_SDP_INT_MSK));

	return IRQ_HANDLED;
}

irqreturn_t dss_sdp_isr_dp(int irq, void *ptr)
{
	char __iomem *ldi_base = NULL;
	char __iomem *ldi_base1 = NULL;
	struct dpu_fb_data_type *dpufd = NULL;
	uint32_t isr_s1;
	uint32_t isr_s2;
	uint32_t isr_s3;
	uint32_t isr_s2_smmu;
	uint32_t mask;
	uint32_t temp;
	void_unused(irq);

	dpufd = (struct dpu_fb_data_type *)ptr;
	dpu_check_and_return(!dpufd, IRQ_NONE, ISR_INFO, "dpufd is NULL!\n");

	ldi_base = dpufd->dss_base + DSS_LDI_DP_OFFSET;
	ldi_base1 = dpufd->dss_base + DSS_LDI_DP1_OFFSET;

	isr_s1 = inp32(dpufd->dss_base + GLB_CPU_DP_INTS);
	isr_s2 = inp32(ldi_base + LDI_CPU_ITF_INTS);
	isr_s3 = inp32(ldi_base1 + LDI_CPU_ITF_INTS);
	isr_s2_smmu = inp32(dpufd->dss_base + DSS_SMMU_OFFSET + SMMU_INTSTAT_NS);

	outp32(dpufd->dss_base + DSS_SMMU_OFFSET + SMMU_INTCLR_NS, isr_s2_smmu);
	outp32(ldi_base + LDI_CPU_ITF_INTS, isr_s2);
	outp32(ldi_base1 + LDI_CPU_ITF_INTS, isr_s3);
	outp32(dpufd->dss_base + GLB_CPU_DP_INTS, isr_s1);

	isr_s1 &= ~((uint32_t)inp32(dpufd->dss_base + GLB_CPU_DP_INT_MSK));
	isr_s2 &= ~((uint32_t)inp32(ldi_base + LDI_CPU_ITF_INT_MSK));
	if (isr_s2 & BIT_VACTIVE0_END)
		dpufd->vactive0_end_flag = 1;

	if (isr_s2 & BIT_VACTIVE0_START) {
		if (dpufd->ov_vactive0_start_isr_handler)
			dpufd->ov_vactive0_start_isr_handler(dpufd);
	}

	if (isr_s2 & BIT_VSYNC)
		dss_te_vsync_isr_handle_base(dpufd);

	if (isr_s2 & BIT_LDI_UNFLOW) {
		mask = inp32(ldi_base + LDI_CPU_ITF_INT_MSK);
		mask |= BIT_LDI_UNFLOW;
		outp32(ldi_base + LDI_CPU_ITF_INT_MSK, mask);

		dss_debug_underflow_and_clear_base(dpufd);

		g_err_status |= DSS_SDP_LDI_UNDERFLOW;
		temp = inp32(dpufd->dss_base + DSS_DPP1_OFFSET + DPP_DBG_CNT);

		DPU_FB_ISR_INFO("ldi1 underflow! frame_no = %d, dpp_dbg = 0x%x\n", dpufd->ov_req.frame_no, temp);
		dpu_dump_current_info(dpufd);
	}

	return IRQ_HANDLED;
}

irqreturn_t dss_pdp_isr(int irq, void *ptr)
{
	struct dpu_fb_data_type *dpufd = NULL;
	uint32_t isr_s1;
	uint32_t isr_s2_dpp;
	uint32_t isr_s2_smmu;
	uint32_t isr_s2_wb;
	uint32_t mask;
	void_unused(irq);

	dpufd = (struct dpu_fb_data_type *)ptr;
	dpu_check_and_return(!dpufd, IRQ_NONE, ISR_INFO, "dpufd is NULL!\n");

	isr_s1 = inp32(dpufd->dss_base + GLB_CPU_PDP_INTS);
	isr_s2_dpp = inp32(dpufd->dss_base + DSS_DISP_CH0_OFFSET + DPP_INTS);

	isr_s2_smmu = inp32(dpufd->dss_base + DSS_SMMU_OFFSET + SMMU_INTSTAT_NS);

#if defined(DSS_WB_OFFSET)
	isr_s2_wb = inp32(dpufd->dss_base + DSS_WB_OFFSET + WB_ONLINE_ERR_INTS);
	outp32(dpufd->dss_base + DSS_WB_OFFSET + WB_ONLINE_ERR_INTS, isr_s2_wb);
#endif

	outp32(dpufd->dss_base + DSS_SMMU_OFFSET + SMMU_INTCLR_NS, isr_s2_smmu);
	outp32(dpufd->dss_base + DSS_DISP_CH0_OFFSET + DPP_INTS, isr_s2_dpp);

	outp32(dpufd->dss_base + GLB_CPU_PDP_INTS, isr_s1);

	isr_s1 &= ~((uint32_t)inp32(dpufd->dss_base + GLB_CPU_PDP_INT_MSK));
	isr_s2_dpp &= ~((uint32_t)inp32(dpufd->dss_base + DSS_DISP_CH0_OFFSET + DPP_INT_MSK));

	if (isr_s1 & BIT_DSS_WB_ERR_INTS) {
		mask = inp32(dpufd->dss_base + GLB_CPU_PDP_INT_MSK);
		mask |= BIT_DSS_WB_ERR_INTS;
		outp32(dpufd->dss_base + GLB_CPU_PDP_INT_MSK, mask);
		DPU_FB_ISR_INFO("isr_s1 = 0x%x, isr_s2_wb = 0x%x\n", isr_s1, isr_s2_wb);
	}

	return IRQ_HANDLED;
}

static void dss_sdp_handle_isr_s2(struct dpu_fb_data_type *dpufd, char __iomem *ldi_base, uint32_t isr_s2)
{
	uint32_t mask;

	if (isr_s2 & BIT_VACTIVE0_END)
		dpufd->vactive0_end_flag = 1;

	if (isr_s2 & BIT_VACTIVE0_START) {
		if (dpufd->ov_vactive0_start_isr_handler != NULL)
			dpufd->ov_vactive0_start_isr_handler(dpufd);
	}

	if (isr_s2 & BIT_VSYNC)
		dss_te_vsync_isr_handle_base(dpufd);

	if (isr_s2 & BIT_LDI_UNFLOW) {
		mask = inp32(ldi_base + LDI_CPU_ITF_INT_MSK);
		mask |= BIT_LDI_UNFLOW;
		outp32(ldi_base + LDI_CPU_ITF_INT_MSK, mask);

		dss_debug_underflow_and_clear_base(dpufd);

		g_err_status |= DSS_SDP_LDI_UNDERFLOW;

		DPU_FB_ISR_INFO("ldi1 underflow! frame_no = %d!\n", dpufd->ov_req.frame_no);
	}
}

irqreturn_t dss_sdp_isr(int irq, void *ptr)
{
	char __iomem *ldi_base = NULL;
	struct dpu_fb_data_type *dpufd = NULL;
	uint32_t isr_s1;
	uint32_t isr_s2;
	uint32_t isr_s2_smmu;
	void_unused(irq);

	dpufd = (struct dpu_fb_data_type *)ptr;

	dpu_check_and_return(!dpufd, IRQ_NONE, ISR_INFO, "dpufd is NULL\n");

	ldi_base = dpufd->dss_base + DSS_LDI1_OFFSET;

	isr_s1 = inp32(dpufd->dss_base + GLB_CPU_SDP_INTS);
	isr_s2 = inp32(ldi_base + LDI_CPU_ITF_INTS);
	isr_s2_smmu = inp32(dpufd->dss_base + DSS_SMMU_OFFSET + SMMU_INTSTAT_NS);

	outp32(dpufd->dss_base + DSS_SMMU_OFFSET + SMMU_INTCLR_NS, isr_s2_smmu);
	outp32(ldi_base + LDI_CPU_ITF_INTS, isr_s2);
	outp32(dpufd->dss_base + GLB_CPU_SDP_INTS, isr_s1);

	isr_s1 &= ~((uint32_t)inp32(dpufd->dss_base + GLB_CPU_SDP_INT_MSK));
	isr_s2 &= ~((uint32_t)inp32(ldi_base + LDI_CPU_ITF_INT_MSK));

	dss_sdp_handle_isr_s2(dpufd, ldi_base, isr_s2);

	return IRQ_HANDLED;
}

static void adp_isr_wch_ints(struct dpu_fb_data_type *dpufd, uint32_t isr_s1, uint32_t isr_s3_copybit)
{
	if (isr_s1 & BIT_OFF_WCH0_INTS) {
		if (dpufd->cmdlist_info->cmdlist_wb_flag[WB_TYPE_WCH0] == 1) {
			dpufd->cmdlist_info->cmdlist_wb_done[WB_TYPE_WCH0] = 1;
			wake_up_interruptible_all(&(dpufd->cmdlist_info->cmdlist_wb_wq[WB_TYPE_WCH0]));
		}
	}

	if (isr_s1 & BIT_OFF_WCH1_INTS) {
		if (dpufd->cmdlist_info->cmdlist_wb_flag[WB_TYPE_WCH1] == 1) {
			dpufd->cmdlist_info->cmdlist_wb_done[WB_TYPE_WCH1] = 1;
			wake_up_interruptible_all(&(dpufd->cmdlist_info->cmdlist_wb_wq[WB_TYPE_WCH1]));
		}
	}

	if (isr_s1 & BIT_OFF_WCH0_WCH1_FRM_END_INT) {
		if (dpufd->cmdlist_info->cmdlist_wb_flag[WB_TYPE_WCH0_WCH1] == 1) {
			dpufd->cmdlist_info->cmdlist_wb_done[WB_TYPE_WCH0_WCH1] = 1;
			wake_up_interruptible_all(&(dpufd->cmdlist_info->cmdlist_wb_wq[WB_TYPE_WCH0_WCH1]));
		}
	}

	if (isr_s3_copybit & BIT_OFF_CAM_WCH2_FRMEND_INTS) {
		if (dpufd->copybit_info->copybit_flag == 1) {
			dpufd->copybit_info->copybit_done = 1;
			wake_up_interruptible_all(&(dpufd->copybit_info->copybit_wq));
		}

		if (dpufd->cmdlist_info->cmdlist_wb_flag[WB_TYPE_WCH2] == 1) {
			dpufd->cmdlist_info->cmdlist_wb_done[WB_TYPE_WCH2] = 1;
			wake_up_interruptible_all(&(dpufd->cmdlist_info->cmdlist_wb_wq[WB_TYPE_WCH2]));
		}
	}
}

irqreturn_t dss_adp_isr(int irq, void *ptr)
{
	struct dpu_fb_data_type *dpufd = NULL;
	uint32_t isr_s1;
	uint32_t isr_s2_smmu;
	uint32_t isr_s3_copybit = 0;
	void_unused(irq);

	dpufd = (struct dpu_fb_data_type *)ptr;
	dpu_check_and_return(!dpufd, IRQ_NONE, ISR_INFO, "dpufd is NULL\n");

	isr_s1 = inp32(dpufd->dss_base + GLB_CPU_OFF_INTS);
	isr_s2_smmu = inp32(dpufd->dss_base + DSS_SMMU_OFFSET + SMMU_INTSTAT_NS);

	outp32(dpufd->dss_base + DSS_SMMU_OFFSET + SMMU_INTCLR_NS, isr_s2_smmu);
	outp32(dpufd->dss_base + GLB_CPU_OFF_INTS, isr_s1);

	isr_s1 &= ~((uint32_t)inp32(dpufd->dss_base + GLB_CPU_OFF_INT_MSK));

	isr_s3_copybit = inp32(dpufd->dss_base + GLB_CPU_OFF_CAM_INTS);
	outp32(dpufd->dss_base + GLB_CPU_OFF_CAM_INTS, isr_s3_copybit);
	isr_s3_copybit &= ~((uint32_t)inp32(dpufd->dss_base + GLB_CPU_OFF_CAM_INT_MSK));

	adp_isr_wch_ints(dpufd, isr_s1, isr_s3_copybit);

	return IRQ_HANDLED;
}

irqreturn_t dss_mdc_isr(int irq, void *ptr)
{
	struct dpu_fb_data_type *dpufd = NULL;
	uint32_t isr_s1;
	void_unused(irq);

	dpufd = (struct dpu_fb_data_type *)ptr;
	if (!dpufd) {
		DPU_FB_ISR_INFO("dpufd is NULL\n");
		return IRQ_NONE;
	}

	isr_s1 = inp32(dpufd->media_common_base + GLB_CPU_OFF_INTS);
	outp32(dpufd->media_common_base + GLB_CPU_OFF_INTS, isr_s1);

	isr_s1 &= ~((uint32_t)inp32(dpufd->media_common_base + GLB_CPU_OFF_INT_MSK));
	if (isr_s1 & (BIT_OFF_WCH1_INTS | BIT_OFF_WCH0_INTS)) {
		if (dpufd->media_common_info->mdc_flag == 1) {
			dpufd->media_common_info->mdc_done = 1;
			wake_up_interruptible_all(&(dpufd->media_common_info->mdc_wq));
		}
	}

	return IRQ_HANDLED;
}

static int vactive_timestamp_changed(struct dpufb_vsync *vsync_ctrl, ktime_t prev_timestamp)
{
	return !(prev_timestamp == vsync_ctrl->vactive_timestamp);
}

static ssize_t vactive_start_show_event(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct fb_info *fbi = NULL;
	struct dpu_fb_data_type *dpufd = NULL;
	int ret;
	ktime_t prev_timestamp;
	void_unused(attr);

	dpu_check_and_return((!dev), -1, ERR, "dev is NULL.\n");

	fbi = dev_get_drvdata(dev);
	dpu_check_and_return((!fbi), -1, ERR, "fbi is NULL.\n");

	dpufd = (struct dpu_fb_data_type *)fbi->par;
	dpu_check_and_return((!dpufd), -1, ERR, "dpufd is NULL.\n");
	dpu_check_and_return((!buf), -1, ERR, "buf is NULL.\n");

	prev_timestamp = dpufd->vsync_ctrl.vactive_timestamp;

	ret = wait_event_interruptible_timeout(dpufd->vsync_ctrl.vactive_wait,
		vactive_timestamp_changed(&dpufd->vsync_ctrl, prev_timestamp),
		msecs_to_jiffies(DSS_WAIT_ISR_TIMEOUT_THRESHOLD));
	if (ret < 0) {
		DPU_FB_INFO("fb%u interrrupt by a signal\n", dpufd->index);
		return -1;
	}

	ret = snprintf(buf, PAGE_SIZE, "VACTIVE_START=%llu\n", ktime_to_ns(dpufd->vsync_ctrl.vactive_timestamp));
	buf[strlen(buf) + 1] = '\0';
	return ret;
}

static DEVICE_ATTR(vactive_start_event, 0444, vactive_start_show_event, NULL);
void dpufb_vactive_start_register(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;

	if (!pdev) {
		DPU_FB_ERR("pdev is NULL\n");
		return;
	}
	dpufd = platform_get_drvdata(pdev);
	if (!dpufd) {
		dev_err(&pdev->dev, "dpufd is NULL\n");
		return;
	}

	dpufd->vsync_ctrl.vactive_timestamp = systime_get();
	DPU_FB_INFO("initial vactive timestamp=%llu\n", ktime_to_ns(dpufd->vsync_ctrl.vactive_timestamp));

	init_waitqueue_head(&(dpufd->vsync_ctrl.vactive_wait));

	if (dpufd->sysfs_attrs_append_fnc) {
		DPU_FB_DEBUG("vactive start register\n");
		dpufd->sysfs_attrs_append_fnc(dpufd, &dev_attr_vactive_start_event.attr);
	}
}
