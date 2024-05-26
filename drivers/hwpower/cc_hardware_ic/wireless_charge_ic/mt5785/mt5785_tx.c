// SPDX-License-Identifier: GPL-2.0
/*
 * mt5785_tx.c
 *
 * mt5785 tx driver
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

#define HWLOG_TAG wireless_mt5785_tx
HWLOG_REGIST();

static const char * const g_mt5785_tx_irq_name[] = {
	/* [n]: n means bit in registers */
	[0]  = "chip reset",
	[8]  = "ss_pkt",
	[9]  = "id_pkt",
	[10] = "cfg_pkt",
	[11] = "power_trans",
	[12] = "power_remove",
	[13] = "tx_stop_work",
	[14] = "tx_start_work",
	[15] = "ask_rev",
	[16] = "other_ac",
	[17] = "ept",
	[18] = "ping",
	[19] = "ovp",
	[20] = "ocp",
	[21] = "fsk_pp_ask",
	[22] = "fsk_done",
	[23] = "fod",
	[24] = "rpp_timeout",
	[25] = "tx_rpp_err",
	[26] = "cep_timeout",
	[27] = "tx_otp",
	[28] = "tx_lvp",
	[29] = "tx_ocp_ping",
	[30] = "tx_ping_ovp",
};

static const char * const g_mt5785_tx_ept_name[] = {
	/* [n]: n means bit in registers */
	[0]  = "tx_ept_src_ovp",
	[1]  = "tx_ept_src_ocp",
	[2]  = "tx_ept_src_otp",
	[3]  = "tx_ept_src_fod",
	[4]  = "tx_ept_src_cmd",
	[5]  = "tx_ept_src_rx",
	[6]  = "tx_ept_src_cep_timeout",
	[7]  = "tx_ept_src_rpp_timeout",
	[8]  = "tx_ept_src_rx_rst",
	[9]  = "tx_ept_src_sys_err",
	[10] = "tx_ept_src_ss_timeout",
	[11] = "tx_ept_src_ss",
	[12] = "tx_ept_src_id",
	[13] = "tx_ept_src_cfg",
	[14] = "tx_ept_src_cfg_cnt",
	[15] = "tx_ept_src_pch",
	[16] = "tx_ept_src_xid",
	[17] = "tx_ept_src_nego",
	[18] = "tx_ept_src_nego_timeout",
};

static bool mt5785_tx_is_in_tx_mode(void *dev_data)
{
	int ret;
	u32 mode = 0;

	ret = mt5785_read_dword(dev_data, MT5785_TX_SYS_MODE_ADDR, &mode);
	if (ret) {
		hwlog_err("is_in_tx_mode: get op_mode failed\n");
		return false;
	}

	if (mode & MT5785_TX_SYS_MODE_TX)
		return true;

	return false;
}

static bool mt5785_tx_is_in_rx_mode(void *dev_data)
{
	int ret;
	u32 mode = 0;

	ret = mt5785_read_dword(dev_data, MT5785_TX_SYS_MODE_ADDR, &mode);
	if (ret) {
		hwlog_err("is_in_rx_mode: get op_mode failed\n");
		return false;
	}

	if (mode & MT5785_TX_SYS_MODE_RX)
		return true;

	return false;
}

static int mt5785_tx_set_tx_open_flag(bool enable, void *dev_data)
{
	struct mt5785_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	di->g_val.tx_open_flag = enable;
	return 0;
}

static void mt5785_tx_chip_reset(void *dev_data)
{
	int ret;

	ret = mt5785_write_byte_mask(dev_data, MT5785_TX_CMD_ADDR,
		MT5785_TX_CMD_CHIP_RST_MASK, MT5785_TX_CMD_CHIP_RST_SHIFT,
		MT5785_TX_CMD_VALUE);
	if (ret) {
		hwlog_err("chip_reset: failed\n");
		return;
	}

	hwlog_info("[chip_reset] succ\n");
}

static int mt5785_tx_get_full_bridge_ith(u16 *ith, void *dev_data)
{
	return 0;
}

static int mt5785_tx_set_bridge(unsigned int v_ask, unsigned int type, void *dev_data)
{
	return 0;
}

