/*
 * drv_mem.c
 *
 * This is for venc drv.
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

#include "drv_mem.h"
#include <asm/uaccess.h>
#include <linux/version.h>
#include <linux/dma-mapping.h>
#include <linux/dma-iommu.h>
#include <linux/dma-buf.h>
#include <linux/iommu.h>
#include <linux/mm_iommu.h>
#include "drv_common.h"

#define  MAX_BUFFER_SIZE (10*1024)

char *g_sbuf;
int32_t   g_venc_node_num;

struct semaphore    g_venc_mem_sem;
venc_mem_buf_t g_venc_mem_node[MAX_KMALLOC_MEM_NODE];

venc_smmu_err_add_t g_smmu_err_mem;

int32_t drv_mem_init(void)
{
	char *sbuf = NULL;
	int32_t ret;
	mem_buffer_t mem_smmu_read_addr;
	mem_buffer_t mem_smmu_write_addr;

	vcodec_venc_init_sem(&g_venc_mem_sem);

	sbuf = vcodec_mem_valloc(MAX_BUFFER_SIZE);
	if (!sbuf) {
		VCODEC_FATAL_VENC("call vmalloc failed");
		goto err_client_exit;
	}

	(void)memset_s((void *)g_venc_mem_node, sizeof(g_venc_mem_node), 0, sizeof(g_venc_mem_node));
	(void)memset_s((void *)&g_smmu_err_mem, sizeof(g_smmu_err_mem), 0, sizeof(g_smmu_err_mem));
	(void)memset_s((void *)&mem_smmu_read_addr, sizeof(mem_buffer_t), 0, sizeof(mem_buffer_t));
	(void)memset_s((void *)&mem_smmu_write_addr, sizeof(mem_buffer_t), 0, sizeof(mem_buffer_t));

	mem_smmu_read_addr.size = SMMU_RWERRADDR_SIZE;
	ret = drv_mem_kalloc("SMMU_RDERR", "OMXVENC", &mem_smmu_read_addr);
	if (ret != 0) {
		VCODEC_ERR_VENC("SMMU_RDERR alloc failed");
		goto err_sbuf_exit;
	}

	mem_smmu_write_addr.size = SMMU_RWERRADDR_SIZE;
	ret = drv_mem_kalloc("SMMU_WRERR", "OMXVENC", &mem_smmu_write_addr);
	if (ret != 0) {
		VCODEC_ERR_VENC("SMMU_WRERR alloc failed");
		goto err_rd_smmu_exit;
	}

	g_smmu_err_mem.read_addr = mem_smmu_read_addr.start_phys_addr;    // config alloc phyaddr,in order system don't dump
	g_smmu_err_mem.write_addr = mem_smmu_write_addr.start_phys_addr;
	g_smmu_err_mem.size = SMMU_RWERRADDR_SIZE;
	g_venc_node_num = 0;

	g_sbuf = sbuf;
	(void)memset_s((void *)g_sbuf, MAX_BUFFER_SIZE, 0, MAX_BUFFER_SIZE);

	return 0;

err_rd_smmu_exit:
	drv_mem_kfree(&mem_smmu_read_addr);
err_sbuf_exit:
	vcodec_mem_vfree(sbuf);
err_client_exit:
	return VCODEC_FAILURE;
}

int32_t drv_mem_exit(void)
{
	int32_t i;

	if (g_sbuf) {
		vcodec_mem_vfree(g_sbuf);
		g_sbuf = VCODEC_NULL;
	}

	/* Exit kfree mem for register's VEDU_COMN1_REGS.COMN1_SMMU_ERR_RDADDRR */
	for (i = 0; i < MAX_KMALLOC_MEM_NODE; i++) {
		if (g_venc_mem_node[i].virt_addr != VCODEC_NULL) {
			kfree(g_venc_mem_node[i].virt_addr);
			(void)memset_s(&g_venc_mem_node[i], sizeof(g_venc_mem_node[i]), 0, sizeof(g_venc_mem_node[i]));
		}
	}

	g_venc_node_num = 0;

	return 0;
}

