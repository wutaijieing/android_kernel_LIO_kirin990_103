/*
 * namtso_debug.c
 *
 * namtso debug
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
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
#define pr_fmt(fmt) "namtso_d: " fmt
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <securec.h>
#include <linux/random.h>
#include <asm/tlbflush.h>
#include <linux/delay.h>
#include <asm/sysreg.h>
#include <linux/perf_event.h>
#include <asm/cacheflush.h>
#include <linux/platform_drivers/hisi_flush_cmo.h>
#include <linux/mm.h>
#include <linux/platform_drivers/mm_ion.h>
#include "ion.h"
#include <linux/syscalls.h>

/* flush cmo */
int test_flush_cluster_cmo(void)
{
	pr_err("%s: enter\n", __func__);
	flush_cluster_cmo();

	return 0;
}

static void _test_flush_cache_all(void *dummy)
{
	flush_cache_all();
}

int test_flush_cache_all(void)
{
	int cpu;
	cpumask_t mask;

	cpumask_clear(&mask);

	preempt_disable();

	for_each_online_cpu(cpu)
		cpumask_set_cpu(cpu, &mask);

	on_each_cpu_mask(&mask, _test_flush_cache_all, NULL, 1);

	preempt_enable();

	return 0;
}

/* check whether flush_cluster_cmo is OK */
DEFINE_SPINLOCK(flush_cmo_lock);
#define ALLOC_SIZE	1024
#define L2CACHE_REFILL_INNER	0x10C
#define L2CACHE_REFILL_OUTER	0x10D
#define INIT_VAL	100
#define NEW_VAL	200
#define UPDATE_VAL	300

enum pmu_idx {
	START_IDX,
	END_IDX,
	MAX_IDX
};

struct pmu_cnt_t {
	u64 cnt;
} __aligned(64);

static struct pmu_cnt_t g_inner_cnt[MAX_IDX];
static struct pmu_cnt_t g_outer_cnt[MAX_IDX];

static void set_pmu_event_type(u32 idx, u32 event_id)
{
	/* set eventid */
	write_sysreg(idx, pmselr_el0);
	isb();
	write_sysreg(event_id, pmxevtyper_el0);

	/* reset init val */
	write_sysreg(0, pmxevcntr_el0);

	/* enable counter */
	write_sysreg(BIT(idx), pmcntenset_el0);
}

static void disable_counter(u32 idx)
{
	write_sysreg(idx, pmselr_el0);
	isb();

	/* disable counter */
	write_sysreg(BIT(idx), pmcntenclr_el0);
}

static void enable_pmu(void)
{
	u32 val;

	val = read_sysreg(pmcr_el0);
	isb();
	write_sysreg(val | ARMV8_PMU_PMCR_E, pmcr_el0);
}

static void disable_pmu(void)
{
	u32 val;

	val = read_sysreg(pmcr_el0);
	isb();
	write_sysreg(val & (~ARMV8_PMU_PMCR_E), pmcr_el0);
}

static void pmu_init(void)
{
	/* config event type */
	set_pmu_event_type(0, L2CACHE_REFILL_INNER);
	set_pmu_event_type(1, L2CACHE_REFILL_OUTER);

	/* enable pmu */
	enable_pmu();
}

static void pmu_uninit(void)
{
	/* disbale counter */
	disable_counter(0);
	disable_counter(1);

	/* enable pmu */
	disable_pmu();
}

static u64 read_pmu_cnt(u32 idx)
{
	u64 val;

	write_sysreg(idx, pmselr_el0);
	isb();

	val = read_sysreg(pmxevcntr_el0);

	return val;
}

