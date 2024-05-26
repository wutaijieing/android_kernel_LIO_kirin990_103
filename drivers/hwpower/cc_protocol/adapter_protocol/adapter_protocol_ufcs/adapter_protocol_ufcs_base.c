// SPDX-License-Identifier: GPL-2.0
/*
 * adapter_protocol_ufcs_base.c
 *
 * ufcs protocol base function
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

#include "adapter_protocol_ufcs_base.h"
#include "adapter_protocol_ufcs_interface.h"
#include <chipset_common/hwpower/common_module/power_common_macro.h>
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG ufcs_protocol
HWLOG_REGIST();

static struct mutex g_msg_number_lock;
static u8 g_msg_number;

static const char * const g_hwufcs_ctl_msg_table[HWUFCS_CTL_MSG_END] = {
	[HWUFCS_CTL_MSG_PING] = "ping",
	[HWUFCS_CTL_MSG_ACK] = "ack",
	[HWUFCS_CTL_MSG_NCK] = "nck",
	[HWUFCS_CTL_MSG_ACCEPT] = "accept",
	[HWUFCS_CTL_MSG_SOFT_RESET] = "soft_reset",
	[HWUFCS_CTL_MSG_POWER_READY] = "power_ready",
	[HWUFCS_CTL_MSG_GET_OUTPUT_CAPABILITIES] = "get_output_capabilities",
	[HWUFCS_CTL_MSG_GET_SOURCE_INFO] = "get_source_info",
	[HWUFCS_CTL_MSG_GET_SINK_INFO] = "get_sink_info",
	[HWUFCS_CTL_MSG_GET_CABLE_INFO] = "get_cable_info",
	[HWUFCS_CTL_MSG_GET_DEVICE_INFO] = "get_device_info",
	[HWUFCS_CTL_MSG_GET_ERROR_INFO] = "get_error_info",
	[HWUFCS_CTL_MSG_DETECT_CABLE_INFO] = "detect_cable_info",
	[HWUFCS_CTL_MSG_START_CABLE_DETECT] = "start_cable_detect",
	[HWUFCS_CTL_MSG_END_CABLE_DETECT] = "end_cable_detect",
	[HWUFCS_CTL_MSG_EXIT_HWUFCS_MODE] = "exit_ufcs_mode",
};

static const char * const g_hwufcs_data_msg_table[HWUFCS_DATA_MSG_END] = {
	[HWUFCS_DATA_MSG_OUTPUT_CAPABILITIES] = "output_capabilities",
	[HWUFCS_DATA_MSG_REQUEST] = "request",
	[HWUFCS_DATA_MSG_SOURCE_INFO] = "source_info",
	[HWUFCS_DATA_MSG_SINK_INFO] = "sink_info",
	[HWUFCS_DATA_MSG_CABLE_INFO] = "cable_info",
	[HWUFCS_DATA_MSG_DEVICE_INFO] = "device_info",
	[HWUFCS_DATA_MSG_ERROR_INFO] = "error_info",
	[HWUFCS_DATA_MSG_CONFIG_WATCHDOG] = "config_watchdog",
	[HWUFCS_DATA_MSG_REFUSE] = "refuse",
	[HWUFCS_DATA_MSG_VERIFY_REQUEST] = "verify_request",
	[HWUFCS_DATA_MSG_VERIFY_RESPONSE] = "verify_response",
	[HWUFCS_DATA_MSG_TEST_REQUEST] = "test_request",
};

static const char *hwufcs_get_ctl_msg_name(unsigned int msg)
{
	if ((msg >= HWUFCS_CTL_MSG_BEGIN) && (msg < HWUFCS_CTL_MSG_END))
		return g_hwufcs_ctl_msg_table[msg];

	return "illegal ctl_msg";
}

static const char *hwufcs_get_data_msg_name(unsigned int msg)
{
	if ((msg >= HWUFCS_DATA_MSG_BEGIN) && (msg < HWUFCS_DATA_MSG_END))
		return g_hwufcs_data_msg_table[msg];

	return "illegal data_msg";
}

void hwufcs_init_msg_number_lock(void)
{
	mutex_init(&g_msg_number_lock);
}

void hwufcs_destroy_msg_number_lock(void)
{
	mutex_destroy(&g_msg_number_lock);
}

static u8 hwufcs_get_msg_number(void)
{
	u8 msg_number;

	mutex_lock(&g_msg_number_lock);
	msg_number = g_msg_number;
	mutex_unlock(&g_msg_number_lock);
	return msg_number;
}

static void hwufcs_set_msg_number(u8 msg_number)
{
	hwlog_info("old_msg_number=%u new_msg_number=%u\n",
		g_msg_number, msg_number);
	mutex_lock(&g_msg_number_lock);
	g_msg_number = (msg_number % HWUFCS_MSG_MAX_COUNTER);
	mutex_unlock(&g_msg_number_lock);
}

static void hwufcs_packet_head(struct hwufcs_package_data *pkt, u8 *buf)
{
	u16 data = 0;

	/* packet tx header data */
	data |= ((pkt->msg_type & HWUFCS_HDR_MASK_MSG_TYPE) <<
		HWUFCS_HDR_SHIFT_MSG_TYPE);
	data |= ((pkt->prot_version & HWUFCS_HDR_MASK_PROT_VERSION) <<
		HWUFCS_HDR_SHIFT_PROT_VERSION);
	data |= ((pkt->msg_number & HWUFCS_HDR_MASK_MSG_NUMBER) <<
		HWUFCS_HDR_SHIFT_MSG_NUMBER);
	data |= ((pkt->dev_address & HWUFCS_HDR_MASK_DEV_ADDRESS) <<
		HWUFCS_HDR_SHIFT_DEV_ADDRESS);

	/* fill tx header data */
	buf[HWUFCS_HDR_HEADER_H_OFFSET] = (data >> POWER_BITS_PER_BYTE) & POWER_MASK_BYTE;
	buf[HWUFCS_HDR_HEADER_L_OFFSET] = (data >> 0) & POWER_MASK_BYTE;
}

static void hwufcs_packet_ctrl_and_data_msg(struct hwufcs_package_data *pkt, u8 *buf)
{
	hwlog_info("type=%u ver=%u num=%u addr=%u cmd=0x%x len=%u\n",
		pkt->msg_type, pkt->prot_version, pkt->msg_number,
		pkt->dev_address, pkt->cmd, pkt->len);

	/* fill tx body data */
	buf[HWUFCS_CTRL_DATA_MSG_CMD_OFFSET] = pkt->cmd;
	buf[HWUFCS_CTRL_DATA_MSG_LEN_OFFSET] = pkt->len;
	if (pkt->len)
		memcpy(&buf[HWUFCS_CTRL_DATA_MSG_DATA_OFFSET], pkt->data, pkt->len);
}

