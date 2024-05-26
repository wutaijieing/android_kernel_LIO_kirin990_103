/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: HKIP Overload module_alloc & module_memfree
 * Create : 2019/12/5
 */

#include "hhee.h"
#include <linux/mm.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>
#include <linux/moduleloader.h>
#include <linux/module.h>
#include <asm-generic/getorder.h>

#define KO_START     0
#define KO_UPDATE    1
#define KO_FINISH    2
#define SHIFT_2M     21
#define MAX_KO_CNT   20
#define MAX_MEM_SIZE 10
#define KO_NEED_FREE 0x6C6C

/* define mutex for g_hhee_module */
static DEFINE_MUTEX(g_hhee_mutex);

struct hhee_module_msg {
	/* 20M max for each KO */
	struct page *pages[MAX_MEM_SIZE];
	void *virt;
	unsigned int need_free;
};

/* assume we have MAX=19 modules to load */
static struct hhee_module_msg g_hhee_module[MAX_KO_CNT];
static struct work_struct g_hhee_module_work;
struct workqueue_struct *g_hhee_response_wq = NULL;

/* ordered work, can be executed one work_item at a time */
static void hhee_module_memfree(struct work_struct *work)
{
	int j;
	unsigned int index;
	unsigned int order;
	void *virt = NULL;
	struct page *pages = NULL;

	for (index = 0; index < MAX_KO_CNT; index++) {
		if (g_hhee_module[index].need_free != KO_NEED_FREE)
			continue;
		virt = g_hhee_module[index].virt;
		if (!virt) {
			pr_err("index 0x%x, virt is NULL\n", index);
			return;
		}
		pr_err("%s, do free in %u.\n", __func__, index);
		vunmap(virt);

		order = get_order(SZ_2M);
		for (j = 0; j < MAX_MEM_SIZE; j++) {
			pages = g_hhee_module[index].pages[j];
			if (pages)
				__free_pages(pages, order);
			g_hhee_module[index].pages[j] = NULL;
		}
		g_hhee_module[index].need_free = 0;
		g_hhee_module[index].virt = NULL;
	}

	(void)work;
}

static void update_pages_list(struct page *pages, unsigned int index)
{
	int i;

	for (i = 0; i < MAX_MEM_SIZE; i++) {
		if (!g_hhee_module[index].pages[i])
			break;
	}
	if (i >= MAX_MEM_SIZE) {
		pr_err("keep: pages leakage warning\n");
		return;
	}
	g_hhee_module[index].pages[i] = pages;
}

static unsigned int store_curr_info(struct page *pages, unsigned int cmd,
				    unsigned int index, void *virt)
{
	int i = 0;

	switch (cmd) {
	case KO_FINISH:
		g_hhee_module[index].virt = virt;
		break;
	case KO_START:
		for (i = 0; i < MAX_KO_CNT; i++)
			if (!g_hhee_module[i].virt)
				break;
		break;
	case KO_UPDATE:
		update_pages_list(pages, index);
		break;
	default:
		break;
	}
	return i;
}

static unsigned int add_to_pages_list(struct page **pages, struct page *p,
				      unsigned long size, unsigned int index)
{
	unsigned int i;
	unsigned int count = index;
	unsigned int max = size >> PAGE_SHIFT;

	for (i = 0; i < max; i++) {
		pages[count] = p + i;
		count++;
	}
	return count;
}

static void *module_vmap(struct page **pages, unsigned long size)
{
	int ret;
	struct vm_struct *area;
	u64 module_alloc_end = module_alloc_base + MODULES_VSIZE;

	area = __get_vm_area_node(size, 1, VM_ALLOC | VM_NO_GUARD, module_alloc_base,
				  module_alloc_end, NUMA_NO_NODE, GFP_KERNEL,
				  __builtin_return_address(0));
	if (!area) {
		pr_err("hhee module alloc __get_vm_area_node failed\n");
		return NULL;
	}

	ret = map_kernel_range((unsigned long)area->addr, area->size, PAGE_KERNEL, pages);
	if (ret != 0) {
		vunmap(area->addr);
		pr_err("hhee module alloc map_kernel_range failed\n");
		return NULL;
	}

	return area->addr;
}

