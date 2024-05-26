// SPDX-License-Identifier: GPL-2.0
/*
 * cps4067_rx.c
 *
 * cps4067 rx driver
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

#define HWLOG_TAG wireless_cps4067_rx
HWLOG_REGIST();

static const char * const g_cps4067_rx_irq_name[] = {
	/* [n]: n means bit in registers */
	[0]  = "power_on",
	[1]  = "ldo_off",
	[2]  = "ldo_on",
	[3]  = "ready",
	[4]  = "fsk_ack",
	[5]  = "fsk_timeout",
	[6]  = "fsk_pkt",
	[7]  = "vrect_ovp",
	[8]  = "vrect_h_ovp",
	[9]  = "vrect_ovp_to",
	[10] = "vrect_bvp",
	[11] = "otp",
	[12] = "htp",
	[13] = "ocp",
	[14] = "hocp",
	[15] = "scp",
	[16] = "mldo_opp",
	[17] = "mldo_uvp",
	[18] = "mldo_ovp",
	[19] = "ac_loss",
	[20] = "ac_short",
	[21] = "sr_ocp",
	[22] = "sr_err",
	[23] = "sr_sw_r",
	[24] = "sr_sw_f",
	[25] = "boost_fail",
	[26] = "half_bridge",
	[27] = "full_bridge",
};

static struct {
	int vpa;
	int vmldo;
} const g_rx_vfc_map[] = {
	{9000, 9000}, {12000, 9000}, {15000, 9000}, /* mV */
};

static int cps4067_rx_get_temp(int *temp, void *dev_data)
{
	s16 l_temp = 0;

	if (!temp || cps4067_read_word(dev_data, CPS4067_RX_CHIP_TEMP_ADDR, (u16 *)&l_temp))
		return -EINVAL;

	*temp = l_temp;
	return 0;
}

static int cps4067_rx_get_fop(int *fop, void *dev_data)
{
	return cps4067_read_word(dev_data, CPS4067_RX_FOP_ADDR, (u16 *)fop);
}

static int cps4067_rx_get_cep(int *cep, void *dev_data)
{
	s16 l_cep = 0;

	if (!cep || cps4067_read_word(dev_data, CPS4067_RX_CE_VAL_ADDR, (u16 *)&l_cep))
		return -EINVAL;

	*cep = l_cep;
	return 0;
}

static int cps4067_rx_get_vrect(int *vrect, void *dev_data)
{
	return cps4067_read_word(dev_data, CPS4067_RX_VRECT_ADDR, (u16 *)vrect);
}

static int cps4067_rx_get_vout(int *vout, void *dev_data)
{
	return cps4067_read_word(dev_data, CPS4067_RX_VOUT_ADDR, (u16 *)vout);
}

static int cps4067_rx_get_iout(int *iout, void *dev_data)
{
	return cps4067_read_word(dev_data, CPS4067_RX_IOUT_ADDR, (u16 *)iout);
}

static void cps4067_rx_get_iavg(int *iavg, void *dev_data)
{
	struct cps4067_dev_info *di = dev_data;

	if (!di)
		return;

	wlic_get_rx_iavg(di->ic_type, iavg);
}

static void cps4067_rx_get_imax(int *imax, void *dev_data)
{
	struct cps4067_dev_info *di = dev_data;

	if (!di) {
		if (imax)
			*imax = WLIC_DEFAULT_RX_IMAX;
		return;
	}
	wlic_get_rx_imax(di->ic_type, imax);
}

static int cps4067_rx_get_rx_vout_reg(int *vreg, void *dev_data)
{
	return cps4067_read_word(dev_data, CPS4067_RX_VOUT_SET_ADDR, (u16 *)vreg);
}

static int cps4067_rx_get_vfc_reg(int *vfc_reg, void *dev_data)
{
	return cps4067_read_word(dev_data, CPS4067_RX_FC_VPA_ADDR, (u16 *)vfc_reg);
}

static void cps4067_rx_show_irq(u32 intr)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(g_cps4067_rx_irq_name); i++) {
		if (intr & BIT(i))
			hwlog_info("[rx_irq] %s\n", g_cps4067_rx_irq_name[i]);
	}
}

