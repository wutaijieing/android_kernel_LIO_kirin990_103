/*
 * schedule_stream_manager.c
 *
 * about schedule stream manager
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
#include "schedule_stream_manager.h"
#include "npu_user_common.h"
#include "npu_platform_resource.h"
#include "task_pool.h"
#include "npu_log.h"

struct schedule_stream g_sched_stream_buff[NPU_MAX_NON_SINK_STREAM_ID];

struct schedule_stream *sched_stream_mngr_get_stream(int32_t stream_id)
{
	cond_return_error(stream_id >= NPU_MAX_NON_SINK_STREAM_ID, NULL,
		"hwts stream_id is error:%u", stream_id);

	return &g_sched_stream_buff[stream_id];
}
