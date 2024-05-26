/*
 * task_pool.c
 *
 * about task pool
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

#include "task_pool.h"
#include "npu_rt_task.h"
#include "npu_id_allocator.h"

static struct npu_id_allocator g_sched_task_pool_allocator;

int32_t task_pool_init(void)
{
	return npu_id_allocator_create(&g_sched_task_pool_allocator, 0, 1024, sizeof(npu_rt_task_t));
}

int32_t task_pool_deinit(void)
{
	return npu_id_allocator_destroy(&g_sched_task_pool_allocator);
}

struct npu_id_entity *task_pool_alloc_task(void)
{
	return npu_id_allocator_alloc(&g_sched_task_pool_allocator);
}

int32_t task_pool_free_task(struct npu_id_entity *sched_task)
{
	return npu_id_allocator_free(&g_sched_task_pool_allocator, sched_task->id);
}
