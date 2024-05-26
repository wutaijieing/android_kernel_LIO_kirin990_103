/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 * Description:
 * Author:
 * Create: 2021-07-08
 */
#ifndef __SCHEDULE_STREAM_ENGINE_H
#define __SCHEDULE_STREAM_ENGINE_H

#include <linux/list.h>
#include <linux/types.h>
#include "schedule_stream.h"
#include "task_pool.h"
#include "npu_common.h"
#include "npu_rt_task.h"
#include "task_pool.h"

typedef int (*handler)(struct schedule_stream *sched_stream, struct npu_rt_task *comm_task);

struct sched_task_proc_item {
	handler normal_proc;
	handler recycle_proc;
};

enum sched_task_status {
	SCHED_TASK_STATUS_FAIL = -1,
	SCHED_TASK_STATUS_DONE,
	SCHED_TASK_STATUS_RUNNING,
	SCHED_TASK_STATUS_MAX,
};

enum sched_stream_priority {
	SCHED_STREAM_PRIORITY_H,
	SCHED_STREAM_PRIORITY_L,
	SCHED_STREAM_PRIORITY_MAX,
};

struct sched_stream_mngr {
	struct list_head sched_stream_list[SCHED_STREAM_PRIORITY_MAX];
};

extern struct sched_stream_mngr g_sched_stream_mngr;

void sched_stream_engine_init(void);
int sched_stream_engine_register_handler(uint8_t task_type, handler fn);
void sched_stream_engine_push_stream(struct schedule_stream *sched_stream);
int sched_stream_engine_run(void);
#endif
