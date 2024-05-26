/* SPDX-License-Identifier: GPL-2.0 */
/*
 * wireless_rx_monitor.h
 *
 * monitor info for wireless charging
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

#ifndef _WIRELESS_RX_MONITOR_H_
#define _WIRELESS_RX_MONITOR_H_

#define WLRX_MON_INTERVAL            100 /* 100ms */

#ifdef CONFIG_WIRELESS_CHARGER
int wlrx_get_monitor_interval(void);
void wlrx_mon_show_info(unsigned int drv_type);
#else
static inline int wlrx_get_monitor_interval(void)
{
	return WLRX_MON_INTERVAL;
}

static inline void wlrx_mon_show_info(unsigned int drv_type)
{
}
#endif /* CONFIG_WIRELESS_CHARGER */

#endif /* _WIRELESS_RX_MONITOR_H_ */
