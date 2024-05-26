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

#include <linux/module.h>
#include "spr_config.h"
#include "dkmd_log.h"

bool is_spr_enabled(struct spr_info *spr)
{
	if (!spr) {
		dpu_pr_err("spr is null!\n");
		return false;
	}

	return (spr->spr_ctrl.reg.spr_en == SPR_ENABLE);
}

static void spr_core_ctl_config(char __iomem * spr_base, struct spr_info *spr)
{
	set_reg(DPU_DPP_SPR_SIZE_ADDR(spr_base), spr->spr_size.value, 32, 0);
	set_reg(DPU_DPP_SPR_CTRL_ADDR(spr_base), spr->spr_ctrl.value, 32, 0);
}

static void spr_core_pixsel_config(char __iomem * spr_base, struct spr_info *spr)
{
	set_reg(DPU_DPP_SPR_PIX_EVEN_COEF_SEL_ADDR(spr_base), spr->spr_pix_even.value, 32, 0);
	set_reg(DPU_DPP_SPR_PIX_ODD_COEF_SEL_ADDR(spr_base), spr->spr_pix_odd.value, 32, 0);
	set_reg(DPU_DPP_SPR_PIX_PANEL_ARRANGE_SEL_ADDR(spr_base), spr->spr_pix_panel.value, 32, 0);
}

static void spr_core_coeffs_r_config(char __iomem * spr_base, struct spr_info *spr)
{
	set_reg(DPU_DPP_SPR_COEFF_V0H0_R0_ADDR(spr_base), spr->spr_coeff_v0h0_r0.value, 32, 0);
	set_reg(DPU_DPP_SPR_COEFF_V0H0_R1_ADDR(spr_base), spr->spr_coeff_v0h0_r1.value, 32, 0);
	set_reg(DPU_DPP_SPR_COEFF_V0H1_R0_ADDR(spr_base), spr->spr_coeff_v0h1_r0.value, 32, 0);
	set_reg(DPU_DPP_SPR_COEFF_V0H1_R1_ADDR(spr_base), spr->spr_coeff_v0h1_r1.value, 32, 0);
	set_reg(DPU_DPP_SPR_COEFF_V1H0_R0_ADDR(spr_base), spr->spr_coeff_v1h0_r0.value, 32, 0);
	set_reg(DPU_DPP_SPR_COEFF_V1H0_R1_ADDR(spr_base), spr->spr_coeff_v1h0_r1.value, 32, 0);
	set_reg(DPU_DPP_SPR_COEFF_V1H1_R0_ADDR(spr_base), spr->spr_coeff_v1h1_r0.value, 32, 0);
	set_reg(DPU_DPP_SPR_COEFF_V1H1_R1_ADDR(spr_base), spr->spr_coeff_v1h1_r1.value, 32, 0);
}

static void spr_core_coeffs_g_config(char __iomem * spr_base, struct spr_info *spr)
{
	set_reg(DPU_DPP_SPR_COEFF_V0H0_G0_ADDR(spr_base), spr->spr_coeff_v0h0_g0.value, 32, 0);
	set_reg(DPU_DPP_SPR_COEFF_V0H0_G1_ADDR(spr_base), spr->spr_coeff_v0h0_g1.value, 32, 0);
	set_reg(DPU_DPP_SPR_COEFF_V0H1_G0_ADDR(spr_base), spr->spr_coeff_v0h1_g0.value, 32, 0);
	set_reg(DPU_DPP_SPR_COEFF_V0H1_G1_ADDR(spr_base), spr->spr_coeff_v0h1_g1.value, 32, 0);
	set_reg(DPU_DPP_SPR_COEFF_V1H0_G0_ADDR(spr_base), spr->spr_coeff_v1h0_g0.value, 32, 0);
	set_reg(DPU_DPP_SPR_COEFF_V1H0_G1_ADDR(spr_base), spr->spr_coeff_v1h0_g1.value, 32, 0);
	set_reg(DPU_DPP_SPR_COEFF_V1H1_G0_ADDR(spr_base), spr->spr_coeff_v1h1_g0.value, 32, 0);
	set_reg(DPU_DPP_SPR_COEFF_V1H1_G1_ADDR(spr_base), spr->spr_coeff_v1h1_g1.value, 32, 0);
}

