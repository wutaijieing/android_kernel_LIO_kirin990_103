/* SPDX-License-Identifier: GPL-2.0 */
/*
 * wireless_tx_client.h
 *
 * tx client head file for wireless reverse charging
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

#ifndef _WIRELESS_TX_CLIENT_H_
#define _WIRELESS_TX_CLIENT_H_

/* type must be increased one by one from 0 */
enum wltx_client {
	WLTX_CLIENT_BEGIN = 0,
	WLTX_CLIENT_UI = WLTX_CLIENT_BEGIN, /* client:0, dont move */
	WLTX_CLIENT_HALL,
	WLTX_CLIENT_COIL_TEST,
	WLTX_CLIENT_LIGHTSTRAP,
	WLTX_CLIENT_BAT_HEATING,
	WLTX_CLIENT_END,
};

#ifdef CONFIG_WIRELESS_CHARGER
enum wltx_client wltx_get_client(void);
void wltx_set_client(enum wltx_client client);
bool wltx_client_is_hall(void);
#else
static inline enum wltx_client wltx_get_client(void)
{
	return WLTX_CLIENT_UI;
}

static inline void wltx_set_client(enum wltx_client client)
{
}

static inline bool wltx_client_is_hall(void)
{
	return false;
}
#endif /* CONFIG_WIRELESS_CHARGER */

#endif /* _WIRELESS_TX_CLIENT_H_ */
