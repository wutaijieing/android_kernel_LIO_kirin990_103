// SPDX-License-Identifier: GPL-2.0
/*
 * wireless_protocol_qi.c
 *
 * qi protocol driver
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/delay.h>
#include <chipset_common/hwpower/protocol/wireless_protocol.h>
#include <chipset_common/hwpower/protocol/wireless_protocol_qi.h>
#include <chipset_common/hwpower/common_module/power_cmdline.h>
#include <chipset_common/hwpower/common_module/power_event_ne.h>
#include <chipset_common/hwpower/wireless_charge/wireless_acc_types.h>
#include <chipset_common/hwpower/wireless_charge/wireless_trx_ic_intf.h>
#include <chipset_common/hwpower/wireless_charge/wireless_trx_intf.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_acc.h>
#include <chipset_common/hwpower/wireless_charge/wireless_tx_auth.h>
#include <huawei_platform/power/wireless/wireless_transmitter.h>
#include <chipset_common/hwpower/common_module/power_common_macro.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/wireless_charge/wireless_tx_acc.h>

#define HWLOG_TAG wireless_protocol_qi
HWLOG_REGIST();

static struct hwqi_dev *g_hwqi_dev[WLTRX_IC_MAX];
static struct hwqi_handle g_hwqi_handle;

static const u8 g_hwqi_ask_hdr[] = {
	0, 0x18, 0x28, 0x38, 0x48, 0x58
};
static const u8 g_hwqi_fsk_hdr[] = {
	0, 0x1f, 0x2f, 0x3f, 0x4f, 0x5f
};

static const struct wireless_protocol_device_data g_hwqi_dev_data[] = {
	{ WIRELESS_DEVICE_ID_IDTP9221, "idtp9221" },
	{ WIRELESS_DEVICE_ID_STWLC68, "stwlc68" },
	{ WIRELESS_DEVICE_ID_IDTP9415, "idtp9415" },
	{ WIRELESS_DEVICE_ID_NU1619, "nu1619" },
	{ WIRELESS_DEVICE_ID_STWLC88, "stwlc88" },
	{ WIRELESS_DEVICE_ID_CPS7181, "cps7181" },
	{ WIRELESS_DEVICE_ID_CPS4057, "cps4057" },
	{ WIRELESS_DEVICE_ID_MT5735, "mt5735" },
	{ WIRELESS_DEVICE_ID_CPS4029, "cps4029" },
	{ WIRELESS_DEVICE_ID_MT5727, "mt5727" },
	{ WIRELESS_DEVICE_ID_STWLC33, "stwlc33" },
	{ WIRELESS_DEVICE_ID_CPS4067, "cps4067" },
	{ WIRELESS_DEVICE_ID_MT5785, "mt5785" },
};

static int hwqi_get_device_id(const char *str)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(g_hwqi_dev_data); i++) {
		if (!strncmp(str, g_hwqi_dev_data[i].name, strlen(str)))
			return g_hwqi_dev_data[i].id;
	}

	return -EINVAL;
}

static bool hwqi_check_ic_type(unsigned int ic_type)
{
	if (wltrx_ic_is_type_valid(ic_type))
		return true;

	hwlog_err("invalid ic_type=%u\n", ic_type);
	return false;
}

static int hwqi_get_ic_type(void *dev_data)
{
	int i;

	for (i = 0; i < WLTRX_IC_MAX; i++) {
		if (!g_hwqi_dev[i])
			continue;

		if (!g_hwqi_dev[i]->p_ops)
			continue;

		if (g_hwqi_dev[i]->p_ops->dev_data != dev_data)
			continue;

		return i;
	}

	hwlog_err("ic_type not find\n");
	return WLTRX_IC_MIN;
}

static int hwqi_get_bnt_wltx_type(unsigned int ic_type)
{
	return ic_type == WLTRX_IC_AUX ? POWER_BNT_WLTX_AUX : POWER_BNT_WLTX;
}

static struct hwqi_dev *hwqi_get_dev(unsigned int ic_type)
{
	if (!hwqi_check_ic_type(ic_type))
		return NULL;

	if (!g_hwqi_dev[ic_type]) {
		hwlog_err("g_hwqi_dev is null\n");
		return NULL;
	}

	return g_hwqi_dev[ic_type];
}

static struct hwqi_ops *hwqi_get_ops(unsigned int ic_type)
{
	if (!hwqi_check_ic_type(ic_type))
		return NULL;

	if (!g_hwqi_dev[ic_type] || !g_hwqi_dev[ic_type]->p_ops) {
		hwlog_err("g_hwqi_dev or p_ops is null\n");
		return NULL;
	}

	return g_hwqi_dev[ic_type]->p_ops;
}

int hwqi_ops_register(struct hwqi_ops *ops, unsigned int ic_type)
{
	int dev_id;

	if (!hwqi_check_ic_type(ic_type))
		return -EINVAL;

	if (!g_hwqi_dev[ic_type] || !ops || !ops->chip_name) {
		hwlog_err("g_hwqi_dev or ops or chip_name is null\n");
		return -EINVAL;
	}

	dev_id = hwqi_get_device_id(ops->chip_name);
	if (dev_id < 0) {
		hwlog_err("%s ops register fail\n", ops->chip_name);
		return -EINVAL;
	}

	g_hwqi_dev[ic_type]->p_ops = ops;
	g_hwqi_dev[ic_type]->dev_id = dev_id;

	hwlog_info("%d:%s,%u, ops register ok\n", dev_id, ops->chip_name, ic_type);
	return 0;
}

static int hwqi_get_acc_dev_no(unsigned int ic_type)
{
	switch (ic_type) {
	case WLTRX_IC_MAIN:
		return ACC_DEV_NO_KB;
	case WLTRX_IC_AUX:
		return ACC_DEV_NO_PEN;
	default:
		return -EINVAL;
	}
}

struct hwqi_handle *hwqi_get_handle(void)
{
	return &g_hwqi_handle;
}

static int hwqi_send_msg(unsigned int ic_type, u8 cmd, u8 *para, int para_len, u8 *data,
	int data_len, int retrys)
{
	int i;
	int ret;
	struct hwqi_ops *l_ops = hwqi_get_ops(ic_type);

	if (!l_ops)
		return -EINVAL;

	if (!l_ops->send_msg || !l_ops->receive_msg) {
		hwlog_err("send_msg or receive_msg is null\n");
		return -EINVAL;
	}

	for (i = 0; i < retrys; i++) {
		ret = l_ops->send_msg(cmd, para, para_len, l_ops->dev_data);
		if (ret) {
			hwlog_err("0x%x msg send fail, retry %d\n", cmd, i);
			continue;
		}

		ret = l_ops->receive_msg(data, data_len, l_ops->dev_data);
		if (ret) {
			hwlog_err("0x%x msg receive fail, retry %d\n", cmd, i);
			continue;
		}

		break;
	}

	if (i >= retrys)
		return -EINVAL;

	/* protocol define: the first byte of the response must be a command */
	if (data[HWQI_ACK_CMD_OFFSET] != cmd) {
		hwlog_err("data[%d] 0x%x not equal cmd 0x%x\n",
			HWQI_ACK_CMD_OFFSET, data[HWQI_ACK_CMD_OFFSET], cmd);
		return -EINVAL;
	}

	return 0;
}

static int hwqi_send_msg_ack(unsigned int ic_type, u8 cmd, u8 *para, int para_len,
	int retrys)
{
	int i;
	int ret;
	struct hwqi_ops *l_ops = hwqi_get_ops(ic_type);

	if (!l_ops)
		return -EINVAL;

	if (!l_ops->send_msg_with_ack) {
		hwlog_err("send_msg_with_ack is null\n");
		return -EINVAL;
	}

	for (i = 0; i < retrys; i++) {
		ret = l_ops->send_msg_with_ack(cmd, para, para_len, l_ops->dev_data);
		if (ret) {
			hwlog_err("0x%x msg_ack send fail, retry %d\n", cmd, i);
			continue;
		}

		break;
	}

	if (i >= retrys)
		return -EINVAL;

	return 0;
}

static int hwqi_send_msg_only(unsigned int ic_type, u8 cmd, u8 *para, int para_len,
	int retrys)
{
	int i;
	int ret;
	struct hwqi_ops *l_ops = hwqi_get_ops(ic_type);

	if (!l_ops)
		return -EINVAL;

	if (!l_ops->send_msg) {
		hwlog_err("send_msg is null\n");
		return -EINVAL;
	}

	for (i = 0; i < retrys; i++) {
		ret = l_ops->send_msg(cmd, para, para_len, l_ops->dev_data);
		if (ret) {
			hwlog_err("0x%x msg send fail, retry %d\n", cmd, i);
			continue;
		}

		break;
	}

	if (i >= retrys)
		return -EINVAL;

	return 0;
}

static int hwqi_send_fsk_msg(unsigned int ic_type, u8 cmd, u8 *para, int para_len,
	int retrys)
{
	int i;
	int ret;
	struct hwqi_ops *l_ops = hwqi_get_ops(ic_type);

	if (!l_ops)
		return -EINVAL;

	if (!l_ops->send_fsk_msg) {
		hwlog_err("send_fsk_msg is null\n");
		return -EINVAL;
	}

	for (i = 0; i < retrys; i++) {
		ret = l_ops->send_fsk_msg(cmd, para, para_len, l_ops->dev_data);
		if (ret) {
			hwlog_err("0x%x fsk_msg send fail, retry %d\n", cmd, i);
			continue;
		}

		break;
	}

	if (i >= retrys)
		return -EINVAL;

	return 0;
}

static int hwqi_auto_send_fsk_msg(unsigned int ic_type, u8 cmd, u8 *para, int para_len,
	int retrys)
{
	int i;
	int ret;
	struct hwqi_ops *l_ops = hwqi_get_ops(ic_type);

	if (!l_ops)
		return -EINVAL;

	if (!l_ops->auto_send_fsk_with_ack) {
		hwlog_err("auto_send_fsk_msg is null\n");
		return -EINVAL;
	}

	for (i = 0; i < retrys; i++) {
		ret = l_ops->auto_send_fsk_with_ack(cmd, para, para_len,
			l_ops->dev_data);
		if (ret) {
			hwlog_err("0x%x fsk_msg auto send fail, retry %d\n", cmd, i);
			continue;
		}

		break;
	}

	if (i >= retrys)
		return -EINVAL;

	return 0;
}

static int hwqi_send_fsk_ack_msg(unsigned int ic_type)
{
	return hwqi_send_fsk_msg(ic_type, HWQI_CMD_ACK, NULL, 0,
		WIRELESS_RETRY_ONE);
}

