// SPDX-License-Identifier: GPL-2.0
/*
 * stwlc68_tx.c
 *
 * stwlc68 tx driver
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

#define HWLOG_TAG wireless_stwlc68_tx
HWLOG_REGIST();

static const char * const g_stwlc68_tx_irq_name[] = {
	/* [n]: n means bit in registers */
	[0]  = "tx_otp",
	[1]  = "tx_ocp",
	[2]  = "tx_ovp",
	[3]  = "tx_sys_err",
	[4]  = "tx_rpp_rcvd",
	[5]  = "tx_cep_rcvd",
	[6]  = "tx_dping_rcvd",
	[7]  = "tx_rp_dm_timeout",
	[8]  = "tx_send_pkt_succ",
	[9]  = "tx_send_pkt_timeout",
	[10] = "tx_ept",
	[11] = "tx_start_ping",
	[12] = "tx_ss_pkt_rcvd",
	[13] = "tx_id_pkt_rcvd",
	[14] = "tx_cfg_pkt_rcvd",
	[15] = "tx_pp_pkt_rcvd",
};

static const char * const g_stwlc68_tx_ept_name[] = {
	/* [n]: n means bit in registers */
	[0]  = "tx_ept_src_cmd",
	[1]  = "tx_ept_src_ss",
	[2]  = "tx_ept_src_id",
	[3]  = "tx_ept_src_xid",
	[4]  = "tx_ept_src_cfg_cnt",
	[5]  = "tx_ept_src_cfg_err",
	[6]  = "tx_ept_src_rx_ept",
	[7]  = "tx_ept_src_ping_timeout",
	[8]  = "tx_ept_src_cep_timeout",
	[9]  = "tx_ept_src_rpp_timeout",
	[10] = "tx_ept_src_ocp",
	[11] = "tx_ept_src_ovp",
	[12] = "tx_ept_src_lvp",
	[13] = "tx_ept_src_fod",
	[14] = "tx_ept_src_otp",
	[15] = "tx_ept_src_lcp",
};

static unsigned int stwlc68_tx_get_bnt_wltx_type(int ic_type)
{
	return ic_type == WLTRX_IC_AUX ? POWER_BNT_WLTX_AUX : POWER_BNT_WLTX;
}

int stwlc68_sw2tx(struct stwlc68_dev_info *di)
{
	int ret;
	int i;
	u8 mode = 0;
	int cnt = STWLC68_SW2TX_RETRY_TIME / STWLC68_SW2TX_RETRY_SLEEP_TIME;

	for (i = 0; i < cnt; i++) {
		if (!di->g_val.tx_open_flag) {
			hwlog_err("sw2tx: tx_open_flag false\n");
			return -EPERM;
		}
		(void)power_msleep(STWLC68_SW2TX_RETRY_SLEEP_TIME, 0, NULL);
		ret = stwlc68_get_mode(di, &mode);
		if (ret) {
			hwlog_err("sw2tx: get mode fail\n");
			continue;
		}

		ret = stwlc68_write_word_mask(di, STWLC68_CMD_ADDR, STWLC68_CMD_SW2TX,
			STWLC68_CMD_SW2TX_SHIFT, STWLC68_CMD_VAL);
		if (ret) {
			hwlog_err("sw2tx: write cmd(sw2tx) fail\n");
			continue;
		}
		if (mode == STWLC68_TX_MODE) {
			hwlog_info("sw2tx: succ, cnt = %d\n", i);
			return 0;
		}
	}
	hwlog_err("sw2tx: fail, cnt = %d\n", i);
	return -ENXIO;
}

static int stwlc68_tx_fw_update(void *dev_data)
{
	return stwlc68_fw_sram_update(dev_data, WIRELESS_TX);
}

static bool stwlc68_tx_in_tx_mode(void *dev_data)
{
	int ret;
	u8 mode = 0;
	struct stwlc68_dev_info *di = dev_data;

	ret = stwlc68_read_byte(di, STWLC68_OP_MODE_ADDR, &mode);
	if (ret) {
		hwlog_err("in_tx_mode: fail\n");
		return false;
	}

	return mode == STWLC68_TX_MODE;
}

static bool stwlc68_tx_in_rx_mode(void *dev_data)
{
	return stwlc68_rx_is_tx_exist(dev_data);
}

static int stwlc68_tx_set_tx_open_flag(bool enable, void *dev_data)
{
	struct stwlc68_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	di->g_val.tx_open_flag = enable;
	return 0;
}

