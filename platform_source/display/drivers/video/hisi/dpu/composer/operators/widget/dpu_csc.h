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

#ifndef DPU_WIDGET_CSC_H
#define DPU_WIDGET_CSC_H

#include <linux/types.h>
#include <soc_dte_define.h>

#include "hisi_disp.h"
#include "hisi_dpu_module.h"

#define GET_CSC_HEAD_BASE(offset) ((offset) - DPU_ARSR_CSC_IDC0_ADDR(0))

#define DEFAULT_CSC_MODE          CSC_709_NARROW

enum CSC_DIRECTION {
	CSC_DIRECTION_NONE = 0,
	CSC_RGB2YUV,
	CSC_YUV2RGB,
	CSC_YUV2P3,
	CSC_SRGB2P3,
	CSC_DIRECTION_MAX,
};

/* csc module include csc and pcsc */
struct dpu_csc_data {
	char __iomem *base;

	/* in data */
	enum CSC_DIRECTION direction;
	enum CSC_MODE csc_mode;

	/* out data */
	enum DPU_FORMAT out_format;
};

void dpu_csc_set_cmd_item(struct dpu_module_desc *module, struct dpu_csc_data *input);
void dpu_post_csc_set_cmd_item(struct dpu_module_desc *module, struct dpu_csc_data *input);

enum CSC_DIRECTION dpu_csc_get_direction(uint32_t in_format, uint32_t out_format);
void dpu_csc_set_coef(struct dpu_module_desc *module, char __iomem *base,
	enum CSC_DIRECTION direction, enum CSC_MODE csc_mode);
void dpu_post_csc_set_coef(struct dpu_module_desc *module, char __iomem *base,
	enum CSC_DIRECTION direction, enum CSC_MODE csc_mode);
#endif /* DPU_WIDGET_CSC_H */
