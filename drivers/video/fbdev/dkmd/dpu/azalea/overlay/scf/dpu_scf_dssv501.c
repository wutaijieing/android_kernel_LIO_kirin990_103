/* Copyright (c) 2013-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
	s_post_scf->dbuf_frm_size = (uint32_t)inp32(dss_base + DSS_DBUF0_OFFSET + DBUF_FRM_SIZE);
	s_post_scf->dbuf_frm_hsize = (uint32_t)inp32(dss_base + DSS_DBUF0_OFFSET + DBUF_FRM_HSIZE);
	s_post_scf->dpp_img_size_bef_sr = (uint32_t)inp32(dss_base + DSS_DPP_OFFSET + DPP_IMG_SIZE_BEF_SR);
	s_post_scf->dpp_img_size_aft_sr = (uint32_t)inp32(dss_base + DSS_DPP_OFFSET + DPP_IMG_SIZE_AFT_SR);

	s_post_scf->ihleft = (uint32_t)inp32(post_scf_base + ARSR_POST_IHLEFT);
	s_post_scf->ihright = (uint32_t)inp32(post_scf_base + ARSR_POST_IHRIGHT);
	s_post_scf->ihleft1 = (uint32_t)inp32(post_scf_base + ARSR_POST_IHLEFT1);
	s_post_scf->ihright1 = (uint32_t)inp32(post_scf_base + ARSR_POST_IHRIGHT1);
	s_post_scf->ivtop = (uint32_t)inp32(post_scf_base + ARSR_POST_IVTOP);
	s_post_scf->ivbottom = (uint32_t)inp32(post_scf_base + ARSR_POST_IVBOTTOM);
	s_post_scf->uv_offset = (uint32_t)inp32(post_scf_base + ARSR_POST_UV_OFFSET);
	s_post_scf->ihinc = (uint32_t)inp32(post_scf_base + ARSR_POST_IHINC);
	s_post_scf->ivinc = (uint32_t)inp32(post_scf_base + ARSR_POST_IVINC);
	s_post_scf->mode = (uint32_t)inp32(post_scf_base + ARSR_POST_MODE);
	s_post_scf->format = (uint32_t)inp32(post_scf_base + ARSR_POST_FORMAT);
}

static void dpu_post_scf_effect_init(dss_arsr1p_t *s_post_scf)
{
	s_post_scf->skin_thres_y = (332 << 10) | 300;
	s_post_scf->skin_thres_u = (40 << 10) | 20;
	s_post_scf->skin_thres_v = (48 << 10) | 24;
	s_post_scf->skin_expected = (580 << 20) | (452 << 10) | 600;
	s_post_scf->skin_cfg = (12 << 16) | (10 << 8) | 6;
	s_post_scf->shoot_cfg1 = (8 << 16) | 20;
	s_post_scf->shoot_cfg2 = (-80 & 0x7ff) | (8 << 16);
	s_post_scf->shoot_cfg3 = 372;
	s_post_scf->sharp_cfg1_h = (256 << 16) | 192;  /* 980 no used */
	s_post_scf->sharp_cfg1_l = (24 << 16) | 8;  /* 980 no used */
	s_post_scf->sharp_cfg2_h = (256 << 16) | 192;
	s_post_scf->sharp_cfg2_l = (32 << 16) | 16;
	s_post_scf->sharp_cfg3 = (150 << 16) | 150;  /* 980 no used */
	s_post_scf->sharp_cfg4 = (200 << 16) | 0;  /* 980 no used */
	s_post_scf->sharp_cfg5 = (200 << 16) | 0;  /* 980 no used */
	s_post_scf->sharp_cfg6 = (160 << 16) | 40;
	s_post_scf->sharp_cfg6_cut = (192 << 16) | 128;
	s_post_scf->sharp_cfg7 = (1 << 17) | 8;
	s_post_scf->sharp_cfg7_ratio = (160 << 16) | 16;
	s_post_scf->sharp_cfg8 = (3 << 22) | 800;  /* 980 no used */
	s_post_scf->sharp_cfg9 = (8 << 22) | 12800;  /* 980 no used */
	s_post_scf->sharp_cfg10 = 800;  /* 980 no used */
	s_post_scf->sharp_cfg11 = (15  <<  22) | 12800;
	s_post_scf->diff_ctrl = (80 << 16) | 64;
	s_post_scf->skin_slop_y = 512;
	s_post_scf->skin_slop_u = 819;
	s_post_scf->skin_slop_v = 682;
	s_post_scf->force_clk_on_cfg = 0;

	s_post_scf->sharp_level = 0x20002;
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