static void spr_core_coeffs_b_config(char __iomem * spr_base, struct spr_info *spr)
{
	set_reg(DPU_DPP_SPR_COEFF_V0H0_B0_ADDR(spr_base), spr->spr_coeff_v0h0_b1.value, 32, 0);
	set_reg(DPU_DPP_SPR_COEFF_V0H0_B1_ADDR(spr_base), spr->spr_coeff_v0h0_b0.value, 32, 0);
	set_reg(DPU_DPP_SPR_COEFF_V0H1_B0_ADDR(spr_base), spr->spr_coeff_v0h1_b0.value, 32, 0);
	set_reg(DPU_DPP_SPR_COEFF_V0H1_B1_ADDR(spr_base), spr->spr_coeff_v0h1_b1.value, 32, 0);
	set_reg(DPU_DPP_SPR_COEFF_V1H0_B0_ADDR(spr_base), spr->spr_coeff_v1h0_b0.value, 32, 0);
	set_reg(DPU_DPP_SPR_COEFF_V1H0_B1_ADDR(spr_base), spr->spr_coeff_v1h0_b1.value, 32, 0);
	set_reg(DPU_DPP_SPR_COEFF_V1H1_B0_ADDR(spr_base), spr->spr_coeff_v1h1_b0.value, 32, 0);
	set_reg(DPU_DPP_SPR_COEFF_V1H1_B1_ADDR(spr_base), spr->spr_coeff_v1h1_b1.value, 32, 0);
}

static void spr_core_larea_config(char __iomem * spr_base, struct spr_info *spr)
{
	set_reg(DPU_DPP_SPR_LAREA_START_ADDR(spr_base), spr->spr_larea_start.value, 32, 0);
	set_reg(DPU_DPP_SPR_LAREA_END_ADDR(spr_base), spr->spr_larea_end.value, 32, 0);
	set_reg(DPU_DPP_SPR_LAREA_OFFSET_ADDR(spr_base), spr->spr_larea_offset.value, 32, 0);
	set_reg(DPU_DPP_SPR_LAREA_GAIN_ADDR(spr_base), spr->spr_larea_gain.value, 32, 0);
	set_reg(DPU_DPP_SPR_LAREA_BORDER_GAIN_R_ADDR(spr_base), spr->spr_larea_border_r.value, 32, 0);
	set_reg(DPU_DPP_SPR_LAREA_BORDER_GAIN_G_ADDR(spr_base), spr->spr_larea_border_g.value, 32, 0);
	set_reg(DPU_DPP_SPR_LAREA_BORDER_GAIN_B_ADDR(spr_base), spr->spr_larea_border_b.value, 32, 0);
}

static void spr_core_border_config(char __iomem * spr_base, struct spr_info *spr)
{
	set_reg(DPU_DPP_SPR_R_BORDERLR_REG_ADDR(spr_base), spr->spr_r_borderlr.value, 32, 0);
	set_reg(DPU_DPP_SPR_R_BORDERTB_REG_ADDR(spr_base), spr->spr_r_bordertb.value, 32, 0);
	set_reg(DPU_DPP_SPR_G_BORDERLR_REG_ADDR(spr_base), spr->spr_g_borderlr.value, 32, 0);
	set_reg(DPU_DPP_SPR_G_BORDERTB_REG_ADDR(spr_base), spr->spr_g_bordertb.value, 32, 0);
	set_reg(DPU_DPP_SPR_B_BORDERLR_REG_ADDR(spr_base), spr->spr_b_borderlr.value, 32, 0);
	set_reg(DPU_DPP_SPR_B_BORDERTB_REG_ADDR(spr_base), spr->spr_b_bordertb.value, 32, 0);
	set_reg(DPU_DPP_SPR_PIXGAIN_REG_ADDR(spr_base), spr->spr_pixgain_reg.value, 32, 0);
	set_reg(DPU_DPP_SPR_PIXGAIN_REG1_ADDR(spr_base), spr->spr_pixgain_reg1.value, 32, 0);
	set_reg(DPU_DPP_SPR_BORDER_POSITION0_ADDR(spr_base), spr->spr_border_p0.value, 32, 0);
	set_reg(DPU_DPP_SPR_BORDER_POSITION1_ADDR(spr_base), spr->spr_border_p1.value, 32, 0);
	set_reg(DPU_DPP_SPR_BORDER_POSITION2_ADDR(spr_base), spr->spr_border_p2.value, 32, 0);
	set_reg(DPU_DPP_SPR_BORDER_POSITION3_ADDR(spr_base), spr->spr_border_p3.value, 32, 0);
}


