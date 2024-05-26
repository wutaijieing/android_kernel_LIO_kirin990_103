/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2021. All rights reserved.
 *
 * jpeg jpu iommu
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"

#include "jpu_iommu.h"
#include <linux/iommu.h>
#include <linux/list.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/dma-buf.h>
#include <linux/mm_iommu.h>
#include "jpu_def.h"

#define MAX_INPUT_DATA_LEN (8192 * 8192 * 4)
#define DMA_64BIT 64
#define JPU_BLOCK_BUF_MAX_SIZE 2048
#define JPU_MCU_SHITF_5_BIT    5
#define JPU_MCU_SHIFT_15_BIT   15

struct jpu_iommu_page_info {
	struct page *page;
	unsigned int order;
	struct list_head list;
};

int jpu_enable_iommu(struct jpu_data_type *jpu_device)
{
	int ret;
	jpu_check_null_return(jpu_device, -EINVAL);
	jpu_check_null_return(jpu_device->pdev, -EINVAL);

	(void)kernel_domain_get_ttbr(&(jpu_device->pdev->dev));
	jpu_info("jpu get iommu ttbr success\n");
	/*lint -e598*/
	ret = dma_set_mask_and_coherent(&(jpu_device->pdev->dev),
		DMA_BIT_MASK(DMA_64BIT));
	if (ret < 0) {
		jpu_err("dma set failed\n");
		return -EFAULT;
	}
	return 0;
}

phys_addr_t jpu_domain_get_ttbr(struct jpu_data_type *jpu_device)
{
	jpu_check_null_return(jpu_device, 0);
	jpu_check_null_return(jpu_device->pdev, 0);

	return kernel_domain_get_ttbr(&(jpu_device->pdev->dev));
}

/*
 * this function allocate physical memory,
 * and make them to scatter lista.
 * table is global.
 */
static struct jpu_iommu_page_info *jpu_dma_create_node(void)
{
	unsigned int order = 1;
	struct jpu_iommu_page_info *info = NULL;
	struct page *page = NULL;

	info = kzalloc(sizeof(struct jpu_iommu_page_info), GFP_KERNEL);
	jpu_check_null_return(info, NULL);

	/* alloc 8kb each time */
	page = alloc_pages(GFP_KERNEL, order);
	if (page == NULL) {
		jpu_err("alloc page error\n");
		kfree(info);
		return NULL;
	}

	info->page = page;
	info->order = order;
	INIT_LIST_HEAD(&info->list);

	return info;
}

static struct sg_table *jpu_dma_alloc_memory(unsigned int size)
{
	unsigned int sum = 0;
	struct list_head pages;
	struct jpu_iommu_page_info *info = NULL;
	struct jpu_iommu_page_info *tmp_info = NULL;
	unsigned int i = 0;
	struct sg_table *table = NULL;
	struct scatterlist *sg = NULL;

	if ((size > SZ_512M) || (size == 0))
		return NULL;

	INIT_LIST_HEAD(&pages);
	do {
		info = jpu_dma_create_node();
		if (info == NULL)
			goto error;

		list_add_tail(&info->list, &pages);
		sum += (unsigned)(1 << info->order) * PAGE_SIZE;
		i++;
	} while (sum < size);

	table = kzalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (table == NULL)
		goto error;

	if (sg_alloc_table(table, i, GFP_KERNEL) != 0) {
		kfree(table);
		goto error;
	}

	sg = table->sgl;
	list_for_each_entry_safe(info, tmp_info, &pages, list) {
		struct page *page = info->page;

		sg_set_page(sg, page, (unsigned)(1 << info->order) * PAGE_SIZE, 0);
		sg = sg_next(sg);
		list_del(&info->list);
		kfree(info);
	}

	jpu_info("alloc total memory 0x%x\n", sum);

	return table;
error:
	list_for_each_entry_safe(info, tmp_info, &pages, list) {
		__free_pages(info->page, info->order);
		list_del(&info->list);
		kfree(info);
	}
	return NULL;
}

static void jpu_dma_free_memory(struct sg_table *table)
{
	unsigned int i = 0;
	struct scatterlist *sg = NULL;
	unsigned int mem_size = 0;

	if (table != NULL) {
		for_each_sg(table->sgl, sg, table->nents, i) {
			__free_pages(sg_page(sg), get_order(sg->length));
			mem_size += sg->length;
		}
		sg_free_table(table);
		kfree(table);
	}
	jpu_info("free total memory 0x%x\n", mem_size);
}

