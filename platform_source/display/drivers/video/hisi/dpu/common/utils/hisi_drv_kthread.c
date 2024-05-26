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

#include <linux/types.h>
#include <linux/slab.h>
#include "hisi_disp_debug.h"
#include "hisi_drv_kthread.h"

static int hisi_disp_kthread_handle(void *data)
{
	struct hisi_disp_kthread *kthread = (struct hisi_disp_kthread *)data;
	int ret;

	while (1) {
		wait_event(kthread->kthread_wq, kthread->wait_flag == 1 || kthread_should_stop());
		if (kthread_should_stop())
			break;

		raw_spin_lock(&kthread->lock);
		ret = kthread->handle(kthread->data);
		if (ret)
			disp_pr_info("kthread handle fail, ret=%d", ret);

		kthread->wait_flag = 0;
		raw_spin_unlock(&kthread->lock);
	}

	return 0;
}

int hisi_disp_create_kthread(struct hisi_disp_kthread *kthread, const char *name)
{
	disp_pr_info("%s kthread initalizing...", name);

	init_waitqueue_head(&kthread->kthread_wq);
	raw_spin_lock_init(&kthread->lock);
	kthread->wait_flag = 0;

	kthread->task = kthread_create(hisi_disp_kthread_handle, kthread, name);
	if(IS_ERR_OR_NULL(kthread->task)) {
		disp_pr_err("create %s kthread failed", name);
		return -1;
	}

	wake_up_process(kthread->task);
	return 0;
}

void hisi_disp_destroy_kthread(struct hisi_disp_kthread *kthread)
{
	if (kthread->task) {
		kthread_stop(kthread->task);
		kthread->task = NULL;

		disp_pr_info("stop kernel thread succ");
	}
}

void hisi_disp_trigger_kthread(struct hisi_disp_kthread *kthread, void *data, bool be_async)
{
	int ret;

	/* if the thread is asynchronous mode, we can triggle the kthread,
	 * else just call the handle function.
	 */
	if (be_async && kthread->task) {
		raw_spin_lock(&kthread->lock);
		if (kthread->wait_flag == 1) {
			disp_pr_err("another kthread is working");
			raw_spin_unlock(&kthread->lock);
			return;
		}

		kthread->data = data;
		kthread->wait_flag = 1;
		raw_spin_unlock(&kthread->lock);

		wake_up(&kthread->kthread_wq);
		return;
	}

	if (!kthread->handle)
		return;

	ret = kthread->handle(data);
	if (ret)
		disp_pr_err("kthread handle fail, async = %d", be_async);
}

