/*
 * mm_lb.c
 *
 * lb drv
 *
 * Copyright (c) 2018-2019 Huawei Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include "mm_lb.h"

#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
#include <linux/arm-smccc.h>
#else
#include <asm/compiler.h>
#endif
#include <asm/cacheflush.h>
#include <asm/memory.h>
#include <asm/tlbflush.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/memblock.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/sizes.h>
#include <linux/smp.h>
#include <linux/moduleparam.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/spinlock.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/page-flags.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/genalloc.h>
#include <linux/uaccess.h>
#include <linux/sort.h>
#include <platform_include/basicplatform/linux/rdr_platform.h>
#include <dsm/dsm_pub.h>

#include "mm_lb_priv.h"

#define MM_LB_FID_VALUE          0xc500bc00u
#define MM_LB_QUOTA_VALUE        0xa13584
#define MM_LB_PLC_VALUE          0x4135a4
#define MM_LB_EFUSE              0xa358767
#define MM_LB_LITE               0x435912f

#define GID_STATIC          BIT(0)
#define GID_DYNAMIC         BIT(1)
#define GID_NON_SEC         BIT(2)
#define GID_SEC             BIT(3)
#define GID_VM_STATE        BIT(4)
#define GID_BYPASS_STATE    BIT(5)
#define GID_PAGES_REMAP     BIT(6)

#define QUOTA_L_SHIFT 16
#define QUOTA_L_MASK  ((1U << QUOTA_L_SHIFT) - 1)
#define QUOTA_H_MASK  (~QUOTA_L_MASK)

#define DFX_CMD_TYPE_MASK BIT(12)
#define DFX_CMD_MID_SHIFT 4
#define DFX_CMD_MID_MASK  (0xff << DFX_CMD_MID_SHIFT)

#define LB_LARGIEST_COUNT         0x7000
#define LB_PAGE_FLAG              0x1
#define LB_PRIV_GID_WIDE          4
#define LB_PRIV_GID_MASK          (BIT(LB_PRIV_GID_WIDE) - 1)
#define FINISH_MASK               0x1
#define CMO_EVENT_INT_CLR_FLAG    BIT(19)
#define CMD_PARAM_SHIFT           16

#if defined(CONFIG_MM_LB_REMAP)
#define INT_MASK                  0xfdffffff
#define ROUTE_INT_MASK            0xfffff
#define CMO_CFG_INT_MASK          0x1f000
#else
#define INT_MASK                  0xffffffff
#define ROUTE_INT_MASK            0xffff
#define CMO_CFG_INT_MASK          0x1f000
#endif

#define MAX_NSEC_GID_NUM          8
#define GID_EN_SHIFT              11
#define GID_TINY                  14
#define GID_HIFI                  15

#if defined(CONFIG_MM_LB_REMAP)
#ifdef CONFIG_HISI_LB_WAY_16
#define GID_WAY_EN     0xffff
#define GID_WAY_SRCH   0xffff
#define GID_WAY_FPGA   0xffff
#else
#define GID_WAY_EN     0x00ff
#define GID_WAY_SRCH   0x00ff
#define GID_WAY_FPGA   0x00ff
#endif
#define LB_LINE_SHIFT  9
#elif defined(CONFIG_MM_LB_GEMINI)
#define GID_WAY_EN     0xffff
#define GID_WAY_SRCH   0xffff
#define GID_WAY_FPGA   0x00ff
#define LB_LINE_SHIFT  9
#else
#define GID_WAY_EN     0x3fff
#define GID_WAY_SRCH   0x3fff
#define GID_WAY_FPGA   0x3fff
#define LB_LINE_SHIFT  7
#endif

#define DFX_ECCERR_SINGLE_MASK     BIT(0)
#define DFX_ECCERR_MULTI_MASK      BIT(1)
#define DSM_LB_ECC_SINGLE_BIT      925205200
#define DSM_LB_ECC_MUTIL_BIT       925205201

#define SINGLE_BIT_EVENT_MAX   10
#define MUTIL_BIT_EVENT_MAX    10

#define CMO_OPERATE_MASK  0xf
#define CMO_BITMAP_MASK   0xffff
#define CMO_64B_PA_SHIFT  6
#define CMO_128B_PA_SHIFT 7
#define CMO_4K_PA_SHIFT   12
#define DFX_CMD_PA_SHIFT  2

#define FLOW_START_MASK   0x4000319
#define FLOW_END_MASK     0x318
#define FLOW_GID_SHIFT    10
#define FLOW_GID_MASK     (0xffff << FLOW_GID_SHIFT)
#define FLOW_ENABLE_MASK  BIT(26)
#define FLOW_MAX_ID       4
#define FLOWID_TO_SLC(x)  (x - 1U)

struct lb_device *lbdev;
static int single_bit;
static int mutil_bit;
static u32 cmo_dummy_pa;
static u32 is_fpga;

static noinline u64 kernel_notify_atf(u64 fid, u32 spid, u32 quota, u32 plc)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	struct arm_smccc_res res;

	arm_smccc_smc(fid, spid, quota, plc,
		0, 0, 0, 0, &res);
	return (int)res.a0;
#else
	register u64 x0 asm("x0") = fid;
	register u64 x1 asm("x1") = spid;
	register u64 x2 asm("x2") = quota;
	register u64 x3 asm("x3") = plc;

	asm volatile (
		__asmeq("%0", "x0")
		__asmeq("%1", "x1")
		__asmeq("%2", "x2")
		__asmeq("%3", "x3")
		"smc    #0\n"
		: "+r" (x0)
		: "r" (x1), "r" (x2), "r"(x3));

	return x0;
#endif
}

static int power_control(u32 flag)
{
	int ret;

	ret = kernel_notify_atf(MM_LB_FID_VALUE, flag,
			MM_LB_QUOTA_VALUE, MM_LB_PLC_VALUE);
	if (ret) {
		lb_print(LB_ERROR, "%s fail flag: 0x%x, ret: 0x%x\n",
			__func__, flag, ret);
		WARN_ON(1);
	}

	return ret;
}

/*lint -e548*/
static int gid_bypass_control(u32 cmd, u32 gid, u32 quota, u32 plc)
{
	int ret;

	ret = (int)kernel_notify_atf(MM_LB_FID_VALUE,
			cmd | (gid << CMD_PARAM_SHIFT), quota, plc);
	if (ret) {
		lb_print(LB_ERROR, "%s fail 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
			__func__, cmd, gid, quota, plc, ret);
		WARN_ON(1);
	}

	return ret;
}

static int sec_gid_set(u32 cmd, u32 gid)
{
	int ret;

	ret = (int)kernel_notify_atf(MM_LB_FID_VALUE,
			cmd | (gid << CMD_PARAM_SHIFT), 0, 0);
	if (ret) {
		lb_print(LB_ERROR, "%s fail cmd: 0x%x, gid: 0x%x, ret: 0x%x\n",
			__func__, cmd, gid, ret);
		WARN_ON(1);
	}

	return ret;
}
/*lint +e548*/

static unsigned int get_lb_efuse(void)
{
	unsigned int ret;

	ret = kernel_notify_atf(MM_LB_FID_VALUE, MM_LB_EFUSE,
		MM_LB_QUOTA_VALUE, MM_LB_PLC_VALUE);

	return ret;
}

static unsigned int get_lb_lite(void)
{
	unsigned int ret;

	ret = kernel_notify_atf(MM_LB_FID_VALUE, MM_LB_LITE,
		MM_LB_QUOTA_VALUE, MM_LB_PLC_VALUE);

	return ret;
}

static int change_size_from_efuse(struct lb_device *lbd)
{
	int i = 0, j = 0;
	unsigned int way = GID_WAY_EN;
	unsigned int way_efuse = lbd->way_efuse;

	while (way) {
		if ((way & BIT(0)) == (way_efuse & BIT(0)))
			j++;

		way >>= 1;
		way_efuse >>= 1;
		i++;
	}

	if (unlikely(i == 0))
		return -EINVAL;

	lbd->gdplc.max_quota = lbd->gdplc.max_quota / i * j;
	lbd->gdplc.max_size = lbd->gdplc.max_size / i * j;

	lb_print(LB_ERROR, "max_quota 0x%x, max_size 0x%x\n",
			lbd->gdplc.max_quota, lbd->gdplc.max_size);

	return 0;
}

static inline unsigned int __is_lb_available(void)
{
	return (unsigned int)(lbdev && lbdev->is_available);
}

unsigned int is_lb_available(void)
{
	return __is_lb_available();
}

unsigned int lb_get_available_size(void)
{
	if (!lbdev)
		return 0;

	if (!__is_lb_available())
		return 0;

	return lbdev->gdplc.max_size;
}

static inline int lb_pfn_valid_within(phys_addr_t phys)
{
	/* In the future, need handle invalid pfn */
	return 1;
}

