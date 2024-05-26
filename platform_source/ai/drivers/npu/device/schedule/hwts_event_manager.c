/*
 * hwts_event_manager.c
 *
 * Copyright (c) 2012-2021 Huawei Technologies Co., Ltd.
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

#include "hwts_event_manager.h"
#include "npu_model_description.h"
#include "npu_log.h"

struct hwts_event g_hwts_event_buf[NPU_MAX_HWTS_EVENT_ID];

void hwts_event_mngr_init(void)
{
	uint16_t idx;
	for (idx = 0; idx < NPU_MAX_HWTS_EVENT_ID; idx++)
		g_hwts_event_buf[idx].id = idx;
}

struct hwts_event *hwts_event_mngr_get_event(uint16_t event_id)
{
	cond_return_error(event_id >= NPU_MAX_HWTS_EVENT_ID, NULL, "id exceeds the maximum, event_id = %u", event_id);
	return &g_hwts_event_buf[event_id];
}