static void hwufcs_packet_vendor_define_msg(struct hwufcs_package_data *pkt, u8 *buf)
{
	hwlog_info("type=%u ver=%u num=%u addr=%u vendor_id=0x%x len=%u\n",
		pkt->msg_type, pkt->prot_version, pkt->msg_number,
		pkt->dev_address, pkt->vender_id, pkt->len);

	/* fill tx body data */
	buf[HWUFCS_VD_MSG_VER_ID_H_OFFSET] = pkt->vender_id >> POWER_BITS_PER_BYTE;
	buf[HWUFCS_VD_MSG_VER_ID_L_OFFSET] = pkt->vender_id & POWER_MASK_BYTE;
	buf[HWUFCS_VD_MSG_MSG_LEN_OFFSET] = pkt->len;
	if (pkt->len)
		memcpy(&buf[HWUFCS_VD_MSG_DATA_OFFSET], pkt->data, pkt->len);
}

static int hwufcs_packet_tx_data(struct hwufcs_package_data *pkt, int pkt_len,
	u8 *buf, int buf_len)
{
	if (pkt_len > buf_len) {
		hwlog_err("length err, pkt_len=%d, buf_len=%d\n", pkt_len, buf_len);
		return -EPERM;
	}

	hwufcs_packet_head(pkt, buf);
	switch (pkt->msg_type) {
	case HWUFCS_MSG_TYPE_CONTROL:
	case HWUFCS_MSG_TYPE_DATA:
		hwufcs_packet_ctrl_and_data_msg(pkt, buf);
		break;
	case HWUFCS_MSG_TYPE_VENDOR_DEFINED:
		hwufcs_packet_vendor_define_msg(pkt, buf);
		break;
	default:
		hwlog_info("invalid msg_type=%d\n", pkt->msg_type);
		break;
	}

	return 0;
}

static void hwufcs_unpacket_rx_header(struct hwufcs_package_data *pkt, u8 *buf)
{
	u16 data;

	/* unpacket rx header data */
	data = (buf[HWUFCS_HDR_HEADER_H_OFFSET] << POWER_BITS_PER_BYTE) |
		buf[HWUFCS_HDR_HEADER_L_OFFSET];
	pkt->msg_type = (data >> HWUFCS_HDR_SHIFT_MSG_TYPE) &
		HWUFCS_HDR_MASK_MSG_TYPE;
	pkt->prot_version = (data >> HWUFCS_HDR_SHIFT_PROT_VERSION) &
		HWUFCS_HDR_MASK_PROT_VERSION;
	pkt->msg_number = (data >> HWUFCS_HDR_SHIFT_MSG_NUMBER) &
		HWUFCS_HDR_MASK_MSG_NUMBER;
	pkt->dev_address = (data >> HWUFCS_HDR_SHIFT_DEV_ADDRESS) &
		HWUFCS_HDR_MASK_DEV_ADDRESS;
}

static void hwufcs_unpacket_ctrl_and_data_msg(struct hwufcs_package_data *pkt, u8 *buf)
{
	/* get rx body data */
	pkt->cmd = buf[HWUFCS_CTRL_DATA_MSG_CMD_OFFSET];
	pkt->len = buf[HWUFCS_CTRL_DATA_MSG_LEN_OFFSET];
	if (pkt->len && (pkt->len <= HWUFCS_CTRL_DATA_MSG_MAX_BUF_SIZE))
		memcpy(pkt->data, &buf[HWUFCS_CTRL_DATA_MSG_DATA_OFFSET], pkt->len);

	hwlog_info("type=%u ver=%u num=%u addr=%u cmd=0x%x len=%u\n",
		pkt->msg_type, pkt->prot_version, pkt->msg_number,
		pkt->dev_address, pkt->cmd, pkt->len);
}

static void hwufcs_unpacket_vendor_define_msg(struct hwufcs_package_data *pkt, u8 *buf)
{
	/* get rx body data */
	pkt->vender_id = (buf[HWUFCS_VD_MSG_VER_ID_H_OFFSET] << POWER_BITS_PER_BYTE) |
		buf[HWUFCS_VD_MSG_VER_ID_L_OFFSET];
	pkt->len = buf[HWUFCS_VD_MSG_MSG_LEN_OFFSET];
	if (pkt->len && (pkt->len <= HWUFCS_VD_MSG_MAX_BUF_SIZE))
		memcpy(pkt->data, &buf[HWUFCS_VD_MSG_DATA_OFFSET], pkt->len);

	hwlog_info("type=%u ver=%u num=%u addr=%u vendor_id=0x%x len=%u\n",
		pkt->msg_type, pkt->prot_version, pkt->msg_number,
		pkt->dev_address, pkt->vender_id, pkt->len);
}

static int hwufcs_unpacket_rx_data(struct hwufcs_package_data *pkt, u8 *buf, int buf_len)
{
	if (buf_len > sizeof(struct hwufcs_package_data)) {
		hwlog_err("length err, buf_len=%d\n", buf_len);
		return -EPERM;
	}

	hwufcs_unpacket_rx_header(pkt, buf);
	switch (pkt->msg_type) {
	case HWUFCS_MSG_TYPE_CONTROL:
	case HWUFCS_MSG_TYPE_DATA:
		hwufcs_unpacket_ctrl_and_data_msg(pkt, buf);
		break;
	case HWUFCS_MSG_TYPE_VENDOR_DEFINED:
		hwufcs_unpacket_vendor_define_msg(pkt, buf);
		break;
	default:
		hwlog_info("invalid msg_type=%d\n", pkt->msg_type);
		break;
	}

	return 0;
}

