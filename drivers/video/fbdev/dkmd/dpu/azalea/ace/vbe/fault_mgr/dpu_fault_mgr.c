;/*
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
#include "../dpu_fb_def.h"
#include "dpu_fault_strategy.h"
#include "../../include/dpu_communi_def.h"

/* Attention : at present only instrument display supports fault management */
int dpu_submit_disp_fault(enum DISP_SOURCE disp_source, enum FAULT_ID fault_id)
{
	int ret;
	struct fault_strategy *fault_st = NULL;

	if ((fault_id >= FAULT_MAX) || (disp_source >= MAX_SRC_NUM)) {
		DPU_FB_ERR("wrong input (disp_source:%u fault_id:%u)\n", disp_source, fault_id);
		return -1;
	}

	fault_st = get_fault_strategy(disp_source, fault_id);
	if ((fault_st == NULL) || (fault_st->deal_func == NULL)) {
		DPU_FB_ERR("get_fault_strategy failed (disp_source:%u fault_id:%u)\n", disp_source, fault_id);
		return -1;
	}

	ret = fault_st->deal_func();
	if (ret != 0) {
		DPU_FB_ERR("deal fault failed (disp_source:%u fault_id:%u)\n", disp_source, fault_id);
		return -1;
	}

	DPU_FB_INFO("deal fault success (disp_source:%u fault_id:%u)\n", disp_source, fault_id);
	return 0;
}
