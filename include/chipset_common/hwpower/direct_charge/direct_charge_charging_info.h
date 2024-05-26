// SPDX-License-Identifier: GPL-2.0
/*
 * direct_charge_charging_info.h
 *
 * charging info for direct charge module
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

#ifndef _DIRECT_CHARGE_CHARGING_INFO_H_
#define _DIRECT_CHARGE_CHARGING_INFO_H_

#include <chipset_common/hwpower/direct_charge/direct_charge_ic.h>

#define COUL_IC_NUM        2

struct dc_charge_info {
	int succ_flag;
	const char *ic_name[CHARGE_IC_MAX_NUM];
	int channel_num;
	int ibat_max;
	int ibus[CHARGE_IC_MAX_NUM];
	int ibat[CHARGE_IC_MAX_NUM];
	int vbat[CHARGE_IC_MAX_NUM];
	int vout[CHARGE_IC_MAX_NUM];
	int tbat[CHARGE_IC_MAX_NUM];
	int coul_check_flag;
	int coul_ibat_max;
	int coul_vbat_max;
	int coul_ibat[COUL_IC_NUM];
	int coul_vbat[COUL_IC_NUM];
};

#ifdef CONFIG_DIRECT_CHARGER
void dc_update_charging_info(int mode, struct dc_charge_info *info, int *vbat_comp);
void dc_show_realtime_charging_info(int mode, unsigned int path);
int dc_get_gain_ibus(void);
void dc_set_stop_charging_flag(bool flag);
int dc_get_stop_charging_flag(void);
#else
static inline void dc_update_charging_info(int mode, struct dc_charge_info *info, int *vbat_comp)
{
}

static inline void dc_show_realtime_charging_info(int mode, unsigned int path)
{
}

static inline int dc_get_gain_ibus(void)
{
	return 0;
}

static inline void dc_set_stop_charging_flag(bool flag)
{
}

static inline int dc_get_stop_charging_flag(void)
{
	return -EINVAL;
}
#endif /* CONFIG_DIRECT_CHARGER */

#endif /* _DIRECT_CHARGE_CHARGING_INFO_H_ */
