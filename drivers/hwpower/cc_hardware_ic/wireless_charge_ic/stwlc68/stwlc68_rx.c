// SPDX-License-Identifier: GPL-2.0
/*
 * stwlc68_rx.c
 *
 * stwlc68 rx driver
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

#define HWLOG_TAG wireless_stwlc68_rx
HWLOG_REGIST();

static const char * const g_stwlc68_rx_irq_name[] = {
	/* [n]: n means bit in registers */
	[0]  = "rx_otp",
	[1]  = "rx_ocp",
	[2]  = "rx_ovp",
	[3]  = "rx_sys_err",
	[5]  = "rx_data_rcvd",
	[6]  = "rx_output_on",
	[7]  = "rx_output_off",
	[8]  = "rx_send_pkt_succ",
	[9]  = "rx_send_pkt_timeout",
	[10] = "rx_power_on",
	[11] = "rx_ready",
};

static int stwlc68_rx_get_temp(int *temp, void *dev_data)
{
	s16 l_temp = 0;

	if (!temp || stwlc68_read_word(dev_data, STWLC68_CHIP_TEMP_ADDR, (u16 *)&l_temp))
		return -EINVAL;

	*temp = l_temp;
	return 0;
}

static int stwlc68_rx_get_fop(int *fop, void *dev_data)
{
	return stwlc68_read_word(dev_data, STWLC68_OP_FREQ_ADDR, (u16 *)fop);
}

static int stwlc68_rx_get_cep(int *cep, void *dev_data)
{
	return 0;
}

static int stwlc68_rx_get_vrect(int *vrect, void *dev_data)
{
	return stwlc68_read_word(dev_data, STWLC68_VRECT_ADDR, (u16 *)vrect);
}

static int stwlc68_rx_get_vout(int *vout, void *dev_data)
{
	return stwlc68_read_word(dev_data, STWLC68_VOUT_ADDR, (u16 *)vout);
}

static int stwlc68_rx_get_iout(int *iout, void *dev_data)
{
	return stwlc68_read_word(dev_data, STWLC68_IOUT_ADDR, (u16 *)iout);
}

static void stwlc68_rx_get_iavg(int *rx_iavg, void *dev_data)
{
	struct stwlc68_dev_info *di = dev_data;

	if (!di)
		return;

	wlic_get_rx_iavg(di->ic_type, rx_iavg);
}

static void stwlc68_rx_get_imax(int *rx_imax, void *dev_data)
{
	struct stwlc68_dev_info *di = dev_data;

	if (!di) {
		if (rx_imax)
			*rx_imax = WLIC_DEFAULT_RX_IMAX;
		return;
	}
	wlic_get_rx_imax(di->ic_type, rx_imax);
}

static int stwlc68_rx_get_vout_reg(int *vreg, void *dev_data)
{
	int ret;

	ret = stwlc68_read_word(dev_data, STWLC68_RX_VOUT_SET_ADDR, (u16 *)vreg);
	if (ret) {
		hwlog_err("get_vout_reg: fail\n");
		return -EIO;
	}

	*vreg *= STWLC68_RX_VOUT_SET_STEP;
	return 0;
}

static int stwlc68_rx_get_vfc_reg(int *vfc_reg, void *dev_data)
{
	return stwlc68_read_word(dev_data, STWLC68_FC_VOLT_ADDR, (u16 *)vfc_reg);
}

static void stwlc68_rx_show_irq(u32 intr)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(g_stwlc68_rx_irq_name); i++) {
		if (intr & BIT(i))
			hwlog_info("[rx_irq] %s\n", g_stwlc68_rx_irq_name[i]);
	}
}

static int stwlc68_rx_get_interrupt(struct stwlc68_dev_info *di, u16 *intr)
{
	int ret;

	ret = stwlc68_read_word(di, STWLC68_INTR_LATCH_ADDR, intr);
	if (ret)
		return ret;

	hwlog_info("[get_interrupt] interrupt 0x%04x\n", *intr);
	stwlc68_rx_show_irq(*intr);

	return 0;
}

static int stwlc68_rx_clear_interrupt(struct stwlc68_dev_info *di, u16 intr)
{
	return stwlc68_write_word(di, STWLC68_INTR_CLR_ADDR, intr);
}

static void stwlc68_sleep_enable(bool enable, void *dev_data)
{
	int gpio_val;
	struct stwlc68_dev_info *di = dev_data;

	if (!di || di->g_val.irq_abnormal_flag || di->gpio_sleep_en_pending)
		return;

	gpio_set_value(di->gpio_sleep_en, enable ? WLTRX_IC_SLEEP_ENABLE : WLTRX_IC_SLEEP_DISABLE);
	gpio_val = gpio_get_value(di->gpio_sleep_en);
	hwlog_info("[sleep_enable] gpio is %s now\n", gpio_val ? "high" : "low");
}

