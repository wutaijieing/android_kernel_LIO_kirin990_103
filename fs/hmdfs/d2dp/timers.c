// SPDX-License-Identifier: GPL-2.0

#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>

#include "timers.h"

static void work_handler(struct work_struct *work)
{
	struct d2d_timer *timer = NULL;

	timer = container_of(work, struct d2d_timer, work.work);
	timer->func(timer->arg);
}

void d2d_timer_init(struct workqueue_struct *wq, struct d2d_timer *timer,
		    u32 msecs, d2d_timer_action action, void *arg)
{
	timer->func = action;
	timer->arg = arg;
	timer->delay = msecs_to_jiffies(msecs);
	timer->wq = wq;

	INIT_DELAYED_WORK(&timer->work, work_handler);
}

void d2d_timer_enable(struct d2d_timer *timer)
{
	queue_delayed_work(timer->wq, &timer->work, timer->delay);
}

void d2d_timer_disable(struct d2d_timer *timer)
{
	cancel_delayed_work_sync(&timer->work);
}

void d2d_timer_reset(struct d2d_timer *timer)
{
	mod_delayed_work(timer->wq, &timer->work, timer->delay);
}
