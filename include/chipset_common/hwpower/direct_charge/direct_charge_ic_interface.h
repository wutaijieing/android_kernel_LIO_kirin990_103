/* SPDX-License-Identifier: GPL-2.0 */
/*
 * direct_charge_ic_interface.h
 *
 * common interface, variables, definition etc for direct charge module
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

#ifndef _DIRECT_CHARGE_IC_INTERFACE_H_
#define _DIRECT_CHARGE_IC_INTERFACE_H_

#include <linux/errno.h>

#ifdef CONFIG_DIRECT_CHARGER
int dc_init_ic(int mode, unsigned int index);
int dc_exit_ic(int mode, unsigned int index);
int dc_enable_ic(int mode, unsigned int index, int enable);
int dc_enable_irq(int mode, unsigned int index, int enable);
int dc_enable_ic_adc(int mode, unsigned int index, int enable);
int dc_discharge_ic(int mode, unsigned int index, int enable);
int dc_is_ic_close(int mode, unsigned int index);
bool dc_is_ic_support_prepare(int mode, unsigned int index);
int dc_prepare_enable_ic(int mode, unsigned int index);
int dc_config_ic_watchdog(int mode, unsigned int index, int time);
int dc_kick_ic_watchdog(int mode, unsigned int index);
int dc_get_ic_id(int mode, unsigned int index);
int dc_get_ic_status(int mode, unsigned int index);
int dc_set_ic_buck_enable(int mode, unsigned int index, int enable);
int dc_reset_and_init_ic_reg(int mode, unsigned int index);
int dc_get_ic_freq(int mode, unsigned int index);
int dc_set_ic_freq(int mode, unsigned int index, int freq);
const char *dc_get_ic_name(int mode, unsigned int index);
int dc_init_batinfo(int mode, unsigned int index);
int dc_exit_batinfo(int mode, unsigned int index);
int dc_get_ic_vbtb(unsigned int index);
int dc_get_ic_vpack(unsigned int index);
int dc_get_ic_vbus(unsigned int index, int *vbus);
int dc_get_ic_ibat(unsigned int index, int *ibat);
int dc_get_ic_ibus(unsigned int index, int *ibus);
int dc_get_ic_temp(unsigned int index, int *temp);
int dc_get_ic_vusb(unsigned int index, int *vusb);
int dc_get_ic_vout(unsigned int index, int *vout);
int dc_set_ic_threshold_enable(int mode, unsigned int index, int enable);
#else
static inline int dc_init_ic(int mode, unsigned int index)
{
	return -EPERM;
}

static inline int dc_exit_ic(int mode, unsigned int index)
{
	return -EPERM;
}

static inline int dc_enable_ic(int mode, unsigned int index, int enable)
{
	return -EPERM;
}

static inline int dc_enable_irq(int mode, unsigned int index, int enable)
{
	return -EPERM;
}

static inline int dc_enable_ic_adc(int mode, unsigned int index, int enable)
{
	return -EPERM;
}

static inline int dc_discharge_ic(int mode, unsigned int index, int enable)
{
	return -EPERM;
}

static inline int dc_is_ic_close(int mode, unsigned int index)
{
	return -EPERM;
}

static inline bool dc_is_ic_support_prepare(int mode, unsigned int index)
{
	return false;
}

static inline int dc_prepare_enable_ic(int mode, unsigned int index)
{
	return -EPERM;
}

static inline int dc_config_ic_watchdog(int mode, unsigned int index, int time)
{
	return -EPERM;
}

static inline int dc_kick_ic_watchdog(int mode, unsigned int index)
{
	return -EPERM;
}

static inline int dc_get_ic_id(int mode, unsigned int index)
{
	return -EPERM;
}

static inline int dc_get_ic_status(int mode, unsigned int index)
{
	return -EPERM;
}

static inline int dc_set_ic_buck_enable(int mode, unsigned int index, int enable)
{
	return -EPERM;
}

static inline int dc_reset_and_init_ic_reg(int mode, unsigned int index)
{
	return -EPERM;
}

static inline int dc_get_ic_freq(int mode, unsigned int index)
{
	return -EPERM;
}

static inline int dc_set_ic_freq(int mode, unsigned int index, int freq)
{
	return -EPERM;
}

static inline const char *dc_get_ic_name(int mode, unsigned int index)
{
	return "invalid ic";
}

static inline int dc_init_batinfo(int mode, unsigned int index)
{
	return -EPERM;
}

static inline int dc_exit_batinfo(int mode, unsigned int index)
{
	return -EPERM;
}

static inline int dc_get_ic_vbtb(unsigned int index)
{
	return -EPERM;
}

static inline int dc_get_ic_vpack(unsigned int index)
{
	return -EPERM;
}

static inline int dc_get_ic_vbus(unsigned int index, int *vbus)
{
	return -EPERM;
}

static inline int dc_get_ic_ibat(unsigned int index, int *ibat)
{
	return -EPERM;
}

static inline int dc_get_ic_ibus(unsigned int index, int *ibus)
{
	return -EPERM;
}

static inline int dc_get_ic_temp(unsigned int index, int *temp)
{
	return -EPERM;
}

static inline int dc_get_ic_vusb(unsigned int index, int *vusb)
{
	return -EPERM;
}

static inline int dc_get_ic_vout(unsigned int index, int *vout)
{
	return -EPERM;
}

static inline int dc_set_ic_threshold_enable(int mode, unsigned int index, int enable)
{
	return -EPERM;
}
#endif /* CONFIG_DIRECT_CHARGER */

#endif /* _DIRECT_CHARGE_IC_INTERFACE_H_ */
