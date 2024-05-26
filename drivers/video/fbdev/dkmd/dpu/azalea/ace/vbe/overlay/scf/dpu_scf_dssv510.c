/* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#include "../dpu_overlay_utils.h"
#include "../../dpu_mmbuf_manager.h"

static void dpu_post_scf_scale_init(const char __iomem *dss_base,
	const char __iomem *post_scf_base, dss_arsr1p_t *s_post_scf)
{
	s_post_scf->dpp_img_size_bef_sr = (uint32_t)inp32(dss_base + DSS_DISP_CH0_OFFSET + IMG_SIZE_BEF_SR);
	s_post_scf->dpp_img_size_aft_sr = (uint32_t)inp32(dss_base + DSS_DISP_CH0_OFFSET + IMG_SIZE_AFT_SR);

	s_post_scf->ihleft = (uint32_t)inp32(post_scf_base + ARSR_POST_IHLEFT);
	s_post_scf->ihleft1 = (uint32_t)inp32(post_scf_base + ARSR_POST_IHLEFT1);
	s_post_scf->ihright = (uint32_t)inp32(post_scf_base + ARSR_POST_IHRIGHT);
	s_post_scf->ihright1 = (uint32_t)inp32(post_scf_base + ARSR_POST_IHRIGHT1);
	s_post_scf->ivtop = (uint32_t)inp32(post_scf_base + ARSR_POST_IVTOP);
	s_post_scf->ivbottom = (uint32_t)inp32(post_scf_base + ARSR_POST_IVBOTTOM);
	s_post_scf->ivbottom1 = (uint32_t)inp32(post_scf_base + ARSR_POST_IVBOTTOM1);
	s_post_scf->ihinc = (uint32_t)inp32(post_scf_base + ARSR_POST_IHINC);
	s_post_scf->ivinc = (uint32_t)inp32(post_scf_base + ARSR_POST_IVINC);
	s_post_scf->uv_offset = (uint32_t)inp32(post_scf_base + ARSR_POST_UV_OFFSET);
	s_post_scf->mode = (uint32_t)inp32(post_scf_base + ARSR_POST_MODE);
}

static void dpu_post_scf_effect_init(dss_arsr1p_t *s_post_scf)
{
	/* The follow code from chip protocol, It contains lots of fixed numbers */
	s_post_scf->skin_thres_y = (600 << 20) | (332 << 10) | 300;  /* 0x2585312C */
	s_post_scf->skin_thres_u = (452 << 20) | (40 << 10) | 20;  /* 0x1C40A014 */
	s_post_scf->skin_thres_v = (580 << 20) | (48 << 10) | 24;  /* 0x2440C018 */
	s_post_scf->skin_cfg0 = (12 << 13) | 512;  /* 0x00018200 */
	s_post_scf->skin_cfg1 = 819;  /* 0x00000333 */
	s_post_scf->skin_cfg2 = 682;  /* 0x000002AA */
	s_post_scf->shoot_cfg1 = (20 << 16) | 341;  /* 0x00140155 */
	s_post_scf->shoot_cfg2 = (-80 & 0x7ff) | (16 << 16);  /* 0x001007B0 */
	s_post_scf->shoot_cfg3 = 20;  /* 0x00000014 */
	s_post_scf->sharp_cfg3 = (0xA0 << 16) | 0x60;  /* 0x00A00060 */
	s_post_scf->sharp_cfg4 = (0x60 << 16) | 0x20;  /* 0x00600020 */
	s_post_scf->sharp_cfg5 = 0;
	s_post_scf->sharp_cfg6 = (0x4 << 16) | 0x8;  /* 0x00040008 */
	s_post_scf->sharp_cfg7 = (6 << 8) | 10;  /* 0x0000060A */
	s_post_scf->sharp_cfg8 = (0xA0 << 16) | 0x10;  /* 0x00A00010 */

	s_post_scf->sharp_level = 0x0020002;
	s_post_scf->sharp_gain_low = 0x3C0078;
	s_post_scf->sharp_gain_mid = 0x6400C8;
	s_post_scf->sharp_gain_high = 0x5000A0;
	s_post_scf->sharp_gainctrl_sloph_mf = 0x280;
	s_post_scf->sharp_gainctrl_slopl_mf = 0x1400;
	s_post_scf->sharp_gainctrl_sloph_hf = 0x140;
	s_post_scf->sharp_gainctrl_slopl_hf = 0xA00;
	s_post_scf->sharp_mf_lmt = 0x40;
	s_post_scf->sharp_gain_mf = 0x12C012C;
	s_post_scf->sharp_mf_b = 0;
	s_post_scf->sharp_hf_lmt = 0x80;
	s_post_scf->sharp_gain_hf = 0x104012C;
	s_post_scf->sharp_hf_b = 0x1400;
	s_post_scf->sharp_lf_ctrl = 0x100010;
	s_post_scf->sharp_lf_var = 0x1800080;
	s_post_scf->sharp_lf_ctrl_slop = 0;
	s_post_scf->sharp_hf_select = 0;
	s_post_scf->sharp_cfg2_h = 0x10000C0;
	s_post_scf->sharp_cfg2_l = 0x200010;
	s_post_scf->texture_analysis = 0x500040;
	s_post_scf->intplshootctrl = 0x8;
}

