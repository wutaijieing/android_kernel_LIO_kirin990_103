/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * phase_feature_tracking.c
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
#include <linux/cpumask.h>
#include <linux/topology.h>
#include <linux/sched/task.h>
#include <securec.h>

#include <../kernel/sched/sched.h>
#include "phase_feature.h"

#define CREATE_TRACE_POINTS
#include <trace/events/phase.h>

#define PHASE_SCALE_VALUE	1024

DECLARE_STATIC_KEY_FALSE(sched_phase);

static inline u64 phase_event_count_sub(u64 curr, u64 prev)
{
	if (curr < prev) { /* ovewrflow */
		u64 tmp = PHASE_EVENT_OVERFLOW - prev;
		return curr + tmp;
	} else {
		return curr - prev;
	}
}

static inline void phase_calc_delta(struct task_struct *p,
				    struct phase_event_count *prev,
				    struct phase_event_count *curr,
				    struct phase_event_count *delta)
{
	int index, event_id;
	int *pevents = phase_perf_pinned_events();
	int *fevents = phase_perf_flexible_events();

	for_each_phase_pevents(index, pevents) {
		event_id = pevents[index];
		delta->pcount.data[index] = phase_event_count_sub(curr->pcount.data[index],
								  prev->pcount.data[index]);
		trace_phase_calc_task_pmu_delta(p, event_id, prev->pcount.data[index],
						curr->pcount.data[index], delta->pcount.data[index]);
	}

	for_each_phase_fevents(index, fevents) {
		event_id = fevents[index];
		if (event_id == PHASE_EVENT_GROUP_TERMINATOR)
			continue;
		delta->fcount.data[index] = phase_event_count_sub(curr->fcount.data[index],
								  prev->fcount.data[index]);
		trace_phase_calc_task_pmu_delta(p, event_id, prev->fcount.data[index],
						curr->fcount.data[index], delta->fcount.data[index]);
	}
}

static inline u64 phase_data_of_pevent(struct phase_event_pcount *counter, int event_id)
{
	int index;
	int *events = phase_perf_pinned_events();

	for_each_phase_pevents(index, events) {
		if (event_id == events[index])
			return counter->data[index];
	}

	return 0;
}

static inline void phase_count_split(struct phase_event_pcount *orig,
				     u64 ratio, struct phase_event_pcount *prev)
{
	int *events = phase_perf_pinned_events();
	int index;

	for_each_phase_pevents(index, events) {
		prev->data[index] = orig->data[index] * PHASE_SCALE_VALUE / ratio;
		orig->data[index] = orig->data[index] - prev->data[index];
	}
}

static inline void phase_count_merge(struct phase_event_pcount *count,
				     struct phase_event_pcount *merge)
{
	int *events = phase_perf_pinned_events();
	int index;

	for_each_phase_pevents(index, events)
		count->data[index] += merge->data[index];
}

static inline void phase_count_assign(struct phase_event_pcount *count,
				      struct phase_event_pcount *assign)
{
	int *events = phase_perf_pinned_events();
	int index;

	for_each_phase_pevents(index, events)
		count->data[index] = assign->data[index];
}

static DEFINE_PER_CPU(struct phase_event_count, prev_phase_event_count);
static DEFINE_PER_CPU(struct phase_event_count, curr_phase_event_count);
static DEFINE_PER_CPU(u64, phase_last_update_time);

static inline struct phase_event_count *phase_read_prev(int cpu)
{
	return &per_cpu(prev_phase_event_count, cpu);
}

static inline struct phase_event_count *phase_read_curr(int cpu)
{
	struct phase_event_count *curr = &per_cpu(curr_phase_event_count, cpu);

	phase_perf_read_events(cpu, curr->pcount.data, curr->fcount.data);

	return curr;
}

static inline void phase_clear_count(struct phase_event_pcount *count)
{
	(void)memset_s(count, sizeof(*count), 0, sizeof(*count));
}

#ifdef CONFIG_PHASE_COUNTER_WINDOW
static void phase_clear_count_window(phase_counter_window *window)
{
	int smt, win;

	for (smt = 0; smt < NR_SMT_MODE; smt++) {
		for (win = 0; win < NR_PHASE_WINDOW; win++)
			phase_clear_count(phase_window_counter(window, smt, win));

		window->queue[smt].rear = 0;
		window->queue[smt].size = 1;
		window->queue[smt].last_delta = 0;
	}
}

