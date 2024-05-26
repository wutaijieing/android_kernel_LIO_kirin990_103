// SPDX-License-Identifier: GPL-2.0
/*
 * power_supply_application.c
 *
 * power supply application for power module
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

#include <linux/device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include <chipset_common/hwpower/common_module/power_debug.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/common_module/power_supply_interface.h>
#include <chipset_common/hwpower/common_module/power_supply.h>
#include <huawei_platform/hwpower/common_module/power_platform_macro.h>

#define HWLOG_TAG power_psy_app
HWLOG_REGIST();

#define PSY_NAME_LEN    16

struct power_supply_app_dev {
	struct device *dev;
	char bat_psy[PSY_NAME_LEN];
	char usb_psy[PSY_NAME_LEN];
};

static struct power_supply_app_dev *g_power_supply_app_dev;

static struct power_supply_app_dev *power_supply_app_get_dev(void)
{
	if (!g_power_supply_app_dev) {
		hwlog_err("g_power_supply_app_dev is null\n");
		return NULL;
	}

	return g_power_supply_app_dev;
}

const char *power_supply_app_get_bat_brand(void)
{
	struct power_supply_app_dev *l_dev = power_supply_app_get_dev();
	const char *val = POWER_SUPPLY_DEFAULT_BRAND;
	int ret;

	if (!l_dev)
		return val;

	ret = power_supply_get_str_property_value(l_dev->bat_psy,
		POWER_SUPPLY_PROP_BRAND, &val);
	if (ret) {
		hwlog_err("use default bat_brand: value=%s\n", val);
		return val;
	}

	hwlog_info("bat_brand: value=%s\n", val);
	return val;
}

int power_supply_app_get_bat_capacity(void)
{
	struct power_supply_app_dev *l_dev = power_supply_app_get_dev();
	int val = POWER_SUPPLY_DEFAULT_CAPACITY;
	int ret;

	if (!l_dev)
		return val;

	ret = power_supply_get_int_property_value(l_dev->bat_psy,
		POWER_SUPPLY_PROP_CAPACITY, &val);
	if (ret) {
		hwlog_err("use default bat_capacity: value=%d\n", val);
		return val;
	}

	hwlog_info("bat_capacity: value=%d\n", val);
	return val;
}

int power_supply_app_get_bat_temp(void)
{
	struct power_supply_app_dev *l_dev = power_supply_app_get_dev();
	int unit = POWER_PLATFORM_BAT_TEMP_UNIT;
	int val = POWER_SUPPLY_DEFAULT_TEMP / 10; /* 10: convert temperature unit */
	int ret;

	if (!l_dev)
		return val;

	ret = power_supply_get_int_property_value(l_dev->bat_psy,
		POWER_SUPPLY_PROP_TEMP, &val);
	if (ret) {
		hwlog_err("use default bat_temp: value=%d\n", val);
		return val;
	}

	hwlog_info("bat_temp: value=%d unit=%d\n", val, unit);
	return val / unit;
}

int power_supply_app_get_bat_status(void)
{
	struct power_supply_app_dev *l_dev = power_supply_app_get_dev();
	int val = POWER_SUPPLY_DEFAULT_STATUS;
	int ret;

	if (!l_dev)
		return val;

	ret = power_supply_get_int_property_value(l_dev->bat_psy,
		POWER_SUPPLY_PROP_STATUS, &val);
	if (ret) {
		hwlog_err("use default bat_status: value=%d\n", val);
		return val;
	}

	hwlog_info("bat_status: value=%d\n", val);
	return val;
}

int power_supply_app_get_bat_current_now(void)
{
	struct power_supply_app_dev *l_dev = power_supply_app_get_dev();
	int val = POWER_SUPPLY_DEFAULT_CURRENT_NOW;
	int ret;

	if (!l_dev)
		return val;

	ret = power_supply_get_int_property_value(l_dev->bat_psy,
		POWER_SUPPLY_PROP_CURRENT_NOW, &val);
	if (ret) {
		hwlog_err("use default bat_current_now: value=%d\n", val);
		return val;
	}

	hwlog_info("bat_current_now: value=%d\n", val);
	return val;
}

int power_supply_app_get_bat_voltage_now(void)
{
	struct power_supply_app_dev *l_dev = power_supply_app_get_dev();
	int unit = POWER_PLATFORM_BAT_VOLT_NOW_UNIT;
	int val = POWER_SUPPLY_DEFAULT_VOLTAGE_NOW;
	int ret;

	if (!l_dev)
		return val;

	ret = power_supply_get_int_property_value(l_dev->bat_psy,
		POWER_SUPPLY_PROP_VOLTAGE_NOW, &val);
	if (ret) {
		hwlog_err("use default bat_voltage_now: value=%d\n", val);
		return val;
	}

	hwlog_info("bat_voltage_now: value=%d unit=%d\n", val, unit);
	return val / unit;
}

int power_supply_app_get_bat_voltage_max(void)
{
	struct power_supply_app_dev *l_dev = power_supply_app_get_dev();
	int unit = POWER_PLATFORM_BAT_VOLT_MAX_UNIT;
	int val = POWER_SUPPLY_DEFAULT_VOLTAGE_MAX;
	int ret;

	if (!l_dev)
		return val;

	ret = power_supply_get_int_property_value(l_dev->bat_psy,
		POWER_SUPPLY_PROP_VOLTAGE_MAX, &val);
	if (ret) {
		hwlog_err("use default bat_voltage_max: value=%d\n", val);
		return val;
	}

	hwlog_info("bat_voltage_max: value=%d unit=%d\n", val, unit);
	return val / unit;
}

