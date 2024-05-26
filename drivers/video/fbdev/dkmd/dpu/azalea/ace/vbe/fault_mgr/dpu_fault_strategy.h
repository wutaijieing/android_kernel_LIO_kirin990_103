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

#ifndef DPU_FAULT_STRATEGY_H
#define DPU_FAULT_STRATEGY_H

#include "../../include/dpu_communi_def.h"

#define FAULT_NAME_MAX_LEN 256
enum FAULT_ID {
	GET_IOVA_BY_DMA_BUF_FAIL = 0,
	CHECK_IOVA_VALID_FAIL,
	FAULT_MAX,
};

struct fault_strategy {
	char fault_name[FAULT_NAME_MAX_LEN];
	int (*deal_func)(void);
};

struct fault_strategy *get_fault_strategy(enum DISP_SOURCE disp_source, enum FAULT_ID fault_id);

#endif /* DPU_FAULT_STRATEGY_H */