static void spr_core_blend_config(char __iomem * spr_base, struct spr_info *spr)
{
	set_reg(DPU_DPP_SPR_BLEND_ADDR(spr_base), spr->spr_blend.value, 32, 0);
	set_reg(DPU_DPP_SPR_WEIGHT_ADDR(spr_base), spr->spr_weight.value, 32, 0);
	set_reg(DPU_DPP_SPR_EDGESTR_R_ADDR(spr_base), spr->spr_edgestr_r.value, 32, 0);
	set_reg(DPU_DPP_SPR_EDGESTR_G_ADDR(spr_base), spr->spr_edgestr_g.value, 32, 0);
	set_reg(DPU_DPP_SPR_EDGESTR_B_ADDR(spr_base), spr->spr_edgestr_b.value, 32, 0);
	set_reg(DPU_DPP_SPR_DIRWEIBLDMIN_ADDR(spr_base), spr->spr_dir_min.value, 32, 0);
	set_reg(DPU_DPP_SPR_DIRWEIBLDMAX_ADDR(spr_base), spr->spr_dir_max.value, 32, 0);
}


static void spr_core_diffdirgain_config(char __iomem * spr_base, struct spr_info *spr)
{
	set_reg(DPU_DPP_SPR_DIFFDIRGAIN_R0_ADDR(spr_base), spr->spr_diff_r0.value, 32, 0);
	set_reg(DPU_DPP_SPR_DIFFDIRGAIN_R1_ADDR(spr_base), spr->spr_diff_r1.value, 32, 0);
	set_reg(DPU_DPP_SPR_DIFFDIRGAIN_G0_ADDR(spr_base), spr->spr_diff_g0.value, 32, 0);
	set_reg(DPU_DPP_SPR_DIFFDIRGAIN_G1_ADDR(spr_base), spr->spr_diff_g1.value, 32, 0);
	set_reg(DPU_DPP_SPR_DIFFDIRGAIN_B0_ADDR(spr_base), spr->spr_diff_b0.value, 32, 0);
	set_reg(DPU_DPP_SPR_DIFFDIRGAIN_B1_ADDR(spr_base), spr->spr_diff_b1.value, 32, 0);
}

static void spr_core_bd_config(char __iomem * spr_base, struct spr_info *spr)
{
	set_reg(DPU_DPP_SPR_HORZGRADBLEND_ADDR(spr_base), spr->spr_horzgradblend.value, 32, 0);
	set_reg(DPU_DPP_SPR_HORZBDBLD_ADDR(spr_base), spr->spr_horzbdbld.value, 32, 0);
	set_reg(DPU_DPP_SPR_HORZBDWEIGHT_ADDR(spr_base), spr->spr_horzbdweight.value, 32, 0);
	set_reg(DPU_DPP_SPR_VERTBDBLD_ADDR(spr_base), spr->spr_vertbdbld.value, 32, 0);
	set_reg(DPU_DPP_SPR_VERTBDWEIGHT_ADDR(spr_base), spr->spr_vertbdweight.value, 32, 0);
	set_reg(DPU_DPP_SPR_VERTBD_GAIN_R_ADDR(spr_base), spr->spr_vertbd_gain_r.value, 32, 0);
	set_reg(DPU_DPP_SPR_VERTBD_GAIN_G_ADDR(spr_base), spr->spr_vertbd_gain_g.value, 32, 0);
	set_reg(DPU_DPP_SPR_VERTBD_GAIN_B_ADDR(spr_base), spr->spr_vertbd_gain_b.value, 32, 0);
}

