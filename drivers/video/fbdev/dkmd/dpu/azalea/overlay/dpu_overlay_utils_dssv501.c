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

	outp32(dpufd->noc_dss_base + 0x8c, 0x1);
	outp32(dpufd->noc_dss_base + 0x90, 0x1000);
	outp32(dpufd->noc_dss_base + 0x94, 0x20);
	outp32(dpufd->noc_dss_base + 0x98, 0x1);

	outp32(dpufd->noc_dss_base + 0x10c, 0x1);
	outp32(dpufd->noc_dss_base + 0x110, 0x1000);
	outp32(dpufd->noc_dss_base + 0x114, 0x20);
	outp32(dpufd->noc_dss_base + 0x118, 0x1);

	outp32(dpufd->noc_dss_base + 0x18c, 0x1);
	outp32(dpufd->noc_dss_base + 0x190, 0x1000);
	outp32(dpufd->noc_dss_base + 0x194, 0x20);
	outp32(dpufd->noc_dss_base + 0x198, 0x1);
}

static int dpu_check_wblayer_buff(struct dpu_fb_data_type *dpufd, dss_wb_layer_t *wb_layer)
{
	if (dpu_check_addr_validate(&wb_layer->dst))
		return 0;

	if (wb_layer->need_cap & (CAP_BASE | CAP_DIM | CAP_PURE_COLOR))
		return 0;

	return -EINVAL;
}

static int dpu_check_wblayer_rect(struct dpu_fb_data_type *dpufd, dss_wb_layer_t *wb_layer)
{
	dpu_check_and_return((wb_layer->src_rect.x < 0 || wb_layer->src_rect.y < 0 ||
		wb_layer->src_rect.w <= 0 || wb_layer->src_rect.h <= 0), -EINVAL, ERR,
		"fb%d, src_rect[%d, %d, %d, %d] is out of range!\n",
		dpufd->index, wb_layer->src_rect.x, wb_layer->src_rect.y,
		wb_layer->src_rect.w, wb_layer->src_rect.h);

	dpu_check_and_return((wb_layer->dst_rect.x < 0 || wb_layer->dst_rect.y < 0 ||
		wb_layer->dst_rect.w <= 0 || wb_layer->dst_rect.h <= 0 ||
		wb_layer->dst_rect.w > wb_layer->dst.width || wb_layer->dst_rect.h > wb_layer->dst.height),
		-EINVAL, ERR, "fb%d, dst_rect[%d, %d, %d, %d], dst[%d, %d] is out of range!\n",
		dpufd->index, wb_layer->dst_rect.x, wb_layer->dst_rect.y, wb_layer->dst_rect.w,
		wb_layer->dst_rect.h, wb_layer->dst.width, wb_layer->dst.height);

	return 0;
}

static int dpu_check_userdata_base(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req, dss_overlay_block_t *pov_h_block_infos)
{
	dpu_check_and_return(((pov_req->ov_block_nums <= 0) || (pov_req->ov_block_nums > DPU_OV_BLOCK_NUMS)),
		-EINVAL, ERR, "fb%d, invalid ov_block_nums=%d!\n", dpufd->index, pov_req->ov_block_nums);

	for (int i = 0; i < pov_req->ov_block_nums; ++i) {
		if (pov_h_block_infos[i].layer_nums <= 0 ||
			pov_h_block_infos[i].layer_nums > OVL_LAYER_NUM_MAX) {
			DPU_FB_ERR("fb%d, invalid layer_nums=%d!",
				dpufd->index, pov_h_block_infos[i].layer_nums);
			return -EINVAL;
		}
	}

	dpu_check_and_return(((pov_req->ovl_idx < 0) || (pov_req->ovl_idx >= DSS_OVL_IDX_MAX)), -EINVAL, ERR,
		"fb%d, invalid ovl_idx=%d!\n", dpufd->index, pov_req->ovl_idx);

	return 0;
}

