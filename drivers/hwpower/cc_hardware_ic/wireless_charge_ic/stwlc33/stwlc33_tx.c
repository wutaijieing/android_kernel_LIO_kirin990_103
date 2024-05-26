// SPDX-License-Identifier: GPL-2.0
/*
 * stwlc33_tx.c
 *
 * stwlc33 tx driver
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

#define HWLOG_TAG wireless_stwlc33_tx
HWLOG_REGIST();

static const char * const g_stwlc33_tx_irq_name[] = {
	/* [n]: n means bit in registers */
	[0] = "tx_ept",
	[1] = "tx_start_ping",
	[2] = "tx_ss_pkt_rcvd",
	[3] = "tx_id_pkt_rcvd",
	[4] = "tx_cfg_pkt_rcvd",
	[5] = "tx_pp_pkt_rcvd",
};

static const char * const g_stwlc33_tx_ept_name[] = {
	/* [n]: n means bit in registers */
	[0]  = "tx_ept_src_cmd",
	[1]  = "tx_ept_src_ss",
	[2]  = "tx_ept_src_id",
	[3]  = "tx_ept_src_xid",
	[4]  = "tx_ept_src_cfg_cnt_err",
	[5]  = "tx_ept_src_pch",
	[6]  = "tx_ept_src_first_cep",
	[7]  = "tx_ept_src_chip_reset",
	[8]  = "tx_ept_src_cep_timeout",
	[9]  = "tx_ept_src_rpp_timeout",
	[10] = "tx_ept_src_ocp",
	[11] = "tx_ept_src_ovp",
	[12] = "tx_ept_src_lvp",
	[13] = "tx_ept_src_fod",
	[14] = "tx_ept_src_otp",
	[15] = "tx_ept_src_lcp",
};

static unsigned int stwlc33_tx_get_bnt_wltx_type(int ic_type)
{
	return ic_type == WLTRX_IC_AUX ? POWER_BNT_WLTX_AUX : POWER_BNT_WLTX;
}

static int stwlc33_tx_check_fwupdate(void *dev_data)
{
	return stwlc33_fw_check_fwupdate(dev_data, WIRELESS_TX);
}

static int stwlc33_tx_stop_config(void *dev_data)
{
	return 0;
}

int stwlc33_tx_enable_tx_mode(bool enable, void *dev_data)
{
	int ret;

	if (enable)
		ret = stwlc33_write_word_mask(dev_data, STWLC33_CMD3_ADDR, STWLC33_CMD3_TX_EN_MASK,
			STWLC33_CMD3_TX_EN_SHIFT, STWLC33_CMD3_VAL);
	else
		ret = stwlc33_write_word_mask(dev_data, STWLC33_CMD3_ADDR, STWLC33_CMD3_TX_DIS_MASK,
			STWLC33_CMD3_TX_DIS_SHIFT, STWLC33_CMD3_VAL);
	if (ret)
		hwlog_err("enable_tx_mode: %s tx mode failed\n", enable ? "enable" : "disable");

	return ret;
}

static void stwlc33_tx_enable_tx_ping(struct stwlc33_dev_info *di)
{
	int ret;

	ret = stwlc33_write_word_mask(di, STWLC33_CHIP_RST_ADDR, STWLC33_TX_PING_EN_MASK,
		STWLC33_TX_PING_EN_SHIFT, STWLC33_TX_PING_EN);
	if (ret)
		hwlog_err("enable_tx_ping: failed\n");
}

static int stwlc33_tx_get_iin(u16 *tx_iin, void *dev_data)
{
	return stwlc33_read_word(dev_data, STWLC33_TX_IIN_ADDR, tx_iin);
}

static int stwlc33_tx_get_vrect(u16 *tx_vrect, void *dev_data)
{
	return stwlc33_read_word(dev_data, STWLC33_TX_VRECT_ADDR, tx_vrect);
}

