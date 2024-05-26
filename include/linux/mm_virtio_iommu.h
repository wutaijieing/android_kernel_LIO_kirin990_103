/* Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef _MM_VIRTIO_IOMMU_H
#define _MM_VIRTIO_IOMMU_H

#include <linux/types.h>
#include <linux/dma-buf.h>

enum smmu_dev_id {
	SMMUID_TEST = 0,
	SMMUID_DSS = 1,
	SMMUID_MAX = 2,
};

#ifdef CONFIG_MM_VIRTIO_IOMMU
unsigned long viommu_map_dmabuf(int smmuid, struct dma_buf *dmabuf);
int viommu_unmap_dmabuf(int smmuid, struct dma_buf *dmabuf, unsigned long iova);
unsigned long viommu_map_sg(int smmuid, struct scatterlist *sgl);
int viommu_unmap_sg(int smmuid, struct scatterlist *sgl, unsigned long iova);
void dmabuf_release_viommu(struct dma_buf *dmabuf);
#else
static inline unsigned long viommu_map_dmabuf(int smmuid, struct dma_buf *dmabuf)
{
	return 0;
}

static inline int viommu_unmap_dmabuf(int smmuid, struct dma_buf *dmabuf, unsigned long iova)
{
	return -EINVAL;
}

static inline unsigned long viommu_map_sg(int smmuid, struct scatterlist *sgl)
{
	return 0;
}

static inline int viommu_unmap_sg(int smmuid, struct scatterlist *sgl, unsigned long iova)
{
	return -EINVAL;
}

static inline void dmabuf_release_viommu(struct dma_buf *dmabuf)
{
}
#endif

#ifdef CONFIG_MM_IOMMU_TEST
void viommu_dump_pgtable(int smmuid, unsigned long iova, size_t iova_size);
#else
static inline void viommu_dump_pgtable(int smmuid, unsigned long iova, size_t iova_size)
{
}
#endif

#endif
