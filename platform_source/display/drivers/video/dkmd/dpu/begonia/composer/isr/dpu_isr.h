/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2021. All rights reserved.
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

#ifndef DPU_DPP_H
#define DPU_DPP_H

#include "dkmd_isr.h"

void dpu_comp_alsc_handle_init(struct dkmd_isr *isr_ctrl, struct dpu_composer *dpu_comp, uint32_t listening_bit);
void dpu_comp_alsc_handle_deinit(struct dkmd_isr *isr_ctrl, uint32_t listening_bit);

void dpu_comp_hiace_handle_init(struct dkmd_isr *isr_ctrl, struct dpu_composer *dpu_comp, uint32_t listening_bit);
void dpu_comp_hiace_handle_deinit(struct dkmd_isr *isr_ctrl, uint32_t listening_bit);

void dpu_comp_m1_qic_handle_init(struct dkmd_isr *isr_ctrl, struct dpu_composer *dpu_comp, uint32_t listening_bit);
void dpu_comp_m1_qic_handle_deinit(struct dkmd_isr *isr_ctrl, uint32_t listening_bit);
#endif