int32_t drv_mem_kalloc(const char *bufname, const char *zone_name, mem_buffer_t *psmbuf)
{
	uint32_t  i;
	int32_t  ret = VCODEC_FAILURE;
	void *virt_addr = VCODEC_NULL;

	if (psmbuf == VCODEC_NULL || psmbuf->size == 0) {
		VCODEC_FATAL_VENC("invalid Param, psmbuf is NULL or size is zero");
		return ret;
	}

	if (vcodec_venc_down_interruptible(&g_venc_mem_sem)) {
		VCODEC_FATAL_VENC("Kalloc, down_interruptible failed");
		return ret;
	}

	for (i = 0; i < MAX_KMALLOC_MEM_NODE; i++) {
		if ((g_venc_mem_node[i].phys_addr == 0) && (g_venc_mem_node[i].virt_addr == VCODEC_NULL))
			break;
	}

	if (i == MAX_KMALLOC_MEM_NODE) {
		VCODEC_FATAL_VENC("No free ion mem node");
		goto err_exit;
	}

	virt_addr = kzalloc(psmbuf->size, GFP_KERNEL | GFP_DMA); /* lint !e747 */
	if (IS_ERR_OR_NULL(virt_addr)) {
		VCODEC_FATAL_VENC("call kzalloc failed, size : %d", psmbuf->size);
		goto err_exit;
	}

	psmbuf->start_virt_addr   = virt_addr;
	psmbuf->start_phys_addr = __pa((uintptr_t)virt_addr); /* lint !e648 !e834 !e712 */

	ret = snprintf_s(g_venc_mem_node[i].node_name, MAX_MEM_NAME_LEN, MAX_MEM_NAME_LEN - 1, "%s", bufname);
	if (ret < 0) {
		VCODEC_FATAL_VENC("call snprintf_s failed");
		goto err_snprintf_s;
	}

	ret = snprintf_s(g_venc_mem_node[i].zone_name, MAX_MEM_NAME_LEN, MAX_MEM_NAME_LEN - 1, "%s", zone_name);
	if (ret < 0) {
		VCODEC_FATAL_VENC("call snprintf_s failed");
		goto err_snprintf_s;
	}

	g_venc_mem_node[i].virt_addr = psmbuf->start_virt_addr;
	g_venc_mem_node[i].phys_addr = psmbuf->start_phys_addr;
	g_venc_mem_node[i].size      = psmbuf->size;

	g_venc_node_num++;

	vcodec_venc_up_interruptible(&g_venc_mem_sem);
	return 0;

err_snprintf_s:
	kfree(virt_addr);
	virt_addr = VCODEC_NULL;
	psmbuf->start_virt_addr = VCODEC_NULL;
	psmbuf->start_phys_addr = VCODEC_NULL;
err_exit:
	vcodec_venc_up_interruptible(&g_venc_mem_sem);
	return ret; /*lint !e593*/
}

int32_t drv_mem_kfree(const mem_buffer_t *psmbuf)
{
	uint32_t  i;
	int32_t  ret = VCODEC_FAILURE;

	if (psmbuf == VCODEC_NULL || psmbuf->start_virt_addr == VCODEC_NULL || psmbuf->start_phys_addr == 0) {
		VCODEC_FATAL_VENC("invalid Parameters");
		return ret;
	}

	if (vcodec_venc_down_interruptible(&g_venc_mem_sem)) {
		VCODEC_FATAL_VENC("Kfree, down interruptible failed");
		return ret;
	}

	for (i = 0; i < MAX_KMALLOC_MEM_NODE; i++)
		if ((psmbuf->start_phys_addr == g_venc_mem_node[i].phys_addr) &&
			(psmbuf->start_virt_addr == g_venc_mem_node[i].virt_addr))
			break;

	if (i == MAX_KMALLOC_MEM_NODE) {
		VCODEC_FATAL_VENC("No free ion mem node");
		goto err_exit;
	}

	kfree(g_venc_mem_node[i].virt_addr);
	(void)memset_s(&g_venc_mem_node[i], sizeof(g_venc_mem_node[i]), 0, sizeof(g_venc_mem_node[i]));
	g_venc_node_num = (g_venc_node_num > 0) ? (g_venc_node_num - 1) : 0;

	ret = 0;

err_exit:
	vcodec_venc_up_interruptible(&g_venc_mem_sem);
	return ret;
}

