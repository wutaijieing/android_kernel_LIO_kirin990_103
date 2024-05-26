/*
 * mm_lb_tst.c
 *
 * tst for lb dev
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

#include <asm/page.h>
#include <asm/cacheflush.h>
#include <linux/delay.h>
#include <linux/dma-contiguous.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/memory.h>
#include <linux/mm.h>
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/page-flags.h>
#include <linux/platform_device.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/sizes.h>
#include <linux/vmalloc.h>
#include <platform_include/basicplatform/linux/rdr_platform.h>

#include "mm_lb_priv.h"
#include "mm_lb.h"
#include "soc_mid.h"

#define LB_CACHEABLE        0x1
#define LB_NON_CACHEABLE    0x0

#ifndef SUCC
#define SUCC   0
#endif
#ifndef FAIL
#define FAIL   (-1)
#endif

#define PLC_W_NALLC         0
#define PLC_W_ALLC          1
#define PLC_W_NREFILL       2
#define PLC_R_NALLC         0
#define PLC_R_ALLC          1
#define PLC_R_INV           2
#define PLC_R_CI            3
#define PLC_SAM_RPLC_DIS    0
#define PLC_SAM_RPLC_EN     1
#define PLC_PRIO_00         0x0
#define PLC_PRIO_01         0x1
#define PLC_PRIO_10         0x2
#define PLC_PRIO_11         0x3
#define PLC_GID_DIS         0
#define PLC_GID_EN          1
#define PART_4_WAY_EN       0xf
#define PART_8_WAY_EN       0xff
#define PART_12_WAY_EN      0xfff
#define PART_4_WAY_CNT      4

#ifdef CONFIG_MM_LB_GEMINI
#ifdef IS_FPGA
#define GID_SZ_MAX         0x400000
#define GID_SZ_HALF        0x200000
#define ALL_WAY_EN         0x00ff
#define WAY_NUM_MAX        8
#define QUOTA_MAX          0x2000
#else
#ifdef CONFIG_HISI_LB_WAY_16
#define GID_SZ_MAX         0x400000
#define GID_SZ_HALF        0x200000
#define ALL_WAY_EN         0xffff
#define WAY_NUM_MAX        16
#define QUOTA_MAX          0x2000
#else
#define GID_SZ_MAX         0x800000
#define GID_SZ_HALF        0x400000
#define ALL_WAY_EN         0xffff
#define WAY_NUM_MAX        16
#define QUOTA_MAX          0x4000
#endif

#endif
#else
#define GID_SZ_MAX         0x380000
#define GID_SZ_HALF        0x1c0000
#define ALL_WAY_EN         0x3fff
#define WAY_NUM_MAX        14
#define QUOTA_MAX          0x7000
#endif

#ifdef CONFIG_MM_LB_GEMINI
#define QUOTA_512K           0x0400
#define QUOTA_1M             0x0800
#define QUOTA_2M             0x1000
#define QUOTA_3M             0x1800
#define QUOTA_4M             0x2000
#else
#define QUOTA_512K           0x1000
#define QUOTA_1M             0x2000
#define QUOTA_2M             0x4000
#endif

#define FCM_M0_MID           SOC_FCM_FCM_M0_MID
#define FCM_M1_MID           SOC_FCM_FCM_M1_MID
#define MID_MASK             0x7f

#define SCB_STEP      8
#define SCB_2_SHIFT   2
#define SCB_5_SHIFT   5
#define SCB_7_SHIFT   7

#define SZ_60M                  (60 * SZ_1M)
#define CACHE_LINE              128
#define INT_CLR_MASK            0xffffffff
#define CMO_BYGID_ERR_MASK      BIT(16)
#define CMO_BYWAY_ERR_MASK      BIT(15)
#define CMO_CMD_OVERFLOW_MASK   BIT(13)
#define CMO_CMD_ACCESS_ERR_MASK BIT(12)
#define CMO_OVERFLOW_CNT        1024
#define INVALID_CMO_CMD_SHIFT   8
#define DFX_CMOGID_ILLEGAL      BIT(20)
#define DFX_PRI11_NOALLOC       BIT(15)
#define DFX_GID_MISMATCH        BIT(14)
#define DFX_GID_ILLEGAL         BIT(13)
#define PMU_CNT_ST_MASK         0xfff
#define PA_17_7_EN              0x3ff80
#define PA_23_6_EN              0xffffc0
#define PA_20_6_EN              0x1fffc0
#define PA_31_7_EN              0xffffff80
#define PA_17_11_EN             0x3f800
#define PA_9_8_EN               0x3f800
#define PA_17_10_EN             0x3fc00
#define PA_7_EN                 0x80
#define PA_9_1_EN               0x3fe
#define FUNC_PARAM_CNT          6
#define THREAD_CNT_MAX          16
#define CMO_PA_STEP             0x20000
#ifdef CONFIG_HISI_LB_WAY_16
#define CMO_SYNC_STEP           32
#else
#define CMO_SYNC_STEP           64
#endif

#define LB_DEFAULT_VALUE        0x5a
#define DDR_DEFAULT_VALUE       0xa5
#define CSZ_THRESHOLD           0x10
#define MM_LB_FID_VALUE         0xc500bc00u
#define GID_SZ                  0xc00000
#define GID_TST_CNT             8
#define MPATTERN_GID_CNT        4
#define TST_LOOP_MAX            0x10000000

#define MEM_ERROR_MAX           10
#define MGID_CNT                3
#define MGID_PLRU_CNT           2
#define LOCK_BUFFER_GID_CNT     2

#define MPATTERN_STEP_5   5
#define MPATTERN_STEP     0xff
#define MPA_LOW_SHIFT     7
#define MPA_HIGH_SHIFT    8

#define PID_3             3
#define LBV_MASK          0xff
#define MEM_CHAR_STAT     0x400

#define CMD_ID_MASK       0xf
#define CMD_TID_SHIFT     4
#define CMD_TID_MASK      0xf
#define CMD_LOOP_SHIFT    8
#define CMD_LOOP_MASK     0xffffff

#define PMU_EVENT_CNT     12

#define CHAR_MASK         0xff
#define LB_TST_ORDER_MAX  8
#define LB_TST_SZ_MAX     0x10000000

struct lb_reserve_region {
	const char *name;
	phys_addr_t base;
	phys_addr_t size;
};

struct lb_buffer {
	int pid;
	int flag;
	int lbv;
	int ddrv;
	size_t count;
	gid quota;
	gid way;
	gid plc;
	gid flt0;
	gid flt1;
	void *va;
	void *lbva;
	struct page **pages;
};

struct lb_thread_func {
	int id;
	int (*func)(u32 cfg0, u32 cfg1, u32 cfg2, u32 cfg3);
};

static void *g_power_saddr;
static void *g_power_daddr;
static int g_thread_state;
static struct lb_reserve_region g_region;
static struct lb_buffer *g_reserve_buffer;
static struct mutex lb_mutex[THREAD_CNT_MAX];
static int g_mthread_result[THREAD_CNT_MAX];
static unsigned int mthread_cfg[THREAD_CNT_MAX][FUNC_PARAM_CNT];

static void lb_save_dynamic_alloc_area(phys_addr_t base, unsigned long len,
	const char *name)
{
	struct lb_reserve_region *reg = &g_region;

	reg->base = base;
	reg->size = len;
	reg->name = name;
}

static int lb_reserve_area(struct reserved_mem *rmem)
{
	const char *region_name = NULL;
	char *status = NULL;
	int name_size;

	status = (char *)of_get_flat_dt_prop(rmem->fdt_node, "status", NULL);
	if (status && (strncmp(status, "ok", strlen("ok")) != 0))
		return SUCC;

	region_name =
		of_get_flat_dt_prop(rmem->fdt_node, "region-name", &name_size);
	if (!region_name || name_size <= 0) {
		lb_print(LB_ERROR, "%s no sz: %d\n", __func__, name_size);
		return FAIL;
	}

	lb_save_dynamic_alloc_area(rmem->base, rmem->size, region_name);

	return SUCC;
}
RESERVEDMEM_OF_DECLARE(lb_tst_region, "lb_tst_region", lb_reserve_area);

static void gid_cfg_show(int pid)
{
	if (!lbdev) {
		lb_print(LB_ERROR, "%s point is NULL\n", __func__);
		return;
	}

	if (pid > GID_IDX) {
		lb_print(LB_ERROR, "%s %d invalid param\n", __func__, pid);
		return;
	}

	lb_print(LB_INFO, "0x%x\n",
		readl((lbdev->base + GID_ADDR100(GID_WAY_ALLC, pid))));
	lb_print(LB_INFO, "0x%x\n",
		readl((lbdev->base + GID_ADDR100(GID_QUOTA, pid))));
	lb_print(LB_INFO, "0x%x\n",
		readl((lbdev->base + GID_ADDR100(GID_ALLC_PLC, pid))));
	lb_print(LB_INFO, "0x%x\n",
		readl((lbdev->base + GID_ADDR100(GID_MID_FLT0, pid))));
	lb_print(LB_INFO, "0x%x\n",
		readl((lbdev->base + GID_ADDR100(GID_MID_FLT1, pid))));
}

static void gid_policy_set(const struct lb_buffer *lb)
{
	struct lb_device *lbd = lbdev;

	if (!lbd || !lb || !(lb->pid)) {
		lb_print(LB_ERROR, "point is NULL\n");
		return;
	}

	writel(lb->way.value, (lbd->base + GID_ADDR100(GID_WAY_ALLC, lb->pid)));
	writel(lb->quota.value, (lbd->base + GID_ADDR100(GID_QUOTA, lb->pid)));
	writel(lb->plc.value, (lbd->base + GID_ADDR100(GID_ALLC_PLC, lb->pid)));
	writel(lb->flt0.value,
		(lbd->base + GID_ADDR100(GID_MID_FLT0, lb->pid)));
	writel(lb->flt1.value,
		(lbd->base + GID_ADDR100(GID_MID_FLT1, lb->pid)));

	/* ensure set gid cfg complete */
	mb();

	gid_cfg_show(lb->pid);
}

