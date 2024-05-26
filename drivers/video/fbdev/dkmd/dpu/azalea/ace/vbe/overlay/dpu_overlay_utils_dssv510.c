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

#include "dpu_overlay_utils.h"
#include "../dpu_mmbuf_manager.h"
#include "dpu_mipi_dsi.h"
#include "dpu_fb_debug.h"
#include "dpu_fb_panel.h"
#include "dpu_dpe_utils.h"

#define RCH_SWITCH_BIT_SHIFT 4
#define RCH_OV_BIND_DEFAULT 0xF

struct timediff_info {
	struct timeval tv0;
	struct timeval tv1;
};

uint32_t g_fpga_flag;
void *g_smmu_rwerraddr_virt;


void dpu_qos_on(struct dpu_fb_data_type *dpufd)
{
	if (!dpufd)
		return;
}


static int dpu_check_wblayer_buff(
	struct dpu_fb_data_type *dpufd, dss_wb_layer_t *wb_layer)
{
	void_unused(dpufd);
	if (wb_layer->need_cap & (CAP_BASE | CAP_DIM | CAP_PURE_COLOR))
		return 0;

	return -EINVAL;
}

static int dpu_check_wblayer_rect(
	struct dpu_fb_data_type *dpufd, dss_wb_layer_t *wb_layer)
{
	if (wb_layer->src_rect.x < 0 || wb_layer->src_rect.y < 0 ||
		wb_layer->src_rect.w <= 0 || wb_layer->src_rect.h <= 0) {
		DPU_FB_ERR("fb%d, src_rect %d, %d, %d, %d is out of range!\n",
			dpufd->index, wb_layer->src_rect.x, wb_layer->src_rect.y,
			wb_layer->src_rect.w, wb_layer->src_rect.h);
		return -EINVAL;
	}

	if (wb_layer->dst_rect.x < 0 || wb_layer->dst_rect.y < 0 ||
		wb_layer->dst_rect.w <= 0 || wb_layer->dst_rect.h <= 0 ||
		wb_layer->dst_rect.w > (int32_t)(wb_layer->dst.width) ||  /*lint !e574*/
		wb_layer->dst_rect.h > (int32_t)(wb_layer->dst.height)) {  /*lint !e574*/
		DPU_FB_ERR("fb%d, dst_rect %d, %d, %d, %d, dst %d, %d is out of range!\n",
			dpufd->index, wb_layer->dst_rect.x,
			wb_layer->dst_rect.y, wb_layer->dst_rect.w,
			wb_layer->dst_rect.h, wb_layer->dst.width, wb_layer->dst.height);
		return -EINVAL;
	}

	return 0;
}

static int dpu_check_userdata_base(struct dpu_fb_data_type *dpufd,
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

	if ((pov_req->ovl_idx < 0) || (pov_req->ovl_idx >= DSS_OVL_IDX_MAX)) {
		DPU_FB_ERR("fb%d, invalid ovl_idx=%d!\n", dpufd->index, pov_req->ovl_idx);
		return -EINVAL;
	}

	return 0;
}

