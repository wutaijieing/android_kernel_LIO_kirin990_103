/*
 * smmu.h
 *
 * This is for smmu description.
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

#ifndef __VCODEC_VENC_SMMU_H__
#define __VCODEC_VENC_SMMU_H__

#include "vcodec_type.h"
#include "drv_venc_ioctl.h"

#define SMMU_V3

#define SMMU_OK 0
#define SMMU_ERR (-1)

#define SECURE_ON    1
#define SECURE_OFF   0
#define SMMU_ON      1
#define SMMU_OFF     0

int32_t venc_smmu_init(bool is_protected, int32_t core_id);
int32_t venc_smmu_cfg(struct encode_info *channelcfg, uint32_t *reg_base);
int32_t venc_smmu_tbu_init(int32_t core_id);
void venc_smmu_tbu_deinit(void);
void venc_smmu_debug(const uint32_t *reg_base, bool first_cfg_flag);
int32_t venc_smmu_get_tcu_regulator_info(void);

#endif
