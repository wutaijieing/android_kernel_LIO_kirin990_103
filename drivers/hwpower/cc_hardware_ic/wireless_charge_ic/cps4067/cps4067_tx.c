// SPDX-License-Identifier: GPL-2.0
/*
 * cps4067_tx.c
 *
 * cps4067 tx driver
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

#define HWLOG_TAG wireless_cps4067_tx
HWLOG_REGIST();

static const char * const g_cps4067_tx_irq_name[] = {
	/* [n]: n means bit in registers */
	[0]  = "ping",
	[1]  = "ss_pkt",
	[2]  = "id_pkt",
	[3]  = "cfg_pkt",
	[4]  = "ask_pkt",
	[5]  = "ept",
	[6]  = "rpp_timeout",
	[7]  = "cep_timeout",
	[8]  = "ac_detect",
	[9]  = "tx_init",
	[10] = "ask_all",
	[11] = "rpp_type_err",
	[12] = "ask_ack",
	[13] = "detect_other_tx",
	[14] = "tx_htp",
	[15] = "ping_ovp",
	[16] = "lp_end",
	[17] = "full_bridge",
	[18] = "half_bridge",
};

static const char * const g_cps4067_tx_ept_name[] = {
	/* [n]: n means bit in registers */
	[0]  = "ept_wrong_pkt",
	[1]  = "ept_povp",
	[2]  = "ept_ac_det",
	[3]  = "ept_rx_ept",
	[4]  = "ept_cep_timeout",
	[5]  = "ept_rpp_timeout",
	[6]  = "ept_ocp",
	[7]  = "ept_ovp",
	[8]  = "ept_uvp",
	[9]  = "ept_fod",
	[10] = "ept_otp",
	[11] = "ept_pocp",
	[12] = "ept_q_fod",
	[13] = "ept_sr_ocp",
	[14] = "ept_vrect_ovp",
	[15] = "ept_vrecth_ovp",
};

static bool cps4067_tx_is_in_tx_mode(void *dev_data)
{
	int ret;
	u8 mode = 0;

	ret = cps4067_read_byte(dev_data, CPS4067_SYS_MODE_ADDR, &mode);
	if (ret) {
		hwlog_err("is_tx_mode: get op_mode failed\n");
		return false;
	}

	return mode == CPS4067_SYS_MODE_TX;
}

static bool cps4067_tx_is_in_rx_mode(void *dev_data)
{
	int ret;
	u8 mode = 0;

	ret = cps4067_read_byte(dev_data, CPS4067_SYS_MODE_ADDR, &mode);
	if (ret) {
		hwlog_err("is_tx_mode: get op_mode failed\n");
		return false;
	}

	return mode == CPS4067_SYS_MODE_RX;
}

static int cps4067_tx_set_tx_open_flag(bool enable, void *dev_data)
{
	struct cps4067_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	di->g_val.tx_open_flag = enable;
	return 0;
}

static void cps4067_tx_chip_reset(void *dev_data)
{
	int ret;
	struct cps4067_dev_info *di = dev_data;

	if (!di)
		return;

	di->need_ignore_irq = true;
	ret = cps4067_write_byte_mask(dev_data, CPS4067_TX_CMD_ADDR,
		CPS4067_TX_CMD_SYS_RST, CPS4067_TX_CMD_SYS_RST_SHIFT,
		CPS4067_TX_CMD_VAL);
	if (ret) {
		hwlog_err("chip_reset: set cmd failed\n");
		return;
	}

	power_msleep(DT_MSLEEP_10MS, 0, NULL);
	di->need_ignore_irq = false;
	hwlog_info("[chip_reset] succ\n");
}

static int cps4067_tx_get_full_bridge_ith(u16 *ith, void *dev_data)
{
	return 0;
}

static int cps4067_tx_set_bridge(unsigned int v_ask, unsigned int type, void *dev_data)
{
	if (v_ask > CPS4067_TX_SWITCH_BRI_CTL_VTH)
		return cps4067_write_dword_mask(dev_data, CPS4067_TX_FUNC_EN_ADDR,
			CPS4067_TX_SWITCH_BRI_CTL_MASK, CPS4067_TX_SWITCH_BRI_CTL_SHIFT,
			CPS4067_TX_FUNC_EN);
	else
		return cps4067_write_dword_mask(dev_data, CPS4067_TX_FUNC_EN_ADDR,
			CPS4067_TX_SWITCH_BRI_CTL_MASK, CPS4067_TX_SWITCH_BRI_CTL_SHIFT,
			CPS4067_TX_FUNC_DIS);
}

