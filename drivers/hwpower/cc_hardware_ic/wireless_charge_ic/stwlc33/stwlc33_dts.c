// SPDX-License-Identifier: GPL-2.0
/*
 * stwlc33_dts.c
 *
 * stwlc33 dts driver
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

#include "stwlc33.h"

#define HWLOG_TAG wireless_stwlc33_dts
HWLOG_REGIST();

int stwlc33_parse_dts(struct device_node *np, struct stwlc33_dev_info *di)
{
	if (!np || !di) {
		hwlog_err("parse_dts: np or di null\n");
		return -EINVAL;
	}

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"gpio_en_valid_val", (u32 *)&di->gpio_en_valid_val, WLTRX_IC_EN_ENABLE);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"tx_fod_th_5v", (u32 *)&di->tx_fod_th_5v, STWLC33_TX_FOD_THD0_VAL);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"support_tx_adjust_vin", (u32 *)&di->support_tx_adjust_vin, false);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"ping_freq_init_dym", (u32 *)&di->ping_freq_init_dym,
		STWLC33_TX_PING_FREQUENCY_INIT_DYM);
	(void)power_dts_read_u8(power_dts_tag(HWLOG_TAG), np,
		"tx_ploss_th0", &di->tx_ploss_th0, STWLC33_TX_FOD_PL_VAL);

	return 0;
}