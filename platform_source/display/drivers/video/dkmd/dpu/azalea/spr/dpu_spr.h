/* Copyright (c) Hisilicon Technologies Co., Ltd. 2021-2021. All rights reserved.
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef DPU_SPR_H
#define DPU_SPR_H

#include "../dpu_fb_config.h"

#if defined(SUPPORT_SPR_2_0)
#include "dpu_spr_2_0.h"
#else
#include "dpu_spr_1_0.h"
#endif

typedef struct spr_dirty_region_update {
	int spr_overlap_type;
	uint32_t region_x;
	uint32_t region_y;
	uint32_t spr_img_size;
} spr_dirty_region;

#if defined(SUPPORT_SPR)
void spr_init(struct dpu_fb_data_type *dpufd, bool fastboot_enable);
void spr_dsc_partial_updt_config(struct dpu_fb_data_type *dpufd, spr_dirty_region *spr_dirty);
uint32_t get_hsize_after_spr_dsc(struct dpu_fb_data_type *dpufd, uint32_t rect_width);
void spr_get_real_dirty_region(struct dpu_fb_data_type *dpufd,
	struct dss_rect *dirty, dss_overlay_t *pov_req);
void spr_fill_dirty_region_info(struct dpu_fb_data_type *dpufd, struct dss_rect *dirty,
	dss_overlay_t *pov_req, spr_dirty_region *spr_dirty);

#else /* not support spr */
#define spr_init(dpufd, fastboot_enable)
#define spr_dsc_partial_updt_config(dpufd, spr_dirty)
#define get_hsize_after_spr_dsc(dpufd, out_width) (out_width)
#define spr_get_real_dirty_region(dpufd, dirty, pov_req)
#define spr_fill_dirty_region_info(dpufd, dirty, pov_req, spr_dirty)
#endif

#endif
