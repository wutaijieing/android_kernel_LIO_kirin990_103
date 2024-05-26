/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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

#ifndef DKMD_DPU_DVFS_H
#define DKMD_DPU_DVFS_H

#include <linux/types.h>
#include <linux/semaphore.h>

struct dpu_dvfs {
	struct semaphore sem;
	uint32_t voted_level; // record last perf level
	uint32_t delay_cnt;   // count value when perf level is descending
	bool is_perf_falling; // whether perf level is descending
	bool reserved[3];     // for 4-byte align
};

void dpu_res_register_dvfs(struct list_head *res_head);

int dpu_dvfs_intra_frame_vote(struct intra_frame_dvfs_info *info);
void dpu_dvfs_reset_dvfs_info(void);

#endif
