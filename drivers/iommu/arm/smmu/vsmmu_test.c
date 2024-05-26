/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2022. All rights reserved.
 *
 * Description:
 * smmu test for smmu driver implementations based on ARM architected
 * SMMU.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Create: 2022-1-8
 */

#include <linux/atomic.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dma-iommu.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/mm.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/syscalls.h>

#include "mm_virtio_iommu.h"
#include "ion.h"
#include "mm_ion_priv.h"

#define US_PER_MS     (1000)
#define SG_NENTS      (512)
#define E_SIZE		(48)
#define HEAP_ID_MASK	(1 << 0)
#define VIOMMU_TEST_PGSIZE		(0x20000)
#define PAGE_NUM_64MB	(0x4000000 / PAGE_SIZE)
#define LEAK_NENTS      (SG_NENTS * PAGE_SIZE / sizeof(unsigned long))

static long vsmmu_time_cost(ktime_t before_tv,
				ktime_t after_tv)
{
	long usdelta;
	long tms, tus;

	usdelta = ktime_us_delta(after_tv, before_tv);

	tms = usdelta / US_PER_MS;
	tus = usdelta % US_PER_MS;
	pr_err("this operation cost %ldms %ldus\n", tms, tus);

	return  usdelta;
}

static void __close_dmabuf_fd(int fd)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	ksys_close(fd);
#else
	sys_close(fd);
#endif
}

void test_vsmmu_map_dmabuf(size_t page_num)
{
	int fd;
	unsigned long iova1 = 0;
	unsigned long iova2 = 0;
	struct dma_buf *dmabuf = NULL;
	ktime_t before_tv1, after_tv1;
	ktime_t before_tv2, after_tv2;

	pr_err("%s start, page_num = %d\n", __func__, page_num);
	fd = ion_alloc(PAGE_SIZE * page_num, HEAP_ID_MASK, ION_FLAG_CACHED);
	if (fd < 0) {
		pr_err("%s %d: alloc ion buffer fail\n", __func__, __LINE__);
		return;
	}

	dmabuf = dma_buf_get(fd);
	if (IS_ERR(dmabuf)) {
		pr_err("%s %d: get dmabuf fail\n", __func__, __LINE__);
		goto free_fd;
	}

	before_tv1 = ktime_get();
	iova1 = viommu_map_dmabuf(0, dmabuf);
	after_tv1 = ktime_get();

	before_tv2 = ktime_get();
	iova2 = viommu_map_dmabuf(0, dmabuf);
	after_tv2 = ktime_get();

	vsmmu_time_cost(before_tv1, after_tv1);
	vsmmu_time_cost(before_tv2, after_tv2);

	pr_err("%s succ, iova1 = 0x%lx, iova2 = 0x%lx\n", __func__, iova1, iova2);

	dma_buf_put(dmabuf);
free_fd:
	__close_dmabuf_fd(fd);
}

void test_vsmmu_map_sg(void)
{
	struct sg_table *table = NULL;
	struct scatterlist *sg = NULL;
	struct page *page = NULL;
	unsigned long iova = 0;
	int nents;
	int i;

	pr_err("%s start\n", __func__);
	table = kmalloc(sizeof(*table), GFP_KERNEL);
	if (!table)
		return;

	if (sg_alloc_table(table, SG_NENTS, GFP_KERNEL))
		goto free_table;

	sg = table->sgl;
	for (i = 0; i < SG_NENTS; i++) {
		page = alloc_page(GFP_KERNEL);
		if (!page)
			goto free_page;
		sg_set_page(sg, page, PAGE_SIZE, 0);
		sg = sg_next(sg);
	}
	sg = table->sgl;
	iova = viommu_map_sg(SMMUID_TEST, sg);
	if (!iova)
		goto free_page;

	pr_err("%s succ, iova = 0x%lx\n", __func__, iova);

	viommu_unmap_sg(SMMUID_TEST, sg, iova);
free_page:
	nents = i;
	for_each_sg(table->sgl, sg, nents, i) {
		page = sg_page(sg);
		__free_page(page);
	}
	sg_free_table(table);
free_table:
	kfree(table);
}