static bool cps4067_tx_check_rx_disconnect(void *dev_data)
{
	struct cps4067_dev_info *di = dev_data;

	if (!di)
		return true;

	if (di->tx_ept_type & CPS4067_TX_EPT_SRC_CEP_TIMEOUT) {
		di->tx_ept_type &= ~CPS4067_TX_EPT_SRC_CEP_TIMEOUT;
		hwlog_info("[check_rx_disconnect] rx disconnect\n");
		return true;
	}

	return false;
}

static int cps4067_tx_get_ping_interval(u16 *ping_interval, void *dev_data)
{
	return cps4067_read_word(dev_data, CPS4067_TX_PING_INTERVAL_ADDR, ping_interval);
}

static int cps4067_tx_set_ping_interval(u16 ping_interval, void *dev_data)
{
	if ((ping_interval < CPS4067_TX_PING_INTERVAL_MIN) ||
		(ping_interval > CPS4067_TX_PING_INTERVAL_MAX)) {
		hwlog_err("set_ping_interval: para out of range\n");
		return -EINVAL;
	}

	return cps4067_write_word(dev_data, CPS4067_TX_PING_INTERVAL_ADDR, ping_interval);
}

static int cps4067_tx_get_ping_freq(u16 *ping_freq, void *dev_data)
{
	return cps4067_read_byte(dev_data, CPS4067_TX_PING_FREQ_ADDR, (u8 *)ping_freq);
}

static int cps4067_tx_set_ping_freq(u16 ping_freq, void *dev_data)
{
	if ((ping_freq < CPS4067_TX_PING_FREQ_MIN) ||
		(ping_freq > CPS4067_TX_PING_FREQ_MAX)) {
		hwlog_err("set_ping_frequency: para out of range\n");
		return -EINVAL;
	}

	return cps4067_write_byte(dev_data, CPS4067_TX_PING_FREQ_ADDR, (u8)ping_freq);
}

static int cps4067_tx_get_min_fop(u16 *fop, void *dev_data)
{
	return cps4067_read_byte(dev_data, CPS4067_TX_MIN_FOP_ADDR, (u8 *)fop);
}

static int cps4067_tx_set_min_fop(u16 fop, void *dev_data)
{
	if ((fop < CPS4067_TX_MIN_FOP) || (fop > CPS4067_TX_MAX_FOP)) {
		hwlog_err("set_min_fop: para out of range\n");
		return -EINVAL;
	}

	return cps4067_write_byte(dev_data, CPS4067_TX_MIN_FOP_ADDR, (u8)fop);
}

static int cps4067_tx_get_max_fop(u16 *fop, void *dev_data)
{
	return cps4067_read_byte(dev_data, CPS4067_TX_MAX_FOP_ADDR, (u8 *)fop);
}

static int cps4067_tx_set_max_fop(u16 fop, void *dev_data)
{
	if ((fop < CPS4067_TX_MIN_FOP) || (fop > CPS4067_TX_MAX_FOP)) {
		hwlog_err("set_max_fop: para out of range\n");
		return -EINVAL;
	}

	return cps4067_write_byte(dev_data, CPS4067_TX_MAX_FOP_ADDR, (u8)fop);
}

static int cps4067_tx_get_fop(u16 *fop, void *dev_data)
{
	return cps4067_read_word(dev_data, CPS4067_TX_OP_FREQ_ADDR, fop);
}

static int cps4067_tx_get_cep(s8 *cep, void *dev_data)
{
	return cps4067_read_byte(dev_data, CPS4067_TX_CEP_ADDR, cep);
}

static int cps4067_tx_get_duty(u8 *duty, void *dev_data)
{
	return 0;
}

static int cps4067_tx_get_ptx(u32 *ptx, void *dev_data)
{
	return 0;
}

static int cps4067_tx_get_prx(u32 *prx, void *dev_data)
{
	return 0;
}

static int cps4067_tx_get_ploss(s32 *ploss, void *dev_data)
{
	return 0;
}

static int cps4067_tx_get_ploss_id(u8 *id, void *dev_data)
{
	return 0;
}

