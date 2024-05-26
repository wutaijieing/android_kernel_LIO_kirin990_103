/*
 * schedule_stream_engine.c
 *
 * Copyright (c) 2012-2021 Huawei Technologies Co., Ltd.
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

#include "schedule_stream_engine.h"
#include "linux/errno.h"
#include "linux/time.h"
#include "npu_log.h"
#include "response.h"
#include "npu_proc_ctx.h"

struct sched_task_proc_item g_handler_table[NPU_TASK_RESERVED];
struct sched_stream_mngr g_sched_stream_mngr;

void sched_stream_engine_init(void)
{
	uint16_t idx;
	for (idx = 0; idx < SCHED_STREAM_PRIORITY_MAX; idx++)
		INIT_LIST_HEAD(&g_sched_stream_mngr.sched_stream_list[idx]);
}

int sched_stream_engine_register_handler(uint8_t task_type, handler fn)
{
	cond_return_error(task_type >= NPU_TASK_RESERVED, -1, "task type exceeds the maximum");

	g_handler_table[task_type].normal_proc = fn;
	return 0;
}

static handler sched_stream_engine_get_handler(uint16_t task_type)
{
	cond_return_error(task_type >= NPU_TASK_RESERVED, NULL, "task type exceeds the maximum");
	return g_handler_table[task_type].normal_proc;
}

void sched_stream_engine_push_stream(struct schedule_stream *sched_stream)
{
	struct list_head *sched_stream_list = NULL;

	if (sched_stream->is_default_stream) {
		sched_stream_list = &g_sched_stream_mngr.sched_stream_list[SCHED_STREAM_PRIORITY_H];
	} else {
		sched_stream_list = &g_sched_stream_mngr.sched_stream_list[SCHED_STREAM_PRIORITY_L];
	}

	list_add_tail(&sched_stream->node, sched_stream_list);
}

static int sched_stream_engine_run_task(struct schedule_stream * sched_stream, struct npu_id_entity *sched_task)
{
	int sched_task_status = SCHED_TASK_STATUS_FAIL;
	handler fn = NULL;
	int ret = SCHED_TASK_STATUS_FAIL;
	struct npu_rt_task *comm_task = (struct npu_rt_task*)(sched_task->data);

	npu_drv_info("run task, task_id = %u, task_type=%u, stream_id=%u",
		comm_task->task_id, comm_task->type, comm_task->stream_id);
	cond_return_error(comm_task->type >= NPU_TASK_RESERVED, SCHED_TASK_STATUS_FAIL,
		"task type exceeds the maximum");

	fn = sched_stream_engine_get_handler(comm_task->type);
	cond_return_error(fn == NULL, SCHED_TASK_STATUS_FAIL,
		"hanlder is null, task_id = %u, task_type = %u", comm_task->task_id, comm_task->type);

	ret = fn(sched_stream, comm_task);
	if (ret == SCHED_TASK_STATUS_FAIL)
		npu_drv_err("hanlder run error, task_id = %u, task_type = %u, stream_id = %u",
			comm_task->task_id, comm_task->type, sched_stream->id);

	return ret;
}

static int sched_stream_engine_run_task_list(struct schedule_stream *sched_stream)
{
	int ret;
	struct npu_id_entity *sched_task = NULL;
	struct npu_id_entity *next = NULL;
	struct list_head *sched_task_list = NULL;
	int sched_task_status;
	struct response resp;
	uint64_t result;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct timespec64 time;
#else
	struct timespec time;
#endif

	sched_task_list = &(sched_stream->sched_task_list);
	list_for_each_entry_safe(sched_task, next, sched_task_list, list) {
		sched_task_status = sched_stream_engine_run_task(sched_stream, sched_task);
		if (sched_task_status == SCHED_TASK_STATUS_RUNNING) {
			sched_stream->status = SCHED_STREAM_STATUS_RUNNING;
			npu_drv_info("schedule task running, task_id = %u", ((struct npu_rt_task*)(sched_task->data))->task_id);
			return 0;
		}

		list_del_init(&sched_task->list);
		resp.stream_id = ((struct npu_rt_task*)(sched_task->data))->stream_id;
		resp.task_id = ((struct npu_rt_task*)(sched_task->data))->task_id;
		result = (sched_task_status == SCHED_TASK_STATUS_DONE ? 0 : 1);
		if (((struct npu_rt_task*)(sched_task->data))->type == NPU_TASK_EVENT_RECORD) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
			ktime_get_coarse_real_ts64(&time);
#else
			time = current_kernel_time();
#endif
			resp.payload = (uint64_t)(time.tv_sec * 1000000 + time.tv_nsec / 1000);;
		} else {
			resp.payload = result;
		}
		ret = resp_queue_push_back(sched_stream->resp_queue, &resp);
		cond_return_error(ret != 0, -1, "response push queue failed");

		ret = task_pool_free_task(sched_task);
		cond_return_error(ret != 0, -1, "free task failed, task pool id = %u", sched_task->id);
	}

	if (list_empty_careful(sched_task_list)) {
		npu_drv_info("schedule stream task finish");
		list_del(&sched_stream->node);
		sched_stream->status = SCHED_STREAM_STATUS_IDLE;
	}

	return 0;
}

int sched_stream_engine_run(void)
{
	int ret;
	uint32_t idx;
	struct list_head *sched_stream_list = NULL;
	struct schedule_stream *sched_stream = NULL;

	for (idx = 0; idx < SCHED_STREAM_PRIORITY_MAX; idx++) {
		sched_stream_list = &g_sched_stream_mngr.sched_stream_list[idx];
		sched_stream = list_first_entry(sched_stream_list, typeof(struct schedule_stream), node);
		while (&sched_stream->node != sched_stream_list) {
			ret = sched_stream_engine_run_task_list(sched_stream);
			cond_return_error(ret != 0, -1, "schedule stream failed");
			sched_stream = list_first_entry(sched_stream_list, typeof(struct schedule_stream), node);
		}
	}

	return 0;
}
