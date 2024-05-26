// SPDX-License-Identifier: GPL-2.0
/*
 * sc8546_ufcs.c
 *
 * sc8546 ufcs driver
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

#include "sc8546_ufcs.h"
#include "sc8546_i2c.h"
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_event_ne.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/protocol/adapter_protocol.h>
#include <chipset_common/hwpower/protocol/adapter_protocol_ufcs.h>

#define HWLOG_TAG sc8546_ufcs
HWLOG_REGIST();

struct sc8546_ufcs_msg_head *g_sc8546_ufcs_msg_head;

int sc8546_ufcs_init_msg_head(struct sc8546_device_info *di)
{
	struct sc8546_ufcs_msg_head *head = NULL;

	if (!di || g_sc8546_ufcs_msg_head)
		return 0;

	mutex_lock(&di->ufcs_node_lock);
	head = kzalloc(sizeof(struct sc8546_ufcs_msg_head), GFP_KERNEL);
	if (!head) {
		mutex_unlock(&di->ufcs_node_lock);
		return -ENOMEM;
	}

	head->num = 0;
	head->next = NULL;
	g_sc8546_ufcs_msg_head = head;
	mutex_unlock(&di->ufcs_node_lock);
	return 0;
}

static struct sc8546_ufcs_msg_node *sc8546_ufcs_create_node(const u8 *data, int len)
{
	struct sc8546_ufcs_msg_node *node = NULL;

	hwlog_info("create node\n");
	if (!data) {
		hwlog_err("data is null\n");
		return NULL;
	}

	node = kzalloc(sizeof(struct sc8546_ufcs_msg_node), GFP_KERNEL);
	if (!node) {
		hwlog_err("create node fail\n");
		return NULL;
	}

	node->len = len;
	memcpy(node->data, &data[0], len);
	return node;
}

static void sc8546_ufcs_add_node(struct sc8546_device_info *di,
	struct sc8546_ufcs_msg_node *node)
{
	struct sc8546_ufcs_msg_head *head = g_sc8546_ufcs_msg_head;
	struct sc8546_ufcs_msg_node *check_node = NULL;

	mutex_lock(&di->ufcs_node_lock);
	if (!head || !node) {
		hwlog_err("msg head or node is null\n");
 		mutex_unlock(&di->ufcs_node_lock);
		return;
	}

	hwlog_info("add msg\n");
	if (!head->next) {
		head->next = node;
		goto end;
	}

	check_node = head->next;
	while (check_node->next)
		check_node = check_node->next;
	check_node->next = node;

end:
	head->num++;
	hwlog_info("current msg num=%d\n", head->num);
	mutex_unlock(&di->ufcs_node_lock);
}

static void sc8546_ufcs_delete_node(struct sc8546_device_info *di)
{
	struct sc8546_ufcs_msg_head *head = g_sc8546_ufcs_msg_head;
	struct sc8546_ufcs_msg_node *check_node = NULL;

	mutex_lock(&di->ufcs_node_lock);
	if (!head || !head->next) {
		hwlog_err("msg head or node is null\n");
 		mutex_unlock(&di->ufcs_node_lock);
		return;
	}

	hwlog_info("delete msg\n");
	check_node = head->next;
	head->next = head->next->next;
	kfree(check_node);
	head->num--;
	hwlog_info("current msg num=%d\n", head->num);
	mutex_unlock(&di->ufcs_node_lock);
}

void sc8546_ufcs_free_node_list(struct sc8546_device_info *di)
{
	struct sc8546_ufcs_msg_head *head = g_sc8546_ufcs_msg_head;
	struct sc8546_ufcs_msg_node *free_node = NULL;
	struct sc8546_ufcs_msg_node *temp = NULL;

	if (!di || !head) {
		hwlog_err("di or msg head is null\n");
		return;
	}

	mutex_lock(&di->ufcs_node_lock);
	hwlog_info("free msg\n");
	free_node = head->next;
	while (free_node) {
		temp = free_node->next;
		kfree(free_node);
		free_node = temp;
	}

	kfree(head);
	g_sc8546_ufcs_msg_head = NULL;
	mutex_unlock(&di->ufcs_node_lock);
}

void sc8546_ufcs_add_msg(struct sc8546_device_info *di)
{
	u8 len = 0;
	int ret;
	u8 data[SC8546_UFCS_RX_BUF_SIZE];
	struct sc8546_ufcs_msg_node *node = NULL;

	if (!di)
		return;

	ret = sc8546_read_byte(di, SC8546_UFCS_RX_LENGTH_REG, &len);
	if (ret)
		return;

	if (!len || len > SC8546_UFCS_RX_BUF_SIZE) {
		hwlog_err("length is %d, invalid\n");
		return;
	}

	ret = sc8546_read_block(di, SC8546_UFCS_RX_BUFFER_REG, data, len);
	if (ret)
		return;

	node = sc8546_ufcs_create_node(data, len);
	if (!node)
		return;

	sc8546_ufcs_add_node(di, node);
}

static struct sc8546_ufcs_msg_node *sc8546_ufcs_get_msg(struct sc8546_device_info *di)
{
	struct sc8546_ufcs_msg_head *head = g_sc8546_ufcs_msg_head;

	if (!head || !head->next) {
		hwlog_err("msg head or node is null\n");
 		return NULL;
	}

	return head->next;
}

static int sc8546_ufcs_wait(struct sc8546_device_info *di, u8 flag)
{
	u8 reg_val1;
	u8 reg_val2;
	int i;

	for (i = 0; i < SC8546_UFCS_WAIT_RETRY_CYCLE; i++) {
		power_usleep(DT_USLEEP_1MS);

		reg_val1 = di->ufcs_irq[0];
		reg_val2 = di->ufcs_irq[1];
		hwlog_info("ufcs_isr1=0x%x, ufcs_isr2=0x%x\n", reg_val1, reg_val2);

		/* isr1[4] must be 0 */
		if ((flag & HWUFCS_WAIT_CRC_ERROR) &&
			(reg_val1 & SC8546_UFCS_ISR1_CRC_ERROR_MASK)) {
			hwlog_err("crc error\n");
			break;
		}

		/* isr1[3] must be 1 */
		if ((flag & HWUFCS_WAIT_SEND_PACKET_COMPLETE) &&
			!(reg_val1 & SC8546_UFCS_ISR1_SEND_PACKET_COMPLETE_MASK)) {
			hwlog_err("sent packet not complete or fail\n");
			continue;
		}

		/* isr2[1] must be 0 */
		if ((flag & HWUFCS_ACK_RECEIVE_TIMEOUT) &&
			(reg_val2 & SC8546_UFCS_ISR2_MSG_TRANS_FAIL_MASK)) {
			hwlog_err("not receive ack after retry\n");
			break;
		}

		/* isr1[2] must be 1 */
		if ((flag & HWUFCS_WAIT_DATA_READY) &&
			!(reg_val1 & SC8546_UFCS_ISR1_DATA_READY_MASK)) {
			hwlog_err("receive data not ready\n");
			continue;
		}

		hwlog_info("wait succ\n");
		return 0;
	}

	hwlog_err("wait fail\n");
	return -EINVAL;
}

