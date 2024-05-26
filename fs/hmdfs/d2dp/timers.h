/* SPDX-License-Identifier: GPL-2.0 */
/*
 * D2D timers API
 */

#ifndef D2D_TIMERS_H
#define D2D_TIMERS_H

#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/types.h>
#include <linux/jiffies.h>

/**
 * d2d_timer_action - timer callback for D2D Protocol
 * @arg: pointer to the function argument structure
 *
 * The callback function will be executed periodically in a workqueue
 */
typedef void (*d2d_timer_action)(void *arg);

struct d2d_timer {
	struct delayed_work work;
	d2d_timer_action func;
	void *arg;
	u32 delay;
	struct workqueue_struct *wq;
};

void d2d_timer_init(struct workqueue_struct *wq, struct d2d_timer *timer,
		    u32 msecs, d2d_timer_action action, void *arg);

void d2d_timer_enable(struct d2d_timer *timer);
void d2d_timer_disable(struct d2d_timer *timer);
void d2d_timer_reset(struct d2d_timer *timer);

#endif /* D2D_TIMERS_H */
