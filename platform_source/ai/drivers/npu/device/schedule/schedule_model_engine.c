/*
 * schedule_model_engine.c
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
#include "schedule_model_engine.h"

#include "hwts_channel_manager.h"
#include "hwts_config.h"
#include "hwts_driver.h"
#include "npu_log.h"
#include "placeholder.h"
#include "schedule_model.h"
#include "schedule_model_manager.h"

struct schedule_model_engine g_sched_model_engine;

static int sched_model_engine_push_pholder_by_sort(struct placeholder *pholder);

uint16_t sched_model_engine_priority_to_group(uint16_t priority)
{
	cond_return_error(priority >= HWTS_PRIORITY_GRP_MAX, MAX_UINT16_NUM, "priority invalid");
	return (priority <= HWTS_SP_LEVEL ? priority : NPU_MODEL_PRIORITY_GROUP_NON_PREEMPT);
}

int sched_model_engine_init(void)
{
	int ret = 0;
	int i;

	for (i = 0; i < NPU_MODEL_PRIORITY_GROUP_MAX; i++) {
		INIT_LIST_HEAD(&g_sched_model_engine.waiting_model[i].list);
		g_sched_model_engine.waiting_model[i].count = 2;
	}

	INIT_LIST_HEAD(&g_sched_model_engine.executing_model_list);

	hwts_channel_mngr_init();

	ret = pholder_mngr_init();
	cond_return_error(ret != 0, ret, "pholder_mngr_init error");

	return ret;
}

int sched_model_engine_push_model(struct schedule_model *sched_model)
{
	int ret = 0;
	struct placeholder *pholder = NULL;

	pholder = pholder_mngr_alloc_node();
	cond_return_error(pholder == NULL, -1, "pholder_mngr_alloc_node error");

	pholder->model_id = sched_model->id;
	pholder->priority = sched_model->priority;
	pholder->pid = sched_model->pid;
	ret = sched_model_engine_push_pholder_by_sort(pholder);
	cond_return_error(ret != 0, ret, "sched_model_engine_push_pholder_by_sort error");

	return ret;
}

int sched_model_engine_empty(uint16_t priority)
{
	uint16_t priority_group = sched_model_engine_priority_to_group(priority);
	cond_return_error(priority_group >= NPU_MODEL_PRIORITY_GROUP_MAX, -1, "invalid priority");

	return list_empty_careful(&g_sched_model_engine.waiting_model[priority_group].list);
}

int sched_model_engine_run(uint16_t priority)
{
	struct placeholder *pholder_pos = NULL;
	struct placeholder *pholder_n = NULL;
	struct schedule_model *sched_model = NULL;
	struct hwts_channel *hwts_channel_alloced = NULL;
	struct hwts_sq_cq *hwts_sq_cq_pos = NULL;
	struct hwts_sq_cq *hwts_sq_cq_n = NULL;
	uint16_t hwts_channel_group;
	uint16_t priority_group = sched_model_engine_priority_to_group(priority);
	cond_return_error(priority_group >= NPU_MODEL_PRIORITY_GROUP_MAX, -1, "invalid priority");

	list_for_each_entry_safe_reverse(pholder_pos, pholder_n,
		&g_sched_model_engine.waiting_model[priority_group].list, node) {
		if (g_sched_model_engine.waiting_model[priority_group].count == 0){
			break;
		}

		sched_model = sched_model_mngr_get_model(pholder_pos->model_id);
		sched_model->schedule_status = SHCED_MODEL_STATUS_RUNNING;

		hwts_channel_group = hwts_config_get_group_id(sched_model->rtsq_type, priority);
		if (sched_model->sqcq_count > hwts_channel_get_free_count(hwts_channel_group)) {
			continue;
		}

		list_for_each_entry_safe(hwts_sq_cq_pos, hwts_sq_cq_n, &sched_model->sqcq_list, node) {
			hwts_channel_alloced = hwts_channel_mngr_alloc_channel(hwts_channel_group);
			cond_return_error(hwts_channel_alloced == NULL, -1, "hwts_channel_mngr_alloc_channel error");

			hwts_sq_cq_pos->priority = sched_model->priority;
			hwts_channel_alloced->sq_id = hwts_sq_cq_pos->schedule_sq.id;
			hwts_sq_cq_pos->channel_id = hwts_channel_alloced->id;
			hwts_driver_schedule_sq(hwts_sq_cq_pos, hwts_channel_alloced->id);
		}

		g_sched_model_engine.waiting_model[priority_group].count--;
		list_move(&pholder_pos->node, &g_sched_model_engine.executing_model_list);
	}

	return 0;
}

static int sched_model_engine_push_pholder_by_sort(struct placeholder *pholder)
{
	struct placeholder *pos = NULL;
	struct placeholder *n = NULL;
	uint16_t priority_group = sched_model_engine_priority_to_group(pholder->priority);
	cond_return_error(priority_group >= NPU_MODEL_PRIORITY_GROUP_MAX, -1, "invalid priority");

	list_add(&pholder->node, &g_sched_model_engine.waiting_model[priority_group].list);
	if (priority_group < NPU_MODEL_PRIORITY_GROUP_NON_PREEMPT)
		return 0;

	// priority is non-preempt
	int index = 0;
	list_for_each_entry_safe(pos, n, &g_sched_model_engine.waiting_model[priority_group].list, node) {
		index++;
		if (pos->pid == pholder->pid && pos->priority> pholder->priority) {
			swap(pos->priority, pholder->priority);
			swap(pos->model_id, pholder->model_id);
			pholder = pos;
		}
	}

	return 0;
}
