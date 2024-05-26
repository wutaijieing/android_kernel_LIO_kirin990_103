/* SPDX-License-Identifier: GPL-2.0 */
/*
 * wireless_rx_intf.h
 *
 * common rx interface, variables, definition etc for wireless_rx_intf
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

#ifndef _WIRELESS_RX_INTF_H_
#define _WIRELESS_RX_INTF_H_

#include <linux/types.h>

#ifdef CONFIG_WIRELESS_CHARGER
bool wlrx_support_wireless_charging(unsigned int drv_type);
#else
static inline bool wlrx_support_wireless_charging(unsigned int drv_type)
{
	return false;
}
#endif /* CONFIG_WIRELESS_CHARGER */

#endif /* _WIRELESS_RX_INTF_H_ */
