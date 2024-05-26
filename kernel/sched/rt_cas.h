/*
 * rt_cas.h
 *
 * RT capacity-aware scheduling enhancement
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

#ifndef __SCHED_RT_CAS_H
#define __SCHED_RT_CAS_H

extern unsigned int sysctl_sched_enable_rt_cas;

int rt_energy_aware_wake_cpu(struct task_struct *task);
void check_for_rt_migration(struct rq *rq, struct task_struct *curr);

DECLARE_PER_CPU(bool, check_delayed_task);
s64 task_delay(struct task_struct *p, struct rq *rq);

#ifdef CONFIG_RT_ENERGY_EFFICIENT_SUPPORT
bool is_energy_efficient_migration(struct rq *src_rq, struct rq *dst_rq);
#endif

void init_rt_cas_flags(struct task_struct *p);

#endif
