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

#include "dpu_opr_config.h"

uint32_t g_cld_offset[OPR_CLD_NUM] = {
	DPU_CLD0_OFFSET,
	DPU_CLD1_OFFSET,
};

uint32_t g_arsr_offset[OPR_ARSR_NUM] = {
	DPU_ARSR0_OFFSET,
	DPU_ARSR1_OFFSET,
};

uint32_t g_itfsw_offset[OPR_ITFSW_NUM] = {
	DPU_ITF_CH0_OFFSET,
	DPU_ITF_CH1_OFFSET,
	DPU_ITF_CH2_OFFSET,
	DPU_ITF_CH3_OFFSET,
};