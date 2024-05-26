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

#include <linux/types.h>
#include <linux/ktime.h>
#include <linux/slab.h>

#include "dpufe_dbg.h"
#include "dpufe_queue.h"

int dpufe_queue_init(struct dpufe_queue *q, int size)
{
	dpufe_check_and_return(q == NULL, -1, "null ptr\n");

	q->header = 0;
	q->tail = 0;
	q->capacity = 0;
	q->size = 0;

	if (size <= 0) {
		DPUFE_ERR("size is invalid\n");
		return -1;
	}

	spin_lock_init(&q->buf_lock);
	q->buf = (ELEM_TYPE *)kzalloc(size * sizeof(ELEM_TYPE), GFP_KERNEL);
	if (q->buf == NULL) {
		DPUFE_ERR("kmalloc failed\n");
		return -1;
	}

	q->capacity = size;
	return 0;
}

static bool dpufe_queue_is_full(struct dpufe_queue *q)
{
	dpufe_check_and_return(q == NULL, false, "null ptr\n");
	return q->size == q->capacity;
}

static bool dpufe_queue_is_empty(struct dpufe_queue *q)
{
	dpufe_check_and_return(q == NULL, false, "null ptr\n");
	return q->size == 0;
}

static void dpufe_queue_pop_head(struct dpufe_queue *q)
{
	dpufe_check_and_no_retval(q == NULL, "null ptr\n");

	if (!dpufe_queue_is_empty(q)) {
		q->header = (q->header + 1) % q->capacity;
		q->size--;
	}
}

void dpufe_queue_push_tail(struct dpufe_queue *q, ELEM_TYPE data)
{
	dpufe_check_and_no_retval(q == NULL, "null ptr\n");

	if (dpufe_queue_is_full(q))
		dpufe_queue_pop_head(q);

	if (!dpufe_queue_is_full(q)) {
		spin_lock(&q->buf_lock);
		if (q->buf != NULL)
			q->buf[q->tail] = data;
		spin_unlock(&q->buf_lock);
		q->tail = (q->tail + 1) % q->capacity;
		q->size++;
	}
}

void dpufe_queue_deinit(struct dpufe_queue *q)
{
	unsigned long flags = 0;
	ELEM_TYPE *free_buf = NULL;

	dpufe_check_and_no_retval(q == NULL, "null ptr\n");

	spin_lock_irqsave(&q->buf_lock, flags);
	if (q->buf != NULL) {
		free_buf = q->buf;
		q->buf = NULL;
	}
	spin_unlock_irqrestore(&q->buf_lock, flags);
	kfree(free_buf);
}

