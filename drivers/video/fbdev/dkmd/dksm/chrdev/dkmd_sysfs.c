/**
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
 */

#include "dkmd_log.h"
#include "dkmd_sysfs.h"

void dkmd_sysfs_init(struct dkmd_attr *attrs)
{
	uint32_t i;

	attrs->sysfs_index = 0;
	for (i = 0; i < DKMD_SYSFS_ATTRS_NUM; i++)
		attrs->sysfs_attrs[i] = NULL;

	attrs->sysfs_attr_group.attrs = attrs->sysfs_attrs;
}
EXPORT_SYMBOL(dkmd_sysfs_init);

void dkmd_sysfs_attrs_append(struct dkmd_attr *attrs, struct attribute *attr)
{
	if (attrs->sysfs_index >= DKMD_SYSFS_ATTRS_NUM) {
		dpu_pr_err("sysfs_atts_num %d is out of range %d!", attrs->sysfs_index, DKMD_SYSFS_ATTRS_NUM);
		return;
	}

	attrs->sysfs_attrs[attrs->sysfs_index++] = attr;
}
EXPORT_SYMBOL(dkmd_sysfs_attrs_append);

int32_t dkmd_sysfs_create(struct device *dev, struct dkmd_attr *attrs)
{
	int32_t ret;

	ret = sysfs_create_group(&dev->kobj, &(attrs->sysfs_attr_group));
	if (ret)
		dpu_pr_err("sysfs group creation failed, error=%d!", ret);

	return ret;
}
EXPORT_SYMBOL(dkmd_sysfs_create);

void dkmd_sysfs_remove(struct device *dev, struct dkmd_attr *attrs)
{
	sysfs_remove_group(&dev->kobj, &(attrs->sysfs_attr_group));

	dkmd_sysfs_init(attrs);
}
EXPORT_SYMBOL(dkmd_sysfs_remove);
