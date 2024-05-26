// SPDX-License-Identifier: GPL-2.0
/*
 * charger_common_interface.c
 *
 * common interface for charger module
 *
 * Copyright (C) 2021-2021 Huawei Technologies Co., Ltd.
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

#include <platform_include/basicplatform/linux/power/platform/coul/coul_drv.h>
#include <chipset_common/hwpower/charger/charger_common_interface.h>
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG charger_common
HWLOG_REGIST();

static struct charge_switch_ops *g_charge_switch_ops;
static int g_charge_online_flag;
static unsigned int g_charge_charger_type = CHARGER_REMOVED;
static unsigned int g_charge_charger_source = POWER_SUPPLY_TYPE_UNKNOWN;
static unsigned int g_charge_reset_adapter_source;
static bool g_charge_ignore_plug_event;

int charge_switch_ops_register(struct charge_switch_ops *ops)
{
	if (ops) {
		g_charge_switch_ops = ops;
		return 0;
	}

	hwlog_err("charge switch ops register fail\n");
	return -EINVAL;
}

int charge_switch_get_charger_type(void)
{
	if (!g_charge_switch_ops || !g_charge_switch_ops->get_charger_type) {
		hwlog_err("g_charge_switch_ops or get_charger_type is null\n");
		return -EINVAL;
	}

	return g_charge_switch_ops->get_charger_type();
}

int charge_set_buck_iterm(unsigned int value)
{
	return 0;
}

int charge_set_buck_fv_delta(unsigned int value)
{
	return 0;
}

int charge_get_battery_current_avg(void)
{
	return coul_drv_battery_current_avg();
}

int charge_get_charger_online(void)
{
	return g_charge_online_flag;
}

void charge_set_charger_online(int online)
{
	g_charge_online_flag = online;
	hwlog_info("set_charger_online: online=%d\n", online);
}

unsigned int charge_get_charger_type(void)
{
	return g_charge_charger_type;
}

void charge_set_charger_type(unsigned int type)
{
	g_charge_charger_type = type;
	hwlog_info("set_charger_type: type=%u\n", type);
}

unsigned int charge_get_charger_source(void)
{
	return g_charge_charger_source;
}

void charge_set_charger_source(unsigned int source)
{
	g_charge_charger_source = source;
	hwlog_info("set_charger_source: source=%u\n", source);
}

unsigned int charge_get_reset_adapter_source(void)
{
	return g_charge_reset_adapter_source;
}

void charge_set_reset_adapter_source(unsigned int mode, unsigned int value)
{
	switch (mode) {
	case RESET_ADAPTER_DIRECT_MODE:
		g_charge_reset_adapter_source = value;
		break;
	case RESET_ADAPTER_SET_MODE:
		if (value >= RESET_ADAPTER_SOURCE_END) {
			hwlog_err("invalid source=%u\n", value);
			return;
		}
		g_charge_reset_adapter_source |= (unsigned int)(1 << value);
		break;
	case RESET_ADAPTER_CLEAR_MODE:
		if (value >= RESET_ADAPTER_SOURCE_END) {
			hwlog_err("invalid source=%u\n", value);
			return;
		}
		g_charge_reset_adapter_source &= (unsigned int)(~(1 << value));
		break;
	default:
		hwlog_err("invalid mode=%u\n", mode);
		return;
	}

	hwlog_info("set_reset_adapter_source: mode=%u value=%u source=%u\n",
		mode, value, g_charge_reset_adapter_source);
}

bool charge_need_ignore_plug_event(void)
{
	if (g_charge_ignore_plug_event)
		hwlog_info("need ignore plug event\n");

	return g_charge_ignore_plug_event == true;
}

void charge_ignore_plug_event(bool state)
{
	g_charge_ignore_plug_event = state;
	hwlog_info("ignore_plug_event: %s plug event\n", (state == true) ? "ignore" : "restore");
}

void charge_update_charger_remove_type(void)
{
	hwlog_info("update charger_remove type\n");
	charge_set_charger_type(CHARGER_REMOVED);
	charge_set_charger_source(POWER_SUPPLY_TYPE_BATTERY);
}

void charge_update_buck_iin_thermal(void)
{
}

void charge_set_usbpd_disable(bool flag)
{
}
