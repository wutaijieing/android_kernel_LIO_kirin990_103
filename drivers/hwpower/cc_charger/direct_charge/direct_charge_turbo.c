// SPDX-License-Identifier: GPL-2.0
/*
 * direct_charge_turbo.c
 *
 * direct charge turbo driver
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <chipset_common/hwpower/common_module/power_common_macro.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_event_ne.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/common_module/power_sysfs.h>
#include <chipset_common/hwpower/direct_charge/direct_charge_turbo.h>
#include <securec.h>

#define HWLOG_TAG direct_charge_turbo
HWLOG_REGIST();

#define TURBO_CHARGE_SUPPORT         1
#define TURBO_SYSFS_BUF_SIZE         32
#define TURBO_CHARGE_STATUS_BUF_SIZE 128

enum direct_charge_turbo_sysfs_type {
	DIRECT_CHARGE_TURBO_SYSFS_BEGIN = 0,
	DIRECT_CHARGE_TURBO_SYSFS_TURBO_SUPPORT = DIRECT_CHARGE_TURBO_SYSFS_BEGIN,
	DIRECT_CHARGE_TURBO_SYSFS_TURBO_CHARGE_STATUS,
	DIRECT_CHARGE_TURBO_SYSFS_END,
};

enum {
	TURBO_CHARGE_DISABLE,
	TURBO_CHARGE_NEED_AGREE,
	TURBO_CHARGE_ENABLE,
};

struct direct_charge_turbo_dev {
	struct device *dev;
	u32 turbo_charge_status;
	bool time_para_parse_ok;
	struct direct_charge_time_para time_para[DC_TIME_PARA_LEVEL];
};

static struct direct_charge_turbo_dev *g_direct_charge_turbo_dev;

int direct_charge_turbo_get_time_para(struct direct_charge_time_para **para, int *size)
{
	struct direct_charge_turbo_dev *l_dev = g_direct_charge_turbo_dev;

	if (!l_dev || (l_dev->turbo_charge_status != TURBO_CHARGE_ENABLE) || !l_dev->time_para_parse_ok)
		return -EINVAL;

	*para = &l_dev->time_para[0];
	*size = DC_TIME_PARA_LEVEL;
	return 0;
}

static void direct_charge_turbo_set_charge_status(u32 value)
{
	struct power_event_notify_data n_data;
	char temp[TURBO_CHARGE_STATUS_BUF_SIZE] = { 0 };
	struct direct_charge_turbo_dev *l_dev = g_direct_charge_turbo_dev;

	if (!l_dev || (value > TURBO_CHARGE_ENABLE)) {
		hwlog_err("%s di is null or value is invalid\n", __func__);
		return;
	}

	if (value == l_dev->turbo_charge_status) {
		hwlog_info("ignore same turbo charge status\n");
		return;
	}

	l_dev->turbo_charge_status = value;
	n_data.event_len = snprintf_s(temp, TURBO_CHARGE_STATUS_BUF_SIZE, TURBO_CHARGE_STATUS_BUF_SIZE - 1,
		"TURBO_CHARGE_STATUS=%d", l_dev->turbo_charge_status);

	if (n_data.event_len < 0) {
		hwlog_info("fill turbo charge uevent buffer fail\n");
		return;
	}

	n_data.event = temp;
	power_event_report_uevent(&n_data);
	hwlog_info("set turbo charge status=%u, report turbo charge status uevent\n", l_dev->turbo_charge_status);
}

#ifdef CONFIG_SYSFS
static ssize_t direct_charge_turbo_sysfs_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t direct_charge_turbo_sysfs_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count);

static struct power_sysfs_attr_info direct_charge_turbo_sysfs_field_tbl[] = {
	power_sysfs_attr_ro(direct_charge_turbo, 0440, DIRECT_CHARGE_TURBO_SYSFS_TURBO_SUPPORT, turbo_support),
	power_sysfs_attr_rw(direct_charge_turbo, 0660, DIRECT_CHARGE_TURBO_SYSFS_TURBO_CHARGE_STATUS, turbo_charge_status),
};

#define DIRECT_CHARGE_TURBO_SYSFS_ATTRS_SIZE ARRAY_SIZE(direct_charge_turbo_sysfs_field_tbl)

static struct attribute *direct_charge_turbo_sysfs_attrs[DIRECT_CHARGE_TURBO_SYSFS_ATTRS_SIZE + 1];

static const struct attribute_group direct_charge_turbo_sysfs_attr_group = {
	.attrs = direct_charge_turbo_sysfs_attrs,
};

static ssize_t direct_charge_turbo_sysfs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct power_sysfs_attr_info *info = NULL;
	struct direct_charge_turbo_dev *l_dev = g_direct_charge_turbo_dev;

	if (!l_dev)
		return -ENODEV;

	info = power_sysfs_lookup_attr(attr->attr.name, direct_charge_turbo_sysfs_field_tbl,
		DIRECT_CHARGE_TURBO_SYSFS_ATTRS_SIZE);
	if (!info)
		return -EINVAL;

	switch (info->name) {
	case DIRECT_CHARGE_TURBO_SYSFS_TURBO_SUPPORT:
		return scnprintf(buf, TURBO_SYSFS_BUF_SIZE, "%d\n", TURBO_CHARGE_SUPPORT);
	case DIRECT_CHARGE_TURBO_SYSFS_TURBO_CHARGE_STATUS:
		return scnprintf(buf, TURBO_SYSFS_BUF_SIZE, "%d\n", l_dev->turbo_charge_status);
	default:
		return 0;
	}
}

static ssize_t direct_charge_turbo_sysfs_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int val = 0;
	struct power_sysfs_attr_info *info = NULL;
	struct direct_charge_turbo_dev *l_dev = g_direct_charge_turbo_dev;

	if (!l_dev)
		return -ENODEV;

	info = power_sysfs_lookup_attr(attr->attr.name, direct_charge_turbo_sysfs_field_tbl,
		DIRECT_CHARGE_TURBO_SYSFS_ATTRS_SIZE);
	if (!info)
		return -EINVAL;

	switch (info->name) {
	case DIRECT_CHARGE_TURBO_SYSFS_TURBO_CHARGE_STATUS:
		if (kstrtoint(buf, POWER_BASE_DEC, &val))
			return -EINVAL;
		direct_charge_turbo_set_charge_status(val);
		break;
	default:
		break;
	}

	return count;
}

static void direct_charge_turbo_sysfs_create_group(struct device *dev)
{
	power_sysfs_init_attrs(direct_charge_turbo_sysfs_attrs, direct_charge_turbo_sysfs_field_tbl,
		DIRECT_CHARGE_TURBO_SYSFS_ATTRS_SIZE);
	power_sysfs_create_link_group("hw_power", "charger", "direct_charge_turbo", dev,
		&direct_charge_turbo_sysfs_attr_group);
}

static void direct_charge_turbo_sysfs_remove_group(struct device *dev)
{
	power_sysfs_remove_link_group("hw_power", "charger", "direct_charge_turbo", dev,
		&direct_charge_turbo_sysfs_attr_group);
}
#else
static inline void direct_charge_turbo_sysfs_create_group(struct device *dev)
{
}

static inline void direct_charge_turbo_sysfs_remove_group(struct device *dev)
{
}
#endif /* CONFIG_SYSFS */

