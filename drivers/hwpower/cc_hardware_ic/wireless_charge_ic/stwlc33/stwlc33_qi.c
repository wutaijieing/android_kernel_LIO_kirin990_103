// SPDX-License-Identifier: GPL-2.0
/*
 * stwlc33_qi.c
 *
 * stwlc33 qi_protocol driver
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

#include "stwlc33.h"

#define HWLOG_TAG wireless_stwlc33_qi
HWLOG_REGIST();

static u8 stwlc33_get_fsk_header(struct stwlc33_dev_info *di, int data_len)
{
	if (!di || !di->g_val.qi_hdl || !di->g_val.qi_hdl->get_fsk_hdr) {
		hwlog_err("get_fsk_header: para null\n");
		return 0;
	}

	return di->g_val.qi_hdl->get_fsk_hdr(data_len);
}

static int stwlc33_qi_get_ask_packet(u8 *pkt_data, int pkt_data_len, void *dev_data)
{
	int ret;
	int i;
	char buff[STWLC33_RX_TO_TX_PACKET_BUFF_LEN] = { 0 };
	char pkt_str[STWLC33_RX_TO_TX_PACKET_STR_LEN] = { 0 };
	struct stwlc33_dev_info *di = dev_data;

	if (!di || !pkt_data || (pkt_data_len <= 0) ||
		(pkt_data_len > STWLC33_RX_TO_TX_PACKET_LEN)) {
		hwlog_err("get_ask_packet: para err\n");
		return -EINVAL;
	}

	ret = stwlc33_read_block(di, STWLC33_RX_TO_TX_HEADER_ADDR, pkt_data, pkt_data_len);
	if (ret) {
		hwlog_err("get_ask_packet: read failed\n");
		return ret;
	}
	for (i = 0; i < STWLC33_RX_TO_TX_PACKET_LEN; i++) {
		snprintf(buff, STWLC33_RX_TO_TX_PACKET_BUFF_LEN, "0x%02x ", pkt_data[i]);
		strncat(pkt_str, buff, strlen(buff));
	}

	hwlog_info("[get_ask_packet] rx back packet: %s\n", pkt_str);
	return 0;
}

static int stwlc33_qi_send_fsk_msg(u8 cmd, u8 *data, int data_len, void *dev_data)
{
	int ret;
	u8 msg_byte_num;
	struct stwlc33_dev_info *di = dev_data;

	if (!di || (data_len > STWLC33_TX_TO_RX_DATA_LEN) || (data_len < 0)) {
		hwlog_err("send_fsk_msg: para err\n");
		return -EINVAL;
	}

	if (cmd == STWLC33_CMD_ACK)
		msg_byte_num = STWLC33_CMD_ACK_HEAD;
	else
		msg_byte_num = stwlc33_get_fsk_header(di, data_len + 1);

	ret = stwlc33_write_byte(di, STWLC33_TX_TO_RX_HEADER_ADDR, msg_byte_num);
	if (ret) {
		hwlog_err("send_fsk_msg: write header failed\n");
		return ret;
	}
	ret = stwlc33_write_byte(di, STWLC33_TX_TO_RX_CMD_ADDR, cmd);
	if (ret) {
		hwlog_err("send_fsk_msg: write cmd failed\n");
		return ret;
	}

	if (data && (data_len > 0)) {
		ret = stwlc33_write_block(di, STWLC33_TX_TO_RX_DATA_ADDR, data, data_len);
		if (ret) {
			hwlog_err("send_fsk_msg: write data into fsk register failed\n");
			return ret;
		}
	}
	ret = stwlc33_write_word_mask(di, STWLC33_CMD3_ADDR, STWLC33_CMD3_TX_SEND_FSK_MASK,
		STWLC33_CMD3_TX_SEND_FSK_SHIFT, STWLC33_CMD3_VAL);
	if (ret) {
		hwlog_err("send_fsk_msg: send fsk failed\n");
		return ret;
	}

	hwlog_info("[send_fsk_msg] success\n");
	return 0;
}

static struct hwqi_ops stwlc33_qi_ops = {
	.chip_name           = "stwlc33",
	.send_msg            = NULL,
	.send_msg_with_ack   = NULL,
	.receive_msg         = NULL,
	.send_fsk_msg        = stwlc33_qi_send_fsk_msg,
	.get_ask_packet      = stwlc33_qi_get_ask_packet,
	.get_chip_fw_version = NULL,
	.get_tx_id_pre       = NULL,
	.set_rpp_format_post = NULL,
};

int stwlc33_qi_ops_register(struct wltrx_ic_ops *ops, struct stwlc33_dev_info *di)
{
	if (!ops || !di)
		return -ENODEV;

	ops->qi_ops = kzalloc(sizeof(*(ops->qi_ops)), GFP_KERNEL);
	if (!ops->qi_ops)
		return -ENODEV;

	memcpy(ops->qi_ops, &stwlc33_qi_ops, sizeof(stwlc33_qi_ops));
	ops->qi_ops->dev_data = (void *)di;
	return hwqi_ops_register(ops->qi_ops, di->ic_type);
}
