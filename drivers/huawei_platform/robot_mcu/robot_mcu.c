// SPDX-License-Identifier: GPL-2.0
/*
 * robot_mcu.c
 *
 * Robot mcu control driver
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

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <huawei_platform/log/hw_log.h>

#include "robot_mcu.h"

#define HWLOG_TAG robot_mcu
HWLOG_REGIST();

static irqreturn_t robot_mcu_wakeup_callback(int irq, void *dev_id)
{
	hwlog_info("Robot mcu wakeup ap interrupt\n");

	return IRQ_HANDLED;
}

static int robot_mcu_wakeup_irq_register(struct device *dev, struct robot_mcu_dev_data *data)
{
	uint32_t gpio_wakeup;
	int ret;

	gpio_wakeup = data->gpio_wakeup;

	if (!gpio_is_valid(gpio_wakeup)) {
		hwlog_err("invalid gpio_wakeup %d.\n", gpio_wakeup);
		return -EINVAL;
	}

	if (gpio_request(gpio_wakeup, "gpio_int1_mcu_soc")) {
		hwlog_err("gpio int1 %d fail.\n", gpio_wakeup);
		return -EINVAL;
	}

	data->irq_wakeup = gpio_to_irq(gpio_wakeup);
	ret = request_irq(data->irq_wakeup, robot_mcu_wakeup_callback,
		IRQF_TRIGGER_RISING | IRQF_NO_SUSPEND, "robot_mcu_wakeup_irq", dev);
	if (ret < 0) {
		hwlog_err("%s: robot_mcu_wakeup_irq fail, ret=%d\n", __func__, ret);
		gpio_free(gpio_wakeup);
		return -EINVAL;
	}
	hwlog_info("%s: robot_mcu_wakeup_irq %d request succ\n", __func__, data->irq_wakeup);

	return 0;
}

static irqreturn_t robot_head_touch_wakeup_callback(int irq, void *dev_id)
{
	hwlog_info("Robot head touch wakeup ap interrupt\n");
	return IRQ_HANDLED;
}

static int robot_head_touch_wakeup_irq_register(struct device *dev, struct robot_mcu_dev_data *data)
{
	uint32_t gpio_wakeup_head_touch;
	int ret;

	gpio_wakeup_head_touch = data->gpio_wakeup_head_touch;

	if (!gpio_is_valid(gpio_wakeup_head_touch)) {
		hwlog_err("invalid gpio_wakeup_head_touch %d.\n", gpio_wakeup_head_touch);
		return -EINVAL;
	}

	if (gpio_request(gpio_wakeup_head_touch, "gpio_int_head_touch")) {
		hwlog_err("gpio int %d fail.\n", gpio_wakeup_head_touch);
		return -EINVAL;
	}

	if (gpio_direction_input(gpio_wakeup_head_touch)) {
		hwlog_err("set gpio input %d fail.\n", gpio_wakeup_head_touch);
		return -EINVAL;
	}

	data->wakeup_head_touch_irq = gpio_to_irq(gpio_wakeup_head_touch);
	ret = request_irq(data->wakeup_head_touch_irq, robot_head_touch_wakeup_callback,
		IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND, "robot_head_touch_wakeup_irq", dev);
	if (ret < 0) {
		hwlog_err("%s: robot_head_touch_wakeup_irq fail, ret=%d\n", __func__, ret);
		gpio_free(data->gpio_wakeup_head_touch);
		return -EINVAL;
	}
	hwlog_info("%s: robot_head_touch_wakeup_irq %d request succ\n", __func__, data->wakeup_head_touch_irq);

	return 0;
}

static int robot_mcu_reset(struct device *dev)
{
	struct robot_mcu_dev_data *data;

	data = (struct robot_mcu_dev_data *)dev_get_drvdata(dev);
	if (data == NULL)
		return -EFAULT;

	if (!gpio_is_valid(data->gpio_reset)) {
		hwlog_err("invalid gpio %d.\n", data->gpio_reset);
		return -EINVAL;
	}

	if (gpio_request(data->gpio_reset, "gpio_reset_soc_mcu")) {
		hwlog_err("gpio reset %d request fail.\n", data->gpio_reset);
		return -EINVAL;
	}

	gpio_direction_output(data->gpio_reset, ROBOT_MCU_LEVEL_LOW);
	mdelay(10);
	gpio_direction_output(data->gpio_reset, ROBOT_MCU_LEVEL_HIGH);

	gpio_free(data->gpio_reset);
	return 0;
}


/*
 * Enable MCU upgrade mode by set boot0 GPIO.
 */