static int dpu_check_userdata_dst(dss_wb_layer_t *wb_layer, uint32_t index)
{
	if (wb_layer->need_cap & CAP_AFBCE)
		dpu_check_and_return(((wb_layer->dst.afbc_header_stride == 0) ||
			(wb_layer->dst.afbc_payload_stride == 0)), -EINVAL, ERR,
			"fb%d, afbc_header_stride=%d, afbc_payload_stride=%d is invalid!\n",
			index, wb_layer->dst.afbc_header_stride, wb_layer->dst.afbc_payload_stride);

	if (wb_layer->need_cap & CAP_HFBCE) {
		if ((wb_layer->dst.hfbc_header_stride0 == 0) ||
			(wb_layer->dst.hfbc_payload_stride0 == 0) ||
			(wb_layer->dst.hfbc_header_stride1 == 0) ||
			(wb_layer->dst.hfbc_payload_stride1 == 0) ||
			(wb_layer->chn_idx != DSS_WCHN_W1)) {
			DPU_FB_ERR("fb%d, hfbc_header_stride0=%d, hfbc_payload_stride0=%d,"
				"hfbc_header_stride1=%d, hfbc_payload_stride1=%d is "
				"invalid or wchn_idx=%d no support hfbce!\n",
				index, wb_layer->dst.hfbc_header_stride0,
				wb_layer->dst.hfbc_payload_stride0,
				wb_layer->dst.hfbc_header_stride1,
				wb_layer->dst.hfbc_payload_stride1,
				wb_layer->chn_idx);
			return -EINVAL;
		}
	}

	return 0;
}

static int dpu_check_dirty_rect(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req)
{
	if (dpufd->index == PRIMARY_PANEL_IDX) {
		if (dpufd->panel_info.dirty_region_updt_support)
			dpu_check_and_return((pov_req->dirty_rect.x < 0 || pov_req->dirty_rect.y < 0 ||
				pov_req->dirty_rect.w < 0 || pov_req->dirty_rect.h < 0), -EINVAL, ERR,
				"dirty_rect[%d, %d, %d, %d] is out of range!\n",
				pov_req->dirty_rect.x, pov_req->dirty_rect.y,
				pov_req->dirty_rect.w, pov_req->dirty_rect.h);
	}

	return 0;
}

static int dpu_check_ov_info(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req)
{
	dpu_check_and_return(((pov_req->wb_layer_nums <= 0) ||
		(pov_req->wb_layer_nums > MAX_DSS_DST_NUM)), -EINVAL, ERR,
		"fb%d, invalid wb_layer_nums=%d!\n",
		dpufd->index, pov_req->wb_layer_nums);

	dpu_check_and_return((pov_req->wb_ov_rect.x < 0 || pov_req->wb_ov_rect.y < 0), -EINVAL, ERR,
		"wb_ov_rect[%d, %d] is out of range!\n",
		pov_req->wb_ov_rect.x, pov_req->wb_ov_rect.y);

	dpu_check_and_return((pov_req->wb_compose_type >= DSS_WB_COMPOSE_TYPE_MAX), -EINVAL, ERR,
		"wb_compose_type=%u is invalid!\n", pov_req->wb_compose_type);

	return 0;
}

static bool is_wb_layer_size_info_empty(dss_wb_layer_t *wb_layer)
{
	return ((wb_layer->dst.bpp == 0) || (wb_layer->dst.width == 0) || (wb_layer->dst.height == 0) ||
		((wb_layer->dst.stride == 0) && (!(wb_layer->need_cap & CAP_HFBCE))));
}

static int dpu_check_layer_afbc_info(struct dpu_fb_data_type *dpufd,
	dss_wb_layer_t *wb_layer)
{
	if (!(wb_layer->need_cap & CAP_HFBCE)) {
		if (is_yuv_semiplanar(wb_layer->dst.format) || is_yuv_plane(wb_layer->dst.format))
			dpu_check_and_return(((wb_layer->dst.stride_plane1 == 0) ||
				(wb_layer->dst.offset_plane1 == 0)), -EINVAL, ERR,
				"fb%d, stride_plane1=%d, offset_plane1=%d is invalid!\n",
				dpufd->index, wb_layer->dst.stride_plane1, wb_layer->dst.offset_plane1);

		if (is_yuv_plane(wb_layer->dst.format))
			dpu_check_and_return(((wb_layer->dst.stride_plane2 == 0) ||
				(wb_layer->dst.offset_plane2 == 0)), -EINVAL, ERR,
				"fb%d, stride_plane2=%d, offset_plane2=%d is invalid!\n",
				dpufd->index, wb_layer->dst.stride_plane2, wb_layer->dst.offset_plane2);
	}

	return 0;
}

