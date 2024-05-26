// SPDX-License-Identifier: GPL-2.0
/*
 * stwlc68_qi.c
 *
 * stwlc68 qi_protocol driver; ask: rx->tx; fsk: tx->rx
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

#define HWLOG_TAG wireless_stwlc68_qi
HWLOG_REGIST();

static u8 stwlc68_get_ask_header(struct stwlc68_dev_info *di, int data_len)
{
	if (!di || !di->g_val.qi_hdl || !di->g_val.qi_hdl->get_ask_hdr) {
		hwlog_err("get_ask_header: para null\n");
		return 0;
	}

	return di->g_val.qi_hdl->get_ask_hdr(data_len);
}

static u8 stwlc68_get_fsk_header(struct stwlc68_dev_info *di, int data_len)
{
	if (!di || !di->g_val.qi_hdl || !di->g_val.qi_hdl->get_fsk_hdr) {
		hwlog_err("get_fsk_header: para null\n");
		return 0;
	}

	return di->g_val.qi_hdl->get_fsk_hdr(data_len);
}

static int stwlc68_qi_send_ask_msg(u8 cmd, u8 *data, int data_len, void *dev_data)
{
	int ret;
	u8 header;
	u8 write_data[STWLC68_SEND_MSG_DATA_LEN] = { 0 };
	struct stwlc68_dev_info *di = dev_data;

	if (!di) {
		hwlog_err("send_ask_msg: di null\n");
		return -ENODEV;
	}

	if ((data_len > STWLC68_SEND_MSG_DATA_LEN) || (data_len < 0)) {
		hwlog_err("send_ask_msg: send data number out of range\n");
		return -ERANGE;
	}

	di->irq_val &= ~STWLC68_RX_RCVD_MSG_INTR_LATCH;
	/* msg_len=cmd_len+data_len  cmd_len=1 */
	header = stwlc68_get_ask_header(di, data_len + 1);

	ret = stwlc68_write_byte(di, STWLC68_SEND_MSG_HEADER_ADDR, header);
	if (ret) {
		hwlog_err("send_ask_msg: write header fail\n");
		return -EIO;
	}
	ret = stwlc68_write_byte(di, STWLC68_SEND_MSG_CMD_ADDR, cmd);
	if (ret) {
		hwlog_err("send_ask_msg: write cmd fail\n");
		return -EIO;
	}

	if (data && (data_len > 0)) {
		memcpy(write_data, data, data_len);
		ret = stwlc68_write_block(di, STWLC68_SEND_MSG_DATA_ADDR, write_data, data_len);
		if (ret) {
			hwlog_err("send_ask_msg: write RX2TX-reg fail\n");
			return -EIO;
		}
	}

	ret = stwlc68_write_word_mask(di, STWLC68_CMD_ADDR, STWLC68_CMD_SEND_MSG_WAIT_RPLY,
		STWLC68_CMD_SEND_MSG_WAIT_RPLY_SHIFT, STWLC68_CMD_VAL);
	if (ret) {
		hwlog_err("send_ask_msg: send RX msg to TX fail\n");
		return -EIO;
	}

	hwlog_info("send_ask_msg: send msg(cmd:0x%x) success\n", cmd);
	return 0;
}

