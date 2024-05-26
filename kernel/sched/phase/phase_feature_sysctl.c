/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * phase_feature_sysctl.c
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
#include <linux/sysctl.h>
#include <linux/capability.h>
#include <linux/cpumask.h>
#include <linux/topology.h>
#include <linux/sched/task.h>
#include <securec.h>

#include <../kernel/sched/sched.h>
#include "phase_feature.h"

#ifdef CONFIG_PROC_SYSCTL
static int __maybe_unused zero;
static int __maybe_unused one = 1;

DEFINE_STATIC_KEY_FALSE(sched_phase);

unsigned int sysctl_phase_invalid_interval_ns;
unsigned int sysctl_phase_counter_window_policy = WINDOW_POLICY_RECENT;
unsigned int sysctl_phase_counter_window_size[NR_SMT_MODE] = {PHASE_WINDOW_SIZE, PHASE_WINDOW_SIZE};
unsigned int sysctl_phase_reverse_enabled;
unsigned int sysctl_phase_reverse_default_val = 1;
unsigned int sysctl_phase_forward_enabled;
unsigned int sysctl_phase_forward_default_val = 1;
unsigned int sysctl_phase_check_interval_ns = 3500000;
unsigned int sysctl_phase_exclude_hv = 1;
unsigned int sysctl_phase_exclude_idle = 0;
unsigned int sysctl_phase_exclude_kernel = 1;
unsigned int sysctl_phase_nr_core_find = 10;
unsigned int sysctl_phase_calc_feature_mode = 1;
static unsigned long *phase_cpumask_bits = cpumask_bits(&sysctl_phase_cpumask);

static void phase_invalid_all_task(void)
{
	struct task_struct *g, *task;
	enum smt_mode smt;

	read_lock(&tasklist_lock);
	for_each_process_thread(g, task) {
		for (smt = 0; smt < NR_SMT_MODE; smt++) {
#ifdef CONFIG_PHASE_COUNTER_WINDOW
			struct phase_counter_queue *queue = &task->phase_info->window.queue[smt];
			(void)memset_s(queue, sizeof(*queue), 0, sizeof(*queue));
			queue->size = 1;
#endif
			task->phase_info->valid[smt] = false;
		}
	}
	read_unlock(&tasklist_lock);
}

static int set_phase_state(bool enabled)
{
	int err;
	int state = static_branch_likely(&sched_phase);

	if (enabled == state) {
		pr_warn("phase has already been %s\n", state ? "enabled" : "disabled");
		return 0;
	}

	if (enabled) {
		err = phase_tk_create();
		if (err) {
			pr_err("phase enable failed\n");
			return err;
		}
		static_branch_enable(&sched_phase);
		pr_info("phase enabled\n");
	} else {
		static_branch_disable(&sched_phase);
		phase_tk_release();
		phase_invalid_all_task();
		pr_info("phase disabled\n");
	}

	return 0;
}

/*
 * the other procfs files of phase cannot be modified if sched_phase is already enabled
 */
static int phase_proc_state(struct ctl_table *table, int write,
			    void __user *buffer, size_t *lenp, loff_t *ppos)
{
	struct ctl_table t;
	int err;
	int state = static_branch_likely(&sched_phase);

	if (write && !capable(CAP_SYS_ADMIN))
		return -EPERM;

	t = *table;
	t.data = &state;
	err = proc_dointvec_minmax(&t, write, buffer, lenp, ppos);
	if (err < 0)
		return err;
	if (write)
		err = set_phase_state(state);

	return err;
}

static int phase_proc_cpumask(struct ctl_table *table, int write,
			      void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int err;

	if (write && static_branch_likely(&sched_phase)) {
		pr_info("[%s] unmodifiable when phase enabled\n", table->procname);
		return -EPERM;
	}

	err = proc_do_large_bitmap(table, write, buffer, lenp, ppos);
	if (err < 0 || !write)
		return err;

	cpumask_and(&sysctl_phase_cpumask, &sysctl_phase_cpumask, cpu_possible_mask);
	return err;
}

static int phase_proc_dointvec(struct ctl_table *table, int write,
			       void __user *buffer, size_t *lenp, loff_t *ppos)
{
	if (write && static_branch_likely(&sched_phase)) {
		pr_info("[%s] unmodifiable when phase enabled\n", table->procname);
		return -EPERM;
	}
	return proc_dointvec(table, write, buffer, lenp, ppos);
}

static int phase_proc_dointvec_minmax(struct ctl_table *table, int write,
				      void __user *buffer, size_t *lenp, loff_t *ppos)
{
	if (write && static_branch_likely(&sched_phase)) {
		pr_info("[%s] unmodifiable when phase enabled\n", table->procname);
		return -EPERM;
	}
	return proc_dointvec_minmax(table, write, buffer, lenp, ppos);
}

