/*
 * smmu.h
 *
 * This is for vdec smmu.
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

#define SMMU_OK  0
#define SMMU_ERR (-1)

#define SECURE_ON  1
#define SECURE_OFF 0
#define SMMU_ON    1
#define SMMU_OFF   0

int32_t smmu_init(void);
void smmu_deinit(void);
void set_tbu_reg(int32_t addr, uint32_t val, uint32_t bw, uint32_t bs);

#endif
