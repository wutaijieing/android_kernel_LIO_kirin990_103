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

/*
 * DSS AIF
 */
static int g_mid_array[DSS_CHN_MAX_DEFINE] = {0xb, 0xa, 0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x2, 0x1, 0x3, 0x0};
#define CREDIT_STEP_LOWER_ENABLE

void dpu_aif_init(const char __iomem *aif_ch_base,
	dss_aif_t *s_aif)
{
	if (!aif_ch_base) {
		DPU_FB_ERR("aif_ch_base is NULL\n");
		return;
	}
	if (!s_aif) {
		DPU_FB_ERR("s_aif is NULL\n");
		return;
	}

	memset(s_aif, 0, sizeof(dss_aif_t));

	s_aif->aif_ch_ctl = inp32(aif_ch_base + AIF_CH_CTL);

	s_aif->aif_ch_hs = inp32(aif_ch_base + AIF_CH_HS);
	s_aif->aif_ch_ls = inp32(aif_ch_base + AIF_CH_LS);
}

void dpu_aif_ch_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *aif_ch_base, dss_aif_t *s_aif)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd is null\n");
		return;
	}

	if (!aif_ch_base) {
		DPU_FB_ERR("aif_ch_base is null\n");
		return;
	}

	if (!s_aif) {
		DPU_FB_ERR("s_aif is null\n");
		return;
	}

	dpufd->set_reg(dpufd, aif_ch_base + AIF_CH_CTL,
		s_aif->aif_ch_ctl, 32, 0);

	dpufd->set_reg(dpufd, aif_ch_base + AIF_CH_HS,
		s_aif->aif_ch_hs, 32, 0);
	dpufd->set_reg(dpufd, aif_ch_base + AIF_CH_LS,
		s_aif->aif_ch_ls, 32, 0);
}

static void print_aif_ch_config_layer_info(struct dpu_fb_data_type *dpufd, const dss_layer_t *layer,
	const dss_overlay_t *pov_req, const dss_rect_t *wb_dst_rect, uint64_t dss_core_rate)
{
	uint32_t credit_step;
	uint32_t scfd_h;
	uint32_t scfd_v;
	uint32_t online_offline_rate = 1;

	if ((layer->src_rect.w > layer->dst_rect.w) && (layer->src_rect.w > get_panel_xres(dpufd)))
		scfd_h = layer->src_rect.w * 100 / get_panel_xres(dpufd);
	else
		scfd_h = 100;

	/* after stretch */
	if (layer->src_rect.h > layer->dst_rect.h)
		scfd_v = layer->src_rect.h * 100 / layer->dst_rect.h;
	else
		scfd_v = 100;

	if (pov_req->wb_compose_type == DSS_WB_COMPOSE_COPYBIT) {
		if (wb_dst_rect)
			online_offline_rate = wb_dst_rect->w * wb_dst_rect->h /
				(dpufd->panel_info.xres * dpufd->panel_info.yres);

		if (online_offline_rate == 0)
			online_offline_rate = 1;
	}
	/* credit_step = pix_f*128/(core_f*16/4)*scfd_h*scfd_v */
	credit_step = dpufd->panel_info.pxl_clk_rate * online_offline_rate * 32 *
		scfd_h * scfd_v / dss_core_rate  / (100 * 100);

	if (g_debug_ovl_online_composer || g_debug_ovl_offline_composer || g_debug_ovl_credit_step)
		DPU_FB_INFO("fb%d, layer_idx[%d], chn_idx[%d], src_rect[%d,%d,%d,%d],"
			"dst_rect[%d,%d,%d,%d], scfd_h=%d, scfd_v=%d, credit_step=%d.\n",
			dpufd->index, layer->layer_idx, layer->chn_idx,
			layer->src_rect.x, layer->src_rect.y, layer->src_rect.w, layer->src_rect.h,
			layer->dst_rect.x, layer->dst_rect.y, layer->dst_rect.w, layer->dst_rect.h,
			scfd_h, scfd_v, credit_step);
}

