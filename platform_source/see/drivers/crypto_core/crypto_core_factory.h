/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: Header file for Crypto Core chip test.
 * Create: 2019/12/30
 */

#ifndef CRYPTO_CORE_CHIP_TEST_H
#define CRYPTO_CORE_CHIP_TEST_H
#include <linux/device.h>
#include <linux/types.h>

enum mspc_factory_test_state {
	MSPC_FACTORY_TEST_FAIL      = -1,
	MSPC_FACTORY_TEST_SUCCESS   = 0,
	MSPC_FACTORY_TEST_RUNNING   = 1,
	MSPC_FACTORY_TEST_NORUNNING = 2,
};

bool mspc_chiptest_otp1_is_running(void);
int32_t mspc_send_smc(uint32_t cmd_type);
int32_t mspc_run_flash_cos(int32_t (*fn)(void));
int32_t mspc_enter_factory_mode(void);
int32_t mspc_exit_factory_mode(void);
int32_t mspc_total_manufacture_func(void);
int32_t mspc_total_slt_func(void);
int32_t mspc_chiptest_rt_run_func(const int8_t *buf, int32_t len);
int32_t mspc_chiptest_rt_stop_func(const int8_t *buf, int32_t len);

int32_t mspc_flash_debug_switchs(void);
int32_t mspc_check_boot_upgrade(void);
void mspc_factory_init(void);

#endif /* CRYPTO_CORE_CHIP_TEST_H */