int check_flush_cmo_by_pmu(int type)
{
	int *addr = NULL;
	unsigned long flag;
	u64 inner_delta;
	u64 outer_delta;
	int i;

	addr = kmalloc(sizeof(int) * ALLOC_SIZE, GFP_KERNEL);
	if (addr == NULL) {
		pr_err("kmalloc failed\n");
		return -1;
	}

	(void)memset_s(addr, sizeof(int) * ALLOC_SIZE, 0, sizeof(int) * ALLOC_SIZE);

	pmu_init();

	/* update new value */
	for (i = 0; i < ALLOC_SIZE; i++)
		addr[i] = i * INIT_VAL;

	spin_lock_irqsave(&flush_cmo_lock, flag);

	/* flush cache */
	if (type == 0)
		flush_cluster_cmo();
	else if (type == 1)
		flush_cache_all();
	else
		pr_err("%s: not supported type\n", __func__);

	g_inner_cnt[START_IDX].cnt = read_pmu_cnt(0);
	g_outer_cnt[START_IDX].cnt = read_pmu_cnt(1);

	isb();
	for (i = 0; i < ALLOC_SIZE; i++)
		addr[i] = i * NEW_VAL;

	dsb(sy);
	isb();

	g_inner_cnt[END_IDX].cnt = read_pmu_cnt(0);
	g_outer_cnt[END_IDX].cnt = read_pmu_cnt(1);

	spin_unlock_irqrestore(&flush_cmo_lock, flag);

	inner_delta = g_inner_cnt[END_IDX].cnt - g_inner_cnt[START_IDX].cnt;
	outer_delta = g_outer_cnt[END_IDX].cnt - g_outer_cnt[START_IDX].cnt;

	pr_err("[start] inner = 0x%llx outer = 0x%llx\n",
	       g_inner_cnt[START_IDX].cnt, g_outer_cnt[START_IDX].cnt);
	pr_err("[end] inner = 0x%llx outer = 0x%llx\n",
	       g_inner_cnt[END_IDX].cnt, g_outer_cnt[END_IDX].cnt);
	pr_err("inner_delta = 0x%llx outer_delta = 0x%llx\n",
	       inner_delta, outer_delta);
	if (inner_delta) {
		pr_err("%s: unexpected\n", __func__);
		BUG_ON(1);
	}

	pmu_uninit();

	kfree(addr);

	return 0;
}

enum DC_OP_STAGE {
	S0_DIS_0,
	S0_DIS_1,
	S0_EN_0,
	S0_EN_1,
	S1_DIS_0,
	S1_DIS_1,
	S1_EN_0,
	S1_EN_1,
	MAX_STAGE
};

static u64 g_sctlr_el1[MAX_STAGE];

static void flush_cmo_warmup(int *addr, int len, bool *verify)
{
		int i;

		/* step1: disable d-cache/i-cache */
		g_sctlr_el1[S0_DIS_0] = read_sysreg(sctlr_el1);
		sysreg_clear_set(sctlr_el1, SCTLR_ELx_C | SCTLR_ELx_I, 0);
		isb();
		g_sctlr_el1[S0_DIS_1] = read_sysreg(sctlr_el1);
		dsb(sy);
		isb();

		/* step2: set/way flush */
		flush_cache_all();

		/* step3: write new value */
		for (i = 0; i < len; i++)
			addr[i] = NEW_VAL;   /* write through to DDR */
		dsb(sy);
		isb();

		/* step4: enable d-cache/i-cache */
		g_sctlr_el1[S0_EN_0] = read_sysreg(sctlr_el1);
		sysreg_clear_set(sctlr_el1, 0, SCTLR_ELx_C | SCTLR_ELx_I);
		isb();
		g_sctlr_el1[S0_EN_1] = read_sysreg(sctlr_el1);
		dsb(sy);
		isb();

		/* step5: read value which allocated into cache */
		for (i = 0; i < len; i++) {
			if (addr[i] != NEW_VAL) {
				pr_err("addr[%d] = %d, expected = %d\n",
				       i, addr[i], NEW_VAL);
				*verify = true;
			}
		}
		dsb(sy);
		isb();

		/* step6: update value */
		for (i = 0; i < len; i++)
			addr[i] = UPDATE_VAL;   /* update in cache */
		dsb(sy);
		isb();
}

