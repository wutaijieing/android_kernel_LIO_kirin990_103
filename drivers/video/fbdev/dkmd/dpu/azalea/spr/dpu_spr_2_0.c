/* Copyright (c) 2019-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#include "../dpu_fb.h"
#include "dpu_spr.h"
#include "../dsc/dsc_algorithm.h"

static bool spr_is_param_valid(struct dpu_panel_info *pinfo, char __iomem * dss_base)
{
	char __iomem * dither_base = dss_base + DSS_DPP_DITHER_OFFSET;
	uint32_t dither_en = inp32(dither_base + DITHER_CTL0) & 0x01;
	uint32_t dither_mode = inp32(dither_base + DITHER_CTL1) & 0x03;

	/* when spr enable, disp_ch dither must enable and 12bits to 10bits out
	 * DITHER_CTL1.dither_mode = 1, DITHER_CTL0.dither_en = 1
	 */
	if (pinfo->spr.spr_en) {
		if ((dither_en != 1) || (dither_mode != 1)) {
			DPU_FB_ERR("error, dither_en=%d, dither_mode=%d", dither_en, dither_mode);
			return false;
		}
	}

	if ((pinfo->spr.spr_size.reg.spr_width != pinfo->xres - 1) ||
		(pinfo->spr.spr_size.reg.spr_height != pinfo->yres - 1)) {
		DPU_FB_ERR("spr width/height error\n");
		return false;
	}

	if (pinfo->spr.txip_ctrl.reg.txip_en == 1) {
		if ((pinfo->spr.txip_size.reg.txip_width != pinfo->xres - 1) ||
			(pinfo->spr.txip_size.reg.txip_height != pinfo->yres - 1)) {
			DPU_FB_ERR("txip width/height error\n");
			return false;
		}
	}

	if (pinfo->spr.datapack_ctrl.reg.datapack_en == 1) {
		if ((pinfo->spr.datapack_size.reg.datapack_width != pinfo->xres - 1) ||
			(pinfo->spr.datapack_size.reg.datapack_height != pinfo->yres - 1)) {
			DPU_FB_ERR("datapack width/height error\n");
			return false;
		}
	}

	/* when spr+dsc, datapack must unpack mode */
	if ((pinfo->spr_dsc_mode == SPR_DSC_MODE_SPR_AND_DSC) &&
		(pinfo->spr.datapack_ctrl.reg.datapack_packmode != 0)) {
		DPU_FB_ERR("error, spr+dsc, datapack must unpack mode");
		return false;
	}

	DPU_FB_INFO("spr parameters check pass\n");
	return true;
}

/* spr core parameters from c04_1_HiM1_SPR_RGBG.cfg */
static void spr_core_ctl_config(char __iomem * spr_base, struct spr_dsc_panel_para *spr)
{
	set_reg(DPU_DPP_SPR_SIZE_ADDR(spr_base), spr->spr_size.value, 32, 0);
	set_reg(DPU_DPP_SPR_CTRL_ADDR(spr_base), spr->spr_ctrl.value, 32, 0);
}

static void spr_core_pixsel_config(char __iomem * spr_base, struct spr_dsc_panel_para *spr)
{
	set_reg(DPU_DPP_SPR_PIX_EVEN_COEF_SEL_ADDR(spr_base), spr->spr_pix_even.value, 32, 0);
	set_reg(DPU_DPP_SPR_PIX_ODD_COEF_SEL_ADDR(spr_base), spr->spr_pix_odd.value, 32, 0);
	set_reg(DPU_DPP_SPR_PIX_PANEL_ARRANGE_SEL_ADDR(spr_base), spr->spr_pix_panel.value, 32, 0);
}

static void spr_core_coeffs_r_config(char __iomem * spr_base, struct spr_dsc_panel_para *spr)
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

static void spr_core_coeffs_g_config(char __iomem * spr_base, struct spr_dsc_panel_para *spr)
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

static void spr_core_coeffs_b_config(char __iomem * spr_base, struct spr_dsc_panel_para *spr)
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

static void spr_core_larea_config(char __iomem * spr_base, struct spr_dsc_panel_para *spr)
{
	set_reg(DPU_DPP_SPR_LAREA_START_ADDR(spr_base), spr->spr_larea_start.value, 32, 0);
	set_reg(DPU_DPP_SPR_LAREA_END_ADDR(spr_base), spr->spr_larea_end.value, 32, 0);
	set_reg(DPU_DPP_SPR_LAREA_OFFSET_ADDR(spr_base), spr->spr_larea_offset.value, 32, 0);
	set_reg(DPU_DPP_SPR_LAREA_GAIN_ADDR(spr_base), spr->spr_larea_gain.value, 32, 0);
	set_reg(DPU_DPP_SPR_LAREA_BORDER_GAIN_R_ADDR(spr_base), spr->spr_larea_border_r.value, 32, 0);
	set_reg(DPU_DPP_SPR_LAREA_BORDER_GAIN_G_ADDR(spr_base), spr->spr_larea_border_g.value, 32, 0);
	set_reg(DPU_DPP_SPR_LAREA_BORDER_GAIN_B_ADDR(spr_base), spr->spr_larea_border_b.value, 32, 0);
}

