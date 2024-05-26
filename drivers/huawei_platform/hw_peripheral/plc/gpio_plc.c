/*
 * gpio_plc.c
 *
 * gpio_plc driver
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
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
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <huawei_platform/log/hw_log.h>
#include <securec.h>

#define HWLOG_TAG gpio_plc
HWLOG_REGIST();
#define PLC_RESET_KEEP_HIGH_TIME        40

struct gpio_plc_data {
	int gpio_id0_plc;
	int gpio_id1_plc;
	int gpio_rst_plc;
};

static int get_gpio_plc_from_dts(struct platform_device *pdev)
{
	struct gpio_plc_data *data = platform_get_drvdata(pdev);
	if (!data) {
		hwlog_err("%s: PLC data is NULL\n", __func__);
		return -1;
	}

	data->gpio_id0_plc = of_get_named_gpio(pdev->dev.of_node, "gpios", 0);
	if (data->gpio_id0_plc < 0) {
		hwlog_err("%s: PLC get gpio id0 failed\n", __func__);
		return -1;
	}
	data->gpio_id1_plc = of_get_named_gpio(pdev->dev.of_node, "gpios", 1);
	if (data->gpio_id1_plc < 0) {
		hwlog_err("%s: PLC get gpio id1 failed\n", __func__);
		return -1;
	}
	data->gpio_rst_plc = of_get_named_gpio(pdev->dev.of_node, "gpios", 2);
	if (data->gpio_rst_plc < 0) {
		hwlog_err("%s: PLC get gpio rst failed\n", __func__);
		return -1;
	}

	return 0;
}

static int config_gpio_plc(struct platform_device *pdev)
{
	int ret;
	struct gpio_plc_data *data = platform_get_drvdata(pdev);
	if (!data) {
		hwlog_err("%s: PLC data is NULL\n", __func__);
		return -1;
	}

	if ((ret = gpio_request(data->gpio_id0_plc, "gpio_id0_plc")) < 0) {
		hwlog_err("%s: PLC request id0 gpio err\n", __func__);
		return ret;
	}
	if ((ret = gpio_request(data->gpio_id1_plc, "gpio_id1_plc")) < 0) {
		hwlog_err("%s: PLC request id1 gpio err\n", __func__);
		goto free_gpio_id0;
	}
	if ((ret = gpio_request(data->gpio_rst_plc, "gpio_rst_plc")) < 0) {
		hwlog_err("%s: PLC request rst gpio err\n", __func__);
		goto free_gpio_id1;
	}

	if ((ret = gpio_direction_input(data->gpio_id0_plc)) < 0) {
		hwlog_err("%s: PLC set id0 gpio direction err\n", __func__);
		goto free_gpio_rst;
	}
	if ((ret = gpio_direction_input(data->gpio_id1_plc)) < 0) {
		hwlog_err("%s: PLC set id1 gpio direction err\n", __func__);
		goto free_gpio_rst;
	}
	if ((ret = gpio_direction_output(data->gpio_rst_plc, 0)) < 0) {
		hwlog_err("%s: PLC set rst gpio direction err\n", __func__);
		goto free_gpio_rst;
	}

	return 0;

free_gpio_rst:
	gpio_free(data->gpio_rst_plc);
free_gpio_id1:
	gpio_free(data->gpio_id1_plc);
free_gpio_id0:
	gpio_free(data->gpio_id0_plc);
	return ret;
}

static const struct of_device_id of_gpio_plc_match[] = {
	{ .compatible = "huawei,gpio-plc", },
	{},
};
MODULE_DEVICE_TABLE(of, of_gpio_plc_match);

static ssize_t gpio_plc_id0_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;

	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct gpio_plc_data *data = platform_get_drvdata(pdev);
	if (!data) {
		hwlog_err("%s: PLC data is NULL\n", __func__);
		return -1;
	}

	ret = gpio_get_value(data->gpio_id0_plc);
	return sprintf_s(buf, PAGE_SIZE, "%d\n", ret);
}
DEVICE_ATTR(gpio_plc_id0, 0644, gpio_plc_id0_show, NULL);

static ssize_t gpio_plc_id1_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;

	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct gpio_plc_data *data = platform_get_drvdata(pdev);
	if (!data) {
		hwlog_err("%s: PLC data is NULL\n", __func__);
		return -1;
	}

	ret = gpio_get_value(data->gpio_id1_plc);
	return sprintf_s(buf, PAGE_SIZE, "%d\n", ret);
}
DEVICE_ATTR(gpio_plc_id1, 0644, gpio_plc_id1_show, NULL);

static ssize_t gpio_plc_rst_store(struct device *dev,
	struct device_attribute *attr, const char *buf,
	size_t count)
{
	int ret;
	unsigned long value;

	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct gpio_plc_data *data = platform_get_drvdata(pdev);
	if (!data) {
		hwlog_err("%s: PLC data is NULL\n", __func__);
		return -ENOMEM;
	}

	ret = kstrtol(buf, sizeof(unsigned long), &value);
	if (ret < 0) {
		hwlog_err("%s: PLC input value error\n", __func__);
		return -EINVAL;
	}

	/*
	 * echo 0 : reset pull up.
	 * echo 1 : reset pull up, keep reset pin at high over 40ms, reset pull down.
	 */
	if (value == 0) {
		ret = gpio_direction_output(data->gpio_rst_plc, 1); // set high
		if (ret < 0) {
			hwlog_err("%s: PLC set rst gpio failed\n", __func__);
			return -EIO;
		}
	} else if (value == 1) {
		ret = gpio_direction_output(data->gpio_rst_plc, 1); // set high
		if (ret < 0) {
			hwlog_err("%s: PLC set rst gpio failed\n", __func__);
			return -EIO;
		}
		mdelay(PLC_RESET_KEEP_HIGH_TIME); // keep reset pin at high over 40ms

		ret = gpio_direction_output(data->gpio_rst_plc, 0); // set low
		if (ret < 0) {
			hwlog_err("%s: PLC set rst gpio failed\n", __func__);
			return -EIO;
		}
	} else {
		hwlog_debug("%s: PLC gpio_rst_plc only allow 0 or 1\n", __func__);
		return -EINVAL;
	}

	return count;
}
DEVICE_ATTR(gpio_plc_rst, 0644, NULL, gpio_plc_rst_store);

