/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#ifndef DPU_IOMMU_H
#define DPU_IOMMU_H
#include <linux/iommu.h>
#include <linux/dma-mapping.h>
#include <linux/dma-buf.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/mm_iommu.h>
#include "dpu.h"
struct dss_mm_info {
	struct list_head mm_list;
	spinlock_t map_lock;
};

extern struct platform_device *g_hdss_platform_device;

static inline struct device *__hdss_get_dev(void)
{
	if (g_hdss_platform_device == NULL) {
		pr_err("g_hdss_platform_device is null.\n");
		return NULL;
	}

	return &(g_hdss_platform_device->dev);
}

struct dma_buf *dpu_get_dmabuf(int32_t sharefd);
void dpu_put_dmabuf(struct dma_buf *buf);
int dpu_iommu_enable(struct platform_device *pdev);
phys_addr_t dpu_smmu_domain_get_ttbr(void);
int dpu_get_buffer_by_dma_buf(uint64_t *iova, struct dma_buf *buf, uint32_t size);

#endif
