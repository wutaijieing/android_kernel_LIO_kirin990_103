/* SPDX-License-Identifier: GPL-2.0 */
/*
 * wireless_rx_event.h
 *
 * charge event for wireless charging
 *
 * Copyright (c) 2022-2022 Huawei Technologies Co., Ltd.
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

#ifndef _WIRELESS_RX_EVENT_H_
#define _WIRELESS_RX_EVENT_H_

#ifdef CONFIG_WIRELESS_CHARGER
void wlrx_evt_clear_err_cnt(unsigned int drv_type);
#else
static inline void wlrx_evt_clear_err_cnt(unsigned int drv_type)
{
}
#endif /* CONFIG_WIRELESS_CHARGER */

#endif /* _WIRELESS_RX_EVENT_H_ */
