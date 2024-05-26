/*
 * piezo_key.h
 *
 * piezo key driver
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

#ifndef PIEZO_KEY_H
#define PIEZO_KEY_H

#include <linux/types.h>

#define HEADER_LEN         2
#define VERSION_LEN        4
#define MAX_LOG_NUM        170
#define UPDATE_PACKAGE_LEN 1024

enum piezo_key_errorno {
	FT_SUCCESS,
	FT_I2C_ERR,
	FT_JUMP_BOOT_FAILED,
	FT_JUMP_APP_FAILED
};

enum control_command_id {
	COMMAND_CONTROL_ID_START = 0x00,
	COMMAND_JUMP_APP_OR_BOOT = COMMAND_CONTROL_ID_START,
	COMMAND_UPDATE_PACKAGE_SEND,
	COMMAND_CLEAR_LOG,
	MAX_CONTROL_COMMAND_ID
};

enum factory_test_command_id {
	COMMAND_FACTORY_TEST_ID_START = 0x20,
	COMMAND_NOISE_TEST = COMMAND_FACTORY_TEST_ID_START,
	COMMAND_VOLTAGE_WAVE_TEST,
	COMMAND_BIBRATE_TEST,
	COMMAND_PRESS_AGING_TEST,
	COMMAND_CONTROL_VIB_TEST,
	COMMAND_SET_SELF_CHECK_PARAM,
	MAX_FACTORY_TEST_COMMAND_ID
};

enum get_command_id {
	COMMAND_GET_ID_START = 0x40,
	COMMAND_GET_VERSION = COMMAND_GET_ID_START,
	COMMAND_GET_LOG,
	COMMAND_GET_CHIPID,
	COMMAND_GET_DEVICE_INFO,
	COMMAND_GET_FACTORY_BIGDATA,
	COMMAND_GET_MCU_RAWDATA,
	COMMAND_GET_WAVE_INFO,
	COMMAND_GET_TARGET_WAVE,
	COMMAND_GET_ACTUAL_WAVE,
	COMMAND_GET_PWM_PARAM,
	COMMAND_GET_SYSTEM_STATUS,
	MAX_GET_COMMAND_ID
};

enum jump_area_t {
	BOOTLOADER,
	APPLICATION
};

enum ioctl_cmd_t {
	IOCTL_CMD_GET_VERSION,
	IOCTL_CMD_SELF_CHECK,
	IOCTL_CMD_SELF_VIB,
	IOCTL_CMD_GET_LOG,
	IOCTL_CMD_CLEAR_LOG,
};

#pragma pack(1)
struct version_struct {
	uint8_t version[VERSION_LEN];
};

struct command_data {
	uint8_t cmd_id;
	uint16_t len;
	uint8_t data[];
};

struct update_command {
	uint8_t cmd_id;
	uint16_t len;
	uint16_t package_num_count;
	uint16_t package_num;
	uint8_t data[UPDATE_PACKAGE_LEN];
	uint16_t crc;
};

struct log_item {
	uint32_t log_tick;
	uint32_t log_id;
	uint32_t para1;
	uint32_t para2;
	uint32_t para3;
	uint32_t para4;
};  // 24Bytes

struct log_data {
	uint8_t log_num;
	struct log_item log[MAX_LOG_NUM];
};
#pragma pack()

#define TYPE 'F'
#define GET_VERSION_CMD _IOR(TYPE, IOCTL_CMD_GET_VERSION, struct version_struct)
#define SELF_CHECK_CMD _IOR(TYPE, IOCTL_CMD_SELF_CHECK, int32_t)
#define SELF_VIB_CMD _IOWR(TYPE, IOCTL_CMD_SELF_VIB, int32_t)
#define GET_LOG_CMD _IOR(TYPE, IOCTL_CMD_GET_LOG, struct log_data)
#define CLEAR_LOG_CMD _IOR(TYPE, IOCTL_CMD_CLEAR_LOG, int32_t)

#endif