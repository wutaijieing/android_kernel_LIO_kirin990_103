/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Header file for Crypto Core smc process.
 * Create: 2019/11/15
 */

#ifndef CRYPTO_CORE_SMC_H
#define CRYPTO_CORE_SMC_H

#include <linux/types.h>

/* ATF service id */
#define MSPC_FN_MAIN_SERVICE_CMD           0xC5000020
#define MSPC_FN_CHANNEL_TEST_CMD           0xC5000040

#define MSPC_ATF_MESSAGE_HEADER_LEN        20
#define MSPC_ATF_READ_IMG_TIMEOUT          30000 /* 30s */
#define MSPC_ATF_SMC_TIMEOUT               30000 /* 30s */

#define MSPC_SMC_PARAM_SIZE                2
#define MSPC_SMC_PARAM_0                   0
#define MSPC_SMC_PARAM_1                   1

enum mspc_smc_cmd {
	MSPC_SMC_UPGRADE_SLOADER             = 0x0,
	MSPC_SMC_UPGRADE_ULOADER             = 0x1,
	MSPC_SMC_UPGRADE_COS                 = 0x2,
	MSPC_SMC_UPGRADE_DCS                 = 0x3,
	MSPC_SMC_UPGRADE_OTP                 = 0x4,
	MSPC_SMC_SET_LCS_SM                  = 0x5,
	MSPC_SMC_READ_COS_IMAGE              = 0x6,
	MSPC_SMC_GET_STATE                   = 0x7,
	MSPC_SMC_POWER_ON                    = 0x8,
	MSPC_SMC_POWER_OFF                   = 0x9,
	MSPC_SMC_SMX_GET_EFUSE               = 0xA,
	MSPC_SMC_SET_SMX                     = 0xB,
	MSPC_SMC_SET_UPGRADE_FLAG_ADDR       = 0xC,
	MSPC_SMC_SET_AUTHSWITCH              = 0xD,
	MSPC_SMC_CLR_UPDATEFLAG              = 0xE,
	MSPC_SMC_SET_UPGRADE                 = 0xF,
	MSPC_SMC_SWITCH_RPMB                 = 0x10,
	MSPC_SMC_ENTER_FACTORY_MODE          = 0x11,
	MSPC_SMC_EXIT_FACTORY_MODE           = 0x12,
	MSPC_SMC_SECFLASH_INITKEY            = 0x13,
	MSPC_SMC_SECFLASH_SETURS             = 0x14,
	MSPC_SMC_DISABLE_KEY                 = 0x15,
	MSPC_SMC_CHANNEL_AUTOTEST            = 0xA5, /* the item is last pos */
};

enum mspc_atf_ack_type {
	MSPC_ATF_ACK_ERROR       = 0x3897C6A9,
	MSPC_ATF_ACK_OK          = 0xC7683956,
};

/* message header between kernel and atf */
struct mspc_atf_msg_header {
	uint32_t cmd;
	uint32_t cos_id;
	uint32_t ack;
	uint32_t result_addr; /* Phymem, CMA memory, 32bits is enough. */
	uint32_t result_size;
};

void mspc_init_smc(void);
void mspc_set_atf_msg_header(struct mspc_atf_msg_header *header,
			     uint32_t cmd_type);
int32_t mspc_send_smc_no_ack(uint64_t fid, uint64_t smc_cmd,
			     const char *msg, uint32_t msg_len);
int32_t mspc_send_smc_process(struct mspc_atf_msg_header *p_msg_header,
			      uint64_t phy_addr, uint64_t size,
			      uint32_t timeout, uint64_t smc_cmd);
#endif /* CRYPTO_CORE_SMC_H */

