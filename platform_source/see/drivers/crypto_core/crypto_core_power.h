/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: Header file for Crypto Core power module.
 * Create: 2019/11/15
 */
#ifndef CRYPTO_CORE_POWER_H
#define CRYPTO_CORE_POWER_H
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/types.h>

#define MSPC_CHAR_NEWLINE      10
#define MSPC_CHAR_SPACE        32
#define MSPC_CHAR_ZERO         48

#define MSPC_VOTE_ON           0xA
#define MSPC_VOTE_OFF          0x5
#define MSPC_VOTE_MASK         0xF

/* The number of '5' must be equal the MSPC_MAX_VOTE_ID defined below. */
#define MSPC_VOTE_OFF_STATUS   0x555

#define MSPC_VOTE_UNIT_SIZE    4 /* 4 bits */

#define MSPC_POWER_OFF_DELAY_TIME           60 /* s */

/* Cos id is "1", vote id is "1" (MSPC_CHIP_TEST). */
#define MSPC_NATIVE_COS_POWER_PARAM       " 11"
/* Cos id is "5", vote id is "1" (MSPC_CHIP_TEST). */
#define MSPC_FLASH_COS_POWER_PARAM        " 51"

/* Cos id is "1", vote id is "2" (MSPC_PINCODE). */
#define MSPC_PINCODE_POWER_PARAM          " 12"

/* The para to lpm3 throug atf */
enum mspc_power_operation {
	MSPC_POWER_OFF                 = 0x01000100,
	MSPC_POWER_ON_BOOTING          = 0x01000101,
	MSPC_POWER_ON_UPGRADE          = 0x01000102,
	MSPC_POWER_ON_UPGRADE_SM       = 0x01000103,
	MSPC_POWER_ON_BOOTING_SECFLASH = 0x01000104,
	MSPC_POWER_ON_DCS              = 0x01000105,
	MSPC_POWER_MAX_OP,
};

/* the powerctrl command */
enum mspc_power_cmd {
	MSPC_POWER_CMD_ON  = 0x01000200,
	MSPC_POWER_CMD_OFF = 0x01000201,
};

enum mspc_vote_status {
	MSPC_VOTE_STATUS_ON   = 0xB4A5A5C3,
	MSPC_VOTE_STATUS_OFF  = 0x4B5A5A3C,
};

/*
 * Any modification of this definition must be sync to
 * "union mspc_power_vote_status", MSPC_VOTE_OFF_STATUS,
 * MSPC_NATIVE_COS_POWER_PARAM, MSPC_FLASH_COS_POWER_PARAM,
 * and MSPC_PINCODE_POWER_PARAM in this file.
 */
enum mspc_vote_id {
	MSPC_UPGRADE       = 0,
	MSPC_CHIP_TEST,
	MSPC_PINCODE,
	MSPC_MAX_VOTE_ID,
};

/*
 * Every vote id has 4 bits which specify the mspc power status
 * of the specific vote id respectively.
 * 0x5 indicate power off status.
 * 0xA indicate power on status.
 * This definition must be sync with enum mspc_vote_id!!!
 */
union mspc_power_vote_status {
	uint32_t value;
	struct {
		uint32_t mspc_upgrade   : 4;
		uint32_t mspc_factory   : 4;
		uint32_t mspc_pincode   : 4;
		uint32_t reserved       : 20;
	} status;
};

enum mspc_state {
	MSPC_STATE_POWER_DOWN       = 0,
	MSPC_STATE_POWER_UP         = 1,
	MSPC_STATE_NATIVE_READY     = 3,
	MSPC_STATE_POWER_DOWN_DOING = 4,
	MSPC_STATE_POWER_UP_DOING   = 5,
	MSPC_STATE_DCS_UPGRADE_DONE = 7,
	MSPC_STATE_SECFLASH         = 8,
	MSPC_STATE_MAX,
};

enum mspc_vote_status mspc_get_vote_status(void);
int32_t mspc_power_func(const int8_t *buf, int32_t len,
			enum mspc_power_operation op_type,
			enum mspc_power_cmd cmd);

int32_t mspc_suspend(struct platform_device *pdev, struct pm_message state);
#ifdef MSPC_POWER_ON_IN_ADVANCE_FEATURE
int32_t mspc_resume(struct platform_device *pdev);
#endif
ssize_t mspc_powerctrl_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count);

ssize_t mspc_powerctrl_show(struct device *dev,
			    struct device_attribute *attr, char *buf);

ssize_t mspc_check_ready_show(struct device *dev,
			      struct device_attribute *attr, char *buf);

int32_t mspc_wait_state(enum mspc_state expected_state, uint32_t ms);

void mspc_init_power(void);

int32_t mspc_force_power_off(void);
#endif /* CRYPTO_CORE_POWER_H */
