/* SPDX-License-Identifier: GPL-2.0 */
/*
 * wireless_rx_auth.h
 *
 * authenticate for wireless rx charge
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

#ifndef _WIRELESS_RX_AUTH_H_
#define _WIRELESS_RX_AUTH_H_

#include <linux/types.h>

#define WLRX_ACC_AUTH_CNT               3
#define WLRX_ACC_AUTH_SUCC              0
#define WLRX_ACC_AUTH_DEV_ERR           (-1)
#define WLRX_ACC_AUTH_CNT_ERR           (-2)
#define WLRX_ACC_AUTH_CM_ERR            (-3)
#define WLRX_ACC_AUTH_SRV_NOT_READY     (-4)
#define WLRX_ACC_AUTH_SRV_ERR           (-5)

#ifdef CONFIG_WIRELESS_CHARGER
bool wlrx_auth_srv_ready(void);
int wlrx_auth_handler(unsigned int ic_type, int prot, int kid);
#else
static inline bool wlrx_auth_srv_ready(void)
{
	return false;
}

static inline int wlrx_auth_handler(unsigned int ic_type, int prot, int kid)
{
	return WLRX_ACC_AUTH_DEV_ERR;
}
#endif /* CONFIG_WIRELESS_CHARGER */

#endif /* _WIRELESS_RX_AUTH_H_ */
