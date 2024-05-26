/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#ifndef CMD_MANAGER_IMPL_H
#define CMD_MANAGER_IMPL_H

#include <linux/types.h>
#include "dkmd_opr_id.h"

#define POST_PIPELINE_MAX_NUM 4

struct dkmd_base_frame;
struct dkmd_network;
struct dkmd_pipeline;

int32_t request_scene_client(struct dkmd_base_frame *frame);
int32_t request_dm_client(const struct dkmd_base_frame *frame, struct dkmd_network *network);
int32_t request_reg_client(const struct dkmd_base_frame *frame, struct dkmd_network *network);
int32_t append_reg_client(const struct dkmd_base_frame *frame, const struct dkmd_network *network);
void flush_all_cmdlist_client(const struct dkmd_base_frame *frame, const struct dkmd_network *network);

int32_t gen_pre_pipeline_cmd(const struct dkmd_base_frame *frame, const struct dkmd_network *network,
	const struct dkmd_pipeline *pre_pipeline, const struct dkmd_pipeline *post_pipelines, uint32_t post_pipelines_num);
int32_t gen_post_pipeline_cmd(const struct dkmd_base_frame *frame, const struct dkmd_network *network,
	union dkmd_opr_id pre_pipeline_last_opr_id, const struct dkmd_pipeline *post_pipeline);

#endif