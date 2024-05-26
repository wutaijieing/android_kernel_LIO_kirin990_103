/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2021. All rights reserved.
 * Description: ioctl source file for kirin platform
 * Create: 2017/8/31
 */

#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <asm/page.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/io.h>
#include <asm/io.h>
#include <linux/module.h>
#include <linux/lsm_hooks.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
#include <asm/tlb.h>
#else
#include <asm-generic/tlb.h>
#endif
#include "hkip.h"

#define PMALLOC_NAME_LEN_MAX  64
#define PMALLOC_BUFF_SIZE_MAX 0x1000000 /* 16MB */
#define PMALLOC_BUFF_CNT_MAX  0x1000000

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#ifndef VA_START
#define VA_START    (UL(0xffffffffffffffff) - \
	(UL(1) << VA_BITS) + 1)
#endif
#endif

static struct mm_struct *_get_mm(unsigned long addr)
{
	if (addr >= VA_START)
		return &init_mm;
	else
		return current->mm;
}

static int _get_pgtable_desc(unsigned long addr, pgd_t **pgdpp, pud_t **pudpp,
			     pmd_t **pmdpp, pte_t **ptepp)
{
	pte_t pte;

	*pgdpp = NULL;
	*pudpp = NULL;
	*pmdpp = NULL;
	*ptepp = NULL;

	if (addr >= VA_START)
		*pgdpp = pgd_offset_k(addr);
	else
		*pgdpp = pgd_offset(current->mm, addr);

#define DBGPT(a, b, c) printk(#a "[0x%016lx]: 0x%016llx\n", (uintptr_t)(b), (c))

	DBG("Find page table descriptors for VA:0x%016lx\n", addr);
	if (pgd_none(**pgdpp))
		goto not_found;

	DBGPT(PGD, *pgdpp, pgd_val(**pgdpp));
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	*pudpp = pud_offset((p4d_t *)*pgdpp, addr);
#else
	*pudpp = pud_offset(*pgdpp, addr);
#endif
	if (pud_none(**pudpp))
		goto not_found;

	DBGPT(PUD, *pudpp, pud_val(**pudpp));
	if (pud_sect(**pudpp))
		goto found_sect;

	*pmdpp = pmd_offset(*pudpp, addr);
	if (pmd_none(**pmdpp))
		goto not_found;

	DBGPT(PMD, *pmdpp, pmd_val(**pmdpp));
	if (pmd_sect(**pmdpp))
		goto found_sect;

	*ptepp = pte_offset_map(*pmdpp, addr);
	pte = **ptepp;
	if (pte_present(pte)) {
		DBGPT(PTE, *ptepp, pte_val(pte));
		pte_unmap(*ptepp);
		goto found_pte;
	}
	pte_unmap(*ptepp);
#undef DBGPT

not_found:
	DBG("Page table descriptors not found\n");
	return -ENOMEM;

found_sect:
	DBG("Page block found\n");
	goto out;

found_pte:
	DBG("Page table descriptors found\n");

out:
	return 0;
}

static int hkip_get_pgtable_descs(struct hkipt_pgtable_descs *req)
{
	pgd_t *pgdp = NULL;
	pud_t *pudp = NULL;
	pmd_t *pmdp = NULL;
	pte_t *ptep = NULL;
	int err;

	err = _get_pgtable_desc(req->va, &pgdp, &pudp, &pmdp, &ptep);
	if (err)
		goto not_found;

#define SET_DESC(index, type)                                                  \
{                                                                              \
	if (type##p) {                                                         \
		req->desc_addr[index] = (uintptr_t)type##p;                    \
		req->desc_val[index] = type##_val(*type##p);                   \
	} else {                                                               \
		req->desc_addr[index] = 0;                                     \
		req->desc_val[index] = 0;                                      \
	}                                                                      \
}

	SET_DESC(PTE_DESC, pte);

#if CONFIG_PGTABLE_LEVELS > 2
	SET_DESC(PMD_DESC, pmd);
#endif

#if CONFIG_PGTABLE_LEVELS > 3
	SET_DESC(PUD_DESC, pud);
#endif

	SET_DESC(PGD_DESC, pgd);
#undef SET_DESC

	return 0;

not_found:
	DBG("Pgtable descriptor not found\n");
	return err;
}

static inline pud_t pmd_pud(pmd_t pmd)
{
	return __pud(pmd_val(pmd));
}

