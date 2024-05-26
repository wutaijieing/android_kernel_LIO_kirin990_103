/* SPDX-License-Identifier: GPL-2.0 */
/*
 * power_supply_application.h
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

#ifndef _POWER_SUPPLY_APPLICATION_
#define _POWER_SUPPLY_APPLICATION_

#include <chipset_common/hwpower/common_module/power_supply.h>

#ifdef CONFIG_HUAWEI_POWER_SUPPLY_APPLICATION
const char *power_supply_app_get_bat_brand(void);
int power_supply_app_get_bat_capacity(void);
int power_supply_app_get_bat_temp(void);
int power_supply_app_get_bat_status(void);
int power_supply_app_get_bat_current_now(void);
int power_supply_app_get_bat_voltage_now(void);
int power_supply_app_get_bat_voltage_max(void);
int power_supply_app_get_usb_voltage_now(void);
#else
static inline const char *power_supply_app_get_bat_brand(void)
{
	return POWER_SUPPLY_DEFAULT_BRAND;
}

static inline int power_supply_app_get_bat_capacity(void)
{
	return POWER_SUPPLY_DEFAULT_CAPACITY;
}

static inline int power_supply_app_get_bat_temp(void)
{
	return POWER_SUPPLY_DEFAULT_TEMP / 10; /* 10: convert temperature unit */
}

static inline int power_supply_app_get_bat_status(void)
{
	return POWER_SUPPLY_DEFAULT_STATUS;
}

static inline int power_supply_app_get_bat_current_now(void)
{
	return POWER_SUPPLY_DEFAULT_CURRENT_NOW;
}

static inline int power_supply_app_get_bat_voltage_now(void)
{
	return POWER_SUPPLY_DEFAULT_VOLTAGE_NOW;
}

static inline int power_supply_app_get_bat_voltage_max(void)
{
	return POWER_SUPPLY_DEFAULT_VOLTAGE_MAX;
}

static inline int power_supply_app_get_usb_voltage_now(void)
{
	return 0;
}
#endif /* CONFIG_HUAWEI_POWER_SUPPLY_APPLICATION */

#endif /* _POWER_SUPPLY_APPLICATION_ */