static void gid_cfg_init(struct lb_buffer *lb)
{
	gid quota_set, way_set, plc_set, flt0_set, flt1_set;

	quota_set.reg.gid_quota.quota_l = QUOTA_MAX;
	quota_set.reg.gid_quota.quota_h = QUOTA_MAX;

	way_set.reg.gid_way_allc.gid_way_en = ALL_WAY_EN;
	way_set.reg.gid_way_allc.gid_way_srch = ALL_WAY_EN;

	plc_set.reg.gid_allc_plc.nor_r_plc = PLC_R_ALLC;
	plc_set.reg.gid_allc_plc.nor_w_plc = PLC_W_ALLC;
	plc_set.reg.gid_allc_plc.flt_r_plc = PLC_R_NALLC;
	plc_set.reg.gid_allc_plc.flt_w_plc = PLC_W_NALLC;
	plc_set.reg.gid_allc_plc.sam_rplc = PLC_SAM_RPLC_EN;
	plc_set.reg.gid_allc_plc.gid_prpty = PLC_PRIO_00;
	plc_set.reg.gid_allc_plc.gid_en = PLC_GID_EN;

	flt0_set.value = 0;
	flt1_set.value = 0;

	lb->quota = quota_set;
	lb->way = way_set;
	lb->plc = plc_set;
	lb->flt0 = flt0_set;
	lb->flt1 = flt1_set;

	gid_policy_set(lb);
	lb_print(LB_ERROR, "0x%x, 0x%x, 0x%x\n", quota_set, way_set, plc_set);
}

static void gid_policy_reset(int pid)
{
	struct lb_device *lbd = lbdev;

	if (!lbd || !pid) {
		lb_print(LB_ERROR, "point is NULL\n");
		return;
	}

	writel(0, (lbd->base + GID_ADDR100(GID_WAY_ALLC, pid)));
	writel(0, (lbd->base + GID_ADDR100(GID_QUOTA, pid)));
	writel(0, (lbd->base + GID_ADDR100(GID_ALLC_PLC, pid)));
	writel(0, (lbd->base + GID_ADDR100(GID_MID_FLT0, pid)));
	writel(0, (lbd->base + GID_ADDR100(GID_MID_FLT1, pid)));

	/* ensure set gid cfg complete */
	mb();

	gid_cfg_show(pid);
}

int gid_csz_show(int pid)
{
	int i;
	u32 csz;
	u32 sum = 0;

	if (!lbdev) {
		lb_print(LB_ERROR, "point is NULL\n");
		return sum;
	}

	for (i = 0; i < SLC_IDX; i++) {
		csz = readl(
			(lbdev->base + SLC_GID_ADDR4(SLC_GID_COUNT, i, pid)));
		sum += csz;
		lb_print(LB_ERROR, "gid-%d slice-%d %u\n", pid, i, csz);
	}

	return sum;
}

static int gid_csz_check(u32 pid, u32 sz)
{
	u32 csz, csz_obj, obj_right, obj_left;
	int ret = SUCC;

	if (g_thread_state)
		return ret;

	csz = gid_csz_show(pid);
	csz_obj = pid ? (sz / CACHE_LINE) : 0;
	obj_left = (csz_obj > CSZ_THRESHOLD) ? csz_obj - CSZ_THRESHOLD : 0;
	obj_right = csz_obj + CSZ_THRESHOLD;
	if (csz < obj_left || csz > obj_right) {
		lb_print(LB_ERROR, "%s 0x%x, 0x%x\n", __func__, csz, csz_obj);
		ret = FAIL;
	}

	return ret;
}

static void cmo_cfg_int_clr(void)
{
	if (!lbdev) {
		lb_print(LB_ERROR, "point is NULL\n");
		return;
	}

	writel(INT_CLR_MASK, lbdev->base + CMO_CFG_INT_CLR);
}

static u32 cmo_int_st(void)
{
	if (!lbdev) {
		lb_print(LB_ERROR, "point is NULL\n");
		return 0;
	}

	return readl(lbdev->base + CMO_CFG_INT_INI);
}

static void slc_int_clr(void)
{
	int i;

	if (!lbdev) {
		lb_print(LB_ERROR, "point is NULL\n");
		return;
	}

	for (i = 0; i < SLC_IDX; i += SLC_STEP) {
		writel(INT_CLR_MASK,
			lbdev->base + SLC_ADDR2000(SLC02_INT_CLEAR, i));
		writel(INT_CLR_MASK,
			lbdev->base + SLC_ADDR2000(SLC13_INT_CLEAR, (i + 1)));
	}
}

static u32 slc_int_st(void)
{
	int i;
	u32 v = 0;

	if (!lbdev) {
		lb_print(LB_ERROR, "point is NULL\n");
		return 0;
	}

	for (i = 0; i < SLC_IDX; i += SLC_STEP) {
		v |= readl(lbdev->base + SLC_ADDR2000(SLC02_INT_ST, i));
		v |= readl(lbdev->base + SLC_ADDR2000(SLC13_INT_ST, (i + 1)));
	}

	lb_print(LB_INFO, "%s, v: 0x%x\n", __func__, v);

	return v;
}

static void cmo_ci_by_gid(int pid)
{
	lb_ops_cache(EVENT, CI, CMO_BY_GID, BIT(pid), 0, 0);

	/* ensure cmo ops complete */
	mb();

	lb_cmo_sync_cmd_by_event();

	gid_csz_show(pid);
}

static void cmo_ci_by_way(u32 way)
{
	lb_ops_cache(EVENT, CI, CMO_BY_WAY, way, 0, 0);

	/* ensure cmo ops complete */
	mb();

	lb_cmo_sync_cmd_by_event();
}

static void mem_write(void *va, int v, size_t sz)
{
	memset(va, v, sz);

	/* ensure memset complete */
	mb();
}

static void mem_read(const void *va, size_t sz)
{
	int i;

	if (g_reserve_buffer == NULL) {
		lb_print(LB_ERROR, "reserve buffer is NULL\n");
		return;
	}

	for (i = 0; i < sz; i += SZ_1M, va += SZ_1M) {
		memcpy(g_reserve_buffer->va, va, SZ_1M);

		/* ensure memcpy complete */
		mb();
	}
}

static int mem_check(const void *va, unsigned char v, size_t size)
{
	int i;
	int ret;
	int err_cnt = 0;
	const unsigned char *p = va;

	for (i = 0; i < size; i++) {
		if (*(p) != v && err_cnt < MEM_ERROR_MAX)
			lb_print(LB_ERROR, "[%s] failed. i: %d, v: 0x%x, p: 0x%x\n", __func__, i, v, *(p));
		if (*(p) != v)
			err_cnt++;

		p++;
	}

	if (!err_cnt) {
		lb_print(LB_ERROR, "%s ok\n", __func__);
		ret = SUCC;
	} else {
		lb_print(LB_ERROR, "[%s] failed. err_cnt: 0x%x\n", __func__, err_cnt);
		ret = FAIL;
	}

	return ret;
}

static int mem_char_statistics(const unsigned char *va, unsigned char v,
	size_t size)
{
	int i;
	u32 sum = 0;
	const unsigned char *p = va;

	if (!p) {
		lb_print(LB_ERROR, "va is NULL\n");
		return 0;
	}

	for (i = 0; i < size; i++)
		if (*(p++) == v)
			sum++;

	lb_print(LB_INFO, "%s: sum = 0x%x\n", __func__, sum);

	return sum;
}

static void *lb_buffer_map(struct lb_buffer *lb, int pid)
{
	pgprot_t pgprot;
	size_t count;
	void *vaddr = NULL;
	struct page **pages = NULL;

	if (!lb)
		return NULL;

	count = lb->count;
	pages = lb->pages;

	if (lb->flag & LB_CACHEABLE)
		pgprot = PAGE_KERNEL;
	else
		pgprot = pgprot_writecombine(PAGE_KERNEL);

	pgprot = pgprot_lb(pgprot, pid);

	vaddr = vmap(pages, count, VM_MAP, pgprot);
	if (!vaddr)
		return ERR_PTR(-ENOMEM);

	return vaddr;
}

static void lb_buffer_unmap(struct lb_buffer *lb)
{
	if (!lb)
		return;

	if (lb->va)
		vunmap(lb->va);

	if (lb->lbva)
		vunmap(lb->lbva);

	lb->va = NULL;
	lb->lbva = NULL;
}

static struct page *lb_pg_alloc(u32 pid, gfp_t gfp_mask, u32 order,
	u32 cnt, u32 alloc_type)
{
	if (alloc_type)
		return alloc_pages(gfp_mask, order);
	else
		return phys_to_page(g_region.base + GID_SZ * pid +
				cnt * PAGE_SIZE);
}

static void lb_pg_free(struct page *page, u32 order, u32 alloc_type)
{
	if (alloc_type)
		return __free_pages(page, order);
}

static void lb_buffer_free(struct lb_buffer *lb, u32 alloc_type)
{
	int i;
	struct page **tmp = NULL;
	struct page *p = NULL;

	lb_print(LB_INFO, "into %s line %d\n", __func__, __LINE__);

	if (!lb)
		return;

	if (lb->pid && lb->plc.reg.gid_allc_plc.gid_en) {
		cmo_ci_by_gid(lb->pid);
		gid_policy_reset(lb->pid);
	}

	lb_buffer_unmap(lb);

	tmp = lb->pages;
	if (!tmp) {
		for (i = 0; i < lb->count; i++) {
			p = *(tmp++);
			if (p)
				lb_pg_free(p, 0, alloc_type);
		}
	}

	if (lb->pages)
		vfree(lb->pages);

	kfree(lb);
}

