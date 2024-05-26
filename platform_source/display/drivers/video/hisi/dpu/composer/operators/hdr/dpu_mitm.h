 /* Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#ifndef DPU_MITM_H
#define DPU_MITM_H

#include "hisi_dpu_module.h"
#include "dpu_hdr_def.h"
#include "hisi_disp.h"
#include "dm/hisi_dm.h"

void dpu_set_dm_mitem_reg(struct dpu_module_desc *module_desc,
	char __iomem *dpu_base, struct wcg_mitm_info *mitm_param);
void dpu_ov_mitm_sel(struct pipeline_src_layer *layer, struct hisi_dm_layer_info *layer_info);
void dpu_mitm_param_set(struct dpu_wcg_info* wcg_info, struct wcg_mitm_info* mitm_param);
#endif

