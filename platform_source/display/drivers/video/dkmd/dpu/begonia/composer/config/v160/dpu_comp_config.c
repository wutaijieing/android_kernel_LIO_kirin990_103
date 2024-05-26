/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#include "dkmd_log.h"
#include "dkmd_object.h"
#include "cmdlist_interface.h"
#include "dpu_comp_config_utils.h"

struct swid_info g_sdma_swid_tlb_info[SDMA_SWID_NUM] = {
	{DPU_SCENE_ONLINE_0,   0,  19},
	{DPU_SCENE_OFFLINE_0,  20,  39},
};

struct swid_info g_wch_swid_tlb_info[WCH_SWID_NUM] = {
	{DPU_SCENE_ONLINE_0,  58,  58},
	{DPU_SCENE_OFFLINE_0, 58,  58},
};

void dpu_comp_wch_axi_sel_set_reg(char __iomem *dbcu_base)
{
	if (dbcu_base == NULL) {
		dpu_pr_err("dbcu_base is nullptr!\n");
		return;
	}

	/* wch select tbu0 config stream bypass */
	set_reg(DPU_DBCU_WCH_1_AXI_SEL_ADDR(dbcu_base), 0, 1, 0);
}

char __iomem *dpu_comp_get_tbu1_base(char __iomem *dpu_base)
{
	return NULL;
}

void dpu_level1_clock_lp(uint32_t cmdlist_id)
{
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x00013100, 0x00000000); // DPU_GLB_MODULE_CLK_SEL_ADDR
}

void dpu_level2_clock_lp(uint32_t cmdlist_id)
{
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x00012740, 0x00000000); // DPU_CMD_CLK_SEL_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x0006D0E4, 0x00000000); // WCH1 DPU_WCH_CH_CLK_SEL_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x00019788, 0x00000000); // DBCU DPU_DBCU_CLK_SEL_0_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x0001978C, 0x00000000); // DBCU DPU_DBCU_CLK_SEL_1_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x0001B000, 0x00000000); // LBUF DPU_LBUF_CTL_CLK_SEL_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x0001B008, 0x00000000); // LBUF DPU_LBUF_PART_CLK_SEL_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x0007F008, 0x00000002); // VSCF0 DPU_VSCF_TOP_CLK_SEL_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x0008D008, 0x00000000); // CLD0 DPU_CLD_CLK_SEL_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x0001C000, 0x00000000); // RCH_OV0 DPU_RCH_OV_RCH_OV0_TOP_CLK_SEL_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x0001C008, 0x00000000); // RCH_OV1 DPU_RCH_OV_RCH_OV1_TOP_CLK_SEL_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x0001C020, 0x00000000); // RCH_OV DPU_RCH_OV_WRAP_CLK_SEL_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x000B4008, 0x00000000); // DPP0 DPU_DPP_CH_CLK_SEL_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x00013124, 0x00000000); // GLB DPU_GLB_DISP_CH0_MODULE_CLK_SEL_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x0001312C, 0x00000000); // GLB DPU_GLB_DISP_CH1_MODULE_CLK_SEL_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x00013134, 0x00000000); // GLB DPU_GLB_DISP_AVHR_MODULE_CLK_SEL_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x0001313C, 0x00000000); // GLB DPU_GLB_DISP_MODULE_CLK_SEL_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x00040400, 0x00000000); // DACC_CFG SOC_DACC_CLK_SEL_REG_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x00040800, 0x00000000); // DACC_CFG SOC_DACC_CLK_SEL_REG_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x00040C00, 0x00000000); // DACC_CFG SOC_DACC_CLK_SEL_REG_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x00041000, 0x00000000); // DACC_CFG SOC_DACC_CLK_SEL_REG_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x00041400, 0x00000000); // DACC_CFG SOC_DACC_CLK_SEL_REG_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x00041800, 0x00000000); // DACC_CFG SOC_DACC_CLK_SEL_REG_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x00041C00, 0x00000000); // DACC_CFG SOC_DACC_CLK_SEL_REG_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x000CC070, 0x00000001); // ITF_CH0 DPU_ITF_CH_ITFCH_CLK_CTL_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x000CC4F4, 0x00000000); // PIPE_SW DPU_PIPE_SW_CLK_SEL_ADDR

	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x000136A0, 0x00000000); // GLB DPU_GLB_REG_DVFS_CTRL_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x00013710, 0x00000000); // GLB DPU_GLB_DBG_CTRL1_ADDR
}

void dpu_ch1_level2_clock_lp(uint32_t cmdlist_id)
{
	void_unused(cmdlist_id);
}

void dpu_memory_lp(uint32_t cmdlist_id)
{
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x0001B100, 0x00000001); // LBUF DPU_LBUF_MEM_DSLP_CTRL_ADDR
}

void dpu_memory_no_lp(uint32_t cmdlist_id)
{
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x0001273C, 0x00000008); // CMD DPU_CMD_MEM_CTRL_ADDR

	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x0006D03C, 0x88888888); // WCH1 DPU_WCH_DMA_CMP_MEM_CTRL0_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x0006D040, 0x88888888); // WCH1 DPU_WCH_DMA_CMP_MEM_CTRL1_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x0006D218, 0x00000088); // WCH1 DPU_WCH_SCF_COEF_MEM_CTRL_ADDR

	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x0001B010, 0x00000000); // LBUF DPU_LBUF_MEM_CTRL_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x0007F290, 0x00000008); // VSCF0 DPU_VSCF_SCF_LB_MEM_CTRL_ADDR

	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x0001C060, 0x00000008); // RCH_OV0 DPU_RCH_OV_OV_MEM_CTRL_0_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x0001C064, 0x00000008); // RCH_OV1 DPU_RCH_OV_OV_MEM_CTRL_1_ADDR

	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x000B4704, 0x00000008); // DPP0 DPU_DPP_DEGAMA_MEM_CTRL_ADDR
	dkmd_set_reg(DPU_SCENE_INITAIL, cmdlist_id, 0x000B58A4, 0x00000008); // DPP0 DPU_DPP_SPR_GAMA_MEM_CTRL_ADDR
}

void dpu_ch1_memory_no_lp(uint32_t cmdlist_id)
{
	void_unused(cmdlist_id);
}

void dpu_lbuf_init(uint32_t cmdlist_id, uint32_t xres, uint32_t dsc_en)
{
	void_unused(cmdlist_id);
	void_unused(xres);
	void_unused(dsc_en);
}

void dpu_enable_m1_qic_intr(char __iomem *actrl_base)
{
	void_unused(actrl_base);
}
