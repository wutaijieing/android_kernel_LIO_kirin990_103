/* SPDX-License-Identifier: GPL-2.0 */
/*
 * adapter_protocol_uvdm.h
 *
 * uvdm protocol driver
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
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

#ifndef _ADAPTER_PROTOCOL_UVDM_H_
#define _ADAPTER_PROTOCOL_UVDM_H_

#include <linux/device.h>
#include <linux/completion.h>
#include <linux/notifier.h>

/* section: protocol data */
#define HWUVDM_VID                    0x12d1
#define HWUVDM_VDM_TYPE               0
#define HWUVDM_VERSION                0
#define HWUVDM_VDOS_LEN               7

/* section: protocol time */
#define HWUVDM_RETRY_TIMES            3
#define HWUVDM_WAIT_RESPONSE_TIME     1000

/* section: unit */
#define HWUVDM_VOLTAGE_UNIT           10

/* section: auth encrypt */
#define HWUVDM_KEY_INDEX_OFFSET       0
#define HWUVDM_RANDOM_S_OFFSET        1
#define HWUVDM_RANDOM_E_OFFESET       7
#define HWUVDM_RANDOM_HASH_SHIFT      8
#define HWUVDM_ENCTYPT_HASH_LEN       8
#define HWUVDM_ENCRYPT_RANDOM_LEN     8

/* section: mask */
#define hwuvdm_bit(i)                 ((1 << (i)) - 1)
#define HWUVDM_MASK_VDM_TYPE          hwuvdm_bit(1)
#define HWUVDM_MASK_POWER_TYPE        hwuvdm_bit(2)
#define HWUVDM_MASK_RESPONSE_STATE    hwuvdm_bit(3)
#define HWUVDM_MASK_OTG_EVENT         hwuvdm_bit(3)
#define HWUVDM_MASK_FUNCTION          hwuvdm_bit(5)
#define HWUVDM_MASK_CMD               hwuvdm_bit(7)
#define HWUVDM_MASK_KEY               hwuvdm_bit(8)
#define HWUVDM_MASK_REPORT_ABNORMAL   hwuvdm_bit(8)
#define HWUVDM_MASK_VOLTAGE           hwuvdm_bit(12)
#define HWUVDM_MASK_VID               hwuvdm_bit(16)

/*
 * uvdm message header data as below:
 * bit0: command direction
 * bit1~7: command
 * bit8~12: function
 * bit13~14: uvdm version
 * bit15: vdm type
 * bit16~31: vendor id
 */
enum hwuvdm_header_data_shift {
	HWUVDM_HDR_SHIFT_CMD_DIRECTTION = 0,
	HWUVDM_HDR_SHIFT_CMD = 1,
	HWUVDM_HDR_SHIFT_FUNCTION = 8,
	HWUVDM_HDR_SHIFT_VERSION = 13,
	HWUVDM_HDR_SHIFT_VDM_TYPE = 15,
	HWUVDM_HDR_SHIFT_VID = 16,
};

enum hwuvdm_command_direction {
	HWUVDM_CMD_DIRECTION_INITIAL,
	HWUVDM_CMD_DIRECTION_ANSWER,
};

enum hwuvdm_command_dc_ctrl {
	HWUVDM_CMD_REPORT_POWER_TYPE = 1,
	HWUVDM_CMD_SEND_RANDOM = 2,
	HWUVDM_CMD_GET_HASH = 3,
	HWUVDM_CMD_SWITCH_POWER_MODE = 4,
	HWUVDM_CMD_SET_ADAPTER_ENABLE = 5,
	HWUVDM_CMD_SET_VOLTAGE = 6,
	HWUVDM_CMD_GET_VOLTAGE = 7,
	HWUVDM_CMD_GET_TEMP = 8,
	HWUVDM_CMD_HARD_RESET = 9,
	HWUVDM_CMD_REPORT_ABNORMAL = 10,
	HWUVDM_CMD_SET_RX_REDUCE_VOLTAGE = 11,
};

