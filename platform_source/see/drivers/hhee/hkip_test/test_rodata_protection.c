/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2021. All rights reserved.
 * Description: hkip early ro data protect test
 * Create: 2017/10/19
 */

#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <platform_include/see/hkip.h>
#include <asm/kernel-pgtable.h>
#include <asm/sections.h>
#include <asm/sizes.h>
#include <asm/tlb.h>
#include <asm/mmu_context.h>

#define INIT_VALUE 0xAAAA
#define NEW_VALUE 0x5555

static const u32 victim = INIT_VALUE;
static const u32 control = INIT_VALUE;
static volatile void *reference;
static bool vict;
static bool ctrl;

static bool inline alter_const(const u32 *target)
{
	reference = (void *)target;
	barrier();
	*(u32 *)reference = NEW_VALUE;
	barrier();
	return (*(u32 *)reference == NEW_VALUE);
}

static int early_rodata_debugfs(void)
{
	struct dentry *dir;

	dir = debugfs_create_dir("early_rodata", NULL);
	if (!dir) {
		pr_err("Failed to create debugfs dir\n");
		return -1;
	}
	debugfs_create_bool("control_test_passed", 0444, dir, &ctrl);
	debugfs_create_bool("victim_test_passed", 0444, dir, &vict);
	return 0;
}
module_init(early_rodata_debugfs);

void test_early_rodata_protection(void)
{
	unsigned long start = (unsigned long)__start_rodata;
	unsigned long end = (unsigned long)__rodata_protect_end;
	unsigned long section_size = (unsigned long)end - (unsigned long)start;

	ctrl = alter_const(&control);
	hkip_register_ro((void *)start, ALIGN(section_size, PAGE_SIZE));
	vict = !alter_const(&victim);
	create_mapping_late(__pa_symbol(__start_rodata), start,
			    section_size, PAGE_KERNEL_RO);
}
EXPORT_SYMBOL(test_early_rodata_protection);

