/*
 * vcodec_vdec_memory.c
 *
 * This is for vdec memory
 *
 * Copyright (c) 2019-2020 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "vcodec_vdec.h"
#include "vcodec_vdec_memory.h"
#include "vcodec_vdec_plat.h"

static DEFINE_MUTEX(g_mem_mutex);

int32_t vdec_mem_probe(void *dev)
{
	int32_t ret;

	if (dev == NULL) {
		dprint(PRN_ERROR, "vdec dev is null\n");
		return VCODEC_FAILURE;
	}

	ret = dma_set_mask_and_coherent((struct device *)dev, ~0ULL);
	if (ret != 0) {
		dprint(PRN_ERROR, "dma set mask and coherent failed, ret = %d\n", ret);
		return VCODEC_FAILURE;
	}

	return 0;
}

int32_t vdec_mem_iommu_map(void *dev, int32_t share_fd, UADDR *iova, unsigned long *size)
{
	uint64_t iova_addr;
	unsigned long phy_size;
	struct dma_buf *dmabuf = VCODEC_NULL;
	vdec_entry *entry = vdec_get_entry();

	*iova = 0;
	*size = 0;

	vdec_check_ret(share_fd >= 0, VCODEC_FAILURE);

	vdec_mutex_lock(&g_mem_mutex);

	dmabuf = dma_buf_get(share_fd);
	if (IS_ERR(dmabuf)) {
		dprint(PRN_FATAL, "dma_buf_get failed");
		vdec_mutex_unlock(&g_mem_mutex);
		return VCODEC_FAILURE;
	}

	iova_addr = kernel_iommu_map_dmabuf((struct device *)dev, dmabuf, 0, &phy_size);
	if (!iova_addr) {
		dprint(PRN_FATAL, "kernel_iommu_map_dmabuf failed");
		dma_buf_put(dmabuf);
		vdec_mutex_unlock(&g_mem_mutex);
		return VCODEC_FAILURE;
	}

	*iova = iova_addr;
	*size = phy_size;

	entry->setting_info.ion_alloc_total_size += phy_size;
	dma_buf_put(dmabuf);
	vdec_mutex_unlock(&g_mem_mutex);

	return 0;
}

int32_t vdec_mem_iommu_unmap(void *dev, int32_t share_fd, UADDR iova)
{
	struct dma_buf *dmabuf = VCODEC_NULL;
	int32_t ret;

	vdec_check_ret(share_fd >= 0, VCODEC_FAILURE);
	vdec_check_ret(iova != 0, VCODEC_FAILURE);

	vdec_mutex_lock(&g_mem_mutex);

	dmabuf = dma_buf_get(share_fd);
	if (IS_ERR(dmabuf)) {
		dprint(PRN_FATAL, "dma buf get failed\n");
		vdec_mutex_unlock(&g_mem_mutex);
		return VCODEC_FAILURE;
	}

	ret = kernel_iommu_unmap_dmabuf((struct device *)dev, dmabuf, iova);
	if (ret) {
		dprint(PRN_ERROR, "kernel iommu unmap dmabuf failed\n");
		ret = VCODEC_FAILURE;
	}

	dma_buf_put(dmabuf);

	vdec_mutex_unlock(&g_mem_mutex);
	return ret;
}

