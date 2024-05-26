/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * phase_feature.h
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

#ifndef __PHASE_FEATURE_H
#define __PHASE_FEATURE_H

#include <linux/phase.h>
#include "phase_perf.h"

#ifdef CONFIG_PHASE

#define PHASE_SCALE(x)         ((x) * 10000)

DECLARE_STATIC_KEY_FALSE(sched_phase);

extern struct cpumask sysctl_phase_cpumask;
extern int sysctl_phase_nop_fevents[PHASE_FEVENT_NUM];
extern int sysctl_phase_nop_pevents[PHASE_PEVENT_NUM];
extern unsigned int sysctl_phase_reverse_enabled;
extern unsigned int sysctl_phase_reverse_default_val;
extern unsigned int sysctl_phase_forward_enabled;
extern unsigned int sysctl_phase_forward_default_val;
extern unsigned int sysctl_phase_calc_feature_mode;
extern unsigned int sysctl_phase_invalid_interval_ns;
extern unsigned int sysctl_phase_counter_window_size[NR_SMT_MODE];
extern unsigned int sysctl_phase_counter_window_policy;
extern unsigned int sysctl_phase_nr_core_find;
extern unsigned int sysctl_phase_check_interval_ns;

static inline bool phase_task_valid(struct task_struct *p, enum smt_mode smt)
{
	return p->phase_info->valid[smt];
}

static inline bool phase_task_new(struct task_struct *p)
{
	return p->phase_info->new_task;
}

#define PHASE_TK_NAME_SIZE		20
extern char phase_tk_name[PHASE_TK_NAME_SIZE];

enum phase_predict_mode {
	FORWARD_MODE = ST_MODE,
	REVERSE_MODE = SMT_MODE,
	NR_PREDICT_MODE,
};

struct phase_think_class {
	unsigned int id;
	char *name;
	struct phase_think_class *next;
	int *pevents;
	int *fevents;
	int (*create)(void);
	void (*release)(void);
	bool (*predict)(struct task_struct *picked,
			struct task_struct *pick,
			enum phase_predict_mode predict_mode);
	struct task_struct* (*sched_core_find)(struct rb_node *node,
					       unsigned long cookie,
					       struct task_struct *picked);
	void *private_data;
};

#define PHASE_TK_ID_NOP			0
#define PHASE_TK_ID_DECISION_TREE	1
#define PHASE_TK_ID_KMEANS_CLUTER	2

extern struct phase_think_class *phase_tk;
extern struct phase_think_class phase_tk_nop;
extern struct phase_think_class phase_tk_decision_tree;
extern struct phase_think_class phase_tk_kmeans_lsvm;

static inline int phase_tk_create(void)
{
	int err;

	err = phase_perf_create(phase_tk->pevents, phase_tk->fevents, &sysctl_phase_cpumask);
	if (err)
		return err;

	if (phase_tk->create) {
		err = phase_tk->create();
		if (err) {
			phase_perf_release(&sysctl_phase_cpumask);
			return err;
		}
	}

	return 0;
}

static inline void phase_tk_release(void)
{
	phase_perf_release(&sysctl_phase_cpumask);

	if (phase_tk->release)
		phase_tk->release();
}

static inline bool phase_tk_has_sched_core_find(void)
{
	return phase_tk->sched_core_find && (!!phase_tk->predict);
}

static inline struct task_struct *phase_tk_sched_core_find(struct rb_node *node,
							   unsigned long cookie,
							   struct task_struct *picked)
{
	struct task_struct *phase_core_pick = NULL;

	if (phase_tk->sched_core_find)
		phase_core_pick = phase_tk->sched_core_find(node, cookie, picked);

	return phase_core_pick;
}

static inline bool phase_tk_predict(struct task_struct *picked,
				    struct task_struct *pick,
				    enum phase_predict_mode predict_mode)
{
	if (unlikely(phase_task_new(picked)) ||
	    unlikely(phase_task_new(pick))) {
		picked->phase_info->new_task = false;
		pick->phase_info->new_task = false;
		return true;
	}

	if (!phase_task_valid(picked, (enum smt_mode)predict_mode) ||
	    !phase_task_valid(pick, (enum smt_mode)predict_mode))
		return false;

	if (picked == pick)
		return false;

	if (phase_tk->predict)
		return phase_tk->predict(picked, pick, predict_mode);

	return false;
}

#ifdef CONFIG_PHASE_COUNTER_WINDOW
struct phase_event_pcount *phase_active_counter(struct phase_counter_queue *queue,
						enum phase_window_policy policy);

static inline struct phase_event_pcount *phase_get_counter(struct phase_info *info,
							   enum smt_mode smt)
{
	struct phase_counter_queue *queue = &info->window.queue[smt];

	return phase_active_counter(queue, sysctl_phase_counter_window_policy);
}

#else

static inline struct phase_event_pcount *phase_active_counter(phase_counter_window *window)
{
	return window;
}

static inline struct phase_event_pcount *phase_get_counter(struct phase_info *info,
							   enum smt_mode smt)
{
	return phase_active_counter(&info->window);
}
#endif

struct task_struct *phase_sched_core_find_default(struct rb_node *node,
						  unsigned long cookie,
						  struct task_struct *picked);

void phase_feature_reverse_tick(struct rq *rq, struct task_struct *p, int cpu);

int phase_proc_update_thinking(struct ctl_table *table, int write,
			       void __user *buffer, size_t *lenp, loff_t *ppos);
bool phase_task_need_check_reverse(const struct task_struct *p);

#else

static inline void phase_feature_reverse_tick(struct rq *rq, struct task_struct *p, int cpu) { }

static inline int phase_proc_update_thinking(struct ctl_table *table, int write,
					     void __user *buffer, size_t *lenp, loff_t *ppos) { return 0; }
static inline bool phase_task_need_check_reverse(const struct task_struct *p) { return false; };

#endif /* CONFIG_PHASE */

extern bool consume_line(const char *str, s64 *start, const s64 end);
extern void get_matches(const char *str, const char *format, ...);
extern char *load_from_file(const char *file_path, loff_t *size);
extern int parse_one_line(int *arr, char *buf, int col);

#endif /* __PHASE_FEATURE_H */
