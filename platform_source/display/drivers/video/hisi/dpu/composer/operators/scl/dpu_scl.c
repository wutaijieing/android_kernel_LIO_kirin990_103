/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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


#include <linux/types.h>

#include "hisi_drv_utils.h"
#include "hisi_disp.h"
#include "hisi_composer_operator.h"

static uint8_t g_scl_index;
static uint8_t g_arsr1_arsr0_en;
/* TODO: redefine scl dm offset */
static uint32_t g_scl_dm_offset[] = {
	/* scl0 */
	0,

	/* scl1 */
	DPU_DM_SCL1_INPUT_IMG_WIDTH_ADDR(0) - DPU_DM_SCL0_INPUT_IMG_WIDTH_ADDR(0),

	/* scl2 */
	DPU_DM_SCL2_INPUT_IMG_WIDTH_ADDR(0) - DPU_DM_SCL0_INPUT_IMG_WIDTH_ADDR(0),

	/* scl3 */
	DPU_DM_SCL3_INPUT_IMG_WIDTH_ADDR(0) - DPU_DM_SCL0_INPUT_IMG_WIDTH_ADDR(0),

	/* scl4 */
	DPU_DM_SCL4_INPUT_IMG_WIDTH_ADDR(0) - DPU_DM_SCL0_INPUT_IMG_WIDTH_ADDR(0),
};

uint32_t dpu_scl_alloc_dm_node(void)
{
	if (g_scl_index >= ARRAY_SIZE(g_scl_dm_offset)) {
		disp_pr_err("g_scl_index %u is overflow", g_scl_index);
		return 0;
	}
	return g_scl_dm_offset[g_scl_index++];
}

void dpu_arsr_cnt(void)
{
	g_arsr1_arsr0_en++;
}

bool is_arsr1_arsr0_pipe(void)
{
	return g_arsr1_arsr0_en == 2; // sdma->arsr1->ov->arsr0
}

void dpu_scl_free_dm_node(void)
{
	g_scl_index = 0;
	g_arsr1_arsr0_en = 0;
}

uint8_t dpu_get_scl_idx(void)
{
	return g_scl_index;
}

