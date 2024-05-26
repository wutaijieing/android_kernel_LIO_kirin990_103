// SPDX-License-Identifier: GPL-2.0
/*
 * smt_mode_gov.h
 *
 * smt mode governor header file
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

#ifndef __SMT_MODE_GOV_H
#define __SMT_MODE_GOV_H

#include <linux/sched/smt.h>

struct smt_cpu_state {
	rwlock_t state_lock; /* protect following fields */
	cpumask_t smt_mask;
	cpumask_t offline_mask;
	cpumask_t isolated_mask;
	cpumask_t idle_mask;
	struct task_struct *update_thread;
	unsigned int req_cpus;
	unsigned int user_req_mode;
	unsigned int target_mode;
	unsigned int curr_mode;
	unsigned int nr_switch;
	unsigned int cpu_max_cap[NR_SMT_MODE];
	bool need_update;
};

void set_smt_cpu_req(int cpu, unsigned int req);
unsigned int get_smt_cpu_req(int cpu);
#endif
