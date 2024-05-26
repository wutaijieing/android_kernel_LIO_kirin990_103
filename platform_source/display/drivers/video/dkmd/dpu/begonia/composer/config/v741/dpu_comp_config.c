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
	{DPU_SCENE_ONLINE_1,  20,  39},
};

struct swid_info g_wch_swid_tlb_info[WCH_SWID_NUM] = {
};

void dpu_comp_wch_axi_sel_set_reg(char __iomem *dbcu_base)
{
	void_unused(dbcu_base);
}

char __iomem *dpu_comp_get_tbu1_base(char __iomem *dpu_base)
{
	if (dpu_base == NULL) {
		dpu_pr_err("dpu_base is nullptr!\n");
		return NULL;
	}

	return dpu_base + DPU_SMMU1_OFFSET;
}

void dpu_level1_clock_lp(uint32_t cmdlist_id)
{
	void_unused(cmdlist_id);
}

void dpu_level2_clock_lp(uint32_t cmdlist_id)
{
	void_unused(cmdlist_id);
}

void dpu_ch1_level2_clock_lp(uint32_t cmdlist_id)
{
	void_unused(cmdlist_id);
}

void dpu_memory_lp(uint32_t cmdlist_id)
{
	void_unused(cmdlist_id);
}

void dpu_memory_no_lp(uint32_t cmdlist_id)
{
	void_unused(cmdlist_id);
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
	if (actrl_base == NULL) {
		dpu_pr_err("actrl_base is nullptr!\n");
	}

	set_reg(SOC_ACTRL_QIC_GLOBAL_CFG_ADDR(actrl_base), 3, 2, 6);
}