int dpu_aif_ch_config(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req,
	dss_layer_t *layer, dss_rect_t *wb_dst_rect, dss_wb_layer_t *wb_layer)
{
	dss_aif_t *aif = NULL;
	dss_aif_bw_t *aif_bw = NULL;
	int chn_idx;
	uint32_t credit_step_lower;
	uint64_t dss_core_rate;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL Point!\n");
	dpu_check_and_return(!pov_req, -EINVAL, ERR, "pov_req is NULL Point!\n");
	dpu_check_and_return((!layer && !wb_layer), -EINVAL, ERR, "layer & wb_layer is NULL Point!\n");

	dpu_check_and_return(((pov_req->ovl_idx < DSS_OVL0) || (pov_req->ovl_idx >= DSS_OVL_IDX_MAX)),
		-EINVAL, ERR, "ovl_idx %d is invalid!\n", pov_req->ovl_idx);

	if (wb_layer)
		chn_idx = wb_layer->chn_idx;
	else
		chn_idx = layer->chn_idx;

	aif = &(dpufd->dss_module.aif[chn_idx]);
	dpufd->dss_module.aif_ch_used[chn_idx] = 1;

	aif_bw = &(dpufd->dss_module.aif_bw[chn_idx]);
	dpu_check_and_return((aif_bw->is_used != 1), -EINVAL, ERR,
		"fb%d, aif_bw->is_used[%d] is invalid!\n", dpufd->index, aif_bw->is_used);

	if (g_mid_array[chn_idx] < 0 || g_mid_array[chn_idx] > 0xb) {
		DPU_FB_ERR("fb%d, mid[%d] is invalid!", dpufd->index, g_mid_array[chn_idx]);
		return -EINVAL;
	}

	aif->aif_ch_ctl = set_bits32(aif->aif_ch_ctl, aif_bw->axi_sel, 1, 0);
	aif->aif_ch_ctl = set_bits32(aif->aif_ch_ctl, 0x0, 3, 8);  /* credit mode */

	if (g_fpga_flag != 0)
		return 0;

	if ((pov_req->ovl_idx == DSS_OVL2) || (pov_req->ovl_idx == DSS_OVL3)) {
		if ((layer && ((layer->need_cap & CAP_AFBCD) != CAP_AFBCD)) || wb_layer) {
			dss_core_rate = dpufd->dss_vote_cmd.dss_pri_clk_rate;
			if (dss_core_rate == 0) {
				DPU_FB_ERR("fb%d, dss_core_rate[%llu] invalid!\n",
					dpufd->index, dss_core_rate);
				dss_core_rate = DEFAULT_DSS_CORE_CLK_07V_RATE;
			}

			credit_step_lower = g_dss_min_bandwidth_inbusbusy * 1000000UL * 8 / dss_core_rate;

			/* credit en lower */
			aif->aif_ch_ctl = set_bits32(aif->aif_ch_ctl, 0x1, 3, 8);  /* credit step lower mode */
			aif->aif_ch_ctl = set_bits32(aif->aif_ch_ctl, 0xf, 4, 12);
			aif->aif_ch_ls = set_bits32(aif->aif_ch_ls, 0x0, 9, 21);
			aif->aif_ch_ls = set_bits32(aif->aif_ch_ls, 0x40, 9, 12);
			aif->aif_ch_ls = set_bits32(aif->aif_ch_ls, credit_step_lower, 7, 4);
			aif->aif_ch_ls = set_bits32(aif->aif_ch_ls, 0x1, 1, 0);
		}

		if (layer && ((layer->need_cap & CAP_AFBCD) != CAP_AFBCD))
			print_aif_ch_config_layer_info(dpufd, layer, pov_req, wb_dst_rect, dss_core_rate);
	}

	return 0;
}

