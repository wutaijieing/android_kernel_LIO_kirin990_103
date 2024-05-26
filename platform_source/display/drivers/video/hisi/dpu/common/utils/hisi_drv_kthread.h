/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#ifndef HISI_DRV_KTHREAD_H
#define HISI_DRV_KTHREAD_H

#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>


typedef int (*msg_handle_func)(void*);

struct hisi_disp_kthread {
	int wait_flag;
	raw_spinlock_t lock;
	wait_queue_head_t kthread_wq;
	struct task_struct *task;

	msg_handle_func handle;
	void *data;
};

int hisi_disp_create_kthread(struct hisi_disp_kthread *kthread, const char *name);
void hisi_disp_destroy_kthread(struct hisi_disp_kthread *kthread);
void hisi_disp_trigger_kthread(struct hisi_disp_kthread *kthread, void *data, bool be_async);


#endif /* HISI_DRV_KTHREAD_H */
