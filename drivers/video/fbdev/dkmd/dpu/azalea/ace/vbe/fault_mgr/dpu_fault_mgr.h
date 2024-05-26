/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef DPU_FAULT_MGR_H
#define DPU_FAULT_MGR_H

#include "dpu_fault_strategy.h"
#include "../../include/dpu_communi_def.h"

int dpu_submit_disp_fault(enum DISP_SOURCE disp_source, enum FAULT_ID fault_id);

#endif