static int dpu_check_wb_layer_info(struct dpu_fb_data_type *dpufd,
	dss_wb_layer_t *wb_layer)
{
	if (wb_layer->chn_idx != DSS_WCHN_W2)
		dpu_check_and_return((wb_layer->chn_idx < DSS_WCHN_W0 || wb_layer->chn_idx > DSS_WCHN_W1),
			-EINVAL, ERR, "fb%d, wchn_idx=%d is invalid!\n", dpufd->index, wb_layer->chn_idx);


	dpu_check_and_return((wb_layer->dst.format >= DPU_FB_PIXEL_FORMAT_MAX), -EINVAL, ERR,
		"fb%d, format=%d is invalid!\n", dpufd->index, wb_layer->dst.format);

	dpu_check_and_return((is_wb_layer_size_info_empty(wb_layer)), -EINVAL, ERR,
		"fb%d, bpp=%d, width=%d, height=%d, stride=%d is invalid!\n",
		dpufd->index, wb_layer->dst.bpp, wb_layer->dst.width, wb_layer->dst.height,
		wb_layer->dst.stride);

	dpu_check_and_return((dpu_check_wblayer_buff(dpufd, wb_layer)), -EINVAL, ERR,
		"fb%d, failed to check_wblayer_buff!\n", dpufd->index);

	if (dpu_check_layer_afbc_info(dpufd, wb_layer) < 0)
		return -EINVAL;

	if (dpu_check_userdata_dst(wb_layer, dpufd->index) < 0)
		return -EINVAL;

	dpu_check_and_return((wb_layer->dst.csc_mode >= DSS_CSC_MOD_MAX), -EINVAL, ERR,
		"fb%d, csc_mode=%d is invalid!\n", dpufd->index, wb_layer->dst.csc_mode);

	dpu_check_and_return((wb_layer->dst.afbc_scramble_mode >= DSS_AFBC_SCRAMBLE_MODE_MAX),
		-EINVAL, ERR, "fb%d, afbc_scramble_mode=%d is invalid!\n",
		dpufd->index, wb_layer->dst.afbc_scramble_mode);

	dpu_check_and_return((dpu_check_wblayer_rect(dpufd, wb_layer)), -EINVAL, ERR,
		"fb%d, failed to check_wblayer_rect!\n", dpufd->index);

	return 0;
}