static struct lb_buffer *lb_buffer_alloc(int pid, gfp_t gfp_mask, size_t sz,
	int flag, int alloc_type)
{
	int i;
	size_t count = sz >> PAGE_SHIFT;
	struct page **pages = NULL;
	struct page **tmp = NULL;
	struct page *p = NULL;
	struct lb_buffer *lb = NULL;

	lb = kzalloc(sizeof(*lb), GFP_KERNEL);
	if (!lb)
		goto err;

	pages = vzalloc(sizeof(struct page *) * count);
	if (!pages)
		goto err;

	lb->pages = pages;
	lb->count = count;
	lb->flag = flag;
	lb->pid = pid;
	lb->lbv = LB_DEFAULT_VALUE + pid;
	lb->ddrv = DDR_DEFAULT_VALUE + pid;

	tmp = pages;
	for (i = 0; i < count; i++) {
		p = lb_pg_alloc(pid, gfp_mask, 0, i, alloc_type);
		if (!p)
			goto err;
		*(tmp++) = p;
	}

	lb->va = lb_buffer_map(lb, 0);
	if (IS_ERR_OR_NULL(lb->va)) {
		lb_print(LB_ERROR, "lb buffer va map is failed\n");
		goto err;
	}

	lb->lbva = lb_buffer_map(lb, pid);
	if (IS_ERR_OR_NULL(lb->lbva)) {
		lb_print(LB_ERROR, "lb buffer lbva map is failed\n");
		goto err;
	}

	gid_cfg_init(lb);

	return lb;

err:
	lb_buffer_free(lb, alloc_type);

	return NULL;
}

static inline void print_result(const char *func, int id, int ret)
{
	char *str = (ret == SUCC) ? "SUCC" : "FAIL";

	lb_print(LB_ERROR, "%s case: %d tst %s\n", func, id, str);
}

static int lb_tst_gid_read_alloc(struct lb_buffer *lb, u32 plc_r, u32 quota)
{
	int ret = SUCC;

	if (plc_r != PLC_R_ALLC && plc_r != PLC_R_NALLC)
		return FAIL;

	lb->plc.reg.gid_allc_plc.nor_r_plc = plc_r;
	lb->quota.reg.gid_quota.quota_l = quota;
	lb->quota.reg.gid_quota.quota_h = quota;
	gid_policy_set(lb);

	mem_write(lb->va, lb->ddrv, SZ_2M);
	ret |= mem_check(lb->va, lb->ddrv, SZ_2M);

	gid_csz_show(lb->pid);
	mem_read(lb->lbva, SZ_2M);

	if (lb->plc.reg.gid_allc_plc.nor_r_plc == PLC_R_NALLC ||
	    !(lb->quota.value))
		ret |= gid_csz_check(lb->pid, 0);
	else
		ret |= gid_csz_check(lb->pid, SZ_2M);

	ret |= mem_check(lb->lbva, lb->ddrv, SZ_2M);

	return ret;
}

static int lb_tst_gid_read_inv_ci(struct lb_buffer *lb, u32 plc_r)
{
	int ret = SUCC;

	if (plc_r != PLC_R_INV && plc_r != PLC_R_CI)
		return FAIL;

	lb->plc.reg.gid_allc_plc.nor_r_plc = plc_r;
	gid_policy_set(lb);

	mem_write(lb->lbva, lb->lbv, SZ_1M);
	mem_write(lb->va, lb->ddrv, SZ_1M);
	ret |= mem_check(lb->va, lb->ddrv, SZ_1M);
	mem_read(lb->lbva, SZ_1M);

	ret |= gid_csz_check(lb->pid, 0);

	if (lb->plc.reg.gid_allc_plc.nor_r_plc == PLC_R_INV) {
		mem_check(lb->lbva, lb->ddrv, SZ_1M);
		ret |= mem_check(lb->va, lb->ddrv, SZ_1M);
	} else {
		mem_check(lb->lbva, lb->lbv, SZ_1M);
		ret |= mem_check(lb->va, lb->lbv, SZ_1M);
	}

	return ret;
}

static int lb_tst_gid_mid_alloc(struct lb_buffer *lb, u32 plc_r, u32 plc_w)
{
	int ret = SUCC;

	/* mid flt disable */
	lb->plc.reg.gid_allc_plc.nor_r_plc = PLC_R_NALLC;
	lb->plc.reg.gid_allc_plc.nor_w_plc = PLC_W_NALLC;
	lb->plc.reg.gid_allc_plc.flt_r_plc = plc_r;
	lb->plc.reg.gid_allc_plc.flt_w_plc = plc_w;
	gid_policy_set(lb);

	mem_write(lb->va, lb->ddrv, SZ_2M);
	mem_write(lb->lbva, lb->lbv, SZ_2M);
	ret |= gid_csz_check(lb->pid, 0);

	ret |= mem_check(lb->lbva, lb->lbv, SZ_2M);
	ret |= mem_check(lb->va, lb->lbv, SZ_2M);
	ret |= gid_csz_check(lb->pid, 0);

	/* mid flt enable */
	lb->flt0.reg.gid_mid_flt0.mid_flt0 = FCM_M0_MID;
	lb->flt0.reg.gid_mid_flt0.mid_flt0_msk = MID_MASK;
	lb->flt0.reg.gid_mid_flt0.mid_flt0_en = 1;
	lb->flt1.reg.gid_mid_flt1.mid_flt1 = FCM_M1_MID;
	lb->flt1.reg.gid_mid_flt1.mid_flt1_msk = MID_MASK;
	lb->flt1.reg.gid_mid_flt1.mid_flt1_en = 1;
	gid_policy_set(lb);

	if (plc_r == PLC_R_ALLC && plc_w == PLC_W_NALLC)
		mem_read(lb->lbva, SZ_2M);
	else if (plc_r == PLC_R_NALLC && plc_w == PLC_W_ALLC)
		mem_write(lb->lbva, lb->lbv, SZ_2M);
	else
		return FAIL;

	ret |= gid_csz_check(lb->pid, SZ_2M);
	mem_write(lb->va, lb->ddrv, SZ_2M);

	ret |= mem_check(lb->lbva, lb->lbv, SZ_2M);
	ret |= mem_check(lb->va, lb->ddrv, SZ_2M);

	return ret;
}

static int lb_tst_gid_replace(struct lb_buffer *lb, u32 rd, u32 wr)
{
	int ret = SUCC;

	if (!!rd == wr)
		return FAIL;

	lb->quota.reg.gid_quota.quota_l = QUOTA_2M;
	lb->quota.reg.gid_quota.quota_h = QUOTA_2M;
	lb->plc.reg.gid_allc_plc.sam_rplc = PLC_SAM_RPLC_DIS;
	gid_policy_set(lb);

	mem_write(lb->va, lb->ddrv, SZ_2M);
	mem_write(lb->lbva, lb->lbv, SZ_8M);
	ret |= gid_csz_check(lb->pid, SZ_2M);

	ret |= mem_check(lb->lbva, lb->lbv, SZ_8M);
	ret |= mem_check(lb->va, lb->ddrv, SZ_2M);

	/* same replace enable */
	lb->plc.reg.gid_allc_plc.sam_rplc = PLC_SAM_RPLC_EN;
	gid_policy_set(lb);

	if (rd)
		ret |= mem_check(lb->lbva, lb->lbv, SZ_8M);
	else
		mem_write(lb->lbva, lb->lbv, SZ_8M);

	ret |= gid_csz_check(lb->pid, SZ_2M);
	ret |= !mem_char_statistics(lb->va, lb->lbv, SZ_2M);

	return ret;
}

static int lb_tst_gid_wr_loop(struct lb_buffer *lb, u32 loop)
{
	int i;
	int ret = SUCC;

	mem_write(lb->va, lb->ddrv, SZ_8M);
	ret |= mem_check(lb->va, lb->ddrv, SZ_8M);

	for (i = 0; i < loop; i++) {
		memcpy(lb->lbva, (void *)((unsigned long)(lb->va) + SZ_4M),
			SZ_4M);
		if (mem_check(lb->lbva, lb->ddrv, SZ_4M)) {
			rdr_syserr_process_for_ap(MODID_AP_S_PANIC_LB, 0ULL,
				0ULL);
			return FAIL;
		}
	}

	return ret;
}

static int lb_tst_cmo_way_gid(struct lb_buffer *lb)
{
	int i;
	int ret = SUCC;
	u64 cmd;

	mem_write(lb->va, lb->ddrv, SZ_4M);
	mem_write(lb->lbva, lb->lbv, SZ_4M);
	ret |= mem_check(lb->lbva, lb->lbv, SZ_4M);
	ret |= mem_check(lb->va, lb->ddrv, SZ_4M);

	/*
	 * bit0     is cmo cmd
	 * bit1~3   is cmo type
	 * bit4~7   is cmo cmd type
	 * bit8~23  is gid bit map
	 * bit24~39 is way bit map
	 */
	cmd = (0x2ULL << 1) | (0x5 << 4) |
		((u64)BIT(lb->pid) << 8) | (0xff << 24);
	writeq(cmd, (lbdev->base + CMO_CMD));
	lb_cmo_sync_cmd_by_event();

	ret |= gid_csz_check(lb->pid, 0);
	ret |= mem_check(lb->va, lb->lbv, SZ_2M);

	return ret;
}

int lb_tst_single_gid_read(u32 id, u32 pid, u32 alloc_type, u32 loop)
{
	int ret;
	struct lb_buffer *lb = NULL;

	if (pid > GID_TST_CNT || loop > TST_LOOP_MAX) {
		lb_print(LB_ERROR, "%s invalid param %u %u\n",
			__func__, pid, loop);
		return FAIL;
	}

	lb = lb_buffer_alloc(pid, GFP_USER, GID_SZ,
		LB_NON_CACHEABLE, alloc_type);
	if (!lb) {
		lb_print(LB_ERROR, "lb buffer alloc is failed\n");
		goto err;
	}

	switch (id) {
	case 0: /* tst0 read when nor_r_plc = 0 */
		ret = lb_tst_gid_read_alloc(lb, PLC_R_NALLC, QUOTA_MAX);

		break;
	case 1: /* tst1 gid read */
		ret = lb_tst_gid_read_alloc(lb, PLC_R_ALLC, QUOTA_MAX);

		break;
	case 2: /* tst2 read when nor_r_plc = 1 and quota = 0 */
		ret = lb_tst_gid_read_alloc(lb, PLC_R_ALLC, 0);

		break;
	case 3: /* tst3 nor_r_plc = 2 read cache with invalid */
		ret = lb_tst_gid_read_inv_ci(lb, PLC_R_INV);

		break;
	case 4: /* tst4 nor_r_plc = 3 read cache with ci */
		ret = lb_tst_gid_read_inv_ci(lb, PLC_R_CI);

		break;
	case 5: /* tst5 mid read */
		ret = lb_tst_gid_mid_alloc(lb, PLC_R_ALLC, PLC_W_NALLC);

		break;
	case 6: /* tst6 rd replace sam_rplc_en = 0 */
		ret = lb_tst_gid_replace(lb, 1, 0);

		break;
	case 7: /* tst7 loop tst gid read/write */
		ret = lb_tst_gid_wr_loop(lb, loop);

		break;
	case 8: /* tst8 way + gid cmo */
		ret = lb_tst_cmo_way_gid(lb);

		break;
	default:
		lb_print(LB_ERROR, "invalid param%u\n", id);
		ret = FAIL;

		break;
	}

	print_result(__func__, id, ret);
	lb_buffer_free(lb, alloc_type);

	return ret;

err:
	return FAIL;
}

