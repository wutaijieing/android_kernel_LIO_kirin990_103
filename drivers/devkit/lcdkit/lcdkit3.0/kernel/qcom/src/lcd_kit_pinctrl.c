/*
 * lcd_kit_pinctrl.c
 *
 * lcdkit pinctrl function for lcd driver
 *
 * Copyright (c) 2020-2021 Huawei Technologies Co., Ltd.
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
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include "lcd_kit_drm_panel.h"

static struct pinctrl *lcd_pinctrl_dev = NULL;

void lcd_kit_set_pinctrl_status(const char *pinctrl_status)
{
	int ret;
	static struct pinctrl_state *lcd_pinctrl_status = NULL;

	if (pinctrl_status == NULL) {
		LCD_KIT_ERR("default_status is null!\n");
		return;
	}

	if ((lcd_pinctrl_dev == NULL) || IS_ERR(lcd_pinctrl_dev)) {
		LCD_KIT_ERR("lcd_pinctrl is null!\n");
		return;
	}

	lcd_pinctrl_status = pinctrl_lookup_state(lcd_pinctrl_dev, pinctrl_status);
	if (IS_ERR(lcd_pinctrl_status)) {
		LCD_KIT_ERR("could not get %s state.\n", pinctrl_status);
		devm_pinctrl_put(lcd_pinctrl_dev);
		lcd_pinctrl_dev = NULL;
		return;
	}

	ret = pinctrl_select_state(lcd_pinctrl_dev, lcd_pinctrl_status);
	if (ret)
		LCD_KIT_ERR("lcd_set_pinctrl %s fail %d.\n", pinctrl_status);

	LCD_KIT_INFO("lcd pinctrl set to %s status.\n", pinctrl_status);
}

void lcd_kit_pinctrl_put(void)
{
	if ((lcd_pinctrl_dev == NULL) || IS_ERR(lcd_pinctrl_dev)) {
		LCD_KIT_ERR("lcd_pinctrl is null!\n");
		return;
	}

	devm_pinctrl_put(lcd_pinctrl_dev);

	LCD_KIT_INFO("lcd pinctrl put.\n");
}

static int lcd_kit_pinctrl_probe(struct platform_device *pdev)
{
	LCD_KIT_INFO(" enter!\n");

	lcd_pinctrl_dev = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(lcd_pinctrl_dev)) {
		LCD_KIT_ERR("could not get pinctrl.\n");
		return LCD_KIT_FAIL;
	}

	LCD_KIT_INFO(" exit!\n");

	return LCD_KIT_OK;
}

static struct of_device_id lcd_kit_pinctrl_cfg_match_table[] = {
	{
		.compatible = "huawei,lcd_pinctrl_config",
		.data = NULL,
	},
	{},
};

static struct platform_driver lcd_kit_pinctrl_cfg_driver = {
	.probe = lcd_kit_pinctrl_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = "lcd_kit_pinctrl_cfg",
		.of_match_table = lcd_kit_pinctrl_cfg_match_table,
	},
};

static int __init lcd_kit_pinctrl_cfg_init(void)
{
	int ret;

	ret = platform_driver_register(&lcd_kit_pinctrl_cfg_driver);
	if (ret)
		LCD_KIT_ERR("platform_driver_register failed, error=%d!\n", ret);

	return ret;
}

module_init(lcd_kit_pinctrl_cfg_init);