static bool stwlc68_is_sleep_enable(void *dev_data)
{
	int gpio_val;
	struct stwlc68_dev_info *di = dev_data;

	if (!di || di->gpio_sleep_en_pending)
		return false;

	gpio_val = gpio_get_value(di->gpio_sleep_en);
	return gpio_val == WLTRX_IC_SLEEP_ENABLE;
}

static void stwlc68_rx_ext_pwr_prev_ctrl(bool flag, void *dev_data)
{
	int ret;
	u8 wr_buff;
	struct stwlc68_dev_info *di = dev_data;

	if (flag)
		wr_buff = STWLC68_LDO5V_EN;
	else
		wr_buff = STWLC68_LDO5V_DIS;

	hwlog_info("[ext_pwr_prev_ctrl] ldo_5v %s\n", flag ? "on" : "off");
	ret = stwlc68_4addr_write_block(di, STWLC68_LDO5V_EN_ADDR, &wr_buff, STWLC68_FW_ADDR_LEN);
	if (ret)
		hwlog_err("ext_pwr_prev_ctrl: write reg failed\n");
}

static int stwlc68_rx_set_vout(int vol, void *dev_data)
{
	int ret;
	struct stwlc68_dev_info *di = dev_data;

	if ((vol < STWLC68_RX_VOUT_MIN) || (vol > STWLC68_RX_VOUT_MAX)) {
		hwlog_err("set_vout: out of range\n");
		return -EINVAL;
	}

	vol = vol / STWLC68_RX_VOUT_SET_STEP;
	ret = stwlc68_write_word(di, STWLC68_RX_VOUT_SET_ADDR, (u16)vol);
	if (ret) {
		hwlog_err("set_vout: fail\n");
		return -EIO;
	}

	return 0;
}

static bool stwlc68_rx_is_cp_really_open(struct stwlc68_dev_info *di)
{
	int rx_ratio;
	int rx_vset = 0;
	int rx_vout = 0;
	int cp_vout;

	if (!charge_pump_is_cp_open(CP_TYPE_MAIN))
		return false;

	rx_ratio = charge_pump_get_cp_ratio(CP_TYPE_MAIN);
	(void)stwlc68_rx_get_vout_reg(&rx_vset, di);
	(void)stwlc68_rx_get_vout(&rx_vout, di);
	cp_vout = charge_pump_get_cp_vout(CP_TYPE_MAIN);
	cp_vout = (cp_vout > 0) ? cp_vout : wldc_get_ls_vbus();

	hwlog_info("[is_cp_really_open] rx_ratio:%d rx_vset:%d rx_vout:%d cp_vout:%d\n",
		rx_ratio, rx_vset, rx_vout, cp_vout);
	if ((cp_vout * rx_ratio) < (rx_vout - STWLC68_FC_VOUT_ERR_LTH))
		return false;
	if ((cp_vout * rx_ratio) > (rx_vout + STWLC68_FC_VOUT_ERR_UTH))
		return false;

	return true;
}

static int stwlc68_rx_check_cp_mode(struct stwlc68_dev_info *di)
{
	int i;
	int cnt;
	int ret;

	if (!di->support_cp)
		return 0;

	ret = charge_pump_set_cp_mode(CP_TYPE_MAIN);
	if (ret) {
		hwlog_err("check_cp_mode: set cp mode fail\n");
		return ret;
	}
	cnt = STWLC68_BPCP_TIMEOUT / STWLC68_BPCP_SLEEP_TIME;
	for (i = 0; i < cnt; i++) {
		(void)power_msleep(STWLC68_BPCP_SLEEP_TIME, 0, NULL);
		if (stwlc68_rx_is_cp_really_open(di)) {
			hwlog_info("[check_cp_mode] set cp mode succ\n");
			break;
		}
		if (di->g_val.rx_stop_chrg_flag)
			return -EPERM;
	}
	if (i == cnt) {
		hwlog_err("check_cp_mode: set cp mode fail\n");
		return -ENXIO;
	}

	return 0;
}

static int stwlc68_rx_send_fc_cmd(struct stwlc68_dev_info *di, int vset)
{
	int ret;

	ret = stwlc68_write_word(di, STWLC68_FC_VOLT_ADDR, (u16)vset);
	if (ret) {
		hwlog_err("send_fc_cmd: set fc reg fail\n");
		return ret;
	}
	ret = stwlc68_write_word_mask(di, STWLC68_CMD_ADDR, STWLC68_CMD_SEND_FC,
		STWLC68_CMD_SEND_FC_SHIFT, STWLC68_CMD_VAL);
	if (ret) {
		hwlog_err("send_fc_cmd: send fc cmd fail\n");
		return ret;
	}
	ret = stwlc68_rx_set_vout(vset, di);
	if (ret) {
		hwlog_err("send_fc_cmd: set rx vout fail\n");
		return ret;
	}

	return 0;
}