static int stwlc33_tx_get_vin(u16 *tx_vin, void *dev_data)
{
	int ret;

	ret = stwlc33_read_word(dev_data, STWLC33_TX_VIN_ADDR, tx_vin);
	if (ret) {
		hwlog_err("get_vin: read failed\n");
		return ret;
	}
	if (!(*tx_vin))
		*tx_vin = TX_DEFAULT_VOUT;

	return 0;
}

static int stwlc33_tx_get_fop(u16 *fop, void *dev_data)
{
	int ret;
	u16 data = 0;

	if (!fop) {
		hwlog_err("get_fop: fop is null\n");
		return -EINVAL;
	}

	ret = stwlc33_read_word(dev_data, STWLC33_TX_FOP_ADDR, &data);
	if (ret) {
		hwlog_err("get_fop: read failed\n");
		return ret;
	}
	if (data)
		*fop = 60000 / data; /* Ping freq(khz) = 60000/PingPeriod */

	return 0;
}

static int stwlc33_tx_set_max_fop(u16 fop, void *dev_data)
{
	if (!fop)
		return -EINVAL;

	/* Ping freq(khz) = 60000/PingPeriod */
	return stwlc33_write_word(dev_data, STWLC33_TX_MAX_FOP_ADDR, 60000 / fop);
}

static int stwlc33_tx_get_max_fop(u16 *fop, void *dev_data)
{
	int ret;
	u16 data = 0;

	if (!fop) {
		hwlog_err("get_max_fop: fop is null\n");
		return -EINVAL;
	}

	ret = stwlc33_read_word(dev_data, STWLC33_TX_MAX_FOP_ADDR, &data);
	if (ret) {
		hwlog_err("get_max_fop: read failed\n");
		return ret;
	}
	if (data)
		*fop = 60000 / data; /* Ping freq(khz) = 60000/PingPeriod */

	return 0;
}

static int stwlc33_tx_set_min_fop(u16 fop, void *dev_data)
{
	if (!fop)
		return -EINVAL;

	/* Ping freq(khz) = 60000/PingPeriod */
	return stwlc33_write_word(dev_data, STWLC33_TX_MIN_FOP_ADDR, 60000 / fop);
}

static int stwlc33_tx_get_min_fop(u16 *fop, void *dev_data)
{
	int ret;
	u16 data = 0;

	if (!fop) {
		hwlog_err("get_min_fop: fop is null\n");
		return -EINVAL;
	}

	ret = stwlc33_read_word(dev_data, STWLC33_TX_MIN_FOP_ADDR, &data);
	if (ret) {
		hwlog_err("get_min_fop: read failed\n");
		return ret;
	}
	if (data)
		*fop = 60000 / data; /* Ping freq(khz) = 60000/PingPeriod */

	return 0;
}

static int stwlc33_tx_set_ping_freq(u16 ping_freq, void *dev_data)
{
	int ret;
	struct stwlc33_dev_info *di = dev_data;

	if ((ping_freq < STWLC33_TX_PING_FREQUENCY_MIN) ||
		(ping_freq > STWLC33_TX_PING_FREQUENCY_MAX)) {
		hwlog_err("set_ping_freq: ping freq is out of range\n");
		return -EINVAL;
	}
	/* Ping freq(khz) = 60000/PingPeriod */
	ret = stwlc33_write_word(di, STWLC33_TX_PING_FREQUENCY_ADDR, 60000 / ping_freq);
	if (ret) {
		hwlog_err("set_ping_freq: write failed\n");
		return ret;
	}
	ret = stwlc33_write_word(di, STWLC33_TX_PING_FREQUENCY_ADDR_DYM, di->ping_freq_init_dym);
	if (ret) {
		hwlog_err("set_ping_freq: write dynamic freq failed\n");
		return ret;
	}

	return 0;
}

static int stwlc33_tx_get_ping_freq(u16 *ping_freq, void *dev_data)
{
	int ret;
	u16 data = 0;

	if (!ping_freq) {
		hwlog_err("get_ping_freq: ping_freq is null\n");
		return -EINVAL;
	}

	ret = stwlc33_read_word(dev_data, STWLC33_TX_PING_FREQUENCY_ADDR, &data);
	if (ret) {
		hwlog_err("get_ping_freq: read failed\n");
		return ret;
	}
	if (data)
		*ping_freq = 60000 / data; /* Ping freq(khz) = 60000/PingPeriod */

	return 0;
}