int jpu_lb_alloc(struct jpu_data_type *jpu_device)
{
	size_t lb_size;
	unsigned long buf_addr;
	unsigned long buf_size = 0;
	struct sg_table *sg = NULL;

	jpu_check_null_return(jpu_device, -EINVAL);
	jpu_device->lb_sg_table = NULL;
	lb_size = JPU_LB_SIZE;
	sg = jpu_dma_alloc_memory(lb_size);
	jpu_check_null_return(sg, -ENOMEM);

	buf_addr = mm_iommu_map_sg(&(jpu_device->pdev->dev), sg->sgl, 0, &buf_size);
	if (buf_addr == 0) {
		jpu_err("mm_iommu_map_sg failed\n");
		jpu_dma_free_memory(sg);
		return -ENOMEM;
	}
	jpu_info("jpu %u alloc lb map sg 0x%zxB succuss\n",
		jpu_device->index, buf_size);

	jpu_device->lb_buf_base = buf_addr;
	jpu_device->lb_addr = (uint32_t)(buf_addr >> 4); /* right shift 4 bit */

	/*
	 * start address for line buffer, unit is 16 byte,
	 * must align to 128 byte
	 */
	if (jpu_device->lb_addr & (JPU_LB_ADDR_ALIGN - 1)) {
		jpu_err("lb_addr 0x%x is not %d bytes aligned\n",
			jpu_device->lb_addr, JPU_LB_ADDR_ALIGN - 1);

		mm_iommu_unmap_sg(&(jpu_device->pdev->dev), sg->sgl, buf_addr);
		jpu_dma_free_memory(sg);

		return -EINVAL;
	}
	jpu_device->lb_sg_table = sg;

	jpu_info("lb_size = %zu, jpu_device->lb_addr 0x%x\n", lb_size,
		jpu_device->lb_addr);
	return 0;
}

void jpu_lb_free(struct jpu_data_type *jpu_device)
{
	bool cond = false;

	if ((jpu_device == NULL) || (jpu_device->pdev == NULL)) {
		jpu_err("jpu_device or jpu_device->pdev is NULL\n");
		return;
	}
	cond = ((jpu_device->lb_addr != 0) && (jpu_device->lb_sg_table != NULL));
	if (cond) {
		mm_iommu_unmap_sg(&(jpu_device->pdev->dev),
			jpu_device->lb_sg_table->sgl, jpu_device->lb_buf_base);

		jpu_dma_free_memory(jpu_device->lb_sg_table);
		jpu_device->lb_buf_base = 0;
		jpu_device->lb_addr = 0;
	}
}

static bool jpu_check_buffer_validate(int fd)
{
	struct dma_buf *buf = NULL;

	/* dim layer share fd -1 */
	if (fd < 0)
		return false;

	buf = dma_buf_get(fd);
	if (IS_ERR(buf)) {
		jpu_err("Invalid file fd %d\n", fd);
		return false;
	}

	dma_buf_put(buf);
	return true;
}

int jpu_check_inbuff_addr(struct jpu_data_type *jpu_device,
	struct jpu_data_t *jpu_req)
{
	unsigned long buf_size = 0;
	uint64_t inbuffer_addr;
	struct dma_buf *buf = NULL;
	bool cond = false;

	jpu_check_null_return(jpu_device, -EINVAL);
	jpu_check_null_return(jpu_device->pdev, -EINVAL);
	jpu_check_null_return(jpu_req, -EINVAL);

	if (!jpu_check_buffer_validate(jpu_req->in_sharefd)) {
		jpu_err("Invalid file fd %d\n", jpu_req->in_sharefd);
		return -EINVAL;
	}

	buf = dma_buf_get(jpu_req->in_sharefd);
	if (IS_ERR(buf)) {
		jpu_err("Invalid file shared_fd %d\n", jpu_req->in_sharefd);
		return -EINVAL;
	}

	inbuffer_addr = kernel_iommu_map_dmabuf(&(jpu_device->pdev->dev),
		buf, 0, &buf_size);
	if (inbuffer_addr == 0) {
		dma_buf_put(buf);
		jpu_err("get iova_size 0x%lx failed\n", buf_size);
		return -EFAULT;
	}

	jpu_debug("get iova success iova_size 0x%lx\n", buf_size);
	if (jpu_req->start_addr >= buf_size || jpu_req->end_addr >= buf_size ||
		jpu_req->end_addr <= jpu_req->start_addr) {
		(void)kernel_iommu_unmap_dmabuf(&(jpu_device->pdev->dev), buf, inbuffer_addr);
		dma_buf_put(buf);
		jpu_err("buffer addr invalid\n");
		return -EINVAL;
	}

	jpu_req->start_addr = jpu_req->start_addr + inbuffer_addr;
	jpu_req->end_addr = jpu_req->end_addr + inbuffer_addr;

