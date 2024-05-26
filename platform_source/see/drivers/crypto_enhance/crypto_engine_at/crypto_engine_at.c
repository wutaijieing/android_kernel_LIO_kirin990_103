/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: crypto engine at command common function.
 * Create: 2020/4/9
 */
#include "crypto_engine_at.h"
#include "crypto_engine_at_smx.h"
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <securec.h>
#include <crypto_enhance.h>

#define MSPE_AT_REQ_BASIC_PARAMS_NUM        2
#define MSPE_AT_REQ_EXTEND_PARAMS_NUM       3
#define MSPE_AT_REQ_PARAM_OBJECT_MAX_LEN    16 /* maximum string length of the object param */
#define MSPE_AT_REQ_PARAM_VALUE_MAX_LEN     32 /* maximum string length of the value param */
#define MSPE_AT_RSP_SUCCESS_STRING          "SUCCESS"
#define MSPE_AT_RSP_FAIL_STRING             "FAIL"
#define MSPE_AT_RSP_UNSUPPORT_STRING        "UNSUPPORT"
#define MSPE_AT_RSP_DATA_LEN                128

#define mspe_at_register_command(obj, set, get, exec, check) \
	{ obj, set, get, exec, check }

#define mspe_at_execute_command(object, ops, func, value)                                           \
	do {                                                                                        \
		if (func) {                                                                         \
			ret = func(value, mspe_at_set_resp_data);                                   \
		} else {                                                                            \
			pr_err("%s: object %s, ops %d, cmd func is null\n", __func__, object, ops); \
			ret = MSPE_AT_CMD_FUNC_NULL_ERROR;                                          \
		}                                                                                   \
	} while (0)

/* Don't modify the enum value, when you add new ops type. */
enum mspe_at_ops_type {
	MSPE_AT_SET   = 0,
	MSPE_AT_GET   = 1,
	MSPE_AT_EXEC  = 2,
	MSPE_AT_CHECK = 3,
};

struct mspe_at_object_command {
	const char *object;
	int32_t (*set)(const char *value, int32_t (*resp)(char *buf, int32_t len));
	int32_t (*get)(const char *value, int32_t (*resp)(char *buf, int32_t len));
	int32_t (*exec)(const char *value, int32_t (*resp)(char *buf, int32_t len));
	int32_t (*check)(const char *value, int32_t (*resp)(char *buf, int32_t len));
};

struct mspe_at_response {
	int32_t errcode;
	char data[MSPE_AT_RSP_DATA_LEN];
};

static DEFINE_MUTEX(g_mspe_at_lock);

static const struct mspe_at_object_command g_mspe_at_obj_cmd_list[] = {
	mspe_at_register_command("SMX", mspe_at_set_smx, mspe_at_get_smx, NULL, NULL),
};

static struct mspe_at_response g_mspe_at_resp = {0};

static void mspe_at_set_resp_errcode(int32_t errcode)
{
	mutex_lock(&g_mspe_at_lock);
	g_mspe_at_resp.errcode = errcode;
	mutex_unlock(&g_mspe_at_lock);
}

static void mspe_at_clr_resp_data(void)
{
	mutex_lock(&g_mspe_at_lock);
	(void)memset_s((void *)g_mspe_at_resp.data, sizeof(g_mspe_at_resp.data), 0,
		       sizeof(g_mspe_at_resp.data));
	mutex_unlock(&g_mspe_at_lock);
}

static int32_t mspe_at_set_resp_data(char *buf, int32_t len)
{
	int32_t ret;

	pr_info("%s: entry", __func__);

	/* not need response, only memset response buffer zero. */
	if (!buf || len == 0) {
		mspe_at_clr_resp_data();
		return MSPE_OK;
	}

	mutex_lock(&g_mspe_at_lock);
	ret = memcpy_s(g_mspe_at_resp.data, sizeof(g_mspe_at_resp.data), buf, len);
	mutex_unlock(&g_mspe_at_lock);
	if (ret != EOK) {
		pr_err("%s: memcpy_s fail,ret=%d\n", __func__, ret);
		return MSPE_AT_MEMCPY_ERROR;
	}
	return MSPE_OK;
}

