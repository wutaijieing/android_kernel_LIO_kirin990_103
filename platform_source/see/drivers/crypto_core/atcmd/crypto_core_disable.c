/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Drivers for Crypto Core disable process.
 * Create: 2021/06/02
 */
#include <platform_include/see/efuse_driver.h>
#include <securec.h>
#include "crypto_core_errno.h"
#include "crypto_core_factory.h"
#include "crypto_core_factory_task.h"
#include "crypto_core_internal.h"
#include "crypto_core_upgrade.h"
#include "crypto_core_smc.h"

#define CRYPTO_CORE_DISABLE_TASK_TRY_CNT   1

static DEFINE_MUTEX(g_crypto_core_disable_lock);

static int32_t crypto_core_disable_key_flash_cos_fn(void)
{
	return mspc_send_smc(MSPC_SMC_DISABLE_KEY);
}

static int crypto_core_load_flashcos_disable_key(void)
{
	int32_t ret;

	ret = mspc_upgrade_image(MSPC_UPGRADE_FLASHCOS);
	if (ret != MSPC_OK) {
		pr_err("%s: mspc_upgrade_image ret %x\n", __func__, ret);
		return ret;
	}

	ret = mspc_run_flash_cos(crypto_core_disable_key_flash_cos_fn);
	if (ret != MSPC_OK) {
		pr_err("%s: disable_key ret %x\n", __func__, ret);
		return ret;
	}

	return MSPC_OK;
}

static bool crypto_core_check_disabled(void)
{
	int32_t ret;
	uint32_t lcs_mode = 0;

	ret = mspc_get_lcs_mode(&lcs_mode);
	if (ret != MSPC_OK) {
		pr_err("%s: get_lcs_mode ret %x\n", __func__, ret);
		return false;
	}

	if (lcs_mode == MSPC_SM_MODE_MAGIC) {
		pr_err("%s lcs is SM\n", __func__);
		return true;
	}
	return false;
}

static int32_t crypto_core_mark_disabled(void)
{
	int32_t ret;
	uint32_t value[CRYPTO_CORE_EFUSE_GROUP_NUM] = {0};
	uint32_t sm_efuse_pos = crypto_core_get_sm_efuse_pos();
	uint32_t group = sm_efuse_pos / CRYPTO_CORE_EFUSE_GROUP_BIT_SIZE;
	uint32_t offset = sm_efuse_pos % CRYPTO_CORE_EFUSE_GROUP_BIT_SIZE;

	if (group >= CRYPTO_CORE_EFUSE_GROUP_NUM) {
		pr_err("%s:sm_flag_pos invalid!\n", __func__);
		return MSPC_LCS_EFUSE_POS_ERROR;
	}

	value[group] = CRYPTO_CORE_EFUSE_SM_MODE << offset;
	ret = set_efuse_hisee_value((uint8_t *)value,
				    CRYPTO_CORE_EFUSE_LENGTH,
				    CRYPTO_CORE_EFUSE_TIMEOUT);
	if (ret != 0) {
		pr_err("%s: set efuse failed, ret=%d\n", __func__, ret);
		return MSPC_SET_LCS_VALUE_ERROR;
	}

	return MSPC_OK;
}

static int32_t crypto_core_disable_otp_key(void)
{
	int32_t ret;

	if (!crypto_core_check_disabled()) {
		ret = crypto_core_load_flashcos_disable_key();
		if (ret != MSPC_OK) {
			pr_err("%s: disable key ret %x\n", __func__, ret);
			return ret;
		}

		ret = crypto_core_mark_disabled();
		if (ret != MSPC_OK) {
			pr_err("%s: mark disabled ret %x\n", __func__, ret);
			return ret;
		}
	}

	return MSPC_OK;
}

static struct crypto_core_task g_crypto_core_disable_task = {
	.state = ATOMIC_INIT(MSPC_FACTORY_TEST_NORUNNING),
	.errcode = ATOMIC_INIT(MSPC_ERROR),
	.try_cnt = CRYPTO_CORE_DISABLE_TASK_TRY_CNT,
	.taskfn = crypto_core_disable_otp_key,
	.name = "crypto_core_disable_task",
	.lock = &g_crypto_core_disable_lock,
};

/*
 * @brief: Execute CryptoCore disable
 * @param[in]: data - not currently used.
 * @param[in]: resp - response callback function, not currently used.
 * @return: if success return MSPC_OK, others are failure.
 */
int32_t crypto_core_execute_disable(const char *data, int32_t (*resp)(const char *buf, size_t len))
{
	(void)data;
	(void)resp;

	return crypto_core_create_task(&g_crypto_core_disable_task);
}

/*
 * @brief: Check CryptoCore disable result.
 * @param[in]: data - not currently used.
 * @param[in]: resp - response callback function.
 * @return: if success return MSPC_OK, others are failure.
 */
int32_t crypto_core_check_disable(const char *data, int32_t (*resp)(const char *buf, size_t len))
{
	(void)data;
	(void)resp;

	return crypto_core_check_task(&g_crypto_core_disable_task);
}
