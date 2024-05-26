// SPDX-License-Identifier: GPL-2.0
/*
 * mt5727_qi.c
 *
 * mt5727 qi_protocol driver; ask: rx->tx; fsk: tx->rx
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

#define HWLOG_TAG wireless_mt5727_qi
HWLOG_REGIST();

static u8 mt5727_get_fsk_header(struct mt5727_dev_info *di, int data_len)
{
	if (!di || !di->g_val.qi_hdl || !di->g_val.qi_hdl->get_fsk_hdr) {
		hwlog_err("get_fsk_header: para null\n");
		return 0;
	}

	return di->g_val.qi_hdl->get_fsk_hdr(data_len);
}

static int mt5727_qi_send_ask_msg(u8 cmd, u8 *data, int data_len, void *dev_data)
{
	return 0;
}

static int mt5727_qi_send_ask_msg_ack(u8 cmd, u8 *data, int data_len, void *dev_data)
{
	return 0;
}

static int mt5727_qi_receive_fsk_msg(u8 *data, int data_len, void *dev_data)
{
	return 0;
}

static int mt5727_qi_send_fsk_msg(u8 cmd, u8 *data, int data_len, void *dev_data)
{
	int ret;
	u8 header;
	u8 write_data[MT5727_SEND_MSG_DATA_LEN] = { 0 };
	struct mt5727_dev_info *di = dev_data;

	if (!di || (data_len > MT5727_SEND_MSG_DATA_LEN) || (data_len < 0)) {
		hwlog_err("send_fsk_msg: di is null or data number out of range\n");
		return -EINVAL;
	}

	if (cmd == HWQI_CMD_ACK)
		header = HWQI_CMD_ACK_HEAD;
	else
		header = mt5727_get_fsk_header(di, data_len + 1);
	if (header <= 0) {
		hwlog_err("send_fsk_msg: header wrong, header:%u\n", header);
		return -EINVAL;
	}
	ret = mt5727_write_byte(di, MT5727_TX_SEND_MSG_HEADER_ADDR, header);
	if (ret) {
		hwlog_err("send_fsk_msg: write header failed\n");
		return ret;
	}
	ret = mt5727_write_byte(di, MT5727_TX_SEND_MSG_CMD_ADDR, cmd);
	if (ret) {
		hwlog_err("send_fsk_msg: write cmd failed\n");
		return ret;
	}

	if (data && (data_len > 0)) {
		memcpy(write_data, data, data_len);
		ret = mt5727_write_block(di, MT5727_TX_SEND_MSG_DATA_ADDR,
			write_data, data_len);
		if (ret) {
			hwlog_err("send_fsk_msg: write fsk reg failed\n");
			return ret;
		}
	}

	ret = mt5727_write_dword_mask(di, MT5727_TX_CMD_ADDR, MT5727_TX_CMD_SEND_MSG,
		MT5727_TX_CMD_SEND_MSG_SHIFT, MT5727_TX_CMD_VAL);
	if (ret) {
		hwlog_err("send_fsk_msg: send fsk failed\n");
		return ret;
	}

	hwlog_info("[send_fsk_msg] succ\n");
	return 0;
}

static int mt5727_qi_receive_ask_pkt(u8 *pkt_data, int pkt_data_len, void *dev_data)
{
	int ret;
	int i;
	char buff[MT5727_RCVD_PKT_BUFF_LEN] = { 0 };
	char pkt_str[MT5727_RCVD_PKT_STR_LEN] = { 0 };
	struct mt5727_dev_info *di = dev_data;

	if (!pkt_data || (pkt_data_len <= 0) ||
		(pkt_data_len > MT5727_RCVD_MSG_PKT_LEN)) {
		hwlog_err("get_ask_pkt: para err\n");
		return -EPERM;
	}
	ret = mt5727_read_block(di, MT5727_TX_RCVD_MSG_HEADER_ADDR,
		pkt_data, pkt_data_len);
	if (ret) {
		hwlog_err("get_ask_pkt: read failed\n");
		return -EPERM;
	}
	for (i = 0; i < pkt_data_len; i++) {
		snprintf(buff, MT5727_RCVD_PKT_BUFF_LEN, "0x%02x ", pkt_data[i]);
		strncat(pkt_str, buff, strlen(buff));
	}
	hwlog_info("[get_ask_pkt] %s\n", pkt_str);
	return 0;
}

static int mt5727_qi_set_rpp_format(u8 pmax, int mode, void *dev_data)
{
	return 0;
}

static struct hwqi_ops g_mt5727_qi_ops = {
	.chip_name           = "mt5727",
	.send_msg            = mt5727_qi_send_ask_msg,
	.send_msg_with_ack   = mt5727_qi_send_ask_msg_ack,
	.receive_msg         = mt5727_qi_receive_fsk_msg,
	.send_fsk_msg        = mt5727_qi_send_fsk_msg,
	.get_ask_packet      = mt5727_qi_receive_ask_pkt,
	.get_chip_fw_version = mt5727_get_chip_fw_version,
	.get_tx_id_pre       = NULL,
	.set_rpp_format_post = mt5727_qi_set_rpp_format,
};

int mt5727_qi_ops_register(struct wltrx_ic_ops *ops, struct mt5727_dev_info *di)
{
	if (!ops || !di)
		return -ENODEV;

	ops->qi_ops = kzalloc(sizeof(*(ops->qi_ops)), GFP_KERNEL);
	if (!ops->qi_ops)
		return -ENOMEM;

	memcpy(ops->qi_ops, &g_mt5727_qi_ops, sizeof(g_mt5727_qi_ops));
	ops->qi_ops->dev_data = (void *)di;

	return hwqi_ops_register(ops->qi_ops, di->ic_type);
}
