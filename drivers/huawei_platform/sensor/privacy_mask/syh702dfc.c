/*
 * syh702dfc.c
 *
 * privacy mask driver
 *
 * Copyright (c) 2021-2022 Huawei Technologies Co., Ltd.
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
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <securec.h>

#include "huawei_platform/log/hw_log.h"

#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif
#define HWLOG_TAG privacy_mask
HWLOG_REGIST();
#define HI_DURATION 150
enum privacy_mask_mode {
	COAST = 0,
	REVERSE,
	FORWARD,
	BRAKE,
};
struct privacy_mask_data {
	int gpio_in1;
	int gpio_in2;
};

static void syh702dfc_change_mode(struct privacy_mask_data *di, enum privacy_mask_mode mode)
{
	if (!di) {
		hwlog_err("%s: SYH702DFC data is NULL\n", __func__);
		return;
	}

	switch (mode) {
	case COAST:
		(void)gpio_direction_output(di->gpio_in1, 0);
		(void)gpio_direction_output(di->gpio_in2, 0);
		break;
	case REVERSE:
		(void)gpio_direction_output(di->gpio_in2, 1);
		mdelay(HI_DURATION);
		(void)gpio_direction_output(di->gpio_in2, 0);
		break;
	case FORWARD:
		(void)gpio_direction_output(di->gpio_in1, 1);
		mdelay(HI_DURATION);
		(void)gpio_direction_output(di->gpio_in1, 0);
		break;
	case BRAKE:
		(void)gpio_direction_output(di->gpio_in1, 1);
		(void)gpio_direction_output(di->gpio_in2, 1);
		mdelay(HI_DURATION);
		(void)gpio_direction_output(di->gpio_in1, 0);
		(void)gpio_direction_output(di->gpio_in2, 0);
		break;
	default:
		hwlog_debug("%s: SYH702DFC invalid param, mode=%d\n", __func__, mode);
		break;
	}
}

static ssize_t syh702dfc_set_value_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long value;

	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct privacy_mask_data *di = platform_get_drvdata(pdev);
	if (!di) {
		hwlog_err("%s: SYH702DFC data is NULL\n", __func__);
		return -ENOMEM;
	}

	if (kstrtol(buf, sizeof(value), &value) != 0) {
		hwlog_err("%s: SYH702DFC input value error\n", __func__);
		return -EINVAL;
	}

	syh702dfc_change_mode(di, value);

	return count;
}
static DEVICE_ATTR(param, 0644, NULL, syh702dfc_set_value_store);

static struct attribute *syh702dfc_attributes[] = {
	&dev_attr_param.attr,
	NULL,
};

static const struct attribute_group syh702dfc_attr_group = {
	.name = "syh702dfc",
	.attrs = syh702dfc_attributes,
};

static int syh702dfc_parse_dts(struct platform_device *pdev)
{
	int ret;
	struct privacy_mask_data *di = platform_get_drvdata(pdev);
	if (!di) {
		hwlog_err("%s: SYH702DFC data is NULL\n", __func__);
		return -ENOMEM;
	}

	ret = of_get_named_gpio(pdev->dev.of_node, "gpios", 0);
	if (ret < 0) {
		hwlog_err("%s: SYH702DFC get gpio in1 failed, ret=%d\n", __func__, ret);
		return ret;
	}
	di->gpio_in1 = ret;
	ret = of_get_named_gpio(pdev->dev.of_node, "gpios", 1);
	if (ret < 0) {
		hwlog_err("%s: SYH702DFC get gpio in2 failed, ret=%d\n", __func__, ret);
		return ret;
	}
	di->gpio_in2 = ret;

	ret = gpio_request(di->gpio_in1, "syh702dfc_in1");
	if (ret < 0) {
		hwlog_err("%s: SYH702DFC request syh702dfc in1 failed, ret=%d\n", __func__, ret);
		return ret;
	}
	ret = gpio_request(di->gpio_in2, "syh702dfc_in2");
	if (ret < 0) {
		gpio_free(di->gpio_in1);
		hwlog_err("%s: SYH702DFC request syh702dfc in2 failed, ret=%d\n", __func__, ret);
		return ret;
	}

	syh702dfc_change_mode(di, COAST);
	return 0;
}

static int syh702dfc_probe(struct platform_device *pdev)
{
	int ret;
	struct privacy_mask_data *di = NULL;

	di = devm_kzalloc(&pdev->dev, sizeof(*di), GFP_KERNEL);
	if (!di) {
		hwlog_err("%s: SYH702DFC alloc memory failed\n", __func__);
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, di);

	ret = syh702dfc_parse_dts(pdev);
	if (ret != 0) {
		hwlog_err("%s: SYH702DFC parse dts failed\n");
		goto parse_dts_failed;
	}

	ret = sysfs_create_group(&pdev->dev.kobj, &syh702dfc_attr_group);
	if (ret != 0) {
		hwlog_err("%s: SYH702DFC sysfs_create_group failed\n", __func__);
		goto create_group_failed;
	}

	return 0;

create_group_failed:
	gpio_free(di->gpio_in1);
	gpio_free(di->gpio_in2);
parse_dts_failed:
	platform_set_drvdata(pdev, NULL);
	return -EFAULT;
}

static int syh702dfc_remove(struct platform_device *pdev)
{
	struct privacy_mask_data *di = platform_get_drvdata(pdev);

	gpio_free(di->gpio_in1);
	gpio_free(di->gpio_in2);
	platform_set_drvdata(pdev, NULL);
	sysfs_remove_group(&pdev->dev.kobj, &syh702dfc_attr_group);

	return 0;
}

static const struct of_device_id syh702dfc_match[] = {
	{.compatible = "huawei,privacy_mask_syh702dfc"},
	{},
};
MODULE_DEVICE_TABLE(of, syh702dfc_match);

static struct platform_driver syh702dfc_driver = {
	.probe = syh702dfc_probe,
	.remove = syh702dfc_remove,
	.driver = {
		.name   = "syh702dfc",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(syh702dfc_match),
	},
};

module_platform_driver(syh702dfc_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("privacy mask driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
