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

#ifndef DKMD_SYSFS_H
#define DKMD_SYSFS_H

#include <linux/platform_device.h>

#define DKMD_SYSFS_ATTRS_NUM 64

struct dkmd_attr {
	int32_t sysfs_index;
	struct attribute *sysfs_attrs[DKMD_SYSFS_ATTRS_NUM];
	struct attribute_group sysfs_attr_group;
};

void dkmd_sysfs_init(struct dkmd_attr *attrs);
void dkmd_sysfs_attrs_append(struct dkmd_attr *attrs, struct attribute *attr);
int32_t dkmd_sysfs_create(struct device *dev, struct dkmd_attr *attrs);
void dkmd_sysfs_remove(struct device *dev, struct dkmd_attr *attrs);

#endif
