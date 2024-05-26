// SPDX-License-Identifier: GPL-2.0
/*
 * battery_fault_notify.c
 *
 * driver for battery fault notify.
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

#include <huawei_platform/hwpower/common_module/power_platform.h>
#include <chipset_common/hwpower/battery/battery_fault.h>

#define HWLOG_TAG hw_battery_fault_notify
HWLOG_REGIST();

static void hw_battery_fault_notify(unsigned int event)
{
	struct blocking_notifier_head *notifier_list = NULL;

	hwlog_info("fault notify event = %u\n", event);
	get_notifier_list(&notifier_list);
	if (!notifier_list)
		return;

	switch (event) {
	case POWER_NE_COUL_LOW_VOL:
		blocking_notifier_call_chain(notifier_list,
			BATTERY_LOW_SHUTDOWN, NULL);
		break;
	default:
		break;
	}
}

static const struct bat_fault_ops hw_battery_fault_ops = {
	.notify = hw_battery_fault_notify,
};

static int hw_battery_fault_notify_probe(struct platform_device *pdev)
{
	return bat_fault_register_ops(&hw_battery_fault_ops);
}

static int hw_battery_fault_notify_remove(struct platform_device *pdev)
{
	return bat_fault_register_ops(NULL);
}

static const struct of_device_id hw_battery_fault_notify_match_table[] = {
	{
		.compatible = "huawei,battery_fault_notify",
		.data = NULL,
	},
	{},
};

static struct platform_driver hw_battery_fault_notify_driver = {
	.probe = hw_battery_fault_notify_probe,
	.remove = hw_battery_fault_notify_remove,
	.driver = {
		.name = "huawei,battery_fault_notify",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hw_battery_fault_notify_match_table),
	},
};

static int __init hw_battery_fault_notify_init(void)
{
	return platform_driver_register(&hw_battery_fault_notify_driver);
}

static void __exit hw_battery_fault_notify_exit(void)
{
	platform_driver_unregister(&hw_battery_fault_notify_driver);
}

late_initcall(hw_battery_fault_notify_init);
module_exit(hw_battery_fault_notify_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("huawei battery fault notify driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