static int hwqi_get_ask_packet(unsigned int ic_type, u8 *data, int data_len, int retrys)
{
	int i;
	int ret;
	struct hwqi_ops *l_ops = hwqi_get_ops(ic_type);

	if (!l_ops)
		return -EINVAL;

	if (!l_ops->get_ask_packet) {
		hwlog_err("get_ask_packet is null\n");
		return -EINVAL;
	}

	for (i = 0; i < retrys; i++) {
		ret = l_ops->get_ask_packet(data, data_len, l_ops->dev_data);
		if (ret) {
			hwlog_err("ask_packet receive fail, retry %d\n", i);
			continue;
		}

		break;
	}

	if (i >= retrys)
		return -EINVAL;

	return 0;
}

static int hwqi_get_chip_fw_version(unsigned int ic_type, u8 *data, int data_len,
	int retrys)
{
	int i;
	int ret;
	struct hwqi_ops *l_ops = hwqi_get_ops(ic_type);

	if (!l_ops)
		return -EINVAL;

	if (!l_ops->get_chip_fw_version) {
		hwlog_err("get_chip_fw_version is null\n");
		return -EINVAL;
	}

	for (i = 0; i < retrys; i++) {
		ret = l_ops->get_chip_fw_version(data, data_len, l_ops->dev_data);
		if (ret) {
			hwlog_err("chip_fw_version get fail, retry %d\n", i);
			continue;
		}

		break;
	}

	if (i >= retrys)
		return -EINVAL;

	return 0;
}

static int hwqi_send_rx_event(unsigned int ic_type, u8 rx_evt)
{
	int retry = WIRELESS_RETRY_ONE;

	if (hwqi_send_msg_ack(ic_type, HWQI_CMD_SEND_RX_EVT,
		&rx_evt, HWQI_PARA_RX_EVT_LEN, retry))
		return -EINVAL;

	hwlog_info("send_rx_event succ\n");
	return 0;
}

static int hwqi_send_rx_vout(unsigned int ic_type, int rx_vout)
{
	int retry = WIRELESS_RETRY_ONE;

	if (hwqi_send_msg_ack(ic_type, HWQI_CMD_SEND_RX_VOUT,
		(u8 *)&rx_vout, HWQI_PARA_RX_VOUT_LEN, retry))
		return -EINVAL;

	hwlog_info("send_rx_vout succ\n");
	return 0;
}

static int hwqi_send_rx_iout(unsigned int ic_type, int rx_iout)
{
	int retry = WIRELESS_RETRY_ONE;

	if (hwqi_send_msg_ack(ic_type, HWQI_CMD_SEND_RX_IOUT,
		(u8 *)&rx_iout, HWQI_PARA_RX_IOUT_LEN, retry))
		return -EINVAL;

	hwlog_info("send_rx_iout succ\n");
	return 0;
}

static int hwqi_send_rx_boost_succ(unsigned int ic_type)
{
	int retry = WIRELESS_RETRY_ONE;

	if (hwqi_send_msg_ack(ic_type, HWQI_CMD_RX_BOOST_SUCC, NULL, 0, retry))
		return -EINVAL;

	hwlog_info("send_rx_boost_succ succ\n");
	return 0;
}

static int hwqi_send_rx_ready(unsigned int ic_type)
{
	int retry = WIRELESS_RETRY_ONE;

	if (hwqi_send_msg_ack(ic_type, HWQI_CMD_SEND_RX_READY, NULL, 0, retry))
		return -EINVAL;

	hwlog_info("send_rx_ready succ\n");
	return 0;
}

static int hwqi_send_tx_capability(unsigned int ic_type, u8 *cap, int len)
{
	int retry = WIRELESS_RETRY_ONE;

	if (!cap)
		return -EINVAL;

	if (len != (HWQI_PARA_TX_CAP_LEN - 1)) {
		hwlog_err("para error %d!=%d\n", len, HWQI_PARA_TX_CAP_LEN - 1);
		return -EINVAL;
	}

	if (hwqi_send_fsk_msg(ic_type, HWQI_CMD_GET_TX_CAP, cap, len, retry))
		return -EINVAL;

	hwlog_info("send_tx_capability succ\n");
	return 0;
}

static int hwqi_send_tx_alarm(unsigned int ic_type, u8 *alarm, int len)
{
	int retry = WIRELESS_RETRY_ONE;

	if (!alarm)
		return -EINVAL;

	if (len != HWQI_TX_ALARM_LEN) {
		hwlog_err("para error %d!=%d\n", len, HWQI_TX_ALARM_LEN);
		return -EINVAL;
	}

	if (hwqi_auto_send_fsk_msg(ic_type, HWQI_CMD_TX_ALARM, alarm, len, retry))
		return -EINVAL;

	hwlog_info("send_tx_alarm succ\n");
	return 0;
}

static int hwqi_send_tx_fw_version(unsigned int ic_type, u8 *fw, int len)
{
	int retry = WIRELESS_RETRY_ONE;

	if (!fw)
		return -EINVAL;

	if (len != (HWQI_ACK_TX_FWVER_LEN - 1)) {
		hwlog_err("para error %d!=%d\n", len, HWQI_ACK_TX_FWVER_LEN - 1);
		return -EINVAL;
	}

	if (hwqi_send_fsk_msg(ic_type, HWQI_CMD_GET_TX_VERSION, fw, len, retry))
		return -EINVAL;

	hwlog_info("send_tx_fw_version succ\n");
	return 0;
}

static int hwqi_send_tx_id(unsigned int ic_type, u8 *id, int len)
{
	int retry = WIRELESS_RETRY_ONE;

	if (!id)
		return -EINVAL;

	if (len != HWQI_PARA_TX_ID_LEN) {
		hwlog_err("para error %d!=%d\n", len, HWQI_PARA_TX_ID_LEN);
		return -EINVAL;
	}

	if (hwqi_send_fsk_msg(ic_type, HWQI_CMD_GET_TX_ID, id, len, retry))
		return -EINVAL;

	hwlog_info("send_tx_id succ\n");
	return 0;
}

static int hwqi_send_lightstrap_control_msg(unsigned int ic_type, u8 *msg, int len)
{
	int retry = WIRELESS_RETRY_THREE;

	if (!msg)
		return -EINVAL;

	if (len != HWQI_PARA_LIGHTSTRAP_CTRL_MSG_LEN) {
		hwlog_err("para error %d!=%d\n", len, HWQI_PARA_LIGHTSTRAP_CTRL_MSG_LEN);
		return -EINVAL;
	}

	if (hwqi_auto_send_fsk_msg(ic_type, HWQI_CMD_LIGHTSTRAP_CTRL, msg, len, retry))
		return -ECOMM;

	hwlog_info("send_lightstrap_control_msg succ\n");
	return 0;
}

static int hwqi_send_cert_confirm(unsigned int ic_type, bool succ_flag)
{
	int retry = WIRELESS_RETRY_ONE;

	if (succ_flag)
		return hwqi_send_msg_ack(ic_type, HWQI_CMD_CERT_SUCC,
			NULL, 0, retry);
	else
		return hwqi_send_msg_ack(ic_type, HWQI_CMD_CERT_FAIL,
			NULL, 0, retry);
}

static int hwqi_send_charge_state(unsigned int ic_type, u8 state)
{
	int retry = WIRELESS_RETRY_ONE;

	if (hwqi_send_msg_ack(ic_type, HWQI_CMD_SEND_CHRG_STATE,
		&state, HWQI_PARA_CHARGE_STATE_LEN, retry))
		return -EINVAL;

	hwlog_info("send_charge_state succ\n");
	return 0;
}

static int hwqi_send_fod_status(unsigned int ic_type, int status)
{
	int retry = WIRELESS_RETRY_ONE;

	if (hwqi_send_msg_ack(ic_type, HWQI_CMD_SEND_FOD_STATUS,
		(u8 *)&status, HWQI_PARA_FOD_STATUS_LEN, retry))
		return -EINVAL;

	hwlog_info("send_fod_status succ\n");
	return 0;
}

static int hwqi_send_charge_mode(unsigned int ic_type, u8 mode)
{
	int retry = WIRELESS_RETRY_ONE;
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev)
		return -EINVAL;

	if (l_dev->dev_id == WIRELESS_DEVICE_ID_IDTP9221) {
		hwlog_info("send_charge_mode: not support\n");
		return 0;
	}

	if (hwqi_send_msg_ack(ic_type, HWQI_CMD_SEND_CHRG_MODE,
		&mode, HWQI_PARA_CHARGE_MODE_LEN, retry))
		return -EINVAL;

	hwlog_info("send_charge_mode succ\n");
	return 0;
}

static int hwqi_set_fan_speed_limit(unsigned int ic_type, u8 limit)
{
	int retry = WIRELESS_RETRY_ONE;
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev)
		return -EINVAL;

	if (l_dev->dev_id == WIRELESS_DEVICE_ID_IDTP9221) {
		hwlog_info("set_fan_speed_limit: not support\n");
		return 0;
	}

	if (hwqi_send_msg_ack(ic_type, HWQI_CMD_SET_FAN_SPEED_LIMIT,
		&limit, HWQI_PARA_FAN_SPEED_LIMIT_LEN, retry))
		return -EINVAL;

	hwlog_info("set_fan_speed_limit succ\n");
	return 0;
}

static int hwqi_set_rpp_format_post(unsigned int ic_type, u8 pmax, int mode)
{
	struct hwqi_ops *l_ops = hwqi_get_ops(ic_type);

	if (!l_ops)
		return -EINVAL;

	if (!l_ops->set_rpp_format_post)
		return 0;

	return l_ops->set_rpp_format_post(pmax, mode, l_ops->dev_data);
}

static int hwqi_set_rpp_format(unsigned int ic_type, u8 pmax)
{
	u8 data[HWQI_PARA_RX_PMAX_LEN] = { 0 };
	int retry = WIRELESS_RETRY_ONE;
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev)
		return -EINVAL;

	if (l_dev->dev_id == WIRELESS_DEVICE_ID_IDTP9221) {
		hwlog_info("set_rpp_format: not support\n");
		return 0;
	}

	data[HWQI_PARA_RX_PMAX_OFFSET] = pmax / HWQI_PARA_RX_PMAX_UNIT;
	data[HWQI_PARA_RX_PMAX_OFFSET] <<= HWQI_PARA_RX_PMAX_SHIFT;

	if (hwqi_send_msg_ack(ic_type, HWQI_CMD_SET_RX_MAX_POWER,
		data, HWQI_PARA_RX_PMAX_LEN, retry))
		return -EINVAL;

	hwlog_info("send_rpp_format: pmax=%d, format=0x%x\n",
		pmax, data[HWQI_PARA_RX_PMAX_OFFSET]);

	if (hwqi_set_rpp_format_post(ic_type, pmax, WIRELESS_RX))
		return -EINVAL;

	return 0;
}

