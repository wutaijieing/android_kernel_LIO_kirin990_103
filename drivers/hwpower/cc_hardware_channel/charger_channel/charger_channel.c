// SPDX-License-Identifier: GPL-2.0
/*
 * charger_channel.c
 *
 * charger channel switch
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

#include <chipset_common/hwpower/hardware_channel/charger_channel.h>
#include <chipset_common/hwpower/common_module/power_gpio.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>

#define HWLOG_TAG charger_channel
HWLOG_REGIST();

struct charger_ch_dev {
	struct device *dev;
	int gpio_ch;
};

static struct charger_ch_dev *g_charger_ch_di;

void charger_select_channel(int channel)
{
	int gpio_val;
	struct charger_ch_dev *di = g_charger_ch_di;

	if (!di)
		return;

	if (channel == CHARGER_CH_USBIN)
		gpio_val = 0;
	else if (channel == CHARGER_CH_WLSIN)
		gpio_val = 1;
	else
		return;

	gpio_set_value(di->gpio_ch, gpio_val);
	hwlog_info("[select_channel] gpio %s now\n",
		gpio_get_value(di->gpio_ch) ? "high" : "low");
}

static int charger_channel_gpio_init(struct device_node *np, struct charger_ch_dev *di)
{
	if (power_gpio_config_output(np, "gpio_ch", "charger_channel",
		&di->gpio_ch, CHARGER_CH_USBIN))
		return -ENODEV;

	return 0;
}

static int charger_channel_probe(struct platform_device *pdev)
{
	struct charger_ch_dev *di = NULL;
	struct device_node *np = NULL;

	if (!pdev || !pdev->dev.of_node)
		return -ENODEV;

	di = devm_kzalloc(&pdev->dev, sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	di->dev = &pdev->dev;
	np = pdev->dev.of_node;
	platform_set_drvdata(pdev, di);

	if (charger_channel_gpio_init(np, di)) {
		devm_kfree(&pdev->dev, di);
		return -ENODEV;
	}

	g_charger_ch_di = di;
	hwlog_info("probe ok\n");
	return 0;
}

static int charger_channel_remove(struct platform_device *pdev)
{
	struct ovp_chsw_info *di = platform_get_drvdata(pdev);

	if (!di)
		return -ENODEV;

	devm_kfree(&pdev->dev, di);
	platform_set_drvdata(pdev, NULL);
	g_charger_ch_di = NULL;
	return 0;
}

static const struct of_device_id charger_ch_match_table[] = {
	{
		.compatible = "huawei,charger_channel",
		.data = NULL,
	},
	{},
};

static struct platform_driver charger_ch_driver = {
	.probe = charger_channel_probe,
	.remove = charger_channel_remove,
	.driver = {
		.name = "huawei,charger_channel",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(charger_ch_match_table),
	},
};

static int __init charger_channel_init(void)
{
	return platform_driver_register(&charger_ch_driver);
}

static void __exit charger_channel_exit(void)
{
	platform_driver_unregister(&charger_ch_driver);
}

fs_initcall_sync(charger_channel_init);
module_exit(charger_channel_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("charger channel module driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