static bool mt5785_tx_check_rx_disconnect(void *dev_data)
{
	struct mt5785_dev_info *di = dev_data;

	if (!di)
		return true;

	if (di->tx_ept_type & MT5785_TX_EPT_CEP_TIMEOUT) {
		di->tx_ept_type &= ~MT5785_TX_EPT_CEP_TIMEOUT;
		hwlog_info("[check_rx_disconnect] rx disconnect\n");
		return true;
	}

	return false;
}

static int mt5785_tx_get_ping_interval(u16 *ping_interval, void *dev_data)
{
	return mt5785_read_word(dev_data, MT5785_TX_PINT_INTERVAL_ADDR, ping_interval);
}

static int mt5785_tx_set_ping_interval(u16 ping_interval, void *dev_data)
{
	if ((ping_interval < MT5785_TX_PING_INTERVAL_MIN) ||
		(ping_interval > MT5785_TX_PING_INTERVAL_MAX)) {
		hwlog_err("set_ping_interval: para out of range\n");
		return -EINVAL;
	}

	return mt5785_write_word(dev_data, MT5785_TX_PINT_INTERVAL_ADDR, ping_interval);
}

static int mt5785_tx_get_ping_freq(u16 *ping_freq, void *dev_data)
{
	int ret;

	ret = mt5785_read_word(dev_data, MT5785_TX_PING_FREQ_ADDR, ping_freq);
	*ping_freq = *ping_freq / MT5785_TX_PING_FREQ_STEP;
	return ret;
}

static int mt5785_tx_set_ping_freq(u16 ping_freq, void *dev_data)
{
	if ((ping_freq < MT5785_TX_PING_FREQ_MIN) ||
		(ping_freq > MT5785_TX_PING_FREQ_MAX)) {
		hwlog_err("set_ping_frequency: para out of range\n");
		return -EINVAL;
	}

	ping_freq = ping_freq * MT5785_TX_PING_FREQ_STEP;
	return mt5785_write_word(dev_data, MT5785_TX_PING_FREQ_ADDR, ping_freq);
}

static int mt5785_tx_get_min_fop(u16 *fop, void *dev_data)
{
	int ret;

	ret = mt5785_read_word(dev_data, MT5785_TX_MIN_OP_FREQ_ADDR, fop);
	*fop = *fop / MT5785_TX_FOP_STEP;
	return ret;
}

static int mt5785_tx_set_min_fop(u16 fop, void *dev_data)
{
	if ((fop < MT5785_TX_MIN_FOP) || (fop > MT5785_TX_MAX_FOP)) {
		hwlog_err("set_min_fop: para out of range\n");
		return -EINVAL;
	}

	fop = fop * MT5785_TX_FOP_STEP;
	return mt5785_write_word(dev_data, MT5785_TX_MIN_OP_FREQ_ADDR, fop);
}

static int mt5785_tx_get_max_fop(u16 *fop, void *dev_data)
{
	int ret;

	ret = mt5785_read_word(dev_data, MT5785_TX_MAX_OP_FREQ_ADDR, fop);
	*fop = *fop / MT5785_TX_FOP_STEP;
	return ret;
}

static int mt5785_tx_set_max_fop(u16 fop, void *dev_data)
{
	if ((fop < MT5785_TX_MIN_FOP) || (fop > MT5785_TX_MAX_FOP)) {
		hwlog_err("set_max_fop: para out of range\n");
		return -EINVAL;
	}

	fop *= MT5785_TX_FOP_STEP;
	return mt5785_write_word(dev_data, MT5785_TX_MAX_OP_FREQ_ADDR, fop);
}

static int mt5785_tx_get_fop(u16 *fop, void *dev_data)
{
	int ret;

	ret = mt5785_read_word(dev_data, MT5785_TX_FOP_ADDR, fop);
	*fop = *fop / MT5785_TX_FOP_STEP;
	return ret;
}

static int mt5785_tx_get_cep(s8 *cep, void *dev_data)
{
	int ret;

	ret = mt5785_read_byte(dev_data, MT5785_TX_CEP_VAL_ADDR, cep);
	*cep = *cep / MT5785_TX_CEP_VAL_STEP;
	return ret;
}