static int lb_tst_gid_write_alloc(struct lb_buffer *lb, u32 plc_w, u32 quota,
	u32 way_en, u32 sz_obj)
{
	int ret = SUCC;

	if (plc_w != PLC_W_ALLC && plc_w != PLC_W_NALLC)
		return FAIL;

	lb->plc.reg.gid_allc_plc.nor_w_plc = plc_w;
	lb->quota.reg.gid_quota.quota_l = quota;
	lb->quota.reg.gid_quota.quota_h = quota;
	lb->way.reg.gid_way_allc.gid_way_en = way_en;
	gid_policy_set(lb);

	gid_csz_show(lb->pid);
	mem_write(lb->lbva, lb->lbv, sz_obj);
	ret |= gid_csz_check(lb->pid, sz_obj);
	ret |= mem_check(lb->lbva, lb->lbv, sz_obj);

	return ret;
}

static int lb_tst_gid_write_nrefill(struct lb_buffer *lb, u32 plc_w)
{
	int ret = SUCC;

	if (plc_w != PLC_W_ALLC && plc_w != PLC_W_NREFILL)
		return FAIL;

	lb->plc.reg.gid_allc_plc.nor_w_plc = PLC_W_NREFILL;
	gid_policy_set(lb);

	mem_write(lb->va, lb->ddrv, SZ_2M);
	mem_write(lb->lbva, lb->lbv, SZ_2M - 1);
	ret |= gid_csz_check(lb->pid, SZ_2M);

	ret |= mem_check(lb->va, lb->ddrv, SZ_2M);
	ret |= mem_check(lb->lbva, lb->lbv, SZ_2M - 1);
	ret |= mem_check((void *)((unsigned long)(lb->lbva) + SZ_2M - 1),
			(plc_w == PLC_W_NREFILL) ? 0 : lb->ddrv, 1);

	return ret;
}

static int lb_tst_gid_row_hit(struct lb_buffer *lb)
{
	int ret = SUCC;

	lb->quota.reg.gid_quota.quota_l = 0;
	lb->quota.reg.gid_quota.quota_h = QUOTA_MAX;
	gid_policy_set(lb);

	mem_write(lb->lbva, lb->lbv, GID_SZ_MAX);
	mem_write(lb->va, lb->ddrv, GID_SZ_MAX);

	mem_write((void *)((unsigned long)lb->lbva + GID_SZ_MAX), lb->lbv, 0x1);

	ret |= mem_check(lb->lbva, lb->lbv, GID_SZ_MAX);
	ret |= !(mem_char_statistics(lb->va, lb->lbv, GID_SZ_MAX) >=
		MEM_CHAR_STAT);

	return ret;
}

static int lb_tst_scb(struct lb_buffer *lb, u32 span)
{
	int i;
	int ret = SUCC;

	lb->quota.reg.gid_quota.quota_l = 0;
	lb->quota.reg.gid_quota.quota_h = QUOTA_2M;
	gid_policy_set(lb);

	mem_write(lb->va, lb->ddrv, SZ_4M);

	for (i = 0; i < SZ_4M / span; i += SCB_STEP) {
		mem_write((void *)((unsigned long)lb->lbva +
			span * i), lb->lbv, span);
		mem_write((void *)((unsigned long)lb->lbva +
			span * (i + SCB_2_SHIFT)), lb->lbv, span);
		mem_write((void *)((unsigned long)lb->lbva +
			span * (i + SCB_5_SHIFT)), lb->lbv, span);
		mem_write((void *)((unsigned long)lb->lbva +
			span * (i + SCB_7_SHIFT)), lb->lbv, span);
	}

	ret |= gid_csz_check(lb->pid, SZ_2M);
	ret |= mem_check(lb->va, lb->ddrv, SZ_4M);

	return ret;
}

int lb_tst_single_gid_write(u32 id, u32 pid, u32 alloc_type, u32 span)
{
	struct lb_buffer *lb = NULL;
	int ret;

	if (pid > GID_TST_CNT || span < SCB_STEP || span > PAGE_SIZE) {
		lb_print(LB_ERROR, "invalid param %u %u\n", pid, span);
		return FAIL;
	}

	lb = lb_buffer_alloc(pid, GFP_USER, GID_SZ,
		LB_NON_CACHEABLE, alloc_type);
	if (!lb)
		goto err;

	switch (id) {
	case 0: /* tst0 write when nor_w_plc = 0 */
		ret = lb_tst_gid_write_alloc(lb, PLC_W_NALLC, QUOTA_MAX,
			ALL_WAY_EN, 0);
		break;
	case 1: /* tst1 write when nor_w_plc = 1 */
		ret = lb_tst_gid_write_alloc(lb, PLC_W_ALLC, QUOTA_MAX,
			ALL_WAY_EN, GID_SZ_MAX);
		break;
	case 2: /* tst2 write when nor_w_plc = 1 and quota = 0 */
		ret = lb_tst_gid_write_alloc(lb, PLC_W_ALLC, 0, ALL_WAY_EN, 0);
		break;
	case 3: /* tst3 nor_w_plc = 2 write not alloc */
		ret = lb_tst_gid_write_nrefill(lb, PLC_W_NREFILL);
		break;
	case 4: /* tst4 mid write */
		ret = lb_tst_gid_mid_alloc(lb, PLC_R_NALLC, PLC_W_ALLC);
		break;
	case 5: /* tst5 wr replace sam */
		ret = lb_tst_gid_replace(lb, 0, 1);
		break;
	case 6: /* tst6 alloc(gid_way_en) */
		ret = lb_tst_gid_write_alloc(
			lb, PLC_W_ALLC, QUOTA_MAX, PART_4_WAY_EN,
			(GID_SZ_MAX * PART_4_WAY_CNT) / WAY_NUM_MAX);
		break;
	case 7: /* tst7 scramble enable */
		ret = lb_tst_scb(lb, span);
		break;
	case 8: /* tst8 clean cache line when row-hit dirty cache line */
		ret = lb_tst_gid_row_hit(lb);
		break;
	default:
		lb_print(LB_ERROR, "invalid param%u\n", id);
		ret = FAIL;
		break;
	}

	print_result(__func__, id, ret);
	lb_buffer_free(lb, alloc_type);

	return ret;

err:
	return FAIL;
}

static int lb_tst_mutil_gid_plru(struct lb_buffer **lb, int cnt, int step)
{
	int i, j;
	int ret = SUCC;

	if (!lb || cnt < MGID_PLRU_CNT)
		return FAIL;

	for (i = 0; i < GID_SZ_MAX; i += step)
		for (j = 0; j < MGID_PLRU_CNT; j++)
			mem_write((void *)((unsigned long)(lb[j]->lbva) + i),
				lb[j]->lbv, step);

	ret |= !(gid_csz_show(lb[0]->pid) <= gid_csz_show(lb[1]->pid));
	ret |= mem_check(lb[0]->lbva, lb[0]->lbv, GID_SZ_MAX);
	ret |= mem_check(lb[1]->lbva, lb[1]->lbv, GID_SZ_MAX);

	return ret;
}

static int lb_tst_mutil_gid_alloc(struct lb_buffer **lb, int cnt,
	u32 quota, u32 prio, u32 sz)
{
	int ret = SUCC;

	if (!lb || cnt < MGID_CNT ||
		(prio != PLC_PRIO_00 && prio != PLC_PRIO_10))
		return FAIL;

	/* mutil gid alloc tst need three gid */
	lb[0]->quota.reg.gid_quota.quota_l = QUOTA_1M;
	lb[0]->quota.reg.gid_quota.quota_h = QUOTA_1M;
	gid_policy_set(lb[0]);

	lb[1]->quota.reg.gid_quota.quota_l = QUOTA_1M;
	lb[1]->quota.reg.gid_quota.quota_h = QUOTA_1M;
	gid_policy_set(lb[1]);

	lb[2]->quota.reg.gid_quota.quota_l = quota;
	lb[2]->quota.reg.gid_quota.quota_h = quota;
	lb[2]->plc.reg.gid_allc_plc.gid_prpty = prio;
	gid_policy_set(lb[2]);

	mem_write(lb[0]->lbva, lb[0]->lbv, SZ_1M);
	ret |= gid_csz_check(lb[0]->pid, SZ_1M);
	mem_write(lb[1]->lbva, lb[1]->lbv, SZ_1M);
	ret |= gid_csz_check(lb[1]->pid, SZ_1M);
	mem_write(lb[2]->lbva, lb[2]->lbv, sz);
	ret |= gid_csz_check(lb[2]->pid, sz);

	if (lb[2]->plc.reg.gid_allc_plc.gid_prpty == PLC_PRIO_10) {
		ret |= gid_csz_check(lb[0]->pid, 0);
		ret |= gid_csz_check(lb[1]->pid, 0);
	} else {
		ret |= gid_csz_check(lb[0]->pid, SZ_1M);
		ret |= gid_csz_check(lb[1]->pid, SZ_1M);
	}

	ret |= gid_csz_check(lb[2]->pid, sz);

	ret |= mem_check(lb[0]->lbva, lb[0]->lbv, SZ_1M);
	ret |= mem_check(lb[1]->lbva, lb[1]->lbv, SZ_1M);
	ret |= mem_check(lb[2]->lbva, lb[2]->lbv, sz);

	return ret;
}

