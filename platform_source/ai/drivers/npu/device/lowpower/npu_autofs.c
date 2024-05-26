/*
 * npu_autofs.c
 *
 * about npu autofs
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

#include <asm/io.h>

#include "npu_platform_register.h"
#include "npu_log.h"
#include "npu_common.h"
#include "npu_platform.h"
#include "npu_pm.h"
#include "npu_autofs.h"
#include "npu_autofs_plat.h"

static void __iomem *g_npu_autofs_crg_vaddr;

void npu_autofs_enable(void)
{
	g_npu_autofs_crg_vaddr = (g_npu_autofs_crg_vaddr == NULL) ?
		ioremap(SOC_ACPU_npu_crg_BASE_ADDR, SOC_ACPU_CRG_NPU_SIZE) :
		g_npu_autofs_crg_vaddr;
	if (g_npu_autofs_crg_vaddr == NULL) {
		npu_drv_err("ioremap npu pcr base addr fail\n");
		return;
	}
	npu_autofs_npubus_on(g_npu_autofs_crg_vaddr);
	npu_autofs_npubuscfg_on(g_npu_autofs_crg_vaddr);
	npu_autofs_npubus_aicore_on(g_npu_autofs_crg_vaddr);
	npu_drv_info("npu autofs enable\n");
}

void npu_autofs_disable(void)
{
	if (g_npu_autofs_crg_vaddr == NULL) {
		npu_drv_info("npu autofs crg vaddr is NULL\n");
		return;
	}
	npu_autofs_npubus_aicore_off(g_npu_autofs_crg_vaddr);
	npu_autofs_npubuscfg_off(g_npu_autofs_crg_vaddr);
	npu_autofs_npubus_off(g_npu_autofs_crg_vaddr);

	iounmap(g_npu_autofs_crg_vaddr);
	g_npu_autofs_crg_vaddr = NULL;
	npu_drv_info("npu autofs disable\n");
}
