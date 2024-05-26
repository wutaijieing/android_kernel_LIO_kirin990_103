/*
 * npu_powercapping.c
 *
 * about npu powercapping
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

#include <linux/errno.h>

#include "npu_log.h"
#include "npu_common.h"
#include "npu_atf_subsys.h"
#include "npu_powercapping.h"

void npu_powercapping_enable(void)
{
	(void)npuatf_lowpower_ops(NPU_LOWPOWER_PCR, NPU_LOWPOWER_FLAG_ENABLE);
}

void npu_powercapping_disable(void)
{
	(void)npuatf_lowpower_ops(NPU_LOWPOWER_PCR, NPU_LOWPOWER_FLAG_DISABLE);
}