static int post_scf_rect_set(const struct dpu_fb_data_type *dpufd, struct dpu_panel_info *pinfo,
	const dss_overlay_t *pov_req, dss_rect_t *src_rect, dss_rect_t *dst_rect)
{
	if (pov_req) {
		if ((pov_req->res_updt_rect.w <= 0) || (pov_req->res_updt_rect.h <= 0)) {
			DPU_FB_DEBUG("fb%d, res_updt_rect[%d,%d, %d,%d] is invalid!\n", dpufd->index,
				pov_req->res_updt_rect.x, pov_req->res_updt_rect.y,
				pov_req->res_updt_rect.w, pov_req->res_updt_rect.h);
			return  0;
		}

		if ((pov_req->res_updt_rect.w == dpufd->ov_req_prev.res_updt_rect.w) &&
			(pov_req->res_updt_rect.h == dpufd->ov_req_prev.res_updt_rect.h)) {
			return 0;
		}
		*src_rect = pov_req->res_updt_rect;

		DPU_FB_DEBUG("fb%d, post scf res_updt_rect[%d, %d]->lcd_rect[%d, %d]\n",
			dpufd->index,
			pov_req->res_updt_rect.w, pov_req->res_updt_rect.h,
			pinfo->xres, pinfo->yres);
	} else {
		src_rect->x = 0;
		src_rect->y = 0;
		src_rect->w = pinfo->xres;
		src_rect->h = pinfo->yres;
	}

	if (dpu_check_panel_feature_support(pinfo) && pov_req &&
		(pov_req->rog_width > 0 && pov_req->rog_height > 0)) {
		/* for foldable display ROG partial update */
		dst_rect->x = pov_req->res_updt_rect.x;
		dst_rect->y = pov_req->res_updt_rect.y;
		dst_rect->w = pov_req->res_updt_rect.w;
		dst_rect->h = pov_req->res_updt_rect.h;
	} else {
		dst_rect->x = 0;
		dst_rect->y = 0;
		dst_rect->w = pinfo->xres;
		dst_rect->h = pinfo->yres;
	}

	return 1;
}

static void post_scf_img_size_config(dss_arsr1p_t *post_scf, dss_rect_t src_rect, dss_rect_t dst_rect)
{
	post_scf->dbuf_frm_size = set_bits32(post_scf->dbuf_frm_size, src_rect.w * src_rect.h, 27, 0);
	post_scf->dbuf_frm_hsize = set_bits32(post_scf->dbuf_frm_hsize, DSS_WIDTH(src_rect.w), 13, 0);
	post_scf->dbuf_used = 1;

	post_scf->dpp_img_size_bef_sr = set_bits32(post_scf->dpp_img_size_bef_sr,
		(DSS_HEIGHT((uint32_t)src_rect.h) << 16) | DSS_WIDTH((uint32_t)src_rect.w), 32, 0);
	post_scf->dpp_img_size_aft_sr = set_bits32(post_scf->dpp_img_size_aft_sr,
		(DSS_HEIGHT((uint32_t)dst_rect.h) << 16) | DSS_WIDTH((uint32_t)dst_rect.w), 32, 0);
}

static int post_scf_mode_config(const struct dpu_panel_info *pinfo, dss_arsr1p_t *post_scf,
	int32_t ihinc, int32_t ivinc)
{
	if ((ihinc == ARSR1P_INC_FACTOR) &&
		(ivinc == ARSR1P_INC_FACTOR) &&
		(pinfo->arsr1p_sharpness_support != 1)) {
		/* ARSR1P bypass */
		post_scf->mode = 0x1;
		return 0;
	}

	/* 0x2000<=ihinc<=0x80000; 0x2000<=ivinc<=0x80000; */
	if ((ihinc < 0x2000) || (ihinc > ARSR1P_INC_FACTOR) ||
		(ivinc < 0x2000) || (ivinc > ARSR1P_INC_FACTOR)) {
		DPU_FB_ERR("invalid ihinc(0x%x), ivinc(0x%x)!\n", ihinc, ivinc);
		/* ARSR1P bypass */
		post_scf->mode = 0x1;
		return -1;
	}

	if ((ihinc > ARSR1P_INC_FACTOR) ||  (ivinc > ARSR1P_INC_FACTOR)) {
		/* scaler down, not supported */
		DPU_FB_ERR("scaling down is not supported by ARSR1P, ihinc = 0x%x, ivinc = 0x%x\n", ihinc, ivinc);
		/* ARSR1P bypass */
		post_scf->mode = 0x1;
		return -1;
	}

	/* enable arsr1p */
	post_scf->mode = 0x0;

	if (pinfo->arsr1p_sharpness_support)
		/* enable sharp  skinctrl, shootdetect */
		post_scf->mode |= 0xe;

	/* enable direction */
	post_scf->mode |= 0x20;

	if ((ihinc < ARSR1P_INC_FACTOR) ||  (ivinc < ARSR1P_INC_FACTOR))
		/* enable diintplen */
		post_scf->mode |= 0x10;
	else
		/* only sharp, enable nointplen */
		post_scf->mode |= 0x40;

	return 1;
}