int32_t drv_mem_get_map_info(int32_t shared_fd, mem_buffer_t *psmbuf)
{
	uint64_t iova_addr;
	struct dma_buf *dmabuf = NULL;
	int32_t ret = 0;
	unsigned long iova_size = 0;

	struct platform_device *venc_dev = NULL;

	if (shared_fd < 0) {
		VCODEC_FATAL_VENC("invalid Param, shared_fd is illegal");
		return VCODEC_FAILURE;
	}

	if (vcodec_venc_down_interruptible(&g_venc_mem_sem)) {
		VCODEC_FATAL_VENC("Map down interruptible failed");
		return VCODEC_FAILURE;
	}

	dmabuf = dma_buf_get(shared_fd);
	if (IS_ERR(dmabuf)) {
		VCODEC_FATAL_VENC("%s, get dma buf failed", __func__);
		ret = VCODEC_FAILURE;
		goto exit_1;
	}

	venc_dev = venc_get_device();

	iova_addr = kernel_iommu_map_dmabuf(&venc_dev->dev, dmabuf, 0, &iova_size);
	if (!iova_addr) {
		VCODEC_FATAL_VENC("%s, iommu map dmabuf failed", __func__);
		ret = VCODEC_FAILURE;
		goto exit_2;
	}

	psmbuf->size = iova_size;
	psmbuf->start_phys_addr = iova_addr;
	psmbuf->share_fd = shared_fd;

exit_2:
	dma_buf_put(dmabuf);
exit_1:
	vcodec_venc_up_interruptible(&g_venc_mem_sem);
	return ret;
}

int32_t drv_mem_put_map_info(mem_buffer_t *psmbuf)
{
	struct dma_buf *dmabuf = NULL;
	int32_t ret;
	struct platform_device *venc_dev = NULL;

	if (!psmbuf) {
		VCODEC_FATAL_VENC("invalid Param, psmbuf is NULL");
		return VCODEC_FAILURE;
	}

	if (psmbuf->share_fd < 0) {
		VCODEC_FATAL_VENC("share fd is invalid");
		return VCODEC_FAILURE;
	}

	if (vcodec_venc_down_interruptible(&g_venc_mem_sem)) {
		VCODEC_FATAL_VENC("Unmap down interruptible failed");
		return VCODEC_FAILURE;
	}

	dmabuf = dma_buf_get(psmbuf->share_fd);
	if (IS_ERR(dmabuf)) {
		VCODEC_FATAL_VENC("%s, get dma buf failed", __func__);
		vcodec_venc_up_interruptible(&g_venc_mem_sem);
		return VCODEC_FAILURE;
	}

	venc_dev = venc_get_device();

	ret = kernel_iommu_unmap_dmabuf(&venc_dev->dev, dmabuf, psmbuf->start_phys_addr);
	if (ret != 0) {
		VCODEC_FATAL_VENC("%s: kernel_iommu_unmap_dmabuf failed", __func__);
		ret = VCODEC_FAILURE;
	}

	dma_buf_put(dmabuf);
	vcodec_venc_up_interruptible(&g_venc_mem_sem);
	return ret;
}

int32_t drv_mem_iommumap(venc_buffer_record_t *node, struct platform_device *pdev)
{
	uint64_t iova_addr;
	struct dma_buf *dmabuf = NULL;

	int32_t ret = 0;
	unsigned long iova_size = 0;

	if (!node) {
		VCODEC_FATAL_VENC("node is invalid");
		return VCODEC_FAILURE;
	}

	if (node->shared_fd < 0) {
		VCODEC_FATAL_VENC("share fd is invalid");
		return VCODEC_FAILURE;
	}

	if (vcodec_venc_down_interruptible(&g_venc_mem_sem)) {
		VCODEC_FATAL_VENC("Map down interruptible failed");
		return VCODEC_FAILURE;
	}

	dmabuf = dma_buf_get(node->shared_fd);
	if (IS_ERR(dmabuf)) {
		VCODEC_FATAL_VENC("%s, get dma buf failed", __func__);
		ret = VCODEC_FAILURE;
		goto exit_1;
	}

	iova_addr = kernel_iommu_map_dmabuf(&pdev->dev, dmabuf, 0, &iova_size);
	if (!iova_addr) {
		VCODEC_FATAL_VENC("%s, iommu map dmabuf failed", __func__);
		ret = VCODEC_FAILURE;
		goto exit_2;
	}

	node->iova = iova_addr;

exit_2:
	dma_buf_put(dmabuf);
exit_1:
	vcodec_venc_up_interruptible(&g_venc_mem_sem);
	return ret;
}