int check_flush_cmo_by_dis_dcache(int type, int brk, int warmup)
{
	int *addr = NULL;
	unsigned long flag;
	bool verify = false;
	int i;

	pr_err("%s: type=%d brk=%d warmup=%d\n", __func__, type, brk, warmup);

	addr = kmalloc(sizeof(int) * ALLOC_SIZE, GFP_KERNEL);
	if (addr == NULL) {
		pr_err("kmalloc failed\n");
		return -1;
	}

	pr_err("%s: addr = 0x%pK\n", __func__, addr);

	spin_lock_irqsave(&flush_cmo_lock, flag);
	if (brk != 0)
		__asm__ volatile ("b .");

	(void)memset_s(addr, sizeof(int) * ALLOC_SIZE, 0, sizeof(int) * ALLOC_SIZE);
	(void)memset_s(g_sctlr_el1, sizeof(u64) * MAX_STAGE, 0, sizeof(u64) * MAX_STAGE);

	/* init value */
	for (i = 0; i < ALLOC_SIZE; i++)
		addr[i] = UPDATE_VAL;
	dsb(sy);
	isb();

	if (warmup != 0)
		flush_cmo_warmup(addr, ALLOC_SIZE, &verify);

	/* step7: disable d-cache/i-cache */
	g_sctlr_el1[S1_DIS_0] = read_sysreg(sctlr_el1);
	sysreg_clear_set(sctlr_el1, SCTLR_ELx_C | SCTLR_ELx_I, 0);
	isb();
	g_sctlr_el1[S1_DIS_1] = read_sysreg(sctlr_el1);
	dsb(sy);
	isb();

	/* step8: flush cache */
	if (type == 0)
		__flush_cluster_cmo();
	else if (type == 1)
		flush_cluster_cmo();
	else if (type == 2)
		flush_cache_all();
	else
		pr_err("%s: not supported type\n", __func__);

	/* step9: verify value loaded from ddr */
	for (i = 0; i < ALLOC_SIZE; i++) {
		if (addr[i] != UPDATE_VAL) {
			pr_err("unexpected! addr[%d] = %d, expected val = %d\n",
			       i, addr[i], UPDATE_VAL);
			verify = true;
		}
	}

	/* step10: will panic? */
	if (verify) {
		pr_err("%s: GAME OVER\n", __func__);
		BUG_ON(1);
	}

	/* step11: enable d-cache/i-cache again */
	g_sctlr_el1[S1_EN_0] = read_sysreg(sctlr_el1);
	sysreg_clear_set(sctlr_el1, 0, SCTLR_ELx_C | SCTLR_ELx_I);
	isb();
	g_sctlr_el1[S1_EN_1] = read_sysreg(sctlr_el1);
	dsb(sy);
	isb();

	pr_err("g_sctlr_el1[S0_DIS_0] = 0x%llx\n", g_sctlr_el1[S0_DIS_0]);
	pr_err("g_sctlr_el1[S0_DIS_1] = 0x%llx\n", g_sctlr_el1[S0_DIS_1]);
	pr_err("g_sctlr_el1[S0_EN_0] = 0x%llx\n", g_sctlr_el1[S0_EN_0]);
	pr_err("g_sctlr_el1[S0_EN_1] = 0x%llx\n", g_sctlr_el1[S0_EN_1]);

	pr_err("g_sctlr_el1[S1_DIS_0] = 0x%llx\n", g_sctlr_el1[S1_DIS_0]);
	pr_err("g_sctlr_el1[S1_DIS_1] = 0x%llx\n", g_sctlr_el1[S1_DIS_1]);
	pr_err("g_sctlr_el1[S1_EN_0] = 0x%llx\n", g_sctlr_el1[S1_EN_0]);
	pr_err("g_sctlr_el1[S1_EN_1] = 0x%llx\n", g_sctlr_el1[S1_EN_1]);
	spin_unlock_irqrestore(&flush_cmo_lock, flag);

	kfree(addr);

	return 0;
}

/* DCZVA */
#define MEM_SIZE	4096
#define WAIT_TIME	1000 /* ms */
static int test_dczva(void)
{
	unsigned char *pbuf = NULL;
	unsigned char *pdest = NULL;
	int ret;
	int i;

	pbuf = vmalloc(MEM_SIZE);
	if (pbuf == NULL) {
		pr_err("pbuf malloc fail\n");
		return -1;
	}

	ret = memset_s(pbuf, MEM_SIZE, 0x5A, MEM_SIZE);
	if (ret != EOK) {
		pr_err("pbuf memset_s of 0x5A fail\n");
		goto free_pbuf;
	}

	pdest = vmalloc(MEM_SIZE);
	if (pdest == NULL) {
		pr_err("pdest malloc fail\n");
		goto free_pbuf;
	}

	ret = memcpy_s(pdest, MEM_SIZE, pbuf, MEM_SIZE);
	if (ret != EOK) {
		pr_err("pdest memcpy_s fail\n");
		goto free_pdest;
	}

	ret = memset_s(pdest, MEM_SIZE, 0, MEM_SIZE);
	if (ret != EOK) {
		pr_err("pdest memset_s of 0 fail\n");
		goto free_pdest;
	}

	for (i = 0; i < MEM_SIZE; i++) {
		if (pdest[i] != 0) {
			pr_err("test failed\n");
			BUG_ON(1);
		}
	}

	vfree(pdest);
	vfree(pbuf);

	pr_err("%s success\n", __func__);
	return 0;

free_pdest:
	vfree(pdest);
free_pbuf:
	vfree(pbuf);
	return -1;
}