static void aif1_ch_config_ovl0_or_ovl1(struct dpu_fb_data_type *dpufd, const dss_layer_t *layer,
	dss_aif_t *aif1)
{
	uint32_t credit_step;
	uint32_t scfd_h;
	uint32_t scfd_v;
	uint64_t dss_core_rate;

	if (layer && (layer->need_cap & CAP_AFBCD)) {
		dss_core_rate = dpufd->dss_vote_cmd.dss_pri_clk_rate;
		if (dss_core_rate == 0) {
			DPU_FB_ERR("fb%d, dss_core_rate[%llu] is invalid!", dpufd->index, dss_core_rate);
			dss_core_rate = DEFAULT_DSS_CORE_CLK_07V_RATE;
		}

		if ((layer->src_rect.w > layer->dst_rect.w) && (layer->src_rect.w > get_panel_xres(dpufd)))
			scfd_h = layer->src_rect.w * 100 / get_panel_xres(dpufd);
		else
			scfd_h = 100;

		/* after stretch */
		if (layer->src_rect.h > layer->dst_rect.h)
			scfd_v = layer->src_rect.h * 100 / layer->dst_rect.h;
		else
			scfd_v = 100;

		/* credit_step = pix_f*128/(core_f*16/4)*1.25*scfd_h*scfd_v */
		credit_step = dpufd->panel_info.pxl_clk_rate * 32 * 150 * scfd_h * scfd_v /
			dss_core_rate  / (100 * 100 * 100);

		if (g_debug_ovl_online_composer || g_debug_ovl_offline_composer || g_debug_ovl_credit_step)
			DPU_FB_INFO("fb%d, layer_idx[%d], chn_idx[%d], src_rect[%d,%d,%d,%d],"
				"dst_rect[%d,%d,%d,%d], scfd_h=%d, scfd_v=%d, credit_step=%d.\n",
				dpufd->index, layer->layer_idx, layer->chn_idx,
				layer->src_rect.x, layer->src_rect.y, layer->src_rect.w, layer->src_rect.h,
				layer->dst_rect.x, layer->dst_rect.y, layer->dst_rect.w, layer->dst_rect.h,
				scfd_h, scfd_v, credit_step);

		if (credit_step < 32)
			credit_step = 32;

		aif1->aif_ch_ctl = set_bits32(aif1->aif_ch_ctl, 0x0, 3, 8);  /* credit mode */
		if (credit_step > 64)
			aif1->aif_ch_hs = set_bits32(aif1->aif_ch_hs, 0x0, 1, 0); /* credit en enable */
		else
			aif1->aif_ch_hs = set_bits32(aif1->aif_ch_hs, 0x1, 1, 0); /* credit en enable */

		aif1->aif_ch_hs = set_bits32(aif1->aif_ch_hs, credit_step, 7, 4); /* credit step hs */
	}
}

static void aif1_ch_config_ovl2_or_ovl3(const struct dpu_fb_data_type *dpufd, const dss_layer_t *layer,
	dss_aif_t *aif1)
{
	uint64_t dss_core_rate;
	uint32_t credit_step_lower;

	if (layer && (layer->need_cap & CAP_AFBCD)) {
		dss_core_rate = dpufd->dss_vote_cmd.dss_pri_clk_rate;
		if (dss_core_rate == 0) {
			DPU_FB_ERR("fb%d, dss_core_rate %llu is invalid!", dpufd->index, dss_core_rate);
			dss_core_rate = DEFAULT_DSS_CORE_CLK_07V_RATE;
		}

		credit_step_lower = g_dss_min_bandwidth_inbusbusy * 1000000UL * 8 / dss_core_rate;

		/* credit en lower */
		aif1->aif_ch_ctl = set_bits32(aif1->aif_ch_ctl, 0x1, 3, 8);  /* credit step lower mode */
		aif1->aif_ch_ctl = set_bits32(aif1->aif_ch_ctl, 0xf, 4, 12);  /* max_cnt */
		aif1->aif_ch_ls = set_bits32(aif1->aif_ch_ls, 0x0, 9, 21);
		aif1->aif_ch_ls = set_bits32(aif1->aif_ch_ls, 0x40, 9, 12);
		aif1->aif_ch_ls = set_bits32(aif1->aif_ch_ls, credit_step_lower, 7, 4);
		aif1->aif_ch_ls = set_bits32(aif1->aif_ch_ls, 0x1, 1, 0);
	}
}

