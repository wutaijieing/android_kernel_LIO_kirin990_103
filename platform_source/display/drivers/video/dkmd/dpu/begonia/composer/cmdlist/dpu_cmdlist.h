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

#ifndef DPU_CMDLIST_H
#define DPU_CMDLIST_H

#include <linux/types.h>

struct dpu_cmdlist_frame_info {
	uint32_t scene_id;
	uint32_t cmdlist_id;
	uint32_t frame_index;
	uint32_t resevered;
};

struct dkmd_timeline;
struct disp_frame;

void dpu_cmdlist_init_commit(char __iomem *dpu_base, dma_addr_t cmdlist_buf_addr);
void dpu_cmdlist_present_commit(char __iomem *dpu_base, uint32_t scene_id, uint32_t cmdlist_id);
int32_t dpu_cmdlist_sync_lock(struct dkmd_timeline *timeline, struct disp_frame *frame);

#endif