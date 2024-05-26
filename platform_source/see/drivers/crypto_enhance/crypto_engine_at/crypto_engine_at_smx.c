/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: crypto engine at command smx function.
 * Create: 2020/4/9
 */
#include "crypto_engine_at_smx.h"
#include <linux/types.h>
#include <linux/kernel.h>
#include <securec.h>
#include <crypto_enhance.h>

#ifndef unused
#define unused(x) (void)(x)
#endif
#define MSPE_SMX_ENABLE_STRING     "0"
#define MSPE_SMX_DISABLE_STRING    "1"
#define MSPE_SMX_UNKNOWN_STRING    "UNKNOW"
#define MSPE_SMX_RES_STRING_MAXLEN 32

#define MSPE_SMX_ENABLE            0x5A5AA5A5
#define MSPE_SMX_DISABLE           0xA5A55A5A

int32_t mspe_at_set_smx(const char *value, int32_t (*resp)(char *buf, int32_t len))
{
	int32_t ret;
	int param = -1;
	u64 smx;

	pr_info("%s: entry", __func__);
	unused(resp);
	ret = sscanf_s(value, "%d", &param);
	if (ret != 1) {
		pr_err("%s: sscanf_s fail,value:%s\n", __func__, value);
		return MSPE_ERROR;
	}

	switch (param) {
	case 0: /* MSPE_SMX_ENABLE_STRING */
		smx= MSPE_SMX_ENABLE;
		break;
	case 1: /* MSPE_SMX_DISABLE_STRING */
		smx= MSPE_SMX_DISABLE;
		break;
	default:
		pr_err("%s: invalid value:%s\n", __func__, value);
		return MSPE_ERROR;
	}

	ret = hieps_smc_send((u64)HIEPS_SMC_FID, (u64)HIEPS_SET_SMX_CMD,
			     (u64)smx, (u64)0);
	if (ret != HIEPS_OK) {
		pr_err("%s hieps_smc_send fail\n", __func__);
		return MSPE_ERROR;
	}

	return MSPE_OK;
}

int32_t mspe_at_get_smx(const char *value, int32_t (*resp)(char *buf, int32_t len))
{
	int32_t smx;
	char *res = NULL;
	int reslen;

	pr_info("%s: entry", __func__);

	smx = hieps_smc_send((u64)HIEPS_SMC_FID,
			     (u64)HIEPS_GET_SMX_CMD, (u64)0, (u64)0);
	switch (smx) {
	case MSPE_SMX_ENABLE:
		res = MSPE_SMX_ENABLE_STRING;
		reslen = sizeof(MSPE_SMX_ENABLE_STRING);
		break;
	case MSPE_SMX_DISABLE:
		res = MSPE_SMX_DISABLE_STRING;
		reslen = sizeof(MSPE_SMX_DISABLE_STRING);
		break;
	default:
		res = MSPE_SMX_UNKNOWN_STRING;
		reslen = sizeof(MSPE_SMX_UNKNOWN_STRING);
		break;
	}

	return resp(res, reslen);
}