static int stwlc33_tx_set_ping_interval(u16 ping_interval, void *dev_data)
{
	int ret;

	if ((ping_interval < STWLC33_TX_PING_INTERVAL_MIN) ||
		(ping_interval > STWLC33_TX_PING_INTERVAL_MAX)) {
		hwlog_err("set_ping_interval: ping interval is out of range\n");
		return -EINVAL;
	}

	ret = stwlc33_write_byte(dev_data, STWLC33_TX_PING_INTERVAL_ADDR,
		ping_interval / STWLC33_TX_PING_INTERVAL_STEP);
	if (ret)
		hwlog_err("set_ping_interval: write failed\n");

	return ret;
}

static int stwlc33_tx_get_ping_interval(u16 *ping_interval, void *dev_data)
{
	int ret;
	u8 data;

	if (!ping_interval) {
		hwlog_err("get_ping_interval: ping_interval is null\n");
		return -EINVAL;
	}

	ret = stwlc33_read_byte(dev_data, STWLC33_TX_PING_INTERVAL_ADDR, &data);
	if (ret) {
		hwlog_err("get_ping_interval: read failed\n");
		return ret;
	}
	*ping_interval = data * STWLC33_TX_PING_INTERVAL_STEP;

	return 0;
}

static int stwlc33_tx_get_temp(s16 *chip_temp, void *dev_data)
{
	return stwlc33_read_byte(dev_data, STWLC33_CHIP_TEMP_ADDR, (u8 *)chip_temp);
}

static int stwlc33_tx_activate_chip(void *dev_data)
{
	return stwlc33_check_nvm_for_power(dev_data);
}

static int stwlc33_tx_chip_init(unsigned int client, void *dev_data)
{
	int ret;
	struct stwlc33_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	stwlc33_enable_irq(di);
	ret = stwlc33_write_word(di, STWLC33_TX_OCP_ADDR, STWLC33_TX_OCP_VAL);
	ret += stwlc33_write_word(di, STWLC33_TX_OVP_ADDR, STWLC33_TX_OVP_VAL);
	ret += stwlc33_write_word_mask(di, STWLC33_CMD3_ADDR, STWLC33_CMD3_TX_FOD_EN_MASK,
		STWLC33_CMD3_TX_FOD_EN_SHIFT, STWLC33_CMD3_VAL);
	ret += stwlc33_write_word(di, STWLC33_TX_FOD_THD0_ADDR, di->tx_fod_th_5v);
	ret += stwlc33_tx_set_ping_freq(STWLC33_TX_PING_FREQUENCY_INIT, di);
	ret += stwlc33_tx_set_ping_interval(STWLC33_TX_PING_INTERVAL_INIT, di);
	ret += stwlc33_write_byte(di, STWLC33_TX_FOD_PL_ADDR, di->tx_ploss_th0);
	if (ret) {
		hwlog_err("chip_init: write failed\n");
		return -EIO;
	}

	if (di->support_tx_adjust_vin) {
		ret = stwlc33_write_byte(di, STWLC33_TX_MAX_FREQ_ADDR, STWLC33_TX_MAX_FREQ);
		ret += stwlc33_write_byte(di, STWLC33_TX_MIN_FREQ_ADDR, STWLC33_TX_MIN_FREQ);
		/* default 5.8v */
		ret += stwlc33_write_byte(di, STWLC33_TX_GPIO_B2_ADDR, STWLC33_TX_GPIO_PU);
	} else {
		ret = stwlc33_write_byte(di, STWLC33_TX_MAX_FREQ_ADDR, STWLC33_TX_MAX_FREQ_DEFAULT);
		ret += stwlc33_write_byte(di, STWLC33_TX_MIN_FREQ_ADDR,
			STWLC33_TX_MIN_FREQ_DEFAULT);
	}
	if (ret) {
		hwlog_err("chip_init: freq write failed\n");
		return -EIO;
	}

	stwlc33_tx_enable_tx_ping(di);

	return 0;
}

