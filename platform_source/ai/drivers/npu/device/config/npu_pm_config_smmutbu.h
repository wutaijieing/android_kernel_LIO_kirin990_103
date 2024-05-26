/*
 * npu_pm_config_smmutbu.h
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef __NPU_PM_CONFIG_SMMUTBU_H
#define __NPU_PM_CONFIG_SMMUTBU_H

int npu_smmu_tbu_powerup(u32 work_mode, u32 subip_set, void **para);
int npu_smmu_tbu_powerdown(u32 work_mode, u32 subip_set, void *para);

#endif
