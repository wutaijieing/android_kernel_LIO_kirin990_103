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

#ifndef DPUFE_IOMMU_H
#define DPUFE_IOMMU_H

struct dma_buf;

typedef struct iova_info {
	uint64_t iova;
	uint64_t size;
	int share_fd;
	int calling_pid;
} iova_info_t;

void dpufe_iommu_enable(void);
struct dma_buf *dpufe_get_dmabuf(int sharefd);
struct dma_buf* dpufe_get_buffer_by_sharefd(uint64_t *iova, int fd);
void dpufe_put_dmabuf(struct dma_buf *buf);
int dpufe_buffer_unmap_iova(const void __user *arg);
int dpufe_buffer_map_iova(void __user *arg);

#endif