int dpu_aif1_ch_config(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req,
	dss_layer_t *layer, dss_wb_layer_t *wb_layer, int ovl_idx)
{
	dss_aif_t *aif1 = NULL;
	dss_aif_bw_t *aif1_bw = NULL;
	int chn_idx;
	uint32_t need_cap;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL Point!\n");
	dpu_check_and_return(!pov_req, -EINVAL, ERR, "pov_req is NULL Point!\n");
	dpu_check_and_return((!layer && !wb_layer), -EINVAL, ERR, "layer & wb_layer is NULL Point!\n");

	dpu_check_and_return(((ovl_idx < DSS_OVL0) || (ovl_idx >= DSS_OVL_IDX_MAX)), -EINVAL, ERR,
		"ovl_idx %d is invalid!\n", ovl_idx);

	if (wb_layer) {
		chn_idx = wb_layer->chn_idx;
		need_cap = wb_layer->need_cap;
	} else {
		chn_idx = layer->chn_idx;
		need_cap = layer->need_cap;
	}

	if (!(need_cap & CAP_AFBCD))
		return 0;

	aif1 = &(dpufd->dss_module.aif1[chn_idx]);
	dpufd->dss_module.aif1_ch_used[chn_idx] = 1;

	aif1_bw = &(dpufd->dss_module.aif1_bw[chn_idx]);
	if (aif1_bw->is_used != 1) {
		DPU_FB_ERR("fb%d, aif1_bw->is_used=%d no equal to 1 is err!\n", dpufd->index, aif1_bw->is_used);
		return 0;
	}

	if (g_mid_array[chn_idx] < 0 || g_mid_array[chn_idx] > 0xb) {
		DPU_FB_ERR("fb%d, mid=%d is invalid!\n", dpufd->index, g_mid_array[chn_idx]);
		return 0;
	}

	aif1->aif_ch_ctl = set_bits32(aif1->aif_ch_ctl, aif1_bw->axi_sel, 1, 0);
	aif1->aif_ch_ctl = set_bits32(aif1->aif_ch_ctl, g_mid_array[chn_idx], 4, 4);
	aif1->aif_ch_ctl = set_bits32(aif1->aif_ch_ctl, 0x0, 3, 8);  /* credit mode */

	if (g_fpga_flag == 0) {
		if ((ovl_idx == DSS_OVL0) || (ovl_idx == DSS_OVL1))
			aif1_ch_config_ovl0_or_ovl1(dpufd, layer, aif1);
		else
			aif1_ch_config_ovl2_or_ovl3(dpufd, layer, aif1);
	}

	return 0;
}

int dpu_aif_handler(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req,
	dss_overlay_block_t *pov_h_block)
{
	int i;
	int k;
	dss_layer_t *layer = NULL;
	dss_wb_layer_t *wb_layer = NULL;
	int chn_idx;
	dss_aif_bw_t *aif_bw = NULL;
	dss_aif_bw_t *aif1_bw = NULL;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL!\n");
	dpu_check_and_return(!pov_req, -EINVAL, ERR, "pov_req is NULL!\n");
	dpu_check_and_return(!pov_h_block, -EINVAL, ERR, "pov_h_block is NULL!\n");

	if (pov_req->wb_enable) {
		if ((pov_req->wb_layer_nums <= 0) || (pov_req->wb_layer_nums > MAX_DSS_DST_NUM)) {
			DPU_FB_ERR("wb_layer_nums=%d out of range\n", pov_req->wb_layer_nums);
			return -EINVAL;
		}

		for (k = 0; k < pov_req->wb_layer_nums; k++) {
			wb_layer = &(pov_req->wb_layer_infos[k]);
			chn_idx = wb_layer->chn_idx;
			dpu_check_and_return((chn_idx < 0) || (chn_idx >= DSS_CHN_MAX_DEFINE),
				-EINVAL, ERR, "wb_layer->chn_idx exceeds array limit\n");

			aif_bw = &(dpufd->dss_module.aif_bw[chn_idx]);
			aif_bw->chn_idx = chn_idx;
			aif_bw->axi_sel = AXI_CHN1;
			aif_bw->is_used = 1;
		}
	}

	for (i = 0; i < pov_h_block->layer_nums; i++) {
		layer = &pov_h_block->layer_infos[i];
		chn_idx = layer->chn_idx;
		dpu_check_and_return((chn_idx < 0) || (chn_idx >= DSS_CHN_MAX_DEFINE),
			-EINVAL, ERR, "layer->chn_idx exceeds array limit\n");

		if (layer->need_cap & (CAP_BASE | CAP_DIM | CAP_PURE_COLOR))
			continue;

		aif_bw = &(dpufd->dss_module.aif_bw[chn_idx]);
		aif_bw->is_used = 1;
		aif_bw->chn_idx = chn_idx;
		if (pov_req->ovl_idx == DSS_OVL0)
			aif_bw->axi_sel = AXI_CHN0;
		else
			aif_bw->axi_sel = AXI_CHN1;

		if (layer->need_cap & CAP_AFBCD) {
			aif1_bw = &(dpufd->dss_module.aif1_bw[chn_idx]);
			aif1_bw->is_used = 1;
			aif1_bw->chn_idx = chn_idx;
			aif1_bw->axi_sel = AXI_CHN0;
		}
	}

	return 0;
}

