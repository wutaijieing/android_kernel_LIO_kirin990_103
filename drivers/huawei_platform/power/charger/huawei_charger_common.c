/*
 * huawei_charger_common.c
 *
 * common interface for huawei charger driver
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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

#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/power/huawei_charger.h>
#include <huawei_platform/power/huawei_charger_common.h>
#include <huawei_platform/hwpower/common_module/power_platform.h>

#define HWLOG_TAG huawei_charger_common
HWLOG_REGIST();

static struct charge_extra_ops *g_extra_ops;
struct charge_device_ops *g_ops;

int charge_extra_ops_register(struct charge_extra_ops *ops)
{
	int ret = 0;

	if (ops) {
		g_extra_ops = ops;
		hwlog_info("charge extra ops register ok\n");
	} else {
		hwlog_err("charge extra ops register fail\n");
		ret = -EPERM;
	}

	return ret;
}

int charge_ops_register(struct charge_device_ops *ops)
{
	int ret = 0;

	if (ops) {
		g_ops = ops;
		hwlog_info("charge ops register ok\n");
	} else {
		hwlog_err("charge ops register fail\n");
		ret = -EPERM;
	}

	return ret;
}

int charge_init_chip(struct charge_init_data *data)
{
	int ret;

	if (!g_ops || !g_ops->chip_init)
		return -EINVAL;

	ret = g_ops->chip_init(data);
	hwlog_info("init_chip: charger_type=%u vbus=%d ret=%d\n",
		data->charger_type, data->vbus, ret);
	return ret;
}

int charge_set_vbus_vset(u32 volt)
{
	int ret;

	if (!g_ops || !g_ops->set_vbus_vset)
		return -EINVAL;

	ret = g_ops->set_vbus_vset(volt);
	hwlog_info("set_vbus_vset: volt=%u ret=%d\n", volt, ret);
	return ret;
}

int charge_set_mivr(u32 volt)
{
	int ret;

	if (!g_ops || !g_ops->set_mivr)
		return -EINVAL;

	ret = g_ops->set_mivr(volt);
	hwlog_info("set_mivr: volt=%u ret=%d\n", volt, ret);
	return ret;
}

int charge_set_batfet_disable(int val)
{
	int ret;

	if (!g_ops || !g_ops->set_batfet_disable)
		return -EINVAL;

	ret = g_ops->set_batfet_disable(val);
	hwlog_info("set_batfet_disable: val=%d ret=%d\n", val, ret);
	return ret;
}

int charge_set_watchdog(int time)
{
	int ret;

	if (!g_ops || !g_ops->set_watchdog_timer)
		return -EINVAL;

	ret = g_ops->set_watchdog_timer(time);
	hwlog_info("set_watchdog: time=%d ret=%d\n", time, ret);
	return ret;
}

int charge_reset_watchdog(void)
{
	int ret;

	if (!g_ops || !g_ops->reset_watchdog_timer)
		return -EINVAL;

	ret = g_ops->reset_watchdog_timer();
	hwlog_info("reset_watchdog: ret=%d\n", ret);
	return ret;
}

void charge_kick_watchdog(void)
{
	int ret = charge_reset_watchdog();

	if (ret)
		hwlog_err("kick watchdog timer fail\n");
	else
		hwlog_info("kick watchdog timer ok\n");
	power_platform_charge_feed_sys_wdt(CHARGE_SYS_WDT_TIMEOUT);
}

void charge_disable_watchdog(void)
{
	int ret = charge_set_watchdog(CHAGRE_WDT_DISABLE);

	if (ret)
		hwlog_err("disable watchdog timer fail\n");
	else
		hwlog_info("disable watchdog timer ok\n");
	power_platform_charge_stop_sys_wdt();
}

int charge_get_vusb(void)
{
	int vusb_vol = 0;

	if (!g_ops || !g_ops->get_vusb) {
		hwlog_err("g_charge_ops or get_vusb is null\n");
		return -EINVAL;
	}

	if (g_ops->get_vusb(&vusb_vol))
		return -EINVAL;

	return vusb_vol;
}

int charge_get_vbus(void)
{
	int ret;
	unsigned int vbus = 0;

	if (!g_ops || !g_ops->get_vbus) {
		hwlog_err("g_charge_ops or get_vbus is null\n");
		return 0;
	}

	ret = g_ops->get_vbus(&vbus);
	hwlog_info("get_vbus: vbus=%u ret=%d\n", vbus, ret);
	return vbus;
}

int charge_get_ibus(void)
{
	if (!g_ops || !g_ops->get_ibus) {
		hwlog_err("g_charge_ops or get_ibus is null\n");
		return -1;
	}

	return g_ops->get_ibus();
}

int charge_get_iin_set(void)
{
	if (!g_ops || !g_ops->get_iin_set) {
		hwlog_err("g_charge_ops or get_iin_set is null\n");
		return CHARGE_CURRENT_0500_MA;
	}
	return g_ops->get_iin_set();
}

unsigned int charge_get_charging_state(void)
{
	unsigned int state = CHAGRE_STATE_NORMAL;

	if (!g_ops || !g_ops->get_charge_state) {
		hwlog_err("g_charge_ops or get_charge_state is null\n");
		return CHAGRE_STATE_NORMAL;
	}

	g_ops->get_charge_state(&state);
	return state;
}

int charge_set_dev_iin(int iin)
{
	if (!g_ops || !g_ops->set_input_current) {
		hwlog_err("g_charge_ops or set_dev_iin is null\n");
		return -1;
	}

	return g_ops->set_input_current(iin);
}

int charge_check_input_dpm_state(void)
{
	if (!g_ops || !g_ops->check_input_dpm_state) {
		hwlog_err("g_charge_ops or check_input_dpm_state is null\n");
		return -1;
	}

	return g_ops->check_input_dpm_state();
}

int charge_check_charger_plugged(void)
{
	if (!g_ops || !g_ops->check_charger_plugged)
		return -1;

	return g_ops->check_charger_plugged();
}

bool charge_check_charger_ts(void)
{
	if (!g_extra_ops || !g_extra_ops->check_ts) {
		hwlog_err("g_extra_ops or check_ts is null\n");
		return false;
	}

	return g_extra_ops->check_ts();
}

bool charge_check_charger_otg_state(void)
{
	if (!g_extra_ops || !g_extra_ops->check_otg_state) {
		hwlog_err("g_extra_ops or check_otg_state is null\n");
		return false;
	}

	return g_extra_ops->check_otg_state();
}

int charge_set_charge_state(int state)
{
	if (!g_extra_ops || !g_extra_ops->set_state) {
		hwlog_err("g_extra_ops or set_state is null\n");
		return -1;
	}

	return g_extra_ops->set_state(state);
}

int get_charge_current_max(void)
{
	if (!g_extra_ops || !g_extra_ops->get_charge_current) {
		hwlog_err("g_extra_ops or get_charge_current is null\n");
		return 0;
	}

	return g_extra_ops->get_charge_current();
}