static int hwqi_get_ept_type(unsigned int ic_type, u16 *type)
{
	u8 data[HWQI_ACK_EPT_TYPE_LEN] = { 0 };
	int retry = WIRELESS_RETRY_TWO;
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev || !type)
		return -EINVAL;

	if (l_dev->dev_id == WIRELESS_DEVICE_ID_IDTP9221) {
		*type = 0;
		hwlog_info("get_ept_type: not support\n");
		return 0;
	}

	if (hwqi_send_msg(ic_type, HWQI_CMD_GET_EPT_TYPE, NULL, 0, data,
		HWQI_ACK_EPT_TYPE_LEN, retry))
		return -EINVAL;

	*type = (data[HWQI_ACK_EPT_TYPE_E_OFFSET] << POWER_BITS_PER_BYTE) |
		data[HWQI_ACK_EPT_TYPE_S_OFFSET];

	return 0;
}

static int hwqi_get_rpp_format(unsigned int ic_type, u8 *format)
{
	u8 data[HWQI_ACK_RPP_FORMAT_LEN] = { 0 };
	int retry = WIRELESS_RETRY_TWO;
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev || !format)
		return -EINVAL;

	if (l_dev->dev_id == WIRELESS_DEVICE_ID_IDTP9221) {
		*format = HWQI_ACK_RPP_FORMAT_8BIT;
		hwlog_info("get_rpp_format: not support\n");
		return 0;
	}

	if (hwqi_send_msg(ic_type, HWQI_CMD_GET_RPP_FORMAT, NULL, 0, data,
		HWQI_ACK_RPP_FORMAT_LEN, retry))
		return -EINVAL;

	*format = data[HWQI_ACK_RPP_FORMAT_OFFSET];

	return 0;
}

static int hwqi_get_tx_fw_version(unsigned int ic_type, char *ver, int size)
{
	int i;
	int retry = WIRELESS_RETRY_TWO;
	u8 tx_fwver[HWQI_ACK_TX_FWVER_LEN] = { 0 };
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev || !ver)
		return -EINVAL;

	memset(l_dev->info.tx_fwver, 0, sizeof(l_dev->info.tx_fwver));
	if (hwqi_send_msg(ic_type, HWQI_CMD_GET_TX_VERSION, NULL, 0, tx_fwver,
		HWQI_ACK_TX_FWVER_LEN, retry))
		return -EINVAL;

	for (i = HWQI_ACK_TX_FWVER_E; i >= HWQI_ACK_TX_FWVER_S; i--) {
		l_dev->info.tx_fwver[HWQI_ACK_TX_FWVER_E - i] = tx_fwver[i];
		if (i != HWQI_ACK_TX_FWVER_S)
			snprintf(ver + strlen(ver), size,
				"0x%02x ", tx_fwver[i]);
		else
			snprintf(ver + strlen(ver), size,
				"0x%02x", tx_fwver[i]);
	}

	return 0;
}

static int hwqi_get_tx_bigdata_info(unsigned int ic_type, char *info, int size)
{
	unsigned int i;
	int str_len;
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev || !info || (size <= (int)sizeof(l_dev->info)))
		return -EINVAL;

	memset(info, 0, size);
	str_len = 0;
	for (i = 0; i < sizeof(l_dev->info.tx_fwver); i++) {
		snprintf(info + str_len, size - str_len,
			"%02x", l_dev->info.tx_fwver[i]);
		str_len += 2; /* 2: hex -> 2 bytes str */
	}
	str_len = strlen(info);
	for (i = 0; i < sizeof(l_dev->info.tx_main_cap); i++) {
		snprintf(info + str_len, size - str_len,
			"%02x", l_dev->info.tx_main_cap[i]);
		str_len += 2; /* 2: hex -> 2 bytes str */
	}
	str_len = strlen(info);
	for (i = 0; i < sizeof(l_dev->info.tx_ext_cap); i++) {
		snprintf(info + str_len, size - str_len,
			"%02x", l_dev->info.tx_ext_cap[i]);
		str_len += 2; /* 2: hex -> 2 bytes str */
	}

	return 0;
}

static int hwqi_get_tx_id_pre(unsigned int ic_type)
{
	struct hwqi_ops *l_ops = hwqi_get_ops(ic_type);

	if (!l_ops)
		return -EINVAL;

	if (!l_ops->get_tx_id_pre)
		return 0;

	return l_ops->get_tx_id_pre(l_ops->dev_data);
}

static int hwqi_get_tx_id(unsigned int ic_type, int *id)
{
	u8 para[HWQI_PARA_TX_ID_LEN] = { HWQI_HANDSHAKE_ID_HIGH, HWQI_HANDSHAKE_ID_LOW };
	u8 data[HWQI_ACK_TX_ID_LEN] = { 0 };
	int retry = WIRELESS_RETRY_TWO;

	if (!id)
		return -EINVAL;

	if (hwqi_get_tx_id_pre(ic_type))
		return -EINVAL;

	if (hwqi_send_msg(ic_type, HWQI_CMD_GET_TX_ID, para, HWQI_PARA_TX_ID_LEN, data,
		HWQI_ACK_TX_ID_LEN, retry))
		return -EINVAL;

	if (power_cmdline_is_factory_mode())
		hwqi_send_rx_ready(ic_type);

	*id = (data[HWQI_ACK_TX_ID_S_OFFSET] << POWER_BITS_PER_BYTE) |
		data[HWQI_ACK_TX_ID_E_OFFSET];

	return 0;
}

static int hwqi_get_tx_type(unsigned int ic_type, int *type)
{
	u8 tx_fwver[HWQI_ACK_TX_FWVER_LEN] = { 0 };
	int retry = WIRELESS_RETRY_TWO;
	u16 data;
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev || !type)
		return -EINVAL;

	if (l_dev->info.tx_type != WLACC_TX_UNKNOWN) {
		*type = l_dev->info.tx_type;
		return 0;
	}

	if (hwqi_send_msg(ic_type, HWQI_CMD_GET_TX_VERSION,
		NULL, 0, tx_fwver, HWQI_ACK_TX_FWVER_LEN, retry))
		return -ENOTSUPP;

	data = (tx_fwver[HWQI_ACK_TX_FWVER_E] << POWER_BITS_PER_BYTE) |
		tx_fwver[HWQI_ACK_TX_FWVER_E - 1];
	switch (data) {
	case HWQI_TX_TYPE_CP85:
		*type = WLACC_TX_CP85;
		break;
	case HWQI_TX_TYPE_CP60:
		*type = WLACC_TX_CP60;
		break;
	case HWQI_TX_TYPE_CP61:
		*type = WLACC_TX_CP61;
		break;
	case HWQI_TX_TYPE_CP62_LX:
		*type = WLACC_TX_CP62_LX;
		break;
	case HWQI_TX_TYPE_CP62_XW:
		*type = WLACC_TX_CP62_XW;
		break;
	case HWQI_TX_TYPE_CP62R:
		*type = WLACC_TX_CP62R;
		break;
	case HWQI_TX_TYPE_CP39S:
		*type = WLACC_TX_CP39S;
		break;
	case HWQI_TX_TYPE_CP39S_HK:
		*type = WLACC_TX_CP39S_HK;
		break;
	case HWQI_TX_TYPE_CK30_LX:
		*type = WLACC_TX_CK30_LX;
		break;
	case HWQI_TX_TYPE_CK30_LP:
		*type = WLACC_TX_CK30_LP;
		break;
	default:
		*type = WLACC_TX_ERROR;
		break;
	}

	if (*(u32 *)&tx_fwver[HWQI_ACK_TX_FWVER_S] == HWQI_TX_FW_CP39S_HK)
		*type = WLACC_TX_CP39S_HK;
	l_dev->info.tx_type = *type;
	hwlog_info("get_tx_type: %d\n", *type);
	return 0;
}

static int hwqi_get_tx_adapter_type(unsigned int ic_type, int *type)
{
	u8 data[HWQI_ACK_TX_ADP_TYPE_LEN] = { 0 };
	int retry = WIRELESS_RETRY_TWO;

	if (!type)
		return -EINVAL;

	if (hwqi_send_msg(ic_type, HWQI_CMD_GET_TX_ADAPTER_TYPE,
		NULL, 0, data, HWQI_ACK_TX_ADP_TYPE_LEN, retry))
		return -EINVAL;

	*type = data[HWQI_ACK_TX_ADP_TYPE_OFFSET];

	return 0;
}

static int hwqi_get_tx_main_capability(unsigned int ic_type, u8 *cap, int len)
{
	int i;
	int retry = WIRELESS_RETRY_TWO;
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev || !cap)
		return -EINVAL;

	if (len != HWQI_PARA_TX_CAP_LEN) {
		hwlog_err("para error %d!=%d\n", len, HWQI_PARA_TX_CAP_LEN);
		return -EINVAL;
	}

	if (hwqi_send_msg(ic_type, HWQI_CMD_GET_TX_CAP, NULL, 0, cap, len, retry))
		return -EINVAL;

	for (i = HWQI_PARA_TX_MAIN_CAP_S; i <= HWQI_PARA_TX_MAIN_CAP_E; i++)
		l_dev->info.tx_main_cap[i - HWQI_PARA_TX_MAIN_CAP_S] = cap[i];

	return 0;
}

static int hwqi_get_tx_extra_capability(unsigned int ic_type, u8 *cap, int len)
{
	int i;
	int retry = WIRELESS_RETRY_TWO;
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev || !cap)
		return -EINVAL;

	if (len != HWQI_PARA_TX_EXT_CAP_LEN) {
		hwlog_err("para error %d!=%d\n", len, HWQI_PARA_TX_EXT_CAP_LEN);
		return -EINVAL;
	}

	if (hwqi_send_msg(ic_type, HWQI_CMD_GET_GET_TX_EXT_CAP, NULL, 0,
		cap, len, retry))
		return -EINVAL;

	for (i = HWQI_PARA_TX_EXT_CAP_S; i <= HWQI_PARA_TX_EXT_CAP_E; i++)
		l_dev->info.tx_ext_cap[i - HWQI_PARA_TX_EXT_CAP_S] = cap[i];

	return 0;
}

