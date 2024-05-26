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

#ifndef DPU_MIF_H
#define DPU_MIF_H

#include "../../dpu.h"
#include "../../dpu_fb.h"

void dpu_mif_init(char __iomem *mif_ch_base, dss_mif_t *s_mif, int chn_idx);
void dpu_mif_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *mif_ch_base, dss_mif_t *s_mif, int chn_idx);
void dpu_mif_on(struct dpu_fb_data_type *dpufd);
int dpu_mif_config(struct dpu_fb_data_type *dpufd,
	dss_layer_t *layer, dss_wb_layer_t *wb_layer, bool rdma_stretch_enable);


#endif /* DPU_MIF_H */
