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

static void aif_bw_sort(dss_aif_bw_t a[], int n)
{
	int i;
	int j;
	dss_aif_bw_t tmp;

	for (i = 0; i < n; ++i) {
		for (j = i; j < n - 1; ++j) {
			if (a[j].bw > a[j + 1].bw) {
				tmp = a[j];
				a[j] = a[j + 1];
				a[j + 1] = tmp;
			}
		}
	}
}

static int dpu_aif_cfg_for_wblayer(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req, dss_overlay_block_t *pov_h_block, dss_aif_bw_t aif_bw_tmp[], uint32_t aif_bw_size)
{
	uint32_t i;
	uint32_t k;
	dss_layer_t *layer = NULL;
	dss_wb_layer_t *wb_layer = NULL;
	dss_aif_bw_t *aif_bw = NULL;
	int32_t chn_idx;

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
		aif_bw->bw = (uint64_t)wb_layer->dst.buf_size *
			(wb_layer->src_rect.w * wb_layer->src_rect.h) / (wb_layer->dst.width * wb_layer->dst.height);
		aif_bw->chn_idx = chn_idx;
		if (pov_req->wb_compose_type == DSS_WB_COMPOSE_MEDIACOMMON)
			aif_bw->axi_sel = AXI_CHN0;
		else
			aif_bw->axi_sel = AXI_CHN1;

		aif_bw->is_used = 1;
	}

	if ((pov_req->wb_compose_type == DSS_WB_COMPOSE_COPYBIT) ||
		(pov_req->wb_compose_type == DSS_WB_COMPOSE_MEDIACOMMON)) {
		for (i = 0; i < pov_h_block->layer_nums; i++) {
			layer = &pov_h_block->layer_infos[i];
			chn_idx = layer->chn_idx;
			dpu_check_and_return((chn_idx < 0) || (chn_idx >= DSS_CHN_MAX_DEFINE),
				-EINVAL, ERR, "layer->chn_idx exceeds array limit\n");

			if (i >= aif_bw_size) {
				DPU_FB_ERR("aif_bw_tmp overflow\n");
				return -EINVAL;
			}

			aif_bw_tmp[i].chn_idx = chn_idx;
			aif_bw_tmp[i].axi_sel = AXI_CHN0;
			aif_bw_tmp[i].is_used = 1;
			dpufd->dss_module.aif_bw[chn_idx] = aif_bw_tmp[i];
		}
	}

	return 0;
}

static void get_aif_ch_axi_sel(const dss_overlay_t *pov_req, bool less_then_bw_half,
	dss_aif_bw_t aif_bw_tmp[], uint32_t bw_sum, int i)
{
	int axi0_cnt = 0;
	int axi1_cnt = 0;

	if ((pov_req->ovl_idx == DSS_OVL0) || (pov_req->ovl_idx == DSS_OVL1)) {
		if (less_then_bw_half) {
			aif_bw_tmp[i].axi_sel = AXI_CHN0;
			if (axi0_cnt >= AXI0_MAX_DSS_CHN_THRESHOLD) {
				aif_bw_tmp[i - AXI0_MAX_DSS_CHN_THRESHOLD].axi_sel = AXI_CHN1;
				axi1_cnt++;
				axi0_cnt--;
			}
			axi0_cnt++;
		} else {
			aif_bw_tmp[i].axi_sel = AXI_CHN1;
			axi1_cnt++;
		}
	} else {
		if (less_then_bw_half) {
			aif_bw_tmp[i].axi_sel = AXI_CHN1;
			if (axi1_cnt >= AXI1_MAX_DSS_CHN_THRESHOLD) {
				aif_bw_tmp[i - AXI1_MAX_DSS_CHN_THRESHOLD].axi_sel = AXI_CHN0;
				axi0_cnt++;
				axi1_cnt--;
			}
			axi1_cnt++;
		} else {
			aif_bw_tmp[i].axi_sel = AXI_CHN0;
			axi0_cnt++;
		}
	}
}

static void dpu_aif_ch_ov_cfg(struct dpu_fb_data_type *dpufd,
	const dss_overlay_t *pov_req, dss_aif_bw_t aif_bw_tmp[], uint32_t bw_sum)
{
	int i;
	uint32_t tmp = 0;
	int chn_idx;

	/* i is not chn_idx, is array idx */
	for (i = 0; i < DSS_CHN_MAX_DEFINE; i++) {
		if (aif_bw_tmp[i].is_used != 1)
			continue;

		tmp += aif_bw_tmp[i].bw;

		get_aif_ch_axi_sel(pov_req, (tmp <= (bw_sum / 2)), aif_bw_tmp, bw_sum, i);

		chn_idx = aif_bw_tmp[i].chn_idx;
		dpufd->dss_module.aif_bw[chn_idx] = aif_bw_tmp[i];

		if (g_debug_ovl_online_composer || g_debug_ovl_offline_composer)
			DPU_FB_INFO("fb%d, aif0, chn_idx=%d, axi_sel=%d, bw=%llu.\n",
				dpufd->index, chn_idx, aif_bw_tmp[i].axi_sel, aif_bw_tmp[i].bw);
	}
}

