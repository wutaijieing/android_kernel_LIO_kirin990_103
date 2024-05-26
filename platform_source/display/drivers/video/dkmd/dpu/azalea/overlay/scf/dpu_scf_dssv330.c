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

#if defined(CONFIG_DPU_FB_V320)
uint32_t g_rog_config_scf_cnt;
#endif

void dpu_post_scf_init(char __iomem *dss_base,
	const char __iomem *post_scf_base, dss_scl_t *s_post_scf)
{
	if (!dss_base) {
		DPU_FB_ERR("dss_base is NULL\n");
		return;
	}
	if (!post_scf_base) {
		DPU_FB_ERR("post_scf_base is NULL\n");
		return;
	}
	if (!s_post_scf) {
		DPU_FB_ERR("s_post_scf is NULL\n");
		return;
	}

	memset(s_post_scf, 0, sizeof(dss_scl_t));
}

void dpu_post_scf_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *post_scf_base, dss_scl_t *s_post_scf)
{
	if (!dpufd) {
		DPU_FB_DEBUG("dpufd is NULL!\n");
		return;
	}

	if (!post_scf_base) {
		DPU_FB_DEBUG("post_scf_base is NULL!\n");
		return;
	}

	if (!s_post_scf) {
		DPU_FB_DEBUG("s_post_scf is NULL!\n");
		return;
	}

	dpufd->set_reg(dpufd, post_scf_base + SCF_EN_HSCL_STR, s_post_scf->en_hscl_str, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + SCF_EN_VSCL_STR, s_post_scf->en_vscl_str, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + SCF_H_V_ORDER, s_post_scf->h_v_order, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + SCF_INPUT_WIDTH_HEIGHT, s_post_scf->input_width_height, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + SCF_OUTPUT_WIDTH_HEIGHT, s_post_scf->output_width_height, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + SCF_EN_HSCL, s_post_scf->en_hscl, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + SCF_EN_VSCL, s_post_scf->en_vscl, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + SCF_ACC_HSCL, s_post_scf->acc_hscl, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + SCF_INC_HSCL, s_post_scf->inc_hscl, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + SCF_INC_VSCL, s_post_scf->inc_vscl, 32, 0);
	dpufd->set_reg(dpufd, post_scf_base + SCF_EN_MMP, s_post_scf->en_mmp, 32, 0);
}

static int post_scf_rect_set(const struct dpu_fb_data_type *dpufd, const struct dpu_panel_info *pinfo,
	const dss_overlay_t *pov_req, dss_rect_t *src_rect, dss_rect_t *dst_rect)
{
	if (pov_req) {
		if ((pov_req->res_updt_rect.h <= 0) || (pov_req->res_updt_rect.w <= 0)) {
			DPU_FB_DEBUG("fb%d, res_updt_rect[%d,%d, %d,%d] is invalid!\n", dpufd->index,
				pov_req->res_updt_rect.x, pov_req->res_updt_rect.y,
				pov_req->res_updt_rect.w, pov_req->res_updt_rect.h);
			return 0;
		}

		if ((pov_req->res_updt_rect.h == dpufd->ov_req_prev.res_updt_rect.h) &&
			(pov_req->res_updt_rect.w == dpufd->ov_req_prev.res_updt_rect.w)) {
#if defined(CONFIG_DPU_FB_V320)
			if (g_rog_config_scf_cnt > 0) {
				DPU_FB_DEBUG("flush same config 5 times! g_rog_config_scf_cnt = %d\n",
					g_rog_config_scf_cnt);
				g_rog_config_scf_cnt--;
			} else {
				return 0;
			}
		} else {
			g_rog_config_scf_cnt = 5;
#elif defined(CONFIG_DPU_FB_V330)
			return 0;
#endif
		}

		DPU_FB_DEBUG("fb%d, post_scf res_updt_rect[%d, %d]->lcd_rect[%d, %d]\n",
			dpufd->index,
			pov_req->res_updt_rect.w,
			pov_req->res_updt_rect.h,
			pinfo->xres, pinfo->yres);

		*src_rect = pov_req->res_updt_rect;
	} else {
		src_rect->y = 0;
		src_rect->x = 0;
		src_rect->w = pinfo->xres;
		src_rect->h = pinfo->yres;
	}

	dst_rect->y = 0;
	dst_rect->x = 0;
	dst_rect->w = pinfo->xres;
	dst_rect->h = pinfo->yres;

	return 1;
}

static int calc_h_ratio(dss_rect_t src_rect, dss_rect_t dst_rect, struct post_scf_config *scf_cfg)
{
	if ((src_rect.w == dst_rect.w) || (DSS_WIDTH(dst_rect.w) == 0))
		return 0;

	scf_cfg->en_hscl = true;

	scf_cfg->h_ratio = DSS_WIDTH(src_rect.w) * SCF_INC_FACTOR / DSS_WIDTH(dst_rect.w);
	if ((DSS_WIDTH(dst_rect.w) > (DSS_WIDTH(src_rect.w) * SCF_UPSCALE_MAX)) ||
		(DSS_WIDTH(src_rect.w) > (DSS_WIDTH(dst_rect.w) * SCF_DOWNSCALE_MAX))) {
		DPU_FB_ERR("width out of range, src_rect[%d, %d, %d, %d], dst_rect[%d, %d, %d, %d].\n",
			src_rect.x, src_rect.y, src_rect.w, src_rect.h,
			dst_rect.x, dst_rect.y, dst_rect.w, dst_rect.h);

		return -EINVAL;
	}

	return 1;
}

