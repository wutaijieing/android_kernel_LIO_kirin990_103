/*
 * memory.c
 *
 * This is functions definition for memory.
 *
 * Copyright (c) 2014-2020 Huawei Technologies CO., Ltd.
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

#include "memory.h"

#include <linux/math64.h>
#include <linux/platform_drivers/mm_ion.h>
#include <linux/file.h>
#include <linux/major.h>
#include <linux/syscalls.h>
#include <linux/dma-mapping.h>
#include <linux/dma-iommu.h>
#include <linux/dma-buf.h>
#include <linux/iommu.h>
#include <linux/mm_iommu.h>

#include "omxvdec.h"
#include "platform.h"

/*lint -e774*/
#define MAX_ION_MEM_NODE  100
#define CLIENT_BUF_NAME   "CLIENT"

/* STRUCT */
typedef struct {
	uint32_t phys_addr;
	void *virt_addr;
	uint32_t size;
	struct ion_handle *handle;
} ion_mem_desc_s;

struct  mutex g_mem_mutex;

/* func: memory module probe entry */
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
	vdec_init_mutex(&g_mem_mutex);

	return 0;
}

/* func: map dma buffer */
int32_t vdec_mem_get_map_info(int32_t share_fd, mem_buffer_s *ps_m_buf)
{
	int32_t rc;
	uint64_t iova_addr;
	unsigned long phy_size;
	struct dma_buf *dmabuf = NULL;
	int32_t ret = 0;
	void *virt_addr = NULL;
	omx_vdec_entry vdec_entry;

	if ((!ps_m_buf) || (share_fd < 0)) {
		dprint(PRN_ERROR, "%s: invalid Param(share_fd:%d)\n", __func__, share_fd);
		return VCODEC_FAILURE;
	}

	vdec_mutex_lock(&g_mem_mutex);

	vdec_entry = omx_vdec_get_entry();

	dmabuf = dma_buf_get(share_fd);
	if (IS_ERR(dmabuf)) {
		dprint(PRN_FATAL, "%s, dma_buf_get failed", __func__);
		vdec_mutex_unlock(&g_mem_mutex);
		return VCODEC_FAILURE;
	}

	iova_addr =
		kernel_iommu_map_dmabuf(vdec_entry.device, dmabuf, 0, &phy_size);
	if (!iova_addr) {
		dprint(PRN_FATAL, "%s, kernel_iommu_map_dmabuf failed", __func__);
		goto exit;
	}

	if (ps_m_buf->is_map_virtual == MEM_BUF_MAPPED) {
		rc = dma_buf_begin_cpu_access(dmabuf, DMA_FROM_DEVICE);
		if (rc < 0) {
			dprint(PRN_ERROR, "%s:dma_buf_begin_cpu_access failed\n", __func__);
			goto error_1;
		}

		virt_addr = dma_buf_kmap(dmabuf, 0);
		if (virt_addr == NULL) {
			dprint(PRN_FATAL, "%s, kmap failed\n", __func__);
			goto error_2;
		}

		ps_m_buf->start_vir_addr = virt_addr;
	}

	ps_m_buf->start_phy_addr = iova_addr;
	ps_m_buf->size = phy_size;
	ps_m_buf->share_fd = (uint32_t)share_fd;

	dma_buf_put(dmabuf);

	vdec_mutex_unlock(&g_mem_mutex);
	return ret;

error_2:
	rc = dma_buf_end_cpu_access(dmabuf, DMA_TO_DEVICE);
	if (rc < 0)
		dprint(PRN_ERROR, "%s: dma buf end cpu access failed\n", __func__);
error_1:
	ret = kernel_iommu_unmap_dmabuf(vdec_entry.device, dmabuf,  phy_size);
	if (ret != 0)
		dprint(PRN_ERROR, "%s: kernel iommu unmap dmabuf failed\n", __func__);
exit:
	dma_buf_put(dmabuf);
	vdec_mutex_unlock(&g_mem_mutex);
	return VCODEC_FAILURE;
}

