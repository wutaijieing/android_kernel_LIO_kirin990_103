/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * dc_tree.h
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
#define TRACE_SYSTEM dc_tree

#if !defined(_TRACE_DC_TREE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_DC_TREE_H

#include <linux/phase.h>
#include <linux/tracepoint.h>
#include <securec.h>

TRACE_EVENT(dc_tree_predict,

	TP_PROTO(struct task_struct *pleft, struct task_struct *pright,
		 u64 *buffer, enum smt_mode mode, bool match),

	TP_ARGS(pleft, pright, buffer, mode, match),

	TP_STRUCT__entry(
		__array(	char,	left_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	left_pid			)
		__array(	char,	right_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	right_pid			)
		__field(	enum smt_mode,		mode	)
		__field(	bool,	match			)

		__field(	u64,	bleft0			)
		__field(	u64,	bleft1			)
		__field(	u64,	bleft2			)
		__field(	u64,	bleft3			)
		__field(	u64,	bleft4			)

		__field(	u64,	bright0			)
		__field(	u64,	bright1			)
		__field(	u64,	bright2			)
		__field(	u64,	bright3			)
		__field(	u64,	bright4			)
	),

	TP_fast_assign(
		memcpy_s(__entry->left_comm, TASK_COMM_LEN, pleft->comm, TASK_COMM_LEN);
		__entry->left_pid	= pleft->pid;
		memcpy_s(__entry->right_comm, TASK_COMM_LEN, pright->comm, TASK_COMM_LEN);
		__entry->right_pid	= pright->pid;
		__entry->mode		= mode;
		__entry->match		= match;
		__entry->bleft0		= buffer[0];
		__entry->bleft1		= buffer[1];
		__entry->bleft2		= buffer[2];
		__entry->bleft3		= buffer[3];
		__entry->bleft4		= buffer[4];
		__entry->bright0	= buffer[5];
		__entry->bright1	= buffer[6];
		__entry->bright2	= buffer[7];
		__entry->bright3	= buffer[8];
		__entry->bright4	= buffer[9];
	),

	TP_printk("match=%d mode=%d "
		  "left_comm=%s left_pid=%d left_buffer=%llu/%llu/%llu/%llu/%llu ==> "
		  "right_comm=%s right_pid=%d right_buffer=%llu/%llu/%llu/%llu/%llu",
		   __entry->match, __entry->mode,
		   __entry->left_comm, __entry->left_pid,
		   __entry->bleft0, __entry->bleft1, __entry->bleft2,
		   __entry->bleft3, __entry->bleft4,
		   __entry->right_comm, __entry->right_pid,
		   __entry->bright0, __entry->bright1, __entry->bright2,
		   __entry->bright3, __entry->bright4)
);

TRACE_EVENT(dc_tree_predict_pmu,

	TP_PROTO(struct task_struct *pleft, struct task_struct *pright,
		 struct phase_event_pcount *cleft, struct phase_event_pcount *cright,
		 enum smt_mode mode, bool match),

	TP_ARGS(pleft, pright, cleft, cright, mode, match),

	TP_STRUCT__entry(
		__array(	char,	left_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	left_pid			)
		__array(	char,	right_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	right_pid			)
		__field(	enum smt_mode,	mode		)
		__field(	bool,	match			)

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
		__entry->mode		= mode;
		__entry->match		= match;
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
		  "left_comm=%s left_pid=%d left_pmu=%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu left_win=%llu ==> "
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

#endif

/* This part must be outside protection */
#include <trace/define_trace.h>