struct phase_event_pcount *phase_active_counter(struct phase_counter_queue *queue,
						enum phase_window_policy policy)
{
	unsigned int index, nr_buffer;
	struct phase_event_pcount *counter = NULL;
	struct phase_event_pcount *buffer = NULL;

	counter = &queue->counter;
	(void)memset_s(counter, sizeof(*counter), 0, sizeof(*counter));

	switch (policy) {
	case WINDOW_POLICY_LATEST: /* using latest buffer: maybe non-full */
		nr_buffer = 1;
		break;
	case WINDOW_POLICY_RECENT: /* using recent buffer: the last two buffers */
		nr_buffer = 2;
		break;
	case WINDOW_POLICY_AVERAGE: /* using average buffer: sum of all buffers in use */
		nr_buffer = queue->size;
		break;
	default:
		nr_buffer = 0;
		pr_err("window policy invalid\n");
	}

	if (nr_buffer > queue->size)
		nr_buffer = queue->size;

	index = queue->rear;
	while (nr_buffer-- > 0) {
		buffer = &queue->buffer[index];
		phase_count_merge(counter, buffer);
		index = (index + NR_PHASE_WINDOW - 1) % NR_PHASE_WINDOW;
	}

	return counter;
}

static void phase_save_count_window(struct phase_info *info,
				    struct phase_event_pcount *delta,
				    u64 delta_exec, enum smt_mode smt)
{
	struct phase_counter_queue *queue = NULL;
	unsigned int rear;
	u64 last_delta, window_size;
	struct phase_event_pcount *curr_counter = NULL;

	queue = &info->window.queue[smt];
	rear = queue->rear;
	last_delta = queue->last_delta;

	curr_counter = &queue->buffer[rear];

	/* Should invalid another window ? */
	if (info->last_mode != smt) {
		info->curr_delta = delta_exec;
		info->last_mode = smt;
	} else {
		info->curr_delta += delta_exec;
	}
	if (info->curr_delta >= sysctl_phase_invalid_interval_ns)
		info->valid[next_smt_mode(smt)] = false;

	window_size = sysctl_phase_counter_window_size[smt];
	if (last_delta + delta_exec < window_size) {
		/* Merged into last buffer */
		phase_count_merge(curr_counter, delta);
		queue->last_delta += delta_exec;
	} else {
		/* Split data into different buffers */
		u64 ratio, remain;
		struct phase_event_pcount prev;

		remain = window_size - last_delta;
		while (delta_exec > remain) {
			ratio = delta_exec * PHASE_SCALE_VALUE / remain;
			phase_count_split(delta, ratio, &prev);
			phase_count_merge(curr_counter, &prev);

			rear = (rear + 1) % NR_PHASE_WINDOW;
			curr_counter = &queue->buffer[rear];
			(void)memset_s(curr_counter, sizeof(*curr_counter), 0, sizeof(*curr_counter));

			if (queue->size < NR_PHASE_WINDOW)
				queue->size++;
			delta_exec -= remain;
			remain = window_size;
		}
		queue->rear = rear;
		queue->last_delta = delta_exec;
		phase_count_merge(curr_counter, delta);
	}
}

#else

static inline void phase_clear_count_window(phase_counter_window *window)
{
	phase_clear_count(phase_active_counter(window));
}

static inline void phase_save_count_window(struct phase_info *info,
					   struct phase_event_pcount *delta,
					   u64 delta_exec, enum smt_mode smt)
{
	struct phase_event_pcount *counter = phase_active_counter(&info->window);

	phase_count_assign(counter, delta);
}
#endif

int phase_fork(struct task_struct *p)
{
	struct phase_info *info = NULL;

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->new_task = true;
	info->last_mode = NR_SMT_MODE;
	phase_clear_count_window(&info->window);

	p->phase_info = info;
	return 0;
}

void phase_free(struct task_struct *p)
{
	if (p->phase_info) {
		kfree(p->phase_info);
		p->phase_info = NULL;
	}
}