static void *module_alloc_mem_remap(unsigned long size, unsigned int count,
				    struct page **pages)
{
	int i;
	unsigned int order;
	unsigned int res;
	struct page *pages_tmp = NULL;
	unsigned int index = 0;
	void *virt = NULL;
	unsigned long alloc_size = roundup(size, SZ_2M);
	/* how many times we need to allocate 2M memory */
	unsigned int alloc_count = alloc_size >> SHIFT_2M;

	mutex_lock(&g_hhee_mutex);
	res = store_curr_info(NULL, KO_START, 0, NULL);
	if (res >= MAX_KO_CNT) {
		pr_err("g_hhee_module leakage warning\n");
		goto mutex_out;
	}

	/* how many 2M memory we need */
	order = get_order(SZ_2M);
	for (count = 0; count < alloc_count; count++) {
		pages_tmp = alloc_pages(GFP_KERNEL, order);
		if (!pages_tmp) {
			pages_tmp = alloc_pages(GFP_KERNEL | __GFP_NOFAIL, order);
			if (!pages_tmp)
				goto free_pages;
		}

		index = add_to_pages_list(pages, pages_tmp, SZ_2M, index);
		/* ignore the return value */
		(void)store_curr_info(pages_tmp, KO_UPDATE, res, NULL);
	}

	virt = module_vmap(pages, alloc_size);
	if (!virt) {
		pr_err("hhee module alloc module_vmap failed\n");
		goto free_pages;
	}

	/* ignore the return value */
	(void)store_curr_info(NULL, KO_FINISH, res, virt);
	pr_info("hhee module alloc count %u\n", res);
	mutex_unlock(&g_hhee_mutex);
	return virt;

free_pages:
	for (i = 0; i < MAX_MEM_SIZE; i++) {
		pages_tmp = g_hhee_module[res].pages[i];
		if (pages_tmp)
			__free_pages(pages_tmp, order);
		g_hhee_module[res].pages[i] = NULL;
	}
	pr_err("%s, alloc_pages failed\n", __func__);
mutex_out:
	mutex_unlock(&g_hhee_mutex);
	kfree(pages);
	return NULL;
}

/* the input size is aligned with 4K size */
void *module_alloc(unsigned long size)
{
	unsigned int count;
	/* get count align up with 2M first */
	unsigned long alloc_size;
	struct page **pages = NULL;
	void *virt = NULL;

	if (size == 0) {
		pr_err("%s: Invalid size = 0\n", __func__);
		return NULL;
	}
	alloc_size = roundup(size, SZ_2M);

	/* how many pages do we need */
	count = alloc_size >> PAGE_SHIFT;
	pages = kcalloc(count, sizeof(struct page *), GFP_KERNEL);
	if (!pages) {
		pr_err("%s: kcalloc(0x%lx) failed\n", __func__, size);
		return NULL;
	}

	virt = module_alloc_mem_remap(size, count, pages);

	kfree(pages);
	return virt;
}

void module_memfree(void *module_region)
{
	int i;

	if (!module_region)
		return;
	for (i = 0; i < MAX_KO_CNT; i++) {
		if (g_hhee_module[i].virt == module_region)
			break;
	}
	if (i >= MAX_KO_CNT) {
		pr_err("free: memory leakage warning\n");
		return;
	}
	g_hhee_module[i].need_free = KO_NEED_FREE;

	if (!g_hhee_response_wq) {
		pr_err("HHEE module_memfree failed\n");
		return;
	}
	queue_work(g_hhee_response_wq, &g_hhee_module_work);
}

void hhee_module_init(void)
{
	/* allocate an ordered worker queue */
	g_hhee_response_wq = create_singlethread_workqueue("hhee_module");
	if (!g_hhee_response_wq) {
		pr_err("Create HHEE work failed\n");
		return;
	}
	INIT_WORK(&g_hhee_module_work, hhee_module_memfree);
}
