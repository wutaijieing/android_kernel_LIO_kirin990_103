// SPDX-License-Identifier: GPL-2.0
/*
 * cps4067_qi.c
 *
 * cps4067 qi_protocol driver; ask: rx->tx; fsk: tx->rx
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

#define HWLOG_TAG wireless_cps4067_qi
HWLOG_REGIST();

static u8 cps4067_get_ask_header(struct cps4067_dev_info *di, int data_len)
{
	if (!di || !di->g_val.qi_hdl || !di->g_val.qi_hdl->get_ask_hdr) {
		hwlog_err("get_ask_header: para null\n");
		return 0;
	}

	return di->g_val.qi_hdl->get_ask_hdr(data_len);
}

static u8 cps4067_get_fsk_header(struct cps4067_dev_info *di, int data_len)
{
	if (!di || !di->g_val.qi_hdl || !di->g_val.qi_hdl->get_fsk_hdr) {
		hwlog_err("get_fsk_header: para null\n");
		return 0;
	}

	return di->g_val.qi_hdl->get_fsk_hdr(data_len);
}

static int cps4067_qi_send_ask_msg(u8 cmd, u8 *data, int data_len, void *dev_data)
{
	int ret;
	u8 hdr;
	struct cps4067_dev_info *di = dev_data;

	if (!di || (data_len > CPS4067_SEND_MSG_DATA_LEN) || (data_len < 0)) {
		hwlog_err("send_ask_msg: input para wrong\n");
		return -EINVAL;
	}

	di->irq_val &= ~CPS4067_RX_IRQ_FSK_PKT;
	di->irq_val &= ~CPS4067_RX_IRQ_FSK_ACK;
	/* msg_len=cmd_len+data_len cmd_len=1 */
	hdr = cps4067_get_ask_header(di, data_len + 1);
	if (hdr <= 0) {
		hwlog_err("send_ask_msg: header wrong\n");
		return -EINVAL;
	}
	ret = cps4067_write_byte(di, CPS4067_SEND_MSG_HEADER_ADDR, hdr);
	if (ret) {
		hwlog_err("send_ask_msg: write header failed\n");
		return ret;
	}
	ret = cps4067_write_byte(di, CPS4067_SEND_MSG_CMD_ADDR, cmd);
	if (ret) {
		hwlog_err("send_ask_msg: write cmd failed\n");
		return ret;
	}
	if (data && (data_len > 0)) {
		ret = cps4067_write_block(di, CPS4067_SEND_MSG_DATA_ADDR, data, data_len);
		if (ret) {
			hwlog_err("send_ask_msg: write rx2tx-reg failed\n");
			return -EIO;
		}
	}

	ret = cps4067_write_byte_mask(di, CPS4067_RX_CMD_ADDR, CPS4067_RX_CMD_SEND_MSG,
		CPS4067_RX_CMD_SEND_MSG_SHIFT, CPS4067_RX_CMD_VAL);
	if (ret) {
		hwlog_err("send_ask_msg: send rx msg to tx failed\n");
		return ret;
	}

	hwlog_info("[send_ask_msg] succ, cmd=0x%x\n", cmd);
	return 0;
}

static int cps4067_qi_send_ask_msg_ack(u8 cmd, u8 *data, int data_len, void *dev_data)
{
	int ret;
	int i, j;
	struct cps4067_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	for (i = 0; i < 2; i++) { /* retry twice */
		ret = cps4067_qi_send_ask_msg(cmd, data, data_len, di);
		if (ret)
			continue;
		/* 500ms=50ms*10 timeout for sending ask msg with ack */
		for (j = 0; j < 10; j++) {
			power_msleep(DT_MSLEEP_50MS, 0, NULL);
			if (di->irq_val & CPS4067_RX_IRQ_FSK_ACK) {
				di->irq_val &= ~CPS4067_RX_IRQ_FSK_ACK;
				hwlog_info("[send_ask_msg_ack] succ\n");
				return 0;
			}
			if (di->g_val.rx_stop_chrg)
				return -EPERM;
		}
		hwlog_info("[send_ask_msg_ack] retry, cnt=%d\n", i);
	}

	ret = cps4067_read_byte(di, CPS4067_RCVD_MSG_CMD_ADDR, &cmd);
	if (ret) {
		hwlog_err("send_ask_msg_ack: get rcv cmd data failed\n");
		return -EIO;
	}
	if ((cmd != HWQI_CMD_ACK) && (cmd != HWQI_CMD_NACK)) {
		hwlog_err("send_ask_msg_ack: failed, ack=0x%x, cnt=%d\n", cmd, i);
		return -EIO;
	}

	return 0;
}

