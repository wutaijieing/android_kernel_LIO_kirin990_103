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
#ifndef DPUFE_ION_H
#define DPUFE_ION_H
#include <linux/dma-mapping.h>
#include <linux/dma-buf.h>
#include <linux/device.h>
#include <linux/of_reserved_mem.h>
#include <linux/ion.h>
#include <linux/fb.h>
#include <linux/version.h>
#include <linux/mm_iommu.h>
#include "dpufe.h"

struct iommu_page_info {
	struct page *page;
	unsigned int order;
	struct list_head list;
};

unsigned long dpufe_alloc_fb_buffer(struct dpufe_data_type *dfd);
void *dpufe_iommu_map_kernel(struct sg_table *sg_table, size_t size);
void dpufe_iommu_unmap_kernel(const void *vaddr);
void dpufe_free_fb_buffer(struct dpufe_data_type *dfd);
int dpufe_fb_mmap(struct fb_info *info, struct vm_area_struct *vma);

#endif