static int mt5785_tx_get_duty(u8 *duty, void *dev_data)
{
	return 0;
}

static int mt5785_tx_get_ptx(u32 *ptx, void *dev_data)
{
	return 0;
}

static int mt5785_tx_get_prx(u32 *prx, void *dev_data)
{
	return 0;
}

static int mt5785_tx_get_ploss(s32 *ploss, void *dev_data)
{
	return 0;
}

static int mt5785_tx_get_ploss_id(u8 *id, void *dev_data)
{
	return 0;
}

static int mt5785_tx_get_temp(s16 *chip_temp, void *dev_data)
{
	return mt5785_read_word(dev_data, MT5785_TX_TEMP_ADDR, chip_temp);
}

static int mt5785_tx_get_vin(u16 *tx_vin, void *dev_data)
{
	return mt5785_read_word(dev_data, MT5785_TX_VIN_ADDR, tx_vin);
}

static int mt5785_tx_get_vrect(u16 *tx_vrect, void *dev_data)
{
	return mt5785_read_word(dev_data, MT5785_TX_VBRG_ADDR, tx_vrect);
}

static int mt5785_tx_get_iin(u16 *tx_iin, void *dev_data)
{
	return mt5785_read_word(dev_data, MT5785_TX_IIN_ADDR, tx_iin);
}

static int mt5785_tx_set_ilimit(u16 tx_ilim, void *dev_data)
{
	if ((tx_ilim < MT5785_TX_ILIM_MIN) || (tx_ilim > MT5785_TX_ILIM_MAX)) {
		hwlog_err("set_ilimit: out of range\n");
		return -EINVAL;
	}

	return mt5785_write_word(dev_data, MT5785_TX_OCP_ADDR, tx_ilim);
}

static int mt5785_tx_init_fod_coef(struct mt5785_dev_info *di)
{
	return 0;
}

static int mt5785_tx_stop_config(void *dev_data)
{
	struct mt5785_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	di->g_val.tx_stop_chrg = true;
	return 0;
}

static int mt5785_sw2tx(struct mt5785_dev_info *di)
{
	int ret;
	int i;
	u32 mode = 0;

	/* 500ms=25ms*20 timeout for switching to tx mode */
	for (i = 0; i < 20; i++) {
		power_msleep(DT_MSLEEP_25MS, 0, NULL);
		ret = mt5785_get_mode(di, &mode);
		if (ret) {
			hwlog_err("sw2tx: get mode failed\n");
			continue;
		}

		if (mode & MT5785_TX_SYS_MODE_TX) {
			hwlog_info("[sw2tx] succ, cnt=%d\n", i);
			return 0;
		}

		ret = mt5785_write_dword_mask(di, MT5785_TX_CMD_ADDR,
			MT5785_TX_CMD_ENTER_TX_MASK, MT5785_TX_CMD_ENTER_TX_SHIFT,
			MT5785_TX_CMD_VALUE);
		if (ret)
			hwlog_err("sw2tx: write cmd(sw2tx) failed\n");
	}
	hwlog_err("sw2tx: failed, cnt=%d\n", i);
	return -ENXIO;
}

static void mt5785_tx_select_init_para(struct mt5785_dev_info *di,
	unsigned int client)
{
	switch (client) {
	case WLTX_CLIENT_UI:
		di->tx_init_para.ping_freq = di->tx_ping_freq;
		di->tx_init_para.ping_interval = MT5785_TX_PING_INTERVAL;
		break;
	case WLTX_CLIENT_COIL_TEST:
		di->tx_init_para.ping_freq = MT5785_TX_PING_FREQ_COIL_TEST;
		di->tx_init_para.ping_interval = MT5785_TX_PING_INTERVAL_COIL_TEST;
		break;
	case WLTX_CLIENT_BAT_HEATING:
		di->tx_init_para.ping_freq = MT5785_TX_PING_FREQ_BAT_HEATING;
		di->tx_init_para.ping_interval = MT5785_TX_PING_INTERVAL_BAT_HEATING;
		break;
	default:
		di->tx_init_para.ping_freq = di->tx_ping_freq;
		di->tx_init_para.ping_interval = MT5785_TX_PING_INTERVAL;
		break;
	}
}

