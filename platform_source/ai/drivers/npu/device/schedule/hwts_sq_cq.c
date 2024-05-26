/*
 * hwts_sq_cq.c
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
#include "hwts_sq_cq.h"
#include "npu_log.h"
#include "npu_pool.h"

void hwts_sq_cq_set_sq(struct hwts_sq_cq *hwts_sqcq, struct npu_hwts_sq_info *hwts_sq_info)
{
	cond_return_void(hwts_sqcq == NULL || hwts_sq_info == NULL, "hwts_sqcq or hwts_sq_info is null");

	hwts_sqcq->schedule_sq.base_addr = hwts_sq_info->iova_addr;
	hwts_sqcq->schedule_sq.head = hwts_sq_info->head;
	hwts_sqcq->schedule_sq.tail = hwts_sq_info->tail;
	hwts_sqcq->schedule_sq.length = hwts_sq_info->length;
}

struct schedule_sq_cq *hwts_sq_cq_get_sq(struct hwts_sq_cq *hwts_sqcq)
{
	if (hwts_sqcq == NULL) {
		npu_drv_err("hwts_sqcq is null");
		return NULL;
	}

	return &hwts_sqcq->schedule_sq;
}

void hwts_sq_cq_set_cq(struct hwts_sq_cq *hwts_sqcq, struct npu_hwts_cq_info *hwts_cq_info)
{
	cond_return_void(hwts_sqcq == NULL || hwts_cq_info == NULL, "hwts_sqcq or hwts_cq_info is null");

	hwts_sqcq->schedule_cq.base_addr = hwts_cq_info->iova_addr;
	hwts_sqcq->schedule_cq.head = hwts_cq_info->head;
	hwts_sqcq->schedule_cq.tail = hwts_cq_info->tail;
}

struct schedule_sq_cq *hwts_sq_cq_get_cq(struct hwts_sq_cq *hwts_sqcq)
{
	if (hwts_sqcq == NULL) {
		npu_drv_err("hwts_sqcq is null");
		return NULL;
	}

	return &hwts_sqcq->schedule_cq;
}