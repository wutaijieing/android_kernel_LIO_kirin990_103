// SPDX-License-Identifier: GPL-2.0
/*
 * mt5785_qi.c
 *
 * mt5785 qi_protocol driver; ask: rx->tx; fsk: tx->rx
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

#define HWLOG_TAG wireless_mt5785_qi
HWLOG_REGIST();

#define MT5785_SNED_MSG_RETRY_CNT            2
#define MT5735_SEND_MSG_DATA_LEN             4
#define MT5785_RCVD_PKT_BUFF_LEN             8
#define MT5785_RCVD_PKT_STR_LEN              64
#define MT5785_RCVD_MSG_PKT_LEN              6
#define MT5785_WAIT_FOR_ACK_RETRY_CNT        5
#define MT5785_RCV_MSG_SLEEP_CNT             10

static u8 mt5785_get_ask_header(struct mt5785_dev_info *di, int data_len)
{
	if (!di || !di->g_val.qi_hdl || !di->g_val.qi_hdl->get_ask_hdr) {
		hwlog_err("get_ask_header: para null\n");
		return 0;
	}

	return di->g_val.qi_hdl->get_ask_hdr(data_len);
}

static u8 mt5785_get_fsk_header(struct mt5785_dev_info *di, int data_len)
{
	if (!di || !di->g_val.qi_hdl || !di->g_val.qi_hdl->get_fsk_hdr) {
		hwlog_err("get_fsk_header: para null\n");
		return 0;
	}

	return di->g_val.qi_hdl->get_fsk_hdr(data_len);
}

static int mt5785_qi_send_ask_msg(u8 cmd, u8 *data, int data_len, void *dev_data)
{
	int ret;
	u8 header;
	u8 write_data[MT5785_RX_SEND_MSG_DATA_LEN] = { 0 };
	struct mt5785_dev_info *di = dev_data;

	if (!di) {
		hwlog_err("send_ask_msg: para null\n");
		return -ENODEV;
	}

	if ((data_len > MT5785_RX_SEND_MSG_DATA_LEN) || (data_len < 0)) {
		hwlog_err("send_ask_msg: data number out of range\n");
		return -EINVAL;
	}

	di->irq_val &= ~MT5785_RX_IRQ_FSK_PKT;
	/* msg_len=cmd_len+data_len  cmd_len=1 */
	header = mt5785_get_ask_header(di, data_len + 1);
	if (header <= 0) {
		hwlog_err("send_ask_msg: header wrong\n");
		return -EINVAL;
	}

	ret = mt5785_write_byte(di, MT5785_RX_SEND_MSG_HEADER_ADDR, header);
	if (ret) {
		hwlog_err("send_ask_msg: write header failed\n");
		return -EIO;
	}

	ret = mt5785_write_byte(di, MT5785_RX_SEND_MSG_CMD_ADDR, cmd);
	if (ret) {
		hwlog_err("send_ask_msg: write cmd failed\n");
		return -EIO;
	}

	if (data && (data_len > 0)) {
		memcpy(write_data, data, data_len);
		ret = mt5785_write_block(di, MT5785_RX_SEND_MSG_DATA_ADDR,
			write_data, data_len);
		if (ret) {
			hwlog_err("send_ask_msg: write rx2tx-reg failed\n");
			return -EIO;
		}
	}

	ret = mt5785_write_dword_mask(di, MT5785_RX_CMD_ADDR,
		MT5785_RX_CMD_SEND_PPP_MASK, MT5785_RX_CMD_SEND_PPP_SHIFT,
		MT5785_RX_CMD_VAL);
	if (ret) {
		hwlog_err("send_ask_msg: send rx msg to tx failed\n");
		return -EIO;
	}

	hwlog_info("[send_ask_msg] succ, cmd=0x%x\n", cmd);
	return 0;
}

