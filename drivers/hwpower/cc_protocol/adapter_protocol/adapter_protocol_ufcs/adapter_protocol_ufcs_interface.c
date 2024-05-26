// SPDX-License-Identifier: GPL-2.0
/*
 * adapter_protocol_ufcs_interface.c
 *
 * ufcs protocol common interface function
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

#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/protocol/adapter_protocol.h>
#include <chipset_common/hwpower/protocol/adapter_protocol_ufcs.h>

#define HWLOG_TAG ufcs_protocol_interface
HWLOG_REGIST();

static struct hwufcs_ops *g_hwufcs_ops;
static int g_dev_id = PROTOCOL_DEVICE_ID_END;

static const struct adapter_protocol_device_data g_hwufcs_dev_data[] = {
	{ PROTOCOL_DEVICE_ID_STM32G031, "stm32g031" },
	{ PROTOCOL_DEVICE_ID_CPS2021, "cps2021" },
	{ PROTOCOL_DEVICE_ID_SC8546, "sc8546" },
};

static int hwufcs_get_device_id(const char *str)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(g_hwufcs_dev_data); i++) {
		if (!strncmp(str, g_hwufcs_dev_data[i].name, strlen(str)))
			return g_hwufcs_dev_data[i].id;
	}

	return -EPERM;
}

static struct hwufcs_ops *hwufcs_get_ops(void)
{
	if (!g_hwufcs_ops) {
		hwlog_err("g_hwufcs_ops is null\n");
		return NULL;
	}

	return g_hwufcs_ops;
}

int hwufcs_ops_register(struct hwufcs_ops *ops)
{
	int dev_id;

	if (!ops || !ops->chip_name) {
		hwlog_err("ops or chip_name is null\n");
		return -EPERM;
	}

	dev_id = hwufcs_get_device_id(ops->chip_name);
	if (dev_id < 0) {
		hwlog_err("%s ops register fail\n", ops->chip_name);
		return -EPERM;
	}

	g_hwufcs_ops = ops;
	g_dev_id = dev_id;
	hwlog_info("%d:%s ops register ok\n", dev_id, ops->chip_name);
	return 0;
}

int hwufcs_detect_adapter(void)
{
	struct hwufcs_ops *l_ops = hwufcs_get_ops();

	if (!l_ops)
		return -EPERM;

	if (!l_ops->detect_adapter) {
		hwlog_err("detect_adapter is null\n");
		return -EPERM;
	}

	hwlog_info("detect_adapter\n");
	return l_ops->detect_adapter(l_ops->dev_data);
}

int hwufcs_soft_reset_master(void)
{
	struct hwufcs_ops *l_ops = hwufcs_get_ops();

	if (!l_ops)
		return -EPERM;

	if (!l_ops->soft_reset_master) {
		hwlog_err("soft_reset_master is null\n");
		return -EPERM;
	}

	hwlog_info("soft_reset_master\n");
	return l_ops->soft_reset_master(l_ops->dev_data);
}

int hwufcs_write_msg(u8 *data, u8 len, u8 flag)
{
	struct hwufcs_ops *l_ops = hwufcs_get_ops();

	if (!l_ops)
		return -EPERM;

	if (!l_ops->write_msg) {
		hwlog_err("write_msg is null\n");
		return -EPERM;
	}

	hwlog_info("write_msg:len=%d flag=%02x\n", len, flag);
	return l_ops->write_msg(l_ops->dev_data, data, len, flag);
}

int hwufcs_read_msg(u8 *data, u8 len)
{
	struct hwufcs_ops *l_ops = hwufcs_get_ops();

	if (!l_ops)
		return -EPERM;

	if (!l_ops->read_msg) {
		hwlog_err("read_msg is null\n");
		return -EPERM;
	}

	hwlog_info("read_msg\n");
	return l_ops->read_msg(l_ops->dev_data, data, len);
}

int hwufcs_wait_msg_ready(u8 flag)
{
	struct hwufcs_ops *l_ops = hwufcs_get_ops();

	if (!l_ops)
		return -EPERM;

	if (!l_ops->wait_msg_ready) {
		hwlog_err("wait_msg_ready is null\n");
		return -EPERM;
	}

	hwlog_info("wait_msg_ready: flag=%02x\n", flag);
	return l_ops->wait_msg_ready(l_ops->dev_data, flag);
}

int hwufcs_get_rx_length(u8 *len)
{
	struct hwufcs_ops *l_ops = hwufcs_get_ops();

	if (!l_ops)
		return -EPERM;

	if (!l_ops->get_rx_len) {
		hwlog_err("get_rx_len is null\n");
		*len = 0;
		return -EPERM;
	}

	return l_ops->get_rx_len(l_ops->dev_data, len);
}

int hwufcs_end_read_msg(void)
{
	struct hwufcs_ops *l_ops = hwufcs_get_ops();

	if (!l_ops)
		return -EPERM;

	if (!l_ops->end_read_msg) {
		hwlog_err("end_read_msg is null\n");
		return -EPERM;
	}

	hwlog_info("end_read_msg\n");
	return l_ops->end_read_msg(l_ops->dev_data);
}

int hwufcs_set_communicating_flag(bool flag)
{
	struct hwufcs_ops *l_ops = hwufcs_get_ops();

	if (!l_ops)
		return -EPERM;

	if (!l_ops->set_communicating_flag) {
		hwlog_err("set_communicating_flag is null\n");
		return -EPERM;
	}

	hwlog_info("set_communicating_flag:%d\n", flag);
	return l_ops->set_communicating_flag(l_ops->dev_data, flag);
}

bool hwufcs_need_check_ack(void)
{
	struct hwufcs_ops *l_ops = hwufcs_get_ops();

	if (!l_ops)
		return true;

	if (!l_ops->need_check_ack) {
		hwlog_err("need_check_ack is null\n");
		return true;
	}

	return l_ops->need_check_ack(l_ops->dev_data);
}

int hwufcs_check_dev_id(void)
{
	if ((g_dev_id >= PROTOCOL_DEVICE_ID_BEGIN) &&
		(g_dev_id < PROTOCOL_DEVICE_ID_END))
		return 0;

	hwlog_info("get_protocol_register_state fail\n");
	return -EPERM;
}
