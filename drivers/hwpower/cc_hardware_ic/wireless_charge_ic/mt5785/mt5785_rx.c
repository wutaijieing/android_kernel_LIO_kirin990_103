// SPDX-License-Identifier: GPL-2.0
/*
 * mt5785_rx.c
 *
 * mt5785 rx driver
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

#define HWLOG_TAG wireless_mt5785_rx
HWLOG_REGIST();

static const char * const g_mt5785_rx_irq_name[] = {
	/* [n]: n means bit in registers */
	[0]  = "chip_reset",
	[8]  = "power_on",
	[9]  = "rx_ready",
	[10] = "ldo_enable",
	[11] = "ldo_disable",
	[12] = "ppp_success",
	[13] = "ppp_fail",
	[14] = "fsk_recv",
	[15] = "ldo_short",
	[16] = "ldo_opp",
	[17] = "ldo_ocp",
	[18] = "rx_ldo_ovp",
	[19] = "ldo_otp",
	[20] = "rx_vout_ovp",
	[21] = "full bridge mode",
	[22] = "half bridge mode",
	[23] = "fc_done",
};

static struct {
	int vpa;
	int vmldo;
} const g_rx_vfc_map[] = {
	{5000, 5000}, {9000, 9000}, {12000, 9000}, {15000, 9000}, /* mV */
};

static int mt5785_rx_get_temp(int *temp, void *dev_data)
{
	s16 l_temp = 0;

	if (!temp || mt5785_read_word(dev_data, MT5785_RX_CHIP_TEMP_ADDR, (u16 *)&l_temp))
		return -EINVAL;

	*temp = l_temp;
	return 0;
}

static int mt5785_rx_get_fop(int *fop, void *dev_data)
{
	int ret;

	ret = mt5785_read_word(dev_data, MT5785_RX_FOP_ADDR, (u16 *)fop);
	if (ret)
		return ret;

	*fop = (*fop / 10); /* 10: fop unit */
	return 0;
}

static int mt5785_rx_get_cep(int *cep, void *dev_data)
{
	s8 l_cep = 0;

	if (!cep || mt5785_read_byte(dev_data, MT5785_RX_CEP_VAL_ADDR, (u8 *)&l_cep))
		return -EINVAL;

	*cep = l_cep;
	return 0;
}

static int mt5785_rx_get_vrect(int *vrect, void *dev_data)
{
	return mt5785_read_word(dev_data, MT5785_RX_VRECT_ADDR, (u16 *)vrect);
}

static int mt5785_rx_get_vout(int *vout, void *dev_data)
{
	return mt5785_read_word(dev_data, MT5785_RX_VOUT_ADDR, (u16 *)vout);
}

static int mt5785_rx_get_iout(int *iout, void *dev_data)
{
	return mt5785_read_word(dev_data, MT5785_RX_IOUT_ADDR, (u16 *)iout);
}

static void mt5785_rx_get_iavg(int *iavg, void *dev_data)
{
	struct mt5785_dev_info *di = dev_data;

	if (!di)
		return;

	wlic_get_rx_iavg(di->ic_type, iavg);
}

static void mt5785_rx_get_imax(int *imax, void *dev_data)
{
	struct mt5785_dev_info *di = dev_data;

	if (!di) {
		if (imax)
			*imax = WLIC_DEFAULT_RX_IMAX;
		return;
	}
	wlic_get_rx_imax(di->ic_type, imax);
}

static int mt5785_rx_get_rx_vout_reg(int *vreg, void *dev_data)
{
	return mt5785_read_word(dev_data, MT5785_RX_VOUT_SET_ADDR, (u16 *)vreg);
}

static int mt5785_rx_get_vfc_reg(int *vfc_reg, void *dev_data)
{
	return mt5785_read_word(dev_data, MT5785_RX_FC_VPA_ADDR, (u16 *)vfc_reg);
}

