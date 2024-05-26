/*
 * event_manager.c
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

#include "event_manager.h"

struct event g_event_buf[NPU_MAX_HWTS_EVENT_ID];

void event_mngr_init(void)
{
	uint16_t idx;
	for (idx = 0; idx < NPU_MAX_HWTS_EVENT_ID; idx++)
		event_init(&g_event_buf[idx]);
}

struct event* event_mngr_get_event(uint16_t id)
{
	if (id < NPU_MAX_HWTS_EVENT_ID)
		return &g_event_buf[id];

	return NULL;
}