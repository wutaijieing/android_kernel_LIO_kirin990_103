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

#ifndef DPU_WCH_DITHER_H
#define DPU_WCH_DITHER_H

int dpu_wdfc_dither_init(const char __iomem *dither_base, struct dpu_wdfc_dither *s_dither);
int dpu_wdfc_dither_config(struct post_offline_info *offline_info, struct hisi_composer_data *ov_data,
	struct disp_wb_layer *layer, uint32_t in_format);
void dpu_wdfc_dither_set_reg(struct hisi_comp_operator *operator, char __iomem *wch_base, struct dpu_wdfc_dither *s_dither);
#endif /* HISI_WCH_DITHER_H */