static int mt5785_qi_send_ask_msg_ack(u8 cmd, u8 *data, int data_len, void *dev_data)
{
	int ret;
	int i, j;
	struct mt5785_dev_info *di = dev_data;

	if (!di) {
		hwlog_err("send_ask_msg_ack: para null\n");
		return -ENODEV;
	}

	for (i = 0; i < MT5785_SNED_MSG_RETRY_CNT; i++) {
		ret = mt5785_qi_send_ask_msg(cmd, data, data_len, di);
		if (ret)
			continue;
		for (j = 0; j < MT5785_WAIT_FOR_ACK_RETRY_CNT; j++) {
			power_msleep(DT_MSLEEP_100MS, 0, NULL);
			if (di->irq_val & MT5785_RX_IRQ_FSK_PKT) {
				di->irq_val &= ~MT5785_RX_IRQ_FSK_PKT;
				hwlog_info("[send_ask_msg_ack] succ\n");
				return 0;
			}
			if (di->g_val.rx_stop_chrg)
				return -EPERM;
		}
		hwlog_info("[send_ask_msg_ack] retry, cnt=%d\n", i);
	}

	ret = mt5785_read_byte(di, MT5785_RX_FSK_CMD_ADDR, &cmd);
	if (ret) {
		hwlog_err("send_ask_msg_ack: get rcv cmd data failed\n");
		return -EIO;
	}
	if ((cmd != HWQI_CMD_ACK) && (cmd != HWQI_CMD_NACK)) {
		hwlog_err("send_ask_msg_ack: failed, ack=0x%x, cnt=%d\n",
			cmd, i);
		return -EIO;
	}

	return 0;
}

static int mt5785_qi_receive_fsk_msg(u8 *data, int data_len, void *dev_data)
{
	int ret;
	int cnt = 0;
	struct mt5785_dev_info *di = dev_data;

	if (!di || !data) {
		hwlog_err("receive_fsk_msg: para null\n");
		return -EINVAL;
	}

	do {
		if (di->irq_val & MT5785_RX_IRQ_FSK_PKT) {
			di->irq_val &= ~MT5785_RX_IRQ_FSK_PKT;
			goto func_end;
		}

		if (di->g_val.rx_stop_chrg)
			return -EPERM;
		power_msleep(DT_MSLEEP_100MS, 0, NULL);
		cnt++;
	} while (cnt < MT5785_RCV_MSG_SLEEP_CNT);

func_end:
	ret = mt5785_read_block(di, MT5785_RX_FSK_CMD_ADDR, data, data_len);
	if (ret) {
		hwlog_err("receive_fsk_msg: get tx2rx data failed\n");
		return -EIO;
	}

	if (!data[0]) { /* data[0]: cmd */
		hwlog_err("receive_fsk_msg: no msg received from tx\n");
		return -EIO;
	}

	hwlog_info("[receive_fsk_msg] get tx2rx data(cmd:0x%x) succ\n", data[0]);
	return 0;
}

