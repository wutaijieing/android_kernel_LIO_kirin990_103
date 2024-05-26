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

#ifndef VCODEC_VDEC_SMMU_H
#define VCODEC_VDEC_SMMU_H

#define SMMU_OK     0
#define SMMU_ERR   (-1)

struct smmu_reg_info {
	uint8_t  *smmu_tbu_reg_vir;
	uint8_t  *smmu_sid_reg_vir;
};

struct smmu_tbu_info {
	uint32_t mmu_tbu_num;
	uint32_t mmu_tbu_offset;
	uint32_t mmu_sid_offset;
};

struct smmu_entry {
	struct smmu_reg_info reg_info;
	uint8_t smmu_init;
	struct smmu_tbu_info tbu_info;
};

int32_t smmu_map_reg(void);
void smmu_unmap_reg(void);

int32_t smmu_init(void);
void smmu_deinit(void);

#endif

