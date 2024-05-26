/*
 * schedule_interface.c
 *
 * about schedule interface
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include "schedule_interface.h"
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <securec.h>

#include "npu_common.h"
#include "schedule_task_process.h"
#include "schedule_stream_manager.h"
#include "schedule_stream_engine.h"
#include "npu_pm_framework.h"
#include "npu_comm_task_verify.h"
#include "npu_rt_task.h"
#include "npu_task_message.h"
#include "task_pool.h"
#include "response.h"
#include "npu_log.h"
#include "npu_svm.h"
#include "mutex_ops.h"

int32_t task_sched_init(void)
{
	int32_t ret = 0;
	ret = sched_task_proc_init();
	if (ret != 0)
		npu_drv_err("sched_task_proc_init error, ret:%d\n", ret);
	swts_mutex_create();

	return ret;
}

void task_sched_deinit(void)
{
	sched_task_proc_deinit();
	swts_mutex_destory();
}

static int32_t task_sched_start(struct npu_proc_ctx *proc_ctx, struct npu_id_entity *task)
{
	struct schedule_stream *sched_stream = NULL;
	struct npu_dev_ctx *cur_dev_ctx = get_dev_ctx_by_id(proc_ctx->devid);
	struct npu_rt_task *comm_task = (struct npu_rt_task *)(task->data);
	int32_t ret = 0;

	cond_return_error(cur_dev_ctx == NULL, -1, "cur_dev_ctx %d is null\n", proc_ctx->devid);
	down_read(&cur_dev_ctx->pm.exception_lock);

	cond_goto_error(cur_dev_ctx->pm.npu_exception_status == NPU_STATUS_EXCEPTION,
		NPU_EXCEPTION, ret, -1, "npu is in an abnormal state\n");

	/* npu power on */
	ret = npu_pm_proc_send_task_enter_wm(proc_ctx, cur_dev_ctx, NPU_NONSEC);
	cond_goto_error(ret != 0, PRE_START_FAIL, ret, ret, "first task : power up fail, ret= %d\n", ret);
	npu_drv_info("first task : power up succ, stream_id = %u, task_id:%u\n", comm_task->stream_id, comm_task->task_id);

	/* bind svm */
	if (comm_task->type == NPU_TASK_MODEL_EXECUTE) {
		ret = npu_svm_bind(cur_dev_ctx, current->tgid, comm_task->u.model_execute_task.execute_pid);
		cond_goto_error(ret != 0, PRE_START_FAIL, ret, ret, "npu_complete_comm_sqe fail, ret= %d\n", ret);
	}

	/* get the schedule stream */
	sched_stream = sched_stream_mngr_get_stream(comm_task->stream_id);
	cond_goto_error(sched_stream == NULL, PRE_START_FAIL, ret, ret, "sched_stream_mngr_get_stream fail, ret:%d", ret);
	sched_stream_push_task(sched_stream, task);

	cond_goto_nolog(sched_stream->status == SCHED_STREAM_STATUS_RUNNING, NPU_EXCEPTION, ret, 0);

	sched_stream_engine_push_stream(sched_stream);
	ret = sched_stream_engine_run();
	cond_goto_error(ret != 0, PRE_START_FAIL, ret, -1, "schedule stream engine run failed");

	up_read(&cur_dev_ctx->pm.exception_lock);
	return 0;

PRE_START_FAIL:
	(void)npu_pm_proc_release_task_exit_wm(proc_ctx, cur_dev_ctx, NPU_NONSEC, 1);
NPU_EXCEPTION:
	up_read(&cur_dev_ctx->pm.exception_lock);
	return ret;
}

