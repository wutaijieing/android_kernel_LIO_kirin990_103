/*
 * drivers/staging/android/ion/ion_dma_pool_heap.c
 *
 * Copyright(C) 2001-2019 Hisilicon Technologies Co., Ltd. All rights reserved.
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

#define pr_fmt(fmt) "[dma_pool_heap]" fmt

#include <linux/atomic.h>
#include <linux/cma.h>
#include <linux/dma-contiguous.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/genalloc.h>
#include <linux/platform_drivers/mm_ion.h>
#include <linux/io.h>
#include <linux/ion.h>
#include <linux/mm.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/time.h>
#include <linux/vmalloc.h>
#include <linux/version.h>
#include <linux/workqueue.h>

#include <asm/cacheflush.h>
#include <asm/tlbflush.h>

#include "mm_ion_priv.h"
#include "mm/sec_alloc.h"
#include "mm/ion_sec_contig.h"
#include "mm/ion_tee_op.h"
#include "ion.h"
#ifdef CONFIG_MM_CMA_DEBUG
#include <linux/platform_drivers/mm_cma_debug.h>
#endif

/*
 * Why pre-allocation size is 64MB?
 * 1.The time of 64MB memory cma releasing is short
 * 2.64MB memory is larger then iris's memory size
 */

/* align must be modify with count */
#define PREALLOC_ALIGN   14U
#define PREALLOC_CNT     (64UL * SZ_1M / PAGE_SIZE)
#define PREALLOC_NWK     (PREALLOC_CNT * 4U)

#define DMA_CAMERA_WATER_MARK (512 * SZ_1M)
#define MM_ION_MAX_LATENCY	(20 * 1000) /* 20ms */

#define POOL_SZ_2M	(21U)

static void pre_alloc_wk_func(struct work_struct *work);

struct ion_dma_pool_heap {
	struct ion_heap heap;
	struct gen_pool *pool;
	struct device	*dev;
	phys_addr_t base;
	size_t size;
	atomic64_t alloc_size;
	atomic64_t prealloc_cnt;
};
struct cma *ion_dma_camera_cma = NULL;
static struct ion_dma_pool_heap *ion_dma_camera_heap;
static DECLARE_WORK(ion_pre_alloc_wk, pre_alloc_wk_func);

struct cma *mm_camera_pool = NULL;

static int mm_camera_pool_set_up(struct reserved_mem *rmem)
{
	phys_addr_t align = PAGE_SIZE << max(MAX_ORDER - 1, pageblock_order);
	phys_addr_t mask = align - 1;
	unsigned long node = rmem->fdt_node;
	struct cma *cma = NULL;
	int err;
	pr_err("%s \n", __func__);
	if (!of_get_flat_dt_prop(node, "reusable", NULL) ||
	    of_get_flat_dt_prop(node, "no-map", NULL))
		return -EINVAL;
	if ((rmem->base & mask) || (rmem->size & mask)) {
		pr_err("Reserved memory: incorrect alignment of CMA region\n");
		return -EINVAL;
	}

	err = cma_init_reserved_mem(rmem->base, rmem->size, 0, rmem->name, &cma);
	if (err) {
		pr_err("Reserved memory: unable to setup CMA region\n");
		return err;
	}
#ifdef CONFIG_MM_CMA_DEBUG
	cma_set_flag(cma, node);
#endif
	mm_camera_pool = cma;
	pr_err("%s done!\n", __func__);
	return 0;
}

RESERVEDMEM_OF_DECLARE(mm_camera_pool, "mm-camera-pool", mm_camera_pool_set_up);

void ion_register_dma_camera_cma(void *p)
{
	struct cma *cma = (struct cma *)p;
	if (cma
	    && cma_get_size(cma) >= DMA_CAMERA_WATER_MARK) {
		ion_dma_camera_cma = cma;
		pr_info("register_dma_camera_cma is ok\n");
	} else {
		pr_err("ion_dma_camera_cma is is not regested\n");
	}
}

