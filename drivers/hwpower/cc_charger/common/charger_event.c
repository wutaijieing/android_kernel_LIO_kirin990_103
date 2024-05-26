// SPDX-License-Identifier: GPL-2.0
/*
 * charger_event.c
 *
 * charger plug in and plug out event module
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
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

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <huawei_platform/log/hw_log.h>
#include <chipset_common/hwpower/charger/charger_event.h>

#define HWLOG_TAG charger_event
HWLOG_REGIST();

const char *charger_event_type_string(enum charger_event_type event)
{
	static const char *const charger_event_strings[] = {
		[START_SINK]          = "START_SINK",
		[STOP_SINK]           = "STOP_SINK",
		[START_SOURCE]        = "START_SOURCE",
		[STOP_SOURCE]         = "STOP_SOURCE",
		[START_SINK_WIRELESS] = "START_SINK_WIRELESS",
		[STOP_SINK_WIRELESS]  = "STOP_SINK_WIRELESS",
		[CHARGER_MAX_EVENT]   = "CHARGER_MAX_EVENT",
	};

	if ((event < START_SINK) || (event > CHARGER_MAX_EVENT))
		return "illegal event";

	return charger_event_strings[event];
}

int charger_event_queue_create(struct charger_event_queue *event_queue, unsigned int count)
{
	if (!event_queue) {
		hwlog_err("%s:bad argument 0x%pK\n", __func__, event_queue);
		return -EINVAL;
	}

	count = (count >= MAX_EVENT_COUNT ? MAX_EVENT_COUNT : count);
	event_queue->max_event = count;
	event_queue->num_event = (count >= EVENT_QUEUE_UNIT ? EVENT_QUEUE_UNIT : count);

	event_queue->event = kzalloc(event_queue->num_event * sizeof(int),
		GFP_KERNEL);
	if (!event_queue->event) {
		hwlog_err("%s:Can't alloc space:%d\n", __func__, event_queue->num_event);
		return -ENOMEM;
	}

	event_queue->enpos = 0;
	event_queue->depos = 0;
	event_queue->overlay = 0;
	event_queue->overlay_index = 0;

	return 0;
}

void charger_event_queue_destroy(struct charger_event_queue *event_queue)
{
	if (!event_queue)
		return;

	kfree(event_queue->event);
	event_queue->event = NULL;
	event_queue->enpos = 0;
	event_queue->depos = 0;
	event_queue->num_event = 0;
	event_queue->max_event = 0;
	event_queue->overlay = 0;
	event_queue->overlay_index = 0;
}

static int charger_event_queue_isfull(struct charger_event_queue *event_queue)
{
	if (!event_queue)
		return -EINVAL;

	return (((event_queue->enpos + 1) % event_queue->num_event) == (event_queue->depos));
}

int charger_event_queue_isempty(struct charger_event_queue *event_queue)
{
	if (!event_queue)
		return -EINVAL;

	return (event_queue->enpos == event_queue->depos);
}

void charger_event_queue_set_overlay(struct charger_event_queue *event_queue)
{
	if (event_queue->overlay)
		return;
	event_queue->overlay = 1;
	event_queue->overlay_index = event_queue->enpos;
}

void charger_event_queue_clear_overlay(struct charger_event_queue *event_queue)
{
	event_queue->overlay = 0;
	event_queue->overlay_index = 0;
}

int charger_event_enqueue(struct charger_event_queue *event_queue,
	enum charger_event_type event)
{
	/* no need verify argument, isfull will check it */
	if (charger_event_queue_isfull(event_queue)) {
		hwlog_err("event queue full\n");
		return -ENOSPC;
	}

	if (event_queue->overlay) {
		if (event_queue->overlay_index == event_queue->enpos)
			event_queue->enpos = ((event_queue->enpos + 1) % event_queue->num_event);

		if (charger_event_queue_isempty(event_queue)) {
			hwlog_info("queue is empty, just enqueue\n");
			event_queue->overlay_index = ((event_queue->overlay_index + 1) %
				event_queue->num_event);
			event_queue->enpos = ((event_queue->enpos + 1) % event_queue->num_event);
			event_queue->overlay = 0;
		}

		event_queue->event[event_queue->overlay_index] = event;
	} else {
		event_queue->event[event_queue->enpos] = event;
		event_queue->enpos = ((event_queue->enpos + 1) % event_queue->num_event);
	}

	return 0;
}

enum charger_event_type charger_event_dequeue(struct charger_event_queue *event_queue)
{
	enum charger_event_type event;

	/* no need verify argument, isempty will check it */
	if (charger_event_queue_isempty(event_queue))
		return CHARGER_MAX_EVENT;

	event = event_queue->event[event_queue->depos];
	event_queue->depos = ((event_queue->depos + 1) % event_queue->num_event);

	return event;
}