static void mt5785_rx_show_irq(u32 intr)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(g_mt5785_rx_irq_name); i++) {
		if (intr & BIT(i))
			hwlog_info("[rx_show_irq] %s\n", g_mt5785_rx_irq_name[i]);
	}
}

static int mt5785_rx_get_interrupt(struct mt5785_dev_info *di, u32 *intr)
{
	int ret;

	ret = mt5785_read_dword(di, MT5785_RX_IRQ_ADDR, intr);
	if (ret)
		return ret;

	hwlog_info("[get_interrupt] irq=0x%08x\n", *intr);
	mt5785_rx_show_irq(*intr);

	return 0;
}

static int mt5785_rx_clear_irq(struct mt5785_dev_info *di, u32 intr)
{
	int ret;

	ret = mt5785_write_dword(di, MT5785_RX_IRQ_CLR_ADDR, intr);
	if (ret) {
		hwlog_err("clear_irq: failed\n");
		return ret;
	}

	ret = mt5785_write_dword_mask(di, MT5785_RX_CMD_ADDR,
		MT5785_RX_CMD_CLEAR_INT_MASK, MT5785_RX_CMD_CLEAR_INT_SHIFT,
		MT5785_RX_CMD_VAL);

	return ret;
}

static void mt5785_sleep_enable(bool enable, void *dev_data)
{
	int gpio_val;
	struct mt5785_dev_info *di = dev_data;

	if (!di || di->g_val.irq_abnormal)
		return;

	gpio_set_value(di->gpio_sleep_en,
		enable ? WLTRX_IC_SLEEP_ENABLE : WLTRX_IC_SLEEP_DISABLE);
	gpio_val = gpio_get_value(di->gpio_sleep_en);
	hwlog_info("[sleep_enable] gpio %s now\n", gpio_val ? "high" : "low");
}

static bool mt5785_is_sleep_enable(void *dev_data)
{
	int gpio_val;
	struct mt5785_dev_info *di = dev_data;

	if (!di)
		return false;

	gpio_val = gpio_get_value(di->gpio_sleep_en);
	return gpio_val == WLTRX_IC_SLEEP_ENABLE;
}

static void mt5785_rx_chip_reset(void *dev_data)
{
	int ret;

	ret = mt5785_write_dword_mask(dev_data, MT5785_RX_CMD_ADDR,
		MT5785_RX_CMD_SYS_RST_MASK, MT5785_RX_CMD_SYS_RST_SHIFT,
		MT5785_RX_CMD_VAL);
	if (ret) {
		hwlog_err("chip_reset: set cmd failed\n");
		return;
	}

	hwlog_info("[chip_reset] succ\n");
}

static void mt5785_rx_ext_pwr_post_ctrl(bool flag, void *dev_data)
{
	if (flag)
		(void)mt5785_write_dword_mask(dev_data, MT5785_RX_CMD_ADDR,
			MT5785_RX_CMD_EXT_VCC_EN_MASK, MT5785_RX_CMD_EXT_VCC_EN_SHIFT,
			MT5785_RX_CMD_VAL);
	else
		(void)mt5785_write_dword_mask(dev_data, MT5785_RX_CMD_ADDR,
			MT5785_RX_CMD_EXT_VCC_DIS_MASK, MT5785_RX_CMD_EXT_VCC_DIS_SHIFT,
			MT5785_RX_CMD_VAL);
}

static int mt5785_rx_set_rx_vout(int vout, void *dev_data)
{
	int ret;

	if ((vout < MT5785_RX_VOUT_MIN) || (vout > MT5785_RX_VOUT_MAX)) {
		hwlog_err("set_rx_vout: out of range\n");
		return -EINVAL;
	}

	ret = mt5785_write_word(dev_data, MT5785_RX_VOUT_SET_ADDR, (u16)vout);
	if (ret) {
		hwlog_err("set_rx_vout: write addr failed\n");
		return ret;
	}

	ret = mt5785_write_dword_mask(dev_data, MT5785_RX_CMD_ADDR,
		MT5785_RX_CMD_VOUT_MASK, MT5785_RX_CMD_VOUT_SHIFT, MT5785_RX_CMD_VAL);
	if (ret) {
		hwlog_err("set_rx_vout: write cmd failed\n");
		return ret;
	}

	return 0;
}

