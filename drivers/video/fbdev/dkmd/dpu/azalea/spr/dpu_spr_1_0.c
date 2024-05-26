/* Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
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

#define SPR_LUT_LEN 65
#define SPR_LUT_INDEX 6
#define SPR_CLOSE_BOARDER_GAIN 0x800080

#if defined(CONFIG_DPU_FB_V600) || defined(CONFIG_DPU_FB_V360)
const uint32_t spr_lut_addr[SPR_LUT_INDEX] = {
	SPR_GAMMA_LUT_R,
	SPR_GAMMA_LUT_G,
	SPR_GAMMA_LUT_B,
	SPR_DEGAMMA_LUT_R,
	SPR_DEGAMMA_LUT_G,
	SPR_DEGAMMA_LUT_B,
};
#endif

static void spr_coef_offset_config(char __iomem *spr_coef_base,
	uint32_t offset, uint32_t coef[])
{
	/* set one group coefficients using 3 registers */
	outp32(spr_coef_base + offset, coef[0]);
	outp32(spr_coef_base + offset + 4, coef[1]);
	outp32(spr_coef_base + offset + 8, coef[2]);
}

static void spr_init_coeff(char __iomem *spr_base, struct spr_dsc_panel_para *spr)
{
	uint32_t offset = 0;

	/* set Red's v0h0 v0h1 v1h0 v1h1 coefficients */
	spr_coef_offset_config(spr_base + SPR_COEFF_OFFSET, offset, spr->spr_r_v0h0_coef);
	offset += 0xC;
	spr_coef_offset_config(spr_base + SPR_COEFF_OFFSET, offset, spr->spr_r_v0h1_coef);
	offset += 0xC;
	spr_coef_offset_config(spr_base + SPR_COEFF_OFFSET, offset, spr->spr_r_v1h0_coef);
	offset += 0xC;
	spr_coef_offset_config(spr_base + SPR_COEFF_OFFSET, offset, spr->spr_r_v1h1_coef);

	/* set Green's v0h0 v0h1 v1h0 v1h1 coefficients */
	offset += 0xC;
	spr_coef_offset_config(spr_base + SPR_COEFF_OFFSET, offset, spr->spr_g_v0h0_coef);
	offset += 0xC;
	spr_coef_offset_config(spr_base + SPR_COEFF_OFFSET, offset, spr->spr_g_v0h1_coef);
	offset += 0xC;
	spr_coef_offset_config(spr_base + SPR_COEFF_OFFSET, offset, spr->spr_g_v1h0_coef);
	offset += 0xC;
	spr_coef_offset_config(spr_base + SPR_COEFF_OFFSET, offset, spr->spr_g_v1h1_coef);

	/* set Blue's v0h0 v0h1 v1h0 v1h1 coefficients */
	offset += 0xC;
	spr_coef_offset_config(spr_base + SPR_COEFF_OFFSET, offset, spr->spr_b_v0h0_coef);
	offset += 0xC;
	spr_coef_offset_config(spr_base + SPR_COEFF_OFFSET, offset, spr->spr_b_v0h1_coef);
	offset += 0xC;
	spr_coef_offset_config(spr_base + SPR_COEFF_OFFSET, offset, spr->spr_b_v1h0_coef);
	offset += 0xC;
	spr_coef_offset_config(spr_base + SPR_COEFF_OFFSET, offset, spr->spr_b_v1h1_coef);
}

