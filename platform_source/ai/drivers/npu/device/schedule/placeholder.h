/*
 * placeholder.h
 *
 * about placeholder
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
#ifndef _PLACEHOLDER_H
#define _PLACEHOLDER_H

#include <linux/list.h>
#include <linux/types.h>
#include "npu_common_resource.h"

struct placeholder {
	struct list_head node;
	uint16_t model_id;
	uint16_t priority;
	pid_t pid;
};

struct placeholder_manager {
	struct list_head idle_list;
	struct placeholder buffer[NPU_MAX_MODEL_ID];
};

int pholder_mngr_init(void);
struct placeholder *pholder_mngr_alloc_node(void);
void pholder_mngr_free_node(struct placeholder *pholder);

#endif /* _PLACEHOLDER_H */
