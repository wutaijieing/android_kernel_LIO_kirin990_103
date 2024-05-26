/* SPDX-License-Identifier: GPL-2.0 */
/*
 * wireless_buck_ictrl.h
 *
 * current control of wireless buck charging
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

#ifndef _WIRELESS_BUCK_ICTRL_H_
#define _WIRELESS_BUCK_ICTRL_H_

#include <chipset_common/hwpower/wireless_charge/wireless_trx_intf.h>
#include <linux/errno.h>

#define WLRX_BUCK_IIN_DELAY            100
#define WLRX_BUCK_DFLT_IMAX            2000 /* mA */

struct device;

#ifdef CONFIG_WIRELESS_CHARGER
void wlrx_buck_set_iin_prop(unsigned int drv_type, int iin_step, int iin_delay, int iset);
void wlrx_buck_set_iin_for_rx(unsigned int drv_type, int iin_ma);
void wlrx_buck_set_iin_for_charger(unsigned int drv_type, int iin_ma);
int wlrx_buck_get_iset(unsigned int drv_type);
void wlrx_buck_update_ictrl_para(unsigned int drv_type, int *iset);
#else
static inline void wlrx_buck_set_iin_prop(unsigned int drv_type, int iin_step, int iin_delay, int iset)
{
}

static inline void wlrx_buck_set_iin_for_rx(unsigned int drv_type, int iin_ma)
{
}

static inline void wlrx_buck_set_iin_for_charger(unsigned int drv_type, int iin_ma)
{
}

static inline int wlrx_buck_get_iset(unsigned int drv_type)
{
	return 0;
}

static inline void wlrx_buck_update_ictrl_para(unsigned int drv_type, int *iset)
{
}
#endif /* CONFIG_WIRELESS_CHARGER */

#endif /* _WIRELESS_BUCK_ICTRL_H_ */
