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

#ifndef DPU_DFC_H
#define DPU_DFC_H

#include "../../dpu_fb.h"
#include "../../dpu.h"
#include "../../dpu_fb_struct.h"
#include "../dpu_overlay_utils_struct.h"
#include "../../../include/dpu_communi_def.h"

void dpu_dfc_init(const char __iomem *dfc_base, dss_dfc_t *s_dfc);
void dpu_dfc_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *dfc_base, dss_dfc_t *s_dfc);
int dpu_rdfc_config(struct dpu_fb_data_type *dpufd,
	dss_layer_t *layer,
	dss_rect_t *aligned_rect,
	dss_rect_ltrb_t clip_rect,
	struct dpu_ov_compose_flag *ov_compose_flag);

#endif /* DPU_DFC_H */
