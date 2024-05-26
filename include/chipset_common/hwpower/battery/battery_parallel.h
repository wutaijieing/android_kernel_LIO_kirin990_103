/* SPDX-License-Identifier: GPL-2.0 */
/*
 * battery_parallel.h
 *
 * huawei parallel battery driver interface
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

#ifndef _BATTERY_PARALLEL_H_
#define _BATTERY_PARALLEL_H_

#define BAT_PARALLEL_COUNT       2

struct bat_cccv_node {
	int vol;
	int cur;
};

struct bat_balance_info {
	int temp;
	int vol;
	int cur;
};

struct bat_balance_cur {
	int total_cur;
	struct bat_cccv_node cccv[BAT_PARALLEL_COUNT];
};

#ifdef CONFIG_HUAWEI_BATTERY_1S2P
int bat_balance_get_cur_info(struct bat_balance_info *info,
	u32 info_len, struct bat_balance_cur *result);
#else
static int bat_balance_get_cur_info(struct bat_balance_info *info,
	u32 info_len, struct bat_balance_cur *result)
{
	return -ENODEV;
}
#endif /* CONFIG_HUAWEI_BATTERY_1S2P */

#endif /* _BATTERY_PARALLEL_H_ */