static int cps4067_rx_get_interrupt(struct cps4067_dev_info *di, u32 *intr)
{
	int ret;

	ret = cps4067_read_dword(di, CPS4067_RX_IRQ_ADDR, intr);
	if (ret)
		return ret;

	hwlog_info("[get_interrupt] irq=0x%08x\n", *intr);
	cps4067_rx_show_irq(*intr);

	return 0;
}

static int cps4067_rx_clear_irq(struct cps4067_dev_info *di, u32 intr)
{
	int ret;

	ret = cps4067_write_dword(di, CPS4067_RX_IRQ_CLR_ADDR, intr);
	if (ret) {
		hwlog_err("clear_irq: failed\n");
		return ret;
	}

	return 0;
}

static void cps4067_sleep_enable(bool enable, void *dev_data)
{
	int gpio_val;
	struct cps4067_dev_info *di = dev_data;

	if (!di || di->g_val.irq_abnormal)
		return;

	gpio_set_value(di->gpio_sleep_en,
		enable ? WLTRX_IC_SLEEP_ENABLE : WLTRX_IC_SLEEP_DISABLE);
	gpio_val = gpio_get_value(di->gpio_sleep_en);
	hwlog_info("[sleep_enable] gpio %s now\n", gpio_val ? "high" : "low");
}

static bool cps4067_is_sleep_enable(void *dev_data)
{
	int gpio_val;
	struct cps4067_dev_info *di = dev_data;

	if (!di)
		return false;

	gpio_val = gpio_get_value(di->gpio_sleep_en);
	return gpio_val == WLTRX_IC_SLEEP_ENABLE;
}

static void cps4067_rx_chip_reset(void *dev_data)
{
	int ret;
	struct cps4067_dev_info *di = dev_data;

	if (!di)
		return;

	di->need_ignore_irq = true;
	ret = cps4067_write_dword_mask(dev_data, CPS4067_RX_CMD_ADDR,
		CPS4067_RX_CMD_SYS_RST, CPS4067_RX_CMD_SYS_RST_SHIFT,
		CPS4067_RX_CMD_VAL);
	if (ret) {
		hwlog_err("chip_reset: set cmd failed\n");
		return;
	}

	power_msleep(DT_MSLEEP_10MS, 0, NULL);
	di->need_ignore_irq = false;
	hwlog_info("[chip_reset] succ\n");
}

static void cps4067_rx_ext_pwr_post_ctrl(bool flag, void *dev_data)
{
	u16 val;

	if (flag)
		val = CPS4067_RX_FUNC_EN;
	else
		val = CPS4067_RX_FUNC_DIS;

	(void)cps4067_write_dword_mask(dev_data, CPS4067_RX_FUNC_EN_ADDR,
		CPS4067_RX_EXT_VCC_EN_MASK, CPS4067_RX_EXT_VCC_EN_SHIFT, val);
}

static int cps4067_rx_set_rx_vout(int vout, void *dev_data)
{
	int ret;

	if ((vout < CPS4067_RX_VOUT_MIN) || (vout > CPS4067_RX_VOUT_MAX)) {
		hwlog_err("set_rx_vout: out of range\n");
		return -EINVAL;
	}

	ret = cps4067_write_word(dev_data, CPS4067_RX_VOUT_SET_ADDR, (u16)vout);
	if (ret) {
		hwlog_err("set_rx_vout: failed\n");
		return ret;
	}

	return 0;
}

static void cps4067_ask_mode_cfg(struct cps4067_dev_info *di, u8 cm, u8 polarity)
{
	int ret;

	ret = cps4067_write_dword_mask(di, CPS4067_RX_FUNC_EN_ADDR,
		CPS4067_RX_CMALL_EN_MASK, CPS4067_RX_CMALL_EN_SHIFT, cm);
	ret += cps4067_write_dword_mask(di, CPS4067_RX_FUNC_EN_ADDR,
		CPS4067_RX_CM_POLARITY_MASK, CPS4067_RX_CM_POLARITY_SHIFT, polarity);
	if (!ret) {
		hwlog_info("[ask_mode_cfg] succ, cm=0x%x, polar=0x%x\n", cm, polarity);
		return;
	}

	hwlog_err("ask_mode_cfg: failed, cm=0x%x, polar=0x%x\n", cm, polarity);
}

static void cps4067_set_mode_cfg(struct cps4067_dev_info *di, int vfc)
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
	cps4067_ask_mode_cfg(di, cm_cfg, polar_cfg);
}

