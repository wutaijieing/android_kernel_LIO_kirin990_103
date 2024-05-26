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
#include "../../dpu_display_effect.h"
#include "../../dpu_dpe_utils.h"
#include "../../dpu_ovl_online_wb.h"
#include "../../dpu_mmbuf_manager.h"
#include "../../spr/dpu_spr.h"
#include "../../dpu_frame_rate_ctrl.h"

struct tag_org_invalid_sel_val {
	enum dss_mmu_tlb_tag_org tag_org;
	uint32_t invalid_sel_val;
};

void dpu_mif_init(char __iomem *mif_ch_base,
	dss_mif_t *s_mif, int chn_idx)
{
	uint32_t rw_type;

	if (mif_ch_base == NULL) {
		DPU_FB_ERR("mif_ch_base is NULL\n");
		return;
	}
	if (s_mif == NULL) {
		DPU_FB_ERR("s_mif is NULL\n");
		return;
	}

	memset(s_mif, 0, sizeof(dss_mif_t));

	s_mif->mif_ctrl1 = 0x00000020;
	s_mif->mif_ctrl2 = 0x0;
	s_mif->mif_ctrl3 = 0x0;
	s_mif->mif_ctrl4 = 0x0;
	s_mif->mif_ctrl5 = 0x0;

	rw_type = (chn_idx < DSS_WCHN_W0 || chn_idx == DSS_RCHN_V2) ? 0x0 : 0x1;

	s_mif->mif_ctrl1 = set_bits32(s_mif->mif_ctrl1, 0x0, 1, 5);
	s_mif->mif_ctrl1 = set_bits32(s_mif->mif_ctrl1, rw_type, 1, 17);
}

#if defined(CONFIG_DPU_FB_V600) || defined(CONFIG_DPU_FB_V360)
void dpu_mif_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *mif_ch_base, dss_mif_t *s_mif, int chn_idx)
{
	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL!\n");
	dpu_check_and_no_retval(!mif_ch_base, ERR, "mif_ch_base is NULL!\n");

	if (dpufd->index == MEDIACOMMON_PANEL_IDX) {
		/* disable pref */
		dpufd->set_reg(dpufd, mif_ch_base + MIF_CTRL0, 0x0, 32, 0);
		dpufd->set_reg(dpufd, dpufd->media_common_base +
			VBIF0_MIF_OFFSET + AIF_CMD_RELOAD, 0x1, 1, 0);
	} else {
#ifdef CONFIG_DSS_SMMU_V3
		/* enable pref */
		dpufd->set_reg(dpufd, mif_ch_base + MIF_CTRL0, 0x3, 32, 0);
#else
		/* disable pref */
		dpufd->set_reg(dpufd, mif_ch_base + MIF_CTRL0, 0x0, 1, 0);
#endif
	}
}
#else
void dpu_mif_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *mif_ch_base, dss_mif_t *s_mif, int chn_idx)
{
	if (dpufd == NULL) {
		DPU_FB_DEBUG("dpufd is NULL!\n");
		return;
	}

	if (mif_ch_base == NULL) {
		DPU_FB_DEBUG("mif_ch_base is NULL!\n");
		return;
	}

	if (s_mif == NULL) {
		DPU_FB_DEBUG("s_mif is NULL!\n");
		return;
	}

	dpufd->set_reg(dpufd, mif_ch_base + MIF_CTRL1, s_mif->mif_ctrl1, 32, 0);
	dpufd->set_reg(dpufd, mif_ch_base + MIF_CTRL2, s_mif->mif_ctrl2, 32, 0);
	dpufd->set_reg(dpufd, mif_ch_base + MIF_CTRL3, s_mif->mif_ctrl3, 32, 0);
	dpufd->set_reg(dpufd, mif_ch_base + MIF_CTRL4, s_mif->mif_ctrl4, 32, 0);
	dpufd->set_reg(dpufd, mif_ch_base + MIF_CTRL5, s_mif->mif_ctrl5, 32, 0);
}
#endif