static int post_scf_cordinate_config(dss_arsr1p_t *post_scf, dss_rect_t src_rect, dss_rect_t dst_rect,
	int32_t ihinc, int32_t ivinc)
{
	int32_t ihleft;
	int32_t ihright;
	int32_t ihleft1;
	int32_t ihright1;
	int32_t ivtop;
	int32_t ivbottom;
	int32_t extraw;
	int32_t extraw_left;
	int32_t extraw_right;

	dpu_check_and_return((ihinc == 0), -1, ERR, "ihinc is zero\n");

	extraw = (8 * ARSR1P_INC_FACTOR) / ihinc;
	extraw_left = (extraw % 2) ? (extraw + 1) : (extraw);
	extraw = (2 * ARSR1P_INC_FACTOR) / ihinc;
	extraw_right = (extraw % 2) ? (extraw + 1) : (extraw);

	/* ihleft1 = (startX_o * ihinc) - (ov_startX0 << 16) */
	ihleft1 = dst_rect.x * ihinc - src_rect.x * ARSR1P_INC_FACTOR;
	if (ihleft1 < 0)
		ihleft1 = 0;
	/* ihleft = ihleft1 - even(8 * 65536 / ihinc) * ihinc; */
	ihleft = ihleft1 - extraw_left * ihinc;
	if (ihleft < 0)
		ihleft = 0;
	/* ihright1 = ihleft1 + (oww-1) * ihinc */
	ihright1 = ihleft1 + (dst_rect.w - 1) * ihinc;
	/* ihright = ihright1 + even(2 * 65536/ihinc) * ihinc */
	ihright = ihright1 + extraw_right * ihinc;
	/* ihright >= img_width * ihinc */
	if (ihright >= src_rect.w * ARSR1P_INC_FACTOR)
		ihright = src_rect.w * ARSR1P_INC_FACTOR - 1;
	/* ivtop = (startY_o * ivinc) - (ov_startY0<<16) */
	ivtop = dst_rect.y * ivinc - src_rect.y * ARSR1P_INC_FACTOR;
	if (ivtop < 0)
		ivtop = 0;
	/* ivbottom = ivtop + (ohh - 1) * ivinc */
	ivbottom = ivtop + (dst_rect.h - 1) * ivinc;
	/* ivbottom >= img_height * ivinc */
	if (ivbottom >= src_rect.h * ARSR1P_INC_FACTOR)
		ivbottom = src_rect.h * ARSR1P_INC_FACTOR - 1;
	/* (ihleft1 - ihleft) % (ihinc) == 0; */
	if ((ihleft1 - ihleft) % (ihinc)) {
		DPU_FB_ERR("(ihleft1[%d]-ihleft[%d]) ihinc[%d] != 0, invalid!\n", ihleft1, ihleft, ihinc);
		post_scf->mode = 0x1;
		return -1;
	}

	/* (ihright1 - ihleft1) % ihinc == 0; */
	if ((ihright1 - ihleft1) % ihinc) {
		DPU_FB_ERR("(ihright1[%d]-ihleft1[%d]) ihinc[%d] != 0, invalid!\n", ihright1, ihleft1, ihinc);
		post_scf->mode = 0x1;
		return -1;
	}

	post_scf->ihleft = set_bits32(post_scf->ihleft, ihleft, 32, 0);
	post_scf->ihright = set_bits32(post_scf->ihright, ihright, 32, 0);
	post_scf->ihleft1 = set_bits32(post_scf->ihleft1, ihleft1, 32, 0);
	post_scf->ihright1 = set_bits32(post_scf->ihright1, ihright1, 32, 0);
	post_scf->ivtop = set_bits32(post_scf->ivtop, ivtop, 32, 0);
	post_scf->ivbottom = set_bits32(post_scf->ivbottom, ivbottom, 32, 0);

	return 1;
}

