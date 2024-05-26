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

#include "overlay/dpu_overlay_utils.h"
#include "dpu_mmbuf_manager.h"

uint32_t g_fpga_flag;
void *g_smmu_rwerraddr_virt;

void dpu_qos_on(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL\n");

	outp32(dpufd->noc_dss_base + 0xc, 0x1);
	outp32(dpufd->noc_dss_base + 0x10, 0x1000);
	outp32(dpufd->noc_dss_base + 0x14, 0x20);
	outp32(dpufd->noc_dss_base + 0x18, 0x1);

	outp32(dpufd->noc_dss_base + 0x10c, 0x1);
	outp32(dpufd->noc_dss_base + 0x110, 0x1000);
	outp32(dpufd->noc_dss_base + 0x114, 0x20);
	outp32(dpufd->noc_dss_base + 0x118, 0x1);

	outp32(dpufd->noc_dss_base + 0x18c, 0x1);
	outp32(dpufd->noc_dss_base + 0x190, 0x1000);
	outp32(dpufd->noc_dss_base + 0x194, 0x20);
	outp32(dpufd->noc_dss_base + 0x198, 0x1);
}

static int dpu_check_userdata_check(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req, dss_overlay_block_t *pov_h_block_infos)
{
	if ((pov_req->ov_block_nums <= 0) ||
		(pov_req->ov_block_nums > DPU_OV_BLOCK_NUMS)) {
		DPU_FB_ERR("fb%d, invalid ov_block_nums=%d!\n",
			dpufd->index, pov_req->ov_block_nums);
		return -EINVAL;
	}

	if ((pov_h_block_infos->layer_nums <= 0) ||
		(pov_h_block_infos->layer_nums > OVL_LAYER_NUM_MAX)) {
		DPU_FB_ERR("fb%d, invalid layer_nums=%d!\n",
			dpufd->index, pov_h_block_infos->layer_nums);
		return -EINVAL;
	}

	if ((pov_req->ovl_idx != DSS_OVL0) && (pov_req->ovl_idx != DSS_OVL2)) {
		DPU_FB_ERR("fb%d, invalid ovl_idx=%d!\n", dpufd->index, pov_req->ovl_idx);
		return -EINVAL;
	}

	if ((dpufd->index != PRIMARY_PANEL_IDX) && (dpufd->index != AUXILIARY_PANEL_IDX)) {
		DPU_FB_ERR("dpu fb%d is invalid!\n", dpufd->index);
		return -EINVAL;
	}

	return 0;
}

static int dpu_check_wblayer_buff(struct dpu_fb_data_type *dpufd, dss_wb_layer_t *wb_layer)
{
	if (dpu_check_addr_validate(&wb_layer->dst))
		return 0;

	if (wb_layer->need_cap & (CAP_BASE | CAP_DIM | CAP_PURE_COLOR))
		return 0;

	return -EINVAL;
}

static int dpu_check_userdata_dst(dss_wb_layer_t *wb_layer, uint32_t index)
{
	if (!wb_layer) {
		DPU_FB_ERR("fb%d, invalid wb_layer!", index);
		return -EINVAL;
	}

	if (wb_layer->need_cap & CAP_AFBCE) {
		if ((wb_layer->dst.afbc_header_stride == 0) || (wb_layer->dst.afbc_payload_stride == 0)) {
			DPU_FB_ERR("fb%d, afbc_header_stride = %d, afbc_payload_stride = %d is invalid!",
				index, wb_layer->dst.afbc_header_stride, wb_layer->dst.afbc_payload_stride);
			return -EINVAL;
		}
	}

	if (wb_layer->need_cap & CAP_HFBCE) {
		if ((wb_layer->dst.hfbc_header_stride0 == 0) || (wb_layer->dst.hfbc_payload_stride0 == 0) ||
			(wb_layer->dst.hfbc_header_stride1 == 0) || (wb_layer->dst.hfbc_payload_stride1 == 0) ||
			(wb_layer->chn_idx != DSS_WCHN_W1)) {
			DPU_FB_ERR("fb%d, hfbc_header_stride0 = %d, hfbc_payload_stride0 = %d,"
				"hfbc_header_stride1 = %d, hfbc_payload_stride1 = %d is invalid or "
				"wchn_idx = %d no support hfbce!\n",
				index, wb_layer->dst.hfbc_header_stride0, wb_layer->dst.hfbc_payload_stride0,
				wb_layer->dst.hfbc_header_stride1, wb_layer->dst.hfbc_payload_stride1,
				wb_layer->chn_idx);
			return -EINVAL;
		}
	}

	return 0;
}

