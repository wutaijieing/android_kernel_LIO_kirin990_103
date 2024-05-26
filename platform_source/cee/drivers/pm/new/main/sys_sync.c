/*
 * suspend.c
 *
 * suspend sync
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2015-2020. All rights reserved.
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

#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/pm_wakeup.h>
#include <linux/syscalls.h>
#include <linux/suspend.h>
#include <linux/version.h>

#define SUSPEND_SYS_SYNC_TIMEOUT		(HZ / 4)

static int g_suspend_sys_sync_count;
static DEFINE_SPINLOCK(suspend_sys_sync_lock);
static struct workqueue_struct *g_suspend_sys_sync_work_queue;
static DECLARE_COMPLETION(suspend_sys_sync_comp);

static bool g_suspend_sys_sync_abort;
static void suspend_sys_sync_handler(struct timer_list *unused);
static DEFINE_TIMER(suspend_sys_sync_timer, suspend_sys_sync_handler);

static void suspend_sys_sync(struct work_struct *work)
{
	pr_info("PM: Syncing filesystems +\n");
	ksys_sync();
	pr_info("PM: Syncing filesystems -\n");

	spin_lock(&suspend_sys_sync_lock);
	g_suspend_sys_sync_count--;
	spin_unlock(&suspend_sys_sync_lock);
}
static DECLARE_WORK(suspend_sys_sync_work, suspend_sys_sync);

void sys_sync_queue(void)
{
	int ret;

	spin_lock(&suspend_sys_sync_lock);
	ret = queue_work(g_suspend_sys_sync_work_queue, &suspend_sys_sync_work);
	if (ret != 0)
		g_suspend_sys_sync_count++;
	spin_unlock(&suspend_sys_sync_lock);
}

/*
 * value should be less then half of input event wake lock timeout value
 * which is currently set to 5*HZ (see drivers/input/evdev.c)
 */
static void suspend_sys_sync_handler(struct timer_list *unused)
{
	if (g_suspend_sys_sync_count == 0) {
		complete(&suspend_sys_sync_comp);
	} else if (pm_wakeup_pending()) {
		g_suspend_sys_sync_abort = true;
		complete(&suspend_sys_sync_comp);
	} else {
		mod_timer(&suspend_sys_sync_timer,
			  jiffies + SUSPEND_SYS_SYNC_TIMEOUT);
	}
}

int sys_sync_wait(void)
{
	g_suspend_sys_sync_abort = false;

	if (g_suspend_sys_sync_count != 0) {
		mod_timer(&suspend_sys_sync_timer,
			  jiffies + SUSPEND_SYS_SYNC_TIMEOUT);
		wait_for_completion(&suspend_sys_sync_comp);
	}

	if (g_suspend_sys_sync_abort) {
		pr_info("suspend aborted....while waiting for sys_sync\n");
		return -EBUSY;
	}

	return 0;
}

int sys_sync_helper(void)
{
	int error;
	ktime_t start;
	long elapsed_msecs;

	start = ktime_get();
	sys_sync_queue();
	error = sys_sync_wait();
	elapsed_msecs = ktime_to_ms(ktime_sub(ktime_get(), start));
	pr_info("Filesystems sync: %ld.%03ld seconds\n",
		elapsed_msecs / MSEC_PER_SEC, elapsed_msecs % MSEC_PER_SEC);

	return error;
}

static int __init sys_sync_thread_init(void)
{
	g_suspend_sys_sync_work_queue = NULL;
	g_suspend_sys_sync_abort = false;

	init_completion(&suspend_sys_sync_comp);

	g_suspend_sys_sync_work_queue =
		alloc_ordered_workqueue("sys_sync_suspend", 0);
	if (g_suspend_sys_sync_work_queue != NULL)
		return 0;

	return -ENOMEM;
}

core_initcall(sys_sync_thread_init);
