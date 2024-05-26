/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * core_sched.h
 *
 * Copyright (c) 2022 Huawei Technologies Co., Ltd.
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
#define TRACE_SYSTEM core_sched

#if !defined(_TRACE_CORE_SCHED_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_CORE_SCHED_H

#include <linux/sched.h>
#include <linux/tracepoint.h>
#include <securec.h>

TRACE_EVENT(task_prio_compare,

	TP_PROTO(struct task_struct *a, int prio_a,
		 unsigned long vruntime_a, unsigned long deadline_a,
		 struct task_struct *b, int prio_b,
		 unsigned long vruntime_b, unsigned long deadline_b),

	TP_ARGS(a, prio_a, vruntime_a, deadline_a,
		b, prio_b, vruntime_b, deadline_b),

	TP_STRUCT__entry(
		__array(char,	comm_a,	TASK_COMM_LEN	)
		__array(char,	comm_b,	TASK_COMM_LEN	)
		__field(pid_t,	pid_a			)
		__field(pid_t,	pid_b			)
		__field(int,	prio_a			)
		__field(int,	prio_b			)
		__field(unsigned long, vruntime_a)
		__field(unsigned long, vruntime_b)
		__field(unsigned long, deadline_a)
		__field(unsigned long, deadline_b)
	),

	TP_fast_assign(
		memcpy_s(__entry->comm_a, TASK_COMM_LEN, a->comm, TASK_COMM_LEN);
		memcpy_s(__entry->comm_b, TASK_COMM_LEN, b->comm, TASK_COMM_LEN);
		__entry->pid_a		= a->pid;
		__entry->pid_b		= b->pid;
		__entry->prio_a		= prio_a;
		__entry->prio_b		= prio_b;
		__entry->vruntime_a	= vruntime_a;
		__entry->vruntime_b	= vruntime_b;
		__entry->deadline_a	= deadline_a;
		__entry->deadline_b	= deadline_b;
	),

	TP_printk("(%s/%d;%d,%lu,%lu) ?< (%s/%d;%d,%lu,%lu)",
		  __entry->comm_a, __entry->pid_a, __entry->prio_a,
		  __entry->vruntime_a, __entry->deadline_a,
		  __entry->comm_b, __entry->pid_b, __entry->prio_b,
		  __entry->vruntime_b, __entry->deadline_b)
);

TRACE_EVENT(sched_core_find,

	TP_PROTO(int cpu, unsigned long cookie, struct task_struct *picked),

	TP_ARGS(cpu, cookie, picked),

	TP_STRUCT__entry(
		__field(int,	cpu		)
		__field(unsigned long,	cookie)
		__array(char,	comm,	TASK_COMM_LEN)
		__field(pid_t,	pid		)
		__field(unsigned long, core_cookie)
	),

	TP_fast_assign(
		__entry->cpu		= cpu;
		__entry->cookie		= cookie;
		memcpy_s(__entry->comm, TASK_COMM_LEN, picked->comm, TASK_COMM_LEN);
		__entry->pid		= picked->pid;
		__entry->core_cookie	= picked->core_cookie;
	),

	TP_printk("cpu%d find cookie %lx match: %s/%d %lx",
		  __entry->cpu, __entry->cookie, __entry->comm,
		  __entry->pid, __entry->core_cookie)
);

TRACE_EVENT(core_pick_pre_select,

	TP_PROTO(int cpu, unsigned int core_task_seq, unsigned int core_pick_seq,
		 unsigned int core_sched_seq, struct task_struct *picked),

	TP_ARGS(cpu, core_task_seq, core_pick_seq, core_sched_seq, picked),

	TP_STRUCT__entry(
		__field(int,	cpu		)
		__field(unsigned int, core_task_seq)
		__field(unsigned int, core_pick_seq)
		__field(unsigned int, core_sched_seq)
		__array(char,	comm,	TASK_COMM_LEN)
		__field(pid_t,	pid		)
		__field(unsigned long, core_cookie)
	),

	TP_fast_assign(
		__entry->cpu		= cpu;
		__entry->core_task_seq	= core_task_seq;
		__entry->core_pick_seq	= core_pick_seq;
		__entry->core_sched_seq	= core_sched_seq;
		memcpy_s(__entry->comm, TASK_COMM_LEN, picked->comm, TASK_COMM_LEN);
		__entry->pid		= picked->pid;
		__entry->core_cookie	= picked->core_cookie;
	),

	TP_printk("cpu%d pick pre selected (%u %u %u): %s/%d %lx",
		  __entry->cpu, __entry->core_task_seq, __entry->core_pick_seq,
		  __entry->core_sched_seq, __entry->comm, __entry->pid, __entry->core_cookie)
);