int dpu_check_userdata(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req, dss_overlay_block_t *pov_h_block_infos)
{
	int i;
	dss_wb_layer_t *wb_layer = NULL;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "invalid dpufd!\n");
	dpu_check_and_return(!pov_req, -EINVAL, ERR, "fb%d, invalid pov_req!\n", dpufd->index);
	dpu_check_and_return(!pov_h_block_infos, -EINVAL, ERR,
		"fb%d, invalid pov_h_block_infos!\n", dpufd->index);

	if (dpu_check_userdata_base(dpufd, pov_req, pov_h_block_infos) < 0)
		return -EINVAL;

	if (dpu_check_dirty_rect(dpufd, pov_req) < 0)
		return -EINVAL;

	if (dpufd->index == AUXILIARY_PANEL_IDX) {
		dpu_check_and_return((pov_req->wb_enable != 1), -EINVAL, ERR,
			"pov_req->wb_enable=%u is invalid!\n", pov_req->wb_enable);

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

static int dpu_check_layer_buff(struct dpu_fb_data_type *dpufd, dss_layer_t *layer)
{
	if (dpu_check_layer_addr_validate(layer))
		return 0;

	if (layer->need_cap & (CAP_BASE | CAP_DIM | CAP_PURE_COLOR))
		return 0;

	return -EINVAL;
}

static int dpu_check_layer_rect(dss_layer_t *layer)
{
	dpu_check_and_return((layer->src_rect.x < 0 || layer->src_rect.y < 0 ||
		layer->src_rect.w <= 0 || layer->src_rect.h <= 0), -EINVAL, ERR,
		"src_rect[%d, %d, %d, %d] is out of range!\n",
		layer->src_rect.x, layer->src_rect.y, layer->src_rect.w, layer->src_rect.h);

	dpu_check_and_return((layer->src_rect_mask.x < 0 || layer->src_rect_mask.y < 0 ||
		layer->src_rect_mask.w < 0 || layer->src_rect_mask.h < 0), -EINVAL, ERR,
		"src_rect_mask[%d, %d, %d, %d] is out of range!\n",
		layer->src_rect_mask.x, layer->src_rect_mask.y,
		layer->src_rect_mask.w, layer->src_rect_mask.h);

	dpu_check_and_return((layer->dst_rect.x < 0 || layer->dst_rect.y < 0 ||
		layer->dst_rect.w <= 0 || layer->dst_rect.h <= 0), -EINVAL, ERR,
		"dst_rect[%d, %d, %d, %d] is out of range!\n",
		layer->dst_rect.x, layer->dst_rect.y, layer->dst_rect.w, layer->dst_rect.h);

	return 0;
}

static int dpu_check_layer_par_need_cap(dss_layer_t *layer, uint32_t index)
{
	if (layer->need_cap & CAP_AFBCD)
		dpu_check_and_return(((layer->img.afbc_header_stride == 0) ||
			(layer->img.afbc_payload_stride == 0) || (layer->img.mmbuf_size == 0)), -EINVAL, ERR,
			"fb%d, afbc_header_stride=%d, afbc_payload_stride=%d, mmbuf_size=%d is invalid!\n",
			index, layer->img.afbc_header_stride,
			layer->img.afbc_payload_stride, layer->img.mmbuf_size);

	if (layer->need_cap & CAP_HFBCD)
		dpu_check_and_return(
			((layer->img.hfbc_header_stride0 == 0) || (layer->img.hfbc_payload_stride0 == 0) ||
			(layer->img.hfbc_header_stride1 == 0) || (layer->img.hfbc_payload_stride1 == 0) ||
			((layer->chn_idx != DSS_RCHN_V0) && (layer->chn_idx != DSS_RCHN_V1))), -EINVAL, ERR,
			"fb%d, hfbc_header_stride0=%d, hfbc_payload_stride0=%d, "
			"hfbc_header_stride1=%d, hfbc_payload_stride1=%d is invalid or chn_idx=%d "
			"no support hfbcd!\n",
			index, layer->img.hfbc_header_stride0, layer->img.hfbc_payload_stride0,
			layer->img.hfbc_header_stride1, layer->img.hfbc_payload_stride1, layer->chn_idx);

	return 0;
}

static int dpu_check_layer_ch_valid(struct dpu_fb_data_type *dpufd, dss_layer_t *layer)
{
	if (dpufd->index == AUXILIARY_PANEL_IDX) {
		if (layer->chn_idx != DSS_RCHN_V2)
			dpu_check_and_return((layer->chn_idx < 0 || layer->chn_idx >= DSS_WCHN_W0), -EINVAL,
				ERR, "fb%d, rchn_idx=%d is invalid!\n", dpufd->index, layer->chn_idx);

		dpu_check_and_return((layer->chn_idx == DSS_RCHN_D2), -EINVAL, ERR,
			"fb%d, chn_idx[%d] does not used by offline play!\n",
			dpufd->index, layer->chn_idx);
	} else {
		dpu_check_and_return((layer->chn_idx < 0 || layer->chn_idx >= DSS_CHN_MAX_DEFINE),
			-EINVAL, ERR, "fb%d, rchn_idx=%d is invalid!\n", dpufd->index, layer->chn_idx);
	}

	return 0;
}

static bool is_layer_basic_size_info_empty(dss_layer_t *layer)
{
	return ((layer->img.bpp == 0) || (layer->img.width == 0) || (layer->img.height == 0) ||
		((layer->img.stride == 0) && (!(layer->need_cap & CAP_HFBCD))));
}

static int dpu_check_layer_basic_param_valid(struct dpu_fb_data_type *dpufd,
	dss_layer_t *layer)
{
	dpu_check_and_return((layer->blending < 0 || layer->blending >= DPU_FB_BLENDING_MAX),
		-EINVAL, ERR, "fb%d, blending=%d is invalid!\n", dpufd->index, layer->blending);

	dpu_check_and_return((layer->img.format >= DPU_FB_PIXEL_FORMAT_MAX),
		-EINVAL, ERR, "fb%d, format=%d is invalid!\n", dpufd->index, layer->img.format);

	dpu_check_and_return((is_layer_basic_size_info_empty(layer)), -EINVAL, ERR,
		"fb%d, bpp=%d, width=%d, height=%d, stride=%d is invalid!\n",
		dpufd->index, layer->img.bpp, layer->img.width, layer->img.height, layer->img.stride);

	dpu_check_and_return((dpu_check_layer_buff(dpufd, layer)), -EINVAL, ERR,
		"fb%d, failed to check_layer_buff!\n", dpufd->index);

	if (!(layer->need_cap & CAP_HFBCD)) {
		if (is_yuv_semiplanar(layer->img.format) || is_yuv_plane(layer->img.format))
			dpu_check_and_return(((layer->img.stride_plane1 == 0) ||
				(layer->img.offset_plane1 == 0)), -EINVAL, ERR,
				"fb%d, stride_plane1=%d, offset_plane1=%d is invalid!\n",
				dpufd->index, layer->img.stride_plane1, layer->img.offset_plane1);

		if (is_yuv_plane(layer->img.format))
			dpu_check_and_return(((layer->img.stride_plane2 == 0) ||
				(layer->img.offset_plane2 == 0)), -EINVAL, ERR,
				"fb%d, stride_plane2=%d, offset_plane2=%d is invalid!\n",
				dpufd->index, layer->img.stride_plane2, layer->img.offset_plane2);
	}

	return 0;
}

static int dpu_check_layer_rot_cap(struct dpu_fb_data_type *dpufd, dss_layer_t *layer)
{
	if (layer->transform & DPU_FB_TRANSFORM_ROT_90) {
		if (((layer->chn_idx == DSS_RCHN_V0) || (layer->chn_idx == DSS_RCHN_V1)) &&
			(layer->need_cap & CAP_HFBCD)) {
			DPU_FB_DEBUG("fb%d, ch%d,need_cap=%d, need rot and hfbcd!\n!",
				dpufd->index, layer->chn_idx, layer->need_cap);
		} else {
			DPU_FB_ERR("fb%d, ch%d is not support need_cap=%d,transform=%d!\n",
				dpufd->index, layer->chn_idx, layer->need_cap, layer->transform);
			return -EINVAL;
		}
	}

	return 0;
}

int dpu_check_layer_par(struct dpu_fb_data_type *dpufd, dss_layer_t *layer)
{
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL, return!\n");
	dpu_check_and_return(!layer, -EINVAL, ERR, "layer is NULL, return!\n");

	dpu_check_and_return((layer->layer_idx < 0 || layer->layer_idx >= OVL_LAYER_NUM_MAX),
		-EINVAL, ERR, "fb%d, layer_idx=%d is invalid!\n", dpufd->index, layer->layer_idx);

	if (layer->need_cap & (CAP_BASE | CAP_DIM | CAP_PURE_COLOR))
		return 0;

	if (dpu_check_layer_ch_valid(dpufd, layer) < 0)
		return -EINVAL;

	if (dpu_check_layer_basic_param_valid(dpufd, layer) < 0)
		return -EINVAL;

	if (dpu_check_layer_par_need_cap(layer, dpufd->index) < 0)
		return -EINVAL;

	if (dpu_check_layer_rot_cap(dpufd, layer) < 0)
		return -EINVAL;

	dpu_check_and_return((layer->img.csc_mode >= DSS_CSC_MOD_MAX), -EINVAL, ERR,
		"fb%d, csc_mode=%d is invalid!\n", dpufd->index, layer->img.csc_mode);

	dpu_check_and_return((layer->img.afbc_scramble_mode >= DSS_AFBC_SCRAMBLE_MODE_MAX),
		-EINVAL, ERR, "fb%d, afbc_scramble_mode=%d is invalid!\n",
		dpufd->index, layer->img.afbc_scramble_mode);

	dpu_check_and_return(((layer->layer_idx != 0) && (layer->need_cap & CAP_BASE)),
		-EINVAL, ERR, "fb%d, layer%d is not base!\n", dpufd->index, layer->layer_idx);

	dpu_check_and_return((dpu_check_layer_rect(layer)), -EINVAL, ERR,
		"fb%d, failed to check_layer_rect!\n", dpufd->index);

	return 0;
}

/*
 * DSS disreset
 */
void dpufb_dss_disreset(struct dpu_fb_data_type *dpufd)
{
	(void)dpufd;
}

static void config_reg_use_gma_lut_param(char __iomem *gamma_lut_base, struct dpu_panel_info *pinfo,
	uint32_t *local_gamma_lut_table_r, uint32_t *local_gamma_lut_table_g, uint32_t *local_gamma_lut_table_b)
{
	uint32_t index;
	uint32_t i;

	/* config regsiter use default or cinema parameter */
	for (index = 0; index < pinfo->gamma_lut_table_len / 2; index++) {
		i = index << 1;
		outp32(gamma_lut_base + (U_GAMA_R_COEF + index * 4),
			(local_gamma_lut_table_r[i] | (local_gamma_lut_table_r[i + 1] << 16)));
		outp32(gamma_lut_base + (U_GAMA_G_COEF + index * 4),
			(local_gamma_lut_table_g[i] | (local_gamma_lut_table_g[i + 1] << 16)));
		outp32(gamma_lut_base + (U_GAMA_B_COEF + index * 4),
			(local_gamma_lut_table_b[i] | (local_gamma_lut_table_b[i + 1] << 16)));
		/* GAMA  PRE LUT */
		outp32(gamma_lut_base + (U_GAMA_PRE_R_COEF + index * 4),
			(local_gamma_lut_table_r[i] | (local_gamma_lut_table_r[i + 1] << 16)));
		outp32(gamma_lut_base + (U_GAMA_PRE_G_COEF + index * 4),
			(local_gamma_lut_table_g[i] | (local_gamma_lut_table_g[i + 1] << 16)));
		outp32(gamma_lut_base + (U_GAMA_PRE_B_COEF + index * 4),
			(local_gamma_lut_table_b[i] | (local_gamma_lut_table_b[i + 1] << 16)));
	}
}

static int dpu_dpp_gamma_para_set_reg(struct dpu_fb_data_type *dpufd, uint8_t last_gamma_type)
{
	uint32_t *local_gamma_lut_table_r = NULL;
	uint32_t *local_gamma_lut_table_g = NULL;
	uint32_t *local_gamma_lut_table_b = NULL;
	char __iomem *gamma_lut_base = dpufd->dss_base + DSS_DPP_GAMA_LUT_OFFSET;
	char __iomem *gamma_base = dpufd->dss_base + DSS_DPP_GAMA_OFFSET;
	struct dpu_panel_info *pinfo = &(dpufd->panel_info);
	uint32_t gama_lut_sel;

	if (last_gamma_type == 1) {
		/* set gamma cinema parameter */
		if (pinfo->cinema_gamma_lut_table_len > 0 && pinfo->cinema_gamma_lut_table_R &&
			pinfo->cinema_gamma_lut_table_G && pinfo->cinema_gamma_lut_table_B) {
			local_gamma_lut_table_r = pinfo->cinema_gamma_lut_table_R;
			local_gamma_lut_table_g = pinfo->cinema_gamma_lut_table_G;
			local_gamma_lut_table_b = pinfo->cinema_gamma_lut_table_B;
		} else {
			DPU_FB_ERR("can't get gamma cinema paramter from pinfo.\n");
			return -EINVAL;
		}
	} else {
		if (pinfo->gamma_lut_table_len > 0 && pinfo->gamma_lut_table_R &&
			pinfo->gamma_lut_table_G && pinfo->gamma_lut_table_B) {
			local_gamma_lut_table_r = pinfo->gamma_lut_table_R;
			local_gamma_lut_table_g = pinfo->gamma_lut_table_G;
			local_gamma_lut_table_b = pinfo->gamma_lut_table_B;
		} else {
			DPU_FB_ERR("can't get gamma normal parameter from pinfo.\n");
			return -EINVAL;
		}
	}

	config_reg_use_gma_lut_param(gamma_lut_base, pinfo, local_gamma_lut_table_r,
		local_gamma_lut_table_g, local_gamma_lut_table_b);

	outp32(gamma_lut_base + U_GAMA_R_LAST_COEF, local_gamma_lut_table_r[pinfo->gamma_lut_table_len - 1]);
	outp32(gamma_lut_base + U_GAMA_G_LAST_COEF, local_gamma_lut_table_g[pinfo->gamma_lut_table_len - 1]);
	outp32(gamma_lut_base + U_GAMA_B_LAST_COEF, local_gamma_lut_table_b[pinfo->gamma_lut_table_len - 1]);
	/* GAMA  PRE LUT */
	outp32(gamma_lut_base + U_GAMA_PRE_R_LAST_COEF, local_gamma_lut_table_r[pinfo->gamma_lut_table_len - 1]);
	outp32(gamma_lut_base + U_GAMA_PRE_G_LAST_COEF, local_gamma_lut_table_g[pinfo->gamma_lut_table_len - 1]);
	outp32(gamma_lut_base + U_GAMA_PRE_B_LAST_COEF, local_gamma_lut_table_b[pinfo->gamma_lut_table_len - 1]);

	gama_lut_sel = (uint32_t)inp32(gamma_base + GAMA_LUT_SEL);
	set_reg(gamma_base + GAMA_LUT_SEL, (~(gama_lut_sel & 0x1)) & 0x1, 1, 0);

	return 0;
}

void dpu_dpp_acm_gm_set_reg(struct dpu_fb_data_type *dpufd)
{
	char __iomem *gmp_base = NULL;
	char __iomem *xcc_base = NULL;
	char __iomem *gamma_base = NULL;
	static uint8_t last_gamma_type;
	static uint32_t gamma_config_flag;

	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL\n");
	if (dpufd->panel_info.gamma_support == 0)
		return;

	dpu_check_and_no_retval((!DPU_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_GAMA)), DEBUG,
		"gamma is not suppportted in this platform.\n");

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
			set_reg(xcc_base + XCC_EN, 0x1, 1, 0);
			gamma_config_flag = 1;
			last_gamma_type = dpufd->panel_info.gamma_type;
		}
		return;
	}

	if ((gamma_config_flag == 1) && (dpu_dpp_gamma_para_set_reg(dpufd, last_gamma_type) != 0))
		return;

	/* enable gamma */
	set_reg(gamma_base + GAMA_EN, 0x1, 1, 0);

	/* enable gmp */
	set_reg(gmp_base + GMP_EN, 0x1, 1, 0);
	/* enable xcc */
	set_reg(xcc_base + XCC_EN, 0x1, 1, 0);
	gamma_config_flag = 0;
}