void dpu_post_scf_init(const char __iomem *dss_base,
	const char __iomem *post_scf_base, dss_arsr1p_t *s_post_scf)
{
	dpu_check_and_no_retval(!post_scf_base, ERR, "post_scf_base is NULL!\n");
	dpu_check_and_no_retval(!s_post_scf, ERR, "s_post_scf is NULL!\n");
	dpu_check_and_no_retval(!dss_base, ERR, "dss_base is NULL!\n");

	memset(s_post_scf, 0, sizeof(dss_arsr1p_t));

	dpu_post_scf_scale_init(dss_base, post_scf_base, s_post_scf);
	dpu_post_scf_effect_init(s_post_scf);
}

static void post_scf_basic_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *post_scf_base, dss_arsr1p_t *s_post_scf)
{
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_IHLEFT,
		s_post_scf->ihleft, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_IHRIGHT,
		s_post_scf->ihright, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_IHLEFT1,
		s_post_scf->ihleft1, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_IHRIGHT1,
		s_post_scf->ihright1, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_IVTOP,
		s_post_scf->ivtop, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_IVBOTTOM,
		s_post_scf->ivbottom, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_IVBOTTOM1,
		s_post_scf->ivbottom1, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_UV_OFFSET,
		s_post_scf->uv_offset, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_IHINC,
		s_post_scf->ihinc, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_IVINC,
		s_post_scf->ivinc, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_MODE,
		s_post_scf->mode, 32, 0);
}

static void post_scf_func_param_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *post_scf_base, dss_arsr1p_t *s_post_scf)
{
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SKIN_THRES_Y,
		s_post_scf->skin_thres_y, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SKIN_THRES_U,
		s_post_scf->skin_thres_u, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SKIN_THRES_V,
		s_post_scf->skin_thres_v, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SKIN_CFG0,
		s_post_scf->skin_cfg0, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SKIN_CFG1,
		s_post_scf->skin_cfg1, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SKIN_CFG2,
		s_post_scf->skin_cfg2, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHOOT_CFG1,
		s_post_scf->shoot_cfg1, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHOOT_CFG2,
		s_post_scf->shoot_cfg2, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHOOT_CFG3,
		s_post_scf->shoot_cfg3, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_CFG3,
		s_post_scf->sharp_cfg3, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_CFG4,
		s_post_scf->sharp_cfg4, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_CFG5,
		s_post_scf->sharp_cfg5, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_CFG6,
		s_post_scf->sharp_cfg6, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_CFG7,
		s_post_scf->sharp_cfg7, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_CFG8,
		s_post_scf->sharp_cfg8, 32, 0);
}

static void post_scf_sharp_control_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *post_scf_base, dss_arsr1p_t *s_post_scf)
{
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_LEVEL,
		s_post_scf->sharp_level, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_GAIN_LOW,
		s_post_scf->sharp_gain_low, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_GAIN_MID,
		s_post_scf->sharp_gain_mid, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_GAIN_HIGH,
		s_post_scf->sharp_gain_high, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_GAINCTRLSLOPH_MF,
		s_post_scf->sharp_gainctrl_sloph_mf, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_GAINCTRLSLOPL_MF,
		s_post_scf->sharp_gainctrl_slopl_mf, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_GAINCTRLSLOPH_HF,
		s_post_scf->sharp_gainctrl_sloph_hf, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_GAINCTRLSLOPL_HF,
		s_post_scf->sharp_gainctrl_slopl_hf, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_MF_LMT,
		s_post_scf->sharp_mf_lmt, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_GAIN_MF,
		s_post_scf->sharp_gain_mf, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_MF_B,
		s_post_scf->sharp_mf_b, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_HF_LMT,
		s_post_scf->sharp_hf_lmt, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_GAIN_HF,
		s_post_scf->sharp_gain_hf, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_HF_B,
		s_post_scf->sharp_hf_b, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_LF_CTRL,
		s_post_scf->sharp_lf_ctrl, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_LF_VAR,
		s_post_scf->sharp_lf_var, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_LF_CTRL_SLOP,
		s_post_scf->sharp_lf_ctrl_slop, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_HF_SELECT,
		s_post_scf->sharp_hf_select, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_CFG2_H,
		s_post_scf->sharp_cfg2_h, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_CFG2_L,
		s_post_scf->sharp_cfg2_l, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_TEXTURE_ANALYSIS,
		s_post_scf->texture_analysis, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_INTPLSHOOTCTRL,
		s_post_scf->intplshootctrl, 32, 0);
}

void dpu_post_scf_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *post_scf_base, dss_arsr1p_t *s_post_scf)
{
	uint32_t img_size_bef_sr;

	if (!dpufd || !post_scf_base || !s_post_scf) {
		DPU_FB_ERR("NULL ptr.\n");
		return;
	}

	img_size_bef_sr = s_post_scf->dpp_img_size_bef_sr;

	if (s_post_scf->dpp_used == 1) {
		dpufd->set_reg(dpufd,
			dpufd->dss_base + DSS_DISP_CH0_OFFSET + IMG_SIZE_BEF_SR,
			img_size_bef_sr, 32, 0);
		dpufd->set_reg(dpufd,
			dpufd->dss_base + DSS_DISP_CH0_OFFSET + IMG_SIZE_AFT_SR,
			s_post_scf->dpp_img_size_aft_sr, 32, 0);
	}

	post_scf_basic_set_reg(dpufd, post_scf_base, s_post_scf);

	post_scf_func_param_set_reg(dpufd, post_scf_base, s_post_scf);

	post_scf_sharp_control_set_reg(dpufd, post_scf_base, s_post_scf);
}