int32_t drv_mem_iommuunmap(int32_t shared_fd, int32_t phys_addr, struct platform_device *pdev)
{
	struct dma_buf *dmabuf = NULL;
	int32_t ret;

	if (shared_fd < 0) {
		VCODEC_FATAL_VENC("share fd is invalid");
		return VCODEC_FAILURE;
	}

	if (!phys_addr) {
		VCODEC_FATAL_VENC("phys addr is invalid");
		return VCODEC_FAILURE;
	}

	if (vcodec_venc_down_interruptible(&g_venc_mem_sem)) {
		VCODEC_FATAL_VENC("IommuUnMap down interruptible failed");
		return VCODEC_FAILURE;
	}

	dmabuf = dma_buf_get(shared_fd);
	if (IS_ERR(dmabuf)) {
		VCODEC_FATAL_VENC("%s, get dma buf failed", __func__);
		vcodec_venc_up_interruptible(&g_venc_mem_sem);
		return VCODEC_FAILURE;
	}

	ret = kernel_iommu_unmap_dmabuf(&pdev->dev, dmabuf, phys_addr);
	if (ret != 0) {
		VCODEC_FATAL_VENC("%s: kernel_iommu_unmap_dmabuf failed", __func__);
		ret = VCODEC_FAILURE;
	}

	dma_buf_put(dmabuf);

	vcodec_venc_up_interruptible(&g_venc_mem_sem);
	return ret;
}

int32_t drv_mem_copy_from_user(uint32_t cmd, const void __user *user_arg, void **kernel_arg)
{
	int32_t err = VCODEC_FAILURE;

	/* Copy arguments into temp kernel buffer */
	if (!(void __user *)user_arg || !kernel_arg) {
		VCODEC_FATAL_VENC("arg is NULL");
		return err;
	}

	if (_IOC_SIZE(cmd) <= MAX_BUFFER_SIZE) {
		*kernel_arg = g_sbuf;
	} else {
		VCODEC_FATAL_VENC("cmd size is too long");
		return err;
	}

	if (!(*kernel_arg)) {
		VCODEC_FATAL_VENC("parg is NULL");
		return err;
	}

	if (_IOC_DIR(cmd) & _IOC_WRITE) {
		if (copy_from_user(*kernel_arg, (void __user *)(uintptr_t)user_arg, _IOC_SIZE(cmd))) { /*lint !e747*/
			VCODEC_FATAL_VENC("copy_from_user failed, cmd value is 0x%x", cmd);
			return err;
		}
	}

	return 0;
}

struct mem_pool *drv_mem_create_pool(struct device *dev, uint32_t each_size, uint32_t num, uint32_t align_size)
{
	struct mem_info buffer;
	uint32_t i;
	uint64_t phy_addr;
	uint32_t size;
	struct mem_pool *pool = NULL;

	if (each_size == 0 || num == 0) {
		VCODEC_FATAL_VENC("parameter is invalid");
		return NULL;
	}

	pool = create_queue(struct mem_pool);
	if (pool == NULL)
		return NULL;

	if (alloc_queue(pool, num) != 0)
		goto alloc_queue_failed;

	size = each_size * num + align_size;
	pool->dev = dev;
	pool->num = num;
	pool->each_size = each_size;
	pool->align_size = align_size;
	pool->start_virt_addr = dma_alloc_coherent(dev, size, &pool->start_phy_addr, GFP_KERNEL);
	if (IS_ERR_OR_NULL(pool->start_virt_addr))
		goto alloc_buffer_failed;

	phy_addr = align_up(pool->start_phy_addr, align_size);
	pool->aligned_virt_addr = (void *)(uintptr_t)align_up((uintptr_t)pool->start_virt_addr, (uint64_t)align_size);
	pool->aligned_iova_addr = mm_iommu_map(dev, phy_addr, pool->each_size * pool->num, IOMMU_READ | IOMMU_WRITE);
	if (!pool->aligned_iova_addr)
		goto map_failed;

