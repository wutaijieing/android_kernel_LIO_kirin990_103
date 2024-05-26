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

#ifndef DPU_WCH_WLT_H
#define DPU_WCH_WLT_H

void dpu_wch_wlt_set_reg(struct hisi_comp_operator *operator, char __iomem *wlt_base, struct dpu_wch_wlt *wlt);

int dpu_wch_wlt_config(struct post_offline_info *offline_info, struct hisi_composer_data *ov_data,
	struct disp_wb_layer *layer, struct disp_rect aligned_rect);

#endif /* DPU_WCH_WLT_H */