TRACE_EVENT(reverse_force_idle,

	TP_PROTO(int cpu, struct task_struct *prev, struct task_struct *next),

	TP_ARGS(cpu, prev, next),

	TP_STRUCT__entry(
		__field(int,	cpu		)
		__array(char,	prev_comm,	TASK_COMM_LEN)
		__field(pid_t,	prev_pid	)
		__array(char,	next_comm,	TASK_COMM_LEN)
		__field(pid_t,	next_pid	)
	),

	TP_fast_assign(
		__entry->cpu		= cpu;
		memcpy_s(__entry->prev_comm, TASK_COMM_LEN, prev->comm, TASK_COMM_LEN);
		__entry->prev_pid	= prev->pid;
		memcpy_s(__entry->next_comm, TASK_COMM_LEN, next->comm, TASK_COMM_LEN);
		__entry->next_pid	= next->pid;
	),

	TP_printk("cpu%d predict reverse forceidle %s/%d -> %s/%d",
		  __entry->cpu, __entry->prev_comm, __entry->prev_pid,
		  __entry->next_comm, __entry->next_pid)
);

TRACE_EVENT(core_pick_cpu_select,

	TP_PROTO(int cpu, struct task_struct *picked),

	TP_ARGS(cpu, picked),

	TP_STRUCT__entry(
		__field(int,	cpu		)
		__array(char,	comm,	TASK_COMM_LEN)
		__field(pid_t,	pid		)
		__field(unsigned long, core_cookie)
	),

	TP_fast_assign(
		__entry->cpu		= cpu;
		memcpy_s(__entry->comm, TASK_COMM_LEN, picked->comm, TASK_COMM_LEN);
		__entry->pid		= picked->pid;
		__entry->core_cookie	= picked->core_cookie;
	),

	TP_printk("cpu%d selected: %s/%d %lx",
		  __entry->cpu, __entry->comm, __entry->pid, __entry->core_cookie)
);

TRACE_EVENT(core_pick_max_select,

	TP_PROTO(int cpu, struct task_struct *old_max, struct task_struct *new_max),

	TP_ARGS(cpu, old_max, new_max),

	TP_STRUCT__entry(
		__field(int,	cpu		)
		__array(char,	old_comm,	TASK_COMM_LEN)
		__field(pid_t,	old_pid	)
		__field(unsigned long, old_cookie)
		__array(char,	new_comm,	TASK_COMM_LEN)
		__field(pid_t,	new_pid	)
		__field(unsigned long, new_cookie)
	),

	TP_fast_assign(
		__entry->cpu		= cpu;
		memcpy_s(__entry->old_comm, TASK_COMM_LEN, old_max ? old_max->comm : "NULL", TASK_COMM_LEN);
		__entry->old_pid	= old_max ? old_max->pid : -1;
		__entry->old_cookie	= old_max ? old_max->core_cookie : 0;
		memcpy_s(__entry->new_comm, TASK_COMM_LEN, new_max->comm, TASK_COMM_LEN);
		__entry->new_pid	= new_max->pid;
		__entry->new_cookie	= new_max->core_cookie;
	),

	TP_printk("cpu%d max %s/%d %lu -> %s/%d %lu",
		  __entry->cpu, __entry->old_comm, __entry->old_pid, __entry->old_cookie,
		  __entry->new_comm, __entry->new_pid, __entry->new_cookie)
);

TRACE_EVENT(steal_cookie,

	TP_PROTO(struct task_struct *p, int src, int dst,
		 int idle_core_occupation, unsigned long cookie),

	TP_ARGS(p, src, dst, idle_core_occupation, cookie),

	TP_STRUCT__entry(
		__array(char,	comm,	TASK_COMM_LEN)
		__field(pid_t,	pid		)
		__field(int, src_cpu)
		__field(int, dst_cpu)
		__field(int, task_core_occupation)
		__field(int, idle_core_occupation)
		__field(unsigned long, cookie)
	),

	TP_fast_assign(
		memcpy_s(__entry->comm, TASK_COMM_LEN, p->comm, TASK_COMM_LEN);
		__entry->pid	= p->pid;
		__entry->src_cpu = src;
		__entry->dst_cpu = dst;
		__entry->task_core_occupation = p->core_occupation;
		__entry->idle_core_occupation = idle_core_occupation;
		__entry->cookie	= cookie;
	),

	TP_printk("core fill: %s/%d (%d->%d) %d %d %lx",
		  __entry->comm, __entry->pid, __entry->src_cpu, __entry->dst_cpu,
		  __entry->task_core_occupation, __entry->idle_core_occupation, __entry->cookie)
);

TRACE_EVENT(task_update_cookie,

	TP_PROTO(struct task_struct *p, unsigned long old_cookie, unsigned long new_cookie),

	TP_ARGS(p, old_cookie, new_cookie),

	TP_STRUCT__entry(
		__array(char,	comm,	TASK_COMM_LEN)
		__field(pid_t,	pid		)
		__field(unsigned long, old_cookie)
		__field(unsigned long, new_cookie)
	),

	TP_fast_assign(
		memcpy_s(__entry->comm, TASK_COMM_LEN, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->old_cookie	= old_cookie;
		__entry->new_cookie	= new_cookie;
	),

	TP_printk("%s/%d update cookie: %lx -> %lx",
		  __entry->comm, __entry->pid, __entry->old_cookie, __entry->new_cookie)
);

#endif /* _TRACE_CORE_SCHED_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