static bool stwlc33_tx_check_rx_disconnect(void *dev_data)
{
	struct stwlc33_dev_info *di = dev_data;

	if (!di)
		return true;

	if (di->ept_type & STWLC33_TX_EPT_CEP_TIMEOUT) {
		di->ept_type &= ~STWLC33_TX_EPT_CEP_TIMEOUT;
		hwlog_info("[check_rx_disconnect] RX disconnect\n");
		return true;
	}

	return false;
}

bool stwlc33_tx_in_tx_mode(void *dev_data)
{
	int ret;
	u8 mode = 0;
	struct stwlc33_dev_info *di = dev_data;

	ret = stwlc33_get_mode(di, &mode);
	if (ret) {
		hwlog_err("in_tx_mode: get mode failed\n");
		return false;
	}
	if (mode & STWLC33_TX_WPCMODE)
		return true;

	return false;
}

static int stwlc33_tx_set_tx_open_flag(bool enable, void *dev_data)
{
	return 0;
}

static void stwlc33_tx_show_ept_type(u16 ept)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(g_stwlc33_tx_ept_name); i++) {
		if (ept & BIT(i))
			hwlog_info("[ept_type] %s\n", g_stwlc33_tx_ept_name[i]);
	}
}

static int stwlc33_tx_get_ept_type(struct stwlc33_dev_info *di, u16 *ept_type)
{
	int ret;
	u16 data = 0;

	if (!ept_type) {
		hwlog_err("get_ept_type: ept_type is null\n");
		return -EINVAL;
	}

	ret = stwlc33_read_word(di, STWLC33_TX_EPT_TYPE_ADDR, &data);
	if (ret) {
		hwlog_err("get_ept_type: read failed\n");
		return ret;
	}
	hwlog_info("[get_ept_type] EPT type=0x%04x", *ept_type);
	stwlc33_tx_show_ept_type(data);
	*ept_type = data;

	ret = stwlc33_write_word(di, STWLC33_TX_EPT_TYPE_ADDR, 0);
	if (ret) {
		hwlog_err("get_ept_type: write failed\n");
		return ret;
	}

	return 0;
}

static int stwlc33_tx_set_vset(int tx_vset, void *dev_data)
{
	struct stwlc33_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	if (!di->support_tx_adjust_vin)
		return 0;

	if (tx_vset == STWLC33_TX_VOLT_5V5)
		return stwlc33_write_byte(di, STWLC33_TX_GPIO_B2_ADDR, STWLC33_TX_GPIO_OPEN);
	else if (tx_vset == STWLC33_TX_VOLT_4V8)
		return stwlc33_write_byte(di, STWLC33_TX_GPIO_B2_ADDR, STWLC33_TX_GPIO_PD);

	hwlog_err("set_vset: para err\n");
	return -EINVAL;
}

static int stwlc33_tx_set_fod_coef(u16 pl_th, u8 pl_cnt, void *dev_data)
{
	return 0;
}

static void stwlc33_tx_handle_ept(struct stwlc33_dev_info *di)
{
	int ret;

	ret = stwlc33_tx_get_ept_type(di, &di->ept_type);
	if (ret) {
		hwlog_err("handle_ept: get tx ept type failed\n");
		return;
	}
	switch (di->ept_type) {
	case STWLC33_TX_EPT_CEP_TIMEOUT:
		di->ept_type &= ~STWLC33_TX_EPT_CEP_TIMEOUT;
		power_event_bnc_notify(stwlc33_tx_get_bnt_wltx_type(di->ic_type),
			POWER_NE_WLTX_CEP_TIMEOUT, NULL);
		break;
	case STWLC33_TX_EPT_FOD:
		di->ept_type &= ~STWLC33_TX_EPT_FOD;
		power_event_bnc_notify(stwlc33_tx_get_bnt_wltx_type(di->ic_type),
			POWER_NE_WLTX_TX_FOD, NULL);
		break;
	case STWLC33_TX_EPT_OVP:
		di->ept_type &= ~STWLC33_TX_EPT_OVP;
		power_event_bnc_notify(stwlc33_tx_get_bnt_wltx_type(di->ic_type),
			POWER_NE_WLTX_OVP, NULL);
		break;
	case STWLC33_TX_EPT_OCP:
		di->ept_type &= ~STWLC33_TX_EPT_OCP;
		power_event_bnc_notify(stwlc33_tx_get_bnt_wltx_type(di->ic_type),
			POWER_NE_WLTX_OCP, NULL);
		break;
	default:
		break;
	}
}