static int lb_tst_mutil_gid_buffer_replace(struct lb_buffer **lb, int cnt)
{
	int ret = SUCC;

	if (!lb || cnt < MGID_CNT)
		return FAIL;

	/* mutil gid buffer replace tst need three gid */
	lb[0]->quota.reg.gid_quota.quota_l = QUOTA_1M;
	lb[0]->quota.reg.gid_quota.quota_h = QUOTA_1M;
	lb[0]->plc.reg.gid_allc_plc.gid_prpty = PLC_PRIO_11;
	gid_policy_set(lb[0]);

	lb[1]->quota.reg.gid_quota.quota_l = QUOTA_MAX;
	lb[1]->quota.reg.gid_quota.quota_h = QUOTA_MAX;
	gid_policy_set(lb[1]);

	lb[2]->quota.reg.gid_quota.quota_l = QUOTA_2M;
	lb[2]->quota.reg.gid_quota.quota_h = QUOTA_2M;
	lb[2]->plc.reg.gid_allc_plc.gid_prpty = PLC_PRIO_11;
	gid_policy_set(lb[2]);

	mem_write(lb[0]->lbva, lb[0]->lbv, SZ_1M);
	mem_write(lb[1]->lbva, lb[1]->lbv, GID_SZ_MAX);
	ret |= gid_csz_check(lb[0]->pid, SZ_1M);
	ret |= gid_csz_check(lb[1]->pid, GID_SZ_MAX - SZ_1M);

	lb[0]->plc.reg.gid_allc_plc.gid_prpty = PLC_PRIO_00;
	lb[0]->quota.reg.gid_quota.quota_l = 0;
	lb[0]->quota.reg.gid_quota.quota_h = 0;
	gid_policy_set(lb[0]);

	mem_write(lb[2]->lbva, lb[2]->lbv, SZ_2M);
	ret |= gid_csz_check(lb[2]->pid, SZ_2M);
	ret |= gid_csz_check(lb[1]->pid, GID_SZ_MAX - SZ_2M);
	ret |= gid_csz_check(lb[0]->pid, 0x0);

	ret |= mem_check(lb[0]->lbva, lb[0]->lbv, SZ_1M);
	ret |= mem_check(lb[1]->lbva, lb[1]->lbv, GID_SZ_MAX);
	ret |= mem_check(lb[2]->lbva, lb[2]->lbv, SZ_2M);

	return ret;
}

static int lb_tst_mutil_gid_cache_replace(struct lb_buffer **lb, int cnt)
{
	int ret = SUCC;

	if (!lb || cnt < MGID_CNT)
		return FAIL;

	/* mutil gid cache replace tst need three gid */
	lb[0]->quota.reg.gid_quota.quota_l = QUOTA_2M;
	lb[0]->quota.reg.gid_quota.quota_h = QUOTA_2M;
	lb[0]->plc.reg.gid_allc_plc.gid_prpty = PLC_PRIO_10;
	gid_policy_set(lb[0]);

	lb[1]->quota.reg.gid_quota.quota_l = QUOTA_MAX - QUOTA_2M;
	lb[1]->quota.reg.gid_quota.quota_h = QUOTA_MAX - QUOTA_2M;
	gid_policy_set(lb[1]);

	lb[2]->quota.reg.gid_quota.quota_l = QUOTA_2M;
	lb[2]->quota.reg.gid_quota.quota_h = QUOTA_2M;
	lb[2]->plc.reg.gid_allc_plc.gid_prpty = PLC_PRIO_10;
	gid_policy_set(lb[2]);

	mem_write(lb[0]->lbva, lb[0]->lbv, SZ_2M);
	mem_write(lb[1]->lbva, lb[1]->lbv, GID_SZ_MAX);
	ret |= gid_csz_check(lb[0]->pid, SZ_2M);
	ret |= gid_csz_check(lb[1]->pid, GID_SZ_MAX - SZ_2M);

	lb[0]->quota.reg.gid_quota.quota_l = QUOTA_1M;
	lb[0]->quota.reg.gid_quota.quota_h = QUOTA_1M;
	gid_policy_set(lb[0]);

	mem_write(lb[2]->lbva, lb[2]->lbv, SZ_1M);
	ret |= gid_csz_check(lb[2]->pid, SZ_1M);
	ret |= gid_csz_check(lb[1]->pid,
			     GID_SZ_MAX - SZ_2M - SLC_IDX * CACHE_LINE);
	ret |= gid_csz_check(lb[0]->pid, SZ_1M + SLC_IDX * CACHE_LINE);

	ret |= mem_check(lb[0]->lbva, lb[0]->lbv, SZ_2M);
	ret |= mem_check(lb[1]->lbva, lb[1]->lbv, GID_SZ_MAX);
	ret |= mem_check(lb[2]->lbva, lb[2]->lbv, SZ_1M);

	return ret;
}

static int lb_tst_mutil_gid_clean_cacheline_replace(struct lb_buffer **lb,
	int cnt)
{
	int ret = SUCC;

	if (!lb || cnt < MGID_CNT)
		return FAIL;

	/* mutil gid clean cacheline replace tst need three gid */
	lb[0]->quota.reg.gid_quota.quota_l = 0;
	lb[0]->quota.reg.gid_quota.quota_h = QUOTA_MAX;
	lb[0]->plc.reg.gid_allc_plc.gid_prpty = PLC_PRIO_01;
	gid_policy_set(lb[0]);

	lb[1]->quota.reg.gid_quota.quota_l = 0;
	lb[1]->quota.reg.gid_quota.quota_h = QUOTA_MAX;
	lb[1]->plc.reg.gid_allc_plc.gid_prpty = PLC_PRIO_01;
	gid_policy_set(lb[1]);

	lb[2]->quota.reg.gid_quota.quota_l = 0;
	lb[2]->quota.reg.gid_quota.quota_h = QUOTA_MAX;
	lb[2]->plc.reg.gid_allc_plc.gid_prpty = PLC_PRIO_10;
	gid_policy_set(lb[2]);

	mem_write(lb[0]->lbva, lb[0]->lbv, GID_SZ_HALF);
	mem_write(lb[1]->va, lb[1]->lbv, GID_SZ_HALF);
	mem_read(lb[1]->lbva, GID_SZ_HALF);

	mem_write(lb[2]->lbva, lb[2]->lbv, GID_SZ_HALF);
	ret |= gid_csz_check(lb[0]->pid, GID_SZ_HALF);
	ret |= gid_csz_check(lb[1]->pid, 0);
	ret |= gid_csz_check(lb[2]->pid, GID_SZ_HALF);

	ret |= mem_check(lb[0]->lbva, lb[0]->lbv, GID_SZ_HALF);
	ret |= mem_check(lb[1]->lbva, lb[1]->lbv, GID_SZ_HALF);
	ret |= mem_check(lb[2]->lbva, lb[2]->lbv, GID_SZ_HALF);

	return ret;
}

int lb_tst_mutil_gid(u32 id, u32 alloc_type, u32 cfg2, u32 cfg3)
{
	u32 i;
	int ret;
	struct lb_buffer *lb[MGID_CNT] = { NULL, NULL, NULL };

	/* mutil gid tst need three gid */
	for (i = 0; i < MGID_CNT; i++) {
		lb[i] = lb_buffer_alloc(i + 1, GFP_USER, GID_SZ,
			LB_NON_CACHEABLE, alloc_type);
		if (!lb[i])
			goto err;
	}

	switch (id) {
	case 0: /* tst0 plru */
		ret = lb_tst_mutil_gid_plru(lb, MGID_PLRU_CNT, GID_SZ_MAX);
		break;
	case 1: /* tst1 plru when write by turn */
		ret = lb_tst_mutil_gid_plru(lb, MGID_PLRU_CNT, CACHE_LINE);
		break;
	case 2: /* tst2 some gids allc */
		ret = lb_tst_mutil_gid_alloc(lb, MGID_CNT,
				QUOTA_1M + QUOTA_512K, PLC_PRIO_00,
				SZ_1M + SZ_512K);
		break;
	case 3: /* tst3 some gids grab */
		ret = lb_tst_mutil_gid_alloc(lb, MGID_CNT, QUOTA_MAX,
				PLC_PRIO_10, GID_SZ_MAX);
		break;
	case 4: /* tst4 some gids all replace */
		ret = lb_tst_mutil_gid_buffer_replace(lb, MGID_CNT);
		break;
	case 5: /* tst5 some gids replace */
		ret = lb_tst_mutil_gid_cache_replace(lb, MGID_CNT);
		break;
	case 6: /* tst6 check priority replace clean cache line */
		ret = lb_tst_mutil_gid_clean_cacheline_replace(lb, MGID_CNT);
		break;
	default:
		lb_print(LB_ERROR, "invalid param%u\n", id);
		ret = FAIL;
		break;
	}

	print_result(__func__, id, ret);

	for (i = 0; i < MGID_CNT; i++)
		lb_buffer_free(lb[i], alloc_type);

	return ret;

err:
	for (i = 0; i < MGID_CNT; i++)
		lb_buffer_free(lb[i], alloc_type);

	return FAIL;
}

int lb_tst_lock_buffer(u32 way_en, u32 sz, u32 alloc_type, u32 cfg3)
{
	u32 i;
	int ret = SUCC;
	struct lb_buffer *lb[LOCK_BUFFER_GID_CNT] = { NULL, NULL };

	/* lock buffer tst need two gid */
	for (i = 0; i < LOCK_BUFFER_GID_CNT; i++) {
		lb[i] = lb_buffer_alloc(i + 1, GFP_USER, GID_SZ,
				LB_NON_CACHEABLE, alloc_type);
		if (!lb[i]) {
			lb_print(LB_ERROR, "lb buffer alloc is failed\n");
			goto err;
		}
	}

	lb[0]->way.reg.gid_way_allc.gid_way_en = way_en;
	lb[0]->plc.reg.gid_allc_plc.gid_prpty = PLC_PRIO_11;
	gid_policy_set(lb[0]);

	mem_write(lb[0]->lbva, lb[0]->lbv, sz);
	mem_write(lb[0]->va, lb[0]->ddrv, sz);

	mem_write(lb[1]->lbva, lb[1]->lbv, GID_SZ_MAX);
	ret |= gid_csz_check(lb[0]->pid, sz);
	ret |= mem_check(lb[0]->lbva, lb[0]->lbv, sz);
	ret |= mem_check(lb[0]->va, lb[0]->ddrv, sz);
	ret |= mem_check(lb[1]->lbva, lb[1]->lbv, GID_SZ_MAX);

	print_result(__func__, 0, ret);

	for (i = 0; i < LOCK_BUFFER_GID_CNT; i++)
		lb_buffer_free(lb[i], alloc_type);

	return ret;

err:
	for (i = 0; i < LOCK_BUFFER_GID_CNT; i++)
		lb_buffer_free(lb[i], alloc_type);

	return FAIL;
}

