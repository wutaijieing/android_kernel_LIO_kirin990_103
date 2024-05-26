/* SPDX-License-Identifier: GPL-2.0 */
/*
 * wireless_trx_ic_intf.h
 *
 * common ic interface, variables, definition etc for wireless_trx_ic_intf
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

#ifndef _WIRELESS_TRX_IC_INTF_H_
#define _WIRELESS_TRX_IC_INTF_H_

#include <linux/types.h>
#include <chipset_common/hwpower/wireless_charge/wireless_trx_intf.h>

/* en: gpio low */
#define WLTRX_IC_EN_ENABLE            0
#define WLTRX_IC_EN_DISABLE           1
/* en: gpio high */
#define WLTRX_IC_SLEEP_ENABLE         1
#define WLTRX_IC_SLEEP_DISABLE        0

#define WLTRX_IC_CHIP_INFO_LEN        128
#define WLTRX_IC_I2C_RETRY_CNT        3

/* type must be appended and unchangeable */
enum wltrx_ic_type {
	WLTRX_IC_MIN = 0,
	WLTRX_IC_MAIN = WLTRX_IC_MIN,
	WLTRX_IC_AUX,
	WLTRX_IC_MAX,
};

struct wlrx_ic_ops;
struct wltx_ic_ops;
struct hwqi_ops;
struct wireless_fw_ops;
struct wireless_hw_test_ops;
struct wltrx_icali_ops;

struct wltrx_ic_ops {
	struct wlrx_ic_ops *rx_ops;
	struct wltx_ic_ops *tx_ops;
	struct hwqi_ops *qi_ops;
	struct wireless_fw_ops *fw_ops;
	struct wireless_hw_test_ops *hw_tst_ops;
	struct wltrx_icali_ops *cali_ops;
};

static inline bool wltrx_ic_is_type_valid(unsigned int type)
{
	return (type >= WLTRX_IC_MIN) && (type < WLTRX_IC_MAX);
}

static inline unsigned int wltrx_get_dflt_ic_type(unsigned int drv_type)
{
	return (drv_type == WLTRX_DRV_MAIN) ? WLTRX_IC_MAIN : WLTRX_IC_AUX;
}
#endif /* _WIRELESS_TRX_IC_INTF_H_ */