/* iova leak test: map more than 3G iova */
void test_vsmmu_map_iova_leak(void)
{
	struct sg_table *table = NULL;
	struct scatterlist *sg = NULL;
	struct page *page = NULL;
	unsigned long iova[E_SIZE] = {0};
	int nents = PAGE_NUM_64MB;
	unsigned long size;
	int ret;
	int i, j;

	pr_err("%s start\n", __func__);
	table = kmalloc(sizeof(*table), GFP_KERNEL);
	if (!table)
		return;

	if (sg_alloc_table(table, nents, GFP_KERNEL))
		goto free_table;
	/* alloc 64MB sgl */
	sg = table->sgl;
	for (i = 0; i < nents; i++) {
		page = alloc_page(GFP_KERNEL);
		if (!page)
			goto free_page;
		sg_set_page(sg, page, PAGE_SIZE, 0);
		sg = sg_next(sg);
	}
	sg = table->sgl;

	size = nents * PAGE_SIZE;
	/* 48 * 64MB = 3072MB(3GB) */
	for (j = 0; j < E_SIZE; j++) {
		iova[j] = viommu_map_sg(SMMUID_TEST, sg);
		if (!iova[j]) {
			pr_err("%s, map fail, iova[%d]:0x%lx, size:0x%lx\n",
						__func__, j, iova[j], size);
			break;
		}
		pr_err("%s, map succ, iova[%d]:0x%lx, size:0x%lx\n",
						__func__, j, iova[j], size);
	}

	for (--j; j >= 0; j--) {
		ret = viommu_unmap_sg(SMMUID_TEST, sg, iova[j]);
		if (ret) {
			pr_err("%s, unmap fail, iova[%d]:0x%lx, size:0x%lx\n",
						__func__, j, iova[j], size);
			break;
		}
		pr_err("%s, unmap succ, iova[%d]:0x%lx, size:0x%lx\n",
						__func__, j, iova[j], size);
	}

	pr_err("%s end\n", __func__);

free_page:
	nents = i;
	for_each_sg(table->sgl, sg, nents, i) {
		page = sg_page(sg);
		__free_page(page);
	}
	sg_free_table(table);
free_table:
	kfree(table);
}

/* virtio fail test: add virtio queue entry more than 512 */
void test_vsmmu_virtio_fail(void)
{
	struct sg_table *table = NULL;
	struct scatterlist *sg = NULL;
	struct page *page = NULL;
	unsigned long iova = 0;
	int nents = LEAK_NENTS;
	int i;
	int ret = 0;

	pr_err("%s start\n", __func__);
	table = kmalloc(sizeof(*table), GFP_KERNEL);
	if (!table)
		return;

	if (sg_alloc_table(table, nents, GFP_KERNEL)) {
		pr_err("%s, sg_alloc_table\n", __func__);
		goto free_table;
	}

	sg = table->sgl;
	/* nents * 4KB = 512MB */
	for (i = 0; i < nents; i++) {
		page = alloc_page(GFP_KERNEL);
		if (!page)
			goto free_page;
		sg_set_page(sg, page, PAGE_SIZE, 0);
		sg = sg_next(sg);
	}
	sg = table->sgl;

	iova = viommu_map_sg(SMMUID_TEST, sg);
	if (!iova) {
		pr_err("%s, map_sg fail\n", __func__);
		goto free_page;
	}

	ret = viommu_unmap_sg(SMMUID_TEST, sg, iova);
	if (ret)
		pr_err("%s, unmap_sg fail\n", __func__);

free_page:
	for_each_sg(table->sgl, sg, nents, i) {
		page = sg_page(sg);
		__free_page(page);
	}
	sg_free_table(table);
free_table:
	kfree(table);
	pr_err("%s end\n", __func__);
}

int test_vsmmu_iova_to_phys(unsigned long iova, size_t iova_size)
{
	return 0;
}

