/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef SCENE_ID_MGR_H
#define SCENE_ID_MGR_H

#include <linux/list.h>
#include <linux/types.h>
#include <linux/semaphore.h>
#include "dkmd_res_mgr.h"
#include "dkmd_log.h"

struct scene_id_node {
	struct list_head list_node;

	struct scene_id_info id_info;
	atomic_t ref_cnt;
};

struct dpu_scene_id_mgr {
	struct semaphore sem;
	struct list_head scene_id_list; // struct opr is scene_id_node
};

void dpu_res_register_scene_id_mgr(struct list_head *resource_head);
#endif