	for (i = 0; i < num; i++) {
		buffer.virt_addr = pool->aligned_virt_addr + i * (uint64_t)each_size;
		buffer.iova_addr = pool->aligned_iova_addr + i * (uint64_t)each_size;
		if (push(pool, &buffer) != 0)
			goto push_queue_failed;
	}
	return pool;

push_queue_failed:
	(void)mm_iommu_unmap(pool->dev, pool->aligned_iova_addr, pool->each_size * pool->num);
map_failed:
	dma_free_coherent(pool->dev, size, pool->start_virt_addr, pool->start_phy_addr);
alloc_buffer_failed:
	free_queue(pool);
alloc_queue_failed:
	destory_queue(pool);

	return NULL;
}

void drv_mem_destory_pool(struct mem_pool *pool)
{
	uint32_t size = pool->each_size * pool->num + pool->align_size;

	(void)mm_iommu_unmap(pool->dev, pool->aligned_iova_addr, pool->each_size * pool->num);
	dma_free_coherent(pool->dev, size, pool->start_virt_addr, pool->start_phy_addr);
	free_queue(pool);
	destory_queue(pool);
}

void *drv_mem_pool_alloc(struct mem_pool *pool, uint64_t *iova_addr)
{
	struct mem_info buffer;

	if (pool == NULL || iova_addr == NULL) {
		VCODEC_FATAL_VENC("parameter is invalid");
		return NULL;
	}

	(void)memset_s((void *)&buffer, sizeof(struct mem_info), 0, sizeof(struct mem_info));
	if (pop(pool, &buffer)) {
		VCODEC_FATAL_VENC("get memory failed");
		*iova_addr = 0;
		return NULL;
	}

	*iova_addr = buffer.iova_addr;

	return buffer.virt_addr;
}

void drv_mem_pool_free(struct mem_pool *pool, void *virt_addr, uint64_t iova_addr)
{
	struct mem_info buffer;

	if (pool == NULL || virt_addr == NULL) {
		VCODEC_FATAL_VENC("parameter is invalid");
		return;
	}

	if (((uintptr_t)virt_addr < (uintptr_t)pool->aligned_virt_addr) ||
		((uintptr_t)virt_addr > (uintptr_t)pool->aligned_virt_addr + pool->num * pool->each_size)) {
		VCODEC_FATAL_VENC("start virt_addr %pK, vir_addr %pK, free memory fail",
			pool->aligned_virt_addr, virt_addr);
		return;
	}

	if ((iova_addr < pool->aligned_iova_addr) ||
		(iova_addr > pool->aligned_iova_addr + pool->num * (uint64_t)pool->each_size)) {
		VCODEC_FATAL_VENC("start iova addr free memory fail");
		return;
	}

	buffer.iova_addr = iova_addr;
	buffer.virt_addr = virt_addr;
	if (push(pool, &buffer))
		VCODEC_FATAL_VENC("put memory failed");
}

int32_t drv_mem_alloc_dma(struct device *dev, uint32_t size, struct venc_dma_mem *mem)
{
	void *virt_addr = NULL;
	dma_addr_t dma_handle = 0;
	unsigned long iova_addr;
	size_t aligned_size;

	if (!dev || !mem || !size)
		return VCODEC_FAILURE;

	aligned_size = PAGE_ALIGN(size);
	virt_addr = dma_alloc_coherent(dev, aligned_size, &dma_handle, GFP_KERNEL);
	if (!virt_addr) {
		VCODEC_ERR_VENC("dma alloc failed");
		return VCODEC_FAILURE;
	}

	iova_addr = mm_iommu_map(dev, dma_handle, aligned_size, IOMMU_READ | IOMMU_WRITE);
	if (iova_addr == 0) {
		VCODEC_ERR_VENC("iommu map failed");
		dma_free_coherent(dev, aligned_size, virt_addr, dma_handle);
		return VCODEC_FAILURE;
	}

	mem->size = size;
	mem->dma_handle = dma_handle;
	mem->iova_addr = iova_addr;
	mem->virt_addr = virt_addr;
	return 0;
}

void drv_mem_free_dma(struct device *dev, struct venc_dma_mem *mem)
{
	int ret;
	size_t aligned_size;

	if (!dev || !mem)
		return;

	aligned_size = PAGE_ALIGN(mem->size);
	ret = mm_iommu_unmap(dev, mem->iova_addr, aligned_size);
	if (ret != 0)
		VCODEC_ERR_VENC("unmap dma fail");

	dma_free_coherent(dev, aligned_size, mem->virt_addr, mem->dma_handle);
}
