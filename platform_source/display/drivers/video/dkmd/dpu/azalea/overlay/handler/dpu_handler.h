/* Copyright (c) 2013-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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

void dpu_unflow_handler(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req, bool unmask);

int dpu_ov_compose_handler(struct dpu_fb_data_type *dpufd,
	dss_overlay_block_t *pov_h_block,
	dss_layer_t *layer,
	struct dpu_ov_compose_rect *ov_compose_rect,
	struct dpu_ov_compose_flag *ov_compose_flag);

int dpu_wb_compose_handler(struct dpu_fb_data_type *dpufd,
	dss_wb_layer_t *wb_layer,
	dss_rect_t *wb_ov_block_rect,
	bool last_block,
	struct dpu_ov_compose_flag ov_compose_flag);

int dpu_wb_ch_module_set_regs(struct dpu_fb_data_type *dpufd, uint32_t wb_type,
	dss_wb_layer_t *wb_layer, bool enable_cmdlist);


void dpu_vactive0_start_isr_handler(struct dpu_fb_data_type *dpufd);
void dpu_vactive0_end_isr_handler(struct dpu_fb_data_type *dpufd);
int dpu_overlay_ioctl_handler(struct dpu_fb_data_type *dpufd,
	uint32_t cmd, void __user *argp);

#endif /* DPU_HANDLER_H */
