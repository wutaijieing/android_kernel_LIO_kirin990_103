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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#include "dpu_iommu.h"
#include "dpu_fb.h"

#define TIME_INTERVAL_FOR_DUMP_MEM_STATUS 10

struct dss_iova_info {
	struct list_head list_node;
	struct dma_buf *dmabuf;
	iova_info_t iova_info;
};

struct platform_device *g_hdss_platform_device;
static struct dss_mm_info g_mm_list;

struct dma_buf *dpu_get_dmabuf(int32_t sharefd)
{
	struct dma_buf *buf = NULL;

	/* dim layer share fd -1 */
	if (sharefd < 0) {
		DPU_FB_ERR("Invalid file sharefd = %d\n", sharefd);
		return NULL;
	}

	buf = dma_buf_get(sharefd);
	if (IS_ERR_OR_NULL(buf)) {
		DPU_FB_ERR("Invalid file buf(%pK), sharefd = %d\n", buf, sharefd);
		return NULL;
	}

	return buf;
}

void dpu_put_dmabuf(struct dma_buf *buf)
{
	if (IS_ERR_OR_NULL(buf)) {
		DPU_FB_ERR("Invalid dmabuf(%pK)\n", buf);
		return;
	}

	dma_buf_put(buf);
}

int dpu_iommu_enable(struct platform_device *pdev)
{
	struct dss_mm_info *mm_list = &g_mm_list;
	int ret;

	if (!pdev) {
		DPU_FB_ERR("pdev is NULL\n");
		return -EINVAL;
	}

	kernel_domain_get_ttbr(&pdev->dev);
	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));  /*lint !e598*/
	if (ret != 0)
		DPU_FB_ERR("dma set mask and coherent failed\n");

	g_hdss_platform_device = pdev;

	spin_lock_init(&mm_list->map_lock);
	INIT_LIST_HEAD(&mm_list->mm_list);

	return 0;
}

phys_addr_t dpu_smmu_domain_get_ttbr(void)
{
	return kernel_domain_get_ttbr(__hdss_get_dev());
}

int dpu_alloc_cmdlist_buffer(struct dpu_fb_data_type *dpufd)
{
	void *cpu_addr = NULL;
	size_t buf_len;
	dma_addr_t dma_handle;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return -EINVAL;
	}

	memset(&dma_handle, 0, sizeof(dma_handle));
	buf_len = dpufd->sum_cmdlist_pool_size;
	cpu_addr = dma_alloc_coherent(__hdss_get_dev(), buf_len, &dma_handle, GFP_KERNEL);
	if (!cpu_addr) {
		DPU_FB_ERR("fb%d dma alloc 0x%zxB coherent failed!\n", dpufd->index, buf_len);
		return -ENOMEM;
	}
	dpufd->cmdlist_pool_vir_addr = cpu_addr;
	dpufd->cmdlist_pool_phy_addr = dma_handle;

	memset(dpufd->cmdlist_pool_vir_addr, 0, buf_len);

	return 0;
}

void dpu_free_cmdlist_buffer(struct dpu_fb_data_type *dpufd)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return;
	}

	if (dpufd->cmdlist_pool_vir_addr != 0) {
		dma_free_coherent(__hdss_get_dev(), dpufd->sum_cmdlist_pool_size,
			dpufd->cmdlist_pool_vir_addr, dpufd->cmdlist_pool_phy_addr);
		dpufd->cmdlist_pool_vir_addr = 0;
	}
}

int dpu_get_buffer_by_dma_buf(uint64_t *iova, struct dma_buf *buf, uint32_t size)
{
	unsigned long buf_size = 0;
	dpu_check_and_return(!buf || !iova, -1, ERR, "buf or iova is null\n");

	*iova = kernel_iommu_map_dmabuf(__hdss_get_dev(), buf, 0, &buf_size);
	if ((*iova == 0) || (buf_size < size)) {
		DPU_FB_ERR("get iova_size(0x%llx) smaller than size(0x%x)\n", buf_size, size);
		if (*iova != 0) {
			(void)kernel_iommu_unmap_dmabuf(__hdss_get_dev(), buf, *iova);
			*iova = 0;
		}
		dpu_put_dmabuf(buf);
		return -1;
	}

	return 0;
}

#pragma GCC diagnostic pop
