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

#ifndef DPU_CLD_H
#define DPU_CLD_H

#include "../dpu_overlay_utils.h"

void dpu_cld_init(const char __iomem *cld_base, struct dss_cld *s_cld);
void dpu_cld_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *cld_base, struct dss_cld *s_cld, int channel);
int dpu_cld_config(struct dpu_fb_data_type *dpufd, dss_layer_t *layer);

#endif /* DPU_CLD_H */

