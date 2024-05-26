/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: krp - Kernel region protect.
 * Create: 2019-04-09
 */

#define pr_fmt(fmt)    "kernel-harden: " fmt

#include <asm/sections.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/printk.h>
#include <linux/arm-smccc.h>
#include "platform_include/see/access_register_smc_id.h"

/*
 * traps to bl31 domain,
 * to set relative reggisters to finish kernel code and ro data protection
 */
void configure_kernel_krp(u64 addr, u64 usize)
{
	struct arm_smccc_res res = {0};

	arm_smccc_smc((u64)ACCESS_REGISTER_FN_MAIN_ID, (u64)addr, (u64)usize,
		(u64)ACCESS_REGISTER_FN_SUB_ID_DDR_KERNEL_CODE_PROTECT, 0, 0, 0, 0, &res);
	if (res.a0 != 0)
		pr_info("configure krp failed\n");
}

static s32 __init krp_set_init(void)
{
	u64 protect_start = virt_to_phys(_protect_start);
	u64 protect_end = virt_to_phys(_protect_end);
	u64 space_size = 0;

	if (protect_end > protect_start)
		space_size = protect_end - protect_start;

	pr_info("krp: enable\n");
	configure_kernel_krp(protect_start, space_size);
	return 0;
}

static void __exit krp_set_exit(void)
{
	pr_info("krp: disable\n");
}

module_init(krp_set_init);
module_exit(krp_set_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hisilicon Security AP Team");
