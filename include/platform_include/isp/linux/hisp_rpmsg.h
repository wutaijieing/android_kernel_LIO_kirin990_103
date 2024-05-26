/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2020. All rights reserved.
 * Description: platform_include/isp/linux/hisp_rpmsg.h.
 *
 * Copyright (c) 2013-2014 ISP Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _PLAT_HISP_RPMSG_H
#define _PLAT_HISP_RPMSG_H

#define CONFIG_HISP_VQUEUE_DMAALLOC_DEBUG

#ifdef CONFIG_HISP_VQUEUE_DMAALLOC_DEBUG
void *get_vqueue_dma_addr(struct virtio_device *vdev, u64 *dma_handle, size_t size);
bool rpmsg_mem_alloc_beforehand(struct virtio_device *vdev);
#endif

#endif /* _PLAT_HISP_RPMSG_H */
