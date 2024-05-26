/*
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
 *
 */

#ifndef CORE_INTERFACE_H
#define CORE_INTERFACE_H

#include <linux/kernel.h>

struct dp_ctrl;

void dptx_ctrl_layer_init(struct dp_ctrl *dptx);
void edp_ctrl_layer_init(struct dp_ctrl *dptx);

/**
 * mst
 */
void dptx_initiate_mst_act(struct dp_ctrl *dptx);
void dptx_clear_vcpid_table(struct dp_ctrl *dptx);
void dptx_set_vcpid_table_slot(struct dp_ctrl *dptx, uint32_t slot, uint32_t stream);
void dptx_mst_enable(struct dp_ctrl *dptx);

#endif
