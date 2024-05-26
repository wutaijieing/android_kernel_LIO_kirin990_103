/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 * Description:
 * Author:
 * Create: 2021-07-08
 */
#ifndef __EVENT_H
#define __EVENT_H

#include <linux/list.h>
#include <linux/types.h>

#include "schedule_stream.h"

struct event {
	uint32_t flag;
	struct list_head sched_stream_pending_list;
};

void event_init(struct event *e);
void event_record(struct event *e, struct list_head* temporary_list);
void event_wait(struct event* e, struct schedule_stream *sched_stream);

#endif