void lb_cmo_sync_cmd_by_event(void)
{
	u32 status;
	int cpu_id;
	ktime_t timeout;
	unsigned long flags;
	cmd snyc;

	preempt_disable();
	local_irq_save(flags);

#ifdef CONFIG_MM_LB_REMAP
	cpu_id = 0;
#else
	cpu_id = smp_processor_id();
#endif
	snyc.value = 0;
	snyc.param.sync.opt = 1;
	snyc.param.sync.cmdt = 1;
	snyc.param.sync.rslt = cpu_id;
	snyc.param.sync.seq = 1;
	writeq(snyc.value, (lbdev->base + CMO_CMD));

	/* ensure cmo cmd complete */
	mb();

	timeout = ktime_add_us(ktime_get(), SC_POLL_TIMEOUT_US);
	while (!(readl(lbdev->base + SLC_ADDR4(CMO_STAT, cpu_id))
		& FINISH_MASK)) {
		if (ktime_compare(ktime_get(), timeout) > 0) {
			lb_print(LB_ERROR, "time out\n");
			break;
		}
		wfe();
	}

	writel(CMO_EVENT_INT_CLR_FLAG,
		(lbdev->base + SLC_ADDR4(CMO_CLEAR, cpu_id)));

	status = readl(lbdev->base + SLC_ADDR4(CMO_STAT, cpu_id));
	WARN_ON(status & FINISH_MASK);

	local_irq_restore(flags);
	preempt_enable();
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
static int f(pte_t *pte, unsigned long addr, void *data)
{
	pte_t ***p = data;

	if (p) {
		*(*p) = pte;
		(*p)++;
	}
	return 0;
}

static struct vm_struct *__alloc_vm_area(size_t size, pte_t **ptes)
{
	struct vm_struct *area;

	area = get_vm_area_caller(size, VM_IOREMAP,
				__builtin_return_address(0));
	if (area == NULL)
		return NULL;

	/*
	 * This ensures that page tables are constructed for this region
	 * of kernel virtual address space and mapped into init_mm.
	 */
	if (apply_to_page_range(&init_mm, (unsigned long)area->addr,
				size, f, ptes ? &ptes : NULL)) {
		free_vm_area(area);
		return NULL;
	}

	return area;
}
#endif

/*lint -e429*/
static int add_gid_vm(struct vm_info *vm, size_t sz)
{
	struct lb_area *vm_ar = NULL;

	vm_ar = kzalloc(sizeof(struct lb_area), GFP_KERNEL);
	if (!vm_ar) {
		lb_print(LB_ERROR, "%s kzalloc failed\n", __func__);
		goto err;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	vm_ar->area = __alloc_vm_area(sz, NULL);
#else
	vm_ar->area = alloc_vm_area(sz, NULL);
#endif
	if (!vm_ar->area) {
		lb_print(LB_ERROR, "%s alloc vm area failed\n", __func__);
		goto free_vm_ar;
	}

	if (gen_pool_add(vm->pool, (uintptr_t)vm_ar->area->addr, sz, -1)) {
		lb_print(LB_ERROR, "%s add gen pool failed\n", __func__);
		goto free_area;
	}

	list_add_tail(&vm_ar->next, &vm->list);

	return 0;

free_area:
	free_vm_area(vm_ar->area);

free_vm_ar:
	kfree(vm_ar);

err:
	return -ENOMEM;
}
/*lint +e429*/

static int map_page_range(unsigned long va, size_t sz,
		pgprot_t prot, struct page **pages)
{
	int err;

	err = map_kernel_range_noflush(va, sz, prot, pages);
	flush_cache_vmap(va, va + sz);

	return err > 0 ? 0 : err;
}

static inline void free_pool(struct vm_info *vm,
		unsigned long va, size_t sz)
{
	gen_pool_free(vm->pool, va, sz);
}

static unsigned long alloc_pool(struct vm_info *vm, size_t sz)
{
	unsigned long va;

	/* try alloc pool, gen_pool_alloc have lock */
	va = gen_pool_alloc(vm->pool, sz);
	if (!va) {
		/* add gid vm need mutex lock */
		mutex_lock(&vm->mutex);

		va = gen_pool_alloc(vm->pool, sz);
		if (!va && !add_gid_vm(vm, SZ_64M))
			va = gen_pool_alloc(vm->pool, sz);

		mutex_unlock(&vm->mutex);
	}

	return va;
}

static void vm_unmap(struct vm_info *vm, const void *va, size_t sz)
{
	if (!vm) {
		lb_print(LB_ERROR, "vm not alloc\n");
		return;
	}

	unmap_kernel_range((uintptr_t)va, sz);
	free_pool(vm, (uintptr_t)va, sz);
}

static void *vm_map(struct vm_info *vm, struct page **pages,
		size_t cnt, pgprot_t prot)
{
	void *va = NULL;
	size_t sz = cnt << PAGE_SHIFT;

	if (!vm) {
		lb_print(LB_ERROR, "%s vm not alloc\n", __func__);
		return NULL;
	}

	va = (void *)alloc_pool(vm, sz);
	if (!va) {
		lb_print(LB_ERROR, "alloc pool failed\n");
		return NULL;
	}

	if (map_page_range((uintptr_t)va, sz, prot, pages)) {
		free_pool(vm, (uintptr_t)va, sz);
		lb_print(LB_ERROR, "map page range failed\n");
		return NULL;
	}

	return va;
}

static void vm_uninit(struct lb_device *lbd)
{
	int i;
	struct vm_info *vm = NULL;
	struct lb_area *vm_ar = NULL;
	struct lb_area *tmp_ar = NULL;

	for (i = 0; i < lbd->gdplc.nr; i++) {
		vm = lbd->gdplc.ptbl[i].vm;
		if (!vm)
			continue;

		list_for_each_entry_safe(vm_ar, tmp_ar, &vm->list, next) {
			list_del(&vm_ar->next);

			if (vm_ar->area)
				free_vm_area(vm_ar->area);

			kfree(vm_ar);
		}

		if (vm->pool)
			gen_pool_destroy(vm->pool);

		mutex_destroy(&vm->mutex);

		kfree(vm);
		lbd->gdplc.ptbl[i].vm = NULL;
	}
}

static int vm_init(struct lb_device *lbd)
{
	int i;
	struct vm_info *vm = NULL;

	for (i = 0; i < lbd->gdplc.nr; i++) {
		if (!(lbd->gdplc.ptbl[i].stat & GID_VM_STATE))
			continue;

		vm = kzalloc(sizeof(struct vm_info), GFP_KERNEL);
		if (!vm) {
			lb_print(LB_ERROR, "kzalloc failed\n");
			goto vm_free;
		}
		lbd->gdplc.ptbl[i].vm = vm;

		mutex_init(&(vm->mutex));
		INIT_LIST_HEAD(&(vm->list));

		vm->pool = gen_pool_create(PAGE_SHIFT, -1);
		if (!vm->pool) {
			lb_print(LB_ERROR, "gen pool create failed\n");
			goto vm_free;
		}

		if (add_gid_vm(vm, SZ_64M)) {
			lb_print(LB_ERROR, "add pool failed\n");
			goto vm_free;
		}
	}

	return 0;

vm_free:
	vm_uninit(lbd);

	return -ENOMEM;
}

static int send_cmd(cmd *cmo)
{
	writeq(cmo->value, (lbdev->base + CMO_CMD));
	return 0;
}

static int build_cmo_cmd(sync_type synct, ops_type ops,
		cmo_type by_x, u32 bitmap, u64 pa, size_t sz, u64 *value)
{
	int ret = 0;
	cmd cmo;

	cmo.value = 0;

	switch (by_x) {
	case CMO_BY_WAY:
		cmo.param.by_way.opt = ops & CMO_OPERATE_MASK;
		cmo.param.by_way.cmdt = by_x;
		cmo.param.by_way.way_bitmap = bitmap & CMO_BITMAP_MASK;
		break;

	case CMO_BY_GID:
		cmo.param.by_gid.opt = ops & CMO_OPERATE_MASK;
		cmo.param.by_gid.cmdt = by_x;
		cmo.param.by_gid.gid_bitmap = bitmap & CMO_BITMAP_MASK;
		break;

	case CMO_BY_64PA:
		cmo.param.by_64pa.opt = ops & CMO_OPERATE_MASK;
		cmo.param.by_64pa.cmdt = by_x;
		cmo.param.by_64pa.pa = pa >> CMO_64B_PA_SHIFT;
		if (!IS_ALIGNED(pa, SZ_64)) {
			lb_print(LB_ERROR, "pa is not align 64\n");
			ret = -EINVAL;
		}
		break;

	case CMO_BY_128PA:
		cmo.param.by_128pa.opt = ops & CMO_OPERATE_MASK;
		cmo.param.by_128pa.cmdt = by_x;
		cmo.param.by_128pa.pa = pa >> CMO_128B_PA_SHIFT;
		if (!IS_ALIGNED(pa, SZ_128)) {
			lb_print(LB_ERROR, "pa is not align 64\n");
			ret = -EINVAL;
		}
		break;
	case CMO_BY_4XKPA:
		cmo.param.by_4xkpa.opt = ops & CMO_OPERATE_MASK;
		cmo.param.by_4xkpa.cmdt = by_x;
		cmo.param.by_4xkpa.spa = (pa >> CMO_4K_PA_SHIFT);
		cmo.param.by_4xkpa.epa = ((pa + sz) >> CMO_4K_PA_SHIFT) - 1;
		if (!IS_ALIGNED(pa, PAGE_SIZE)
			|| !IS_ALIGNED(pa + sz, PAGE_SIZE)
			|| pa >= pa + sz) {
			WARN_ON(1);

			ret = -EINVAL;
		}
		break;

	default:
		lb_print(LB_ERROR, "invalid type %d\n", by_x);
		ret = -EINVAL;
		break;
	}

	*value = cmo.value;

	return ret;
}

int lb_ops_cache(sync_type synct, ops_type ops, cmo_type by_x,
	u32 bitmap, u64 pa, size_t sz)
{
	int ret;
	cmd cmo;

	if ((ops != INV && ops != CLEAN && ops != CI) || by_x > CMO_BY_4XKPA) {
		lb_print(LB_ERROR, "%d %d invalid para\n", ops, by_x);
		return -EINVAL;
	}

	cmo.value = 0;

	ret = build_cmo_cmd(synct, ops, by_x, bitmap, pa, sz, &cmo.value);
	if (ret)
		return ret;

	(void)send_cmd(&cmo);

	return ret;
}

#ifdef CONFIG_MM_LB_L3_EXTENSION
void lb_ci_cache_exclusive(void)
{
	u32 r_quota;
	unsigned long flags = 0;
	struct lb_plc_info *policy = NULL;

	if (!__is_lb_available())
		return;

	if (!lbdev) {
		lb_print(1, "lbdev is null\n");
		return;
	}

	spin_lock_irqsave(&lbdev->gdplc.lock, flags);

	policy = &lbdev->gdplc.ptbl[0];
	if (!(lbdev->power_state & BIT(0)) || policy->enabled != STATUS_ENABLE)
		goto unlock;

	/*
	 * save gid0 quota
	 * set gid0 quota 0
	 */
	r_quota = readl((lbdev->base + GID_ADDR100(GID_QUOTA, 0)));
	writel(0, (lbdev->base + GID_ADDR100(GID_QUOTA, 0)));

	if (lb_ops_cache(EVENT, CI, CMO_BY_GID, BIT(0), 0, 0))
		goto unlock;

	if (cmo_dummy_pa && lb_ops_cache(EVENT, CLEAN, CMO_BY_4XKPA, 0,
		cmo_dummy_pa, PAGE_SIZE))
		goto unlock;

	/* ensure cmo ops complete */
	mb();

	lb_cmo_sync_cmd_by_event();

	/* recover gid0 quota */
	writel(r_quota, (lbdev->base + GID_ADDR100(GID_QUOTA, 0)));

unlock:
	spin_unlock_irqrestore(&lbdev->gdplc.lock, flags);
}
#endif

void lb_invalid_cache(u64 pa, size_t sz)
{
	unsigned long flags = 0;

	if (!lbdev) {
		lb_print(LB_ERROR, "lbdev is null\n");
		return;
	}

	spin_lock_irqsave(&lbdev->gdplc.lock, flags);

	if (!lbdev->power_state)
		goto unlock;

	if (lb_ops_cache(EVENT, INV, CMO_BY_4XKPA, 0, pa, sz))
		goto unlock;

	if (cmo_dummy_pa && lb_ops_cache(EVENT, CLEAN, CMO_BY_4XKPA, 0,
		cmo_dummy_pa, PAGE_SIZE))
		goto unlock;

	/* ensure cmo ops complete */
	mb();

	lb_cmo_sync_cmd_by_event();
unlock:
	spin_unlock_irqrestore(&lbdev->gdplc.lock, flags);
}

void lb_clean_cache(u64 pa, size_t sz)
{
	unsigned long flags = 0;

	if (!lbdev) {
		lb_print(LB_ERROR, "lbdev is null\n");
		return;
	}

	spin_lock_irqsave(&lbdev->gdplc.lock, flags);
	if (!lbdev->power_state)
		goto unlock;

	if (lb_ops_cache(EVENT, CLEAN, CMO_BY_4XKPA, 0, pa, sz))
		goto unlock;

	if (cmo_dummy_pa && lb_ops_cache(EVENT, CLEAN, CMO_BY_4XKPA, 0,
		cmo_dummy_pa, PAGE_SIZE))
		goto unlock;

	/* ensure cmo ops complete */
	mb();

	lb_cmo_sync_cmd_by_event();
unlock:
	spin_unlock_irqrestore(&lbdev->gdplc.lock, flags);
}

/*
 * assert at page to virt
 * assert at kmap
 * it is ok
 */
void __lb_assert_page(struct page *pg)
{
	if (!lbdev || !pg)
		return;

	if (PageLB(pg))
		rdr_syserr_process_for_ap(MODID_AP_S_PANIC_LB, 0ULL, 0ULL);
}

/* assert at page to virt */
void __lb_assert_phys(phys_addr_t phys)
{
	struct page *p = NULL;

	if (!lbdev)
		return;

	/* on owner platform valid check is not ok */
	if (!lb_pfn_valid_within(phys))
		return;

	p = phys_to_page(phys);
	if (p && PageLB(p)) {
		lb_print(LB_ERROR, "page flag 0x%lx\n", p->flags);
		rdr_syserr_process_for_ap(MODID_AP_S_PANIC_LB, 0ULL, 0ULL);
	}
}

static inline unsigned int page_to_gid(struct page *page)
{
	return (page->private & LB_PRIV_GID_MASK);
}

/*
 * assert at set pte
 * assert for page's gid is diff pte gid
 */
void __lb_assert_pte(pte_t pte)
{
}

static unsigned int is_support_bypass(void)
{
	if (!lbdev) {
		lb_print(LB_ERROR, "lbdev is null\n");
		return 0;
	} else {
		return lbdev->gdplc.is_support_bypass;
	}
}

static unsigned int get_max_quota(void)
{
	if (!lbdev) {
		lb_print(LB_ERROR, "lbdev is null\n");
		return 0;
	} else {
		return lbdev->gdplc.max_quota;
	}
}

static bool policy_id_valid(unsigned int policy_id, unsigned int mask)
{
	if (!lbdev) {
		lb_print(LB_ERROR, "lbdev is null\n");
		return false;
	}

	if ((!policy_id && !(mask & GID_BYPASS_STATE)) ||
			policy_id >= (unsigned int)lbdev->gdplc.nr) {
		lb_print(LB_ERROR, "policy_id%u is invalid\n", policy_id);
		return false;
	}

	if (!(lbdev->gdplc.ptbl[policy_id].stat & mask)) {
		lb_print(LB_ERROR, "policy_id%u mask 0x%x 0x%x is invalid\n",
			policy_id, mask, lbdev->gdplc.ptbl[policy_id].stat);
		return false;
	}

	return true;
}

static unsigned int alloc_gid(struct lb_plc_info *policy)
{
	unsigned int i;

	if (policy->stat & GID_STATIC) {
		WARN_ON(!policy->gid);
		return policy->gid;
	}

	for (i = 1; i < MAX_NSEC_GID_NUM; i++) {
		if (!(lbdev->gdplc.gidmap & BIT(i))) {
			policy->gid = i;
			lbdev->gdplc.gidmap |= BIT(i);
			return policy->gid;
		}
	}

	return 0;
}

static void free_gid(struct lb_plc_info *policy)
{
	if (policy->stat & GID_STATIC)
		return;

	lbdev->gdplc.gidmap &= ~BIT(policy->gid);
	policy->gid = 0;
}

static int update_status(struct lb_plc_info *policy, unsigned int quota_value)
{
	int ret = 0;
	gid quota_set;

	if (!(policy->stat & GID_BYPASS_STATE))
		return ret;

	DEFINE_INIT_QUOTA(quota_set, quota_value & QUOTA_L_MASK,
		quota_value & QUOTA_L_MASK);

	if (quota_value && policy->enabled != STATUS_ENABLE) {
		ret = gid_bypass_control(GID_ENABLE, policy->gid,
			quota_set.value, policy->plc);
		policy->enabled = STATUS_ENABLE;
	}

	if (!quota_value && policy->enabled != STATUS_BYPASS) {
		ret = gid_bypass_control(GID_BYPASS, policy->gid, 0, 0);
		policy->enabled = STATUS_BYPASS;
	}

	if (ret)
		lb_print(LB_ERROR, "%s is failed 0x%x 0x%x\n", __func__,
			policy->gid, quota_value);

	return ret;
}

static void set_quota(struct lb_plc_info *policy, unsigned int quota_value)
{
	gid quota_set;
	gid allc_set;
	gid allc_efuse;

	if (is_fpga)
		DEFINE_INIT_ALLOC(allc_set, GID_WAY_FPGA, GID_WAY_FPGA);
	else
		DEFINE_INIT_ALLOC(allc_set, GID_WAY_EN, GID_WAY_SRCH);

	DEFINE_INIT_ALLOC(allc_efuse, lbdev->way_efuse, lbdev->way_efuse);
	DEFINE_INIT_QUOTA(quota_set, quota_value & QUOTA_L_MASK,
		quota_value & QUOTA_L_MASK);

	writel(allc_set.value & allc_efuse.value, (lbdev->base +
			GID_ADDR100(GID_WAY_ALLC, policy->gid)));
	writel(quota_set.value, (lbdev->base +
			GID_ADDR100(GID_QUOTA, policy->gid)));
}

static int set_quota_and_update(struct lb_plc_info *policy,
		unsigned int quota)
{
	int ret;
	unsigned long flags = 0;

	spin_lock_irqsave(&policy->lock, flags);
	set_quota(policy, quota);
	ret = update_status(policy, quota);
	spin_unlock_irqrestore(&policy->lock, flags);

	return ret;
}

static int update_quota(void)
{
	int i;
	int ret = 0;
	int quota_set = 0;
	int last_prio = 0;
	unsigned int quota;
	unsigned int remaining_quota = get_max_quota();
	struct lb_plc_info *policy = NULL;

	for (i = 0; i < lbdev->gdplc.job_cnt; i++) {
		policy = &lbdev->gdplc.ptbl[lbdev->gdplc.job_queue[i][0]];
		quota = (policy->quota & QUOTA_H_MASK) >> QUOTA_L_SHIFT;

		/* need set quota = 0 when job be deleted */
		if (lbdev->gdplc.job_queue[i][1] == LB_CACHE_LOW_PRIO) {
			ret |= set_quota_and_update(policy, 0);
			continue;
		}

		if (last_prio == policy->prio && quota_set) {
			ret |= set_quota_and_update(policy, quota);
			remaining_quota = remaining_quota > quota ?
				remaining_quota - quota : 0;
			continue;
		}

		if (remaining_quota < quota)
			quota_set = remaining_quota;
		else
			quota_set = quota;

		remaining_quota -= quota_set;
		ret |= set_quota_and_update(policy, quota_set);
		last_prio = policy->prio;
	}

	return ret;
}

static int cmp(const void *a, const void *b)
{
	const unsigned int *ap = (unsigned int *)a;
	const unsigned int *bp = (unsigned int *)b;

	return ap[1] < bp[1] ? -1 : ap[1] > bp[1] ? 1 : 0;
}

static int update_job(void)
{
	sort(lbdev->gdplc.job_queue, lbdev->gdplc.job_cnt,
		sizeof(lbdev->gdplc.job_queue[0]), cmp, NULL);

	return update_quota();
}

static int add_job(unsigned int pid, unsigned int prio)
{
	int i;

	if (pid && !is_support_bypass())
		return 0;

	if (lbdev->gdplc.job_cnt >= PID_MAX) {
		lb_print(LB_ERROR, "%s invalid add job 0x%x 0x%x\n",
			__func__, pid, prio);
		return -EPERM;
	}

	for (i = 0; i < lbdev->gdplc.job_cnt; i++) {
		if (lbdev->gdplc.job_queue[i][0] == pid)
			return 0;
	}

	lbdev->gdplc.job_queue[lbdev->gdplc.job_cnt][0] = pid;
	lbdev->gdplc.job_queue[lbdev->gdplc.job_cnt][1] = prio;

	lbdev->gdplc.job_cnt++;

	return update_job();
}

static int del_job(unsigned int pid)
{
	int i, index;
	int ret;

	if (pid && !is_support_bypass())
		return 0;

	index = lbdev->gdplc.job_cnt;
	for (i = 0; i < lbdev->gdplc.job_cnt; i++)
		if (lbdev->gdplc.job_queue[i][0] == pid)
			index = i;

	if (index == lbdev->gdplc.job_cnt)
		return 0;

	lbdev->gdplc.job_queue[index][0] = pid;
	lbdev->gdplc.job_queue[index][1] = LB_CACHE_LOW_PRIO;

	ret = update_job();

	lbdev->gdplc.job_cnt--;

	return ret;
}

unsigned int lb_get_master_policy(unsigned int pid, unsigned int type)
{
	unsigned int ret = 0;
	unsigned long flags = 0;
	struct lb_plc_info *policy = NULL;

	if (!is_support_bypass())
		return 0;

	if (!policy_id_valid(pid, GID_NON_SEC | GID_BYPASS_STATE)) {
		lb_print(LB_ERROR, "0x%x: pid invalid\n", pid);
		return 0;
	}

	spin_lock_irqsave(&lbdev->gdplc.lock, flags);
	policy = &lbdev->gdplc.ptbl[pid];

	switch (type) {
	case MASTER_PRIO:
		ret = policy->prio;
		break;

	case MASTER_QUOTA:
		ret = policy->quota;
		break;

	case MASTER_NUM:
		ret = lbdev->gdplc.job_cnt;
		break;

	default:
		ret = 0;
		break;
	}

	spin_unlock_irqrestore(&lbdev->gdplc.lock, flags);

	return ret;
}

int lb_set_master_policy(unsigned int pid, unsigned int quota,
		unsigned int prio)
{
	int i;
	unsigned long flags = 0;
	struct lb_plc_info *policy = NULL;

	if (!is_support_bypass())
		return 0;

	if (!IS_ALIGNED(quota, SZ_2M))
		return -EINVAL;

	if (prio < LB_BUF_HIGH_PRIO || prio > LB_CACHE_LOW_PRIO)
		return -EINVAL;

	if (!policy_id_valid(pid, GID_NON_SEC | GID_BYPASS_STATE)) {
		lb_print(LB_ERROR, "0x%x: pid invalid\n", pid);
		return -EINVAL;
	}

	spin_lock_irqsave(&lbdev->gdplc.lock, flags);
	policy = &lbdev->gdplc.ptbl[pid];
	policy->prio = prio;

	for (i = 0; i < lbdev->gdplc.job_cnt; i++) {
		if (lbdev->gdplc.job_queue[i][0] == pid) {
			lbdev->gdplc.job_queue[i][1] = prio;
			update_job();
			break;
		}
	}

	spin_unlock_irqrestore(&lbdev->gdplc.lock, flags);

	return 0;
}

int lb_reset_master_policy(unsigned int pid)
{
	int i;
	unsigned long flags = 0;
	struct lb_plc_info *policy = NULL;

	if (!is_support_bypass())
		return 0;

	if (!policy_id_valid(pid, GID_NON_SEC | GID_BYPASS_STATE)) {
		lb_print(LB_ERROR, "0x%x: pid invalid\n", pid);
		return -EINVAL;
	}

	spin_lock_irqsave(&lbdev->gdplc.lock, flags);
	policy = &lbdev->gdplc.ptbl[pid];
	policy->prio = policy->r_prio;
	policy->quota = policy->r_quota;

	for (i = 0; i < lbdev->gdplc.job_cnt; i++) {
		if (lbdev->gdplc.job_queue[i][0] == pid) {
			lbdev->gdplc.job_queue[i][1] = policy->prio;
			update_job();
			break;
		}
	}

	spin_unlock_irqrestore(&lbdev->gdplc.lock, flags);

	return 0;
}

int lb_up_policy_prio(unsigned int pid)
{
	if (!is_support_bypass())
		return 0;

	if (pid == PID_ISPNN)
		return lb_set_master_policy(pid, SZ_8M, LB_BUF_HIGH_PRIO);

	if (pid == PID_NPU) {
		unsigned long flags = 0;
		unsigned int quota_value = get_max_quota();
		struct lb_plc_info *policy = &lbdev->gdplc.ptbl[pid];

		spin_lock_irqsave(&policy->lock, flags);
		if (policy->enabled != STATUS_ENABLE) {
			spin_unlock_irqrestore(&policy->lock, flags);
			return 0;
		}

		set_quota(policy, quota_value);
		spin_unlock_irqrestore(&policy->lock, flags);

		return 0;
	}

	return 0;
}

int lb_down_policy_prio(unsigned int pid)
{
	if (!is_support_bypass())
		return 0;

	if (pid == PID_ISPNN)
		return lb_set_master_policy(pid, SZ_8M, LB_CACHE_LOW_PRIO);

	if (pid == PID_NPU) {
		unsigned long flags = 0;
		struct lb_plc_info *policy = &lbdev->gdplc.ptbl[pid];

		spin_lock_irqsave(&policy->lock, flags);
		if (policy->enabled != STATUS_ENABLE) {
			spin_unlock_irqrestore(&policy->lock, flags);
			return 0;
		}

		set_quota(policy, 0);
		spin_unlock_irqrestore(&policy->lock, flags);

		return 0;
	}

	return 0;
}

static int gid_enable(struct lb_plc_info *policy)
{
	int ret;

	if (policy->pid && !is_support_bypass())
		return 0;

	if (policy->pid && !policy->page_count) {
		lb_print(LB_ERROR, "%s gid:0x%x page count invalid\n",
			__func__, policy->pid);
		return -EPERM;
	}

	/* lb need powerup when lb is powered down */
	if (!lbdev->power_state && power_control(POWERUP)) {
		lb_print(LB_ERROR, "%s powerup fail\n", __func__);
		return -EINVAL;
	}

	ret = add_job(policy->pid, policy->prio);
	if (ret) {
		lb_print(LB_ERROR, "%s 0x%x, %d fail\n",
			__func__, policy->pid, ret);

		return ret;
	}

	lbdev->power_state |= BIT(policy->gid);

	return ret;
}

static int gid_bypass(struct lb_plc_info *policy)
{
	int ret;

	if (policy->pid && !is_support_bypass())
		return 0;

	ret = del_job(policy->pid);
	if (ret) {
		lb_print(LB_ERROR, "%s pid: 0x%x, ret: %d fail\n",
			__func__, policy->pid, ret);
		return -EINVAL;
	}

	/* lb need powerdown when only gid is enable */
	if (lbdev->power_state && !(lbdev->power_state & ~BIT(policy->gid)))
		ret = power_control(POWERDOWN);

	lbdev->power_state &= ~BIT(policy->gid);

	return ret;
}

static int set_gid(struct lb_plc_info *policy, unsigned int way_alloc,
		unsigned int quota_set, unsigned int plc_set)
{
	int ret = 0;
	gid allc_efuse;

	/* lb need powerup when lb is powered down */
	if (!lbdev->power_state)
		if (power_control(POWERUP))
			return -EINVAL;

	if (policy->gid == GID_TINY) {
		sec_gid_set(SEC_GID_REQUEST, policy->gid);
	} else {
		DEFINE_INIT_ALLOC(allc_efuse, lbdev->way_efuse, lbdev->way_efuse);

		writel(way_alloc & allc_efuse.value, (lbdev->base +
				GID_ADDR100(GID_WAY_ALLC, policy->gid)));
		writel(quota_set, (lbdev->base + GID_ADDR100(GID_QUOTA, policy->gid)));
		writel(plc_set, (lbdev->base + GID_ADDR100(GID_ALLC_PLC, policy->gid)));
	}

	/* ensure set gid cfg complete */
	mb();

	policy->enabled = STATUS_ENABLE;
	lbdev->power_state |= BIT(policy->gid);

	if ((policy->stat & GID_NON_SEC)) {
		ret = add_job(policy->pid, policy->prio);
		if ((policy->stat & GID_BYPASS_STATE) && policy->pid != PID_NPU)
			gid_bypass(policy);
	}

	return ret;
}

static void clr_bypass_status(struct lb_plc_info *policy)
{
	u32 gid_byp;

	gid_byp = readl(lbdev->base + GLB_ROUTE_PLC_S);
	writel(gid_byp & (~BIT(policy->gid)), (lbdev->base + GLB_ROUTE_PLC_S));
	writel(0, (lbdev->base + GID_ADDR100(GID_ALLC_PLC, policy->gid)));
}

static int reset_gid(struct lb_plc_info *policy)
{
	int ret = 0;

	lb_print(LB_ERROR, "%s: gid = %u\n", __func__, policy->gid);

	/* doubel free same gid */
	if (!(lbdev->power_state & BIT(policy->gid))) {
		if (is_support_bypass() && (policy->stat & GID_BYPASS_STATE)) {
			clr_bypass_status(policy);
			policy->enabled = STATUS_DISABLE;
		} else {
			lb_print(LB_ERROR, "double free gid %u %u\n",
				lbdev->power_state, policy->gid);
		}

		return 0;
	}

	if ((policy->stat & GID_NON_SEC))
		ret |= del_job(policy->pid);

	if (policy->gid == GID_TINY) {
		sec_gid_set(SEC_GID_RELEASE, policy->gid);
	} else {
		writel(0, (lbdev->base + GID_ADDR100(GID_QUOTA, policy->gid)));
		writel(0, (lbdev->base + GID_ADDR100(GID_WAY_ALLC, policy->gid)));
		writel(0, (lbdev->base + GID_ADDR100(GID_ALLC_PLC, policy->gid)));
		if (is_support_bypass() && (policy->stat & GID_BYPASS_STATE))
			clr_bypass_status(policy);
	}

	/* ensure set gid cfg complete */
	mb();
	policy->enabled = STATUS_DISABLE;

	lbdev->power_state &= ~BIT(policy->gid);

	/* lb need powerdown when all gid is disable */
	if (!lbdev->power_state)
		ret |= power_control(POWERDOWN);

	return ret;
}

static int set_page(struct lb_plc_info *policy,
		struct page *page_alloc, size_t count)
{
	void *vaddr = NULL;
	struct page *pg = page_alloc;
	struct page **pages = NULL;
	struct page **tmp = NULL;
	int i;

#ifdef CONFIG_MM_LB_L3_EXTENSION
	change_secpage_range(page_to_phys(page_alloc),
		(uintptr_t)page_address(page_alloc),
		count << PAGE_SHIFT, __pgprot(PROT_DEVICE_nGnRE));
	flush_tlb_kernel_range((uintptr_t)page_to_virt(page_alloc),
		(uintptr_t)page_to_virt(pg) + (count << PAGE_SHIFT));
#endif

	/*
	 * invalid acpu cache by normal va
	 * it is necessary by normal va
	 * cache |--va--| |--va with gid--|
	 * lb             |-------lb------|
	 * ddr   |------|
	 */
	__dma_unmap_area(page_to_virt(pg), count << PAGE_SHIFT,
			DMA_FROM_DEVICE);

	pages = kcalloc(count, sizeof(struct page *), GFP_KERNEL);
	if (!pages) {
		lb_print(LB_ERROR, "zalloc page\n");
		goto change_mapping;
	}

	tmp = pages;
	for (i = 0; i < (int)count; i++)
		*(tmp++) = pg++;

	vaddr = vm_map(policy->vm, pages, count,
			pgprot_lb(PAGE_KERNEL, policy->gid));
	if (!vaddr) {
		lb_print(LB_ERROR, "%s:vm map failed\n", __func__);
		goto free_page;
	}

	kfree(pages);

	/* set the page privite */
	pg = page_alloc;

	for (i = 0; i < (int)count; i++) {
		set_page_private(pg, (uintptr_t)vaddr | policy->gid);
		SetPagePrivate(pg);
		SetPageLB(pg);
		pg++;
		vaddr += PAGE_SIZE;
	}

	return 0;

free_page:
	kfree(pages);

change_mapping:
#ifdef CONFIG_MM_LB_L3_EXTENSION
	change_secpage_range(page_to_phys(page_alloc),
		(uintptr_t)page_address(page_alloc),
		count << PAGE_SHIFT, PAGE_KERNEL);
	flush_tlb_kernel_range((uintptr_t)page_to_virt(page_alloc),
		(uintptr_t)page_to_virt(page_alloc) + (count << PAGE_SHIFT));
#endif

	return -ENOMEM;
}

static void reset_page(struct lb_plc_info *policy,
		struct page *page, size_t count)
{
	int i;
	void *vaddr = NULL;
	struct page *pg = page;
	phys_addr_t phys = page_to_phys(pg);
	unsigned long size = count << PAGE_SHIFT;

	WARN_ON(!PageLB(pg));
	WARN(policy->gid != lb_page_to_gid(pg),
		"pgid %u ggid %u\n",
		policy->gid, lb_page_to_gid(pg));

	/*
	 * invalid acpu cache by lb va
	 * it is necessary by lb va
	 * cache |--va--| |--va with gid--|
	 * lb             |-------lb------|
	 * ddr   |------|
	 */
	vaddr = lb_page_to_virt(page);
	__dma_unmap_area(vaddr, size, DMA_FROM_DEVICE);

	vm_unmap(policy->vm, vaddr, size);

	for (i = 0; i < (int)count; i++) {
		set_page_private(pg, 0);
		ClearPagePrivate(pg);
		ClearPageLB(pg);
		pg++;
	}

	/* invalid system cache */
	lb_invalid_cache(phys, size);

#ifdef CONFIG_MM_LB_L3_EXTENSION
	change_secpage_range(page_to_phys(page),
		(uintptr_t)page_address(page),
		count << PAGE_SHIFT, PAGE_KERNEL);
	flush_tlb_kernel_range((uintptr_t)page_to_virt(page),
		(uintptr_t)page_to_virt(page) + (count << PAGE_SHIFT));
#endif
}

unsigned int search_quota(struct lb_plc_info *policy, unsigned int quota)
{
	return 0;
}

static unsigned int alloc_quota(struct lb_plc_info *policy, unsigned int quota)
{
	return policy->quota;
}

static void free_quota(struct lb_plc_info *policy, unsigned int quota)
{
	/*
	 * Current quota static alloc
	 * In the future, need dynamic free quota
	 */
}

void *lb_page_to_virt(struct page *page)
{
	unsigned long virt;

	if (!__is_lb_available() || !page)
		return NULL;

	WARN(!PageLB(page), "fail pagelb");
	WARN(!PagePrivate(page), "fail pageprivate");
	virt = page->private & (~LB_PRIV_GID_MASK);

	return (void *)virt;
}
EXPORT_SYMBOL(lb_page_to_virt);

u64 lb_pid_to_gidphys(u32 pid)
{
	if (!__is_lb_available())
		return 0;

	if (!policy_id_valid(pid, GID_NON_SEC)) {
		lb_print(LB_ERROR, "0x%x: pid invalid\n", pid);
		return 0;
	}

	return PTE_LB(lbdev->gdplc.ptbl[pid].gid);
}

unsigned int lb_page_to_gid(struct page *page)
{
	unsigned int gid_idx;

	if (!__is_lb_available() || !page || !PageLB(page))
		return 0;

	WARN(!PageLB(page), "fail pagelb");
	WARN(!PagePrivate(page), "fail pageprivate");
	gid_idx = page->private & LB_PRIV_GID_MASK;

	return gid_idx;
}

/*
 * this API is called by HIFI, Tiny, GPU
 * size is not used
 */
int lb_request_quota(unsigned int pid)
{
	int ret = 0;
	unsigned long flags = 0;
	struct lb_plc_info *policy = NULL;

	lb_print(LB_INFO, "into%s\n", __func__);

	if (!__is_lb_available())
		return 0;

	if (!policy_id_valid(pid, GID_NON_SEC | GID_SEC | GID_BYPASS_STATE)) {
		lb_print(LB_ERROR, "0x%x: pid invalid\n", pid);
		ret = -ENOMEM;
		goto err;
	}

	spin_lock_irqsave(&lbdev->gdplc.lock, flags);
	policy = &lbdev->gdplc.ptbl[pid];
	if (likely(policy->stat & GID_SEC))
		ret = set_gid(policy, 0, 0, BIT(GID_EN_SHIFT));
	else
		ret = gid_enable(policy);

	spin_unlock_irqrestore(&lbdev->gdplc.lock, flags);

err:
	lb_print(LB_INFO, "out%s\n", __func__);
	return ret;
}

int lb_release_quota(unsigned int pid)
{
	int ret = 0;
	unsigned long flags = 0;
	struct lb_plc_info *policy = NULL;

	lb_print(LB_INFO, "into%s\n", __func__);

	if (!__is_lb_available())
		return 0;

	if (!policy_id_valid(pid, GID_NON_SEC | GID_SEC | GID_BYPASS_STATE)) {
		lb_print(LB_ERROR, "%u: pid invalid\n", pid);
		ret = -EINVAL;
		goto err;
	}

	spin_lock_irqsave(&lbdev->gdplc.lock, flags);
	policy = &lbdev->gdplc.ptbl[pid];
	if (likely(policy->stat & GID_SEC))
		ret = reset_gid(policy);
	else
		ret = gid_bypass(policy);

	spin_unlock_irqrestore(&lbdev->gdplc.lock, flags);

err:
	return ret;
}

/* when call it set pte for cpu/iommu/gpu */
unsigned long lb_pte_attr(phys_addr_t phy_addr)
{
	unsigned long gid_idx;
	struct page *pg = NULL;

	if (!__is_lb_available())
		return 0;

	/*
	 * pfn illegal judgment affects efficiency
	 * the current only gpu use lb_pte_attr
	 * gpu input a valid addree
	 */
	pg = phys_to_page(phy_addr);
	if (!PageLB(pg))
		return 0;

	gid_idx = lb_page_to_gid(pg);

	return pgprot_val(PAGE_LB_CACHEABLE(gid_idx));
}

/*
 * in order to avoid GPU mapping framebuf with dss'gid,
 * only is called by GPU r18,
 * only is a circumvention plan.
 */
unsigned long lb_pte_attr_gpu(phys_addr_t phy_addr)
{
	unsigned long gid_idx;
	struct page *pg = NULL;

	if (!__is_lb_available())
		return 0;

	/*
	 * pfn illegal judgment affects efficiency
	 * the current only gpu use lb_pte_attr
	 * gpu input a valid addree
	 */
	pg = phys_to_page(phy_addr);
	if (!PageLB(pg))
		return 0;

	gid_idx = lb_page_to_gid(pg);
	if (gid_idx == lbdev->gdplc.ptbl[PID_IDISPLAY].gid) {
		WARN_ON_ONCE(1);
		return 0;
	}

	return pgprot_val(PAGE_LB_CACHEABLE(gid_idx));
}

/* when call it @ dma XXX function */
int lb_prot_build(struct page *pages, pgprot_t *pageprot)
{
	unsigned long gid_idx;

	if (!__is_lb_available() || !pages || !pageprot || !PageLB(pages))
		return 0;

	gid_idx = lb_page_to_gid(pages);
	*pageprot = pgprot_lb(*pageprot, gid_idx);

	return 0;
}

/*
 * when call it @ ion
 * @ map to cpu user/kernel space
 */
int lb_pid_prot_build(unsigned int pid, pgprot_t *pageprot)
{
	struct lb_plc_info *policy = NULL;

	if (!__is_lb_available())
		return 0;

	if (!pageprot)
		return -EINVAL;

	if (!policy_id_valid(pid, GID_NON_SEC)) {
		lb_print(LB_ERROR, "%s:policy_id invalid\n", __func__);
		return -EINVAL;
	}

	policy = &lbdev->gdplc.ptbl[pid];
	*pageprot = pgprot_lb(*pageprot, policy->gid);

	return 0;
}

int lb_pages_attach(unsigned int pid, struct page *pg, size_t count)
{
	int ret;
	unsigned long flags = 0;
	gid quota_set;
	gid allc_set;
	unsigned int quota = 0;
	struct lb_plc_info *policy = NULL;

	lb_print(LB_INFO, "into%s\n", __func__);

	if (!__is_lb_available())
		return 0;

	if (!pg) {
		lb_print(LB_ERROR, "%s:pages is  invalid\n", __func__);
		goto err;
	}

	if (!policy_id_valid(pid, GID_PAGES_REMAP)) {
		lb_print(LB_ERROR, "%s:policy_id invalid\n", __func__);
		goto err;
	}

	/* protect gid map and quota */
	spin_lock_irqsave(&lbdev->gdplc.lock, flags);
	policy = &lbdev->gdplc.ptbl[pid];
	if (!policy->page_count) {
		policy->gid = alloc_gid(policy);
		quota = alloc_quota(policy,
			count << (PAGE_SHIFT - LB_LINE_SHIFT));

		DEFINE_INIT_QUOTA(quota_set, (quota & QUOTA_L_MASK),
			(quota & QUOTA_H_MASK) >> QUOTA_L_SHIFT);
		if (is_fpga)
			DEFINE_INIT_ALLOC(allc_set, GID_WAY_FPGA, GID_WAY_FPGA);
		else
			DEFINE_INIT_ALLOC(allc_set, GID_WAY_EN, GID_WAY_SRCH);

		if (set_gid(policy, allc_set.value, quota_set.value,
			policy->plc))
			goto err_set_gid;
	}
	policy->page_count += count;
	spin_unlock_irqrestore(&lbdev->gdplc.lock, flags);

	/* set pages */
	ret = set_page(policy, pg, count);
	if (ret) {
		lb_print(LB_ERROR, "%s: set page failed\n", __func__);
		goto err_set_page;
	}

	lb_print(LB_INFO, "out%s\n", __func__);

	return 0;

err_set_page:
	spin_lock_irqsave(&lbdev->gdplc.lock, flags);

	if (WARN_ON(policy->page_count < count))
		policy->page_count = 0;
	else
		policy->page_count -= count;

	if (!policy->page_count) {
		reset_gid(policy);
		free_quota(policy, quota);
		free_gid(policy);
	}

	spin_unlock_irqrestore(&lbdev->gdplc.lock, flags);

err:
	return -EINVAL;

err_set_gid:
	free_quota(policy, quota);
	free_gid(policy);
	spin_unlock_irqrestore(&lbdev->gdplc.lock, flags);
	return -EINVAL;
}

int lb_pages_detach(unsigned int pid, struct page *pages, size_t count)
{
	struct lb_plc_info *policy = NULL;
	unsigned long flags = 0;

	lb_print(LB_INFO, "into%s\n", __func__);

	if (!__is_lb_available())
		return 0;

	if (!pages) {
		lb_print(LB_ERROR, "%s:pages is  invalid\n", __func__);
		return -EINVAL;
	}

	if (!policy_id_valid(pid, GID_PAGES_REMAP)) {
		lb_print(LB_ERROR, "%s:policy_id invalid\n", __func__);
		return -EINVAL;
	}

	policy = &lbdev->gdplc.ptbl[pid];
	reset_page(policy, pages, count);

	spin_lock_irqsave(&lbdev->gdplc.lock, flags);

	if (WARN_ON(policy->page_count < count))
		policy->page_count = 0;
	else
		policy->page_count -= count;

	if (!policy->page_count) {
		reset_gid(policy);
		free_quota(policy, 0);
		free_gid(policy);
	}

	spin_unlock_irqrestore(&lbdev->gdplc.lock, flags);

	lb_print(LB_INFO, "out%s\n", __func__);

	return 0;
}

int lb_sg_attach(unsigned int pid, struct scatterlist *sgl, unsigned int nents)
{
	int i, j;
	struct scatterlist *sg = NULL;

	lb_print(LB_INFO, "into %s pid %u\n", __func__, pid);

	/* for gpu pid is normal call path */
	if (!__is_lb_available() || !pid)
		return 0;

	if (!sgl)
		return -EINVAL;

	for_each_sg(sgl, sg, (int)nents, i) {
		if (!sg || lb_pages_attach(pid, phys_to_page(sg_phys(sg)),
			sg->length >> PAGE_SHIFT))
			goto err;
	}

	lb_print(LB_INFO, "out %s\n", __func__);

	return 0;

err:
	for_each_sg(sgl, sg, i, j) {
		if (!sg || lb_pages_detach(pid, phys_to_page(sg_phys(sg)),
			sg->length >> PAGE_SHIFT))
			lb_print(LB_ERROR, "%s page detach failed\n", __func__);
	}

	return -EINVAL;
}

int lb_sg_detach(unsigned int pid, struct scatterlist *sgl, unsigned int nents)
{
	int i;
	struct scatterlist *sg = NULL;

	lb_print(LB_INFO, "into %s\n", __func__);

	/* for gpu pid is normal call path */
	if (!__is_lb_available() || !pid)
		return 0;

	if (!sgl && sg_nents(sgl) != nents)
		return -EINVAL;

	for_each_sg(sgl, sg, (int)nents, i)
		if (lb_pages_detach(pid, phys_to_page(sg_phys(sg)),
			sg->length >> PAGE_SHIFT))
			lb_print(LB_ERROR, "%s page detach failed\n", __func__);

	lb_print(LB_INFO, "out %s\n", __func__);

	return 0;
}

struct page *lb_alloc_pages(unsigned int pid, gfp_t gfp_mask,
		unsigned int order)
{
	struct page *lb_page = NULL;

	/*
	 * alloc pages
	 * pid = 0 for GPU
	 */
	lb_page = alloc_pages(gfp_mask, order);
	if (!lb_page) {
		lb_print(LB_ERROR, "%u:alloc pages failed\n", pid);
		goto err;
	}

	if (!__is_lb_available() || !pid)
		return lb_page;

	if (lb_pages_attach(pid, lb_page, 1ULL << order)) {
		lb_print(LB_ERROR, "%u:lb pages attach failed\n", pid);
		goto err;
	}

	return lb_page;

err:
	if (lb_page)
		__free_pages(lb_page, order);
	return NULL;
}

int lb_free_pages(unsigned int pid, struct page *pages, unsigned int order)
{
	if (!__is_lb_available() || !pid)
		goto succ;

	if (lb_pages_detach(pid, pages, 1UL << order)) {
		lb_print(LB_ERROR, "%s:lb pages detach failed\n", __func__);
		goto err_nofree;
	}

succ:
	if (pages)
		__free_pages(pages, order);
	return 0;

err_nofree:
	return -EINVAL;
}

#ifdef CONFIG_MM_LB_GEMINI
int lb_gid_enable(unsigned int pid)
{
	if (pid && !is_support_bypass())
		return 0;

	return lb_request_quota(pid);
}

int lb_gid_bypass(unsigned int pid)
{
	if (pid && !is_support_bypass())
		return 0;

	return lb_release_quota(pid);
}

int perf_ctrl_lb_set_policy(void __user *uarg)
{
	int res;
	struct lb_policy_config lb_config;

	if (uarg == NULL)
		return -EINVAL;

	if (!__is_lb_available() || !is_support_bypass())
		return 0;

	if (copy_from_user(&lb_config, uarg, sizeof(struct lb_policy_config))) {
		pr_err("set master prio copy_to_user fail.\n");
		return -EFAULT;
	}

	res = lb_set_master_policy(lb_config.pid, lb_config.quota, lb_config.prio);

	return res;
}

int perf_ctrl_lb_reset_policy(void __user *uarg)
{
	int res;
	struct lb_policy_config lb_config;

	if (uarg == NULL)
		return -EINVAL;

	if (!__is_lb_available() || !is_support_bypass())
		return 0;

	if (copy_from_user(&lb_config, uarg, sizeof(struct lb_policy_config))) {
		pr_err("reset master prio copy_to_user fail.\n");
		return -EFAULT;
	}

	res = lb_reset_master_policy(lb_config.pid);

	return res;
}

int perf_ctrl_lb_lite_info(void __user *uarg)
{
	struct lb_policy_config lb_config = {};

	if (uarg == NULL)
		return -EINVAL;

	lb_config.available = __is_lb_available();

	if (copy_to_user(uarg, &lb_config, sizeof(struct lb_policy_config))) {
		pr_err("get lite info copy_to_user fail.\n");
		return -EFAULT;
	}

	return 0;
}
#endif

static int gid_to_pid(unsigned int id)
{
	int i;

	for (i = 0; i < lbdev->gdplc.nr; i++) {
		if (lbdev->gdplc.ptbl[i].gid == id)
			return i;
	}

	return 0;
}

#ifdef CONFIG_MM_LB_REMAP
static unsigned int get_flow(unsigned int id, unsigned int dir)
{
	if (dir)
		return readl(lbdev->base + SLC_ADDR2000(GLB_FLOW_STAT_IN, id));
	else
		return readl(lbdev->base + SLC_ADDR2000(GLB_FLOW_STAT_OUT, id));
}

static void start_flow(unsigned int gid, unsigned int id)
{
	unsigned int reg;

	reg = readl(lbdev->base + SLC_ADDR2000(GLB_FLOW_STAT, id));
	if (reg & FLOW_ENABLE_MASK)
		return;

	reg = FLOW_START_MASK | BIT(FLOW_GID_SHIFT + gid);
	writel(reg, (lbdev->base + SLC_ADDR2000(GLB_FLOW_STAT, id)));
}

static void end_flow(unsigned int id)
{
	writel(FLOW_END_MASK, (lbdev->base + SLC_ADDR2000(GLB_FLOW_STAT, id)));
}

int get_flow_stat(struct lb_policy_flow *flow)
{
	unsigned int id;
	unsigned int pid;
	struct lb_plc_info *policy = NULL;

	if (!__is_lb_available())
		return 0;

	if (!flow) {
		lb_print(LB_ERROR, "policy flow is null\n");
		return -EINVAL;
	}

	pid = flow->pid;
	if (!policy_id_valid(pid, GID_NON_SEC | GID_BYPASS_STATE)) {
		lb_print(LB_ERROR, "0x%x: pid invalid\n", pid);
		return -EINVAL;
	}

	policy = &lbdev->gdplc.ptbl[pid];
	if (!policy->flowid || policy->flowid > FLOW_MAX_ID) {
		lb_print(LB_ERROR, "0x%x: pid not support flow\n", pid);
		return -EINVAL;
	}

	id = FLOWID_TO_SLC(policy->flowid);

	end_flow(id);
	flow->in = get_flow(id, 1);
	flow->out = get_flow(id, 0);
	start_flow(policy->gid, id);

	return 0;
}
#endif

static void parse_dfx_err_info(uint32_t slice_idx, uint32_t err_inf1,
	uint32_t err_inf2)
{
	unsigned int master_id;
	unsigned int id;
	unsigned int pid;

	lb_print(LB_ERROR, "SLC_DFX_ERR_1 slice%u:0x%x\n", slice_idx, err_inf1);
	lb_print(LB_ERROR, "SLC_DFX_ERR_2 slice%u:0x%x\n", slice_idx, err_inf2);

	lb_print(LB_ERROR, "dfx_fail_cmd_pa:0x%x\n",
		err_inf1 << DFX_CMD_PA_SHIFT);

	if (err_inf2 & DFX_CMD_TYPE_MASK)
		lb_print(LB_ERROR, "dfx_fail_cmd_type is write\n");
	else
		lb_print(LB_ERROR, "dfx_fail_cmd_type is read\n");

	master_id = (err_inf2 & DFX_CMD_MID_MASK) >> DFX_CMD_MID_SHIFT;
	lb_print(LB_ERROR, "Master id is 0x%x\n", master_id);

	id = err_inf2 & 0xf;
	lb_print(LB_ERROR, "dfx_fail_cmd_gid:%u\n", id);

	pid = gid_to_pid(id);
	if (!pid)
		return;

	if (lbdev->gdplc.ptbl[pid].stat & GID_NON_SEC) {
		lb_print(LB_ERROR, "GID_WAY_ALLC-gid%u:0x%x\n", id,
			readl(lbdev->base + GID_ADDR100(GID_WAY_ALLC, id)));
		lb_print(LB_ERROR, "GID_QUOTA-gid%u:0x%x\n", id,
			readl(lbdev->base + GID_ADDR100(GID_QUOTA, id)));
		lb_print(LB_ERROR, "GID_ALLC_PLC-gid%u:0x%x\n", id,
			readl(lbdev->base + GID_ADDR100(GID_ALLC_PLC, id)));
	}
}

static void lb_ecc_info(uint32_t info)
{
	struct dsm_client *ai_client = lbdev->dsm.ai_client;

	if (!ai_client) {
		lb_print(LB_ERROR, "ai_client is null\n");
		return;
	}

	if ((info & DFX_ECCERR_MULTI_MASK) &&
		(mutil_bit < MUTIL_BIT_EVENT_MAX)) {
#ifdef CONFIG_MM_LB_ENG_DEBUG
		rdr_syserr_process_for_ap(MODID_AP_S_PANIC_LB, 0ULL, 0ULL);
		mutil_bit++;
#else
		if (!dsm_client_ocuppy(ai_client)) {
			dsm_client_record(ai_client, "lb ecc mutil bit\n");
			dsm_client_notify(ai_client, DSM_LB_ECC_MUTIL_BIT);
			mutil_bit++;
		}
#endif
	} else if ((info & DFX_ECCERR_SINGLE_MASK) &&
		(single_bit < SINGLE_BIT_EVENT_MAX)) {
		if (!dsm_client_ocuppy(ai_client)) {
			dsm_client_record(ai_client, "lb ecc 1bit\n");
			dsm_client_notify(ai_client, DSM_LB_ECC_SINGLE_BIT);
			single_bit++;
		}
	}
}

static void dsm_lb_handle_work(struct work_struct *work)
{
	char client_name[] = "dsm_ai";
	struct dsm_client *ai_client;

	ai_client = dsm_find_client((char *)client_name);
	if (!ai_client) {
		lb_print(LB_ERROR, "lb dsm is null\n");
		return;
	}
	lbdev->dsm.ai_client = ai_client;
}

static irqreturn_t lb_dfx_handle(int irq, void *lbd)
{
	int i;
	bool ecc_single = false;
	struct lb_device *d = lbd;
	uint32_t err_inf1, err_inf2, ecc_inf;

	lb_print(LB_ERROR, "--> %s\n", __func__);

	/* disable int */
#ifdef CONFIG_MM_LB_GEMINI
	writel(0, (d->base + SC_RT_INT_EN));
#endif
	writel(0, (d->base + CMO_CFG_INT_EN));
	for (i = 0; i < SLC_IDX; i += SLC_STEP) {
		writel(0, (d->base + SLC_ADDR2000(SLC02_INT_EN, i)));
		writel(0, (d->base + SLC_ADDR2000(SLC13_INT_EN, (i + 1))));
	}

	/* read the int status */
	lb_print(LB_ERROR, "glb interrupt 0x%x",
		readl(d->base + CFG_INT_STATUS));

	lb_print(LB_ERROR, "CMO interrupt 0x%x",
		readl(d->base + CMO_CFG_INT_INI));
	for (i = 0; i < SLC_IDX; i += SLC_STEP) {
		lb_print(LB_ERROR, "slc interrupt 0x%x",
			readl(d->base + SLC_ADDR2000(SLC02_INT_INI, i)));
		lb_print(LB_ERROR, "slc interrupt 0x%x",
			readl(d->base + SLC_ADDR2000(SLC13_INT_INI, (i + 1))));
	}

	/* read ecc info */
	if (!lbdev->dsm.ai_client)
		schedule_work(&(d->dsm.dsm_work));

	for (i = 0; i < SLC_IDX; i++) {
		ecc_inf = readl(d->base + SLC_ADDR2000(SLC_DFX_ECCERR, i));
		lb_print(LB_ERROR, "ecc info: 0x%x", ecc_inf);
		lb_ecc_info(ecc_inf);
		if (ecc_inf & DFX_ECCERR_SINGLE_MASK)
			ecc_single = true;
	}

	/* clear the int */
	for (i = 0; i < SLC_IDX; i += SLC_STEP) {
		writel(INT_MASK,
			(d->base + SLC_ADDR2000(SLC02_INT_CLEAR, i)));
		writel(INT_MASK,
			(d->base + SLC_ADDR2000(SLC13_INT_CLEAR, (i + 1))));
	}
#ifdef CONFIG_MM_LB_GEMINI
	writel(ROUTE_INT_MASK, (d->base + SC_RT_INT_CLR));
#endif
	writel(CMO_CFG_INT_MASK, (d->base + CMO_CFG_INT_CLR));

	/* enable int */
	for (i = 0; i < SLC_IDX; i += SLC_STEP) {
		writel(INT_MASK, (d->base + SLC_ADDR2000(SLC02_INT_EN, i)));
		writel(INT_MASK,
			(d->base + SLC_ADDR2000(SLC13_INT_EN, (i + 1))));
	}

#ifdef CONFIG_MM_LB_GEMINI
	writel(ROUTE_INT_MASK, (d->base + SC_RT_INT_EN));
#endif
	writel(CMO_CFG_INT_MASK, (d->base + CMO_CFG_INT_EN));

	for (i = 0; i < SLC_IDX; i++) {
		err_inf1 = readl(d->base + SLC_ADDR2000(SLC_DFX_ERR_INF1, i));
		err_inf2 = readl(d->base + SLC_ADDR2000(SLC_DFX_ERR_INF2, i));
		if (err_inf1 || err_inf2)
			parse_dfx_err_info(i, err_inf1, err_inf2);
	}

#ifdef CONFIG_MM_LB_ENG_DEBUG
	if (!ecc_single)
		rdr_syserr_process_for_ap(MODID_AP_S_PANIC_LB, 0ULL, 0ULL);
#else
	WARN_ON(1);
#endif

	return IRQ_HANDLED;
}

static irqreturn_t lb_cmd_handle(int irq, void *lbd)
{
	struct lb_device *d = lbd;

	lb_print(LB_INFO, "into %s\n", __func__);
	if (!d) {
		lb_print(LB_ERROR, "lbd is null\n");
		return IRQ_NONE;
	}

	lb_print(LB_INFO, "out %s\n", __func__);
	return IRQ_NONE;
}

static int get_policy_pdata(struct platform_device *pdev, struct lb_device *lbd)
{
	int i = 0;
	u32 prot;
	int ret;
	const char *plc_name = NULL;

	struct device_node *np = NULL;
	struct lb_plc_info *p_data = NULL;
	const struct device_node *dt_node = pdev->dev.of_node;

	lb_print(LB_INFO, "into %s\n", __func__);

	ret = of_property_read_u32(dt_node, "cmo-dummy-pa", &prot);
	if (ret >= 0)
		cmo_dummy_pa = prot;

	ret = of_property_read_u32(dt_node, "support-bypass", &prot);
	if (ret >= 0)
		lbd->gdplc.is_support_bypass = prot;

	ret = of_property_read_u32(dt_node, "max-quota", &prot);
	if (ret >= 0)
		lbd->gdplc.max_quota = prot;

	ret = of_property_read_u32(dt_node, "max-size", &prot);
	if (ret >= 0)
		lbd->gdplc.max_size = prot;

	ret = of_property_read_u32(dt_node, "lb-fpga", &prot);
	if (ret >= 0)
		is_fpga = prot;

	/* if policy id is add, so plc-num also need be add */
	ret = of_property_read_u32(dt_node, "plc-num", &prot);
	if (ret < 0) {
		lb_print(LB_ERROR, "can not get plc number\n");
		return -EINVAL;
	}
	lbd->gdplc.nr = prot;

	p_data = devm_kzalloc(&pdev->dev, sizeof(*p_data) * prot,
				  GFP_KERNEL);
	if (!p_data) {
		lb_print(LB_ERROR, "get dfx irq failed\n");
		return -ENOMEM;
	}
	lbd->gdplc.ptbl = p_data;

	for_each_child_of_node(dt_node, np) {
		ret = of_property_read_string(np, "lb-name", &plc_name);
		if (ret < 0)
			continue;
		lbd->gdplc.ptbl[i].name = plc_name;

		ret = of_property_read_u32(np, "lb-pid", &prot);
		if (ret < 0)
			continue;

		if (i != prot)
			rdr_syserr_process_for_ap(MODID_AP_S_PANIC_LB,
					0ULL, 0ULL);

		lbd->gdplc.ptbl[i].pid = prot;

		ret = of_property_read_u32(np, "lb-prio", &prot);
		if (ret < 0)
			continue;

		lbd->gdplc.ptbl[i].prio = prot;
		lbd->gdplc.ptbl[i].r_prio = prot;

		ret = of_property_read_u32(np, "lb-gid", &prot);
		if (ret < 0)
			continue;

		lbd->gdplc.ptbl[i].gid = prot;

		ret = of_property_read_u32(np, "lb-stat", &prot);
		if (ret < 0)
			continue;

		lbd->gdplc.ptbl[i].stat = prot;

		ret = of_property_read_u32(np, "lb-quota", &prot);
		if (ret < 0)
			continue;

		lbd->gdplc.ptbl[i].quota = prot;
		lbd->gdplc.ptbl[i].r_quota = prot;

		ret = of_property_read_u32(np, "lb-alloc", &prot);
		if (ret < 0)
			continue;

		lbd->gdplc.ptbl[i].alloc = prot;

		ret = of_property_read_u32(np, "lb-plc", &prot);
		if (ret < 0)
			continue;

		lbd->gdplc.ptbl[i].plc = prot;

		ret = of_property_read_u32(np, "lb-flow", &prot);
		if (ret < 0)
			continue;

		lbd->gdplc.ptbl[i].flowid = prot;

		i++;
	}

	if (i != lbd->gdplc.nr)
		rdr_syserr_process_for_ap(MODID_AP_S_PANIC_LB, 0ULL, 0ULL);

	lb_print(LB_INFO, "out(%s)\n", __func__);

	return ret;
}

static int reset_ip(struct lb_device *lbd)
{
	int i;
	int ret;

	/* disable int */
#ifdef CONFIG_MM_LB_GEMINI
	writel(0, (lbd->base + SC_RT_INT_EN));
#endif
	writel(0, (lbd->base + CMO_CFG_INT_EN));
	for (i = 0; i < SLC_IDX; i += SLC_STEP) {
		writel(0, (lbd->base + SLC_ADDR2000(SLC02_INT_EN, i)));
		writel(0, (lbd->base + SLC_ADDR2000(SLC13_INT_EN, (i + 1))));
	}

	/* clear the int */
	for (i = 0; i < SLC_IDX; i += SLC_STEP) {
		writel(INT_MASK,
			(lbd->base + SLC_ADDR2000(SLC02_INT_CLEAR, i)));
		writel(INT_MASK,
			(lbd->base + SLC_ADDR2000(SLC13_INT_CLEAR, (i + 1))));
	}

#ifdef CONFIG_MM_LB_GEMINI
	writel(ROUTE_INT_MASK, (lbd->base + SC_RT_INT_CLR));
#endif
	writel(CMO_CFG_INT_MASK, (lbd->base + CMO_CFG_INT_CLR));

	ret = devm_request_threaded_irq(lbd->dev, lbd->dfx_irq, lb_dfx_handle,
		NULL, IRQF_TRIGGER_RISING, "dfx-intr", lbd);
	if (ret) {
		lb_print(LB_ERROR, "failed to enable dfx irq\n");
		return -EINVAL;
	}
	ret = devm_request_threaded_irq(lbd->dev, lbd->cmd_irq, lb_cmd_handle,
		NULL, IRQF_TRIGGER_RISING, "cmd-intr", lbd);
	if (ret) {
		lb_print(LB_ERROR, "failed to enable cmd irq\n");
		return -EINVAL;
	}

	/* enable int */
#ifdef CONFIG_MM_LB_GEMINI
	writel(ROUTE_INT_MASK, (lbd->base + SC_RT_INT_EN));
#endif
	writel(CMO_CFG_INT_MASK, (lbd->base + CMO_CFG_INT_EN));
	for (i = 0; i < SLC_IDX; i += SLC_STEP) {
		writel(INT_MASK, (lbd->base + SLC_ADDR2000(SLC02_INT_EN, i)));
		writel(INT_MASK,
			(lbd->base + SLC_ADDR2000(SLC13_INT_EN, (i + 1))));
	}

	return 0;
}

static int soc_init(struct platform_device *pdev, struct lb_device *lbd)
{
	int irq;

	lb_print(LB_INFO, "into %s\n", __func__);

	/* register intr */
	irq = platform_get_irq_byname(pdev, "dfx-intr");
	if (irq < 0) {
		lb_print(LB_ERROR, "get dfx irq failed\n");
		return -EINVAL;
	}
	lbd->dfx_irq = irq;

	irq = platform_get_irq_byname(pdev, "cmd-intr");
	if (irq < 0) {
		lb_print(LB_ERROR, "get cmd-intr failed\n");
		return -EINVAL;
	}
	lbd->cmd_irq = irq;

	/* enable intr reg */
	if (reset_ip(lbd)) {
		lb_print(LB_ERROR, "reset ip failed\n");
		return -EINVAL;
	}

	lb_print(LB_INFO, "out %s\n", __func__);

	return 0;
}

static void gidmap_init(struct lb_device *lbd)
{
	int i;

	for (i = 0; i < lbd->gdplc.nr; i++) {
		if (!(lbd->gdplc.ptbl[i].stat & GID_STATIC))
			continue;

		lbd->gdplc.gidmap |= BIT(lbd->gdplc.ptbl[i].gid);
		spin_lock_init(&lbd->gdplc.ptbl[i].lock);
	}
}

static int gid_init(struct platform_device *pdev, struct lb_device *lbd)
{
	int ret;

	lb_print(LB_INFO, "into %s\n", __func__);

	spin_lock_init(&lbd->gdplc.lock);

	/* get the gid set from dts */
	ret = get_policy_pdata(pdev, lbd);
	if (ret) {
		lb_print(LB_ERROR, "get git set from dts failed\n");
		return ret;
	}

	gidmap_init(lbd);

	if (vm_init(lbd)) {
		lb_print(LB_ERROR, "vm init failed\n");
		return -EINVAL;
	}

	if (change_size_from_efuse(lbd)) {
		lb_print(LB_ERROR, "change size from efuse failed\n");
		return -EINVAL;
	}

	lb_print(LB_INFO, "out %s\n", __func__);

	return 0;
}

static int power_init(struct platform_device *pdev, struct lb_device *lbd)
{
	/* lb power init */
	return 0;
}

static int dsm_init(struct lb_device *lbd)
{
	INIT_WORK(&(lbd->dsm.dsm_work), dsm_lb_handle_work);

	return 0;
}

static int lb_probe(struct platform_device *pdev)
{
	struct resource *res = NULL;
	struct lb_device *lb_dev = NULL;
	struct device *dev = &pdev->dev;

	lb_print(LB_INFO, "into %s\n", __func__);

	lb_dev = devm_kzalloc(dev, sizeof(*lb_dev), GFP_KERNEL);
	if (!lb_dev) {
		lb_print(LB_ERROR, "failed to allocate lb_dev\n");
		return -ENOMEM;
	}
	lb_dev->dev = dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		lb_print(LB_ERROR, "get resource is failed\n");
		return -EINVAL;/*lint !e429 */
	}

	lb_dev->base = devm_ioremap_resource(dev, res);
	if (!lb_dev->base) {
		lb_print(LB_ERROR, "io remap failed\n");
		return -EINVAL;/*lint !e429 */
	}

	lb_dev->is_available = !get_lb_lite();
	if (!lb_dev->is_available) {
		lb_print(LB_ERROR, "lb unusable\n");
		lbdev = lb_dev;
		return 0;
	}

	lb_dev->way_efuse = get_lb_efuse();
	if ((lb_dev->way_efuse & GID_WAY_EN) != lb_dev->way_efuse) {
		lb_print(LB_ERROR, "lb efuse init fail\n");
		return -EINVAL;
	}

	if (soc_init(pdev, lb_dev)) {
		lb_print(LB_ERROR, "lb init failed\n");
		return -EINVAL;
	}

	if (gid_init(pdev, lb_dev)) {
		lb_print(LB_ERROR, "gid init failed\n");
		return -EINVAL;
	}

	if (power_init(pdev, lb_dev)) {
		lb_print(LB_ERROR, "power init failed\n");
		return -EINVAL;
	}

	if (lb_pmu_init(pdev, lb_dev)) {
		lb_print(LB_ERROR, "pmu init failed\n");
		return -EINVAL;
	}

	if (dsm_init(lb_dev)) {
		lb_print(LB_ERROR, "dsm init failed\n");
		return -EINVAL;
	}

	/* register to loacal device */
	lbdev = lb_dev;

	/*
	 * need lb power down
	 * because lb powerup when atf init
	 */
	if (power_control(POWERDOWN)) {
		lb_print(LB_ERROR, "powerdown failed\n");
		return -EINVAL;
	}

	lb_print(LB_INFO, "out %s\n", __func__);

	return 0;
}

static int lb_remove(struct platform_device *pdev)
{
	lbdev = NULL;
	return 0;
}

static const struct of_device_id lb_match_table[] = {
	{.compatible = "hisilicon,hisi-lb" },
	{},
};

static struct platform_driver lb_driver = {
	.probe = lb_probe,
	.remove = lb_remove,
	.driver = {
		.name = "lb-hisi",
		.owner = THIS_MODULE,
		.of_match_table = lb_match_table,
	},
};

static int lb_init_ns(void)
{
	return platform_driver_register(&lb_driver);
}
subsys_initcall(lb_init_ns);
module_param_named(debug_level, lb_d_lvl, uint, 0644);