static int8_t get_aif1_axi_sel(int32_t ovl_idx, uint32_t use_mmbuf_cnt)
{
	if ((ovl_idx == DSS_OVL0) || (ovl_idx == DSS_OVL1))
		return ((use_mmbuf_cnt % 2) ? AXI_CHN0 : AXI_CHN1);
	else
		return ((use_mmbuf_cnt % 2) ? AXI_CHN1 : AXI_CHN0);
}

static void dpu_aif_ch_cfg(struct dpu_fb_data_type *dpufd, const dss_overlay_t *pov_req,
	dss_overlay_block_t *pov_h_block, dss_aif_bw_t aif_bw_tmp[], uint32_t aif_bw_size)
{
	uint32_t i;
	dss_layer_t *layer = NULL;
	dss_aif_bw_t *aif1_bw = NULL;
	int chn_idx;
	uint32_t bw_sum = 0;
	int rch_cnt = 0;

	/* i is not chn_idx, is array idx */
	for (i = 0; i < pov_h_block->layer_nums; i++) {
		layer = &pov_h_block->layer_infos[i];
		chn_idx = layer->chn_idx;
		dpu_check_and_no_retval((chn_idx < 0) || (chn_idx >= DSS_CHN_MAX_DEFINE),
			ERR, "layer->chn_idx exceeds array limit\n");

		if (layer->need_cap & (CAP_BASE | CAP_DIM | CAP_PURE_COLOR))
			continue;

		if ((layer->img.width == 0) || (layer->img.height == 0))
			continue;

		/* MMBUF */
		if ((layer->need_cap & CAP_AFBCD) || (layer->need_cap & CAP_HFBCD)) {
			if (layer->dst_rect.y >= pov_h_block->ov_block_rect.y)
				dpufd->use_mmbuf_cnt++;
			else
				continue;

			aif1_bw = &(dpufd->dss_module.aif1_bw[chn_idx]);
			aif1_bw->is_used = 1;
			aif1_bw->chn_idx = chn_idx;
			aif1_bw->axi_sel = get_aif1_axi_sel(pov_req->ovl_idx, dpufd->use_mmbuf_cnt);

			if (g_debug_ovl_online_composer || g_debug_ovl_offline_composer)
				DPU_FB_INFO("fb%d, aif1, chn_idx=%d, axi_sel=%d.\n",
					dpufd->index, chn_idx, aif1_bw->axi_sel);
		}

		if (i >= aif_bw_size) {
			DPU_FB_ERR("aif_bw_tmp overflow\n");
			return;
		}

		aif_bw_tmp[i].bw = (uint64_t)layer->img.buf_size *
			(layer->src_rect.w * layer->src_rect.h) / (layer->img.width * layer->img.height);
		aif_bw_tmp[i].chn_idx = chn_idx;
		aif_bw_tmp[i].axi_sel = AXI_CHN0;
		aif_bw_tmp[i].is_used = 1;

		bw_sum += aif_bw_tmp[i].bw;
		rch_cnt++;
	}

	/* sort */
	aif_bw_sort(aif_bw_tmp, rch_cnt);

	dpu_aif_ch_ov_cfg(dpufd, pov_req, aif_bw_tmp, bw_sum);
}

int dpu_aif_handler(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req,
	dss_overlay_block_t *pov_h_block)
{
	dss_aif_bw_t aif_bw_tmp[DSS_CHN_MAX_DEFINE];

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL!\n");
	dpu_check_and_return(!pov_req, -EINVAL, ERR, "pov_req is NULL!\n");
	dpu_check_and_return(!pov_h_block, -EINVAL, ERR, "pov_h_block is NULL!\n");

	memset(aif_bw_tmp, 0, sizeof(aif_bw_tmp));

	if (pov_req->wb_enable) {
		if (dpu_aif_cfg_for_wblayer(dpufd, pov_req, pov_h_block, aif_bw_tmp, DSS_CHN_MAX_DEFINE) < 0)
			return -EINVAL;
	}

	dpu_aif_ch_cfg(dpufd, pov_req, pov_h_block, aif_bw_tmp, DSS_CHN_MAX_DEFINE);

	return 0;
}