static int hwqi_get_tx_capability(unsigned int ic_type, struct wlprot_acc_cap *cap)
{
	u8 data1[HWQI_PARA_TX_CAP_LEN] = { 0 };
	u8 data2[HWQI_PARA_TX_EXT_CAP_LEN] = { 0 };

	if (!cap)
		return -EINVAL;

	if (hwqi_get_tx_main_capability(ic_type, data1, HWQI_PARA_TX_CAP_LEN))
		return -EINVAL;

	cap->adp_type = data1[HWQI_TX_CAP_TYPE];
	cap->vmax = data1[HWQI_TX_CAP_VOUT_MAX] * HWQI_PARA_TX_CAP_VOUT_STEP;
	cap->imax = data1[HWQI_TX_CAP_IOUT_MAX] * HWQI_PARA_TX_CAP_IOUT_STEP;
	cap->cable_ok = data1[HWQI_TX_CAP_ATTR] & HWQI_PARA_TX_CAP_CABLE_OK_MASK;
	cap->can_boost = data1[HWQI_TX_CAP_ATTR] & HWQI_PARA_TX_CAP_CAN_BOOST_MASK;
	cap->ext_type = data1[HWQI_TX_CAP_ATTR] & HWQI_PARA_TX_CAP_EXT_TYPE_MASK;
	cap->no_need_cert = data1[HWQI_TX_CAP_ATTR] & HWQI_PARA_TX_CAP_CERT_MASK;
	cap->support_scp = data1[HWQI_TX_CAP_ATTR] & HWQI_PARA_TX_CAP_SUPPORT_SCP_MASK;
	cap->support_12v = data1[HWQI_TX_CAP_ATTR] & HWQI_PARA_TX_CAP_SUPPORT_12V_MASK;
	cap->support_extra_cap = data1[HWQI_TX_CAP_ATTR] & HWQI_PARA_TX_CAP_SUPPORT_EXTRA_MASK;

	if ((cap->vmax > ADAPTER_9V * POWER_MV_PER_V) &&
		(cap->vmax < ADAPTER_12V * POWER_MV_PER_V) && !cap->support_12v) {
		cap->vmax = ADAPTER_9V * POWER_MV_PER_V;
		hwlog_info("tx not support 12v, set to %dmv\n", cap->vmax);
	}

	if (cap->ext_type == HWQI_PARA_TX_EXT_TYPE_CAR)
		cap->adp_type += WLACC_ADP_CAR_BASE;
	if (cap->ext_type == HWQI_PARA_TX_EXT_TYPE_PWR_BANK)
		cap->adp_type += WLACC_ADP_PWR_BANK_BASE;

	hwlog_info("tx_main_cap: adp_type=0x%x vmax=%d imax=%d\t"
		"can_boost=%d cable_ok=%d no_need_cert=%d\t"
		"[support] scp=%d 12V=%d extra_cap=%d\n",
		cap->adp_type, cap->vmax, cap->imax, cap->can_boost,
		cap->cable_ok, cap->no_need_cert, cap->support_scp,
		cap->support_12v, cap->support_extra_cap);

	if (!cap->support_extra_cap) {
		hwlog_info("tx not support extra capability\n");
		return 0;
	}

	if (hwqi_get_tx_extra_capability(ic_type, data2, HWQI_PARA_TX_EXT_CAP_LEN))
		return -EINVAL;

	cap->support_fan = data2[HWQI_TX_EXTRA_CAP_ATTR1] &
		HWQI_PARA_TX_EXT_CAP_SUPPORT_FAN_MASK;
	cap->support_tec = data2[HWQI_TX_EXTRA_CAP_ATTR1] &
		HWQI_PARA_TX_EXT_CAP_SUPPORT_TEC_MASK;
	cap->support_fod_status = data2[HWQI_TX_EXTRA_CAP_ATTR1] &
		HWQI_PARA_TX_EXT_CAP_SUPPORT_QVAL_MASK;
	cap->support_get_ept = data2[HWQI_TX_EXTRA_CAP_ATTR1] &
		HWQI_PARA_TX_EXT_CAP_SUPPORT_GET_EPT_MASK;
	cap->support_fop_range = data2[HWQI_TX_EXTRA_CAP_ATTR1] &
		HWQI_PARA_TX_EXT_CAP_SUPPORT_FOP_RANGE_MASK;

	hwlog_info("tx_extra_cap: [support] fan=%d tec=%d fod_status=%d fop_range=%d type=%u\n",
		cap->support_fan, cap->support_tec, cap->support_fod_status,
		cap->support_fop_range, ic_type);

	return 0;
}

static int hwqi_get_tx_max_power_preprocess(int tx_type)
{
	switch (tx_type) {
	case WLACC_TX_CP85:
		return HWQI_TX_MAX_POWER_CP85;
	case WLACC_TX_CP60:
		return HWQI_TX_MAX_POWER_CP60;
	case WLACC_TX_CP61:
	case WLACC_TX_CP39S:
	case WLACC_TX_CP39S_HK:
		return HWQI_TX_MAX_POWER_CP61;
	case WLACC_TX_CP62_LX:
	case WLACC_TX_CP62_XW:
		return HWQI_TX_MAX_POWER_CP62;
	default:
		return 0;
	}
}

static bool hwqi_adapter_cable_cmd_support(int tx_type)
{
	switch (tx_type) {
	case WLACC_TX_CP85:
	case WLACC_TX_CP60:
	case WLACC_TX_CP61:
	case WLACC_TX_CP39S:
	case WLACC_TX_CP39S_HK:
	case WLACC_TX_CP62_LX:
	case WLACC_TX_CP62_XW:
		return false;
	default:
		return true;
	}
}

static int hwqi_get_tx_max_power(unsigned int ic_type, int *power)
{
	u8 data[HWQI_PARA_TX_MAX_PWR_LEN] = { 0 };
	int tx_type = WLACC_TX_UNKNOWN;

	if (!power)
		return -EINVAL;

	if (!hwqi_get_tx_type(ic_type, &tx_type) &&
		!hwqi_adapter_cable_cmd_support(tx_type)) {
		*power = hwqi_get_tx_max_power_preprocess(tx_type);
		return 0;
	}

	if (hwqi_send_msg(ic_type, HWQI_CMD_GET_TX_MAX_PWR, NULL, 0, data,
		HWQI_PARA_TX_MAX_PWR_LEN, WIRELESS_RETRY_TWO))
		return -ENOTSUPP;
	*power = ((data[HWQI_PARA_TX_MAX_PWR_E] << POWER_BITS_PER_BYTE) |
		data[HWQI_PARA_TX_MAX_PWR_S]) * HWQI_PARA_TX_MAX_PWR_BASE;
	*power = *power * WLRX_ACC_TX_PWR_RATIO / POWER_PERCENT;

	hwlog_info("get_tx_max_power: %d\n", *power);

	return 0;
}

static int hwqi_get_tx_adapter_capability(unsigned int ic_type, int *vout, int *iout)
{
	u8 data[HWQI_PARA_TX_MAX_ADAPTER_LEN] = { 0 };
	int tx_type = WLACC_TX_UNKNOWN;

	if (!vout || !iout)
		return -EINVAL;

	if (!hwqi_get_tx_type(ic_type, &tx_type) &&
		!hwqi_adapter_cable_cmd_support(tx_type))
		return -ENOTSUPP;

	if (hwqi_send_msg(ic_type, HWQI_CMD_GET_TX_MAX_VIN_IIN, NULL, 0, data,
		HWQI_PARA_TX_MAX_ADAPTER_LEN, WIRELESS_RETRY_TWO))
		return -ENOTSUPP;
	*vout = (data[HWQI_PARA_TX_MAX_ADAPTER_VOUT_E] << POWER_BITS_PER_BYTE) |
		data[HWQI_PARA_TX_MAX_ADAPTER_VOUT_S];
	*iout = (data[HWQI_PARA_TX_MAX_ADAPTER_IOUT_E] << POWER_BITS_PER_BYTE) |
		data[HWQI_PARA_TX_MAX_ADAPTER_IOUT_S];

	hwlog_info("get_tx_adapter_capability: vout=%d, iout=%d\n", *vout, *iout);

	return 0;
}

static int hwqi_get_tx_cable_type(unsigned int ic_type, int *type, int *iout)
{
	u8 data[HWQI_PARA_TX_CABLE_TYPE_LEN] = { 0 };
	int tx_type = WLACC_TX_UNKNOWN;

	if (!type || !iout)
		return -EINVAL;

	if (!hwqi_get_tx_type(ic_type, &tx_type) &&
		!hwqi_adapter_cable_cmd_support(tx_type))
		return -ENOTSUPP;

	if (hwqi_send_msg(ic_type, HWQI_CMD_GET_CABLE_TYPE, NULL, 0, data,
		HWQI_PARA_TX_CABLE_TYPE_LEN, WIRELESS_RETRY_TWO))
		return -ENOTSUPP;
	*type = data[HWQI_PARA_TX_CABLE_TYPE_OFFSET];
	*iout = data[HWQI_PARA_TX_CABLE_IOUT_OFFSET] * HWQI_PARA_TX_CABLE_TYPE_BASE;

	hwlog_info("get_tx_cable_type: type=%d, iout=%d\n", *type, *iout);

	return 0;
}

static int hwqi_get_tx_fop_range(unsigned int ic_type, struct wlprot_acc_fop_range *fop)
{
	u8 data[HWQI_PARA_TX_FOP_RANGE_LEN] = { 0 };

	if (!fop)
		return -EINVAL;

	if (hwqi_send_msg(ic_type, HWQI_CMD_GET_TX_FOP_RANGE, NULL, 0, data,
		HWQI_PARA_TX_FOP_RANGE_LEN, WIRELESS_RETRY_TWO))
		return -EINVAL;

	fop->base_min = data[HWQI_TX_FOP_RANGE_BASE_MIN] + HWQI_PARA_TX_FOP_RANGE_BASE;
	fop->base_max = data[HWQI_TX_FOP_RANGE_BASE_MAX] + HWQI_PARA_TX_FOP_RANGE_BASE;
	if (data[HWQI_TX_FOP_RANGE_EXT1] != 0) {
		fop->ext1_min = data[HWQI_TX_FOP_RANGE_EXT1] +
			HWQI_PARA_TX_FOP_RANGE_BASE - HWQI_PARA_TX_FOP_RANGE_EXT_LIMIT;
		fop->ext1_max = data[HWQI_TX_FOP_RANGE_EXT1] +
			HWQI_PARA_TX_FOP_RANGE_BASE + HWQI_PARA_TX_FOP_RANGE_EXT_LIMIT;
	}
	if (data[HWQI_TX_FOP_RANGE_EXT2] != 0) {
		fop->ext2_min = data[HWQI_TX_FOP_RANGE_EXT2] +
			HWQI_PARA_TX_FOP_RANGE_BASE - HWQI_PARA_TX_FOP_RANGE_EXT_LIMIT;
		fop->ext2_max = data[HWQI_TX_FOP_RANGE_EXT2] +
			HWQI_PARA_TX_FOP_RANGE_BASE + HWQI_PARA_TX_FOP_RANGE_EXT_LIMIT;
	}

	hwlog_info("tx_fop_range: base_min=%d base_max=%d ext1_min=%d ext1_max=%d ext2_min=%d ext2_max=%d\n",
		fop->base_min, fop->base_max, fop->ext1_min, fop->ext1_max,
		fop->ext2_min, fop->ext2_max);

	return 0;
}

