/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Drivers for Crypto Core factory task.
 * Create: 2021/06/02
 */
#include "crypto_core_factory_task.h"
#include <linux/kthread.h>
#include <linux/mutex.h>
#include "crypto_core_errno.h"

static void crypto_core_set_task_state(struct crypto_core_task *task, enum mspc_factory_test_state state)
{
	atomic_set(&task->state, (int)state);
}

static enum mspc_factory_test_state crypto_core_get_task_state(struct crypto_core_task *task)
{
	return (enum mspc_factory_test_state)atomic_read(&task->state);
}

static void crypto_core_set_task_errcode(struct crypto_core_task *task, int32_t errcode)
{
	atomic_set(&task->errcode, errcode);
}

static int32_t crypto_core_get_task_errcode(struct crypto_core_task *task)
{
	return atomic_read(&task->errcode);
}

static void crypto_core_set_task_state_and_errcode(struct crypto_core_task *task,
						   enum mspc_factory_test_state state, int32_t errcode)
{
	mutex_lock(task->lock);
	crypto_core_set_task_state(task, state);
	crypto_core_set_task_errcode(task, errcode);
	mutex_unlock(task->lock);
}

static void crypto_core_get_task_state_and_errcode(struct crypto_core_task *task,
						   enum mspc_factory_test_state *state, int32_t *errcode)
{
	mutex_lock(task->lock);
	*state = crypto_core_get_task_state(task);
	*errcode = crypto_core_get_task_errcode(task);
	mutex_unlock(task->lock);
}

static int32_t crypto_core_check_task_param(struct crypto_core_task *task)
{
	if (!task || !task->lock)
		return MSPC_INVALID_PARAM_ERROR;
	return MSPC_OK;
}

static int32_t crypto_core_run_task(void *data)
{
	struct crypto_core_task *task = (struct crypto_core_task *)data;
	enum mspc_factory_test_state state;
	int32_t try_cnt;
	int32_t ret;

	if (crypto_core_check_task_param(task) != MSPC_OK) {
		pr_err("%s: data err\n", __func__);
		return MSPC_INVALID_PARAM_ERROR;
	}

	state = crypto_core_get_task_state(task);
	if (state != MSPC_FACTORY_TEST_RUNNING) {
		pr_err("%s: state error: %x\n", __func__, state);
		crypto_core_set_task_state_and_errcode(task, MSPC_FACTORY_TEST_FAIL, MSPC_FACTORY_TASK_STATE_ERROR);
		return MSPC_FACTORY_TASK_STATE_ERROR;
	}

	try_cnt = task->try_cnt;
	do {
		ret = (task->taskfn) ? task->taskfn() : MSPC_OK;
		if (ret == MSPC_OK)
			break;
		try_cnt--;
	} while (try_cnt > 0);

	if (ret != MSPC_OK) {
		pr_err("%s:run failed!\n", __func__);
		crypto_core_set_task_state_and_errcode(task, MSPC_FACTORY_TEST_FAIL, ret);
	} else {
		pr_info("%s:run successful!\n", __func__);
		crypto_core_set_task_state_and_errcode(task, MSPC_FACTORY_TEST_SUCCESS, ret);
	}

	return ret;
}

int32_t crypto_core_create_task(struct crypto_core_task *task)
{
	struct task_struct *thread = NULL;

	if (crypto_core_check_task_param(task) != MSPC_OK)
		return MSPC_INVALID_PARAM_ERROR;

	if (task->conditon_fn && !task->conditon_fn()) {
		pr_err("%s:task %s conditon unsatisfied\n", __func__, task->name);
		return MSPC_OK;
	}

	mutex_lock(task->lock);
	if (crypto_core_get_task_state(task) == MSPC_FACTORY_TEST_RUNNING) {
		pr_err("%s:task already run!\n", __func__);
		mutex_unlock(task->lock);
		return MSPC_OK;
	}
	crypto_core_set_task_state(task, MSPC_FACTORY_TEST_RUNNING);
	mutex_unlock(task->lock);

	thread = kthread_run(crypto_core_run_task, (void *)task, "%s", task->name);
	if (!thread) {
		pr_err("%s:create %s failed!\n", __func__, task->name);
		crypto_core_set_task_state_and_errcode(task, MSPC_FACTORY_TEST_FAIL, MSPC_THREAD_CREATE_ERROR);
		return MSPC_THREAD_CREATE_ERROR;
	}

	return MSPC_OK;
}

int32_t crypto_core_check_task(struct crypto_core_task *task)
{
	enum mspc_factory_test_state state = MSPC_FACTORY_TEST_NORUNNING;
	int32_t errcode = MSPC_ERROR;

	if (crypto_core_check_task_param(task) != MSPC_OK)
		return MSPC_INVALID_PARAM_ERROR;

	crypto_core_get_task_state_and_errcode(task, &state, &errcode);

	if (state == MSPC_FACTORY_TEST_NORUNNING)
		return MSPC_AT_CMD_NOTRUNNING_ERROR;

	if (state == MSPC_FACTORY_TEST_RUNNING)
		return MSPC_FACTORY_TASK_RUNNING_ERROR;

	if (state == MSPC_FACTORY_TEST_SUCCESS && errcode == MSPC_OK)
		return MSPC_OK;

	if (state == MSPC_FACTORY_TEST_FAIL && errcode != MSPC_OK)
		return errcode;

	pr_err("%s: failed, state %d, errcode 0x%x\n", __func__, (int32_t)state, errcode);
	return MSPC_FACTORY_TASK_STATE_ERROR;
}
