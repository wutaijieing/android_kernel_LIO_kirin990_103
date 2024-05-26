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

#ifndef __DPU_DP_DBG_H__
#define __DPU_DP_DBG_H__

#include <linux/kernel.h>
#include <linux/types.h>
#include <huawei_platform/dp_source/dp_source_switch.h>
#include <huawei_platform/dp_source/dp_debug.h>

/*******************************************************************************
 * DP GRAPHIC DEBUG TOOL
 */
struct dp_ctrl;
void dp_graphic_debug_node_init(struct dp_ctrl *dptx);
void dptx_debug_set_params(struct dp_ctrl *dptx);
extern int g_dp_debug_mode_enable;
extern uint g_dp_aux_ctrl;
extern uint g_pll1ckgctrlr0;
extern uint g_pll1ckgctrlr1;
extern uint g_pll1ckgctrlr2;
extern uint32_t g_hbr2_pll1_sscg[4];
extern int g_dp_same_source_debug;

#endif /* __DPU_DP_DBG_H__ */
