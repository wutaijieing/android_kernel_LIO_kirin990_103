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

#ifndef DPU_WCH_SCF_H
#define DPU_WCH_SCF_H

void dpu_wb_scf_init(const char __iomem *scl_base, struct dpu_scf *s_scl);

void dpu_wb_scf_set_reg(struct hisi_comp_operator *operator, char __iomem *scl_base, struct dpu_scf *s_scl);

int dpu_wb_scf_config(struct post_offline_info *off_info, struct hisi_composer_data *ov_data, struct disp_wb_layer *wb_layer);

#endif /* DPU_WCH_SCF_H */
