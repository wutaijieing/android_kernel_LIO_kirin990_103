/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mm_virtio_iommu.h>
#include <linux/dma-buf.h>
#include "dpufe_dbg.h"
#include "dpufe_iommu.h"

struct dpu_mm_info {
	struct list_head mm_list;
	spinlock_t map_lock;
};

struct dpu_iova_info {
	struct list_head list_node;
	struct dma_buf *dmabuf;
	iova_info_t iova_info;
};

static struct dpu_mm_info g_mm_list;
void dpufe_iommu_enable(void)
{
	struct dpu_mm_info *mm_list = &g_mm_list;

	spin_lock_init(&mm_list->map_lock);
	INIT_LIST_HEAD(&mm_list->mm_list);
}

struct dma_buf *dpufe_get_dmabuf(int sharefd)
{
	struct dma_buf *buf = NULL;

	/* dim layer share fd -1 */
	if (sharefd < 0) {
		DPUFE_ERR("Invalid file sharefd = %d\n", sharefd);
		return NULL;
	}

	buf = dma_buf_get(sharefd);
	if (IS_ERR_OR_NULL(buf)) {
		DPUFE_ERR("Invalid file buf(%pK), sharefd = %d\n", buf, sharefd);
		return NULL;
	}

	return buf;
}

void dpufe_put_dmabuf(struct dma_buf *buf)
{
	if (IS_ERR_OR_NULL(buf)) {
		DPUFE_ERR("Invalid dmabuf(%pK)\n", buf);
		return;
	}

	dma_buf_put(buf);
}

struct dma_buf* dpufe_get_buffer_by_sharefd(uint64_t *iova, int fd)
{
	struct dma_buf *buf = NULL;

	buf = dpufe_get_dmabuf(fd);
	if (IS_ERR_OR_NULL(buf)) {
		DPUFE_ERR("Invalid file shared_fd[%d]\n", fd);
		return NULL;
	}

	*iova = viommu_map_dmabuf(SMMUID_DSS, buf);
	if (*iova == 0) {
		DPUFE_ERR("map iova fail, shared_fd[%d]\n", fd);
		dpufe_put_dmabuf(buf);
		return NULL;
	}

	return buf;
}

static void dpufe_put_buffer_by_sharefd(uint64_t iova, struct dma_buf *dmabuf)
{
	if (IS_ERR_OR_NULL(dmabuf)) {
		DPUFE_ERR("Invalid dmabuf(%pK)\n", dmabuf);
		return;
	}
	viommu_unmap_dmabuf(SMMUID_DSS, dmabuf, iova);
	dpufe_put_dmabuf(dmabuf);
}

int dpufe_buffer_map_iova(void __user *arg)
{
	struct dpu_iova_info *node = NULL;
	struct dpu_mm_info *mm_list = &g_mm_list;
	iova_info_t map_data;

	if (!arg) {
		DPUFE_ERR("arg is NULL\n");
		return -EFAULT;
	}
	node = kzalloc(sizeof(struct dpu_iova_info), GFP_KERNEL);
	if (!node) {
		DPUFE_ERR("alloc display meminfo failed\n");
		goto error;
	}

	if (copy_from_user(&map_data, (void __user *)arg, sizeof(map_data))) {
		DPUFE_ERR("copy_from_user failed\n");
		goto error;
	}

	node->iova_info.share_fd = map_data.share_fd;
	node->iova_info.calling_pid = map_data.calling_pid;
	node->iova_info.size = map_data.size;
	node->dmabuf = dpufe_get_buffer_by_sharefd(&map_data.iova, map_data.share_fd);
	if (!node->dmabuf) {
		DPUFE_ERR("dma buf map share_fd(%d) failed\n", map_data.share_fd);
		goto error;
	}

	DPUFE_DEBUG("pid:%d iova:0x%lx size:0x%lx\n", map_data.calling_pid, map_data.iova, map_data.size);
	node->iova_info.iova = map_data.iova;

	if (copy_to_user((void __user *)arg, &map_data, sizeof(map_data))) {
		DPUFE_ERR("copy_to_user failed\n");
		goto error;
	}

	/* save map list */
	spin_lock(&mm_list->map_lock);
	list_add_tail(&node->list_node, &mm_list->mm_list);
	spin_unlock(&mm_list->map_lock);

	return 0;

error:
	if (node) {
		if (node->dmabuf)
			dpufe_put_buffer_by_sharefd(node->iova_info.iova, node->dmabuf);
		kfree(node);
	}

	return -EFAULT;
}

int dpufe_buffer_unmap_iova(const void __user *arg)
{
	struct dma_buf *dmabuf = NULL;
	struct dpu_iova_info *node = NULL;
	struct dpu_iova_info *_node_ = NULL;
	struct dpu_mm_info *mm_list = &g_mm_list;
	iova_info_t umap_data;

	if (!arg) {
		DPUFE_ERR("arg is NULL\n");
		return -EFAULT;
	}

	if (copy_from_user(&umap_data, (void __user *)arg, sizeof(umap_data))) {
		DPUFE_ERR("copy_from_user failed\n");
		return -EFAULT;
	}
	dmabuf = dpufe_get_dmabuf(umap_data.share_fd);

	spin_lock(&mm_list->map_lock);
	list_for_each_entry_safe(node, _node_, &mm_list->mm_list, list_node) {
		if (node->dmabuf == dmabuf) {
			list_del(&node->list_node);
			/* already map, need put twice. */
			dpufe_put_dmabuf(node->dmabuf);
			/* iova would be unmapped by dmabuf put. */
			kfree(node);
		}
	}
	spin_unlock(&mm_list->map_lock);
	dpufe_put_dmabuf(dmabuf);

	return 0;
}
