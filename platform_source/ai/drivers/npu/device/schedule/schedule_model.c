/*
 * schedule_model.c
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

#include "schedule_model.h"
#include <linux/list.h>
#include "npu_log.h"
#include "hwts_driver.h"

void sched_model_push_sqcq(struct schedule_model *sched_model, struct hwts_sq_cq *hwts_sqcq)
{
	cond_return_void(sched_model == NULL || hwts_sqcq == NULL, "sched_model or hwts_sqcq is null");
	list_add_tail(&hwts_sqcq->node, &sched_model->sqcq_list);
}

void sched_model_push_stream(struct schedule_model *sched_model, struct schedule_stream *sched_stream)
{
	cond_return_void(sched_model == NULL || sched_stream == NULL, "sched_model or sched_stream is null");
	list_move_tail(&sched_stream->node, &sched_model->schedule_stream_list);
}

void sched_model_push_event(struct schedule_model *sched_model, struct hwts_event *event)
{
	cond_return_void(sched_model == NULL || event == NULL, "sched_model or event is null");
	list_add_tail(&event->node, &sched_model->hwts_event_list);
}

void sched_model_reset_hwts_event(struct schedule_model *sched_model, struct schedule_stream *sched_stream)
{
	struct hwts_event *event = NULL;
	struct hwts_event *next = NULL;
	list_for_each_entry_safe(event, next, &sched_model->hwts_event_list, node) {
		hwts_driver_reset_event_table(event->id);
	}
}