static int stwlc68_tx_set_vset(int tx_vset, void *dev_data)
{
	struct stwlc68_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	if (tx_vset == STWLC68_PS_TX_VOLT_5V5)
		return stwlc68_write_byte(dev_data, STWLC68_PS_TX_GPIO_ADDR,
			STWLC68_PS_TX_GPIO_PU);
	else if ((tx_vset == STWLC68_PS_TX_VOLT_6V8) || (tx_vset == STWLC68_PS_TX_VOLT_6V))
		return stwlc68_write_byte(dev_data, STWLC68_PS_TX_GPIO_ADDR,
			STWLC68_PS_TX_GPIO_OPEN);
	else if ((tx_vset == STWLC68_PS_TX_VOLT_10V) || (tx_vset == STWLC68_PS_TX_VOLT_6V9))
		return stwlc68_write_byte(dev_data, STWLC68_PS_TX_GPIO_ADDR,
			STWLC68_PS_TX_GPIO_PD);

	if (di->ic_type == WLTRX_IC_AUX)
		return 0;

	hwlog_err("set_vset: para err\n");
	return -EINVAL;
}

static bool stwlc68_tx_check_rx_disconnect(void *dev_data)
{
	struct stwlc68_dev_info *di = dev_data;

	if (!di)
		return true;

	if (di->ept_type & STWLC68_TX_EPT_SRC_CEP_TIMEOUT) {
		di->ept_type &= ~STWLC68_TX_EPT_SRC_CEP_TIMEOUT;
		hwlog_info("[check_rx_disconnect] RX disconnect\n");
		return true;
	}

	return false;
}

static int stwlc68_tx_get_ping_interval(u16 *ping_interval, void *dev_data)
{
	return stwlc68_read_word(dev_data, STWLC68_TX_PING_INTERVAL, ping_interval);
}

static int stwlc68_tx_set_ping_interval(u16 ping_interval, void *dev_data)
{
	if ((ping_interval < STWLC68_TX_PING_INTERVAL_MIN) ||
		(ping_interval > STWLC68_TX_PING_INTERVAL_MAX)) {
		hwlog_err("set_ping_interval: ping interval is out of range\n");
		return -EINVAL;
	}

	return stwlc68_write_word(dev_data, STWLC68_TX_PING_INTERVAL, ping_interval);
}

static int stwlc68_tx_get_ping_freq(u16 *ping_freq, void *dev_data)
{
	return stwlc68_read_byte(dev_data, STWLC68_TX_PING_FREQ_ADDR, (u8 *)ping_freq);
}

static int stwlc68_tx_set_ping_freq(u16 ping_freq, void *dev_data)
{
	if ((ping_freq < STWLC68_TX_PING_FREQ_MIN) || (ping_freq > STWLC68_TX_PING_FREQ_MAX)) {
		hwlog_err("set_ping_freq: ping frequency is out of range\n");
		return -EINVAL;
	}

	return stwlc68_write_byte(dev_data, STWLC68_TX_PING_FREQ_ADDR, (u8)ping_freq);
}

static int stwlc68_tx_get_min_fop(u16 *fop, void *dev_data)
{
	return stwlc68_read_byte(dev_data, STWCL68_TX_MIN_FOP_ADDR, (u8 *)fop);
}

static int stwlc68_tx_set_min_fop(u16 fop, void *dev_data)
{
	if ((fop < STWLC68_TX_MIN_FOP_VAL) || (fop > STWLC68_TX_MAX_FOP_VAL))
		return -EINVAL;

	return stwlc68_write_byte(dev_data, STWCL68_TX_MIN_FOP_ADDR, (u8)fop);
}

static int stwlc68_tx_get_max_fop(u16 *fop, void *dev_data)
{
	return stwlc68_read_byte(dev_data, STWLC68_TX_MAX_FOP_ADDR, (u8 *)fop);
}

static int stwlc68_tx_set_max_fop(u16 fop, void *dev_data)
{
	if ((fop < STWLC68_TX_MIN_FOP_VAL) || (fop > STWLC68_TX_MAX_FOP_VAL))
		return -EINVAL;

	return stwlc68_write_byte(dev_data, STWLC68_TX_MAX_FOP_ADDR, (u8)fop);
}

static int stwlc68_tx_get_fop(u16 *fop, void *dev_data)
{
	return stwlc68_read_word(dev_data, STWLC68_TX_OP_FREQ_ADDR, fop);
}