int32_t task_sched_put_request(struct npu_proc_ctx *proc_ctx,  struct npu_rt_task *task)
{
	int32_t ret = 0;
	struct npu_id_entity *task_entity = NULL;
	cond_return_error(proc_ctx == NULL || task == NULL, -1, "invalid proc ctx or task");

	/* verify ts_sqe */
	ret = npu_verify_ts_sqe(proc_ctx, task);
	cond_return_error(ret != 0, -1, "npu_verify_ts_sqe fail, ret= %d", ret);

	task_entity = task_pool_alloc_task();
	cond_return_error(task_entity == NULL, -1, "alloc sched task_entity fail");

	swts_mutex_acquire();
	ret = memcpy_s(task_entity->data, sizeof(struct npu_rt_task), task, sizeof(struct npu_rt_task));
	cond_goto_error(ret != 0, FREE_TASK_POOL, ret, ret, "fail to copy task_entity, ret:%d", ret);

	ret = task_sched_start(proc_ctx, task_entity);
	cond_goto_error(ret != 0, FREE_TASK_POOL, ret, ret, "task_sched_start fail, ret:%d", ret);
	swts_mutex_release();
	return 0;

FREE_TASK_POOL:
	swts_mutex_release();
	(void)task_pool_free_task(task_entity);
	return ret;
}

static int32_t task_sched_copy_response_to_user(struct npu_receive_response_info *report_info, struct response *report)
{
	int32_t ret = 0;
	report_info->response_count = 1;
	report_info->wait_result = 1;
	ret = copy_to_user_safe((void *)(uintptr_t)report_info->response_addr, report, sizeof(struct response));
	if (ret != 0) {
		npu_drv_err("copy report to user fail ret %d\n", ret);
		return ret;
	}
	return 0;
}

void task_sched_set_sched_stream_status(void)
{
	uint32_t idx;
	struct list_head *sched_stream_list = NULL;
	struct schedule_stream *sched_stream = NULL;
	struct schedule_stream *next = NULL;
	struct npu_id_entity *sched_task = NULL;

	for (idx = 0; idx < SCHED_STREAM_PRIORITY_MAX; idx++) {
		sched_stream_list = &g_sched_stream_mngr.sched_stream_list[idx];
		list_for_each_entry_safe(sched_stream, next, sched_stream_list, node) {
			if (sched_stream->status == SCHED_STREAM_STATUS_RUNNING) {
				break;
			}
		}
	}
	cond_return_void(sched_stream == NULL, "sched_stream is null\n");

	sched_stream->status = SCHED_STREAM_STATUS_IDLE;

	return;
}

int32_t task_sched_get_response(struct npu_proc_ctx *proc_ctx, struct npu_receive_response_info *report_info)
{
	struct response report;
	int32_t ret = 0;
	struct npu_dev_ctx *cur_dev_ctx = NULL;
	int32_t wait_time = -1;
	unsigned long tmp = 0;
	struct response_queue *resp_queue = NULL;

	cond_return_error(proc_ctx == NULL, -1, "invalid proc ctx\n");
	cond_return_error(report_info == NULL, -1, "invalid report info\n");

	cur_dev_ctx = get_dev_ctx_by_id(proc_ctx->devid);
	cond_return_error(cur_dev_ctx == NULL, -1, "cur_dev_ctx is null, devid = %u\n", proc_ctx->devid);
	resp_queue = proc_ctx->resp_queue;
	ret = resp_queue_pop_front(resp_queue, &report);
	if (ret == 0) {
		(void)npu_pm_proc_release_task_exit_wm(proc_ctx, cur_dev_ctx, NPU_NONSEC, 1);
		return task_sched_copy_response_to_user(report_info, &report);
	}

#ifdef NPU_FPGA_PLAT
	wait_time = cur_dev_ctx->pm.npu_task_time_out * 20;
#else
	wait_time = cur_dev_ctx->pm.npu_task_time_out;
#endif
	tmp = msecs_to_jiffies(wait_time);
	ret = wait_event_timeout(resp_queue->report_wait, (!resp_queue_is_empty(resp_queue)) ||
		(cur_dev_ctx->pm.npu_exception_status == NPU_STATUS_EXCEPTION &&
			cur_dev_ctx->pm.npu_pm_vote == NPU_PM_VOTE_STATUS_PD), tmp);
	if (ret == 0) {
		npu_drv_info("get response wait timeout, need powerdown!!!\n");
		npu_exception_timeout(cur_dev_ctx);
	}

	ret = resp_queue_pop_front(resp_queue, &report);
	if (ret == 0) {
		(void)npu_pm_proc_release_task_exit_wm(proc_ctx, cur_dev_ctx, NPU_NONSEC, 1);
		return task_sched_copy_response_to_user(report_info, &report);
	}

	task_sched_set_sched_stream_status();
	report_info->response_count = 1;

	return ret;
}
