// SPDX-License-Identifier: GPL-2.0
/*
 * cps4067_dts.c
 *
 * cps4067 dts driver
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

#include "cps4067.h"

#define HWLOG_TAG wireless_cps4067_dts
HWLOG_REGIST();

#define CPS4067_CM_CFG_LEN                   3
#define CPS4067_POLAR_CFG_LEN                3

static void cps4067_parse_tx_fod(struct device_node *np, struct cps4067_dev_info *di)
{
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "tx_ploss_th0",
		&di->tx_fod.ploss_th0, CPS4067_TX_PLOSS_TH0_VAL);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "tx_ploss_th1",
		&di->tx_fod.ploss_th1, CPS4067_TX_PLOSS_TH1_VAL);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "tx_ploss_th2",
		&di->tx_fod.ploss_th2, CPS4067_TX_PLOSS_TH2_VAL);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "tx_ploss_th3",
		&di->tx_fod.ploss_th3, CPS4067_TX_PLOSS_TH3_VAL);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "tx_ploss_cnt",
		&di->tx_fod.ploss_cnt, CPS4067_TX_PLOSS_CNT_VAL);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "tx_q_en",
		&di->tx_fod.q_en, CPS4067_TX_FUNC_DIS);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "tx_q_coil_th",
		&di->tx_fod.q_coil_th, CPS4067_TX_Q_ONLY_COIL_TH);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "tx_q_th",
		&di->tx_fod.q_th, CPS4067_TX_Q_TH);
}

static int cps4067_parse_ldo_cfg(struct device_node *np, struct cps4067_dev_info *di)
{
	if (power_dts_read_u8_array(power_dts_tag(HWLOG_TAG), np,
		"rx_ldo_cfg_5v", di->rx_ldo_cfg.l_volt, CPS4067_RX_LDO_CFG_LEN))
		return -EINVAL;
	if (power_dts_read_u8_array(power_dts_tag(HWLOG_TAG), np,
		"rx_ldo_cfg_9v", di->rx_ldo_cfg.m_volt, CPS4067_RX_LDO_CFG_LEN))
		return -EINVAL;
	if (power_dts_read_u8_array(power_dts_tag(HWLOG_TAG), np,
		"rx_ldo_cfg_sc", di->rx_ldo_cfg.sc, CPS4067_RX_LDO_CFG_LEN))
		return -EINVAL;

	return 0;
}

static void cps4067_parse_rx_ask_mod_cfg(struct device_node *np,
	struct cps4067_dev_info *di)
{
	if (power_dts_read_u8_array(power_dts_tag(HWLOG_TAG), np,
		"rx_mod_cm_cfg", (u8 *)&di->rx_mod_cfg.cm, CPS4067_CM_CFG_LEN)) {
		di->rx_mod_cfg.cm.l_volt = CPS4067_RX_CMALL_EN_VAL;
		di->rx_mod_cfg.cm.h_volt = CPS4067_RX_CMC_EN_VAL;
		di->rx_mod_cfg.cm.fac_h_volt = CPS4067_RX_CMBC_EN_VAL;
	}
	hwlog_info("[parse_rx_ask_mod_cfg] [cm] l_volt=0x%x h_volt=0x%x fac_h_volt=0x%x\n",
		di->rx_mod_cfg.cm.l_volt, di->rx_mod_cfg.cm.h_volt,
		di->rx_mod_cfg.cm.fac_h_volt);
	if (power_dts_read_u8_array(power_dts_tag(HWLOG_TAG), np,
		"rx_mod_polar_cfg", (u8 *)&di->rx_mod_cfg.polar, CPS4067_POLAR_CFG_LEN)) {
		di->rx_mod_cfg.polar.l_volt = CPS4067_RX_CM_POSITIVE;
		di->rx_mod_cfg.polar.h_volt = CPS4067_RX_CM_NEGTIVE;
		di->rx_mod_cfg.polar.fac_h_volt = CPS4067_RX_CM_NEGTIVE;
	}
	hwlog_info("[parse_rx_ask_mod_cfg] [polar] l_volt=0x%x h_volt=0x%x fac_h_volt=0x%x\n",
		di->rx_mod_cfg.polar.l_volt, di->rx_mod_cfg.polar.h_volt,
		di->rx_mod_cfg.polar.fac_h_volt);
}

int cps4067_parse_dts(struct device_node *np, struct cps4067_dev_info *di)
{
	int ret;

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"rx_ss_good_lth", (u32 *)&di->rx_ss_good_lth,
		CPS4067_RX_SS_MAX);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"gpio_en_valid_val", (u32 *)&di->gpio_en_valid_val,
		WLTRX_IC_EN_ENABLE);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"full_bridge_ith", (u32 *)&di->tx_full_bri_ith,
		CPS4067_TX_FULL_BRI_ITH);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"mtp_check_delay_user", &di->mtp_check_delay.user,
		WIRELESS_FW_WORK_DELAYED_TIME);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"mtp_check_delay_fac", &di->mtp_check_delay.fac,
		WIRELESS_FW_WORK_DELAYED_TIME);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"tx_ping_freq", &di->tx_ping_freq,
		CPS4067_TX_PING_FREQ);

	cps4067_parse_tx_fod(np, di);
	ret = cps4067_parse_ldo_cfg(np, di);
	if (ret) {
		hwlog_err("parse_dts: parse ldo cfg failed\n");
		return ret;
	}

	cps4067_parse_rx_ask_mod_cfg(np, di);

	return 0;
}
