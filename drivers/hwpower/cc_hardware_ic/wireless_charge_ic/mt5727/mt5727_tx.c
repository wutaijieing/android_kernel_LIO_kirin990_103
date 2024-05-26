// SPDX-License-Identifier: GPL-2.0
/*
 * mt5727_tx.c
 *
 * mt5727 tx driver
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

#include "mt5727.h"

#define HWLOG_TAG wireless_mt5727_tx
HWLOG_REGIST();

static const char * const g_mt5727_tx_irq_name[] = {
	/* [n]: n means bit in registers */
	[0]  = "tx_ss_pkt_rcvd",
	[1]  = "tx_id_pkt_rcvd",
	[2]  = "tx_cfg_pkt_rcvd",
	[4]  = "tx_ocp",
	[5]  = "tx_fod_det",
	[6]  = "tx_rmv_pwr",
	[8]  = "tx_pwr_trans",
	[10] = "tx_ovp",
	[11] = "tx_pp_pkt_rcvd",
	[13] = "tx_disable",
	[14] = "tx_enable",
	[15] = "tx_ept_pkt_rcvd",
	[17] = "tx_start_ping",
	[18] = "tx_cep_timeout",
	[19] = "tx_rpp_timeout",
	[22] = "tx_ping_ocp",
	[23] = "tx_ping_ovp",
};

static const char * const g_mt5727_tx_ept_name[] = {
	/* [n]: n means bit in registers */
	[0]  = "tx_ept_src_cmd",
	[1]  = "tx_ept_src_ss",
	[2]  = "tx_ept_src_id",
	[3]  = "tx_ept_src_xid",
	[4]  = "tx_ept_src_cfg_cnt",
	[8]  = "tx_ept_src_cep_timeout",
	[9]  = "tx_ept_src_rpp_timeout",
	[12] = "tx_ept_src_lvp",
	[14] = "tx_ept_src_otp",
	[15] = "tx_ept_src_lcp",
	[16] = "tx_ept_src_cfg",
	[17] = "tx_ept_src_ping_ovp",
};

static unsigned int mt5727_tx_get_bnt_wltx_type(int ic_type)
{
	return ic_type == WLTRX_IC_AUX ? POWER_BNT_WLTX_AUX : POWER_BNT_WLTX;
}

static bool mt5727_tx_is_tx_mode(void *dev_data)
{
	int ret;
	u32 mode = 0;
	struct mt5727_dev_info *di = dev_data;

	ret = mt5727_read_block(di, MT5727_OP_MODE_ADDR, (u8 *)&mode,
		MT5727_OP_MODE_LEN);
	if (ret) {
		hwlog_err("is_tx_mode: get op_mode failed, ret:%d\n", ret);
		return false;
	}

	return (mode & MT5727_OP_MODE_TX);
}

static bool mt5727_tx_is_rx_mode(void *dev_data)
{
	int ret;
	u32 mode = 0;
	struct mt5727_dev_info *di = dev_data;

	ret = mt5727_read_block(di, MT5727_OP_MODE_ADDR, (u8 *)&mode,
		MT5727_OP_MODE_LEN);
	if (ret) {
		hwlog_err("is_rx_mode: get rx mode failed, ret:%d\n", ret);
		return false;
	}

	return (mode & MT5727_OP_MODE_RX);
}

static int mt5727_tx_set_tx_open_flag(bool enable, void *dev_data)
{
	return 0;
}

static int mt5727_tx_get_full_bridge_ith(u16 *ith, void *dev_data)
{
	return 0;
}

static int mt5727_tx_set_vset(int tx_vset, void *dev_data)
{
	return 0;
}

static int mt5727_tx_set_bridge(unsigned int v_ask, unsigned int type, void *dev_data)
{
	return 0;
}

static bool mt5727_tx_check_rx_disconnect(void *dev_data)
{
	struct mt5727_dev_info *di = dev_data;

	if (!di)
		return true;

	if (di->ept_type & MT5727_TX_EPT_SRC_CEP_TIMEOUT) {
		di->ept_type &= ~MT5727_TX_EPT_SRC_CEP_TIMEOUT;
		hwlog_info("[check_rx_disconnect] rx disconnect\n");
		return true;
	}

	return false;
}