static void sc8546_ufcs_handshake_preparation(struct sc8546_device_info *di)
{
	sc8546_write_byte(di, SC8546_COMPATY_PROTOCOL_REG,
		SC8546_UFCS_PROTOCOL_VAL);

	/* disable scp&ufcs */
	sc8546_write_mask(di, SC8546_SCP_CTRL_REG, SC8546_SCP_EN_MASK,
		SC8546_SCP_EN_SHIFT, 0);
	sc8546_write_byte(di, SC8546_UFCS_CTL1_REG, 0);

	/* enable DPDM */
	sc8546_write_mask(di, SC8546_DPDM_CTRL1_REG,
		SC8546_DPDM_EN_MASK, SC8546_DPDM_EN_SHIFT, 1);
	/* enable ufcs */
	sc8546_write_mask(di, SC8546_UFCS_CTL1_REG,
		SC8546_UFCS_CTL1_EN_PROTOCOL_MASK,
		SC8546_UFCS_CTL1_EN_PROTOCOL_SHIFT, 1);

	sc8546_write_mask(di, SC8546_UFCS_CTL2_REG,
		SC8546_UFCS_CTL2_ACK_NOT_BLOCK_MSG_MASK,
		SC8546_UFCS_CTL2_ACK_NOT_BLOCK_MSG_SHIFT, 1);
	sc8546_write_mask(di, SC8546_UFCS_CTL2_REG,
		SC8546_UFCS_CTL2_MSG_NOT_BLOCK_ACK_MASK,
		SC8546_UFCS_CTL2_MSG_NOT_BLOCK_ACK_SHIFT, 1);
	sc8546_write_mask(di, SC8546_UFCS_CTL2_REG,
		SC8546_UFCS_CTL2_LATE_RX_BUFFER_BUSY_MASK,
		SC8546_UFCS_CTL2_LATE_RX_BUFFER_BUSY_SHIFT, 1);

	/* hidden register, disable the 600 μs delay detection between data frames */
	sc8546_write_byte(di, 0xEA, 0x04);
	sc8546_write_byte(di, 0xED, 0x04);
	/* enable handshake */
	sc8546_write_mask(di, SC8546_UFCS_CTL1_REG,
		SC8546_UFCS_CTL1_EN_HANDSHAKE_MASK,
		SC8546_UFCS_CTL1_EN_HANDSHAKE_SHIFT, 1);
}