int power_supply_app_get_usb_voltage_now(void)
{
	struct power_supply_app_dev *l_dev = power_supply_app_get_dev();
	int unit = POWER_PLATFORM_USB_VOLT_NOW_UNIT;
	int val = 0;
	int ret;

	if (!l_dev)
		return val;

	ret = power_supply_get_int_property_value(l_dev->usb_psy,
		POWER_SUPPLY_PROP_VOLTAGE_NOW, &val);
	if (ret) {
		hwlog_err("use default usb_voltage_now: value=%d\n", val);
		return val;
	}

	hwlog_info("usb_voltage_now: value=%d unit=%d\n", val, unit);
	return val / unit;
}

static ssize_t power_supply_app_dbg_bat_brand_show(void *dev_data,
	char *buf, size_t size)
{
	return scnprintf(buf, size, "bat_brand:%s\n",
		power_supply_app_get_bat_brand());
}

static ssize_t power_supply_app_dbg_bat_capacity_show(void *dev_data,
	char *buf, size_t size)
{
	return scnprintf(buf, size, "bat_capacity:%d\n",
		power_supply_app_get_bat_capacity());
}

static ssize_t power_supply_app_dbg_bat_temp_show(void *dev_data,
	char *buf, size_t size)
{
	return scnprintf(buf, size, "bat_temp:%d\n",
		power_supply_app_get_bat_temp());
}

static ssize_t power_supply_app_dbg_bat_status_show(void *dev_data,
	char *buf, size_t size)
{
	return scnprintf(buf, size, "bat_status:%d\n",
		power_supply_app_get_bat_status());
}

static ssize_t power_supply_app_dbg_bat_current_now_show(void *dev_data,
	char *buf, size_t size)
{
	return scnprintf(buf, size, "bat_current_now:%d\n",
		power_supply_app_get_bat_current_now());
}

static ssize_t power_supply_app_dbg_bat_voltage_now_show(void *dev_data,
	char *buf, size_t size)
{
	return scnprintf(buf, size, "bat_voltage_now:%d\n",
		power_supply_app_get_bat_voltage_now());
}

static ssize_t power_supply_app_dbg_bat_voltage_max_show(void *dev_data,
	char *buf, size_t size)
{
	return scnprintf(buf, size, "bat_voltage_max:%d\n",
		power_supply_app_get_bat_voltage_max());
}

static ssize_t power_supply_app_dbg_usb_voltage_now_show(void *dev_data,
	char *buf, size_t size)
{
	return scnprintf(buf, size, "usb_voltage_now:%d\n",
		power_supply_app_get_usb_voltage_now());
}

static void power_supply_app_dbg_register(void *dev_data)
{
	power_dbg_ops_register("power_supply_app", "bat_brand", dev_data,
		power_supply_app_dbg_bat_brand_show, NULL);
	power_dbg_ops_register("power_supply_app", "bat_capacity", dev_data,
		power_supply_app_dbg_bat_capacity_show, NULL);
	power_dbg_ops_register("power_supply_app", "bat_temp", dev_data,
		power_supply_app_dbg_bat_temp_show, NULL);
	power_dbg_ops_register("power_supply_app", "bat_status", dev_data,
		power_supply_app_dbg_bat_status_show, NULL);
	power_dbg_ops_register("power_supply_app", "bat_current_now", dev_data,
		power_supply_app_dbg_bat_current_now_show, NULL);
	power_dbg_ops_register("power_supply_app", "bat_voltage_now", dev_data,
		power_supply_app_dbg_bat_voltage_now_show, NULL);
	power_dbg_ops_register("power_supply_app", "bat_voltage_max", dev_data,
		power_supply_app_dbg_bat_voltage_max_show, NULL);
	power_dbg_ops_register("power_supply_app", "usb_voltage_now", dev_data,
		power_supply_app_dbg_usb_voltage_now_show, NULL);
}

static void power_supply_app_parse_dts(struct power_supply_app_dev *l_dev)
{
	const char *bat_psy = NULL;
	const char *usb_psy = NULL;

	if (power_dts_read_string_compatible(power_dts_tag(HWLOG_TAG),
		"huawei,power_supply_app", "bat_psy_name", &bat_psy)) {
		strncpy(l_dev->bat_psy, POWER_PLATFORM_BAT_PSY_NAME, PSY_NAME_LEN - 1);
		hwlog_info("default bat_psy=%s\n", l_dev->bat_psy);
	} else {
		strncpy(l_dev->bat_psy, bat_psy, PSY_NAME_LEN - 1);
		hwlog_info("customize bat_psy=%s\n", l_dev->bat_psy);
	}

	if (power_dts_read_string_compatible(power_dts_tag(HWLOG_TAG),
		"huawei,power_supply_app", "usb_psy_name", &usb_psy)) {
		strncpy(l_dev->usb_psy, POWER_PLATFORM_USB_PSY_NAME, PSY_NAME_LEN - 1);
		hwlog_info("default usb_psy=%s\n", l_dev->usb_psy);
	} else {
		strncpy(l_dev->usb_psy, usb_psy, PSY_NAME_LEN - 1);
		hwlog_info("customize usb_psy=%s\n", l_dev->usb_psy);
	}
}

static int __init power_supply_app_init(void)
{
	struct power_supply_app_dev *l_dev = NULL;

	l_dev = kzalloc(sizeof(*l_dev), GFP_KERNEL);
	if (!l_dev)
		return -ENOMEM;

	g_power_supply_app_dev = l_dev;
	power_supply_app_parse_dts(l_dev);
	power_supply_app_dbg_register(l_dev);
	return 0;
}

static void __exit power_supply_app_exit(void)
{
	if (!g_power_supply_app_dev)
		return;

	kfree(g_power_supply_app_dev);
	g_power_supply_app_dev = NULL;
}

subsys_initcall_sync(power_supply_app_init);
module_exit(power_supply_app_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("power supply application driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
