/*
 * npu_pm_config_aicore.c
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

#include "npu_adapter.h"
#include "npu_platform_pm.h"
#include "npu_atf_subsys.h"
#include "npu_platform.h"

int npu_aicore_powerup(u32 work_mode, u32 subip_set, void **para)
{
	int ret;
	int aicore_flag;

	unused(para);

	if (subip_set == NPU_AICORE0)
		aicore_flag = NPU_FLAGS_PWONOFF_AIC0;
	else if (subip_set == NPU_AICORE1)
		aicore_flag = NPU_FLAGS_PWONOFF_AIC1;
	else
		return 0;

	ret = npu_powerup_aicore(work_mode, aicore_flag);
	if (ret != 0)
		return ret;

	if (work_mode != NPU_SEC)
		npu_plat_aicore_pmu_enable(subip_set);

	return 0;
}

int npu_aicore_powerdown(u32 work_mode, u32 subip_set, void *para)
{
	int ret;
	int aicore_flag;

	unused(para);

	if (subip_set == NPU_AICORE0)
		aicore_flag = NPU_FLAGS_PWONOFF_AIC0;
	else if (subip_set == NPU_AICORE1)
		aicore_flag = NPU_FLAGS_PWONOFF_AIC1;
	else
		return 0;

	ret = npu_plat_powerdown_tbu(aicore_flag);
	if (ret)
		return ret;

	ret = npu_powerdown_aicore(work_mode, aicore_flag);
	return ret;
}
