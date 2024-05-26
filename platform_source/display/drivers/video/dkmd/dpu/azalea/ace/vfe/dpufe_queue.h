/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef DPUFE_QUEUE_H
#define DPUFE_QUEUE_H

#include <linux/types.h>
#include <linux/ktime.h>
#include <linux/spinlock.h>

#define ELEM_TYPE ktime_t

struct dpufe_queue
{
	int header;
	int tail;
	int size;
	int capacity;
	spinlock_t buf_lock;
	ELEM_TYPE *buf;
};

int dpufe_queue_init(struct dpufe_queue *q, int size);
void dpufe_queue_push_tail(struct dpufe_queue *q, ELEM_TYPE data);
void dpufe_queue_deinit(struct dpufe_queue *q);

#endif
