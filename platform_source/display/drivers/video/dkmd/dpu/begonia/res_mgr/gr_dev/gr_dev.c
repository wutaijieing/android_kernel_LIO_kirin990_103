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

#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mm_iommu.h>
#include "gr_dev.h"
#include "res_mgr.h"
#include "dkmd_log.h"

static struct platform_device *g_rm_device;

static void *gr_dev_init(struct dpu_res_data *rm_data)
{
	struct dpu_res *rm = NULL;
	struct dpu_gr_dev *gr_dev = kzalloc(sizeof(struct dpu_gr_dev), GFP_KERNEL);
	if (!gr_dev)
		return NULL;

	INIT_LIST_HEAD(&gr_dev->mem_info.mem_info_head);
	spin_lock_init(&gr_dev->mem_info.mem_lock);

	rm = container_of(rm_data, struct dpu_res, data);

	g_rm_device = rm->pdev;

	if (dma_set_mask_and_coherent(&rm->pdev->dev, DMA_BIT_MASK(64)) != 0)
		dpu_pr_err("dma set mask and coherent failed\n");

	return gr_dev;
}

static void gr_dev_deinit(void *data)
{
	struct dpu_gr_dev *gr_dev = NULL;

	if (!data)
		return;

	gr_dev = (struct dpu_gr_dev *)data;
	kfree(gr_dev);
}

static struct dma_buf *gr_dev_get_dmabuf(int32_t sharefd)
{
	struct dma_buf *buf = NULL;

	if (sharefd < 0) {
		dpu_pr_err("Invalid file sharefd = %d", sharefd);
		return NULL;
	}

	buf = dma_buf_get(sharefd);
	if (IS_ERR_OR_NULL(buf)) {
		dpu_pr_err("Invalid file buf(%pK), sharefd = %d", buf, sharefd);
		return NULL;
	}

	return buf;
}

void gr_dev_put_dmabuf(struct dma_buf *buf)
{
	if (IS_ERR_OR_NULL(buf)) {
		dpu_pr_err("Invalid dmabuf(%pK)", buf);
		return;
	}

	dma_buf_put(buf);
}

static struct dma_buf *gr_dev_map_dmabuf_by_sharefd(uint64_t *iova, int32_t fd, uint32_t size)
{
	unsigned long buf_size = 0;
	struct dma_buf *buf = gr_dev_get_dmabuf(fd);

	if (IS_ERR_OR_NULL(buf)) {
		dpu_pr_err("Invalid file share_fd[%d]", fd);
		return NULL;
	}

	*iova = kernel_iommu_map_dmabuf(dpu_res_get_device(), buf, 0, &buf_size);
	if ((*iova == 0) || (buf_size < size)) {
		dpu_pr_err("get iova_size(0x%llx) smaller than size(0x%x)", buf_size, size);
		if (*iova != 0) {
			(void)kernel_iommu_unmap_dmabuf(dpu_res_get_device(), buf, *iova);
			*iova = 0;
		}
		gr_dev_put_dmabuf(buf);
		return NULL;
	}

	return buf;
}

static void gr_dev_unmap_dmabuf(uint64_t iova, struct dma_buf *dmabuf)
{
	if (IS_ERR_OR_NULL(dmabuf)) {
		dpu_pr_err("Invalid dmabuf(%pK)", dmabuf);
		return;
	}
	(void)kernel_iommu_unmap_dmabuf(dpu_res_get_device(), dmabuf, iova);

	gr_dev_put_dmabuf(dmabuf);
}

static int32_t gr_dev_map_iova_buffer(void *data, void __user *argp)
{
	struct dpu_gr_dev *gr_dev = (struct dpu_gr_dev *)data;
	struct res_dma_buf buf_data;
	struct dpu_iova_info *node = kzalloc(sizeof(struct dpu_iova_info), GFP_KERNEL);

	if (!node) {
		dpu_pr_err("alloc dpu iova_info fail");
		return -EINVAL;
	}

	if (!argp) {
		dpu_pr_err("argp is nullptr");
		kfree(node);
		return -1;
	}

	if (copy_from_user(&buf_data, argp, sizeof(buf_data))) {
		dpu_pr_err("iova_info copy_from_user fail");
		goto OUT;
	}

	node->map_buf.share_fd = buf_data.share_fd;
	node->map_buf.size = buf_data.size;
	node->dmabuf = gr_dev_map_dmabuf_by_sharefd(&buf_data.iova, buf_data.share_fd, buf_data.size);
	if (!node->dmabuf) {
		dpu_pr_err("dma buf map share_fd(%d) failed", buf_data.share_fd);
		goto OUT;
	}
	node->map_buf.iova = buf_data.iova;

	if (copy_to_user(argp, &buf_data, sizeof(buf_data)) != 0) {
		dpu_pr_err("copy_to_user failed");
		goto OUT;
	}

	spin_lock(&gr_dev->mem_info.mem_lock);
	list_add_tail(&node->iova_info_head, &gr_dev->mem_info.mem_info_head);
	spin_unlock(&gr_dev->mem_info.mem_lock);

	dpu_pr_debug("share_fd = %d map iova = 0x%llx  size = %llu",
				 buf_data.share_fd, buf_data.iova, buf_data.size);

	return 0;

OUT:
	if (node) {
		if (node->dmabuf)
			gr_dev_unmap_dmabuf(node->map_buf.iova, node->dmabuf);
		kfree(node);
	}
	return -EFAULT;
}