static void spr_core_border_config(char __iomem * spr_base, struct spr_dsc_panel_para *spr)
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


static void spr_core_blend_config(char __iomem * spr_base, struct spr_dsc_panel_para *spr)
{
	set_reg(DPU_DPP_SPR_BLEND_ADDR(spr_base), spr->spr_blend.value, 32, 0);
	set_reg(DPU_DPP_SPR_WEIGHT_ADDR(spr_base), spr->spr_weight.value, 32, 0);
	set_reg(DPU_DPP_SPR_EDGESTR_R_ADDR(spr_base), spr->spr_edgestr_r.value, 32, 0);
	set_reg(DPU_DPP_SPR_EDGESTR_G_ADDR(spr_base), spr->spr_edgestr_g.value, 32, 0);
	set_reg(DPU_DPP_SPR_EDGESTR_B_ADDR(spr_base), spr->spr_edgestr_b.value, 32, 0);
	set_reg(DPU_DPP_SPR_DIRWEIBLDMIN_ADDR(spr_base), spr->spr_dir_min.value, 32, 0);
	set_reg(DPU_DPP_SPR_DIRWEIBLDMAX_ADDR(spr_base), spr->spr_dir_max.value, 32, 0);
}


static void spr_core_diffdirgain_config(char __iomem * spr_base, struct spr_dsc_panel_para *spr)
{
	set_reg(DPU_DPP_SPR_DIFFDIRGAIN_R0_ADDR(spr_base), spr->spr_diff_r0.value, 32, 0);
	set_reg(DPU_DPP_SPR_DIFFDIRGAIN_R1_ADDR(spr_base), spr->spr_diff_r1.value, 32, 0);
	set_reg(DPU_DPP_SPR_DIFFDIRGAIN_G0_ADDR(spr_base), spr->spr_diff_g0.value, 32, 0);
	set_reg(DPU_DPP_SPR_DIFFDIRGAIN_G1_ADDR(spr_base), spr->spr_diff_g1.value, 32, 0);
	set_reg(DPU_DPP_SPR_DIFFDIRGAIN_B0_ADDR(spr_base), spr->spr_diff_b0.value, 32, 0);
	set_reg(DPU_DPP_SPR_DIFFDIRGAIN_B1_ADDR(spr_base), spr->spr_diff_b1.value, 32, 0);
}

static void spr_core_bd_config(char __iomem * spr_base, struct spr_dsc_panel_para *spr)
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

static void spr_core_config(char __iomem * spr_base, struct spr_dsc_panel_para *spr)
{
	DPU_FB_DEBUG("+\n");

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

	DPU_FB_DEBUG("-\n");
}

/* spr degamma, gamma lut table format:
 * lut_table[6*258] = gamma_r/g/b + degamma_r/g/b
 */
void spr_degamma_gamma_config(char __iomem * spr_base, struct spr_dsc_panel_para *spr)
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

static void spr_txip_cfg(char __iomem * spr_base, struct spr_dsc_panel_para *spr)
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

static void spr_datapack_cfg(char __iomem * spr_base, struct spr_dsc_panel_para *spr)
{
	set_reg(DPU_DSC_DATAPACK_CTRL_ADDR(spr_base), spr->datapack_ctrl.value, 32, 0);
	set_reg(DPU_DSC_DATAPACK_SIZE_ADDR(spr_base), spr->datapack_size.value, 32, 0);
}

static void spr_dither_config(char __iomem * spr_base, struct dpu_panel_info *pinfo)
{
	/* 4: 12bits->8bits ,5:12bits->10bits , 0x5 is bettter! */
	set_reg(DPU_DPP_DITHER_CTL1_ADDR(spr_base), 0x00000005, 6, 0);
	set_reg(DPU_DPP_DITHER_CTL0_ADDR(spr_base), 0x0000000b, 5, 0);

	set_reg(DPU_DPP_DITHER_IMG_SIZE_ADDR(spr_base),
		((pinfo->yres - 1) << 16) | (pinfo->xres - 1), 32, 0);
}