static void mt5785_ask_mode_cfg(struct mt5785_dev_info *di, u8 cm, u8 polarity)
{
	int ret;
	u8 askcap;

	askcap = (cm & MT5785_RX_ASK_CAP_CM_MASK) |
		((polarity << MT5785_RX_ASK_CAP_POLARITY_SHIFT) & 0xff); /* 0xff: 8bit */
	hwlog_info("[ask_mode_cfg] cap=0x%x\n", askcap);
	ret = mt5785_write_byte(di, MT5785_RX_ASK_CAP_ADDR, askcap);
	if (ret)
		hwlog_err("ask_mode_cfg: failed\n");
}

static void mt5785_set_mode_cfg(struct mt5785_dev_info *di, int vfc)
{
	u8 cm_cfg, polar_cfg;

	if (vfc <= WLRX_IC_HIGH_VOUT1) {
		cm_cfg = di->rx_mod_cfg.cm.l_volt;
		polar_cfg = di->rx_mod_cfg.polar.l_volt;
	} else {
		if (!power_cmdline_is_factory_mode() &&
			(vfc >= WLRX_IC_HIGH_VOUT2)) {
			cm_cfg = di->rx_mod_cfg.cm.h_volt;
			polar_cfg = di->rx_mod_cfg.polar.h_volt;
		} else {
			cm_cfg = di->rx_mod_cfg.cm.fac_h_volt;
			polar_cfg = di->rx_mod_cfg.polar.fac_h_volt;
		}
	}
	mt5785_ask_mode_cfg(di, cm_cfg, polar_cfg);
}

static int mt5785_rx_enable_boost_mode(struct mt5785_dev_info *di)
{
	return mt5785_write_byte(di, MT5785_RX_SWITCH_BRIDGE_MODE_ADDR, MT5785_RX_HALF_BRIDGE);
}

static int mt5785_rx_disable_boost_mode(struct mt5785_dev_info *di)
{
	return mt5785_write_byte(di, MT5785_RX_SWITCH_BRIDGE_MODE_ADDR, MT5785_RX_FULL_BRIDGE);
}

static void mt5785_show_sr_status(struct mt5785_dev_info *di)
{
	u32 sr_status = 0;

	mt5785_read_dword_mask(di, MT5785_RX_SYS_MODE_ADDR, MT5785_RX_SYS_MODE_BRIDGE_MASK,
		MT5785_RX_SYS_MODE_BRIDGE_SHIFT, &sr_status);
	hwlog_info("[sr_status] boost mode %s\n",
		sr_status ? "disabled" : "enable"); /* 1: SR is in half-bridge mode */
}

static int mt5785_rx_set_vpa(struct mt5785_dev_info *di, u16 vpa)
{
	return mt5785_write_word(di, MT5785_RX_FC_VPA_ADDR, vpa);
}

static int mt5785_rx_get_vmldo(u16 vfc)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(g_rx_vfc_map); i++) {
		if (vfc == g_rx_vfc_map[i].vpa)
			return g_rx_vfc_map[i].vmldo;
	}

	hwlog_err("get_vmldo: mismatch\n");
	return -EINVAL;
}

static int mt5785_rx_set_vmldo(struct mt5785_dev_info *di, u16 vfc)
{
	int vmldo;

	vmldo = mt5785_rx_get_vmldo(vfc);
	if (vmldo < 0)
		return -EINVAL;

	return mt5785_write_word(di, MT5785_RX_FC_VMLDO_ADDR, vmldo);
}

