/*
 * Copyright (C) 2019-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/io.h>
#include <linux/mm_iommu.h>
#include <linux/delay.h>

#include "arpp_smmu.h"
#include "arpp_log.h"

#define MAX_TOK_TRANS                   16
#define MAX_WAIT_TBU_CONNECTION_TIMES   1000
#define ADDR_ALIGNMENT                  0x80

static int set_sid_ssid_to_tbu(struct arpp_device *arpp_dev)
{
	uint32_t smmu_sid;
	uint32_t smmu_ssid;
	uint32_t ssid_valid;
	union rw_channel_attr ch_attr;
	struct arpp_iomem *io_info = NULL;
	char __iomem *sctrl_base_addr = NULL;

	if (arpp_dev == NULL) {
		hiar_loge("io_info is NULL");
		return -EINVAL;
	}

	io_info = &arpp_dev->io_info;
	sctrl_base_addr = io_info->sctrl_base;
	if (sctrl_base_addr == NULL) {
		hiar_logi("sctrl_base_addr is NULL");
		return -EINVAL;
	}

	/* set sid, ssid */
	smmu_sid = arpp_dev->sid;
	smmu_ssid = arpp_dev->ssid;
	ssid_valid = 1;

	ch_attr.value = 0; /* clear */
	ch_attr.reg.rch_sid = smmu_sid;
	ch_attr.reg.rch_ssid = smmu_ssid;
	ch_attr.reg.rch_ssidv = ssid_valid;

	writel(ch_attr.value, sctrl_base_addr + RCH0_ID_ATTR_S);
	writel(ch_attr.value, sctrl_base_addr + RCH1_ID_ATTR_S);
	writel(ch_attr.value, sctrl_base_addr + RCH2_ID_ATTR_S);
	writel(ch_attr.value, sctrl_base_addr + RCH3_ID_ATTR_S);
	writel(ch_attr.value, sctrl_base_addr + RCH4_ID_ATTR_S);
	writel(ch_attr.value, sctrl_base_addr + RCH5_ID_ATTR_S);
	writel(ch_attr.value, sctrl_base_addr + RCH6_ID_ATTR_S);
	writel(ch_attr.value, sctrl_base_addr + RCH7_ID_ATTR_S);

	writel(ch_attr.value, sctrl_base_addr + WCH0_ID_ATTR_S);
	writel(ch_attr.value, sctrl_base_addr + WCH1_ID_ATTR_S);
	writel(ch_attr.value, sctrl_base_addr + WCH2_ID_ATTR_S);
	writel(ch_attr.value, sctrl_base_addr + WCH3_ID_ATTR_S);
	writel(ch_attr.value, sctrl_base_addr + WCH4_ID_ATTR_S);
	writel(ch_attr.value, sctrl_base_addr + WCH5_ID_ATTR_S);
	writel(ch_attr.value, sctrl_base_addr + WCH6_ID_ATTR_S);
	writel(ch_attr.value, sctrl_base_addr + WCH7_ID_ATTR_S);

	return 0;
}

int smmu_enable(struct arpp_device *arpp_dev)
{
	int ret;
	union smmu_tbu_cr smmu_cr;
	union smmu_tbu_crack smmu_crack;
	int times;
	char __iomem *tbu_base_addr = NULL;
	struct arpp_iomem *io_info = NULL;

	if (arpp_dev == NULL) {
		hiar_loge("io_info is NULL");
		return -EINVAL;
	}

	io_info = &arpp_dev->io_info;
	tbu_base_addr = io_info->tbu_base;
	if (tbu_base_addr == NULL) {
		hiar_logi("tbu_base_addr is NULL");
		return -EINVAL;
	}

	ret = set_sid_ssid_to_tbu(arpp_dev);
	if (ret != 0) {
		hiar_logi("set tbu failed, error=%d", ret);
		return ret;
	}

	/* tbu connnet to tcu */
	smmu_cr.value = readl(tbu_base_addr + SMMU_TBU_CR);
	smmu_cr.reg.max_tok_trans = MAX_TOK_TRANS - 1;
	smmu_cr.reg.tbu_en_req = 0x1;
	writel(smmu_cr.value, tbu_base_addr + SMMU_TBU_CR);

	times = 0;
	do {
		smmu_crack.value = readl(tbu_base_addr + SMMU_TBU_CRACK);
		if (smmu_crack.reg.tbu_en_ack == 0x1) {
			hiar_logi("conect tcu succ");
			break;
		}
		udelay(1);
		times++;
	} while (times <= MAX_WAIT_TBU_CONNECTION_TIMES);

	if (smmu_crack.reg.tbu_connected == 0x0) {
		hiar_logi("conect failed");
		return -1;
	}
	hiar_logi("tbu connected is %d", smmu_crack.reg.tbu_connected);

	if (smmu_crack.reg.tok_trans_gnt < smmu_cr.reg.max_tok_trans) {
		hiar_loge("tok trans gnt error=%d",
			smmu_crack.reg.tok_trans_gnt);
		return -1;
	}

	return 0;
}