void spr_init(struct dpu_fb_data_type *dpufd, bool fastboot_enable)
{
	char __iomem * spr_base;
	struct dpu_panel_info *pinfo = NULL;
	struct spr_dsc_panel_para *spr = NULL;

	dpu_check_and_no_retval(!dpufd, ERR, "null dpufd!\n");

	pinfo = &dpufd->panel_info;
	if (pinfo->spr_dsc_mode == SPR_DSC_MODE_NONE)
		return;

	if (fastboot_enable) {
		DPU_FB_INFO("fastboot already enable!\n");
		return;
	}

	if (!spr_is_param_valid(pinfo, dpufd->dss_base))
		return;

	DPU_FB_INFO("+\n");
	spr = &(pinfo->spr);
	spr_base = dpufd->dss_base + SPR_OFFSET;

	/* init spr txip and datapack */
	spr_core_config(spr_base, spr);
	spr_degamma_gamma_config(spr_base, spr);
	spr_dither_config(spr_base, pinfo);
	spr_txip_cfg(spr_base, spr);
	spr_datapack_cfg(spr_base, spr);

	set_reg(spr_base + SPR_GLOBAL_EN, spr->spr_en, 32, 0);
	set_reg(spr_base + SPR_MEM_CTRL, 0x8, 32, 0);

	DPU_FB_INFO("-\n");
}

void spr_dsc_partial_updt_config(struct dpu_fb_data_type *dpufd, spr_dirty_region *spr_dirty)
{
	uint32_t overlap_value;
	struct dpu_panel_info *pinfo = NULL;
	struct spr_dsc_panel_para *spr = NULL;
	char __iomem * spr_base;

	if((dpufd == NULL) || (spr_dirty == NULL))
		return;

	if ((dpufd->panel_info.spr_dsc_mode == SPR_DSC_MODE_NONE) ||
		(dpufd->panel_info.spr.spr_en == 0))
		return;

	pinfo = &(dpufd->panel_info);
	spr = &(pinfo->spr);
	spr_base = dpufd->dss_base + SPR_OFFSET;

	switch (spr_dirty->spr_overlap_type) {
	case SPR_OVERLAP_TOP:
		overlap_value = 1;
		spr->spr_r_bordertb.reg.spr_bordertb_offsett_r = 0;
		spr->spr_r_bordertb.reg.spr_bordertb_gaint_r = 0x80;
		spr->spr_g_bordertb.reg.spr_bordertb_offsett_g = 0;
		spr->spr_g_bordertb.reg.spr_bordertb_gaint_g = 0x80;
		spr->spr_b_bordertb.reg.spr_bordertb_offsett_b = 0;
		spr->spr_b_bordertb.reg.spr_bordertb_gaint_b = 0x80;
		break;
	case SPR_OVERLAP_BOTTOM:
		overlap_value = 2;
		spr->spr_r_bordertb.reg.spr_bordertb_offsetb_r = 0;
		spr->spr_r_bordertb.reg.spr_bordertb_gainb_r = 0x80;
		spr->spr_g_bordertb.reg.spr_bordertb_offsetb_g = 0;
		spr->spr_g_bordertb.reg.spr_bordertb_gainb_g = 0x80;
		spr->spr_b_bordertb.reg.spr_bordertb_offsetb_b = 0;
		spr->spr_b_bordertb.reg.spr_bordertb_gainb_b = 0x80;
		break;
	case SPR_OVERLAP_TOP_BOTTOM:
		overlap_value = 3;
		spr->spr_r_bordertb.value = SPR_CLOSE_BOARDER_GAIN;
		spr->spr_g_bordertb.value = SPR_CLOSE_BOARDER_GAIN;
		spr->spr_b_bordertb.value = SPR_CLOSE_BOARDER_GAIN;
		break;
	default:
		overlap_value = 0;
		spr->spr_r_bordertb.reg.spr_bordertb_offsetb_r = 0;
		spr->spr_r_bordertb.reg.spr_bordertb_gainb_r = 0;
		spr->spr_g_bordertb.reg.spr_bordertb_offsetb_g = 0;
		spr->spr_g_bordertb.reg.spr_bordertb_gainb_g = 0;
		spr->spr_b_bordertb.reg.spr_bordertb_offsetb_b = 0;
		spr->spr_b_bordertb.reg.spr_bordertb_gainb_b = 0;
		break;
	}

	DPU_FB_DEBUG("spr_overlap_type = %d\n", spr_dirty->spr_overlap_type);

	set_reg(DPU_DPP_SPR_R_BORDERTB_REG_ADDR(spr_base), spr->spr_r_bordertb.value, 32, 0);
	set_reg(DPU_DPP_SPR_G_BORDERTB_REG_ADDR(spr_base), spr->spr_g_bordertb.value, 32, 0);
	set_reg(DPU_DPP_SPR_B_BORDERTB_REG_ADDR(spr_base), spr->spr_b_bordertb.value, 32, 0);

	/* spr_ctrl.spr_hpartial_mode */
	set_reg(DPU_DPP_SPR_CTRL_ADDR(spr_base), overlap_value, 2, 17);
	set_reg(DPU_DPP_SPR_SIZE_ADDR(spr_base), spr_dirty->spr_img_size, 32, 0);
	return;
}

