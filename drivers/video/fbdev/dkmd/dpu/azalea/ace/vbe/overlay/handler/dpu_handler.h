/* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#ifndef DPU_HANDLER_H
#define DPU_HANDLER_H

#include "../../dpu_fb.h"
#include "../../dpu.h"
#include "../../dpu_fb_struct.h"
#include "../dpu_overlay_utils_struct.h"
#include "../../../include/dpu_communi_def.h"

void dpu_unflow_handler(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req, bool unmask);

int dpu_ov_compose_handler(struct dpu_fb_data_type *dpufd,
	dss_overlay_block_t *pov_h_block,
	dss_layer_t *layer,
	struct dpu_ov_compose_rect *ov_compose_rect,
	struct dpu_ov_compose_flag *ov_compose_flag);

void dpu_vactive0_start_isr_handler(struct dpu_fb_data_type *dpufd);
int dpu_overlay_ioctl_handler(struct dpu_fb_data_type *dpufd,
	uint32_t cmd, void __user *argp);

#endif /* DPU_HANDLER_H */