static struct attribute *gpio_plc_attributes[] = {
	&dev_attr_gpio_plc_id0.attr,
	&dev_attr_gpio_plc_id1.attr,
	&dev_attr_gpio_plc_rst.attr,
	NULL,
};

static const struct attribute_group gpio_plc_node = {
	.name = "gpio_plc",
	.attrs = gpio_plc_attributes,
};

static int gpio_plc_probe(struct platform_device *pdev)
{
	int ret;
	struct gpio_plc_data *data = kzalloc(sizeof(struct gpio_plc_data), GFP_KERNEL);
	if (!data) {
		hwlog_err("%s: PLC kzalloc memory failed\n", __func__);
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, data);

	ret = get_gpio_plc_from_dts(pdev);
	if (ret) {
		hwlog_err("%s: PLC parse dts failed\n", __func__);
		goto parse_dts_failed;
	}
	ret = config_gpio_plc(pdev);
	if (ret) {
		hwlog_err("%s: PLC parse dts failed\n", __func__);
		goto parse_dts_failed;
	}

	ret = sysfs_create_group(&pdev->dev.kobj, &gpio_plc_node);
	if (ret) {
		hwlog_err("%s: PLC sysfs_create_group failed\n", __func__);
		goto config_gpio_failed;
	}

	return 0;

config_gpio_failed:
	gpio_free(data->gpio_id0_plc);
	gpio_free(data->gpio_id1_plc);
	gpio_free(data->gpio_rst_plc);
parse_dts_failed:
	kfree(data);
	platform_set_drvdata(pdev, NULL);
	return -ENOMEM;
}

static int gpio_plc_remove(struct platform_device *pdev)
{
	struct gpio_plc_data *data = platform_get_drvdata(pdev);
	if (!data) {
		hwlog_err("%s: PLC data is NULL\n", __func__);
		return -1;
	}

	gpio_free(data->gpio_id0_plc);
	gpio_free(data->gpio_id1_plc);
	gpio_free(data->gpio_rst_plc);
	kfree(data);
	platform_set_drvdata(pdev, NULL);
	sysfs_remove_group(&pdev->dev.kobj, &gpio_plc_node);

	return 0;
}

static struct platform_driver gpio_plc_driver = {
	.probe = gpio_plc_probe,
	.remove = gpio_plc_remove,
	.driver = {
		.name = "gpio-plc",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(of_gpio_plc_match),
	},
};

module_platform_driver(gpio_plc_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("gpio-plc driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