static int stwlc68_qi_send_ask_msg_ack(u8 cmd, u8 *data, int data_len, void *dev_data)
{
	int ret;
	int count = 0;
	int ack_cnt;
	struct stwlc68_dev_info *di = dev_data;

	do {
		(void)stwlc68_qi_send_ask_msg(cmd, data, data_len, di);
		for (ack_cnt = 0; ack_cnt < STWLC68_WAIT_FOR_ACK_RETRY_CNT; ack_cnt++) {
			(void)power_msleep(STWLC68_WAIT_FOR_ACK_SLEEP_TIME, 0, NULL);
			if (STWLC68_RX_RCVD_MSG_INTR_LATCH & di->irq_val) {
				di->irq_val &= ~STWLC68_RX_RCVD_MSG_INTR_LATCH;
				hwlog_info("[send_ask_msg_ack] succ, retry times = %d\n", count);
				return 0;
			}
			if (di->g_val.rx_stop_chrg_flag)
				return -EPERM;
		}
		count++;
		hwlog_info("[send_ask_msg_ack] retry\n");
	} while (count < STWLC68_SNED_MSG_RETRY_CNT);

	if (count < STWLC68_SNED_MSG_RETRY_CNT) {
		hwlog_info("[send_ask_msg_ack] succ\n");
		return 0;
	}

	ret = stwlc68_read_byte(di, STWLC68_RCVD_MSG_CMD_ADDR, &cmd);
	if (ret) {
		hwlog_err("send_ask_msg_ack: get rcv cmd data fail\n");
		return -EIO;
	}
	if (cmd != STWLC68_CMD_ACK) {
		hwlog_err("[send_ask_msg_ack] fail, ack = 0x%x, retry times = %d\n", cmd, count);
		return -EIO;
	}

	return 0;
}

static int stwlc68_qi_receive_fsk_msg(u8 *data, int data_len, void *dev_data)
{
	int ret;
	int count = 0;
	struct stwlc68_dev_info *di = dev_data;

	if (!di || !data) {
		hwlog_err("receive_fsk_msg: para null\n");
		return -EINVAL;
	}

	do {
		if (di->irq_val & STWLC68_RX_RCVD_MSG_INTR_LATCH) {
			di->irq_val &= ~STWLC68_RX_RCVD_MSG_INTR_LATCH;
			goto exit;
		}
		if (di->g_val.rx_stop_chrg_flag)
			return -EPERM;
		(void)power_msleep(STWLC68_RCV_MSG_SLEEP_TIME, 0, NULL);
		count++;
	} while (count < STWLC68_RCV_MSG_SLEEP_CNT);

exit:
	ret = stwlc68_read_block(di, STWLC68_RCVD_MSG_CMD_ADDR, data, data_len);
	if (ret) {
		hwlog_err("receive_fsk_msg: get tx to rx data fail\n");
		return -EIO;
	}
	if (!data[0]) { /* data[0]: cmd */
		hwlog_err("receive_fsk_msg: no msg received from tx\n");
		return -EIO;
	}
	hwlog_info("[receive_fsk_msg] get tx2rx data(cmd:0x%x) succ\n", data[0]);
	return 0;
}

static int stwlc68_qi_send_fsk_msg(u8 cmd, u8 *data, int data_len, void *dev_data)
{
	int ret;
	u8 header;
	u8 write_data[STWLC68_SEND_MSG_DATA_LEN] = { 0 };
	struct stwlc68_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	if ((data_len > STWLC68_SEND_MSG_DATA_LEN) || (data_len < 0)) {
		hwlog_err("send_fsk_msg: data len out of range\n");
		return -EINVAL;
	}

	if (cmd == STWLC68_CMD_ACK)
		header = STWLC68_CMD_ACK_HEAD;
	else
		header = stwlc68_get_fsk_header(di, data_len + 1);

	ret = stwlc68_write_byte(di, STWLC68_SEND_MSG_HEADER_ADDR, header);
	if (ret) {
		hwlog_err("send_fsk_msg: write header fail\n");
		return ret;
	}
	ret = stwlc68_write_byte(di, STWLC68_SEND_MSG_CMD_ADDR, cmd);
	if (ret) {
		hwlog_err("send_fsk_msg: write cmd fail\n");
		return ret;
	}

	if (data && data_len > 0) {
		memcpy(write_data, data, data_len);
		ret = stwlc68_write_block(di, STWLC68_SEND_MSG_DATA_ADDR, write_data, data_len);
		if (ret) {
			hwlog_err("send_fsk_msg: write fsk reg fail\n");
			return ret;
		}
	}
	ret = stwlc68_write_word_mask(di, STWLC68_CMD_ADDR, STWLC68_CMD_SEND_MSG,
		STWLC68_CMD_SEND_MSG_SHIFT, STWLC68_CMD_VAL);
	if (ret) {
		hwlog_err("send_fsk_msg: send fsk fail\n");
		return ret;
	}

	hwlog_info("[send_fsk_msg] success\n");
	return 0;
}

