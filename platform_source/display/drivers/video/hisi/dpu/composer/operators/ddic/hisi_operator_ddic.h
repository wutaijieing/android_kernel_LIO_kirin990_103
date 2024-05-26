/** @file
 * Copyright (c) 2021-2021, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#ifndef HISI_OPERATOR_DDIC_H
#define HISI_OPERATOR_DDIC_H

#include <linux/platform_device.h>
#include <linux/types.h>

#include "hisi_disp.h"
#include "hisi_composer_operator.h"

struct dpu_ddic {
	DPU_DDIC_DITHER_CTL1_UNION dither_ctr1;
	DPU_DDIC_DITHER_CTL0_UNION dither_ctr0;
};
struct dpu_ddic_demura {
	DPU_DDIC_DEMURA_EN_UNION demura_en;
	DPU_DDIC_DEMURA_IMG_SIZE_UNION demura_img_size;
	DPU_DDIC_DEMURA_NUM_OF_UNION demura_num_of;
	DPU_DDIC_DEMURA_LUT_SIZE_UNION demura_lut_size;
	DPU_DDIC_DEMURA_CTRL_UNION demura_ctrl;
	DPU_DDIC_DEMURA_PIX_GAIN_R_UNION demura_pix_gain_r;
	DPU_DDIC_DEMURA_PIX_GAIN_G_UNION demura_pix_gain_g;
	DPU_DDIC_DEMURA_PIX_GAIN_B_UNION demura_pix_gain_b;
	DPU_DDIC_DEMURA_PIX_PRECISION_R_UNION demura_pix_precision_r;
	DPU_DDIC_DEMURA_PIX_PRECISION_G_UNION demura_pix_precision_g;
	DPU_DDIC_DEMURA_PIX_PRECISION_B_UNION demura_pix_precision_b;
	DPU_DDIC_DEMURA_DBV_ROI_WIN1_STA_UNION dbv_roi_win1_sta;
};
struct hisi_ddic_out_data {
	struct hisi_pipeline_data base;

	/* other data */
};

struct hisi_ddic_in_data {
	struct hisi_pipeline_data base;

	/* other data */
};

struct hisi_operator_ddic {
	struct hisi_comp_operator base;

	/* other sdma data */
};

void hisi_ddic_init(struct hisi_operator_type *operators, struct dpu_module_ops ops, void *cookie);
#endif /* HISI_OPERATOR_DDIC_H */
