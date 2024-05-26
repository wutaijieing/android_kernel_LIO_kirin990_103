/*
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

#ifndef DPU_DISPLAYENGINE_EFFECT_H
#define DPU_DISPLAYENGINE_EFFECT_H

#include "dpu_fb.h"

struct effect_bl_buf {
	uint32_t blc_enable;
	int delta;
	struct dss_display_effect_xcc xcc_param;
	uint32_t dc_enable;
	uint32_t dimming_enable;
	int ddic_alpha;
	int panel_id;
};

extern uint8_t *g_share_mem_virt;
extern phys_addr_t g_share_mem_phy;

int display_engine_param_get(struct fb_info *info, void __user *argp);
void display_engine_separate_alpha_from_xcc(struct effect_bl_buf *resolved_buf);
uint32_t display_engine_get_dither_ctl0_value(void *pinfo, uint32_t dither_ctl0);

#endif /* DPU_DISPLAYENGINE_EFFECT_H */