int test_dczva_loop(int loop)
{
	int i;
	int ret;

	pr_err("%s\n", __func__);
	for (i = 0; i < loop; i++) {
		msleep(WAIT_TIME);
		ret = test_dczva();
	}

	return 0;
}

/* ADD ONE for multi-cores */
#define ADD_ONE_SIZE        (1024 * 1024)
typedef struct {
	u64 a;
	u64 b;
} add_one_t;
static add_one_t g_add_one;
static u64 g_cnt_a;
static u64 g_cnt_b;

int add_one_run_a(void)
{
	u32 i;
	u32 loop = prandom_u32() % ADD_ONE_SIZE;

	pr_err("%s a loop = %u\n", __func__, loop);
	for (i = 0; i < loop; i++) {
		g_add_one.a += 1;
		g_cnt_a++;
	}

	if (g_add_one.a != g_cnt_a) {
		pr_err("%s a = %llu, g_cnt_a = %llu\n",
		       __func__, g_add_one.a, g_cnt_a);
		BUG_ON(1);
	}

	return 0;
}

int add_one_run_b(void)
{
	u32 i;
	u32 loop = prandom_u32() % ADD_ONE_SIZE;

	pr_err("%s b loop = %u\n", __func__, loop);
	for (i = 0; i < loop; i++) {
		g_add_one.b += 1;
		g_cnt_b++;
	}

	if (g_add_one.b != g_cnt_b) {
		pr_err("%s b = %llu, g_cnt_b = %llu\n",
		       __func__, g_add_one.b, g_cnt_b);
		BUG_ON(1);
	}

	return 0;
}

/* many many tlbi and device operation */
int test_flush_tlb_all(void)
{
	pr_err("%s\n", __func__);
	flush_tlb_all();
	return 0;
}

/* hwp */
u64 show_hwp_status(void)
{
	u64 val = 0;

	asm volatile("mrs %0, s3_1_c15_c6_4" : "=r" (val));
	pr_err("%s hwp status = 0x%llx\n", __func__,  val);
	return val;
}

/*  klein prefetch target */
u64 show_klein_prefetch_target(void)
{
	u64 val = 0;

	asm volatile("mrs %0, s3_0_c15_c1_7" : "=r" (val));
	pr_err("%s pft = 0x%llx\n", __func__,  val);
	return val;
}

/* non-shareable access */
#define PTE_NONSHARED	    (_AT(pteval_t, 0) << 8)
#define _PROT_NON_DET	    (PTE_TYPE_PAGE | PTE_AF | PTE_NONSHARED)
#define PROT_NON_DET	    (_PROT_NON_DET | PTE_MAYBE_NG)
#define PROT_NONSHARABLE    (PROT_NON_DET | PTE_PXN | PTE_UXN | PTE_DIRTY | \
			     PTE_WRITE | PTE_ATTRINDX(MT_NORMAL))

#define BUFFER_SIZE	4096
int test_nonshareable_access(void)
{
	int fd = -1;
	size_t size;
	unsigned int heap_id_mask;
	unsigned int flags;
	struct dma_buf *dmabuf = NULL;
	struct ion_buffer *buffer = NULL;
	uint64_t phy_addr;
	u32 *virt_addr = NULL;
	int ret = 0;

	size = BUFFER_SIZE;
	heap_id_mask = 0x1U << ION_SYSTEM_HEAP_ID;
	flags = 0;
	fd = ion_alloc(size, heap_id_mask, flags);
	if (fd < 0) {
		pr_err("%s: ion_alloc failed\n", __func__);
		return -1;
	}

	dmabuf = dma_buf_get(fd);
	if (IS_ERR(dmabuf)) {
		pr_err("%s: dma_buf_get failed\n", __func__);
		ret = -1;
		goto free_fd;
	}
	buffer = (struct ion_buffer *)dmabuf->priv;

	phy_addr = sg_phys(buffer->sg_table->sgl);
	virt_addr = phys_to_virt(phy_addr);

	change_secpage_range(phy_addr,
			     (uintptr_t)virt_addr,
			     size,
			     __pgprot(PROT_NONSHARABLE));

	(void)memset_s(virt_addr, BUFFER_SIZE, 0, BUFFER_SIZE);

	dma_buf_put(dmabuf);

free_fd:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0))
	ksys_close(fd);
#else
	sys_close(fd);
#endif
	return ret;
}

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Hisi Namtso Debug Driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