static int hwqi_set_encrypt_kid(u8 *data, int kid)
{
	if (!data)
		return -EINVAL;

	data[HWQI_PARA_KEY_INDEX_OFFSET] = kid;

	return 0;
}

static int hwqi_set_random_num(u8 *data, int start, int end)
{
	int i;

	if (!data)
		return -EINVAL;

	for (i = start; i <= end; i++)
		get_random_bytes(&data[i], sizeof(u8));

	return 0;
}

static int hwqi_send_random_num(unsigned int ic_type, u8 *random, int len)
{
	int i;
	int retry = WIRELESS_RETRY_ONE;

	if (!random)
		return -EINVAL;

	if (len != HWQI_PARA_RANDOM_LEN) {
		hwlog_err("para error %d!=%d\n", len, HWQI_PARA_RANDOM_LEN);
		return -EINVAL;
	}

	for (i = 0; i < len / HWQI_PARA_RANDOM_GROUP_LEN; i++) {
		if (hwqi_send_msg_ack(ic_type, HWQI_CMD_START_CERT + i,
			random + i * HWQI_PARA_RANDOM_GROUP_LEN,
			HWQI_PARA_RANDOM_GROUP_LEN, retry))
			return -EINVAL;
	}

	return 0;
}

static int hwqi_get_encrypted_value(unsigned int ic_type, u8 *hash, int len)
{
	int i;
	int retry = WIRELESS_RETRY_TWO;

	if (!hash)
		return -EINVAL;

	if (len != HWQI_ACK_HASH_LEN) {
		hwlog_err("para error %d!=%d\n", len, HWQI_ACK_HASH_LEN);
		return -EINVAL;
	}

	for (i = 0; i < len / HWQI_ACK_HASH_GROUP_LEN; i++) {
		/* cmd 0x38 & 0x39 */
		if (hwqi_send_msg(ic_type, HWQI_CMD_GET_HASH + i, NULL, 0,
			hash + i * HWQI_ACK_HASH_GROUP_LEN, HWQI_ACK_HASH_GROUP_LEN, retry))
			return -EINVAL;
	}

	return 0;
}

static int hwqi_copy_hash_value(u8 *data, int len, u8 *hash, int size)
{
	int i, j;

	for (i = 0; i < size; i++) {
		j = i + HWQI_ACK_HASH_S_OFFSET * (i / HWQI_ACK_HASH_E_OFFSET + 1);
		hash[i] = data[j];
	}

	return 0;
}

static int hwqi_auth_encrypt_start(unsigned int ic_type, int key, u8 *random, int r_size,
	u8 *hash, int h_size)
{
	u8 data[HWQI_ACK_HASH_LEN] = { 0 };
	int size;

	if (!random || !hash)
		return -EINVAL;

	memset(random, 0, r_size);
	memset(hash, 0, h_size);

	size = HWQI_PARA_RANDOM_LEN;
	if (r_size != size) {
		hwlog_err("invalid r_size=%d\n", r_size);
		return -EINVAL;
	}

	size = HWQI_ACK_HASH_LEN - HWQI_ACK_HASH_LEN / HWQI_ACK_HASH_GROUP_LEN;
	if (h_size != size) {
		hwlog_err("invalid h_size=%d\n", h_size);
		return -EINVAL;
	}

	/* first: set key index */
	if (hwqi_set_encrypt_kid(random, key))
		return -EINVAL;

	/* second: host create random num */
	if (hwqi_set_random_num(random,
		HWQI_PARA_RANDOM_S_OFFSET, HWQI_PARA_RANDOM_E_OFFSET))
		return -EINVAL;

	/* third: host set random num to slave */
	if (hwqi_send_random_num(ic_type, random, HWQI_PARA_RANDOM_LEN))
		return -EINVAL;

	/* fouth: host get hash num from slave */
	if (hwqi_get_encrypted_value(ic_type, data, HWQI_ACK_HASH_LEN))
		return -EINVAL;

	/* fifth: copy hash value */
	if (hwqi_copy_hash_value(data, HWQI_ACK_HASH_LEN, hash, h_size))
		return -EINVAL;

	return 0;
}

static int hwqi_fix_tx_fop(unsigned int ic_type, int fop)
{
	int retry = WIRELESS_RETRY_ONE;

	if (!(((fop >= HWQI_FIXED_FOP_MIN) && (fop <= HWQI_FIXED_FOP_MAX)) ||
		((fop >= HWQI_FIXED_HIGH_FOP_MIN) && (fop <= HWQI_FIXED_HIGH_FOP_MAX)))) {
		hwlog_err("fixed fop %d exceeds range[%d, %d] or [%d, %d]\n",
			fop, HWQI_FIXED_FOP_MIN, HWQI_FIXED_FOP_MAX,
			HWQI_FIXED_HIGH_FOP_MIN, HWQI_FIXED_HIGH_FOP_MAX);
		return -EINVAL;
	}

	/* cmd 0x44 */
	if (hwqi_send_msg_ack(ic_type, HWQI_CMD_FIX_TX_FOP,
		(u8 *)&fop, HWQI_PARA_TX_FOP_LEN, retry))
		return -EINVAL;

	hwlog_info("fix_tx_fop: %d\n", fop);
	return 0;
}

static int hwqi_unfix_tx_fop(unsigned int ic_type)
{
	int retry = WIRELESS_RETRY_ONE;

	/* cmd 0x45 */
	if (hwqi_send_msg_ack(ic_type, HWQI_CMD_UNFIX_TX_FOP, NULL, 0, retry))
		return -EINVAL;

	hwlog_info("unfix_tx_fop success\n");
	return 0;
}

static int hwqi_reset_dev_info(unsigned int ic_type)
{
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev)
		return -EINVAL;

	memset(&l_dev->info, 0, sizeof(l_dev->info));
	return 0;
}

static int hwqi_acc_set_tx_dev_state(unsigned int ic_type, u8 state)
{
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev)
		return -EINVAL;

	l_dev->acc_info.dev_state = state;
	hwlog_info("acc_set_tx_dev_state: 0x%x\n", state);
	return 0;
}

static int hwqi_acc_set_tx_dev_info_cnt(unsigned int ic_type, u8 cnt)
{
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev)
		return -EINVAL;

	l_dev->acc_info.dev_info_cnt = cnt;
	hwlog_info("acc_set_tx_dev_info_cnt: 0x%x\n", cnt);
	return 0;
}

static int hwqi_acc_get_tx_dev_state(unsigned int ic_type, u8 *state)
{
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev || !state)
		return -EINVAL;

	*state = l_dev->acc_info.dev_state;
	hwlog_info("acc_get_tx_dev_state: 0x%x\n", *state);
	return 0;
}

static int hwqi_acc_get_tx_dev_no(unsigned int ic_type, u8 *no)
{
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev || !no)
		return -EINVAL;

	*no = l_dev->acc_info.dev_no;
	hwlog_info("acc_get_tx_dev_no: 0x%x\n", *no);
	return 0;
}

static int hwqi_acc_get_tx_dev_mac(unsigned int ic_type, u8 *mac, int len)
{
	int i;
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev || !mac)
		return -EINVAL;

	if (len != HWQI_ACC_TX_DEV_MAC_LEN) {
		hwlog_err("para error %d!=%d\n", len, HWQI_ACC_TX_DEV_MAC_LEN);
		return -EINVAL;
	}

	for (i = 0; i < len; i++) {
		mac[i] = l_dev->acc_info.dev_mac[i];
		hwlog_info("acc_get_tx_dev_mac: mac[%d]=0x%x\n", i, mac[i]);
	}
	return 0;
}

static int hwqi_acc_get_tx_dev_model_id(unsigned int ic_type, u8 *id, int len)
{
	int i;
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev || !id)
		return -EINVAL;

	if (len != HWQI_ACC_TX_DEV_MODELID_LEN) {
		hwlog_err("para error %d!=%d\n", len, HWQI_ACC_TX_DEV_MODELID_LEN);
		return -EINVAL;
	}

	for (i = 0; i < len; i++) {
		id[i] = l_dev->acc_info.dev_model_id[i];
		hwlog_info("acc_get_tx_dev_model_id: id[%d]=0x%x\n", i, id[i]);
	}
	return 0;
}

static int hwqi_acc_get_tx_dev_submodel_id(unsigned int ic_type, u8 *id)
{
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev || !id)
		return -EINVAL;

	*id = l_dev->acc_info.dev_submodel_id;
	hwlog_info("acc_get_tx_dev_submodel_id: 0x%x\n", *id);
	return 0;
}

static int hwqi_acc_get_tx_dev_version(unsigned int ic_type, u8 *ver)
{
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev || !ver)
		return -EINVAL;

	*ver = l_dev->acc_info.dev_version;
	hwlog_info("acc_get_tx_dev_version: 0x%x\n", *ver);
	return 0;
}

static int hwqi_acc_get_tx_dev_business(unsigned int ic_type, u8 *bus)
{
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev || !bus)
		return -EINVAL;

	*bus = l_dev->acc_info.dev_business;
	hwlog_info("acc_get_tx_dev_business: 0x%x\n", *bus);
	return 0;
}

static int hwqi_acc_get_tx_dev_info_cnt(unsigned int ic_type, u8 *cnt)
{
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev || !cnt)
		return -EINVAL;

	*cnt = l_dev->acc_info.dev_info_cnt;
	hwlog_info("acc_get_tx_dev_info_cnt: %d\n", *cnt);
	return 0;
}

