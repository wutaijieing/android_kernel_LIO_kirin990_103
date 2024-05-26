/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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

#ifndef __HIDPTX_DP_CORE_H__
#define __HIDPTX_DP_CORE_H__

#include <linux/kernel.h>

struct dp_ctrl;

/**
 * Core interface functions
 */
void dptx_global_intr_en(struct dp_ctrl *dp);
void dptx_global_intr_dis(struct dp_ctrl *dp);
void dptx_global_intr_clear(struct dp_ctrl *dp);
int dptx_core_deinit(struct dp_ctrl *dptx);
void dptx_typec_reset_ack(struct dp_ctrl *dptx);
void dptx_soft_reset(struct dp_ctrl *dptx, uint32_t bits);
void dptx_soft_reset_all(struct dp_ctrl *dptx);

/**
 * PHY IF Control
 */
void dptx_phy_set_lanes(struct dp_ctrl *dptx, uint32_t num);
void dptx_phy_set_rate(struct dp_ctrl *dptx, uint32_t rate);
void dptx_phy_data_lane_clear(struct dp_ctrl *dptx);
void dptx_phy_lane_reset(struct dp_ctrl *dptx, bool bopen);
int dwc_phy_get_rate(struct dp_ctrl *dptx);
int dptx_phy_wait_busy(struct dp_ctrl *dptx, uint32_t lanes);
void dptx_link_set_preemp_vswing(struct dp_ctrl *dptx);
void dptx_phy_set_lanes_power(struct dp_ctrl *dptx, bool bopen);
void dptx_init_hwparams(struct dp_ctrl *dptx);

/**
 * DPRX with DPCD Data Structure r1.1 and higher shall set enhanced framing mode enabled
 * Applies only to Single-Stream Transport (SST) mode
 */
int dptx_set_enhanced_framing_mode(struct dp_ctrl *dptx);
#endif /* HIDPTX_DP_CORE_H */
