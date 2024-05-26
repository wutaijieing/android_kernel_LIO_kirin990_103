/*
 * hw_pi.h
 *
 * Description: provide external call interfaces of hw_pi.
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2022. All rights reserved.
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
#ifndef HW_PI_H_INCLUDED
#define HW_PI_H_INCLUDED
#include <linux/sched.h>

#ifdef CONFIG_SCHED_VIP_OLD
#define MIN_PREEMPT_VIP_PRIO	(10U)
#define MIN_INHERIT_VIP_PRIO	(1U)
#endif
#ifdef CONFIG_HW_QOS_THREAD
#define MIN_PREEMPT_QOS		(VALUE_QOS_CRITICAL)
#define MIN_INHERIT_QOS		(VALUE_QOS_CRITICAL)
#endif

#define SUPPORT_FUTEX_PI		(0x1)
#define USER_SPACE_ENABLE		(0x2)
#define QOS_INHERIT_ENABLE		(0x4)
#define VIP_INHERIT_ENABLE		(0x8)
#define VIP_PRIO_INHERIT_ENABLE	(0x10)
#define FUTEX_PI_ENABLE		(SUPPORT_FUTEX_PI | USER_SPACE_ENABLE)
extern unsigned int g_hw_futex_pi_enabled;
static inline int is_hw_futex_pi_enabled(void)
{
	return (g_hw_futex_pi_enabled & FUTEX_PI_ENABLE) == FUTEX_PI_ENABLE;
}

static inline int is_hw_futex_pi_qos_enabled(void)
{
	return g_hw_futex_pi_enabled & QOS_INHERIT_ENABLE;
}

static inline int is_hw_futex_pi_vip_enabled(void)
{
	return g_hw_futex_pi_enabled & VIP_INHERIT_ENABLE;
}

static inline int is_hw_futex_pi_vip_prio_enabled(void)
{
	return g_hw_futex_pi_enabled & VIP_PRIO_INHERIT_ENABLE;
}

bool rt_mutex_mix_prio_equal(struct task_struct *p, int prio,
	unsigned int vip_prio, struct task_struct *pi_task);

unsigned int rt_mutex_calculate_vip_prio(struct task_struct *p,
	struct task_struct *pi_task);
#ifdef CONFIG_SECCOMP_FILTER
bool can_skip_filter(int this_syscall);
#endif
#endif /* HW_PI_H_INCLUDED */