static int cps4067_rx_enable_boost_mode(struct cps4067_dev_info *di)
{
	int ret;

	ret = cps4067_write_byte(di, CPS4067_RX_FC_BST_DIS_ADDR, 0); /* 0: disable */
	ret += cps4067_write_byte(di, CPS4067_RX_FC_BST_EN_ADDR, 1); /* 1: enable */

	return ret;
}

static int cps4067_rx_disable_boost_mode(struct cps4067_dev_info *di)
{
	int ret;

	ret = cps4067_write_byte(di, CPS4067_RX_FC_BST_EN_ADDR, 0); /* 0: disable */
	ret += cps4067_write_byte(di, CPS4067_RX_FC_BST_DIS_ADDR, 1); /* 1: enable */

	return ret;
}

static void cps4067_show_sr_status(struct cps4067_dev_info *di)
{
	u8 sr_status = 0;

	cps4067_read_byte_mask(di, CPS4067_RX_STATUS_ADDR, CPS4067_RX_SR_BRI_MASK,
		CPS4067_RX_SR_BRI_SHIFT, &sr_status);
	hwlog_info("[sr_status] boost mode %s\n",
		sr_status ? "enabled" : "disabled"); /* 1: SR is in half-bridge mode */
}

static int cps4067_rx_set_vpa(struct cps4067_dev_info *di, u16 vpa)
{
	return cps4067_write_word(di, CPS4067_RX_FC_VPA_ADDR, vpa);
}

static int cps4067_rx_get_vmldo(u16 vfc)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(g_rx_vfc_map); i++) {
		if (vfc == g_rx_vfc_map[i].vpa)
			return g_rx_vfc_map[i].vmldo;
	}

	hwlog_err("get_vmldo: mismatch\n");
	return -EINVAL;
}

static int cps4067_rx_set_vmldo(struct cps4067_dev_info *di, u16 vfc)
{
	int vmldo, ret;

	vmldo = cps4067_rx_get_vmldo(vfc);
	if (vmldo < 0)
		return -EINVAL;

	ret = cps4067_write_word(di, CPS4067_RX_FC_VMLDO_ADDR, vmldo);
	ret += cps4067_rx_set_rx_vout(vmldo, di);
	return ret;
}

static int cps4067_rx_send_fc_cmd(struct cps4067_dev_info *di, int vfc)
{
	int ret;

	ret = cps4067_rx_set_vpa(di, vfc);
	if (ret) {
		hwlog_err("send_fc_cmd: set fc_vpa failed\n");
		return ret;
	}
	ret = cps4067_write_byte_mask(di, CPS4067_RX_CMD_ADDR,
		CPS4067_RX_CMD_SEND_FC, CPS4067_RX_CMD_SEND_FC_SHIFT,
		CPS4067_RX_CMD_VAL);
	if (ret) {
		hwlog_err("send_fc_cmd: send fc_cmd failed\n");
		return ret;
	}

	return 0;
}

static bool cps4067_rx_is_fc_succ(struct cps4067_dev_info *di)
{
	int i;
	int cnt;
	u8 rx_status;

	cnt = CPS4067_RX_FC_VOUT_TIMEOUT / CPS4067_RX_FC_VOUT_SLEEP_TIME;
	for (i = 0; i < cnt; i++) {
		power_msleep(CPS4067_RX_FC_VOUT_SLEEP_TIME, 0, NULL); /* wait for vout stability */
		cps4067_read_byte_mask(di, CPS4067_RX_STATUS_ADDR, CPS4067_RX_FC_SUCC_MASK,
			CPS4067_RX_FC_SUCC_SHIFT, &rx_status);
		if (rx_status) {
			hwlog_info("[is_fc_succ] succ, cost_time: %dms\n",
				(i + 1) * CPS4067_RX_FC_VOUT_SLEEP_TIME);
			return true;
		}
	}

	hwlog_err("is_fc_succ: timeout\n");
	return false;
}