static int hwqi_set_tx_cipherkey(unsigned int ic_type, u8 *cipherkey, int len)
{
	int i;
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev)
		return -EINVAL;

	if (!cipherkey || (len != HWQI_TX_CIPHERKEY_LEN)) {
		hwlog_err("para error\n");
		return -EINVAL;
	}

	memset(l_dev->cipherkey_info.tx_cipherkey, 0,
		sizeof(l_dev->cipherkey_info.tx_cipherkey));
	for (i = 0; i < HWQI_TX_CIPHERKEY_LEN; i++)
		l_dev->cipherkey_info.tx_cipherkey[i] = cipherkey[i];

	hwlog_info("set_tx_cipherkey succ\n");
	return 0;
}

static int hwqi_get_rx_random(unsigned int ic_type, u8 *random, int len)
{
	int i;
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev)
		return -EINVAL;

	if (!random || (len != HWQI_RX_RANDOM_LEN)) {
		hwlog_err("para error\n");
		return -EINVAL;
	}

	for (i = 0; i < len; i++) {
		random[i] = l_dev->cipherkey_info.rx_random[i];
		l_dev->cipherkey_info.rx_random[i] = 0;
	}

	return 0;
}

static struct wireless_protocol_ops wireless_protocol_hwqi_ops = {
	.type_name = "hw_qi",
	.send_rx_evt = hwqi_send_rx_event,
	.send_rx_vout = hwqi_send_rx_vout,
	.send_rx_iout = hwqi_send_rx_iout,
	.send_rx_boost_succ = hwqi_send_rx_boost_succ,
	.send_rx_ready = hwqi_send_rx_ready,
	.send_tx_capability = hwqi_send_tx_capability,
	.send_tx_alarm = hwqi_send_tx_alarm,
	.send_tx_fw_version = hwqi_send_tx_fw_version,
	.send_lightstrap_ctrl_msg = hwqi_send_lightstrap_control_msg,
	.send_cert_confirm = hwqi_send_cert_confirm,
	.send_charge_state = hwqi_send_charge_state,
	.send_fod_status = hwqi_send_fod_status,
	.send_charge_mode = hwqi_send_charge_mode,
	.set_fan_speed_limit = hwqi_set_fan_speed_limit,
	.set_rpp_format = hwqi_set_rpp_format,
	.get_ept_type = hwqi_get_ept_type,
	.get_rpp_format = hwqi_get_rpp_format,
	.get_tx_fw_version = hwqi_get_tx_fw_version,
	.get_tx_bigdata_info = hwqi_get_tx_bigdata_info,
	.get_tx_id = hwqi_get_tx_id,
	.get_tx_type = hwqi_get_tx_type,
	.get_tx_adapter_type = hwqi_get_tx_adapter_type,
	.get_tx_capability = hwqi_get_tx_capability,
	.get_tx_max_power = hwqi_get_tx_max_power,
	.get_tx_adapter_capability = hwqi_get_tx_adapter_capability,
	.get_tx_cable_type = hwqi_get_tx_cable_type,
	.get_tx_fop_range = hwqi_get_tx_fop_range,
	.auth_encrypt_start = hwqi_auth_encrypt_start,
	.fix_tx_fop = hwqi_fix_tx_fop,
	.unfix_tx_fop = hwqi_unfix_tx_fop,
	.reset_dev_info = hwqi_reset_dev_info,
	.acc_set_tx_dev_state = hwqi_acc_set_tx_dev_state,
	.acc_set_tx_dev_info_cnt = hwqi_acc_set_tx_dev_info_cnt,
	.acc_get_tx_dev_state = hwqi_acc_get_tx_dev_state,
	.acc_get_tx_dev_no = hwqi_acc_get_tx_dev_no,
	.acc_get_tx_dev_mac = hwqi_acc_get_tx_dev_mac,
	.acc_get_tx_dev_model_id = hwqi_acc_get_tx_dev_model_id,
	.acc_get_tx_dev_submodel_id = hwqi_acc_get_tx_dev_submodel_id,
	.acc_get_tx_dev_version = hwqi_acc_get_tx_dev_version,
	.acc_get_tx_dev_business = hwqi_acc_get_tx_dev_business,
	.acc_get_tx_dev_info_cnt = hwqi_acc_get_tx_dev_info_cnt,
	.get_rx_random = hwqi_get_rx_random,
	.set_tx_cipherkey = hwqi_set_tx_cipherkey,
};

/* 0x01 + signal_strength + checksum */
static int hwqi_handle_ask_packet_cmd_0x01(unsigned int ic_type, u8 *data)
{
	hwlog_info("ask_packet_cmd_0x01: %d\n", data[HWQI_ASK_PACKET_DAT0]);
	return 0;
}

/* 0x02 + 0xaa + checksum */
static int hwqi_handle_ask_packet_cmd_0x02(unsigned int ic_type, u8 *data)
{
	u8 cmd = data[HWQI_ASK_PACKET_CMD];

	switch (cmd) {
	case HWQI_CMD_GET_RX_EPT:
		power_event_bnc_notify(POWER_BNT_LIGHTSTRAP, POWER_NE_LIGHTSTRAP_EPT, NULL);
		break;
	default:
		break;
	}

	return 0;
}

/* 0x18 + 0x05 + checksum */
static int hwqi_handle_ask_packet_cmd_0x05(unsigned int ic_type)
{
	u8 data[HWQI_ACK_TX_FWVER_LEN - 1] = { 0 };
	int retry = WIRELESS_RETRY_ONE;

	hwqi_get_chip_fw_version(ic_type, data, HWQI_ACK_TX_FWVER_LEN - 1, retry);
	hwqi_send_tx_fw_version(ic_type, data, HWQI_ACK_TX_FWVER_LEN - 1);

	hwlog_info("ask_packet_cmd_0x05\n");
	return 0;
}

/* 0x38 + 0x0a + volt_lbyte + volt_hbyte + checksum */
static int hwqi_handle_ask_packet_cmd_0x0a(unsigned int ic_type, u8 *data)
{
	int tx_vset;

	tx_vset = (data[HWQI_ASK_PACKET_DAT2] << POWER_BITS_PER_BYTE) |
		data[HWQI_ASK_PACKET_DAT1];
	power_event_bnc_notify(hwqi_get_bnt_wltx_type(ic_type),
		POWER_NE_WLTX_ASK_SET_VTX, &tx_vset);
	if (hwqi_send_fsk_ack_msg(ic_type))
		hwlog_err("ask_packet_cmd_0x0a: send ack fail\n");

	hwlog_info("ask_packet_cmd_0x0a: tx_vset=0x%x\n", tx_vset);
	return 0;
}

/* 0x28 + 0x1a + rx_evt + checksum */
static int hwqi_handle_ask_packet_cmd_0x1a(unsigned int ic_type, u8 *data)
{
	power_event_bnc_notify(hwqi_get_bnt_wltx_type(ic_type),
		POWER_NE_WLTX_ASK_RX_EVT, &data[HWQI_ASK_PACKET_DAT1]);
	if (hwqi_send_fsk_ack_msg(ic_type))
		hwlog_err("ask_packet_cmd_0x1a: send ack fail\n");

	return 0;
}

/* 0x38 + 0x3b + id_hbyte + id_lbyte + checksum */
static int hwqi_handle_ask_packet_cmd_0x3b(unsigned int ic_type, u8 *data)
{
	int tx_id;

	tx_id = (data[HWQI_ASK_PACKET_DAT1] << POWER_BITS_PER_BYTE) |
		data[HWQI_ASK_PACKET_DAT2];

	if (tx_id == HWQI_HANDSHAKE_ID_HW) {
		hwqi_send_tx_id(ic_type, &data[HWQI_ASK_PACKET_DAT1],
			HWQI_PARA_TX_ID_LEN);
		hwlog_info("0x8866 handshake succ\n");
		power_event_bnc_notify(hwqi_get_bnt_wltx_type(ic_type),
			POWER_NE_WLTX_HANDSHAKE_SUCC, NULL);
	}

	hwlog_info("ask_packet_cmd_0x3b: tx_id=0x%x\n", tx_id);
	return 0;
}

/* 0x18 + 0x41 + checksum */
static int hwqi_handle_ask_packet_cmd_0x41(unsigned int ic_type, u8 *data)
{
	power_event_bnc_notify(hwqi_get_bnt_wltx_type(ic_type),
		POWER_NE_WLTX_GET_TX_CAP, NULL);

	hwlog_info("ask_packet_cmd_0x41\n");
	return 0;
}

/* 0x18 + 0x38 + checksum */
static int hwqi_handle_ask_packet_cmd_0x38(unsigned int ic_type)
{
	int i;
	int retry = WIRELESS_RETRY_ONE;
	u8 cipherkey[HWQI_TX_CIPHERKEY_FSK_LEN] = { 0 };
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev)
		return -EINVAL;

	for (i = 0; i < HWQI_TX_CIPHERKEY_FSK_LEN; i++)
		cipherkey[i] = l_dev->cipherkey_info.tx_cipherkey[i];

	if (hwqi_send_fsk_msg(ic_type, HWQI_CMD_GET_HASH, cipherkey,
		HWQI_TX_CIPHERKEY_FSK_LEN, retry)) {
		hwlog_err("send fsk 0x38 error\n");
		return -EINVAL;
	}

	hwlog_info("ask_packet_cmd_0x38\n");
	return 0;
}

/* 0x18 + 0x39 + checksum */
static int hwqi_handle_ask_packet_cmd_0x39(unsigned int ic_type)
{
	int i;
	int retry = WIRELESS_RETRY_ONE;
	u8 cipherkey[HWQI_TX_CIPHERKEY_FSK_LEN] = { 0 };
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev)
		return -EINVAL;

	for (i = 0; i < HWQI_TX_CIPHERKEY_FSK_LEN; i++)
		cipherkey[i] = l_dev->cipherkey_info.tx_cipherkey[HWQI_TX_CIPHERKEY_FSK_LEN + i];

	if (hwqi_send_fsk_msg(ic_type, HWQI_CMD_GET_HASH7_4, cipherkey,
		HWQI_TX_CIPHERKEY_FSK_LEN, retry)) {
		hwlog_err("send fsk 0x39 error\n");
		return -EINVAL;
	}

	hwlog_info("ask_packet_cmd_0x39\n");
	return 0;
}

