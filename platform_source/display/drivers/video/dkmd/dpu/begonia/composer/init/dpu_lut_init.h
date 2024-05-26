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

#ifndef DPU_LUT_INIT_H
#define DPU_LUT_INIT_H

#define ARSR_LUT_H_OFFSET 64
#define CSC_COEF_OFFSET 0x80

struct dpu_composer;

void dpu_lut_init(struct dpu_composer *dpu_comp);

#endif