static int mt5785_rx_send_fc_cmd(struct mt5785_dev_info *di, int vfc)
{
	int ret;

	ret = mt5785_rx_set_vpa(di, vfc);
	if (ret) {
		hwlog_err("send_fc_cmd: set fc_vpa failed\n");
		return ret;
	}

	ret = mt5785_rx_set_vmldo(di, vfc);
	if (ret) {
		hwlog_err("send_fc_cmd: send fc_vmldo failed\n");
		return ret;
	}

	ret = mt5785_write_dword_mask(di, MT5785_RX_CMD_ADDR,
		MT5785_RX_CMD_SEND_FC_MASK, MT5785_RX_CMD_SEND_FC_SHIFT,
		MT5785_RX_CMD_VAL);
	if (ret) {
		hwlog_err("send_fc_cmd: send fc_cmd failed\n");
		return ret;
	}

	return 0;
}

static bool mt5785_rx_is_fc_succ(struct mt5785_dev_info *di)
{
	int i;
	int cnt;
	u32 rx_status = 0;

	cnt = MT5785_RX_FC_VOUT_TIMEOUT / MT5785_RX_FC_VOUT_SLEEP_TIME;
	for (i = 0; i < cnt; i++) {
		power_msleep(MT5785_RX_FC_VOUT_SLEEP_TIME, 0, NULL); /* wait for vout stability */
		mt5785_read_dword_mask(di, MT5785_RX_SYS_MODE_ADDR, MT5785_RX_SYS_MODE_FC_SUCC_MASK,
			MT5785_RX_SYS_MODE_FC_SUCC_SHIFT, &rx_status);
		if (!rx_status) {
			hwlog_info("[is_fc_succ] succ, cost_time: %dms\n",
				(i + 1) * MT5785_RX_FC_VOUT_SLEEP_TIME);
			return true;
		}
	}

	hwlog_err("is_fc_succ: timeout\n");
	return false;
}

static bool mt5785_rx_is_vmldo_set_succ(struct mt5785_dev_info *di, int vfc)
{
	int i;
	int cnt;
	int vout = 0;
	int vmldo;

	vmldo = mt5785_rx_get_vmldo(vfc);
	if (vmldo < 0)
		return false;

	cnt = MT5785_RX_SET_VMLDO_SLEEP_TIMEOUT / MT5785_RX_SET_VMLDO_SLEEP_TIME;
	for (i = 0; i < cnt; i++) {
		if (di->g_val.rx_stop_chrg)
			return false;
		power_msleep(MT5785_RX_SET_VMLDO_SLEEP_TIME, 0, NULL); /* wait for vout stability */
		(void)mt5785_rx_get_vout(&vout, di);
		if (vout >= (vmldo - 200)) { /* 200: maximum deviation */
			hwlog_info("[is_vmldo_set_succ] succ, cost_time: %dms\n",
				(i + 1) * MT5785_RX_SET_VMLDO_SLEEP_TIME);
			return true;
		}
	}

	hwlog_err("is_vmldo_set_succ: timeout\n");
	return true;
}

static int mt5785_rx_set_vfc(int vfc, void *dev_data)
{
	int i;
	int ret;
	struct mt5785_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	hwlog_info("[set_vfc] vfc=%d\n", vfc);
	if (vfc >= WLRX_IC_HIGH_VOUT2) {
		ret = mt5785_rx_disable_boost_mode(di);
		if (ret)
			return ret;
	}

	for (i = 0; i < MT5785_RX_FC_VPA_RETRY_CNT; i++) {
		if (di->g_val.rx_stop_chrg && (vfc > WLRX_IC_DFLT_VOUT))
			return -EPERM;

		ret = mt5785_rx_send_fc_cmd(di, vfc);
		if (ret) {
			hwlog_err("set_vfc: send fc_cmd failed\n");
			continue;
		}
		hwlog_info("[set_vfc] send fc_cmd sucess, cnt: %d\n", i);

		if (mt5785_rx_is_fc_succ(di)) {
			if (!mt5785_rx_is_vmldo_set_succ(di, vfc))
				continue;

			if (vfc < WLRX_IC_HIGH_VOUT2)
				(void)mt5785_rx_enable_boost_mode(di);

			mt5785_set_mode_cfg(di, vfc);
			hwlog_info("[set_vfc] succ\n");
			return 0;
		}
	}

	hwlog_err("set_vfc: failed\n");
	return -EIO;
}

