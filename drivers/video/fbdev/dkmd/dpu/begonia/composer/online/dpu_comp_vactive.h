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

#ifndef COMP_VACTIVE_H
#define COMP_VACTIVE_H

#include "dkmd_isr.h"
#include "dpu_comp_online.h"

int32_t dpu_comp_vactive_wait_event(struct comp_online_present *present);
void dpu_comp_vactive_init(struct dkmd_isr *isr, struct dpu_composer *data, uint32_t listening_bit);
void dpu_comp_vactive_deinit(struct dkmd_isr *isr, uint32_t listening_bit);

#endif