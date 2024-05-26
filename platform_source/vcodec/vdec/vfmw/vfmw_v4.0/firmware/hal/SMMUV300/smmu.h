/*
 * smmu.h
 *
 * This is for smmu driver.
 *
 * Copyright (c) 2017-2020 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __VCODEC_VDEC_SMMU_H__
#define __VCODEC_VDEC_SMMU_H__

#include "sysconfig.h"
#include "vfmw.h"

#define SMMU_OK     0
#define SMMU_ERR   -1

#define SECURE_ON   1
#define SECURE_OFF  0
#define SMMU_ON     1
#define SMMU_OFF    0

// SMMU common and Master(MFDE/SCD/BPD) virtual base address
struct smmu_reg_info {
	uint32_t *smmu_mfde_reg_vir;
	uint32_t *smmu_scd_reg_vir;
	uint32_t *smmu_tbu_reg_vir;
	uint32_t *smmu_sid_reg_vir;
};

enum smmu_master_type {
	MFDE = 0,
	SCD,
};

void smmu_set_master_reg(
	enum smmu_master_type master_type, uint8_t secure_en, uint8_t mmu_en);
void smmu_init_global_reg(void);
void smmu_int_serv_proc(void);
int32_t smmu_init(void);
int32_t smmu_v3_init(void);
void smmu_deinit(void);

#endif