static void print_phase_delta(struct task_struct *p, u64 delta_exec,
			      enum smt_mode smt,
			      struct phase_event_count *delta,
			      unsigned long flags, u64 total_inst)
{
	char buff[64] = { 0 };
	char output[1024] = { 0 };
	int event_id;
	int *pevents = phase_perf_pinned_events();
	int *fevents = phase_perf_flexible_events();
	int index;

	for_each_phase_pevents(index, pevents) {
		event_id = pevents[index];
		(void)sprintf_s(buff, sizeof(buff), " event_id=0x%llx delta=%llu",
				event_id, delta->pcount.data[index]);
		(void)strcat_s(output, sizeof(output), buff);
	}

	for_each_phase_fevents(index, fevents) {
		event_id = fevents[index];
		if (event_id == PHASE_EVENT_GROUP_TERMINATOR)
			continue;

		(void)sprintf_s(buff, sizeof(buff), " event_id=0x%llx delta=%llu",
				event_id, delta->fcount.data[index]);
		(void)strcat_s(output, sizeof(output), buff);
	}

	trace_printk("pmu_delta[%lu]: comm=%s pid=%d mode=%d delta_exec=%llu"
		     "%s total_inst=%llu\n",
		     flags & ~(PHASE_RESET_MT_INFO | PHASE_PREV_MODE),
		     p->comm, p->pid, smt, delta_exec, output, total_inst);
}

static void __phase_account_task(struct task_struct *p,
				 unsigned int cpu,
				 struct phase_event_count *delta,
				 unsigned long flags)
{
	struct phase_info *info = p->phase_info;
	u64 *phase_update_time = NULL;
	u64 *task_update_time = NULL;
	u64 sched_exec, sched_latency, delta_exec;
	struct rq *rq = cpu_rq(cpu);
	u64 now = rq_clock_task(rq);
	enum smt_mode smt = ST_MODE;

	if (!info || is_idle_task(p))
		return;

#ifdef CONFIG_SCHED_CORE
	if (sched_core_enabled(rq))
		smt = flags & PHASE_PREV_MODE ?
			rq->core->core_prev_mode : get_smt_mode(cpu, true);
#endif

	phase_update_time = &per_cpu(phase_last_update_time, cpu);
	task_update_time = &info->last_update_time;

	if (now < *task_update_time)
		return;

	/*
	 *	SWITCH_OUT              OTHER_PRCOESS   SWITCH_IN               SWITCH_OUT
	 *	task_last_update_time   ......          phase_last_update_time  now
	 */
	sched_exec = now - *task_update_time;
	sched_latency = *phase_update_time - *task_update_time;
	delta_exec = now - *phase_update_time;

	/* update last phase update time */
	*task_update_time = now;
	*phase_update_time = now;

	/* ignore invalid PMU data */
	if (delta_exec == 0 || phase_data_of_pevent(&delta->pcount, CYCLES) == 0)
		goto out;

	info->total_inst += phase_data_of_pevent(&delta->pcount, INST_RETIRED);
	print_phase_delta(p, delta_exec, smt, delta, flags, info->total_inst);
	trace_phase_sched_time(p, smt, sched_exec, sched_latency, delta_exec);

out:
	if (!sched_core_enabled(rq))
		return;

	if (!fair_policy(p->policy))
		return;

	if (smt < ST_MODE || smt > SMT_MODE)
		return;
	phase_save_count_window(info, &delta->pcount, delta_exec, smt);

	if (!info->valid[smt])
		info->valid[smt] = true;

#ifdef CONFIG_SCHED_CORE
	if ((flags & PHASE_RESET_MT_INFO) && (rq->core->core_next_mode == SMT_MODE)) {
#ifdef CONFIG_PHASE_COUNTER_WINDOW
		struct phase_counter_queue *queue = &info->window.queue[SMT_MODE];
		(void)memset_s(queue, sizeof(*queue), 0, sizeof(*queue));
		queue->size = 1;
#endif
		rq->core->core_start_time = now;
		info->valid[SMT_MODE] = false;
	}
#endif
}

void phase_account_task(struct task_struct *p, int cpu, unsigned long flags)
{
	struct phase_event_count delta;
	struct phase_event_count *prev = NULL;
	struct phase_event_count *curr = NULL;

	if (!static_branch_likely(&sched_phase))
		return;

	if (!cpumask_test_cpu(cpu, &sysctl_phase_cpumask))
		return;

	/* update phase_event_count */
	prev = phase_read_prev(cpu);
	curr = phase_read_curr(cpu);
	phase_calc_delta(p, prev, curr, &delta);
	*prev = *curr;

	/* calculate phase */
	__phase_account_task(p, cpu, &delta, flags);
}
