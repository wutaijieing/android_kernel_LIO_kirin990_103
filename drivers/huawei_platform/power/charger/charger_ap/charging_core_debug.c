// SPDX-License-Identifier: GPL-2.0
/*
 * charging_core_debug.c
 *
 * debug for charging core module
 *
 * Copyright (c) 2019-2020 Huawei Technologies Co., Ltd.
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
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/raid/pq.h>
#include <chipset_common/hwpower/common_module/power_debug.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include "charging_core.h"

#define HWLOG_TAG charging_core_debug
HWLOG_REGIST();

static ssize_t charging_core_dbg_iterm_store(void *dev_data,
	const char *buf, size_t size)
{
	int iterm = 0;
	int ret;
	struct charge_core_info *di = dev_data;

	if (!di)
		return -EINVAL;

	ret = kstrtoint(buf, 0, &iterm);
	if (ret) {
		hwlog_err("unable to parse input:%s\n", buf);
		return -EINVAL;
	}

	di->data.iterm = iterm;

	return size;
}

static ssize_t charging_core_dbg_iterm_show(void *dev_data,
	char *buf, size_t size)
{
	struct charge_core_info *di = dev_data;

	if (!di)
		return scnprintf(buf, size, "not support\n");

	return scnprintf(buf, size, "iterm is %d\n", di->data.iterm);
}

static ssize_t charging_core_dbg_ichg_ac_store(void *dev_data,
	const char *buf, size_t size)
{
	int ichg = 0;
	int ret;
	struct charge_core_info *di = dev_data;

	if (!di)
		return -EINVAL;

	ret = kstrtoint(buf, 0, &ichg);
	if (ret) {
		hwlog_err("unable to parse input:%s\n", buf);
		return -EINVAL;
	}

	di->data.ichg_ac = ichg;

	return size;
}

static ssize_t charging_core_dbg_ichg_ac_show(void *dev_data,
	char *buf, size_t size)
{
	struct charge_core_info *di = dev_data;

	if (!di)
		return scnprintf(buf, size, "not support\n");

	return scnprintf(buf, size, "ichg_ac is %d\n", di->data.ichg_ac);
}

static ssize_t charging_core_dbg_iin_ac_store(void *dev_data,
	const char *buf, size_t size)
{
	int iin = 0;
	int ret;
	struct charge_core_info *di = dev_data;

	if (!di)
		return -EINVAL;

	ret = kstrtoint(buf, 0, &iin);
	if (ret) {
		hwlog_err("unable to parse input:%s\n", buf);
		return -EINVAL;
	}

	di->data.iin_ac = iin;

	return size;
}

static ssize_t charging_core_dbg_iin_ac_show(void *dev_data,
	char *buf, size_t size)
{
	struct charge_core_info *di = dev_data;

	if (!di)
		return scnprintf(buf, size, "not support\n");

	return scnprintf(buf, size, "iin_ac is %d\n", di->data.iin_ac);
}

static ssize_t charging_core_dbg_ichg_fcp_store(void *dev_data,
	const char *buf, size_t size)
{
	int ichg_fcp = 0;
	int ret;
	struct charge_core_info *di = dev_data;

	if (!di)
		return -EINVAL;

	ret = kstrtoint(buf, 0, &ichg_fcp);
	if (ret) {
		hwlog_err("unable to parse input:%s\n", buf);
		return -EINVAL;
	}

	di->data.ichg_fcp = ichg_fcp;

	return size;
}

static ssize_t charging_core_dbg_ichg_fcp_show(void *dev_data,
	char *buf, size_t size)
{
	struct charge_core_info *di = dev_data;

	if (!di)
		return scnprintf(buf, size, "not support\n");

	return scnprintf(buf, size, "ichg_fcp is %d\n", di->data.ichg_fcp);
}

static ssize_t charging_core_dbg_iin_fcp_store(void *dev_data,
	const char *buf, size_t size)
{
	int iin_fcp = 0;
	int ret;
	struct charge_core_info *di = dev_data;

	if (!di)
		return -EINVAL;

	ret = kstrtoint(buf, 0, &iin_fcp);
	if (ret) {
		hwlog_err("unable to parse input:%s\n", buf);
		return -EINVAL;
	}

	di->data.iin_fcp = iin_fcp;

	return size;
}

static ssize_t charging_core_dbg_iin_fcp_show(void *dev_data,
	char *buf, size_t size)
{
	struct charge_core_info *di = dev_data;

	if (!di)
		return scnprintf(buf, size, "not support\n");

	return scnprintf(buf, size, "iin_fcp is %d\n", di->data.iin_fcp);
}

void charging_core_dbg_register(struct charge_core_info *di)
{
	power_dbg_ops_register("charging_core", "iterm", (void *)di,
		charging_core_dbg_iterm_show,
		charging_core_dbg_iterm_store);
	power_dbg_ops_register("charging_core", "ichg_ac", (void *)di,
		charging_core_dbg_ichg_ac_show,
		charging_core_dbg_ichg_ac_store);
	power_dbg_ops_register("charging_core", "iin_ac", (void *)di,
		charging_core_dbg_iin_ac_show,
		charging_core_dbg_iin_ac_store);
	power_dbg_ops_register("charging_core", "ichg_fcp", (void *)di,
		charging_core_dbg_ichg_fcp_show,
		charging_core_dbg_ichg_fcp_store);
	power_dbg_ops_register("charging_core", "iin_fcp", (void *)di,
		charging_core_dbg_iin_fcp_show,
		charging_core_dbg_iin_fcp_store);
}