static bool stwlc68_rx_is_fc_succ(struct stwlc68_dev_info *di, int vset)
{
	int i;
	int cnt;
	int vout = 0;

	cnt = STWLC68_FC_VOUT_TIMEOUT / STWLC68_FC_VOUT_SLEEP_TIME;
	for (i = 0; i < cnt; i++) {
		if (di->g_val.rx_stop_chrg_flag && (vset > STWLC68_FC_VOUT_DEFAULT))
			return false;
		(void)power_msleep(STWLC68_FC_VOUT_SLEEP_TIME, 0, NULL);
		(void)stwlc68_rx_get_vout(&vout, di);
		if ((vout >= vset - STWLC68_FC_VOUT_ERR_LTH) &&
			(vout <= vset + STWLC68_FC_VOUT_ERR_UTH)) {
			hwlog_info("[is_fc_succ] succ, cost_time: %dms\n",
				(i + 1) * STWLC68_FC_VOUT_SLEEP_TIME);
			return true;
		}
	}

	return false;
}

static void stwlc68_ask_mode_cfg(struct stwlc68_dev_info *di, u8 mode_cfg)
{
	int ret;
	u8 val = 0;

	ret = stwlc68_write_byte(di, STWLC68_ASK_CFG_ADDR, mode_cfg);
	if (ret)
		hwlog_err("ask_mode_cfg: write fail\n");

	ret = stwlc68_read_byte(di, STWLC68_ASK_CFG_ADDR, &val);
	if (ret) {
		hwlog_err("ask_mode_cfg: read fail\n");
		return;
	}

	hwlog_info("[ask_mode_cfg] val=0x%x\n", val);
}

static void stwlc68_set_mode_cfg(struct stwlc68_dev_info *di, int vset)
{
	if (vset <= RX_HIGH_VOUT) {
		stwlc68_ask_mode_cfg(di, STWLC68_BOTH_CAP_POSITIVE);
	} else {
		if (!power_cmdline_is_factory_mode())
			stwlc68_ask_mode_cfg(di, STWLC68_MOD_C_NEGATIVE);
		else
			stwlc68_ask_mode_cfg(di, STWLC68_BOTH_CAP_POSITIVE);
	}
}

static int stwlc68_rx_set_vfc(int vfc, void *dev_data)
{
	int ret;
	int i;
	struct stwlc68_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	if (vfc >= RX_HIGH_VOUT2) {
		ret = stwlc68_rx_check_cp_mode(di);
		if (ret)
			return ret;
	}
	stwlc68_set_mode_cfg(di, vfc);

	for (i = 0; i < STWLC68_FC_VOUT_RETRY_CNT; i++) {
		if (di->g_val.rx_stop_chrg_flag && (vfc > STWLC68_FC_VOUT_DEFAULT))
			return -EPERM;
		ret = stwlc68_rx_send_fc_cmd(di, vfc);
		if (ret) {
			hwlog_err("set_vfc: send fc_cmd fail\n");
			continue;
		}
		hwlog_info("[set_vfc] send fc cmd, cnt: %d\n", i);
		if (stwlc68_rx_is_fc_succ(di, vfc)) {
			if (vfc < RX_HIGH_VOUT2)
				(void)charge_pump_set_bp_mode(CP_TYPE_MAIN);
			stwlc68_set_mode_cfg(di, vfc);
			hwlog_info("[set_vfc] succ\n");
			return 0;
		}
	}

	return -EIO;
}

static void stwlc68_rx_send_ept(int ept_type, void *dev_data)
{
	int ret;
	u8 rx_ept_type;
	struct stwlc68_dev_info *di = dev_data;

	switch (ept_type) {
	case WIRELESS_EPT_ERR_VRECT:
		rx_ept_type = STWLC68_EPT_ERR_VRECT;
		break;
	case WIRELESS_EPT_ERR_VOUT:
		rx_ept_type = STWLC68_EPT_ERR_VOUT;
		break;
	default:
		return;
	}
	ret = stwlc68_write_byte(di, STWLC68_EPT_MSG_ADDR, rx_ept_type);
	ret += stwlc68_write_word_mask(di, STWLC68_CMD_ADDR, STWLC68_CMD_SEND_EPT,
		STWLC68_CMD_SEND_EPT_SHIFT, STWLC68_CMD_VAL);
	if (ret)
		hwlog_err("send_ept: failed\n");
}