/* func: unmap dma buffer */
int32_t vdec_mem_put_map_info(mem_buffer_s *ps_m_buf)
{
	int32_t rc;
	struct dma_buf *dmabuf = NULL;
	int32_t ret;
	omx_vdec_entry vdec_entry;

	if (ps_m_buf == VCODEC_NULL) {
		dprint(PRN_ERROR, "%s: ps_m_buf is NULL\n", __func__);
		return VCODEC_FAILURE;
	}

	vdec_mutex_lock(&g_mem_mutex);

	vdec_entry = omx_vdec_get_entry();

	dmabuf = dma_buf_get(ps_m_buf->share_fd);
	if (IS_ERR(dmabuf)) {
		dprint(PRN_FATAL, "%s, dma_buf_get failed share fd %d\n", __func__, ps_m_buf->share_fd);
		vdec_mutex_unlock(&g_mem_mutex);
		return VCODEC_FAILURE;
	}

	ret = kernel_iommu_unmap_dmabuf(vdec_entry.device,
		dmabuf,  ps_m_buf->start_phy_addr);
	if (ret != 0) {
		dprint(PRN_ERROR, "%s: kernel iommu unmap dmabuf failed\n", __func__);
		ret = VCODEC_FAILURE;
		goto exit;
	}

	if (ps_m_buf->is_map_virtual == 1) {
		dma_buf_kunmap(dmabuf, 0, NULL);

		rc = dma_buf_end_cpu_access(dmabuf, DMA_TO_DEVICE);
		if (rc != 0) {
			dprint(PRN_ERROR, "%s: end cpu access failed\n", __func__);
			ret = VCODEC_FAILURE;
		}
	}

exit:
	dma_buf_put(dmabuf);
	vdec_mutex_unlock(&g_mem_mutex);
	return ret;
}

int32_t vdec_mem_iommu_map(int32_t share_fd, UADDR *iova)
{
	uint64_t iova_addr;
	unsigned long phy_size;
	struct dma_buf *dmabuf = NULL;
	omx_vdec_entry vdec_entry;

	if (share_fd < 0) {
		dprint(PRN_ERROR, "%s, share fd is invalid \n", __func__);
		return VCODEC_FAILURE;
	}

	vdec_mutex_lock(&g_mem_mutex);

	vdec_entry = omx_vdec_get_entry();

	dmabuf = dma_buf_get(share_fd);
	if (IS_ERR(dmabuf)) {
		dprint(PRN_FATAL, "%s, dma_buf_get failed", __func__);
		vdec_mutex_unlock(&g_mem_mutex);
		return VCODEC_FAILURE;
	}

	iova_addr = kernel_iommu_map_dmabuf(vdec_entry.device,
		dmabuf, 0, &phy_size);
	if (!iova_addr) {
		dprint(PRN_FATAL, "%s, kernel_iommu_map_dmabuf failed", __func__);
		dma_buf_put(dmabuf);
		*iova = 0;
		vdec_mutex_unlock(&g_mem_mutex);
		return VCODEC_FAILURE;
	}

	*iova = iova_addr;

	dma_buf_put(dmabuf);
	vdec_mutex_unlock(&g_mem_mutex);

	return 0;
}

int32_t vdec_mem_iommu_unmap(int32_t share_fd, UADDR iova)
{
	struct dma_buf *dmabuf = NULL;
	int32_t ret;
	omx_vdec_entry vdec_entry;

	if (share_fd < 0) {
		dprint(PRN_ERROR, "%s, share fd is invalid\n", __func__);
		return VCODEC_FAILURE;
	}

	if (!iova) {
		dprint(PRN_ERROR, "%s, iova is invalid\n", __func__);
		return VCODEC_FAILURE;
	}

	vdec_mutex_lock(&g_mem_mutex);

	vdec_entry = omx_vdec_get_entry();

	dmabuf = dma_buf_get(share_fd);
	if (IS_ERR(dmabuf)) {
		dprint(PRN_FATAL, "%s, dma buf get failed\n", __func__);
		vdec_mutex_unlock(&g_mem_mutex);
		return VCODEC_FAILURE;
	}

	ret = kernel_iommu_unmap_dmabuf(vdec_entry.device, dmabuf, iova);
	if (ret) {
		dprint(PRN_ERROR, "%s: kernel iommu unmap dmabuf failed\n", __func__);
		ret = VCODEC_FAILURE;
	}

	dma_buf_put(dmabuf);

	vdec_mutex_unlock(&g_mem_mutex);
	return ret;
}