static bool cps4067_rx_is_vmldo_set_succ(struct cps4067_dev_info *di, int vfc)
{
	int i;
	int cnt;
	int vout;
	int vmldo;

	vmldo = cps4067_rx_get_vmldo(vfc);
	if (vmldo < 0)
		return false;

	cnt = CPS4067_RX_SET_VMLDO_SLEEP_TIMEOUT / CPS4067_RX_SET_VMLDO_SLEEP_TIME;
	for (i = 0; i < cnt; i++) {
		if (di->g_val.rx_stop_chrg)
			return false;
		power_msleep(CPS4067_RX_SET_VMLDO_SLEEP_TIME, 0, NULL); /* wait for vout stability */
		(void)cps4067_rx_get_vout(&vout, di);
		if (vout >= (vmldo - 200)) { /* 200: maximum deviation */
			hwlog_info("[is_vmldo_set_succ] succ, cost_time: %dms\n",
				(i + 1) * CPS4067_RX_SET_VMLDO_SLEEP_TIME);
			return true;
		}
	}

	hwlog_err("is_vmldo_set_succ: timeout\n");
	return true;
}

static int cps4067_rx_set_vfc(int vfc, void *dev_data)
{
	int i;
	int ret;
	struct cps4067_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	if (vfc >= WLRX_IC_HIGH_VOUT2) {
		ret = cps4067_rx_disable_boost_mode(di);
		if (ret)
			return ret;
	}

	for (i = 0; i < CPS4067_RX_FC_VPA_RETRY_CNT; i++) {
		if (di->g_val.rx_stop_chrg && (vfc > WLRX_IC_DFLT_VOUT))
			return -EPERM;
		ret = cps4067_rx_send_fc_cmd(di, vfc);
		if (ret) {
			hwlog_err("set_vfc: send fc_cmd failed\n");
			continue;
		}
		hwlog_info("[set_vfc] send fc_cmd, cnt: %d\n", i);
		if (cps4067_rx_is_fc_succ(di)) {
			ret = cps4067_rx_set_vmldo(di, vfc);
			if (ret) {
				hwlog_err("set_vfc: send fc_vmldo failed\n");
				continue;
			}
			if (!cps4067_rx_is_vmldo_set_succ(di, vfc))
				continue;
			if (vfc < WLRX_IC_HIGH_VOUT2)
				(void)cps4067_rx_enable_boost_mode(di);
			cps4067_set_mode_cfg(di, vfc);
			hwlog_info("[set_vfc] succ\n");
			return 0;
		}
	}
	return 0;
}

static void cps4067_rx_send_ept(int ept_type, void *dev_data)
{
	int ret;

	switch (ept_type) {
	case HWQI_EPT_ERR_VRECT:
	case HWQI_EPT_ERR_VOUT:
		break;
	default:
		return;
	}
	ret = cps4067_write_byte(dev_data, CPS4067_RX_EPT_MSG_ADDR, (u8)ept_type);
	ret += cps4067_write_byte_mask(dev_data, CPS4067_RX_CMD_ADDR,
		CPS4067_RX_CMD_SEND_EPT, CPS4067_RX_CMD_SEND_EPT_SHIFT,
		CPS4067_RX_CMD_VAL);
	if (ret)
		hwlog_err("send_ept: failed, ept=0x%x\n", ept_type);
}

bool cps4067_rx_is_tx_exist(void *dev_data)
{
	int ret;
	u8 mode = 0;

	ret = cps4067_get_mode(dev_data, &mode);
	if (ret) {
		hwlog_err("is_tx_exist: get rx mode fail\n");
		return false;
	}

	return mode == CPS4067_SYS_MODE_RX;
}

static int cps4067_rx_kick_watchdog(void *dev_data)
{
	return cps4067_write_word(dev_data, CPS4067_RX_WDT_ADDR, CPS4067_RX_WDT_TO);
}

static int cps4067_rx_get_fod(char *fod_str, int len, void *dev_data)
{
	return 0;
}

static int cps4067_rx_set_fod(const char *fod_str, void *dev_data)
{
	return 0;
}

static int cps4067_rx_5vbuck_chip_init(struct cps4067_dev_info *di)
{
	int ret;

	hwlog_info("[5vbuck_chip_init] chip init\n");
	ret = cps4067_write_block(di, CPS4067_RX_LDO_CFG_ADDR,
		di->rx_ldo_cfg.l_volt, CPS4067_RX_LDO_CFG_LEN);
	ret += cps4067_write_byte(di, CPS4067_RX_DUMMY_LOAD_NO_MOD_ADDR,
		CPS4067_RX_DUMMY_LOAD_NO_MOD_VAL);
	ret += cps4067_write_byte(di, CPS4067_RX_DUMMY_LOAD_MOD_ADDR,
		CPS4067_RX_DUMMY_LOAD_MOD_VAL);
	ret += cps4067_write_word(di, CPS4067_RX_FC_DELTA_ADDR,
		CPS4067_RX_FC_DELTA_VAL);

	return ret;
}

