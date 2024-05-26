/* Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
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
#include <linux/fb.h>
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

struct dma_buf *dpu_get_dmabuf(int sharefd);
void dpu_put_dmabuf(struct dma_buf *buf);
bool dpu_check_addr_validate(dss_img_t *img);
bool dpu_check_layer_addr_validate(dss_layer_t *layer);
struct dma_buf *dpu_get_buffer_by_sharefd(uint64_t *iova, int fd, uint32_t size);
void dpu_put_buffer_by_dmabuf(uint64_t iova, struct dma_buf *dmabuf);
int dpu_buffer_map_iova(struct fb_info *info, void __user *arg);
int dpu_buffer_unmap_iova(struct fb_info *info, const void __user *arg);
int dpu_iommu_enable(struct platform_device *pdev);
phys_addr_t dpu_smmu_domain_get_ttbr(void);
int dpu_alloc_cma_buffer(size_t size, dma_addr_t *dma_handle, void **cpu_addr);
void dpu_free_cma_buffer(size_t size, dma_addr_t dma_handle, void *cpu_addr);

#endif