static void dpu_dump_vote_info(struct dpu_fb_data_type *dpufd)
{
	struct dpu_fb_data_type *fb0 = dpufd_list[PRIMARY_PANEL_IDX];
	struct dpu_fb_data_type *fb1 = dpufd_list[EXTERNAL_PANEL_IDX];
	struct dpu_fb_data_type *fb2 = dpufd_list[AUXILIARY_PANEL_IDX];
	struct dpu_fb_data_type *fb3 = dpufd_list[MEDIACOMMON_PANEL_IDX];

	DPU_FB_INFO("fb%d, voltage[%d, %d, %d, %d], edc[%llu, %llu, %llu, %llu]\n", dpufd->index,
		(fb0 != NULL) ? fb0->dss_vote_cmd.dss_voltage_level : 0,
		(fb1 != NULL) ? fb1->dss_vote_cmd.dss_voltage_level : 0,
		(fb2 != NULL) ? fb2->dss_vote_cmd.dss_voltage_level : 0,
		(fb3 != NULL) ? fb3->dss_vote_cmd.dss_voltage_level : 0,
		(fb0 != NULL) ? fb0->dss_vote_cmd.dss_pri_clk_rate : 0,
		(fb1 != NULL) ? fb1->dss_vote_cmd.dss_pri_clk_rate : 0,
		(fb2 != NULL) ? fb2->dss_vote_cmd.dss_pri_clk_rate : 0,
		(fb3 != NULL) ? fb3->dss_vote_cmd.dss_pri_clk_rate : 0);
}