static void spr_core_config(char __iomem * spr_base, struct spr_info *spr)
{
	dpu_pr_debug("+\n");

	spr_core_ctl_config(spr_base, spr);
	spr_core_pixsel_config(spr_base, spr);
	spr_core_coeffs_r_config(spr_base, spr);
	spr_core_coeffs_g_config(spr_base, spr);
	spr_core_coeffs_b_config(spr_base, spr);
	spr_core_larea_config(spr_base, spr);
	spr_core_border_config(spr_base, spr);
	spr_core_blend_config(spr_base, spr);
	spr_core_diffdirgain_config(spr_base, spr);
	spr_core_bd_config(spr_base, spr);

	set_reg(DPU_DPP_SPR_MEM_CTRL_ADDR(spr_base), 0x8, 32, 0);
	dpu_pr_debug("-\n");
}

/* spr degamma, gamma lut table format:
 * lut_table[6*258] = gamma_r/g/b + degamma_r/g/b
 */
void spr_degamma_gamma_config(char __iomem * spr_base, struct spr_info *spr)
{
	uint32_t i, row, column, idx;
	uint32_t lut_val[SPR_GAMMA_LUT_ROW] = {0};
	uint32_t *spr_lut_addr = spr->spr_lut_table;
	uint32_t lut_length = spr->spr_lut_table_len / SPR_GAMMA_LUT_ROW;

	set_reg(DPU_DPP_SPR_GAMA_EN_ADDR(spr_base), spr->spr_gamma_en.value, 32, 0);
	set_reg(DPU_DPP_SPR_GAMA_SHIFTEN_ADDR(spr_base), spr->spr_gamma_shiften.value, 32, 0);
	set_reg(DPU_DPP_SPR_GAMA_MEM_CTRL_ADDR(spr_base), 0x8, 32, 0);

	set_reg(DPU_DPP_DEGAMA_EN_ADDR(spr_base), spr->degamma_en.value, 32, 0);
	set_reg(DPU_DPP_DEGAMA_MEM_CTRL_ADDR(spr_base), 0x8, 32, 0);

	for (i = 0, column = 0; column < lut_length; column += COEF_PER_REG, i++) {
		for (row = 0; row < SPR_GAMMA_LUT_ROW; row++) {
			idx = lut_length * row + column;
			lut_val[row] = (spr_lut_addr[idx + 1] << 16) | spr_lut_addr[idx];
		}
		set_reg(DPU_DPP_U_GAMA_R_COEF_ADDR(spr_base, i), lut_val[SPR_GAMMA_R], 32, 0);
		set_reg(DPU_DPP_U_GAMA_G_COEF_ADDR(spr_base, i), lut_val[SPR_GAMMA_G], 32, 0);
		set_reg(DPU_DPP_U_GAMA_B_COEF_ADDR(spr_base, i), lut_val[SPR_GAMMA_B], 32, 0);
		set_reg(DPU_DPP_U_DEGAMA_R_COEF_ADDR(spr_base, i), lut_val[SPR_DEGAMMA_R], 32, 0);
		set_reg(DPU_DPP_U_DEGAMA_G_COEF_ADDR(spr_base, i), lut_val[SPR_DEGAMMA_G], 32, 0);
		set_reg(DPU_DPP_U_DEGAMA_B_COEF_ADDR(spr_base, i), lut_val[SPR_DEGAMMA_B], 32, 0);
	}
}

