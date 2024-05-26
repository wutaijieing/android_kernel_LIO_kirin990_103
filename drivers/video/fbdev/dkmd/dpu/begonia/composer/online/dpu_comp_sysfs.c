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
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#include "dpu_comp_sysfs.h"
#include "dpu_comp_mgr.h"
#include "gfxdev_mgr.h"

static ssize_t check_lcd_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret;
	struct dpu_composer *dpu_comp = NULL;
	int32_t lcd_check_status = 0;

	if (!dev || !buf) {
		dpu_pr_err("input is NULL pointer\n");
		return -1;
	}

	dpu_comp = to_dpu_composer(get_comp_from_device(dev));
	if (!dpu_comp) {
		dpu_pr_err("dpu_comp is NULL pointer");
		return -1;
	}

	lcd_check_status = pipeline_next_ops_handle(dpu_comp->conn_info->conn_device, dpu_comp->conn_info,
													CHECK_LCD_STATUS, NULL);
	if (lcd_check_status == 0)
		ret = snprintf(buf, PAGE_SIZE, "OK\n");
	else
		ret = snprintf(buf, PAGE_SIZE, "ERROR\n");

	return ret;
}

static DEVICE_ATTR(check_lcd_status, 0444, check_lcd_status_show, NULL);

void dpu_comp_add_attrs(struct dkmd_attr *attrs)
{
	dkmd_sysfs_attrs_append(attrs, &dev_attr_check_lcd_status.attr);
}
