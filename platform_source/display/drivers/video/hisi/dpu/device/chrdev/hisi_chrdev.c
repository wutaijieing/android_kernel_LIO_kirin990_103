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
#include <linux/device.h>
#include "hisi_chrdev.h"
#include "hisi_disp_debug.h"

static struct class *g_disp_cdev_class;
static atomic_t g_disp_cdev_ref = ATOMIC_INIT(0);
static dev_t g_disp_devno;

static struct class *hisi_disp_get_chrdev_class(void)
{
	if (!g_disp_cdev_class) {
		g_disp_cdev_class = class_create(THIS_MODULE, HISI_DISP_CHR_DEV_CLASS_NAME);
		if (!g_disp_cdev_class) {
			return NULL;
		}
	}

	return g_disp_cdev_class;
}

static dev_t hisi_disp_get_devno(void)
{
	int ret;

	if (!g_disp_devno) {
		ret = alloc_chrdev_region(&g_disp_devno, 0, 1, "dte_chrdev");
		if (ret) {
			disp_pr_err("get chrdev devno error");
			return NULL;
		}
	}

	return MKDEV(MAJOR(g_disp_devno), atomic_read(&g_disp_cdev_ref));
}

void hisi_disp_create_chrdev(struct hisi_disp_chrdev *chrdev, void *dev_data)
{
	struct class *dte_cdev_class = NULL;
	int ret;

	dte_cdev_class = hisi_disp_get_chrdev_class();
	if (!dte_cdev_class)
		return;

	chrdev->devno = hisi_disp_get_devno();
	disp_pr_info("start major=%d, minor=%d", MAJOR(chrdev->devno), MINOR(chrdev->devno));

	chrdev->dte_cdevice = device_create(dte_cdev_class, NULL, chrdev->devno, dev_data, "%s", chrdev->name);
	if (!chrdev->dte_cdevice) {
		disp_pr_err("create device fail, major=%d, minor=%d", MAJOR(chrdev->devno), MINOR(chrdev->devno));
		return;
	}

	cdev_init(&chrdev->cdev, chrdev->chrdev_fops);
	chrdev->cdev.owner = THIS_MODULE;
	ret = cdev_add(&chrdev->cdev, chrdev->devno, 1);
	if (ret) {
		disp_pr_err("cdev_add fail, major=%d, minor=%d", MAJOR(chrdev->devno), MINOR(chrdev->devno));
		goto cdev_add_fail;
	}

	chrdev->dev_class = dte_cdev_class;
	atomic_inc(&g_disp_cdev_ref);

	disp_pr_info("end major=%d, minor=%d, name=%s, id=%d",
			MAJOR(chrdev->devno), MINOR(chrdev->devno), chrdev->name, chrdev->id);
	return;

cdev_add_fail:
	device_destroy(chrdev->dev_class, chrdev->devno);
}

void hisi_disp_destroy_chrdev(struct hisi_disp_chrdev *chrdev)
{
	if (!chrdev->dev_class)
		return;

	cdev_del(&chrdev->cdev);
	device_destroy(chrdev->dev_class, chrdev->devno);
	unregister_chrdev_region(chrdev->devno, 1);

	if (atomic_dec_return(&g_disp_cdev_ref) == 0) {
		class_destroy(g_disp_cdev_class);
		chrdev->dev_class = NULL;
		g_disp_cdev_class = NULL;
	}

	chrdev->dte_cdevice = NULL;
	chrdev->chrdev_fops = NULL;
}

void hisi_disp_create_attrs(struct device *dte_cdevice, struct device_attribute *device_attrs, uint32_t len)
{
	uint32_t i;
	int32_t error = 0;

	for (i = 0; i < len; i++) {
		error = device_create_file(dte_cdevice, &device_attrs[i]);
		if (error)
			break;
	}

	if (error) {
		while (--i >= 0)
			device_remove_file(dte_cdevice, &device_attrs[i]);
	}
}

void hisi_disp_cleanup_attrs(struct device *dte_cdevice, struct device_attribute *device_attrs, uint32_t len)
{
	uint32_t i;

	for (i = 0; i < len; i++)
		device_remove_file(dte_cdevice, &device_attrs[i]);
}