static inline pgd_t pud_pgd(pud_t pud)
{
	return __pgd(pud_val(pud));
}

static inline pud_t pgd_pud(pgd_t pgd)
{
	return __pud(pgd_val(pgd));
}

static pte_t __pte_modify(pte_t pte, pgprot_t newprot)
{
	/* Allow following additonal bits to be modified */
	pteval_t mask = PTE_AF | PTE_DBM | PTE_DIRTY;

	pte = pte_modify(pte, newprot);

	pte_val(pte) = (pte_val(pte) & ~mask) | (pgprot_val(newprot) & mask);

	return pte;
}

static inline pmd_t __pmd_modify(pmd_t pmd, pgprot_t newprot)
{
	return pte_pmd(__pte_modify(pmd_pte(pmd), newprot));
}

static inline pud_t __pud_modify(pud_t pud, pgprot_t newprot)
{
	return pmd_pud(__pmd_modify(pud_pmd(pud), newprot));
}

static inline pgd_t __pgd_modify(pgd_t pgd, pgprot_t newprot)
{
	return pud_pgd(__pud_modify(pgd_pud(pgd), newprot));
}

static void update_pte(pte_t *ptep, u64 prot, const char *type)
{
	pte_t old, now, new;

	old = *ptep;
	new = __pte_modify(old, __pgprot(prot));
	if (pte_val(old) == pte_val(new))
		/* Protection bits are unchanged, avoid writing PTE */
		return;

	now = __pte(pte_val(*ptep));
	if (pte_val(old) != pte_val(now))
		/* PTE has changed since read, recalculate update */
		new = __pte_modify(now, __pgprot(prot));

	DBG("%s update [0x%pK]: 0x%016llx, 0x%016llx\n", type, ptep,
	    pte_val(new), prot);

	set_pte(ptep, new);
}

static void update_pmd_block(pmd_t *pmdp, u64 prot, const char *type)
{
	pmd_t old, now, new;

	old = *pmdp;
	new = __pmd_modify(old, __pgprot(prot));
	if (pmd_val(old) == pmd_val(new))
		/* Protection bits are unchanged, avoid writing PTE */
		return;

	now = __pmd(pmd_val(*pmdp));
	if (pmd_val(old) != pmd_val(now))
		/* PTE has changed since read, recalculate update */
		new = __pmd_modify(now, __pgprot(prot));

	DBG("%s block update [0x%pK]: 0x%016llx, 0x%016llx\n", type, pmdp,
	    pmd_val(new), prot);

	set_pmd(pmdp, new);
}

#define PDE(shift)     (1UL << shift)
#define PDE_NS         PDE(63)
#define PDE_RO         PDE(62)
#define PDE_KERN       PDE(61)
#define PDE_UXN        PDE(60)
#define PDE_PXN        PDE(59)

#define PGD_TYPE_TABLE PMD_TYPE_TABLE