static int cps4067_rx_chip_init(unsigned int init_type, unsigned int tx_type, void *dev_data)
{
	int ret = 0;
	struct cps4067_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	switch (init_type) {
	case WLRX_IC_DFLT_CHIP_INIT:
		hwlog_info("[chip_init] default chip init\n");
		di->g_val.rx_stop_chrg = false;
		ret += cps4067_write_dword_mask(di, CPS4067_RX_FUNC_EN_ADDR,
			CPS4067_RX_HV_WDT_EN_MASK, CPS4067_RX_HV_WDT_EN_SHIFT,
			CPS4067_RX_FUNC_EN);
		ret += cps4067_rx_kick_watchdog(di);
		/* fall through */
	case WLRX_IC_5VBUCK_CHIP_INIT:
		ret += cps4067_rx_5vbuck_chip_init(di);
		break;
	case WLRX_IC_9VBUCK_CHIP_INIT:
		hwlog_info("[chip_init] 9v chip init\n");
		cps4067_write_block(di, CPS4067_RX_LDO_CFG_ADDR,
			di->rx_ldo_cfg.m_volt, CPS4067_RX_LDO_CFG_LEN);
		break;
	case WLRX_IC_SC_CHIP_INIT:
		hwlog_info("[chip_init] sc chip init\n");
		cps4067_write_block(di, CPS4067_RX_LDO_CFG_ADDR,
			di->rx_ldo_cfg.sc, CPS4067_RX_LDO_CFG_LEN);
		break;
	default:
		hwlog_err("chip_init: input para invalid\n");
		break;
	}

	return ret;
}

static void cps4067_rx_stop_charging(void *dev_data)
{
	struct cps4067_dev_info *di = dev_data;

	if (!di)
		return;

	di->g_val.rx_stop_chrg = true;
	wlic_iout_stop_sample(di->ic_type);

	if (!di->g_val.irq_abnormal)
		return;
}

static int cps4067_rx_data_rcvd_handler(struct cps4067_dev_info *di)
{
	int ret;
	int i;
	u8 cmd;
	u8 buff[HWQI_PKT_LEN] = { 0 };

	ret = cps4067_read_block(di, CPS4067_RCVD_MSG_HEADER_ADDR,
		buff, HWQI_PKT_LEN);
	if (ret) {
		hwlog_err("data_received_handler: read received data failed\n");
		return ret;
	}

	cmd = buff[HWQI_PKT_CMD];
	hwlog_info("[data_received_handler] cmd: 0x%x\n", cmd);
	for (i = HWQI_PKT_DATA; i < HWQI_PKT_LEN; i++)
		hwlog_info("[data_received_handler] data: 0x%x\n", buff[i]);

	switch (cmd) {
	case HWQI_CMD_TX_ALARM:
	case HWQI_CMD_ACK_BST_ERR:
		di->irq_val &= ~CPS4067_RX_IRQ_FSK_PKT;
		if (di->g_val.qi_hdl &&
			di->g_val.qi_hdl->hdl_non_qi_fsk_pkt)
			di->g_val.qi_hdl->hdl_non_qi_fsk_pkt(buff, HWQI_PKT_LEN, di);
		break;
	default:
		break;
	}
	return 0;
}

void cps4067_rx_abnormal_irq_handler(struct cps4067_dev_info *di)
{
}

static void cps4067_rx_ready_handler(struct cps4067_dev_info *di)
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