static int stwlc68_qi_receive_ask_pkt(u8 *pkt_data, int pkt_data_len, void *dev_data)
{
	int ret;
	int i;
	char buff[STWLC68_RCVD_PKT_BUFF_LEN] = { 0 };
	char pkt_str[STWLC68_RCVD_PKT_STR_LEN] = { 0 };
	struct stwlc68_dev_info *di = dev_data;

	if (!di || !pkt_data || (pkt_data_len <= 0) || (pkt_data_len > STWLC68_RCVD_MSG_PKT_LEN)) {
		hwlog_err("receive_ask_pkt: para err\n");
		return -EINVAL;
	}
	ret = stwlc68_read_block(di, STWLC68_RCVD_MSG_HEADER_ADDR, pkt_data, pkt_data_len);
	if (ret) {
		hwlog_err("receive_ask_pkt: read fail\n");
		return ret;
	}
	for (i = 0; i < pkt_data_len; i++) {
		snprintf(buff, STWLC68_RCVD_PKT_BUFF_LEN, "0x%02x ", pkt_data[i]);
		strncat(pkt_str, buff, strlen(buff));
	}
	hwlog_info("[receive_ask_pkt] RX back packet: %s\n", pkt_str);
	return 0;
}

static int stwlc68_get_tx_id_pre(void *dev_data)
{
	struct stwlc68_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	if (!power_cmdline_is_factory_mode() && !stwlc68_fw_sram_update(di, WIRELESS_RX))
		(void)stwlc68_rx_chip_init(WIRELESS_CHIP_INIT, WLACC_TX_UNKNOWN, di);

	return 0;
}

static int stwlc68_qi_set_rpp_format(u8 pmax, int mode, void *dev_data)
{
	u8 format = 0x80; /* bit[7:6]=10:  Qi RP24 mode No reply */

	format |= ((pmax * STWLC68_RX_RPP_VAL_UNIT) & STWLC68_RX_RPP_VAL_MASK);
	hwlog_info("[set_rpp_format] format=0x%x\n", format);
	return stwlc68_write_byte(dev_data, STWLC68_RX_RPP_SET_ADDR, format);
}

static struct hwqi_ops stwlc68_qi_ops = {
	.chip_name           = "stwlc68",
	.send_msg            = stwlc68_qi_send_ask_msg,
	.send_msg_with_ack   = stwlc68_qi_send_ask_msg_ack,
	.receive_msg         = stwlc68_qi_receive_fsk_msg,
	.send_fsk_msg        = stwlc68_qi_send_fsk_msg,
	.get_ask_packet      = stwlc68_qi_receive_ask_pkt,
	.get_chip_fw_version = stwlc68_get_chip_fw_version,
	.get_tx_id_pre       = stwlc68_get_tx_id_pre,
	.set_rpp_format_post = stwlc68_qi_set_rpp_format,
};

int stwlc68_qi_ops_register(struct wltrx_ic_ops *ops, struct stwlc68_dev_info *di)
{
	if (!ops || !di)
		return -ENODEV;

	ops->qi_ops = kzalloc(sizeof(*(ops->qi_ops)), GFP_KERNEL);
	if (!ops->qi_ops)
		return -ENODEV;

	memcpy(ops->qi_ops, &stwlc68_qi_ops, sizeof(stwlc68_qi_ops));
	ops->qi_ops->dev_data = (void *)di;
	return hwqi_ops_register(ops->qi_ops, di->ic_type);
}
