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

#ifndef DISP_COMP_CONFIG_UTILS_H
#define DISP_COMP_CONFIG_UTILS_H

#include <linux/types.h>
#include <dpu/soc_dpu_define.h>

#define SOC_ACTRL_QIC_GLOBAL_CFG_ADDR(base) ((base) + (0x09a8))

struct swid_info {
	enum SCENE_ID scene_id;
	uint32_t swid_reg_offset_start;
	uint32_t swid_reg_offset_end;
};

extern struct swid_info g_sdma_swid_tlb_info[SDMA_SWID_NUM];
extern struct swid_info g_wch_swid_tlb_info[WCH_SWID_NUM];

struct swid_info *dpu_comp_get_sdma_swid_tlb_info(int *length);
struct swid_info *dpu_comp_get_wch_swid_tlb_info(int *length);
void dpu_comp_wch_axi_sel_set_reg(char __iomem *dbcu_base);
char __iomem *dpu_comp_get_tbu1_base(char __iomem *dpu_base);
void dpu_level1_clock_lp(uint32_t cmdlist_id);
void dpu_level2_clock_lp(uint32_t cmdlist_id);
void dpu_ch1_level2_clock_lp(uint32_t cmdlist_id);
void dpu_memory_lp(uint32_t cmdlist_id);
void dpu_memory_no_lp(uint32_t cmdlist_id);
void dpu_ch1_memory_no_lp(uint32_t cmdlist_id);
void dpu_lbuf_init(uint32_t cmdlist_id, uint32_t xres, uint32_t dsc_en);
void dpu_enable_m1_qic_intr(char __iomem *actrl_base);

static inline bool is_offline_scene(uint32_t scene_id)
{
	return (scene_id >= DPU_SCENE_OFFLINE_0 && scene_id <= DPU_SCENE_OFFLINE_2);
}

static inline bool is_online_scene(uint32_t scene_id)
{
	return (scene_id >= DPU_SCENE_ONLINE_0 && scene_id <= DPU_SCENE_ONLINE_3);
}

#endif /* DISP_COMP_CONFIG_UTILS_H */
