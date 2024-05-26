/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * phase.h
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

#undef TRACE_SYSTEM
#define TRACE_SYSTEM phase

#if !defined(_TRACE_PHASE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_PHASE_H

#include <linux/phase.h>
#include <linux/perf_event.h>
#include <linux/tracepoint.h>
#include <securec.h>

TRACE_EVENT(phase_calc_task_pmu_delta,

	TP_PROTO(struct task_struct *p, int event_id,
		 u64 prev, u64 curr, u64 delta),

	TP_ARGS(p, event_id, prev, curr, delta),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	int,	event_id		)
		__field(	u64,	prev			)
		__field(	u64,	curr			)
		__field(	u64,	delta			)
	),

	TP_fast_assign(
		memcpy_s(__entry->comm, TASK_COMM_LEN, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->event_id	= event_id;
		__entry->prev		= prev;
		__entry->curr		= curr;
		__entry->delta		= delta;
	),

	TP_printk("comm=%s pid=%d pmu_event_id=%#x "
		  "prev=%llu curr=%llu delta=%llu",
		   __entry->comm, __entry->pid, __entry->event_id,
		   __entry->prev, __entry->curr, __entry->delta)
);

TRACE_EVENT(phase_sched_time,

	TP_PROTO(struct task_struct *p, enum smt_mode mode, u64 sched_exec,
		 u64 sched_latency, u64 delta_exec),

	TP_ARGS(p, mode, sched_exec, sched_latency, delta_exec),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	enum smt_mode,	mode		)
		__field(	u64,	sched_exec		)
		__field(	u64,	sched_latency		)
		__field(	u64,	delta_exec		)
		__field(	u64,	load_avg		)
	),

	TP_fast_assign(
		memcpy_s(__entry->comm, TASK_COMM_LEN, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->mode		= mode;
		__entry->sched_exec	= sched_exec;
		__entry->sched_latency	= sched_latency;
		__entry->delta_exec	= delta_exec;
		__entry->load_avg	= p->se.avg.load_avg;
	),

	TP_printk("comm=%s pid=%d mode=%d sched_exec=%llu sched_latency=%llu "
		  "delta_exec=%llu load=%llu",
		   __entry->comm, __entry->pid, __entry->mode, __entry->sched_exec,
		   __entry->sched_latency, __entry->delta_exec, __entry->load_avg)
);
#endif /* _TRACE_PHASE_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
