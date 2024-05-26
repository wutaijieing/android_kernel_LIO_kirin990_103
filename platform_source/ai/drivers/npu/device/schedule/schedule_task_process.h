/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 * Description:
 * Author:
 * Create: 2021-07-08
 */
#ifndef __SCHEDULE_TASK_PROCESS_H
#define __SCHEDULE_TASK_PROCESS_H

#include <linux/types.h>
#include "schedule_stream.h"
#include "npu_model_description.h"
#include "task_pool.h"
#include "response.h"
#include "npu_rt_task.h"
#include "npu_hwts_plat.h"
#include "hwts_plat.h"
#include "hwts_config.h"
#include "schedule_interface.h"

#define SUCCESS 0

int sched_task_proc_event_record(struct schedule_stream *sched_stream, struct npu_rt_task *sched_task);
int sched_task_proc_event_wait(struct schedule_stream *sched_stream, struct npu_rt_task *sched_task);
int sched_task_proc_maintenance(struct schedule_stream *sched_stream, struct npu_rt_task *sched_task);
int sched_task_proc_model_maintenance(struct schedule_stream *sched_stream, struct npu_rt_task *sched_task);
int sched_task_proc_model_execute(struct schedule_stream *sched_stream, struct npu_rt_task *sched_task);
void sched_task_proc_hwts_sq_done(uint8_t irq_type, uint64_t channel_bitmap);
void sched_task_proc_hwts_error(uint8_t event_type, uint64_t channel_bitmap);
int sched_task_proc_init(void);
void sched_task_proc_deinit(void);
#endif