static void mt5785_rx_send_ept(int ept_type, void *dev_data)
{
	int ret;

	switch (ept_type) {
	case HWQI_EPT_ERR_VRECT:
	case HWQI_EPT_ERR_VOUT:
		break;
	default:
		return;
	}
	ret = mt5785_write_byte(dev_data, MT5785_RX_EPT_MSG_ADDR, (u8)ept_type);
	ret += mt5785_write_dword_mask(dev_data, MT5785_RX_CMD_ADDR,
		MT5785_RX_CMD_SEND_EPT_MASK, MT5785_RX_CMD_SEND_EPT_SHIFT,
		MT5785_RX_CMD_VAL);
	if (ret)
		hwlog_err("send_ept: failed, ept=0x%x\n", ept_type);
}

bool mt5785_rx_is_tx_exist(void *dev_data)
{
	int ret;
	u32 mode = 0;

	ret = mt5785_get_mode(dev_data, &mode);
	if (ret) {
		hwlog_err("is_tx_exist: get mode failed\n");
		return false;
	}

	if (mode & MT5785_RX_SYS_MODE_RX)
		return true;

	return false;
}

static int mt5785_rx_kick_watchdog(void *dev_data)
{
	return mt5785_write_word(dev_data, MT5785_RX_WDT_ADDR, MT5785_RX_KICK_WDT_VAL);
}

static int mt5785_rx_get_fod(char *fod_str, int len, void *dev_data)
{
	return 0;
}

static int mt5785_rx_set_fod(const char *fod_str, void *dev_data)
{
	return 0;
}

static int mt5785_rx_5vbuck_chip_init(struct mt5785_dev_info *di)
{
	int ret;

	hwlog_info("[5vbuck_chip_init] chip init\n");
	ret = mt5785_write_block(di, MT5785_RX_LDO_CFG_ADDR,
		di->rx_ldo_cfg.l_volt, MT5785_RX_LDO_CFG_LEN);
	return ret;
}

static int mt5785_rx_chip_init(unsigned int init_type, unsigned int tx_type, void *dev_data)
{
	int ret = 0;
	struct mt5785_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	switch (init_type) {
	case WLRX_IC_DFLT_CHIP_INIT:
		hwlog_info("[chip_init] default chip init\n");
		di->g_val.rx_stop_chrg = false;
		ret += mt5785_write_word(di, MT5785_RX_WDT_TIMEOUT_ADDR,
			MT5785_RX_WDT_TIMEOUT);
		ret += mt5785_rx_kick_watchdog(di);
		/* fall through */
	case WLRX_IC_5VBUCK_CHIP_INIT:
		ret += mt5785_rx_5vbuck_chip_init(di);
		break;
	case WLRX_IC_9VBUCK_CHIP_INIT:
		hwlog_info("[chip_init] 9v chip init\n");
		mt5785_write_block(di, MT5785_RX_LDO_CFG_ADDR,
			di->rx_ldo_cfg.m_volt, MT5785_RX_LDO_CFG_LEN);
		break;
	case WLRX_IC_SC_CHIP_INIT:
		hwlog_info("[chip_init] sc chip init\n");
		mt5785_write_block(di, MT5785_RX_LDO_CFG_ADDR,
			di->rx_ldo_cfg.sc, MT5785_RX_LDO_CFG_LEN);
		break;
	default:
		hwlog_err("chip_init: input para invalid\n");
		break;
	}

	return ret;
}

