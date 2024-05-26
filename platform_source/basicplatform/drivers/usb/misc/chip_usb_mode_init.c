/*
 * chip_usb_mode_init.c
 *
 * chip usb init mode driver
 *
 * Copyright (c) 2016-2020 Huawei Technologies Co., Ltd.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_drivers/usb/tca.h>

enum usb_init_mode {
	COMBOPHY_INIT_DEFAULT_MODE,
	COMBOPHY_INIT_USB_DP_MODE,
	COMBOPHY_INIT_USB_DEVICE_MODE
};

static int chip_usb_mode_init_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	const char *init_mode = NULL;
	int mode;

	if (of_property_read_string(node, "init-mode",
				&init_mode) == 0) {
		if (strcmp(init_mode, "usb_dp") == 0)
			mode = COMBOPHY_INIT_USB_DP_MODE;
		else if (strcmp(init_mode, "usb_device") == 0)
			mode = COMBOPHY_INIT_USB_DEVICE_MODE;
		else
			mode = COMBOPHY_INIT_DEFAULT_MODE;
	} else {
		pr_info("init-mode property is not found\n");
		mode = COMBOPHY_INIT_DEFAULT_MODE;
	}

	if (mode == COMBOPHY_INIT_USB_DP_MODE) {
		pd_event_notify(TCA_IRQ_HPD_IN, TCPC_USB31_AND_DP_2LINE,
				TCA_ID_FALL_EVENT, TYPEC_ORIEN_POSITIVE);
		pd_event_notify(TCA_IRQ_HPD_IN, TCPC_USB31_AND_DP_2LINE,
				TCA_DP_IN, TYPEC_ORIEN_POSITIVE);
	} else if (mode == COMBOPHY_INIT_USB_DEVICE_MODE){
		pd_event_notify(TCA_IRQ_HPD_IN, TCPC_USB31_CONNECTED,
			TCA_CHARGER_CONNECT_EVENT, TYPEC_ORIEN_POSITIVE);
	}

	pr_info("[USB] usb mode init success, init-mode:%d\n", mode);
	return 0;
}

static int chip_usb_mode_init_remove(struct platform_device *pdev)
{
	pr_info("usb mode init remove\n");
	return 0;
}

static const struct of_device_id chip_usb_mode_init_match_table[] = {
	{ .compatible = "chip,usb_mode_init", },
	{},
};
MODULE_DEVICE_TABLE(of, chip_usb_mode_init_match_table);

static struct platform_driver chip_usb_mode_init_driver = {
	.probe = chip_usb_mode_init_probe,
	.remove = chip_usb_mode_init_remove,
	.driver = {
		.name = "chip,usb_mode_init",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(chip_usb_mode_init_match_table),
	},
};

static int __init chip_usb_mode_init(void)
{
	return platform_driver_register(&chip_usb_mode_init_driver);
}

static void __exit chip_usb_mode_exit(void)
{
	platform_driver_unregister(&chip_usb_mode_init_driver);
}

device_initcall_sync(chip_usb_mode_init);
module_exit(chip_usb_mode_exit);