int dpu_post_scf_config(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req)
{
	struct dpu_panel_info *pinfo = NULL;
	dss_rect_t src_rect = {0};
	dss_rect_t dst_rect = {0};
	dss_arsr1p_t *post_scf = NULL;
	int ret;
	int32_t ihinc;
	int32_t ivinc;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return -EINVAL;
	}
	pinfo = &(dpufd->panel_info);

	if (!DPU_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_POST_SCF))
		return 0;

	if (dpufd->index != PRIMARY_PANEL_IDX)
		return 0;

	ret = post_scf_rect_set(dpufd, pinfo, pov_req, &src_rect, &dst_rect);
	if (ret <= 0)
		return ret;

	post_scf = &(dpufd->dss_module.post_scf);
	dpufd->dss_module.post_scf_used = 1;

	post_scf_img_size_config(post_scf, src_rect, dst_rect);
	post_scf->dpp_used = 1;

	/* 16: ARSR_POST_SRC_MIN_WIDTH
	 * 16: ARSR_POST_SRC_MIN_HEIGHT
	 * 3840: ARSR_POST_SRC_MAX_WIDTH
	 * 8192: ARSR_POST_SRC_MAX_HEIGHT
	 * 8192: ARSR_POST_DST_MAX_WIDTH
	 * 8192: ARSR_POST_DST_MAX_HEIGHT
	 */
	if ((src_rect.w < 16) || (src_rect.h < 16) ||
		(src_rect.w > 3840) || (src_rect.h > 8192) ||
		(dst_rect.w > 8192) || (dst_rect.h > 8192)) {
		DPU_FB_ERR("invalid input size: src_rect[%d,%d,%d,%d] should be larger than 16*16, "
			"less than 3840*8192!\n"
			"invalid output size: dst_rect[%d,%d,%d,%d] should be less than 8192*8192!\n",
			src_rect.x, src_rect.y, src_rect.w, src_rect.h,
			dst_rect.x, dst_rect.y, dst_rect.w, dst_rect.h);
		/* ARSR1P bypass */
		post_scf->mode = 0x1;
		return 0;
	}

	ihinc = ARSR1P_INC_FACTOR * src_rect.w / dst_rect.w;
	ivinc = ARSR1P_INC_FACTOR * src_rect.h / dst_rect.h;

	ret = post_scf_mode_config(pinfo, post_scf, ihinc, ivinc);
	if (ret <= 0)
		return ret;

	ret = post_scf_cordinate_config(post_scf, src_rect, dst_rect, ihinc, ivinc);
	if (ret <= 0)
		return ret;

	post_scf->ihinc = set_bits32(post_scf->ihinc, ihinc, 32, 0);
	post_scf->ivinc = set_bits32(post_scf->ivinc, ivinc, 32, 0);

	return 0;
}

static void post_scf_basic_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *post_scf_base, const dss_arsr1p_t *s_post_scf)
{
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_IHLEFT, s_post_scf->ihleft, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_IHRIGHT, s_post_scf->ihright, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_IHLEFT1, s_post_scf->ihleft1, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_IHRIGHT1, s_post_scf->ihright1, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_IVTOP, s_post_scf->ivtop, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_IVBOTTOM, s_post_scf->ivbottom, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_UV_OFFSET, s_post_scf->uv_offset, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_IHINC, s_post_scf->ihinc, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_IVINC, s_post_scf->ivinc, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_MODE, s_post_scf->mode, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_FORMAT, s_post_scf->format, 32, 0);
}

