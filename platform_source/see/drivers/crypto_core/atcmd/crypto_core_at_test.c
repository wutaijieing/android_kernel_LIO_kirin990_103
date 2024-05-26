/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: Crypto Core at command test function.
 * Create: 2020/6/1
 */
#ifdef CONFIG_DFX_DEBUG_FS

#include <linux/types.h>
#include <linux/kernel.h>
#include <securec.h>
#include <crypto_core_errno.h>
#include <crypto_core_factory.h>

#ifndef UNUSED_PARAM
#define UNUSED_PARAM(param)             (void)(param)
#endif

#define MSPC_AT_TEST_DATA_STR    "test_data"
#define MSPC_AT_TEST_RSP_DATA_OVERSIZED_STR  "0123456789abcdefghijklmnopqrstuvwxyz" \
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ0"
#define MSPC_AT_TEST_RESULT(scene_id, with_rsp_data, ret, rsp_data) \
	{ { scene_id, with_rsp_data }, ret, rsp_data }

/*
 * result               with response data
 * not running          yes or no
 * is running           yes or no
 * success              yes or no
 * fail                 yes or no
 */
enum mspc_at_test_scene_id {
	MSPC_AT_TEST_SCENE_INVALID    = 0,
	MSPC_AT_TEST_SCENE_NOTRUNNING = 1,
	MSPC_AT_TEST_SCENE_ISRUNNING  = 2,
	MSPC_AT_TEST_SCENE_SUCCESS    = 3,
	MSPC_AT_TEST_SCENE_FAIL       = 4,
};

enum mspc_at_test_rsp_data {
	MSPC_AT_TEST_WITHOUT_RSP_DATA        = 0,
	MSPC_AT_TEST_WITH_RSP_DATA           = 1,
	MSPC_AT_TEST_WITH_RSP_DATA_OVERSIZED = 2,
};

struct mspc_at_test_debug {
	enum mspc_at_test_scene_id scene_id;
	enum mspc_at_test_rsp_data with_rsp_data;
};

struct mspc_at_test_result {
	struct mspc_at_test_debug debug;
	int32_t ret;
	const char *rsp_data;
};

static DEFINE_MUTEX(g_mspc_at_test_lock);

static struct mspc_at_test_debug g_mspc_at_test_debug = {
	.scene_id = MSPC_AT_TEST_SCENE_INVALID,
	.with_rsp_data = MSPC_AT_TEST_WITHOUT_RSP_DATA,
};

static const struct mspc_at_test_result g_mspc_at_test_result[] = {
	/* NOT RUNNING */
	MSPC_AT_TEST_RESULT(MSPC_AT_TEST_SCENE_NOTRUNNING, MSPC_AT_TEST_WITHOUT_RSP_DATA,
			    MSPC_AT_CMD_NOTRUNNING_ERROR, NULL),
	MSPC_AT_TEST_RESULT(MSPC_AT_TEST_SCENE_NOTRUNNING, MSPC_AT_TEST_WITH_RSP_DATA,
			    MSPC_AT_CMD_NOTRUNNING_ERROR, "NOT RUNNING"),

	/* IS RUNNING */
	MSPC_AT_TEST_RESULT(MSPC_AT_TEST_SCENE_ISRUNNING, MSPC_AT_TEST_WITHOUT_RSP_DATA, MSPC_OK, NULL),
	MSPC_AT_TEST_RESULT(MSPC_AT_TEST_SCENE_ISRUNNING, MSPC_AT_TEST_WITH_RSP_DATA, MSPC_OK, "RUNNING"),

	/* SUCCESS */
	MSPC_AT_TEST_RESULT(MSPC_AT_TEST_SCENE_SUCCESS, MSPC_AT_TEST_WITHOUT_RSP_DATA, MSPC_OK, NULL),
	MSPC_AT_TEST_RESULT(MSPC_AT_TEST_SCENE_SUCCESS, MSPC_AT_TEST_WITH_RSP_DATA, MSPC_OK, "PASS"),
	MSPC_AT_TEST_RESULT(MSPC_AT_TEST_SCENE_SUCCESS, MSPC_AT_TEST_WITH_RSP_DATA_OVERSIZED,
			    MSPC_OK, MSPC_AT_TEST_RSP_DATA_OVERSIZED_STR),

	/* FAIL */
	MSPC_AT_TEST_RESULT(MSPC_AT_TEST_SCENE_FAIL, MSPC_AT_TEST_WITHOUT_RSP_DATA, MSPC_AT_TEST_ERROR, NULL),
	MSPC_AT_TEST_RESULT(MSPC_AT_TEST_SCENE_FAIL, MSPC_AT_TEST_WITH_RSP_DATA, MSPC_AT_TEST_ERROR, "FAIL")
};

int32_t mspc_at_test_debug(enum mspc_at_test_scene_id scene_id, enum mspc_at_test_rsp_data with_rsp_data)
{
	pr_info("%s scene_id %d, with_rsp_data %d\n", __func__, scene_id, with_rsp_data);
	mutex_lock(&g_mspc_at_test_lock);
	g_mspc_at_test_debug.scene_id = scene_id;
	g_mspc_at_test_debug.with_rsp_data = with_rsp_data;
	mutex_unlock(&g_mspc_at_test_lock);
	return 0;
}

static const struct mspc_at_test_result *mspc_at_get_test_result(void)
{
	const struct mspc_at_test_result *result = NULL;
	struct mspc_at_test_debug *debug = &g_mspc_at_test_debug;
	int32_t i;

	for (i = 0; i < (int32_t)ARRAY_SIZE(g_mspc_at_test_result); i++) {
		if (g_mspc_at_test_result[i].debug.scene_id == debug->scene_id &&
		    g_mspc_at_test_result[i].debug.with_rsp_data == debug->with_rsp_data) {
			result = &g_mspc_at_test_result[i];
			break;
		}
	}
	return result;
}

/*
 * @brief: Test MSPC at basic function.
 * @param[in]: data - MSPC at test input data.
 * @param[in]: resp - MSPC at response callback function.
 * @return: if success return MSPC_OK, others are failure.
 */
int32_t mspc_at_test(const char *data, int32_t (*resp)(const char *buf, size_t len))
{
	const struct mspc_at_test_result *result = NULL;
	int32_t ret;

	if (!data || (strcmp(data, MSPC_AT_TEST_DATA_STR) != 0)) {
		pr_err("%s invalid param data\n", __func__);
		return MSPC_INVALID_PARAMS_ERROR;
	}

	result = mspc_at_get_test_result();
	if (!result) {
		pr_err("%s result not found\n", __func__);
		return MSPC_AT_TEST_ERROR;
	}

	if (result->debug.with_rsp_data && result->rsp_data) {
		ret = resp(result->rsp_data, (size_t)(strlen(result->rsp_data) + 1));
		if (ret != MSPC_OK) {
			pr_err("%s resp fail, ret 0x%x\n", __func__, ret);
			return ret;
		}
	}
	return result->ret;
}
#endif