static int cps4067_tx_get_temp(s16 *chip_temp, void *dev_data)
{
	return cps4067_read_word(dev_data, CPS4067_TX_CHIP_TEMP_ADDR, chip_temp);
}

static int cps4067_tx_get_vin(u16 *tx_vin, void *dev_data)
{
	return cps4067_read_word(dev_data, CPS4067_TX_VIN_ADDR, tx_vin);
}

static int cps4067_tx_get_vrect(u16 *tx_vrect, void *dev_data)
{
	return cps4067_read_word(dev_data, CPS4067_TX_VRECT_ADDR, tx_vrect);
}

static int cps4067_tx_get_iin(u16 *tx_iin, void *dev_data)
{
	return cps4067_read_word(dev_data, CPS4067_TX_IIN_ADDR, tx_iin);
}

static int cps4067_tx_set_ilimit(u16 tx_ilim, void *dev_data)
{
	if ((tx_ilim < CPS4067_TX_ILIM_MIN) || (tx_ilim > CPS4067_TX_ILIM_MAX)) {
		hwlog_err("set_ilimit: out of range\n");
		return -EINVAL;
	}

	return cps4067_write_word(dev_data, CPS4067_TX_ILIM_ADDR, tx_ilim);
}

static int cps4067_tx_init_fod_coef(struct cps4067_dev_info *di)
{
	return 0;
}

static int cps4067_tx_stop_config(void *dev_data)
{
	struct cps4067_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	di->g_val.tx_stop_chrg = true;
	return 0;
}

static int cps4067_sw2tx(struct cps4067_dev_info *di)
{
	int ret;
	int i;
	u8 mode = 0;

	di->need_ignore_irq = true;
	/* 500ms=25ms*20 timeout for switching to tx mode */
	for (i = 0; i < 20; i++) {
		power_msleep(DT_MSLEEP_25MS, 0, NULL);
		ret = cps4067_get_mode(di, &mode);
		if (ret) {
			hwlog_err("sw2tx: get mode failed\n");
			continue;
		}
		if (mode == CPS4067_SYS_MODE_TX) {
			hwlog_info("sw2tx: succ, cnt=%d\n", i);
			ret = 0;
			goto exit;
		}
		ret = cps4067_write_byte_mask(di, CPS4067_TX_CMD_ADDR,
			CPS4067_TX_CMD_EN_TX, CPS4067_TX_CMD_EN_TX_SHIFT,
			CPS4067_TX_CMD_VAL);
		if (ret)
			hwlog_err("sw2tx: write cmd(sw2tx) failed\n");
	}
	hwlog_err("sw2tx: failed, cnt=%d\n", i);
	ret = -ENXIO;

exit:
	di->need_ignore_irq = false;
	return ret;
}

static void cps4067_tx_select_init_para(struct cps4067_dev_info *di,
	unsigned int client)
{
	switch (client) {
	case WLTX_CLIENT_UI:
		di->tx_init_para.ping_freq = di->tx_ping_freq;
		di->tx_init_para.ping_interval = CPS4067_TX_PING_INTERVAL;
		break;
	case WLTX_CLIENT_COIL_TEST:
		di->tx_init_para.ping_freq = CPS4067_COIL_TEST_PING_FREQ;
		di->tx_init_para.ping_interval = CPS4067_COIL_TEST_PING_INTERVAL;
		break;
	case WLTX_CLIENT_BAT_HEATING:
		di->tx_init_para.ping_freq = CPS4067_BAT_HEATING_PING_FREQ;
		di->tx_init_para.ping_interval = CPS4067_BAT_HEATING_PING_INTERVAL;
		break;
	default:
		di->tx_init_para.ping_freq = di->tx_ping_freq;
		di->tx_init_para.ping_interval = CPS4067_TX_PING_INTERVAL;
		break;
	}
}

