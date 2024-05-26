/*
 * Copyright(C) 2019-2020 Hisilicon Technologies Co., Ltd. All rights reserved.
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

#define pr_fmt(fmt) "mm_iommu_dma: " fmt

#include <linux/acpi.h>
#include <linux/cache.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 4, 0))
#include <linux/dma-contiguous.h>
#endif
#include <linux/dma-iommu.h>
#include <linux/dma-mapping.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#include <linux/dma-map-ops.h>
#endif
#include <linux/export.h>
#include <linux/genalloc.h>
#include <linux/gfp.h>
#include <linux/mm_iommu.h>
#include <linux/iommu.h>
#include <linux/pci.h>
#include <linux/vmalloc.h>

#include <asm/cacheflush.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 4, 0))
#include <linux/bootmem.h>
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
#include <linux/dma-direct.h>
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
#include <linux/dma-noncoherent.h>
#endif
#include <linux/dma-iommu.h>

#define MM_DMA_IOMMU_MAPPING_ERROR 0

static inline bool dev_is_dma_coherent_common(struct device *dev)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	return dev_is_dma_coherent(dev);
#else
	return is_device_dma_coherent(dev);
#endif
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
static inline void *dma_iommu_alloc(struct device *dev, size_t size,
				dma_addr_t *dma_handle, gfp_t flags,
				unsigned long attrs)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	return arch_dma_alloc(dev, size, dma_handle, flags, attrs);
#else
	return dma_direct_alloc(dev, size, dma_handle, flags, attrs);
#endif
}

static inline void dma_iommu_free(struct device *dev, size_t size,
				void *vaddr, dma_addr_t dma_handle,
				unsigned long attrs)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	arch_dma_free(dev, size, vaddr, dma_handle, attrs);
#else
	dma_direct_free(dev, size, vaddr, dma_handle, attrs);
#endif
}
#endif

static int mm_dma_info_to_prot(enum dma_data_direction dir, bool coherent,
				unsigned long attrs)
{
	int prot = coherent ? IOMMU_CACHE : 0;

	if (attrs & DMA_ATTR_PRIVILEGED)
		prot |= IOMMU_PRIV;

	switch (dir) {
	case DMA_BIDIRECTIONAL:
		return prot | IOMMU_READ | IOMMU_WRITE;
	case DMA_TO_DEVICE:
		return prot | IOMMU_READ;
	case DMA_FROM_DEVICE:
		return prot | IOMMU_WRITE;
	default:
		break;
	}
	return 0;
}

static phys_addr_t mm_iommu_iova_to_phys(struct device *dev, dma_addr_t iova)
{
	struct iommu_domain *domain = NULL;

	domain = iommu_get_domain_for_dev(dev);
	if (!domain) {
		dev_err(dev, "%s has no iommu domain!\n", __func__);
		return 0;
	}

	return iommu_iova_to_phys(domain, iova);
}

static void *mm_iommu_iova_to_va(struct device *dev, dma_addr_t iova)
{
	return phys_to_virt(mm_iommu_iova_to_phys(dev, iova));
}

static void *mm_iommu_dma_alloc(struct device *dev, size_t size,
			 dma_addr_t *iova, gfp_t gfp,
			 unsigned long attrs)
{
	int ioprot;
	void *addr = NULL;
	phys_addr_t paddr;
	unsigned long iova_temp;
	dma_addr_t dma_handle;
	size_t iosize = PAGE_ALIGN(size);
	bool coherent = dev_is_dma_coherent_common(dev);

	if (WARN(!dev, "cannot map IOMMU mapping for unknown device\n"))
		return NULL;

	ioprot = mm_dma_info_to_prot(DMA_BIDIRECTIONAL, coherent, attrs);

	addr = dma_iommu_alloc(dev, iosize, &dma_handle, gfp, attrs);
	if (!addr) {
		dev_err(dev, "%s alloc fail:size:%zx, %#x, attr:%lx\n",
			__func__, size, gfp, attrs);
		return NULL;
	}
	paddr = dma_to_phys(dev, dma_handle);

	iova_temp = mm_iommu_map(dev, paddr, iosize, ioprot);
	if (!iova_temp) {
		dma_iommu_free(dev, size, addr, dma_handle, attrs);
		dev_err(dev, "%s map iommu fail:size:%zx, %#x, attr:%lx\n",
			__func__, size, gfp, attrs);
		return NULL;
	}
	*iova = iova_temp;

	return addr;
}

static void mm_iommu_dma_free(struct device *dev, size_t size,
				void *vaddr, dma_addr_t iova,
				unsigned long attrs)
{
	size_t iosize = PAGE_ALIGN(size);
	dma_addr_t dma_handle;

	if (WARN(!dev, "cannot unmap IOMMU mapping for unknown device\n"))
		return;

	if (WARN(!vaddr, "cannot unmap IOMMU mapping for null pointer\n"))
		return;

	dma_handle = phys_to_dma(dev, mm_iommu_iova_to_phys(dev, iova));

	if (mm_iommu_unmap(dev, iova, iosize)) {
		dev_err(dev, "%s unmap:va:%pK,iova:%llx,size:%zx,iosize:%zx\n",
		    __func__, vaddr, iova, size, iosize);
		return;
	}

	dma_iommu_free(dev, size, vaddr, dma_handle, attrs);
}

static dma_addr_t mm_iommu_dma_map_page(struct device *dev, struct page *page,
				unsigned long offset, size_t size,
				enum dma_data_direction dir,
				unsigned long attrs)
{
	unsigned long iova;
	phys_addr_t paddr;
	size_t iosize = PAGE_ALIGN(size + offset);
	bool coherent = dev_is_dma_coherent_common(dev);
	int prot = mm_dma_info_to_prot(dir, coherent, attrs);

	paddr = page_to_phys(page);
	iova = mm_iommu_map(dev, paddr, iosize, prot);
	if (!iova) {
		dev_err(dev, "%s map:offset:%lu,size:%lu,dir:%d,attrs:%lx\n",
			    __func__, offset, size, dir, attrs);
		return 0;
	}

	if (!coherent && (attrs & DMA_ATTR_SKIP_CPU_SYNC) == 0)
		__dma_map_area(phys_to_virt(paddr + offset), size, dir);

	return iova + offset;
}

static void mm_iommu_dma_unmap_page(struct device *dev, dma_addr_t iova,
				size_t size, enum dma_data_direction dir,
				unsigned long attrs)
{
	phys_addr_t phys = mm_iommu_iova_to_phys(dev, iova);
	unsigned int offset = iova - ALIGN_DOWN(iova, PAGE_SIZE);
	size_t iosize = PAGE_ALIGN(size + offset);

	if (!phys) {
		dev_err(dev, "%s iova-0x%llx not exist\n", __func__, iova);
		return;
	}

	if (!dev_is_dma_coherent_common(dev) &&
		(attrs & DMA_ATTR_SKIP_CPU_SYNC) == 0)
		__dma_unmap_area(phys_to_virt(phys), size, dir);

	if (mm_iommu_unmap(dev, ALIGN_DOWN(iova, PAGE_SIZE), iosize)) {
		dev_err(dev, "%s failed to unmap iommu:iova:%llx, size:%zx\n",
			    __func__, iova, size);
		return;
	}
}

static int mm_iommu_dma_map_sg_attrs(struct device *dev,
				struct scatterlist *sgl, int nelems,
				enum dma_data_direction dir,
				unsigned long attrs)
{
	int i;
	unsigned long iova;
	unsigned long out_size;
	unsigned long mapped = 0;
	struct scatterlist *sg = NULL;
	bool coherent = dev_is_dma_coherent_common(dev);
	int prot = mm_dma_info_to_prot(dir, coherent, attrs);

	iova = mm_iommu_map_sg(dev, sgl, prot, &out_size);
	if (!iova) {
		dev_err(dev, "%s map_sg:nelems:%d, dir:%d, attrs:%lx\n",
			    __func__, nelems, dir, attrs);
		return 0;
	}

	for_each_sg(sgl, sg, nelems, i) {
		sg->dma_address = iova + mapped;
		sg_dma_len(sg) = sg->length;
		mapped += sg->length;
	}

	if (out_size != mapped) {
		dev_err(dev, "%s nelem:%d,dir:%d,attrs:%lx,size:%lx,map:%lx\n",
			    __func__, nelems, dir, attrs, out_size, mapped);
		return 0;
	}

	if (!coherent && (attrs & DMA_ATTR_SKIP_CPU_SYNC) == 0) {
		for_each_sg(sgl, sg, nelems, i) {
			void *va = mm_iommu_iova_to_va(dev, sg->dma_address);

			__dma_map_area(va, sg->length, dir);
		}
	}

	return nelems;
}

static void mm_iommu_dma_unmap_sg_attrs(struct device *dev,
				struct scatterlist *sgl,
				int nelems,
				enum dma_data_direction dir,
				unsigned long attrs)
{
	int i;
	int ret;
	struct scatterlist *sg = NULL;

	if (!dev_is_dma_coherent_common(dev) &&
	    (attrs & DMA_ATTR_SKIP_CPU_SYNC) == 0) {
		for_each_sg(sgl, sg, nelems, i) {
			void *virt = NULL;

			virt = mm_iommu_iova_to_va(dev, sg->dma_address);
			__dma_unmap_area(virt, sg->length, dir);
		}
	}

	ret = mm_iommu_unmap_sg(dev, sgl, sgl->dma_address);
	if (ret)
		dev_err(dev, "%s:sg:%pK,nelems:%d,dir:%d,attrs:%lx,ret:%d\n",
			    __func__, sgl, nelems, dir, attrs, ret);
}

static int mm_iommu_dma_supported(struct device *dev, u64 mask)
{
	if (dev_is_pci(dev) && mask == ~0ULL)
		return 1;

	return 0;
}

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 4, 0))
static int mm_iommu_dma_error(struct device *hwdev, dma_addr_t addr)
{
	return addr == MM_DMA_IOMMU_MAPPING_ERROR;
}
#endif

static void mm_iommu_dma_sync_single_for_cpu(struct device *dev,
	dma_addr_t iova, size_t size, enum dma_data_direction dir)
{
	phys_addr_t phys;

	if (dev_is_dma_coherent_common(dev))
		return;

	phys = mm_iommu_iova_to_phys(dev, iova);
	if (!phys) {
		dev_info(dev, "%s iova-0x%llx not exist\n", __func__, iova);
		return;
	}

	__dma_unmap_area(phys_to_virt(phys), size, dir);
}

static void mm_iommu_dma_sync_single_for_device(struct device *dev,
	dma_addr_t iova, size_t size, enum dma_data_direction dir)
{
	phys_addr_t phys;

	if (dev_is_dma_coherent_common(dev))
		return;

	phys = mm_iommu_iova_to_phys(dev, iova);
	if (!phys) {
		dev_info(dev, "%s iova-0x%llx not exist\n", __func__, iova);
		return;
	}

	__dma_map_area(phys_to_virt(phys), size, dir);
}

static void mm_iommu_dma_sync_sg_for_cpu(struct device *dev,
	struct scatterlist *sgl, int nelems, enum dma_data_direction dir)
{
	struct scatterlist *sg = NULL;
	phys_addr_t phys;
	int i;

	if (dev_is_dma_coherent_common(dev))
		return;

	for_each_sg(sgl, sg, nelems, i) {
		phys = mm_iommu_iova_to_phys(dev, sg->dma_address);
		if (!phys) {
			dev_info(dev, "%s iova-0x%llx phys is null\n",
				__func__, sg->dma_address);
			continue;
		}
		__dma_unmap_area(phys_to_virt(phys), sg->length, dir);
	}
}

static void mm_iommu_dma_sync_sg_for_device(struct device *dev,
	struct scatterlist *sgl, int nelems, enum dma_data_direction dir)
{
	struct scatterlist *sg = NULL;
	phys_addr_t phys;
	int i;

	if (dev_is_dma_coherent_common(dev))
		return;

	for_each_sg(sgl, sg, nelems, i) {
		phys = mm_iommu_iova_to_phys(dev, sg->dma_address);
		if (!phys) {
			dev_info(dev, "%s iova-0x%llx not exist\n",
				__func__, sg->dma_address);
			continue;
		}
		__dma_map_area(phys_to_virt(phys), sg->length, dir);
	}
}
static const struct dma_map_ops mm_iommu_dma_ops = {
	.alloc = mm_iommu_dma_alloc,
	.free = mm_iommu_dma_free,
	.map_page = mm_iommu_dma_map_page,
	.unmap_page = mm_iommu_dma_unmap_page,
	.map_sg = mm_iommu_dma_map_sg_attrs,
	.unmap_sg = mm_iommu_dma_unmap_sg_attrs,
	.dma_supported = mm_iommu_dma_supported,
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 4, 0))
	.mapping_error = mm_iommu_dma_error,
#endif
	.sync_single_for_cpu = mm_iommu_dma_sync_single_for_cpu,
	.sync_single_for_device = mm_iommu_dma_sync_single_for_device,
	.sync_sg_for_cpu = mm_iommu_dma_sync_sg_for_cpu,
	.sync_sg_for_device = mm_iommu_dma_sync_sg_for_device,
};

void mm_iommu_setup_dma_ops(struct device *dev)
{
	if (!dev || !dev_is_pci(dev))
		return;

	dev->dma_ops = &mm_iommu_dma_ops;
	pr_info("%s:dev name:%s!!\n", __func__, dev->driver->name);
}
