/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: implement Crypto Core autotest functions
 * Create: 2020-06-09
 */
#ifndef CRYPTO_CORE_AUTOTEST_H
#define CRYPTO_CORE_AUTOTEST_H
#include <linux/types.h>
#include "crypto_core_errno.h"

/* channel test for hisee */
#define TEST_DIRECTORY_PATH     "/data/hisee_test"
#define TEST_SUCCESS_FILE       "/data/hisee_test/test_success.txt"
#define TEST_FAIL_FILE          "/data/hisee_test/test_fail.txt"
#define TEST_RESULT_FILE        "/data/hisee_test/test_result.img"

#define ALIGN_UP_4KB(x)                     (((x) + 0xFFF) & (~0xFFF))
#define CHANNEL_TEST_RESULT_SIZE_DEFAULT    0x40000
#define TEST_RESULT_SIZE_DEFAULT            0x40000

#define MSPC_CHAR_SPACE       32
#define MSPC_CHAR_NEWLINE     10
#define MSPC_INT_NUM_HEX_LEN  8
#define MSPC_CMD_NAME_LEN     128
#define MAX_PATH_NAME_LEN     128

struct mspc_autotest_work_struct {
	int8_t *buffer;
	phys_addr_t phy;
	uint32_t size;
};

#define bypass_space_char() \
do { \
	if (offset >= MSPC_CMD_NAME_LEN) { \
		pr_err("%s(): mspc channel test cmd is bad\n", __func__); \
		return MSPC_CHANNEL_TEST_CMD_ERROR; \
	} \
	if (buff[offset] != MSPC_CHAR_SPACE) \
		break; \
	offset++; \
} while (1)

int mspc_channel_test_func(const void *buf, int para);
#endif
