/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: Crypto Core at command smx function.
 * Create: 2020/4/9
 */
#ifndef CRYPTO_CORE_AT_SMX_H
#define CRYPTO_CORE_AT_SMX_H

#include <linux/kernel.h>
#include <linux/types.h>
#include "crypto_core_errno.h"

/*
 * @brief: Set MSPC smx enable and disable.
 * @param[in]: data - smx setting parameter, the parameter indicates whether to enable or disable SMX.
 * @param[in]: resp - MSPC at response callback function, not currently used.
 * @return: if success return MSPC_OK, others are failure.
 */
int32_t mspc_at_set_smx(const char *data, int32_t (*resp)(const char *buf, size_t len));

/*
 * @brief: Get MSPC smx status.
 * @param[in]: data - not currently used.
 * @param[in]: resp - MSPC at response callback function.
 * @return: if success return MSPC_OK, others are failure.
 */
int32_t mspc_at_get_smx(const char *data, int32_t (*resp)(const char *buf, size_t len));

#endif /* CRYPTO_CORE_AT_SMX_H */
