// SPDX-License-Identifier: GPL-2.0
/*
 * stwlc68_dts.c
 *
 * stwlc68 dts driver
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

#include "stwlc68.h"

#define HWLOG_TAG wireless_stwlc68_dts
HWLOG_REGIST();

static int stwlc68_parse_fod(struct device_node *np, struct stwlc68_dev_info *di)
{
	if (power_dts_read_u8_array(power_dts_tag(HWLOG_TAG), np,
		"rx_fod_5v", di->rx_fod_5v, STWLC68_RX_FOD_LEN))
		return -EINVAL;

	if (power_dts_read_u8_array(power_dts_tag(HWLOG_TAG), np,
		"pu_rx_fod_5v", di->pu_rx_fod_5v, STWLC68_RX_FOD_LEN))
		di->need_chk_pu_shell = false;
	else
		di->need_chk_pu_shell = true;
	hwlog_info("[parse_fod] need_chk_pu_shell=%d\n", di->need_chk_pu_shell);

	if (power_dts_read_u8_array(power_dts_tag(HWLOG_TAG), np,
		"rx_fod_9v", di->rx_fod_9v, STWLC68_RX_FOD_LEN))
		return -EINVAL;

	if (power_dts_read_u8_array(power_dts_tag(HWLOG_TAG), np,
		"rx_fod_12v", di->rx_fod_12v, STWLC68_RX_FOD_LEN))
		return -EINVAL;

	if (power_dts_read_u8_array(power_dts_tag(HWLOG_TAG), np,
		"rx_fod_15v", di->rx_fod_15v, STWLC68_RX_FOD_LEN))
		return -EINVAL;

	if (power_dts_read_u8_array(power_dts_tag(HWLOG_TAG), np,
		"rx_fod_9v_cp60", di->rx_fod_9v_cp60, STWLC68_RX_FOD_LEN))
		memcpy(di->rx_fod_9v_cp60, di->rx_fod_9v, sizeof(di->rx_fod_9v));

	if (power_dts_read_u8_array(power_dts_tag(HWLOG_TAG), np,
		"rx_fod_9v_cp39s", di->rx_fod_9v_cp39s, STWLC68_RX_FOD_LEN))
		memcpy(di->rx_fod_9v_cp39s, di->rx_fod_9v, sizeof(di->rx_fod_9v));

	if (power_dts_read_u8_array(power_dts_tag(HWLOG_TAG), np,
		"rx_fod_15v_cp39s", di->rx_fod_15v_cp39s, STWLC68_RX_FOD_LEN))
		memcpy(di->rx_fod_15v_cp39s, di->rx_fod_15v, sizeof(di->rx_fod_15v));

	return 0;
}

static int stwlc68_parse_ldo_cfg(struct device_node *np, struct stwlc68_dev_info *di)
{
	if (power_dts_read_u8_array(power_dts_tag(HWLOG_TAG), np,
		"rx_ldo_cfg_5v", di->rx_ldo_cfg_5v, STWLC68_LDO_CFG_LEN))
		return -EINVAL;

	if (power_dts_read_u8_array(power_dts_tag(HWLOG_TAG), np,
		"rx_ldo_cfg_9v", di->rx_ldo_cfg_9v, STWLC68_LDO_CFG_LEN))
		return -EINVAL;

	if (power_dts_read_u8_array(power_dts_tag(HWLOG_TAG), np,
		"rx_ldo_cfg_12v", di->rx_ldo_cfg_12v, STWLC68_LDO_CFG_LEN))
		return -EINVAL;

	if (power_dts_read_u8_array(power_dts_tag(HWLOG_TAG), np,
		"rx_ldo_cfg_sc", di->rx_ldo_cfg_sc, STWLC68_LDO_CFG_LEN))
		return -EINVAL;

	return 0;
}

static void stwlc68_parse_rx_offset(struct device_node *np, struct stwlc68_dev_info *di)
{
	(void)power_dts_read_u8(power_dts_tag(HWLOG_TAG), np,
		"rx_offset_9v", &di->rx_offset_9v, STWLC68_RX_OFFSET_DEFAULT_VALUE);

	(void)power_dts_read_u8(power_dts_tag(HWLOG_TAG), np,
		"rx_offset_15v", &di->rx_offset_15v, STWLC68_RX_OFFSET_DEFAULT_VALUE);
}

int stwlc68_parse_dts(struct device_node *np, struct stwlc68_dev_info *di)
{
	int ret;

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"rx_ss_good_lth", (u32 *)&di->rx_ss_good_lth, STWLC68_RX_SS_MAX);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"gpio_en_valid_val", (u32 *)&di->gpio_en_valid_val, WLTRX_IC_EN_ENABLE);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"gpio_sleep_en_pending", (u32 *)&di->gpio_sleep_en_pending, false);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"gpio_pwr_good_pending", (u32 *)&di->gpio_pwr_good_pending, false);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"support_cp", (u32 *)&di->support_cp, 1); /* 1: support cp */

	ret = stwlc68_parse_fod(np, di);
	if (ret) {
		hwlog_err("parse_dts: parse fod para fail\n");
		return ret;
	}
	stwlc68_parse_rx_offset(np, di);
	ret = stwlc68_parse_ldo_cfg(np, di);
	if (ret) {
		hwlog_err("parse_dts: parse ldo cfg fail\n");
		return ret;
	}
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"dev_type", (u32 *)&di->dev_type, WIRELESS_DEVICE_UNKNOWN);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"tx_ocp_val", (u32 *)&di->tx_ocp_val, STWLC68_TX_OCP_VAL);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"tx_ovp_val", (u32 *)&di->tx_ovp_val, STWLC68_TX_OVP_VAL);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"tx_uvp_th", (u32 *)&di->tx_uvp_th, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"tx_ping_interval", (u32 *)&di->tx_ping_interval, STWLC68_TX_PING_INTERVAL_INIT);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"tx_max_fop", (u32 *)&di->tx_max_fop, STWLC68_TX_MAX_FOP_VAL);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"tx_min_fop", (u32 *)&di->tx_min_fop, STWLC68_TX_MIN_FOP_VAL);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"rx_ready_ext_pwr_en", (u32 *)&di->rx_ready_ext_pwr_en, 1); /* 1: ext_pwr enable */

	return 0;
}