void dpu_mif_on(struct dpu_fb_data_type *dpufd)
{
	char __iomem *mif_base = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL\n");
		return;
	}
	mif_base = dpufd->dss_base + DSS_MIF_OFFSET;

#if defined(CONFIG_DPU_FB_V600) || defined(CONFIG_DPU_FB_V360)
	/* the default value of mif is ok for dss_v600, so do nothing in here */
	return;
#else
	set_reg(mif_base + MIF_ENABLE, 0x1, 1, 0);
	set_reg(dpufd->dss_base + MIF_CH0_OFFSET + MIF_CTRL0, 0x1, 1, 0);
	set_reg(dpufd->dss_base + MIF_CH1_OFFSET + MIF_CTRL0, 0x1, 1, 0);
	set_reg(dpufd->dss_base + MIF_CH2_OFFSET + MIF_CTRL0, 0x1, 1, 0);
	set_reg(dpufd->dss_base + MIF_CH3_OFFSET + MIF_CTRL0, 0x1, 1, 0);
	set_reg(dpufd->dss_base + MIF_CH4_OFFSET + MIF_CTRL0, 0x1, 1, 0);
	set_reg(dpufd->dss_base + MIF_CH5_OFFSET + MIF_CTRL0, 0x1, 1, 0);
	set_reg(dpufd->dss_base + MIF_CH6_OFFSET + MIF_CTRL0, 0x1, 1, 0);
	set_reg(dpufd->dss_base + MIF_CH7_OFFSET + MIF_CTRL0, 0x1, 1, 0);
	set_reg(dpufd->dss_base + MIF_CH8_OFFSET + MIF_CTRL0, 0x1, 1, 0);
	set_reg(dpufd->dss_base + MIF_CH9_OFFSET + MIF_CTRL0, 0x1, 1, 0);

#if defined(CONFIG_DPU_FB_V410) || defined(CONFIG_DPU_FB_V501) || \
	defined(CONFIG_DPU_FB_V510)
	set_reg(dpufd->dss_base + MIF_CH10_OFFSET + MIF_CTRL0, 0x1, 1, 0);
	set_reg(dpufd->dss_base + MIF_CH11_OFFSET + MIF_CTRL0, 0x1, 1, 0);
#endif

#if defined(CONFIG_DPU_FB_V501) || defined(CONFIG_DPU_FB_V510) || \
	defined(CONFIG_DPU_FB_V350) || defined(CONFIG_DPU_FB_V345) || \
	defined(CONFIG_DPU_FB_V346)
	set_reg(mif_base + AIF_CMD_RELOAD, 0x1, 1, 0);
#endif

#endif
}

static void set_mif_ctrl_accord_to_sel_value(dss_mif_t *mif, dss_img_t *img)
{
	uint32_t *semi_plane2 = NULL;

	semi_plane2 = &mif->mif_ctrl4;

	if (img->stride > 0)
		mif->mif_ctrl5 = set_bits32(mif->mif_ctrl5,
			((img->stride / MIF_STRIDE_UNIT) +
			(((img->stride % MIF_STRIDE_UNIT) > 0) ? 1 : 0)), 20, 0);

	if (is_yuv_semiplanar(img->format)) {
		if (img->stride_plane1 > 0)
			*semi_plane2 = set_bits32(*semi_plane2,
				((img->stride_plane1 / MIF_STRIDE_UNIT) +
				(((img->stride_plane1 % MIF_STRIDE_UNIT) > 0) ? 1 : 0)), 20, 0);
	} else if (is_yuv_plane(img->format)) {
		if (img->stride_plane1 > 0)
			mif->mif_ctrl4 = set_bits32(mif->mif_ctrl4,
				((img->stride_plane1 / MIF_STRIDE_UNIT) +
				(((img->stride_plane1 % MIF_STRIDE_UNIT) > 0) ? 1 : 0)), 20, 0);

		if (img->stride_plane2 > 0)
			mif->mif_ctrl3 = set_bits32(mif->mif_ctrl3,
				((img->stride_plane2 / MIF_STRIDE_UNIT) +
				(((img->stride_plane2 % MIF_STRIDE_UNIT) > 0) ? 1 : 0)), 20, 0);
	} else {
		;
	}
}