static void cps4067_rx_power_on_handler(struct cps4067_dev_info *di)
{
	u8 rx_ss = 0; /* ss: signal strength */
	int vrect = 0;
	int pwr_flag = WLRX_IC_PWR_ON_NOT_GOOD;

	if (wlrx_get_wired_channel_state() == WIRED_CHANNEL_ON) {
		hwlog_err("rx_power_on_handler: wired channel on, ignore\n");
		return;
	}

	(void)cps4067_rx_5vbuck_chip_init(di);
	cps4067_rx_abnormal_irq_handler(di);
	cps4067_read_byte(di, CPS4067_RX_SS_ADDR, &rx_ss);
	cps4067_rx_get_vrect(&vrect, di);
	hwlog_info("[rx_power_on_handler] ss=%u vrect=%d\n", rx_ss, vrect);
	if ((rx_ss > di->rx_ss_good_lth) && (rx_ss <= CPS4067_RX_SS_MAX))
		pwr_flag = WLRX_IC_PWR_ON_GOOD;
	cps4067_show_sr_status(di);

	power_event_bnc_notify(POWER_BNT_WLRX, POWER_NE_WLRX_PWR_ON, &pwr_flag);
}

static void cps4067_rx_mode_irq_recheck(struct cps4067_dev_info *di)
{
	int ret;
	u32 irq_val = 0;

	if (gpio_get_value(di->gpio_int))
		return;

	hwlog_info("[rx_mode_irq_recheck] gpio_int low, re-check irq\n");
	ret = cps4067_rx_get_interrupt(di, &irq_val);
	if (ret)
		return;

	if (irq_val & CPS4067_RX_IRQ_READY)
		cps4067_rx_ready_handler(di);

	cps4067_rx_clear_irq(di, CPS4067_RX_IRQ_CLR_ALL);
}

static void cps4067_rx_fault_irq_handler(struct cps4067_dev_info *di)
{
	if (di->irq_val & CPS4067_RX_IRQ_OCP) {
		di->irq_val &= ~CPS4067_RX_IRQ_OCP;
		power_event_bnc_notify(POWER_BNT_WLRX, POWER_NE_WLRX_OCP, NULL);
	}

	if (di->irq_val & CPS4067_RX_IRQ_VRECT_OVP) {
		di->irq_val &= ~CPS4067_RX_IRQ_VRECT_OVP;
		power_event_bnc_notify(POWER_BNT_WLRX, POWER_NE_WLRX_OVP, NULL);
	}

	if (di->irq_val & CPS4067_RX_IRQ_MLDO_OVP) {
		di->irq_val &= ~CPS4067_RX_IRQ_MLDO_OVP;
		power_event_bnc_notify(POWER_BNT_WLRX, POWER_NE_WLRX_OVP, NULL);
	}

	if (di->irq_val & CPS4067_RX_IRQ_OTP) {
		di->irq_val &= ~CPS4067_RX_IRQ_OTP;
		power_event_bnc_notify(POWER_BNT_WLRX, POWER_NE_WLRX_OTP, NULL);
	}
}

void cps4067_rx_mode_irq_handler(struct cps4067_dev_info *di)
{
	int ret;

	if (!di)
		return;

	ret = cps4067_rx_get_interrupt(di, &di->irq_val);
	if (ret) {
		hwlog_err("irq_handler: read irq failed, clear\n");
		cps4067_rx_clear_irq(di, CPS4067_RX_IRQ_CLR_ALL);
		cps4067_rx_abnormal_irq_handler(di);
		goto rechk_irq;
	}

	cps4067_rx_clear_irq(di, di->irq_val);

	if (di->irq_val & CPS4067_RX_IRQ_POWER_ON) {
		di->irq_val &= ~CPS4067_RX_IRQ_POWER_ON;
		cps4067_rx_power_on_handler(di);
	}
	if (di->irq_val & CPS4067_RX_IRQ_READY) {
		di->irq_val &= ~CPS4067_RX_IRQ_READY;
		cps4067_rx_ready_handler(di);
	}
	if (di->irq_val & CPS4067_RX_IRQ_FSK_PKT)
		cps4067_rx_data_rcvd_handler(di);

	cps4067_rx_fault_irq_handler(di);

rechk_irq:
	cps4067_rx_mode_irq_recheck(di);
}