/*
 * DSS AIF
 */
static int g_mid_array[DSS_CHN_MAX_DEFINE] = { 0xb, 0xa, 0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x2, 0x1, 0x3, 0x0 };

void dpu_aif_init(const char __iomem *aif_ch_base,
	dss_aif_t *s_aif)
{
	dpu_check_and_no_retval(!aif_ch_base, ERR, "aif_ch_base is NULL\n");
	dpu_check_and_no_retval(!s_aif, ERR, "s_aif is NULL\n");

	memset(s_aif, 0, sizeof(dss_aif_t));

	s_aif->aif_ch_ctl = inp32(aif_ch_base + AIF_CH_CTL);

	s_aif->aif_ch_hs = inp32(aif_ch_base + AIF_CH_HS);
	s_aif->aif_ch_ls = inp32(aif_ch_base + AIF_CH_LS);
}

void dpu_aif_ch_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *aif_ch_base, dss_aif_t *s_aif)
{
	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL\n");
	dpu_check_and_no_retval(!aif_ch_base, ERR, "aif_ch_base is NULL\n");
	dpu_check_and_no_retval(!s_aif, ERR, "s_aif is NULL\n");

	dpufd->set_reg(dpufd, aif_ch_base + AIF_CH_CTL, s_aif->aif_ch_ctl, 32, 0);

	dpufd->set_reg(dpufd, aif_ch_base + AIF_CH_HS, s_aif->aif_ch_hs, 32, 0);
	dpufd->set_reg(dpufd, aif_ch_base + AIF_CH_LS, s_aif->aif_ch_ls, 32, 0);
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

	chn_idx = wb_layer ? wb_layer->chn_idx : layer->chn_idx;
	aif = &(dpufd->dss_module.aif[chn_idx]);
	dpufd->dss_module.aif_ch_used[chn_idx] = 1;

	aif_bw = &(dpufd->dss_module.aif_bw[chn_idx]);
	dpu_check_and_return((aif_bw->is_used != 1), -EINVAL, ERR, "fb%d, aif_bw->is_used[%d] is invalid!\n",
		dpufd->index, aif_bw->is_used);

	dpu_check_and_return((g_mid_array[chn_idx] < 0) || (g_mid_array[chn_idx] > 0xb), -EINVAL, ERR,
		"fb%d, mid[%d] is invalid!\n", dpufd->index, g_mid_array[chn_idx]);

	aif->aif_ch_ctl = set_bits32(aif->aif_ch_ctl, aif_bw->axi_sel, 1, 0);

	if (g_fpga_flag != 0)
		return 0;

	if ((pov_req->ovl_idx == DSS_OVL2) || (pov_req->ovl_idx == DSS_OVL3)) {
		if ((layer && ((layer->need_cap & CAP_AFBCD) != CAP_AFBCD)) || wb_layer) {
			dss_core_rate = dpufd->dss_vote_cmd.dss_pri_clk_rate;
			if (dss_core_rate == 0) {
				DPU_FB_ERR("fb%d, dss_core_rate[%llu] is invalid!\n",
					dpufd->index, dss_core_rate);
				dss_core_rate = DEFAULT_DSS_CORE_CLK_RATE_L1;
			}
			credit_step_lower = g_dss_min_bandwidth_inbusbusy * 1000000UL * 8 / dss_core_rate;

			/* credit en lower */
			aif->aif_ch_ctl = set_bits32(aif->aif_ch_ctl, 0x0, 3, 8);
			aif->aif_ch_ctl = set_bits32(aif->aif_ch_ctl, 0x2, 4, 12);
			aif->aif_ch_ls = set_bits32(aif->aif_ch_ls, 0x0, 9, 21);
			aif->aif_ch_ls = set_bits32(aif->aif_ch_ls, 0x40, 9, 12);
			aif->aif_ch_ls = set_bits32(aif->aif_ch_ls, credit_step_lower, 7, 4);
			aif->aif_ch_ls = set_bits32(aif->aif_ch_ls, 0x1, 1, 0);
		}
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
			DPU_FB_ERR("fb%d, dss_core_rate[%llu] is invalid!\n", dpufd->index, dss_core_rate);
			dss_core_rate = DEFAULT_DSS_CORE_CLK_RATE_L1;
		}
		if ((layer->src_rect.w > layer->dst_rect.w) &&
			(layer->src_rect.w > get_panel_xres(dpufd))) {
			scfd_h = layer->src_rect.w * 100 / get_panel_xres(dpufd);
		} else {
			scfd_h = 100;
		}
		/* after stretch */
		if (layer->src_rect.h > layer->dst_rect.h)
			scfd_v = layer->src_rect.h * 100 / layer->dst_rect.h;
		else
			scfd_v = 100;

		/* credit_step = pix_f*128/(core_f*16/4)*1.25*scfd_h*scfd_v */
		credit_step = dpufd->panel_info.pxl_clk_rate * 32 * 150 * scfd_h * scfd_v /
			dss_core_rate  / (100 * 100 * 100);

		if (g_debug_ovl_online_composer || g_debug_ovl_offline_composer || g_debug_ovl_credit_step)
			DPU_FB_INFO("fb%d, layer_idx[%d], chn_idx[%d], src_rect[%d,%d,%d,%d], "
				"dst_rect[%d,%d,%d,%d], scfd_h=%d, scfd_v=%d, credit_step=%d.\n",
				dpufd->index, layer->layer_idx, layer->chn_idx,
				layer->src_rect.x, layer->src_rect.y, layer->src_rect.w, layer->src_rect.h,
				layer->dst_rect.x, layer->dst_rect.y, layer->dst_rect.w, layer->dst_rect.h,
				scfd_h, scfd_v, credit_step);

		if (credit_step < 50)
			credit_step = 50;

		aif1->aif_ch_ctl = set_bits32(aif1->aif_ch_ctl, 0x0, 3, 8);
		if (credit_step > 64)
			aif1->aif_ch_hs = set_bits32(aif1->aif_ch_hs, 0x0, 1, 0);
		else
			aif1->aif_ch_hs = set_bits32(aif1->aif_ch_hs, 0x1, 1, 0);

		aif1->aif_ch_hs = set_bits32(aif1->aif_ch_hs, credit_step, 7, 4);
	}
}