static int lb_tst_pmu_interrupt(struct lb_buffer *lb, u32 id)
{
	if (id >= PMU_EVENT_CNT) {
		lb_print(LB_ERROR, "invalid param 0x%x\n", id);
		return FAIL;
	}

	return !(slc_int_st() & BIT(id));
}

static int lb_tst_dfx_interrupt(struct lb_buffer *lb, u32 id)
{
	int ret = SUCC;

	switch (id) {
	case 0: /* tst0 interrupt by dfx gid illegal */
		lb->plc.reg.gid_allc_plc.gid_en = PLC_GID_DIS;
		gid_policy_set(lb);

		mem_write(lb->lbva, lb->lbv, SZ_1M);

		ret |= !(slc_int_st() & DFX_GID_ILLEGAL);
		break;
	case 1: /* tst1 interrupt by dfx pri 11 noalloc */
		lb->quota.reg.gid_quota.quota_l = 0;
		lb->quota.reg.gid_quota.quota_h = 0;
		lb->plc.reg.gid_allc_plc.gid_prpty = PLC_PRIO_11;
		gid_policy_set(lb);

		mem_write(lb->lbva, lb->lbv, SZ_1M);
		cmo_ci_by_gid(lb->pid);

		ret |= !(slc_int_st() & DFX_PRI11_NOALLOC);
		break;
	case 2: /* tst2 interrupt by dfx cmo gid illegal */
		lb->plc.reg.gid_allc_plc.gid_en = PLC_GID_DIS;
		gid_policy_set(lb);

		cmo_ci_by_gid(lb->pid);

		ret |= !(slc_int_st() & DFX_CMOGID_ILLEGAL);
		break;
	default:
		lb_print(LB_ERROR, "invalid param 0x%x\n", id);
		ret = FAIL;
		break;
	}

	return ret;
}

static int lb_tst_cmo_interrupt(struct lb_buffer *lb, u32 id)
{
	int i;
	int ret = SUCC;

	if (!lbdev) {
		lb_print(LB_ERROR, "point is NULL\n");
		return FAIL;
	}

	switch (id) {
	case 0: /* tst0 interrupt by gid err */
		lb->way.reg.gid_way_allc.gid_way_srch = 0;
		gid_policy_set(lb);

		cmo_ci_by_gid(lb->pid);
		ret |= !(cmo_int_st() & CMO_BYGID_ERR_MASK);

		break;
	case 1: /* tst1 interrupt by way err */
		lb_ops_cache(EVENT, CI, CMO_BY_WAY, 0, 0, 0);

		/* ensure cmo ops complete */
		mb();

		lb_cmo_sync_cmd_by_event();

		ret |= !(cmo_int_st() & CMO_BYWAY_ERR_MASK);

		break;
	case 2: /* tst2 interrupt by overflow err */
		for (i = 0; i < CMO_OVERFLOW_CNT; i++)
			lb_ops_cache(EVENT, CI, CMO_BY_GID, BIT(lb->pid), 0, 0);

		/* ensure cmo ops complete */
		mb();

		lb_cmo_sync_cmd_by_event();
		ret |= !(cmo_int_st() & CMO_CMD_OVERFLOW_MASK);

		break;
	case 3: /* tst3 interrupt by cmo cmd access err */
		lb_ops_cache(EVENT, CI, CMO_BY_GID, BIT(lb->pid), 0, 0);
		writel(0, lbdev->base + CMO_CMD + INVALID_CMO_CMD_SHIFT);

		/* ensure cmo ops complete */
		mb();

		lb_cmo_sync_cmd_by_event();
		ret |= !(cmo_int_st() & CMO_CMD_ACCESS_ERR_MASK);

		break;
	default:
		lb_print(LB_ERROR, "invalid param 0x%x\n", id);
		ret = FAIL;
		break;
	}

	return ret;
}

int lb_tst_interrupt(u32 int_type, u32 id, u32 alloc_type, u32 cfg2)
{
	int ret;
	struct lb_buffer *lb = NULL;

	lb_print(LB_INFO, "into %s line %d %u\n", __func__, __LINE__, id);

	lb = lb_buffer_alloc(1, GFP_USER, GID_SZ, LB_NON_CACHEABLE, alloc_type);
	if (!lb) {
		lb_print(LB_ERROR, "lb buffer alloc is failed\n");
		goto err;
	}

	switch (int_type) {
	case 0: /* tst0 cmo interrupt */
		ret = lb_tst_cmo_interrupt(lb, id);
		break;
	case 1: /* tst1 dfx interrupt */
		ret = lb_tst_dfx_interrupt(lb, id);
		break;
	case 2: /* tst2 pmu interrupt */
		ret = lb_tst_pmu_interrupt(lb, id);
		break;
	default:
		lb_print(LB_ERROR, "invalid param 0x%x\n", id);
		ret = FAIL;
		break;
	}

	print_result(__func__, id, ret);

	/* clr lb interrupt */
	slc_int_clr();
	cmo_cfg_int_clr();

	lb_buffer_free(lb, alloc_type);

	lb_print(LB_INFO, "out %s %d\n", __func__, __LINE__);

	return ret;

err:
	lb_print(LB_INFO, "err %s %d\n", __func__, __LINE__);

	return FAIL;
}

int lb_tst_cmo_func(u32 id, u32 pid, u32 cfg2, u32 cfg3)
{
	u32 i;

	if (pid > GID_TST_CNT) {
		lb_print(LB_ERROR, "%s invalid param 0x%x\n", __func__, pid);
		return FAIL;
	}

	switch (id) {
	case CMO_BY_WAY:
		cmo_ci_by_way(ALL_WAY_EN);
		break;
	case CMO_BY_GID:
		cmo_ci_by_gid(pid);
		break;
	case CMO_BY_64PA:
	case CMO_BY_128PA:
	case CMO_BY_4XKPA:
		for (i = 0; i < g_region.size; i += CMO_PA_STEP) {
			lb_ops_cache(EVENT, CI, id, 0, i + g_region.base, CMO_PA_STEP);
			if (!(i % (CMO_SYNC_STEP * CMO_PA_STEP)))
				lb_cmo_sync_cmd_by_event();
		}

		/* ensure cmo ops complete */
		mb();

		lb_cmo_sync_cmd_by_event();

		break;
	default:
		lb_print(LB_ERROR, "invalid param 0x%x, 0x%x\n", id, pid);
		return -1;
	}

	return SUCC;
}

static int mpattern_check(void *va, u64 start, u64 end, u64 pattern, int v)
{
	u8 *p = NULL;
	u32 step;
	u64 i, tmp, i_start, i_end;

	switch (pattern) {
	case PA_31_7_EN:
		step = prandom_u32() % MPATTERN_STEP_5;
		i_start = 0;
		i_end = SZ_128;
		break;
	case PA_17_10_EN | PA_7_EN:
		step = prandom_u32() % MPATTERN_STEP_5;
		i_start = 0;
		i_end = SZ_512;
		break;
	case PA_17_7_EN:
	case PA_23_6_EN:
	case PA_20_6_EN:
	case PA_17_11_EN | PA_9_8_EN:
		step = prandom_u32() % MPATTERN_STEP;
		i_start = start;
		i_end = end;
		break;

	default:
		lb_print(LB_ERROR, "%s invalid 0x%x\n", __func__, pattern);
		return FAIL;
	}

	step = step ? step : 1;

	for (i = i_start; i < i_end; i += step) {
		if (pattern == (PA_17_10_EN | PA_7_EN))
			tmp = (((i & BIT(0)) << MPA_LOW_SHIFT) |
			       (start & ~pattern) |
			       ((i & PA_9_1_EN) << MPA_HIGH_SHIFT));
		else if (pattern == PA_23_6_EN || pattern == PA_20_6_EN)
			tmp = (i & pattern) | (start & ~pattern);
		else
			tmp = (i & ~pattern) | (start & pattern);

		if ((tmp - start) > (end - start) || tmp < start)
			tmp = start;

		p = (u8 *)((u64)va + (tmp - start));

		if (*p != v) {
			rdr_syserr_process_for_ap(MODID_AP_S_PANIC_LB,
				0ULL, 0ULL);

			return FAIL;
		}
	}

	return SUCC;
}

static void mpattern_gid_cfg(struct lb_buffer *lb)
{
	switch (lb->pid) {
	case 1: /* git1 config */
		lb->plc.reg.gid_allc_plc.sam_rplc = PLC_SAM_RPLC_EN;
		break;
	case 2: /* git2 config */
		lb->quota.reg.gid_quota.quota_l = QUOTA_1M;
		lb->quota.reg.gid_quota.quota_l = QUOTA_1M + QUOTA_512K;
		lb->plc.reg.gid_allc_plc.nor_r_plc = PLC_R_NALLC;
		break;
	case 3: /* git3 config */
		lb->quota.reg.gid_quota.quota_l = QUOTA_1M;
		lb->quota.reg.gid_quota.quota_l = QUOTA_1M + QUOTA_512K;
		lb->plc.reg.gid_allc_plc.nor_w_plc = PLC_W_NALLC;
		break;
	case 4: /* git4 config */
		lb->quota.reg.gid_quota.quota_l = QUOTA_1M;
		lb->quota.reg.gid_quota.quota_l = QUOTA_1M;
		lb->way.reg.gid_way_allc.gid_way_en = PART_4_WAY_EN;
		lb->plc.reg.gid_allc_plc.nor_r_plc = PLC_R_NALLC;
		lb->plc.reg.gid_allc_plc.nor_w_plc = PLC_W_NREFILL;
		lb->plc.reg.gid_allc_plc.gid_prpty = PLC_PRIO_11;
		break;
	default:
		lb_print(LB_ERROR, "invalid param: 0x%x\n", lb->pid);
		break;
	}

	gid_policy_set(lb);
}

