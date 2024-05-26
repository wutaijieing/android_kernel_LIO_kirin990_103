// SPDX-License-Identifier: GPL-2.0
/*
 * cps2021_ufcs.c
 *
 * cps2021 ufcs driver
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

#include "cps2021_ufcs.h"
#include "cps2021_i2c.h"
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_event_ne.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/protocol/adapter_protocol.h>
#include <chipset_common/hwpower/protocol/adapter_protocol_ufcs.h>

#define HWLOG_TAG cps2021_ufcs
HWLOG_REGIST();

struct cps2021_ufcs_msg_head *g_cps2021_ufcs_msg_head;

int cps2021_ufcs_init_msg_head(struct cps2021_device_info *di)
{
	struct cps2021_ufcs_msg_head *head = NULL;

	if (g_cps2021_ufcs_msg_head)
		return 0;

	mutex_lock(&di->ufcs_node_lock);
	head = kzalloc(sizeof(struct cps2021_ufcs_msg_head), GFP_KERNEL);
	if (!head) {
		mutex_unlock(&di->ufcs_node_lock);
		return -ENOMEM;
	}

	head->num = 0;
	head->next = NULL;
	g_cps2021_ufcs_msg_head = head;
	mutex_unlock(&di->ufcs_node_lock);
	return 0;
}

static struct cps2021_ufcs_msg_node *cps2021_ufcs_create_node(u8 irq1,
	u8 irq2, const u8 *data, int len)
{
	struct cps2021_ufcs_msg_node *node = NULL;

	hwlog_info("create node\n");
	if (!data) {
		hwlog_err("data is null\n");
		return NULL;
	}

	node = kzalloc(sizeof(struct cps2021_ufcs_msg_node), GFP_KERNEL);
	if (!node) {
		hwlog_err("create node fail\n");
		return NULL;
	}

	node->irq1 = irq1;
	node->irq2 = irq2;
	node->len = len;
	memcpy(node->data, &data[0], len);
	return node;
}

static void cps2021_ufcs_add_node(struct cps2021_device_info *di,
	struct cps2021_ufcs_msg_node *node)
{
	struct cps2021_ufcs_msg_head *head = g_cps2021_ufcs_msg_head;
	struct cps2021_ufcs_msg_node *check_node = NULL;

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

static void cps2021_ufcs_delete_node(struct cps2021_device_info *di)
{
	struct cps2021_ufcs_msg_head *head = g_cps2021_ufcs_msg_head;
	struct cps2021_ufcs_msg_node *check_node = NULL;

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

void cps2021_ufcs_free_node_list(struct cps2021_device_info *di)
{
	struct cps2021_ufcs_msg_head *head = g_cps2021_ufcs_msg_head;
	struct cps2021_ufcs_msg_node *free_node = NULL;
	struct cps2021_ufcs_msg_node *temp = NULL;

	mutex_lock(&di->ufcs_node_lock);
	if (!head) {
		hwlog_err("msg headis null\n");
		mutex_unlock(&di->ufcs_node_lock);
		return;
	}

	hwlog_info("free msg\n");
	free_node = head->next;
	while (free_node) {
		temp = free_node->next;
		kfree(free_node);
		free_node = temp;
	}

	kfree(head);
	g_cps2021_ufcs_msg_head = NULL;
	mutex_unlock(&di->ufcs_node_lock);
}

static void cps2021_ufcs_add_msg(struct cps2021_device_info *di)
{
	u8 len = 0;
	int ret;
	u8 data[CPS2021_UFCS_RX_BUF_SIZE];
	struct cps2021_ufcs_msg_node *node = NULL;
	u8 irq1 = di->ufcs_irq[0];
	u8 irq2 = di->ufcs_irq[1];

	ret = cps2021_read_byte(di, CPS2021_UFCS_RX_LENGTH_REG, &len);
	if (ret)
		return;

	if (!len || len > CPS2021_UFCS_RX_BUF_SIZE) {
		hwlog_err("length is %d, invalid\n");
		return;
	}

	ret = cps2021_read_block(di, CPS2021_UFCS_RX_BUFFER_REG, data, len);
	if (ret)
		return;

	di->ufcs_irq[0] = 0;
	di->ufcs_irq[1] = 0;
	node = cps2021_ufcs_create_node(irq1, irq2, data, len);
	if (!node)
		return;

	cps2021_ufcs_add_node(di, node);
}

static struct cps2021_ufcs_msg_node *cps2021_ufcs_get_msg(struct cps2021_device_info *di)
{
	struct cps2021_ufcs_msg_head *head = g_cps2021_ufcs_msg_head;

	if (!head || !head->next) {
		hwlog_err("msg head or node is null\n");
		return NULL;
	}

	return head->next;
}

void cps2021_ufcs_work(struct work_struct *work)
{
	struct cps2021_device_info *di = NULL;
	u8 ufcs_irq[2] = { 0 }; /* 2: two interrupt regs */

	if (!work)
		return;

	di = container_of(work, struct cps2021_device_info, ufcs_work);
	if (!di || !di->client) {
		hwlog_err("di is null\n");
		return;
	}

	(void)cps2021_read_byte(di, CPS2021_UFCS_ISR1_REG, &ufcs_irq[0]);
	(void)cps2021_read_byte(di, CPS2021_UFCS_ISR2_REG, &ufcs_irq[1]);
	di->ufcs_irq[0] |= ufcs_irq[0];
	di->ufcs_irq[1] |= ufcs_irq[1];

	if (ufcs_irq[0] & CPS2021_UFCS_ISR1_DATA_READY_MASK) {
		if (!di->ufcs_communicating_flag)
			power_event_bnc_notify(POWER_BNT_UFCS,
				POWER_NE_UFCS_REC_UNSOLICITED_DATA, NULL);
		cps2021_ufcs_add_msg(di);
	}

	hwlog_info("UFCS_ISR1[0x%x]=0x%x, UFCS_ISR2[0x%x]=0x%x\n",
		CPS2021_UFCS_ISR1_REG, ufcs_irq[0], CPS2021_UFCS_ISR2_REG, ufcs_irq[1]);
}

