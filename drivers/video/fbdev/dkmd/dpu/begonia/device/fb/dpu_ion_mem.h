/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef DPU_ION_MEM_H
#define DPU_ION_MEM_H

#include <linux/ion.h>
#include <linux/fb.h>
#include <linux/mm_iommu.h>

struct iommu_page_info {
	struct page *page;
	unsigned order;
	struct list_head list;
};

void *dpu_iommu_map_kernel(struct sg_table *sg_table, size_t size);
void dpu_iommu_unmap_kernel(const void *vaddr);
unsigned long dpu_alloc_fb_buffer(struct fb_info *info);
void dpu_free_fb_buffer(struct fb_info *info);
int32_t dpu_fb_mmap(struct fb_info *info, struct vm_area_struct *vma);

#endif
