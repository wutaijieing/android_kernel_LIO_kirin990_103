/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * npu autofs plat details
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <asm/io.h>

#include "npu_common.h"
#include "npu_platform_register.h"
#include "npu_autofs_plat.h"

void npu_autofs_npubus_on(void *autofs_crg_vaddr)
{
	writel(0x0, (SOC_NPUCRG_PERI_AUTODIV0_ADDR(autofs_crg_vaddr)));
	writel(0x07E0283F, (SOC_NPUCRG_PERI_AUTODIV2_ADDR(autofs_crg_vaddr)));
	npu_reg_update((SOC_NPUCRG_PERI_AUTODIV2_ADDR(autofs_crg_vaddr)), 0x0, 0x1);
	npu_reg_update((SOC_NPUCRG_PEREN1_ADDR(autofs_crg_vaddr)), 0x100, 0x100);
}

void npu_autofs_npubus_off(void *autofs_crg_vaddr)
{
	npu_reg_update((SOC_NPUCRG_PERDIS1_ADDR(autofs_crg_vaddr)), 0x100, 0x100);
	npu_reg_update((SOC_NPUCRG_PERI_AUTODIV2_ADDR(autofs_crg_vaddr)), 0x1, 0x1);
}

void npu_autofs_npubuscfg_on(void *autofs_crg_vaddr)
{
	writel(0x00001800, (SOC_NPUCRG_PERI_AUTODIV3_ADDR(autofs_crg_vaddr)));
	writel(0x0060283F, (SOC_NPUCRG_PERI_AUTODIV1_ADDR(autofs_crg_vaddr)));
	npu_reg_update((SOC_NPUCRG_PERI_AUTODIV1_ADDR(autofs_crg_vaddr)), 0x0, 0x1);
	npu_reg_update((SOC_NPUCRG_PEREN1_ADDR(autofs_crg_vaddr)), 0x200, 0x200);
}

void npu_autofs_npubuscfg_off(void *autofs_crg_vaddr)
{
	npu_reg_update((SOC_NPUCRG_PERDIS1_ADDR(autofs_crg_vaddr)), 0x200, 0x200);
	npu_reg_update((SOC_NPUCRG_PERI_AUTODIV1_ADDR(autofs_crg_vaddr)), 0x1, 0x1);
}

void npu_autofs_npubus_aicore_on(void *autofs_crg_vaddr)
{
	writel(0x00000001, (SOC_NPUCRG_PERI_AUTODIV4_ADDR(autofs_crg_vaddr)));
	writel(0x00602FFF, (SOC_NPUCRG_PERI_AUTODIV5_ADDR(autofs_crg_vaddr)));
	npu_reg_update((SOC_NPUCRG_PERI_AUTODIV5_ADDR(autofs_crg_vaddr)), 0x0, 0x1);
	npu_reg_update((SOC_NPUCRG_PEREN1_ADDR(autofs_crg_vaddr)), 0x800, 0x800);
}

void npu_autofs_npubus_aicore_off(void *autofs_crg_vaddr)
{
	npu_reg_update((SOC_NPUCRG_PERDIS1_ADDR(autofs_crg_vaddr)), 0x800, 0x800);
	npu_reg_update((SOC_NPUCRG_PERI_AUTODIV5_ADDR(autofs_crg_vaddr)), 0x1, 0x1);
}