static int mt5785_tx_set_init_para(struct mt5785_dev_info *di)
{
	int ret;

	ret = mt5785_sw2tx(di);
	if (ret) {
		hwlog_err("set_init_para: sw2tx failed\n");
		return ret;
	}

	ret = mt5785_write_word(di, MT5785_TX_OCP1_ADDR, MT5785_TX_OCP1_TH);
	ret += mt5785_write_word(di, MT5785_TX_OVP_ADDR, MT5785_TX_OVP_TH);
	ret += mt5785_write_dword(di, MT5785_TX_INT_EN_ADDR, MT5785_TX_INT_EN_ALL);
	ret += mt5785_write_word(di, MT5785_TX_LVP_ADDR, MT5785_TX_LVP_TH);
	ret += mt5785_tx_init_fod_coef(di);
	ret += mt5785_tx_set_ping_freq(di->tx_init_para.ping_freq, di);
	ret += mt5785_tx_set_min_fop(MT5785_TX_MIN_FOP, di);
	ret += mt5785_tx_set_max_fop(MT5785_TX_MAX_FOP, di);
	ret += mt5785_tx_set_ping_interval(di->tx_init_para.ping_interval, di);
	if (ret) {
		hwlog_err("set_init_para: write failed\n");
		return -EIO;
	}

	return 0;
}

static int mt5785_tx_chip_init(unsigned int client, void *dev_data)
{
	struct mt5785_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	di->irq_cnt = 0;
	di->g_val.irq_abnormal = false;
	di->g_val.tx_stop_chrg = false;
	mt5785_enable_irq(di);

	mt5785_tx_select_init_para(di, client);
	return mt5785_tx_set_init_para(di);
}

static int mt5785_tx_enable_tx_mode(bool enable, void *dev_data)
{
	int ret;

	if (enable)
		ret = mt5785_write_dword_mask(dev_data, MT5785_TX_CMD_ADDR,
			MT5785_TX_CMD_START_WORK_MASK, MT5785_TX_CMD_START_WORK_SHIFT,
			MT5785_TX_CMD_VALUE);
	else
		ret = mt5785_write_dword_mask(dev_data, MT5785_TX_CMD_ADDR,
			MT5785_TX_CMD_STOP_WORK_MASK, MT5785_TX_CMD_STOP_WORK_SHIFT,
			MT5785_TX_CMD_VALUE);

	if (ret)
		hwlog_err("%s tx_mode failed\n", enable ? "enable" : "disable");

	return ret;
}

static void mt5785_tx_show_ept_type(u32 ept)
{
	unsigned int i;

	hwlog_info("[show_ept_type] type=0x%02x", ept);
	for (i = 0; i < ARRAY_SIZE(g_mt5785_tx_ept_name); i++) {
		if (ept & BIT(i))
			hwlog_info("[show_ept_type] %s\n", g_mt5785_tx_ept_name[i]);
	}
}

static int mt5785_tx_get_ept_type(struct mt5785_dev_info *di, u32 *ept)
{
	int ret;

	if (!ept) {
		hwlog_err("get_ept_type: para null\n");
		return -EINVAL;
	}

	ret = mt5785_read_dword(di, MT5785_TX_EPT_TYPE_ADDR, ept);
	if (ret) {
		hwlog_err("get_ept_type: read failed\n");
		return ret;
	}

	mt5785_tx_show_ept_type(*ept);
	return 0;
}

static int mt5785_tx_clear_ept_src(struct mt5785_dev_info *di)
{
	return mt5785_write_dword(di, MT5785_TX_EPT_TYPE_ADDR, MT5785_TX_EPT_CLEAR);
}

static void mt5785_tx_ept_handler(struct mt5785_dev_info *di)
{
	int ret;

	ret = mt5785_tx_get_ept_type(di, &di->tx_ept_type);
	ret += mt5785_tx_clear_ept_src(di);
	if (ret)
		return;

	switch (di->tx_ept_type) {
	case MT5785_TX_EPT_RX:
	case MT5785_TX_EPT_CEP_TIMEOUT:
	case MT5785_TX_EPT_RX_RST:
		di->tx_ept_type &= ~MT5785_TX_EPT_CEP_TIMEOUT;
		power_event_bnc_notify(POWER_BNT_WLTX, POWER_NE_WLTX_CEP_TIMEOUT, NULL);
		break;
	case MT5785_TX_EPT_FOD:
		di->tx_ept_type &= ~MT5785_TX_EPT_FOD;
		power_event_bnc_notify(POWER_BNT_WLTX, POWER_NE_WLTX_TX_FOD, NULL);
		break;
	default:
		break;
	}
}

