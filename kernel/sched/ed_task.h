// SPDX-License-Identifier: GPL-2.0
/*
 * ed_task.h
 *
 * early detection header file
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

#ifndef __ED_TASK_H
#define __ED_TASK_H

#include <linux/sched.h>

#define EARLY_DETECTION_TASK_WAITING_DURATION 11500000
#define EARLY_DETECTION_TASK_RUNNING_DURATION 120000000
#define EARLY_DETECTION_NEW_TASK_RUNNING_DURATION 100000000

struct rq;

bool is_ed_task(struct rq *rq, struct task_struct *p, u64 wall);
bool early_detection_notify(struct rq *rq, u64 wallclock);
void clear_ed_task(struct task_struct *p, struct rq *rq);
void migrate_ed_task(struct task_struct *p, struct rq *src_rq,
		     struct rq *dest_rq, u64 wallclock);
void note_task_waking(struct task_struct *p, u64 wallclock);
#endif
