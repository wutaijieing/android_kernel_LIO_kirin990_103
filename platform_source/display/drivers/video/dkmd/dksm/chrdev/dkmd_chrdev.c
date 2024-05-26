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
#include <linux/module.h>
#include "dkmd_log.h"
#include "dkmd_chrdev.h"

int dkmd_create_chrdev(struct dkmd_chrdev *chrdev)
{
	bool have_create_class = false;

	if (!chrdev)
		return -1;

	if (alloc_chrdev_region(&chrdev->devno, 0, 1, chrdev->name)) {
		dpu_pr_err("alloc_chrdev_region fail!, name=%s", chrdev->name);
		return -1;
	}

	cdev_init(&chrdev->cdev, chrdev->fops);
	chrdev->cdev.owner = THIS_MODULE;
	if (cdev_add(&chrdev->cdev, chrdev->devno, 1)) {
		dpu_pr_err("cdev_add fail!, name=%s", chrdev->name);
		goto err_cdev_add;
	}

	if (chrdev->chr_class == NULL) {
		chrdev->chr_class = class_create(THIS_MODULE, chrdev->name);
		if (IS_ERR_OR_NULL(chrdev->chr_class)) {
			dpu_pr_err("class_create fail!, name=%s", chrdev->name);
			goto err_create_class;
		}
		have_create_class = true;
	}

	chrdev->chr_dev = device_create(chrdev->chr_class, NULL, chrdev->devno, NULL, chrdev->name);
	if (IS_ERR_OR_NULL(chrdev->chr_dev)) {
		dpu_pr_err("device_create fail!, name=%s", chrdev->name);
		goto err_create_device;
	}

	dev_set_drvdata(chrdev->chr_dev, chrdev->drv_data);

	dpu_pr_info("dkmd_create_chrdev succ! major=%d, minor=%d, name=%s",
		MAJOR(chrdev->devno), MINOR(chrdev->devno), chrdev->name);

	return 0;

err_create_device:
	if (have_create_class)
		class_destroy(chrdev->chr_class);

err_create_class:
	cdev_del(&chrdev->cdev);

err_cdev_add:
	unregister_chrdev_region(chrdev->devno, 1);

	return -1;
}
EXPORT_SYMBOL(dkmd_create_chrdev);

void dkmd_destroy_chrdev(struct dkmd_chrdev *chrdev)
{
	if (!chrdev)
		return;

	device_destroy(chrdev->chr_class, chrdev->devno);
	class_destroy(chrdev->chr_class);
	cdev_del(&chrdev->cdev);
	unregister_chrdev_region(chrdev->devno, 1);
}
EXPORT_SYMBOL(dkmd_destroy_chrdev);

void dkmd_create_attrs(struct device *dev, struct device_attribute *device_attrs, uint32_t len)
{
	int32_t i;
	int32_t error = 0;

	for (i = 0; i < (int32_t)len; i++) {
		error = device_create_file(dev, &device_attrs[i]);
		if (error)
			break;
	}

	if (error) {
		while (--i >= 0)
			device_remove_file(dev, &device_attrs[i]);
	}
}
EXPORT_SYMBOL(dkmd_create_attrs);

void dkmd_cleanup_attrs(struct device *dev, struct device_attribute *device_attrs, uint32_t len)
{
	uint32_t i;

	for (i = 0; i < len; i++)
		device_remove_file(dev, &device_attrs[i]);
}
EXPORT_SYMBOL(dkmd_cleanup_attrs);

MODULE_LICENSE("GPL");