static int cps4067_qi_receive_fsk_msg(u8 *data, int data_len, void *dev_data)
{
	u8 cmd;
	int ret;
	int i;
	struct cps4067_dev_info *di = dev_data;

	if (!di || !data)
		return -EINVAL;

	/* 1000ms=100ms*10 timeout for waiting fsk pkt */
	for (i = 0; i < 10; i++) {
		if (di->irq_val & CPS4067_RX_IRQ_FSK_PKT) {
			di->irq_val &= ~CPS4067_RX_IRQ_FSK_PKT;
			goto exit;
		}
		if (di->g_val.rx_stop_chrg)
			return -EPERM;
		power_msleep(DT_MSLEEP_100MS, 0, NULL);
	}

exit:
	ret = cps4067_read_block(di, CPS4067_RCVD_MSG_CMD_ADDR, data, data_len);
	if (ret) {
		hwlog_err("receive_msg: get tx2rx data failed\n");
		return ret;
	}
	cmd = data[0]; /* data[0]: cmd */
	if (!cmd) {
		hwlog_err("receive_msg: no msg received from tx\n");
		return -EIO;
	}
	hwlog_info("[receive_msg] get tx2rx data(cmd:0x%x) succ\n", cmd);
	return 0;
}

static int cps4067_qi_send_fsk_msg(u8 cmd, u8 *data, int data_len, void *dev_data)
{
	int ret;
	u8 hdr;
	struct cps4067_dev_info *di = dev_data;

	if (!di || (data_len > CPS4067_SEND_MSG_DATA_LEN) || (data_len < 0)) {
		hwlog_err("send_fsk_msg: input para wrong\n");
		return -EINVAL;
	}

	if (cmd == HWQI_CMD_ACK)
		hdr = HWQI_CMD_ACK_HEAD;
	else
		hdr = cps4067_get_fsk_header(di, data_len + 1);
	if (hdr <= 0) {
		hwlog_err("send_fsk_msg: header wrong\n");
		return -EINVAL;
	}
	ret = cps4067_write_byte(di, CPS4067_SEND_MSG_HEADER_ADDR, hdr);
	if (ret) {
		hwlog_err("send_fsk_msg: write header failed\n");
		return ret;
	}
	ret = cps4067_write_byte(di, CPS4067_SEND_MSG_CMD_ADDR, cmd);
	if (ret) {
		hwlog_err("send_fsk_msg: write cmd failed\n");
		return ret;
	}

	if (data && data_len > 0) {
		ret = cps4067_write_block(di, CPS4067_SEND_MSG_DATA_ADDR, data, data_len);
		if (ret) {
			hwlog_err("send_fsk_msg: write fsk reg failed\n");
			return ret;
		}
	}
	ret = cps4067_write_byte_mask(di, CPS4067_TX_CMD_ADDR, CPS4067_TX_CMD_SEND_MSG,
		CPS4067_TX_CMD_SEND_MSG_SHIFT, CPS4067_TX_CMD_VAL);
	if (ret) {
		hwlog_err("send_fsk_msg: send fsk failed\n");
		return ret;
	}

	hwlog_info("[send_fsk_msg] succ\n");
	return 0;
}

static int cps4067_qi_send_fsk_with_ack(u8 cmd, u8 *data, int data_len, void *dev_data)
{
	int i;
	int ret;
	struct cps4067_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	di->irq_val &= ~CPS4067_TX_IRQ_FSK_ACK;
	ret = cps4067_qi_send_fsk_msg(cmd, data, data_len, dev_data);
	if (ret)
		return ret;

	/* 500ms=50ms*10 timeout for sending fsk with ack */
	for (i = 0; i < 10; i++) {
		power_msleep(DT_MSLEEP_50MS, 0, NULL);
		if (di->irq_val & CPS4067_TX_IRQ_FSK_ACK) {
			di->irq_val &= ~CPS4067_TX_IRQ_FSK_ACK;
			hwlog_info("[send_fsk_with_ack] succ\n");
			return 0;
		}
		if (di->g_val.tx_stop_chrg)
			return -EPERM;
	}

	hwlog_err("send_fsk_with_ack: failed\n");
	return -EIO;
}

static int cps4067_qi_receive_ask_pkt(u8 *pkt, int pkt_len, void *dev_data)
{
	int ret;
	int i;
	char buff[CPS4067_RCVD_PKT_BUFF_LEN] = { 0 };
	char pkt_str[CPS4067_RCVD_PKT_STR_LEN] = { 0 };
	struct cps4067_dev_info *di = dev_data;

	if (!di || !pkt || (pkt_len <= 0) || (pkt_len > CPS4067_RCVD_MSG_PKT_LEN)) {
		hwlog_err("get_ask_pkt: para err\n");
		return -EINVAL;
	}

	ret = cps4067_read_block(di, CPS4067_RCVD_MSG_HEADER_ADDR, pkt, pkt_len);
	if (ret) {
		hwlog_err("get_ask_pkt: read failed\n");
		return ret;
	}
	for (i = 0; i < pkt_len; i++) {
		snprintf(buff, CPS4067_RCVD_PKT_BUFF_LEN, "0x%02x ", pkt[i]);
		strncat(pkt_str, buff, strlen(buff));
	}

	hwlog_info("[get_ask_pkt] RX back packet: %s\n", pkt_str);
	return 0;
}

