/* SPDX-License-Identifier: GPL-2.0 */
/*
 * charger_channel.h
 *
 * charger channel driver
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

#ifndef _CHARGER_CHANNEL_H_
#define _CHARGER_CHANNEL_H_

#define CHARGER_CH_USBIN    0
#define CHARGER_CH_WLSIN    1

#ifdef CONFIG_CHARGER_CHANNEL
void charger_select_channel(int channel);
#else
static inline void charger_select_channel(int channel)
{
}
#endif /* CONFIG_CHARGER_CHANNEL */

#endif /* _CHARGER_CHANNEL_H_ */