static int dpu_check_dirty_rect(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req)
{
	if (dpufd->index == PRIMARY_PANEL_IDX) {
		if (dpufd->panel_info.dirty_region_updt_support) {
			if (pov_req->dirty_rect.x < 0 || pov_req->dirty_rect.y < 0 ||
				pov_req->dirty_rect.w < 0 || pov_req->dirty_rect.h < 0) {
				DPU_FB_ERR("dirty_rect[%d, %d, %d, %d] is out of range!\n",
					pov_req->dirty_rect.x, pov_req->dirty_rect.y,
					pov_req->dirty_rect.w, pov_req->dirty_rect.h);
				return -EINVAL;
			}
		}
	}

	return 0;
}

static int dpu_check_ov_info(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req)
{
	if ((pov_req->wb_layer_nums <= 0) ||
		(pov_req->wb_layer_nums > MAX_DSS_DST_NUM)) {
		DPU_FB_ERR("fb%d, invalid wb_layer_nums=%d!",
			dpufd->index, pov_req->wb_layer_nums);
		return -EINVAL;
	}

	if (pov_req->wb_ov_rect.x < 0 || pov_req->wb_ov_rect.y < 0) {
		DPU_FB_ERR("wb_ov_rect[%d, %d] is out of range!\n",
			pov_req->wb_ov_rect.x, pov_req->wb_ov_rect.y);
		return -EINVAL;
	}

	if (pov_req->wb_compose_type >= DSS_WB_COMPOSE_TYPE_MAX) {
		DPU_FB_ERR("wb_compose_type=%u is invalid!\n", pov_req->wb_compose_type);
		return -EINVAL;
	}

	return 0;
}

static bool is_wb_layer_size_info_empty(dss_wb_layer_t *wb_layer)
{
	return ((wb_layer->dst.bpp == 0) || (wb_layer->dst.width == 0) ||
		(wb_layer->dst.height == 0) || (wb_layer->dst.stride == 0));
}

static int dpu_check_wblayer_rect(
	struct dpu_fb_data_type *dpufd, dss_wb_layer_t *wb_layer)
{
	if (wb_layer->src_rect.x < 0 || wb_layer->src_rect.y < 0 ||
		wb_layer->src_rect.w <= 0 || wb_layer->src_rect.h <= 0) {
		DPU_FB_ERR("src_rect[%d, %d, %d, %d] is out of range!\n",
			wb_layer->src_rect.x, wb_layer->src_rect.y,
			wb_layer->src_rect.w, wb_layer->src_rect.h);
		return -EINVAL;
	}

	if (wb_layer->dst_rect.x < 0 || wb_layer->dst_rect.y < 0 ||
		wb_layer->dst_rect.w <= 0 || wb_layer->dst_rect.h <= 0 ||
		wb_layer->dst_rect.w > wb_layer->dst.width || wb_layer->dst_rect.h > wb_layer->dst.height) {
		DPU_FB_ERR("dst_rect[%d, %d, %d, %d], dst[%d, %d] is out of range!\n",
			wb_layer->dst_rect.x, wb_layer->dst_rect.y, wb_layer->dst_rect.w,
			wb_layer->dst_rect.h, wb_layer->dst.width, wb_layer->dst.height);
		return -EINVAL;
	}

	return 0;
}