static int stwlc68_rx_fw_update(void *dev_data)
{
	return stwlc68_fw_sram_update(dev_data, WIRELESS_RX);
}

static bool stwlc68_rx_need_check_pu_shell(void *dev_data)
{
	struct stwlc68_dev_info *di = dev_data;

	if (!di)
		return false;

	return di->need_chk_pu_shell;
}

static void stwlc68_rx_set_pu_shell_flag(bool flag, void *dev_data)
{
	struct stwlc68_dev_info *di = dev_data;

	if (!di)
		return;

	di->pu_shell_flag = flag;
	hwlog_info("[set_pu_shell_flag] %s succ\n", flag ? "true" : "false");
}

bool stwlc68_rx_is_tx_exist(void *dev_data)
{
	int ret;
	u8 mode = 0;

	ret = stwlc68_get_mode(dev_data, &mode);
	if (ret) {
		hwlog_err("is_tx_exist: get rx mode fail\n");
		return false;
	}

	return mode == STWLC68_RX_MODE;
}

static int stwlc68_rx_kick_watchdog(void *dev_data)
{
	return stwlc68_write_byte(dev_data, STWLC68_WDT_FEED_ADDR, 0);
}

static int stwlc68_rx_get_fod(char *fod_str, int len, void *dev_data)
{
	int i;
	int ret;
	char tmp[STWLC68_RX_FOD_TMP_STR_LEN] = { 0 };
	u8 fod_arr[STWLC68_RX_FOD_LEN] = { 0 };
	struct stwlc68_dev_info *di = dev_data;

	if (!fod_str || (len < WLRX_IC_FOD_COEF_LEN))
		return -EINVAL;

	ret = stwlc68_read_block(di, STWLC68_RX_FOD_ADDR, fod_arr, STWLC68_RX_FOD_LEN);
	if (ret)
		return ret;

	for (i = 0; i < STWLC68_RX_FOD_LEN; i++) {
		snprintf(tmp, STWLC68_RX_FOD_TMP_STR_LEN, "%x ", fod_arr[i]);
		strncat(fod_str, tmp, strlen(tmp));
	}

	return strlen(fod_str);
}

static int stwlc68_rx_set_fod(const char *fod_str, void *dev_data)
{
	char *cur = (char *)fod_str;
	char *token = NULL;
	int i;
	u8 val = 0;
	const char *sep = " ,";
	u8 fod_arr[STWLC68_RX_FOD_LEN] = { 0 };
	struct stwlc68_dev_info *di = dev_data;

	if (!di || !fod_str) {
		hwlog_err("set_fod: di or fod_str null\n");
		return -EINVAL;
	}

	for (i = 0; i < STWLC68_RX_FOD_LEN; i++) {
		token = strsep(&cur, sep);
		if (!token) {
			hwlog_err("set_fod: input fod_str number err\n");
			return -EINVAL;
		}
		if (kstrtou8(token, POWER_BASE_DEC, &val)) {
			hwlog_err("set_fod: input fod_str type err\n");
			return -EINVAL;
		}
		fod_arr[i] = val;
		hwlog_info("[set_fod] fod[%d] = 0x%x\n", i, fod_arr[i]);
	}

	return stwlc68_write_block(di, STWLC68_RX_FOD_ADDR, fod_arr, STWLC68_RX_FOD_LEN);
}

