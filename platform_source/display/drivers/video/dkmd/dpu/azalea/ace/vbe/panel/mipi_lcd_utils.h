/* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 */
#ifndef MIPI_LCD_UTILS_H
#define MIPI_LCD_UTILS_H

#include "../dpu_fb_panel_struct.h"
#include "../dpu_fb_struct.h"

#define LG_2LANE_NT36870     0x1
#define JDI_2LANE_NT36860C   0x2
#define SHARP_2LANE_NT36870  0x4
#define HX_4LANE_NT36682C  0x8
#define TCL_4LANE_NT36682C 0x10
#define MIPI_LCD_PROJECT_ID_LEN 10

extern uint32_t g_mipi_lcd_name;
int switch_panel_mode(struct dpu_fb_data_type *dpufd, uint8_t mode_switch_to);
#endif

