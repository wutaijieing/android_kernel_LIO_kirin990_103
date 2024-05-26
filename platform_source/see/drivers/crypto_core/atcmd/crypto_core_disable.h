/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Drivers for Crypto Core disable process.
 * Create: 2021/06/02
 */

#ifndef CRYPTO_CORE_DISABLE_H
#define CRYPTO_CORE_DISABLE_H
#include <linux/types.h>

/*
 * @brief: Execute CryptoCore disable
 * @param[in]: data - not currently used.
 * @param[in]: resp - response callback function, not currently used.
 * @return: if success return MSPC_OK, others are failure.
 */
int32_t crypto_core_execute_disable(const char *data, int32_t (*resp)(const char *buf, size_t len));

/*
 * @brief: Check CryptoCore disable result.
 * @param[in]: data - not currently used.
 * @param[in]: resp - response callback function.
 * @return: if success return MSPC_OK, others are failure.
 */
int32_t crypto_core_check_disable(const char *data, int32_t (*resp)(const char *buf, size_t len));

#endif