static int hwufcs_handle_refuse_data_msg(struct hwufcs_package_data *pkt)
{
	u32 data = 0;
	u32 tmp_data;
	u8 len = sizeof(u32);
	struct hwufcs_refuse_data p = {0};

	if ((pkt->msg_type != HWUFCS_MSG_TYPE_DATA) ||
		(pkt->cmd != HWUFCS_DATA_MSG_REFUSE)) {
		hwlog_err("type=%d,cmd=0x%x,is not refuse\n", pkt->msg_type, pkt->cmd);
		return -EPERM;
	}

	if (len != pkt->len) {
		hwlog_err("refuse length=%u,%u invalid\n", len, pkt->len);
		return -EPERM;
	}

	memcpy((u8 *)&data, pkt->data, pkt->len);
	tmp_data = cpu_to_be32(data);
	/* reason */
	p.reason = (tmp_data >> HWUFCS_REFUSE_SHIFT_REASON) &
		HWUFCS_REFUSE_MASK_REASON;
	/* command number */
	p.cmd_number = (tmp_data >> HWUFCS_REFUSE_SHIFT_CMD_NUMBER) &
		HWUFCS_REFUSE_MASK_CMD_NUMBER;
	/* message type */
	p.msg_type = (tmp_data >> HWUFCS_REFUSE_SHIFT_MSG_TYPE) &
		HWUFCS_REFUSE_MASK_MSG_TYPE;
	/* message number */
	p.msg_number = (tmp_data >> HWUFCS_REFUSE_SHIFT_MSG_NUMBER) &
		HWUFCS_REFUSE_MASK_MSG_NUMBER;

	hwlog_info("refuse=%lx %x %x %x %x\n", tmp_data,
		p.reason, p.cmd_number, p.msg_type, p.msg_number);
	return 0;
}

static int hwufcs_send_control_cmd(u8 msg_number, u8 cmd)
{
	struct hwufcs_package_data pkt = { 0 };
	u8 buf[HWUFCS_MSG_MAX_BUFFER_SIZE] = { 0 };
	u8 flag = HWUFCS_WAIT_SEND_PACKET_COMPLETE | HWUFCS_ACK_RECEIVE_TIMEOUT;
	int len = HWUFCS_HDR_HEADER_LEN + HWUFCS_MSG_CMD_LEN;
	int ret;

	pkt.msg_type = HWUFCS_MSG_TYPE_CONTROL;
	pkt.prot_version = HWUFCS_MSG_PROT_VERSION;
	pkt.msg_number = msg_number;
	pkt.dev_address = HWUFCS_DEV_ADDRESS_SOURCE;
	pkt.cmd = cmd;
	pkt.len = 0;

	hwlog_info("send ctrl msg=%s\n", hwufcs_get_ctl_msg_name(pkt.cmd));
	ret = hwufcs_packet_tx_data(&pkt, len, buf, HWUFCS_MSG_MAX_BUFFER_SIZE);
	if (ret) {
		hwlog_err("packet control msg msg_number=%u cmd=%u fail\n",
			msg_number, cmd);
		return -EPERM;
	}

	ret = hwufcs_write_msg(buf, len, flag);
	if (ret) {
		hwlog_err("send control msg msg_number=%u cmd=%u fail\n",
			msg_number, cmd);
		return -EPERM;
	}

	return 0;
}

static int hwufcs_send_data_cmd(u8 msg_number, u8 cmd, u8 *data, u8 len)
{
	struct hwufcs_package_data pkt = { 0 };
	u8 buf[HWUFCS_MSG_MAX_BUFFER_SIZE] = { 0 };
	u8 flag = HWUFCS_WAIT_SEND_PACKET_COMPLETE | HWUFCS_ACK_RECEIVE_TIMEOUT;
	int ret;
	int len_all = HWUFCS_HDR_HEADER_LEN + HWUFCS_MSG_CMD_LEN +
		HWUFCS_MSG_LENGTH_LEN + len;

	if (len > HWUFCS_CTRL_DATA_MSG_MAX_BUF_SIZE) {
		hwlog_err("invalid length=%u\n", len);
		return -EINVAL;
	}

	pkt.msg_type = HWUFCS_MSG_TYPE_DATA;
	pkt.prot_version = HWUFCS_MSG_PROT_VERSION;
	pkt.msg_number = msg_number;
	pkt.dev_address = HWUFCS_DEV_ADDRESS_SOURCE;
	pkt.cmd = cmd;
	pkt.len = len;
	memcpy(pkt.data, data, len);

	hwlog_info("send data msg=%s\n", hwufcs_get_data_msg_name(pkt.cmd));
	ret = hwufcs_packet_tx_data(&pkt, len_all, buf, HWUFCS_MSG_MAX_BUFFER_SIZE);
	if (ret) {
		hwlog_err("packet data msg msg_number=%u cmd=%u fail\n",
			msg_number, cmd);
		return -EPERM;
	}

	ret = hwufcs_write_msg(buf, len_all, flag);
	if (ret) {
		hwlog_err("send data msg msg_number=%u cmd=%u fail\n",
			msg_number, cmd);
		return -EPERM;
	}

	return 0;
}

#ifdef POWER_MODULE_DEBUG_FUNCTION
static int hwufcs_send_vendor_defined_cmd(u8 msg_number, u8 *data, u8 len)
{
	struct hwufcs_package_data pkt = { 0 };
	u8 buf[HWUFCS_MSG_MAX_BUFFER_SIZE] = { 0 };
	u8 flag = HWUFCS_WAIT_SEND_PACKET_COMPLETE | HWUFCS_ACK_RECEIVE_TIMEOUT;
	int ret;
	int len_all = HWUFCS_HDR_HEADER_LEN + HWUFCS_MSG_VER_ID_LEN +
		HWUFCS_MSG_LENGTH_LEN + len;

	if (len > HWUFCS_VD_MSG_MAX_BUF_SIZE) {
		hwlog_err("invalid length=%u\n", len);
		return -EINVAL;
	}

	pkt.msg_type = HWUFCS_MSG_TYPE_VENDOR_DEFINED;
	pkt.prot_version = HWUFCS_MSG_PROT_VERSION;
	pkt.msg_number = msg_number;
	pkt.dev_address = HWUFCS_DEV_ADDRESS_SOURCE;
	pkt.vender_id = HWUFCS_MSG_VERSION_ID;
	pkt.len = len;
	memcpy(pkt.data, data, len);

	ret = hwufcs_packet_tx_data(&pkt, len_all, buf, HWUFCS_MSG_MAX_BUFFER_SIZE);
	if (ret) {
		hwlog_err("packet vendor defined msg msg_number=%u fail\n",
			msg_number);
		return -EPERM;
	}

	ret = hwufcs_write_msg(buf, len_all, flag);
	if (ret) {
		hwlog_err("send vendor defined msg msg_number=%u fail\n",
			msg_number);
		return -EPERM;
	}

	return 0;
}
#endif /* POWER_MODULE_DEBUG_FUNCTION */

