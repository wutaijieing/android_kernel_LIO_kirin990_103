/*
 * schedule_stream.h
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

#ifndef _SCHEDULE_STREAM_H_
#define _SCHEDULE_STREAM_H_

#include <linux/list.h>
#include <linux/types.h>
#include "task_pool.h"

enum sched_stream_status {
	SCHED_STREAM_STATUS_IDLE,
	SCHED_STREAM_STATUS_RUNNING
};

struct schedule_stream {
	int32_t id;
	struct list_head node;
	void *resp_queue;
	struct list_head sched_task_list;
	uint8_t is_default_stream;
	uint8_t status;
};

void sched_stream_init(struct schedule_stream *sched_stream, int32_t index, void *resp_queue);
void sched_stream_pop_task(struct schedule_stream *sched_stream);
void sched_stream_push_task(struct schedule_stream *sched_stream, struct npu_id_entity *sched_task);
#endif
