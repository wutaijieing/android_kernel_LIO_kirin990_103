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

#ifndef MEM_MGR_H
#define MEM_MGR_H

#include <linux/list.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/dma-buf.h>

#include "dkmd_res_mgr.h"

struct dpu_mem_info {
	struct list_head mem_info_head;
	spinlock_t mem_lock;
};

struct dpu_iova_info {
	struct list_head iova_info_head;
	struct dma_buf *dmabuf;
	struct res_dma_buf map_buf;
};

struct dpu_gr_dev {
	struct platform_device *pdev;
	struct dpu_mem_info mem_info;
};

void dpu_res_register_gr_dev(struct list_head *resource_head);

#endif
