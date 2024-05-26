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

#ifndef __DSC_CONFIG_H__
#define __DSC_CONFIG_H__

#include "dsc_output_calc.h"

#define DSC_V_1_2 2

void dsc_init(struct dsc_calc_info *dsc, char __iomem * dsc_base);
bool is_ifbc_vesa_panel(uint32_t ifbc_type);
bool is_dsc_enabled(struct dsc_calc_info *dsc);

#endif
