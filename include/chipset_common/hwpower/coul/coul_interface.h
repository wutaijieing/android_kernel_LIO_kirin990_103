/* SPDX-License-Identifier: GPL-2.0 */
/*
 * coul_interface.h
 *
 * interface for coul driver
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

#ifndef _COUL_INTERFACE_H_
#define _COUL_INTERFACE_H_

#include <linux/errno.h>
#include <linux/power_supply.h>

enum coul_interface_type {
	COUL_TYPE_BEGIN = 0,
	COUL_TYPE_MAIN = COUL_TYPE_BEGIN,
	COUL_TYPE_AUX,
	COUL_TYPE_1S2P,
	COUL_TYPE_END,
};

struct coul_interface_ops {
	const char *type_name;
	void *dev_data;
	int (*is_coul_ready)(void *dev_data);
	int (*is_battery_exist)(void *dev_data);
	int (*get_battery_capacity)(void *dev_data);
	int (*get_battery_voltage)(void *dev_data);
	int (*get_battery_current)(void *dev_data);
	int (*get_battery_avg_current)(void *dev_data);
	int (*get_battery_temperature)(void *dev_data);
	int (*get_battery_cycle)(void *dev_data);
	int (*get_battery_fcc)(void *dev_data);
	int (*set_vterm_dec)(int vterm_dec, void *dev_data);
	int (*set_battery_low_voltage)(int val, void *dev_data);
	int (*set_battery_last_capacity)(int capacity, void *dev_data);
	int (*get_battery_last_capacity)(void *dev_data);
	int (*get_battery_rm)(void *dev_data);
};

struct coul_interface_dev {
	unsigned int total_ops;
	struct coul_interface_ops *p_ops[COUL_TYPE_END];
};

#ifdef CONFIG_HUAWEI_COUL
int coul_interface_ops_register(struct coul_interface_ops *ops);
int coul_interface_is_coul_ready(int type);
int coul_interface_is_battery_exist(int type);
int coul_interface_get_battery_capacity(int type);
int coul_interface_get_battery_rm(int type);
int coul_interface_get_battery_last_capacity(int type);
int coul_interface_set_battery_last_capacity(int type, int capacity);
int coul_interface_get_battery_voltage(int type);
int coul_interface_get_battery_current(int type);
int coul_interface_get_battery_avg_current(int type);
int coul_interface_get_battery_temperature(int type);
int coul_interface_get_battery_cycle(int type);
int coul_interface_get_battery_fcc(int type);
int coul_interface_set_vterm_dec(int type, int vterm_dec);
int coul_interface_set_battery_low_voltage(int type, int val);
#else
static inline int coul_interface_ops_register(struct coul_interface_ops *ops)
{
	return -EPERM;
}

static inline int coul_interface_is_coul_ready(int type)
{
	return 0;
}

static inline int coul_interface_is_battery_exist(int type)
{
	return 0;
}

static inline int coul_interface_get_battery_capacity(int type)
{
	return -EPERM;
}

static inline int coul_interface_get_battery_rm(int type)
{
	return -EPERM;
}

static inline int coul_interface_get_battery_last_capacity(int type)
{
	return -EPERM;
}

static inline int coul_interface_set_battery_last_capacity(int type, int capacity)
{
	return -EPERM;
}

static inline int coul_interface_get_battery_voltage(int type)
{
	return -EPERM;
}

static inline int coul_interface_get_battery_current(int type)
{
	return -EPERM;
}

static inline int coul_interface_get_battery_avg_current(int type)
{
	return -EPERM;
}

static inline int coul_interface_get_battery_temperature(int type)
{
	return -EPERM;
}

static inline int coul_interface_get_battery_cycle(int type)
{
	return -EPERM;
}

static inline int coul_interface_get_battery_fcc(int type)
{
	return -EPERM;
}

static inline int coul_interface_set_vterm_dec(int type, int vterm_dec)
{
	return 0;
}

static inline int coul_interface_set_battery_low_voltage(int type, int val)
{
	return -EPERM;
}
#endif /* CONFIG_HUAWEI_COUL */

#endif /* _COUL_INTERFACE_H_ */
