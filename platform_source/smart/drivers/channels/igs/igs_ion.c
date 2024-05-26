/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: process ion in igs dev
 * Create: 2020-10-15
 */

#include "igs_ion.h"
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/platform_drivers/mm_ion.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#define igs_ion_log_info(msg...) pr_info("[I/IGS]" msg)
#define igs_ion_log_err(msg...) pr_err("[E/IGS]" msg)
#define igs_ion_log_warn(msg...) pr_warn("[W/IGS]" msg)

int igs_get_ion_phys(int share_fd, dma_addr_t *addr, struct device *dev)
{
	int ret = -ENODEV;
	struct dma_buf *buf = NULL;
	struct dma_buf_attachment *attach = NULL;
	struct sg_table *sgt = NULL;
	struct scatterlist *sgl = NULL;

	igs_ion_log_info("[%s]...\n", __func__);

	if (share_fd < 0) {
		igs_ion_log_err("[%s] Failed : ion_share_dma_buf_fd, share_fd.%d\n", __func__, share_fd);
		return -EFAULT;
	}

	if (addr == NULL) {
		igs_ion_log_err("[%s] Failed : addr is NULL\n", __func__);
		return -EFAULT;
	}

	if (IS_ERR_OR_NULL(dev)) {
		igs_ion_log_err("[%s] Failed : device is invalid\n", __func__);
		return -EFAULT;
	}

	buf = dma_buf_get(share_fd);
	if (IS_ERR_OR_NULL(buf)) {
		igs_ion_log_err("[%s] Failed : dma_buf_get, buf.%pK\n", __func__, buf);
		return -EFAULT;
	}

	attach = dma_buf_attach(buf, dev);
	if (IS_ERR_OR_NULL(attach)) {
		igs_ion_log_err("[%s] Failed : dma_buf_attach, attach.%pK\n", __func__, attach);
		goto ERR_MDMA_BUF_ATTACH;
	}

	sgt = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
	if (IS_ERR_OR_NULL(sgt)) {
		igs_ion_log_err("[%s] Failed : dma_buf_map_attachment, sgt.%pK\n", __func__, sgt);
		goto ERR_DMA_BUF_MAP_ATTACHMENT;
	}

	sgl = sgt->sgl;
	if (IS_ERR_OR_NULL(sgl)) {
		igs_ion_log_err("[%s] Failed : sgl.NULL\n", __func__);
		goto ERR_SGL;
	}

	*addr = sg_phys(sgl);

	igs_ion_log_info("[%s] end\n", __func__);
	ret = 0;

ERR_SGL:
	dma_buf_unmap_attachment(attach, sgt, DMA_BIDIRECTIONAL);
ERR_DMA_BUF_MAP_ATTACHMENT:
	dma_buf_detach(buf, attach);
ERR_MDMA_BUF_ATTACH:
	dma_buf_put(buf);

	return ret;
}