static int dpu_check_wb_layer_info(struct dpu_fb_data_type *dpufd,
	dss_wb_layer_t *wb_layer)
{
	if (wb_layer->chn_idx != DSS_WCHN_W2) {
		if (wb_layer->chn_idx < DSS_WCHN_W0 || wb_layer->chn_idx > DSS_WCHN_W1) {
			DPU_FB_ERR("fb%d, wchn_idx=%d is invalid!\n", dpufd->index, wb_layer->chn_idx);
			return -EINVAL;
		}
	}

	if (dpu_check_wblayer_buff(dpufd, wb_layer)) {
		DPU_FB_ERR("fb%d, failed to check_wblayer_buff!\n", dpufd->index);
		return -EINVAL;
	}

	if (dpu_check_userdata_dst(wb_layer, dpufd->index) < 0)
		return -EINVAL;

	if (wb_layer->dst.format >= DPU_FB_PIXEL_FORMAT_MAX) {
		DPU_FB_ERR("fb%d, format=%d is invalid!\n", dpufd->index, wb_layer->dst.format);
		return -EINVAL;
	}

	if (is_wb_layer_size_info_empty(wb_layer)) {
		DPU_FB_ERR("fb%d, bpp=%d, width=%d, height=%d, stride=%d is invalid!\n",
			dpufd->index, wb_layer->dst.bpp, wb_layer->dst.width,
			wb_layer->dst.height, wb_layer->dst.stride);
		return -EINVAL;
	}

	if (wb_layer->need_cap & CAP_AFBCE) {
		if ((wb_layer->dst.afbc_header_stride == 0) || (wb_layer->dst.afbc_payload_stride == 0)) {
			DPU_FB_ERR("fb%d, afbc_header_stride=%d, afbc_payload_stride=%d is invalid!\n",
				dpufd->index, wb_layer->dst.afbc_header_stride, wb_layer->dst.afbc_payload_stride);
			return -EINVAL;
		}
	}

	if (wb_layer->dst.csc_mode >= DSS_CSC_MOD_MAX) {
		DPU_FB_ERR("fb%d, csc_mode=%d is invalid!\n", dpufd->index, wb_layer->dst.csc_mode);
		return -EINVAL;
	}

	if (wb_layer->dst.afbc_scramble_mode >= DSS_AFBC_SCRAMBLE_MODE_MAX) {
		DPU_FB_ERR("fb%d, afbc_scramble_mode=%d is invalid!\n",
			dpufd->index, wb_layer->dst.afbc_scramble_mode);
		return -EINVAL;
	}

	if (dpu_check_wblayer_rect(dpufd, wb_layer)) {
		DPU_FB_ERR("fb%d, failed to check_wblayer_rect!\n", dpufd->index);
		return -EINVAL;
	}

	return 0;
}

int dpu_check_userdata(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req, dss_overlay_block_t *pov_h_block_infos)
{
	int i;
	dss_wb_layer_t *wb_layer = NULL;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "invalid dpufd!\n");
	dpu_check_and_return(!pov_req, -EINVAL, ERR, "invalid pov_req!\n");
	dpu_check_and_return(!pov_h_block_infos, -EINVAL, ERR, "invalid pov_h_block_infos!\n");

	if (dpu_check_userdata_check(dpufd, pov_req, pov_h_block_infos))
		return -EINVAL;

	if (dpu_check_dirty_rect(dpufd, pov_req) < 0)
		return -EINVAL;

	if (dpufd->index == AUXILIARY_PANEL_IDX) {
		if (pov_req->wb_enable != 1) {
			DPU_FB_ERR("pov_req->wb_enable=%u is invalid!\n", pov_req->wb_enable);
			return -EINVAL;
		}

		if (dpu_check_ov_info(dpufd, pov_req) < 0)
			return -EINVAL;

		for (i = 0; i < pov_req->wb_layer_nums; i++) {
			wb_layer = &(pov_req->wb_layer_infos[i]);
			if (dpu_check_wb_layer_info(dpufd, wb_layer) < 0)
				return -EINVAL;
		}
	} else {
		if (pov_req->wb_layer_nums > MAX_DSS_DST_NUM) {
			DPU_FB_ERR("fb%d, invalid wb_layer_nums=%d!\n",
				dpufd->index, pov_req->wb_layer_nums);
			return -EINVAL;
		}
	}

	return 0;
}

