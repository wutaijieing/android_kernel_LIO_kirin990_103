/*
 * task_pool.h
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

#ifndef _TASK_POOL_H_
#define _TASK_POOL_H_

#include <linux/types.h>
#include "npu_common.h"

int32_t task_pool_init(void);
int32_t task_pool_deinit(void);
struct npu_id_entity *task_pool_alloc_task(void);
int32_t task_pool_free_task(struct npu_id_entity *sched_task);

#endif