static int mt5727_tx_get_ping_interval(u16 *ping_interval, void *dev_data)
{
	int ret;
	u16 data = 0;
	struct mt5727_dev_info *di = dev_data;

	if (!ping_interval) {
		hwlog_err("get_ping_interval: para null\n");
		return -EINVAL;
	}

	ret = mt5727_read_word(di, MT5727_TX_PING_INTERVAL_ADDR, &data);
	if (ret) {
		hwlog_err("get_ping_interval: read failed\n");
		return ret;
	}
	*ping_interval = data / MT5727_TX_PING_INTERVAL_STEP;

	return 0;
}

static int mt5727_tx_set_ping_interval(u16 ping_interval, void *dev_data)
{
	struct mt5727_dev_info *di = dev_data;

	if ((ping_interval < MT5727_TX_PING_INTERVAL_MIN) ||
		(ping_interval > MT5727_TX_PING_INTERVAL_MAX)) {
		hwlog_err("set_ping_interval: para out of range\n");
		return -EINVAL;
	}

	return mt5727_write_word(di, MT5727_TX_PING_INTERVAL_ADDR,
		ping_interval * MT5727_TX_PING_INTERVAL_STEP);
}

static int mt5727_tx_get_ping_freq(u16 *ping_freq, void *dev_data)
{
	int ret;
	u16 data = 0;
	struct mt5727_dev_info *di = dev_data;

	if (!ping_freq) {
		hwlog_err("get_ping_frequency: para null\n");
		return -EINVAL;
	}

	ret = mt5727_read_word(di, MT5727_TX_PING_FREQ_ADDR, &data);
	if (ret) {
		hwlog_err("get_ping_frequency: read failed\n");
		return ret;
	}
	if (data != 0)
		*ping_freq = OSCP2F(data);

	return 0;
}

static int mt5727_tx_set_ping_freq(u16 ping_freq, void *dev_data)
{
	struct mt5727_dev_info *di = dev_data;

	if ((ping_freq < MT5727_TX_PING_FREQ_MIN) ||
		(ping_freq > MT5727_TX_PING_FREQ_MAX)) {
		hwlog_err("set_ping_frequency: para out of range\n");
		return -EINVAL;
	}

	return mt5727_write_word(di, MT5727_TX_PING_FREQ_ADDR,
		OSCF2P(ping_freq) * MT5727_TX_PING_STEP);
}

static int mt5727_tx_get_min_fop(u16 *fop, void *dev_data)
{
	int ret;
	u16 data = 0;
	struct mt5727_dev_info *di = dev_data;

	if (!fop) {
		hwlog_err("get_min_fop: para null\n");
		return -EINVAL;
	}

	ret = mt5727_read_word(di, MT5727_TX_MIN_FOP_ADDR, &data);
	if (ret) {
		hwlog_err("get_min_fop: read failed, ret:%d\n", ret);
		return ret;
	}
	if (data != 0)
		*fop = OSCP2F(data);

	return 0;
}

static int mt5727_tx_set_min_fop(u16 fop, void *dev_data)
{
	struct mt5727_dev_info *di = dev_data;

	if ((fop < MT5727_TX_MIN_FOP) || (fop > MT5727_TX_MAX_FOP)) {
		hwlog_err("set_min_fop: para out of range\n");
		return -EINVAL;
	}

	return mt5727_write_word(di, MT5727_TX_MIN_FOP_ADDR,
		OSCF2P(fop) * MT5727_TX_FOP_STEP);
}

static int mt5727_tx_get_max_fop(u16 *fop, void *dev_data)
{
	int ret;
	u16 data = 0;
	struct mt5727_dev_info *di = dev_data;

	if (!fop) {
		hwlog_err("get_max_fop: para null\n");
		return -EINVAL;
	}

	ret = mt5727_read_word(di, MT5727_TX_MAX_FOP_ADDR, &data);
	if (ret) {
		hwlog_err("get_max_fop: read failed\n");
		return ret;
	}
	if (data != 0)
		*fop = OSCP2F(data);

	return 0;
}

static int mt5727_tx_set_max_fop(u16 fop, void *dev_data)
{
	struct mt5727_dev_info *di = dev_data;

	if ((fop < MT5727_TX_MIN_FOP) || (fop > MT5727_TX_MAX_FOP)) {
		hwlog_err("set_max_fop: para out of range\n");
		return -EINVAL;
	}

	return mt5727_write_word(di, MT5727_TX_MAX_FOP_ADDR,
		OSCF2P(fop) * MT5727_TX_FOP_STEP);
}

