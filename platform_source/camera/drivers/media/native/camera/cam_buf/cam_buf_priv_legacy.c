/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2020. All rights reserved.
 * Description: Implement of camera buffer priv legacy.
 * Create: 2018-11-28
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <cam_buf.h>
#include <cam_log.h>

#include <linux/mm_iommu.h>
#include <linux/device.h>
#include <linux/rbtree.h>
#include <linux/ion.h>
#include <linux/dma-buf.h>
#include <linux/kernel.h>
#include <linux/platform_drivers/mm_ion.h>
#include <linux/list.h>

struct sgtable_node {
	struct kref ref;
	struct list_head list;
	struct priv_ion_t *ion;
	struct sg_table *sgt;
	struct dma_buf *buf;
	struct dma_buf_attachment *attachment;
};

struct priv_ion_t {
	struct device *dev;
	struct ion_client *ion_client;
	struct mutex sg_mutex;
	struct list_head sg_nodes;
};

int cam_internal_sync(int fd, struct sync_format *fmt)
{
	(void)fd;
	(void)fmt;
	cam_err("%s: not supported, please use /dev/ion ioctl", __FUNCTION__);
	return -EFAULT;
}

int cam_internal_local_sync(int fd, struct local_sync_format *fmt)
{
	(void)fd;
	(void)fmt;
	cam_err("%s: not supported, please use /dev/ion ioctl", __FUNCTION__);
	return -EFAULT;
}

void cam_internal_dump_debug_info(struct device *dev)
{
	struct sgtable_node *node_legacy = NULL;
	struct priv_ion_t *ion = dev_get_drvdata(dev);
	cam_info("%s: dumping.....%pK", __FUNCTION__, ion);
	if (!ion) {
		cam_err("%s: get device ion fail", __FUNCTION__);
		return;
	}

	list_for_each_entry(node_legacy, &ion->sg_nodes, list) {
		cam_info("%s: pending sg_table:%pK for dmabuf:%pK",
			__FUNCTION__, node_legacy->sgt, node_legacy->buf);
	}
	cam_info("%s: end", __FUNCTION__);
}

void *cam_internal_map_kernel(struct device *dev, int fd)
{
	void *kaddr = NULL;
	struct ion_handle *handle = NULL;
	struct priv_ion_t *ion = dev_get_drvdata(dev);

	if (!ion) {
		cam_err("%s: get device ion fail", __FUNCTION__);
		return NULL;
	}

	handle = ion_import_dma_buf_fd(ion->ion_client, fd);
	if (unlikely(IS_ERR(handle))) {
		cam_err("%s: fail to import ion buffer", __FUNCTION__);
		return NULL;
	}

	kaddr = ion_map_kernel(ion->ion_client, handle);
	if (IS_ERR_OR_NULL(kaddr))
		cam_err("%s: fail to map iommu", __FUNCTION__);

	ion_free(ion->ion_client, handle);
	return kaddr;
}

void cam_internal_unmap_kernel(struct device *dev, int fd)
{
	struct ion_handle *handle = NULL;
	struct priv_ion_t *ion = NULL;

	ion = dev_get_drvdata(dev);
	if (!ion) {
		cam_err("%s: get device ion fail", __FUNCTION__);
		return;
	}

	handle = ion_import_dma_buf_fd(ion->ion_client, fd);
	if (unlikely(IS_ERR(handle))) {
		cam_err("%s: fail to import ion buffer", __FUNCTION__);
		return;
	}

	ion_unmap_kernel(ion->ion_client, handle);
	ion_free(ion->ion_client, handle);
}

int cam_internal_map_iommu(struct device *dev,
	int fd, struct iommu_format *fmt)
{
	int rc = 0;
	struct ion_handle *handle = NULL;
	struct priv_ion_t *ion = dev_get_drvdata(dev);
	if (!ion) {
		cam_err("%s: get ion fail", __FUNCTION__);
		return -ENOENT;
	}
	struct iommu_map_format map_format = {
		.prot = fmt->prot,
	};

	handle = ion_import_dma_buf_fd(ion->ion_client, fd);
	if (unlikely(IS_ERR(handle))) {
		cam_err("%s: fail to import ion buffer", __FUNCTION__);
		return -ENOENT;
	}

	if (ion_map_iommu(ion->ion_client, handle, &map_format)) {
		cam_err("%s: fail to map iommu", __FUNCTION__);
		rc = -ENOMEM;
		goto err_map_iommu;
	}
	cam_debug("%s: fd:%d, iova:%#lx, size:%#lx", __FUNCTION__, fd,
		map_format.iova_start, map_format.iova_size);
	fmt->iova = map_format.iova_start;
	fmt->size = map_format.iova_size;

err_map_iommu:
	ion_free(ion->ion_client, handle);
	return rc;
}

