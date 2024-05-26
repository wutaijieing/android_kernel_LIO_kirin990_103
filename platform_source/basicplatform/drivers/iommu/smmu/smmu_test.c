/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
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
 * Create: 2019-12-6
 */

#include <linux/atomic.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dma-iommu.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/iommu.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_iommu.h>
#include <linux/platform_device.h>
#include <linux/sizes.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/syscalls.h>
#include <linux/mm_iommu.h>

#include "mm_smmu.h"
#include "ion.h"
#include "mm_ion_priv.h"

static struct platform_device *test_pdev, *test_pdev2;
#define US_PER_MS     1000
#define SG_NENTS      6
#define SMMU_TEST_PA  0x10000000
#define SG_CHECK      64
#define HEAP_ID_MASK  (1 << 0)
#define PAD_SIZE_IN_TEST  0x10000
#define PAD_SIZE_OUT_TEST  0x20000

static long mm_smmu_time_cost(ktime_t before_tv,
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

static phys_addr_t mm_smmu_iova_to_phys(struct device *dev, dma_addr_t iova)
{
	struct iommu_domain *domain;

	domain = iommu_get_domain_for_dev(dev);
	if (!domain) {
		pr_err("Dev(%s) has no iommu domain!\n", dev_name(dev));
		return 0;
	}

	return iommu_iova_to_phys(domain, iova);
}

static unsigned long mm_smmu_map_sg_time(struct device *dev,
				struct scatterlist *sgl, int prot,
				unsigned long *out_size)
{
	unsigned long iova;
	ktime_t before_tv, after_tv;

	before_tv = ktime_get();
	iova = mm_iommu_map_sg(dev, sgl, prot, out_size);
	after_tv = ktime_get();
	mm_smmu_time_cost(before_tv, after_tv);
	pr_err("%s: map iova 0x%lx, size:0x%lx\n", __func__, iova, *out_size);

	return iova;
}

static int mm_iommu_unmap_sg_time(struct device *dev,
				struct scatterlist *sgl, unsigned long iova)
{
	ktime_t before_tv, after_tv;

	before_tv = ktime_get();
	mm_iommu_unmap_sg(dev, sgl, iova);
	after_tv = ktime_get();
	mm_smmu_time_cost(before_tv, after_tv);
	pr_err("%s: unmap iova 0x%lx\n", __func__, iova);

	return 0;
}

void test_smmu_map_sg(void)
{
	struct sg_table *table = NULL;
	struct scatterlist *sg = NULL;
	struct page *page = NULL;
	unsigned long iova = 0;
	unsigned long size;
	int i;
	int nents = 0;

	pr_err("into %s %d\n", __func__, __LINE__);
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
	iova = mm_smmu_map_sg_time(&test_pdev->dev,
		sg, IOMMU_READ | IOMMU_WRITE, &size);
	if (!iova)
		goto free_page;

	mm_iommu_unmap_sg_time(&test_pdev->dev, sg, iova);
	pr_err("%s succ\n", __func__);

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

static unsigned long map_dmabuf_with_time(struct platform_device *pdev,
				struct dma_buf *dmabuf)
{
	long tus;
	unsigned long size = 0;
	unsigned long iova;
	ktime_t before_tv, after_tv;
	int prot = IOMMU_READ | IOMMU_WRITE;

	before_tv = ktime_get();
	iova = kernel_iommu_map_dmabuf(&pdev->dev, dmabuf, prot, &size);
	after_tv = ktime_get();
	tus = mm_smmu_time_cost(before_tv, after_tv);
	pr_err("map size 0x%lx used %ldus\n", size, tus);
	if (!iova)
		pr_err("%s: map dmabuf fail\n", dev_name(&pdev->dev));

	return iova;
}

static void map_dmabuf_error_print(unsigned long *iov1, unsigned long *iov2,
			 struct dma_buf *dbuf)
{
	phys_addr_t pa;

	*iov1 = map_dmabuf_with_time(test_pdev, dbuf);
	pr_err("%s: iova 0x%lx\n", dev_name(&test_pdev->dev), *iov1);
	pa = mm_smmu_iova_to_phys(&test_pdev->dev, *iov1);
	pr_err("%s: pa 0x%lx\n", dev_name(&test_pdev->dev), pa);

	*iov2 = map_dmabuf_with_time(test_pdev2, dbuf);
	pr_err("%s: iova2 0x%lx\n", dev_name(&test_pdev2->dev), *iov2);
	pa = mm_smmu_iova_to_phys(&test_pdev2->dev, *iov2);
	pr_err("%s: pa 0x%lx\n", dev_name(&test_pdev2->dev), pa);

	if (*iov1 != *iov2)
		pr_err("test iova refs fail\n");
}

static void unmap_dmabuf_error_print(unsigned long iova, unsigned long iova2,
			 struct dma_buf *dbuf)
{
	int ret;
	long tus;
	ktime_t before_tv, after_tv;

	before_tv = ktime_get();
	ret = kernel_iommu_unmap_dmabuf(&test_pdev->dev, dbuf, iova);
	after_tv = ktime_get();
	if (ret)
		pr_err("%s: map undmabuf fail\n", dev_name(&test_pdev->dev));

	ret = kernel_iommu_unmap_dmabuf(&test_pdev2->dev, dbuf, iova2);
	if (ret)
		pr_err("%s: map undmabuf fail\n", dev_name(&test_pdev2->dev));

	tus = mm_smmu_time_cost(before_tv, after_tv);
	pr_err("unmap dmabuf used %ldus\n", tus);
}

void test_smmu_map_dmabuf(size_t len)
{
	int fd;
	unsigned long iova = 0;
	unsigned long iova2 = 0;
	struct dma_buf *dmabuf = NULL;
	struct mm_domain *mm_domain = NULL;
	struct iommu_domain *domain = NULL;

	pr_err("into %s %d\n", __func__, __LINE__);
	fd = ion_alloc(len, HEAP_ID_MASK, ION_FLAG_CACHED);
	if (fd < 0) {
		pr_err("%s %d: alloc ion buffer fail\n", __func__, __LINE__);
		return;
	}

	dmabuf = dma_buf_get(fd);
	if (IS_ERR(dmabuf)) {
		pr_err("%s %d: get dmabuf fail\n", __func__, __LINE__);
		goto free_fd;
	}

	domain = iommu_get_domain_for_dev(&test_pdev->dev);
	if (!domain) {
		pr_err("dev has no iommu domain!\n");
		goto free_dmabuf;
	}
	mm_domain = to_mm_domain(domain);

	map_dmabuf_error_print(&iova, &iova2, dmabuf);
	mm_iova_dom_info(&test_pdev->dev);
	unmap_dmabuf_error_print(iova, iova2, dmabuf);
	pr_err("%s succ\n", __func__);

free_dmabuf:
	dma_buf_put(dmabuf);
free_fd:
	__close_dmabuf_fd(fd);
}

void test_smmu_unmap_sg_check(size_t len)
{
	unsigned long iova[SG_CHECK];
	unsigned long size = 0;
	int fd[SG_CHECK];
	int prot = IOMMU_READ | IOMMU_WRITE;
	struct dma_buf *dmabuf[SG_CHECK];
	struct scatterlist *sgl = NULL;
	int i;
	int ret = 0;

	pr_err("into %s %d\n", __func__, __LINE__);
	for (i = 0; i < SG_CHECK; i++) {
		fd[i] = -1;
		dmabuf[i] = NULL;
	}

	for (i = 0; i < SG_CHECK; i++) {
		fd[i] = ion_alloc(len, HEAP_ID_MASK, ION_FLAG_CACHED);
		if (fd[i] < 0) {
			pr_err("%s %d: alloc ion buffer fail, i = %d\n",
				__func__, __LINE__, i);
			goto free;
		}

		dmabuf[i] = dma_buf_get(fd[i]);
		if (IS_ERR_OR_NULL(dmabuf[i])) {
			pr_err("%s %d: get dmabuf fail, i = %d\n",
				__func__, __LINE__, i);
			goto free;
		}

		iova[i] = kernel_iommu_map_dmabuf(&test_pdev->dev, dmabuf[i],
			prot, &size);
	}

	sgl = ((struct ion_buffer *)dmabuf[SG_CHECK - 1]->priv)->sg_table->sgl;
	ret = mm_iommu_unmap_sg(&test_pdev->dev, sgl, iova[SG_CHECK - 1]);
	if (ret)
		pr_err("%s %d: mm_iommu_unmap_sg ret = %d\n",
			__func__, __LINE__, ret);

	for (i = 0; i < SG_CHECK; i++) {
		ret = kernel_iommu_unmap_dmabuf(&test_pdev->dev, dmabuf[i],
			iova[i]);
		if (ret)
			pr_err("%s %d: map undmabuf fail, i = %d\n",
				__func__, __LINE__, i);
	}
	pr_err("%s test end\n", __func__);

free:
	for (i = 0; i < SG_CHECK; i++) {
		if (!IS_ERR_OR_NULL(dmabuf[i]))
			dma_buf_put(dmabuf[i]);
		if (fd[i] > 0)
			__close_dmabuf_fd(fd[i]);
	}
}

void test_smmu_padding_size_check(size_t len)
{
	struct dma_buf *dmabuf = NULL;
	unsigned long iova;
	unsigned long size = 0;
	int prot = IOMMU_READ | IOMMU_WRITE;
	int fd   = -1;
	int ret  = 0;

	fd = ion_alloc(len, HEAP_ID_MASK, ION_FLAG_CACHED);
	if (fd < 0) {
		pr_err("%s %d: alloc ion buffer fail!\n",
				__func__, __LINE__);
		return;
	}

	dmabuf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(dmabuf)) {
		pr_err("%s: get dmabuf fail!\n", __func__);
		goto free_fd;
	}

	iova = kernel_iommu_map_padding_dmabuf(&test_pdev->dev,
		dmabuf, PAD_SIZE_IN_TEST, prot, &size);
	if (!iova) {
		pr_err("%s: map padding size dmabuf fail!\n", __func__);
		goto free_dbuf;
	}

	ret = kernel_iommu_unmap_padding_dmabuf(&test_pdev->dev, dmabuf,
				PAD_SIZE_OUT_TEST, iova);
	if (ret) {
		pr_err("%s: unmap padding_size_out dmabuf fail,ret = %d!\n",
			__func__, ret);

		ret = kernel_iommu_unmap_padding_dmabuf(&test_pdev->dev, dmabuf,
				PAD_SIZE_IN_TEST, iova);
		if (!ret)
			pr_err("%s: unmap padding_size_in dmabuf succ,ret = %d!\n",
			__func__, ret);
	}
	pr_err("%s: test end\n", __func__);

free_dbuf:
	dma_buf_put(dmabuf);
free_fd:
	__close_dmabuf_fd(fd);
}

static void map_dmabuf_print_dom_pte(unsigned long iov1, unsigned long iov2,
				struct dma_buf *dbuf1, struct dma_buf *dbuf2)
{
	int ret;
	unsigned long size;
	int prot = IOMMU_READ | IOMMU_WRITE;

	iov1 = kernel_iommu_map_dmabuf(&test_pdev->dev, dbuf1, prot, &size);
	if (!iov1) {
		pr_err("%s: map dmabuf fail\n", dev_name(&test_pdev->dev));
		return;
	}
	pr_err("%s:iova 0x%lx\n", dev_name(&test_pdev->dev), iov1);

	iov2 = kernel_iommu_map_dmabuf(&test_pdev->dev, dbuf2, prot, &size);
	if (!iov2) {
		pr_err("%s: map dmabuf2 fail\n", dev_name(&test_pdev->dev));
		return;
	}
	pr_err("%s:iova2 0x%lx\n", dev_name(&test_pdev->dev), iov2);

#if !defined(CONFIG_ARM_SMMU_V3) && !defined(CONFIG_MM_SMMU_V3)
	mm_print_iova_dom(&test_pdev->dev);
#endif
	ret = kernel_iommu_unmap_dmabuf(&test_pdev->dev, dbuf1, iov1);
	pr_err("return %d after unmap dmabuf1.\n", ret);
	if (ret)
		pr_err("%s: map dmabuf fail\n", dev_name(&test_pdev->dev));

	ret = kernel_iommu_unmap_dmabuf(&test_pdev->dev, dbuf2, iov2);
	pr_err("return %d after unmap dmabuf2.\n", ret);
	if (ret)
		pr_err("%s: map dmabuf2 fail\n", dev_name(&test_pdev->dev));
}

static void test_smmu_print_dom_pte(void)
{
	int fd;
	int fd2;
	struct dma_buf *dmabuf = NULL;
	struct dma_buf *dmabuf2 = NULL;
	unsigned long iova = 0;
	unsigned long iova2 = 0;
	struct iommu_domain *domain = NULL;

	pr_err("into %s %d\n", __func__, __LINE__);
	fd = ion_alloc(SZ_64K, HEAP_ID_MASK, ION_FLAG_CACHED);
	if (fd < 0) {
		pr_err("alloc ion buffer fail\n");
		return;
	}

	dmabuf = dma_buf_get(fd);
	if (IS_ERR(dmabuf)) {
		pr_err("get dmabuf fail\n");
		return;
	}

	fd2 = ion_alloc(SZ_64K, HEAP_ID_MASK, ION_FLAG_CACHED);
	if (fd2 < 0) {
		pr_err("alloc ion buffer 2 fail\n");
		return;
	}

	dmabuf2 = dma_buf_get(fd2);
	if (IS_ERR(dmabuf)) {
		pr_err("get dmabuf2 fail\n");
		return;
	}

	domain = iommu_get_domain_for_dev(&test_pdev->dev);
	if (!domain) {
		pr_err("dev has no iommu domain!\n");
		return;
	}

	map_dmabuf_print_dom_pte(iova, iova2, dmabuf, dmabuf2);

	dma_buf_put(dmabuf);
	dma_buf_put(dmabuf2);
	__close_dmabuf_fd(fd);
	__close_dmabuf_fd(fd2);
	pr_err("%s succ!!\n", __func__);
}

void test_smmu_map(size_t size)
{
	struct iommu_domain *domain =
		iommu_get_domain_for_dev(&test_pdev->dev);
	unsigned long iova;
	ktime_t before_tv, after_tv;
	int prot = IOMMU_READ | IOMMU_WRITE;

	pr_err("into %s %d\n", __func__, __LINE__);
	if (!domain) {
		pr_err("%s:domian is NULL\n", __func__);
		return;
	}

	before_tv = ktime_get();
	iova = mm_iommu_map(&test_pdev->dev, SMMU_TEST_PA, size, prot);
	after_tv = ktime_get();
	mm_smmu_time_cost(before_tv, after_tv);
	if (!iova) {
		pr_err("iommu_map fail\n");
		return;
	}
	pr_err("%s:map ova is 0x%lx, size 0x%lx\n", __func__, iova, size);

	before_tv = ktime_get();
	if (mm_iommu_unmap(&test_pdev->dev, iova, size))
		pr_err("iommu_unmap_phy fail\n");

	after_tv = ktime_get();
	mm_smmu_time_cost(before_tv, after_tv);
	pr_err("%s:map ova is 0x%lx\n", __func__, iova);
	pr_err("%s succ\n", __func__);
}

static void test_smmu_print_pte(void)
{
	struct iommu_domain *domain =
		iommu_get_domain_for_dev(&test_pdev->dev);
	unsigned long iova;
	size_t size = SZ_64K;
	int prot = IOMMU_READ | IOMMU_WRITE;
	int ret;

	if (!domain) {
		pr_err("%s:domian is NULL\n", __func__);
		return;
	}

	iova = mm_iommu_map(&test_pdev->dev, SMMU_TEST_PA, size, prot);
	if (!iova) {
		pr_err("iommu_map fail\n");
		return;
	}
	pr_err("%s: iova is 0x%lx\n", __func__, iova);
#ifndef CONFIG_ARM_SMMU_V3
	mm_smmu_show_pte(&test_pdev->dev, iova, size);
#endif
	ret = mm_iommu_unmap(&test_pdev->dev, iova, size);
	if (ret) {
		pr_err("iommu_unmap_phy fail\n");
		return;
	}

	pr_err("print_pte succ\n");
}

void test_smmu_dma_release(size_t len)
{
	int fd;
	unsigned long iova, iova2;
	ktime_t before_tv, after_tv;
	struct dma_buf *dmabuf = NULL;

	pr_err("into %s %d\n", __func__, __LINE__);
	fd = ion_alloc(len, HEAP_ID_MASK, ION_FLAG_CACHED);
	if (fd < 0) {
		pr_err("%s %d: alloc ion buffer fail\n", __func__, __LINE__);
		return;
	}

	dmabuf = dma_buf_get(fd);
	if (IS_ERR(dmabuf)) {
		pr_err("%s %d: get dmabuf fail\n", __func__, __LINE__);
		__close_dmabuf_fd(fd);
		return;
	}

	map_dmabuf_error_print(&iova, &iova2, dmabuf);
	before_tv = ktime_get();
	dmabuf_release_iommu(dmabuf);
	after_tv = ktime_get();
	mm_smmu_time_cost(before_tv, after_tv);
	dma_buf_put(dmabuf);
	__close_dmabuf_fd(fd);
	pr_err("%s succ\n", __func__);
}

void test_smmu(void)
{
	struct iommu_group *dev_group = NULL;
	struct iommu_domain *domain = NULL;

	if (!test_pdev || !test_pdev2) {
		pr_err("test fail! device NULL! dts is not suitable\n");
		return;
	}

	dev_group = iommu_group_get(&test_pdev->dev);
	domain = iommu_get_domain_for_dev(&test_pdev->dev);

	if (!dev_group) {
		pr_err("test fail! dev_group NULL\n");
		return;
	}

	if (!domain) {
		pr_err("test fail! domian is NULL\n");
		return;
	}

	pr_err("%s %d:start\n", __func__, __LINE__);
	test_smmu_map_sg();
	test_smmu_map(SZ_32M);
	test_smmu_map_dmabuf(SZ_32M);
	test_smmu_print_pte();
	test_smmu_print_dom_pte();
	pr_err("%s %d:end\n", __func__, __LINE__);
}

static int mm_smmu_test_probe(struct platform_device *pdev)
{
	if (!test_pdev) {
		test_pdev = pdev;
		pr_err("%s %d:test dev1\n", __func__, __LINE__);
	} else {
		test_pdev2 = pdev;
		pr_err("%s %d:test dev2\n", __func__, __LINE__);
	}
	dma_set_mask_and_coherent(&pdev->dev, ~0ULL);
	pr_err("dev %pK (%s)\n", &pdev->dev, dev_name(&pdev->dev));

	return 0;
}

static int mm_smmu_test_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id mm_smmu_test_of_match[] = {
	{.compatible = "mm,smmu_test" },
	{},
};
MODULE_DEVICE_TABLE(of, mm_smmu_test_of_match);

static struct platform_driver mm_smmu_test_driver = {
	.driver = {
			.owner = THIS_MODULE,
			.name = "smmu-test",
			.of_match_table = of_match_ptr(mm_smmu_test_of_match),
		},
	.probe = mm_smmu_test_probe,
	.remove = mm_smmu_test_remove,
};

static int __init mm_smmu_test_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&mm_smmu_test_driver);
	return ret;
}

static void __exit mm_smmu_test_exit(void)
{
	platform_driver_unregister(&mm_smmu_test_driver);
}

module_init(mm_smmu_test_init);
module_exit(mm_smmu_test_exit);
