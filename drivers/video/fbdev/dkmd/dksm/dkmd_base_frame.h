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

#ifndef DKMD_BASE_FRAME_H
#define DKMD_BASE_FRAME_H

#include <linux/types.h>

struct dkmd_base_layer;

struct dkmd_base_frame {
	int32_t scene_id;
	int32_t scene_mode;
	uint32_t scene_cmdlist_id;
	uint32_t layers_num;
	struct dkmd_base_layer *layers; // original layers with no segmentation
};

#endif