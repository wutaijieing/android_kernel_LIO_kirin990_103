/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: Drivers for Crypto Core smx process.
 * Create: 2020/03/01
 */
#include "crypto_core_smx.h"
#include "crypto_core_internal.h"
#include "crypto_core_errno.h"
#include "crypto_core_factory.h"
#include "crypto_core_fs.h"
#include "crypto_core_smc.h"

#define SMX_PROCESS_0     0x5A5AA5A5
#define SMX_PROCESS_1     0xA5A55A5A

 /* get smx status of the phone. Input is not used */
int32_t mspc_get_smx_status(void)
{
	int32_t smx;

	smx = mspc_send_smc_no_ack((uint64_t)MSPC_FN_MAIN_SERVICE_CMD,
				   (uint64_t)MSPC_SMC_SMX_GET_EFUSE, NULL, 0);
	/* SMX_PROCESS_1: smx is not disabled */
	if (smx != (int32_t)SMX_PROCESS_1) {
		pr_err("%s(): %x\n", __func__, smx);
		return SMX_DISABLE;
	}
	return SMX_ENABLE;
}

/* send set smx smc to atf */
static int32_t mspc_disable_smx_flash_cos_fn(void)
{
	return mspc_send_smc(MSPC_SMC_SET_SMX);
}

/* run flash cos to disable smx */
int32_t mspc_disable_smx(void)
{
	int32_t ret;
	int32_t ret1;
	uint32_t state = 0;

	ret = mspc_check_flash_cos_file(&state);
	if (ret != MSPC_OK || state != FLASH_COS_EXIST) {
		pr_err("%s:check flashcos state %u failed\n", __func__, state);
		return MSPC_FLASH_COS_ERROR;
	}

	/* try to enter the factory mode to prevent smx and business conflict. */
	ret = mspc_enter_factory_mode();
	if (ret != MSPC_OK) {
		pr_err("%s enter factory mode fail\n", __func__);
		return ret;
	}

	/* run flashCOS to close smx. */
	ret = mspc_run_flash_cos(mspc_disable_smx_flash_cos_fn);
	if (ret != MSPC_OK)
		pr_err("%s:run flash cos failed\n", __func__);

	/* when smx disable is completed, exit factory mode. */
	ret1 = mspc_exit_factory_mode();
	if (ret1 != MSPC_OK)
		pr_err("%s exit factory mode fail\n", __func__);

	return (ret != MSPC_OK) ? ret : ret1;
}