static int stwlc33_tx_clear_interrupt(struct stwlc33_dev_info *di, u16 itr)
{
	int ret;

	ret = stwlc33_write_word(di, STWLC33_TX_INT_CLEAR_ADDR, itr);
	if (ret) {
		hwlog_err("clear_interrupt: write failed\n");
		return ret;
	}

	ret = stwlc33_write_word_mask(di, STWLC33_CMD3_ADDR, STWLC33_CMD3_TX_CLRINT_MASK,
		STWLC33_CMD3_TX_CLRINT_SHIFT, STWLC33_CMD3_VAL);
	if (ret) {
		hwlog_err("clear_interrupt: write failed\n");
		return ret;
	}

	return 0;
}

static void stwlc33_tx_ask_pkt_handler(struct stwlc33_dev_info *di)
{
	if (di->irq_val & STWLC33_TX_STATUS_GET_SS) {
		di->irq_val &= ~STWLC33_TX_STATUS_GET_SS;
		if (di->g_val.qi_hdl && di->g_val.qi_hdl->hdl_qi_ask_pkt)
			di->g_val.qi_hdl->hdl_qi_ask_pkt(di);
	}

	if (di->irq_val & STWLC33_TX_STATUS_GET_ID) {
		di->irq_val &= ~STWLC33_TX_STATUS_GET_ID;
		if (di->g_val.qi_hdl && di->g_val.qi_hdl->hdl_qi_ask_pkt)
			di->g_val.qi_hdl->hdl_qi_ask_pkt(di);
	}

	if (di->irq_val & STWLC33_TX_STATUS_GET_CFG) {
		di->irq_val &= ~STWLC33_TX_STATUS_GET_CFG;
		if (di->g_val.qi_hdl && di->g_val.qi_hdl->hdl_qi_ask_pkt)
			di->g_val.qi_hdl->hdl_qi_ask_pkt(di);
		power_event_bnc_notify(stwlc33_tx_get_bnt_wltx_type(di->ic_type),
			POWER_NE_WLTX_GET_CFG, NULL);
	}

	if (di->irq_val & STWLC33_TX_STATUS_GET_PPP) {
		di->irq_val &= ~STWLC33_TX_STATUS_GET_PPP;
		if (di->g_val.qi_hdl && di->g_val.qi_hdl->hdl_non_qi_ask_pkt)
			di->g_val.qi_hdl->hdl_non_qi_ask_pkt(di);
	}
}

static void stwlc33_tx_show_irq(u32 intr)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(g_stwlc33_tx_irq_name); i++) {
		if (intr & BIT(i))
			hwlog_info("[tx_irq] %s\n", g_stwlc33_tx_irq_name[i]);
	}
}

static int stwlc33_tx_get_interrupt(struct stwlc33_dev_info *di, u16 *intr)
{
	int ret;

	ret = stwlc33_read_word(di, STWLC33_TX_INT_STATUS_ADDR, intr);
	if (ret)
		return ret;

	hwlog_info("[get_interrupt] interrupt 0x%04x\n", *intr);
	stwlc33_tx_show_irq(*intr);

	return 0;
}

static void stwlc33_tx_mode_irq_recheck(struct stwlc33_dev_info *di)
{
	int ret;
	u16 irq_value = 0;

	if (gpio_get_value(di->gpio_int))
		return;

	ret = stwlc33_read_word(di, STWLC33_TX_INT_STATUS_ADDR, &irq_value);
	if (ret)
		return;

	hwlog_info("[irq_recheck] gpio_int low,clear interrupt,irq_value=0x%x\n", irq_value);
	stwlc33_tx_clear_interrupt(di, POWER_MASK_WORD);
}