static int cps2021_ufcs_wait(struct cps2021_device_info *di, u8 flag)
{
	u8 reg_val1 = 0;
	u8 reg_val2 = 0;
	int i;
	struct cps2021_ufcs_msg_node *node = NULL;

	for (i = 0; i < CPS2021_UFCS_WAIT_RETRY_CYCLE; i++) {
		power_usleep(DT_USLEEP_1MS);
		node = cps2021_ufcs_get_msg(di);
		if (!node)
			continue;

		reg_val1 |= node->irq1;
		reg_val2 |= node->irq2;
		hwlog_info("ufcs_isr1=0x%x, ufcs_isr2=0x%x\n", reg_val1, reg_val2);

		/* isr1[4] must be 0 */
		if ((flag & HWUFCS_WAIT_CRC_ERROR) &&
			(reg_val1 & CPS2021_UFCS_ISR1_CRC_ERROR_MASK)) {
			hwlog_err("crc error\n");
			break;
		}

		/* isr2[1] must be 0 */
		if ((flag & HWUFCS_ACK_RECEIVE_TIMEOUT) &&
			(reg_val2 & CPS2021_UFCS_ISR2_MSG_TRANS_FAIL_MASK)) {
			hwlog_err("not receive ack after retry\n");
			break;
		}

		/* isr1[3] must be 1 */
		if ((flag & HWUFCS_WAIT_SEND_PACKET_COMPLETE) &&
			!(reg_val1 & CPS2021_UFCS_ISR1_SEND_PACKET_COMPLETE_MASK)) {
			hwlog_err("sent packet not complete or fail\n");
			continue;
		}

		/* isr1[2] must be 1 */
		if ((flag & HWUFCS_WAIT_DATA_READY) &&
			(!(reg_val1 & CPS2021_UFCS_ISR1_DATA_READY_MASK) &&
			!(reg_val2 & CPS2021_UFCS_ISR2_RX_BUF_BUSY_MASK))) {
			hwlog_err("receive data not ready\n");
			continue;
		}

		hwlog_info("wait succ\n");
		return 0;
	}

	hwlog_err("wait fail\n");
	return -EINVAL;
}

