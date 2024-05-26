/*
 * gpu_top.c
 *
 * PM QOS for gpu top
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/errno.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/sysfs.h>
#include <linux/pm_qos.h>
#include <linux/uaccess.h>
#include "gpufreq_fs.h"

struct miscdevice g_min_miscdev;
struct miscdevice g_max_miscdev;
struct device *g_gpufreq_dec = NULL;

static ssize_t gpufreq_qos_write(struct file *filp, const char __user *buf,
				 size_t count, loff_t *f_pos)
{
	int value;
	struct dev_pm_qos_request *req = NULL;
	int ret;

	if (count == sizeof(s32)) {
		if (copy_from_user(&value, buf, sizeof(s32)))
			return -EFAULT;
	} else {
		ret = kstrtos32_from_user(buf, count, 16, &value);
		if (ret)
			return ret;
	}

	req = filp->private_data;
	dev_pm_qos_update_request(req, value);

	return count;
}

static ssize_t gpufreq_qos_read(struct file *filp, char __user *buf,
				size_t count, loff_t *f_pos)
{
	int value;
	struct dev_pm_qos_request *req = filp->private_data;

	if (!req)
		return -EINVAL;

	value = dev_pm_qos_read_value(g_gpufreq_dec, req->type);

	return simple_read_from_buffer(buf, count, f_pos, &value, sizeof(int));
}

static int gpufreq_qos_open(struct inode *inode, struct file *filp)
{
	struct dev_pm_qos_request *req = NULL;
	int minor;

	minor = iminor(inode);
	req = kzalloc(sizeof(struct dev_pm_qos_request), GFP_KERNEL);
	if (req == NULL)
		return -ENOMEM;

	if (g_min_miscdev.minor == minor)
		dev_pm_qos_add_request(g_gpufreq_dec, req, DEV_PM_QOS_MIN_FREQUENCY, PM_QOS_DEFAULT_VALUE);
	else
		dev_pm_qos_add_request(g_gpufreq_dec, req, DEV_PM_QOS_MAX_FREQUENCY, PM_QOS_DEFAULT_VALUE);

	filp->private_data = req;

	return 0;
}

static int gpufreq_qos_release(struct inode *inode, struct file *filp)
{
	struct dev_pm_qos_request *req = NULL;

	req = filp->private_data;

	dev_pm_qos_remove_request(req);
	kfree(req);
	return 0;
}

static const struct file_operations gpufreq_qos_fops = {
	.write = gpufreq_qos_write,
	.read = gpufreq_qos_read,
	.open = gpufreq_qos_open,
	.release = gpufreq_qos_release,
	.llseek = noop_llseek,
};

int gpufreq_fs_register(struct device *gpu_dev)
{
	int ret;

	if (gpu_dev == NULL)
		return -EINVAL;

	g_min_miscdev.minor = MISC_DYNAMIC_MINOR;
	g_min_miscdev.name = "gpu_min_freq";
	g_min_miscdev.fops = &gpufreq_qos_fops;

	ret = misc_register(&g_min_miscdev);
	if (ret != 0)
		return ret;

	g_max_miscdev.minor = MISC_DYNAMIC_MINOR;
	g_max_miscdev.name = "gpu_max_freq";
	g_max_miscdev.fops = &gpufreq_qos_fops;

	ret = misc_register(&g_max_miscdev);
	if (ret != 0)
		return ret;

	g_gpufreq_dec = gpu_dev;
	return 0;
}

void gpufreq_fs_deregister(void)
{
	misc_deregister(&g_min_miscdev);
	misc_deregister(&g_max_miscdev);

	g_gpufreq_dec = NULL;
}