void stwlc33_tx_mode_irq_handler(struct stwlc33_dev_info *di)
{
	int ret;

	ret = stwlc33_tx_get_interrupt(di, &di->irq_val);
	if (ret) {
		hwlog_err("irq_handler: read int status failed,clear\n");
		stwlc33_tx_clear_interrupt(di, POWER_MASK_WORD);
		goto func_end;
	}

	stwlc33_tx_clear_interrupt(di, di->irq_val);

	stwlc33_tx_ask_pkt_handler(di);

	if (di->irq_val & STWLC33_TX_STATUS_START_DPING) {
		di->irq_val &= ~STWLC33_TX_STATUS_START_DPING;
		power_event_bnc_notify(stwlc33_tx_get_bnt_wltx_type(di->ic_type),
			POWER_NE_WLTX_PING_RX, NULL);
	}

	if (di->irq_val & STWLC33_TX_STATUS_EPT_TYPE) {
		di->irq_val &= ~STWLC33_TX_STATUS_EPT_TYPE;
		stwlc33_tx_handle_ept(di);
	}

func_end:
	stwlc33_tx_mode_irq_recheck(di);
}

static struct wltx_ic_ops g_stwlc33_tx_ic_ops = {
	.get_dev_node           = stwlc33_dts_dev_node,
	.fw_update              = stwlc33_tx_check_fwupdate,
	.chip_init              = stwlc33_tx_chip_init,
	.chip_reset             = stwlc33_chip_reset,
	.chip_enable            = stwlc33_chip_enable,
	.mode_enable            = stwlc33_tx_enable_tx_mode,
	.activate_chip          = stwlc33_tx_activate_chip,
	.set_open_flag          = stwlc33_tx_set_tx_open_flag,
	.set_stop_cfg           = stwlc33_tx_stop_config,
	.is_rx_discon           = stwlc33_tx_check_rx_disconnect,
	.is_in_tx_mode          = stwlc33_tx_in_tx_mode,
	.is_in_rx_mode          = NULL,
	.get_vrect              = stwlc33_tx_get_vrect,
	.get_vin                = stwlc33_tx_get_vin,
	.get_iin                = stwlc33_tx_get_iin,
	.get_temp               = stwlc33_tx_get_temp,
	.get_fop                = stwlc33_tx_get_fop,
	.get_cep                = NULL,
	.get_duty               = NULL,
	.get_ptx                = NULL,
	.get_prx                = NULL,
	.get_ploss              = NULL,
	.get_ploss_id           = NULL,
	.get_ping_freq          = stwlc33_tx_get_ping_freq,
	.get_ping_interval      = stwlc33_tx_get_ping_interval,
	.get_min_fop            = stwlc33_tx_get_min_fop,
	.get_max_fop            = stwlc33_tx_get_max_fop,
	.set_ping_freq          = stwlc33_tx_set_ping_freq,
	.set_ping_interval      = stwlc33_tx_set_ping_interval,
	.set_min_fop            = stwlc33_tx_set_min_fop,
	.set_max_fop            = stwlc33_tx_set_max_fop,
	.set_ilim               = NULL,
	.set_vset               = stwlc33_tx_set_vset,
	.set_fod_coef           = stwlc33_tx_set_fod_coef,
	.set_rp_dm_to           = NULL,
};

int stwlc33_tx_ops_register(struct wltrx_ic_ops *ops, struct stwlc33_dev_info *di)
{
	if (!ops || !di)
		return -ENODEV;

	ops->tx_ops = kzalloc(sizeof(*(ops->tx_ops)), GFP_KERNEL);
	if (!ops->tx_ops)
		return -ENODEV;

	memcpy(ops->tx_ops, &g_stwlc33_tx_ic_ops, sizeof(g_stwlc33_tx_ic_ops));
	ops->tx_ops->dev_data = (void *)di;
	return wltx_ic_ops_register(ops->tx_ops, di->ic_type);
}