static int sc8546_ufcs_detect_adapter(void *dev_data)
{
	struct sc8546_device_info *di = dev_data;
	u8 reg_val = 0;
	int ret, i;

	if (!di) {
		hwlog_err("di is null\n");
		return HWUFCS_DETECT_OTHER;
	}

	di->ufcs_irq[0] = 0;
	di->ufcs_irq[1] = 0;
	mutex_lock(&di->ufcs_detect_lock);

	sc8546_ufcs_handshake_preparation(di);
	(void)power_usleep(DT_USLEEP_20MS);
	/* waiting for handshake */
	for (i = 0; i < SC8546_UFCS_HANDSHARK_RETRY_CYCLE; i++) {
		(void)power_usleep(DT_USLEEP_2MS);
		ret = sc8546_read_byte(di, SC8546_UFCS_ISR1_REG, &reg_val);
		if (ret) {
			hwlog_err("read isr reg[%x] fail\n", SC8546_UFCS_ISR1_REG);
			continue;
		}

		reg_val |= di->ufcs_irq[0];
		hwlog_info("ufcs_isr1=0x%x\n", reg_val);
		if (reg_val & SC8546_UFCS_ISR1_HANDSHAKE_FAIL_MASK) {
			i = SC8546_UFCS_HANDSHARK_RETRY_CYCLE;
			break;
		}

		if (reg_val & SC8546_UFCS_ISR1_HANDSHAKE_SUCC_MASK)
			break;
	}

	mutex_unlock(&di->ufcs_detect_lock);

	if (i == SC8546_UFCS_HANDSHARK_RETRY_CYCLE) {
		hwlog_err("handshake fail\n");
		return HWUFCS_DETECT_FAIL;
	}

	(void)sc8546_write_mask(di, SC8546_UFCS_CTL2_REG,
		SC8546_UFCS_CTL2_DEV_ADDR_ID_MASK,
		SC8546_UFCS_CTL2_DEV_ADDR_ID_SHIFT, SC8546_UFCS_CTL2_SOURCE_ADDR);
	hwlog_info("handshake succ\n");
	return HWUFCS_DETECT_SUCC;
}

static int sc8546_ufcs_write_msg(void *dev_data, u8 *data, u8 len, u8 flag)
{
	struct sc8546_device_info *di = dev_data;
	int ret;

	if (!di || !data) {
		hwlog_err("di or data is null\n");
		return -ENODEV;
	}

	if (len > SC8546_UFCS_TX_BUF_WITHOUTHEAD_SIZE) {
		hwlog_err("invalid length=%u\n", len);
		return -EINVAL;
	}

	di->ufcs_irq[0] = 0;
	di->ufcs_irq[1] = 0;
	ret = sc8546_write_byte(di, SC8546_UFCS_TX_LENGTH_REG, len);
	ret += sc8546_write_block(di, SC8546_UFCS_TX_BUFFER_REG,
		data, len);
	ret += sc8546_write_mask(di, SC8546_UFCS_CTL1_REG,
		SC8546_UFCS_CTL1_SEND_MASK, SC8546_UFCS_CTL1_SEND_SHIFT, 1);
	ret += sc8546_ufcs_wait(di, flag);
	return ret;
}