static int cps4067_tx_set_init_para(struct cps4067_dev_info *di)
{
	int ret;

	ret = cps4067_sw2tx(di);
	if (ret) {
		hwlog_err("set_init_para: sw2tx failed\n");
		return ret;
	}

	ret = cps4067_write_word(di, CPS4067_TX_OCP_TH_ADDR, CPS4067_TX_OCP_TH);
	ret += cps4067_write_word(di, CPS4067_TX_OVP_TH_ADDR, CPS4067_TX_OVP_TH);
	ret += cps4067_write_dword(di, CPS4067_TX_IRQ_EN_ADDR, CPS4067_TX_IRQ_VAL);
	ret += cps4067_tx_init_fod_coef(di);
	ret += cps4067_tx_set_ping_freq(di->tx_init_para.ping_freq, di);
	ret += cps4067_tx_set_min_fop(CPS4067_TX_MIN_FOP, di);
	ret += cps4067_tx_set_max_fop(CPS4067_TX_MAX_FOP, di);
	ret += cps4067_tx_set_ping_interval(di->tx_init_para.ping_interval, di);
	ret += cps4067_write_word(di, CPS4067_TX_BRIDGE_EN_ADDR, CPS4067_TX_BRIDGE_EN_TH);
	if (ret) {
		hwlog_err("set_init_para: write failed\n");
		return -EIO;
	}

	return 0;
}

static int cps4067_tx_chip_init(unsigned int client, void *dev_data)
{
	struct cps4067_dev_info *di = dev_data;
	u8 data = 0;

	if (!di)
		return -ENODEV;

	(void)cps4067_read_byte(di, CPS4067_TX_QI_STATUS_ADDR, &data);
	if ((client == WLTX_CLIENT_LIGHTSTRAP) && (data & CPS4067_TX_QI_PT_PHASE)) {
		hwlog_info("[chip_init] already in sink_pt state\n");
		power_event_bnc_notify(POWER_BNT_WLTX, POWER_NE_WLTX_GET_CFG, NULL);
		return 0;
	}
	cps4067_tx_chip_reset(di);

	di->irq_cnt = 0;
	di->g_val.irq_abnormal = false;
	di->g_val.tx_stop_chrg = false;
	cps4067_enable_irq(di);

	cps4067_tx_select_init_para(di, client);
	return cps4067_tx_set_init_para(di);
}

static int cps4067_tx_enable_tx_mode(bool enable, void *dev_data)
{
	int ret;

	if (enable)
		ret = cps4067_write_dword_mask(dev_data, CPS4067_TX_FUNC_EN_ADDR,
			CPS4067_TX_PING_EN_MASK, CPS4067_TX_PING_EN_SHIFT,
			CPS4067_TX_FUNC_EN);
	else
		ret = cps4067_write_dword_mask(dev_data, CPS4067_TX_FUNC_EN_ADDR,
			CPS4067_TX_PING_EN_MASK, CPS4067_TX_PING_EN_SHIFT,
			CPS4067_TX_FUNC_DIS);

	if (ret)
		hwlog_err("%s tx_mode failed\n", enable ? "enable" : "disable");

	return ret;
}

static void cps4067_tx_show_ept_type(u16 ept)
{
	unsigned int i;

	hwlog_info("[show_ept_type] type=0x%02x", ept);
	for (i = 0; i < ARRAY_SIZE(g_cps4067_tx_ept_name); i++) {
		if (ept & BIT(i))
			hwlog_info("[tx_ept] %s\n", g_cps4067_tx_ept_name[i]);
	}
}

static int cps4067_tx_get_ept_type(struct cps4067_dev_info *di, u16 *ept)
{
	int ret;

	if (!ept) {
		hwlog_err("get_ept_type: para null\n");
		return -EINVAL;
	}

	ret = cps4067_read_word(di, CPS4067_TX_EPT_SRC_ADDR, ept);
	if (ret) {
		hwlog_err("get_ept_type: read failed\n");
		return ret;
	}

	cps4067_tx_show_ept_type(*ept);
	return 0;
}

static int cps4067_tx_clear_ept_src(struct cps4067_dev_info *di)
{
	return cps4067_write_word(di, CPS4067_TX_EPT_SRC_ADDR, CPS4067_TX_EPT_SRC_CLR);
}

