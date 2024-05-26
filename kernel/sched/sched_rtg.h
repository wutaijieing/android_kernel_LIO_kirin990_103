// SPDX-License-Identifier: GPL-2.0
/*
 * hisi_rtg.h
 *
 * related thread group internal head file.
 *
 * Copyright (c) 2018-2020 Huawei Technologies Co., Ltd.
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
#ifndef __HISI_RTG_H__
#define __HISI_RTG_H__

#ifdef CONFIG_SCHED_RTG
int alloc_related_thread_groups(void);
void init_task_rtg(struct task_struct *p);
int sync_cgroup_colocation(struct task_struct *p, bool insert);
int _sched_set_group_id(struct task_struct *p, unsigned int group_id);
struct group_cpu_time *group_update_cpu_time(struct rq *rq,
					     struct related_thread_group *grp);
bool group_migrate_task(struct task_struct *p,
			struct rq *src_rq, struct rq *dest_rq);
void sched_get_max_group_util(const struct cpumask *query_cpus,
			      unsigned long *util, unsigned int *freq);
void sched_update_rtg_tick(struct task_struct *p);
#ifdef CONFIG_SCHED_CGROUP_RTG
void add_new_task_to_grp(struct task_struct *new);
#else
static inline void add_new_task_to_grp(struct task_struct *new __maybe_unused) {}
#endif
static inline struct related_thread_group *task_related_thread_group(struct task_struct *p)
{
	return rcu_dereference(p->grp);
}
struct cpumask *find_rtg_target(struct task_struct *p);
struct cpumask *rtg_prefer_cluster(struct task_struct *p);

#else /* !CONFIG_SCHED_RTG */

static inline int alloc_related_thread_groups(void) { return 0; }
static inline void init_task_rtg(struct task_struct *p __maybe_unused) {}
static inline int sync_cgroup_colocation(struct task_struct *p, bool insert) { return 0; }
static inline int _sched_set_group_id(struct task_struct *p, unsigned int group_id) { return 0; }
static inline void add_new_task_to_grp(struct task_struct *new __maybe_unused) {}
static inline struct group_cpu_time *group_update_cpu_time(struct rq *rq,
							   struct related_thread_group *grp)
{
	return NULL;
}
static inline bool group_migrate_task(struct task_struct *p,
				      struct rq *src_rq, struct rq *dest_rq)
{
	return false;
}
static inline void sched_get_max_group_util(const struct cpumask *query_cpus, unsigned int *freq) {}
static inline void sched_update_rtg_tick(struct task_struct *p) { return; }
static inline struct related_thread_group *task_related_thread_group(struct task_struct *p)
{
	return NULL;
}
static inline struct cpumask *find_rtg_target(struct task_struct *p) { return NULL; }
static inline struct cpumask *rtg_prefer_cluster(struct task_struct *p) { return NULL; }
#endif /* CONFIG_SCHED_RTG */

#endif