static void mt5785_rx_stop_charging(void *dev_data)
{
	struct mt5785_dev_info *di = dev_data;

	if (!di)
		return;

	di->g_val.rx_stop_chrg = true;
	wlic_iout_stop_sample(di->ic_type);

	if (!di->g_val.irq_abnormal)
		return;
}

static int mt5785_rx_data_rcvd_handler(struct mt5785_dev_info *di)
{
	int ret;
	int i;
	u8 cmd;
	u8 buff[HWQI_PKT_LEN] = { 0 };

	ret = mt5785_read_block(di, MT5785_RX_FSK_HEADER_ADDR,
		buff, HWQI_PKT_LEN);
	if (ret) {
		hwlog_err("data_rcvd_handler: read received data failed\n");
		return ret;
	}

	cmd = buff[HWQI_PKT_CMD];
	hwlog_info("[data_rcvd_handler] cmd: 0x%x\n", cmd);
	for (i = HWQI_PKT_DATA; i < HWQI_PKT_LEN; i++)
		hwlog_info("[data_rcvd_handler] data: 0x%x\n", buff[i]);

	switch (cmd) {
	case HWQI_CMD_TX_ALARM:
	case HWQI_CMD_ACK_BST_ERR:
		di->irq_val &= ~MT5785_RX_IRQ_FSK_PKT;
		if (di->g_val.qi_hdl &&
			di->g_val.qi_hdl->hdl_non_qi_fsk_pkt)
			di->g_val.qi_hdl->hdl_non_qi_fsk_pkt(buff, HWQI_PKT_LEN, di);
		break;
	default:
		break;
	}
	return 0;
}

void mt5785_rx_abnormal_irq_handler(struct mt5785_dev_info *di)
{
}

static void mt5785_rx_ready_handler(struct mt5785_dev_info *di)
{
	if (wlrx_get_wired_channel_state() == WIRED_CHANNEL_ON) {
		hwlog_err("rx_ready_handler: wired channel on, ignore\n");
		return;
	}

	hwlog_info("[rx_ready_handler] rx ready, goto wireless charging\n");
	di->g_val.rx_stop_chrg = false;
	di->irq_cnt = 0;
	power_event_bnc_notify(POWER_BNT_WLRX, POWER_NE_WLRX_PREV_READY, NULL);
	power_msleep(DT_MSLEEP_50MS, 0, NULL); /* delay 50ms for volt steady */
	gpio_set_value(di->gpio_sleep_en, WLTRX_IC_SLEEP_DISABLE);
	wlic_iout_start_sample(di->ic_type);
	power_event_bnc_notify(POWER_BNT_WLRX, POWER_NE_WLRX_READY, NULL);
}

static void mt5785_rx_power_on_handler(struct mt5785_dev_info *di)
{
	u8 rx_ss = 0;
	int vrect = 0;
	int pwr_flag = WLRX_IC_PWR_ON_NOT_GOOD;

	if (wlrx_get_wired_channel_state() == WIRED_CHANNEL_ON) {
		hwlog_err("rx_power_on_handler: wired channel on, ignore\n");
		return;
	}

	(void)mt5785_rx_5vbuck_chip_init(di);
	mt5785_rx_abnormal_irq_handler(di);
	mt5785_read_byte(di, MT5785_RX_SS_ADDR, &rx_ss);
	mt5785_rx_get_vrect(&vrect, di);
	hwlog_info("[rx_power_on_handler] ss=%u vrect=%d\n", rx_ss, vrect);
	if ((rx_ss > di->rx_ss_good_lth) && (rx_ss <= MT5785_RX_SS_MAX))
		pwr_flag = WLRX_IC_PWR_ON_GOOD;

	mt5785_show_sr_status(di);
	power_event_bnc_notify(POWER_BNT_WLRX, POWER_NE_WLRX_PWR_ON, &pwr_flag);
}