static int mt5785_qi_send_fsk_msg(u8 cmd, u8 *data, int data_len,
	void *dev_data)
{
	int ret;
	u8 header;
	u8 write_data[MT5735_SEND_MSG_DATA_LEN] = { 0 };
	struct mt5785_dev_info *di = dev_data;

	if (!di) {
		hwlog_err("send_fsk_msg: para null\n");
		return -EINVAL;
	}

	if ((data_len > MT5735_SEND_MSG_DATA_LEN) || (data_len < 0)) {
		hwlog_err("send_fsk_msg: data number out of range\n");
		return -EINVAL;
	}

	if (cmd == HWQI_CMD_ACK)
		header = HWQI_CMD_ACK_HEAD;
	else
		header = mt5785_get_fsk_header(di, data_len + 1);

	if (header <= 0) {
		hwlog_err("send_fsk_msg: header wrong\n");
		return -EINVAL;
	}

	ret = mt5785_write_byte(di, MT5785_TX_SEND_MSG_HEADER_ADDR, header);
	if (ret) {
		hwlog_err("send_fsk_msg: write header failed\n");
		return ret;
	}

	ret = mt5785_write_byte(di, MT5785_TX_SEND_MSG_CMD_ADDR, cmd);
	if (ret) {
		hwlog_err("send_fsk_msg: write cmd failed\n");
		return ret;
	}

	if (data && (data_len > 0)) {
		memcpy(write_data, data, data_len);
		ret = mt5785_write_block(di, MT5785_TX_SEND_MSG_DATA_ADDR,
			write_data, data_len);
		if (ret) {
			hwlog_err("send_fsk_msg: write fsk reg failed\n");
			return ret;
		}
	}

	ret = mt5785_write_dword_mask(di, MT5785_TX_CMD_ADDR, MT5785_TX_CMD_SEND_PPP_MASK,
		MT5785_TX_CMD_SEND_PPP_SHIFT, MT5785_TX_CMD_VALUE);
	if (ret) {
		hwlog_err("send_fsk_msg: send fsk failed\n");
		return ret;
	}

	hwlog_info("[send_fsk_msg] succ\n");
	return 0;
}

static int mt5785_qi_send_fsk_with_ack(u8 cmd, u8 *data, int data_len,
	void *dev_data)
{
	int i;
	int ret;
	struct mt5785_dev_info *di = dev_data;

	if (!di) {
		hwlog_err("send_fsk_with_ack: para null\n");
		return -ENODEV;
	}

	di->irq_val &= ~MT5785_TX_INT_FSK_PP_ACK;
	ret = mt5785_qi_send_fsk_msg(cmd, data, data_len, dev_data);
	if (ret)
		return ret;

	for (i = 0; i < MT5785_WAIT_FOR_ACK_RETRY_CNT; i++) {
		power_msleep(DT_MSLEEP_100MS, 0, NULL);
		if (di->irq_val & MT5785_TX_INT_FSK_PP_ACK) {
			di->irq_val &= ~MT5785_TX_INT_FSK_PP_ACK;
			hwlog_info("[send_fsk_with_ack] succ\n");
			return 0;
		}
		if (di->g_val.tx_stop_chrg)
			return -EPERM;
	}

	hwlog_err("send_fsk_with_ack: failed\n");
	return -EIO;
}

static int mt5785_qi_receive_ask_pkt(u8 *pkt_data, int pkt_data_len,
	void *dev_data)
{
	int ret;
	int i;
	char buff[MT5785_RCVD_PKT_BUFF_LEN] = { 0 };
	char pkt_str[MT5785_RCVD_PKT_STR_LEN] = { 0 };
	struct mt5785_dev_info *di = dev_data;

	if (!di) {
		hwlog_err("receive_ask_pkt: para null\n");
		return -ENODEV;
	}

	if (!pkt_data || (pkt_data_len <= 0) ||
		(pkt_data_len > MT5785_RCVD_MSG_PKT_LEN)) {
		hwlog_err("receive_ask_pkt: para err\n");
		return -EPERM;
	}

	ret = mt5785_read_block(di, MT5785_TX_RCVD_MSG_HEADER_ADDR,
		pkt_data, pkt_data_len);
	if (ret) {
		hwlog_err("receive_ask_pkt: read failed\n");
		return -EPERM;
	}

	for (i = 0; i < pkt_data_len; i++) {
		snprintf(buff, MT5785_RCVD_PKT_BUFF_LEN, "0x%02x ", pkt_data[i]);
		strncat(pkt_str, buff, strlen(buff));
	}

	hwlog_info("[receive_ask_pkt] %s\n", pkt_str);
	return 0;
}