static int stwlc68_rx_init_fod_coef(struct stwlc68_dev_info *di, int tx_type)
{
	int vfc_reg = 0;
	int ret;
	u8 rx_fod[STWLC68_RX_FOD_LEN] = { 0 };
	u8 rx_offset = 0;

	(void)stwlc68_rx_get_vfc_reg(&vfc_reg, di);
	hwlog_info("[init_fod_coef] vfc_reg: %dmV\n", vfc_reg);

	if (vfc_reg < 9000) { /* (0, 9)V, set 5v fod */
		if (di->pu_shell_flag && di->need_chk_pu_shell)
			memcpy(rx_fod, di->pu_rx_fod_5v, sizeof(rx_fod));
		else
			memcpy(rx_fod, di->rx_fod_5v, sizeof(rx_fod));
	} else if (vfc_reg < 12000) { /* [9, 12)V, set 9V fod */
		if ((tx_type == WLACC_TX_CP60) || (tx_type == WLACC_TX_CP85)) {
			memcpy(rx_fod, di->rx_fod_9v_cp60, sizeof(rx_fod));
		} else if (tx_type == WLACC_TX_CP39S) {
			memcpy(rx_fod, di->rx_fod_9v_cp39s, sizeof(rx_fod));
		} else {
			rx_offset = di->rx_offset_9v;
			memcpy(rx_fod, di->rx_fod_9v, sizeof(rx_fod));
		}
	} else if (vfc_reg < 15000) { /* [12, 15)V, set 12V fod */
		memcpy(rx_fod, di->rx_fod_12v, sizeof(rx_fod));
	} else if (vfc_reg < 18000) { /* [15, 18)V, set 15V fod */
		if (tx_type == WLACC_TX_CP39S) {
			memcpy(rx_fod, di->rx_fod_15v_cp39s, sizeof(rx_fod));
		} else {
			rx_offset = di->rx_offset_15v;
			memcpy(rx_fod, di->rx_fod_15v, sizeof(rx_fod));
		}
	}

	ret = stwlc68_write_block(di, STWLC68_RX_FOD_ADDR, rx_fod, STWLC68_RX_FOD_LEN);
	if (ret) {
		hwlog_err("init_fod_coef: write fod fail\n");
		return ret;
	}

	ret = stwlc68_write_byte(di, STWLC68_RX_OFFSET_ADDR, rx_offset);
	if (ret)
		hwlog_err("init_fod_coef: write offset fail\n");

	return ret;
}

int stwlc68_rx_chip_init(unsigned int init_type, unsigned int tx_type, void *dev_data)
{
	int ret = 0;
	struct stwlc68_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	switch (init_type) {
	case WIRELESS_CHIP_INIT:
		hwlog_info("[chip_init] DEFAULT_CHIP_INIT\n");
		di->g_val.rx_stop_chrg_flag = false;
		ret += stwlc68_write_byte(di, STWLC68_FC_VRECT_DELTA_ADDR,
			STWLC68_FC_VRECT_DELTA / STWLC68_FC_VRECT_DELTA_STEP);
		/* fall through */
	case ADAPTER_5V * POWER_MV_PER_V:
		hwlog_info("[chip_init] 5V_CHIP_INIT\n");
		ret += stwlc68_write_block(di, STWLC68_LDO_CFG_ADDR,
			di->rx_ldo_cfg_5v, STWLC68_LDO_CFG_LEN);
		ret += stwlc68_rx_init_fod_coef(di, tx_type);
		stwlc68_set_mode_cfg(di, init_type);
		break;
	case ADAPTER_9V * POWER_MV_PER_V:
		hwlog_info("[chip_init] 9V_CHIP_INIT\n");
		ret += stwlc68_write_block(di, STWLC68_LDO_CFG_ADDR,
			di->rx_ldo_cfg_9v, STWLC68_LDO_CFG_LEN);
		ret += stwlc68_rx_init_fod_coef(di, tx_type);
		break;
	case ADAPTER_12V * POWER_MV_PER_V:
		hwlog_info("[chip_init] 12V_CHIP_INIT\n");
		ret += stwlc68_write_block(di, STWLC68_LDO_CFG_ADDR,
			di->rx_ldo_cfg_12v, STWLC68_LDO_CFG_LEN);
		ret += stwlc68_rx_init_fod_coef(di, tx_type);
		break;
	case WILREESS_SC_CHIP_INIT:
		hwlog_info("[chip_init] SC_CHIP_INIT\n");
		ret += stwlc68_write_block(di, STWLC68_LDO_CFG_ADDR,
			di->rx_ldo_cfg_sc, STWLC68_LDO_CFG_LEN);
		ret += stwlc68_rx_init_fod_coef(di, tx_type);
		break;
	default:
		hwlog_info("chip_init: input para is invalid\n");
		break;
	}

	return ret;
}

static void stwlc68_rx_stop_charging(void *dev_data)
{
	struct stwlc68_dev_info *di = dev_data;

	if (!di)
		return;

	di->g_val.rx_stop_chrg_flag = true;
	wlic_iout_stop_sample(di->ic_type);
	if (!di->g_val.irq_abnormal_flag)
		return;

	if (wlrx_get_wired_channel_state() != WIRED_CHANNEL_ON) {
		hwlog_info("[stop_charging] irq_abnormal,keep rx_sw on\n");
		di->g_val.irq_abnormal_flag = true;
		wlps_control(di->ic_type, WLPS_RX_SW, true);
	} else {
		di->irq_cnt = 0;
		di->g_val.irq_abnormal_flag = false;
		stwlc68_enable_irq(di);
		hwlog_info("[stop_charging] wired channel on, enable irq_int\n");
	}
}

