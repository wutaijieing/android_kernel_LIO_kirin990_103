/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#ifndef DPUFE_DTS_H
#define DPUFE_DTS_H

#include <linux/types.h>
#include "dpufe_panel_def.h"

uint32_t get_disp_chn_num(void);
uint32_t get_panel_num_for_one_disp(int disp_chn);
struct panel_base_info *get_panel_info_for_one_disp(int disp_chn);
int init_panel_info_by_dts(void);

#endif