	cond = ((jpu_req->addr_offset > jpu_req->start_addr) ||
		(jpu_req->addr_offset > MAX_INPUT_DATA_LEN));
	if (cond) {
		(void)kernel_iommu_unmap_dmabuf(&(jpu_device->pdev->dev), buf, inbuffer_addr);
		dma_buf_put(buf);
		jpu_err("addr offset invalid\n");
		return -EINVAL;
	}

	dma_buf_put(buf);
	return 0;
}

static int jpu_check_addr(struct jpu_data_type *jpu_device,
	struct jpu_data_t *jpu_req)
{
	jpu_check_null_return(jpu_device, -EINVAL);
	jpu_check_null_return(jpu_device->pdev, -EINVAL);
	jpu_check_null_return(jpu_req, -EINVAL);

	if (!jpu_check_buffer_validate(jpu_req->out_sharefd)) {
		jpu_err("Invalid file fd %d\n", jpu_req->out_sharefd);
		return -EINVAL;
	}
	return 0;
}

int jpu_check_outbuff_addr(struct jpu_data_type *jpu_device,
	struct jpu_data_t *jpu_req)
{
	unsigned long buf_size = 0;
	bool cond = false;
	struct dma_buf *buf = NULL;
	uint64_t outbuffer_addr;

	if (jpu_check_addr(jpu_device, jpu_req) != 0)
		return -EINVAL;

	buf = dma_buf_get(jpu_req->out_sharefd);
	if (IS_ERR(buf)) {
		jpu_err("Invalid file shared_fd %d\n", jpu_req->out_sharefd);
		return -EINVAL;
	}

	outbuffer_addr = kernel_iommu_map_dmabuf(&(jpu_device->pdev->dev),
		buf, 0, &buf_size);
	if (outbuffer_addr == 0) {
		dma_buf_put(buf);
		jpu_err("get iova_size 0x%lx failed\n", buf_size);
		return -EFAULT;
	}
	jpu_info("get iova success iova_size 0x%lx\n", buf_size);

	if (jpu_req->last_page_y > (buf_size >> JPU_MCU_SHIFT_15_BIT) ||
		jpu_req->start_addr_c > (buf_size >> JPU_MCU_SHITF_5_BIT) ||
		jpu_req->last_page_c > (buf_size >> JPU_MCU_SHIFT_15_BIT)) {
		jpu_err("jpu_req invalid\n");
		(void)kernel_iommu_unmap_dmabuf(&(jpu_device->pdev->dev), buf, outbuffer_addr);
		dma_buf_put(buf);
		return -EINVAL;
	}

	jpu_req->start_addr_y = outbuffer_addr / JPU_MCU_16ALIGN;
	if ((jpu_req->out_color_format >= JPEG_DECODE_OUT_RGBA4444) &&
		(jpu_req->out_color_format <= JPEG_DECODE_OUT_BGRA8888)) {
		jpu_req->last_page_y += jpu_req->start_addr_y / JPU_BLOCK_BUF_MAX_SIZE;
	} else {
		if (jpu_req->out_color_format == JPEG_DECODE_OUT_YUV400)
			jpu_req->last_page_y += jpu_req->start_addr_y / JPU_BLOCK_BUF_MAX_SIZE;

		cond = (jpu_req->out_color_format == JPEG_DECODE_OUT_YUV420) ||
			(jpu_req->out_color_format == JPEG_DECODE_OUT_YUV444) ||
			(jpu_req->out_color_format == JPEG_DECODE_OUT_YUV422_H1V2) ||
			(jpu_req->out_color_format == JPEG_DECODE_OUT_YUV422_H2V1);
		if (cond) {
			jpu_req->last_page_y += jpu_req->start_addr_y / JPU_BLOCK_BUF_MAX_SIZE;
			jpu_req->start_addr_c += jpu_req->start_addr_y;
			jpu_req->last_page_c += jpu_req->start_addr_y / JPU_BLOCK_BUF_MAX_SIZE;

			jpu_info("outbuf: stride_y 0x%x, stride_c 0x%x\n",
				jpu_req->stride_y, jpu_req->stride_c);
		}
	}

	/* start_addr unit is 16 byte, page unit is 32KB,
	 * so start_addr need to divide 2048
	 */
	cond = (jpu_req->last_page_y < (jpu_req->start_addr_y / JPU_BLOCK_BUF_MAX_SIZE)) ||
		(jpu_req->last_page_c < (jpu_req->start_addr_c / JPU_BLOCK_BUF_MAX_SIZE));
	if (cond) {
		jpu_err("last_page_y invalid\n");
		(void)kernel_iommu_unmap_dmabuf(&(jpu_device->pdev->dev), buf, outbuffer_addr);
		dma_buf_put(buf);
		return -EINVAL;
	}

	dma_buf_put(buf);
	return 0;
}

#pragma GCC diagnostic pop
