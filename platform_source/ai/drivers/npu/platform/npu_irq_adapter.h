/*
 * npu_irq_adapter.h
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
#ifndef __NPU_IRQ_ADAPTER_H__
#define __NPU_IRQ_ADAPTER_H__

#include <linux/types.h>

int npu_plat_handle_irq_tophalf(u32 irq_index);

#endif
