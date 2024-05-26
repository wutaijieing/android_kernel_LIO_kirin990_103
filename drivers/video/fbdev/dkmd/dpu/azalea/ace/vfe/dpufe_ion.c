/* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#include <linux/mm_virtio_iommu.h>
#include "dpufe.h"
#include "dpufe_dbg.h"
#include "dpufe_panel_def.h"
#include "dpufe_ion.h"

/*
 * this function allocate physical memory,
 * and make them to scatter lista.
 * table is global .
 */
static struct iommu_page_info *__dpufe_dma_create_node(void)
{
	/* alloc 8kb each time */
	unsigned int order = 1;
	struct iommu_page_info *info = NULL;
	struct page *page = NULL;

	info = kzalloc(sizeof(struct iommu_page_info), GFP_KERNEL);
	if (info == NULL) {
		DPUFE_INFO("kzalloc info failed\n");
		return NULL;
	}

	page = alloc_pages(GFP_KERNEL, order);
	if (page == NULL) {
		DPUFE_INFO("alloc page error\n");
		kfree(info);
		return NULL;
	}

	info->page = page;
	info->order = order;
	INIT_LIST_HEAD(&info->list);

	return info;
}

static struct sg_table *__dpufe_dma_alloc_memory(unsigned int size)
{
	unsigned int i = 0;
	unsigned int sum = 0;
	struct iommu_page_info *info = NULL;
	struct iommu_page_info *tmp_info = NULL;
	struct sg_table *table = NULL;
	struct scatterlist *sg = NULL;
	struct page *page = NULL;
	struct list_head pages;
	unsigned int ret;

	DPUFE_INFO("+, size %d\n", size);
	if ((size > SZ_512M) || (size == 0))
		return NULL;

	INIT_LIST_HEAD(&pages);
	do {
		info = __dpufe_dma_create_node();
		if (!info)
			goto error;
		list_add_tail(&info->list, &pages);
		sum += (1 << info->order) * PAGE_SIZE;  /*lint !e647*/
		i++;
	} while (sum < size);

	table = kzalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (table == NULL)
		goto error;

	ret = sg_alloc_table(table, i, GFP_KERNEL);
	if (ret) {
		kfree(table);
		goto error;
	}

	sg = table->sgl;
	list_for_each_entry_safe(info, tmp_info, &pages, list) {
		page = info->page;
		sg_set_page(sg, page, (1 << info->order) * PAGE_SIZE, 0);  /*lint !e647*/
		sg = sg_next(sg);
		list_del(&info->list);
		kfree(info);
	}

	DPUFE_INFO("-, alloc total memory size 0x%x\n", sum);
	return table;

error:
	list_for_each_entry_safe(info, tmp_info, &pages, list) {
		__free_pages(info->page, info->order);
		list_del(&info->list);
		kfree(info);
	}
	return NULL;
}

static int __dpufe_dma_free_memory(struct sg_table *table)
{
	struct scatterlist *sg = NULL;
	unsigned int mem_size = 0;
	int i;

	if (table != NULL) {
		for_each_sg(table->sgl, sg, table->nents, i) {  /*lint !e574*/
			__free_pages(sg_page(sg), get_order(sg->length));
			mem_size += sg->length;
		}
		sg_free_table(table);
		kfree(table);
	}
	DPUFE_INFO("free total memory size 0x%x\n", mem_size);
	table = NULL;

	return 0;
}

unsigned long dpufe_alloc_fb_buffer(struct dpufe_data_type *dfd)
{
	struct sg_table *sg = NULL;
	struct fb_info *fbi = NULL;
	unsigned long buf_addr = 0;
	unsigned long buf_size;
	size_t buf_len;
	DPUFE_INFO("+\n");

	if (dfd == NULL) {
		DPUFE_ERR("dfd is NULL\n");
		return 0;
	}

	fbi = dfd->fbi;
	if (fbi == NULL) {
		DPUFE_ERR("fbi is NULL\n");
		return 0;
	}

	buf_len = fbi->fix.smem_len;  /* align to PAGE_SIZE */
	sg = __dpufe_dma_alloc_memory(buf_len);
	if (sg == NULL) {
		DPUFE_ERR("__dpufe_dma_alloc_memory failed!\n");
		return 0;
	}

	buf_addr = viommu_map_sg(SMMUID_DSS, sg->sgl);
	if (buf_addr == 0) {
		DPUFE_ERR("mm_iommu_map_sg failed!\n");
		__dpufe_dma_free_memory(sg);
		return 0;
	}
	DPUFE_INFO("fb%d alloc framebuffer map sg 0x%zxB succuss\n", dfd->index, buf_size);

	fbi->screen_base = dpufe_iommu_map_kernel(sg, buf_len);
	if (!fbi->screen_base) {
		DPUFE_ERR("dpufe_iommu_map_kernel failed!\n");
		viommu_unmap_sg(SMMUID_DSS, sg->sgl, buf_addr);
		__dpufe_dma_free_memory(sg);
		return 0;
	}

	fbi->fix.smem_start = buf_addr;
	fbi->screen_size = buf_len;
	dfd->fb_sg_table = sg;

	DPUFE_INFO("-\n");

	return buf_addr;
}