static int dpu_check_layer_par_base(struct dpu_fb_data_type *dpufd, dss_layer_t *layer)
{
	if (layer->layer_idx < 0 || layer->layer_idx >= OVL_LAYER_NUM_MAX) {
		DPU_FB_ERR("fb%d, layer_idx=%d is invalid!\n", dpufd->index, layer->layer_idx);
		return -EINVAL;
	}

	if (layer->need_cap & (CAP_BASE | CAP_DIM | CAP_PURE_COLOR))
		return 0;

	return 1;
}

static int dpu_check_layer_buff(struct dpu_fb_data_type *dpufd, dss_layer_t *layer)
{
	if (dpu_check_layer_addr_validate(layer))
		return 0;

	if (layer->need_cap & (CAP_BASE | CAP_DIM | CAP_PURE_COLOR))
		return 0;

	return -EINVAL;
}

static int dpu_check_layer_ch_valid(struct dpu_fb_data_type *dpufd,
	dss_layer_t *layer)
{
	if (dpufd->index == AUXILIARY_PANEL_IDX) {
		if (layer->chn_idx != DSS_RCHN_V2) {
			if ((layer->chn_idx < 0 || layer->chn_idx >= DSS_WCHN_W0) ||
			(layer->chn_idx == DSS_RCHN_G0) || (layer->chn_idx == DSS_RCHN_V0)) {
				DPU_FB_ERR("fb%d, rchn_idx=%d is invalid!\n", dpufd->index, layer->chn_idx);
				return -EINVAL;
			}
		}

		if (layer->chn_idx == DSS_RCHN_D2) {
			DPU_FB_ERR("fb%d, chn_idx[%d] does not used by offline play!\n",
				dpufd->index, layer->chn_idx);
			return -EINVAL;
		}
	} else if (dpufd->index == PRIMARY_PANEL_IDX) {
		if ((layer->chn_idx > DSS_WCHN_W0) ||
			(layer->chn_idx == DSS_RCHN_G0) ||
			(layer->chn_idx == DSS_RCHN_V0) ||
			(layer->chn_idx < DSS_RCHN_D2)) {
			DPU_FB_ERR("fb%d, rchn_idx=%d is invalid!\n", dpufd->index, layer->chn_idx);
			return -EINVAL;
		}
	}

	return 0;
}

static bool is_layer_basic_size_info_empty(const dss_layer_t *layer)
{
	return ((layer->img.bpp == 0) || (layer->img.width == 0) ||
		(layer->img.height == 0) || (layer->img.stride == 0));
}

static int dpu_check_layer_basic_param_valid(struct dpu_fb_data_type *dpufd,
	dss_layer_t *layer)
{
	if (layer->blending < 0 || layer->blending >= DPU_FB_BLENDING_MAX) {
		DPU_FB_ERR("fb%d, blending=%d is invalid!\n", dpufd->index, layer->blending);
		return -EINVAL;
	}

	if (layer->img.format >= DPU_FB_PIXEL_FORMAT_MAX) {
		DPU_FB_ERR("fb%d, format=%d is invalid!\n", dpufd->index, layer->img.format);
		return -EINVAL;
	}

	if (is_layer_basic_size_info_empty(layer)) {
		DPU_FB_ERR("fb%d, bpp=%d, width=%d, height=%d, stride=%d is invalid!\n",
			dpufd->index, layer->img.bpp, layer->img.width, layer->img.height,
			layer->img.stride);
		return -EINVAL;
	}

	if (dpu_check_layer_buff(dpufd, layer)) {
		DPU_FB_ERR("fb%d, failed to check_layer_buff!\n", dpufd->index);
		return -EINVAL;
	}

	return 0;
}

