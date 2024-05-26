// SPDX-License-Identifier: GPL-2.0
/*
 * mt5785_dts.c
 *
 * mt5785 dts driver
 *
 * Copyright (c) 2022-2022 Huawei Technologies Co., Ltd.
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

#include "mt5785.h"

#define HWLOG_TAG wireless_mt5785_dts
HWLOG_REGIST();

#define MT5785_CM_CFG_LEN                   3
#define MT5785_POLAR_CFG_LEN                3

static void mt5785_parse_tx_fod(struct device_node *np, struct mt5785_dev_info *di)
{
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "tx_ploss_th0",
		&di->tx_fod.ploss_th0, MT5785_TX_PLOSS_TH0_VAL);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "tx_ploss_th1",
		&di->tx_fod.ploss_th1, MT5785_TX_PLOSS_TH1_VAL);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "tx_ploss_th2",
		&di->tx_fod.ploss_th2, MT5785_TX_PLOSS_TH2_VAL);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "tx_ploss_th3",
		&di->tx_fod.ploss_th3, MT5785_TX_PLOSS_TH3_VAL);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "tx_ploss_cnt",
		&di->tx_fod.ploss_cnt, MT5785_TX_PLOSS_CNT_VAL);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "tx_q_en",
		&di->tx_fod.q_en, MT5785_TX_FUNC_DIS);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "tx_q_coil_th",
		&di->tx_fod.q_coil_th, MT5785_TX_Q_ONLY_COIL_TH);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "tx_q_th",
		&di->tx_fod.q_th, MT5785_TX_Q_TH);
}

static int mt5785_parse_ldo_cfg(struct device_node *np, struct mt5785_dev_info *di)
{
	if (power_dts_read_u8_array(power_dts_tag(HWLOG_TAG), np,
		"rx_ldo_cfg_5v", di->rx_ldo_cfg.l_volt, MT5785_RX_LDO_CFG_LEN))
		return -EINVAL;
	if (power_dts_read_u8_array(power_dts_tag(HWLOG_TAG), np,
		"rx_ldo_cfg_9v", di->rx_ldo_cfg.m_volt, MT5785_RX_LDO_CFG_LEN))
		return -EINVAL;
	if (power_dts_read_u8_array(power_dts_tag(HWLOG_TAG), np,
		"rx_ldo_cfg_sc", di->rx_ldo_cfg.sc, MT5785_RX_LDO_CFG_LEN))
		return -EINVAL;

	return 0;
}

static void mt5785_parse_rx_ask_mod_cfg(struct device_node *np,
	struct mt5785_dev_info *di)
{
	if (power_dts_read_u8_array(power_dts_tag(HWLOG_TAG), np,
		"rx_mod_cm_cfg", (u8 *)&di->rx_mod_cfg.cm, MT5785_CM_CFG_LEN)) {
		di->rx_mod_cfg.cm.l_volt = MT5785_RX_CMALL_EN_VAL;
		di->rx_mod_cfg.cm.h_volt = MT5785_RX_CMALL_EN_VAL;
		di->rx_mod_cfg.cm.fac_h_volt = MT5785_RX_CMALL_EN_VAL;
	}
	hwlog_info("[parse_rx_ask_mod_cfg] [cm] l_volt=0x%x h_volt=0x%x fac_h_volt=0x%x\n",
		di->rx_mod_cfg.cm.l_volt, di->rx_mod_cfg.cm.h_volt,
		di->rx_mod_cfg.cm.fac_h_volt);
	if (power_dts_read_u8_array(power_dts_tag(HWLOG_TAG), np,
		"rx_mod_polar_cfg", (u8 *)&di->rx_mod_cfg.polar, MT5785_POLAR_CFG_LEN)) {
		di->rx_mod_cfg.polar.l_volt = MT5785_RX_CM_POSITIVE;
		di->rx_mod_cfg.polar.h_volt = MT5785_RX_CM_POSITIVE;
		di->rx_mod_cfg.polar.fac_h_volt = MT5785_RX_CM_POSITIVE;
	}
	hwlog_info("[parse_rx_ask_mod_cfg] [polar] l_volt=0x%x h_volt=0x%x fac_h_volt=0x%x\n",
		di->rx_mod_cfg.polar.l_volt, di->rx_mod_cfg.polar.h_volt,
		di->rx_mod_cfg.polar.fac_h_volt);
}

int mt5785_parse_dts(struct device_node *np, struct mt5785_dev_info *di)
{
	int ret;

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"rx_ss_good_lth", (u32 *)&di->rx_ss_good_lth,
		MT5785_RX_SS_MAX);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"gpio_en_valid_val", (u32 *)&di->gpio_en_valid_val,
		WLTRX_IC_EN_ENABLE);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"mtp_check_delay_user", &di->mtp_check_delay.user,
		WIRELESS_FW_WORK_DELAYED_TIME);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"mtp_check_delay_fac", &di->mtp_check_delay.fac,
		WIRELESS_FW_WORK_DELAYED_TIME);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"tx_ping_freq", &di->tx_ping_freq,
		MT5785_TX_PING_FREQ);

	mt5785_parse_tx_fod(np, di);
	ret = mt5785_parse_ldo_cfg(np, di);
	if (ret) {
		hwlog_err("parse_dts: parse ldo cfg failed\n");
		return ret;
	}

	mt5785_parse_rx_ask_mod_cfg(np, di);
	return 0;
}