/* 0x28 + 0x43 + charger_state + checksum */
static int hwqi_handle_ask_packet_cmd_0x43(unsigned int ic_type, u8 *data)
{
	int chrg_state;

	chrg_state = data[HWQI_ASK_PACKET_DAT1];
	hwqi_send_fsk_ack_msg(ic_type);

	if (chrg_state & HWQI_CHRG_STATE_DONE) {
		hwlog_info("tx received rx charge-done event\n");
		power_event_bnc_notify(hwqi_get_bnt_wltx_type(ic_type),
			POWER_NE_WLTX_CHARGEDONE, NULL);
	}

	hwlog_info("ask_packet_cmd_0x43: charger_state=0x%x\n", chrg_state);
	return 0;
}

/* 0x28 + 0x55 + rx_pmax + checksum */
static int hwqi_handle_ask_packet_cmd_0x55(unsigned int ic_type, u8 *data)
{
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);
	u16 rx_pmax = data[HWQI_ASK_PACKET_DAT1] * HWQI_SEND_RX_PMAX_UNIT;

	if (!l_dev)
		return -EINVAL;

	hwqi_send_fsk_ack_msg(ic_type);
	power_event_bnc_notify(hwqi_get_bnt_wltx_type(ic_type),
		POWER_NE_WLTX_GET_RX_MAX_POWER, &rx_pmax);

	hwlog_info("ask_packet_cmd_0x55: rx_pmax=%u\n", rx_pmax);
	return 0;
}

/* 0x58 + 0x0e + mfr_id + product_id + product_type + rx_ver + checksum */
static int hwqi_handle_ask_packet_cmd_0x0e(unsigned int ic_type, u8 *data)
{
	u8 product_info[HWQI_PKT_DATA];

	product_info[HWQI_ACC_OFFSET0] = data[HWQI_ASK_PACKET_DAT3];
	product_info[HWQI_ACC_OFFSET1] = data[HWQI_ASK_PACKET_DAT2];
	hwqi_send_fsk_msg(ic_type, HWQI_CMD_ACK, product_info, 0, WIRELESS_RETRY_THREE);
	power_event_bnc_notify(POWER_BNT_LIGHTSTRAP,
		POWER_NE_LIGHTSTRAP_GET_PRODUCT_INFO, product_info);
	power_event_bnc_notify(hwqi_get_bnt_wltx_type(ic_type),
		POWER_NE_WLTX_GET_RX_PRODUCT_TYPE, &product_info[HWQI_ACC_OFFSET0]);

	hwlog_info("ask_packet_cmd_0x0e: get rx product_info\n");
	return 0;
}

/* 0x58 + 0x52 + mac1 + mac2 + mac3 + mac4 + checksum */
static int hwqi_handle_ask_packet_cmd_0x52(unsigned int ic_type, u8 *data)
{
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev)
		return -EINVAL;

	hwqi_send_fsk_ack_msg(ic_type);

	hwlog_info("ask_packet_cmd_0x52: %u\n", l_dev->acc_info.dev_info_cnt);
	if (l_dev->acc_info.dev_info_cnt != 0)
		l_dev->acc_info.dev_info_cnt = 0;

	l_dev->acc_info.dev_mac[HWQI_ACC_OFFSET0] = data[HWQI_ASK_PACKET_DAT1];
	l_dev->acc_info.dev_mac[HWQI_ACC_OFFSET1] = data[HWQI_ASK_PACKET_DAT2];
	l_dev->acc_info.dev_mac[HWQI_ACC_OFFSET2] = data[HWQI_ASK_PACKET_DAT3];
	l_dev->acc_info.dev_mac[HWQI_ACC_OFFSET3] = data[HWQI_ASK_PACKET_DAT4];
	l_dev->acc_info.dev_info_cnt += HWQI_ASK_PACKET_DAT_LEN;

	return 0;
}

/* 0x58 + 0x53 + mac5 + mac6 + version + business + checksum */
static int hwqi_handle_ask_packet_cmd_0x53(unsigned int ic_type, u8 *data)
{
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev)
		return -EINVAL;

	hwqi_send_fsk_ack_msg(ic_type);

	hwlog_info("ask_packet_cmd_0x53: %u\n", l_dev->acc_info.dev_info_cnt);
	if (l_dev->acc_info.dev_info_cnt < HWQI_ASK_PACKET_DAT_LEN) {
		hwlog_info("cmd_0x53 cnt not right\n");
		return -EINVAL;
	}

	/*
	 * solve rx not receive ack from tx, and sustain send ask packet,
	 * but tx data count check fail, reset info count
	 */
	l_dev->acc_info.dev_info_cnt = HWQI_ASK_PACKET_DAT_LEN;
	l_dev->acc_info.dev_mac[HWQI_ACC_OFFSET4] = data[HWQI_ASK_PACKET_DAT1];
	l_dev->acc_info.dev_mac[HWQI_ACC_OFFSET5] = data[HWQI_ASK_PACKET_DAT2];
	l_dev->acc_info.dev_version = data[HWQI_ASK_PACKET_DAT3];
	l_dev->acc_info.dev_business = data[HWQI_ASK_PACKET_DAT4];
	l_dev->acc_info.dev_info_cnt += HWQI_ASK_PACKET_DAT_LEN;

	return 0;
}

/* 0x58 + 0x54 + model1 + model2 + model3 + submodel + checksum */
static int hwqi_handle_ask_packet_cmd_0x54(unsigned int ic_type, u8 *data)
{
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev)
		return -EINVAL;

	hwqi_send_fsk_ack_msg(ic_type);

	hwlog_info("ask_packet_cmd_0x54: %u\n", l_dev->acc_info.dev_info_cnt);
	if (l_dev->acc_info.dev_info_cnt < HWQI_ASK_PACKET_DAT_LEN * 2) {
		hwlog_info("cmd_0x54 cnt not right\n");
		return -EINVAL;
	}

	/*
	 * solve rx not receive ack from tx, and sustain send ask packet,
	 * but tx data count check fail, reset info count
	 */
	l_dev->acc_info.dev_info_cnt = HWQI_ASK_PACKET_DAT_LEN * 2;
	l_dev->acc_info.dev_model_id[HWQI_ACC_OFFSET0] = data[HWQI_ASK_PACKET_DAT1];
	l_dev->acc_info.dev_model_id[HWQI_ACC_OFFSET1] = data[HWQI_ASK_PACKET_DAT2];
	l_dev->acc_info.dev_model_id[HWQI_ACC_OFFSET2] = data[HWQI_ASK_PACKET_DAT3];
	l_dev->acc_info.dev_submodel_id = data[HWQI_ASK_PACKET_DAT4];
	l_dev->acc_info.dev_info_cnt += HWQI_ASK_PACKET_DAT_LEN;
	l_dev->acc_info.dev_state = WLTX_ACC_DEV_STATE_ONLINE;
	power_event_bnc_notify(hwqi_get_bnt_wltx_type(ic_type),
		POWER_NE_WLTX_ACC_DEV_CONNECTED, NULL);

	hwlog_info("get acc dev info succ\n");
	return 0;
}

/* 0x18 + 0x6b + checksum */
static int hwqi_handle_ask_packet_cmd_0x6b(unsigned int ic_type)
{
	u8 data = HWQI_ACK_RPP_FORMAT_24BIT;

	return hwqi_send_fsk_msg(ic_type, HWQI_CMD_GET_RPP_FORMAT,
		&data, HWQI_FSK_RPP_FORMAT_LEN, WIRELESS_RETRY_ONE);
}

/* 0x28 + 0x6c + pmax + checksum */
static int hwqi_handle_ask_packet_cmd_0x6c(unsigned int ic_type, u8 *data)
{
	u8 pmax;

	(void)hwqi_send_fsk_ack_msg(ic_type);
	pmax = (data[HWQI_ASK_PACKET_DAT1] >> HWQI_PARA_RX_PMAX_SHIFT) *
		HWQI_PARA_RX_PMAX_UNIT;

	hwlog_info("received pmax=%dw\n", pmax);
	return hwqi_set_rpp_format_post(ic_type, pmax, WIRELESS_TX);
}

/* 0x58 + 0x36 + random0 + random1 + random2 + random3 + checksum */
static int hwqi_handle_ask_packet_cmd_0x36(unsigned int ic_type, u8 *data)
{
	int i;
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev)
		return -EINVAL;

	hwqi_send_fsk_ack_msg(ic_type);

	memset(l_dev->cipherkey_info.rx_random, 0,
		sizeof(l_dev->cipherkey_info.rx_random));
	l_dev->cipherkey_info.rx_random_len = 0;
	for (i = 0; i < HWQI_ASK_PACKET_DAT_LEN; i++) {
		l_dev->cipherkey_info.rx_random[i] = data[HWQI_PKT_DATA + i];
		l_dev->cipherkey_info.rx_random_len++;
	}

	return 0;
}

/* 0x58 + 0x37 + random4 + random5 + random6 + random7 + checksum */
static int hwqi_handle_ask_packet_cmd_0x37(unsigned int ic_type, u8 *data)
{
	u8 *rx_random = NULL;
	u8 rx_random_len;
	int i;
	struct hwqi_dev *l_dev = hwqi_get_dev(ic_type);

	if (!l_dev)
		return -EINVAL;

	hwqi_send_fsk_ack_msg(ic_type);

	for (i = 0; i < HWQI_ASK_PACKET_DAT_LEN; i++) {
		l_dev->cipherkey_info.rx_random[HWQI_ASK_PACKET_DAT_LEN + i] =
			data[HWQI_PKT_DATA + i];
		l_dev->cipherkey_info.rx_random_len++;
	}

	rx_random = wltx_auth_get_hash_data_header();
	if (!rx_random) {
		hwlog_err("get hash data header fail\n");
		return -EINVAL;
	}

	rx_random_len = wltx_auth_get_hash_data_size();
	if (l_dev->cipherkey_info.rx_random_len != rx_random_len) {
		hwlog_err("rx random len error\n");
		return -EINVAL;
	}

	wltx_auth_clean_hash_data();
	for (i = 0; i < l_dev->cipherkey_info.rx_random_len; i++)
		rx_random[i] = l_dev->cipherkey_info.rx_random[i];

	wltx_auth_wait_completion();

	wltx_auth_clean_hash_data();
	for (i = 0; i < l_dev->cipherkey_info.rx_random_len; i++)
		l_dev->cipherkey_info.rx_random[i] = 0;

	return 0;
}