static int dpu_check_userdata_dst(dss_wb_layer_t *wb_layer,
	uint32_t index)
{
	if (wb_layer->need_cap & CAP_AFBCE) {
		if ((wb_layer->dst.afbc_header_stride == 0) ||
			(wb_layer->dst.afbc_payload_stride == 0)) {
			DPU_FB_ERR("fb%d, afbc_header_stride=%d, afbc_payload_stride=%d is invalid!\n",
				index, wb_layer->dst.afbc_header_stride, wb_layer->dst.afbc_payload_stride);
			return -EINVAL;
		}
	}

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

static int dpu_check_ov_info(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req)
{
	if ((pov_req->wb_layer_nums <= 0) ||
		(pov_req->wb_layer_nums > MAX_DSS_DST_NUM)) {
		DPU_FB_ERR("fb%d, invalid wb_layer_nums=%d!\n",
			dpufd->index, pov_req->wb_layer_nums);
		return -EINVAL;
	}

	if (pov_req->wb_ov_rect.x < 0 || pov_req->wb_ov_rect.y < 0) {
		DPU_FB_ERR("wb_ov_rect %d, %d is out of range!\n",
			pov_req->wb_ov_rect.x, pov_req->wb_ov_rect.y);
		return -EINVAL;
	}

	if (pov_req->wb_compose_type >= DSS_WB_COMPOSE_TYPE_MAX) {
		DPU_FB_ERR("wb_compose_type=%u is invalid!\n", pov_req->wb_compose_type);
		return -EINVAL;
	}

	return 0;
}

static int dpu_check_layer_afbc_info(struct dpu_fb_data_type *dpufd,
	dss_wb_layer_t *wb_layer)
{
	if (!(wb_layer->need_cap & (CAP_HFBCE | CAP_HEBCE))) {
		if (is_yuv_semiplanar(wb_layer->dst.format) ||
			is_yuv_plane(wb_layer->dst.format)) {
			if ((wb_layer->dst.stride_plane1 == 0) || (wb_layer->dst.offset_plane1 == 0)) {
				DPU_FB_ERR("fb%d, stride_plane1=%d, offset_plane1=%d is invalid!\n",
					dpufd->index, wb_layer->dst.stride_plane1,
					wb_layer->dst.offset_plane1);
				return -EINVAL;
			}
		}

		if (is_yuv_plane(wb_layer->dst.format)) {
			if ((wb_layer->dst.stride_plane2 == 0) || (wb_layer->dst.offset_plane2 == 0)) {
				DPU_FB_ERR("fb%d, stride_plane2=%d, offset_plane2=%d is invalid!\n",
					dpufd->index, wb_layer->dst.stride_plane2,
					wb_layer->dst.offset_plane2);
				return -EINVAL;
			}
		}
	}

	if (wb_layer->dst.afbc_scramble_mode >= DSS_AFBC_SCRAMBLE_MODE_MAX) {
		DPU_FB_ERR("fb%d, afbc_scramble_mode=%d is invalid!\n",
			dpufd->index, wb_layer->dst.afbc_scramble_mode);
		return -EINVAL;
	}

	return 0;
}

static bool is_wb_layer_size_info_empty(dss_wb_layer_t *wb_layer)
{
	return ((wb_layer->dst.bpp == 0) || (wb_layer->dst.width == 0) ||
		(wb_layer->dst.height == 0) || ((wb_layer->dst.stride == 0) &&
		(!(wb_layer->need_cap & (CAP_HFBCE | CAP_HEBCE)))));
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

	if (dpu_check_wblayer_buff(dpufd, wb_layer)) {
		DPU_FB_ERR("fb%d, failed to check_wblayer_buff!\n", dpufd->index);
		return -EINVAL;
	}

	if (dpu_check_layer_afbc_info(dpufd, wb_layer) < 0)
		return -EINVAL;

	if (dpu_check_userdata_dst(wb_layer, dpufd->index) < 0)
		return -EINVAL;

	if (wb_layer->dst.csc_mode >= DSS_CSC_MOD_MAX) {
		DPU_FB_ERR("fb%d, csc_mode=%d is invalid!\n", dpufd->index, wb_layer->dst.csc_mode);
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
	uint32_t i;
	dss_wb_layer_t *wb_layer = NULL;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "invalid dpufd!\n");
	dpu_check_and_return(!pov_req, -EINVAL, ERR, "invalid pov_req!\n");
	dpu_check_and_return(!pov_h_block_infos, -EINVAL, ERR, "invalid pov_h_block_infos!\n");

	if (dpu_check_userdata_base(dpufd, pov_req, pov_h_block_infos) < 0)
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
	} else if (dpufd->index == MEDIACOMMON_PANEL_IDX) {
		if (pov_req->wb_layer_nums > MAX_DSS_DST_NUM) {
			DPU_FB_ERR("fb%d, invalid wb_layer_nums=%d!\n",
				dpufd->index, pov_req->wb_layer_nums);
			return -EINVAL;
		}
		for (i = 0; i < pov_req->wb_layer_nums; i++) {
			wb_layer = &(pov_req->wb_layer_infos[i]);
			if (dpu_check_wblayer_buff(dpufd, wb_layer)) {
				DPU_FB_ERR("fb%d, failed to check_wblayer_buff!\n", dpufd->index);
				return -EINVAL;
			}
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

static int dpu_check_layer_rect(dss_layer_t *layer)
{
	if (layer->src_rect.x < 0 || layer->src_rect.y < 0 ||
		layer->src_rect.w <= 0 || layer->src_rect.h <= 0) {
		DPU_FB_ERR("src_rect %d, %d, %d, %d is out of range!\n",
			layer->src_rect.x, layer->src_rect.y,
			layer->src_rect.w, layer->src_rect.h);
		return -EINVAL;
	}

	if (layer->src_rect_mask.x < 0 || layer->src_rect_mask.y < 0 ||
		layer->src_rect_mask.w < 0 || layer->src_rect_mask.h < 0) {
		DPU_FB_ERR("src_rect_mask %d, %d, %d, %d is out of range!\n",
			layer->src_rect_mask.x, layer->src_rect_mask.y,
			layer->src_rect_mask.w, layer->src_rect_mask.h);
		return -EINVAL;
	}

	if (layer->dst_rect.x < 0 || layer->dst_rect.y < 0 ||
		layer->dst_rect.w <= 0 || layer->dst_rect.h <= 0) {
		DPU_FB_ERR("dst_rect %d, %d, %d, %d is out of range!\n",
			layer->dst_rect.x, layer->dst_rect.y,
			layer->dst_rect.w, layer->dst_rect.h);
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

	if (layer->need_cap & CAP_HFBCD) {
		if ((layer->img.hfbc_header_stride0 == 0) ||
			(layer->img.hfbc_payload_stride0 == 0) ||
			(layer->img.hfbc_header_stride1 == 0) ||
			(layer->img.hfbc_payload_stride1 == 0) ||
			((layer->chn_idx != DSS_RCHN_V0) &&
			(layer->chn_idx != DSS_RCHN_V1))) {
			DPU_FB_ERR("fb%d, hfbc_header_stride0=%d, hfbc_payload_stride0=%d,"
				"hfbc_header_stride1=%d, hfbc_payload_stride1=%d is invalid or "
				"chn_idx=%d no support hfbcd!\n",
				index, layer->img.hfbc_header_stride0,
				layer->img.hfbc_payload_stride0,
				layer->img.hfbc_header_stride1,
				layer->img.hfbc_payload_stride1, layer->chn_idx);
			return -EINVAL;
		}
	}

	return 0;
}

static int dpu_check_layer_ch_valid(struct dpu_fb_data_type *dpufd,
	dss_layer_t *layer)
{
	if (dpufd->index == AUXILIARY_PANEL_IDX) {
		if (layer->chn_idx != DSS_RCHN_V2) {
			if (layer->chn_idx < 0 || layer->chn_idx >= DSS_WCHN_W0) {
				DPU_FB_ERR("fb%d, rchn_idx=%d is invalid!\n",
					dpufd->index, layer->chn_idx);
				return -EINVAL;
			}
		}

		if (layer->chn_idx == DSS_RCHN_D2) {
			DPU_FB_ERR("fb%d, chn_idx[%d] does not used by offline play!\n",
				dpufd->index, layer->chn_idx);
			return -EINVAL;
		}
	} else {
		if (layer->chn_idx < 0 || layer->chn_idx >= DSS_CHN_MAX_DEFINE) {
			DPU_FB_ERR("fb%d, rchn_idx=%d is invalid!\n",
				dpufd->index, layer->chn_idx);
			return -EINVAL;
		}
	}

	return 0;
}

static bool is_layer_basic_size_info_empty(dss_layer_t *layer)
{
	return ((layer->img.bpp == 0) || (layer->img.width == 0) ||
		(layer->img.height == 0) || ((layer->img.stride == 0) &&
		(!(layer->need_cap & (CAP_HFBCD | CAP_HEBCD)))));
}

static int dpu_check_layer_basic_param_valid(struct dpu_fb_data_type *dpufd,
	dss_layer_t *layer)
{
	dpu_check_and_return((layer->blending < 0 || layer->blending >= DPU_FB_BLENDING_MAX), -EINVAL, ERR,
		"fb%d, blending=%d is invalid!\n", dpufd->index, layer->blending);
	dpu_check_and_return((layer->img.format >= DPU_FB_PIXEL_FORMAT_MAX), -EINVAL, ERR,
		"fb%d, format=%d is invalid!\n", dpufd->index, layer->img.format);

	if (is_layer_basic_size_info_empty(layer)) {
		DPU_FB_ERR("fb%d, bpp=%d, width=%d, height=%d, stride=%d is invalid!\n",
			dpufd->index, layer->img.bpp, layer->img.width, layer->img.height, layer->img.stride);
		return -EINVAL;
	}

	/* fix: dpu check layer buff */
	if (!(layer->need_cap & (CAP_HFBCD | CAP_HEBCD))) {
		if (is_yuv_semiplanar(layer->img.format) || is_yuv_plane(layer->img.format)) {
			if ((layer->img.stride_plane1 == 0) || (layer->img.offset_plane1 == 0)) {
				DPU_FB_ERR("fb%d, stride_plane1=%d, offset_plane1=%d is invalid!\n",
					dpufd->index, layer->img.stride_plane1, layer->img.offset_plane1);
				return -EINVAL;
			}
		}

		if (is_yuv_plane(layer->img.format)) {
			if ((layer->img.stride_plane2 == 0) || (layer->img.offset_plane2 == 0)) {
				DPU_FB_ERR("fb%d, stride_plane2=%d, offset_plane2=%d is invalid!\n",
					dpufd->index, layer->img.stride_plane2, layer->img.offset_plane2);
				return -EINVAL;
			}
		}
	}

	return 0;
}

static int dpu_check_layer_rot_cap(struct dpu_fb_data_type *dpufd,
	dss_layer_t *layer)
{
	u32 cap_online_rot = CAP_HFBCD;

	if (layer->transform & DPU_FB_TRANSFORM_ROT_90) {
		if (((layer->chn_idx == DSS_RCHN_V0) ||
			(layer->chn_idx == DSS_RCHN_V1)) &&
			(layer->need_cap & cap_online_rot)) {
			DPU_FB_DEBUG("fb%d, ch%d,need_cap=%d, need rot!\n!",
				dpufd->index, layer->chn_idx, layer->need_cap);
		} else {
			DPU_FB_ERR("fb%d, ch%d is not support need_cap=%d, transform=%d!\n",
				dpufd->index, layer->chn_idx, layer->need_cap, layer->transform);
			return -EINVAL;
		}
	}

	return 0;
}

int dpu_check_layer_par(struct dpu_fb_data_type *dpufd,
	dss_layer_t *layer)
{
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL!\n");
	dpu_check_and_return(!layer, -EINVAL, ERR, "layer is NULL, return!\n");

	if (layer->layer_idx < 0 || layer->layer_idx >= OVL_LAYER_NUM_MAX) {
		DPU_FB_ERR("fb%d, layer_idx=%d is invalid!\n", dpufd->index, layer->layer_idx);
		return -EINVAL;
	}

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

	if (layer->img.csc_mode >= DSS_CSC_MOD_MAX && !(csc_need_p3_process(layer->img.csc_mode))) {
		DPU_FB_ERR("fb%d, csc_mode=%d is invalid!\n", dpufd->index, layer->img.csc_mode);
		return -EINVAL;
	}

	if (layer->img.afbc_scramble_mode >= DSS_AFBC_SCRAMBLE_MODE_MAX) {
		DPU_FB_ERR("fb%d, afbc_scramble_mode=%d is invalid!\n",
			dpufd->index, layer->img.afbc_scramble_mode);
		return -EINVAL;
	}

	if ((layer->layer_idx != 0) && (layer->need_cap & CAP_BASE)) {
		DPU_FB_ERR("fb%d, layer%d is not base!\n",
			dpufd->index, layer->layer_idx);
		return -EINVAL;
	}

	if (dpu_check_layer_rect(layer)) {
		DPU_FB_ERR("fb%d, failed to check_layer_rect!\n", dpufd->index);
		return -EINVAL;
	}

	return 0;
}

/*******************************************************************************
 * DSS disreset
 */
void dpufb_dss_disreset(struct dpu_fb_data_type *dpufd)
{
	(void)dpufd;
}

static int dpu_dpp_gamma_para_set_reg(struct dpu_fb_data_type *dpufd, uint8_t last_gamma_type)
{
	uint32_t *gamma_lut_r = NULL;
	uint32_t *gamma_lut_g = NULL;
	uint32_t *gamma_lut_b = NULL;
	struct dpu_panel_info *pinfo = &(dpufd->panel_info);
	char __iomem *gamma_base = dpufd->dss_base + DSS_DPP_GAMA_OFFSET;
	char __iomem *gamma_lut_base = dpufd->dss_base + DSS_DPP_GAMA_LUT_OFFSET;
	uint32_t i;
	uint32_t index;
	uint32_t gama_lut_sel;

	if (last_gamma_type == 1) {
		/* set gamma cinema parameter */
		if (pinfo->cinema_gamma_lut_table_len > 0 && pinfo->cinema_gamma_lut_table_R &&
			pinfo->cinema_gamma_lut_table_G && pinfo->cinema_gamma_lut_table_B) {
			gamma_lut_r = pinfo->cinema_gamma_lut_table_R;
			gamma_lut_g = pinfo->cinema_gamma_lut_table_G;
			gamma_lut_b = pinfo->cinema_gamma_lut_table_B;
		} else {
			DPU_FB_ERR("can't get gamma cinema paramter from pinfo.\n");
			return -1;
		}
	} else {
		if (pinfo->gamma_lut_table_len > 0 && pinfo->gamma_lut_table_R &&
			pinfo->gamma_lut_table_G && pinfo->gamma_lut_table_B) {
			gamma_lut_r = pinfo->gamma_lut_table_R;
			gamma_lut_g = pinfo->gamma_lut_table_G;
			gamma_lut_b = pinfo->gamma_lut_table_B;
		} else {
			DPU_FB_ERR("can't get gamma normal parameter from pinfo.\n");
			return -1;
		}
	}

	/* config regsiter use default or cinema parameter */
	for (index = 0; index < pinfo->gamma_lut_table_len / 2; index++) {
		i = index << 1;
		outp32(gamma_lut_base + (U_GAMA_R_COEF + index * 4), (gamma_lut_r[i] | (gamma_lut_r[i + 1] << 16)));
		outp32(gamma_lut_base + (U_GAMA_G_COEF + index * 4), (gamma_lut_g[i] | (gamma_lut_g[i + 1] << 16)));
		outp32(gamma_lut_base + (U_GAMA_B_COEF + index * 4), (gamma_lut_b[i] | (gamma_lut_b[i + 1] << 16)));
		/* GAMA  PRE LUT */
		outp32(gamma_lut_base + (U_GAMA_PRE_R_COEF + index * 4), (gamma_lut_r[i] | (gamma_lut_r[i + 1] << 16)));
		outp32(gamma_lut_base + (U_GAMA_PRE_G_COEF + index * 4), (gamma_lut_g[i] | (gamma_lut_g[i + 1] << 16)));
		outp32(gamma_lut_base + (U_GAMA_PRE_B_COEF + index * 4), (gamma_lut_b[i] | (gamma_lut_b[i + 1] << 16)));
	}
	outp32(gamma_lut_base + U_GAMA_R_LAST_COEF, gamma_lut_r[pinfo->gamma_lut_table_len - 1]);
	outp32(gamma_lut_base + U_GAMA_G_LAST_COEF, gamma_lut_g[pinfo->gamma_lut_table_len - 1]);
	outp32(gamma_lut_base + U_GAMA_B_LAST_COEF, gamma_lut_b[pinfo->gamma_lut_table_len - 1]);
	/* GAMA PRE LUT */
	outp32(gamma_lut_base + U_GAMA_PRE_R_LAST_COEF, gamma_lut_r[pinfo->gamma_lut_table_len - 1]);
	outp32(gamma_lut_base + U_GAMA_PRE_G_LAST_COEF, gamma_lut_g[pinfo->gamma_lut_table_len - 1]);
	outp32(gamma_lut_base + U_GAMA_PRE_B_LAST_COEF, gamma_lut_b[pinfo->gamma_lut_table_len - 1]);

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

	if (!DPU_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_GAMA)) {
		DPU_FB_DEBUG("gamma is not suppportted in this platform\n");
		return;
	}

	dpu_check_and_no_retval((dpufd->index != PRIMARY_PANEL_IDX), ERR, "fb%d, not support!\n", dpufd->index);

	gmp_base = dpufd->dss_base + DSS_DPP_GMP_OFFSET;
	xcc_base = dpufd->dss_base + DSS_DPP_XCC_OFFSET;
	gamma_base = dpufd->dss_base + DSS_DPP_GAMA_OFFSET;

	if (gamma_config_flag == 0) {
		if (last_gamma_type != dpufd->panel_info.gamma_type) {
			set_reg(gamma_base + GAMA_EN, 0x0, 1, 0);
			set_reg(gmp_base + GMP_EN, 0x0, 1, 0);
			set_reg(xcc_base + XCC_EN, 0x1, 1, 0);

			gamma_config_flag = 1;
			last_gamma_type = dpufd->panel_info.gamma_type;
		}
		return;
	}

	if ((gamma_config_flag == 1) && (dpu_dpp_gamma_para_set_reg(dpufd, last_gamma_type) != 0))
		return;

	set_reg(gamma_base + GAMA_EN, 0x1, 1, 0);
	set_reg(gmp_base + GMP_EN, 0x1, 1, 0);
	set_reg(xcc_base + XCC_EN, 0x1, 1, 0);

	gamma_config_flag = 0;
}

static void dpu_dump_vote_info(struct dpu_fb_data_type *dpufd)
{
	struct dpu_fb_data_type *fb0 = dpufd_list[PRIMARY_PANEL_IDX];
	struct dpu_fb_data_type *fb1 = dpufd_list[EXTERNAL_PANEL_IDX];
	struct dpu_fb_data_type *fb2 = dpufd_list[AUXILIARY_PANEL_IDX];

	DPU_FB_INFO("fb%d, voltage[%d, %d, %d], edc[%llu, %llu, %llu]\n",
		dpufd->index,
		(fb0 != NULL) ? fb0->dss_vote_cmd.dss_voltage_level : 0,
		(fb1 != NULL) ? fb1->dss_vote_cmd.dss_voltage_level : 0,
		(fb2 != NULL) ? fb2->dss_vote_cmd.dss_voltage_level : 0,
		(fb0 != NULL) ? fb0->dss_vote_cmd.dss_pri_clk_rate : 0,
		(fb1 != NULL) ? fb1->dss_vote_cmd.dss_pri_clk_rate : 0,
		(fb2 != NULL) ? fb2->dss_vote_cmd.dss_pri_clk_rate : 0);
}

static void dpu_dump_dpp_info(struct dpu_fb_data_type *dpufd)
{
	DPU_FB_INFO("dpp: 0x%x, 0x%x,0x%x, 0x%x,0x%x, 0x%x,0x%x, 0x%x,0x%x, 0x%x, 0x%x,0x%x,0x%x, 0x%x\n",
		inp32(dpufd->dss_base + DSS_DISP_GLB_OFFSET + DYN_SW_DEFAULT),
		inp32(dpufd->dss_base + DSS_DPP_CH0_GAMA_OFFSET + GAMA_EN),
		inp32(dpufd->dss_base + DSS_DPP_CH1_GAMA_OFFSET + GAMA_EN),
		inp32(dpufd->dss_base + DSS_DPP_CH0_DEGAMMA_OFFSET + DEGAMA_EN),
		inp32(dpufd->dss_base + DSS_DPP_CH1_DEGAMMA_OFFSET + DEGAMA_EN),
		inp32(dpufd->dss_base + DSS_DPP_CH0_GMP_OFFSET + GMP_EN),
		inp32(dpufd->dss_base + DSS_DPP_CH1_GMP_OFFSET + GMP_EN),
		inp32(dpufd->dss_base + DSS_DPP_CH0_XCC_OFFSET + XCC_EN),
		inp32(dpufd->dss_base + DSS_DPP_CH1_XCC_OFFSET + XCC_EN),
		inp32(dpufd->dss_base + DSS_DPP_POST_XCC_OFFSET + POST_XCC_EN),
		inp32(dpufd->dss_base + DSS_HI_ACE_OFFSET + DPE_BYPASS_ACE),
		inp32(dpufd->dss_base + DSS_HI_ACE_OFFSET + DPE_BYPASS_NR),
		inp32(dpufd->dss_base + DSS_HI_ACE_OFFSET + DPE_IMAGE_INFO),
		inp32(dpufd->dss_base + DSS_DPP_DITHER_OFFSET + DITHER_CTL0));

	DPU_FB_INFO("arsr_post: 0x%x, 0x%x,0x%x, 0x%x,0x%x, 0x%x, 0x%x,0x%x,0x%x, 0x%x\n",
		inp32(dpufd->dss_module.post_scf_base + ARSR_POST_MODE),
		inp32(dpufd->dss_module.post_scf_base + ARSR_POST_IHLEFT),
		inp32(dpufd->dss_module.post_scf_base + ARSR_POST_IHLEFT1),
		inp32(dpufd->dss_module.post_scf_base + ARSR_POST_IHRIGHT),
		inp32(dpufd->dss_module.post_scf_base + ARSR_POST_IHRIGHT1),
		inp32(dpufd->dss_module.post_scf_base + ARSR_POST_IVTOP),
		inp32(dpufd->dss_module.post_scf_base + ARSR_POST_IVBOTTOM),
		inp32(dpufd->dss_module.post_scf_base + ARSR_POST_IVBOTTOM1),
		inp32(dpufd->dss_module.post_scf_base + ARSR_POST_IHINC),
		inp32(dpufd->dss_module.post_scf_base + ARSR_POST_IVINC));
}

static void dpu_dump_peri_status_info(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_no_retval(!dpufd->media_crg_base, ERR, "media_crg_base is NULL!\n");

	/* 0-5:div  6-9:pll */
	DPU_FB_INFO("edc0 clkdiv: 0x%x\n", inp32(dpufd->media_crg_base + MEDIA_CLKDIV2));
}

static void dpu_dump_panel_info(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = &dpufd->panel_info;

	DPU_FB_INFO("fb%d xres:%d yres:%d type:%d w:%d h:%d ifbc:%u fps=%d\n",
		dpufd->index, pinfo->xres, pinfo->yres, pinfo->type, pinfo->width,
		pinfo->height, pinfo->ifbc_type, pinfo->fps);
}
static void dpu_dump_disp_ch_info(struct dpu_fb_data_type *dpufd)
{
	if (dpufd->index == PRIMARY_PANEL_IDX)
		DPU_FB_INFO("ov%d:0x%x,dbuf:0x%x,0x%x,disp_ch:0x%x,0x%x,0x%x dpp_dbg:0x%x online_fill_lvl:0x%x\n",
			dpufd->ov_req.ovl_idx,
			inp32(dpufd->dss_base +  DSS_OVL0_OFFSET + OV_SIZE),
			inp32(dpufd->dss_base +  DSS_DBUF0_OFFSET + DBUF_FRM_SIZE),
			inp32(dpufd->dss_base +  DSS_DBUF0_OFFSET + DBUF_FRM_HSIZE),
			inp32(dpufd->dss_base + DSS_DISP_CH0_OFFSET + IMG_SIZE_BEF_SR),
			inp32(dpufd->dss_base + DSS_DISP_CH0_OFFSET + IMG_SIZE_AFT_SR),
			inp32(dpufd->dss_base + DSS_DISP_CH0_OFFSET + IMG_SIZE_AFT_IFBCSW),
			inp32(dpufd->dss_base + DSS_DISP_CH0_OFFSET + DISP_CH_DBG_CNT),
			inp32(dpufd->dss_base + DSS_DBUF0_OFFSET + DBUF_ONLINE_FILL_LEVEL));

	if (dpufd->index == EXTERNAL_PANEL_IDX)
		DPU_FB_INFO("ov%d:0x%x,dbuf:0x%x,0x%x,disp_ch:0x%x,0x%x,0x%x dpp_dbg:0x%x online_fill_lvl:0x%x\n",
			dpufd->ov_req.ovl_idx,
			inp32(dpufd->dss_base +  DSS_OVL1_OFFSET + OV_SIZE),
			inp32(dpufd->dss_base +  DSS_DBUF1_OFFSET + DBUF_FRM_SIZE),
			inp32(dpufd->dss_base +  DSS_DBUF1_OFFSET + DBUF_FRM_HSIZE),
			inp32(dpufd->dss_base + DSS_DISP_CH2_OFFSET + IMG_SIZE_BEF_SR),
			inp32(dpufd->dss_base + DSS_DISP_CH2_OFFSET + IMG_SIZE_AFT_SR),
			inp32(dpufd->dss_base + DSS_DISP_CH2_OFFSET + IMG_SIZE_AFT_IFBCSW),
			inp32(dpufd->dss_base + DSS_DISP_CH2_OFFSET + DISP_CH_DBG_CNT),
			inp32(dpufd->dss_base + DSS_DBUF1_OFFSET + DBUF_ONLINE_FILL_LEVEL));
}

static void dpu_dump_dsi_info(struct dpu_fb_data_type *dpufd)
{
	char __iomem *mipi_dsi_base = NULL;

	if (dpufd->index == PRIMARY_PANEL_IDX)
		mipi_dsi_base = get_mipi_dsi_base(dpufd);
	else if (dpufd->index == EXTERNAL_PANEL_IDX)
		mipi_dsi_base = dpufd->mipi_dsi1_base;
	dpu_check_and_no_retval(!mipi_dsi_base, ERR, "mipi_dsi_base is NULL\n");
	DPU_FB_ERR("itf_ints=0x%x, ldi_vstate=0x%x, phy_status=0x%x\n",
		inp32(mipi_dsi_base + MIPI_LDI_CPU_ITF_INTS),
		inp32(mipi_dsi_base + MIPI_LDI_VSTATE),
		inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET));

	DPU_FB_INFO("ldi-dsi:0x%x,0x%x,0x%x,0x%x\n",
		inp32(mipi_dsi_base + MIPI_LDI_DPI0_HRZ_CTRL2),
		inp32(mipi_dsi_base + MIPI_LDI_VRT_CTRL2),
		inp32(mipi_dsi_base + MIPIDSI_EDPI_CMD_SIZE_OFFSET),
		inp32(mipi_dsi_base + MIPIDSI_VID_VACTIVE_LINES_OFFSET));
}

static void dpu_dump_mctl_info(struct dpu_fb_data_type *dpufd)
{
	uint32_t mctl_idx;
	uint32_t ov_idx;
	char __iomem *mctl_base = NULL;
	char __iomem *mctl_sys_base = NULL;
	uint32_t rch_sel;
	uint32_t chn_idx = 0;

	uint32_t rch_ov_oen_reg[] = {
		MCTL_RCH0_OV_OEN, MCTL_RCH1_OV_OEN, MCTL_RCH2_OV_OEN, MCTL_RCH3_OV_OEN,
		MCTL_RCH4_OV_OEN, MCTL_RCH5_OV_OEN, MCTL_RCH6_OV_OEN, MCTL_RCH7_OV_OEN, MCTL_RCH8_OV_OEN,
	};

	ov_idx = dpufd->ov_req.ovl_idx;
	mctl_idx = (dpufd->index == PRIMARY_PANEL_IDX) ? DSS_MCTL0 : DSS_MCTL1;
	mctl_base = dpufd->dss_base + g_dss_module_ovl_base[mctl_idx][MODULE_MCTL_BASE];
	mctl_sys_base = dpufd->dss_base + DSS_MCTRL_SYS_OFFSET;

	DPU_FB_INFO("mctl[%u]:en:0x%x mutex:0x%x mutex_staus:0x%x dbuf:0x%x ov:0x%x itf:0x%x top:0x%x status:0x%x\n",
		mctl_idx,
		inp32(mctl_base + MCTL_CTL_EN),
		inp32(mctl_base + MCTL_CTL_MUTEX),
		inp32(mctl_base + MCTL_CTL_MUTEX_STATUS),
		inp32(mctl_base + MCTL_CTL_MUTEX_DBUF),
		inp32(mctl_base + MCTL_CTL_MUTEX_OV),
		inp32(mctl_base + MCTL_CTL_MUTEX_ITF),
		inp32(mctl_base + MCTL_CTL_TOP),
		inp32(mctl_base + MCTL_CTL_STATUS));

	rch_sel = inp32(mctl_sys_base + MCTL_RCH_OV0_SEL + ov_idx * 0x4);
	/* rch binded on ov */
	DPU_FB_INFO("rchs on ov[%u]:0x%x\n", ov_idx, rch_sel);
	while (rch_sel > 0) {
		if (rch_sel == RCH_OV_BIND_DEFAULT)
			break;
		rch_sel = rch_sel >> RCH_SWITCH_BIT_SHIFT;
		chn_idx  = rch_sel & RCH_OV_BIND_DEFAULT;
		if (chn_idx < ARRAY_SIZE(rch_ov_oen_reg))
			DPU_FB_INFO("rch[%u] binds to OV[0x%x]\n", chn_idx, inp32(mctl_sys_base + rch_ov_oen_reg[chn_idx]));
	}
}

void dpu_dump_current_info(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL!\n");

	if ((dpufd->index != PRIMARY_PANEL_IDX) && (dpufd->index != EXTERNAL_PANEL_IDX))
		return;

	dpu_dump_mctl_info(dpufd);
	dpu_dump_vote_info(dpufd);
	dpu_dump_dpp_info(dpufd);
	dpu_dump_disp_ch_info(dpufd);
	dpu_dump_dsi_info(dpufd);
	dpu_dump_peri_status_info(dpufd);
	dpu_dump_panel_info(dpufd);
}

static int dpu_vactive0_get_cmdlist_idxs(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req,
	struct timediff_info *timediff)
{
	int ret;
	uint32_t cmdlist_idxs;
	uint32_t cmdlist_idxs_prev = 0;
	uint32_t cmdlist_idxs_prev_prev = 0;
	dss_overlay_t *pov_req_dump = NULL;
	dss_overlay_t *pov_req_prev = NULL;
	dss_overlay_t *pov_req_prev_prev = NULL;

	pov_req_dump = &(dpufd->ov_req_prev_prev);
	pov_req_prev = &(dpufd->ov_req_prev);
	pov_req_prev_prev = &(dpufd->ov_req_prev_prev);

	ret = dpu_cmdlist_get_cmdlist_idxs(pov_req_prev, &cmdlist_idxs_prev, NULL);
	if (ret != 0)
		DPU_FB_INFO("fb%d, dpu_cmdlist_get_cmdlist_idxs pov_req_prev failed! ret = %d\n",
			dpufd->index, ret);

	ret = dpu_cmdlist_get_cmdlist_idxs(pov_req_prev_prev, &cmdlist_idxs_prev_prev, NULL);
	if (ret != 0)
		DPU_FB_INFO("fb%d, dpu_cmdlist_get_cmdlist_idxs "
			"pov_req_prev_prev failed! ret = %d\n", dpufd->index, ret);

	cmdlist_idxs = cmdlist_idxs_prev | cmdlist_idxs_prev_prev;
	/* cppcheck-suppress */
	DPU_FB_ERR("fb%d, 1st timeout!"
		"flag=%d, pre_pre_frm_no=%u, frm_no=%u, timediff= %u us, "
		"cmdlist_idxs_prev=0x%x, cmdlist_idxs_prev_prev=0x%x, "
		"cmdlist_idxs=0x%x\n",
		dpufd->index, dpufd->vactive0_start_flag,
		pov_req_dump->frame_no, pov_req->frame_no,
		dpufb_timestamp_diff(&(timediff->tv0), &(timediff->tv1)),
		cmdlist_idxs_prev, cmdlist_idxs_prev_prev, cmdlist_idxs);

	return cmdlist_idxs;
}

static void dpu_vactive0_check_and_reset(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req,
	struct timediff_info *timediff)
{
	uint32_t cmdlist_idxs;
	char __iomem *mipi_dsi_base = NULL;
	dss_overlay_t *pov_req_dump = NULL;

	pov_req_dump = &(dpufd->ov_req_prev_prev);
	mipi_dsi_base = (dpufd->index == EXTERNAL_PANEL_IDX) ? dpufd->mipi_dsi1_base : get_mipi_dsi_base(dpufd);
	dpu_check_and_no_retval(!mipi_dsi_base, ERR, "mipi_dsi_base is NULL\n");
	cmdlist_idxs = dpu_vactive0_get_cmdlist_idxs(dpufd, pov_req, timediff);

	if (g_debug_ovl_online_composer_hold > 0)
		mdelay(DPU_COMPOSER_HOLD_TIME);

	if (g_debug_ldi_underflow != 0) {
		dpu_dump_current_info(dpufd);
		dpu_cmdlist_config_reset(dpufd, pov_req_dump, cmdlist_idxs);
		memset(dpufd->ov_block_infos_prev, 0, DPU_OV_BLOCK_NUMS * sizeof(dss_overlay_block_t));
		dpufb_dss_overlay_info_init(&dpufd->ov_req_prev);
	}
}

static int dpu_vactive0_start_event_check(struct dpu_fb_data_type *dpufd, int timeout)
{
	long ret;
	int times = 0;

	while (true) {
		ret = wait_event_interruptible_timeout(dpufd->vactive0_start_wq,
			dpufd->vactive0_start_flag, msecs_to_jiffies(timeout));
		if ((ret == -ERESTARTSYS) && (times++ < 50)) /* wait 500ms */
			mdelay(10); /* 10ms */
		else
			break;
	}
	return ret > 0 ? 0 : -1;
}

static int dpu_vactive0_start_config_br1(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req)
{
	int ret = 0;
	struct timediff_info time_diff = { {0}, {0} };

	if (dpufd->vactive0_start_flag == 0) {
		dpufb_get_timestamp(&(time_diff.tv0));
		ret = dpu_vactive0_start_event_check(dpufd, DSS_COMPOSER_TIMEOUT_THRESHOLD_ASIC);
		if (ret != 0) {
			dpufb_get_timestamp(&(time_diff.tv1));
			dpu_vactive0_check_and_reset(dpufd, pov_req, &time_diff);
		}
	}

	dpufd->vactive0_start_flag = 0;
	dpufd->vactive0_end_flag = 0;
	return ret;
}

static int dpu_vactive0_start_config_video(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req)
{
	int times = 0;
	struct timeval tv0;
	struct timeval tv1;
	uint32_t cmdlist_idxs = 0;
	dss_overlay_t *pov_req_dump = NULL;
	uint32_t timeout_interval;
	uint32_t prev_vactive0_start;
	int ret;
	void_unused(pov_req);

	pov_req_dump = &(dpufd->ov_req_prev);

	if (g_fpga_flag == 0)
		timeout_interval = DSS_COMPOSER_TIMEOUT_THRESHOLD_ASIC;
	else
		timeout_interval = DSS_COMPOSER_TIMEOUT_THRESHOLD_FPGA;

	dpufb_get_timestamp(&tv0);
	ldi_data_gate(dpufd, false);
	prev_vactive0_start = dpufd->vactive0_start_flag;

	while (1) {
		/*lint -e666*/
		ret = wait_event_interruptible_timeout(dpufd->vactive0_start_wq,  /*lint !e578*/
			(prev_vactive0_start != dpufd->vactive0_start_flag),
			msecs_to_jiffies(timeout_interval));
		/*lint +e666*/
		if ((ret == -ERESTARTSYS) && (times++ < 50))  /* 500ms */
			mdelay(10);  /* 10ms */
		else
			break;
	}

	if (ret <= 0) {
		dpufb_get_timestamp(&tv1);

		ret = dpu_cmdlist_get_cmdlist_idxs(pov_req_dump, &cmdlist_idxs, NULL);
		if (ret != 0)
			DPU_FB_INFO("fb%d, dpu_cmdlist_get_cmdlist_idxs pov_req_prev failed! ret = %d\n",
				dpufd->index, ret);

		DPU_FB_ERR("fb%d, 1wait_for vactive0_start_flag timeout!ret=%d, "
			"vactive0_start_flag=%d, pre frame_no=%u, TIMESTAMP_DIFF is %u us,"
			"cmdlist_idxs=0x%x!\n",
			dpufd->index, ret, dpufd->vactive0_start_flag,
			pov_req_dump->frame_no,
			dpufb_timestamp_diff(&tv0, &tv1), cmdlist_idxs);

		dpu_dump_current_info(dpufd);

		if (g_debug_ovl_online_composer_hold)
			mdelay(DPU_COMPOSER_HOLD_TIME);
		ret = 0;
	} else {
		ret = 0;
	}

	return ret;
}

int dpu_vactive0_start_config(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req)
{
	int ret;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL!\n");
	dpu_check_and_return(!pov_req, -EINVAL, ERR, "pov_req is NULL!\n");

	if (is_mipi_cmd_panel(dpufd))
		ret = dpu_vactive0_start_config_br1(dpufd, pov_req);
	else  /* video mode */
		ret = dpu_vactive0_start_config_video(dpufd, pov_req);

	return ret;
}