static int robot_mcu_set_boot0(struct device *dev, enum robot_mcu_gpio_level level)
{
	struct robot_mcu_dev_data *data;

	data = (struct robot_mcu_dev_data *)dev_get_drvdata(dev);
	if (data == NULL)
		return -EFAULT;

	if (!gpio_is_valid(data->gpio_boot0)) {
		hwlog_err("invalid gpio %d.\n", data->gpio_boot0);
		return -EINVAL;
	}

	if (gpio_request(data->gpio_boot0, "gpio_boot0_soc_mcu")) {
		hwlog_err("gpio boot0 %d request fail.\n", data->gpio_boot0);
		return -EINVAL;
	}

	gpio_direction_output(data->gpio_boot0, level);

	gpio_free(data->gpio_boot0);
	return 0;
}

/*
 * Enable MCU upgrade mode by set sync GPIO level high.
 */
static int robot_mcu_upgrade_gpioctl(struct device *dev, enum robot_mcu_gpio_level level)
{
	struct robot_mcu_dev_data *data;

	data = (struct robot_mcu_dev_data *)dev_get_drvdata(dev);
	if (data == NULL)
		return -EFAULT;

	if (!gpio_is_valid(data->gpio_upgrade_ioctl)) {
		hwlog_err("invalid gpio %d.\n", data->gpio_upgrade_ioctl);
		return -EINVAL;
	}

	gpio_direction_output(data->gpio_upgrade_ioctl, level);
	return 0;
}

static int robot_mcu_upgrade_gpio_init(struct device *dev, struct robot_mcu_dev_data *data)
{
	uint32_t gpio_upgrade_status;
	uint32_t gpio_upgrade_ioctl;
	int ret;

	if (data == NULL)
		return -EFAULT;

	gpio_upgrade_status = data->gpio_upgrade_status;
	if (!gpio_is_valid(gpio_upgrade_status)) {
		hwlog_err("invalid gpio_upgrade_status %d.\n", gpio_upgrade_status);
		return -EINVAL;
	}

	if (gpio_request(gpio_upgrade_status, "gpio_int2_mcu_soc")) {
		hwlog_err("gpio upgrade status %d request fail.\n", gpio_upgrade_status);
		return -EINVAL;
	}

	gpio_direction_input(data->gpio_upgrade_status);

	gpio_upgrade_ioctl = data->gpio_upgrade_ioctl;
	if (!gpio_is_valid(gpio_upgrade_ioctl)) {
		hwlog_err("invalid gpio_upgrade_ioctl %d.\n", gpio_upgrade_ioctl);
		gpio_free(data->gpio_upgrade_status);
		return -EINVAL;
	}

	if (gpio_request(gpio_upgrade_ioctl, "gpio_sync_mcu_soc")) {
		hwlog_err("gpio upgrade ioctl %d request fail.\n", gpio_upgrade_ioctl);
		gpio_free(data->gpio_upgrade_status);
		return -EINVAL;
	}

	gpio_direction_output(data->gpio_upgrade_ioctl, ROBOT_MCU_LEVEL_LOW);
	return 0;
}

static ssize_t robot_mcu_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int value;
	struct robot_mcu_dev_data *data;

	data = (struct robot_mcu_dev_data *)dev_get_drvdata(dev);
	value = gpio_get_value(data->gpio_upgrade_status);

	return scnprintf(buf, PAGE_SIZE, "%d", value);
}

static ssize_t robot_mcu_store(struct device *dev, struct device_attribute *attr, const char *buf,
				size_t size)
{
	unsigned long value = 0;
	int ret;

	ret = kstrtoul(buf, 10, &value);
	if (ret)
		return -EINVAL;

	switch (value) {
	case ROBOT_MCU_RESET:
		ret = robot_mcu_reset(dev);
		break;
	case ROBOT_MCU_BOOT0_SET:
		ret = robot_mcu_set_boot0(dev, ROBOT_MCU_LEVEL_HIGH);
		break;
	case ROBOT_MCU_BOOT0_RESET:
		ret = robot_mcu_set_boot0(dev, ROBOT_MCU_LEVEL_LOW);
		break;
	case ROBOT_MCU_SYNC_UPGRADE_SET:
		ret = robot_mcu_upgrade_gpioctl(dev, ROBOT_MCU_LEVEL_HIGH);
		break;
	case ROBOT_MCU_SYNC_UPGRADE_RESET:
		ret = robot_mcu_upgrade_gpioctl(dev, ROBOT_MCU_LEVEL_LOW);
		break;
	default:
		hwlog_err("Unsupported parameters %d.\n", value);
		return -ENXIO;
	}

	if (ret)
		return ret;

	return size;
}

