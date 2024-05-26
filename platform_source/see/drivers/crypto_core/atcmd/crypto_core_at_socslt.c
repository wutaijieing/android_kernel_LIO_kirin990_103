/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: Crypto Core at command socslt function.
 * Create: 2020/4/9
 */
#ifdef CONFIG_DFX_DEBUG_FS

#include <linux/types.h>
#include <linux/kernel.h>
#include <crypto_core_errno.h>
#include <crypto_core_factory.h>
#include <crypto_core_factory_task.h>

#ifndef UNUSED_PARAM
#define UNUSED_PARAM(param)          (void)(param)
#endif

#define MSPC_AT_SOCSLT_RETRY_CNT     3

static DEFINE_MUTEX(g_mspc_at_socslt_lock);

static struct crypto_core_task g_crypto_core_socslt_task = {
	.state = ATOMIC_INIT(MSPC_FACTORY_TEST_NORUNNING),
	.errcode = ATOMIC_INIT(MSPC_ERROR),
	.try_cnt = MSPC_AT_SOCSLT_RETRY_CNT,
	.taskfn = mspc_total_slt_func,
	.name = "mspc_at_socslt_task",
	.lock = &g_mspc_at_socslt_lock,
};

/*
 * @brief: Execute MSPC at soc slt.
 * @param[in]: data - soc slt parameter, not currently used.
 * @param[in]: resp - MSPC at response callback function, not currently used.
 * @return: if success return MSPC_OK, others are failure.
 */
int32_t mspc_at_execute_socslt(const char *data, int32_t (*resp)(const char *buf, size_t len))
{
	UNUSED_PARAM(data);
	UNUSED_PARAM(resp);

	return crypto_core_create_task(&g_crypto_core_socslt_task);
}

/*
 * @brief: Check MSPC at soc slt result.
 * @param[in]: data - soc slt parameter, not currently used.
 * @param[in]: resp - MSPC at response callback function, not currently used.
 * @return: if success return MSPC_OK, others are failure.
 */
int32_t mspc_at_check_socslt(const char *data, int32_t (*resp)(const char *buf, size_t len))
{
	UNUSED_PARAM(data);
	UNUSED_PARAM(resp);

	return crypto_core_check_task(&g_crypto_core_socslt_task);
}
#endif