static int dpu_check_layer_par_need_cap(dss_layer_t *layer, uint32_t index)
{
	if (layer->need_cap & CAP_AFBCD) {
		if ((layer->img.afbc_header_stride == 0) ||
			(layer->img.afbc_payload_stride == 0) ||
			(layer->img.mmbuf_size == 0)) {
			DPU_FB_ERR("fb%d, afbc_header_stride=%d, afbc_payload_stride=%d, mmbuf_size=%d is invalid!",
				index, layer->img.afbc_header_stride,
				layer->img.afbc_payload_stride, layer->img.mmbuf_size);
			return -EINVAL;
		}
	}

	return 0;
}

static int dpu_check_layer_rot_cap(struct dpu_fb_data_type *dpufd, dss_layer_t *layer)
{
	if (layer->transform & DPU_FB_TRANSFORM_ROT_90) {
		if (((layer->chn_idx == DSS_RCHN_V0) ||
			(layer->chn_idx == DSS_RCHN_V1)) &&
			(layer->need_cap & CAP_HFBCD)) {
			DPU_FB_DEBUG("fb%d, ch%d need rot and hfbcd!\n!", dpufd->index, layer->chn_idx);
		} else {
			DPU_FB_ERR("fb%d, ch%d is not support need_cap=%d,transform=%d!\n",
				dpufd->index, layer->chn_idx, layer->need_cap, layer->transform);
			return -EINVAL;
		}
	}

	return 0;
}

static int dpu_check_layer_rect(dss_layer_t *layer)
{
	if (layer->src_rect.x < 0 || layer->src_rect.y < 0 ||
		layer->src_rect.w <= 0 || layer->src_rect.h <= 0) {
		DPU_FB_ERR("src_rect[%d, %d, %d, %d] is out of range!\n",
			layer->src_rect.x, layer->src_rect.y,
			layer->src_rect.w, layer->src_rect.h);
		return -EINVAL;
	}

	if (layer->src_rect_mask.x < 0 || layer->src_rect_mask.y < 0 ||
		layer->src_rect_mask.w < 0 || layer->src_rect_mask.h < 0) {
		DPU_FB_ERR("src_rect_mask[%d, %d, %d, %d] is out of range!\n",
			layer->src_rect_mask.x, layer->src_rect_mask.y,
			layer->src_rect_mask.w, layer->src_rect_mask.h);
		return -EINVAL;
	}

	if (layer->dst_rect.x < 0 || layer->dst_rect.y < 0 ||
		layer->dst_rect.w <= 0 || layer->dst_rect.h <= 0) {
		DPU_FB_ERR("dst_rect[%d, %d, %d, %d] is out of range!\n",
			layer->dst_rect.x, layer->dst_rect.y,
			layer->dst_rect.w, layer->dst_rect.h);
		return -EINVAL;
	}

	return 0;
}


int dpu_check_layer_par(struct dpu_fb_data_type *dpufd, dss_layer_t *layer)
{
	int ret;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL!\n");
	dpu_check_and_return(!layer, -EINVAL, ERR, "layer is NULL, return!\n");

	ret = dpu_check_layer_par_base(dpufd, layer);
	if (ret <= 0)
		return ret;

	if (dpu_check_layer_ch_valid(dpufd, layer) < 0)
		return -EINVAL;

	if (dpu_check_layer_basic_param_valid(dpufd, layer) < 0)
		return -EINVAL;

	if (dpu_check_layer_par_need_cap(layer, dpufd->index) < 0)
		return -EINVAL;

	if (dpu_check_layer_rot_cap(dpufd, layer) < 0)
		return -EINVAL;

	if (layer->img.csc_mode >= DSS_CSC_MOD_MAX) {
		DPU_FB_ERR("fb%d, csc_mode=%d is invalid!\n", dpufd->index, layer->img.csc_mode);
		return -EINVAL;
	}

	if (layer->img.afbc_scramble_mode >= DSS_AFBC_SCRAMBLE_MODE_MAX) {
		DPU_FB_ERR("fb%d, afbc_scramble_mode=%d is invalid!\n", dpufd->index, layer->img.afbc_scramble_mode);
		return -EINVAL;
	}

	if ((layer->layer_idx != 0) && (layer->need_cap & CAP_BASE)) {
		DPU_FB_ERR("fb%d, layer%d is not base!\n", dpufd->index, layer->layer_idx);
		return -EINVAL;
	}

	if (dpu_check_layer_rect(layer)) {
		DPU_FB_ERR("fb%d, failed to check_layer_rect!\n", dpufd->index);
		return -EINVAL;
	}

	return 0;
}

