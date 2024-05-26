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

#ifndef _CHARGER_EVENT_H_
#define _CHARGER_EVENT_H_

#define MAX_EVENT_COUNT                     16
#define EVENT_QUEUE_UNIT                    MAX_EVENT_COUNT

enum charger_event_type {
	START_SINK = 0,
	STOP_SINK,
	START_SOURCE,
	STOP_SOURCE,
	START_SINK_WIRELESS,
	STOP_SINK_WIRELESS,
	CHARGER_MAX_EVENT,
};

struct charger_event_queue {
	enum charger_event_type *event;
	unsigned int num_event;
	unsigned int max_event;
	unsigned int enpos, depos;
	unsigned int overlay, overlay_index;
};

const char *charger_event_type_string(enum charger_event_type event);
int charger_event_queue_create(struct charger_event_queue *event_queue, unsigned int count);
void charger_event_queue_destroy(struct charger_event_queue *event_queue);
int charger_event_queue_isempty(struct charger_event_queue *event_queue);
void charger_event_queue_set_overlay(struct charger_event_queue *event_queue);
void charger_event_queue_clear_overlay(struct charger_event_queue *event_queue);
int charger_event_enqueue(struct charger_event_queue *event_queue,
	enum charger_event_type event);
enum charger_event_type charger_event_dequeue(struct charger_event_queue *event_queue);

#endif /* _CHARGER_EVENT_H_ */
