/*
 * hwts_sq_cq_manager.c
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

#include "hwts_sq_cq_manager.h"
#include "npu_platform_resource.h"

#include <linux/list.h>
#include "npu_log.h"
#include "npu_platform_resource.h"
struct hwts_sq_cq g_hwts_sq_cq_buf[NPU_MAX_HWTS_SQ_NUM];

void hwts_sq_cq_mngr_init(void)
{
	int idx;
	for (idx = 0; idx < NPU_MAX_HWTS_SQ_NUM; idx++)
		g_hwts_sq_cq_buf[idx].schedule_sq.id = idx;
}

struct hwts_sq_cq* hwts_sq_cq_mngr_get_sqcq(uint16_t sq_id)
{
	cond_return_error(sq_id >= NPU_MAX_HWTS_SQ_NUM, NULL, "id exceeds the maximum, sq_id = %u", sq_id);
	return &g_hwts_sq_cq_buf[sq_id];
}