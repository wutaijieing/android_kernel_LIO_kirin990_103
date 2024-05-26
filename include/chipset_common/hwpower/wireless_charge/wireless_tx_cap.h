/* SPDX-License-Identifier: GPL-2.0 */
/*
 * wireless_tx_cap.h
 *
 * tx capability head file for wireless reverse charging
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

#ifndef _WIRELESS_TX_CAP_H_
#define _WIRELESS_TX_CAP_H_

#define WLTX_CAP_LOW_PWR    BIT(0)
#define WLTX_CAP_HIGH_PWR   BIT(1)
#define WLTX_CAP_HIGH_PWR2  BIT(2)

enum wltx_cap_info {
	WLTX_CAP_BEGIN = 0,
	WLTX_CAP_TYPE = WLTX_CAP_BEGIN,
	WLTX_CAP_VMAX,
	WLTX_CAP_IMAX,
	WLTX_CAP_ATTR,
	WLTX_CAP_END,
};

struct wltx_cap {
	u8 type;
	int vout;
	int iout;
	u8 attr;
};

enum wltx_cap_level {
	WLTX_DFLT_CAP = 0,
	WLTX_HIGH_PWR_CAP,
	WLTX_HIGH_PWR2_CAP,
	WLTX_TOTAL_CAP,
};

#ifdef CONFIG_WIRELESS_CHARGER
void wltx_send_tx_cap(unsigned int drv_type);
unsigned int wltx_cap_get_mode_id(unsigned int drv_type);
unsigned int wltx_cap_get_cur_id(unsigned int drv_type);
void wltx_cap_set_cur_id(unsigned int drv_type, unsigned int cur_id);
void wltx_cap_set_exp_id(unsigned int drv_type, unsigned int exp_id);
void wltx_cap_reset_exp_tx_id(unsigned int drv_type);
u8 wltx_cap_get_tx_type(unsigned int drv_type);
int wltx_cap_init(unsigned int drv_type, const struct device_node *np);
void wltx_cap_deinit(unsigned int drv_type);
#else
static inline void wltx_send_tx_cap(unsigned int drv_type)
{
}

static inline unsigned int wltx_cap_get_mode_id(unsigned int drv_type)
{
	return WLTX_DFLT_CAP;
}

static inline unsigned int wltx_cap_get_cur_id(unsigned int drv_type)
{
	return WLTX_DFLT_CAP;
}

static inline void wltx_cap_set_cur_id(unsigned int drv_type, unsigned int cur_id)
{
}

static inline void wltx_cap_set_exp_id(unsigned int drv_type, unsigned int exp_id)
{
}

static inline void wltx_cap_reset_exp_tx_id(unsigned int drv_type)
{
}

static inline u8 wltx_cap_get_tx_type(unsigned int drv_type)
{
	return 0;
}

static inline int wltx_cap_init(unsigned int drv_type, const struct device_node *np)
{
	return 0;
}

static inline void wltx_cap_deinit(unsigned int drv_type)
{
}
#endif /* CONFIG_WIRELESS_CHARGER */

#endif /* _WIRELESS_TX_CAP_H_ */