static DEVICE_ATTR(robot_mcu, S_IWUSR | S_IWGRP | S_IRUSR | S_IRGRP, robot_mcu_show, robot_mcu_store);

static int robot_mcu_parse_dts(struct device *dev, struct robot_mcu_dev_data *data)
{
	int ret;

	ret = of_property_read_u32(dev->of_node, "gpio_reset_soc_mcu", &data->gpio_reset);
	if (ret)
		hwlog_err("get reset gpio fail.\n");

	ret = of_property_read_u32(dev->of_node, "gpio_int1_mcu_soc", &data->gpio_wakeup);
	if (ret)
		hwlog_err("get int1 gpio fail.\n");

	ret = of_property_read_u32(dev->of_node, "gpio_boot0_soc_mcu", &data->gpio_boot0);
	if (ret)
		hwlog_err("get boot0 gpio fail.\n");

	ret = of_property_read_u32(dev->of_node, "gpio_int_head_touch", &data->gpio_wakeup_head_touch);
	if (ret)
		hwlog_err("get int head gpio fail.\n");

	ret = of_property_read_u32(dev->of_node, "gpio_sync_mcu_soc", &data->gpio_upgrade_ioctl);
	if (ret)
		hwlog_err("get upgrade ctl gpio fail.\n");

	ret = of_property_read_u32(dev->of_node, "gpio_int2_mcu_soc", &data->gpio_upgrade_status);
	if (ret)
		hwlog_err("get upgrade status gpio fail.\n");

	return 0;
}

static int robot_mcu_device_probe(struct platform_device *pdev)
{
	struct robot_mcu_dev_data *data = NULL;
	int ret;

	device_create_file(&pdev->dev, &dev_attr_robot_mcu);

	data = kzalloc(sizeof(struct robot_mcu_dev_data), GFP_KERNEL);
	if (data == NULL)
		return -ENOMEM;

	ret = robot_mcu_parse_dts(&pdev->dev, data);
	if (ret < 0)
		hwlog_err("smf mcu parse dts fail!\n");

	ret = robot_mcu_wakeup_irq_register(&pdev->dev, data);
	if (ret < 0)
		hwlog_err("%s: smf mcu register fail, ret=%d\n", __func__, ret);

	ret = robot_head_touch_wakeup_irq_register(&pdev->dev, data);
	if (ret < 0)
		hwlog_err("%s: smf mcu head touch irq register fail, ret=%d\n", __func__, ret);

	ret = robot_mcu_upgrade_gpio_init(&pdev->dev, data);
	if (ret < 0)
		hwlog_err("%s:smf mcu upgrade gpio fail, ret=%d\n", __func__, ret);

	platform_set_drvdata(pdev, data);

	return ret;
}

static int robot_mcu_device_remove(struct platform_device *pdev)
{
	struct robot_mcu_dev_data *data;

	data = (struct robot_mcu_dev_data *)platform_get_drvdata(pdev);
	if (data == NULL)
		return -EFAULT;

	free_irq(data->irq_wakeup, &pdev->dev);
	gpio_free(data->gpio_wakeup);
	free_irq(data->wakeup_head_touch_irq, &pdev->dev);
	gpio_free(data->gpio_wakeup_head_touch);
	gpio_free(data->gpio_upgrade_status);
	gpio_free(data->gpio_upgrade_ioctl);
	kfree(data);

	device_remove_file(&pdev->dev, &dev_attr_robot_mcu);

	return 0;
}

static const struct of_device_id robot_mcu_of_match[] = {
	{ .compatible = "huawei,robot_mcu", },
	{ },
};

static struct platform_driver robot_mcu_driver = {
	.driver = {
		.name = "robot_mcu",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(robot_mcu_of_match),
	},
	.probe = robot_mcu_device_probe,
	.remove = robot_mcu_device_remove,
};

static int __init robot_mcu_module_init(void)
{
	platform_driver_register(&robot_mcu_driver);
	return 0;
}

static void __exit robot_mcu_module_exit(void)
{
	platform_driver_unregister(&robot_mcu_driver);
}

late_initcall(robot_mcu_module_init);
module_exit(robot_mcu_module_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Robot mcu control driver.");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
