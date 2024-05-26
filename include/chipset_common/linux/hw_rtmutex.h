/*
 * hw_rtmutex.h
 *
 * Description: provide external call interfaces of hw_rtmutex.
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef HW_RTMUTEX_H_INCLUDED
#define HW_RTMUTEX_H_INCLUDED
#include <linux/sched.h>
#ifndef MAX_DL_PRIO
#include <linux/sched/deadline.h>
#endif
#include <linux/sched/rt.h>
#include <linux/cgroup.h>
#include <linux/futex.h>
#include <chipset_common/linux/rtmutex_common.h>
#ifdef CONFIG_HW_QOS_THREAD
#include <chipset_common/hwqos/hwqos_common.h>
#endif
#include <chipset_common/linux/hw_pi.h>

static inline int can_all_pi(const struct rt_mutex_waiter *left,
	const struct rt_mutex_waiter *right)
{
	return (left && right && !left->major_only && !right->major_only);
}

#ifdef CONFIG_HW_QOS_THREAD
static inline unsigned int get_preempt_qos(struct task_struct *p)
{
	int qos = likely(p) ? get_task_qos(p) : 0;

	return (qos < MIN_PREEMPT_QOS) ? 0 : (unsigned int)qos;
}
#else
static inline unsigned int get_preempt_qos(struct task_struct *p)
{
	return 0;
}
#endif

static inline unsigned int calculate_major_prio(struct task_struct *p)
{
	return is_hw_futex_pi_qos_enabled() ? get_preempt_qos(p) : 0;
}

static inline void set_preempt_policy(struct rt_mutex_waiter *waiter,
	struct task_struct *p, bool set_policy)
{
	if (likely(waiter) && set_policy)
		waiter->major_only = likely(p) ? (IS_ENABLED(CONFIG_CPUSETS) &&
		    !task_css_is_root(p, cpuset_cgrp_id)) : 0;
}

#define task_to_waiter(p) &(struct rt_mutex_waiter) { \
	.major_prio = calculate_major_prio(p), .major_only = 0, \
	.prio = (p)->prio, .deadline = (p)->dl.deadline }

static inline void rt_mutex_waiter_fill_additional_infos(
	struct rt_mutex_waiter *waiter, struct task_struct *p, bool set_policy)
{
	if (likely(waiter))
		waiter->major_prio = calculate_major_prio(p);
	set_preempt_policy(waiter, p, set_policy);
}

static inline int waiter_less(const struct rt_mutex_waiter *left,
	const struct rt_mutex_waiter *right)
{
	return (likely(left) && likely(right) && ((left->major_prio >
		right->major_prio) || ((left->major_prio == right->major_prio) &&
		can_all_pi(left, right) && (left->prio < right->prio)))) ? 1 : 0;
}

static inline int waiter_more(const struct rt_mutex_waiter *left,
	const struct rt_mutex_waiter *right)
{
	return (likely(left) && likely(right) && ((left->major_prio <
		right->major_prio) || ((left->major_prio == right->major_prio) &&
		can_all_pi(left, right) && (left->prio > right->prio)))) ? 1 : 0;
}

static inline int waiter_equal(const struct rt_mutex_waiter *left,
	const struct rt_mutex_waiter *right)
{
	return (unlikely(!left) || unlikely(!right) ||
		(left->major_prio != right->major_prio) ||
		(can_all_pi(left, right) && (left->prio != right->prio))) ? 0 : 1;
}

static inline int hw_rt_mutex_waiter_less(const struct rt_mutex_waiter *left,
	const struct rt_mutex_waiter *right)
{
	return (likely(is_hw_futex_pi_enabled()) && likely(left) &&
		likely(right) && !rt_prio(left->prio) && !rt_prio(right->prio)) ?
		waiter_less(left, right) : -1;
}

static inline int hw_rt_mutex_waiter_more(const struct rt_mutex_waiter *left,
	const struct rt_mutex_waiter *right)
{
	return (likely(is_hw_futex_pi_enabled()) && likely(left) &&
		likely(right) && !rt_prio(left->prio) && !rt_prio(right->prio)) ?
		waiter_more(left, right) : -1;
}

static inline int hw_rt_mutex_waiter_equal(const struct rt_mutex_waiter *left,
	const struct rt_mutex_waiter *right)
{
	return (likely(is_hw_futex_pi_enabled()) && likely(left) &&
		likely(right) && !rt_prio(left->prio) && !rt_prio(right->prio)) ?
		waiter_equal(left, right) : -1;
}

static inline int rt_mutex_waiter_more(const struct rt_mutex_waiter *l,
	const struct rt_mutex_waiter *r)
{
	int ret = hw_rt_mutex_waiter_more(l, r);

	return (ret >= 0) ? ret : (((l->prio > r->prio) ||
		(dl_prio(l->prio) && (l->deadline > r->deadline))) ? 1 : 0);
}

static inline bool hw_pi_can_steal(const struct rt_mutex_waiter *l,
	const struct rt_mutex_waiter *r)
{
	return likely(is_hw_futex_pi_enabled()) && likely(l) &&
		likely(r) && !rt_prio(l->prio) && !rt_prio(r->prio);
}
#endif /* HW_RTMUTEX_H_INCLUDED */
