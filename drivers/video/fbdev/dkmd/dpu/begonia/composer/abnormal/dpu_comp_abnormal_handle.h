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

#ifndef COMP_ABNORMAL_HANDLE_H
#define COMP_ABNORMAL_HANDLE_H

#include "dkmd_isr.h"

enum {
	NO_ERROR_HAPPEND = 0,
	ERROR_ABNORMAL_HAPPEND = 1,
	ERROR_HAPPEND_HANDLE_CLEAR = 2,
	ERROR_HAPPEND_HANDLE_DSI_CLEAR = 3,
};

void dpu_comp_abnormal_handle_init(struct dkmd_isr *isr, struct dpu_composer *data, uint32_t listening_bit);
void dpu_comp_abnormal_handle_deinit(struct dkmd_isr *isr, uint32_t listening_bit);
void dpu_comp_abnormal_debug_dump(char __iomem *dpu_base, uint32_t scene_id);

#endif