static int stwlc68_rx_data_received_handler(struct stwlc68_dev_info *di)
{
	int ret;
	int i;
	u8 cmd;
	u8 buff[HWQI_PKT_LEN] = { 0 };

	ret = stwlc68_read_block(di, STWLC68_RCVD_MSG_HEADER_ADDR, buff, HWQI_PKT_LEN);
	if (ret) {
		hwlog_err("data_received_handler: read data received from TX fail\n");
		return -EIO;
	}

	cmd = buff[HWQI_PKT_CMD];
	hwlog_info("[data_received_handler] cmd: 0x%x\n", cmd);
	for (i = HWQI_PKT_DATA; i < HWQI_PKT_LEN; i++)
		hwlog_info("[data_received_handler] data: 0x%x\n", buff[i]);

	switch (cmd) {
	case HWQI_CMD_TX_ALARM:
	case HWQI_CMD_ACK_BST_ERR:
		di->irq_val &= ~STWLC68_RX_RCVD_MSG_INTR_LATCH;
		if (di->g_val.qi_hdl && di->g_val.qi_hdl->hdl_non_qi_fsk_pkt)
			di->g_val.qi_hdl->hdl_non_qi_fsk_pkt(buff, HWQI_PKT_LEN, di);
		break;
	default:
		break;
	}
	return 0;
}

static void stwlc68_rx_ready_ps_control(struct stwlc68_dev_info *di, bool flag)
{
	if (di->rx_ready_ext_pwr_en)
		wlps_control(di->ic_type, WLPS_RX_EXT_PWR, flag);
}

static void stwlc68_rx_ready_handler(struct stwlc68_dev_info *di)
{
	if (wlrx_get_wired_channel_state() == WIRED_CHANNEL_ON) {
		hwlog_err("ready_handler: wired channel on, ignore\n");
		return;
	}

	hwlog_info("[ready_handler] rx ready, goto wireless charging\n");
	di->g_val.rx_stop_chrg_flag = false;
	di->irq_cnt = 0;
	wired_chsw_set_wired_channel(WIRED_CHANNEL_ALL, WIRED_CHANNEL_CUTOFF);
	stwlc68_rx_ready_ps_control(di, true);
	(void)power_msleep(CHANNEL_SW_TIME, 0, NULL);
	stwlc68_fw_sram_update(di, WIRELESS_RX);
	if (!di->gpio_sleep_en_pending)
		gpio_set_value(di->gpio_sleep_en, RX_SLEEP_EN_DISABLE);
	stwlc68_rx_ready_ps_control(di, false);
	wlic_iout_start_sample(di->ic_type);
	power_event_bnc_notify(POWER_BNT_WLRX, POWER_NE_WLRX_READY, NULL);
}

void stwlc68_rx_handle_abnormal_irq(struct stwlc68_dev_info *di)
{
	static struct timespec64 ts64_timeout;
	struct timespec64 ts64_interval;
	struct timespec64 ts64_now;

	ts64_now = power_get_current_kernel_time64();
	ts64_interval.tv_sec = 0;
	ts64_interval.tv_nsec = WIRELESS_INT_TIMEOUT_TH * NSEC_PER_MSEC;

	hwlog_info("[handle_abnormal_irq] irq_cnt = %d\n", ++di->irq_cnt);
	/* power on interrupt happen first time, so start monitor it */
	if (di->irq_cnt == 1) {
		ts64_timeout = timespec64_add_safe(ts64_now, ts64_interval);
		if (ts64_timeout.tv_sec == TIME_T_MAX) {
			di->irq_cnt = 0;
			hwlog_err("handle_abnormal_irq: time overflow happened\n");
			return;
		}
	}

	if (timespec64_compare(&ts64_now, &ts64_timeout) >= 0) {
		if (di->irq_cnt >= WIRELESS_INT_CNT_TH) {
			di->g_val.irq_abnormal_flag = true;
			wlps_control(di->ic_type, WLPS_RX_SW, true);
			stwlc68_disable_irq_nosync(di);
			if (!di->gpio_sleep_en_pending)
				gpio_set_value(di->gpio_sleep_en, RX_SLEEP_EN_DISABLE);
			hwlog_err("handle_abnormal_irq: more than %d irq in %ds, disable irq\n",
			    WIRELESS_INT_CNT_TH, WIRELESS_INT_TIMEOUT_TH / POWER_MS_PER_S);
		} else {
			di->irq_cnt = 0;
			hwlog_info("handle_abnormal_irq: less than %d irq in %ds, clr irq cnt\n",
			    WIRELESS_INT_CNT_TH, WIRELESS_INT_TIMEOUT_TH / POWER_MS_PER_S);
		}
	}
}