int hwufcs_receive_msg(struct hwufcs_package_data *pkt, bool need_wait_msg)
{
	u8 buf[HWUFCS_MSG_MAX_BUFFER_SIZE] = { 0 };
	u8 flag = HWUFCS_WAIT_CRC_ERROR | HWUFCS_WAIT_DATA_READY;
	u8 buf_len = 0;
	int ret;

	/* wait msg from rx */
	if (need_wait_msg && hwufcs_wait_msg_ready(flag)) {
		hwlog_err("wait for msg time out\n");
		return -EPERM;
	}

	ret = hwufcs_get_rx_length(&buf_len);
	if (ret || !buf_len || (buf_len > HWUFCS_MSG_MAX_BUFFER_SIZE)) {
		hwlog_err("get data length err, len=%d,ret=%d\n", buf_len, ret);
		return -EPERM;
	}

	hwlog_info("rx msg len=%d\n", buf_len);
	/* receive msg header from rx */
	ret = hwufcs_read_msg(buf, buf_len);
	if (ret)
		return -EPERM;

	ret = hwufcs_end_read_msg();
	if (ret)
		return -EPERM;

	ret = hwufcs_unpacket_rx_data(pkt, buf, buf_len);
	if (ret)
		return -EPERM;

	return 0;
}

int hwufcs_receive_control_msg(u8 cmd, bool check_msg_num)
{
	struct hwufcs_package_data pkt = { 0 };
	u8 msg_number = hwufcs_get_msg_number();

	if ((cmd == HWUFCS_CTL_MSG_ACK) && !hwufcs_need_check_ack())
		return 0;

	if (hwufcs_receive_msg(&pkt, true)) {
		hwlog_err("receive msg err\n");
		return -EPERM;
	}

	/* check control msg header */
	if (check_msg_num && (pkt.msg_number != msg_number)) {
		hwlog_err("msg_number=%u,%u invalid\n",
			pkt.msg_number, msg_number);
		return -EPERM;
	}

	if (pkt.msg_type != HWUFCS_MSG_TYPE_CONTROL) {
		hwlog_err("receive_control_msg msg_type=%u,%u invalid\n",
			pkt.msg_type, HWUFCS_MSG_TYPE_CONTROL);
		hwufcs_handle_refuse_data_msg(&pkt);
		return -EPERM;
	}

	if (pkt.cmd != cmd) {
		hwlog_err("receive_control_msg cmd=%u,%u invalid\n",
			pkt.cmd, cmd);
		return -EPERM;
	}

	return 0;
}

static int hwufcs_receive_data_msg(u8 cmd, u8 *data, u8 len, u8 *ret_len)
{
	struct hwufcs_package_data pkt = { 0 };

	if (hwufcs_receive_msg(&pkt, true)) {
		hwlog_err("receive msg err\n");
		return -EPERM;
	}

	/* check data msg header */
	if (pkt.msg_type != HWUFCS_MSG_TYPE_DATA) {
		hwlog_err("receive_data_msg msg_type=%u,%u invalid\n",
			pkt.msg_type, HWUFCS_MSG_TYPE_DATA);
		return -EPERM;
	}

	/* check data msg header */
	if (pkt.cmd != cmd) {
		hwlog_err("receive_data_msg cmd=%u,%u invalid\n",
			pkt.cmd, cmd);
		hwufcs_handle_refuse_data_msg(&pkt);
		return -EPERM;
	}
	if (pkt.len > len) {
		hwlog_err("receive_data_msg length=%u,%u invalid\n",
			pkt.len, len);
		return -EPERM;
	}

	memcpy(data, pkt.data, pkt.len);
	*ret_len = pkt.len;

	return 0;
}

int hwufcs_send_control_msg(u8 cmd, bool ack)
{
	u8 msg_number = hwufcs_get_msg_number();
	int ret;

	/* send control cmd to tx */
	ret = hwufcs_send_control_cmd(msg_number, cmd);
	if (ret)
		return -EPERM;

	if (!ack)
		goto end;

	/* wait ack from rx */
	ret = hwufcs_receive_control_msg(HWUFCS_CTL_MSG_ACK, true);
	if (ret) {
		hwlog_err("wait for ack err\n");
		return -EPERM;
	}

end:
	hwufcs_set_msg_number(++msg_number);
	return 0;
}

static int hwufcs_send_data_msg(u8 cmd, u8 *data, u8 len, bool ack)
{
	u8 msg_number = hwufcs_get_msg_number();
	int ret;

	/* send data cmd to tx */
	ret = hwufcs_send_data_cmd(msg_number, cmd, data, len);
	if (ret)
		return -EPERM;

	if (!ack)
		goto end;

	/* wait ack from rx */
	ret = hwufcs_receive_control_msg(HWUFCS_CTL_MSG_ACK, true);
	if (ret) {
		hwlog_err("wait for ack err\n");
		return -EPERM;
	}

end:
	hwufcs_set_msg_number(++msg_number);
	return 0;
}

int hwufcs_request_communication(bool flag)
{
	int ret = 0;
	int i;

	for (i = 0; i < HWUFCS_REQUEST_COMMUNICATION_RETRY; i++) {
		ret = hwufcs_set_communicating_flag(flag);
		if (!ret)
			break;

		power_usleep(DT_USLEEP_10MS);
	}

	hwlog_info("request communication finish, ret=%d\n", ret);
	return ret;
}

/* data message from sink to source: 0x02 request */
int hwufcs_send_request_data_msg(struct hwufcs_request_data *p)
{
	u64 data = 0;
	u64 tmp_data;
	u8 len = sizeof(u64);

	data |= ((u64)((p->output_curr / HWUFCS_REQ_UNIT_OUTPUT_CURR) &
		HWUFCS_REQ_MASK_OUTPUT_CURR) << HWUFCS_REQ_SHIFT_OUTPUT_CURR);
	data |= ((u64)((p->output_volt / HWUFCS_REQ_UNIT_OUTPUT_VOLT) &
		HWUFCS_REQ_MASK_OUTPUT_VOLT) << HWUFCS_REQ_SHIFT_OUTPUT_VOLT);
	data |= ((u64)(p->output_mode & HWUFCS_REQ_MASK_OUTPUT_MODE) <<
		HWUFCS_REQ_SHIFT_OUTPUT_MODE);
	tmp_data = be64_to_cpu(data);

	return hwufcs_send_data_msg(HWUFCS_DATA_MSG_REQUEST,
		(u8 *)&tmp_data, len, true);
}