static const struct mspe_at_object_command *mspe_at_get_obj_cmd(const char *object)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(g_mspe_at_obj_cmd_list); i++) {
		if (strncmp(object, g_mspe_at_obj_cmd_list[i].object,
			    MSPE_AT_REQ_PARAM_OBJECT_MAX_LEN) == 0)
			return &g_mspe_at_obj_cmd_list[i];
	}

	return NULL;
}

static int32_t mspe_at_exec_cmd(const char *object, int32_t ops, const char *value)
{
	const struct mspe_at_object_command *obj_cmd = NULL;
	int32_t ret;

	obj_cmd = mspe_at_get_obj_cmd(object);
	if (!obj_cmd) {
		pr_err("%s: obj command not found\n", __func__);
		return MSPE_AT_OBJ_NOT_FOUND_ERROR;
	}

	switch (ops) {
	case MSPE_AT_SET:
		mspe_at_execute_command(object, ops, obj_cmd->set, value);
		break;
	case MSPE_AT_GET:
		mspe_at_execute_command(object, ops, obj_cmd->get, value);
		break;
	case MSPE_AT_EXEC:
		mspe_at_execute_command(object, ops, obj_cmd->exec, value);
		break;
	case MSPE_AT_CHECK:
		mspe_at_execute_command(object, ops, obj_cmd->check, value);
		break;
	default:
		ret = MSPE_AT_OPS_NOT_FOUND_ERROR;
		pr_err("%s: invalid ops, object %s, ops %d\n", __func__, object, ops);
		break;
	};

	if (ret != MSPE_OK)
		pr_err("%s: execute [%s, %d] fail, ret 0x%x\n", __func__, object, ops, ret);

	return ret;
}

ssize_t mspe_at_ctrl_store(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	char object[MSPE_AT_REQ_PARAM_OBJECT_MAX_LEN] = {0};
	int32_t ops = 0;
	char value[MSPE_AT_REQ_PARAM_VALUE_MAX_LEN] = {0};
	int32_t param_num;
	int32_t ret;

	pr_info("%s: entry, buf %s, count %d", __func__, buf, (int)count);
	param_num = sscanf_s(buf, "%s %d %s", object, sizeof(object), &ops, value, sizeof(value));
	if (param_num != (int32_t)MSPE_AT_REQ_BASIC_PARAMS_NUM &&
	    param_num != (int32_t)MSPE_AT_REQ_EXTEND_PARAMS_NUM) {
		pr_err("%s: param_num %d\n", __func__, param_num);
		return -EINVAL;
	}

	pr_err("%s: object %s, ops %d, value %s\n", __func__, object, ops, value);
	mspe_at_clr_resp_data();
	if (param_num == MSPE_AT_REQ_BASIC_PARAMS_NUM)
		value[0] = '\0';
	ret = mspe_at_exec_cmd(object, ops, value);
	mspe_at_set_resp_errcode(ret);

	return count;
}

ssize_t mspe_at_ctrl_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	const char *rsp_str = NULL;
	int32_t errcode;
	char data[sizeof(g_mspe_at_resp.data)];

	pr_info("%s: entry", __func__);

	mutex_lock(&g_mspe_at_lock);
	errcode = g_mspe_at_resp.errcode;
	(void)memcpy_s(data, sizeof(data),
		       g_mspe_at_resp.data, sizeof(g_mspe_at_resp.data));
	mutex_unlock(&g_mspe_at_lock);

	if (errcode == MSPE_OK)
		rsp_str = MSPE_AT_RSP_SUCCESS_STRING;
	else if (errcode == MSPE_AT_OBJ_NOT_FOUND_ERROR ||
		 errcode == MSPE_AT_OPS_NOT_FOUND_ERROR)
		rsp_str = MSPE_AT_RSP_UNSUPPORT_STRING;
	else
		rsp_str = MSPE_AT_RSP_FAIL_STRING;

	return scnprintf(buf, PAGE_SIZE, "%s 0x%x %s", rsp_str, errcode, data);
}
