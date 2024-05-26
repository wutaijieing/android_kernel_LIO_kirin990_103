/* Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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

#ifndef DPU_DISPLAYENGINE_UTILS_H
#define DPU_DISPLAYENGINE_UTILS_H

#include "dpu_fb.h"
#include "dpu_displayengine_fingerprint_utils.h"

#define ALPHA_DEFAULT 0xFFF

enum alpha_support_type {
	ALPHA_NOT_SUPPORT = 0,
	DC_ALPHA_SUPPORT = 1,
	LOCAL_HBM_ALPHA_SUPPORT = 2
};

int display_engine_ddic_irc_set(struct dpu_fb_data_type *dpufd,
	display_engine_ddic_irc_param_t *param);
int display_engine_alpha_set_inner(struct dpu_fb_data_type *dpufd, uint32_t alpha,
	int force_alpha_enable);
int display_engine_alpha_set(struct dpu_fb_data_type *dpufd, uint32_t alpha);
struct dpu_panel_info *display_engine_get_panel_info(struct dpu_fb_data_type *dpufd, int panel_id);
u32 display_engine_alpha_get_support(void);
u32 display_engine_force_delta_bl_update_get_support(void);
void display_engine_effect_dbv_map(struct dpu_fb_data_type *dpufd, int *backlight_out);
int display_engine_local_hbm_mmie_set(struct dpu_fb_data_type *dpufd,
	struct display_engine_ddic_local_hbm_param *param);

#endif /* DPU_DISPLAYENGINE_UTILS_H */