static int sc8546_ufcs_wait_msg_ready(void *dev_data, u8 flag)
{
	struct sc8546_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -ENODEV;
	}

	return sc8546_ufcs_wait(di, flag);
}

static int sc8546_ufcs_get_rx_len(void *dev_data, u8 *len)
{
	struct sc8546_device_info *di = dev_data;
	struct sc8546_ufcs_msg_node *msg = sc8546_ufcs_get_msg(di);

	if (!di || !msg) {
		hwlog_err("di or msg is null\n");
		*len = 0;
		return -ENODEV;
	}

	*len = msg->len;
	return 0;
}

static int sc8546_ufcs_read_msg(void *dev_data, u8 *data, u8 len)
{
	struct sc8546_device_info *di = dev_data;
	struct sc8546_ufcs_msg_node *msg = sc8546_ufcs_get_msg(di);

	if (!di || !data || !msg) {
		hwlog_err("di or data or msg is null\n");
		return -ENODEV;
	}

	if (len > SC8546_UFCS_RX_BUF_WITHOUTHEAD_SIZE) {
		hwlog_err("invalid length=%u\n", len);
		return -EINVAL;
	}

	memcpy(data, &msg->data[0], len);
	di->ufcs_irq[0] = 0;
	di->ufcs_irq[1] = 0;
	return 0;
}

static int sc8546_ufcs_end_read_msg(void *dev_data)
{
	struct sc8546_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -ENODEV;
	}

	sc8546_ufcs_delete_node(di);
	return 0;
}

static int sc8546_ufcs_soft_reset_master(void *dev_data)
{
	struct sc8546_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -ENODEV;
	}

	di->ufcs_irq[0] = 0;
	di->ufcs_irq[1] = 0;
	return sc8546_write_mask(di, SC8546_UFCS_CTL1_REG,
		SC8546_UFCS_CTL1_EN_PROTOCOL_MASK,
		SC8546_UFCS_CTL1_EN_PROTOCOL_SHIFT, 0);
}

static int sc8546_ufcs_set_communicating_flag(void *dev_data, bool flag)
{
	struct sc8546_device_info *di = dev_data;
	struct sc8546_ufcs_msg_head *head = g_sc8546_ufcs_msg_head;

	if (!di) {
		hwlog_err("di is null\n");
		return -ENODEV;
	}

	if (di->ufcs_communicating_flag && flag) {
		hwlog_info("is communicating, wait\n");
		return -EINVAL;
	}

	if (di->ufcs_communicating_flag && !flag && head->num) {
		hwlog_info("unprocessed information exists,num=%d\n", head->num);
		power_event_bnc_notify(POWER_BNT_UFCS,
			POWER_NE_UFCS_REC_UNSOLICITED_DATA, NULL);
	}

	/* init msg list */
	sc8546_ufcs_free_node_list(di);
	sc8546_ufcs_init_msg_head(di);
	if (!head) {
		hwlog_err("head is null\n");
		return -ENODEV;
	}

	di->ufcs_communicating_flag = flag;
	return 0;
}

static bool sc8546_ufcs_need_check_ack(void *dev_data)
{
	return false;
}

static struct hwufcs_ops sc8546_hwufcs_ops = {
	.chip_name = "sc8546",
	.detect_adapter = sc8546_ufcs_detect_adapter,
	.write_msg = sc8546_ufcs_write_msg,
	.wait_msg_ready = sc8546_ufcs_wait_msg_ready,
	.read_msg = sc8546_ufcs_read_msg,
	.end_read_msg = sc8546_ufcs_end_read_msg,
	.soft_reset_master = sc8546_ufcs_soft_reset_master,
	.get_rx_len = sc8546_ufcs_get_rx_len,
	.set_communicating_flag = sc8546_ufcs_set_communicating_flag,
	.need_check_ack = sc8546_ufcs_need_check_ack,
};

int sc8546_ufcs_ops_register(struct sc8546_device_info *di)
{
	if (di->dts_ufcs_support) {
		sc8546_hwufcs_ops.dev_data = (void *)di;
		return hwufcs_ops_register(&sc8546_hwufcs_ops);
	}

	return 0;
}
