/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * phase_feature.c
 *
 * Copyright (c) 2021 Huawei Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/sched.h>
#include <linux/phase.h>
#include <securec.h>

#include <../kernel/sched/sched.h>
#include "phase_perf.h"
#include "phase_feature.h"

#ifdef pr_fmt
#undef pr_fmt
#define pr_fmt(fmt) "[" KBUILD_MODNAME "] " fmt
#endif

struct cpumask sysctl_phase_cpumask __read_mostly;

/* Helpers for Phase interface */
static void phase_sched_core_find_helper(struct rb_node *node,
				  unsigned long cookie,
				  struct task_struct *picked,
				  struct task_struct **max,
				  unsigned int nr)
{
#ifdef CONFIG_SCHED_CORE
	struct task_struct *node_task = NULL;

	if (!node)
		return;

	if (nr > sysctl_phase_nr_core_find)
		return;

	node_task = container_of(node, struct task_struct, core_node);
	BUG_ON(!node_task);

	/*
	 * Node is a red-black tree with root key as cookie,
	 * then its left subtree may be smaller than cookie,
	 * and its right subtree may be larger than cookie
	 */
	if (node_task->core_cookie != cookie)
		return;

	if (phase_tk_predict(node_task, picked, FORWARD_MODE)) {
		*max = node_task;
		return;
	}

	phase_sched_core_find_helper(node->rb_left, cookie, picked, max, ++nr);
	phase_sched_core_find_helper(node->rb_right, cookie, picked, max, ++nr);
#endif
}

struct task_struct *phase_sched_core_find_default(struct rb_node *node,
						  unsigned long cookie,
						  struct task_struct *picked)
{
	struct task_struct *phase_core_pick = NULL;

	phase_sched_core_find_helper(node, cookie, picked, &phase_core_pick, 0);

	return phase_core_pick;
}

/* Helpers for Phase NOP Algorithm interface */
int sysctl_phase_nop_pevents[PHASE_PEVENT_NUM] = {
	CYCLES,
	INST_RETIRED,
	PHASE_EVENT_FINAL_TERMINATOR,
};

int sysctl_phase_nop_fevents[PHASE_FEVENT_NUM] = {
	PHASE_EVENT_FINAL_TERMINATOR,
};

struct phase_think_class phase_tk_nop = {
	.id = PHASE_TK_ID_NOP,
	.name = "nop",
	.pevents = sysctl_phase_nop_pevents,
	.fevents = sysctl_phase_nop_fevents,
	.create = NULL,
	.release = NULL,
	.sched_core_find = NULL,
	.next = &phase_tk_decision_tree,
};

#define phase_tk_class_head	(&phase_tk_nop)
#define default_phase_tk_class	(&phase_tk_nop)
#define for_each_phase_tk_class(tk)		\
	for ((tk) = phase_tk_class_head; (tk); (tk) = (tk)->next)

char phase_tk_name[PHASE_TK_NAME_SIZE] = "nop";
struct phase_think_class *phase_tk = default_phase_tk_class;

int phase_proc_update_thinking(struct ctl_table *table, int write,
			       void __user *buffer, size_t *lenp, loff_t *ppos)
{
	struct phase_think_class *tk = NULL;
	int ret;

	if (write && static_branch_likely(&sched_phase)) {
		pr_info("cannot modify when phase enabled\n");
		return -EPERM;
	}

	ret = proc_dostring(table, write, buffer, lenp, ppos);
	if (ret || !write)
		return ret;

	if (strcmp(phase_tk_name, phase_tk->name) == 0) {
		pr_info("already used think class [%s]\n", phase_tk_name);
		return 0;
	}

	for_each_phase_tk_class(tk) {
		if (strcmp(phase_tk_name, tk->name) == 0)
			break;
	}

	if (tk == NULL) {
		pr_err("unknown think class name [%s]\n", phase_tk_name);
		strcpy_s(phase_tk_name, sizeof(phase_tk_name), phase_tk->name);
		return -EINVAL;
	}

	/* switch to new phase tk class */
	pr_info("change think class from [%s] to [%s]\n", phase_tk->name, tk->name);
	phase_tk = tk;

	return 0;
}

#ifdef CONFIG_PHASE_REVERSE
bool phase_task_need_check_reverse(const struct task_struct *p)
{
	if (is_idle_task(p))
		return false;

	if (p->flags & (PF_EXITING | PF_KTHREAD))
		return false;

	if (test_tsk_need_resched(p))
		return false;

	return true;
}

static void phase_reverse_tick(struct rq *rq, struct task_struct *curr, int cpu)
{
	int i;
	u64 now;
	u64 task_start_time;
	struct task_struct *slibing_task = NULL;
	struct rq *max_rq = rq->core->core_max;

	if (!curr->core_cookie)
		return;

	if (!max_rq || max_rq != rq)
		return;

	if (!phase_task_need_check_reverse(curr))
		return;

	if (rq->core->core_flags & SCF_HANDLING_REVERSE)
		return;

	/* time interval */
	now = rq_clock_task(rq);
	task_start_time = rq->core->core_start_time;
	if ((task_start_time > 0) &&
	    (now - task_start_time > sysctl_phase_check_interval_ns)) {
		for_each_cpu(i, cpu_smt_mask(cpu)) {
			if (i == cpu)
				continue;

			slibing_task = cpu_rq(i)->curr;
			if (phase_task_need_check_reverse(slibing_task) &&
			    cookie_match(curr, slibing_task))
				cpu_rq(i)->core_flags |= SCF_CHECK_REVERSE;
				rq->core->core_flags |= SCF_HANDLING_REVERSE;
				resched_curr(cpu_rq(i));
				break;
			}
		}
	}
}

void phase_feature_reverse_tick(struct rq *rq, struct task_struct *curr, int cpu)
{
	if (!static_branch_likely(&sched_phase))
		return;

	if (!sched_core_enabled(rq))
		return;

	WARN_ON(curr != current);
	phase_reverse_tick(rq, curr, cpu);
}
#endif
