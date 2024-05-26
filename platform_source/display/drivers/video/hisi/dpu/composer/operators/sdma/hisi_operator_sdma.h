/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#ifndef HISI_OPERATOR_SDMA_H
#define HISI_OPERATOR_SDMA_H

#include <linux/types.h>

#include "hisi_drv_utils.h"
#include "hisi_disp.h"
#include "hisi_composer_operator.h"

static dss_ovl_alpha_t g_ovl_alpha[DSS_BLEND_MAX] = {
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* DSS_BLEND_CLEAR */
	{0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},  /* DSS_BLEND_SRC */
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* DSS_BLEND_DST */
	{3, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0},  /* DSS_BLEND_SRC_OVER_DST */
	{1, 0, 0, 0, 0, 0, 3, 0, 0, 1, 0},  /* DSS_BLEND_DST_OVER_SRC */
	{0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0},  /* DSS_BLEND_SRC_IN_DST */
	{3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* DSS_BLEND_DST_IN_SRC */
	{0, 0, 0, 0, 0, 0, 3, 0, 0, 1, 0},  /* DSS_BLEND_SRC_OUT_DST */
	{3, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},  /* DSS_BLEND_DST_OUT_SRC */
	{3, 0, 0, 0, 1, 0, 3, 0, 0, 0, 0},  /* DSS_BLEND_SRC_ATOP_DST */
	{3, 0, 0, 0, 0, 0, 3, 0, 0, 1, 0},  /* DSS_BLEND_DST_ATOP_SRC */
	{3, 0, 0, 0, 1, 0, 3, 0, 0, 1, 0},  /* DSS_BLEND_SRC_XOR_DST */
	{1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},  /* DSS_BLEND_SRC_ADD_DST */
	{3, 0, 0, 0, 1, 0, 3, 0, 0, 0, 1},  /* DSS_BLEND_FIX_OVER */
	{3, 0, 0, 0, 1, 0, 3, 2, 0, 0, 0},  /* DSS_BLEND_FIX_PER0 */
	{3, 0, 0, 0, 1, 1, 3, 1, 0, 0, 1},  /* DSS_BLEND_FIX_PER1 */
	{2, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0},  /* DSS_BLEND_FIX_PER2 */
	{1, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0},  /* DSS_BLEND_FIX_PER3 */
	{0, 0, 0, 0, 0, 0, 3, 2, 0, 0, 0},  /* DSS_BLEND_FIX_PER4 */
	{3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* DSS_BLEND_FIX_PER5 */
	{0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0},  /* DSS_BLEND_FIX_PER6 */
	{2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* DSS_BLEND_FIX_PER7 */
	{2, 2, 0, 0, 0, 0, 3, 2, 0, 0, 0},  /* DSS_BLEND_FIX_PER8 */
	{3, 2, 0, 0, 0, 0, 2, 2, 0, 0, 0},  /* DSS_BLEND_FIX_PER9 */
	{2, 2, 0, 0, 0, 0, 2, 2, 0, 0, 0},  /* DSS_BLEND_FIX_PER10 */
	{3, 2, 0, 0, 0, 0, 3, 2, 0, 0, 0},  /* DSS_BLEND_FIX_PER11 */
	{2, 1, 0, 0, 0, 0, 3, 2, 0, 0, 0},  /* DSS_BLEND_FIX_PER12 DSS_BLEND_SRC_OVER_DST */
	{2, 1, 0, 0, 0, 0, 3, 1, 0, 0, 1},  /* DSS_BLEND_FIX_PER13 DSS_BLEND_FIX_OVER */
	{0, 0, 0, 0, 0, 1, 3, 0, 1, 0, 0},  /* DSS_BLEND_FIX_PER14 BASE_FF */
	{2, 1, 0, 0, 0, 1, 3, 2, 0, 0, 0},  /* DSS_BLEND_FIX_PER15 DSS_BLEND_SRC_OVER_DST */
	{2, 1, 0, 0, 0, 1, 3, 1, 0, 0, 1},  /* DSS_BLEND_FIX_PER16 DSS_BLEND_FIX_OVER */
	{0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},  /* DSS_BLEND_FIX_PER17 DSS_BLEND_SRC */
};

struct hisi_sdma_out_data {
	struct hisi_pipeline_data base;

	/* other data */
};

struct hisi_sdma_in_data {
	struct hisi_pipeline_data base;

	/* other data */
};

struct hisi_operator_sdma {
	struct hisi_comp_operator base;

	/* other sdma data */
};

static inline int get_ov_endx(const struct disp_rect *rect, uint32_t panel_width)
{
	return rect->x + rect->w > panel_width ? panel_width - 1 : rect->x + rect->w - 1;
}

static inline int get_ov_endy(const struct disp_rect *rect, uint32_t panel_height)
{
	return rect->y + rect->h > panel_height ? panel_height - 1 : rect->y + rect->h - 1;
}

void hisi_sdma_init(struct hisi_operator_type *operators, struct dpu_module_ops ops, void *cookie);
void hisi_set_dm_addr_reg(struct hisi_comp_operator *operator, char __iomem *dpu_base_addr);
uint32_t get_ovl_blending_mode(struct pipeline_src_layer *layer);
void set_ov_mode(struct hisi_dm_layer_info *layer_info, uint32_t blending_mode);
void set_layer_mirr(struct hisi_dm_layer_info *layer_info, struct pipeline_src_layer *layer);
#endif /* HISI_OPERATOR_SDMA_H */