/* data message from sink to source: 0x06 device_information */
int hwufcs_send_device_information_data_msg(struct hwufcs_dev_info_data *p)
{
	u64 data = 0;
	u64 tmp_data;
	u8 len = sizeof(u64);

	data |= ((u64)(p->sw_ver & HWUFCS_DEV_INFO_MASK_SW_VER) <<
		HWUFCS_DEV_INFO_SHIFT_SW_VER);
	data |= ((u64)(p->hw_ver & HWUFCS_DEV_INFO_MASK_HW_VER) <<
		HWUFCS_DEV_INFO_SHIFT_HW_VER);
	data |= ((u64)(p->chip_vid & HWUFCS_DEV_INFO_MASK_CHIP_VID) <<
		HWUFCS_DEV_INFO_SHIFT_CHIP_VID);
	data |= ((u64)(p->manu_vid & HWUFCS_DEV_INFO_MASK_MANU_VID) <<
		HWUFCS_DEV_INFO_SHIFT_MANU_VID);
	tmp_data = be64_to_cpu(data);

	return hwufcs_send_data_msg(HWUFCS_DATA_MSG_DEVICE_INFO,
		(u8 *)&tmp_data, len, true);
}

#ifdef POWER_MODULE_DEBUG_FUNCTION
/* data message from sink to source: 0x04 sink_information */
static int hwufcs_send_sink_information_data_msg(struct hwufcs_sink_info_data *p)
{
	u64 data = 0;
	u64 tmp_data;
	u8 len = sizeof(u64);

	data |= ((u64)((p->bat_curr / HWUFCS_SINK_INFO_UNIT_CURR) &
		HWUFCS_SINK_INFO_MASK_BAT_CURR) << HWUFCS_SINK_INFO_SHIFT_BAT_CURR);
	data |= ((u64)((p->bat_volt / HWUFCS_SINK_INFO_UNIT_VOLT) &
		HWUFCS_SINK_INFO_MASK_BAT_VOLT) << HWUFCS_SINK_INFO_SHIFT_BAT_VOLT);
	data |= ((u64)((p->usb_temp + HWUFCS_SINK_INFO_TEMP_BASE) &
		HWUFCS_SINK_INFO_MASK_USB_TEMP) << HWUFCS_SINK_INFO_SHIFT_USB_TEMP);
	data |= ((u64)((p->bat_temp + HWUFCS_SINK_INFO_TEMP_BASE) &
		HWUFCS_SINK_INFO_MASK_BAT_TEMP) << HWUFCS_SINK_INFO_SHIFT_BAT_TEMP);
	tmp_data = be64_to_cpu(data);

	return hwufcs_send_data_msg(HWUFCS_DATA_MSG_SINK_INFO,
		(u8 *)&tmp_data, len, true);
}

/* data message from sink to source: 0x05 cable_information */
static int hwufcs_send_cable_information_data_msg(struct hwufcs_cable_info_data *p)
{
	u64 data = 0;
	u64 tmp_data;
	u8 len = sizeof(u64);

	data |= ((u64)((p->max_curr / HWUFCS_CABLE_INFO_UNIT_CURR) &
		HWUFCS_CABLE_INFO_MASK_MAX_CURR) << HWUFCS_CABLE_INFO_SHIFT_MAX_CURR);
	data |= ((u64)((p->max_volt / HWUFCS_CABLE_INFO_UNIT_VOLT) &
		HWUFCS_CABLE_INFO_MASK_MAX_VOLT) << HWUFCS_CABLE_INFO_SHIFT_MAX_VOLT);
	data |= ((p->resistance & HWUFCS_CABLE_INFO_MASK_RESISTANCE) <<
		HWUFCS_CABLE_INFO_SHIFT_RESISTANCE);
	data |= ((u64)(p->elable_vid & HWUFCS_CABLE_INFO_MASK_ELABLE_VID) <<
		HWUFCS_CABLE_INFO_SHIFT_ELABLE_VID);
	data |= ((u64)(p->vid & HWUFCS_CABLE_INFO_MASK_VID) <<
		HWUFCS_CABLE_INFO_SHIFT_VID);
	tmp_data = be64_to_cpu(data);

	return hwufcs_send_data_msg(HWUFCS_DATA_MSG_CABLE_INFO,
		(u8 *)&tmp_data, len, true);
}

/* data message from sink to source: 0x07 error_information */
static int hwufcs_send_error_information_data_msg(struct hwufcs_error_info_data *p)
{
	u32 data = 0;
	u32 tmp_data;
	u8 len = sizeof(u32);

	data |= ((u32)(p->output_ovp & HWUFCS_ERROR_INFO_MASK_OUTPUT_OVP) <<
		HWUFCS_ERROR_INFO_SHIFT_OUTPUT_OVP);
	data |= ((u32)(p->output_uvp & HWUFCS_ERROR_INFO_MASK_OUTPUT_UVP) <<
		HWUFCS_ERROR_INFO_SHIFT_OUTPUT_UVP);
	data |= ((u32)(p->output_ocp & HWUFCS_ERROR_INFO_MASK_OUTPUT_OCP) <<
		HWUFCS_ERROR_INFO_SHIFT_OUTPUT_OCP);
	data |= ((u32)(p->output_scp & HWUFCS_ERROR_INFO_MASK_OUTPUT_SCP) <<
		HWUFCS_ERROR_INFO_SHIFT_OUTPUT_SCP);
	data |= ((u32)(p->usb_otp & HWUFCS_ERROR_INFO_MASK_USB_OTP) <<
		HWUFCS_ERROR_INFO_SHIFT_USB_OTP);
	data |= ((u32)(p->device_otp & HWUFCS_ERROR_INFO_MASK_DEVICE_OTP) <<
		HWUFCS_ERROR_INFO_SHIFT_DEVICE_OTP);
	data |= ((u32)(p->cc_ovp & HWUFCS_ERROR_INFO_MASK_CC_OVP) <<
		HWUFCS_ERROR_INFO_SHIFT_CC_OVP);
	data |= ((u32)(p->dminus_ovp & HWUFCS_ERROR_INFO_MASK_DMINUS_OVP) <<
		HWUFCS_ERROR_INFO_SHIFT_DMINUS_OVP);
	data |= ((u32)(p->dplus_ovp & HWUFCS_ERROR_INFO_MASK_DPLUS_OVP) <<
		HWUFCS_ERROR_INFO_SHIFT_DPLUS_OVP);
	data |= ((u32)(p->input_ovp & HWUFCS_ERROR_INFO_MASK_INPUT_OVP) <<
		HWUFCS_ERROR_INFO_SHIFT_INPUT_OVP);
	data |= ((u32)(p->input_uvp & HWUFCS_ERROR_INFO_MASK_INPUT_UVP) <<
		HWUFCS_ERROR_INFO_SHIFT_INPUT_UVP);
	data |= ((u32)(p->over_leakage & HWUFCS_ERROR_INFO_MASK_OVER_LEAKAGE) <<
		HWUFCS_ERROR_INFO_SHIFT_OVER_LEAKAGE);
	data |= ((u32)(p->input_drop & HWUFCS_ERROR_INFO_MASK_INPUT_DROP) <<
		HWUFCS_ERROR_INFO_SHIFT_INPUT_DROP);
	data |= ((u32)(p->crc_error & HWUFCS_ERROR_INFO_MASK_CRC_ERROR) <<
		HWUFCS_ERROR_INFO_SHIFT_CRC_ERROR);
	data |= ((u32)(p->wtg_overflow & HWUFCS_ERROR_INFO_MASK_WTG_OVERFLOW) <<
		HWUFCS_ERROR_INFO_SHIFT_WTG_OVERFLOW);
	tmp_data = be32_to_cpu(data);

	return hwufcs_send_data_msg(HWUFCS_DATA_MSG_ERROR_INFO,
		(u8 *)&tmp_data, len, true);
}
#endif /* POWER_MODULE_DEBUG_FUNCTION */