static void dpufb_dss_media1_subsys_pu_reset(char __iomem *peri_crg_base, char __iomem *pctrl_base)
{
	uint32_t ret;

	/* MEDIA1_SUBSYS_PU
	 * 1: module mtcmos on
	 */
	outp32(peri_crg_base + PERPWREN, 0x00000020);
	udelay(100);

	/* 2: module mem sd off */
	outp32(pctrl_base + PERI_CTRL102, 0x10000000);
	udelay(1);

	/* 2: module unrst */
	outp32(peri_crg_base + PERRSTDIS5, 0x00040000);

	/* 3: module clk enable */
	outp32(peri_crg_base + PEREN6, 0x1e002028);
	outp32(peri_crg_base + PEREN4, 0x00000040);
	outp32(peri_crg_base + PEREN7, 0x00000040);
	udelay(1);

	/* 4: module clk disable */
	outp32(peri_crg_base + PERDIS6, 0x1e002028);
	outp32(peri_crg_base + PERDIS4, 0x00000040);
	outp32(peri_crg_base + PERDIS7, 0x00000040);
	udelay(1);

	/* 5: module iso disable */
	outp32(peri_crg_base + ISODIS, 0x00000040);

	/* 2: memory repair == 1 */
	ret = inp32(pctrl_base + PERI_STAT64);
	udelay(400);
	DPU_FB_DEBUG("pctrl_base + PERI_STAT64 = 0x%x\n", ret);

	/* 6: module unrst */
	outp32(peri_crg_base + PERRSTDIS5, 0x00020000);

	/* 7: module clk enable */
	outp32(peri_crg_base + PEREN6, 0x1e002028);
	outp32(peri_crg_base + PEREN4, 0x00000040);
	outp32(peri_crg_base + PEREN7, 0x00000040);
}

static void dpufb_dss_vivobus_pu_reset(char __iomem *media_crg_base, char __iomem *pmctrl_base)
{
	uint32_t ret;

	/* VIVOBUS_PU
	 * 1: module clk enable
	 */
	outp32(media_crg_base + MEDIA_CLKDIV9, 0x00080008);
	outp32(media_crg_base + MEDIA_PEREN0, 0x08040040);
	udelay(1);

	/* 2: module clk disable */
	outp32(media_crg_base + MEDIA_PERDIS0, 0x08040040);
	udelay(1);

	/* 3: module clk enable */
	outp32(media_crg_base + MEDIA_PEREN0, 0x08040040);

	/* 4: bus idle clear */
	outp32(pmctrl_base + NOC_POWER_IDLEREQ, 0x80000000);

	ret = inp32(pmctrl_base + NOC_POWER_IDLEACK);
	udelay(1);
	DPU_FB_DEBUG("pmctrl_base + NOC_POWER_IDLEACK = 0x%x\n", ret);

	ret = inp32(pmctrl_base + NOC_POWER_IDLE);
	udelay(1);
	DPU_FB_DEBUG("pmctrl_base + NOC_POWER_IDLE = 0x%x\n", ret);
}

