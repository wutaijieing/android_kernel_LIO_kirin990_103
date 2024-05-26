/* Copyright (c) Hisilicon Technologies Co., Ltd. 2021-2021. All rights reserved.
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

void spr_fill_dirty_region_info(struct dpu_fb_data_type *dpufd, struct dss_rect *dirty,
	dss_overlay_t *pov_req, spr_dirty_region *spr_dirty)
{
	if((dpufd == NULL) || (dirty == NULL) || (pov_req == NULL) || (spr_dirty == NULL))
		return;

	if ((dpufd->panel_info.spr_dsc_mode == SPR_DSC_MODE_NONE) ||
		(dpufd->panel_info.spr.spr_en == 0))
		return;

	spr_dirty->spr_overlap_type = pov_req->spr_overlap_type;
	spr_dirty->region_x = dirty->x;
	spr_dirty->region_y = dirty->y;
	spr_dirty->spr_img_size = (DSS_WIDTH((uint32_t)dirty->h) << 16) | DSS_WIDTH((uint32_t)dirty->w);
}

void spr_get_real_dirty_region(struct dpu_fb_data_type *dpufd, struct dss_rect *dirty,
	dss_overlay_t *pov_req)
{
	if((dpufd == NULL) || (dirty == NULL) || (pov_req == NULL))
		return;

	if ((dpufd->panel_info.spr_dsc_mode == SPR_DSC_MODE_NONE) ||
		(dpufd->panel_info.spr.spr_en == 0))
		return;

	DPU_FB_INFO("check dirty height need > 0: dirty->h = %d\n", dirty->h);

	switch (pov_req->spr_overlap_type) {
	case SPR_OVERLAP_TOP:
		if (dirty->h > 0) {
			dirty->y += 1;
			dirty->h -= 1;
		}
		break;
	case SPR_OVERLAP_BOTTOM:
		if (dirty->h > 0)
			dirty->h -= 1;
		break;
	case SPR_OVERLAP_TOP_BOTTOM:
		if (dirty->h > 1) {
			dirty->y += 1;
			dirty->h -= 2;
		}
		break;
	default:
		return;
	}
}

#ifdef SUPPORT_SPR_DSC1_2_SEPARATE
uint32_t get_hsize_after_spr_dsc(struct dpu_fb_data_type *dpufd, uint32_t rect_width)
{
	struct dpu_panel_info *pinfo = NULL;
	struct spr_dsc_panel_para *spr = NULL;
	uint32_t hsize = rect_width;
	uint8_t cpnt_num;
	uint8_t bpc;
	uint8_t bpp;

	if (!dpufd) {
		DPU_FB_ERR("null dpufd\n");
		return hsize;
	}

	pinfo = &(dpufd->panel_info);

	if (pinfo->spr_dsc_mode == SPR_DSC_MODE_NONE)
		return hsize;

	spr = &(pinfo->spr);
	cpnt_num = (spr->dsc_sample & BIT(0)) ? 4 : 3;
	bpc = spr->bits_per_component;
	bpp = (uint8_t)((spr->bpp_chk & 0x3FF) >> 4);

	/* compress_ratio = bpp / (bpc * cpnt_num)
	 * bpp 10bit-8, 8bit-6, dsc out 3bytes-24bit
	 * SPR compress 3/2; DSC 1.2 compress 8/3
	 */
	switch (pinfo->spr_dsc_mode) {
	case SPR_DSC_MODE_SPR_AND_DSC:
		if ((bpc != 0) && (cpnt_num  != 0))
			hsize = (rect_width * bpp * 2) / (bpc * cpnt_num * 3);
		break;
	case SPR_DSC_MODE_SPR_ONLY:
		hsize = rect_width * 2 / 3;
		break;
	case SPR_DSC_MODE_DSC_ONLY:
		if ((bpc != 0) && (cpnt_num  != 0))
			hsize = (rect_width * bpp) / (bpc * cpnt_num);
		break;
	default:
		break;
	}

	return hsize;
}

#else
uint32_t get_hsize_after_spr_dsc(struct dpu_fb_data_type *dpufd, uint32_t rect_width)
{
	struct dpu_panel_info *pinfo = NULL;
	uint32_t hsize = rect_width;
	uint32_t bits_per_pixel;
	uint32_t bits_per_component;
	uint32_t dsc_slice_num;
	struct panel_dsc_info *panel_dsc_info = NULL;

	if (!dpufd) {
		DPU_FB_ERR("null dpufd\n");
		return hsize;
	}

	pinfo = &(dpufd->panel_info);
	panel_dsc_info = &(pinfo->panel_dsc_info);

	if (pinfo->spr_dsc_mode == SPR_DSC_MODE_NONE)
		return hsize;

	if (pinfo->vesa_dsc.bits_per_pixel == DSC_0BPP) {
		/* DSC new mode */
		bits_per_pixel = pinfo->panel_dsc_info.dsc_info.dsc_bpp;
		bits_per_component = pinfo->panel_dsc_info.dsc_info.dsc_bpc;
	} else {
		bits_per_pixel = pinfo->vesa_dsc.bits_per_pixel;
		bits_per_component = pinfo->vesa_dsc.bits_per_component;
	}
	dsc_slice_num = panel_dsc_info->dual_dsc_en + 1;

	if (bits_per_component == 0) {
		DPU_FB_ERR("bpp value is error\n");
		return hsize;
	}

	if (rect_width == 0) {
		DPU_FB_ERR("rect_width is zero\n");
		return hsize;
	}

	if (pinfo->xres / rect_width == 0) {
		DPU_FB_ERR("xres = %d, rect_width is %d \n", pinfo->xres, rect_width);
		return hsize;
	}

	/*
	 * bits_per_component * 3: 3 means GPR888 have 3 component
	 * pinfo->vesa_dsc.bits_per_pixel / 2: 2 means YUV422 BPP nead plus 2 config.
	 */
	switch (pinfo->spr_dsc_mode) {
	case SPR_DSC_MODE_SPR_AND_DSC:
		hsize = ((panel_dsc_info->dsc_info.chunk_size + panel_dsc_info->dsc_insert_byte_num) *
			dsc_slice_num) * 8 / DSC_OUTPUT_MODE;

		hsize = hsize / (pinfo->xres / rect_width);
		break;
	case SPR_DSC_MODE_SPR_ONLY:
		hsize = rect_width * 2 / 3;
		break;
	case SPR_DSC_MODE_DSC_ONLY:
		hsize = (rect_width * bits_per_pixel) / (bits_per_component * 3);
		break;
	default:
		break;
	}

	return hsize;
}
#endif