static int stwlc68_tx_get_duty(u8 *duty, void *dev_data)
{
	if (!duty)
		return -EINVAL;

	*duty = 50; /* default 50, full duty */
	return 0;
}

static int stwlc68_tx_get_cep(s8 *cep, void *dev_data)
{
	return stwlc68_read_byte(dev_data, STWLC68_TX_CEP_ADDR, cep);
}

static int stwlc68_tx_get_ptx(u32 *ptx, void *dev_data)
{
	return stwlc68_read_word(dev_data, STWLC68_TX_TFRD_PWR_ADDR, (u16 *)ptx);
}

static int stwlc68_tx_get_prx(u32 *prx, void *dev_data)
{
	return stwlc68_read_word(dev_data, STWLC68_TX_RCVD_PWR_ADDR, (u16 *)prx);
}

static int stwlc68_tx_get_ploss(s32 *ploss, void *dev_data)
{
	int ret;
	u32 ptx = 0;
	u32 prx = 0;

	if (!ploss)
		return -EINVAL;

	ret = stwlc68_tx_get_ptx(&ptx, dev_data);
	ret += stwlc68_tx_get_prx(&prx, dev_data);
	if (ret)
		return -EIO;

	*ploss = ptx - prx;
	return 0;
}

static int stwlc68_tx_get_ploss_id(u8 *id, void *dev_data)
{
	if (!id)
		return -EINVAL;

	*id = 0; /* default 0, 5v ploss */
	return 0;
}

static int stwlc68_tx_get_temp(s16 *chip_temp, void *dev_data)
{
	return stwlc68_read_word(dev_data, STWLC68_CHIP_TEMP_ADDR, chip_temp);
}

static int stwlc68_tx_get_vin(u16 *tx_vin, void *dev_data)
{
	return stwlc68_read_word(dev_data, STWLC68_TX_VIN_ADDR, tx_vin);
}

static int stwlc68_tx_get_vrect(u16 *tx_vrect, void *dev_data)
{
	return stwlc68_read_word(dev_data, STWLC68_TX_VRECT_ADDR, tx_vrect);
}

static int stwlc68_tx_get_iin(u16 *tx_iin, void *dev_data)
{
	return stwlc68_read_word(dev_data, STWLC68_TX_IIN_ADDR, tx_iin);
}

static int stwlc68_tx_set_ilimit(u16 tx_ilim, void *dev_data)
{
	if ((tx_ilim < STWLC68_TX_ILIMIT_MIN) || (tx_ilim > STWLC68_TX_ILIMIT_MAX))
		return -EINVAL;

	return stwlc68_write_byte(dev_data, STWLC68_TX_ILIMIT,
		(u8)(tx_ilim / STWLC68_TX_ILIMIT_STEP));
}

static int stwlc68_tx_set_fod_coef(u16 pl_th, u8 pl_cnt, void *dev_data)
{
	int ret;

	pl_th /= STWLC68_TX_PLOSS_TH_UNIT;
	/* tx ploss threshold 0:disabled */
	ret = stwlc68_write_byte(dev_data, STWLC68_TX_PLOSS_TH_ADDR, (u8)pl_th);
	/* tx ploss fod debounce count 0:no debounce */
	ret += stwlc68_write_byte(dev_data, STWLC68_TX_PLOSS_CNT_ADDR, pl_cnt);

	return ret;
}

static int stwlc68_tx_set_rp_dm_timeout_val(u8 val, void *dev_data)
{
	if (stwlc68_write_byte(dev_data, STWLC68_TX_RP_TIMEOUT_ADDR, val))
		hwlog_err("set_rp_dm_timeout_val: fail\n");

	return 0;
}

static int stwlc68_tx_stop_config(void *dev_data)
{
	return 0;
}

