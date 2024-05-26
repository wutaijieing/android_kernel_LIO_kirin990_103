/*
 * hwts_channel_manager.h
 *
 * about hwts channel manager
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

#ifndef _HWTS_CHANNEL_MANAGER_H_
#define _HWTS_CHANNEL_MANAGER_H_

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include "npu_hwts_plat.h"
#include "hwts_plat.h"

struct  hwts_channel {
	struct list_head node;
	uint16_t id;
	uint16_t sq_id;
	uint16_t grp_id;
};

struct hwts_group {
	uint16_t id;
	uint16_t free_num;
	struct list_head idle_channel_list;
	struct list_head work_channel_list;
};

struct hwts_channel_manager {
	struct hwts_group group[HWTS_GROUP_NUM];
	struct hwts_channel channel[HWTS_CHANNEL_NUM];
};

void hwts_channel_mngr_init (void);
struct hwts_channel *hwts_mngr_get_channel(uint16_t channel_id);
struct hwts_channel *hwts_channel_mngr_alloc_channel(uint16_t group_id);
void hwts_channel_mngr_free_channel(struct hwts_channel *channel);
uint16_t hwts_channel_get_free_count(uint16_t group_id);

#endif
