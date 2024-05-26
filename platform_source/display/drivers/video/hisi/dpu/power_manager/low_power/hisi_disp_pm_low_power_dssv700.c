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
#include <soc_dte_define.h>

#include "hisi_disp_pm.h"

static void lp_first_level_clk_gate_ctrl(char __iomem *dpu_base)
{
	disp_pr_info("+++");

	outp32(DPU_GLB_MODULE_CLK_SEL_ADDR(dpu_base), 0x00000000); // core clk, pclk
}

static void lp_second_level_clk_gate_ctrl(char __iomem *dpu_base)
{
	disp_pr_info("+++");

	outp32(DPU_CMD_CLK_SEL_ADDR(dpu_base + DSS_CMDLIST_OFFSET), 0x00000000); // CMD
#ifndef CONFIG_DKMD_DPU_V720
	outp32(DPU_WCH_CH_CLK_SEL_ADDR(dpu_base + DSS_WCH0_OFFSET), 0x00000000); // WCH0
#endif
	outp32(DPU_WCH_CH_CLK_SEL_ADDR(dpu_base + DSS_WCH1_OFFSET), 0x00000000); // WCH1
	outp32(DPU_WCH_CH_CLK_SEL_ADDR(dpu_base + DSS_WCH2_OFFSET), 0x00000000); // WCH2

	outp32(DPU_DBCU_CLK_SEL_0_ADDR(dpu_base + DSS_DBCU_OFFSET), 0x00000000); // DBCU
	outp32(DPU_DBCU_CLK_SEL_1_ADDR(dpu_base + DSS_DBCU_OFFSET), 0x00000000); // DBCU

	outp32(DPU_LBUF_CTL_CLK_SEL_ADDR(dpu_base + DSS_LBUF_OFFSET), 0x00000000); // LBUF
	outp32(DPU_LBUF_MEM_DSLP_CTRL_ADDR(dpu_base + DSS_LBUF_OFFSET), 0x00000000); // LBUF

	outp32(DPU_ARSR_TOP_CLK_SEL_ADDR(dpu_base + DSS_ARSR0_OFFSET), 0x00000000); // ARSR0
	outp32(DPU_ARSR_TOP_CLK_SEL_ADDR(dpu_base + DSS_ARSR1_OFFSET), 0x00000000); // ARSR1

#ifndef CONFIG_DKMD_DPU_V720
	outp32(DPU_VSCF_TOP_CLK_SEL_ADDR(dpu_base + DSS_VSCF0_OFFSET), 0x00000000); // VSCF0
	outp32(DPU_VSCF_TOP_CLK_SEL_ADDR(dpu_base + DSS_VSCF1_OFFSET), 0x00000000); // VSCF1
#endif

	outp32(DPU_HDR_TOP_CLK_SEL_ADDR(dpu_base + DSS_HDR_OFFSET), 0x00000000); // HDR

	outp32(DPU_CLD_CLK_SEL_ADDR(dpu_base + DSS_CLD0_OFFSET), 0x00000000); // CLD0
#ifndef CONFIG_DKMD_DPU_V720
	outp32(DPU_CLD_CLK_SEL_ADDR(dpu_base + DSS_CLD1_OFFSET), 0x00000000); // CLD1

	outp32(DPU_HEMCD_CLK_SEL_ADDR(dpu_base + DSS_HEMCD0_OFFSET), 0x00000000); // HEMCD0
	outp32(DPU_HEMCD_CLK_SEL_ADDR(dpu_base + DSS_HEMCD1_OFFSET), 0x00000000); // HEMCD1
#endif

	outp32(DPU_RCH_OV_RCH_OV0_TOP_CLK_SEL_ADDR(dpu_base + DSS_RCH_OV_OFFSET), 0x00000000); // RCH_OV0
	outp32(DPU_RCH_OV_RCH_OV1_TOP_CLK_SEL_ADDR(dpu_base + DSS_RCH_OV_OFFSET), 0x00000000); // RCH_OV1
	outp32(DPU_RCH_OV_RCH_OV2_TOP_CLK_SEL_ADDR(dpu_base + DSS_RCH_OV_OFFSET), 0x00000000); // RCH_OV2
#ifndef CONFIG_DKMD_DPU_V720
	outp32(DPU_RCH_OV_RCH_OV3_TOP_CLK_SEL_ADDR(dpu_base + DSS_RCH_OV_OFFSET), 0x00000000); // RCH_OV3
#endif
	outp32(DPU_RCH_OV_WRAP_CLK_SEL_ADDR(dpu_base + DSS_RCH_OV_OFFSET), 0x00000000); // RCH_OV_WRAP

	outp32(DPU_DPP_CH_CLK_SEL_ADDR(dpu_base + DSS_DPP0_OFFSET), 0x00000000); // DPP0
#ifndef CONFIG_DKMD_DPU_V720
	outp32(DPU_DPP_CH_CLK_SEL_ADDR(dpu_base + DSS_DPP1_OFFSET), 0x00000000); // DPP1
#endif

	outp32(DPU_DDIC_TOP_CLK_SEL_ADDR(dpu_base + DSS_DDIC0_OFFSET), 0x00000000); // DDIC0
#ifndef CONFIG_DKMD_DPU_V720
	outp32(DPU_DDIC_TOP_CLK_SEL_ADDR(dpu_base + DSS_DDIC1_OFFSET), 0x00000000); // DDIC1
#endif

	outp32(DPU_DSC_CLK_SEL_ADDR(dpu_base + DSS_DSC_OFFSET), 0x00000000); // DSC
	outp32(DPU_DSC_TOP_CLK_SEL_ADDR(dpu_base + DSS_DSC_OFFSET), 0x00000000); // DSC_TOP
#ifndef CONFIG_DKMD_DPU_V720
	outp32(DPU_DSC_CLK_SEL_ADDR(dpu_base + DSS_DSC1_OFFSET), 0x00000000); // DSC1
	outp32(DPU_DSC_TOP_CLK_SEL_ADDR(dpu_base + DSS_DSC1_OFFSET), 0x00000000); // DSC1_TOP
#endif

	outp32(DPU_GLB_DISP_CH0_MODULE_CLK_SEL_ADDR(dpu_base), 0x00000000); // GLB
	outp32(DPU_GLB_DISP_CH0_MODULE_CLK_EN_ADDR(dpu_base), 0x00000000); // GLB

#ifndef CONFIG_DKMD_DPU_V720
	outp32(DPU_GLB_DISP_CH1_MODULE_CLK_SEL_ADDR(dpu_base), 0x00000000); // GLB
	outp32(DPU_GLB_DISP_CH1_MODULE_CLK_EN_ADDR(dpu_base), 0x00000000); // GLB
#endif
}

static void hisi_disp_pm_platform_set_lp_clk(struct hisi_disp_pm_lowpower *lp_mng)
{
	disp_pr_info("+++");
	lp_first_level_clk_gate_ctrl(lp_mng->base_addr);
	lp_second_level_clk_gate_ctrl(lp_mng->base_addr);
}

static void hisi_disp_pm_platform_set_lp_memory(struct hisi_disp_pm_lowpower *lp_mng)
{
}

void hisi_disp_pm_registe_platform_lp_cb(struct hisi_disp_pm_lowpower *lp_mng)
{
	lp_mng->set_lp_clk = hisi_disp_pm_platform_set_lp_clk;
	lp_mng->set_lp_memory = hisi_disp_pm_platform_set_lp_memory; // memory default is low power mode
}