static int stwlc68_tx_chip_init(unsigned int client, void *dev_data)
{
	int ret;
	struct stwlc68_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	di->irq_cnt = 0;
	di->g_val.irq_abnormal_flag = false;
	/*
	 * when stwlc68_handle_abnormal_irq disable irq, tx will irq-mask
	 * so, here enable irq
	 */
	stwlc68_enable_irq(di);

	ret = stwlc68_write_byte(di, STWLC68_TX_OTP_ADDR, STWLC68_TX_OTP_THRES);
	ret += stwlc68_write_byte(di, STWLC68_TX_OCP_ADDR, di->tx_ocp_val / STWLC68_TX_OCP_STEP);
	ret += stwlc68_write_byte(di, STWLC68_TX_OVP_ADDR, di->tx_ovp_val / STWLC68_TX_OVP_STEP);
	ret += stwlc68_write_word_mask(di, STWLC68_CMD_ADDR, STWLC68_CMD_TX_FOD_EN,
		STWLC68_CMD_TX_FOD_EN_SHIFT, STWLC68_CMD_VAL);
	ret += stwlc68_tx_set_ping_freq(STWLC68_TX_PING_FREQ_INIT, di);
	ret += stwlc68_tx_set_min_fop(di->tx_min_fop, di);
	ret += stwlc68_tx_set_max_fop(di->tx_max_fop, di);
	ret += stwlc68_tx_set_ping_interval(di->tx_ping_interval, di);
	ret += stwlc68_write_byte(di, STWLC68_TX_UVP_ADDR, di->tx_uvp_th / STWLC68_TX_UVP_STEP);
	if (ret) {
		hwlog_err("chip_init: write fail\n");
		return -EIO;
	}

	return 0;
}

static int stwlc68_tx_enable_tx_mode(bool enable, void *dev_data)
{
	int ret;
	struct stwlc68_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	if (enable)
		ret = stwlc68_write_word_mask(di, STWLC68_CMD_ADDR,
			STWLC68_CMD_TX_EN, STWLC68_CMD_TX_EN_SHIFT, STWLC68_CMD_VAL);
	else
		ret = stwlc68_write_word_mask(di, STWLC68_CMD_ADDR,
			STWLC68_CMD_TX_DIS, STWLC68_CMD_TX_DIS_SHIFT, STWLC68_CMD_VAL);

	if (ret) {
		hwlog_err("enable_tx_mode: %s tx mode fail\n", enable ? "enable" : "disable");
		return ret;
	}

	return 0;
}

static void stwlc68_tx_show_ept_type(u16 ept)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(g_stwlc68_tx_ept_name); i++) {
		if (ept & BIT(i))
			hwlog_info("[ept_type] %s\n", g_stwlc68_tx_ept_name[i]);
	}
}

static int stwlc68_tx_get_ept_type(struct stwlc68_dev_info *di, u16 *ept_type)
{
	int ret;
	u16 data = 0;

	if (!ept_type || !di) {
		hwlog_err("get_ept_type: ept_type null\n");
		return -EINVAL;
	}

	ret = stwlc68_read_word(di, STWLC68_TX_EPT_REASON_RCVD_ADDR, &data);
	if (ret) {
		hwlog_err("get_ept_type: read fail\n");
		return ret;
	}
	hwlog_info("[get_ept_type] EPT type = 0x%04x", data);
	stwlc68_tx_show_ept_type(data);
	*ept_type = data;

	ret = stwlc68_write_word(di, STWLC68_TX_EPT_REASON_RCVD_ADDR, 0);
	if (ret) {
		hwlog_err("get_ept_type: write fail\n");
		return ret;
	}

	return 0;
}

static void stwlc68_tx_ept_handler(struct stwlc68_dev_info *di)
{
	int ret;

	ret = stwlc68_tx_get_ept_type(di, &di->ept_type);
	if (ret) {
		hwlog_err("ept_handler: get tx ept type fail\n");
		return;
	}
	switch (di->ept_type) {
	case STWLC68_TX_EPT_SRC_CEP_TIMEOUT:
		di->ept_type &= ~STWLC68_TX_EPT_SRC_CEP_TIMEOUT;
		power_event_bnc_notify(stwlc68_tx_get_bnt_wltx_type(di->ic_type),
			POWER_NE_WLTX_CEP_TIMEOUT, NULL);
		break;
	case STWLC68_TX_EPT_SRC_FOD:
		di->ept_type &= ~STWLC68_TX_EPT_SRC_FOD;
		power_event_bnc_notify(stwlc68_tx_get_bnt_wltx_type(di->ic_type),
			POWER_NE_WLTX_TX_FOD, NULL);
		break;
	case STWLC68_TX_EPT_SRC_RX_EPT:
		if (di->dev_type == WIRELESS_DEVICE_PAD)
			power_event_bnc_notify(stwlc68_tx_get_bnt_wltx_type(di->ic_type),
				POWER_NE_WLTX_CEP_TIMEOUT, NULL);
		break;
	default:
		break;
	}
}

static int stwlc68_tx_clear_interrupt(struct stwlc68_dev_info *di, u16 itr)
{
	return stwlc68_write_word(di, STWLC68_TX_INTR_CLR_ADDR, itr);
}

