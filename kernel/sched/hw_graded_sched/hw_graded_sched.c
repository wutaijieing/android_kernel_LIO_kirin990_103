/*
 * hw_graded_sched.c
 *
 * hw graded schedule implementation
 *
 * Copyright (c) 2022-2023 Huawei Technologies Co., Ltd.
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

#include <trace/events/sched.h>
#include <linux/sched/signal.h>
#include <linux/hw_graded_sched.h>
#include "../kernel/sched/sched.h"

DEFINE_STATIC_KEY_TRUE(g_graded_sched_key);

static inline void restore_graded_nice(struct task_struct* task)
{
	int ori_nice;

	task_lock(task);
	ori_nice = task->original_nice;
	task->original_nice = DEFAULT_ORIGINAL_NICE;
	set_user_nice(task, ori_nice);
	task_unlock(task);
}

void update_graded_nice(struct task_struct* task, int increase)
{
	struct task_struct *child = NULL;
	int ori_nice;
	int new_nice;

	if (!static_branch_unlikely(&g_graded_sched_key) || !task)
		return;

	if (task->pid != task->tgid) {
#ifdef CONFIG_HW_VIP_THREAD
		if (task->static_vip && task->original_nice != DEFAULT_ORIGINAL_NICE)
			restore_graded_nice(task);
#endif
		return;
	}

	rcu_read_lock();
	child = task;
	if (increase) {
		do {
			if (task == child || !fair_policy(child->policy))
				continue;
			task_lock(child);
			if (child->original_nice == DEFAULT_ORIGINAL_NICE) {
				ori_nice = task_nice(child);
				new_nice = max(ori_nice - HCFS_GRADED_OFFSET, MIN_NICE);
				set_user_nice(child, new_nice);
				child->original_nice = ori_nice;
			}
			task_unlock(child);
			trace_sched_update_graded_nice(child, task_nice(child), increase);
		} while_each_thread (task, child);
	} else {
		do {
			if (child->original_nice != DEFAULT_ORIGINAL_NICE)
				restore_graded_nice(child);
			trace_sched_update_graded_nice(child, task_nice(child), increase);
		} while_each_thread (task, child);
	}
	rcu_read_unlock();
}

void init_graded_nice(struct task_struct* task, int increase)
{
	int new_nice = 0;

	if (!static_branch_unlikely(&g_graded_sched_key) || !task)
		return;

	if (increase && fair_policy(task->policy)) {
		task->original_nice = task_nice(task);
		new_nice = max(task->original_nice - HCFS_GRADED_OFFSET, MIN_NICE);
		task->static_prio = NICE_TO_PRIO(new_nice);
	} else {
		task->original_nice = DEFAULT_ORIGINAL_NICE;
	}
	trace_sched_init_graded_nice(task, task_nice(task), increase);
}