void smmu_disable(struct arpp_device *arpp_dev)
{
	int times;
	union smmu_tbu_cr smmu_cr;
	union smmu_tbu_crack smmu_crack;
	struct arpp_iomem *io_info = NULL;
	char __iomem *tbu_base_addr = NULL;

	if (arpp_dev == NULL) {
		hiar_loge("io_info is NULL");
		return;
	}

	io_info = &arpp_dev->io_info;
	tbu_base_addr = io_info->tbu_base;
	if (tbu_base_addr == NULL) {
		hiar_logi("tbu_base_addr is NULL");
		return;
	}

	smmu_cr.value = readl(tbu_base_addr + SMMU_TBU_CR);
	smmu_cr.reg.tbu_en_req = 0x0;

	writel(smmu_cr.value, tbu_base_addr + SMMU_TBU_CR);

	times = 0;
	do {
		smmu_crack.value = readl(tbu_base_addr + SMMU_TBU_CRACK);
		if (smmu_crack.reg.tbu_en_ack == 0x1) {
			hiar_logi("disconect tcu succ");
			break;
		}
		udelay(1);
		times++;
	} while (times <= MAX_WAIT_TBU_CONNECTION_TIMES);

	if (smmu_crack.reg.tbu_connected == 0x0)
		hiar_logi("disconect succ");
	else
		hiar_logw("disconect failed");
}

static int is_alignment(uint64_t addr)
{
	if (addr & (ADDR_ALIGNMENT - 1))
		return 0;

	return 1;
}

static int map_iova_by_fd(struct device *dev, struct mapped_buffer *map_buf)
{
	int ret;
	struct dma_buf *buf = NULL;

	if (dev == NULL) {
		hiar_loge("dev is NULL");
		return -EINVAL;
	}

	if (map_buf == NULL) {
		hiar_loge("map_buf is NULL");
		return -EINVAL;
	}

	if (map_buf->base_buf.fd < 0) {
		hiar_loge("fd is invalid");
		return -EINVAL;
	}

	ret = 0;

	buf = dma_buf_get(map_buf->base_buf.fd);
	if (IS_ERR(buf)) {
		hiar_loge("Invalid file shared_fd=%d", map_buf->base_buf.fd);
		return -EINVAL;
	}

	map_buf->iova = kernel_iommu_map_dmabuf(dev, buf,
		0, &(map_buf->mapped_size));
	if (map_buf->iova == 0) {
		hiar_loge("fail to map iommu iova");
		ret = -ENOMEM;
		goto ERR_MAP_IOMMU;
	}
	hiar_logi("iova is [%lu], size is %lu",
		map_buf->iova, map_buf->mapped_size);

ERR_MAP_IOMMU:
	dma_buf_put(buf);

	return ret;
}

static void unmap_iova_by_fd(struct device *dev,
	struct mapped_buffer *map_buf)
{
	int ret;
	struct dma_buf *buf = NULL;

	if (dev == NULL) {
		hiar_loge("dev is NULL");
		return;
	}

	if (map_buf == NULL) {
		hiar_loge("map_buf is NULL");
		return;
	}

	if (map_buf->base_buf.fd < 0) {
		hiar_loge("fd is invalid");
		return;
	}

	buf = dma_buf_get(map_buf->base_buf.fd);
	if (IS_ERR(buf)) {
		hiar_loge("fail to get dma dmabuf, fd=%d", map_buf->base_buf.fd);
		return;
	}

	ret = kernel_iommu_unmap_dmabuf(dev, buf, map_buf->iova);
	if (ret < 0)
		hiar_loge("fail to unmap fd=%d", map_buf->base_buf.fd);

	map_buf->iova = 0;

	dma_buf_put(buf);
}

int map_cmdlist_buffer(struct device *dev, struct mapped_buffer *cmdlist_buf)
{
	int ret;

	if (dev == NULL) {
		hiar_loge("dev is NULL");
		return -EINVAL;
	}

	if (cmdlist_buf == NULL) {
		hiar_loge("cmdlist_buf is NULL");
		return -EINVAL;
	}

	ret = map_iova_by_fd(dev, cmdlist_buf);
	if (ret != 0) {
		hiar_loge("map_iova_by_fd failed, error=%d", ret);
		return ret;
	}

	return 0;
}

void unmap_cmdlist_buffer(struct device *dev,
	struct mapped_buffer *cmdlist_buf)
{
	if (dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return;
	}

	if (cmdlist_buf == NULL) {
		hiar_loge("cmdlist_buf is NULL");
		return;
	}

	unmap_iova_by_fd(dev, cmdlist_buf);
}