static void stwlc68_tx_ask_pkt_handler(struct stwlc68_dev_info *di)
{
	if (di->irq_val & STWLC68_TX_SS_PKG_RCVD_LATCH) {
		di->irq_val &= ~STWLC68_TX_SS_PKG_RCVD_LATCH;
		if (di->g_val.qi_hdl && di->g_val.qi_hdl->hdl_qi_ask_pkt)
			di->g_val.qi_hdl->hdl_qi_ask_pkt(di);
	}

	if (di->irq_val & STWLC68_TX_ID_PCKET_RCVD_LATCH) {
		di->irq_val &= ~STWLC68_TX_ID_PCKET_RCVD_LATCH;
		if (di->g_val.qi_hdl && di->g_val.qi_hdl->hdl_qi_ask_pkt)
			di->g_val.qi_hdl->hdl_qi_ask_pkt(di);
	}

	if (di->irq_val & STWLC68_TX_CFG_PKT_RCVD_LATCH) {
		di->irq_val &= ~STWLC68_TX_CFG_PKT_RCVD_LATCH;
		if (di->g_val.qi_hdl && di->g_val.qi_hdl->hdl_qi_ask_pkt)
			di->g_val.qi_hdl->hdl_qi_ask_pkt(di);
		power_event_bnc_notify(stwlc68_tx_get_bnt_wltx_type(di->ic_type),
			POWER_NE_WLTX_GET_CFG, NULL);
	}

	if (di->irq_val & STWLC68_TX_PP_PKT_RCVD_LATCH) {
		di->irq_val &= ~STWLC68_TX_PP_PKT_RCVD_LATCH;
		if (di->g_val.qi_hdl && di->g_val.qi_hdl->hdl_non_qi_ask_pkt)
			di->g_val.qi_hdl->hdl_non_qi_ask_pkt(di);
	}
}

static void stwlc68_tx_show_irq(u32 intr)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(g_stwlc68_tx_irq_name); i++) {
		if (intr & BIT(i))
			hwlog_info("[tx_irq] %s\n", g_stwlc68_tx_irq_name[i]);
	}
}

static int stwlc68_tx_get_interrupt(struct stwlc68_dev_info *di, u16 *intr)
{
	int ret;

	ret = stwlc68_read_word(di, STWLC68_TX_INTR_LATCH_ADDR, intr);
	if (ret)
		return ret;

	hwlog_info("[get_interrupt] interrupt 0x%04x\n", *intr);
	stwlc68_tx_show_irq(*intr);

	return 0;
}

static void stwlc68_tx_mode_irq_recheck(struct stwlc68_dev_info *di)
{
	int i;
	u16 irq_value = 0;

	for (i = 0; (!gpio_get_value(di->gpio_int)) && (i < STWLC68_TX_INTR_CLR_CNT); i++) {
		stwlc68_read_word(di, STWLC68_TX_INTR_LATCH_ADDR, &irq_value);
		hwlog_info("[irq_recheck] gpio_int low, clear irq cnt=%d irq=0x%x\n",
			i, irq_value);
		stwlc68_tx_clear_interrupt(di, STWLC68_ALL_INTR_CLR);
		power_usleep(DT_USLEEP_1MS); /* delay for gpio int pull up */
	}
}

