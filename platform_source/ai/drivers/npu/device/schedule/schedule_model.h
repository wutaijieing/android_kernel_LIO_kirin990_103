/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 * Description:
 * Author:
 * Create: 2021-07-08
 */
#ifndef __SCHEDULE_MODEL_H
#define __SCHEDULE_MODEL_H

#include <linux/types.h>
#include "hwts_sq_cq.h"
#include "schedule_stream.h"
#include "hwts_event_manager.h"
#include "hwts_config.h"

enum sched_model_status{
	SCHED_MODEL_STATUS_IDLE,
	SHCED_MODEL_STATUS_RUNNING,
	SHCED_MODEL_STATUS_MAX
};

struct schedule_model {
	struct list_head node;
	uint16_t id;
	uint8_t is_valid;
	uint8_t schedule_status;
	uint16_t sqcq_count;
	uint16_t sqcq_exec_count;
	struct list_head sqcq_list;
	struct list_head hwts_event_list;
	struct list_head schedule_stream_list;
	uint16_t priority;
	uint16_t pid;
	uint16_t rtsq_type;
};

void sched_model_push_sqcq(struct schedule_model *sched_model, struct hwts_sq_cq *hwts_sqcq);
void sched_model_push_stream(struct schedule_model *sched_model, struct schedule_stream *sched_stream);
void sched_model_push_event(struct schedule_model *sched_model, struct hwts_event *event);
void sched_model_reset_hwts_event(struct schedule_model *sched_model, struct schedule_stream *sched_stream);
#endif