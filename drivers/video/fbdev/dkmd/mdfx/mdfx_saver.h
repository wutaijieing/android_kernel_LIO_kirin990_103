/*
 * Copyright (c) 2019-2019, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef MDFX_SAVER_H
#define MDFX_SAVER_H

#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>

typedef void (*msg_handle_func)(void*);

struct mdfx_thread_data_t {
	void *data;
	msg_handle_func handle;
};

struct mdfx_saver_thread_t {
	int saver_wq_flag;
	raw_spinlock_t saver_lock;
	wait_queue_head_t saver_wq;
	struct task_struct *saver_task;
	struct mdfx_thread_data_t saver_data;
};

void mdfx_saver_triggle_thread(struct mdfx_saver_thread_t *saver_thread,
		void *data, msg_handle_func handle);
int mdfx_saver_kthread_init(struct mdfx_saver_thread_t *saver_thread, const char *thread_name);
void mdfx_saver_thread_deinit(struct mdfx_saver_thread_t *saver_thread);


#endif