static int mt5727_tx_get_fop(u16 *fop, void *dev_data)
{
	int ret;
	u16 data = 0;
	struct mt5727_dev_info *di = dev_data;

	if (!fop) {
		hwlog_err("get_fop: para null\n");
		return -EINVAL;
	}

	ret = mt5727_read_word(di, MT5727_TX_OP_FREQ_ADDR, &data);
	if (ret) {
		hwlog_err("get_fop: failed\n");
		return ret;
	}
	if (data != 0)
		*fop = OSCP2F(data);

	return 0;
}

static int mt5727_tx_get_temp(s16 *chip_temp, void *dev_data)
{
	*chip_temp = 30; /* default normal temp */

	return 0;
}

static int mt5727_tx_get_cep(s8 *cep, void *dev_data)
{
	return 0;
}

static int mt5727_tx_get_duty(u8 *duty, void *dev_data)
{
	return mt5727_read_byte(dev_data, MT5727_TX_PWM_DUTY_ADDR, duty);
}

static int mt5727_tx_get_ptx(u32 *ptx, void *dev_data)
{
	return 0;
}

static int mt5727_tx_get_prx(u32 *prx, void *dev_data)
{
	return 0;
}

static int mt5727_tx_get_ploss(s32 *ploss, void *dev_data)
{
	return 0;
}

static int mt5727_tx_get_ploss_id(u8 *id, void *dev_data)
{
	return 0;
}

static int mt5727_tx_get_vrect(u16 *tx_vrect, void *dev_data)
{
	return mt5727_read_word(dev_data, MT5727_TX_VRECT_ADDR, tx_vrect);
}

static int mt5727_tx_get_vin(u16 *tx_vin, void *dev_data)
{
	return mt5727_read_word(dev_data, MT5727_TX_VIN_ADDR, tx_vin);
}

static int mt5727_tx_get_iin(u16 *tx_iin, void *dev_data)
{
	return mt5727_read_word(dev_data, MT5727_TX_IIN_ADDR, tx_iin);
}

static int mt5727_tx_set_ilimit(u16 tx_ilim, void *dev_data)
{
	return 0;
}

static int mt5727_tx_init_fod_coef(struct mt5727_dev_info *di)
{
	int ret;

	ret = mt5727_write_word(di, MT5727_TX_PLOSS_TH0_ADDR, di->tx_fod.ploss_th0);
	ret += mt5727_write_word(di, MT5727_TX_PLOSS_CNT_ADDR, di->tx_fod.ploss_cnt);
	if (ret) {
		hwlog_err("init_fod_coef: failed\n");
		return -EIO;
	}

	return 0;
}

static int mt5727_tx_stop_config(void *dev_data)
{
	return 0;
}

static int mt5727_tx_activate_chip(void *dev_data)
{
	return 0;
}

static int mt5727_tx_set_irq_en(u32 val, void *dev_data)
{
	return mt5727_write_block(dev_data, MT5727_TX_IRQ_EN_ADDR, (u8 *)&val,
		MT5727_TX_IRQ_EN_LEN);
}

static void mt5727_tx_select_init_para(struct mt5727_dev_info *di)
{
		di->tx_init_para.ping_freq = MT5727_TX_PING_FREQ;
		di->tx_init_para.ping_interval = MT5727_TX_PING_INTERVAL;
}

static int mt5727_tx_set_init_para(struct mt5727_dev_info *di)
{
	int ret;

	ret = mt5727_write_word(di, MT5727_TX_OVP_TH_ADDR, MT5727_TX_OVP_TH);
	ret += mt5727_write_dword_mask(di, MT5727_TX_CMD_ADDR, MT5727_TX_CMD_OVP,
		MT5727_TX_CMD_OVP_SHIFT, MT5727_TX_CMD_VAL);
	ret += mt5727_write_word(di, MT5727_TX_OCP_TH_ADDR, MT5727_TX_OCP_TH);
	ret += mt5727_write_word(di, MT5727_TX_PING_OCP_TH_ADDR, di->tx_pocp_th);
	ret += mt5727_write_dword_mask(di, MT5727_TX_CMD_ADDR, MT5727_TX_CMD_OCP,
		MT5727_TX_CMD_OCP_SHIFT, MT5727_TX_CMD_VAL);
	ret += mt5727_tx_init_fod_coef(di);
	ret += mt5727_tx_set_ping_freq(di->tx_init_para.ping_freq, di);
	ret += mt5727_tx_set_min_fop(di->tx_fop.tx_min_fop, di);
	ret += mt5727_tx_set_max_fop(di->tx_fop.tx_max_fop, di);
	ret += mt5727_tx_set_ping_interval(di->tx_init_para.ping_interval, di);
	ret += mt5727_tx_set_irq_en(MT5727_TX_IRQ_EN_VAL, di);
	if (ret) {
		hwlog_err("set_init_para: write failed\n");
		return -EIO;
	}

	return 0;
}

