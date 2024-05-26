/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: Crypto Core at command socslt interface.
 * Create: 2020/4/9
 */
#ifndef CRYPTO_CORE_AT_SOCSLT_H
#define CRYPTO_CORE_AT_SOCSLT_H

#include <linux/types.h>
#include <linux/kernel.h>
#include <crypto_core_errno.h>

/*
 * @brief: Execute MSPC at soc slt.
 * @param[in]: data - soc slt parameter, not currently used.
 * @param[in]: resp - MSPC at response callback function, not currently used.
 * @return: if success return MSPC_OK, others are failure.
 */
int32_t mspc_at_execute_socslt(const char *data, int32_t (*resp)(const char *buf, size_t len));

/*
 * @brief: Check MSPC at soc slt result.
 * @param[in]: data - soc slt parameter, not currently used.
 * @param[in]: resp - MSPC at response callback function, not currently used.
 * @return: if success return MSPC_OK, others are failure.
 */
int32_t mspc_at_check_socslt(const char *data, int32_t (*resp)(const char *buf, size_t len));

#endif /* CRYPTO_CORE_AT_SOCSLT_H */
