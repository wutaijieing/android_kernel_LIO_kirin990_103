/*
 * response.h
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
#ifndef _RESPONSE_H
#define _RESPONSE_H

#include <linux/kfifo.h>
#include <linux/types.h>
#include <linux/wait.h>

#define NPU_RESPONSE_MAX 1024

struct response {
	volatile uint16_t resv0 : 6;
	volatile uint16_t stream_id : 10;
	volatile uint16_t task_id;
	volatile uint32_t resv1;
	volatile uint64_t payload;
};

struct response_queue {
	struct kfifo fifo;
	struct response buffer[NPU_RESPONSE_MAX];
	wait_queue_head_t report_wait;
};

struct response_queue *resp_queue_alloc(void);

void resp_queue_free(struct response_queue *resp_queue);

int resp_queue_is_empty(struct response_queue *resp_queue);

int resp_queue_push_back(struct response_queue *resp_queue, struct response *resp);

int resp_queue_pop_front(struct response_queue *resp_queue, struct response *resp);

#endif /* _RESPONSE_H */