static void dpu_dump_arsr_post_info(struct dpu_fb_data_type *dpufd)
{
	DPU_FB_INFO("arsr_post: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
		inp32(dpufd->dss_module.post_scf_base + ARSR_POST_MODE),
		inp32(dpufd->dss_base + DSS_TOP_OFFSET + DPP_IMG_SIZE_BEF_SR),
		inp32(dpufd->dss_base + DSS_TOP_OFFSET + DPP_IMG_SIZE_AFT_SR),
		inp32(dpufd->dss_module.post_scf_base + ARSR_POST_IHLEFT),
		inp32(dpufd->dss_module.post_scf_base + ARSR_POST_IHLEFT1),
		inp32(dpufd->dss_module.post_scf_base + ARSR_POST_IHRIGHT),
		inp32(dpufd->dss_module.post_scf_base + ARSR_POST_IHRIGHT1),
		inp32(dpufd->dss_module.post_scf_base + ARSR_POST_IVTOP),
		inp32(dpufd->dss_module.post_scf_base + ARSR_POST_IVBOTTOM),
		inp32(dpufd->dss_module.post_scf_base + ARSR_POST_IHINC),
		inp32(dpufd->dss_module.post_scf_base + ARSR_POST_IVINC));
}

static void dpu_dump_dpp_info(struct dpu_fb_data_type *dpufd)
{
	DPU_FB_INFO("dpp: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
		inp32(dpufd->dss_base + DSS_DPP_GAMA_OFFSET + GAMA_EN),
		inp32(dpufd->dss_base + DSS_DPP_DEGAMMA_OFFSET + DEGAMA_EN),
		inp32(dpufd->dss_base + DSS_DPP_GMP_OFFSET + GMP_EN),
		inp32(dpufd->dss_base + DSS_DPP_XCC_OFFSET + XCC_EN),
		inp32(dpufd->dss_base + DSS_HI_ACE_OFFSET + DPE_BYPASS_ACE),
		inp32(dpufd->dss_base + DSS_HI_ACE_OFFSET + DPE_BYPASS_NR),
		inp32(dpufd->dss_base + DSS_HI_ACE_OFFSET + DPE_IMAGE_INFO),
		inp32(dpufd->dss_base + DSS_DPP_DITHER_OFFSET + DITHER_CTL0));
}

