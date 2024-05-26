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

#include "dpu_ion_mem.h"
#include "dpu_fb.h"
#include "res_mgr.h"
#include "dkmd_log.h"

void *dpu_iommu_map_kernel(struct sg_table *sg_table, size_t size)
{
	void *vaddr = NULL;
	struct scatterlist *sg = NULL;
	int32_t npages = PAGE_ALIGN(size) / PAGE_SIZE;
	struct page **pages = vmalloc(sizeof(struct page *) * npages);
	struct page **tmp = pages;
	struct page *page = NULL;
	pgprot_t pgprot;
	int32_t npages_this_entry;
	uint32_t i;
	int32_t j;

	if (IS_ERR_OR_NULL(pages)) {
		dpu_pr_err("vmalloc failed");
		return NULL;
	}

	if (!sg_table) {
		dpu_pr_err("sg_table is NULL");
		vfree(pages);
		return NULL;
	}

	for_each_sg(sg_table->sgl, sg, sg_table->nents, i) {
		npages_this_entry = PAGE_ALIGN(sg->length) / PAGE_SIZE;
		page = sg_page(sg);

		dpu_assert(i >= (uint32_t)npages);
		for (j = 0; j < npages_this_entry; j++)
			*(tmp++) = page++;
	}

	pgprot = pgprot_writecombine(PAGE_KERNEL);
	vaddr = vmap(pages, npages, VM_MAP, pgprot);
	vfree(pages);
	if (!vaddr) {
		dpu_pr_err("vmap failed");
		return NULL;
	}

	return vaddr;
}

void dpu_iommu_unmap_kernel(const void *vaddr)
{
	vunmap(vaddr);
}

/*
 * this function allocate physical memory,
 * and make them to scatter lista.
 * table is global .
 */
static struct iommu_page_info *dpu_dma_create_node(void)
{
	/* alloc 8kb each time */
	unsigned order = 1;
	struct iommu_page_info *info = NULL;
	struct page *page = NULL;

	info = kzalloc(sizeof(struct iommu_page_info), GFP_KERNEL);
	if (!info) {
		dpu_pr_err("kzalloc info failed");
		return NULL;
	}
	page = alloc_pages(GFP_KERNEL, order);
	if (!page) {
		dpu_pr_err("alloc page error");
		kfree(info);
		return NULL;
	}
	info->page = page;
	info->order = order;
	INIT_LIST_HEAD(&info->list);

	return info;
}

static struct sg_table *dpu_dma_alloc_mem(unsigned size)
{
	unsigned i = 0;
	unsigned alloc_size = 0;
	struct iommu_page_info *info = NULL;
	struct iommu_page_info *tmp_info = NULL;
	struct sg_table *table = NULL;
	struct scatterlist *sg = NULL;
	struct page *page = NULL;
	struct list_head pages;
	int32_t ret;

	if ((size > SZ_512M) || (size == 0))
		return NULL;

	INIT_LIST_HEAD(&pages);
	do {
		info = dpu_dma_create_node();
		if (!info)
			goto error;
		list_add_tail(&info->list, &pages);
		alloc_size += (1 << info->order) * PAGE_SIZE;
		i++;
	} while (alloc_size < size);

	table = kzalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!table)
		goto error;

	ret = sg_alloc_table(table, i, GFP_KERNEL);
	if (ret) {
		kfree(table);
		goto error;
	}

	sg = table->sgl;
	list_for_each_entry_safe(info, tmp_info, &pages, list) {
		page = info->page;
		sg_set_page(sg, page, (1 << info->order) * PAGE_SIZE, 0);
		sg = sg_next(sg);
		list_del(&info->list);
		kfree(info);
	}

	dpu_pr_info("alloc total memory size 0x%x", alloc_size);
	return table;

error:
	list_for_each_entry_safe(info, tmp_info, &pages, list) {
		__free_pages(info->page, info->order);
		list_del(&info->list);
		kfree(info);
	}
	return NULL;
}

static void dpu_dma_free_mem(struct sg_table *table)
{
	struct scatterlist *sg = NULL;
	unsigned mem_size = 0;
	uint32_t i;

	if (table) {
		for_each_sg(table->sgl, sg, table->nents, i) {
			__free_pages(sg_page(sg), get_order(sg->length));
			mem_size += sg->length;
		}
		sg_free_table(table);
		kfree(table);
	}
	dpu_pr_info("free total memory size 0x%x", mem_size);
}

