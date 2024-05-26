/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_GENERIC_PGALLOC_H
#define __ASM_GENERIC_PGALLOC_H

#ifdef CONFIG_MMU

#ifdef CONFIG_HKIP_PRMEM
#include <platform_include/see/prmem.h>
#endif

#define GFP_PGTABLE_KERNEL	(GFP_KERNEL | __GFP_ZERO)
#define GFP_PGTABLE_USER	(GFP_PGTABLE_KERNEL | __GFP_ACCOUNT)

/**
 * __pte_alloc_one_kernel - allocate a page for PTE-level kernel page table
 * @mm: the mm_struct of the current context
 *
 * This function is intended for architectures that need
 * anything beyond simple page allocation.
 *
 * Return: pointer to the allocated memory or %NULL on error
 */
static inline pte_t *__pte_alloc_one_kernel(struct mm_struct *mm)
{
	return (pte_t *)__get_free_page(GFP_PGTABLE_KERNEL);
}

#ifndef __HAVE_ARCH_PTE_ALLOC_ONE_KERNEL
/**
 * pte_alloc_one_kernel - allocate a page for PTE-level kernel page table
 * @mm: the mm_struct of the current context
 *
 * Return: pointer to the allocated memory or %NULL on error
 */
static inline pte_t *pte_alloc_one_kernel(struct mm_struct *mm, unsigned long addr)
{
#ifdef CONFIG_HKIP_PRMEM
	if (unlikely(is_pmalloc((void *)addr, 1)))
		return (pte_t *)pmalloc_get_page_table_page();
#endif
	return __pte_alloc_one_kernel(mm);
}
#endif

/**
 * pte_free_kernel - free PTE-level kernel page table page
 * @mm: the mm_struct of the current context
 * @pte: pointer to the memory containing the page table
 */
static inline void pte_free_kernel(struct mm_struct *mm, pte_t *pte)
{
#ifdef CONFIG_HKIP_PRMEM
	if (unlikely(pte &&
		     is_pmalloc_page_table_page((unsigned long)pte))) {
		pmalloc_put_page_table_page((unsigned long)pte);
		return;
	}
#endif
	free_page((unsigned long)pte);
}

#ifdef CONFIG_HKIP_PRMEM
static inline pgtable_t
pmalloc_pte_alloc_one(struct mm_struct *mm, unsigned long addr)
{
	struct page *pte = NULL;
	void *p = NULL;

	p = (void *)pmalloc_get_page_table_page();
	if (unlikely(p == NULL))
		return NULL;
	pte = virt_to_page(p);
	if (!pgtable_pte_page_ctor(pte)) {
		pmalloc_put_page_table_page((unsigned long)(uintptr_t)p);
		return NULL;
	}
	return pte;
}
#endif

/**
 * __pte_alloc_one - allocate a page for PTE-level user page table
 * @mm: the mm_struct of the current context
 * @gfp: GFP flags to use for the allocation
 *
 * Allocates a page and runs the pgtable_pte_page_ctor().
 *
 * This function is intended for architectures that need
 * anything beyond simple page allocation or must have custom GFP flags.
 *
 * Return: `struct page` initialized as page table or %NULL on error
 */
static inline pgtable_t __pte_alloc_one(struct mm_struct *mm, gfp_t gfp)
{
	struct page *pte;

	pte = alloc_page(gfp);
	if (!pte)
		return NULL;
	if (!pgtable_pte_page_ctor(pte)) {
		__free_page(pte);
		return NULL;
	}

	return pte;
}

#ifndef __HAVE_ARCH_PTE_ALLOC_ONE
/**
 * pte_alloc_one - allocate a page for PTE-level user page table
 * @mm: the mm_struct of the current context
 *
 * Allocates a page and runs the pgtable_pte_page_ctor().
 *
 * Return: `struct page` initialized as page table or %NULL on error
 */
static inline pgtable_t pte_alloc_one(struct mm_struct *mm, unsigned long addr)
{
#ifdef CONFIG_HKIP_PRMEM
	if (unlikely(is_pmalloc((void *)addr, 1)))
		return pmalloc_pte_alloc_one(mm, addr);
#endif
	return __pte_alloc_one(mm, GFP_PGTABLE_USER);
}
#endif

/*
 * Should really implement gc for free page table pages. This could be
 * done with a reference count in struct page.
 */

