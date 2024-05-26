/*
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

#ifndef __DP_MST_TOPOLOGY_H__
#define __DP_MST_TOPOLOGY_H__

#include <linux/kernel.h>

struct dp_ctrl;

#define MST_DOWN_REP_MAX_LENGTH 1024
struct mst_msg_info {
	uint8_t msg[MST_DOWN_REP_MAX_LENGTH];
	uint8_t msg_len;
};

int dptx_mst_payload_setting(struct dp_ctrl *dptx);
int dptx_mst_initial(struct dp_ctrl *dptx);
int dptx_get_topology(struct dp_ctrl *dptx);
int dptx_sideband_get_up_req(struct dp_ctrl *dptx);

#ifndef CONFIG_DRM_KMS_HELPER
int drm_dp_calc_pbn_mode(int clock, int bpp);
#endif

#endif /* DP_MST_TOPOLOGY_H */