unsigned long dpu_alloc_fb_buffer(struct fb_info *info)
{
	struct sg_table *sg = NULL;
	struct device_fb *dpu_fb = NULL;
	struct composer *comp = NULL;
	unsigned long buf_addr;
	unsigned long buf_size;
	size_t buf_len;

	if (unlikely(!info)) {
		dpu_pr_err("fb_info is NULL");
		return 0;
	}

	comp = (struct composer *)info->par;
	if (unlikely(!comp)) {
		dpu_pr_err("fb_info's par is NULL");
		return 0;
	}

	dpu_fb = (struct device_fb *)comp->device_data;
	if (unlikely(!dpu_fb)) {
		dpu_pr_err("dpu_fb is null");
		return 0;
	}

	buf_len = info->fix.smem_len;  /* align to PAGE_SIZE */
	sg = dpu_dma_alloc_mem(buf_len);
	if (!sg) {
		dpu_pr_err("dpu_dma_alloc_mem failed!");
		return 0;
	}

	buf_addr = mm_iommu_map_sg(dpu_res_get_device(), sg->sgl, 0, &buf_size);
	if (!buf_addr) {
		dpu_pr_err("mm_iommu_map_sg failed!");
		dpu_dma_free_mem(sg);
		return 0;
	}
	dpu_pr_info("scene%d alloc framebuffer map sg 0x%zxB succuss", dpu_fb->pinfo->pipe_sw_itfch_idx, buf_size);

	info->screen_base = dpu_iommu_map_kernel(sg, buf_len);
	if (!info->screen_base) {
		dpu_pr_err("dpufb_iommu_map_kernel failed!");
		mm_iommu_unmap_sg(dpu_res_get_device(), sg->sgl, buf_addr);
		dpu_dma_free_mem(sg);
		return 0;
	}
	info->fix.smem_start = buf_addr;
	info->screen_size = buf_len;
	dpu_fb->fb_sg_table = sg;
	dpu_fb->fb_mem_acquired = true;

	return buf_addr;
}

void dpu_free_fb_buffer(struct fb_info *info)
{
	struct device_fb *dpu_fb = NULL;
	struct composer *comp = NULL;

	if (unlikely(!info)) {
		dpu_pr_err("fb_info is NULL");
		return;
	}

	comp = (struct composer *)info->par;
	if (unlikely(!comp)) {
		dpu_pr_err("fb_info's par is NULL");
		return;
	}

	dpu_fb = (struct device_fb *)comp->device_data;
	if (unlikely(!dpu_fb)) {
		dpu_pr_err("dpu_fb is null");
		return;
	}

	if (likely(dpu_fb->fb_sg_table) && likely(info->screen_base != 0)) {
		dpu_iommu_unmap_kernel(info->screen_base);
		mm_iommu_unmap_sg(dpu_res_get_device(), dpu_fb->fb_sg_table->sgl, info->fix.smem_start);
		dpu_dma_free_mem(dpu_fb->fb_sg_table);

		dpu_fb->fb_sg_table = NULL;
		info->screen_base = 0;
		info->fix.smem_start = 0;
		dpu_fb->fb_mem_acquired = false;
	}
}

static int32_t dpu_mmap_table(struct fb_info *info, struct vm_area_struct *vma)
{
	uint32_t i = 0;
	int32_t ret = 0;
	struct composer *comp = info->par;
	struct device_fb *dpu_fb = (struct device_fb *)comp->device_data;
	struct sg_table *table = dpu_fb->fb_sg_table;
	struct scatterlist *sg = NULL;
	struct page *page = NULL;
	unsigned long size;
	unsigned long addr;
	unsigned long offset;
	unsigned long len;
	unsigned long remainder;

	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	addr = vma->vm_start;
	offset = vma->vm_pgoff * PAGE_SIZE;
	size = vma->vm_end - vma->vm_start;

	if (size > info->fix.smem_len) {
		dpu_pr_err("vma size=%lu is out of range(%u)!", size, info->fix.smem_len);
		return -EFAULT;
	}

	for_each_sg(table->sgl, sg, table->nents, i) {
		page = sg_page(sg);
		remainder = vma->vm_end - addr;
		len = sg->length;

		if (offset >= sg->length) {
			offset -= sg->length;
			continue;
		}

		if (offset) {
			page += offset / PAGE_SIZE;
			len = sg->length - offset;
			offset = 0;
		}

		len = min(len, remainder);
		ret = remap_pfn_range(vma, addr, page_to_pfn(page), len, vma->vm_page_prot);
		if (ret != 0) {
			dpu_pr_err("failed to remap_pfn_range! ret=%d", ret);
			return ret;
		}

		addr += len;
		if (addr >= vma->vm_end) {
			ret = 0;
			break;
		}
	}

	return ret;
}

int32_t dpu_fb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	int32_t ret;
	struct device_fb *dpu_fb = NULL;
	struct composer *comp = NULL;

	if (unlikely(!info)) {
		dpu_pr_err("fb_info is NULL");
		return -EINVAL;
	}

	comp = (struct composer *)info->par;
	if (unlikely(!comp)) {
		dpu_pr_err("fb_info's par is NULL");
		return -EINVAL;
	}

	dpu_fb = (struct device_fb *)comp->device_data;
	if (unlikely(!dpu_fb)) {
		dpu_pr_err("dpu_fb is null");
		return -EINVAL;
	}

	if (unlikely(!lock_fb_info(info)))
		return -ENODEV;

	if (!dpu_fb->fb_mem_acquired) {
		if (!dpu_alloc_fb_buffer(info)) {
			dpu_pr_err("dpu_alloc_fb_buffer failed!");
			ret = -ENOMEM;
			goto OUT;
		}
		dpu_fb->fb_mem_acquired = true;
	}

	ret = dpu_mmap_table(info, vma);

OUT:
	unlock_fb_info(info);
	return ret;
}