static void spr_core_init(char __iomem *spr_base, struct spr_dsc_panel_para *spr)
{
	outp32(spr_base + SPR_PIX_EVEN_COEF_SEL, spr->spr_coeffsel_even);
	outp32(spr_base + SPR_PIX_ODD_COEF_SEL, spr->spr_coeffsel_odd);
	outp32(spr_base + SPR_PIX_PANEL_ARRANGE_SEL, spr->pix_panel_arrange_sel);

	spr_init_coeff(spr_base, spr);

	/* set RGB's border detect gain parameters */
	outp32(spr_base + SPR_BORDERLR_REG, spr->spr_borderlr_detect_r);
	outp32(spr_base + SPR_BORDERLR_REG + 0x4, spr->spr_bordertb_detect_r);
	outp32(spr_base + SPR_BORDERLR_REG + 0x8, spr->spr_borderlr_detect_g);
	outp32(spr_base + SPR_BORDERLR_REG + 0xC, spr->spr_bordertb_detect_g);
	outp32(spr_base + SPR_BORDERLR_REG + 0x10, spr->spr_borderlr_detect_b);
	outp32(spr_base + SPR_BORDERLR_REG + 0x14, spr->spr_bordertb_detect_b);

	/* set RGB's final gain parameters */
	outp32(spr_base + SPR_PIXGAIN_REG, (spr->spr_pixgain >> 32) & 0xFFFFFFFF);
	outp32(spr_base + SPR_PIXGAIN_REG1, spr->spr_pixgain & 0xFFFFFFFF);

	/* set boarder region */
	outp32(spr_base + SPR_BORDER_POSITION0, spr->spr_borderl_position);
	outp32(spr_base + SPR_BORDER_POSITION0 + 0x4, spr->spr_borderr_position);
	outp32(spr_base + SPR_BORDER_POSITION0 + 0x8, spr->spr_bordert_position);
	outp32(spr_base + SPR_BORDER_POSITION0 + 0xC, spr->spr_borderb_position);

	/* set RGB's diff ,weight regs */
	outp32(spr_base + SPR_R_DIFF_REG, spr->spr_r_diff);
	outp32(spr_base + SPR_R_WEIGHT_REG0, (spr->spr_r_weight >> 32) & 0xFFFFFFFF);
	outp32(spr_base + SPR_R_WEIGHT_REG1, spr->spr_r_weight & 0xFFFFFFFF);
	outp32(spr_base + SPR_R_DIFF_REG + 0xC, spr->spr_g_diff);
	outp32(spr_base + SPR_R_WEIGHT_REG0 + 0xC, (spr->spr_g_weight >> 32) & 0xFFFFFFFF);
	outp32(spr_base + SPR_R_WEIGHT_REG1 + 0xC, spr->spr_g_weight & 0xFFFFFFFF);
	outp32(spr_base + SPR_R_DIFF_REG + 0xC + 0xC, spr->spr_b_diff);
	outp32(spr_base + SPR_R_WEIGHT_REG0 + 0xC + 0xC, (spr->spr_b_weight >> 32) & 0xFFFFFFFF);
	outp32(spr_base + SPR_R_WEIGHT_REG1 + 0xC + 0xC, spr->spr_b_weight & 0xFFFFFFFF);

	outp32(spr_base + SPR_CSC_COEFF0, spr->spr_rgbg2uyvy_coeff[0]);
	outp32(spr_base + SPR_CSC_COEFF1, spr->spr_rgbg2uyvy_coeff[1]);
	outp32(spr_base + SPR_CSC_OFFSET0, spr->spr_rgbg2uyvy_coeff[2]);
	outp32(spr_base + SPR_CSC_OFFSET1, spr->spr_rgbg2uyvy_coeff[3]);
	outp32(spr_base + SPR_RESO, spr->input_reso);
}

#ifdef SUPPORT_SPR_DSC1_2_SEPARATE
static void spr_dsc12_separate_init(char __iomem *spr_base, struct spr_dsc_panel_para *spr)
{
	uint32_t offset;

	outp32(spr_base + DSC1_2_CTRL, (spr->slice1_ck_gt_en << 2) | (spr->slice0_ck_gt_en << 1) | spr->dsc_enable);
	outp32(spr_base + DSC_RC_BITS, spr->rcb_bits);
	outp32(spr_base + DSC_ALG_CTRL, spr->dsc_alg_ctrl);
	outp32(spr_base + DSC_BPC, spr->bits_per_component);
	outp32(spr_base + DSC_SAMPLE, spr->dsc_sample);
	outp32(spr_base + DSC_BPP_CHK, spr->bpp_chk);
	outp32(spr_base + DSC_PIC_RESO, spr->pic_reso);
	outp32(spr_base + DSC_SLC_RESO, spr->slc_reso);

	outp32(spr_base + DSC_INIT_XMIT_DLY, spr->initial_xmit_delay);
	outp32(spr_base + DSC_INIT_DEC_DLY, spr->initial_dec_delay);
	outp32(spr_base + DSC_INIT_SCALE, spr->initial_scale_value);
	outp32(spr_base + DSC_SCALE_INTVAL, spr->scale_interval);
	outp32(spr_base + DSC_FIRST_BPG, spr->first_bpg);
	outp32(spr_base + DSC_SECOND_BPG, spr->second_bpg);
	outp32(spr_base + DSC_SECOND_ADJ, spr->second_adj);
	outp32(spr_base + DSC_INIT_FINL_OFS, spr->init_finl_ofs);
	outp32(spr_base + DSC_SLC_BPG, spr->slc_bpg);
	outp32(spr_base + DSC_FLAT_RANGE, spr->flat_range);
	outp32(spr_base + DSC_RC_MOD_EDGE, spr->rc_mode_edge);
	outp32(spr_base + DSC_RC_QUA_TGT, spr->rc_qua_tgt);

	/* Specify thresholds in the "RC model" for the 15 ranges defined by 14 thresholds */
	for (offset = 0; offset < SPR_RC_BUF_THRESH_LEN; offset++)
		outp32(spr_base + DSC_RC_THRE + (offset << 2), spr->rc_buf_thresh[offset]);

	/* set RC params for 15 registers */
	for (offset = 0; offset < SPR_RC_PARA_LEN; offset++)
		outp32(spr_base + DSC_RC_PARAM + (offset << 2), spr->rc_para[offset]);
}