/* data message from sink to source: 0x08 config_watchdog */
int hwufcs_send_config_watchdog_data_msg(struct hwufcs_wtg_data *p)
{
	u16 data = 0;
	u16 tmp_data;
	u8 len = sizeof(u16);

	data |= ((u16)(p->time & HWUFCS_WTG_MASK_TIME) <<
		HWUFCS_WTG_SHIFT_TIME);
	tmp_data = be16_to_cpu(data);

	return hwufcs_send_data_msg(HWUFCS_DATA_MSG_CONFIG_WATCHDOG,
		(u8 *)&tmp_data, len, true);
}

/* data message from sink to source: 0x0a verify_request */
int hwufcs_send_verify_request_data_msg(struct hwufcs_verify_request_data *p)
{
	u8 len = sizeof(struct hwufcs_verify_request_data);

	return hwufcs_send_data_msg(HWUFCS_DATA_MSG_VERIFY_REQUEST,
		(u8 *)p, len, true);
}

#ifdef POWER_MODULE_DEBUG_FUNCTION
/* data message from sink to source: 0x09 refuse */
static int hwufcs_send_refuse_data_msg(struct hwufcs_refuse_data *p)
{
	u32 data = 0;
	u32 tmp_data;
	u8 len = sizeof(u32);

	data |= ((u32)(p->reason & HWUFCS_REFUSE_MASK_REASON) <<
		HWUFCS_REFUSE_SHIFT_REASON);
	data |= ((u32)(p->cmd_number & HWUFCS_REFUSE_MASK_CMD_NUMBER) <<
		HWUFCS_REFUSE_SHIFT_CMD_NUMBER);
	data |= ((u32)(p->msg_type & HWUFCS_REFUSE_MASK_MSG_TYPE) <<
		HWUFCS_REFUSE_SHIFT_MSG_TYPE);
	data |= ((u32)(p->msg_number & HWUFCS_REFUSE_MASK_MSG_NUMBER) <<
		HWUFCS_REFUSE_SHIFT_MSG_NUMBER);
	tmp_data = be32_to_cpu(data);

	return hwufcs_send_data_msg(HWUFCS_DATA_MSG_REFUSE,
		(u8 *)&tmp_data, len, true);
}

/* data message from sink to source: 0x0b verify_response */
static int hwufcs_send_verify_response_data_msg(struct hwufcs_verify_response_data *p)
{
	u8 len = sizeof(struct hwufcs_verify_response_data);

	return hwufcs_send_data_msg(HWUFCS_DATA_MSG_VERIFY_RESPONSE,
		(u8 *)p, len, true);
}

/* data message from sink to source: 0xff test request */
static int hwufcs_send_test_request_data_msg(struct hwufcs_test_request_data *p)
{
	u16 data = 0;
	u16 tmp_data;
	u8 len = sizeof(u16);

	data |= ((u16)(p->msg_number & HWUFCS_TEST_REQ_MASK_MSG_NUMBER) <<
		HWUFCS_TEST_REQ_SHIFT_MSG_NUMBER);
	data |= ((u16)(p->msg_type & HWUFCS_TEST_REQ_MASK_MSG_TYPE) <<
		HWUFCS_TEST_REQ_SHIFT_MSG_TYPE);
	data |= ((u16)(p->dev_address & HWUFCS_TEST_REQ_MASK_DEV_ADDRESS) <<
		HWUFCS_TEST_REQ_SHIFT_DEV_ADDRESS);
	data |= ((u16)(p->volt_test_mode & HWUFCS_TEST_REQ_MASK_VOLT_TEST_MODE) <<
		HWUFCS_TEST_REQ_SHIFT_VOLT_TEST_MODE);
	tmp_data = be16_to_cpu(data);

	return hwufcs_send_data_msg(HWUFCS_DATA_MSG_TEST_REQUEST,
		(u8 *)&tmp_data, len, true);
}
#endif /* POWER_MODULE_DEBUG_FUNCTION */

