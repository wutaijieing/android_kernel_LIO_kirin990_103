/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * km_cluster.h
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
#define TRACE_SYSTEM km_cluster

#if !defined(_TRACE_KM_TREE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_KM_TREE_H

#include <linux/phase.h>
#include <linux/tracepoint.h>
#include <securec.h>

TRACE_EVENT(km_cluster_predict_buffer,

	TP_PROTO(struct task_struct *pleft, struct task_struct *pright,
		 u64 *buffer, enum smt_mode mode),

	TP_ARGS(pleft, pright, buffer, mode),

	TP_STRUCT__entry(
		__array(	char,	left_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	left_pid			)
		__array(	char,	right_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	right_pid			)
		__field(	int,	mode			)

		__field(	u64,	buffer0			)
		__field(	u64,	buffer1			)
		__field(	u64,	buffer2			)
		__field(	u64,	buffer3			)
		__field(	u64,	buffer4			)
		__field(	u64,	buffer5			)
		__field(	u64,	buffer6			)
		__field(	u64,	buffer7			)
		__field(	u64,	buffer8			)
		__field(	u64,	buffer9			)
		__field(	u64,	buffer10		)
	),

	TP_fast_assign(
		memcpy_s(__entry->left_comm, TASK_COMM_LEN, pleft->comm, TASK_COMM_LEN);
		__entry->left_pid	= pleft->pid;
		memcpy_s(__entry->right_comm, TASK_COMM_LEN, pright->comm, TASK_COMM_LEN);
		__entry->right_pid	= pright->pid;
		__entry->mode		= mode;
		__entry->buffer0	= buffer[0];
		__entry->buffer1	= buffer[1];
		__entry->buffer2	= buffer[2];
		__entry->buffer3	= buffer[3];
		__entry->buffer4	= buffer[4];
		__entry->buffer5	= buffer[5];
		__entry->buffer6	= buffer[6];
		__entry->buffer7	= buffer[7];
		__entry->buffer8	= buffer[8];
		__entry->buffer9	= buffer[9];
		__entry->buffer10	= buffer[10];
	),

	TP_printk("mode %d "
		  "left_comm=%s left_pid=%d left_buffer=%llu/%llu/%llu/%llu/%llu ===> "
		  "right_comm=%s right_pid=%d right_buffer=%llu/%llu/%llu/%llu/%llu "
		  "other_buffer=%llu",
		    __entry->mode,
		    __entry->left_comm, __entry->left_pid,
		    __entry->buffer0, __entry->buffer1, __entry->buffer2, __entry->buffer3, __entry->buffer4,
		    __entry->right_comm, __entry->right_pid,
		    __entry->buffer5, __entry->buffer6, __entry->buffer7, __entry->buffer8,__entry->buffer9,
		    __entry->buffer10)
);

TRACE_EVENT(km_cluster_predict_pmu,

	TP_PROTO(struct task_struct *pleft, struct task_struct *pright,
		 struct phase_event_pcount *cleft, struct phase_event_pcount *cright,
		 enum smt_mode mode, bool match),

	TP_ARGS(pleft, pright, cleft, cright, mode, match),

	TP_STRUCT__entry(
		__array(	char,	left_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	left_pid			)
		__array(	char,	right_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	right_pid			)
		__field(	bool,	match			)
		__field(	int,	mode			)

		__field(	u64,	cleft0			)
		__field(	u64,	cleft1			)
		__field(	u64,	cleft2			)
		__field(	u64,	cleft3			)
		__field(	u64,	cleft4			)
		__field(	u64,	cleft5			)
		__field(	u64,	cleft6			)
		__field(	u64,	cleft7			)
		__field(	u64,	winleft			)

		__field(	u64,	cright0			)
		__field(	u64,	cright1			)
		__field(	u64,	cright2			)
		__field(	u64,	cright3			)
		__field(	u64,	cright4			)
		__field(	u64,	cright5			)
		__field(	u64,	cright6			)
		__field(	u64,	cright7			)
		__field(	u64,	winright		)
	),

	TP_fast_assign(
		memcpy_s(__entry->left_comm, TASK_COMM_LEN, pleft->comm, TASK_COMM_LEN);
		__entry->left_pid	= pleft->pid;
		memcpy_s(__entry->right_comm, TASK_COMM_LEN, pright->comm, TASK_COMM_LEN);
		__entry->right_pid	= pright->pid;
		__entry->match		= match;
		__entry->mode		= mode;
		__entry->cleft0		= cleft->data[0];
		__entry->cleft1		= cleft->data[1];
		__entry->cleft2		= cleft->data[2];
		__entry->cleft3		= cleft->data[3];
		__entry->cleft4		= cleft->data[4];
		__entry->cleft5		= cleft->data[5];
		__entry->cleft6		= cleft->data[6];
		__entry->cleft7		= cleft->data[7];
		__entry->winleft	= phase_active_window_size(&pleft->phase_info->window, mode);

		__entry->cright0	= cright->data[0];
		__entry->cright1	= cright->data[1];
		__entry->cright2	= cright->data[2];
		__entry->cright3	= cright->data[3];
		__entry->cright4	= cright->data[4];
		__entry->cright5	= cright->data[5];
		__entry->cright6	= cright->data[6];
		__entry->cright7	= cright->data[7];
		__entry->winright	= phase_active_window_size(&pright->phase_info->window, mode);

	),

	TP_printk("match=%d mode=%d "
		  "left_comm=%s left_pid=%d left_pmu=%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu left_win=%llu ===> "
		  "right_comm=%s right_pid=%d right_pmu=%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu right_win=%llu",
		    __entry->match, __entry->mode,
		    __entry->left_comm, __entry->left_pid,
		    __entry->cleft0, __entry->cleft1, __entry->cleft2,
		    __entry->cleft3, __entry->cleft4, __entry->cleft5,
		    __entry->cleft6, __entry->cleft7, __entry->winleft,
		    __entry->right_comm, __entry->right_pid,
		    __entry->cright0, __entry->cright1, __entry->cright2,
		    __entry->cright3, __entry->cright4, __entry->cright5,
		    __entry->cright6, __entry->cright7, __entry->winright)
);

TRACE_EVENT(km_cluster_make_decision,

	TP_PROTO(unsigned long long  fdis_min, int lsvm_col, int lsvm,
		 long long sum, int lower, int upper, int threshold),

	TP_ARGS(fdis_min, lsvm_col, lsvm, sum, lower, upper, threshold),

	TP_STRUCT__entry(
		__field(	unsigned long long,	fdis_min		)
		__field(	int,			lsvm_col		)
		__field(	int,			lsvm			)
		__field(	long long,		sum			)
		__field(	int,			lower			)
		__field(	int,			upper			)
		__field(	int,			threshold		)
	),

	TP_fast_assign(
		__entry->fdis_min	= fdis_min;
		__entry->lsvm_col	= lsvm_col;
		__entry->lsvm		= lsvm;
		__entry->sum		= sum;
		__entry->lower		= lower;
		__entry->upper		= upper;
		__entry->threshold	= threshold;
	),

	TP_printk("fdis_min=%llu lsvm_col=%d lsvm=%d sum=%lld "
		  "lower=%d upper=%d threshold=%d",
		    __entry->fdis_min, __entry->lsvm_col, __entry->lsvm, __entry->sum,
		    __entry->lower, __entry->upper, __entry->threshold)

);
#endif

/* This part must be outside protection */
#include <trace/define_trace.h>