static void cps4067_tx_ept_handler(struct cps4067_dev_info *di)
{
	int ret;
	u16 q_detect = 0;

	ret = cps4067_tx_get_ept_type(di, &di->tx_ept_type);
	ret += cps4067_tx_clear_ept_src(di);
	if (ret)
		return;

	switch (di->tx_ept_type) {
	case CPS4067_TX_EPT_SRC_RX_EPT:
	case CPS4067_TX_EPT_SRC_CEP_TIMEOUT:
	case CPS4067_TX_EPT_SRC_OCP:
		di->tx_ept_type &= ~CPS4067_TX_EPT_SRC_CEP_TIMEOUT;
		power_event_bnc_notify(POWER_BNT_WLTX, POWER_NE_WLTX_CEP_TIMEOUT, NULL);
		break;
	case CPS4067_TX_EPT_SRC_Q_FOD:
		hwlog_err("tx_ept_handler: detected q_val=%u\n", q_detect);
		/* fall-through */
	case CPS4067_TX_EPT_SRC_FOD:
	case CPS4067_TX_EPT_SRC_POCP:
		di->tx_ept_type &= ~CPS4067_TX_EPT_SRC_FOD;
		power_event_bnc_notify(POWER_BNT_WLTX, POWER_NE_WLTX_TX_FOD, NULL);
		break;
	default:
		break;
	}
}

static int cps4067_tx_clear_irq(struct cps4067_dev_info *di, u32 itr)
{
	return cps4067_write_dword(di, CPS4067_TX_IRQ_CLR_ADDR, itr);
}

static void cps4067_tx_ask_pkt_handler(struct cps4067_dev_info *di)
{
	if (di->irq_val & CPS4067_TX_IRQ_SS_PKG_RCVD) {
		di->irq_val &= ~CPS4067_TX_IRQ_SS_PKG_RCVD;
		if (di->g_val.qi_hdl && di->g_val.qi_hdl->hdl_qi_ask_pkt)
			di->g_val.qi_hdl->hdl_qi_ask_pkt(di);
	}

	if (di->irq_val & CPS4067_TX_IRQ_ID_PKT_RCVD) {
		di->irq_val &= ~CPS4067_TX_IRQ_ID_PKT_RCVD;
		if (di->g_val.qi_hdl && di->g_val.qi_hdl->hdl_qi_ask_pkt)
			di->g_val.qi_hdl->hdl_qi_ask_pkt(di);
	}

	if (di->irq_val & CPS4067_TX_IRQ_CFG_PKT_RCVD) {
		di->irq_val &= ~CPS4067_TX_IRQ_CFG_PKT_RCVD;
		if (di->g_val.qi_hdl && di->g_val.qi_hdl->hdl_qi_ask_pkt)
			di->g_val.qi_hdl->hdl_qi_ask_pkt(di);
		power_event_bnc_notify(POWER_BNT_WLTX, POWER_NE_WLTX_GET_CFG, NULL);
	}

	if (di->irq_val & CPS4067_TX_IRQ_ASK_PKT_RCVD) {
		di->irq_val &= ~CPS4067_TX_IRQ_ASK_PKT_RCVD;
		if (di->g_val.qi_hdl && di->g_val.qi_hdl->hdl_non_qi_ask_pkt)
			di->g_val.qi_hdl->hdl_non_qi_ask_pkt(di);
	}
}

static void cps4067_tx_show_irq(u32 intr)
{
	unsigned int i;

	hwlog_info("[show_irq] irq=0x%04x\n", intr);
	for (i = 0; i < ARRAY_SIZE(g_cps4067_tx_irq_name); i++) {
		if (intr & BIT(i))
			hwlog_info("[tx_irq] %s\n", g_cps4067_tx_irq_name[i]);
	}
}

static int cps4067_tx_get_interrupt(struct cps4067_dev_info *di, u32 *intr)
{
	int ret;

	ret = cps4067_read_dword(di, CPS4067_TX_IRQ_ADDR, intr);
	if (ret)
		return ret;

	cps4067_tx_show_irq(*intr);
	return 0;
}

static void cps4067_tx_mode_irq_recheck(struct cps4067_dev_info *di)
{
	int ret;
	u32 irq_val = 0;

	if (gpio_get_value(di->gpio_int))
		return;

	hwlog_info("[tx_mode_irq_recheck] gpio_int low, re-check irq\n");
	ret = cps4067_tx_get_interrupt(di, &irq_val);
	if (ret)
		return;

	cps4067_tx_clear_irq(di, CPS4067_TX_IRQ_CLR_ALL);
}