static int cps4067_qi_set_rx_rpp_format(struct cps4067_dev_info *di, u8 pmax)
{
	int ret;

	ret = cps4067_write_byte(di, CPS4067_RX_RP_PMAX_ADDR,
		pmax * CPS4067_RX_RP_VAL_UNIT);
	ret += cps4067_write_dword_mask(di, CPS4067_RX_FUNC_EN_ADDR,
		CPS4067_RX_RP24BIT_RES_EN_MASK, CPS4067_RX_RP24BIT_RES_EN_SHIFT,
		CPS4067_RX_FUNC_DIS);
	ret += cps4067_write_dword_mask(di, CPS4067_RX_FUNC_EN_ADDR,
		CPS4067_RX_RP24BIT_EN_MASK, CPS4067_RX_RP24BIT_EN_SHIFT,
		CPS4067_RX_FUNC_EN);
	if (ret) {
		hwlog_err("set_rx_rpp_format: failed\n");
		return -EIO;
	}

	return 0;
}

static int cps4067_qi_set_tx_rpp_format(struct cps4067_dev_info *di, u8 pmax)
{
	int ret;

	ret = cps4067_write_byte(di, CPS4067_TX_PMAX_ADDR,
		pmax * CPS4067_TX_RPP_VAL_UNIT);
	ret += cps4067_write_byte_mask(di, CPS4067_TX_FUNC_EN_ADDR,
		CPS4067_TX_24BITRPP_EN_MASK, CPS4067_TX_24BITRPP_EN_SHIFT,
		CPS4067_TX_FUNC_EN);
	if (ret) {
		hwlog_err("set_tx_rpp_format: failed\n");
		return -EIO;
	}

	return 0;
}

static int cps4067_qi_set_rpp_format(u8 pmax, int mode, void *dev_data)
{
	struct cps4067_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	if (mode == WIRELESS_RX)
		return cps4067_qi_set_rx_rpp_format(di, pmax);

	return cps4067_qi_set_tx_rpp_format(di, pmax);
}

int cps4067_qi_get_fw_version(u8 *data, int len, void *dev_data)
{
	struct cps4067_chip_info chip_info;
	struct cps4067_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	/* fw version length must be 4 */
	if (!di || !data || (len != 4)) {
		hwlog_err("get_fw_version: para err");
		return -EINVAL;
	}

	if (cps4067_get_chip_info(di, &chip_info)) {
		hwlog_err("get_fw_version: get chip_info failed\n");
		return -EIO;
	}

	/* byte[0:1]=chip_id, byte[2:3]=mtp_ver */
	data[0] = (u8)((chip_info.chip_id >> 0) & POWER_MASK_BYTE);
	data[1] = (u8)((chip_info.chip_id >> POWER_BITS_PER_BYTE) & POWER_MASK_BYTE);
	data[2] = (u8)((chip_info.mtp_ver >> 0) & POWER_MASK_BYTE);
	data[3] = (u8)((chip_info.mtp_ver >> POWER_BITS_PER_BYTE) & POWER_MASK_BYTE);

	return 0;
}

static struct hwqi_ops g_cps4067_qi_ops = {
	.chip_name              = "cps4067",
	.send_msg               = cps4067_qi_send_ask_msg,
	.send_msg_with_ack      = cps4067_qi_send_ask_msg_ack,
	.receive_msg            = cps4067_qi_receive_fsk_msg,
	.send_fsk_msg           = cps4067_qi_send_fsk_msg,
	.auto_send_fsk_with_ack = cps4067_qi_send_fsk_with_ack,
	.get_ask_packet         = cps4067_qi_receive_ask_pkt,
	.get_chip_fw_version    = cps4067_qi_get_fw_version,
	.set_rpp_format_post    = cps4067_qi_set_rpp_format,
};

int cps4067_qi_ops_register(struct wltrx_ic_ops *ops, struct cps4067_dev_info *di)
{
	if (!ops || !di)
		return -ENODEV;

	ops->qi_ops = kzalloc(sizeof(*(ops->qi_ops)), GFP_KERNEL);
	if (!ops->qi_ops)
		return -ENODEV;

	memcpy(ops->qi_ops, &g_cps4067_qi_ops, sizeof(g_cps4067_qi_ops));
	ops->qi_ops->dev_data = (void *)di;
	return hwqi_ops_register(ops->qi_ops, di->ic_type);
}
