/*
 * npu_pm_config_nputop.c
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

#include "npu_common.h"
#include <linux/platform_drivers/npu_pm.h>

int npu_top_powerup(u32 work_mode, u32 subip_set, void **para)
{
	unused(para);
	unused(work_mode);
	unused(subip_set);
	return 0; /* npu_pm_power_on() call in npu_npusubsys_powerup */
}

int npu_top_powerdown(u32 work_mode, u32 subip_set, void *para)
{
	unused(para);
	unused(work_mode);
	unused(subip_set);
	return npu_pm_power_off();
}