static int calc_v_ratio(dss_rect_t src_rect, dss_rect_t dst_rect, struct post_scf_config *scf_cfg)
{
	if ((src_rect.h == dst_rect.h) || (DSS_HEIGHT(dst_rect.h) == 0))
		return 0;

	scf_cfg->en_vscl = true;
	scf_cfg->scf_en_vscl = 1;

	scf_cfg->v_ratio = (DSS_HEIGHT(src_rect.h) * SCF_INC_FACTOR + SCF_INC_FACTOR / 2) / DSS_HEIGHT(dst_rect.h);
	if ((DSS_HEIGHT(dst_rect.h) > (DSS_HEIGHT(src_rect.h) * SCF_UPSCALE_MAX)) ||
		(DSS_HEIGHT(src_rect.h) > (DSS_HEIGHT(dst_rect.h) * SCF_DOWNSCALE_MAX))) {
		DPU_FB_ERR("height out of range, src_rect[%d, %d, %d, %d], dst_rect[%d, %d, %d, %d].\n",
			src_rect.x, src_rect.y, src_rect.w, src_rect.h,
			dst_rect.x, dst_rect.y, dst_rect.w, dst_rect.h);
		return -EINVAL;
	}

	return 1;
}

static void post_scf_cordinate_config(dss_scl_t *post_scf, dss_rect_t src_rect,
	dss_rect_t dst_rect, struct post_scf_config *scf_cfg)
{
	if (scf_cfg->en_hscl || scf_cfg->en_vscl) {
		/* scale down, do hscl first; scale up, do vscl first */
		scf_cfg->h_v_order = (src_rect.w > dst_rect.w) ? 0 : 1;

		post_scf->en_hscl_str = set_bits32(post_scf->en_hscl_str, 0x0, 1, 0);

		if (scf_cfg->v_ratio >= 2 * SCF_INC_FACTOR)
			post_scf->en_vscl_str = set_bits32(post_scf->en_vscl_str, 0x1, 1, 0);
		else
			post_scf->en_vscl_str = set_bits32(post_scf->en_vscl_str, 0x0, 1, 0);

		if (src_rect.h > dst_rect.h)
			scf_cfg->scf_en_vscl = 0x3;

		post_scf->h_v_order = set_bits32(post_scf->h_v_order, scf_cfg->h_v_order, 1, 0);
		post_scf->input_width_height = set_bits32(post_scf->input_width_height,
			DSS_HEIGHT(src_rect.h), 13, 0);
		post_scf->input_width_height = set_bits32(post_scf->input_width_height,
			DSS_WIDTH(src_rect.w), 13, 16);
		post_scf->output_width_height = set_bits32(post_scf->output_width_height,
			DSS_HEIGHT(dst_rect.h), 13, 0);
		post_scf->output_width_height = set_bits32(post_scf->output_width_height,
			DSS_WIDTH(dst_rect.w), 13, 16);

		post_scf->en_hscl = set_bits32(post_scf->en_hscl, (scf_cfg->en_hscl ? 0x1 : 0x0), 1, 0);
		post_scf->en_vscl = set_bits32(post_scf->en_vscl, scf_cfg->scf_en_vscl, 2, 0);
		post_scf->acc_hscl = set_bits32(post_scf->acc_hscl, 0x0, 31, 0);
		post_scf->inc_hscl = set_bits32(post_scf->inc_hscl, scf_cfg->h_ratio, 24, 0);
		post_scf->inc_vscl = set_bits32(post_scf->inc_vscl, scf_cfg->v_ratio, 24, 0);
		post_scf->en_mmp = set_bits32(post_scf->en_mmp, 0x1, 1, 0);
	} else {
		post_scf->en_hscl = set_bits32(post_scf->en_hscl, 0x0, 1, 0);
		post_scf->en_vscl = set_bits32(post_scf->en_vscl, 0x0, 2, 0);
	}
}

int dpu_post_scf_config(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req)
{
	struct dpu_panel_info *pinfo = NULL;
	dss_rect_t src_rect = {0};
	dss_rect_t dst_rect = {0};
	dss_scl_t *post_scf = NULL;
	int ret;
	struct post_scf_config scf_cfg = {0};

	scf_cfg.h_ratio = SCF_INC_FACTOR;
	scf_cfg.v_ratio = SCF_INC_FACTOR;
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");

	pinfo = &(dpufd->panel_info);

	if (!DPU_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_POST_SCF))
		return 0;

	ret = post_scf_rect_set(dpufd, pinfo, pov_req, &src_rect, &dst_rect);
	if (ret <= 0)
		return ret;

	post_scf = &(dpufd->dss_module.post_scf);
	dpufd->dss_module.post_scf_used = 1;

	ret = calc_h_ratio(src_rect, dst_rect, &scf_cfg);
	if (ret < 0)
		return ret;

	ret = calc_v_ratio(src_rect, dst_rect, &scf_cfg);
	if (ret < 0)
		return ret;

	post_scf_cordinate_config(post_scf, src_rect, dst_rect, &scf_cfg);

	return 0;
}

