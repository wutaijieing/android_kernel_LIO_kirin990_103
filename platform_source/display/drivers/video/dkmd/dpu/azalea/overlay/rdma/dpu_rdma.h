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

#ifndef DPU_RDMA_H
#define DPU_RDMA_H

#include "../../dpu_fb.h"

void dpu_rdma_init(const char __iomem *dma_base, dss_rdma_t *s_dma);
void dpu_rdma_u_init(const char __iomem *dma_base, dss_rdma_t *s_dma);
void dpu_rdma_v_init(const char __iomem *dma_base, dss_rdma_t *s_dma);
int dpu_get_rdma_tile_interleave(uint32_t stride);
void dpu_chn_set_reg_default_value(struct dpu_fb_data_type *dpufd, char __iomem *dma_base);
void dpu_rdma_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *dma_base, dss_rdma_t *s_dma);
void dpu_rdma_u_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *dma_base, dss_rdma_t *s_dma);
void dpu_rdma_v_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *dma_base, dss_rdma_t *s_dma);
int dpu_rdma_config(struct dpu_fb_data_type *dpufd, dss_layer_t *layer,
	struct dpu_ov_compose_rect *ov_compose_rect,
	struct dpu_ov_compose_flag *ov_compose_flag);
uint32_t get_rdma_stretch_line_num(dss_layer_t *layer);


#endif /* DPU_RDMA_H */