void cam_internal_unmap_iommu(struct device *dev,
	int fd, struct iommu_format *fmt)
{
	struct ion_handle *handle = NULL;
	struct priv_ion_t *ion = NULL;

	ion = dev_get_drvdata(dev);
	if (!ion) {
		cam_err("%s: get ion fail", __FUNCTION__);
		return;
	}

	handle = ion_import_dma_buf_fd(ion->ion_client, fd);
	if (unlikely(IS_ERR(handle))) {
		cam_err("%s: fail to import ion buffer", __FUNCTION__);
		return;
	}

	cam_debug("%s: fd:%d", __FUNCTION__, fd);
	ion_unmap_iommu(ion->ion_client, handle);
	ion_free(ion->ion_client, handle);
}

static struct sgtable_node *find_sgtable_node_by_fd(struct priv_ion_t *ion,
	int fd)
{
	struct dma_buf *dmabuf = NULL;
	struct sgtable_node *node_legacy = NULL;
	struct sgtable_node *ret_node_legacy = ERR_PTR(-ENOENT);

	dmabuf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(dmabuf)) {
		cam_err("%s: fail to get dma buf", __FUNCTION__);
		return ret_node_legacy;
	}

	list_for_each_entry(node_legacy, &ion->sg_nodes, list) {
		if (node_legacy->buf == dmabuf) {
			ret_node_legacy = node_legacy;
			break;
		}
	}

	dma_buf_put(dmabuf);
	return ret_node_legacy;
}

static struct sgtable_node *find_sgtable_node_by_sg(struct priv_ion_t *ion,
	struct sg_table *sgt)
{
	struct sgtable_node *node_legacy = NULL;

	list_for_each_entry(node_legacy, &ion->sg_nodes, list) {
		if (node_legacy->sgt == sgt)
			return node_legacy;
	}

	return ERR_PTR(-ENOENT);
}

static struct sgtable_node *create_sgtable_node(struct priv_ion_t *ion, int fd)
{
	struct sgtable_node *node_legacy = NULL;

	node_legacy = kzalloc(sizeof(*node_legacy), GFP_KERNEL);
	if (!node_legacy) {
		cam_err("%s: fail to alloc sgtable_node", __FUNCTION__);
		return ERR_PTR(-ENOMEM);
	}

	node_legacy->buf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(node_legacy->buf)) {
		cam_err("%s: fail to get dma buf", __FUNCTION__);
		goto err_get_buf;
	}

	node_legacy->attachment = dma_buf_attach(node_legacy->buf, ion->dev);
	if (IS_ERR_OR_NULL(node_legacy->attachment)) {
		cam_err("%s: fail to attach dma buf", __FUNCTION__);
		goto err_attach_buf;
	}

	node_legacy->sgt = dma_buf_map_attachment(node_legacy->attachment, DMA_BIDIRECTIONAL);
	if (IS_ERR_OR_NULL(node_legacy->sgt)) {
		cam_err("%s: fail to map attachment", __FUNCTION__);
		goto err_map_buf;
	}

	kref_init(&node_legacy->ref);
	return node_legacy;

err_map_buf:
	dma_buf_detach(node_legacy->buf, node_legacy->attachment);
err_attach_buf:
	dma_buf_put(node_legacy->buf);
err_get_buf:
	kfree(node_legacy);
	return ERR_PTR(-ENOENT);
}

struct sg_table *cam_internal_get_sgtable(struct device *dev, int fd)
{
	struct sgtable_node *node_legacy;
	struct priv_ion_t *ion = dev_get_drvdata(dev);
	if (!ion) {
		cam_err("%s: ion is null", __FUNCTION__);
		return ERR_PTR(-ENODEV);
	}

	mutex_lock(&ion->sg_mutex);
	node_legacy = find_sgtable_node_by_fd(ion, fd);
	if (!IS_ERR(node_legacy)) {
		kref_get(&node_legacy->ref);
		mutex_unlock(&ion->sg_mutex);
		return node_legacy->sgt;
	}

