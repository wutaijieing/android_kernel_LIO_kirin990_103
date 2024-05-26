// SPDX-License-Identifier: GPL-2.0
/*
 * ed_task.c
 *
 * early detection task
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
#include "sched.h"


#ifdef CONFIG_SCHED_RTG
static inline bool is_rtg_ed_task(struct rq *rq,
				  struct task_struct *p, u64 wall)
{
	struct related_thread_group *grp = NULL;
	struct sched_cluster *prefer_cluster = NULL;
	int cpu = cpu_of(rq);

	rcu_read_lock();
	grp = task_related_thread_group(p);
	rcu_read_unlock();

	if (!grp || !grp->ed_enabled)
		return false;

	prefer_cluster = grp->preferred_cluster;
	if (!prefer_cluster)
		return false;

	/* if the task running on perferred cluster, then igore */
	if (cpumask_test_cpu(cpu, &prefer_cluster->cpus) ||
	    capacity_orig_of(cpu) > capacity_orig_of(cpumask_first(&prefer_cluster->cpus)))
		return false;

	if (p->last_wake_wait_sum >= grp->ed_task_waiting_duration)
		return true;

	if (wall - p->last_wake_ts >= grp->ed_task_running_duration)
		return true;

	if (is_new_task(p) &&
	    wall - p->last_wake_ts >= grp->ed_new_task_running_duration)
		return true;

	return false;
}
#endif /* CONFIG_SCHED_RTG */

#ifdef CONFIG_SCHED_RUNNING_TASK_ROTATION
#define ED_TASK_SHORT_DURATION 8000000 /* 8ms */
#endif

bool is_ed_task(struct rq *rq, struct task_struct *p, u64 wall)
{
#ifdef CONFIG_SCHED_HISI_UTIL_CLAMP
	if (get_task_max_util(p) < capacity_orig_of(cpu_of(rq))) {
		/*
		 * Obviously p is not a ed task.
		 * Besides we have a overload_detection check in
		 * schedutil which may break our util_clamp.
		 * There's no easy way to keep both util_clamp and
		 * overload_detection perfectly correct. Just shut
		 * overload_detection off here because it's not
		 * designed for user-experience.
		 */
#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
		rq->skip_overload_detect = true;
#endif
		return false;
	}
#endif

#ifdef CONFIG_SCHED_RTG
	/* handle rtg task */
	if (is_rtg_ed_task(rq, p, wall))
		return true;
#endif

#if defined(CONFIG_SCHED_TUNE)
	if (schedtune_prefer_idle(p)) {
#elif defined(CONFIG_UCLAMP_TASK_GROUP)
	if (uclamp_latency_sensitive(p)) {
#else
	if (false) {
#endif
		if (p->last_wake_wait_sum >= rq->ed_task_waiting_duration)
			return true;

		if (wall - p->last_wake_ts >= rq->ed_task_running_duration)
			return true;

		if (is_new_task(p) && wall - p->last_wake_ts >=
					rq->ed_new_task_running_duration)
			return true;
	}

#ifdef CONFIG_SCHED_RUNNING_TASK_ROTATION
	if (rotation_enabled) {
		if (wall - p->last_wake_ts >= ED_TASK_SHORT_DURATION)
			return true;
	}
#endif

	return false;
}

/*
 * Tasks that are runnable continuously for a period greather than
 * EARLY_DETECTION_DURATION can be flagged early as potential
 * high load tasks.
 */
bool early_detection_notify(struct rq *rq, u64 wallclock)
{
	struct task_struct *p = NULL;
	int loop_max = 10;

	if (!rq->cfs.h_nr_running)
		return false;

	if (rq->ed_task)
		return false;

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	rq->skip_overload_detect = false;
#endif

	list_for_each_entry(p, &rq->cfs_tasks, se.group_node) {
		if (!loop_max)
			break;

		if (is_ed_task(rq, p, wallclock)) {
			rq->ed_task = p;
			sugov_mark_util_change(cpu_of(rq), ADD_ED_TASK);
			return true;
		}

		loop_max--;
	}

	return false;
}

void clear_ed_task(struct task_struct *p, struct rq *rq)
{
	if (p == rq->ed_task) {
		rq->ed_task = NULL;
		sugov_mark_util_change(cpu_of(rq), CLEAR_ED_TASK);
	}
}

void migrate_ed_task(struct task_struct *p, struct rq *src_rq,
		     struct rq *dest_rq, u64 wallclock)
{
	int src_cpu = cpu_of(src_rq);
	int dest_cpu = cpu_of(dest_rq);
	unsigned long src_cpu_cap = capacity_orig_of(src_cpu);
	unsigned long dest_cpu_cap = capacity_orig_of(dest_cpu);

	if (p == src_rq->ed_task) {
		if (src_cpu_cap != dest_cpu_cap)
			sugov_mark_util_change(src_cpu, CLEAR_ED_TASK);

		src_rq->ed_task = NULL;

#ifdef CONFIG_ED_TASK_RESET_AT_UPMIGRATION
		if (src_cpu_cap < dest_cpu_cap) {
			p->last_wake_ts = wallclock;
			p->last_wake_wait_sum = 0;
		}
#endif
	}

	if (is_ed_task(dest_rq, p, wallclock)) {
		if (src_cpu_cap != dest_cpu_cap &&
		    !dest_rq->ed_task)
			sugov_mark_util_change(dest_cpu, ADD_ED_TASK);

		dest_rq->ed_task = p;
	}
}

void note_task_waking(struct task_struct *p, u64 wallclock)
{
	/*
	 * These members will be checked in the following calls:
	 * 1. scheduler_tick -> early_detection_notify -> is_ed_ask
	 * 2. set_task_cpu -> walt_fixup_busy_time
	 *	-> migrate_ed_task -> is_ed_task
	 * So, make sure note_task_waking is called before set_task_cpu
	 * in ttwu().
	 */
	p->last_wake_wait_sum = 0;
	p->last_wake_ts = wallclock;
}