static void direct_charge_turbo_parse_time_para(struct device_node *np, struct direct_charge_turbo_dev *di)
{
	int len;

	len = power_dts_read_string_array(power_dts_tag(HWLOG_TAG), np, "time_para",
		(int *)di->time_para, DC_TIME_PARA_LEVEL, DC_TIME_INFO_MAX);
	if (len < 0)
		return;

	di->time_para_parse_ok = true;
}

static void direct_charge_turbo_parse_dts(struct device_node *np, struct direct_charge_turbo_dev *di)
{
	direct_charge_turbo_parse_time_para(np, di);
}

static int direct_charge_turbo_probe(struct platform_device *pdev)
{
	struct direct_charge_turbo_dev *l_dev = NULL;
	struct device_node *np = NULL;

	if (!pdev || !pdev->dev.of_node)
		return -ENODEV;

	hwlog_info("%s start\n", __func__);
	l_dev = kzalloc(sizeof(*l_dev), GFP_KERNEL);
	if (!l_dev)
		return -ENOMEM;

	l_dev->dev = &pdev->dev;
	np = l_dev->dev->of_node;

	direct_charge_turbo_parse_dts(np, l_dev);
	direct_charge_turbo_sysfs_create_group(l_dev->dev);
	g_direct_charge_turbo_dev = l_dev;
	platform_set_drvdata(pdev, l_dev);

	hwlog_info("%s ok\n", __func__);
	return 0;
}

static int direct_charge_turbo_remove(struct platform_device *pdev)
{
	struct direct_charge_turbo_dev *l_dev = platform_get_drvdata(pdev);

	if (!l_dev)
		return -ENODEV;

	direct_charge_turbo_sysfs_remove_group(l_dev->dev);
	platform_set_drvdata(pdev, NULL);
	kfree(l_dev);
	g_direct_charge_turbo_dev = NULL;
	return 0;
}

static const struct of_device_id direct_charge_turbo_match_table[] = {
	{
		.compatible = "huawei,direct_charge_turbo",
		.data = NULL,
	},
	{},
};

static struct platform_driver direct_charge_turbo_driver = {
	.probe = direct_charge_turbo_probe,
	.remove = direct_charge_turbo_remove,
	.driver = {
		.name = "huawei,direct_charge_turbo",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(direct_charge_turbo_match_table),
	},
};

static int __init direct_charge_turbo_init(void)
{
	return platform_driver_register(&direct_charge_turbo_driver);
}

static void __exit direct_charge_turbo_exit(void)
{
	platform_driver_unregister(&direct_charge_turbo_driver);
}

late_initcall(direct_charge_turbo_init);
module_exit(direct_charge_turbo_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("direct charge turbo driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
