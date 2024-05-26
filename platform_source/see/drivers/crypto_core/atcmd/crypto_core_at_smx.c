/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: Crypto Core at command smx function.
 * Create: 2020/4/9
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <securec.h>
#include "crypto_core_errno.h"
#include "crypto_core_factory.h"
#include "crypto_core_smx.h"

#ifndef UNUSED_PARAM
#define UNUSED_PARAM(param)             (void)(param)
#endif

#define MSPC_AT_SMX_ENABLE_STR          "0"
#define MSPC_AT_SMX_DISABLE_STR         "1"

/*
 * @brief: Set MSPC smx enable and disable.
 * @param[in]: data - smx setting parameter, the parameter indicates whether to enable or disable SMX.
 * @param[in]: resp - MSPC at response callback function, not currently used.
 * @return: if success return MSPC_OK, others are failure.
 */
int32_t mspc_at_set_smx(const char *data, int32_t (*resp)(const char *buf, size_t len))
{
	UNUSED_PARAM(resp);

	if (!data) {
		pr_err("%s invalid param data\n", __func__);
		return MSPC_INVALID_PARAMS_ERROR;
	}

	if (strcmp(data, MSPC_AT_SMX_ENABLE_STR) == 0) {
		if (mspc_get_smx_status() != SMX_ENABLE) {
			pr_err("%s smx disabled\n", __func__);
			return MSPC_AT_SMX_ENABLE_ERROR;
		}
		return MSPC_OK;
	}

	if (strcmp(data, MSPC_AT_SMX_DISABLE_STR) == 0) {
		if (mspc_get_smx_status() == SMX_DISABLE)
			return MSPC_OK;
		return mspc_disable_smx();
	}

	pr_err("%s para not support\n", __func__);
	return MSPC_INVALID_PARAMS_ERROR;
}

/*
 * @brief: Get MSPC smx status.
 * @param[in]: data - not currently used.
 * @param[in]: resp - MSPC at response callback function.
 * @return: if success return MSPC_OK, others are failure.
 */
int32_t mspc_at_get_smx(const char *data, int32_t (*resp)(const char *buf, size_t len))
{
	UNUSED_PARAM(data);

	if (!resp)
		return MSPC_AT_RSP_CALLBACK_NULL_ERROR;

	if (mspc_get_smx_status() == SMX_ENABLE)
		return resp(MSPC_AT_SMX_ENABLE_STR, sizeof(MSPC_AT_SMX_ENABLE_STR));
	else
		return resp(MSPC_AT_SMX_DISABLE_STR, sizeof(MSPC_AT_SMX_DISABLE_STR));
}