void ion_clean_dma_camera_cma(void)
{
	phys_addr_t addr;
	unsigned long size_remain = 0;
#ifdef ION_DMA_POOL_DEBUG
	int free_count = 0;
	unsigned long t_ns = 0;
	ktime_t t1 = ktime_get();
#endif
	if (!ion_dma_camera_heap)
		return;

	if(!atomic64_sub_and_test(0L, &ion_dma_camera_heap->alloc_size))
		return;

	while (!!(addr = gen_pool_alloc(ion_dma_camera_heap->pool, PAGE_SIZE))) {
		if (ion_dma_camera_cma) {
			if (cma_release(ion_dma_camera_cma,  phys_to_page(addr), 1)) {
				atomic64_sub(1, &ion_dma_camera_heap->prealloc_cnt);
#ifdef ION_DMA_POOL_DEBUG
				free_count++;
#endif
			} else {
				pr_err("cma release failed\n");
			}
		}

		/* here is mean the camera is start again */
		if (!atomic64_sub_and_test(0L, &ion_dma_camera_heap->alloc_size)) {
			pr_err("@free memory camera is start again, alloc sz %lx\n",
					atomic64_read(&ion_dma_camera_heap->alloc_size));
			break;
		}
	}

	size_remain = gen_pool_avail(ion_dma_camera_heap->pool);
	if (!!size_remain)
		pr_err("out %s, size_remain = 0x%lx\n", __func__, size_remain);

	pr_info("quit %s,prealloc_cnt now:%ld\n",
		__func__, atomic64_read(&ion_dma_camera_heap->prealloc_cnt));

#ifdef ION_DMA_POOL_DEBUG
	t_ns = ktime_to_ns(ktime_sub(ktime_get(), t1));
	pr_err("cma free size %lu B time is %lu us\n",
		free_count * PAGE_SIZE, t_ns / 1000UL);
#endif
}

static void pre_alloc_wk_func(struct work_struct *work)
{
	struct page *page = NULL;
	phys_addr_t addr;
#ifdef CONFIG_DFX_KERNELDUMP
	struct page *t_page = NULL;
	int k;
#endif
#ifdef ION_DMA_POOL_DEBUG
	unsigned long t_ns = 0;
	ktime_t t1 = ktime_get();
#endif

	if (!ion_dma_camera_heap)
		return;

	if ((unsigned long)atomic64_read(&ion_dma_camera_heap->prealloc_cnt) > PREALLOC_NWK)
		return;
	page = cma_alloc(ion_dma_camera_cma, PREALLOC_CNT, PREALLOC_ALIGN, GFP_KERNEL);
	if (page) {
		addr = page_to_phys(page);
		memset(phys_to_virt(addr), 0x0, PREALLOC_CNT * PAGE_SIZE); /* unsafe_function_ignore: memset  */
#ifdef CONFIG_DFX_KERNELDUMP
		t_page = page;
		for (k = 0; k < (int)PREALLOC_CNT; k++) {
			SetPageMemDump(t_page);
			t_page++;
		}
#endif
		atomic64_add(PREALLOC_CNT, &ion_dma_camera_heap->prealloc_cnt);
		gen_pool_free(ion_dma_camera_heap->pool,
			      page_to_phys(page),
			      PREALLOC_CNT * PAGE_SIZE);

#ifdef ION_DMA_POOL_DEBUG
		t_ns = ktime_to_ns(ktime_sub(ktime_get(), t1));
		pr_err("cma alloc size %lu B time is %lu us\n",
			PREALLOC_CNT * PAGE_SIZE, t_ns / 1000);
#endif
		pr_info("enter %s,prealloc_cnt now:%ld\n",
			__func__, atomic64_read(&ion_dma_camera_heap->prealloc_cnt));

		return;
	}
}

static phys_addr_t ion_dma_pool_allocate(struct ion_heap *heap,
					unsigned long size,
					unsigned long align,
					unsigned long flags)
{
	unsigned long offset = 0;
	struct ion_dma_pool_heap *dma_pool_heap =
		container_of(heap, struct ion_dma_pool_heap, heap);

	if (!dma_pool_heap)
		return (phys_addr_t)-1L;

	offset = gen_pool_alloc(dma_pool_heap->pool, size);
	if (!offset) {
		if ((heap->id == ION_CAMERA_DAEMON_HEAP_ID)
			/*
			 * When the camera can only use 7/16 of all CMA size,
			 * the watermark its looks ok.
			 * v    wm    used   maxchun
			 * NA   NA    400M
			 * 1/2  304M  360M   160M
			 * 1/3  200M  260M   300M
			 * 3/8  228M  300M   240M
			 * 7/16 266M  340M   256M
			 */
			&& (atomic64_read(&dma_pool_heap->alloc_size) < dma_pool_heap->size / 16 * 7))/*lint !e574*/
			schedule_work(&ion_pre_alloc_wk);
		return (phys_addr_t)-1L;
	}
	atomic64_add(size, &dma_pool_heap->alloc_size);
	return offset;
}

static void ion_dma_pool_free(struct ion_heap *heap, phys_addr_t addr,
		       unsigned long size)
{
	struct ion_dma_pool_heap *dma_pool_heap =
		container_of(heap, struct ion_dma_pool_heap, heap);

	if (addr == (phys_addr_t)-1L)
		return;

	gen_pool_free(dma_pool_heap->pool, addr, size);
	atomic64_sub(size, &dma_pool_heap->alloc_size);

	if (atomic64_sub_and_test(0L, &dma_pool_heap->alloc_size))
		ion_clean_dma_camera_cma();
}

