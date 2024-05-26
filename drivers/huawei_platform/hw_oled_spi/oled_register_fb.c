// SPDX-License-Identifier: GPL-2.0
/*
 * oled_register_fb.c
 *
 * driver for oled register fb
 *
 * Copyright (c) 2022-2022 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/irqflags.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/bug.h>
#include <linux/kobject.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/export.h>
#include <huawei_platform/log/hw_log.h>
#include "oled_spi_display.h"

#define HWLOG_TAG oled_reg_fb
HWLOG_REGIST();

#define CMD_FB4_REGISTER 0
#define CMD_ADJUST_BRIGHTNESS 1

static struct miscdevice *g_fb4_miscdev;

int fb4_register_open(struct inode *inode, struct file *file)
{
	return 0;
}

int fb4_register_close(struct inode *inode, struct file *file)
{
	return 0;
}

long fb4_unlock_ioctrl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = -1;

	switch (cmd) {
	case CMD_FB4_REGISTER:
		ret = oled_spi_register_fb();
		break;
	case CMD_ADJUST_BRIGHTNESS:
		if (arg > ARG_NUM_MAX) {
			hwlog_err("%s: set brightness failed!\n", __func__);
			return -1;
		}
		ret = oled_spi_set_brightness(arg);
		break;
	default:
		break;
	}
	return ret;
}

long fb4_compat_ioctrl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = -1;

	switch (cmd) {
	case CMD_FB4_REGISTER:
		ret = oled_spi_register_fb();
		break;
	case CMD_ADJUST_BRIGHTNESS:
		if (arg > ARG_NUM_MAX) {
			hwlog_err("%s: set brightness failed!\n", __func__);
			return -1;
		}
		ret = oled_spi_set_brightness(arg);
		break;
	default:
		return ret;
	}
	return ret;
}

const struct file_operations fb4_register_fops = {
	.open = fb4_register_open,
	.release = fb4_register_close,
	.unlocked_ioctl = fb4_unlock_ioctrl,
	.compat_ioctl = fb4_compat_ioctrl,
	};

static int fb4_register_probe(struct platform_device *pdev)
{
	int ret = -1;

	g_fb4_miscdev = devm_kzalloc(&pdev->dev, sizeof(struct miscdevice), GFP_KERNEL);
	if (!g_fb4_miscdev)
		return -ENOMEM;

	g_fb4_miscdev->minor = MISC_DYNAMIC_MINOR;
	g_fb4_miscdev->name = "fb4_register";
	g_fb4_miscdev->fops = &fb4_register_fops;
	ret = misc_register(g_fb4_miscdev);
	if (ret < 0)
		goto err_unregister;

	return 0;

err_unregister:
	misc_deregister(g_fb4_miscdev);
	devm_kfree(&pdev->dev, g_fb4_miscdev);
	hwlog_err("%s: fb4_register failed\n", __func__);
	return ret;
}

static const struct of_device_id fb4_register_match_table[] = {
	{ .compatible = "huawei,oled_fb4", },
	{},
};

static struct platform_driver fb4_register_driver = {
	.probe		= fb4_register_probe,
	.remove		= NULL,
	.driver		= {
		.name	= "huawei,oled_fb4",
		.owner	= THIS_MODULE,
		.of_match_table = fb4_register_match_table,
	}
};

static int __init fb4_register_init(void)
{
	return platform_driver_register(&fb4_register_driver);
}

static void __exit fb4_register_exit(void)
{
	platform_driver_unregister(&fb4_register_driver);
}


late_initcall(fb4_register_init);
module_exit(fb4_register_exit);
MODULE_DESCRIPTION("oled register fb4 driver");
MODULE_LICENSE("GPL v2");
