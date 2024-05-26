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

#ifndef DPU_WIDGET_DFC_H
#define DPU_WIDGET_DFC_H

#include <linux/types.h>
#include <soc_dte_define.h>

#include "hisi_disp.h"
#include "hisi_composer_operator.h"
#include "hisi_dpu_module.h"
#include "hisi_disp_debug.h"

struct dpu_dfc_data {
	char __iomem *dfc_base;
	uint32_t format;
	bool enable_dither;
	bool enable_icg_submodule;
	uint16_t glb_alpha;

	struct disp_rect src_rect;
	struct disp_rect dst_rect;

	struct dpu_edit_rect clip_rect;
	struct dpu_edit_rect padding_rect;
};

#define disp_pr_dfc_input(input) do { \
		disp_pr_info("dfc_base = %p,format = %u, enable_dither = %d,"\
					 "enable_icg_submodule = %d,glb_alpha = %u, ", \
					input->dfc_base, input->format, input->enable_dither, \
					input->enable_icg_submodule, input->glb_alpha); \
		disp_pr_rect(info, &input->src_rect); \
		disp_pr_rect(info, &input->dst_rect); \
		disp_pr_edit_rect(input->clip_rect);  \
		disp_pr_edit_rect(input->padding_rect); \
	} while (0)

int dpu_dfc_rb_swap(int format);
int dpu_dfc_uv_swap(int format);
int dpu_dfc_ax_swap(int format);
int dpu_pixel_format_dpu2dfc(int format, int dfc_type);
void dpu_dfc_set_cmd_item(struct dpu_module_desc *module, struct dpu_dfc_data *input);


#endif /* DPU_WIDGET_DFC_H */
