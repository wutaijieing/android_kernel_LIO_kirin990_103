/*
 * schedule_model_engine.h
 *
 * about schedule model engine
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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
#ifndef _SCHEDULE_MODEL_ENGINE_H
#define _SCHEDULE_MODEL_ENGINE_H

#include <linux/list.h>
#include <linux/types.h>

#include "schedule_model.h"
#include "npu_hwts_plat.h"
#include "hwts_plat.h"

#define NPU_MODEL_PRIORITY_GROUP_NON_PREEMPT ((HWTS_SP_LEVEL) + 1)
#define NPU_MODEL_PRIORITY_GROUP_MAX ((NPU_MODEL_PRIORITY_GROUP_NON_PREEMPT) + 1)

struct waiting_model_struct {
	struct list_head list;
	uint32_t count;
};

struct schedule_model_engine {
	struct waiting_model_struct waiting_model[NPU_MODEL_PRIORITY_GROUP_MAX];
	struct list_head executing_model_list;
};

extern struct schedule_model_engine g_sched_model_engine;

int sched_model_engine_init(void);

int sched_model_engine_push_model(struct schedule_model *sched_model);

int sched_model_engine_empty(uint16_t priority);

int sched_model_engine_run(uint16_t priority);

uint16_t sched_model_engine_priority_to_group(uint16_t priority);
#endif /* _SCHEDULE_MODEL_ENGINE_H */