#define DEFINE_UPDATE_PTE(type, TYPE)                                          \
static inline void update_##type(type##_t *ptr, uint64_t newprot,              \
				 const char *type)                             \
{                                                                              \
	const type##val_t mask =                                               \
		PDE_NS | PDE_RO | PDE_KERN | PDE_UXN | PDE_PXN;                \
                                                                               \
	if ((type##_val(*ptr) & PMD_TYPE_MASK) == TYPE##_TYPE_TABLE) {         \
		type##_t pde;                                                  \
		pde = __##type((type##_val(*ptr) & ~mask) |                    \
			       (newprot & mask));                              \
		DBG(#type                                                      \
		    "table update [0x%016lx]: 0x%016llx, 0x%016llx\n",         \
		    (uintptr_t)ptr, type##_val(pde), newprot);                 \
		if (type##_val(*ptr) != type##_val(pde))                       \
			set_##type(ptr, pde);                                  \
	} else if ((type##_val(*ptr) & PMD_TYPE_MASK) ==                       \
		   PMD_TYPE_SECT) {                                            \
		update_pmd_block((pmd_t *)ptr, newprot, #type);                \
	} else {                                                               \
		update_pte((pte_t *)ptr, newprot, #type);                      \
	}                                                                      \
}

DEFINE_UPDATE_PTE(pgd, PGD)
DEFINE_UPDATE_PTE(pmd, PMD)
DEFINE_UPDATE_PTE(pud, PUD)

static int hkip_set_pgtable_descs(struct hkipt_pgtable_descs *req)
{
	pgd_t *pgdp = NULL;
	pud_t *pudp = NULL;
	pmd_t *pmdp = NULL;
	pte_t *ptep = NULL;
	int err;
	unsigned long start, end, size;

	err = _get_pgtable_desc(req->va, &pgdp, &pudp, &pmdp, &ptep);
	if (err)
		return err;

#define CHECK_AND_SET_DESC(type)                                               \
	if (type##p) {                                                         \
		if (req_##type##p == type##p) {                                \
			update_##type(type##p, req_##type, #type);             \
		} else {                                                       \
			DBG("Address mismatch for " #type ", addr should be "  \
			    "0x%016lx but it's 0x%016lx \n",                   \
			    (uintptr_t)type##p, (uintptr_t)req_##type##p);     \
		}                                                              \
	}

	CHECK_AND_SET_DESC(pte)

#if CONFIG_PGTABLE_LEVELS > 2
	CHECK_AND_SET_DESC(pmd)
#endif

#if CONFIG_PGTABLE_LEVELS > 3
	CHECK_AND_SET_DESC(pud)
#endif

	CHECK_AND_SET_DESC(pgd)
#undef CHECK_AND_SET_DESC

	if (ptep) {
		size = PAGE_SIZE;
	} else if (pmdp) {
		size = PMD_SIZE;
	} else if (pudp) {
		size = PUD_SIZE;
	} else if (pgdp) {
		size = PGDIR_SIZE;
	} else {
		DBG("WARNING: TLB not flushed\n");
		goto out;
	}

	start = req->va & PAGE_MASK;
	end = start + size;

	printk(KERN_ERR "Flushing TLB for VA 0x%016lx - 0x%016lx\n", start, end);
	flush_tlb_kernel_range(start, end);
out:
	printk(KERN_ERR "pgtable descriptor updated\n");
	return 0;
}

static int hkip_set_pte(struct hkipt_pgtable_descs *req)
{
	pgd_t *pgdp = NULL;
	pud_t *pudp = NULL;
	pmd_t *pmdp = NULL;
	pte_t *ptep = NULL;
	int err;

	err = _get_pgtable_desc(req->va, &pgdp, &pudp, &pmdp, &ptep);
	if (err)
		return err;

	if (!ptep) {
		pr_err("PTE not found");
		return -ENOENT;
	}

	if (req_ptep != ptep) {
		pr_err("PTE address does not match");
		return -ENOENT;
	}

	set_pte(ptep, __pte(req_pte));
	return 0;
}
static int hkip_vmalloc(struct hkipt_vmem *req)
{
	req->va = (uintptr_t)vmalloc_user(req->size);
	if (req->va)
		return 0;

	return -ENOMEM;
}

static int hkip_vfree(struct hkipt_vmem *req)
{
	vfree((void *)req->va);
	return 0;
}

static int hkip_get_event_log_info(struct hkipt_eventlog_info *req)
{
	uint64_t base, size;
	int err;

	err = hkip_get_monitorlog_info(&base, &size);
	if (err)
		return err;

	req->base = base;
	req->size = size;

	return 0;
}

static void __iomem *do_map_phy_mem(phys_addr_t phys_addr, size_t size,
				    pgprot_t prot, void *caller)
{
	uintptr_t last_addr;
	uintptr_t offset = phys_addr & ~PAGE_MASK;
	int err;
	uintptr_t addr;
	struct vm_struct *area = NULL;

	/*
	 * Page align the mapping address and size, taking account of any
	 * offset.
	 */
	phys_addr &= PAGE_MASK;
	size = PAGE_ALIGN(size + offset);

	/*
	 * Don't allow wraparound, zero size or outside PHYS_MASK.
	 */
	last_addr = phys_addr + size - 1;
	if (!size || last_addr < phys_addr || (last_addr & ~PHYS_MASK))
		return ERR_PTR(-EINVAL);

	/* Allow remapping dynamically allocated memory */
#ifdef HKIP_TEST_DEBUG
	if (WARN_ON(pfn_valid(__phys_to_pfn(phys_addr))))
		return NULL;
#endif

	area = get_vm_area_caller(size, VM_IOREMAP,
				  __builtin_return_address(0));
	if (!area)
		return ERR_PTR(-ENOSPC);

	addr = (uintptr_t)area->addr;
	area->phys_addr = phys_addr;

	err = ioremap_page_range(addr, addr + size, phys_addr, prot);
	if (err) {
		vunmap((void *)addr);
		return ERR_PTR(err);
	}

	return (void __iomem *)(uintptr_t)(offset + addr);
}

static int hkip_map_phy_mem(hkipt_mem_area_t *req)
{
	void __iomem *va = NULL;

	va = do_map_phy_mem(req->base, req->size, PAGE_KERNEL,
			    __builtin_return_address(0));
	if (IS_ERR(va))
		return PTR_ERR(va);

	req->base = (uint64_t)(uintptr_t)va;

	return 0;
}

static int hkip_unmap_phy_mem(hkipt_mem_area_t *req)
{
	iounmap((uintptr_t *)req->base);
	return 0;
}

static uint64_t g_token = 0;

static int fetch_token(u64 *token)
{
	uint64_t ret = 0;

	if (HHEE_DISABLE == hhee_check_enable()) {
		pr_err("[%s]hhee is disabled\n", __func__);
		return 0;
	}

	hkip_hvc_2(HVC_GET_TOKEN, &ret, token, 0, 0, 0);
	return ret;
}

static int init_token(void)
{
	int ret;
	pr_info("Getting token for first time\n");
	ret = fetch_token(&g_token);
	if (ret) {
		pr_err("Failed to get magic number for the first time [%d]\n", ret);
	} else {
		pr_info("magic number fetched successfully\n");
	}

	return 0;
}
early_initcall(init_token);

static int hkip_get_token(struct hkipt_token *req)
{
	int ret;

	if (req->refetch) {
		ret = fetch_token(&req->token);
		if (ret) {
			pr_err("Failed to refetch magic num. ret: [%d]\n", ret);
			req->error = ret;
			return -EPERM;
		}
	} else {
		req->token = g_token;
	}
	req->error = 0;
	return 0;
}

static int hkip_get_stage2(struct hkipt_stage2_info *req)
{
	do_hvc(HKIP_RETURN_STAGE2_INFO, &req->va, &req->pxn, &req->rd, &req->rw,
	       &req->flag);
	return 0;
}

static int hkip_patch(struct hkipt_patch *req)
{
	/* check for errors */
	return hkip_hvc(HVC_LIVEPATCH, req->va, req->size, req->low, req->high,
			req->token);
}

static int hkip_make_hvc(struct hkipt_hvc *req)
{
	uint64_t unknown = 0xffffffff;

	do_hvc(req->call_id, &req->x0, &req->x1, &req->x2, &req->x3, &req->x4);
	if (req->x0 == unknown)
		return -EINVAL;

	return 0;
}

static int hkip_make_smc(struct hkipt_smc *req)
{
	uint64_t unknown = 0xffffffff;

	do_smc(req->call_id, &req->x0, &req->x1, &req->x2, &req->x3, &req->x4);
	if (req->x0 == unknown)
		return -EINVAL;

	return 0;
}
static int hkip_read(struct hkipt_read_write *req)
{
	if (!req->va)
		return -EINVAL;

	switch (req->size) {
	case 8:
		req->data.x8 = (uint8_t)req->va;
		break;
	case 16:
		req->data.x16 = (uint16_t)req->va;
		break;
	case 32:
		req->data.x32 = (uint32_t)req->va;
		break;
	case 64:
		req->data.x64 = (uint64_t)req->va;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int hkip_write(struct hkipt_read_write *req)
{
	void *va = NULL;
	if (!req->va)
		return -EINVAL;

	va = (void *)req->va;

	switch (req->size) {
	case 8:
		*(uint8_t *)va = req->data.x8;
		break;
	case 16:
		*(uint16_t *)va = req->data.x16;
		break;
	case 32:
		*(uint32_t *)va = req->data.x32;
		break;
	case 64:
		*(uint64_t *)va = req->data.x64;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static unsigned long *lsm_hooks = NULL;
static DEFINE_MUTEX(lsm_hooks_lock);

static int hkip_get_lsm_hooks(hkipt_mem_area_t *req)
{
	struct list_head *const heads =
		(struct list_head *)&security_hook_heads;
	unsigned long *table = NULL;
	unsigned long *pp = NULL;
	const unsigned nr_heads =
		sizeof(security_hook_heads) / sizeof(struct list_head);
	unsigned nr_hooks = 0;
	unsigned size;
	unsigned i;

	DBG("Counting hooks\n");

	for (i = 0; i < nr_heads; i++) {
		struct list_head *pos = NULL;

		list_for_each (pos, &heads[i])
			nr_hooks++;
	}

	DBG("Found %u hooks\n", nr_hooks);

	size = (nr_heads + nr_hooks) * sizeof(*table);
	table = vmalloc_user(PAGE_ALIGN(size));
	pp = table;

	if (unlikely(table == NULL))
		return -ENOMEM;

	/* NOTE: assume that hooks did not change asynchronously */
	for (i = 0; i < nr_heads; i++) {
		struct list_head *pos = NULL;

		*(pp++) = (unsigned long)(uintptr_t)&heads[i];

		list_for_each (pos, &heads[i])
			*(pp++) = (unsigned long)(uintptr_t)pos;
	}

	DBG("Return hook addresses\n");
	req->base = ((uintptr_t)table) & HKIP_VADDR_ADDMASK;
	req->size = size;

	mutex_lock(&lsm_hooks_lock);
	if (lsm_hooks)
		vfree(lsm_hooks);

	lsm_hooks = table;
	mutex_unlock(&lsm_hooks_lock);
	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
static int noop_file_open(struct file *file)
{
	return 0;
}
#else
static int noop_file_open(struct file *file, const struct cred *cred)
{
	return 0;
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
struct security_hook_list hkipt_lsm_hooks[] __ro_after_init = { LSM_HOOK_INIT(
	file_open, noop_file_open) };
#else
struct security_hook_list hkipt_lsm_hooks[] HISI_RO_LSM_HOOKS = { LSM_HOOK_INIT(
	file_open, noop_file_open) };
#endif

static int ioc_register_lsm_hooks(const void __user *argp
				  __attribute__((unused)))
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	security_add_hooks(hkipt_lsm_hooks, ARRAY_SIZE(hkipt_lsm_hooks),
			   "hkipt_lsm");
#else
	security_add_hooks(hkipt_lsm_hooks, ARRAY_SIZE(hkipt_lsm_hooks));
#endif
	return 0;
}

static int ioc_deregister_lsm_hooks(const void __user *argp
				    __attribute__((unused)))
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(hkipt_lsm_hooks); i++)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
		hlist_del_rcu(&hkipt_lsm_hooks[i].list);
#else
		list_del_rcu(&hkipt_lsm_hooks[i].list);
#endif
	return 0;
}

static uintptr_t *sel_db_ptrs = 0;
static size_t sel_db_size = 0;

static int sandbox_dispatcher(long (*func)(void *), void *data)
{
	unsigned long flags;
	long ret;

	DBG("sandbox_dispatcher: disable irqs\n");
	local_irq_save(flags);
	ret = (long)(uintptr_t)sandbox_try_execute(func, data);
	local_irq_restore(flags);
	DBG("sandbox_dispatcher: enable irqs\n");
	DBG("sandbox_dispatcher: got reply: %ld\n", ret);

	if (ret == -4)
		return -EINTR; /* Exception occurred during access */
	if (ret < 0)
		return -EINVAL;

	return ret;
}

static int hkip_jump(struct hkipt_jump *req)
{
	int ret;
	typedef long (*jump_t)(void *);

	jump_t jump = (jump_t)(uintptr_t)req->addr;

	pr_info("hkip_jump: sandbox addr %pK\n", el1_text_sandbox);
	pr_info("hkip_jump: input %d\n", req->in);
	pr_info("hkip_jump: jump addr 0x%llx (%pK)\n", req->addr, jump);

	ret = sandbox_dispatcher(jump, (void *)(uintptr_t)req->in);
	if (ret < 0)
		return ret;

	req->out = (uint32_t)ret;
	pr_info("hkip_jump: output %d\n", req->out);
	return 0;
}

static long make_read_access(void *data)
{
	struct hkipt_pgtable_descs *req = (struct hkipt_pgtable_descs *)data;
	uint32_t *ptr = NULL;
	ptr = (uint32_t *)req->va;

	/* prevent to be optimized by compiler.  */
	__asm__ volatile("ldr   x0, [%0]" : : "r"(ptr) : "memory");

	return 0;
}

static long make_write_access(void *data)
{
	struct hkipt_pgtable_descs *req = (struct hkipt_pgtable_descs *)data;
	uint32_t val;
	uint32_t volatile *ptr = NULL;
	ptr = (uint32_t *)req->va;

	val = *ptr;
	*ptr = 0xa5;
	*ptr = val;

	return 0;
}

static int hkip_check_tthm_ha(struct hkipt_pgtable_descs *req)
{
#ifdef HKIP_TEST_DEBUG
	/* Check with AT can not be done here,
	   * it will report fault if AF is clear,
	   * which is likely scenario */

	if (el1_addr_invalid(req->va, 0))
		return -EIO;
#endif

	return sandbox_dispatcher(make_read_access, req);
}

static int hkip_check_tthm_hd(struct hkipt_pgtable_descs *req)
{
	return sandbox_dispatcher(make_write_access, req);
}

static int _get_pte_leaf_level(struct hkipt_pgtable_descs *req)
{
	int level;

	/* Get to lowest pgtable descriptor */
	level = -1;
	if (req->desc_addr[PGD_DESC]) {
		level = PGD_DESC;
		if (req->desc_addr[PUD_DESC]) {
			level = PUD_DESC;
			if (req->desc_addr[PMD_DESC]) {
				level = PMD_DESC;
				if (req->desc_addr[PTE_DESC])
					level = PTE_DESC;
			}
		}
	}

	if (level == -1) {
		pr_err("Failed to find final page table level descriptor\n");
		return -EINVAL;
	}
	return level;
}

static int hkip_pgtable_test_and_clear_young(struct hkipt_pgtable_descs *req)
{
	int level;
	unsigned long addr;
	struct mm_struct *mm = NULL;
	struct vm_area_struct *vma = NULL;

	addr = req->va;
	if (!addr)
		return -EINVAL;

	level = _get_pte_leaf_level(req);
	if (level < 0) {
		return level;
	}

	mm = _get_mm(addr);
	vma = find_vma(mm, addr);
	if (!vma) {
		pr_warn("VMA not found for 0x%lx\n", addr);
		return -ENOMEM;
	}

	return ptep_test_and_clear_young(vma, addr, (pte_t *)(void *)req->desc_addr[level]);
}

static int hkip_pgtable_set_wrprotect(struct hkipt_pgtable_descs *req)
{
	int level;
	uintptr_t addr;
	struct mm_struct *mm = NULL;

	addr = req->va;
	if (!addr)
		return -EINVAL;

	level = _get_pte_leaf_level(req);
	if (level < 0) {
		return level;
	}

	mm = _get_mm(addr);
	ptep_set_wrprotect(mm, addr, (pte_t *)(void *)req->desc_addr[level]);
	return 0;
}

static int ioc_enable_tvm(const void __user *argp __attribute__((unused)))
{
	return hkip_hvc(HVC_ENABLE_TVM, 0, 0, 0, 0, 0);
}

static int ioc_disable_livepatch(const void __user *argp
				 __attribute__((unused)))
{
	return hkip_hvc(HVC_DISABLE_LIVEPATCH, 0, 0, 0, 0, 0);
}

#define DEFINE_IOCFUNC_INOUT(ioc, func, _struct)                               \
static int ioc_##func(void __user *argp)                                       \
{                                                                              \
	struct hkipt_##_struct req;                                            \
	int err;                                                               \
                                                                               \
	if (!argp) {                                                           \
		printk("userspace input null\n");                              \
		return -EFAULT;                                                \
	}                                                                      \
	err = (int)copy_from_user(&req, argp,                                       \
			     sizeof(struct hkipt_##_struct));                  \
	if (err) {                                                             \
		DBG("userspace access failed\n");                              \
		return -EFAULT;                                                \
	}                                                                      \
                                                                               \
	err = hkip_##func(&req);                                               \
	if (err)                                                               \
		return err;                                                    \
                                                                               \
	err = (int)copy_to_user(argp, &req,                                         \
			   sizeof(struct hkipt_##_struct));                    \
	if (err) {                                                             \
		DBG("userspace access failed\n");                              \
		return -EFAULT;                                                \
	}                                                                      \
                                                                               \
	return 0;                                                              \
}

#define DEFINE_IOCFUNC_IN(ioc, func, _struct)                                  \
static int ioc_##func(const void __user *argp)                                 \
{                                                                              \
	struct hkipt_##_struct req;                                            \
	int err;                                                               \
                                                                               \
	if (!argp) {                                                           \
		printk("userspace input null\n");                              \
		return -EFAULT;                                                \
	}                                                                      \
	err = (int)copy_from_user(&req, argp,                                       \
			     sizeof(struct hkipt_##_struct));                  \
	if (err) {                                                             \
		DBG("userspace access failed\n");                              \
		return -EFAULT;                                                \
	}                                                                      \
                                                                               \
	err = hkip_##func(&req);                                               \
	if (err)                                                               \
		return err;                                                    \
                                                                               \
	return 0;                                                              \
}

#define DEFINE_IOCFUNC_OUT(ioc, func, _struct)                                 \
static int ioc_##func(void __user *argp)                                       \
{                                                                              \
	struct hkipt_##_struct req;                                            \
	int err;                                                               \
                                                                               \
	if (!argp) {                                                           \
		printk("userspace input null\n");                              \
		return -EFAULT;                                                \
	}                                                                      \
                                                                               \
	err = hkip_##func(&req);                                               \
	if (err)                                                               \
		return err;                                                    \
                                                                               \
	err = (int)copy_to_user(argp, &req,                                         \
			   sizeof(struct hkipt_##_struct));                    \
	if (err) {                                                             \
		DBG("userspace access failed\n");                              \
		return -EFAULT;                                                \
	}                                                                      \
                                                                               \
	return 0;                                                              \
}

DEFINE_IOCFUNC_INOUT(ALLOCVMEM, vmalloc, vmem)
DEFINE_IOCFUNC_IN(FREEVMEM, vfree, vmem)
DEFINE_IOCFUNC_INOUT(GETPGTABLEDESCS, get_pgtable_descs, pgtable_descs)
DEFINE_IOCFUNC_IN(SETPGTABLEDESCS, set_pgtable_descs, pgtable_descs)
DEFINE_IOCFUNC_IN(SETPTE, set_pte, pgtable_descs)
DEFINE_IOCFUNC_OUT(GETEVENTLOGINFO, get_event_log_info, eventlog_info)
DEFINE_IOCFUNC_INOUT(MAPPHYMEM, map_phy_mem, eventlog_info)
DEFINE_IOCFUNC_IN(UNMAPPHYMEM, unmap_phy_mem, eventlog_info)
DEFINE_IOCFUNC_INOUT(GETTOKEN, get_token, token)
DEFINE_IOCFUNC_IN(PATCH, patch, patch)
DEFINE_IOCFUNC_INOUT(HVC, make_hvc, hvc)
DEFINE_IOCFUNC_INOUT(SMC, make_smc, smc)
DEFINE_IOCFUNC_INOUT(READ, read, read_write)
DEFINE_IOCFUNC_IN(WRITE, write, read_write)
DEFINE_IOCFUNC_OUT(LSM_GETHOOKS, get_lsm_hooks, eventlog_info)
DEFINE_IOCFUNC_INOUT(GET_STAGE2, get_stage2, stage2_info)
DEFINE_IOCFUNC_INOUT(JUMP, jump, jump)
DEFINE_IOCFUNC_INOUT(CHECK_HA, check_tthm_ha, pgtable_descs)
DEFINE_IOCFUNC_INOUT(CHECK_HD, check_tthm_hd, pgtable_descs)
DEFINE_IOCFUNC_IN(PGT_TCY, pgtable_test_and_clear_young, pgtable_descs)
DEFINE_IOCFUNC_IN(PGT_SETWP, pgtable_set_wrprotect, pgtable_descs)

typedef int (*hkip_ioctl_func_const_t)(const void __user *argp);

typedef int (*hkip_ioctl_func_t)(void __user *argp);

struct hkip_ioctl_ops {
	unsigned int cmd;
	unsigned char is_const;
	hkip_ioctl_func_t func;
	hkip_ioctl_func_const_t func_c;
};

#define HKIP_IOCTL_OPS_CONST(_cmd, _func)                                      \
	{                                                                      \
		.cmd = (_cmd), .is_const = 1, .func = NULL, .func_c = (_func)  \
	}
#define HKIP_IOCTL_OPS(_cmd, _func)                                            \
	{                                                                      \
		.cmd = (_cmd), .is_const = 0, .func = (_func), .func_c = NULL  \
	}

static struct hkip_ioctl_ops ioctl_ops[] = {
	HKIP_IOCTL_OPS(HKIPT_IOCALLOCVMEM, ioc_vmalloc),
	HKIP_IOCTL_OPS_CONST(HKIPT_IOCFREEVMEM, ioc_vfree),
	HKIP_IOCTL_OPS(HKIPT_IOCGETPGTABLEDESCS, ioc_get_pgtable_descs),
	HKIP_IOCTL_OPS_CONST(HKIPT_IOCSETPGTABLEDESCS, ioc_set_pgtable_descs),
	HKIP_IOCTL_OPS_CONST(HKIPT_IOCSETPTE, ioc_set_pte),
	HKIP_IOCTL_OPS(HKIPT_IOCGETEVENTLOGINFO, ioc_get_event_log_info),
	HKIP_IOCTL_OPS(HKIPT_IOCMAPPHYMEM, ioc_map_phy_mem),
	HKIP_IOCTL_OPS_CONST(HKIPT_IOCUNMAPPHYMEM, ioc_unmap_phy_mem),
	HKIP_IOCTL_OPS_CONST(HKIPT_IOCPATCH, ioc_patch),
	HKIP_IOCTL_OPS(HKIPT_IOCGETTOKEN, ioc_get_token),
	HKIP_IOCTL_OPS(HKIPT_IOCHVC, ioc_make_hvc),
	HKIP_IOCTL_OPS(HKIPT_IOCREAD, ioc_read),
	HKIP_IOCTL_OPS_CONST(HKIPT_IOCWRITE, ioc_write),
	HKIP_IOCTL_OPS(HKIPT_IOCLSM_GETHOOKS, ioc_get_lsm_hooks),
	HKIP_IOCTL_OPS_CONST(HKIPT_IOCLSM_REGHOOKS, ioc_register_lsm_hooks),
	HKIP_IOCTL_OPS_CONST(HKIPT_IOCLSM_DEREGHOOKS, ioc_deregister_lsm_hooks),
	HKIP_IOCTL_OPS(HKIPT_IOCGET_STAGE2, ioc_get_stage2),
	HKIP_IOCTL_OPS(HKIPT_IOCJUMP, ioc_jump),
	HKIP_IOCTL_OPS(HKIPT_IOCCHECK_HA, ioc_check_tthm_ha),
	HKIP_IOCTL_OPS(HKIPT_IOCCHECK_HD, ioc_check_tthm_hd),
	HKIP_IOCTL_OPS_CONST(HKIPT_IOCPGT_TCY, ioc_pgtable_test_and_clear_young),
	HKIP_IOCTL_OPS_CONST(HKIPT_IOCPGT_SETWP, ioc_pgtable_set_wrprotect),
	HKIP_IOCTL_OPS_CONST(HKIPT_IOCENABLE_TVM, ioc_enable_tvm),
	HKIP_IOCTL_OPS_CONST(HKIPT_IOCDISABLE_LIVEPATCH, ioc_disable_livepatch),
	HKIP_IOCTL_OPS(HKIPT_IOCSMC, ioc_make_smc),
};

static int hkip_ioctl(struct file *file, unsigned int cmd, uintptr_t arg)
{
	uint32_t idx;
	void __user *argp = (void __user *)arg;
	hkip_ioctl_func_t func = NULL;
	hkip_ioctl_func_const_t func_c = NULL;

	if (hhee_check_enable() == HHEE_DISABLE) {
		pr_err("hkip ioctl: hhee is disabled\n");
		return 0;
	}

	for (idx = 0; idx < sizeof(ioctl_ops) / sizeof(ioctl_ops[0]); idx++) {
		if (cmd == ioctl_ops[idx].cmd) {
			if (ioctl_ops[idx].is_const == 1) {
				if (ioctl_ops[idx].func_c == NULL) {
					pr_err("hkip ioctl: invalid func\n");
					return -EINVAL;
				}
				func_c = ioctl_ops[idx].func_c;
				return func_c(argp);
			} else {
				if (ioctl_ops[idx].func == NULL) {
					pr_err("hkip ioctl: invalid func\n");
					return -EINVAL;
				}
				func = ioctl_ops[idx].func;
				return func(argp);
			}
		}
	}

	pr_err("hkip ioctl: invalid cmd\n");
	return -EINVAL;
}

static DEFINE_MUTEX(hkip_mutex);

long hkip_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret;

	mutex_lock(&hkip_mutex);
	ret = hkip_ioctl(file, cmd, arg);
	mutex_unlock(&hkip_mutex);

	return ret;
}

static void __exit cleanup(void)
{
	if (lsm_hooks) {
		vfree(lsm_hooks);
		lsm_hooks = NULL;
	}
}
module_exit(cleanup);
