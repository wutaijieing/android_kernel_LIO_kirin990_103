/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#ifndef HISI_DISP_CHRDEV_H
#define HISI_DISP_CHRDEV_H

#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/types.h>
#include <linux/cdev.h>

#define HISI_DISP_CHR_DEV_CLASS_NAME    "dte_graphics"
#define HISI_DISP_DEVICE_OFFLINE1_NAME  "dte_offline"
#define HISI_DISP_DEVICE_MIPI_NAME      "dte_mipi"
#define HISI_DISP_DEVICE_DP_NAME      "dte_dp"
#define HISI_DISP_DEVICE_FB2_NAME      "dte_fb2"

struct hisi_disp_chrdev {
	const char *name;
	int id;
	dev_t devno;
	struct class *dev_class;
	struct device *dte_cdevice;
	struct cdev cdev;
	struct file_operations *chrdev_fops;
};

void hisi_disp_create_chrdev(struct hisi_disp_chrdev *chrdev, void *dev_data);
void hisi_disp_destroy_chrdev(struct hisi_disp_chrdev *chrdev);
void hisi_disp_create_attrs(struct device *dte_cdevice, struct device_attribute *device_attrs, uint32_t len);
void hisi_disp_cleanup_attrs(struct device *dte_cdevice, struct device_attribute *device_attrs, uint32_t len);


#endif /* HISI_DISP_CHRDEV_H */
