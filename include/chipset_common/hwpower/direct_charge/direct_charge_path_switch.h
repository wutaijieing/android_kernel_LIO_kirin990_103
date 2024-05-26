/* SPDX-License-Identifier: GPL-2.0 */
/*
 * direct_charge_path_switch.h
 *
 * path switch for direct charge
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

#ifndef _DIRECT_CHARGE_PATH_SWITCH_H_
#define _DIRECT_CHARGE_PATH_SWITCH_H_

#include <linux/errno.h>
#include <chipset_common/hwpower/hardware_channel/wired_channel_switch.h>

#ifdef CONFIG_DIRECT_CHARGER
int dc_close_dc_channel(void);
int dc_open_dc_channel(void);
void dc_open_aux_wired_channel(void);
void dc_close_aux_wired_channel(void);
void dc_close_lvc_mos_channel(void);
void dc_select_charge_path(void);
int dc_switch_charging_path(unsigned int path);
#else
static inline int dc_close_dc_channel(void)
{
	return 0;
}

static inline int dc_open_dc_channel(void)
{
	return 0;
}

static inline void dc_open_aux_wired_channel(void)
{
}

static inline void dc_close_aux_wired_channel(void)
{
}

static inline void dc_close_lvc_mos_channel(void)
{
}

static inline void dc_select_charge_path(void)
{
}

static inline int dc_switch_charging_path(unsigned int path)
{
	return -EPERM;
}
#endif /* CONFIG_DIRECT_CHARGER */

#endif /* _DIRECT_CHARGE_PATH_SWITCH_H_ */
