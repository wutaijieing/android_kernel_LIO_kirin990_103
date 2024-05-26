/* SPDX-License-Identifier: GPL-2.0 */
/*
 * direct_charge_check.h
 *
 * check for direct charge
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

#ifndef _DIRECT_CHARGE_CHECK_H_
#define _DIRECT_CHARGE_CHECK_H_

#ifdef CONFIG_DIRECT_CHARGER
bool direct_charge_get_can_enter_status(void);
bool direct_charge_check_enable_status(void);
bool direct_charge_in_mode_check(void);
void direct_charge_set_can_enter_status(bool status);
void direct_charge_check(int hv_flag);
int direct_charge_pre_check(void);
bool direct_charge_check_switch_sc4_to_sc_mode(void);
bool direct_charge_check_charge_done(void);
#else
static inline bool direct_charge_get_can_enter_status(void)
{
	return false;
}

static inline bool direct_charge_check_enable_status(void)
{
	return true;
}

static inline bool direct_charge_in_mode_check(void)
{
	return false;
}

static inline void direct_charge_set_can_enter_status(bool status)
{
}

static inline void direct_charge_check(int hv_flag)
{
}

static inline int direct_charge_pre_check(void)
{
	return -EINVAL;
}

static inline bool direct_charge_check_switch_sc4_to_sc_mode(void)
{
	return false;
}

static inline bool direct_charge_check_charge_done(void)
{
	return true;
}
#endif /* CONFIG_DIRECT_CHARGER */

#endif /* _DIRECT_CHARGE_CHECK_H_ */
