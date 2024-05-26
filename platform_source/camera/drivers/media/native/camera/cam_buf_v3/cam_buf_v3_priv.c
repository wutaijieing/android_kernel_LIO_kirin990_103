/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: Implement of camera buffer v3 priv.
 * Create: 2019-07-30
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <cam_buf.h>
#include <cam_log.h>

#include <linux/mm_iommu.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/dma-buf.h>

#define MAX_PADDING_SIZE (150 * 1024 * 1024)

int cam_v3_internal_map_iommu(struct device *dev,
							  int fd, struct iommu_format *fmt, int padding_support)
{
	int rc = 0;
	struct dma_buf *dmabuf = NULL;
	unsigned long iova_addr = 0;
	unsigned long iova_size = 0;

	if (IS_ERR_OR_NULL(dev)) {
		cam_err("%s: fail to get dev", __func__);
		return -ENOENT;
	}

	if (fmt->size > MAX_PADDING_SIZE) {
		cam_err("%s: padding size is oversize", __func__);
		return -ENOENT;
	}

	dmabuf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(dmabuf)) {
		cam_err("%s: fail to get dma dmabuf", __func__);
		return -ENOENT;
	}

	if (padding_support && fmt->size != 0)
		iova_addr = kernel_iommu_map_padding_dmabuf(dev, dmabuf,
													fmt->size, fmt->prot, &iova_size);
	else
		iova_addr = kernel_iommu_map_dmabuf(dev, dmabuf, fmt->prot,
											&iova_size);

	if (!iova_addr) {
		cam_err("%s: fail to map iommu", __func__);
		rc = -ENOMEM;
		goto err_map_iommu;
	}

	cam_info("%s: fd:%d, dmabuf:%pK iova:%#lx, maped size:%#lx, padding_support: %d, fmt size: %#lx",
			 __func__, fd, dmabuf, iova_addr, iova_size, padding_support,
			 fmt->size);
	fmt->iova = iova_addr;
	fmt->size = iova_size;

err_map_iommu:
	dma_buf_put(dmabuf);
	return rc;
}

void cam_v3_internal_unmap_iommu(struct device *dev,
								 int fd, struct iommu_format *fmt, int padding_support)
{
	int rc;
	struct dma_buf *dmabuf = NULL;

	if (IS_ERR_OR_NULL(dev)) {
		cam_err("%s: fail to get dev", __func__);
		return;
	}

	dmabuf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(dmabuf)) {
		cam_err("%s: fail to get dma dmabuf", __func__);
		return;
	}

	if (padding_support && fmt->size != 0)
		rc = kernel_iommu_unmap_padding_dmabuf(dev, dmabuf,
											   fmt->size, fmt->iova);
	else
		rc = kernel_iommu_unmap_dmabuf(dev, dmabuf, fmt->iova);

	if (rc < 0)
		cam_err("%s: failed", __func__);

	cam_info("%s: fd:%d, dmabuf:%pK, unmaped iova:0x%016lx, padding_support: %d, fmt size: %#lx",
			 __func__, fd, dmabuf, fmt->iova, padding_support,
			 fmt->size);
	dma_buf_put(dmabuf);
}