static void post_scf_func_param_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *post_scf_base, const dss_arsr1p_t *s_post_scf)
{
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SKIN_THRES_Y, s_post_scf->skin_thres_y, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SKIN_THRES_U, s_post_scf->skin_thres_u, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SKIN_THRES_V, s_post_scf->skin_thres_v, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SKIN_EXPECTED, s_post_scf->skin_expected, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SKIN_CFG, s_post_scf->skin_cfg, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHOOT_CFG1, s_post_scf->shoot_cfg1, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHOOT_CFG2, s_post_scf->shoot_cfg2, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHOOT_CFG3, s_post_scf->shoot_cfg3, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_CFG1_H, s_post_scf->sharp_cfg1_h, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_CFG1_L, s_post_scf->sharp_cfg1_l, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_CFG2_H, s_post_scf->sharp_cfg2_h, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_CFG2_L, s_post_scf->sharp_cfg2_l, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_CFG3, s_post_scf->sharp_cfg3, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_CFG4, s_post_scf->sharp_cfg4, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_CFG5, s_post_scf->sharp_cfg5, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_CFG6, s_post_scf->sharp_cfg6, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_CFG6_CUT, s_post_scf->sharp_cfg6_cut, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_CFG7, s_post_scf->sharp_cfg7, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_CFG7_RATIO, s_post_scf->sharp_cfg7_ratio, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_CFG8, s_post_scf->sharp_cfg8, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_CFG9, s_post_scf->sharp_cfg9, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_CFG10, s_post_scf->sharp_cfg10, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_CFG11, s_post_scf->sharp_cfg11, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_DIFF_CTRL, s_post_scf->diff_ctrl, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SKIN_SLOP_Y, s_post_scf->skin_slop_y, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SKIN_SLOP_U, s_post_scf->skin_slop_u, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SKIN_SLOP_V, s_post_scf->skin_slop_v, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_FORCE_CLK_ON_CFG, s_post_scf->force_clk_on_cfg, 32, 0);
}

static void post_scf_sharp_control_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *post_scf_base, const dss_arsr1p_t *s_post_scf)
{
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_LEVEL, s_post_scf->sharp_level, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_GAIN_LOW, s_post_scf->sharp_gain_low, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_GAIN_MID, s_post_scf->sharp_gain_mid, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_GAIN_HIGH, s_post_scf->sharp_gain_high, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_GAINCTRLSLOPH_MF,
		s_post_scf->sharp_gainctrl_sloph_mf, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_GAINCTRLSLOPL_MF,
		s_post_scf->sharp_gainctrl_slopl_mf, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_GAINCTRLSLOPH_HF,
		s_post_scf->sharp_gainctrl_sloph_hf, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_GAINCTRLSLOPL_HF,
		s_post_scf->sharp_gainctrl_slopl_hf, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_MF_LMT, s_post_scf->sharp_mf_lmt, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_GAIN_MF, s_post_scf->sharp_gain_mf, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_MF_B, s_post_scf->sharp_mf_b, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_HF_LMT, s_post_scf->sharp_hf_lmt, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_GAIN_HF, s_post_scf->sharp_gain_hf, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_HF_B, s_post_scf->sharp_hf_b, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_LF_CTRL, s_post_scf->sharp_lf_ctrl, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_LF_VAR, s_post_scf->sharp_lf_var, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_LF_CTRL_SLOP, s_post_scf->sharp_lf_ctrl_slop, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + ARSR_POST_SHARP_HF_SELECT, s_post_scf->sharp_hf_select, 32, 0);
}

void dpu_post_scf_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *post_scf_base, dss_arsr1p_t *s_post_scf)
{
	if (!dpufd || !post_scf_base || !s_post_scf) {
		DPU_FB_ERR("NULL ptr.\n");
		return;
	}

	if (s_post_scf->dbuf_used == 1) {
		dpufd->set_reg(dpufd, dpufd->dss_base + DSS_DBUF0_OFFSET + DBUF_FRM_SIZE,
			s_post_scf->dbuf_frm_size, 32, 0);
		dpufd->set_reg(dpufd, dpufd->dss_base + DSS_DBUF0_OFFSET + DBUF_FRM_HSIZE,
			s_post_scf->dbuf_frm_hsize, 32, 0);
	}
	if (s_post_scf->dpp_used == 1) {
		dpufd->set_reg(dpufd, dpufd->dss_base + DSS_DPP_OFFSET + DPP_IMG_SIZE_BEF_SR,
			s_post_scf->dpp_img_size_bef_sr, 32, 0);
		dpufd->set_reg(dpufd, dpufd->dss_base + DSS_DPP_OFFSET + DPP_IMG_SIZE_AFT_SR,
			s_post_scf->dpp_img_size_aft_sr, 32, 0);
	}

	post_scf_basic_set_reg(dpufd, post_scf_base, s_post_scf);

	post_scf_func_param_set_reg(dpufd, post_scf_base, s_post_scf);

	post_scf_sharp_control_set_reg(dpufd, post_scf_base, s_post_scf);
}

