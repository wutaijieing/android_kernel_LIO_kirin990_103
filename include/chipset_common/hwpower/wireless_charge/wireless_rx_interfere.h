/* SPDX-License-Identifier: GPL-2.0 */
/*
 * wireless_rx_interfere.h
 *
 * common interface, variables, definition etc of wireless_rx_interfere.c
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

#ifndef _WIRELESS_RX_INTERFERE_H_
#define _WIRELESS_RX_INTERFERE_H_

#include <chipset_common/hwpower/wireless_charge/wireless_trx_intf.h>

#define VOUT_9V_STAGE_MAX          10500
#define VOUT_9V_STAGE_MIN          7500
#define WLRX_INTFR_DFT_FIXED_FOP   (-1) /* no need fix fop */

struct wlrx_pctrl;

#ifdef CONFIG_WIRELESS_CHARGER
u8 wlrx_intfr_get_src(unsigned int drv_type);
int wlrx_intfr_get_fixed_fop(unsigned int drv_type);
int wlrx_intfr_get_vtx(unsigned int drv_type);
int wlrx_intfr_get_vrx(unsigned int drv_type);
int wlrx_intfr_get_irx(unsigned int drv_type);
void wlrx_intfr_handle_settings(unsigned int drv_type, u8 src_state);
void wlrx_intfr_update_pctrl(unsigned int drv_type, struct wlrx_pctrl *pctrl);
void wlrx_intfr_clear_settings(unsigned int drv_type);
#else
static inline u8 wlrx_intfr_get_src(unsigned int drv_type)
{
	return 0;
}

static inline int wlrx_intfr_get_fixed_fop(unsigned int drv_type)
{
	return WLRX_INTFR_DFT_FIXED_FOP;
}

static inline int wlrx_intfr_get_vtx(unsigned int drv_type)
{
	return 0;
}

static inline int wlrx_intfr_get_vrx(unsigned int drv_type)
{
	return 0;
}

static inline int wlrx_intfr_get_imax(unsigned int drv_type)
{
	return 0;
}

static inline void wlrx_intfr_handle_settings(unsigned int drv_type, u8 src_states)
{
}

static inline void wlrx_intfr_update_pctrl(unsigned int drv_type, struct wlrx_pctrl *pctrl)
{
}

static inline void wlrx_intfr_clear_settings(unsigned int drv_type)
{
}
#endif /* CONFIG_WIRELESS_CHARGER */

#endif /* _WIRELESS_RX_INTERFERE_H_ */