static void dpufb_dss_pu_reset(char __iomem *media_crg_base, char __iomem *pmctrl_base)
{
	uint32_t ret;

	/* DSS_PU
	 * 1.1: module unrst
	 */
	outp32(media_crg_base + MEDIA_PERRSTDIS0, 0x02000000);

	/* 2: module clk enable */
	outp32(media_crg_base + MEDIA_CLKDIV9, 0xCA80CA80);
	outp32(media_crg_base + MEDIA_PEREN0, 0x0009C000);
	outp32(media_crg_base + MEDIA_PEREN1, 0x00660000);
	outp32(media_crg_base + MEDIA_PEREN2, 0x0000003F);
	udelay(1);

	/* 3: module clk disable */
	outp32(media_crg_base + MEDIA_PERDIS0, 0x0009C000);
	outp32(media_crg_base + MEDIA_PERDIS1, 0x00600000);
	outp32(media_crg_base + MEDIA_PERDIS2, 0x0000003F);
	udelay(1);

	/* 4: module unrst */
	outp32(media_crg_base + MEDIA_PERRSTDIS0, 0x000000C0);
	outp32(media_crg_base + MEDIA_PERRSTDIS1, 0x000000F0);

	/* 5: module clk enable */
	outp32(media_crg_base + MEDIA_PEREN0, 0x0009C000);
	outp32(media_crg_base + MEDIA_PEREN1, 0x00600000);
	outp32(media_crg_base + MEDIA_PEREN2, 0x0000003F);

	/* 6: bus idle clear */
	outp32(pmctrl_base + NOC_POWER_IDLEREQ, 0x20000000);

	ret = inp32(pmctrl_base + NOC_POWER_IDLEACK);
	udelay(1);
	DPU_FB_DEBUG("pmctrl_base + NOC_POWER_IDLEACK = 0x%x\n", ret);

	ret = inp32(pmctrl_base + NOC_POWER_IDLE);
	udelay(1);
	DPU_FB_DEBUG("pmctrl_base + NOC_POWER_IDLE = 0x%x\n", ret);
}

/*
 * DSS disreset
 */
void dpufb_dss_disreset(struct dpu_fb_data_type *dpufd)
{
	char __iomem *peri_crg_base = NULL;
	char __iomem *pmctrl_base = NULL;
	char __iomem *media_crg_base = NULL;
	char __iomem *pctrl_base = NULL;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return;
	}

	peri_crg_base = dpufd->peri_crg_base;
	pmctrl_base = dpufd->pmctrl_base;
	media_crg_base = dpufd->media_crg_base;
	pctrl_base = dpufd->pctrl_base;

	dpufb_dss_media1_subsys_pu_reset(peri_crg_base, pctrl_base);
	dpufb_dss_vivobus_pu_reset(media_crg_base, pmctrl_base);
	dpufb_dss_pu_reset(media_crg_base, pmctrl_base);
}