static void spr_lut_config(char __iomem *spr_base, struct spr_dsc_panel_para *pinfo)
{
	int idx;
	uint32_t lut;
	int gama_len = SPR_LUT_LEN * 3;

	if (!pinfo)
		return;

	set_reg(spr_base + SPR_CTRL, 0x1, 1, 12);
	for (idx = 0; idx < gama_len; idx++) {
		lut = (pinfo->spr_lut_table[idx] << 16) | pinfo->spr_lut_table[idx + gama_len];
		outp32(spr_base + SPR_LUT_WR_DATA, lut);
	}
	set_reg(spr_base + SPR_CTRL, 0x0, 1, 12);
}
#else
static void spr_lut_config(char __iomem *spr_base, struct spr_dsc_panel_para *pinfo)
{
	int idx;
	int lut_idx;
	uint32_t low_part_index;
	uint32_t hight_part_index;
	uint32_t lut;

	if ((!pinfo) || (!pinfo->spr_lut_table))
		return;

	for (idx = 0; idx < SPR_LUT_LEN; idx += 2) {
		for (lut_idx = 0; lut_idx < SPR_LUT_INDEX; lut_idx++) {
			low_part_index = idx + SPR_LUT_LEN * lut_idx;

			if (idx + 1 == SPR_LUT_LEN)
				hight_part_index = low_part_index;
			else
				hight_part_index = idx + 1 + SPR_LUT_LEN * lut_idx;

			if ((low_part_index > (SPR_LUT_LEN * SPR_LUT_INDEX - 1)) ||
				(hight_part_index > (SPR_LUT_LEN * SPR_LUT_INDEX - 1))) {
				DPU_FB_ERR("LUT CONFIG ERROR low_part_index = %d, hight_part_index = %d\n",
					low_part_index, hight_part_index);
				return;
			}

			lut = (pinfo->spr_lut_table[hight_part_index] << 16) | pinfo->spr_lut_table[low_part_index];

			outp32(spr_base + spr_lut_addr[lut_idx] + 0x4 * (idx / 2), lut);
		}
	}
}
#endif

void spr_init(struct dpu_fb_data_type *dpufd, bool fastboot_enable)
{
	char __iomem *spr_base = NULL;
	struct dpu_panel_info *pinfo = NULL;
	struct spr_dsc_panel_para *spr = NULL;
	uint32_t spr_ctrl_value;

	if (!dpufd)
		return;

	pinfo = &(dpufd->panel_info);
	spr_base = dpufd->dss_base + SPR_OFFSET;

	if (pinfo->spr_dsc_mode == SPR_DSC_MODE_NONE) {
		outp32(spr_base + SPR_CTRL, 0);
		return;
	}

	if (fastboot_enable)
		return;

	DPU_FB_INFO("+\n");
	spr = &(pinfo->spr);

	/* init spr ctrl reg */
	spr_ctrl_value = (spr->spr_rgbg2uyvy_8biten << 17) |
		(spr->spr_hpartial_mode << 15) |
		(spr->spr_partial_mode << 13) |
		(spr->spr_rgbg2uyvy_en << 11) |
		(spr->spr_horzborderdect << 10) |
		(spr->spr_linebuf_1dmode << 9) |
		(spr->spr_bordertb_dummymode << 8) |
		(spr->spr_borderlr_dummymode << 6) |
		(spr->spr_pattern_mode << 4) |
		(spr->spr_pattern_en << 3) |
		(spr->spr_subpxl_layout << 2) |
		(spr->ck_gt_spr_en << 1) |
		spr->spr_en;

	DPU_FB_INFO("spr_ctrl_value = 0x%x\n", spr_ctrl_value);
	outp32(spr_base + SPR_CTRL, spr_ctrl_value);

	if (spr->spr_pattern_en) {
		/* spr pattern generate config */
		outp32(spr_base + SPR_PATTERNGEN_POSITION, (pinfo->xres << 12) | 0x0);
		outp32(spr_base + SPR_PATTERNGEN_POSITION1, (pinfo->yres << 12) | 0x0);
		outp32(spr_base + SPR_PATTERNGEN_PIX0, (0x3fc << 20) | (0x3fc << 10) | 0x3fc);
		outp32(spr_base + SPR_PATTERNGEN_PIX1, (0x200 << 20) | (0x200 << 10) | 0x200);
	}

	spr_core_init(spr_base, spr);
#ifdef SUPPORT_SPR_DSC1_2_SEPARATE
	spr_lut_config(spr_base, spr);
	if ((pinfo->spr_dsc_mode == SPR_DSC_MODE_SPR_AND_DSC) || (pinfo->spr_dsc_mode == SPR_DSC_MODE_DSC_ONLY))
		spr_dsc12_separate_init(spr_base, spr);
#else
	spr_lut_config(spr_base, spr);
#endif
	DPU_FB_INFO("-\n");
}