static void set_mif_ctrl(struct dpu_fb_data_type *dpufd, dss_mif_t *mif,
		dss_img_t *img, uint32_t invalid_sel)
{
	uint32_t *semi_plane1 = NULL;

	semi_plane1 = &mif->mif_ctrl4;

	mif->mif_ctrl1 = set_bits32(mif->mif_ctrl1, 0x0, 1, 5);
	mif->mif_ctrl1 = set_bits32(mif->mif_ctrl1, invalid_sel, 2, 10);
	mif->mif_ctrl1 = set_bits32(mif->mif_ctrl1, ((invalid_sel == 0) ? 0x1 : 0x0), 1, 19);

	if (invalid_sel == 0) {
		mif->mif_ctrl2 = set_bits32(mif->mif_ctrl2, 0x0, 20, 0);
		mif->mif_ctrl3 = set_bits32(mif->mif_ctrl3, 0x0, 20, 0);
		mif->mif_ctrl4 = set_bits32(mif->mif_ctrl4, 0x0, 20, 0);
		mif->mif_ctrl5 = set_bits32(mif->mif_ctrl5, 0x0, 20, 0);
	} else if ((invalid_sel == 1) || (invalid_sel == 2)) {
		set_mif_ctrl_accord_to_sel_value(mif, img);
	} else if (invalid_sel == 3) {
		if (img->stride > 0)
			mif->mif_ctrl5 = set_bits32(mif->mif_ctrl5, DSS_MIF_CTRL2_INVAL_SEL3_STRIDE_MASK, 4, 16);
		if (is_yuv_semiplanar(img->format)) {
			if (img->stride_plane1 > 0)
				*semi_plane1 = set_bits32(*semi_plane1, 0xE, 4, 16);
		} else if (is_yuv_plane(img->format)) {
			if (img->stride_plane1 > 0)
				mif->mif_ctrl3 = set_bits32(mif->mif_ctrl3, 0xE, 4, 16);

			if (img->stride_plane2 > 0)
				mif->mif_ctrl4 = set_bits32(mif->mif_ctrl4, 0xE, 4, 16);
		} else {
			;  /* do nothing */
		}

		/* Tile
		 * YUV_SP: RDMA0 128KB aligned, RDMA1 64KB aligned
		 * YUV_P: RDMA0 128KB aligned, RDMA1 64KB aligned, RDMA2 64KB aligned
		 * ROT: 256KB aligned.
		 */
	} else {
		DPU_FB_ERR("fb%d, invalid_sel:%d not support!\n", dpufd->index, invalid_sel);
	}
}

static struct tag_org_invalid_sel_val g_tag_org_sel_val[] = {
	{MMU_TLB_TAG_ORG_0x0, 1}, {MMU_TLB_TAG_ORG_0x1, 1},
	{MMU_TLB_TAG_ORG_0x2, 2}, {MMU_TLB_TAG_ORG_0x3, 2},
	{MMU_TLB_TAG_ORG_0x4, 0}, {MMU_TLB_TAG_ORG_0x7, 0},
	{MMU_TLB_TAG_ORG_0x8, 3}, {MMU_TLB_TAG_ORG_0x9, 3},
	{MMU_TLB_TAG_ORG_0xA, 3}, {MMU_TLB_TAG_ORG_0xB, 3},
	{MMU_TLB_TAG_ORG_0xC, 0}, {MMU_TLB_TAG_ORG_0xF, 0},
	{MMU_TLB_TAG_ORG_0x10, 1}, {MMU_TLB_TAG_ORG_0x11, 1},
	{MMU_TLB_TAG_ORG_0x12, 2}, {MMU_TLB_TAG_ORG_0x13, 2},
	{MMU_TLB_TAG_ORG_0x14, 0}, {MMU_TLB_TAG_ORG_0x17, 0},
	{MMU_TLB_TAG_ORG_0x18, 3}, {MMU_TLB_TAG_ORG_0x19, 3},
	{MMU_TLB_TAG_ORG_0x1A, 3}, {MMU_TLB_TAG_ORG_0x1B, 3},
	{MMU_TLB_TAG_ORG_0x1C, 0}, {MMU_TLB_TAG_ORG_0x1F, 0}
};