void stwlc68_tx_mode_irq_handler(struct stwlc68_dev_info *di)
{
	int ret;

	ret = stwlc68_tx_get_interrupt(di, &di->irq_val);
	if (ret) {
		hwlog_err("irq_handler: read interrupt fail, clear\n");
		stwlc68_tx_clear_interrupt(di, STWLC68_ALL_INTR_CLR);
		goto rechk_irq;
	}

	if (di->irq_val == STWLC68_ABNORMAL_INTR) {
		hwlog_err("irq_handler: abnormal interrupt\n");
		stwlc68_tx_clear_interrupt(di, STWLC68_ALL_INTR_CLR);
		goto rechk_irq;
	}

	stwlc68_tx_clear_interrupt(di, di->irq_val);

	stwlc68_tx_ask_pkt_handler(di);

	if (di->irq_val & STWLC68_TX_START_PING_LATCH) {
		di->irq_val &= ~STWLC68_TX_START_PING_LATCH;
		power_event_bnc_notify(stwlc68_tx_get_bnt_wltx_type(di->ic_type),
			POWER_NE_WLTX_PING_RX, NULL);
	}

	if (di->irq_val & STWLC68_TX_EPT_PKT_RCVD_LATCH) {
		di->irq_val &= ~STWLC68_TX_EPT_PKT_RCVD_LATCH;
		stwlc68_tx_ept_handler(di);
	}

	if (di->irq_val & STWLC68_TX_OVP_INTR_LATCH) {
		di->irq_val &= ~STWLC68_TX_OVP_INTR_LATCH;
		if (di->dev_type == WIRELESS_DEVICE_PAD)
			power_event_bnc_notify(stwlc68_tx_get_bnt_wltx_type(di->ic_type),
				POWER_NE_WLTX_OVP, NULL);
	}

	if (di->irq_val & STWLC68_TX_EXT_MON_INTR_LATCH) {
		di->irq_val &= ~STWLC68_TX_EXT_MON_INTR_LATCH;
		power_event_bnc_notify(stwlc68_tx_get_bnt_wltx_type(di->ic_type),
			POWER_NE_WLTX_RCV_DPING, NULL);
	}

	if (di->irq_val & STWLC68_TX_RP_DM_TIMEOUT_LATCH) {
		di->irq_val &= ~STWLC68_TX_RP_DM_TIMEOUT_LATCH;
		power_event_bnc_notify(stwlc68_tx_get_bnt_wltx_type(di->ic_type),
			POWER_NE_WLTX_RP_DM_TIMEOUT, NULL);
	}

rechk_irq:
	stwlc68_tx_mode_irq_recheck(di);
}

static struct wltx_ic_ops g_stwlc68_tx_ic_ops = {
	.get_dev_node           = stwlc68_dts_dev_node,
	.fw_update              = stwlc68_tx_fw_update,
	.chip_init              = stwlc68_tx_chip_init,
	.chip_reset             = stwlc68_chip_reset,
	.chip_enable            = stwlc68_chip_enable,
	.mode_enable            = stwlc68_tx_enable_tx_mode,
	.set_open_flag          = stwlc68_tx_set_tx_open_flag,
	.set_stop_cfg           = stwlc68_tx_stop_config,
	.is_rx_discon           = stwlc68_tx_check_rx_disconnect,
	.is_in_tx_mode          = stwlc68_tx_in_tx_mode,
	.is_in_rx_mode          = stwlc68_tx_in_rx_mode,
	.get_vrect              = stwlc68_tx_get_vrect,
	.get_vin                = stwlc68_tx_get_vin,
	.get_iin                = stwlc68_tx_get_iin,
	.get_temp               = stwlc68_tx_get_temp,
	.get_fop                = stwlc68_tx_get_fop,
	.get_cep                = stwlc68_tx_get_cep,
	.get_duty               = stwlc68_tx_get_duty,
	.get_ptx                = stwlc68_tx_get_ptx,
	.get_prx                = stwlc68_tx_get_prx,
	.get_ploss              = stwlc68_tx_get_ploss,
	.get_ploss_id           = stwlc68_tx_get_ploss_id,
	.get_ping_freq          = stwlc68_tx_get_ping_freq,
	.get_ping_interval      = stwlc68_tx_get_ping_interval,
	.get_min_fop            = stwlc68_tx_get_min_fop,
	.get_max_fop            = stwlc68_tx_get_max_fop,
	.set_ping_freq          = stwlc68_tx_set_ping_freq,
	.set_ping_interval      = stwlc68_tx_set_ping_interval,
	.set_min_fop            = stwlc68_tx_set_min_fop,
	.set_max_fop            = stwlc68_tx_set_max_fop,
	.set_ilim               = stwlc68_tx_set_ilimit,
	.set_vset               = stwlc68_tx_set_vset,
	.set_fod_coef           = stwlc68_tx_set_fod_coef,
	.set_rp_dm_to           = stwlc68_tx_set_rp_dm_timeout_val,
};

int stwlc68_tx_ops_register(struct wltrx_ic_ops *ops, struct stwlc68_dev_info *di)
{
	if (!ops || !di)
		return -ENODEV;

	ops->tx_ops = kzalloc(sizeof(*(ops->tx_ops)), GFP_KERNEL);
	if (!ops->tx_ops)
		return -ENODEV;

	memcpy(ops->tx_ops, &g_stwlc68_tx_ic_ops, sizeof(g_stwlc68_tx_ic_ops));
	ops->tx_ops->dev_data = (void *)di;
	return wltx_ic_ops_register(ops->tx_ops, di->ic_type);
}