static void dpu_dump_mediacrg_info(struct dpu_fb_data_type *dpufd)
{
	DPU_FB_INFO("Mediacrg CLKDIV: 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",
		inp32(dpufd->media_crg_base + MEDIA_CLKDIV1 + 0 * 4),
		inp32(dpufd->media_crg_base + MEDIA_CLKDIV1 + 1 * 4),
		inp32(dpufd->media_crg_base + MEDIA_CLKDIV1 + 2 * 4),
		inp32(dpufd->media_crg_base + MEDIA_CLKDIV1 + 3 * 4),
		inp32(dpufd->media_crg_base + MEDIA_CLKDIV1 + 4 * 4),
		inp32(dpufd->media_crg_base + MEDIA_CLKDIV1 + 5 * 4),
		inp32(dpufd->media_crg_base + MEDIA_CLKDIV1 + 6 * 4),
		inp32(dpufd->media_crg_base + MEDIA_CLKDIV1 + 7 * 4),
		inp32(dpufd->media_crg_base + MEDIA_CLKDIV1 + 8 * 4),
		inp32(dpufd->media_crg_base + MEDIA_CLKDIV1 + 9 * 4));
}

void dpu_dump_current_info(struct dpu_fb_data_type *dpufd)
{
	int i;

	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL!\n");
	dpu_check_and_no_retval(!dpufd->dss_base, ERR, "dpufd->dss_base is NULL!\n");
	dpu_check_and_no_retval(!dpufd->pctrl_base, ERR, "dpufd->pctrl_base is NULL!\n");
	dpu_check_and_no_retval(!dpufd->pmctrl_base, ERR, "dpufd->pmctrl_base is NULL!\n");
	dpu_check_and_no_retval(!dpufd->dss_module.post_scf_base, ERR, "dpufd->dss_module.post_scf_base is NULL!\n");
	dpu_check_and_no_retval(!dpufd->mipi_dsi0_base, ERR, "dpufd->mipi_dsi0_base is NULL!\n");
	dpu_check_and_no_retval(!dpufd->media_crg_base, ERR, "dpufd->media_crg_base is NULL!\n");

	if (dpufd->index != PRIMARY_PANEL_IDX)
		return;

	for (i = 0; i < DSS_CHN_MAX_DEFINE; i++) {
		if (g_dss_module_base[i][MODULE_DMA] == 0)
			continue;
		DPU_FB_INFO("chn%d DMA_BUF_DBG0=0x%x, DMA_BUF_DBG1=0x%x\n", i,
			inp32(dpufd->dss_base + g_dss_module_base[i][MODULE_DMA] + DMA_BUF_DBG0),
			inp32(dpufd->dss_base + g_dss_module_base[i][MODULE_DMA] + DMA_BUF_DBG1));
	}

	DPU_FB_INFO("AIF0 status: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
		inp32(dpufd->dss_base + AIF0_MONITOR_OS_R0),
		inp32(dpufd->dss_base + AIF0_MONITOR_OS_R1),
		inp32(dpufd->dss_base + AIF0_MONITOR_OS_R2),
		inp32(dpufd->dss_base + AIF0_MONITOR_OS_R3),
		inp32(dpufd->dss_base + AIF0_MONITOR_OS_R4));

	DPU_FB_INFO("PERI_STAT: 0x%x, 0x%x, 0x%x\n",
		inp32(dpufd->pctrl_base + PERI_STAT0),
		inp32(dpufd->pmctrl_base + NOC_POWER_IDLEREQ),
		inp32(dpufd->pmctrl_base + NOC_POWER_IDLEACK));

	dpu_dump_vote_info(dpufd);
	dpu_dump_arsr_post_info(dpufd);
	dpu_dump_dpp_info(dpufd);

	DPU_FB_INFO("ov:0x%x,dbuf:0x%x,0x%x,dsc:0x%x\n",
		inp32(dpufd->dss_base + DSS_OVL0_OFFSET + OV_SIZE),
		inp32(dpufd->dss_base + DSS_DBUF0_OFFSET + DBUF_FRM_SIZE),
		inp32(dpufd->dss_base + DSS_DBUF0_OFFSET + DBUF_FRM_HSIZE),
		inp32(dpufd->dss_base + DSS_DSC_OFFSET + DSC_PIC_SIZE));

	DPU_FB_INFO("ldi-dsi:0x%x,0x%x\n",
		inp32(dpufd->mipi_dsi0_base + MIPIDSI_EDPI_CMD_SIZE_OFFSET),
		inp32(dpufd->mipi_dsi0_base + MIPIDSI_VID_VACTIVE_LINES_OFFSET));

	DPU_FB_INFO("dpi0_hsize:0x%x, vsize:0x%x, dpi1_hsize:0x%x, overlap_size:0x%x\n",
		inp32(dpufd->dss_base + DSS_LDI0_OFFSET + LDI_DPI0_HRZ_CTRL2),
		inp32(dpufd->dss_base + DSS_LDI0_OFFSET + LDI_VRT_CTRL2),
		inp32(dpufd->dss_base + DSS_LDI0_OFFSET + LDI_DPI1_HRZ_CTRL2),
		inp32(dpufd->dss_base + DSS_LDI0_OFFSET + LDI_OVERLAP_SIZE));

	dpu_dump_mediacrg_info(dpufd);

	DPU_FB_INFO("PMCTRL PERI_CTRL4, CTRL5 = 0x%x, 0x%x\n",
		inp32(dpufd->pmctrl_base + PMCTRL_PERI_CTRL4),
		inp32(dpufd->pmctrl_base + PMCTRL_PERI_CTRL5));
}

