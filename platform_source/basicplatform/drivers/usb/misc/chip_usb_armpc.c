/*
 * hisi_usb_armpc.c
 *
 * hisi usb armpc driver
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

static int hisi_usb_armpc_probe(struct platform_device *pdev)
{
	bool mode;
	struct device_node *node = pdev->dev.of_node;

	mode = of_property_read_bool(node, "switch_usb_mode");
	pr_info("mode is %s\n", mode ? "true" : "false");
	if (mode) {
		pd_event_notify(TCA_IRQ_HPD_IN, TCPC_USB31_AND_DP_2LINE, TCA_ID_FALL_EVENT, TYPEC_ORIEN_POSITIVE);
		pd_event_notify(TCA_IRQ_HPD_IN, TCPC_USB31_AND_DP_2LINE, TCA_DP_IN, TYPEC_ORIEN_POSITIVE);
	}

	return 0;
}

static int hisi_usb_armpc_remove(struct platform_device *pdev)
{
	pr_info("usb armpc remove\n");
	return 0;
}

static const struct of_device_id hisi_usb_armpc_match_table[] = {
	{ .compatible = "hisilicon,armpc-usb", },
	{},
};
MODULE_DEVICE_TABLE(of, hisi_usb_armpc_match_table);

static struct platform_driver hisi_usb_armpc_driver = {
	.probe = hisi_usb_armpc_probe,
	.remove = hisi_usb_armpc_remove,
	.driver = {
		.name = "hisilicon,armpc-usb",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hisi_usb_armpc_match_table),
	},
};

static int __init hisi_usb_armpc_init(void)
{
	return platform_driver_register(&hisi_usb_armpc_driver);
}

static void __exit hisi_usb_armpc_exit(void)
{
	platform_driver_unregister(&hisi_usb_armpc_driver);
}

late_initcall(hisi_usb_armpc_init);
module_exit(hisi_usb_armpc_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("usb armpc logic driver");