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

#include "../dpu_fb_def.h"
#include "../include/dpu_communi_def.h"
#include "../merger_mgr/dpu_disp_merger_mgr.h"
#include "../dpu_fb_struct.h"
#include "../dpu_fb.h"
#include "dpu_fault_strategy.h"

static struct fault_strategy fault_deal[MAX_SRC_NUM][FAULT_MAX];
static bool is_inited;

static int close_rda_display(void)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct disp_merger_ctl merger_ctl = {0};

	merger_ctl.disp_source = RDA_DISP;
	merger_ctl.enable = DISP_CLOSE;

	dpufd = dpufb_get_dpufd(EXTERNAL_PANEL_IDX);
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is nullptr!\n");

	if (dpufd->merger_mgr.ops && dpufd->merger_mgr.ops->ctl_enable)
		dpufd->merger_mgr.ops->ctl_enable(&dpufd->merger_mgr, &merger_ctl);

	return 0;
}

static void dpu_init_fault_strategy(void)
{
	// RDA
	fault_deal[RDA_DISP][GET_IOVA_BY_DMA_BUF_FAIL].deal_func = close_rda_display;
	fault_deal[RDA_DISP][CHECK_IOVA_VALID_FAIL].deal_func = close_rda_display;

	is_inited = true;
}

struct fault_strategy* get_fault_strategy(enum DISP_SOURCE disp_source, enum FAULT_ID fault_id)
{
	if ((disp_source >= MAX_SRC_NUM) || (fault_id >= FAULT_MAX)) {
		DPU_FB_ERR("wrong input vars (disp_source:%u fault_id:%u)\n", disp_source, fault_id);
		return NULL;
	}

	if (is_inited == false)
		dpu_init_fault_strategy();

	return &fault_deal[disp_source][fault_id];
}
