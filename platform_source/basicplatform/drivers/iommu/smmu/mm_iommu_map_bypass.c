/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2020 . All rights reserved.
 *
 * Description:
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Create: 2022-03-03
 */

#define pr_fmt(fmt) "[IOMMU_BYPASS: ]" fmt

#include <linux/dma-buf.h>
#include <linux/mm_iommu.h>
#include <linux/iommu.h>
#include <linux/kernel.h>
#include <linux/types.h>

static struct sg_table *
__dmabuf_get_sgt(struct device *dev, struct dma_buf *dmabuf, struct dma_buf_attachment **attach)
{
	struct sg_table *sgt = NULL;

	*attach = dma_buf_attach(dmabuf, dev);
	if (IS_ERR(*attach)) {
		pr_err("%s failed to attach the dmabuf\n", __func__);
		return NULL;
	}

	sgt = dma_buf_map_attachment(*attach, DMA_TO_DEVICE);
	if (IS_ERR(sgt)) {
		pr_err("%s failed to map dma buf to get sgt\n", __func__);
		dma_buf_detach(dmabuf, *attach);
		return NULL;
	}

	return sgt;
}

static void __release_dmabuf_attach(struct dma_buf *dmabuf,
				struct dma_buf_attachment *attach, struct sg_table *sgt)
{
	if (attach) {
		dma_buf_unmap_attachment(attach, sgt, DMA_FROM_DEVICE);
		dma_buf_detach(dmabuf, attach);
	}

}

int mm_iommu_dev_flush_tlb(struct device *dev, unsigned int ssid)
{
	return 0;
}
EXPORT_SYMBOL(mm_iommu_dev_flush_tlb);

unsigned long kernel_iommu_map_dmabuf(struct device *dev, struct dma_buf *dmabuf,
				int prot, unsigned long *out_size)
{
	struct sg_table *sgt = NULL;
	struct dma_buf_attachment *attach = NULL;
	unsigned long bypass_ret;

	if (!dev || !dmabuf || !out_size) {
		dev_err(dev, "input err! dev %pK, dmabuf %pK\n", dev, dmabuf);
		return 0;
	}

	sgt = __dmabuf_get_sgt(dev, dmabuf, &attach);
	if (!sgt)
		return 0;
	bypass_ret = (unsigned long)sg_phys(sgt->sgl);
	*out_size = sgt->sgl->length;
	__release_dmabuf_attach(dmabuf, attach, sgt);
	return bypass_ret;
}
EXPORT_SYMBOL(kernel_iommu_map_dmabuf);

int kernel_iommu_unmap_dmabuf(struct device *dev, struct dma_buf *dmabuf,
				unsigned long iova)
{
	return 0;
}
EXPORT_SYMBOL(kernel_iommu_unmap_dmabuf);

unsigned long mm_iommu_map_sg(struct device *dev, struct scatterlist *sgl,
				int prot, unsigned long *out_size)
{
	if (!sgl || !out_size) {
		pr_err("dev %pK, sgl %pK,or outsize is null\n", dev, sgl);
		return 0;
	}

	*out_size = sgl->length;
	return sg_phys(sgl);
}
EXPORT_SYMBOL(mm_iommu_map_sg);

int mm_iommu_unmap_sg(struct device *dev, struct scatterlist *sgl,
				unsigned long iova)
{
	return 0;
}
EXPORT_SYMBOL(mm_iommu_unmap_sg);

unsigned long mm_iommu_map(struct device *dev, phys_addr_t paddr,
				size_t size, int prot)
{
	if (!paddr) {
		pr_err("input Err! Dev %pK, addr =0x%llx\n", dev, paddr);
		return 0;
	}
	return (unsigned long)paddr;
}
EXPORT_SYMBOL(mm_iommu_map);

int mm_iommu_unmap(struct device *dev, unsigned long iova, size_t size)
{
	return 0;
}
EXPORT_SYMBOL(mm_iommu_unmap);

size_t mm_iommu_unmap_fast(struct device *dev,
			     unsigned long iova, size_t size)
{
	return 0;
}

phys_addr_t kernel_domain_get_ttbr(struct device *dev)
{
	return 0;
}
EXPORT_SYMBOL(kernel_domain_get_ttbr);

void dmabuf_release_iommu(struct dma_buf *dmabuf)
{
}

int mm_iommu_idle_display_unmap(struct device *dev, unsigned long iova,
		size_t size, u32 policy_id, struct dma_buf *dmabuf)
{
	return 0;
}

unsigned long mm_iommu_idle_display_map(struct device *dev, u32 policy_id,
				struct dma_buf *dmabuf, size_t allsize,
				size_t l3size, size_t lbsize)
{
	return 0;
}
EXPORT_SYMBOL(mm_iommu_idle_display_map);

void __dmabuf_release_iommu(struct dma_buf *dmabuf,
				struct iommu_domain domain)
{
}

struct gen_pool *iova_pool_setup(unsigned long start, unsigned long size,
				unsigned long align)
{
	return NULL;
}

void iova_pool_destroy(struct gen_pool *pool)
{
	return;
}

void mm_iova_dom_info(struct device *dev)
{
	return;
}

unsigned long kernel_iommu_map_padding_dmabuf(
				struct device *dev, struct dma_buf *dmabuf, unsigned long padding_len,
				int prot, unsigned long *out_size)
{
	return 0;
}

int kernel_iommu_unmap_padding_dmabuf(struct device *dev, struct dma_buf *dmabuf,
				unsigned long padding_len, unsigned long iova)
{
	return 0;
}
