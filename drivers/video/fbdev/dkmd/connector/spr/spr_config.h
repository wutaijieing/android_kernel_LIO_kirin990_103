/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __SPR_H__
#define __SPR_H__

#include "dpu/soc_dpu_define.h"
#include "dkmd_connector.h"

enum SPR_GAMMA_TYPE {
	SPR_GAMMA_R = 0,
	SPR_GAMMA_G,
	SPR_GAMMA_B,
	SPR_DEGAMMA_R,
	SPR_DEGAMMA_G,
	SPR_DEGAMMA_B,
	SPR_GAMMA_LUT_ROW
};

#define SPR_GAMMA_LUT_COLUMN 258
#define SPR_GAMMA_LUT_SIZE (SPR_GAMMA_LUT_ROW * SPR_GAMMA_LUT_COLUMN)
#define COEF_PER_REG 2
#define DSC_OUTPUT_MODE 24
#define SPR_CLOSE_BOARDER_GAIN 0x800080

#define SPR_ENABLE 1
#define TXIP_ENABLE 1
#define DATAPACK_ENABLE 1
#define DATAPACK_UNPACK 0
#define GMP_BITEXT_COPY_HIGH_TO_LOW 1

struct spr_info {
	DPU_DPP_SPR_SIZE_UNION spr_size;
	DPU_DPP_SPR_CTRL_UNION spr_ctrl;

	DPU_DPP_SPR_PIX_EVEN_COEF_SEL_UNION spr_pix_even;
	DPU_DPP_SPR_PIX_ODD_COEF_SEL_UNION spr_pix_odd;
	DPU_DPP_SPR_PIX_PANEL_ARRANGE_SEL_UNION spr_pix_panel;

	DPU_DPP_SPR_COEFF_V0H0_R0_UNION spr_coeff_v0h0_r0;
	DPU_DPP_SPR_COEFF_V0H0_R1_UNION spr_coeff_v0h0_r1;
	DPU_DPP_SPR_COEFF_V0H1_R0_UNION spr_coeff_v0h1_r0;
	DPU_DPP_SPR_COEFF_V0H1_R1_UNION spr_coeff_v0h1_r1;
	DPU_DPP_SPR_COEFF_V1H0_R0_UNION spr_coeff_v1h0_r0;
	DPU_DPP_SPR_COEFF_V1H0_R1_UNION spr_coeff_v1h0_r1;
	DPU_DPP_SPR_COEFF_V1H1_R0_UNION spr_coeff_v1h1_r0;
	DPU_DPP_SPR_COEFF_V1H1_R1_UNION spr_coeff_v1h1_r1;

	DPU_DPP_SPR_COEFF_V0H0_G0_UNION spr_coeff_v0h0_g0;
	DPU_DPP_SPR_COEFF_V0H0_G1_UNION spr_coeff_v0h0_g1;
	DPU_DPP_SPR_COEFF_V0H1_G0_UNION spr_coeff_v0h1_g0;
	DPU_DPP_SPR_COEFF_V0H1_G1_UNION spr_coeff_v0h1_g1;
	DPU_DPP_SPR_COEFF_V1H0_G0_UNION spr_coeff_v1h0_g0;
	DPU_DPP_SPR_COEFF_V1H0_G1_UNION spr_coeff_v1h0_g1;
	DPU_DPP_SPR_COEFF_V1H1_G0_UNION spr_coeff_v1h1_g0;
	DPU_DPP_SPR_COEFF_V1H1_G1_UNION spr_coeff_v1h1_g1;

	DPU_DPP_SPR_COEFF_V0H0_B1_UNION spr_coeff_v0h0_b1;
	DPU_DPP_SPR_COEFF_V0H0_B0_UNION spr_coeff_v0h0_b0;
	DPU_DPP_SPR_COEFF_V0H1_B0_UNION spr_coeff_v0h1_b0;
	DPU_DPP_SPR_COEFF_V0H1_B1_UNION spr_coeff_v0h1_b1;
	DPU_DPP_SPR_COEFF_V1H0_B0_UNION spr_coeff_v1h0_b0;
	DPU_DPP_SPR_COEFF_V1H0_B1_UNION spr_coeff_v1h0_b1;
	DPU_DPP_SPR_COEFF_V1H1_B0_UNION spr_coeff_v1h1_b0;
	DPU_DPP_SPR_COEFF_V1H1_B1_UNION spr_coeff_v1h1_b1;

	DPU_DPP_SPR_LAREA_START_UNION spr_larea_start;
	DPU_DPP_SPR_LAREA_END_UNION spr_larea_end;
	DPU_DPP_SPR_LAREA_OFFSET_UNION spr_larea_offset;
	DPU_DPP_SPR_LAREA_GAIN_UNION spr_larea_gain;
	DPU_DPP_SPR_LAREA_BORDER_GAIN_R_UNION spr_larea_border_r;
	DPU_DPP_SPR_LAREA_BORDER_GAIN_G_UNION spr_larea_border_g;
	DPU_DPP_SPR_LAREA_BORDER_GAIN_B_UNION spr_larea_border_b;