void spr_dsc_partial_updt_config(struct dpu_fb_data_type *dpufd, spr_dirty_region *spr_dirty)
{
	uint32_t overlap_value;
	struct dpu_panel_info *pinfo = NULL;
	struct spr_dsc_panel_para *spr = NULL;

	if((dpufd == NULL) || (spr_dirty == NULL))
		return;

	if ((dpufd->panel_info.spr_dsc_mode == SPR_DSC_MODE_NONE) ||
		(dpufd->panel_info.spr.spr_en == 0))
		return;

	pinfo = &(dpufd->panel_info);
	spr = &(pinfo->spr);

	/* The follow code from chip protocol, It contains lots of fixed numbers */
	switch (spr_dirty->spr_overlap_type) {
	case SPR_OVERLAP_TOP:
		overlap_value = 1;
		dpufd->set_reg(dpufd, dpufd->dss_base + SPR_OFFSET + SPR_BORDERLR_REG + 0x4,
			((spr->spr_bordertb_detect_r & 0xFFFF0000) | 0x80), 32, 0);
		dpufd->set_reg(dpufd, dpufd->dss_base + SPR_OFFSET + SPR_BORDERLR_REG + 0xC,
			((spr->spr_bordertb_detect_g & 0xFFFF0000) | 0x80), 32, 0);
		dpufd->set_reg(dpufd, dpufd->dss_base + SPR_OFFSET + SPR_BORDERLR_REG + 0x14,
			((spr->spr_bordertb_detect_b & 0xFFFF0000) | 0x80), 32, 0);
		break;
	case SPR_OVERLAP_BOTTOM:
		overlap_value = 2;
		dpufd->set_reg(dpufd, dpufd->dss_base + SPR_OFFSET + SPR_BORDERLR_REG + 0x4,
			((spr->spr_bordertb_detect_r & 0xFFFF) | (0x80 << 16)), 32, 0);
		dpufd->set_reg(dpufd, dpufd->dss_base + SPR_OFFSET + SPR_BORDERLR_REG + 0xC,
			((spr->spr_bordertb_detect_g & 0xFFFF) | (0x80 << 16)), 32, 0);
		dpufd->set_reg(dpufd, dpufd->dss_base + SPR_OFFSET + SPR_BORDERLR_REG + 0x14,
			((spr->spr_bordertb_detect_b & 0xFFFF) | (0x80 << 16)), 32, 0);
		break;
	case SPR_OVERLAP_TOP_BOTTOM:
		overlap_value = 3;
		dpufd->set_reg(dpufd, dpufd->dss_base + SPR_OFFSET + SPR_BORDERLR_REG + 0x4,
			SPR_CLOSE_BOARDER_GAIN, 32, 0);
		dpufd->set_reg(dpufd, dpufd->dss_base + SPR_OFFSET + SPR_BORDERLR_REG + 0xC,
			SPR_CLOSE_BOARDER_GAIN, 32, 0);
		dpufd->set_reg(dpufd, dpufd->dss_base + SPR_OFFSET + SPR_BORDERLR_REG + 0x14,
			SPR_CLOSE_BOARDER_GAIN, 32, 0);
		break;
	default:
		overlap_value = 0;
		dpufd->set_reg(dpufd, dpufd->dss_base + SPR_OFFSET + SPR_BORDERLR_REG + 0x4,
			spr->spr_bordertb_detect_r, 32, 0);
		dpufd->set_reg(dpufd, dpufd->dss_base + SPR_OFFSET + SPR_BORDERLR_REG + 0xC,
			spr->spr_bordertb_detect_g, 32, 0);
		dpufd->set_reg(dpufd, dpufd->dss_base + SPR_OFFSET + SPR_BORDERLR_REG + 0x14,
			spr->spr_bordertb_detect_b, 32, 0);
		spr_dirty->spr_img_size = spr->input_reso;
		break;
	}

	DPU_FB_DEBUG("spr_overlap_type = %d\n spr reso:%u", spr_dirty->spr_overlap_type, spr_dirty->spr_img_size);

	dpufd->set_reg(dpufd, dpufd->dss_base + SPR_OFFSET + SPR_CTRL, overlap_value, 2, 15);
	dpufd->set_reg(dpufd, dpufd->dss_base + SPR_OFFSET + SPR_RESO, spr_dirty->spr_img_size, 32, 0);

	return;
}

