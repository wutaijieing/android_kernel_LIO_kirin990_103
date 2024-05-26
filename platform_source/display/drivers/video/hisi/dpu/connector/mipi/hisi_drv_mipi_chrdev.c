/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/types.h>
#include <linux/fs.h>
#include "hisi_connector_utils.h"
#include "hisi_disp_debug.h"
#include "hisi_chrdev.h"

static int hisi_mipi_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int hisi_mipi_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static long hisi_mipi_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static struct file_operations dte_mipi_fops = {
	.owner = THIS_MODULE,
	.open = hisi_mipi_open,
	.release = hisi_mipi_release,
	.unlocked_ioctl = hisi_mipi_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl =  hisi_mipi_ioctl,
#endif
};

void hisi_drv_mipi_create_chrdev(struct hisi_connector_device *connector_data)
{
	/* registe peripheral device, will generate /dev/graphic/dte_mipi1 */
	connector_data->mipi_chrdev.name = HISI_DISP_DEVICE_MIPI_NAME;
	connector_data->mipi_chrdev.id = connector_data->master.connector_id;
	connector_data->mipi_chrdev.chrdev_fops = &dte_mipi_fops;

	hisi_disp_create_chrdev(&connector_data->mipi_chrdev, connector_data);
}

void hisi_drv_mipi_destroy_chrdev(struct hisi_connector_device *connector_data)
{
	hisi_disp_destroy_chrdev(&connector_data->mipi_chrdev);
}