void cps4067_tx_mode_irq_handler(struct cps4067_dev_info *di)
{
	int ret;

	if (!di)
		return;

	ret = cps4067_tx_get_interrupt(di, &di->irq_val);
	if (ret) {
		hwlog_err("irq_handler: get irq failed, clear\n");
		cps4067_tx_clear_irq(di, CPS4067_TX_IRQ_CLR_ALL);
		goto rechk_irq;
	}

	cps4067_tx_clear_irq(di, di->irq_val);
	cps4067_tx_ask_pkt_handler(di);

	if (di->irq_val & CPS4067_TX_IRQ_START_PING) {
		di->irq_val &= ~CPS4067_TX_IRQ_START_PING;
		power_event_bnc_notify(POWER_BNT_WLTX, POWER_NE_WLTX_PING_RX, NULL);
	}
	if (di->irq_val & CPS4067_TX_IRQ_EPT_PKT_RCVD) {
		di->irq_val &= ~CPS4067_TX_IRQ_EPT_PKT_RCVD;
		cps4067_tx_ept_handler(di);
	}
	if (di->irq_val & CPS4067_TX_IRQ_AC_DET) {
		di->irq_val &= ~CPS4067_TX_IRQ_AC_DET;
		power_event_bnc_notify(POWER_BNT_WLTX, POWER_NE_WLTX_RCV_DPING, NULL);
	}
	if (di->irq_val & CPS4067_TX_IRQ_RPP_TIMEOUT) {
		di->irq_val &= ~CPS4067_TX_IRQ_RPP_TIMEOUT;
		power_event_bnc_notify(POWER_BNT_WLTX, POWER_NE_WLTX_RP_DM_TIMEOUT, NULL);
	}

rechk_irq:
	cps4067_tx_mode_irq_recheck(di);
}

static struct wltx_ic_ops g_cps4067_tx_ic_ops = {
	.get_dev_node           = cps4067_dts_dev_node,
	.fw_update              = cps4067_fw_sram_update,
	.chip_init              = cps4067_tx_chip_init,
	.chip_reset             = cps4067_tx_chip_reset,
	.chip_enable            = cps4067_chip_enable,
	.mode_enable            = cps4067_tx_enable_tx_mode,
	.set_open_flag          = cps4067_tx_set_tx_open_flag,
	.set_stop_cfg           = cps4067_tx_stop_config,
	.is_rx_discon           = cps4067_tx_check_rx_disconnect,
	.is_in_tx_mode          = cps4067_tx_is_in_tx_mode,
	.is_in_rx_mode          = cps4067_tx_is_in_rx_mode,
	.get_vrect              = cps4067_tx_get_vrect,
	.get_vin                = cps4067_tx_get_vin,
	.get_iin                = cps4067_tx_get_iin,
	.get_temp               = cps4067_tx_get_temp,
	.get_fop                = cps4067_tx_get_fop,
	.get_cep                = cps4067_tx_get_cep,
	.get_duty               = cps4067_tx_get_duty,
	.get_ptx                = cps4067_tx_get_ptx,
	.get_prx                = cps4067_tx_get_prx,
	.get_ploss              = cps4067_tx_get_ploss,
	.get_ploss_id           = cps4067_tx_get_ploss_id,
	.get_ping_freq          = cps4067_tx_get_ping_freq,
	.get_ping_interval      = cps4067_tx_get_ping_interval,
	.get_min_fop            = cps4067_tx_get_min_fop,
	.get_max_fop            = cps4067_tx_get_max_fop,
	.get_full_bridge_ith    = cps4067_tx_get_full_bridge_ith,
	.set_ping_freq          = cps4067_tx_set_ping_freq,
	.set_ping_interval      = cps4067_tx_set_ping_interval,
	.set_min_fop            = cps4067_tx_set_min_fop,
	.set_max_fop            = cps4067_tx_set_max_fop,
	.set_ilim               = cps4067_tx_set_ilimit,
	.set_bridge             = cps4067_tx_set_bridge,
};

int cps4067_tx_ops_register(struct wltrx_ic_ops *ops, struct cps4067_dev_info *di)
{
	if (!ops || !di)
		return -ENODEV;

	ops->tx_ops = kzalloc(sizeof(*(ops->tx_ops)), GFP_KERNEL);
	if (!ops->tx_ops)
		return -ENODEV;

	memcpy(ops->tx_ops, &g_cps4067_tx_ic_ops, sizeof(g_cps4067_tx_ic_ops));
	ops->tx_ops->dev_data = (void *)di;
	return wltx_ic_ops_register(ops->tx_ops, di->ic_type);
}
