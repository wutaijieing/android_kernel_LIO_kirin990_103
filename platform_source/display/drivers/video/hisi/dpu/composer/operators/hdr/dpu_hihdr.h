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

#ifndef __DPU_HIHDR_H__
#define __DPU_HIHDR_H__

#include "hisi_fb.h"
#include "dpu_hdr_def.h"
#include "hisi_dpu_module.h"

void dpu_hdr_reg_init(struct dpu_module_desc *module_desc, char __iomem *hdr_base);

void dpu_hihdr_gtm_set_reg(struct hisi_comp_operator *operator, struct dpu_module_desc *module_desc,
	struct pipeline_src_layer *src_layer,
	char __iomem *hdr_base,
	struct dpu_hdr_info *hdr_info);

void dpu_hdr_degamma_set_lut(struct dpu_module_desc *module_desc, struct pipeline_src_layer *src_layer,
	char __iomem *hdr_base, struct dpu_hdr_info *hdr_info);

void dpu_hdr_gmp_set_lut(struct dpu_module_desc *module_desc, struct pipeline_src_layer *src_layer,
	char __iomem *hdr_base, struct dpu_hdr_info *hdr_info);

void dpu_hdr_gamma_set_lut(struct dpu_module_desc *module_desc, struct pipeline_src_layer *src_layer,
	char __iomem *hdr_base, struct dpu_hdr_info *hdr_info);
void hihdr_gtm_param_copyto_ghdrinfo(struct dpu_hdr_info *hdr_info,
	struct dpu_hdr_info *hihdr_param);
#endif

