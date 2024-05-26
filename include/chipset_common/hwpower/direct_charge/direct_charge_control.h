// SPDX-License-Identifier: GPL-2.0
/*
 * direct_charge_control.h
 *
 * control module for direct charge
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

#ifndef _DIRECT_CHARGE_CONTROL_H_
#define _DIRECT_CHARGE_CONTROL_H_

#ifdef CONFIG_DIRECT_CHARGER
void dc_preparation_before_switch_to_singlepath(int working_mode, int ratio, int vdelt);
void dc_preparation_before_stop(void);
#else
static inline void dc_preparation_before_switch_to_singlepath(int working_mode, int ratio, int vdelt)
{
}

static inline void dc_preparation_before_stop(void)
{
}
#endif /* CONFIG_DIRECT_CHARGER */

#endif /* _DIRECT_CHARGE_CONTROL_H_ */