static void mt5785_rx_mode_irq_recheck(struct mt5785_dev_info *di)
{
	int ret;
	u32 irq_val = 0;

	if (gpio_get_value(di->gpio_int))
		return;

	hwlog_info("[rx_mode_irq_recheck] gpio_int low, re-check irq\n");
	ret = mt5785_rx_get_interrupt(di, &irq_val);
	if (ret)
		return;

	if (irq_val & MT5785_RX_IRQ_READY)
		mt5785_rx_ready_handler(di);

	mt5785_rx_clear_irq(di, MT5785_RX_IRQ_CLR_ALL);
}

static void mt5785_rx_fault_irq_handler(struct mt5785_dev_info *di)
{
	if (di->irq_val & MT5785_RX_IRQ_OCP) {
		di->irq_val &= ~MT5785_RX_IRQ_OCP;
		power_event_bnc_notify(POWER_BNT_WLRX, POWER_NE_WLRX_OCP, NULL);
	}

	if (di->irq_val & MT5785_RX_IRQ_OVP) {
		di->irq_val &= ~MT5785_RX_IRQ_OVP;
		power_event_bnc_notify(POWER_BNT_WLRX, POWER_NE_WLRX_OVP, NULL);
	}

	if (di->irq_val & MT5785_RX_IRQ_OTP) {
		di->irq_val &= ~MT5785_RX_IRQ_OTP;
		power_event_bnc_notify(POWER_BNT_WLRX, POWER_NE_WLRX_OTP, NULL);
	}
}

void mt5785_rx_mode_irq_handler(struct mt5785_dev_info *di)
{
	int ret;

	if (!di)
		return;

	ret = mt5785_rx_get_interrupt(di, &di->irq_val);
	hwlog_info("[mode_irq_handler] irq_val=0x%x\n", di->irq_val);
	if (ret) {
		hwlog_err("irq_handler: read irq failed, clear\n");
		mt5785_rx_clear_irq(di, MT5785_RX_IRQ_CLR_ALL);
		mt5785_rx_abnormal_irq_handler(di);
		goto rechk_irq;
	}

	mt5785_rx_clear_irq(di, di->irq_val);

	if (di->irq_val & MT5785_RX_IRQ_POWER_ON) {
		di->irq_val &= ~MT5785_RX_IRQ_POWER_ON;
		mt5785_rx_power_on_handler(di);
	}
	if (di->irq_val & MT5785_RX_IRQ_READY) {
		di->irq_val &= ~MT5785_RX_IRQ_READY;
		mt5785_rx_ready_handler(di);
	}
	if (di->irq_val & MT5785_RX_IRQ_FSK_PKT)
		mt5785_rx_data_rcvd_handler(di);

	mt5785_rx_fault_irq_handler(di);

rechk_irq:
	mt5785_rx_mode_irq_recheck(di);
}

static void mt5785_rx_pmic_vbus_handler(bool vbus_state, void *dev_data)
{
	int ret;
	u32 irq_val = 0;
	struct mt5785_dev_info *di = dev_data;

	if (!di || !vbus_state || !di->g_val.irq_abnormal)
		return;

	if (wlrx_get_wired_channel_state() == WIRED_CHANNEL_ON)
		return;

	if (!mt5785_rx_is_tx_exist(di))
		return;

	ret = mt5785_rx_get_interrupt(di, &irq_val);
	if (ret) {
		hwlog_err("pmic_vbus_handler: read irq failed\n");
		return;
	}
	hwlog_info("[pmic_vbus_handler] irq_val=0x%x\n", irq_val);
	if (irq_val & MT5785_RX_IRQ_READY) {
		mt5785_rx_clear_irq(di, MT5785_RX_IRQ_CLR_ALL);
		mt5785_rx_ready_handler(di);
		di->irq_cnt = 0;
		di->g_val.irq_abnormal = false;
		mt5785_enable_irq(di);
	}
}

