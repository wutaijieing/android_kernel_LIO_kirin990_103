// SPDX-License-Identifier: GPL-2.0
/*
 * adapter_protocol_pd.c
 *
 * pd protocol driver
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
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
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/delay.h>
#include <chipset_common/hwpower/protocol/adapter_protocol.h>
#include <chipset_common/hwpower/protocol/adapter_protocol_pd.h>
#include <chipset_common/hwpower/charger/charger_common_interface.h>
#include <chipset_common/hwpower/common_module/power_common_macro.h>
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG pd_protocol
HWLOG_REGIST();

static struct hwpd_dev *g_hwpd_dev;

static const struct adapter_protocol_device_data g_hwpd_dev_data[] = {
	{ PROTOCOL_DEVICE_ID_SCHARGER_V600, "scharger_v600" },
	{ PROTOCOL_DEVICE_ID_FUSB3601, "fusb3601" },
	{ PROTOCOL_DEVICE_ID_RT1711H, "rt1711h" },
	{ PROTOCOL_DEVICE_ID_FUSB30X, "fusb30x" },
};

static int hwpd_get_device_id(const char *str)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(g_hwpd_dev_data); i++) {
		if (!strncmp(str, g_hwpd_dev_data[i].name, strlen(str)))
			return g_hwpd_dev_data[i].id;
	}

	return -EPERM;
}

static struct hwpd_dev *hwpd_get_dev(void)
{
	if (!g_hwpd_dev) {
		hwlog_err("g_hwpd_dev is null\n");
		return NULL;
	}

	return g_hwpd_dev;
}

static struct hwpd_ops *hwpd_get_ops(void)
{
	if (!g_hwpd_dev || !g_hwpd_dev->p_ops) {
		hwlog_err("g_hwpd_dev or p_ops is null\n");
		return NULL;
	}

	return g_hwpd_dev->p_ops;
}

int hwpd_ops_register(struct hwpd_ops *ops)
{
	int dev_id;

	if (!g_hwpd_dev || !ops || !ops->chip_name) {
		hwlog_err("g_hwpd_dev or ops or chip_name is null\n");
		return -EPERM;
	}

	dev_id = hwpd_get_device_id(ops->chip_name);
	if (dev_id < 0) {
		hwlog_err("%s ops register fail\n", ops->chip_name);
		return -EPERM;
	}

	g_hwpd_dev->p_ops = ops;
	g_hwpd_dev->dev_id = dev_id;

	hwlog_info("%d:%s ops register ok\n", dev_id, ops->chip_name);
	return 0;
}

static int hwpd_get_output_voltage(int *volt)
{
	struct hwpd_dev *l_dev = NULL;

	l_dev = hwpd_get_dev();
	if (!l_dev || !volt)
		return -EPERM;

	*volt = l_dev->volt;
	return 0;
}

static int hwpd_set_output_voltage(int volt)
{
	struct hwpd_ops *l_ops = NULL;

	l_ops = hwpd_get_ops();
	if (!l_ops)
		return -EPERM;

	if (!l_ops->set_output_voltage) {
		hwlog_err("set_output_voltage is null\n");
		return -EPERM;
	}

	hwlog_info("set output voltage: %d\n", volt);
	g_hwpd_dev->volt = volt;
	l_ops->set_output_voltage(volt, l_ops->dev_data);
	return 0;
}

static int hwpd_hard_reset_master(void)
{
	struct hwpd_ops *l_ops = NULL;

	l_ops = hwpd_get_ops();
	if (!l_ops)
		return -EPERM;

	if (!l_ops->hard_reset_master) {
		hwlog_err("hard_reset_master is null\n");
		return -EPERM;
	}

	l_ops->hard_reset_master(l_ops->dev_data);
	return 0;
}

static int hwpd_get_protocol_register_state(void)
{
	struct hwpd_dev *l_dev = NULL;

	l_dev = hwpd_get_dev();
	if (!l_dev)
		return -EPERM;

	if ((l_dev->dev_id >= PROTOCOL_DEVICE_ID_BEGIN) &&
		(l_dev->dev_id < PROTOCOL_DEVICE_ID_END))
		return 0;

	hwlog_info("get_protocol_register_state fail\n");
	return -EPERM;
}

static struct adapter_protocol_ops adapter_protocol_hwpd_ops = {
	.type_name = "hw_pd",
	.hard_reset_master = hwpd_hard_reset_master,
	.set_output_voltage = hwpd_set_output_voltage,
	.get_output_voltage = hwpd_get_output_voltage,
	.get_protocol_register_state = hwpd_get_protocol_register_state,
};

static int __init hwpd_init(void)
{
	int ret;
	struct hwpd_dev *l_dev = NULL;

	l_dev = kzalloc(sizeof(*l_dev), GFP_KERNEL);
	if (!l_dev)
		return -ENOMEM;

	g_hwpd_dev = l_dev;
	l_dev->dev_id = PROTOCOL_DEVICE_ID_END;
	l_dev->volt = ADAPTER_9V * POWER_MV_PER_V;

	ret = adapter_protocol_ops_register(&adapter_protocol_hwpd_ops);
	if (ret)
		goto fail_register_ops;

	return 0;

fail_register_ops:
	kfree(l_dev);
	g_hwpd_dev = NULL;
	return ret;
}

static void __exit hwpd_exit(void)
{
	if (!g_hwpd_dev)
		return;

	kfree(g_hwpd_dev);
	g_hwpd_dev = NULL;
}

subsys_initcall_sync(hwpd_init);
module_exit(hwpd_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("pd protocol driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
