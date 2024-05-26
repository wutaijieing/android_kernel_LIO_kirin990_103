/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: Crypto Core at command common function.
 * Create: 2020/4/9
 */
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <securec.h>
#include "crypto_core_at_install.h"
#include "crypto_core_at_smx.h"
#ifdef CONFIG_DFX_DEBUG_FS
#include "crypto_core_at_socslt.h"
#include "crypto_core_at_test.h"
#ifdef CONFIG_CRYPTO_CORE_DISABLE_KEY
#include "crypto_core_disable.h"
#endif
#endif
#include "crypto_core_errno.h"

/* MSPC at request information */
#define MSPC_AT_REQ_BASIC_PARAMS_NUM       2
#define MSPC_AT_REQ_EXTEND_PARAMS_NUM      3
#define MSPC_AT_REQ_PARAM_OBJECT_MAX_LEN   8  /* maximum string length of the object param */
#define MSPC_AT_REQ_PARAM_DATA_MAX_LEN     32 /* maximum string length of the data param */

/* MSPC at response information */
#define MSPC_AT_RSP_UNSUPPORT_STR          "UNSUPPORT"
#define MSPC_AT_RSP_NOTRUNNING_STR         "NT" /* NOT TEST */
#define MSPC_AT_RSP_SUCCESS_STR            "SUCCESS"
#define MSPC_AT_RSP_FAIL_STR               "FAIL"
#define MSPC_AT_RSP_DATA_LEN               64

/* MSPC at command handler function type */
typedef int32_t (*mspc_at_cmd_handler_t)(const char *data, int32_t (*resp)(const char *buf, size_t len));

#define MSPC_AT_INSTALL_OBJECT   "INSTALL"
#define MSPC_AT_SMX_OBJECT       "SMX"
#define MSPC_AT_SOCSLT_OBJECT    "SOCSLT"
#define MSPC_AT_TEST_OBJECT      "TEST"
#define MSPC_AT_DISABLE_OBJECT   "DISABLE"

/* Don't modify the enum value, when you add new ops type. */
enum mspc_at_ops_type {
	MSPC_AT_SET_OPS   = 0,
	MSPC_AT_GET_OPS   = 1,
	MSPC_AT_EXEC_OPS  = 2,
	MSPC_AT_CHECK_OPS = 3,
	MSPC_AT_OPS_MAX
};

/* MSPC at command defines */
#define MSPC_AT_REGISTER_COMMAND(obj, ops, handler) \
	{ obj, ops, handler }

struct mspc_at_command {
	const char *object;
	int32_t ops;
	mspc_at_cmd_handler_t handler;
};

struct mspc_at_response {
	int32_t errcode;
	char data[MSPC_AT_RSP_DATA_LEN];
};

static DEFINE_MUTEX(g_mspc_at_lock);

static const struct mspc_at_command g_mspc_at_cmd_list[] = {
	/* SMX */
	MSPC_AT_REGISTER_COMMAND(MSPC_AT_SMX_OBJECT, MSPC_AT_SET_OPS, mspc_at_set_smx),
	MSPC_AT_REGISTER_COMMAND(MSPC_AT_SMX_OBJECT, MSPC_AT_GET_OPS, mspc_at_get_smx),

	/* INSTALL */
	MSPC_AT_REGISTER_COMMAND(MSPC_AT_INSTALL_OBJECT, MSPC_AT_EXEC_OPS, mspc_at_execute_install),
	MSPC_AT_REGISTER_COMMAND(MSPC_AT_INSTALL_OBJECT, MSPC_AT_CHECK_OPS, mspc_at_check_install),

#ifdef CONFIG_DFX_DEBUG_FS
	/* SOCSLT */
	MSPC_AT_REGISTER_COMMAND(MSPC_AT_SOCSLT_OBJECT, MSPC_AT_EXEC_OPS, mspc_at_execute_socslt),
	MSPC_AT_REGISTER_COMMAND(MSPC_AT_SOCSLT_OBJECT, MSPC_AT_CHECK_OPS, mspc_at_check_socslt),

	/* TEST */
	MSPC_AT_REGISTER_COMMAND(MSPC_AT_TEST_OBJECT, MSPC_AT_SET_OPS, mspc_at_test),
	MSPC_AT_REGISTER_COMMAND(MSPC_AT_TEST_OBJECT, MSPC_AT_GET_OPS, mspc_at_test),
	MSPC_AT_REGISTER_COMMAND(MSPC_AT_TEST_OBJECT, MSPC_AT_EXEC_OPS, mspc_at_test),
	MSPC_AT_REGISTER_COMMAND(MSPC_AT_TEST_OBJECT, MSPC_AT_CHECK_OPS, mspc_at_test),

#ifdef CONFIG_CRYPTO_CORE_DISABLE_KEY
	/* DISABLE */
	MSPC_AT_REGISTER_COMMAND(MSPC_AT_DISABLE_OBJECT, MSPC_AT_EXEC_OPS, crypto_core_execute_disable),
	MSPC_AT_REGISTER_COMMAND(MSPC_AT_DISABLE_OBJECT, MSPC_AT_CHECK_OPS, crypto_core_check_disable),
#endif
#endif
};

static struct mspc_at_response g_mspc_at_resp = {0};

static void mspc_at_set_resp_errcode(int32_t errcode)
{
	mutex_lock(&g_mspc_at_lock);
	g_mspc_at_resp.errcode = errcode;
	mutex_unlock(&g_mspc_at_lock);
}

static void mspc_at_clr_resp_data(void)
{
	mutex_lock(&g_mspc_at_lock);
	(void)memset_s((void *)g_mspc_at_resp.data, sizeof(g_mspc_at_resp.data), 0,
		       sizeof(g_mspc_at_resp.data));
	mutex_unlock(&g_mspc_at_lock);
}

