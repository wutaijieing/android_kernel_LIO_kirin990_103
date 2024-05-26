// SPDX-License-Identifier: GPL-2.0
/*
 * stm32g031_ufcs.c
 *
 * stm32g031 ufcs driver
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

#include "stm32g031_ufcs.h"
#include "stm32g031_i2c.h"
#include "stm32g031_fw.h"
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/protocol/adapter_protocol.h>
#include <chipset_common/hwpower/protocol/adapter_protocol_ufcs.h>

#define HWLOG_TAG stm32g031_ufcs
HWLOG_REGIST();

static int stm32g031_ufcs_wait(struct stm32g031_device_info *di, u8 flag)
{
	u8 reg_val1 = 0;
	u8 reg_val2 = 0;
	int ret, i;

	for (i = 0; i < STM32G031_UFCS_WAIT_RETRY_CYCLE; i++) {
		power_usleep(DT_USLEEP_1MS);
		ret = stm32g031_read_byte(di, STM32G031_UFCS_ISR1_REG, &reg_val1);
		ret += stm32g031_read_byte(di, STM32G031_UFCS_ISR2_REG, &reg_val2);
		if (ret) {
			hwlog_err("read isr reg fail\n");
			continue;
		}

		di->ufcs_isr[0] |= reg_val1;
		di->ufcs_isr[1] |= reg_val2;
		hwlog_info("ufcs_isr1=0x%x, ufcs_isr2=0x%x\n",
			di->ufcs_isr[0], di->ufcs_isr[1]);

		/* isr1[4] must be 0 */
		if ((flag & HWUFCS_WAIT_CRC_ERROR) &&
			(di->ufcs_isr[0] & STM32G031_UFCS_ISR1_CRC_ERROR_MASK)) {
			hwlog_err("crc error\n");
			break;
		}

		/* isr2[1] must be 0 */
		if ((flag & HWUFCS_ACK_RECEIVE_TIMEOUT) &&
			(di->ufcs_isr[1] & STM32G031_UFCS_ISR2_MSG_TRANS_FAIL_MASK)) {
			hwlog_err("not receive ack after retry\n");
			break;
		}

		/* isr1[3] must be 1 */
		if ((flag & HWUFCS_WAIT_SEND_PACKET_COMPLETE) &&
			!(di->ufcs_isr[0] & STM32G031_UFCS_ISR1_SEND_PACKET_COMPLETE_MASK)) {
			hwlog_err("sent packet not complete or fail\n");
			continue;
		}

		/* isr1[2] must be 1 */
		if ((flag & HWUFCS_WAIT_DATA_READY) &&
			!(di->ufcs_isr[0] & STM32G031_UFCS_ISR1_DATA_READY_MASK)) {
			hwlog_err("receive data not ready\n");
			continue;
		}

		hwlog_info("wait succ\n");
		return 0;
	}

	hwlog_err("wait fail\n");
	return -EINVAL;
}

static int stm32g031_ufcs_detect_adapter(void *dev_data)
{
	struct stm32g031_device_info *di = dev_data;
	u8 reg_val = 0;
	int ret, i;

	if (!di) {
		hwlog_err("di is null\n");
		return HWUFCS_DETECT_OTHER;
	}

	mutex_lock(&di->ufcs_detect_lock);

	ret = stm32g031_fw_set_hw_config(di);
	if (ret && !di->is_low_power_mode) {
		stm32g031_hard_reset(di);
		ret = stm32g031_fw_set_hw_config(di);
	}

	power_usleep(DT_USLEEP_20MS);
	ret += stm32g031_fw_get_hw_config(di);
	if (ret) {
		hwlog_err("set hw config fail\n");
		mutex_unlock(&di->ufcs_detect_lock);
		return HWUFCS_DETECT_FAIL;
	}

	/* clear handshake bit */
	ret = stm32g031_write_mask(di, STM32G031_UFCS_CTL1_REG,
			STM32G031_UFCS_CTL1_EN_UFCS_HANDSHAKE_MASK,
			STM32G031_UFCS_CTL1_EN_UFCS_HANDSHAKE_SHIFT, 0);
	/* waiting for mcu ready */
	(void)power_usleep(DT_USLEEP_1MS);
	/* start handshake */
	ret += stm32g031_write_mask(di, STM32G031_UFCS_CTL1_REG,
		STM32G031_UFCS_CTL1_EN_PROTOCOL_MASK,
		STM32G031_UFCS_CTL1_EN_PROTOCOL_SHIFT, STM32G031_CTL1_EN_UFCS);
	ret += stm32g031_write_mask(di, STM32G031_UFCS_CTL1_REG,
		STM32G031_UFCS_CTL1_EN_UFCS_HANDSHAKE_MASK,
		STM32G031_UFCS_CTL1_EN_UFCS_HANDSHAKE_SHIFT, 1);
	(void)power_usleep(DT_USLEEP_20MS);
	/* waiting for handshake */
	for (i = 0; i < STM32G031_UFCS_HANDSHARK_RETRY_CYCLE; i++) {
		(void)power_usleep(DT_USLEEP_2MS);
		ret = stm32g031_read_byte(di, STM32G031_UFCS_ISR1_REG, &reg_val);
		if (ret) {
			hwlog_err("read isr reg[%x] fail\n", STM32G031_UFCS_ISR1_REG);
			continue;
		}

		reg_val |= di->ufcs_isr[0];
		hwlog_info("ufcs_isr1=0x%x\n", reg_val);
		if (reg_val & STM32G031_UFCS_ISR1_HANDSHAKE_FAIL_MASK) {
			i = STM32G031_UFCS_HANDSHARK_RETRY_CYCLE;
			break;
		}

		if (reg_val & STM32G031_UFCS_ISR1_HANDSHAKE_SUCC_MASK)
			break;
	}

	mutex_unlock(&di->ufcs_detect_lock);

	if (i == STM32G031_UFCS_HANDSHARK_RETRY_CYCLE) {
		hwlog_err("handshake fail\n");
		return HWUFCS_DETECT_FAIL;
	}

	(void)stm32g031_write_mask(di, STM32G031_UFCS_CTL2_REG,
		STM32G031_UFCS_CTL2_DEV_ADDR_ID_MASK,
		STM32G031_UFCS_CTL2_DEV_ADDR_ID_SHIFT, STM32G031_UFCS_CTL2_SOURCE_ADDR);
	hwlog_info("handshake succ\n");
	return HWUFCS_DETECT_SUCC;
}

