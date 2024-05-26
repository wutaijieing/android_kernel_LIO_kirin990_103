/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: Crypto Core at command test function.
 * Create: 2020/6/1
 */
#ifndef CRYPTO_CORE_AT_TEST_H
#define CRYPTO_CORE_AT_TEST_H

#include <linux/types.h>
#include <linux/kernel.h>
#include <crypto_core_errno.h>

/*
 * @brief: Test MSPC at basic function.
 * @param[in]: data - MSPC at test input data.
 * @param[in]: resp - MSPC at response callback function.
 * @return: if success return MSPC_OK, others are failure.
 */
int32_t mspc_at_test(const char *data, int32_t (*resp)(const char *buf, size_t len));

#endif /* CRYPTO_CORE_AT_TEST_H */
