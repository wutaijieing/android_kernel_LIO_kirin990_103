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

#ifndef DPU_OPERATOR_DFC_H
#define DPU_OPERATOR_DFC_H

#include <linux/platform_device.h>
#include <linux/types.h>

#include "hisi_disp.h"
#include "dpu_offline_define.h"

void dpu_wdfc_init(const char __iomem *dfc_base, struct dpu_wdfc *s_dfc);

int dpu_wdfc_config(struct post_offline_info *offline_info, struct hisi_composer_data *ov_data,
	struct disp_wb_layer *layer, struct disp_rect *aligned_rect);

void dpu_wdfc_set_reg(struct hisi_comp_operator *operator, char __iomem *dfc_base, struct dpu_wdfc *s_dfc);

#endif /* DPU_OPERATOR_DFC_H */
