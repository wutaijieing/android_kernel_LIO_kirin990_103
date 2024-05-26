/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 * Description:
 * Author:
 * Create: 2021-07-08
 */
#ifndef __HWTS_SQ_CQ_H
#define __HWTS_SQ_CQ_H

#include <linux/types.h>
#include <linux/list.h>
#include "npu_model_description.h"

struct schedule_sq_cq {
	uint16_t id;
	uint16_t head;
	uint16_t tail;
	uint16_t length;
	uint64_t base_addr;
};

struct hwts_sq_cq{
	struct list_head node;
	struct schedule_sq_cq schedule_sq;
	struct schedule_sq_cq schedule_cq;
	uint16_t model_id;
	uint16_t channel_id;
	uint16_t smmu_substream_id;
	uint16_t priority;
};

void hwts_sq_cq_set_sq(struct hwts_sq_cq *hwts_sqcq, struct npu_hwts_sq_info *hwts_sq_info);
struct schedule_sq_cq *hwts_sq_cq_get_sq(struct hwts_sq_cq *hwts_sqcq);
void hwts_sq_cq_set_cq(struct hwts_sq_cq *hwts_sqcq, struct npu_hwts_cq_info *hwts_cq_info);
struct schedule_sq_cq *hwts_sq_cq_get_cq(struct hwts_sq_cq *hwts_sqcq);
#endif

