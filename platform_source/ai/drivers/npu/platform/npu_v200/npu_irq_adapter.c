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
#include "npu_platform.h"
#include "npu_platform_register.h"
#include "npu_hw_exp_irq.h"
#include "npu_log.h"

#define NPU_IRQ_GIC_MAP_COLUMN       2
u32 g_npu_irq_gic_map[][NPU_IRQ_GIC_MAP_COLUMN] = {
	// irq_type,                 gic_num
	{NPU_IRQ_CALC_CQ_UPDATE0, NPU_NPU2ACPU_HW_EXP_IRQ_NS_2},
	{NPU_IRQ_DFX_CQ_UPDATE,   NPU_NPU2ACPU_HW_EXP_IRQ_NS_1},
	{NPU_IRQ_MAILBOX_ACK,     NPU_NPU2ACPU_HW_EXP_IRQ_NS_0}
};

int npu_plat_handle_irq_tophalf(u32 irq_index)
{
	u32 i;
	u32 gic_irq = 0;
	u32 map_len = sizeof(g_npu_irq_gic_map) /
		(NPU_IRQ_GIC_MAP_COLUMN * sizeof(u32));
	struct npu_platform_info *plat_info = npu_plat_get_info();
	cond_return_error(plat_info == NULL, -1, "get platform info is NULL\n");

	for (i = 0; i < map_len; i++) {
		if (g_npu_irq_gic_map[i][0] == irq_index) {
			gic_irq = g_npu_irq_gic_map[i][1];
			break;
		}
	}
	cond_return_error(gic_irq == 0, -1, "invalide irq_index:%d\n", irq_index);

	npu_clr_hw_exp_irq_int(
		(u64)(uintptr_t)plat_info->dts_info.reg_vaddr[NPU_REG_HW_EXP_IRQ_NS_BASE], gic_irq);
	return 0;
}
