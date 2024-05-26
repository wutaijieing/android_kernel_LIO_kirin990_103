/* Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
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
#ifndef DPUFE_PAN_DISPLAY_H
#define DPUFE_PAN_DISPLAY_H

#include <linux/fb.h>
#include "dpufe.h"
#include "dpufe_panel_def.h"

void dpufe_fb_init_screeninfo_base(struct fb_fix_screeninfo *fix, struct fb_var_screeninfo *var);
void dpufe_fb_init_sreeninfo_by_img_type(struct fb_fix_screeninfo *fix, struct fb_var_screeninfo *var,
	uint32_t fb_img_type, int *bpp);
void dpufe_fb_init_sreeninfo_by_panel_info(struct fb_var_screeninfo *var, struct panel_base_info panel_info,
	uint32_t fb_num, int bpp);
uint32_t dpufe_line_length(int index, uint32_t xres, int bpp);
void dpufe_free_reserve_buffer(struct dpufe_data_type *dfd);

int dpufe_fb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info);
int dpufe_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info);
int dpufe_fb_set_par(struct fb_info *info);

#endif
