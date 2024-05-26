/* SPDX-License-Identifier: GPL-2.0 */
/*
 * wireless_rx_plim.h
 *
 * power limit for wireless charging
 *
 * Copyright (c) 2020-2021 Huawei Technologies Co., Ltd.
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

#ifndef _WIRELESS_RX_PLIM_H_
#define _WIRELESS_RX_PLIM_H_

#include <chipset_common/hwpower/wireless_charge/wireless_trx_intf.h>

enum wlrx_plim_src {
	WLRX_PLIM_SRC_BEGIN = 0,
	WLRX_PLIM_SRC_OTG = WLRX_PLIM_SRC_BEGIN,
	WLRX_PLIM_SRC_RPP,
	WLRX_PLIM_SRC_FAN,
	WLRX_PLIM_SRC_VOUT_ERR,
	WLRX_PLIM_SRC_TX_BST_ERR,
	WLRX_PLIM_SRC_KB,
	WLRX_PLIM_SRC_THERMAL,
	WLRX_PLIM_SRC_ANA_HS,
	WLRX_PLIM_SRC_END,
};

struct wlrx_pctrl;

#ifdef CONFIG_WIRELESS_CHARGER
unsigned int wlrx_plim_get_src(unsigned int drv_type);
void wlrx_plim_set_src(unsigned int drv_type, unsigned int src_id);
void wlrx_plim_clear_src(unsigned int drv_type, unsigned int src_id);
void wlrx_plim_reset_src(unsigned int drv_type);
void wlrx_plim_update_pctrl(unsigned int drv_type, struct wlrx_pctrl *pctrl);
#else
static inline int wlrx_plim_get_src(unsigned int drv_type)
{
	return 0;
}

static inline void wlrx_plim_set_src(unsigned int drv_type, unsigned int src_id)
{
}

static inline void wlrx_plim_clear_src(unsigned int drv_type, unsigned int src_id)
{
}

static inline void wlrx_plim_reset_src(unsigned int drv_type)
{
}

static inline void wlrx_plim_update_pctrl(unsigned int drv_type, struct wlrx_pctrl *pctrl)
{
}
#endif /* CONFIG_WIRELESS_CHARGER */

#endif /* _WIRELESS_RX_PLIM_H_ */