static int mt5785_tx_clear_irq(struct mt5785_dev_info *di, u32 itr)
{
	int ret;

	ret = mt5785_write_dword(di, MT5785_TX_INT_CLEAR_ADDR, itr);
	ret += mt5785_write_dword_mask(di, MT5785_TX_CMD_ADDR,
		MT5785_TX_CMD_CLEAR_INT_MASK, MT5785_TX_CMD_CLEAR_INT_SHIFT, MT5785_TX_CMD_VALUE);
	return ret;
}

static void mt5785_tx_ask_pkt_handler(struct mt5785_dev_info *di)
{
	if (di->irq_val & MT5785_TX_INT_SS_PKG_RCVD) {
		di->irq_val &= ~MT5785_TX_INT_SS_PKG_RCVD;
		if (di->g_val.qi_hdl && di->g_val.qi_hdl->hdl_qi_ask_pkt)
			di->g_val.qi_hdl->hdl_qi_ask_pkt(di);
	}

	if (di->irq_val & MT5785_TX_INT_ID_PKT_RCVD) {
		di->irq_val &= ~MT5785_TX_INT_ID_PKT_RCVD;
		if (di->g_val.qi_hdl && di->g_val.qi_hdl->hdl_qi_ask_pkt)
			di->g_val.qi_hdl->hdl_qi_ask_pkt(di);
	}

	if (di->irq_val & MT5785_TX_INT_CFG_PKT_RCVD) {
		di->irq_val &= ~MT5785_TX_INT_CFG_PKT_RCVD;
		if (di->g_val.qi_hdl && di->g_val.qi_hdl->hdl_qi_ask_pkt)
			di->g_val.qi_hdl->hdl_qi_ask_pkt(di);
		power_event_bnc_notify(POWER_BNT_WLTX, POWER_NE_WLTX_GET_CFG, NULL);
	}

	if (di->irq_val & MT5785_TX_INT_ASK_PKT_RCVD) {
		di->irq_val &= ~MT5785_TX_INT_ASK_PKT_RCVD;
		if (di->g_val.qi_hdl && di->g_val.qi_hdl->hdl_non_qi_ask_pkt)
			di->g_val.qi_hdl->hdl_non_qi_ask_pkt(di);
	}
}

static void mt5785_tx_show_irq(u32 intr)
{
	unsigned int i;

	hwlog_info("[tx_show_irq] irq=0x%04x\n", intr);
	for (i = 0; i < ARRAY_SIZE(g_mt5785_tx_irq_name); i++) {
		if (intr & BIT(i))
			hwlog_info("[tx_show_irq] %s\n", g_mt5785_tx_irq_name[i]);
	}
}

static int mt5785_tx_get_interrupt(struct mt5785_dev_info *di, u32 *intr)
{
	int ret;

	ret = mt5785_read_dword(di, MT5785_TX_INT_FLAG_ADDR, intr);
	if (ret)
		return ret;

	mt5785_tx_show_irq(*intr);
	return 0;
}

static void mt5785_tx_mode_irq_recheck(struct mt5785_dev_info *di)
{
	int ret;
	u32 irq_val = 0;

	if (gpio_get_value(di->gpio_int))
		return;

	hwlog_info("[tx_mode_irq_recheck] gpio_int low, re-check irq\n");
	ret = mt5785_tx_get_interrupt(di, &irq_val);
	if (ret)
		return;

	mt5785_tx_clear_irq(di, MT5785_TX_CLEAR_ALL_INT);
}