static int mt5785_qi_set_rx_rpp_format(struct mt5785_dev_info *di, u8 pmax)
{
	int ret;

	ret = mt5785_write_byte(di, MT5785_RX_MAX_POWER_ADDR,
		pmax * MT5785_RX_RP_VAL_UNIT);
	ret += mt5785_write_dword_mask(di, MT5785_RX_CMD_ADDR,
		MT5785_RX_CMD_24BIT_RPP_MASK, MT5785_RX_CMD_24BIT_RPP_SHIFT, MT5785_RX_CMD_VAL);
	if (ret)
		hwlog_err("set_rx_rpp_format: failed\n");

	return ret;
}

static int mt5785_qi_set_tx_rpp_format(struct mt5785_dev_info *di, u8 pmax)
{
	int ret;

	ret = mt5785_write_byte(di, MT5785_TX_MAX_POWER_ADDR,
		pmax * MT5785_TX_RP_VAL_UNIT);
	ret += mt5785_write_dword_mask(di, MT5785_TX_CMD_ADDR,
		MT5785_TX_CMD_24BIT_RPP_EN_MASK, MT5785_TX_CMD_24BIT_RPP_EN_SHIFT,
		MT5785_TX_CMD_VALUE);
	if (ret)
		hwlog_err("set_tx_rpp_format: failed\n");

	return ret;
}

static int mt5785_get_chip_fw_version(u8 *data, int len, void *dev_data)
{
	struct mt5785_chip_info chip_info = { 0 };
	struct mt5785_dev_info *di = dev_data;

	/* fw version length must be 4 */
	if (!di || !data || (len != 4)) {
		hwlog_err("get_chip_fw_version: para err\n");
		return -EINVAL;
	}

	if (mt5785_get_chip_info(di, &chip_info)) {
		hwlog_err("get_chip_fw_version: get chip info failed\n");
		return -EIO;
	}

	/* byte[0:1]=chip_id, byte[2:3]=mtp_ver */
	data[0] = (u8)((chip_info.chip_id >> 0) & POWER_MASK_BYTE);
	data[1] = (u8)((chip_info.chip_id >> POWER_BITS_PER_BYTE) & POWER_MASK_BYTE);
	data[2] = (u8)((chip_info.mtp_ver >> 0) & POWER_MASK_BYTE);
	data[3] = (u8)((chip_info.mtp_ver >> POWER_BITS_PER_BYTE) & POWER_MASK_BYTE);

	return 0;
}

static int mt5785_qi_set_rpp_format(u8 pmax, int mode, void *dev_data)
{
	struct mt5785_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	if (mode == WIRELESS_RX)
		return mt5785_qi_set_rx_rpp_format(di, pmax);

	return mt5785_qi_set_tx_rpp_format(di, pmax);
}

static struct hwqi_ops g_mt5785_qi_ops = {
	.chip_name              = "mt5785",
	.send_msg               = mt5785_qi_send_ask_msg,
	.send_msg_with_ack      = mt5785_qi_send_ask_msg_ack,
	.receive_msg            = mt5785_qi_receive_fsk_msg,
	.send_fsk_msg           = mt5785_qi_send_fsk_msg,
	.auto_send_fsk_with_ack = mt5785_qi_send_fsk_with_ack,
	.get_ask_packet         = mt5785_qi_receive_ask_pkt,
	.get_chip_fw_version    = mt5785_get_chip_fw_version,
	.get_tx_id_pre          = NULL,
	.set_rpp_format_post    = mt5785_qi_set_rpp_format,
};

int mt5785_qi_ops_register(struct wltrx_ic_ops *ops, struct mt5785_dev_info *di)
{
	if (!ops || !di)
		return -ENODEV;

	ops->qi_ops = kzalloc(sizeof(*(ops->qi_ops)), GFP_KERNEL);
	if (!ops->qi_ops)
		return -ENODEV;

	memcpy(ops->qi_ops, &g_mt5785_qi_ops, sizeof(g_mt5785_qi_ops));
	ops->qi_ops->dev_data = (void *)di;
	return hwqi_ops_register(ops->qi_ops, di->ic_type);
}