static int hwqi_handle_ask_packet_hdr_0x18(unsigned int ic_type, u8 *data)
{
	u8 cmd = data[HWQI_ASK_PACKET_CMD];

	switch (cmd) {
	case HWQI_CMD_GET_TX_VERSION:
		return hwqi_handle_ask_packet_cmd_0x05(ic_type);
	case HWQI_CMD_GET_TX_CAP:
		return hwqi_handle_ask_packet_cmd_0x41(ic_type, data);
	case HWQI_CMD_GET_HASH:
		return hwqi_handle_ask_packet_cmd_0x38(ic_type);
	case HWQI_CMD_GET_HASH7_4:
		return hwqi_handle_ask_packet_cmd_0x39(ic_type);
	case HWQI_CMD_GET_RPP_FORMAT:
		return hwqi_handle_ask_packet_cmd_0x6b(ic_type);
	default:
		hwlog_err("invalid hdr=0x18 cmd=0x%x\n", cmd);
		return -EINVAL;
	}
}

static int hwqi_handle_ask_packet_hdr_0x28(unsigned int ic_type, u8 *data)
{
	u8 cmd = data[HWQI_ASK_PACKET_CMD];

	switch (cmd) {
	case HWQI_CMD_SEND_RX_EVT:
		return hwqi_handle_ask_packet_cmd_0x1a(ic_type, data);
	case HWQI_CMD_SEND_CHRG_STATE:
		return hwqi_handle_ask_packet_cmd_0x43(ic_type, data);
	case HWQI_CMD_GET_RX_PMAX:
		return hwqi_handle_ask_packet_cmd_0x55(ic_type, data);
	case HWQI_CMD_SET_RX_MAX_POWER:
		return hwqi_handle_ask_packet_cmd_0x6c(ic_type, data);
	default:
		hwlog_err("invalid hdr=0x28 cmd=0x%x\n", cmd);
		return -EINVAL;
	}
}

static int hwqi_handle_ask_packet_hdr_0x38(unsigned int ic_type, u8 *data)
{
	u8 cmd = data[HWQI_ASK_PACKET_CMD];

	switch (cmd) {
	case HWQI_CMD_SET_TX_VIN:
		return hwqi_handle_ask_packet_cmd_0x0a(ic_type, data);
	case HWQI_CMD_GET_TX_ID:
		return hwqi_handle_ask_packet_cmd_0x3b(ic_type, data);
	default:
		hwlog_err("invalid hdr=0x38 cmd=0x%x\n", cmd);
		return -EINVAL;
	}
}

static int hwqi_handle_ask_packet_hdr_0x48(unsigned int ic_type, u8 *data)
{
	u8 cmd = data[HWQI_ASK_PACKET_CMD];

	switch (cmd) {
	default:
		hwlog_err("invalid hdr=0x48 cmd=0x%x\n", cmd);
		return -EINVAL;
	}
}

static int hwqi_handle_ask_packet_hdr_0x58(unsigned int ic_type, u8 *data)
{
	u8 cmd = data[HWQI_ASK_PACKET_CMD];

	switch (cmd) {
	case HWQI_CMD_GET_RX_PRODUCT_INFO:
		return hwqi_handle_ask_packet_cmd_0x0e(ic_type, data);
	case HWQI_CMD_SEND_BT_MAC1:
		return hwqi_handle_ask_packet_cmd_0x52(ic_type, data);
	case HWQI_CMD_SEND_BT_MAC2:
		return hwqi_handle_ask_packet_cmd_0x53(ic_type, data);
	case HWQI_CMD_SEND_BT_MODEL_ID:
		return hwqi_handle_ask_packet_cmd_0x54(ic_type, data);
	case HWQI_CMD_START_CERT:
		return hwqi_handle_ask_packet_cmd_0x36(ic_type, data);
	case HWQI_CMD_SEND_RAMDOM7_4:
		return hwqi_handle_ask_packet_cmd_0x37(ic_type, data);
	default:
		hwlog_err("invalid hdr=0x58 cmd=0x%x\n", cmd);
		return -EINVAL;
	}
}

static int hwqi_handle_ask_packet_data(unsigned int ic_type, u8 *data)
{
	u8 hdr = data[HWQI_ASK_PACKET_HDR];

	switch (hdr) {
	case HWQI_CMD_GET_SIGNAL_STRENGTH:
		return hwqi_handle_ask_packet_cmd_0x01(ic_type, data);
	case HWQI_CMD_RX_EPT_TYPE:
		return hwqi_handle_ask_packet_cmd_0x02(ic_type, data);
	case HWQI_ASK_PACKET_HDR_MSG_SIZE_1_BYTE:
		return hwqi_handle_ask_packet_hdr_0x18(ic_type, data);
	case HWQI_ASK_PACKET_HDR_MSG_SIZE_2_BYTE:
		return hwqi_handle_ask_packet_hdr_0x28(ic_type, data);
	case HWQI_ASK_PACKET_HDR_MSG_SIZE_3_BYTE:
		return hwqi_handle_ask_packet_hdr_0x38(ic_type, data);
	case HWQI_ASK_PACKET_HDR_MSG_SIZE_4_BYTE:
		return hwqi_handle_ask_packet_hdr_0x48(ic_type, data);
	case HWQI_ASK_PACKET_HDR_MSG_SIZE_5_BYTE:
		return hwqi_handle_ask_packet_hdr_0x58(ic_type, data);
	default:
		hwlog_err("invalid hdr=0x%x\n", hdr);
		return -EINVAL;
	}
}

/*
 * ask: rx to tx
 * we use ask mode when rx sends a message to tx
 */
static int hwqi_handle_ask_packet(void *dev_data)
{
	u8 data[HWQI_ASK_PACKET_LEN] = { 0 };
	int retry = WIRELESS_RETRY_ONE;
	unsigned int ic_type = hwqi_get_ic_type(dev_data);

	if (hwqi_get_ask_packet(ic_type, data, HWQI_ASK_PACKET_LEN, retry))
		return -EINVAL;

	if (hwqi_handle_ask_packet_data(ic_type, data))
		return -EINVAL;

	return 0;
}

static void hwqi_handle_fsk_tx_alarm(unsigned int ic_type, u8 *pkt)
{
	u8 header;
	struct wireless_protocol_tx_alarm tx_alarm;

	header = pkt[HWQI_PKT_HDR];
	tx_alarm.src = pkt[HWQI_PKT_DATA + HWQI_TX_ALARM_SRC];
	tx_alarm.plim = pkt[HWQI_PKT_DATA + HWQI_TX_ALARM_PLIM] * HWQI_TX_ALARM_PLIM_STEP;
	if (header == HWQI_FSK_PKT_CMD_MSG_SIZE_5_BYTE) {
		tx_alarm.vlim = pkt[HWQI_PKT_DATA + HWQI_TX_ALARM_VLIM] *
			HWQI_TX_ALARM_VLIM_STEP;
		tx_alarm.reserved = pkt[HWQI_PKT_DATA + HWQI_TX_ALARM_RSVD];
	} else if (tx_alarm.src) {
		tx_alarm.vlim = ADAPTER_9V * POWER_MV_PER_V;
		tx_alarm.reserved = 0;
	} else {
		tx_alarm.vlim = 0;
		tx_alarm.reserved = 0;
	}
	power_event_bnc_notify(POWER_BNT_WLRX, POWER_NE_WLRX_TX_ALARM, &tx_alarm);
	(void)hwqi_send_msg_only(ic_type, HWQI_CMD_ACK, NULL, 0, WIRELESS_RETRY_THREE);
}

/*
 * fsk: tx to rx
 * we use fsk mode when tx sends a message to rx
 */
static void hwqi_handle_fsk_packet(u8 *pkt, int len, void *dev_data)
{
	unsigned int ic_type = hwqi_get_ic_type(dev_data);

	if (!pkt || (len != HWQI_PKT_LEN)) {
		hwlog_err("para error\n");
		return;
	}

	switch (pkt[HWQI_PKT_CMD]) {
	case HWQI_CMD_TX_ALARM:
		hwlog_info("received cmd with tx alarm\n");
		hwqi_handle_fsk_tx_alarm(ic_type, pkt);
		break;
	case HWQI_CMD_ACK_BST_ERR:
		hwlog_info("received cmd with tx boost err\n");
		power_event_bnc_notify(POWER_BNT_WLRX, POWER_NE_WLRX_TX_BST_ERR, NULL);
		break;
	default:
		break;
	}
}

static u8 hwqi_get_ask_header(int data_len)
{
	if ((data_len <= 0) ||
		(data_len >= (int)ARRAY_SIZE(g_hwqi_ask_hdr))) {
		hwlog_err("para error\n");
		return 0;
	}

	return g_hwqi_ask_hdr[data_len];
}

static u8 hwqi_get_fsk_header(int data_len)
{
	if ((data_len <= 0) ||
		(data_len >= (int)ARRAY_SIZE(g_hwqi_fsk_hdr))) {
		hwlog_err("para error\n");
		return 0;
	}

	return g_hwqi_fsk_hdr[data_len];
}

static struct hwqi_handle g_hwqi_handle = {
	.get_ask_hdr = hwqi_get_ask_header,
	.get_fsk_hdr = hwqi_get_fsk_header,
	.hdl_qi_ask_pkt = hwqi_handle_ask_packet,
	.hdl_non_qi_ask_pkt = hwqi_handle_ask_packet,
	.hdl_non_qi_fsk_pkt = hwqi_handle_fsk_packet,
};

static int __init hwqi_init(void)
{
	int ret;
	int i = 0;
	struct hwqi_dev *l_dev = NULL;

	while (i < WLTRX_IC_MAX) {
		l_dev = kzalloc(sizeof(*l_dev), GFP_KERNEL);
		if (!l_dev) {
			ret = -ENOMEM;
			goto fail_malloc_dev;
		}
		g_hwqi_dev[i] = l_dev;
		l_dev->dev_id = WIRELESS_DEVICE_ID_END;
		l_dev->acc_info.dev_no = hwqi_get_acc_dev_no(i);
		i++;
	}

	ret = wireless_protocol_ops_register(&wireless_protocol_hwqi_ops);
	if (ret)
		goto fail_register_ops;

	return 0;

fail_malloc_dev:
fail_register_ops:
	while (--i >= 0) {
		kfree(g_hwqi_dev[i]);
		g_hwqi_dev[i] = NULL;
	}
	return ret;
}

static void __exit hwqi_exit(void)
{
	int i = 0;

	while (i < WLTRX_IC_MAX) {
		kfree(g_hwqi_dev[i]);
		g_hwqi_dev[i] = NULL;
		i++;
	}
}

subsys_initcall_sync(hwqi_init);
module_exit(hwqi_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("qi protocol driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