void mt5785_tx_mode_irq_handler(struct mt5785_dev_info *di)
{
	int ret;

	if (!di)
		return;

	ret = mt5785_tx_get_interrupt(di, &di->irq_val);
	if (ret) {
		hwlog_err("irq_handler: get irq failed, clear\n");
		mt5785_tx_clear_irq(di, MT5785_TX_CLEAR_ALL_INT);
		goto rechk_irq;
	}

	mt5785_tx_clear_irq(di, di->irq_val);
	mt5785_tx_ask_pkt_handler(di);

	if (di->irq_val & MT5785_TX_INT_START_PING) {
		di->irq_val &= ~MT5785_TX_INT_START_PING;
		power_event_bnc_notify(POWER_BNT_WLTX, POWER_NE_WLTX_PING_RX, NULL);
	}
	if (di->irq_val & MT5785_TX_INT_EPT) {
		di->irq_val &= ~MT5785_TX_INT_EPT;
		mt5785_tx_ept_handler(di);
	}
	if (di->irq_val & MT5785_TX_INT_AC_DET) {
		di->irq_val &= ~MT5785_TX_INT_AC_DET;
		power_event_bnc_notify(POWER_BNT_WLTX, POWER_NE_WLTX_RCV_DPING, NULL);
	}
	if (di->irq_val & MT5785_TX_INT_RPP_TIMEOUT) {
		di->irq_val &= ~MT5785_TX_INT_RPP_TIMEOUT;
		power_event_bnc_notify(POWER_BNT_WLTX, POWER_NE_WLTX_RP_DM_TIMEOUT, NULL);
	}

rechk_irq:
	mt5785_tx_mode_irq_recheck(di);
}

static struct wltx_ic_ops g_mt5785_tx_ic_ops = {
	.fw_update              = mt5785_fw_sram_update,
	.chip_init              = mt5785_tx_chip_init,
	.chip_reset             = mt5785_tx_chip_reset,
	.chip_enable            = mt5785_chip_enable,
	.mode_enable            = mt5785_tx_enable_tx_mode,
	.set_open_flag          = mt5785_tx_set_tx_open_flag,
	.set_stop_cfg           = mt5785_tx_stop_config,
	.is_rx_discon           = mt5785_tx_check_rx_disconnect,
	.is_in_tx_mode          = mt5785_tx_is_in_tx_mode,
	.is_in_rx_mode          = mt5785_tx_is_in_rx_mode,
	.get_vrect              = mt5785_tx_get_vrect,
	.get_vin                = mt5785_tx_get_vin,
	.get_iin                = mt5785_tx_get_iin,
	.get_temp               = mt5785_tx_get_temp,
	.get_fop                = mt5785_tx_get_fop,
	.get_cep                = mt5785_tx_get_cep,
	.get_duty               = mt5785_tx_get_duty,
	.get_ptx                = mt5785_tx_get_ptx,
	.get_prx                = mt5785_tx_get_prx,
	.get_ploss              = mt5785_tx_get_ploss,
	.get_ploss_id           = mt5785_tx_get_ploss_id,
	.get_ping_freq          = mt5785_tx_get_ping_freq,
	.get_ping_interval      = mt5785_tx_get_ping_interval,
	.get_min_fop            = mt5785_tx_get_min_fop,
	.get_max_fop            = mt5785_tx_get_max_fop,
	.get_full_bridge_ith    = mt5785_tx_get_full_bridge_ith,
	.set_ping_freq          = mt5785_tx_set_ping_freq,
	.set_ping_interval      = mt5785_tx_set_ping_interval,
	.set_min_fop            = mt5785_tx_set_min_fop,
	.set_max_fop            = mt5785_tx_set_max_fop,
	.set_ilim               = mt5785_tx_set_ilimit,
	.set_bridge             = mt5785_tx_set_bridge,
};

int mt5785_tx_ops_register(struct wltrx_ic_ops *ops, struct mt5785_dev_info *di)
{
	if (!ops || !di)
		return -ENODEV;

	ops->tx_ops = kzalloc(sizeof(*(ops->tx_ops)), GFP_KERNEL);
	if (!ops->tx_ops)
		return -ENODEV;

	memcpy(ops->tx_ops, &g_mt5785_tx_ic_ops, sizeof(g_mt5785_tx_ic_ops));
	ops->tx_ops->dev_data = (void *)di;
	return wltx_ic_ops_register(ops->tx_ops, di->ic_type);
}
