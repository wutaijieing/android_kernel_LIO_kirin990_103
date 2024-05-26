/* SPDX-License-Identifier: GPL-2.0 */
/*
 * wireless_rx_acc.h
 *
 * accessory(tx,cable,adapter etc.) for wireless charging
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

#ifndef _WIRELESS_RX_ACC_H_
#define _WIRELESS_RX_ACC_H_

#include <linux/types.h>

#define WLRX_ACC_TX_PWR_RATIO              75
#define WLRX_ACC_NORMAL_MAX_FOP            150

/* for handshake */
#define WLRX_ACC_HW_ID                     0x8866
#define WLRX_ACC_HS_CNT                    3
#define WLRX_ACC_HS_SUCC                   0
#define WLRX_ACC_HS_DEV_ERR                (-1)
#define WLRX_ACC_HS_CNT_ERR                (-2)
#define WLRX_ACC_HS_ID_ERR                 (-3)
#define WLRX_ACC_HS_CM_ERR                 (-4)

/* for tx_cap */
#define WLRX_ACC_DET_CAP_CNT               3
#define WLRX_ACC_DET_CAP_SUCC              0
#define WLRX_ACC_DET_CAP_DEV_ERR           (-1)
#define WLRX_ACC_DET_CAP_CNT_ERR           (-2)
#define WLRX_ACC_DET_CAP_CM_ERR            (-3)

/* for authentication */
#define WLRX_ACC_AUTH_CNT                  3
#define WLRX_ACC_AUTH_SUCC                 0
#define WLRX_ACC_AUTH_DEV_ERR              (-1)
#define WLRX_ACC_AUTH_CNT_ERR              (-2)
#define WLRX_ACC_AUTH_CM_ERR               (-3)
#define WLRX_ACC_AUTH_SRV_NOT_READY        (-4)
#define WLRX_ACC_AUTH_SRV_ERR              (-5)

#ifdef CONFIG_WIRELESS_CHARGER
bool wlrx_is_std_tx(unsigned int drv_type);
bool wlrx_is_err_tx(unsigned int drv_type);
bool wlrx_is_fac_tx(unsigned int drv_type);
bool wlrx_is_car_tx(unsigned int drv_type);
bool wlrx_is_rvs_tx(unsigned int drv_type);
bool wlrx_is_qc_adp(unsigned int drv_type);
bool wlrx_is_highpwr_rvs_tx(unsigned int drv_type);
bool wlrx_is_qval_err_tx(unsigned int drv_type);
unsigned int wlrx_acc_get_tx_type(unsigned int drv_type);
struct wlprot_acc_cap *wlrx_acc_get_cap(unsigned int drv_type);
int wlrx_acc_get_tx_eff(unsigned int drv_type);
bool wlrx_acc_support_fix_fop(unsigned int drv_type, int fop);
void wlrx_acc_reset_para(unsigned int drv_type);
bool wlrx_acc_auth_need_recheck(unsigned int drv_type);
bool wlrx_acc_auth_succ(unsigned int drv_type);
int wlrx_acc_auth(unsigned int drv_type);
int wlrx_acc_detect_cap(unsigned int drv_type);
int wlrx_acc_handshake(unsigned int drv_type);
#else
static inline bool wlrx_is_std_tx(unsigned int drv_type)
{
	return false;
}

static inline bool wlrx_is_err_tx(unsigned int drv_type)
{
	return true;
}

static inline bool wlrx_is_fac_tx(unsigned int drv_type)
{
	return false;
}

static inline bool wlrx_is_car_tx(unsigned int drv_type)
{
	return false;
}

static inline bool wlrx_is_rvs_tx(unsigned int drv_type)
{
	return false;
}

static inline bool wlrx_is_qc_adp(unsigned int drv_type)
{
	return false;
}

static inline bool wlrx_is_highpwr_rvs_tx(unsigned int drv_type)
{
	return false;
}

static inline bool wlrx_is_qval_err_tx(unsigned int drv_type)
{
	return false;
}

static inline unsigned int wlrx_acc_get_tx_type(unsigned int drv_type)
{
	return 0;
}

static inline struct wlprot_acc_cap *wlrx_acc_get_cap(unsigned int drv_type)
{
	return NULL;
}

static inline int wlrx_acc_get_tx_eff(unsigned int drv_type)
{
	return 0;
}

static inline bool wlrx_acc_support_fix_fop(unsigned int drv_type, int fop)
{
	return false;
}

static inline void wlrx_acc_reset_para(unsigned int drv_type)
{
}

static inline bool wlrx_acc_auth_need_recheck(unsigned int drv_type)
{
	return false;
}

static inline bool wlrx_acc_auth_succ(unsigned int drv_type)
{
	return false;
}

static inline int wlrx_acc_auth(unsigned int drv_type)
{
	return WLRX_ACC_AUTH_DEV_ERR;
}

static inline int wlrx_acc_detect_cap(unsigned int drv_type)
{
	return WLRX_ACC_DET_CAP_DEV_ERR;
}

static inline int wlrx_acc_handshake(unsigned int drv_type)
{
	return WLRX_ACC_HS_DEV_ERR;
}
#endif /* CONFIG_WIRELESS_CHARGER */

#endif /* _WIRELESS_RX_ACC_ */