	DPU_DPP_SPR_R_BORDERLR_REG_UNION spr_r_borderlr;
	DPU_DPP_SPR_R_BORDERTB_REG_UNION spr_r_bordertb;
	DPU_DPP_SPR_G_BORDERLR_REG_UNION spr_g_borderlr;
	DPU_DPP_SPR_G_BORDERTB_REG_UNION spr_g_bordertb;
	DPU_DPP_SPR_B_BORDERLR_REG_UNION spr_b_borderlr;
	DPU_DPP_SPR_B_BORDERTB_REG_UNION spr_b_bordertb;
	DPU_DPP_SPR_PIXGAIN_REG_UNION spr_pixgain_reg;
	DPU_DPP_SPR_PIXGAIN_REG1_UNION spr_pixgain_reg1;
	DPU_DPP_SPR_BORDER_POSITION0_UNION spr_border_p0;
	DPU_DPP_SPR_BORDER_POSITION1_UNION spr_border_p1;
	DPU_DPP_SPR_BORDER_POSITION2_UNION spr_border_p2;
	DPU_DPP_SPR_BORDER_POSITION3_UNION spr_border_p3;

	DPU_DPP_SPR_BLEND_UNION spr_blend;
	DPU_DPP_SPR_WEIGHT_UNION spr_weight;
	DPU_DPP_SPR_EDGESTR_R_UNION spr_edgestr_r;
	DPU_DPP_SPR_EDGESTR_G_UNION spr_edgestr_g;
	DPU_DPP_SPR_EDGESTR_B_UNION spr_edgestr_b;
	DPU_DPP_SPR_DIRWEIBLDMIN_UNION spr_dir_min;
	DPU_DPP_SPR_DIRWEIBLDMAX_UNION spr_dir_max;

	DPU_DPP_SPR_DIFFDIRGAIN_R0_UNION spr_diff_r0;
	DPU_DPP_SPR_DIFFDIRGAIN_R1_UNION spr_diff_r1;
	DPU_DPP_SPR_DIFFDIRGAIN_G0_UNION spr_diff_g0;
	DPU_DPP_SPR_DIFFDIRGAIN_G1_UNION spr_diff_g1;
	DPU_DPP_SPR_DIFFDIRGAIN_B0_UNION spr_diff_b0;
	DPU_DPP_SPR_DIFFDIRGAIN_B1_UNION spr_diff_b1;

	DPU_DPP_SPR_HORZGRADBLEND_UNION spr_horzgradblend;
	DPU_DPP_SPR_HORZBDBLD_UNION spr_horzbdbld;
	DPU_DPP_SPR_HORZBDWEIGHT_UNION spr_horzbdweight;
	DPU_DPP_SPR_VERTBDBLD_UNION spr_vertbdbld;
	DPU_DPP_SPR_VERTBDWEIGHT_UNION spr_vertbdweight;
	DPU_DPP_SPR_VERTBD_GAIN_R_UNION spr_vertbd_gain_r;
	DPU_DPP_SPR_VERTBD_GAIN_G_UNION spr_vertbd_gain_g;
	DPU_DPP_SPR_VERTBD_GAIN_B_UNION spr_vertbd_gain_b;

	DPU_DPP_SPR_GAMA_EN_UNION spr_gamma_en;
	DPU_DPP_SPR_GAMA_SHIFTEN_UNION spr_gamma_shiften;
	DPU_DPP_DEGAMA_EN_UNION degamma_en;

	DPU_DSC_TXIP_CTRL_UNION txip_ctrl;
	DPU_DSC_TXIP_SIZE_UNION txip_size;
	DPU_DSC_TXIP_COEF0_UNION txip_coef0;
	DPU_DSC_TXIP_COEF1_UNION txip_coef1;
	DPU_DSC_TXIP_COEF2_UNION txip_coef2;
	DPU_DSC_TXIP_COEF3_UNION txip_coef3;
	DPU_DSC_TXIP_OFFSET0_UNION txip_offset0;
	DPU_DSC_TXIP_OFFSET1_UNION txip_offset1;
	DPU_DSC_TXIP_CLIP_UNION txip_clip;

	DPU_DSC_DATAPACK_CTRL_UNION datapack_ctrl;
	DPU_DSC_DATAPACK_SIZE_UNION datapack_size;

	/* spr_lut_table contain gamma lut and degamma lut */
	uint32_t spr_lut_table_len;
	uint32_t *spr_lut_table;

	uint32_t panel_xres;
	uint32_t panel_yres;
};

void spr_init(struct spr_info *spr, char __iomem * dpp_base, char __iomem * dsc_base);
bool is_spr_enabled(struct spr_info *spr);
#endif