/* data message from source to sink: 0x01 output_capabilities */
int hwufcs_receive_output_capabilities_data_msg(struct hwufcs_capabilities_data *p,
	u8 *ret_len)
{
	u64 data[HWUFCS_CAP_MAX_OUTPUT_MODE] = { 0 };
	u64 tmp_data;
	u8 len = HWUFCS_CAP_MAX_OUTPUT_MODE * sizeof(u64);
	u8 tmp_len = 0;
	int ret;
	u8 i;

	ret = hwufcs_receive_data_msg(HWUFCS_DATA_MSG_OUTPUT_CAPABILITIES,
		(u8 *)&data, len, &tmp_len);
	if (ret)
		return -EPERM;

	if (tmp_len % sizeof(u64) != 0) {
		hwlog_err("output_capabilities length=%u,%u invalid\n", len, tmp_len);
		return -EPERM;
	}

	*ret_len = tmp_len / sizeof(u64);
	for (i = 0; i < *ret_len; i++) {
		tmp_data = cpu_to_be64(data[i]);
		/* min output current */
		p[i].min_curr = (tmp_data >> HWUFCS_CAP_SHIFT_MIN_CURR) &
			HWUFCS_CAP_MASK_MIN_CURR;
		p[i].min_curr *= HWUFCS_CAP_UNIT_CURR;
		/* max output current */
		p[i].max_curr = (tmp_data >> HWUFCS_CAP_SHIFT_MAX_CURR) &
			HWUFCS_CAP_MASK_MAX_CURR;
		p[i].max_curr *= HWUFCS_CAP_UNIT_CURR;
		/* min output voltage */
		p[i].min_volt = (tmp_data >> HWUFCS_CAP_SHIFT_MIN_VOLT) &
			HWUFCS_CAP_MASK_MIN_VOLT;
		p[i].min_volt *= HWUFCS_CAP_UNIT_VOLT;
		/* max output voltage */
		p[i].max_volt = (tmp_data >> HWUFCS_CAP_SHIFT_MAX_VOLT) &
			HWUFCS_CAP_MASK_MAX_VOLT;
		p[i].max_volt *= HWUFCS_CAP_UNIT_VOLT;
		/* voltage adjust step */
		p[i].volt_step = (tmp_data >> HWUFCS_CAP_SHIFT_VOLT_STEP) &
			HWUFCS_CAP_MASK_VOLT_STEP;
		p[i].volt_step++;
		p[i].volt_step *= HWUFCS_CAP_UNIT_VOLT;
		/* current adjust step */
		p[i].curr_step = (tmp_data >> HWUFCS_CAP_SHIFT_CURR_STEP) &
			HWUFCS_CAP_MASK_CURR_STEP;
		p[i].curr_step++;
		p[i].curr_step *= HWUFCS_CAP_UNIT_CURR;
		/* output mode */
		p[i].output_mode = (tmp_data >> HWUFCS_CAP_SHIFT_OUTPUT_MODE) &
			HWUFCS_CAP_MASK_OUTPUT_MODE;
	}

	for (i = 0; i < *ret_len; i++)
		hwlog_info("cap[%u]=%lx %umA %umA %umV %umV %umV %umA %u\n", i, tmp_data,
			p[i].min_curr, p[i].max_curr,
			p[i].min_volt, p[i].max_volt,
			p[i].volt_step, p[i].curr_step,
			p[i].output_mode);
	return 0;
}

/* data message from source to sink: 0x03 source_information */
int hwufcs_receive_source_info_data_msg(struct hwufcs_source_info_data *p)
{
	u64 data = 0;
	u64 tmp_data;
	u8 len = sizeof(u64);
	u8 tmp_len = 0;
	int ret;

	ret = hwufcs_receive_data_msg(HWUFCS_DATA_MSG_SOURCE_INFO,
		(u8 *)&data, len, &tmp_len);
	if (ret)
		return -EPERM;

	if (len != tmp_len) {
		hwlog_err("source_info length=%u,%u invalid\n", len, tmp_len);
		return -EPERM;
	}

	tmp_data = cpu_to_be64(data);
	/* current output current */
	p->output_curr = (tmp_data >> HWUFCS_SOURCE_INFO_SHIFT_OUTPUT_CURR) &
		HWUFCS_SOURCE_INFO_MASK_OUTPUT_CURR;
	p->output_curr *= HWUFCS_SOURCE_INFO_UNIT_CURR;
	/* current output voltage */
	p->output_volt = (tmp_data >> HWUFCS_SOURCE_INFO_SHIFT_OUTPUT_VOLT) &
		HWUFCS_SOURCE_INFO_MASK_OUTPUT_VOLT;
	p->output_volt *= HWUFCS_SOURCE_INFO_UNIT_VOLT;
	/* current usb port temp */
	p->port_temp = (tmp_data >> HWUFCS_SOURCE_INFO_SHIFT_PORT_TEMP) &
		HWUFCS_SOURCE_INFO_MASK_PORT_TEMP;
	p->port_temp -= HWUFCS_SOURCE_INFO_TEMP_BASE;
	/* current device temp */
	p->dev_temp = (tmp_data >> HWUFCS_SOURCE_INFO_SHIFT_DEV_TEMP) &
		HWUFCS_SOURCE_INFO_MASK_DEV_TEMP;
	p->dev_temp -= HWUFCS_SOURCE_INFO_TEMP_BASE;

	hwlog_info("source_info=%lx %umA %umV %dC %dC\n", tmp_data,
		p->output_curr, p->output_volt, p->port_temp, p->dev_temp);
	return 0;
}

#ifdef POWER_MODULE_DEBUG_FUNCTION
/* data message from source to sink: 0x05 cable_information */
static int hwufcs_receive_cable_info_data_msg(struct hwufcs_cable_info_data *p)
{
	u64 data = 0;
	u64 tmp_data;
	u8 len = sizeof(u64);
	u8 tmp_len = 0;
	int ret;

	ret = hwufcs_receive_data_msg(HWUFCS_DATA_MSG_CABLE_INFO,
		(u8 *)&data, len, &tmp_len);
	if (ret)
		return -EPERM;

	if (len != tmp_len) {
		hwlog_err("cable_info length=%u,%u invalid\n", len, tmp_len);
		return -EPERM;
	}

	tmp_data = cpu_to_be64(data);
	/* max loading current */
	p->max_curr = (tmp_data >> HWUFCS_CABLE_INFO_SHIFT_MAX_CURR) &
		HWUFCS_CABLE_INFO_MASK_MAX_CURR;
	p->max_curr *= HWUFCS_CABLE_INFO_UNIT_CURR;
	/* max loading voltage */
	p->max_volt = (tmp_data >> HWUFCS_CABLE_INFO_SHIFT_MAX_VOLT) &
		HWUFCS_CABLE_INFO_MASK_MAX_VOLT;
	p->max_volt *= HWUFCS_CABLE_INFO_UNIT_VOLT;
	/* cable resistance */
	p->resistance = (tmp_data >> HWUFCS_CABLE_INFO_SHIFT_RESISTANCE) &
		HWUFCS_CABLE_INFO_MASK_RESISTANCE;
	p->resistance *= HWUFCS_CABLE_INFO_UNIT_RESISTANCE;
	/* cable electronic lable vendor id */
	p->elable_vid = (tmp_data >> HWUFCS_CABLE_INFO_SHIFT_ELABLE_VID) &
		HWUFCS_CABLE_INFO_MASK_ELABLE_VID;
	/* cable vendor id */
	p->vid = (tmp_data >> HWUFCS_CABLE_INFO_SHIFT_VID) &
		HWUFCS_CABLE_INFO_MASK_VID;

	hwlog_info("cable_info=%lx %xmA %xmV %xmO %x\n", tmp_data,
		p->max_curr, p->max_volt, p->resistance, p->elable_vid, p->vid);
	return 0;
}
#endif /* POWER_MODULE_DEBUG_FUNCTION */