int lb_tst_mutil_pattern(u32 pid, u32 loop, u32 sz, u32 alloc_type)
{
	struct lb_buffer *lb = NULL;
	u64 addr_start, addr_end;
	int ret = SUCC;
	int i;

	lb_print(LB_INFO, "into %s line %d\n", __func__, __LINE__);

	if (sz > GID_SZ || pid > MPATTERN_GID_CNT || loop > TST_LOOP_MAX) {
		lb_print(LB_ERROR, "0x%x %u %u invalid\n", sz, pid, loop);
		return FAIL;
	}

	lb = lb_buffer_alloc(pid, GFP_USER, GID_SZ, LB_NON_CACHEABLE,
			alloc_type);
	if (!lb) {
		lb_print(LB_ERROR, "lb buffer alloc is failed\n");
		goto err;
	}

	mpattern_gid_cfg(lb);

	addr_start = g_region.base + GID_SZ * pid;
	addr_end = addr_start + sz;

	for (i = 0; i < loop; i++) {
		lb->lbv = prandom_u32() % LBV_MASK;
		mem_write(lb->lbva, lb->lbv, sz);

		ret |= mpattern_check(lb->lbva, addr_start, addr_end,
				PA_17_7_EN, lb->lbv);
		ret |= mpattern_check(lb->lbva, addr_start, addr_end,
				PA_31_7_EN, lb->lbv);
		ret |= mpattern_check(lb->lbva, addr_start, addr_end,
				PA_17_10_EN | PA_7_EN, lb->lbv);
		ret |= mpattern_check(lb->lbva, addr_start, addr_end,
				PA_17_11_EN | PA_9_8_EN, lb->lbv);
		ret |= mpattern_check(lb->lbva, addr_start, addr_end,
				(pid == PID_3) ? PA_20_6_EN : PA_23_6_EN,
				lb->lbv);
	}

	print_result(__func__, pid, ret);
	lb_buffer_free(lb, alloc_type);

	lb_print(LB_INFO, "out %s line %d\n", __func__, __LINE__);

	return ret;

err:
	lb_print(LB_INFO, "err %s line %d\n", __func__, __LINE__);

	return -FAIL;
}

int lb_tst_exclusive(u32 loop, u32 sz, u32 cacheable, u32 alloc_type)
{
	struct lb_buffer *lb = NULL;
	int *tmp = NULL;
	int i, j;
	int sum = 0;

	lb_print(LB_INFO, "into%s line %d\n", __func__, __LINE__);

	if (sz > GID_SZ || loop > TST_LOOP_MAX) {
		lb_print(LB_ERROR, "0x%x %u invalid\n", sz, loop);
		return FAIL;
	}

	lb = lb_buffer_alloc(0, GFP_USER, GID_SZ, cacheable, alloc_type);
	if (!lb) {
		lb_print(LB_ERROR, "lb buffer alloc is failed\n");
		goto err;
	}

	mem_write(lb->va, 0, sz);
	for (i = 0; i < loop; i++) {
		for (j = 0; j < sz; j += SZ_256K) {
			tmp = (int *)((unsigned long)lb->va + j);
			if (*tmp != sum) {
				lb_print(LB_ERROR, "%s, 0x%x, 0x%x\n", __func__,
					*tmp, sum);
				rdr_syserr_process_for_ap(MODID_AP_S_PANIC_LB,
					0ULL, 0ULL);
				goto err;
			}

			(*tmp)++;
		}

		sum++;
	}

	gid_csz_show(0);
	lb_buffer_free(lb, alloc_type);

	lb_print(LB_INFO, "out %s line %d\n", __func__, __LINE__);

	return SUCC;

err:
	lb_print(LB_INFO, "err %s line %d\n", __func__, __LINE__);
	return FAIL;
}

int lb_tst_gid_bypass(u32 flag, u32 pid, u32 loop, u32 cfg3)
{
	int ret;

	if (pid > GID_TST_CNT || loop > TST_LOOP_MAX) {
		lb_print(LB_ERROR, "%s invalid param %u %u\n", __func__, pid,
			loop);
		return FAIL;
	}

	if (!lbdev) {
		lb_print(LB_ERROR, "point is NULL\n");
		return FAIL;
	}

	if (flag)
		ret = lb_gid_enable(pid);
	else
		ret = lb_gid_bypass(pid);

	return ret;
}

static struct sg_table *sg_table_alloc(u32 nents, u32 order)
{
	int i, ret;
	struct sg_table *table = NULL;
	struct list_head pages;
	struct scatterlist *sg = NULL;
	struct page *page = NULL;
	struct page *tmp_page = NULL;

	INIT_LIST_HEAD(&pages);

	for (i = 0; i < nents; i++) {
		page = alloc_pages(GFP_KERNEL, order);
		if (!page) {
			lb_print(LB_ERROR, "alloc pages %d failed\n", i);
			goto free_list_pages;
		}

		list_add_tail(&page->lru, &pages);
	}

	table = kzalloc(sizeof(*table), GFP_KERNEL);
	if (!table) {
		lb_print(LB_ERROR, "alloc table failed\n");
		goto free_list_pages;
	}

	ret = sg_alloc_table(table, nents, GFP_KERNEL);
	if (ret) {
		lb_print(LB_ERROR, "sg alloc table failed\n");
		goto free_table;
	}

	sg = table->sgl;
	list_for_each_entry_safe(page, tmp_page, &pages, lru) {
		sg_set_page(sg, page, PAGE_SIZE << order, 0);
		sg = sg_next(sg);
		list_del(&page->lru);
	}

	return table;

free_table:
	kfree(table);

free_list_pages:
	list_for_each_entry_safe(page, tmp_page, &pages, lru)
		__free_pages(page, order);

	return NULL;
}

static void sg_table_free(struct sg_table *table, u32 order)
{
	int i;
	struct scatterlist *sg = NULL;

	for_each_sg(table->sgl, sg, table->nents, i)
		__free_pages(sg_page(sg), order);

	sg_free_table(table);

	kfree(table);
}

static void *sg_table_map(struct sg_table *table,
	struct page **pgtable, u32 count, u32 pid)
{
	int i, j;
	pgprot_t prot;
	void *va = NULL;
	struct page *p = NULL;
	struct page **tmp = NULL;
	struct scatterlist *sg = NULL;

	tmp = pgtable;
	i = 0;
	for_each_sg(table->sgl, sg, table->nents, i) {
		p = sg_page(sg);
		for (j = 0; j < PAGE_ALIGN(sg->length) / PAGE_SIZE; j++)
			*(tmp++) = p++;
	}

	prot = PAGE_KERNEL;
	lb_pid_prot_build(pid, &prot);
	va = vmap(pgtable, count, VM_MAP, prot);

	return va;
}

int lb_tst_sg_attach(u32 pid, u32 sz, u32 order, u32 cfg3)
{
	int ret = 0;
	u32 count, nents, value;
	void *va = NULL;
	struct sg_table *table = NULL;
	struct page **pgtable = NULL;

	count = sz >> PAGE_SHIFT;
	nents = count >> order;

	if (pid > GID_TST_CNT || sz > LB_TST_SZ_MAX ||
		order > LB_TST_ORDER_MAX) {
		lb_print(LB_ERROR, "%s invalid param %u %u %u\n",
			__func__, pid, sz, order);
		return FAIL;
	}

	table = sg_table_alloc(nents, order);
	if (!table) {
		ret = FAIL;
		goto err;
	}

	if (lb_sg_attach(pid, table->sgl, nents)) {
		ret = FAIL;
		goto free_sgl;
	}

	pgtable = vmalloc(sizeof(struct page *) * count);
	if (!pgtable) {
		ret = FAIL;
		goto sg_detach;
	}

	value = prandom_u32() % CHAR_MASK;
	va = sg_table_map(table, pgtable, count, pid);
	mem_write(va, value, sz);
	if (mem_check(va, value, sz))
		rdr_syserr_process_for_ap(MODID_AP_S_PANIC_LB, 0ULL, 0ULL);

	vunmap(va);
	vfree(pgtable);

sg_detach:
	lb_sg_detach(pid, table->sgl, nents);

free_sgl:
	sg_table_free(table, order);

err:
	return ret;
}

int lb_tst_alloc_pages(u32 pid, u32 order, u32 cfg2, u32 cfg3)
{
	u32 gid_idx, value;
	u64 attr;
	void *va = NULL;
	struct page *pg = NULL;

	if (pid > GID_TST_CNT || order > LB_TST_ORDER_MAX) {
		lb_print(LB_ERROR, "%s invalid param %u %u\n",
			__func__, pid, order);
		return FAIL;
	}

	pg = lb_alloc_pages(pid, GFP_USER, order);
	if (!pg) {
		lb_print(LB_ERROR, "lb alloc pages failed\n");
		return FAIL;
	}

	gid_idx = lb_page_to_gid(pg);
	va = lb_page_to_virt(pg);
	if (!va) {
		lb_print(LB_ERROR, "get va failed\n");
		return FAIL;
	}

	value = prandom_u32() % CHAR_MASK;
	mem_write(va, value, PAGE_SIZE << order);
	gid_csz_check(gid_idx, 0);
	if (mem_check(va, value, PAGE_SIZE << order))
		rdr_syserr_process_for_ap(MODID_AP_S_PANIC_LB, 0ULL, 0ULL);

	value = prandom_u32() % CHAR_MASK;
	mem_write(va, value, PAGE_SIZE << order);
	gid_csz_show(gid_idx);
	if (mem_check(va, value, PAGE_SIZE << order))
		rdr_syserr_process_for_ap(MODID_AP_S_PANIC_LB, 0ULL, 0ULL);

	lb_print(LB_ERROR, "attr is 0x%lx\n", lb_pte_attr(page_to_phys(pg)));

	lb_free_pages(pid, pg, order);

	return SUCC;
}