	node_legacy = create_sgtable_node(ion, fd);
	if (!IS_ERR(node_legacy)) {
		list_add(&node_legacy->list, &ion->sg_nodes);
		mutex_unlock(&ion->sg_mutex);
		return node_legacy->sgt;
	}
	mutex_unlock(&ion->sg_mutex);
	return ERR_PTR(-ENODEV);
}

static void cam_sgtable_deletor(struct kref *ref)
{
	struct sgtable_node *node_legacy = container_of(ref, struct sgtable_node, ref);

	/* release sgtable things we saved. */
	dma_buf_unmap_attachment(node_legacy->attachment, node_legacy->sgt,
		DMA_BIDIRECTIONAL);
	dma_buf_detach(node_legacy->buf, node_legacy->attachment);
	dma_buf_put(node_legacy->buf);

	list_del(&node_legacy->list);
	kfree(node_legacy);
}

void cam_internal_put_sgtable(struct device *dev, struct sg_table *sgt)
{
	struct sgtable_node *node_legacy;
	struct priv_ion_t *ion = dev_get_drvdata(dev);
	if (!ion) {
		cam_err("%s: ion is null", __FUNCTION__);
		return;
	}

	mutex_lock(&ion->sg_mutex);
	node_legacy = find_sgtable_node_by_sg(ion, sgt);
	if (IS_ERR(node_legacy)) {
		cam_err("%s: putting non-exist sg_table:%pK", __FUNCTION__,
			sgt);
		goto err_out;
	}
	kref_put(&node_legacy->ref, cam_sgtable_deletor);
err_out:
	mutex_unlock(&ion->sg_mutex);
}

int cam_internal_get_phys(struct device *dev, int fd, struct phys_format *fmt)
{
	struct sg_table *sgt = cam_internal_get_sgtable(dev, fd);
	if (IS_ERR(sgt))
		return PTR_ERR(sgt);

	fmt->phys = sg_phys(sgt->sgl);
	cam_internal_put_sgtable(dev, sgt);
	return 0;
}

phys_addr_t cam_internal_get_pgd_base(struct device *dev)
{
	struct iommu_domain *domain = NULL;
	struct iommu_domain_data *data = NULL;

	(void)dev;
	domain = hisi_ion_enable_iommu(NULL);
	if (IS_ERR_OR_NULL(domain)) {
		cam_err("%s: fail to get iommu domain", __FUNCTION__);
		return 0;
	}

	data = domain->priv;
	if (IS_ERR_OR_NULL(data)) {
		cam_err("%s: iommu domain data is null", __FUNCTION__);
		return 0;
	}

	return data->phy_pgd_base;
}

int cam_internal_init(struct device *dev)
{
	const char *devname = NULL;
	struct ion_client *ion_client = NULL;
	struct priv_ion_t *ion = devm_kzalloc(dev, /* ion saved in drvdata. */
		sizeof(struct priv_ion_t), GFP_KERNEL);
	if (!ion) {
		cam_err("%s: failed to allocate internal data", __FUNCTION__);
		return -ENOMEM;
	}

	devname = dev_name(dev) ? dev_name(dev) : __FILE__;
	ion_client = hisi_ion_client_create(devname);
	if (IS_ERR(ion_client)) {
		cam_err("%s: failed to create ion_client", __FUNCTION__);
		devm_kfree(dev, ion);
		return PTR_ERR(ion_client);
	}

	cam_info("%s: ion:%pK, ion_client:%pK", __FUNCTION__, ion, ion_client);

	ion->dev = dev;
	ion->ion_client = ion_client;
	mutex_init(&ion->sg_mutex);
	INIT_LIST_HEAD(&ion->sg_nodes);
	dev_set_drvdata(dev, ion);
	return 0;
}

int cam_internal_deinit(struct device *dev)
{
	struct priv_ion_t *ion = dev_get_drvdata(dev);
	if (!ion) {
		cam_err("%s: deinit before init", __FUNCTION__);
		return -EINVAL;
	}

	if (ion->ion_client) {
		ion_client_destroy(ion->ion_client);
		ion->ion_client = NULL;
	}

	cam_internal_dump_debug_info(ion->dev);
	devm_kfree(dev, ion);
	return 0;
}