static int dpu_dpp_gamma_para_set_reg(struct dpu_fb_data_type *dpufd, uint8_t last_gamma_type)
{
	uint32_t *local_gamma_lut_table_r = NULL;
	uint32_t *local_gamma_lut_table_g = NULL;
	uint32_t *local_gamma_lut_table_b = NULL;
	char __iomem *gamma_lut_base = dpufd->dss_base + DSS_DPP_GAMA_LUT_OFFSET;
	struct dpu_panel_info *pinfo = &(dpufd->panel_info);
	uint32_t index;
	uint32_t i;

	if (last_gamma_type == 1) {
		/* set gamma cinema parameter */
		if (pinfo->cinema_gamma_lut_table_len > 0 && pinfo->cinema_gamma_lut_table_R &&
			pinfo->cinema_gamma_lut_table_G && pinfo->cinema_gamma_lut_table_B) {
			local_gamma_lut_table_r = pinfo->cinema_gamma_lut_table_R;
			local_gamma_lut_table_g = pinfo->cinema_gamma_lut_table_G;
			local_gamma_lut_table_b = pinfo->cinema_gamma_lut_table_B;
		} else {
			DPU_FB_ERR("can't get gamma cinema paramter from pinfo.\n");
			return -1;
		}
	} else {
		if (pinfo->gamma_lut_table_len > 0 && pinfo->gamma_lut_table_R &&
			pinfo->gamma_lut_table_G && pinfo->gamma_lut_table_B) {
			local_gamma_lut_table_r = pinfo->gamma_lut_table_R;
			local_gamma_lut_table_g = pinfo->gamma_lut_table_G;
			local_gamma_lut_table_b = pinfo->gamma_lut_table_B;
		} else {
			DPU_FB_ERR("can't get gamma normal parameter from pinfo.\n");
			return -1;
		}
	}

	/* config regsiter use default or cinema parameter */
	for (index = 0; index < pinfo->gamma_lut_table_len / 2; index++) {
		i = index << 1;
		outp32(gamma_lut_base + (U_GAMA_R_COEF + index * 4),
			(local_gamma_lut_table_r[i] | (local_gamma_lut_table_r[i + 1] << 16)));
		outp32(gamma_lut_base + (U_GAMA_G_COEF + index * 4),
			(local_gamma_lut_table_g[i] | (local_gamma_lut_table_g[i + 1] << 16)));
		outp32(gamma_lut_base + (U_GAMA_B_COEF + index * 4),
			(local_gamma_lut_table_b[i] | (local_gamma_lut_table_b[i + 1] << 16)));
	}
	outp32(gamma_lut_base + U_GAMA_R_LAST_COEF, local_gamma_lut_table_r[pinfo->gamma_lut_table_len - 1]);
	outp32(gamma_lut_base + U_GAMA_G_LAST_COEF, local_gamma_lut_table_g[pinfo->gamma_lut_table_len - 1]);
	outp32(gamma_lut_base + U_GAMA_B_LAST_COEF, local_gamma_lut_table_b[pinfo->gamma_lut_table_len - 1]);

	return 0;
}

void dpu_dpp_acm_gm_set_reg(struct dpu_fb_data_type *dpufd)
{
	char __iomem *gmp_base = NULL;
	char __iomem *xcc_base = NULL;
	char __iomem *gamma_base = NULL;
	static uint8_t last_gamma_type;
	static uint32_t gamma_config_flag;
	uint32_t gama_lut_sel;

	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL\n");

	if (dpufd->panel_info.gamma_support == 0 || dpufd->panel_info.acm_support == 0)
		return;

	if (!DPU_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_GAMA) || !DPU_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_ACM)) {
		DPU_FB_DEBUG("gamma or acm are not suppportted in this platform.\n");
		return;
	}

	dpu_check_and_no_retval((dpufd->index != PRIMARY_PANEL_IDX), ERR, "fb%d, not support!\n", dpufd->index);

	gmp_base = dpufd->dss_base + DSS_DPP_GMP_OFFSET;
	xcc_base = dpufd->dss_base + DSS_DPP_XCC_OFFSET;
	gamma_base = dpufd->dss_base + DSS_DPP_GAMA_OFFSET;

	if (gamma_config_flag == 0) {
		if (last_gamma_type != dpufd->panel_info.gamma_type) {
			/* disable gamma */
			set_reg(gamma_base + GAMA_EN, 0x0, 1, 0);
			/* disable gmp */
			set_reg(gmp_base + GMP_EN, 0x0, 1, 0);
			/* disable xcc */
			set_reg(xcc_base + LCP_XCC_BYPASS_EN, 0x1, 1, 0);

			gamma_config_flag = 1;
			last_gamma_type = dpufd->panel_info.gamma_type;
		}
		return;
	}

	if ((gamma_config_flag == 1) && (dpu_dpp_gamma_para_set_reg(dpufd, last_gamma_type) != 0))
		return;

	gama_lut_sel = (uint32_t)inp32(gamma_base + GAMA_LUT_SEL);
	set_reg(gamma_base + GAMA_LUT_SEL, (~(gama_lut_sel & 0x1)) & 0x1, 1, 0);

	/* enable gamma */
	set_reg(gamma_base + GAMA_EN, 0x1, 1, 0);
	/* enable gmp */
	set_reg(gmp_base + GMP_EN, 0x1, 1, 0);
	/* enable xcc */
	set_reg(xcc_base + LCP_XCC_BYPASS_EN, 0x0, 1, 0);
	gamma_config_flag = 0;
}