static int mt5727_tx_chip_init(unsigned int client, void *dev_data)
{
	struct mt5727_dev_info *di = dev_data;

	if (!di)
		return -EINVAL;

	di->irq_cnt = 0;
	mt5727_enable_irq(di);

	mt5727_tx_select_init_para(di);

	return mt5727_tx_set_init_para(di);
}

static int mt5727_tx_enable_tx_mode(bool enable, void *dev_data)
{
	int ret;
	struct mt5727_dev_info *di = dev_data;

	if (enable) {
		ret = mt5727_write_dword_mask(di, MT5727_TX_CMD_ADDR,
			MT5727_TX_CMD_START_TX, MT5727_TX_CMD_START_TX_SHIFT,
			MT5727_TX_CMD_VAL);
		ret += mt5727_write_word(di, MT5727_TX_FSK_DEPTH_ADDR,
			MT5727_TX_FSK_DEPTH_OFFSET);
	} else {
		ret = mt5727_write_dword_mask(di, MT5727_TX_CMD_ADDR,
			MT5727_TX_CMD_STOP_TX, MT5727_TX_CMD_STOP_TX_SHIFT,
			MT5727_TX_CMD_VAL);
	}
	if (ret) {
		hwlog_err("%s tx_mode failed\n", enable ? "enable" : "disable");
		return ret;
	}

	return 0;
}

static void mt5727_tx_show_ept_type(u32 ept)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(g_mt5727_tx_ept_name); i++) {
		if (ept & BIT(i))
			hwlog_info("[tx_ept] %s\n", g_mt5727_tx_ept_name[i]);
	}
}

static int mt5727_tx_get_ept_type(struct mt5727_dev_info *di, u32 *ept)
{
	int ret;
	u32 data = 0;

	if (!ept) {
		hwlog_err("get_ept_type: para null\n");
		return -EINVAL;
	}

	ret = mt5727_read_block(di, MT5727_TX_EPT_SRC_ADDR, (u8 *)&data,
		MT5727_TX_EPT_SRC_LEN);
	if (ret) {
		hwlog_err("get_ept_type: read failed\n");
		return ret;
	}
	hwlog_info("[get_ept_type] type=0x%08x", data);
	mt5727_tx_show_ept_type(data);
	*ept = data;

	ret = mt5727_write_word(di, MT5727_TX_EPT_SRC_ADDR, 0);
	if (ret) {
		hwlog_err("get_ept_type: clr failed\n");
		return ret;
	}

	return 0;
}

static void mt5727_tx_ept_handler(struct mt5727_dev_info *di)
{
	int ret;

	ret = mt5727_tx_get_ept_type(di, &di->ept_type);
	if (ret)
		return;

	switch (di->ept_type) {
	case MT5727_TX_EPT_SRC_CEP_TIMEOUT:
		di->ept_type &= ~MT5727_TX_EPT_SRC_CEP_TIMEOUT;
		power_event_bnc_notify(mt5727_tx_get_bnt_wltx_type(di->ic_type),
			POWER_NE_WLTX_CEP_TIMEOUT, NULL);
		break;
	default:
		break;
	}
}

static int mt5727_tx_clear_irq(struct mt5727_dev_info *di, u32 itr)
{
	int ret;

	ret = mt5727_write_block(di, MT5727_TX_IRQ_CLR_ADDR, (u8 *)&itr,
		MT5727_TX_IRQ_CLR_LEN);
	ret += mt5727_write_byte(di, MT5727_TX_IRQ_CLR_CTRL_ADDR,
		MT5727_TX_IRQ_CLR_CTRL);
	if (ret) {
		hwlog_err("clear_irq: write failed\n");
		return ret;
	}

	return 0;
}

