/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#include "dkmd_log.h"
#include "dkmd_object.h"
#include "dpu_opr_config.h"
#include "dpu_config_utils.h"

static uint32_t s_scf_lut_addr_tlb[] = {
	DPU_WCH1_OFFSET,  /* only wch1 support scf */
	DPU_VSCF0_OFFSET,
};

uint32_t *dpu_config_get_scf_lut_addr_tlb(uint32_t *length)
{
	*length = ARRAY_SIZE(s_scf_lut_addr_tlb);
	return s_scf_lut_addr_tlb;
}
EXPORT_SYMBOL(dpu_config_get_scf_lut_addr_tlb);

uint32_t *dpu_config_get_arsr_lut_addr_tlb(uint32_t *length)
{
	void_unused(length);
	/* do not support arsr, so return null */
	return NULL;
}
EXPORT_SYMBOL(dpu_config_get_arsr_lut_addr_tlb);

uint32_t dpu_config_get_version(void)
{
	return DPU_ACCEL_DPUV160;
}

MODULE_LICENSE("GPL");
