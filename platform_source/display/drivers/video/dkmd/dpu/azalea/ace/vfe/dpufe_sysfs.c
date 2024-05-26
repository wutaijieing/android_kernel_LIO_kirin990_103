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

#include "../include/dpu_communi_def.h"
#include "dpufe_sysfs.h"
#include "dpufe_dbg.h"

void dpufe_sysfs_init(struct dpufe_attr *attrs)
{
	uint32_t i;

	dpufe_check_and_no_retval(!attrs, "attrs is null\n");

	attrs->sysfs_index = 0;
	for (i = 0; i < DPUFE_SYSFS_ATTRS_NUM; i++)
		attrs->sysfs_attrs[i] = NULL;

	attrs->sysfs_attr_group.attrs = attrs->sysfs_attrs;
	DPUFE_INFO("sysfs init exit");
}

void dpufe_sysfs_attrs_append(struct dpufe_attr *attrs, struct attribute *attr)
{
	dpufe_check_and_no_retval(!attr, "attr is null\n");
	dpufe_check_and_no_retval(!attrs, "attrs is null\n");

	if (attrs->sysfs_index >= DPUFE_SYSFS_ATTRS_NUM) {
		DPUFE_ERR("sysfs_atts_num %d is out of range %d!", attrs->sysfs_index, DPUFE_SYSFS_ATTRS_NUM);
		return;
	}

	attrs->sysfs_attrs[attrs->sysfs_index++] = attr;
}

int32_t dpufe_sysfs_create(struct device *dev, struct dpufe_attr *attrs)
{
	int32_t ret;

	dpufe_check_and_return(!dev, -1, "dev is null");
	dpufe_check_and_return(!attrs, -1, "attrs is null");

	ret = sysfs_create_group(&dev->kobj, &(attrs->sysfs_attr_group));
	if (ret)
		DPUFE_ERR("sysfs group creation failed, error=%d!", ret);

	return ret;
}

void dpufe_sysfs_remove(struct device *dev, struct dpufe_attr *attrs)
{
	dpufe_check_and_no_retval(!dev, "dev is null");
	dpufe_check_and_no_retval(!attrs, "attrs is null");

	sysfs_remove_group(&dev->kobj, &(attrs->sysfs_attr_group));

	dpufe_sysfs_init(attrs);
}
