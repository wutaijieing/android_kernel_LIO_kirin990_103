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

#ifndef _DKMD_CHRDEV_H_
#define _DKMD_CHRDEV_H_

#include <linux/cdev.h>

/* dkmd char device create interface */
struct dkmd_chrdev {
	const char *name;
	dev_t devno;
	struct class *chr_class;
	struct device *chr_dev;
	struct file_operations *fops;
	struct cdev cdev;
	void *drv_data;
};

int dkmd_create_chrdev(struct dkmd_chrdev *chrdev);
void dkmd_destroy_chrdev(struct dkmd_chrdev *chrdev);
void dkmd_create_attrs(struct device *dte_cdevice, struct device_attribute *device_attrs, uint32_t len);
void dkmd_cleanup_attrs(struct device *dte_cdevice, struct device_attribute *device_attrs, uint32_t len);

#endif