static int ion_dma_pool_heap_allocate(struct ion_heap *heap,
	struct ion_buffer *buffer, unsigned long size, unsigned long flags)
{
	struct sg_table *table = NULL;
	struct page *page = NULL;
	phys_addr_t paddr;
	int ret;
	ktime_t _stime, _time1, _etime;
	s64 _timedelta;

	_stime = ktime_get();
	table = kmalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!table)
		return -ENOMEM;

	ret = sg_alloc_table(table, 1, GFP_KERNEL);
	if (ret)
		goto err_free;

	paddr = ion_dma_pool_allocate(heap, size, 0, flags);
	if (paddr == (phys_addr_t)-1L) {
		ret = -ENOMEM;
		goto err_free_table;
	}
	_time1 = ktime_get();
	page = pfn_to_page(PFN_DOWN(paddr));
	sg_set_page(table->sgl, page, (unsigned int)size, (unsigned int)0);
	buffer->sg_table = table;

	if (heap->id == ION_CAMERA_DAEMON_HEAP_ID)
		(void)ion_heap_pages_zero(page, size,
				pgprot_writecombine(PAGE_KERNEL));
	_etime = ktime_get();
	_timedelta = ktime_us_delta(_etime, _stime);
	if (_timedelta >= MM_ION_MAX_LATENCY)
		pr_err("[%s] , size:0x%lx, cost:%lld us, memalloc cost:%lld us\n",
			__func__, size, _timedelta,
			ktime_us_delta(_time1, _stime));

	return 0;

err_free_table:
	sg_free_table(table);
err_free:
	kfree(table);
	return ret;
}

static void ion_dma_pool_heap_free(struct ion_buffer *buffer)
{
	struct ion_heap *heap = buffer->heap;
	struct platform_device *mm_ion_dev = get_mm_ion_platform_device();
	struct sg_table *table = buffer->sg_table;
	struct page *page = NULL;
	struct scatterlist *sg = NULL;
	phys_addr_t paddr = 0;
	u32 i;

	ion_heap_buffer_zero(buffer);

	if (buffer->flags & ION_FLAG_CACHED)
		dma_sync_sg_for_device(&mm_ion_dev->dev, table->sgl,
			(int)table->nents, DMA_BIDIRECTIONAL);

	for_each_sg(table->sgl, sg, table->nents, i) {
		page = sg_page(sg);
		paddr = PFN_PHYS(page_to_pfn(page));
		ion_dma_pool_free(heap, paddr, sg->length);
	}
	sg_free_table(table);
	kfree(table);
}

int ion_dma_pool_heap_map_user(struct ion_heap *heap,
		struct ion_buffer *buffer, struct vm_area_struct *vma)
{
	if (buffer->flags & ION_FLAG_SECURE_BUFFER)
		return -EINVAL;

	return ion_heap_map_user(heap, buffer, vma);
}

void *ion_dma_pool_heap_map_kern(struct ion_heap *heap,
		struct ion_buffer *buffer)
{
	if (buffer->flags & ION_FLAG_SECURE_BUFFER)
		return ERR_PTR(-EINVAL);

	return ion_heap_map_kernel(heap, buffer);
}

void ion_dma_pool_heap_unmap_kern(struct ion_heap *heap,
		struct ion_buffer *buffer)
{
	if (buffer->flags & ION_FLAG_SECURE_BUFFER)
		return;

	ion_heap_unmap_kernel(heap, buffer);
}

static struct ion_heap_ops dma_pool_heap_ops = {
	.allocate = ion_dma_pool_heap_allocate,
	.free = ion_dma_pool_heap_free,
	.map_user = ion_dma_pool_heap_map_user,
	.map_kernel = ion_dma_pool_heap_map_kern,
	.unmap_kernel = ion_dma_pool_heap_unmap_kern,
};

static struct ion_heap *ion_dynamic_dma_pool_heap_create(
		struct ion_platform_heap *heap_data)
{
	struct ion_dma_pool_heap *dma_pool_heap =
		kzalloc(sizeof(struct ion_dma_pool_heap), GFP_KERNEL);
	if (!dma_pool_heap) {
		pr_err("%s, out of memory whne kzalloc \n", __func__);
		return ERR_PTR(-ENOMEM);
	}

	dma_pool_heap->heap.ops = &dma_pool_heap_ops;
	dma_pool_heap->heap.type = ION_HEAP_TYPE_DMA_POOL;
	dma_pool_heap->heap.flags = ION_HEAP_FLAG_DEFER_FREE;
	if (!ion_dma_camera_cma)
		goto alloc_err;
	dma_pool_heap->base = cma_get_base(ion_dma_camera_cma);
	dma_pool_heap->size = cma_get_size(ion_dma_camera_cma);
	dma_pool_heap->pool = gen_pool_create(PAGE_SHIFT, -1);
	if (!dma_pool_heap->pool)
		goto alloc_err;
	if (gen_pool_add(dma_pool_heap->pool,
			 dma_pool_heap->base,
			 dma_pool_heap->size,
			 -1))
		goto pool_create_err;
	if (!gen_pool_alloc(dma_pool_heap->pool, dma_pool_heap->size))
		goto pool_create_err;

