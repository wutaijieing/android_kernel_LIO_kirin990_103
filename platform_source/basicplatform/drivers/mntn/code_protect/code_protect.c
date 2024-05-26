/*
 * code_protect.c
 *
 * for kernel code protection within ddrc mechanism
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <asm/sections.h>
#include <linux/arm-smccc.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <platform_include/basicplatform/linux/pr_log.h>
#include <linux/types.h>
#include "platform_include/see/access_register_smc_id.h"

#define PR_LOG_TAG CODE_PROTECT_TAG

/*
 * Description: access bl31 domain, to set relative reggisters to finish kernel code and ro data protection
 * Input:       phy addr and size to protect.
 * Output:      NA
 * Return:      NA
 */
static void configure_kernel_code_protect(unsigned long addr, unsigned long usize)
{
	struct arm_smccc_res res = {0};

	arm_smccc_smc((u64)ACCESS_REGISTER_FN_MAIN_ID, (u64)addr, (u64)usize,
		(u64)ACCESS_REGISTER_FN_SUB_ID_DDR_KERNEL_CODE_PROTECT,
		0, 0, 0, 0, &res);
}

/*
 * Description:    init function ,to start setting kernel code and ro data protection.
 * Input:          NA
 * Output:         NA
 * Return:         OK:success
 */
static s32 __init code_protect_set_init(void)
{
	unsigned long addr_start = virt_to_phys(_stext);
	unsigned long addr_end = virt_to_phys(_etext);
	unsigned long space_size = 0;

	if (addr_end > addr_start)
		space_size = addr_end - addr_start;
	configure_kernel_code_protect(addr_start, space_size);
	return 0;
}

static void __exit code_protect_set_exit(void)
{
}

module_init(code_protect_set_init);
module_exit(code_protect_set_exit);
