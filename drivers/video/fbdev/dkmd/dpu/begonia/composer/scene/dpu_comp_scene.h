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

#ifndef COMPOSER_SCENE_H
#define COMPOSER_SCENE_H

#include <dkmd_dpu.h>
#include "buf_sync/dkmd_buf_sync.h"
#include "peri/dkmd_peri.h"

struct dpu_comp_frame {
	struct dkmd_dma_buf layer_dma_buf[DISP_LAYER_MAX_COUNT];
	struct disp_frame in_frame;
};

struct composer_scene {
	uint32_t scene_id;
	uint32_t frame_index;
	char __iomem *dpu_base;

	int32_t (*present)(struct composer_scene *scene, uint32_t cmdlist_id);
};

int32_t dpu_comp_scene_device_setup(struct composer_scene *scene);
void dpu_comp_scene_switch(struct dkmd_connector_info *pinfo, struct composer_scene *scene);

#endif