static int stm32g031_ufcs_write_msg(void *dev_data, u8 *data, u8 len, u8 flag)
{
	struct stm32g031_device_info *di = dev_data;
	int ret;

	if (!di || !data) {
		hwlog_err("di or data is null\n");
		return -ENODEV;
	}

	if (len > STM32G031_UFCS_TX_BUF_WITHOUTHEAD_SIZE) {
		hwlog_err("invalid length=%u\n", len);
		return -EINVAL;
	}

	di->ufcs_isr[0] = 0;
	di->ufcs_isr[1] = 0;
	ret = stm32g031_write_byte(di, STM32G031_UFCS_TX_LENGTH_REG, len);
	ret += stm32g031_write_block(di, STM32G031_UFCS_TX_BUFFER_REG,
		data, len);
	ret += stm32g031_write_mask(di, STM32G031_UFCS_CTL1_REG,
		STM32G031_UFCS_CTL1_SEND_MASK, STM32G031_UFCS_CTL1_SEND_SHIFT, 1);
	ret += stm32g031_ufcs_wait(di, flag);
	return ret;
}

static int stm32g031_ufcs_wait_msg_ready(void *dev_data, u8 flag)
{
	struct stm32g031_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -ENODEV;
	}

	return stm32g031_ufcs_wait(di, flag);
}

static int stm32g031_ufcs_get_rx_len(void *dev_data, u8 *len)
{
	struct stm32g031_device_info *di = dev_data;
	u8 reg_val = 0;
	int ret;

	if (!di) {
		hwlog_err("di is null\n");
		return -ENODEV;
	}

	ret = stm32g031_read_byte(di, STM32G031_UFCS_RX_LENGTH_REG, &reg_val);
	if (ret) {
		*len = 0;
		return -EINVAL;
	}

	*len = reg_val;
	return ret;
}

static int stm32g031_ufcs_read_msg(void *dev_data, u8 *data, u8 len)
{
	struct stm32g031_device_info *di = dev_data;

	if (!di || !data) {
		hwlog_err("di or data is null\n");
		return -ENODEV;
	}

	if (len > STM32G031_UFCS_RX_BUF_WITHOUTHEAD_SIZE) {
		hwlog_err("invalid length=%u\n", len);
		return -EINVAL;
	}

	return stm32g031_read_block(di, STM32G031_UFCS_RX_BUFFER_REG,
		data, len);
}

static int stm32g031_ufcs_end_read_msg(void *dev_data)
{
	struct stm32g031_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -ENODEV;
	}

	di->ufcs_isr[0] = 0;
	di->ufcs_isr[1] = 0;

	return 0;
}

static int stm32g031_ufcs_soft_reset_master(void *dev_data)
{
	struct stm32g031_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -ENODEV;
	}

	return stm32g031_write_mask(di, STM32G031_UFCS_CTL1_REG,
		STM32G031_UFCS_CTL1_EN_PROTOCOL_MASK,
		STM32G031_UFCS_CTL1_EN_PROTOCOL_SHIFT, STM32G031_CTL1_DISABLE_PROTOCOL);
}

static int stm32g031_ufcs_set_communicating_flag(void *dev_data, bool flag)
{
	struct stm32g031_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -ENODEV;
	}

	if (di->ufcs_communicating_flag && flag) {
		hwlog_info("is communicating, wait\n");
		return -EINVAL;
	}

	di->ufcs_communicating_flag = flag;
	return 0;
}

static bool stm32g031_ufcs_need_check_ack(void *dev_data)
{
	return false;
}

static struct hwufcs_ops stm32g031_hwufcs_ops = {
	.chip_name = "stm32g031",
	.detect_adapter = stm32g031_ufcs_detect_adapter,
	.write_msg = stm32g031_ufcs_write_msg,
	.wait_msg_ready = stm32g031_ufcs_wait_msg_ready,
	.read_msg = stm32g031_ufcs_read_msg,
	.end_read_msg = stm32g031_ufcs_end_read_msg,
	.soft_reset_master = stm32g031_ufcs_soft_reset_master,
	.get_rx_len = stm32g031_ufcs_get_rx_len,
	.set_communicating_flag = stm32g031_ufcs_set_communicating_flag,
	.need_check_ack = stm32g031_ufcs_need_check_ack,
};

int stm32g031_hwufcs_register(struct stm32g031_device_info *di)
{
	stm32g031_hwufcs_ops.dev_data = (void *)di;
	return hwufcs_ops_register(&stm32g031_hwufcs_ops);
}