	atomic64_set(&dma_pool_heap->alloc_size, 0);
	atomic64_set(&dma_pool_heap->prealloc_cnt, 0);

	ion_dma_camera_heap = dma_pool_heap;

	heap_data->size = dma_pool_heap->size;
	heap_data->base = dma_pool_heap->base;

	return &dma_pool_heap->heap;

pool_create_err:
	gen_pool_destroy(dma_pool_heap->pool);
alloc_err:
	kfree(dma_pool_heap);

	return ERR_PTR(-ENOMEM);
}

void ion_pages_sync_for_device(struct device *dev, struct page *page,
		size_t size, enum dma_data_direction dir)
{
	struct scatterlist sg;
	struct platform_device *mm_ion_dev = get_mm_ion_platform_device();

	sg_init_table(&sg, 1);
	sg_set_page(&sg, page, size, 0);
	/*
	 * This is not correct - sg_dma_address needs a dma_addr_t that is valid
	 * for the targeted device, but this works on the currently targeted
	 * hardware.
	 */
	sg_dma_address(&sg) = page_to_phys(page);
	dma_sync_sg_for_device(&mm_ion_dev->dev, &sg, 1, dir);
}

struct ion_heap *ion_static_dma_pool_heap_create(struct ion_platform_heap *heap_data)
{
	struct ion_dma_pool_heap *dma_pool_heap = NULL;
	struct page *page = NULL;
#ifdef CONFIG_DFX_KERNELDUMP
	struct page *t_page = NULL;
	int k;
#endif
	int ret;
	size_t size;

	if (!mm_camera_pool) {
		pr_err("%s, mm_camera_pool not found!\n", __func__);
		return ERR_PTR(-ENOMEM);
	}

	size = cma_get_size(mm_camera_pool);
	page = cma_alloc(mm_camera_pool, size >> PAGE_SHIFT, 0, GFP_KERNEL);
	if (!page) {
		pr_err("%s, mm_camera_pool cma_alloc out of memory!\n", __func__);
		return ERR_PTR(-ENOMEM);
	}

#ifdef CONFIG_DFX_KERNELDUMP
	t_page = page;
	for (k = 0; k < (int)(size >> PAGE_SHIFT); k++) {
		SetPageMemDump(t_page);
		t_page++;
	}
#endif

	ion_pages_sync_for_device(NULL, page, size, DMA_BIDIRECTIONAL);

	ret = ion_heap_pages_zero(page, size, pgprot_writecombine(PAGE_KERNEL));
	if (ret) {
		pr_err("%s, mm_camera_pool zero fail, error: 0x%x\n", __func__, ret);
		return ERR_PTR(ret);
	}

	dma_pool_heap = kzalloc(sizeof(struct ion_dma_pool_heap), GFP_KERNEL);
	if (!dma_pool_heap) {
		pr_err("%s, mm_camera_pool kzalloc out of memory!\n", __func__);
		return ERR_PTR(-ENOMEM);
	}

	dma_pool_heap->pool = gen_pool_create(PAGE_SHIFT, -1);
	if (!dma_pool_heap->pool) {
		kfree(dma_pool_heap);
		pr_err("%s, mm_camera_pool gen_pool_create fail!\n", __func__);
		return ERR_PTR(-ENOMEM);
	}

	dma_pool_heap->base = page_to_phys(page);
	dma_pool_heap->size = size;
	gen_pool_add(dma_pool_heap->pool,
			dma_pool_heap->base,
			dma_pool_heap->size,
			-1);
	dma_pool_heap->heap.ops = &dma_pool_heap_ops;
	dma_pool_heap->heap.type = ION_HEAP_TYPE_DMA_POOL;
	dma_pool_heap->heap.flags = ION_HEAP_FLAG_DEFER_FREE;
	atomic64_set(&dma_pool_heap->alloc_size, 0);
	atomic64_set(&dma_pool_heap->prealloc_cnt, 0);

	heap_data->size = size;
	heap_data->base = page_to_phys(page);

	return &dma_pool_heap->heap;
}

struct ion_heap *ion_dma_pool_heap_create(struct ion_platform_heap *heap_data)
{
	if (heap_data->id == ION_CAMERA_DAEMON_HEAP_ID)
		return ion_dynamic_dma_pool_heap_create(heap_data);
	else
		return ion_static_dma_pool_heap_create(heap_data);
}
