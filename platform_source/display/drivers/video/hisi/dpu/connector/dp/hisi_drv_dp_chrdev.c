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

static int hisi_dp_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int hisi_dp_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static long hisi_dp_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static struct file_operations dte_dp_fops = {
	.owner = THIS_MODULE,
	.open = hisi_dp_open,
	.release = hisi_dp_release,
	.unlocked_ioctl = hisi_dp_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl =  hisi_dp_ioctl,
#endif
};

void hisi_drv_dp_create_chrdev(struct hisi_connector_device *connector_data)
{
	/* registe peripheral device, will generate /dev/graphic/dte_dp1 */
	if (connector_data->master.connector_id == 2) {
		connector_data->mipi_chrdev.name = HISI_DISP_DEVICE_FB2_NAME;
	} else {
		connector_data->mipi_chrdev.name = HISI_DISP_DEVICE_DP_NAME;
	}
	connector_data->mipi_chrdev.id = connector_data->master.connector_id;
	connector_data->mipi_chrdev.chrdev_fops = &dte_dp_fops;

	hisi_disp_create_chrdev(&connector_data->mipi_chrdev, connector_data);
}

void hisi_drv_dp_destroy_chrdev(struct hisi_connector_device *connector_data)
{
	hisi_disp_destroy_chrdev(&connector_data->mipi_chrdev);
}


