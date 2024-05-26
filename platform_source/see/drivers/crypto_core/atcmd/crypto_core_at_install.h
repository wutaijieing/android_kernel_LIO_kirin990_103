/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: msp core at command install interface.
 * Create: 2020/4/9
 */
#ifndef CRYPTO_CORE_AT_INSTALL_H
#define CRYPTO_CORE_AT_INSTALL_H

#include <linux/kernel.h>
#include <linux/types.h>
#include "crypto_core_errno.h"

/*
 * @brief: Execute MSPC install test.
 * @param[in]: data - not currently used.
 * @param[in]: resp - MSPC at response callback function, not currently used.
 * @return: if success return MSPC_OK, others are failure.
 */
int32_t mspc_at_execute_install(const char *data, int32_t (*resp)(const char *buf, size_t len));

/*
 * @brief: Check MSPC install test result.
 * @param[in]: data - not currently used.
 * @param[in]: resp - MSPC at response callback function, not currently used.
 * @return: if success return MSPC_OK, others are failure.
 */
int32_t mspc_at_check_install(const char *data, int32_t (*resp)(const char *buf, size_t len));

/*
 * @brief: Check whether MSPC install should be bypass or not.
 * @return: if bypass return TRUE, not bypass is FALSE.
 */
bool mspc_at_install_bypass(void);

#endif /* CRYPTO_CORE_AT_INSTALL_H */