/* data message from source to sink: 0x06 device_information */
int hwufcs_receive_dev_info_data_msg(struct hwufcs_dev_info_data *p)
{
	u64 data = 0;
	u64 tmp_data;
	u8 len = sizeof(u64);
	u8 tmp_len = 0;
	int ret;

	ret = hwufcs_receive_data_msg(HWUFCS_DATA_MSG_DEVICE_INFO,
		(u8 *)&data, len, &tmp_len);
	if (ret)
		return -EPERM;

	if (len != tmp_len) {
		hwlog_err("dev_info length=%u,%u invalid\n", len, tmp_len);
		return -EPERM;
	}

	tmp_data = cpu_to_be64(data);
	/* software version */
	p->sw_ver = (tmp_data >> HWUFCS_DEV_INFO_SHIFT_SW_VER) &
		HWUFCS_DEV_INFO_MASK_SW_VER;
	/* hardware version */
	p->hw_ver = (tmp_data >> HWUFCS_DEV_INFO_SHIFT_HW_VER) &
		HWUFCS_DEV_INFO_MASK_HW_VER;
	/* protocol chip vendor id */
	p->chip_vid = (tmp_data >> HWUFCS_DEV_INFO_SHIFT_CHIP_VID) &
		HWUFCS_DEV_INFO_MASK_CHIP_VID;
	/* manufacture vendor id */
	p->manu_vid = (tmp_data >> HWUFCS_DEV_INFO_SHIFT_MANU_VID) &
		HWUFCS_DEV_INFO_MASK_MANU_VID;

	hwlog_info("dev_info=%lx %x %x %x %x\n", tmp_data,
		p->sw_ver, p->hw_ver, p->chip_vid, p->manu_vid);
	return 0;
}

#ifdef POWER_MODULE_DEBUG_FUNCTION
/* data message from source to sink: 0x07 error_information */
static int hwufcs_receive_error_info_data_msg(struct hwufcs_error_info_data *p)
{
	u64 data = 0;
	u64 tmp_data;
	u8 len = sizeof(u64);
	u8 tmp_len = 0;
	int ret;

	ret = hwufcs_receive_data_msg(HWUFCS_DATA_MSG_ERROR_INFO,
		(u8 *)&data, len, &tmp_len);
	if (ret)
		return -EPERM;

	if (len != tmp_len) {
		hwlog_err("error_info length=%u,%u invalid\n", len, tmp_len);
		return -EPERM;
	}

	tmp_data = cpu_to_be64(data);
	/* output ovp */
	p->output_ovp = (tmp_data >> HWUFCS_ERROR_INFO_SHIFT_OUTPUT_OVP) &
		HWUFCS_ERROR_INFO_MASK_OUTPUT_OVP;
	/* output uvp */
	p->output_uvp = (tmp_data >> HWUFCS_ERROR_INFO_SHIFT_OUTPUT_UVP) &
		HWUFCS_ERROR_INFO_MASK_OUTPUT_UVP;
	/* output ocp */
	p->output_ocp = (tmp_data >> HWUFCS_ERROR_INFO_SHIFT_OUTPUT_OCP) &
		HWUFCS_ERROR_INFO_MASK_OUTPUT_OCP;
	/* output scp */
	p->output_scp = (tmp_data >> HWUFCS_ERROR_INFO_SHIFT_OUTPUT_SCP) &
		HWUFCS_ERROR_INFO_MASK_OUTPUT_SCP;
	/* usb otp */
	p->usb_otp = (tmp_data >> HWUFCS_ERROR_INFO_SHIFT_USB_OTP) &
		HWUFCS_ERROR_INFO_MASK_USB_OTP;
	/* device otp */
	p->device_otp = (tmp_data >> HWUFCS_ERROR_INFO_SHIFT_DEVICE_OTP) &
		HWUFCS_ERROR_INFO_MASK_DEVICE_OTP;
	/* cc ovp */
	p->cc_ovp = (tmp_data >> HWUFCS_ERROR_INFO_SHIFT_CC_OVP) &
		HWUFCS_ERROR_INFO_MASK_CC_OVP;
	/* d- ovp */
	p->dminus_ovp = (tmp_data >> HWUFCS_ERROR_INFO_SHIFT_DMINUS_OVP) &
		HWUFCS_ERROR_INFO_MASK_DMINUS_OVP;
	/* d+ ovp */
	p->dplus_ovp = (tmp_data >> HWUFCS_ERROR_INFO_SHIFT_DPLUS_OVP) &
		HWUFCS_ERROR_INFO_MASK_DPLUS_OVP;
	/* input ovp */
	p->input_ovp = (tmp_data >> HWUFCS_ERROR_INFO_SHIFT_INPUT_OVP) &
		HWUFCS_ERROR_INFO_MASK_INPUT_OVP;
	/* input uvp */
	p->input_uvp = (tmp_data >> HWUFCS_ERROR_INFO_SHIFT_INPUT_UVP) &
		HWUFCS_ERROR_INFO_MASK_INPUT_UVP;
	/* over leakage current */
	p->over_leakage = (tmp_data >> HWUFCS_ERROR_INFO_SHIFT_OVER_LEAKAGE) &
		HWUFCS_ERROR_INFO_MASK_OVER_LEAKAGE;
	/* input drop */
	p->input_drop = (tmp_data >> HWUFCS_ERROR_INFO_SHIFT_INPUT_DROP) &
		HWUFCS_ERROR_INFO_MASK_INPUT_DROP;
	/* crc error */
	p->crc_error = (tmp_data >> HWUFCS_ERROR_INFO_SHIFT_CRC_ERROR) &
		HWUFCS_ERROR_INFO_MASK_CRC_ERROR;
	/* watchdog overflow */
	p->wtg_overflow = (tmp_data >> HWUFCS_ERROR_INFO_SHIFT_WTG_OVERFLOW) &
		HWUFCS_ERROR_INFO_MASK_WTG_OVERFLOW;

	hwlog_info("error_info=%lx\n", tmp_data);
	return 0;
}
#endif /* POWER_MODULE_DEBUG_FUNCTION */

/* data message from source to sink: 0x0b verify_response */
int hwufcs_receive_verify_response_data_msg(struct hwufcs_verify_response_data *p)
{
	u8 len = HWUFCS_VERIFY_RSP_ENCRYPT_SIZE + HWUFCS_VERIFY_RSP_RANDOM_SIZE;
	u8 tmp_len = 0;
	int ret;

	ret = hwufcs_receive_data_msg(HWUFCS_DATA_MSG_VERIFY_RESPONSE,
		(u8 *)p, len, &tmp_len);
	if (ret)
		return -EPERM;

	if (len != tmp_len) {
		hwlog_err("verify_response length=%u,%u invalid\n", len, tmp_len);
		return -EPERM;
	}

	return 0;
}
