/*
 * schedule_stream.c
 *
 * about schedule stream
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
#include "schedule_stream.h"
#include "npu_log.h"

void sched_stream_init(struct schedule_stream *sched_stream, int32_t index, void *resp_queue)
{
	sched_stream->id = index;
	sched_stream->status = SCHED_STREAM_STATUS_IDLE;
	sched_stream->resp_queue = resp_queue;
	sched_stream->is_default_stream = 0;
	INIT_LIST_HEAD(&(sched_stream->sched_task_list));
}

void sched_stream_pop_task(struct schedule_stream *sched_stream)
{
	cond_return_void(sched_stream == NULL, "sched_stream is null");
	list_del(&(sched_stream->sched_task_list));
}

void sched_stream_push_task(struct schedule_stream *sched_stream, struct npu_id_entity *sched_task)
{
	cond_return_void(sched_stream == NULL, "sched_stream is null");
	cond_return_void(sched_task == NULL, "sched_task is null");
	list_add_tail(&(sched_task->list), &(sched_stream->sched_task_list));
}