static void mt5727_tx_ask_pkt_handler(struct mt5727_dev_info *di)
{
	if (di->irq_val & MT5727_TX_IRQ_SS_PKG_RCVD) {
		di->irq_val &= ~MT5727_TX_IRQ_SS_PKG_RCVD;
		if (di->g_val.qi_hdl && di->g_val.qi_hdl->hdl_qi_ask_pkt)
			di->g_val.qi_hdl->hdl_qi_ask_pkt(di);
	}

	if (di->irq_val & MT5727_TX_IRQ_ID_PKT_RCVD) {
		di->irq_val &= ~MT5727_TX_IRQ_ID_PKT_RCVD;
		if (di->g_val.qi_hdl && di->g_val.qi_hdl->hdl_qi_ask_pkt)
			di->g_val.qi_hdl->hdl_qi_ask_pkt(di);
	}

	if (di->irq_val & MT5727_TX_IRQ_CFG_PKT_RCVD) {
		di->irq_val &= ~MT5727_TX_IRQ_CFG_PKT_RCVD;
		if (di->g_val.qi_hdl && di->g_val.qi_hdl->hdl_qi_ask_pkt)
			di->g_val.qi_hdl->hdl_qi_ask_pkt(di);
		power_event_bnc_notify(mt5727_tx_get_bnt_wltx_type(di->ic_type),
			POWER_NE_WLTX_GET_CFG, NULL);
	}

	if (di->irq_val & MT5727_TX_IRQ_PP_PKT_RCVD) {
		di->irq_val &= ~MT5727_TX_IRQ_PP_PKT_RCVD;
		if (di->g_val.qi_hdl && di->g_val.qi_hdl->hdl_non_qi_ask_pkt)
			di->g_val.qi_hdl->hdl_non_qi_ask_pkt(di);
	}
}

static void mt5727_tx_show_irq(u32 intr)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(g_mt5727_tx_irq_name); i++) {
		if (intr & BIT(i))
			hwlog_info("[tx_irq] %s\n", g_mt5727_tx_irq_name[i]);
	}
}

static int mt5727_tx_get_interrupt(struct mt5727_dev_info *di, u32 *intr)
{
	int ret;

	ret = mt5727_read_block(di, MT5727_TX_IRQ_ADDR, (u8 *)intr,
		MT5727_TX_IRQ_LEN);
	if (ret)
		return ret;

	hwlog_info("[get_interrupt] irq=0x%08x\n", *intr);
	mt5727_tx_show_irq(*intr);

	return 0;
}

static void mt5727_tx_mode_irq_recheck(struct mt5727_dev_info *di)
{
	int ret;
	u32 irq_val = 0;

	if (gpio_get_value(di->gpio_int))
		return;

	hwlog_info("[tx_mode_irq_recheck] gpio_int low, re-check irq\n");
	ret = mt5727_tx_get_interrupt(di, &irq_val);
	if (ret)
		return;

	mt5727_tx_clear_irq(di, MT5727_TX_IRQ_CLR_ALL);
}

void mt5727_tx_mode_irq_handler(struct mt5727_dev_info *di)
{
	int ret;

	if (!di)
		return;

	ret = mt5727_tx_get_interrupt(di, &di->irq_val);
	if (ret) {
		hwlog_err("irq_handler: get irq failed, clear\n");
		mt5727_tx_clear_irq(di, MT5727_TX_IRQ_CLR_ALL);
		goto rechk_irq;
	}

	mt5727_tx_clear_irq(di, di->irq_val);

	mt5727_tx_ask_pkt_handler(di);

	if (di->irq_val & MT5727_TX_IRQ_START_PING) {
		di->irq_val &= ~MT5727_TX_IRQ_START_PING;
		power_event_bnc_notify(mt5727_tx_get_bnt_wltx_type(di->ic_type),
			POWER_NE_WLTX_PING_RX, NULL);
	}
	if (di->irq_val & MT5727_TX_IRQ_EPT_PKT_RCVD) {
		di->irq_val &= ~MT5727_TX_IRQ_EPT_PKT_RCVD;
		mt5727_tx_ept_handler(di);
	}
	if (di->irq_val & MT5727_TX_IRQ_RPP_TIMEOUT) {
		di->irq_val &= ~MT5727_TX_IRQ_RPP_TIMEOUT;
		power_event_bnc_notify(mt5727_tx_get_bnt_wltx_type(di->ic_type),
			POWER_NE_WLTX_RP_DM_TIMEOUT, NULL);
	}
	if (di->irq_val & MT5727_TX_IRQ_FOD_DET) {
		di->irq_val &= ~MT5727_TX_IRQ_FOD_DET;
		power_event_bnc_notify(mt5727_tx_get_bnt_wltx_type(di->ic_type),
			POWER_NE_WLTX_TX_FOD, NULL);
	}
	if (di->irq_val & MT5727_TX_IRQ_PING_OCP) {
		di->irq_val &= ~MT5727_TX_IRQ_PING_OCP;
		power_event_bnc_notify(mt5727_tx_get_bnt_wltx_type(di->ic_type),
			(di->ic_type == WLTRX_IC_AUX) ? POWER_NE_WLTX_TX_PING_OCP :
			POWER_NE_WLTX_TX_FOD, NULL);
	}
	if (di->irq_val & MT5727_TX_IRQ_OCP) {
		di->irq_val &= ~MT5727_TX_IRQ_OCP;
		power_event_bnc_notify(mt5727_tx_get_bnt_wltx_type(di->ic_type),
			POWER_NE_WLTX_OCP, NULL);
	}
	if (di->irq_val & MT5727_TX_IRQ_OVP) {
		di->irq_val &= ~MT5727_TX_IRQ_OVP;
		power_event_bnc_notify(mt5727_tx_get_bnt_wltx_type(di->ic_type),
			POWER_NE_WLTX_OVP, NULL);
	}

rechk_irq:
	mt5727_tx_mode_irq_recheck(di);
}

