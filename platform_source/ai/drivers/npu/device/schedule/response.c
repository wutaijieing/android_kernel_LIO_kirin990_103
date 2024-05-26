/*
 * response.c
 *
 * about response
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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
#include "response.h"

#include <linux/slab.h>

#include "npu_log.h"

struct response_queue *resp_queue_alloc(void)
{
	int ret = 0;
	struct response_queue *resp_queue = NULL;

	resp_queue = kzalloc(sizeof(struct response_queue), GFP_KERNEL);
	cond_return_error(resp_queue == NULL, NULL, "alloc response queue error");

	ret = kfifo_init(&resp_queue->fifo, &resp_queue->buffer, sizeof(resp_queue->buffer));
	if (ret != 0) {
		npu_drv_err("resp_queue_init failed, ret = %d", ret);
		kfree(resp_queue);
		return NULL;
	}

	init_waitqueue_head(&resp_queue->report_wait);

	return resp_queue;
}

void resp_queue_free(struct response_queue *resp_queue)
{
	if (resp_queue != NULL)
		kfree(resp_queue);
}

int resp_queue_is_empty(struct response_queue *resp_queue)
{
	cond_return_error(resp_queue == NULL, -1, "resp_queue is nullptr");

	return kfifo_is_empty(&resp_queue->fifo);
}

int resp_queue_push_back(struct response_queue *resp_queue, struct response *resp)
{
	unsigned int size;
	cond_return_error(resp_queue == NULL, -1, "resp_queue is nullptr");

	size = kfifo_len(&resp_queue->fifo);

	cond_return_error(kfifo_is_full(&resp_queue->fifo), -1, "kfifo is full");

	size = kfifo_in(&resp_queue->fifo, resp, sizeof(*resp));
	cond_return_error(size != sizeof(*resp), -1, "kfifo_in error");

	size = kfifo_len(&resp_queue->fifo);

	wake_up(&resp_queue->report_wait);

	return 0;
}

int resp_queue_pop_front(struct response_queue *resp_queue, struct response *resp)
{
	unsigned int size;

	cond_return_error(resp_queue == NULL, -1, "resp_queue is nullptr");

	size = kfifo_len(&resp_queue->fifo);

	if (kfifo_is_empty(&resp_queue->fifo))
		return -1;
	size = kfifo_out(&resp_queue->fifo, resp, sizeof(*resp));
	cond_return_error(size != sizeof(*resp), -1, "kfifo_out error");
	size = kfifo_len(&resp_queue->fifo);
	return 0;
}