void dpufe_free_fb_buffer(struct dpufe_data_type *dfd)
{
	struct fb_info *fbi = NULL;

	if (dfd == NULL) {
		DPUFE_ERR("dfd is NULL\n");
		return;
	}
	fbi = dfd->fbi;
	if (fbi == NULL) {
		DPUFE_ERR("fbi is NULL\n");
		return;
	}
	DPUFE_INFO("+\n");
	if ((dfd->fb_sg_table) && (fbi->screen_base != 0)) {
		dpufe_iommu_unmap_kernel(fbi->screen_base);
		viommu_unmap_sg(SMMUID_DSS, dfd->fb_sg_table->sgl, fbi->fix.smem_start);
		__dpufe_dma_free_memory(dfd->fb_sg_table);

		dfd->fb_sg_table = NULL;
		fbi->screen_base = 0;
		fbi->fix.smem_start = 0;
	}
	DPUFE_INFO("-\n");
}

void *dpufe_iommu_map_kernel(struct sg_table *sg_table, size_t size)
{
	void *vaddr = NULL;
	struct scatterlist *sg = NULL;
	struct sg_table *table = sg_table;
	int npages = PAGE_ALIGN(size) / PAGE_SIZE;
	struct page **pages = vmalloc(sizeof(struct page *) * npages);
	struct page **tmp = pages;
	pgprot_t pgprot;
	int i;
	int j;

	if (IS_ERR_OR_NULL(pages)) {
		DPUFE_ERR("%s: vmalloc failed.\n", __func__);
		return NULL;
	}

	if (table == NULL) {
		DPUFE_ERR("%s: table is NULL\n", __func__);
		vfree(pages);
		return NULL;
	}
	pgprot = pgprot_writecombine(PAGE_KERNEL); /*lint !e446*/

	for_each_sg(table->sgl, sg, table->nents, i) {  /*lint !e574*/
		int npages_this_entry = PAGE_ALIGN(sg->length) / PAGE_SIZE;
		struct page *page = sg_page(sg);

		if (i >= npages) {
			DPUFE_ERR("i >= npages!\n");
			vfree(pages);
			return NULL;
		}
		for (j = 0; j < npages_this_entry; j++)
			*(tmp++) = page++;
	}
	vaddr = vmap(pages, npages, VM_MAP, pgprot);
	vfree(pages);
	if (vaddr == NULL) {
		DPUFE_ERR("%s: vmap failed.\n", __func__);
		return NULL;
	}

	return vaddr;
}

void dpufe_iommu_unmap_kernel(const void *vaddr)
{
	vunmap(vaddr);
}

int dpufe_create_buffer_client(struct dpufe_data_type *dfd)
{
	return 0;
}

void dpufe_destroy_buffer_client(struct dpufe_data_type *dfd)
{
}

static int dpufe_fb_mmap_table(struct fb_info *info, struct dpufe_data_type *dfd, struct vm_area_struct *vma)
{
	int i = 0;
	int ret = 0;
	struct sg_table *table = NULL;
	struct scatterlist *sg = NULL;
	struct page *page = NULL;
	unsigned long size;
	unsigned long addr;
	unsigned long offset;
	unsigned long len;
	unsigned long remainder;

	table = dfd->fb_sg_table;
	dpufe_check_and_return((table == NULL), -EFAULT, "fb%d, table is NULL!\n", dfd->index);

	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	addr = vma->vm_start;
	offset = vma->vm_pgoff * PAGE_SIZE;
	size = vma->vm_end - vma->vm_start;

	dpufe_check_and_return((size > info->fix.smem_len), -EFAULT,
		"fb%d, size=%lu is out of range(%u)!\n", dfd->index, size, info->fix.smem_len);

	for_each_sg(table->sgl, sg, table->nents, i) {  /*lint !e574*/
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
		dpufe_check_and_return((ret != 0), ret, "fb%d, failed to remap_pfn_range! ret=%d\n",
				dfd->index, ret);

		addr += len;
		if (addr >= vma->vm_end) {
			ret = 0;
			break;
		}
	}

	return ret;
}

int dpufe_fb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	struct dpufe_data_type *dfd = NULL;
	int ret;

	if (info == NULL || vma == NULL) {
		DPUFE_INFO("NULL vma or info !\n");
		return -EINVAL;
	}

	dfd = (struct dpufe_data_type *)info->par;
	if (dfd == NULL) {
		DPUFE_ERR("dfd is NULL!\n");
		return -EINVAL;
	}
	DPUFE_INFO("fb%d, +\n", dfd->index);
	if (dfd->index != PRIMARY_PANEL_IDX) {
		DPUFE_INFO("fb%u, no fb buffer!\n", dfd->index);
		return -EFAULT;
	}

	if (lock_fb_info(info) == 0)
		return -ENODEV;

	if (dfd->fb_mem_free_flag) {
		if (info->fix.smem_len > 0) {
			if (dpufe_alloc_fb_buffer(dfd) == 0) {
				DPUFE_ERR("fb%d, dpufe_alloc_buffer failed!\n", dfd->index);
				ret = -ENOMEM;
				goto return_unlock;
			}
		}
		dfd->fb_mem_free_flag = false;
	}

	ret = dpufe_fb_mmap_table(info, dfd, vma);
	DPUFE_INFO("fb%d, -\n", dfd->index);
return_unlock:
	unlock_fb_info(info);
	return ret;
}

