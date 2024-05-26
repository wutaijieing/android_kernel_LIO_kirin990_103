/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
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
#include <linux/debugfs.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/seq_file.h>

#include <asm/fixmap.h>
#include <asm/kasan.h>
#include <asm/memory.h>
#include <asm/pgtable.h>
#include <asm/pgtable-hwdef.h>
#include <asm/ptdump.h>
#include <asm-generic/memory_model.h>
#include <linux/page-flags.h>

#define TABLE_FLAG 3
#define TABLE 3
#define MASK 0xFFFFFFFFF000
#define MMU_REGS_SIZE 4
unsigned long long g_mmu_regs[MMU_REGS_SIZE];

void record_mmu_regs(void)
{
	asm (
	"	mrs	%0, sctlr_el1\n"
	"	mrs	%1, tcr_el1\n"
	"	mrs	%2, ttbr1_el1\n"
	"	mrs	%3, mair_el1\n"
	: "=r" (g_mmu_regs[0]), "=r" (g_mmu_regs[1]), "=r" (g_mmu_regs[2]), "=r" (g_mmu_regs[3])
	);

	pr_err("record mmu regs success!\n");
}

static void walk_pte(unsigned long long phys_addr)
{
	SetPageKernelPage(phys_to_page(phys_addr & MASK));
}

static void walk_pmd(unsigned long long phys_addr)
{
	unsigned int i;
	unsigned long long *ptr = NULL;

	if (PTRS_PER_PMD <= 1) {
		walk_pte(phys_addr);
		return;
	}

	ptr = phys_to_virt(phys_addr & MASK);
	SetPageKernelPage(phys_to_page(phys_addr & MASK));

	for (i = 0; i < PTRS_PER_PMD; i++) {
		if ((ptr[i] & TABLE_FLAG) == TABLE)
			walk_pte(ptr[i]);
	}
}

static void walk_pud(unsigned long long phys_addr)
{
	unsigned int i;
	unsigned long long *ptr = NULL;

	if (PTRS_PER_PUD <= 1) {
		walk_pmd(phys_addr);
		return;
	}

	ptr = phys_to_virt(phys_addr & MASK);
	SetPageKernelPage(phys_to_page(phys_addr & MASK));

	for (i = 0; i < PTRS_PER_PUD; i++) {
		if ((ptr[i] & TABLE_FLAG) == TABLE)
			walk_pmd(ptr[i]);
	}
}

static void walk_pgd(struct mm_struct *mm)
{
	unsigned int i;
	unsigned long long *ptr = (unsigned long long *)mm->pgd;
	unsigned long long phys_addr = (size_t)virt_to_phys(ptr);

	pr_err("start to register kernel page table!\n");
	SetPageKernelPage(phys_to_page(phys_addr));

	for (i = 0; i < PTRS_PER_PGD; i++) {
		if ((ptr[i] & TABLE_FLAG) == TABLE)
			walk_pud(ptr[i]);
	}
	pr_err("success registered kernel page table!\n");
}

static int register_in_kerneldump(struct mm_struct *mm)
{
	walk_pgd(mm);
	return 0;
}

int pagetable_dump_init(void)
{
	record_mmu_regs();
	return register_in_kerneldump(&init_mm);
}
late_initcall(pagetable_dump_init);