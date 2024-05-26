/*
 * vip.h
 *
 * VIP sched implemented inside RT sched class
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
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
#ifndef __SCHED_VIP_H
#define __SCHED_VIP_H

#ifdef CONFIG_RT_PRIO_EXTEND_VIP
#define VIP_PRIO_WIDTH 10
void spare_vip_width(struct task_struct *p, int policy, struct sched_attr *attr);
unsigned int rt_priority_userspace(struct task_struct *p);
#endif

#ifdef CONFIG_SCHED_VIP_PI
unsigned int get_max_vip_boost(struct task_struct *p);
#endif

#ifdef CONFIG_SCHED_VIP_CGROUP
void set_cgroup_vip_prio(struct task_struct *p);
#endif

#endif