static void cps4067_rx_pmic_vbus_handler(bool vbus_state, void *dev_data)
{
	int ret;
	u32 irq_val = 0;
	struct cps4067_dev_info *di = dev_data;

	if (!di || !vbus_state || !di->g_val.irq_abnormal)
		return;

	if (wlrx_get_wired_channel_state() == WIRED_CHANNEL_ON)
		return;

	if (!cps4067_rx_is_tx_exist(di))
		return;

	ret = cps4067_rx_get_interrupt(di, &irq_val);
	if (ret) {
		hwlog_err("pmic_vbus_handler: read irq failed, clear\n");
		return;
	}
	hwlog_info("[pmic_vbus_handler] irq_val=0x%x\n", irq_val);
	if (irq_val & CPS4067_RX_IRQ_READY) {
		cps4067_rx_clear_irq(di, CPS4067_RX_IRQ_CLR_ALL);
		cps4067_rx_ready_handler(di);
		di->irq_cnt = 0;
		di->g_val.irq_abnormal = false;
		cps4067_enable_irq(di);
	}
}

void cps4067_rx_probe_check_tx_exist(struct cps4067_dev_info *di)
{
	if (!di)
		return;

	if (cps4067_rx_is_tx_exist(di)) {
		cps4067_rx_clear_irq(di, CPS4067_RX_IRQ_CLR_ALL);
		hwlog_info("[rx_probe_check_tx_exist] rx exist\n");
		cps4067_rx_ready_handler(di);
	} else {
		cps4067_sleep_enable(true, di);
	}
}

void cps4067_rx_shutdown_handler(struct cps4067_dev_info *di)
{
	if (!di || !cps4067_rx_is_tx_exist(di))
		return;

	wlps_control(di->ic_type, WLPS_RX_EXT_PWR, false);
	power_msleep(DT_MSLEEP_50MS, 0, NULL); /* delay 50ms for power off */
	(void)cps4067_rx_set_vfc(WLRX_IC_DFLT_VOUT, di);
	(void)cps4067_rx_set_rx_vout(WLRX_IC_DFLT_VOUT, di);
	(void)cps4067_chip_enable(false, di);
	power_msleep(DT_MSLEEP_200MS, 0, NULL); /* delay 200ms for chip disable */
	(void)cps4067_chip_enable(true, di);
}

static struct wlrx_ic_ops g_cps4067_rx_ic_ops = {
	.fw_update              = cps4067_fw_sram_update,
	.chip_init              = cps4067_rx_chip_init,
	.chip_reset             = cps4067_rx_chip_reset,
	.chip_enable            = cps4067_chip_enable,
	.is_chip_enable         = cps4067_is_chip_enable,
	.sleep_enable           = cps4067_sleep_enable,
	.is_sleep_enable        = cps4067_is_sleep_enable,
	.ext_pwr_post_ctrl      = cps4067_rx_ext_pwr_post_ctrl,
	.get_chip_info          = cps4067_get_chip_info_str,
	.get_vrect              = cps4067_rx_get_vrect,
	.get_vout               = cps4067_rx_get_vout,
	.get_iout               = cps4067_rx_get_iout,
	.get_iavg               = cps4067_rx_get_iavg,
	.get_imax               = cps4067_rx_get_imax,
	.get_vout_reg           = cps4067_rx_get_rx_vout_reg,
	.get_vfc_reg            = cps4067_rx_get_vfc_reg,
	.set_vfc                = cps4067_rx_set_vfc,
	.set_vout               = cps4067_rx_set_rx_vout,
	.get_fop                = cps4067_rx_get_fop,
	.get_cep                = cps4067_rx_get_cep,
	.get_temp               = cps4067_rx_get_temp,
	.get_fod_coef           = cps4067_rx_get_fod,
	.set_fod_coef           = cps4067_rx_set_fod,
	.is_tx_exist            = cps4067_rx_is_tx_exist,
	.kick_watchdog          = cps4067_rx_kick_watchdog,
	.send_ept               = cps4067_rx_send_ept,
	.stop_charging          = cps4067_rx_stop_charging,
	.pmic_vbus_handler      = cps4067_rx_pmic_vbus_handler,
};

int cps4067_rx_ops_register(struct wltrx_ic_ops *ops, struct cps4067_dev_info *di)
{
	if (!ops || !di)
		return -ENODEV;

	ops->rx_ops = kzalloc(sizeof(*(ops->rx_ops)), GFP_KERNEL);
	if (!ops->rx_ops)
		return -ENODEV;

	memcpy(ops->rx_ops, &g_cps4067_rx_ic_ops, sizeof(g_cps4067_rx_ic_ops));
	ops->rx_ops->dev_data = (void *)di;
	return wlrx_ic_ops_register(ops->rx_ops, di->ic_type);
}
