/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2020. All rights reserved.
 * Description: Declaration of camera buffer priv.
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

#ifndef __CAM_BUF_PRIV_H__
#define __CAM_BUF_PRIV_H__

#include "cam_buf.h"

struct device;
int cam_internal_map_iommu(struct device *dev,
						   int fd, struct iommu_format *fmt);
void cam_internal_unmap_iommu(struct device *dev,
							  int fd, struct iommu_format *fmt);
int cam_internal_sc_available_size(struct systemcache_format *fmt);
int cam_internal_sc_wakeup(struct systemcache_format *fmt);

struct sg_table *cam_internal_get_sgtable(struct device *dev, int fd);
void cam_internal_put_sgtable(struct device *dev, struct sg_table *sgt);

int cam_internal_get_phys(struct device *dev,
						  int fd, struct phys_format *fmt);
phys_addr_t cam_internal_get_pgd_base(struct device *dev);

int cam_internal_init(struct device *dev);
int cam_internal_deinit(struct device *dev);

int cam_internal_sync(int fd, struct sync_format *fmt);
int cam_internal_local_sync(int fd, struct local_sync_format *fmt);

void cam_internal_unmap_kernel(struct device *dev, int fd);
void *cam_internal_map_kernel(struct device *dev, int fd);

void cam_internal_dump_debug_info(struct device *dev);

#endif /* __CAM_BUF_PRIV_H__ */