static int32_t gr_dev_unmap_iova_buffer(void *data, const void __user *argp)
{
	struct dpu_gr_dev *gr_dev = (struct dpu_gr_dev *)data;
	struct res_dma_buf buf_data;
	struct dma_buf *dmabuf = NULL;
	struct dpu_iova_info *node = NULL;
	struct dpu_iova_info *_node_ = NULL;

	if (!argp) {
		dpu_pr_err("argp is nullptr");
		return -1;
	}

	if (copy_from_user(&buf_data, argp, sizeof(buf_data))) {
		dpu_pr_err("copy_from_user failed\n");
		return -EFAULT;
	}

	dmabuf = gr_dev_get_dmabuf(buf_data.share_fd);
	spin_lock(&gr_dev->mem_info.mem_lock);
	list_for_each_entry_safe(node, _node_, &gr_dev->mem_info.mem_info_head, iova_info_head) {
		if (node->dmabuf == dmabuf) {
			list_del(&node->iova_info_head);
			/* already map, need put twice. */
			gr_dev_put_dmabuf(node->dmabuf);

			dpu_pr_debug("share_fd = %d unmap iova = 0x%llx  size = %llu",
				 		node->map_buf.share_fd, node->map_buf.iova, node->map_buf.size);
			/* iova would be unmapped by dmabuf put. */
			kfree(node);
		}
	}
	spin_unlock(&gr_dev->mem_info.mem_lock);

	gr_dev_put_dmabuf(dmabuf);
	return 0;
}

static int32_t gr_dev_get_vscreen_info(void __user *argp)
{
	struct screen_info info;

	if (!argp) {
		dpu_pr_err("argp is nullptr");
		return -1;
	}

	dpu_config_get_screen_info(&info.xres, &info.yres);
	dpu_pr_info("get screen xres = %u, yres = %u", info.xres, info.yres);

	if (copy_to_user(argp, &info, sizeof(info)) != 0) {
		dpu_pr_debug("copy to user failed!");
		return -EFAULT;
	}

	return 0;
}

static int32_t gr_dev_get_dpu_version(void __user *argp)
{
	uint64_t value = dpu_config_get_version_value();

	if (!argp) {
		dpu_pr_debug("NULL Pointer!\n");
		return -EINVAL;
	}

	if (copy_to_user(argp, &value, sizeof(value)) != 0) {
		dpu_pr_debug("copy to user failed!");
		return -EFAULT;
	}

	return 0;
}

static long gr_dev_ioctl(struct dpu_res_resouce_node *res_node, uint32_t cmd, void __user *argp)
{
	if (!res_node || !res_node->data || !argp) {
		dpu_pr_err("res_node or node data or argp is NULL");
		return -EINVAL;
	}

	switch (cmd) {
	case RES_MAP_IOVA:
		return gr_dev_map_iova_buffer(res_node->data, argp);
	case RES_UNMAP_IOVA:
		return gr_dev_unmap_iova_buffer(res_node->data, argp);
	case RES_GET_VSCREEN_INFO:
		return gr_dev_get_vscreen_info(argp);
	case RES_GET_DISP_VERSION:
		return gr_dev_get_dpu_version(argp);
	default:
		dpu_pr_debug("mem mgr unsupport cmd, need processed by other module");
		return 1;
	}

	return 0;
}

void dpu_res_register_gr_dev(struct list_head *resource_head)
{
	struct dpu_res_resouce_node *gr_dev_node = kzalloc(sizeof(struct dpu_res_resouce_node), GFP_KERNEL);
	if (!gr_dev_node)
		return;

	gr_dev_node->res_type = RES_GR_DEV;
	atomic_set(&gr_dev_node->res_ref_cnt, 0);

	gr_dev_node->init = gr_dev_init;
	gr_dev_node->deinit = gr_dev_deinit;
	gr_dev_node->ioctl = gr_dev_ioctl;

	list_add_tail(&gr_dev_node->list_node, resource_head);
	dpu_pr_debug("dpu_res_register_gr_dev success! gr_dev_node addr=%pK", gr_dev_node);
}