uint32_t dpu_mif_get_invalid_sel(dss_img_t *img, uint32_t transform, int v_scaling_factor,
	uint8_t is_tile, bool rdma_stretch_enable)
{
	uint32_t invalid_sel_val = 0;
	bool find_tag_org = false;
	uint32_t tlb_tag_org;
	uint32_t i;

	dpu_check_and_return(!img, 0, ERR, "img is NULL!\n");

	if ((transform == (DPU_FB_TRANSFORM_ROT_90 | DPU_FB_TRANSFORM_FLIP_H)) ||
		(transform == (DPU_FB_TRANSFORM_ROT_90 | DPU_FB_TRANSFORM_FLIP_V)))
		transform = DPU_FB_TRANSFORM_ROT_90;

	tlb_tag_org =  (transform & 0x7) | ((is_tile ? 1 : 0) << 3) | ((rdma_stretch_enable ? 1 : 0) << 4);

	for (i = 0; i < sizeof(g_tag_org_sel_val) / sizeof(struct tag_org_invalid_sel_val); i++) {
		if (tlb_tag_org == g_tag_org_sel_val[i].tag_org) {
			invalid_sel_val = g_tag_org_sel_val[i].invalid_sel_val;
			find_tag_org = true;
			break;
		}
	}

	if (!find_tag_org) {
		invalid_sel_val = 0;
		DPU_FB_ERR("not support this tlb_tag_org 0x%x!\n", tlb_tag_org);
	}

	return invalid_sel_val;
}

int dpu_mif_config(struct dpu_fb_data_type *dpufd,
	dss_layer_t *layer, dss_wb_layer_t *wb_layer, bool rdma_stretch_enable)
{
	dss_mif_t *mif = NULL;
	int chn_idx = 0;
	dss_img_t *img = NULL;
	uint32_t transform = 0;
	uint32_t invalid_sel = 0;
	uint32_t need_cap = 0;
	int v_scaling_factor = 0;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL\n");
		return -EINVAL;
	}
	if ((layer == NULL) && (wb_layer == NULL)) {
		DPU_FB_ERR("layer and wb_layer is NULL\n");
		return -EINVAL;
	}

	if (wb_layer != NULL) {
		img = &(wb_layer->dst);
		chn_idx = wb_layer->chn_idx;
		transform = wb_layer->transform;
		need_cap = wb_layer->need_cap;
		v_scaling_factor = 1;
	} else {
		img = &(layer->img);
		chn_idx = layer->chn_idx;
		transform = layer->transform;
		need_cap = layer->need_cap;
		v_scaling_factor = layer->src_rect.h / layer->dst_rect.h +
			((layer->src_rect.h % layer->dst_rect.h) > 0 ? 1 : 0);
	}

	mif = &(dpufd->dss_module.mif[chn_idx]);
	dpufd->dss_module.mif_used[chn_idx] = 1;

	if (img->mmu_enable == 0) {
		mif->mif_ctrl1 = set_bits32(mif->mif_ctrl1, 0x1, 1, 5);
	} else {
		if (need_cap & (CAP_AFBCD | CAP_AFBCE | CAP_HFBCD | CAP_HFBCE | CAP_HEBCD | CAP_HEBCE))
			invalid_sel = 0;
		else
			invalid_sel = dpu_mif_get_invalid_sel(img, transform, v_scaling_factor,
				((need_cap & CAP_TILE) ? 1 : 0), rdma_stretch_enable);

		set_mif_ctrl(dpufd, mif, img, invalid_sel);
	}

	return 0;
}
