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

#ifndef DKMD_NETWORK_H
#define DKMD_NETWORK_H

#include <linux/types.h>

struct dkmd_base_layer;
union dkmd_opr_id;

struct dkmd_pipeline {
	struct dkmd_base_layer *block_layer; // a layer or segmented layer if segmentation is enabled
	uint32_t opr_num;
	union dkmd_opr_id *opr_ids;
};

struct dkmd_network {
	uint32_t pre_pipeline_num;
	struct dkmd_pipeline *pre_pipelines;
	uint32_t post_pipeline_num;
	struct dkmd_pipeline *post_pipelines;
	uint32_t pre_dm_cmdlist_id;
	uint32_t dm_cmdlist_id;
	uint32_t reg_cmdlist_id;

	/* First/last block indicates the first/last block of this frame segmentation
	 * Frame's network (default with no segmentation) is not only first block but last block
	 */
	bool is_first_block;
	bool is_last_block;
};

#endif