static int cps2021_ufcs_detect_adapter(void *dev_data)
{
	struct cps2021_device_info *di = dev_data;
	u8 reg_val = 0;
	int ret, i;

	if (!di) {
		hwlog_err("di is null\n");
		return HWUFCS_DETECT_OTHER;
	}

	di->ufcs_irq[0] = 0;
	di->ufcs_irq[1] = 0;
	mutex_lock(&di->ufcs_detect_lock);

	/* disable scp */
	ret = cps2021_write_mask(di, CPS2021_UFCS_CTL1_REG,
		CPS2021_UFCS_CTL1_EN_PROTOCOL_MASK,
		CPS2021_UFCS_CTL1_EN_PROTOCOL_SHIFT, 0);

	/* enable ufcs */
	ret += cps2021_write_mask(di, CPS2021_UFCS_CTL1_REG,
		CPS2021_UFCS_CTL1_EN_PROTOCOL_MASK,
		CPS2021_UFCS_CTL1_EN_PROTOCOL_SHIFT, CPS2021_UFCS_ENABLE);

	/* enable handshake */
	ret += cps2021_write_mask(di, CPS2021_UFCS_CTL1_REG,
		CPS2021_UFCS_CTL1_EN_HANDSHAKE_MASK,
		CPS2021_UFCS_CTL1_EN_HANDSHAKE_SHIFT, CPS2021_ENABLE_HANDSHAKE);
	(void)power_usleep(DT_USLEEP_20MS);

	/* waiting for handshake */
	for (i = 0; i < CPS2021_UFCS_HANDSHARK_RETRY_CYCLE; i++) {
		(void)power_usleep(DT_USLEEP_2MS);
		ret = cps2021_read_byte(di, CPS2021_UFCS_ISR1_REG, &reg_val);
		if (ret) {
			hwlog_err("read isr reg[%x] fail\n", CPS2021_UFCS_ISR1_REG);
			continue;
		}

		reg_val |= di->ufcs_irq[0];
		hwlog_info("ufcs_isr1=0x%x\n", reg_val);
		if (reg_val & CPS2021_UFCS_ISR1_HANDSHAKE_FAIL_MASK) {
			i = CPS2021_UFCS_HANDSHARK_RETRY_CYCLE;
			break;
		}

		if (reg_val & CPS2021_UFCS_ISR1_HANDSHAKE_SUCC_MASK)
			break;
	}

	(void)cps2021_write_mask(di, CPS2021_UFCS_CTL1_REG,
		CPS2021_UFCS_CTL1_EN_HANDSHAKE_MASK,
		CPS2021_UFCS_CTL1_EN_HANDSHAKE_SHIFT, CPS2021_DISABLE_HANDSHAKE);
	mutex_unlock(&di->ufcs_detect_lock);

	if (i == CPS2021_UFCS_HANDSHARK_RETRY_CYCLE) {
		hwlog_err("handshake fail\n");
		return HWUFCS_DETECT_FAIL;
	}

	(void)cps2021_write_mask(di, CPS2021_UFCS_CTL2_REG,
		CPS2021_UFCS_CTL2_DEV_ADDR_ID_MASK,
		CPS2021_UFCS_CTL2_DEV_ADDR_ID_SHIFT, CPS2021_UFCS_CTL2_SOURCE_ADDR);
	hwlog_info("handshake succ\n");
	return HWUFCS_DETECT_SUCC;
}