/*
 * @brief: Set MSPC at response data.
 * @param[in]: buf - MSPC at command response data buffer.
 * @param[in]: len - MSPC at command response data buffer length.
 * @return: if success return MSPC_OK, others are failure.
 */
static int32_t mspc_at_set_resp_data(const char *buf, size_t len)
{
	int32_t ret;

	if (!buf || len == 0)
		return MSPC_AT_SET_RESP_DATA_ERROR;

	mutex_lock(&g_mspc_at_lock);
	ret = memcpy_s(g_mspc_at_resp.data, sizeof(g_mspc_at_resp.data) - 1, buf, len);
	mutex_unlock(&g_mspc_at_lock);
	if (ret != EOK) {
		pr_err("%s: memcpy_s fail\n", __func__);
		return MSPC_AT_SET_RESP_DATA_ERROR;
	}
	return MSPC_OK;
}

/*
 * @brief: According to object and ops, get MSPC at command handler function.
 * @param[in]: object - MSPC at command object name.
 * @param[in]: ops - MSPC at command operation type.
 * @return: if success return MSPC at command handler function, NULL are failure.
 */
static mspc_at_cmd_handler_t mspc_at_get_cmd_handler(const char *object, int32_t ops)
{
	mspc_at_cmd_handler_t handler = NULL;
	int32_t i;

	for (i = 0; i < (int32_t)ARRAY_SIZE(g_mspc_at_cmd_list); i++) {
		if (strncmp(object, g_mspc_at_cmd_list[i].object, MSPC_AT_REQ_PARAM_OBJECT_MAX_LEN) == 0 &&
		    ops == g_mspc_at_cmd_list[i].ops) {
			handler = g_mspc_at_cmd_list[i].handler;
			break;
		}
	}

	return handler;
}

/*
 * @brief: Execute MSPC at command.
 * @param[in]: object - MSPC at command object name.
 * @param[in]: ops - MSPC at command operation type.
 * @param[in]: data - MSPC at command input data, maybe NULL.
 */
static void mspc_at_excute_command(const char *object, int32_t ops, const char *data)
{
	mspc_at_cmd_handler_t handler = NULL;
	int32_t ret;

	mspc_at_clr_resp_data();
	handler = mspc_at_get_cmd_handler(object, ops);
	if (!handler) {
		mspc_at_set_resp_errcode(MSPC_AT_CMD_UNSUPPORT_ERROR);
		return;
	}

	ret = handler(data, mspc_at_set_resp_data);
	if (ret != MSPC_OK)
		pr_err("%s: execute [%s, %d] fail, ret 0x%x\n", __func__, object, ops, ret);
	mspc_at_set_resp_errcode(ret);
}

/*
 * @brief: Convert error code to response string.
 * @param[in]: errcode - error code.
 * @return: the response string corresponding to errcode.
 */
static const char *mspc_at_errcode2str(int32_t errcode)
{
	const char *str = NULL;

	switch (errcode) {
	case MSPC_AT_CMD_UNSUPPORT_ERROR:
		str = MSPC_AT_RSP_UNSUPPORT_STR;
		break;
	case MSPC_AT_CMD_NOTRUNNING_ERROR:
		str = MSPC_AT_RSP_NOTRUNNING_STR;
		break;
	case MSPC_OK:
		str = MSPC_AT_RSP_SUCCESS_STR;
		break;
	default:
		str = MSPC_AT_RSP_FAIL_STR;
		break;
	}
	return str;
}

/*
 * @brief: MSPC at ctrl device node store function.
 * @param[in]: dev - device node.
 * @param[in]: attr - device attribute.
 * @param[in]: buf - MSPC at ctrl input buffer, which save request content.
 * @param[in]: count - MSPC at ctrl input buffer length.
 * @return: if success return count, others are failure.
 */
ssize_t mspc_at_ctrl_store(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	char object[MSPC_AT_REQ_PARAM_OBJECT_MAX_LEN] = {0};
	int32_t ops = 0;
	char data[MSPC_AT_REQ_PARAM_DATA_MAX_LEN] = {0};
	int32_t param_num;

	if (!buf || count == 0)
		return -EINVAL;

	pr_info("%s: entry, buf %s, count %d", __func__, buf, (int32_t)count);
	param_num = sscanf_s(buf, "%s %d %s", object, sizeof(object), &ops, data, sizeof(data));
	if (param_num != (int32_t)MSPC_AT_REQ_BASIC_PARAMS_NUM &&
	    param_num != (int32_t)MSPC_AT_REQ_EXTEND_PARAMS_NUM) {
		pr_err("%s: param_num %d\n", __func__, param_num);
		return -EINVAL;
	}

	pr_info("%s: object %s, ops %d, data %s\n", __func__, object, ops, data);
	if (param_num == MSPC_AT_REQ_BASIC_PARAMS_NUM)
		mspc_at_excute_command(object, ops, NULL);
	else
		mspc_at_excute_command(object, ops, data);

	return (ssize_t)count;
}

/*
 * @brief: MSPC at ctrl device node show function.
 * @param[in]: dev - device node.
 * @param[in]: attr - device attribute.
 * @param[out]: buf - MSPC at ctrl output buffer which save response content.
 * @return: the data length written to the buffer.
 */
ssize_t mspc_at_ctrl_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	const char *rsp_str = mspc_at_errcode2str(g_mspc_at_resp.errcode);

	if (!buf || !rsp_str)
		return -EINVAL;

	return scnprintf(buf, PAGE_SIZE, "%s 0x%08x %s", rsp_str, g_mspc_at_resp.errcode, g_mspc_at_resp.data);
}
