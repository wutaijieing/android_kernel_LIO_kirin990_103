/*
 *  Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *  Description: socid
 *  Create : 2021/08/17
 */
#include <linux/device.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/completion.h>
#include <platform_include/see/efuse_driver.h>
#include <teek_client_api.h>
#include <teek_client_id.h>
#include "socid.h"

#define SOCID_QUEUE_NAME         "socid"

struct socid_work_ctx {
	u8 *buf;
	u32 size;
	s32 result;
	struct completion completion;
	struct work_struct work;
	struct workqueue_struct *queue;
};

static void get_socid_work(struct work_struct *work)
{
	s32 ret;
	u32 origin = 0;
	TEEC_Operation operation = {0};
	TEEC_Result result;
	TEEC_Session session;
	TEEC_Context context;
	struct socid_work_ctx *ctx = NULL;

	pr_err("[%s] enter.\n", __func__);

	ctx = container_of(work, struct socid_work_ctx, work);

	ret = teek_init(&session, &context);
	if (ret != 0) {
		pr_err("error, teek_init 0x%x!\n", ret);
		ret = -EFAULT;
		goto exit_err;
	}

	operation.started = 1;
	operation.cancel_flag = 0;
	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_MEMREF_TEMP_OUTPUT,
			TEEC_NONE,
			TEEC_NONE,
			TEEC_NONE);
	operation.params[0].tmpref.buffer = ctx->buf;
	operation.params[0].tmpref.size = ctx->size;

	result = TEEK_InvokeCommand(
		&session,
		SECBOOT_CMD_ID_GET_SOCID,
		&operation,
		&origin);
	if (result != TEEC_SUCCESS) {
		pr_err("error, TEEK_InvokeCommand 0x%x!\n", result);
		ret = -EFAULT;
		goto exit_close;
	}
	ret = 0;
exit_close:
	TEEK_CloseSession(&session);
	TEEK_FinalizeContext(&context);

exit_err:
	pr_err("[%s]exit, ret=0x%x!\n", __func__, ret);
	ctx->result = ret;
	complete(&ctx->completion);
}

static s32 socid_work_ctx_init(
	struct socid_work_ctx *ctx, u8 *buf, u32 size)
{
	struct workqueue_struct *queue = NULL;

	queue = create_singlethread_workqueue(SOCID_QUEUE_NAME);
	if (IS_ERR_OR_NULL(queue)) {
		pr_err("%s, create_workqueue error\n", __func__);
		return -EFAULT;
	}

	ctx->buf = buf;
	ctx->size = size;
	ctx->queue = queue;
	INIT_WORK(&ctx->work, get_socid_work);
	init_completion(&ctx->completion);

	return 0;
}

static void socid_work_ctx_deinit(struct socid_work_ctx *ctx)
{
	destroy_workqueue(ctx->queue);
}

s32 get_socid(u8 *buf, u32 size)
{
	s32 ret;
	struct socid_work_ctx ctx = {0};

	if (IS_ERR_OR_NULL(buf) || size != EFUSE_SOCID_LENGTH_BYTES) {
		pr_err("%s: para invalid.\n", __func__);
			return -EFAULT;
	}

	ret = socid_work_ctx_init(&ctx, buf, size);
	if (ret != 0) {
		pr_err("error, init socid work %d!\n", ret);
		return ret;
	}

	queue_work(ctx.queue, &ctx.work);
	pr_err("wait socid work complete\n");
	wait_for_completion(&ctx.completion);

	ret = ctx.result;

	socid_work_ctx_deinit(&ctx);

	return ret;
}

