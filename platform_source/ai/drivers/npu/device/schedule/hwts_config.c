/*
 * hwts_config.c
 *
 * about hwts config
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
#include "hwts_config.h"
#include <linux/kernel.h>
#include "npu_hwts_plat.h"
#include "hwts_plat.h"
#include "npu_user_common.h"
#include "npu_log.h"

uint16_t hwts_config_get_group_channel_count(uint16_t group_id)
{
	struct hwts_group_item *hwts_group_config_table = NULL;

	if (group_id >= HWTS_GROUP_NUM) {
		npu_drv_warn("hwts group_id is error:%u", group_id);
		return MAX_UINT16_NUM;
	}

	hwts_group_config_table = get_hwts_group_config_table();
	return hwts_group_config_table[group_id].channel_count;
}

uint16_t hwts_config_get_group_start_channel_id(uint16_t group_id)
{
	struct hwts_group_item *hwts_group_config_table = NULL;

	if (group_id >= HWTS_GROUP_NUM) {
		npu_drv_warn("hwts group_id is error:%u", group_id);
		return MAX_UINT16_NUM;
	}

	hwts_group_config_table = get_hwts_group_config_table();
	return hwts_group_config_table[group_id].start_channel_id;
}

uint16_t hwts_config_get_group_id(uint16_t rtsq_type, uint16_t priority)
{
	uint16_t i;
	struct hwts_priority_item *item = NULL;
	uint16_t priority_config_size = get_hwts_priority_config_table_size();
	struct hwts_priority_item *hwts_priority = get_hwts_priority_config_table();

	for (i = 0; i < priority_config_size; ++i) {
		item = &hwts_priority[i];
		if (item->rtsq_type == rtsq_type && item->priority == priority)
			return item->group_id;
	}
	npu_drv_warn("get hwts group_id error, rtsq_type:%u, priority:%u", rtsq_type, priority);
	return MAX_UINT16_NUM;
}