int lb_tst_master_prio(u32 pid, u32 prio, u32 quota, u32 type)
{
	int ret;

	if (pid > GID_TST_CNT) {
		lb_print(LB_ERROR, "%s invalid param %u\n", __func__, pid);
		return FAIL;
	}

	if (!lbdev) {
		lb_print(LB_ERROR, "point is NULL\n");
		return FAIL;
	}

	switch (type) {
	case 0: /* tst0 set prio */
		ret = lb_set_master_policy(pid, quota, prio);
		break;
	case 1: /* tst1 reset prio */
		ret = lb_reset_master_policy(pid);
		break;
	case 2: /* tst2 get info */
		ret = lb_get_master_policy(pid, prio);
		break;
	default:
		lb_print(LB_ERROR, "invalid param 0x%x\n", type);
		ret = FAIL;
		break;
	}

	return ret;
}

static void mthread_cfg_print(u32 id)
{
	if (id > THREAD_CNT_MAX)
		return;

	/* print mthread set param */
	lb_print(LB_ERROR, "%s 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", __func__,
		id, mthread_cfg[id][0], mthread_cfg[id][1], mthread_cfg[id][2],
		mthread_cfg[id][3], mthread_cfg[id][4], mthread_cfg[id][5]);
}

static void mthread_result_init(u32 tcnt)
{
	int i;

	for (i = 0; i < tcnt; i++)
		g_mthread_result[i] = SUCC;

	/* g_thread_state 1: mutil thread, 0: single thread */
	if (tcnt > 1)
		g_thread_state = 1;
}

static int mthread_result(u32 tcnt)
{
	int i;
	int ret = SUCC;

	for (i = 0; i < tcnt; i++)
		ret |= g_mthread_result[i];

	if (tcnt > 1)
		g_thread_state = 0;

	return ret;
}

static struct lb_thread_func lbtstcmd[] = {
	/* id       func */
	{ 0, lb_tst_single_gid_read },
	{ 1, lb_tst_single_gid_write },
	{ 2, lb_tst_mutil_gid },
	{ 3, lb_tst_lock_buffer },
	{ 4, lb_tst_interrupt },
	{ 5, lb_tst_cmo_func },
	{ 6, lb_tst_mutil_pattern },
	{ 7, lb_tst_exclusive },
	{ 8, lb_tst_gid_bypass },
	{ 9, lb_tst_sg_attach },
	{ 10, lb_tst_alloc_pages },
	{ 11, lb_tst_master_prio },
};

static int sthread_tst(void *arg)
{
	int i;
	int cfg[FUNC_PARAM_CNT];
	u32 id;

	id = (u32)arg;

	for (i = 0; i < FUNC_PARAM_CNT; i++)
		cfg[i] = mthread_cfg[id][i];

	mthread_cfg_print(id);

	mutex_lock(&(lb_mutex[id]));

	for (i = 0; i < cfg[1]; i++) {
		/* set test cmd param */
		g_mthread_result[id] =
			lbtstcmd[cfg[0]].func(cfg[2], cfg[3], cfg[4], cfg[5]);
		if (g_mthread_result[id] != SUCC) {
			lb_print(LB_ERROR, "%s %d is failed\n", __func__, i);
			mthread_cfg_print(id);
			mutex_unlock(&(lb_mutex[id]));
			return FAIL;
		}
	}

	mutex_unlock(&(lb_mutex[id]));

	return SUCC;
}

/*
 * hava test
 * call the function
 * init mutil thread cfg
 */
int lb_mthread_cfg_init(u32 cmd, u32 cfg0, u32 cfg1, u32 cfg2, u32 cfg3)
{
	u32 id, tid, loop;
	int cmdmax;

	cmdmax = ARRAY_SIZE(lbtstcmd);
	id = cmd & CMD_ID_MASK;
	tid = (cmd >> CMD_TID_SHIFT) & CMD_TID_MASK;
	loop = (cmd >> CMD_LOOP_SHIFT) & CMD_LOOP_MASK;

	lb_print(LB_INFO, "%s 0x%x, 0x%x, %d\n", __func__, id, tid, cmdmax);

	if (id >= THREAD_CNT_MAX || tid >= cmdmax || loop >= TST_LOOP_MAX) {
		lb_print(LB_ERROR, "invalid param 0x%x, 0x%x\n", id, tid);
		return FAIL;
	}
	/* set mthread test param */
	mthread_cfg[id][0] = tid;
	mthread_cfg[id][1] = loop;
	mthread_cfg[id][2] = cfg0;
	mthread_cfg[id][3] = cfg1;
	mthread_cfg[id][4] = cfg2;
	mthread_cfg[id][5] = cfg3;

	return SUCC;
}

/*
 * hava test
 * call the function
 */
int lb_tst_mutil_thread(u32 tcnt)
{
	u64 i;
	int ret;

	struct task_struct *mthread[THREAD_CNT_MAX];

	if (tcnt > THREAD_CNT_MAX) {
		lb_print(LB_ERROR, "%s invalid param 0x%x\n", tcnt);
		return FAIL;
	}

	mthread_result_init(tcnt);

	for (i = 0; i < tcnt; i++)
		mthread[i] =
			kthread_run(sthread_tst, (void *)i, "thread-%lu", i);

	ret = mthread_result(tcnt);

	print_result(__func__, 0, ret);

	return ret;
}

/* lb dynamic power tst for mem init */
int lb_power_init_mem(u32 spid, u32 dpid, u32 flag)
{
	int i;
	pgprot_t pgprot;
	u32 acnt, scnt, dcnt;
	phys_addr_t spa, dpa;
	struct page **pages = NULL;
	struct page **tmp = NULL;

	if (spid > GID_TST_CNT || dpid > GID_TST_CNT) {
		lb_print(LB_ERROR, "%s invalid param %u %u\n",
			__func__, spid, dpid);
		return -1;
	}

	acnt = SZ_64M >> PAGE_SHIFT;
	scnt = SZ_60M >> PAGE_SHIFT;
	dcnt = SZ_4M >> PAGE_SHIFT;
	spa = g_region.base;
	dpa = spa + SZ_60M;

	pages = vmalloc(sizeof(struct page *) * acnt);
	if (!pages) {
		lb_print(LB_ERROR, "%s vmalloc failed\n",
			__func__);
		return -1;
	}

	tmp = pages;

	lb_pages_attach(spid, phys_to_page(spa), scnt);
	pgprot = flag ? pgprot_writecombine(PAGE_KERNEL) : PAGE_KERNEL;
	lb_prot_build(phys_to_page(spa), &pgprot);
	for (i = 0; i < scnt; i++)
		*(tmp++) = phys_to_page(spa + i * PAGE_SIZE);

	g_power_saddr = vmap(pages, scnt, VM_MAP, pgprot);
	if (!g_power_saddr) {
		lb_print(LB_ERROR, "%s saddr vmap failed\n",
			__func__);
		return -1;
	}

	lb_pages_attach(dpid, phys_to_page(dpa), dcnt);
	pgprot = flag ? pgprot_writecombine(PAGE_KERNEL) : PAGE_KERNEL;
	lb_prot_build(phys_to_page(dpa), &pgprot);
	tmp = pages;
	for (i = 0; i < dcnt; i++)
		*(tmp++) = phys_to_page(dpa + i * PAGE_SIZE);

	g_power_daddr = vmap(pages, dcnt, VM_MAP, pgprot);
	if (!g_power_saddr) {
		lb_print(LB_ERROR, "%s daddr vmap failed\n",
			__func__);
		return -1;
	}

	return 0;
}

/* lb dynamic power tst for mem free */
int lb_power_free_mem(u32 spid, u32 dpid, u32 flag)
{
	u32 scnt, dcnt;
	phys_addr_t spa, dpa;

	if (spid > GID_TST_CNT || spid > GID_TST_CNT) {
		lb_print(LB_ERROR, "%s invalid param %u %u\n",
			__func__, spid, dpid);
		return FAIL;
	}

	scnt = SZ_60M >> PAGE_SHIFT;
	dcnt = SZ_4M >> PAGE_SHIFT;
	spa = g_region.base;
	dpa = spa + SZ_60M;

	lb_pages_detach(spid, phys_to_page(spa), scnt);
	lb_pages_detach(dpid, phys_to_page(dpa), dcnt);
	return SUCC;
}

/* lb dynamic power tst for write tst */
int lb_power_tst_write(u32 loop, u32 sz)
{
	int i, j;

	if (loop > TST_LOOP_MAX || sz > SZ_64M) {
		lb_print(LB_ERROR, "%s invalid param %u %u\n",
			__func__, loop, sz);
		return FAIL;
	}

	for (i = 0; i < loop; i++)
		for (j = 0; j < sz; j += CACHE_LINE)
			memset((void *)((uintptr_t)g_power_saddr + j),
				0x1, CACHE_LINE);
	return SUCC;
}

/* lb dynamic power tst for read tst */
int lb_power_tst_read(u32 loop, u32 sz)
{
	int i, j;

	if (loop > TST_LOOP_MAX || sz > SZ_64M) {
		lb_print(LB_ERROR, "%s invalid param %u %u\n",
			__func__, loop, sz);
		return FAIL;
	}

	for (i = 0; i < loop; i++)
		for (j = 0; j < sz; j += CACHE_LINE)
			memcpy(g_power_daddr,
				(void *)((uintptr_t)g_power_saddr + j),
				CACHE_LINE);
	return SUCC;
}

/*
 * hava test
 * need call the function before call other test function
 */
int hisi_lb_tst_init(void)
{
	int i;

	g_reserve_buffer =
		lb_buffer_alloc(0, GFP_USER, SZ_1M, LB_NON_CACHEABLE, 0);
	if (!g_reserve_buffer)
		return -ENOMEM;

	for (i = 0; i < THREAD_CNT_MAX; i++)
		mutex_init(&(lb_mutex[i]));

	return SUCC;
}

/*
 * hava test
 * need call the function after call other test function
 */
int hisi_lb_tst_free(void)
{
	lb_buffer_free(g_reserve_buffer, 0);
	return SUCC;
}
