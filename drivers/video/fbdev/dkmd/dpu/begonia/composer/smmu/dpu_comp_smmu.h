/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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

#ifndef COMP_SMMU_H
#define COMP_SMMU_H

#include "dpu_comp_mgr.h"

#define SMMU_TIMEOUT 1000   /* us */
#define DPU_TBU0_DTI_NUMS 0x17
#define DPU_TBU1_DTI_NUMS 0x0F

#define DPU_SSID_MAX_NUM 3

typedef union {
	uint32_t value;
	struct {
		uint32_t ch_sid          : 8;  /* bit[7:0]  */
		uint32_t reserved_0      : 3;  /* bit[10:8] */
		uint32_t ch_ssid         : 8;  /* bit[18:11]  */
		uint32_t ch_ssidv        : 1;  /* bit[19] */
		uint32_t ch_sc_hint      : 4;  /* bit[23:20] */
		uint32_t ch_gid          : 4;  /* bit[27:24] */
		uint32_t ch_gid_en       : 1;  /* bit[28] */
		uint32_t ch_ptl_as_ful   : 1;  /* bit[29] */
		uint32_t reserved_2      : 2;  /* bit[31:30] */
	} reg;
} soc_dss_mmu_id_attr;

enum smmu_event {
	TBU_DISCONNECT = 0x0,
	TBU_CONNECT = 0x1,
};

static inline uint32_t dpu_comp_get_ssid(uint32_t frame_no)
{
	return frame_no % DPU_SSID_MAX_NUM;
}

void dpu_comp_smmu_ch_set_reg(uint32_t reg_cmdlist_id, uint32_t scene_id, uint32_t frame_index);
void dpu_comp_smmu_offline_tlb_flush(uint32_t scene_id, uint32_t block_num);
void dpu_comp_smmu_tlb_flush(uint32_t scene_id, uint32_t frame_index);

void dpu_comp_smmuv3_on(struct composer_manager *comp_mgr, struct dpu_composer *dpu_comp);
void dpu_comp_smmuv3_off(struct composer_manager *comp_mgr, struct dpu_composer *dpu_comp);

void dpu_comp_smmuv3_recovery_on(struct composer_manager *comp_mgr, struct dpu_composer *dpu_comp);
void dpu_comp_smmuv3_reset_off(struct composer_manager *comp_mgr, struct dpu_composer *dpu_comp);

#endif
