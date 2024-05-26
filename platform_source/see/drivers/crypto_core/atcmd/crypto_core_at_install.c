/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: Crypto Core at command install function.
 * Create: 2020/4/9
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include "crypto_core_internal.h"
#include "crypto_core_errno.h"
#include "crypto_core_factory.h"
#include "crypto_core_factory_task.h"
#include "crypto_core_fs.h"

#ifndef UNUSED_PARAM
#define UNUSED_PARAM(param)                (void)(param)
#endif

#define MSPC_AT_INSTALL_RUNNING_RSP_STR    "Running"
#define MSPC_AT_INSTALL_SUCCESS_RSP_STR    "PASS"
#define MSPC_AT_INSTALL_FAIL_RSP_STR       "FAIL"

#define CRYPTO_CORE_INSTALL_TASK_TRY_CNT   1

static DEFINE_MUTEX(g_crypto_core_install_lock);

static bool mspc_at_install_bypass(void)
{
	int32_t ret;
	uint32_t status_result = SECFLASH_IS_ABSENCE_MAGIC;

	ret = mspc_secflash_is_available(&status_result);
	if (ret != MSPC_OK) {
		pr_err("%s: get secflash dts info fail\n", __func__);
		return false;
	}
	if (status_result == SECFLASH_IS_ABSENCE_MAGIC) {
		pr_info("%s: secflash dts is disable, bypass install\n", __func__);
		return true;
	}
	return false;
}

static int32_t mspc_install_without_secflash(void)
{
	int32_t ret;
	uint32_t state = 0;

	ret = mspc_check_flash_cos_file(&state);
	if (ret != MSPC_OK || state == FLASH_COS_EMPTY) {
		pr_err("%s:check flash cos failed\n", __func__);
		return MSPC_FLASH_COS_ERROR;
	}

	/* Test already, exit directly. */
	if (state == FLASH_COS_ERASE) {
		pr_err("%s:flash cos is erased\n", __func__);
		return MSPC_OK;
	}

	/* delete flashcos image */
	ret = mspc_remove_flash_cos();
	if (ret != MSPC_OK)
		pr_err("%s:remove flash cos err %d\n", __func__, ret);
	return ret;
}

static int32_t mspc_install_func(void)
{
	int32_t ret;

	if (mspc_at_install_bypass()) {
		pr_info("%s: bypass, install without secflash!\n", __func__);
		ret = mspc_install_without_secflash();
	} else {
		pr_info("%s: install not bypass!\n", __func__);
		ret = mspc_total_manufacture_func();
	}

	return ret;
}

static bool mspc_install_task_run_contition(void)
{
	if (mspc_chiptest_otp1_is_running())
		return false;
	return true;
}

static struct crypto_core_task g_mspc_at_install = {
	.state = ATOMIC_INIT(MSPC_FACTORY_TEST_NORUNNING),
	.errcode = ATOMIC_INIT(MSPC_ERROR),
	.try_cnt = CRYPTO_CORE_INSTALL_TASK_TRY_CNT,
	.taskfn = mspc_install_func,
	.conditon_fn = mspc_install_task_run_contition,
	.name = "mspc_install_task",
	.lock = &g_crypto_core_install_lock,
};

/*
 * @brief: Execute MSPC install test.
 * @param[in]: data - not currently used.
 * @param[in]: resp - MSPC at response callback function, not currently used.
 * @return: if success return MSPC_OK, others are failure.
 */
int32_t mspc_at_execute_install(const char *data, int32_t (*resp)(const char *buf, size_t len))
{
	UNUSED_PARAM(data);
	UNUSED_PARAM(resp);

	return crypto_core_create_task(&g_mspc_at_install);
}

/*
 * @brief: Check MSPC install test result.
 * @param[in]: data - not currently used.
 * @param[in]: resp - MSPC at response callback function, not currently used.
 * @return: if success return MSPC_OK, others are failure.
 */
int32_t mspc_at_check_install(const char *data, int32_t (*resp)(const char *buf, size_t len))
{
	enum mspc_factory_test_state state;
	int32_t errcode;
	int32_t ret;

	UNUSED_PARAM(data);

	if (!resp)
		return MSPC_AT_RSP_CALLBACK_NULL_ERROR;

	mutex_lock(&g_crypto_core_install_lock);
	state = (enum mspc_factory_test_state)atomic_read(&g_mspc_at_install.state);
	errcode = atomic_read(&g_mspc_at_install.errcode);
	mutex_unlock(&g_crypto_core_install_lock);

	if (state == MSPC_FACTORY_TEST_NORUNNING)
		return MSPC_AT_CMD_NOTRUNNING_ERROR;

	if (mspc_at_install_bypass()) {
		pr_info("%s: bypass, check result of install without secflash!\n", __func__);
		if (state == MSPC_FACTORY_TEST_RUNNING)
			return resp(MSPC_AT_INSTALL_RUNNING_RSP_STR, sizeof(MSPC_AT_INSTALL_RUNNING_RSP_STR));

		if (state == MSPC_FACTORY_TEST_SUCCESS && errcode == MSPC_OK)
			return resp(MSPC_AT_INSTALL_SUCCESS_RSP_STR, sizeof(MSPC_AT_INSTALL_SUCCESS_RSP_STR));

		pr_err("%s: failed, result = %d\n", __func__, errcode);
		(void)resp(MSPC_AT_INSTALL_FAIL_RSP_STR, sizeof(MSPC_AT_INSTALL_FAIL_RSP_STR));
		return errcode;
	}

	if (state == MSPC_FACTORY_TEST_RUNNING || mspc_chiptest_otp1_is_running())
		return resp(MSPC_AT_INSTALL_RUNNING_RSP_STR, sizeof(MSPC_AT_INSTALL_RUNNING_RSP_STR));

	if (state == MSPC_FACTORY_TEST_SUCCESS && errcode == MSPC_OK) {
		ret = mspc_exit_factory_mode();
		if (ret != MSPC_OK) {
			(void)resp(MSPC_AT_INSTALL_FAIL_RSP_STR, sizeof(MSPC_AT_INSTALL_FAIL_RSP_STR));
			return ret;
		}
		return resp(MSPC_AT_INSTALL_SUCCESS_RSP_STR, sizeof(MSPC_AT_INSTALL_SUCCESS_RSP_STR));
	}

	if (state == MSPC_FACTORY_TEST_FAIL && errcode != MSPC_OK) {
		(void)mspc_exit_factory_mode();
		(void)resp(MSPC_AT_INSTALL_FAIL_RSP_STR, sizeof(MSPC_AT_INSTALL_FAIL_RSP_STR));
		return errcode;
	}

	pr_err("%s: failed, state %d, errcode 0x%x\n", __func__, (int32_t)state, errcode);
	(void)resp(MSPC_AT_INSTALL_FAIL_RSP_STR, sizeof(MSPC_AT_INSTALL_FAIL_RSP_STR));
	return MSPC_AT_INSTALL_STATE_ERROR;
}
