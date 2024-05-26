/*
 * schedule_model_manager.c
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

#include "schedule_model_manager.h"
#include "npu_log.h"
#include "npu_user_common.h"

struct schedule_model g_sched_model_buf[NPU_MAX_MODEL_ID];

void sched_model_mngr_init(void)
{
	uint16_t idx;
	for (idx = 0; idx < NPU_MAX_MODEL_ID; idx++) {
		INIT_LIST_HEAD(&g_sched_model_buf[idx].sqcq_list);
		INIT_LIST_HEAD(&g_sched_model_buf[idx].hwts_event_list);
		INIT_LIST_HEAD(&g_sched_model_buf[idx].schedule_stream_list);

		g_sched_model_buf[idx].id = idx;
		g_sched_model_buf[idx].is_valid = false;
		g_sched_model_buf[idx].rtsq_type = HWTS_RTSQ_TYPE_NS_AIC;
		g_sched_model_buf[idx].sqcq_count = 0;
		g_sched_model_buf[idx].sqcq_exec_count = 0;
		g_sched_model_buf[idx].schedule_status = SCHED_MODEL_STATUS_IDLE;
		g_sched_model_buf[idx].priority = MAX_UINT16_NUM;
		g_sched_model_buf[idx].pid = MAX_UINT16_NUM;
	}
}

struct schedule_model* sched_model_mngr_get_model(uint16_t sched_model_id)
{
	if (sched_model_id >= NPU_MAX_MODEL_ID) {
		npu_drv_err("schedule model id exceeds the maximum, sched_model_id = %u", sched_model_id);
		return NULL;
	}

	return &g_sched_model_buf[sched_model_id];
}