void mt5785_rx_probe_check_tx_exist(struct mt5785_dev_info *di)
{
	if (!di)
		return;

	if (mt5785_rx_is_tx_exist(di)) {
		mt5785_rx_clear_irq(di, MT5785_RX_IRQ_CLR_ALL);
		hwlog_info("[rx_probe_check_tx_exist] rx exist\n");
		mt5785_rx_ready_handler(di);
	} else {
		mt5785_sleep_enable(true, di);
	}
}

void mt5785_rx_shutdown_handler(struct mt5785_dev_info *di)
{
	if (!di || !mt5785_rx_is_tx_exist(di))
		return;

	wlps_control(di->ic_type, WLPS_RX_EXT_PWR, false);
	power_msleep(DT_MSLEEP_50MS, 0, NULL); /* delay 50ms for power off */
	(void)mt5785_rx_set_vfc(WLRX_IC_DFLT_VOUT, di);
	(void)mt5785_rx_set_rx_vout(WLRX_IC_DFLT_VOUT, di);
	(void)mt5785_chip_enable(false, di);
	power_msleep(DT_MSLEEP_200MS, 0, NULL); /* delay 200ms for chip disable */
	(void)mt5785_chip_enable(true, di);
}

static struct wlrx_ic_ops g_mt5785_rx_ic_ops = {
	.fw_update              = mt5785_fw_sram_update,
	.chip_init              = mt5785_rx_chip_init,
	.chip_reset             = mt5785_rx_chip_reset,
	.chip_enable            = mt5785_chip_enable,
	.is_chip_enable         = mt5785_is_chip_enable,
	.sleep_enable           = mt5785_sleep_enable,
	.is_sleep_enable        = mt5785_is_sleep_enable,
	.ext_pwr_post_ctrl      = mt5785_rx_ext_pwr_post_ctrl,
	.get_chip_info          = mt5785_get_chip_info_str,
	.get_vrect              = mt5785_rx_get_vrect,
	.get_vout               = mt5785_rx_get_vout,
	.get_iout               = mt5785_rx_get_iout,
	.get_iavg               = mt5785_rx_get_iavg,
	.get_imax               = mt5785_rx_get_imax,
	.get_vout_reg           = mt5785_rx_get_rx_vout_reg,
	.get_vfc_reg            = mt5785_rx_get_vfc_reg,
	.set_vfc                = mt5785_rx_set_vfc,
	.set_vout               = mt5785_rx_set_rx_vout,
	.get_fop                = mt5785_rx_get_fop,
	.get_cep                = mt5785_rx_get_cep,
	.get_temp               = mt5785_rx_get_temp,
	.get_fod_coef           = mt5785_rx_get_fod,
	.set_fod_coef           = mt5785_rx_set_fod,
	.is_tx_exist            = mt5785_rx_is_tx_exist,
	.kick_watchdog          = mt5785_rx_kick_watchdog,
	.send_ept               = mt5785_rx_send_ept,
	.stop_charging          = mt5785_rx_stop_charging,
	.pmic_vbus_handler      = mt5785_rx_pmic_vbus_handler,
};

int mt5785_rx_ops_register(struct wltrx_ic_ops *ops, struct mt5785_dev_info *di)
{
	if (!ops || !di)
		return -ENODEV;

	ops->rx_ops = kzalloc(sizeof(*(ops->rx_ops)), GFP_KERNEL);
	if (!ops->rx_ops)
		return -ENODEV;

	memcpy(ops->rx_ops, &g_mt5785_rx_ic_ops, sizeof(g_mt5785_rx_ic_ops));
	ops->rx_ops->dev_data = (void *)di;
	return wlrx_ic_ops_register(ops->rx_ops, di->ic_type);
}
