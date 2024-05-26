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

#ifndef DPUFE_SYSFS_H
#define DPUFE_SYSFS_H

#include <linux/platform_device.h>

#define DPUFE_SYSFS_ATTRS_NUM 64

struct dpufe_attr {
	int32_t sysfs_index;
	struct attribute *sysfs_attrs[DPUFE_SYSFS_ATTRS_NUM];
	struct attribute_group sysfs_attr_group;
};

void dpufe_sysfs_init(struct dpufe_attr *attrs);
void dpufe_sysfs_attrs_append(struct dpufe_attr *attrs, struct attribute *attr);
int32_t dpufe_sysfs_create(struct device *dev, struct dpufe_attr *attrs);
void dpufe_sysfs_remove(struct device *dev, struct dpufe_attr *attrs);

#endif