static int cps2021_ufcs_write_msg(void *dev_data, u8 *data, u8 len, u8 flag)
{
	struct cps2021_device_info *di = dev_data;
	int ret;

	if (!di || !data) {
		hwlog_err("di or data is null\n");
		return -ENODEV;
	}

	if (len > CPS2021_UFCS_TX_BUF_WITHOUTHEAD_SIZE) {
		hwlog_err("invalid length=%u\n", len);
		return -EINVAL;
	}

	di->ufcs_irq[0] = 0;
	di->ufcs_irq[1] = 0;
	ret = cps2021_write_byte(di, CPS2021_UFCS_TX_LENGTH_REG, len);
	ret += cps2021_write_block(di, CPS2021_UFCS_TX_BUFFER_REG,
		data, len);
	ret += cps2021_write_mask(di, CPS2021_UFCS_CTL1_REG,
		CPS2021_UFCS_CTL1_SEND_MASK, CPS2021_UFCS_CTL1_SEND_SHIFT, 1);
	ret += cps2021_ufcs_wait(di, flag);
	return ret;
}

static int cps2021_ufcs_wait_msg_ready(void *dev_data, u8 flag)
{
	struct cps2021_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -ENODEV;
	}

	return cps2021_ufcs_wait(di, flag);
}

static int cps2021_ufcs_get_rx_len(void *dev_data, u8 *len)
{
	struct cps2021_device_info *di = dev_data;
	struct cps2021_ufcs_msg_node *msg = cps2021_ufcs_get_msg(di);

	if (!di || !msg) {
		hwlog_err("di or msg is null\n");
		*len = 0;
		return -ENODEV;
	}

	*len = msg->len;
	return 0;
}

static int cps2021_ufcs_read_msg(void *dev_data, u8 *data, u8 len)
{
	struct cps2021_device_info *di = dev_data;
	struct cps2021_ufcs_msg_node *msg = cps2021_ufcs_get_msg(di);

	if (!di || !data || !msg) {
		hwlog_err("di or data or msg is null\n");
		return -ENODEV;
	}

	if (len > CPS2021_UFCS_RX_BUF_WITHOUTHEAD_SIZE) {
		hwlog_err("invalid length=%u\n", len);
		return -EINVAL;
	}

	memcpy(data, &msg->data[0], len);
	return 0;
}

static int cps2021_ufcs_end_read_msg(void *dev_data)
{
	struct cps2021_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -ENODEV;
	}

	cps2021_ufcs_delete_node(di);
	return 0;
}

static int cps2021_ufcs_soft_reset_master(void *dev_data)
{
	struct cps2021_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -ENODEV;
	}

	di->ufcs_irq[0] = 0;
	di->ufcs_irq[1] = 0;
	return cps2021_write_mask(di, CPS2021_UFCS_CTL1_REG,
		CPS2021_UFCS_CTL1_EN_PROTOCOL_MASK,
		CPS2021_UFCS_CTL1_EN_PROTOCOL_SHIFT, 0);
}

static int cps2021_ufcs_set_communicating_flag(void *dev_data, bool flag)
{
	struct cps2021_device_info *di = dev_data;
	struct cps2021_ufcs_msg_head *head = g_cps2021_ufcs_msg_head;

	if (!di || !head) {
		hwlog_err("di or head is null\n");
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

	di->ufcs_communicating_flag = flag;
	return 0;
}

static struct hwufcs_ops cps2021_hwufcs_ops = {
	.chip_name = "cps2021",
	.detect_adapter = cps2021_ufcs_detect_adapter,
	.write_msg = cps2021_ufcs_write_msg,
	.wait_msg_ready = cps2021_ufcs_wait_msg_ready,
	.read_msg = cps2021_ufcs_read_msg,
	.end_read_msg = cps2021_ufcs_end_read_msg,
	.soft_reset_master = cps2021_ufcs_soft_reset_master,
	.get_rx_len = cps2021_ufcs_get_rx_len,
	.set_communicating_flag = cps2021_ufcs_set_communicating_flag,
};

int cps2021_ufcs_ops_register(struct cps2021_device_info *di)
{
	if (di->param_dts.ufcs_support) {
		cps2021_hwufcs_ops.dev_data = (void *)di;
		return hwufcs_ops_register(&cps2021_hwufcs_ops);
	}

	return 0;
}
