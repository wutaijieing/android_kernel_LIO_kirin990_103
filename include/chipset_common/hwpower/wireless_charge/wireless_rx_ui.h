/* SPDX-License-Identifier: GPL-2.0 */
/*
 * wireless_rx_ui.h
 *
 * ui handler for wireless charge
 *
 * Copyright (c) 2020-2022 Huawei Technologies Co., Ltd.
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

#ifndef _WIRELESS_RX_UI_H_
#define _WIRELESS_RX_UI_H_

#include <linux/errno.h>

struct device;

struct wlrx_ui_para {
	int ui_max_pwr;
	int product_max_pwr;
	int tx_max_pwr;
};

enum wlrx_ui_icon_type {
	WLRX_UI_CHARGE_BEGIN = 0,
	WLRX_UI_NORMAL_CHARGE = WLRX_UI_CHARGE_BEGIN,
	WLRX_UI_FAST_CHARGE,
	WLRX_UI_SUPER_CHARGE,
	WLRX_UI_CHARGE_END,
};

#ifdef CONFIG_WIRELESS_CHARGER
enum wlrx_ui_icon_type wlrx_ui_get_icon_type(unsigned int drv_type);
void wlrx_ui_reset_icon_type(unsigned int drv_type);
void wlrx_ui_reset_icon_flag(unsigned int drv_type, int icon_type);
void wlrx_ui_send_charge_uevent(unsigned int drv_type, unsigned int icon_type);
int wlrx_ui_init(unsigned int drv_type, struct device *dev);
void wlrx_ui_deinit(unsigned int drv_type);
#else
static inline enum wlrx_ui_icon_type wlrx_ui_get_icon_type(unsigned int drv_type)
{
	return WLRX_UI_NORMAL_CHARGE;
}

static inline void wlrx_ui_reset_icon_type(unsigned int drv_type)
{
}

static inline void wlrx_ui_reset_icon_flag(unsigned int drv_type, int icon_type)
{
}

static inline void wlrx_ui_send_charge_uevent(unsigned int drv_type, unsigned int icon_type)
{
}

static inline int wlrx_ui_init(unsigned int drv_type, struct device *dev)
{
	return -ENOTSUPP;
}

static inline void wlrx_ui_deinit(unsigned int drv_type)
{
}
#endif /* CONFIG_WIRELESS_CHARGER */

#endif /* _WIRELESS_RX_UI_H_ */