static void spr_txip_cfg(char __iomem * spr_base, struct spr_info *spr)
{
	set_reg(DPU_DSC_TXIP_CTRL_ADDR(spr_base), spr->txip_ctrl.value, 32, 0);
	set_reg(DPU_DSC_TXIP_SIZE_ADDR(spr_base), spr->txip_size.value, 32, 0);
	set_reg(DPU_DSC_TXIP_COEF0_ADDR(spr_base), spr->txip_coef0.value, 32, 0);
	set_reg(DPU_DSC_TXIP_COEF1_ADDR(spr_base), spr->txip_coef1.value, 32, 0);
	set_reg(DPU_DSC_TXIP_COEF2_ADDR(spr_base), spr->txip_coef2.value, 32, 0);
	set_reg(DPU_DSC_TXIP_COEF3_ADDR(spr_base), spr->txip_coef3.value, 32, 0);
	set_reg(DPU_DSC_TXIP_OFFSET0_ADDR(spr_base), spr->txip_offset0.value, 32, 0);
	set_reg(DPU_DSC_TXIP_OFFSET1_ADDR(spr_base), spr->txip_offset1.value, 32, 0);
	set_reg(DPU_DSC_TXIP_CLIP_ADDR(spr_base), spr->txip_clip.value, 32, 0);
}

static void spr_datapack_cfg(char __iomem * spr_base, struct spr_info *spr)
{
	set_reg(DPU_DSC_DATAPACK_CTRL_ADDR(spr_base), spr->datapack_ctrl.value, 32, 0);
	set_reg(DPU_DSC_DATAPACK_SIZE_ADDR(spr_base), spr->datapack_size.value, 32, 0);
}

static void spr_dither_config(char __iomem * spr_base, uint32_t panel_xres, uint32_t panel_yres)
{
	DPU_DPP_DITHER_CTL0_UNION ctrl0 = { .value = 0 };
	DPU_DPP_DITHER_CTL1_UNION ctrl1 = { .value = 0 };
	DPU_DPP_DITHER_IMG_SIZE_UNION image_size = { .value = 0 };

	ctrl0.reg.dither_en = 1;
	ctrl0.reg.dither_hifreq_noise_mode = 1;
	ctrl0.reg.dither_rgb_shift_mode = 1;
	set_reg(DPU_DPP_DITHER_CTL0_ADDR(spr_base), ctrl0.value, 5, 0);

	/* 4: 12bits->8bits ,5:12bits->10bits , 0x5 is bettter! */
	ctrl1.reg.dither_mode = 1;
	ctrl1.reg.dither_sel = 1;
	set_reg(DPU_DPP_DITHER_CTL1_ADDR(spr_base), ctrl1.value, 6, 0);

	image_size.reg.dpp_dither_img_width = panel_xres - 1;
	image_size.reg.dpp_dither_img_height = panel_yres - 1;
	set_reg(DPU_DPP_DITHER_IMG_SIZE_ADDR(spr_base), image_size.value, 32, 0);
}

static void dpp_gmp_config(char __iomem * dpp_base, uint32_t panel_xres, uint32_t panel_yres)
{
	DPU_DPP_GMP_EN_UNION gmp_en;
	DPU_DPP_GMP_SIZE_UNION gmp_size;

	gmp_en.value = 0;
	gmp_en.reg.gmp_bitext_mode = 2;
	gmp_en.reg.gmp_en = 0;

	gmp_size.value = 0;
	gmp_size.reg.gmp_width = panel_xres - 1;
	gmp_size.reg.gmp_height = panel_yres - 1;

	set_reg(DPU_DPP_GMP_EN_ADDR(dpp_base), gmp_en.value, 32, 0);
	set_reg(DPU_DPP_GMP_SIZE_ADDR(dpp_base), gmp_size.value, 32, 0);
}

void spr_init(struct spr_info *spr, char __iomem *dpp_base, char __iomem *dsc_base)
{
	if (!spr || !dpp_base || !dsc_base) {
		dpu_pr_err("spr or dpp_base or dsc_base is null!\n");
		return;
	}

	if (!is_spr_enabled(spr))
		return;

	dpu_pr_info("enter\n");
	spr_core_config(dpp_base, spr);
	spr_degamma_gamma_config(dpp_base, spr);
	spr_dither_config(dpp_base, spr->panel_xres, spr->panel_yres);
	spr_txip_cfg(dsc_base, spr);
	spr_datapack_cfg(dsc_base, spr);
	dpp_gmp_config(dpp_base, spr->panel_xres, spr->panel_yres);
}

MODULE_LICENSE("GPL");