struct ctl_table phase_table[] = {
	{
		.procname	= "enabled",
		.data		= NULL,
		.maxlen		= sizeof(unsigned int),
		.mode		= 0640,
		.proc_handler	= phase_proc_state,
		.extra1		= &zero,
		.extra2		= &one,
	},
	{
		.procname	= "cpumask",
		.data		= &phase_cpumask_bits,
		.maxlen		= NR_CPUS,
		.mode		= 0640,
		.proc_handler	= phase_proc_cpumask,
	},
	{
		.procname	= "invalid_interval_ns",
		.data		= &sysctl_phase_invalid_interval_ns,
		.maxlen		= sizeof(unsigned int),
		.mode		= 0640,
		.proc_handler	= phase_proc_dointvec,
	},
	{
		.procname	= "check_interval_ns",
		.data		= &sysctl_phase_check_interval_ns,
		.maxlen		= sizeof(unsigned int),
		.mode		= 0640,
		.proc_handler	= phase_proc_dointvec,
	},
	{
		.procname	= "reverse_enabled",
		.data		= &sysctl_phase_reverse_enabled,
		.maxlen		= sizeof(unsigned int),
		.mode		= 0640,
		.proc_handler	= phase_proc_dointvec,
	},
#ifdef CONFIG_PHASE_COUNTER_WINDOW
	{
		.procname	= "window_policy",
		.data		= &sysctl_phase_counter_window_policy,
		.maxlen		= sizeof(unsigned int),
		.mode		= 0640,
		.proc_handler	= phase_proc_dointvec,
	},
	{
		.procname	= "window_size_stmode",
		.data		= &sysctl_phase_counter_window_size[ST_MODE],
		.maxlen		= sizeof(unsigned int),
		.mode		= 0640,
		.proc_handler	= phase_proc_dointvec,
	},
	{
		.procname	= "window_size_mtmode",
		.data		= &sysctl_phase_counter_window_size[SMT_MODE],
		.maxlen		= sizeof(unsigned int),
		.mode		= 0640,
		.proc_handler	= phase_proc_dointvec,
	},
#endif
	{
		.procname	= "flexible_events",
		.data		= sysctl_phase_nop_fevents,
		.maxlen		= sizeof(sysctl_phase_nop_fevents),
		.mode		= 0640,
		.proc_handler	= phase_proc_dointvec,
	},
	{
		.procname	= "pinned_events",
		.data		= sysctl_phase_nop_pevents,
		.maxlen		= sizeof(sysctl_phase_nop_pevents),
		.mode		= 0640,
		.proc_handler	= phase_proc_dointvec,
	},
	{
		.procname	= "exclude_hv",
		.data		= &sysctl_phase_exclude_hv,
		.maxlen		= sizeof(unsigned int),
		.mode		= 0640,
		.proc_handler	= phase_proc_dointvec_minmax,
		.extra1		= &zero,
		.extra2		= &one,
	},
	{
		.procname	= "exclude_idle",
		.data		= &sysctl_phase_exclude_idle,
		.maxlen		= sizeof(unsigned int),
		.mode		= 0640,
		.proc_handler	= phase_proc_dointvec_minmax,
		.extra1		= &zero,
		.extra2		= &one,
	},
	{
		.procname	= "exclude_kernel",
		.data		= &sysctl_phase_exclude_kernel,
		.maxlen		= sizeof(unsigned int),
		.mode		= 0640,
		.proc_handler	= phase_proc_dointvec_minmax,
		.extra1		= &zero,
		.extra2		= &one,
	},
	{
		.procname	= "reverse_default_val",
		.data		= &sysctl_phase_reverse_default_val,
		.maxlen		= sizeof(unsigned int),
		.mode		= 0640,
		.proc_handler	= phase_proc_dointvec,
	},
	{
		.procname	= "forward_enabled",
		.data		= &sysctl_phase_forward_enabled,
		.maxlen		= sizeof(unsigned int),
		.mode		= 0640,
		.proc_handler	= phase_proc_dointvec,
	},
	{
		.procname	= "forward_default_val",
		.data		= &sysctl_phase_forward_default_val,
		.maxlen		= sizeof(unsigned int),
		.mode		= 0640,
		.proc_handler	= phase_proc_dointvec,
	},
	{
		.procname	= "nr_core_find",
		.data		= &sysctl_phase_nr_core_find,
		.maxlen		= sizeof(unsigned int),
		.mode		= 0640,
		.proc_handler	= phase_proc_dointvec,
	},
	{
		.procname	= "think_class",
		.data		= phase_tk_name,
		.maxlen		= PHASE_TK_NAME_SIZE,
		.mode		= 0640,
		.proc_handler	= phase_proc_update_thinking,
	},
	{
		.procname       = "calc_feature_mode",
		.data           = &sysctl_phase_calc_feature_mode,
		.maxlen         = sizeof(unsigned int),
		.mode           = 0640,
		.proc_handler   = phase_proc_dointvec,
	},

	{ }
};
#endif /* CONFIG_PROC_SYSCTL */

static int __init phase_init(void)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		if (cpumask_weight(cpu_smt_mask(cpu)) >= 2)
			cpumask_set_cpu(cpu, &sysctl_phase_cpumask);
	}

	return 0;
}
late_initcall(phase_init);