static int check_address_validity(struct lba_buffer_info *buf_addr)
{
	uint64_t in_buffer_addr;
	uint64_t swap_buffer_addr;
	uint64_t free_buffer_addr;
	uint64_t out_buffer_addr;

	if (buf_addr == NULL) {
		hiar_loge("buf_addr is NULL");
		return -EINVAL;
	}

	in_buffer_addr = buf_addr->inout_buf.iova;
	swap_buffer_addr = buf_addr->swap_buf.iova;
	free_buffer_addr = buf_addr->free_buf.iova;
	out_buffer_addr = buf_addr->out_buf.iova;

	if (in_buffer_addr == 0) {
		hiar_loge("addr is invalid");
		return -EINVAL;
	}

	if (swap_buffer_addr == 0) {
		hiar_loge("address is invalid");
		return -EINVAL;
	}

	if (free_buffer_addr == 0) {
		hiar_loge("address is invalid");
		return -EINVAL;
	}

	if (out_buffer_addr == 0) {
		hiar_loge("address is invalid");
		return -EINVAL;
	}

	if (is_alignment(in_buffer_addr) != 1) {
		hiar_loge("in_buffer_addr is not 128 alignment");
		return -EINVAL;
	}

	if (is_alignment(swap_buffer_addr) != 1) {
		hiar_loge("swap_buffer_addr is not 128 alignment");
		return -EINVAL;
	}

	if (is_alignment(free_buffer_addr) != 1) {
		hiar_loge("free_buffer_addr is not 128 alignment");
		return -EINVAL;
	}

	if (is_alignment(out_buffer_addr) != 1) {
		hiar_loge("out_buffer_addr is not 128 alignment");
		return -EINVAL;
	}

	return 0;
}

static int check_mapped_buffer_size(struct lba_buffer_info *buf_addr)
{
	if (buf_addr == NULL) {
		hiar_loge("buf_addr is NULL");
		return -EINVAL;
	}

	if (buf_addr->inout_buf.mapped_size <
		buf_addr->inout_buf.base_buf.size) {
		hiar_loge("inout_buf buffer size[%llu] check failed",
			buf_addr->inout_buf.base_buf.size);
		return -EINVAL;
	}
	if (buf_addr->swap_buf.mapped_size <
		buf_addr->swap_buf.base_buf.size) {
		hiar_loge("swap_buf buffer size[%llu] check failed",
			buf_addr->swap_buf.base_buf.size);
		return -EINVAL;
	}
	if (buf_addr->free_buf.mapped_size <
		buf_addr->free_buf.base_buf.size) {
		hiar_loge("free_buf buffer size[%llu] check failed",
			buf_addr->free_buf.base_buf.size);
		return -EINVAL;
	}
	if (buf_addr->out_buf.mapped_size <
		buf_addr->out_buf.base_buf.size) {
		hiar_loge("out_buf buffer size[%llu] check failed",
			buf_addr->out_buf.base_buf.size);
		return -EINVAL;
	}

	return 0;
}

int map_buffer_from_user(struct device *dev, struct lba_buffer_info *buf_addr)
{
	uint32_t ret;

	if (dev == NULL) {
		hiar_loge("dev is NULL");
		return -EINVAL;
	}

	if (buf_addr == NULL) {
		hiar_loge("buf_addr is NULL");
		return -EINVAL;
	}

	/*
	 * If one of buffers are mapped failed or checked failed,
	 * it need to unmap all of buffers, because it can not work.
	 */
	ret = 0;
	ret |= (uint32_t)map_iova_by_fd(dev, &buf_addr->inout_buf);
	ret |= (uint32_t)map_iova_by_fd(dev, &buf_addr->swap_buf);
	ret |= (uint32_t)map_iova_by_fd(dev, &buf_addr->free_buf);
	ret |= (uint32_t)map_iova_by_fd(dev, &buf_addr->out_buf);
	if (ret != 0) {
		hiar_loge("map iova failed");
		goto err_unmap_buf;
	}

	ret = (uint32_t)check_address_validity(buf_addr);
	if (ret != 0) {
		hiar_loge("check_address_validity failed");
		goto err_unmap_buf;
	}

	ret = (uint32_t)check_mapped_buffer_size(buf_addr);
	if (ret != 0) {
		hiar_loge("check_mapped_buffer_size failed");
		goto err_unmap_buf;
	}

	return 0;

err_unmap_buf:
	unmap_buffer_from_user(dev, buf_addr);
	return -EINVAL;
}

void unmap_buffer_from_user(struct device *dev,
	struct lba_buffer_info *buf_addr)
{
	if (dev == NULL) {
		hiar_loge("dev is NULL");
		return;
	}

	if (buf_addr == NULL) {
		hiar_loge("buf_addr is NULL");
		return;
	}

	unmap_iova_by_fd(dev, &buf_addr->inout_buf);
	unmap_iova_by_fd(dev, &buf_addr->swap_buf);
	unmap_iova_by_fd(dev, &buf_addr->free_buf);
	unmap_iova_by_fd(dev, &buf_addr->out_buf);
}