static void aif1_ch_config_ovl2_or_ovl3(const struct dpu_fb_data_type *dpufd, const dss_layer_t *layer,
	dss_aif_t *aif1)
{
	uint64_t dss_core_rate;
	uint32_t credit_step_lower;

	if (layer && ((layer->need_cap & CAP_AFBCD) || (layer->need_cap & CAP_HFBCD))) {
		dss_core_rate = dpufd->dss_vote_cmd.dss_pri_clk_rate;
		if (dss_core_rate == 0) {
			DPU_FB_ERR("fb%d, dss_core_rate[%llu] is invalid!\n", dpufd->index, dss_core_rate);
			dss_core_rate = DEFAULT_DSS_CORE_CLK_RATE_L1;
		}

		credit_step_lower = g_dss_min_bandwidth_inbusbusy * 1000000UL * 8 / dss_core_rate;

		/* credit en lower */
		aif1->aif_ch_ctl = set_bits32(aif1->aif_ch_ctl, 0x0, 3, 8);
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

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL!\n");
	dpu_check_and_return(!pov_req, -EINVAL, ERR, "pov_req is NULL!\n");
	dpu_check_and_return(!layer && !wb_layer, -EINVAL, ERR, "layer & wb_layer is NULL!\n");

	dpu_check_and_return(((ovl_idx < DSS_OVL0) || (ovl_idx >= DSS_OVL_IDX_MAX)), -EINVAL, ERR,
		"ovl_idx %d is invalid!\n", ovl_idx);

	if (wb_layer) {
		chn_idx = wb_layer->chn_idx;
		need_cap = wb_layer->need_cap;
	} else {
		chn_idx = layer->chn_idx;
		need_cap = layer->need_cap;
	}

	if (!(need_cap & (CAP_AFBCD | CAP_HFBCD)))
		return 0;

	aif1_bw = &(dpufd->dss_module.aif1_bw[chn_idx]);
	if (aif1_bw->is_used != 1) {
		DPU_FB_ERR("fb%d, aif1_bw->is_used=%d no equal to 1 is err!\n", dpufd->index, aif1_bw->is_used);
		return 0;
	}

	if (g_mid_array[chn_idx] < 0 || g_mid_array[chn_idx] > 0xb) {
		DPU_FB_ERR("fb%d, mid=%d is invalid!\n", dpufd->index, g_mid_array[chn_idx]);
		return 0;
	}

	aif1 = &(dpufd->dss_module.aif1[chn_idx]);
	dpufd->dss_module.aif1_ch_used[chn_idx] = 1;

	aif1->aif_ch_ctl = set_bits32(aif1->aif_ch_ctl, aif1_bw->axi_sel, 1, 0);

	if (g_fpga_flag == 0) {
		if ((ovl_idx == DSS_OVL0) || (ovl_idx == DSS_OVL1))
			aif1_ch_config_ovl0_or_ovl1(dpufd, layer, aif1);
		else
			aif1_ch_config_ovl2_or_ovl3(dpufd, layer, aif1);
	}

	return 0;
}