static struct wltx_ic_ops g_mt5727_tx_ic_ops = {
	.get_dev_node           = mt5727_dts_dev_node,
	.fw_update              = mt5727_fw_sram_update,
	.prev_psy               = NULL,
	.chip_init              = mt5727_tx_chip_init,
	.chip_reset             = mt5727_chip_reset,
	.chip_enable            = mt5727_chip_enable,
	.mode_enable            = mt5727_tx_enable_tx_mode,
	.activate_chip          = mt5727_tx_activate_chip,
	.set_open_flag          = mt5727_tx_set_tx_open_flag,
	.set_stop_cfg           = mt5727_tx_stop_config,
	.is_rx_discon           = mt5727_tx_check_rx_disconnect,
	.is_in_tx_mode          = mt5727_tx_is_tx_mode,
	.is_in_rx_mode          = mt5727_tx_is_rx_mode,
	.get_vrect              = mt5727_tx_get_vrect,
	.get_vin                = mt5727_tx_get_vin,
	.get_iin                = mt5727_tx_get_iin,
	.get_temp               = mt5727_tx_get_temp,
	.get_fop                = mt5727_tx_get_fop,
	.get_cep                = mt5727_tx_get_cep,
	.get_duty               = mt5727_tx_get_duty,
	.get_ptx                = mt5727_tx_get_ptx,
	.get_prx                = mt5727_tx_get_prx,
	.get_ploss              = mt5727_tx_get_ploss,
	.get_ploss_id           = mt5727_tx_get_ploss_id,
	.get_ping_freq          = mt5727_tx_get_ping_freq,
	.get_ping_interval      = mt5727_tx_get_ping_interval,
	.get_min_fop            = mt5727_tx_get_min_fop,
	.get_max_fop            = mt5727_tx_get_max_fop,
	.get_full_bridge_ith    = mt5727_tx_get_full_bridge_ith,
	.set_ping_freq          = mt5727_tx_set_ping_freq,
	.set_ping_interval      = mt5727_tx_set_ping_interval,
	.set_min_fop            = mt5727_tx_set_min_fop,
	.set_max_fop            = mt5727_tx_set_max_fop,
	.set_ilim               = mt5727_tx_set_ilimit,
	.set_vset               = mt5727_tx_set_vset,
	.set_fod_coef           = NULL,
	.set_rp_dm_to           = NULL,
	.set_bridge             = mt5727_tx_set_bridge,
};

int mt5727_tx_ops_register(struct wltrx_ic_ops *ops, struct mt5727_dev_info *di)
{
	if (!ops || !di)
		return -ENODEV;

	ops->tx_ops = kzalloc(sizeof(*(ops->tx_ops)), GFP_KERNEL);
	if (!ops->tx_ops)
		return -ENOMEM;

	memcpy(ops->tx_ops, &g_mt5727_tx_ic_ops, sizeof(g_mt5727_tx_ic_ops));
	ops->tx_ops->dev_data = (void *)di;

	return wltx_ic_ops_register(ops->tx_ops, di->ic_type);
}
