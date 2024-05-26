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

#ifndef DPU_DACC_H
#define DPU_DACC_H

#include <linux/types.h>

void dpu_dacc_load(void);
void dpu_dacc_config_scene(char __iomem *dpu_base, uint32_t scene_id, bool enable_cmdlist);
uint32_t dpu_dacc_handle_clear(char __iomem *dpu_base, uint32_t scene_id);

#endif