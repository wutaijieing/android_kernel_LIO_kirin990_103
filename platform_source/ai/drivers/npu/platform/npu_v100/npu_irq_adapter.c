/*
 * npu_irq_adapter.c
 *
 * about npu irq adapter
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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
#include "npu_irq_adapter.h"
#include "npu_common.h"

int npu_plat_handle_irq_tophalf(u32 irq_index)
{
	unused(irq_index);
	return 0;
}