/**
 * pte_free - free PTE-level user page table page
 * @mm: the mm_struct of the current context
 * @pte_page: the `struct page` representing the page table
 */
static inline void pte_free(struct mm_struct *mm, struct page *pte_page)
{
#ifdef CONFIG_HKIP_PRMEM
	unsigned long addr;

	addr = (unsigned long)page_to_virt(pte_page);
	if (unlikely(is_pmalloc_page_table_page(addr))) {
		pmalloc_put_page_table_page(addr);
		return;
	}
#endif
	pgtable_pte_page_dtor(pte_page);
	__free_page(pte_page);
}


#if CONFIG_PGTABLE_LEVELS > 2

#ifndef __HAVE_ARCH_PMD_ALLOC_ONE
/**
 * pmd_alloc_one - allocate a page for PMD-level page table
 * @mm: the mm_struct of the current context
 *
 * Allocates a page and runs the pgtable_pmd_page_ctor().
 * Allocations use %GFP_PGTABLE_USER in user context and
 * %GFP_PGTABLE_KERNEL in kernel context.
 *
 * Return: pointer to the allocated memory or %NULL on error
 */
static inline pmd_t *pmd_alloc_one(struct mm_struct *mm, unsigned long addr)
{
	struct page *page;
	gfp_t gfp = GFP_PGTABLE_USER;

#ifdef CONFIG_HKIP_PRMEM
	if (unlikely(is_pmalloc((void *)addr, 1)))
		return (pmd_t *)pmalloc_get_page_table_page();
#endif

	if (mm == &init_mm)
		gfp = GFP_PGTABLE_KERNEL;
	page = alloc_pages(gfp, 0);
	if (!page)
		return NULL;
	if (!pgtable_pmd_page_ctor(page)) {
		__free_pages(page, 0);
		return NULL;
	}
	return (pmd_t *)page_address(page);
}
#endif

#ifndef __HAVE_ARCH_PMD_FREE
static inline void pmd_free(struct mm_struct *mm, pmd_t *pmd)
{
	BUG_ON((unsigned long)pmd & (PAGE_SIZE-1));
#ifdef CONFIG_HKIP_PRMEM
	if (unlikely(is_pmalloc_page_table_page((unsigned long)pmd))) {
		pmalloc_put_page_table_page((unsigned long)pmd);
		return;
	}
#endif
	pgtable_pmd_page_dtor(virt_to_page(pmd));
	free_page((unsigned long)pmd);
}
#endif

#endif /* CONFIG_PGTABLE_LEVELS > 2 */

#if CONFIG_PGTABLE_LEVELS > 3

#ifndef __HAVE_ARCH_PUD_ALLOC_ONE
/**
 * pud_alloc_one - allocate a page for PUD-level page table
 * @mm: the mm_struct of the current context
 *
 * Allocates a page using %GFP_PGTABLE_USER for user context and
 * %GFP_PGTABLE_KERNEL for kernel context.
 *
 * Return: pointer to the allocated memory or %NULL on error
 */
static inline pud_t *pud_alloc_one(struct mm_struct *mm, unsigned long addr)
{
	gfp_t gfp = GFP_PGTABLE_USER;

#ifdef CONFIG_HKIP_PRMEM
	if (unlikely(is_pmalloc((void *)addr, 1)))
		return (pud_t *)pmalloc_get_page_table_page();
#endif

	if (mm == &init_mm)
		gfp = GFP_PGTABLE_KERNEL;
	return (pud_t *)get_zeroed_page(gfp);
}
#endif

static inline void pud_free(struct mm_struct *mm, pud_t *pud)
{
	BUG_ON((unsigned long)pud & (PAGE_SIZE-1));
#ifdef CONFIG_HKIP_PRMEM
	if (unlikely(is_pmalloc_page_table_page((unsigned long)pud))) {
		pmalloc_put_page_table_page((unsigned long)pud);
		return;
	}
#endif
	free_page((unsigned long)pud);
}

#endif /* CONFIG_PGTABLE_LEVELS > 3 */

#ifndef __HAVE_ARCH_PGD_FREE
static inline void pgd_free(struct mm_struct *mm, pgd_t *pgd)
{
	free_page((unsigned long)pgd);
}
#endif

#endif /* CONFIG_MMU */

#endif /* __ASM_GENERIC_PGALLOC_H */
