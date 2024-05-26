/*
 * event.c
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
#include "event.h"
#include <linux/list.h>
#include "npu_log.h"

void event_init(struct event *e)
{
	e->flag = 0;
	INIT_LIST_HEAD(&e->sched_stream_pending_list);
}

void event_record(struct event *e, struct list_head *temporary_list)
{
	struct schedule_stream *sched_stream = NULL;
	struct schedule_stream *next = NULL;

	e->flag = 1;

	if (!list_empty_careful(&e->sched_stream_pending_list)) {
		list_for_each_entry_safe(sched_stream, next, &e->sched_stream_pending_list, node) {
			sched_stream->status = SCHED_STREAM_STATUS_IDLE;
			list_move_tail(&sched_stream->node, temporary_list);
		}
	}
}

void event_wait(struct event *e, struct schedule_stream *sched_stream)
{
	if (e->flag == 1) {
		npu_drv_info("before wait already record");
		return;
	}

	list_move_tail(&sched_stream->node, &e->sched_stream_pending_list);
}