enum hwuvdm_command_pd_ctrl {
	HWUVDM_CMD_REPORT_OTG_EVENT = 1,
};

enum hwuvdm_function {
	HWUVDM_FUNCTION_BEGIN,
	HWUVDM_FUNCTION_TA_CTRL,
	HWUVDM_FUNCTION_DOCK_CTRL,
	HWUVDM_FUNCTION_DC_CTRL,
	HWUVDM_FUCNTION_RX_CTRL,
	HWUVDM_FUNCTION_PD_CTRL,
	HWUVDM_FUNCTION_USB_EXT_MODEM,
	HWUVDM_FUNCTION_END,
};

enum hwuvdm_vdos_count {
	HWUVDM_VDOS_COUNT_BEGIN,
	HWUVDM_VDOS_COUNT_ONE,
	HWUVDM_VDOS_COUNT_TWO,
	HWUVDM_VDOS_COUNT_THREE,
	HWUVDM_VDOS_COUNT_FOUR,
	HWUVDM_VDOS_COUNT_FIVE,
	HWUVDM_VDOS_COUNT_SIX,
	HWUVDM_VDOS_COUNT_SEVEN,
	HWUVDM_VDOS_COUNT_END,
};

enum hwuvdm_vdo_byte {
	HWUVDM_VDO_BYTE_ZERO,
	HWUVDM_VDO_BYTE_ONE,
	HWUVDM_VDO_BYTE_TWO,
	HWUVDM_VDO_BYTE_THREE,
	HWUVDM_VDO_BYTE_FOUR,
	HWUVDM_VDO_BYTE_FIVE,
	HWUVDM_VDO_BYTE_SIX,
	HWUVDM_VDO_BYTE_SEVEN,
};

enum hwuvdm_power_mode {
	HWUVDM_PWR_MODE_DEFAULT,
	HWUVDM_PWR_MODE_BUCK2SC,
	HWUVDM_PWR_MODE_SC2BUCK5W,
	HWUVDM_PWR_MODE_SC2BUCK10W,
};

enum hwuvdm_response_status {
	HWUVDM_RESPONSE_ACK,
	HWUVDM_RESPONSE_NAK,
	HWUVDM_RESPONSE_BUSY,
	HWUVDM_RESPONSE_TX_INTERRUPT,
	HWUVDM_RESPONSE_POWER_READY,
};

struct hwuvdm_header_data {
	u32 cmd_direction;
	u32 cmd;
	u32 function;
	u32 uvdm_version;
	u32 vdm_type;
	u32 vid;
};

struct hwuvdm_device_info {
	int power_type;
	int volt;
	int abnormal_flag;
	int otg_event;
};

struct hwuvdm_ops {
	const char *chip_name;
	void *dev_data;
	void (*send_data)(u32 *data, u8 cnt, bool wait_rsp, void *dev_data);
};

struct hwuvdm_dev {
	struct device *dev;
	struct notifier_block nb;
	struct completion rsp_comp;
	struct completion report_type_comp;
	int dev_id;
	struct hwuvdm_ops *p_ops;
	u8 encrypt_random_value[HWUVDM_ENCRYPT_RANDOM_LEN];
	u8 encrypt_hash_value[HWUVDM_ENCTYPT_HASH_LEN];
	struct hwuvdm_device_info info;
};

#ifdef CONFIG_ADAPTER_PROTOCOL_UVDM
int hwuvdm_ops_register(struct hwuvdm_ops *ops);
int hwuvdm_handle_vdo_data(u32 *data, int len, bool response,
	unsigned int retrys);
#else
static inline int hwuvdm_ops_register(struct hwuvdm_ops *ops)
{
	return 0;
}

static inline int hwuvdm_handle_vdo_data(u32 *data, int len, bool response,
	unsigned int retrys)
{
	return 0;
}
#endif /* CONFIG_ADAPTER_PROTOCOL_UVDM */

#endif /* _ADAPTER_PROTOCOL_UVDM_H_ */