static void stwlc68_rx_power_on_handler(struct stwlc68_dev_info *di)
{
	u8 rx_ss = 0; /* ss: Signal Strength */
	int pwr_flag = RX_PWR_ON_NOT_GOOD;

	if (wlrx_get_wired_channel_state() == WIRED_CHANNEL_ON) {
		hwlog_err("power_on_handler: wired channel on, ignore\n");
		return;
	}

	stwlc68_rx_handle_abnormal_irq(di);
	(void)stwlc68_read_byte(di, STWLC68_RX_SS_ADDR, &rx_ss);
	hwlog_info("[power_on_handler] SS = %u\n", rx_ss);
	if ((rx_ss > di->rx_ss_good_lth) && (rx_ss <= STWLC68_RX_SS_MAX))
		pwr_flag = RX_PWR_ON_GOOD;
	power_event_bnc_notify(POWER_BNT_WLRX, POWER_NE_WLRX_PWR_ON, &pwr_flag);
}

static void stwlc68_rx_mode_irq_recheck(struct stwlc68_dev_info *di)
{
	int i;
	u16 irq_value = 0;

	for (i = 0; (!gpio_get_value(di->gpio_int)) && (i < STWLC68_INTR_CLR_CNT); i++) {
		stwlc68_read_word(di, STWLC68_INTR_LATCH_ADDR, &irq_value);
		if (irq_value & STWLC68_RX_STATUS_READY)
			stwlc68_rx_ready_handler(di);
		hwlog_info("[irq_recheck] gpio_int low, clear irq cnt=%d irq=0x%x\n",
			i, irq_value);
		stwlc68_rx_clear_interrupt(di, STWLC68_ALL_INTR_CLR);
		power_usleep(DT_USLEEP_1MS); /* delay for gpio int pull up */
	}
}

static void stwlc68_rx_fault_irq_handler(struct stwlc68_dev_info *di)
{
	if (di->irq_val & STWLC68_OCP_INTR_LATCH) {
		di->irq_val &= ~STWLC68_OCP_INTR_LATCH;
		power_event_bnc_notify(POWER_BNT_WLRX, POWER_NE_WLRX_OCP, NULL);
	}
	if (di->irq_val & STWLC68_OVP_INTR_LATCH) {
		di->irq_val &= ~STWLC68_OVP_INTR_LATCH;
		power_event_bnc_notify(POWER_BNT_WLRX, POWER_NE_WLRX_OVP, NULL);
	}
	if (di->irq_val & STWLC68_OVTP_INTR_LATCH) {
		di->irq_val &= ~STWLC68_OVTP_INTR_LATCH;
		power_event_bnc_notify(POWER_BNT_WLRX, POWER_NE_WLRX_OTP, NULL);
	}
}

void stwlc68_rx_mode_irq_handler(struct stwlc68_dev_info *di)
{
	int ret;

	ret = stwlc68_rx_get_interrupt(di, &di->irq_val);
	if (ret) {
		hwlog_err("irq_handler: read interrupt fail, clear\n");
		stwlc68_rx_clear_interrupt(di, STWLC68_ALL_INTR_CLR);
		stwlc68_rx_handle_abnormal_irq(di);
		goto rechk_irq;
	}

	if (di->irq_val == STWLC68_ABNORMAL_INTR) {
		hwlog_err("irq_handler: abnormal interrupt\n");
		stwlc68_rx_clear_interrupt(di, STWLC68_ALL_INTR_CLR);
		stwlc68_rx_handle_abnormal_irq(di);
		goto rechk_irq;
	}

	stwlc68_rx_clear_interrupt(di, di->irq_val);

	if (di->irq_val & STWLC68_RX_STATUS_POWER_ON) {
		di->irq_val &= ~STWLC68_RX_STATUS_POWER_ON;
		stwlc68_rx_power_on_handler(di);
	}
	if (di->irq_val & STWLC68_RX_STATUS_READY) {
		di->irq_val &= ~STWLC68_RX_STATUS_READY;
		stwlc68_rx_ready_handler(di);
	}
	if (di->irq_val & STWLC68_RX_RCVD_MSG_INTR_LATCH)
		stwlc68_rx_data_received_handler(di);

	stwlc68_rx_fault_irq_handler(di);

rechk_irq:
	stwlc68_rx_mode_irq_recheck(di);
}

