/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: implement general see autotest functions
 * Create: 2020-02-17
 */
#ifndef GENERAL_SEE_AUTOTEST_H
#define GENERAL_SEE_AUTOTEST_H

/* channel test for hisee */
#define TEST_DIRECTORY_PATH     "/data/hisee_test"
#define TEST_SUCCESS_FILE       "/data/hisee_test/test_success.txt"
#define TEST_FAIL_FILE          "/data/hisee_test/test_fail.txt"
#define TEST_RESULT_FILE        "/data/hisee_test/test_result.img"

#define HISEE_CHANNEL_TEST_CMD_ERROR            (-6000)
#define HISEE_CHANNEL_TEST_RESULT_MALLOC_ERROR  (-6001)
#define HISEE_CHANNEL_TEST_PATH_ABSENT_ERROR    (-6002)
#define HISEE_CHANNEL_TEST_WRITE_RESULT_ERROR   (-6003)

#define HISEE_CHAR_SPACE      32
#define HISEE_INT_NUM_HEX_LEN 8

#define bypass_space_char() \
do { \
	if (offset >= HISEE_CMD_NAME_LEN) { \
		pr_err("%s(): hisee channel test cmd is bad\n", __func__); \
		return HISEE_CHANNEL_TEST_CMD_ERROR; \
	} \
	if (buff[offset] != HISEE_CHAR_SPACE) \
		break; \
	offset++; \
} while (1)

#define ALIGN_UP_4KB(x)                     (((x) + 0xFFF) & (~0xFFF))
#define CHANNEL_TEST_RESULT_SIZE_DEFAULT    0x40000
#define TEST_RESULT_SIZE_DEFAULT            0x40000

int hisee_channel_test_func(const void *buf, int para);

#endif