static void stwlc68_rx_pmic_vbus_handler(bool vbus_state, void *dev_data)
{
	int ret;
	int wired_ch_state;
	u16 irq_val = 0;
	struct stwlc68_dev_info *di = dev_data;

	if (!di)
		return;

	wired_ch_state = wlrx_get_wired_channel_state();
	if (vbus_state && di->g_val.irq_abnormal_flag &&
		(wired_ch_state != WIRED_CHANNEL_ON) && stwlc68_rx_is_tx_exist(di)) {
		ret = stwlc68_rx_get_interrupt(di, &irq_val);
		if (ret) {
			hwlog_err("pmic_vbus_handler: read interrupt fail, clear\n");
			return;
		}
		hwlog_info("[pmic_vbus_handler] irq_val = 0x%x\n", irq_val);
		if (irq_val & STWLC68_RX_STATUS_READY) {
			stwlc68_rx_clear_interrupt(di, POWER_MASK_WORD);
			stwlc68_rx_ready_handler(di);
			di->irq_cnt = 0;
			di->g_val.irq_abnormal_flag = false;
			stwlc68_enable_irq(di);
		}
	}
}

void stwlc68_rx_probe_check_tx_exist(struct stwlc68_dev_info *di)
{
	if (!di)
		return;

	if (stwlc68_rx_is_tx_exist(di)) {
		stwlc68_rx_clear_interrupt(di, STWLC68_ALL_INTR_CLR);
		hwlog_info("[probe_check_tx_exist] rx exsit\n");
		stwlc68_rx_ready_handler(di);
	} else {
		stwlc68_sleep_enable(true, di);
	}
}

void stwlc68_rx_shutdown_handler(struct stwlc68_dev_info *di)
{
	wlps_control(di->ic_type, WLPS_RX_EXT_PWR, false);
	(void)power_msleep(DT_MSLEEP_50MS, 0, NULL); /* delay 50ms for power off */
	(void)stwlc68_rx_set_vfc(ADAPTER_5V * POWER_MV_PER_V, di);
	(void)stwlc68_rx_set_vout(ADAPTER_5V * POWER_MV_PER_V, di);
	stwlc68_chip_enable(false, di);
	(void)power_msleep(STWLC68_SHUTDOWN_SLEEP_TIME, 0, NULL);
	stwlc68_chip_enable(true, di);
}

static struct wlrx_ic_ops g_stwlc68_rx_ic_ops = {
	.get_dev_node           = stwlc68_dts_dev_node,
	.fw_update              = stwlc68_rx_fw_update,
	.chip_init              = stwlc68_rx_chip_init,
	.chip_reset             = stwlc68_chip_reset,
	.chip_enable            = stwlc68_chip_enable,
	.is_chip_enable         = stwlc68_is_chip_enable,
	.sleep_enable           = stwlc68_sleep_enable,
	.is_sleep_enable        = stwlc68_is_sleep_enable,
	.ext_pwr_prev_ctrl      = stwlc68_rx_ext_pwr_prev_ctrl,
	.get_vrect              = stwlc68_rx_get_vrect,
	.get_vout               = stwlc68_rx_get_vout,
	.get_iout               = stwlc68_rx_get_iout,
	.get_iavg               = stwlc68_rx_get_iavg,
	.get_imax               = stwlc68_rx_get_imax,
	.get_vout_reg           = stwlc68_rx_get_vout_reg,
	.get_vfc_reg            = stwlc68_rx_get_vfc_reg,
	.set_vfc                = stwlc68_rx_set_vfc,
	.set_vout               = stwlc68_rx_set_vout,
	.get_fop                = stwlc68_rx_get_fop,
	.get_cep                = stwlc68_rx_get_cep,
	.get_temp               = stwlc68_rx_get_temp,
	.get_chip_info          = stwlc68_get_chip_info_str,
	.get_fod_coef           = stwlc68_rx_get_fod,
	.set_fod_coef           = stwlc68_rx_set_fod,
	.is_tx_exist            = stwlc68_rx_is_tx_exist,
	.kick_watchdog          = stwlc68_rx_kick_watchdog,
	.send_ept               = stwlc68_rx_send_ept,
	.stop_charging          = stwlc68_rx_stop_charging,
	.pmic_vbus_handler      = stwlc68_rx_pmic_vbus_handler,
	.need_chk_pu_shell      = stwlc68_rx_need_check_pu_shell,
	.set_pu_shell_flag      = stwlc68_rx_set_pu_shell_flag,
};

int stwlc68_rx_ops_register(struct wltrx_ic_ops *ops, struct stwlc68_dev_info *di)
{
	if (!ops || !di)
		return -ENODEV;

	ops->rx_ops = kzalloc(sizeof(*(ops->rx_ops)), GFP_KERNEL);
	if (!ops->rx_ops)
		return -ENODEV;

	memcpy(ops->rx_ops, &g_stwlc68_rx_ic_ops, sizeof(g_stwlc68_rx_ic_ops));
	ops->rx_ops->dev_data = (void *)di;
	return wlrx_ic_ops_register(ops->rx_ops, di->ic_type);
}
