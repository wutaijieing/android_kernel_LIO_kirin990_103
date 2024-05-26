/* Copyright (c) Huawei Technologies Co., Ltd. 2013-2021. All rights reserved.
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

#include "dpu_mipi_dsi.h"

struct mipi_dsi_clk_data {
	uint32_t clk_post;
	uint32_t clk_pre;
	uint32_t clk_t_hs_exit;
	uint32_t clk_pre_delay;
	uint32_t clk_t_hs_prepare;
	uint32_t clk_t_lpx;
	uint32_t clk_t_hs_zero;
	uint32_t clk_t_hs_trial;
	uint32_t data_post_delay;
	uint32_t data_t_hs_prepare;
	uint32_t data_t_hs_zero;
	uint32_t data_t_hs_trial;
	uint32_t data_t_lpx;
};

static uint32_t get_data_t_hs_prepare(struct dpu_fb_data_type *dpufd, uint32_t accuracy, uint32_t ui)
{
	struct dpu_panel_info *pinfo = NULL;
	uint32_t data_t_hs_prepare;

	pinfo = &(dpufd->panel_info);

	/*
	 * D-PHY Specification : 40ns + 4*UI <= data_t_hs_prepare <= 85ns + 6*UI
	 * clocked by TXBYTECLKHS
	 * 35 is default adjust value
	 */
	if (pinfo->mipi.data_t_hs_prepare_adjust == 0)
		pinfo->mipi.data_t_hs_prepare_adjust = 35;

	data_t_hs_prepare = ((400 * accuracy + 4 * ui + pinfo->mipi.data_t_hs_prepare_adjust * ui) <=
		(850 * accuracy + 6 * ui - 8 * ui)) ?
		(400 * accuracy + 4 * ui + pinfo->mipi.data_t_hs_prepare_adjust * ui) :
		(850 * accuracy + 6 * ui - 8 * ui);

	return data_t_hs_prepare;
}

static uint32_t get_data_pre_delay(uint32_t lp11_flag, const struct mipi_dsi_phy_ctrl *phy_ctrl,
	uint32_t clk_pre)
{
	uint32_t data_pre_delay = 0;
	/* if use 1080 X 2160 resolution panel,need reduce the lp11 time,and disable noncontinue mode */
	if (lp11_flag != MIPI_SHORT_LP11)
		data_pre_delay = phy_ctrl->clk_pre_delay + 2 + phy_ctrl->clk_t_lpx +
			phy_ctrl->clk_t_hs_prepare + phy_ctrl->clk_t_hs_zero + 8 + clk_pre;

	return data_pre_delay;
}

static uint32_t get_data_pre_delay_reality(uint32_t lp11_flag, const struct mipi_dsi_phy_ctrl *phy_ctrl)
{
	uint32_t data_pre_delay_reality = 0;
	/* if use 1080 X 2160 resolution panel,need reduce the lp11 time,and disable noncontinue mode */
	if (lp11_flag != MIPI_SHORT_LP11)
		data_pre_delay_reality = phy_ctrl->data_pre_delay + 5;

	return data_pre_delay_reality;
}

static uint32_t get_clk_post_delay_reality(uint32_t lp11_flag, const struct mipi_dsi_phy_ctrl *phy_ctrl)
{
	uint32_t clk_post_delay_reality = 0;
	/* if use 1080 X 2160 resolution panel,need reduce the lp11 time,and disable noncontinue mode */
	if (lp11_flag != MIPI_SHORT_LP11)
		clk_post_delay_reality = phy_ctrl->clk_post_delay + 4;

	return clk_post_delay_reality;
}

int mipi_dsi_ulps_cfg(struct dpu_fb_data_type *dpufd, int enable);

static void mipi_dsi_pll_dphy_config(struct mipi_dsi_phy_ctrl *phy_ctrl, uint64_t *lane_clock,
	uint32_t *m_pll, uint32_t *n_pll)
{
	uint64_t vco_div = 1;  /* default clk division */

	/* D PHY Data rate range is from 2500 Mbps to 80 Mbps
	 * The following devil numbers from chip protocol
	 * It contains lots of fixed numbers
	 */
	if ((*lane_clock >= 320) && (*lane_clock <= 2500)) {
		phy_ctrl->rg_band_sel = 0;
		vco_div = 1;
	} else if ((*lane_clock >= 80) && (*lane_clock < 320)) {
		phy_ctrl->rg_band_sel = 1;
		vco_div = 4;
	} else {
		DPU_FB_ERR("80M <= lane_clock< = 2500M, not support lane_clock = %llu M\n", *lane_clock);
	}

	/* accord chip protocol, lane_clock from MHz to Hz */
	*n_pll = 2;
	*m_pll = (uint32_t)(*lane_clock * vco_div * (*n_pll) * 1000000UL / DEFAULT_MIPI_CLK_RATE);

	*lane_clock = *m_pll * (DEFAULT_MIPI_CLK_RATE / (*n_pll)) / vco_div;
	dpu_check_and_no_retval((*lane_clock == 0), ERR, "lane_clock is zero\n");
	if (*lane_clock > 750000000) {  /* 750MHz */
		phy_ctrl->rg_cp = 3;
	/* 80M <= lane_clock <= 750M */
	} else if ((*lane_clock >= 80000000) && (*lane_clock <= 750000000)) {
		phy_ctrl->rg_cp = 1;
	} else {
		DPU_FB_ERR("80M <= lane_clock <= 2500M, not support lane_clock = %llu M.\n", *lane_clock);
	}

	/* chip spec : */
	phy_ctrl->rg_pre_div = (*n_pll) - 1;
	phy_ctrl->rg_div = (*m_pll);
	phy_ctrl->rg_0p8v = 0;
	phy_ctrl->rg_2p5g = 1;
	phy_ctrl->rg_320m = 0;
	phy_ctrl->rg_lpf_r = 0;

	/* TO DO HSTX select VCM VREF */
	phy_ctrl->rg_vrefsel_vcm = 0x5d;
}

static void mipi_dsi_clk_data_lane_dphy_prepare(const struct dpu_panel_info *pinfo,
	struct mipi_dsi_clk_data *clk_data, struct dpu_fb_data_type *dpufd,
	uint32_t *unit_tx_byte_clk_hs, uint64_t lane_clock)
{
	uint32_t ui;
	uint32_t accuracy = 10;  /* magnification */

	dpu_check_and_no_retval((lane_clock == 0), ERR, "lane_clock is zero\n");
	ui =  (uint32_t)(10 * 1000000000UL * accuracy / lane_clock);
	/* unit of measurement */
	*unit_tx_byte_clk_hs = 8 * ui;
	dpu_check_and_no_retval((*unit_tx_byte_clk_hs == 0), ERR, "unit_tx_byte_clk_hs is zero\n");
	/* D-PHY Specification : 60ns + 52*UI <= clk_post */
	clk_data->clk_post = 600 * accuracy + 52 * ui + (*unit_tx_byte_clk_hs) + pinfo->mipi.clk_post_adjust * ui;

	/* D-PHY Specification : clk_pre >= 8*UI */
	clk_data->clk_pre = 8 * ui + (*unit_tx_byte_clk_hs) + pinfo->mipi.clk_pre_adjust * ui;

	/* D-PHY Specification : clk_t_hs_exit >= 100ns */
	clk_data->clk_t_hs_exit = 1000 * accuracy + 100 * accuracy + pinfo->mipi.clk_t_hs_exit_adjust * ui;

	/* clocked by TXBYTECLKHS */
	clk_data->clk_pre_delay = 0 + pinfo->mipi.clk_pre_delay_adjust * ui;

	/* D-PHY Specification : clk_t_hs_trial >= 60ns
	 * clocked by TXBYTECLKHS
	 */
	clk_data->clk_t_hs_trial = 600 * accuracy + 3 * (*unit_tx_byte_clk_hs) + pinfo->mipi.clk_t_hs_trial_adjust * ui;

	/* D-PHY Specification : 38ns <= clk_t_hs_prepare <= 95ns
	 * clocked by TXBYTECLKHS
	 */
	clk_data->clk_t_hs_prepare = 660 * accuracy;

	/* clocked by TXBYTECLKHS */
	clk_data->data_post_delay = 0 + pinfo->mipi.data_post_delay_adjust * ui;

	/* D-PHY Specification : data_t_hs_trial >= max( n*8*UI, 60ns + n*4*UI ), n = 1
	 * clocked by TXBYTECLKHS
	 */
	clk_data->data_t_hs_trial = ((600 * accuracy + 4 * ui) >= (8 * ui) ? (600 * accuracy + 4 * ui) : (8 * ui)) +
		8 * ui + 3 * (*unit_tx_byte_clk_hs) + pinfo->mipi.data_t_hs_trial_adjust * ui;

	/* D-PHY Specification : 40ns + 4*UI <= data_t_hs_prepare <= 85ns + 6*UI
	 * clocked by TXBYTECLKHS
	 */
	clk_data->data_t_hs_prepare = get_data_t_hs_prepare(dpufd, accuracy, ui);
	/* D-PHY chip spec : clk_t_lpx + clk_t_hs_prepare > 200ns
	 * D-PHY Specification : clk_t_lpx >= 50ns
	 * clocked by TXBYTECLKHS
	 */
	clk_data->clk_t_lpx = (uint32_t)(2000 * accuracy + 10 * accuracy + pinfo->mipi.clk_t_lpx_adjust * ui -
		 clk_data->clk_t_hs_prepare);

	/* D-PHY Specification : clk_t_hs_zero + clk_t_hs_prepare >= 300 ns
	 * clocked by TXBYTECLKHS
	 */
	clk_data->clk_t_hs_zero = (uint32_t)(3000 * accuracy + 3 * (*unit_tx_byte_clk_hs) +
		pinfo->mipi.clk_t_hs_zero_adjust * ui - clk_data->clk_t_hs_prepare);

	/* D-PHY chip spec : data_t_lpx + data_t_hs_prepare > 200ns
	 * D-PHY Specification : data_t_lpx >= 50ns
	 * clocked by TXBYTECLKHS
	 */
	clk_data->data_t_lpx = (uint32_t)(2000 * accuracy + 10 * accuracy + pinfo->mipi.data_t_lpx_adjust * ui -
		 clk_data->data_t_hs_prepare);

	/* D-PHY Specification : data_t_hs_zero + data_t_hs_prepare >= 145ns + 10*UI
	 * clocked by TXBYTECLKHS
	 */
	clk_data->data_t_hs_zero = (uint32_t)(1450 * accuracy + 10 * ui +
		3 * (*unit_tx_byte_clk_hs) + pinfo->mipi.data_t_hs_zero_adjust * ui - clk_data->data_t_hs_prepare);
}

static void mipi_dsi_clk_data_lane_dphy_config(const struct dpu_panel_info *pinfo,
	struct mipi_dsi_phy_ctrl *phy_ctrl, struct dpu_fb_data_type *dpufd, uint64_t lane_clock)
{
	uint32_t unit_tx_byte_clk_hs = 0;
	struct mipi_dsi_clk_data clk_data = {0};

	mipi_dsi_clk_data_lane_dphy_prepare(pinfo, &clk_data, dpufd, &unit_tx_byte_clk_hs, lane_clock);

	phy_ctrl->clk_pre_delay = round1(clk_data.clk_pre_delay, unit_tx_byte_clk_hs);
	phy_ctrl->clk_t_hs_prepare = round1(clk_data.clk_t_hs_prepare, unit_tx_byte_clk_hs);
	phy_ctrl->clk_t_lpx = round1(clk_data.clk_t_lpx, unit_tx_byte_clk_hs);
	phy_ctrl->clk_t_hs_zero = round1(clk_data.clk_t_hs_zero, unit_tx_byte_clk_hs);
	phy_ctrl->clk_t_hs_trial = round1(clk_data.clk_t_hs_trial, unit_tx_byte_clk_hs);

	phy_ctrl->data_post_delay = round1(clk_data.data_post_delay, unit_tx_byte_clk_hs);
	phy_ctrl->data_t_hs_prepare = round1(clk_data.data_t_hs_prepare, unit_tx_byte_clk_hs);
	phy_ctrl->data_t_lpx = round1(clk_data.data_t_lpx, unit_tx_byte_clk_hs);
	phy_ctrl->data_t_hs_zero = round1(clk_data.data_t_hs_zero, unit_tx_byte_clk_hs);
	phy_ctrl->data_t_hs_trial = round1(clk_data.data_t_hs_trial, unit_tx_byte_clk_hs);

	phy_ctrl->clk_post_delay = phy_ctrl->data_t_hs_trial + round1(clk_data.clk_post, unit_tx_byte_clk_hs);
	phy_ctrl->data_pre_delay = get_data_pre_delay(pinfo->mipi.lp11_flag, phy_ctrl,
		round1(clk_data.clk_pre, unit_tx_byte_clk_hs));

	phy_ctrl->clk_lane_lp2hs_time = phy_ctrl->clk_pre_delay + phy_ctrl->clk_t_lpx + phy_ctrl->clk_t_hs_prepare +
		phy_ctrl->clk_t_hs_zero + 5 + 7;
	phy_ctrl->clk_lane_hs2lp_time = phy_ctrl->clk_t_hs_trial + phy_ctrl->clk_post_delay + 8 + 4;
	phy_ctrl->data_lane_lp2hs_time = get_data_pre_delay_reality(pinfo->mipi.lp11_flag, phy_ctrl) +
		phy_ctrl->data_t_lpx + phy_ctrl->data_t_hs_prepare + phy_ctrl->data_t_hs_zero + 7;
	phy_ctrl->data_lane_hs2lp_time = phy_ctrl->data_t_hs_trial + 8 + 5;

	phy_ctrl->phy_stop_wait_time = get_clk_post_delay_reality(pinfo->mipi.lp11_flag, phy_ctrl) +
		phy_ctrl->clk_t_hs_trial + round1(clk_data.clk_t_hs_exit, unit_tx_byte_clk_hs) -
		(phy_ctrl->data_post_delay + 4 + phy_ctrl->data_t_hs_trial) + 3;

	phy_ctrl->lane_byte_clk = lane_clock / 8;
	dpu_check_and_no_retval((pinfo->mipi.max_tx_esc_clk == 0), ERR, "pinfo->mipi.max_tx_esc_clk is zero.\n");
	phy_ctrl->clk_division = (((phy_ctrl->lane_byte_clk / 2) % pinfo->mipi.max_tx_esc_clk) > 0) ?
		(uint32_t)(phy_ctrl->lane_byte_clk / 2 / pinfo->mipi.max_tx_esc_clk + 1) :
		(uint32_t)(phy_ctrl->lane_byte_clk / 2 / pinfo->mipi.max_tx_esc_clk);
}

static void get_dsi_dphy_ctrl(struct dpu_fb_data_type *dpufd, struct mipi_dsi_phy_ctrl *phy_ctrl)
{
	struct dpu_panel_info *pinfo = NULL;
	uint32_t dsi_bit_clk;
	uint32_t m_pll;
	uint32_t n_pll;
	uint64_t lane_clock;

	dpu_check_and_no_retval((!phy_ctrl || !dpufd), ERR, "dpufd or phyctrl is NULL.\n");
	pinfo = &(dpufd->panel_info);

	dsi_bit_clk = pinfo->mipi.dsi_bit_clk_upt;
	lane_clock = (uint64_t)(2 * dsi_bit_clk);
	DPU_FB_INFO("Expected : lane_clock = %llu M\n", lane_clock);

	/************************  PLL parameters config  *********************/
	/* chip spec :
	 * If the output data rate is below 320 Mbps, RG_BNAD_SEL should be set to 1.
	 * At this mode a post divider of 1/4 will be applied to VCO.
	 */
	mipi_dsi_pll_dphy_config(phy_ctrl, &lane_clock, &m_pll, &n_pll);

	/********************  clock/data lane parameters config  ******************/
	mipi_dsi_clk_data_lane_dphy_config(pinfo, phy_ctrl, dpufd, lane_clock);

	DPU_FB_INFO("DPHY clock_lane and data_lane config :\n"
		"lane_clock = %llu, n_pll=%u, m_pll=%u\n"
		"rg_cp=%u, rg_band_sel=%u, rg_vrefsel_vcm=%u\n"
		"clk_pre_delay=%u, clk_post_delay=%u\n"
		"clk_t_hs_prepare=%u, clk_t_lpx=%u\n"
		"clk_t_hs_zero=%u, clk_t_hs_trial=%u\n"
		"data_pre_delay=%u, data_post_delay=%u\n"
		"data_t_hs_prepare=%u, data_t_lpx=%u\n"
		"data_t_hs_zero=%u, data_t_hs_trial=%u\n"
		"clk_lane_lp2hs_time=%u, clk_lane_hs2lp_time=%u\n"
		"data_lane_lp2hs_time=%u, data_lane_hs2lp_time=%u\n"
		"phy_stop_wait_time=%u\n",
		lane_clock, n_pll, m_pll,
		pinfo->dsi_phy_ctrl.rg_cp, pinfo->dsi_phy_ctrl.rg_band_sel, phy_ctrl->rg_vrefsel_vcm,
		phy_ctrl->clk_pre_delay, phy_ctrl->clk_post_delay,
		phy_ctrl->clk_t_hs_prepare, phy_ctrl->clk_t_lpx,
		phy_ctrl->clk_t_hs_zero, phy_ctrl->clk_t_hs_trial,
		phy_ctrl->data_pre_delay, phy_ctrl->data_post_delay,
		phy_ctrl->data_t_hs_prepare, phy_ctrl->data_t_lpx,
		phy_ctrl->data_t_hs_zero, phy_ctrl->data_t_hs_trial,
		phy_ctrl->clk_lane_lp2hs_time, phy_ctrl->clk_lane_hs2lp_time,
		phy_ctrl->data_lane_lp2hs_time, phy_ctrl->data_lane_hs2lp_time,
		phy_ctrl->phy_stop_wait_time);
}

static void mipi_dsi_pll_cphy_config(struct mipi_dsi_phy_ctrl *phy_ctrl, uint64_t *lane_clock,
	uint32_t *m_pll, uint32_t *n_pll)
{
	uint64_t vco_div = 1;  /* default clk division */

	if (((*lane_clock) >= 320) && ((*lane_clock) <= 1500)) {
		phy_ctrl->rg_cphy_div = 0;
		vco_div = 1;
	} else if (((*lane_clock) >= 160) && ((*lane_clock) < 320)) {
		phy_ctrl->rg_cphy_div = 1;
		vco_div = 2;
	} else if (((*lane_clock) >= 80) && ((*lane_clock) < 160)) {
		phy_ctrl->rg_cphy_div = 2;
		vco_div = 4;
	} else if (((*lane_clock) >= 40) && ((*lane_clock) < 80)) {
		phy_ctrl->rg_cphy_div = 3;
		vco_div = 8;
	} else {
		DPU_FB_ERR("40M <= lane_clock< = 1500M, not support lane_clock = %llu M\n", (*lane_clock));
	}

	/* accord chip protocol, lane_clock from MHz to Hz */
	*n_pll = 2;
	*m_pll = (uint32_t)((*lane_clock) * vco_div * (*n_pll) * 1000000UL / DEFAULT_MIPI_CLK_RATE);

	*lane_clock = (*m_pll) * (DEFAULT_MIPI_CLK_RATE / (*n_pll)) / vco_div;
	dpu_check_and_no_retval(((*lane_clock) == 0), ERR, "lane_clock is zero\n");
	if (*lane_clock > 750000000)   /* 750Mhz */
		phy_ctrl->rg_cp = 3;
	/* 40M <= lane_clock <= 750M */
	else if ((40000000 <= (*lane_clock)) && ((*lane_clock) <= 750000000))
		phy_ctrl->rg_cp = 1;
	else
		DPU_FB_ERR("40M <= lane_clock <= 1500M, not support lane_clock = %llu M.\n", (*lane_clock));

	/* chip spec : */
	phy_ctrl->rg_pre_div = (*n_pll) - 1;
	phy_ctrl->rg_div = (*m_pll);
	phy_ctrl->rg_0p8v = 0;
	phy_ctrl->rg_2p5g = 1;
	phy_ctrl->rg_320m = 0;
	phy_ctrl->rg_lpf_r = 0;

	/* TO DO HSTX select VCM VREF */
	phy_ctrl->rg_vrefsel_vcm = 0x51;
}

static void mipi_dsi_clk_data_lane_cphy_config(const struct dpu_panel_info *pinfo,
	struct mipi_dsi_phy_ctrl *phy_ctrl, uint64_t lane_clock)
{
	uint32_t accuracy = 10;  /* magnification */
	uint32_t unit_tx_word_clk_hs;
	uint32_t ui;

	dpu_check_and_no_retval((lane_clock == 0), ERR, "lane_clock is zero\n");
	ui = (uint32_t)(10 * 1000000000UL * accuracy / lane_clock);
	/* unit of measurement */
	unit_tx_word_clk_hs = 7 * ui;
	dpu_check_and_no_retval((unit_tx_word_clk_hs == 0), ERR, "unit_tx_word_clk_hs is zero\n");
	/* CPHY Specification: 38ns <= t3_prepare <= 95ns */
	phy_ctrl->t_prepare = 650 * accuracy;

	/* CPHY Specification: 50ns <= t_lpx */
	phy_ctrl->t_lpx = 600 * accuracy + 8 * ui - unit_tx_word_clk_hs;

	/* CPHY Specification: 7*UI <= t_prebegin <= 448UI */
	phy_ctrl->t_prebegin = 350 * ui - unit_tx_word_clk_hs;

	/* CPHY Specification: 7*UI <= t_post <= 224*UI */
	phy_ctrl->t_post = 224 * ui - unit_tx_word_clk_hs;

	phy_ctrl->t_prepare = round1(phy_ctrl->t_prepare, unit_tx_word_clk_hs);
	phy_ctrl->t_lpx = round1(phy_ctrl->t_lpx, unit_tx_word_clk_hs);
	phy_ctrl->t_prebegin = round1(phy_ctrl->t_prebegin, unit_tx_word_clk_hs);
	phy_ctrl->t_post = round1(phy_ctrl->t_post, unit_tx_word_clk_hs);

	phy_ctrl->data_lane_lp2hs_time = phy_ctrl->t_lpx + phy_ctrl->t_prepare + phy_ctrl->t_prebegin + 5 + 17;
	phy_ctrl->data_lane_hs2lp_time = phy_ctrl->t_post + 8 + 5;
	/* pinfo->mipi.max_tx_esc_clk ? 0 */
	phy_ctrl->lane_word_clk = lane_clock / 7;
	dpu_check_and_no_retval((pinfo->mipi.max_tx_esc_clk == 0), ERR, "pinfo->mipi.max_tx_esc_clk is zero.\n");
	phy_ctrl->clk_division = (((phy_ctrl->lane_word_clk / 2) % pinfo->mipi.max_tx_esc_clk) > 0) ?
		(uint32_t)(phy_ctrl->lane_word_clk / 2 / pinfo->mipi.max_tx_esc_clk + 1) :
		(uint32_t)(phy_ctrl->lane_word_clk / 2 / pinfo->mipi.max_tx_esc_clk);

	phy_ctrl->phy_stop_wait_time = phy_ctrl->t_post + 8 + 5;
}

static void get_dsi_cphy_ctrl(struct dpu_fb_data_type *dpufd, struct mipi_dsi_phy_ctrl *phy_ctrl)
{
	struct dpu_panel_info *pinfo = NULL;
	uint32_t m_pll;
	uint32_t n_pll;
	uint64_t lane_clock;

	dpu_check_and_no_retval((!phy_ctrl), ERR, "phy_ctrl is NULL.\n");
	pinfo = &(dpufd->panel_info);

	lane_clock = pinfo->mipi.dsi_bit_clk_upt;
	DPU_FB_INFO("Expected : lane_clock = %llu M\n", lane_clock);

	/************************  PLL parameters config  *********************/
	/* chip spec :
	 * C PHY Data rate range is from 1500 Mbps to 40 Mbps
	 */
	mipi_dsi_pll_cphy_config(phy_ctrl, &lane_clock, &m_pll, &n_pll);

	/********************  data lane parameters config  ******************/
	mipi_dsi_clk_data_lane_cphy_config(pinfo, phy_ctrl, lane_clock);

	DPU_FB_INFO("CPHY clock_lane and data_lane config :\n"
		"lane_clock=%llu, n_pll=%u, m_pll=%u\n"
		"rg_cphy_div=%u\n"
		"rg_cp=%u\n"
		"rg_vrefsel_vcm=%u\n"
		"t_prepare=%u\n"
		"t_lpx=%u\n"
		"t_prebegin=%u\n"
		"t_post=%u\n"
		"lane_word_clk=%llu\n"
		"data_lane_lp2hs_time=%u\n"
		"data_lane_hs2lp_time=%u\n"
		"clk_division=%u\n"
		"phy_stop_wait_time=%u\n",
		lane_clock, n_pll, m_pll,
		phy_ctrl->rg_cphy_div,
		phy_ctrl->rg_cp,
		phy_ctrl->rg_vrefsel_vcm,
		phy_ctrl->t_prepare,
		phy_ctrl->t_lpx,
		phy_ctrl->t_prebegin,
		phy_ctrl->t_post,
		phy_ctrl->lane_word_clk,
		phy_ctrl->data_lane_lp2hs_time,
		phy_ctrl->data_lane_hs2lp_time,
		phy_ctrl->clk_division,
		phy_ctrl->phy_stop_wait_time);
}

static uint32_t mipi_pixel_clk(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = &(dpufd->panel_info);

	if ((pinfo->pxl_clk_rate_div == 0) || (g_fpga_flag == 1))
		return (uint32_t)pinfo->pxl_clk_rate;

	if ((pinfo->ifbc_type == IFBC_TYPE_NONE) && !is_dual_mipi_panel(dpufd))
		pinfo->pxl_clk_rate_div = 1;

	return (uint32_t)pinfo->pxl_clk_rate / pinfo->pxl_clk_rate_div;
}

void mipi_config_phy_test_code(char __iomem *mipi_dsi_base, uint32_t test_code_addr,
	uint32_t test_code_parameter)
{
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL1_OFFSET, test_code_addr);
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL0_OFFSET, 0x00000002);
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL0_OFFSET, 0x00000000);
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL1_OFFSET, test_code_parameter);
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL0_OFFSET, 0x00000002);
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL0_OFFSET, 0x00000000);
}

static void mipi_config_cphy_spec1v0_parameter(char __iomem *mipi_dsi_base,
	const struct dpu_panel_info *pinfo)
{
	uint32_t i;
	uint32_t addr;

	for (i = 0; i <= pinfo->mipi.lane_nums; i++) {
		/* Lane Transmission Property */
		addr = MIPIDSI_PHY_TST_LANE_TRANSMISSION_PROPERTY + (i << 5);
		mipi_config_phy_test_code(mipi_dsi_base, addr, 0x43);

		/* Lane Timing Control - DPHY: THS-PREPARE/CPHY: T3-PREPARE */
		addr = MIPIDSI_PHY_TST_DATA_PREPARE + (i << 5);
		mipi_config_phy_test_code(mipi_dsi_base, addr, dss_reduce(pinfo->dsi_phy_ctrl.t_prepare));

		/* Lane Timing Control - TLPX */
		addr = MIPIDSI_PHY_TST_DATA_TLPX + (i << 5);
		mipi_config_phy_test_code(mipi_dsi_base, addr, dss_reduce(pinfo->dsi_phy_ctrl.t_lpx));
	}
}

static void mipi_config_dphy_spec1v2_parameter(char __iomem *mipi_dsi_base,
	const struct dpu_panel_info *pinfo)
{
	uint32_t i;
	uint32_t addr;

	for (i = 0; i <= (pinfo->mipi.lane_nums + 1); i++) {
		/* Lane Transmission Property */
		addr = MIPIDSI_PHY_TST_LANE_TRANSMISSION_PROPERTY + (i << 5);
		mipi_config_phy_test_code(mipi_dsi_base, addr, 0x43);
	}

	/* pre_delay of clock lane request setting */
	mipi_config_phy_test_code(mipi_dsi_base, MIPIDSI_PHY_TST_CLK_PRE_DELAY,
		dss_reduce(pinfo->dsi_phy_ctrl.clk_pre_delay));

	/* post_delay of clock lane request setting */
	mipi_config_phy_test_code(mipi_dsi_base, MIPIDSI_PHY_TST_CLK_POST_DELAY,
		dss_reduce(pinfo->dsi_phy_ctrl.clk_post_delay));

	/* clock lane timing ctrl - t_lpx */
	mipi_config_phy_test_code(mipi_dsi_base, MIPIDSI_PHY_TST_CLK_TLPX,
		dss_reduce(pinfo->dsi_phy_ctrl.clk_t_lpx));

	/* clock lane timing ctrl - t_hs_prepare */
	mipi_config_phy_test_code(mipi_dsi_base, MIPIDSI_PHY_TST_CLK_PREPARE,
		dss_reduce(pinfo->dsi_phy_ctrl.clk_t_hs_prepare));

	/* clock lane timing ctrl - t_hs_zero */
	mipi_config_phy_test_code(mipi_dsi_base, MIPIDSI_PHY_TST_CLK_ZERO,
		dss_reduce(pinfo->dsi_phy_ctrl.clk_t_hs_zero));

	/* clock lane timing ctrl - t_hs_trial */
	mipi_config_phy_test_code(mipi_dsi_base, MIPIDSI_PHY_TST_CLK_TRAIL,
		dss_reduce(pinfo->dsi_phy_ctrl.clk_t_hs_trial));

	for (i = 0; i <= (pinfo->mipi.lane_nums + 1); i++) {
		if (i == 2)
			i++;  /* addr: lane0:0x60; lane1:0x80; lane2:0xC0; lane3:0xE0 */

		/* data lane pre_delay */
		addr = MIPIDSI_PHY_TST_DATA_PRE_DELAY + (i << 5);
		mipi_config_phy_test_code(mipi_dsi_base, addr, dss_reduce(pinfo->dsi_phy_ctrl.data_pre_delay));

		/* data lane post_delay */
		addr = MIPIDSI_PHY_TST_DATA_POST_DELAY + (i << 5);
		mipi_config_phy_test_code(mipi_dsi_base, addr, dss_reduce(pinfo->dsi_phy_ctrl.data_post_delay));

		/* data lane timing ctrl - t_lpx */
		addr = MIPIDSI_PHY_TST_DATA_TLPX + (i << 5);
		mipi_config_phy_test_code(mipi_dsi_base, addr, dss_reduce(pinfo->dsi_phy_ctrl.data_t_lpx));

		/* data lane timing ctrl - t_hs_prepare */
		addr = MIPIDSI_PHY_TST_DATA_PREPARE + (i << 5);
		mipi_config_phy_test_code(mipi_dsi_base, addr, dss_reduce(pinfo->dsi_phy_ctrl.data_t_hs_prepare));

		/* data lane timing ctrl - t_hs_zero */
		addr = MIPIDSI_PHY_TST_DATA_ZERO + (i << 5);
		mipi_config_phy_test_code(mipi_dsi_base, addr, dss_reduce(pinfo->dsi_phy_ctrl.data_t_hs_zero));

		/* data lane timing ctrl - t_hs_trial */
		addr = MIPIDSI_PHY_TST_DATA_TRAIL + (i << 5);
		mipi_config_phy_test_code(mipi_dsi_base, addr, dss_reduce(pinfo->dsi_phy_ctrl.data_t_hs_trial));

		DPU_FB_INFO("DPHY spec1v2 config :\n"
			"addr=0x%x\n"
			"clk_pre_delay=%u\n"
			"clk_t_hs_trial=%u\n"
			"data_t_hs_zero=%u\n"
			"data_t_lpx=%u\n"
			"data_t_hs_prepare=%u\n",
			addr,
			pinfo->dsi_phy_ctrl.clk_pre_delay,
			pinfo->dsi_phy_ctrl.clk_t_hs_trial,
			pinfo->dsi_phy_ctrl.data_t_hs_zero,
			pinfo->dsi_phy_ctrl.data_t_lpx,
			pinfo->dsi_phy_ctrl.data_t_hs_prepare);
	}
}

static inline void _mipi_set_overlap_size(struct dpu_fb_data_type *dpufd,
	const char __iomem *mipi_dsi_base, struct dss_rect *rect)
{
	struct dpu_panel_info *pinfo = NULL;

	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL\n");
	pinfo = &(dpufd->panel_info);

	if (mipi_dsi_base == dpufd->mipi_dsi0_base)
		rect->w += pinfo->ldi.dpi0_overlap_size;
	else if (mipi_dsi_base == dpufd->mipi_dsi1_base)
		rect->w += pinfo->ldi.dpi1_overlap_size;
	else
		rect->w += pinfo->ldi.dpi0_overlap_size;
}

static void mipi_dsi_cphy_config(char __iomem *mipi_dsi_base, struct dpu_panel_info *pinfo)
{
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010001, 0x3e); /* T3-PREBEGIN */
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010002, 0x1e); /* T3-POST */

	mipi_config_phy_test_code(mipi_dsi_base, 0x00010042, 0x21);

	/* PLL configuration I */
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010046, pinfo->dsi_phy_ctrl.rg_cp +
		(pinfo->dsi_phy_ctrl.rg_lpf_r << 4));

	/* PLL configuration II */
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010048, pinfo->dsi_phy_ctrl.rg_0p8v +
		(pinfo->dsi_phy_ctrl.rg_2p5g << 1) + (pinfo->dsi_phy_ctrl.rg_320m << 2) +
		(pinfo->dsi_phy_ctrl.rg_band_sel << 3) + (pinfo->dsi_phy_ctrl.rg_cphy_div << 4));

	/* PLL configuration III */
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010049, pinfo->dsi_phy_ctrl.rg_pre_div);

	/* PLL configuration IV */
	mipi_config_phy_test_code(mipi_dsi_base, 0x0001004A, pinfo->dsi_phy_ctrl.rg_div);

	mipi_config_phy_test_code(mipi_dsi_base, 0x0001004F, 0xf0);

	if (pinfo->mipi.dsi_bit_clk > 1500) {
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010053, 0xc3);
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010056, 0x07);
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010057, 0xfe);
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010058, 0x03);
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010059, 0x17);
		mipi_config_phy_test_code(mipi_dsi_base, 0x0001005a, 0xff);
	} else {
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010053, 0xc3);
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010056, 0x00);
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010057, 0x00);
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010058, 0x00);
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010059, 0x12);
		mipi_config_phy_test_code(mipi_dsi_base, 0x0001005a, 0xff);
	}

	if (g_fpga_flag == 0)
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010054, 0x07); /* enable BTA */

	/* PLL update control */
	mipi_config_phy_test_code(mipi_dsi_base, 0x0001004B, 0x1);

	/* set cphy spec parameter */
	mipi_config_cphy_spec1v0_parameter(mipi_dsi_base, pinfo);
}

static void mipi_dsi_dphy_config(char __iomem *mipi_dsi_base, struct dpu_panel_info *pinfo)
{
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010042, 0x21);
	/* PLL configuration I */
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010046, pinfo->dsi_phy_ctrl.rg_cp +
		(pinfo->dsi_phy_ctrl.rg_lpf_r << 4));

	/* PLL configuration II */
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010048,
		pinfo->dsi_phy_ctrl.rg_0p8v + (pinfo->dsi_phy_ctrl.rg_2p5g << 1) +
		(pinfo->dsi_phy_ctrl.rg_320m << 2) + (pinfo->dsi_phy_ctrl.rg_band_sel << 3));

	/* PLL configuration III */
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010049, pinfo->dsi_phy_ctrl.rg_pre_div);

	/* PLL configuration IV */
	mipi_config_phy_test_code(mipi_dsi_base, 0x0001004A, pinfo->dsi_phy_ctrl.rg_div);

	mipi_config_phy_test_code(mipi_dsi_base, 0x0001004F, 0xf0);
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010050, 0xc0);
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010051, 0x22);

	if (pinfo->mipi.dsi_bit_clk == 1250) {
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010053, 0xc3);
		mipi_config_phy_test_code(mipi_dsi_base, 0x0001005A, 0xf1);
	} else {
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010053, pinfo->dsi_phy_ctrl.rg_vrefsel_vcm);
	}

	if (g_fpga_flag == 0)
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010054, 0x03); /* enable BTA */

	/* PLL update control */
	mipi_config_phy_test_code(mipi_dsi_base, 0x0001004B, 0x1);
	/* set dphy spec parameter */
	mipi_config_dphy_spec1v2_parameter(mipi_dsi_base, pinfo);
}

static void mipi_dsi_phy_config(struct dpu_fb_data_type *dpufd, char __iomem *mipi_dsi_base,
	struct dpu_panel_info *pinfo)
{
	bool is_ready = false;
	unsigned long dw_jiffies;
	uint32_t cmp_stopstate_val;
	uint32_t tmp;

	set_reg(mipi_dsi_base + MIPIDSI_PHY_IF_CFG_OFFSET, pinfo->mipi.lane_nums, 2, 0);
	set_reg(mipi_dsi_base + MIPIDSI_CLKMGR_CFG_OFFSET, pinfo->dsi_phy_ctrl.clk_division, 8, 0);
	set_reg(mipi_dsi_base + MIPIDSI_CLKMGR_CFG_OFFSET, pinfo->dsi_phy_ctrl.clk_division, 8, 8);

	outp32(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x00000000);

	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL0_OFFSET, 0x00000000);
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL0_OFFSET, 0x00000001);
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL0_OFFSET, 0x00000000);

	if (pinfo->mipi.phy_mode == CPHY_MODE)
		mipi_dsi_cphy_config(mipi_dsi_base, pinfo);
	else
		mipi_dsi_dphy_config(mipi_dsi_base, pinfo);

	outp32(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x0000000F);

	is_ready = false;
	dw_jiffies = jiffies + HZ / 2;
	do {
		tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		if ((tmp & 0x00000001) == 0x00000001) {
			is_ready = true;
			break;
		}
	} while (time_after(dw_jiffies, jiffies));

	if (!is_ready)
		DPU_FB_INFO("fb%d, phylock is not ready!MIPIDSI_PHY_STATUS_OFFSET=0x%x.\n", dpufd->index, tmp);

	if (pinfo->mipi.lane_nums >= DSI_4_LANES)
		cmp_stopstate_val = (BIT(4) | BIT(7) | BIT(9) | BIT(11));
	else if (pinfo->mipi.lane_nums >= DSI_3_LANES)
		cmp_stopstate_val = (BIT(4) | BIT(7) | BIT(9));
	else if (pinfo->mipi.lane_nums >= DSI_2_LANES)
		cmp_stopstate_val = (BIT(4) | BIT(7));
	else
		cmp_stopstate_val = (BIT(4));

	is_ready = false;
	dw_jiffies = jiffies + HZ / 2;
	do {
		tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		if ((tmp & cmp_stopstate_val) == cmp_stopstate_val) {
			is_ready = true;
			break;
		}
	} while (time_after(dw_jiffies, jiffies));

	if (!is_ready)
		DPU_FB_INFO("fb%d, phystopstateclklane is not ready! MIPIDSI_PHY_STATUS_OFFSET=0x%x.\n",
			dpufd->index, tmp);
}

static void mipi_dsi_config_dpi_interface(const struct dpu_panel_info *pinfo, char __iomem *mipi_dsi_base)
{
	set_reg(mipi_dsi_base + MIPIDSI_DPI_VCID_OFFSET, pinfo->mipi.vc, 2, 0);
	set_reg(mipi_dsi_base + MIPIDSI_DPI_COLOR_CODING_OFFSET, pinfo->mipi.color_mode, 4, 0);

	set_reg(mipi_dsi_base + MIPIDSI_DPI_CFG_POL_OFFSET, pinfo->ldi.data_en_plr, 1, 0);
	set_reg(mipi_dsi_base + MIPIDSI_DPI_CFG_POL_OFFSET, pinfo->ldi.vsync_plr, 1, 1);
	set_reg(mipi_dsi_base + MIPIDSI_DPI_CFG_POL_OFFSET, pinfo->ldi.hsync_plr, 1, 2);
	set_reg(mipi_dsi_base + MIPIDSI_DPI_CFG_POL_OFFSET, 0x0, 1, 3);
	set_reg(mipi_dsi_base + MIPIDSI_DPI_CFG_POL_OFFSET, 0x0, 1, 4);
}

static void mipi_dsi_video_mode_config(struct dpu_fb_data_type *dpufd, char __iomem *mipi_dsi_base,
	struct dpu_panel_info *pinfo, dss_rect_t rect)
{
	if (pinfo->mipi.lp11_flag == MIPI_DISABLE_LP11) {
		set_reg(mipi_dsi_base + MIPIDSI_VID_MODE_CFG_OFFSET, 0x0f, 6, 8);
		DPU_FB_INFO("set_reg MIPIDSI_VID_MODE_CFG_OFFSET 0x0f.\n");
	} else {
		set_reg(mipi_dsi_base + MIPIDSI_VID_MODE_CFG_OFFSET, 0x3f, 6, 8);
		DPU_FB_INFO("set_reg MIPIDSI_VID_MODE_CFG_OFFSET 0x3f.\n");
	}
	/* set_reg(mipi_dsi_base + MIPIDSI_VID_MODE_CFG_OFFSET, 0x0, 1, 14); */
	if (is_mipi_video_panel(dpufd)) {
		/* TODO: fix blank display bug when set backlight */
		set_reg(mipi_dsi_base + MIPIDSI_DPI_LP_CMD_TIM_OFFSET, 0x4, 8, 16);
		/* video mode: send read cmd by lp mode */
		set_reg(mipi_dsi_base + MIPIDSI_VID_MODE_CFG_OFFSET, 0x1, 1, 15);
	}

	if ((pinfo->mipi.dsi_version == DSI_1_2_VERSION) &&
		(is_mipi_video_panel(dpufd)) &&
		((pinfo->ifbc_type == IFBC_TYPE_VESA3X_SINGLE) ||
		(pinfo->ifbc_type == IFBC_TYPE_VESA3X_DUAL))) {
		set_reg(mipi_dsi_base + MIPIDSI_VID_PKT_SIZE_OFFSET, rect.w * pinfo->pxl_clk_rate_div, 14, 0);
		/* video vase3x must be set BURST mode */
		if (pinfo->mipi.burst_mode < DSI_BURST_SYNC_PULSES_1) {
			DPU_FB_INFO("pinfo->mipi.burst_mode = %d. video need config BURST mode\n",
				pinfo->mipi.burst_mode);
			pinfo->mipi.burst_mode = DSI_BURST_SYNC_PULSES_1;
		}
	} else {
		set_reg(mipi_dsi_base + MIPIDSI_VID_PKT_SIZE_OFFSET, rect.w, 14, 0);
	}

	/* burst mode */
	set_reg(mipi_dsi_base + MIPIDSI_VID_MODE_CFG_OFFSET, pinfo->mipi.burst_mode, 2, 0);
}

static void mipi_dsi_horizontal_timing_config(struct dpu_fb_data_type *dpufd,
	const struct dpu_panel_info *pinfo, char __iomem *mipi_dsi_base, dss_rect_t rect, uint32_t *hline_time)
{
	uint32_t hsa_time;
	uint32_t hbp_time;
	uint64_t pixel_clk;

	pixel_clk = mipi_pixel_clk(dpufd);
	DPU_FB_INFO("pixel_clk = %llu\n", pixel_clk);

	if (pinfo->mipi.phy_mode == DPHY_MODE) {
		hsa_time = round1(pinfo->ldi.h_pulse_width * pinfo->dsi_phy_ctrl.lane_byte_clk, pixel_clk);
		hbp_time = round1(pinfo->ldi.h_back_porch * pinfo->dsi_phy_ctrl.lane_byte_clk, pixel_clk);
		*hline_time = round1((pinfo->ldi.h_pulse_width + pinfo->ldi.h_back_porch +
			rect.w + pinfo->ldi.h_front_porch) * pinfo->dsi_phy_ctrl.lane_byte_clk, pixel_clk);
	} else {
		hsa_time = round1(pinfo->ldi.h_pulse_width * pinfo->dsi_phy_ctrl.lane_word_clk, pixel_clk);
		hbp_time = round1(pinfo->ldi.h_back_porch * pinfo->dsi_phy_ctrl.lane_word_clk, pixel_clk);
		*hline_time = round1((pinfo->ldi.h_pulse_width + pinfo->ldi.h_back_porch +
			rect.w + pinfo->ldi.h_front_porch) * pinfo->dsi_phy_ctrl.lane_word_clk, pixel_clk);
	}
	DPU_FB_INFO("hsa_time = %d, hbp_time = %d, hline_time = %d.\n", hsa_time, hbp_time, *hline_time);

	set_reg(mipi_dsi_base + MIPIDSI_VID_HSA_TIME_OFFSET, hsa_time, 12, 0);
	set_reg(mipi_dsi_base + MIPIDSI_VID_HBP_TIME_OFFSET, hbp_time, 12, 0);
	set_reg(mipi_dsi_base + MIPIDSI_VID_HLINE_TIME_OFFSET, *hline_time, 15, 0);

	/* Define the Vertical line configuration */
	set_reg(mipi_dsi_base + MIPIDSI_VID_VSA_LINES_OFFSET, pinfo->ldi.v_pulse_width, 10, 0);
	set_reg(mipi_dsi_base + MIPIDSI_VID_VBP_LINES_OFFSET, pinfo->ldi.v_back_porch, 10, 0);
}

static void mipi_dsi_vfp_vsync_config(const struct dpu_panel_info *pinfo,
	char __iomem *mipi_dsi_base, dss_rect_t rect)
{
	set_reg(mipi_dsi_base + MIPIDSI_VID_VFP_LINES_OFFSET, pinfo->ldi.v_front_porch, 10, 0);
	set_reg(mipi_dsi_base + MIPIDSI_VID_VACTIVE_LINES_OFFSET, rect.h, 14, 0);
	set_reg(mipi_dsi_base + MIPIDSI_TO_CNT_CFG_OFFSET, 0x7FF, 16, 0);
}

static void mipi_dsi_phy_timing_config(const struct dpu_panel_info *pinfo, char __iomem *mipi_dsi_base)
{
	/* Configure core's phy parameters */
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_LPCLK_CFG_OFFSET, pinfo->dsi_phy_ctrl.clk_lane_lp2hs_time, 10, 0);
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_LPCLK_CFG_OFFSET, pinfo->dsi_phy_ctrl.clk_lane_hs2lp_time, 10, 16);

	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_RD_CFG_OFFSET, 0x7FFF, 15, 0);
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_CFG_OFFSET, pinfo->dsi_phy_ctrl.data_lane_lp2hs_time, 10, 0);
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_CFG_OFFSET, pinfo->dsi_phy_ctrl.data_lane_hs2lp_time, 10, 16);
}

static void mipi_dsi_auto_ulps_config(const struct dpu_panel_info *pinfo,
	char __iomem *mipi_dsi_base, uint32_t hline_time)
{
	uint32_t twakeup_clk_div = 8;  /* frequency division is 8 */
	uint32_t auto_ulps_enter_delay;
	uint32_t twakeup_cnt;

	auto_ulps_enter_delay = hline_time * 3 / 2;  /* chip protocol */
	/* twakeup_cnt*twakeup_clk_div*t_lanebyteclk>1ms */
	if (pinfo->mipi.phy_mode == CPHY_MODE)
		twakeup_cnt = pinfo->dsi_phy_ctrl.lane_word_clk / 1000 * 3 / 2 / twakeup_clk_div;
	else
		twakeup_cnt = pinfo->dsi_phy_ctrl.lane_byte_clk / 1000 * 3 / 2 / twakeup_clk_div;

	set_reg(mipi_dsi_base + AUTO_ULPS_ENTER_DELAY, auto_ulps_enter_delay, 32, 0);
	set_reg(mipi_dsi_base + AUTO_ULPS_WAKEUP_TIME, twakeup_clk_div, 16, 0);
	set_reg(mipi_dsi_base + AUTO_ULPS_WAKEUP_TIME, twakeup_cnt, 16, 16);
}

void mipi_init(struct dpu_fb_data_type *dpufd, char __iomem *mipi_dsi_base)
{
	struct dpu_panel_info *pinfo = NULL;
	dss_rect_t rect = {0};
	uint32_t hline_time = 0;

	pinfo = &(dpufd->panel_info);

	if (pinfo->mipi.max_tx_esc_clk == 0) {
		DPU_FB_ERR("fb%d, max_tx_esc_clk is invalid!", dpufd->index);
		pinfo->mipi.max_tx_esc_clk = DEFAULT_MAX_TX_ESC_CLK;
	}

	memset(&(pinfo->dsi_phy_ctrl), 0, sizeof(pinfo->dsi_phy_ctrl));

	if (pinfo->mipi.phy_mode == CPHY_MODE)
		get_dsi_cphy_ctrl(dpufd, &(pinfo->dsi_phy_ctrl));
	else
		get_dsi_dphy_ctrl(dpufd, &(pinfo->dsi_phy_ctrl));

	rect.x = 0;
	rect.y = 0;
	rect.w = pinfo->xres;
	rect.h = pinfo->yres;

	mipi_ifbc_get_rect(dpufd, &rect);
	_mipi_set_overlap_size(dpufd, mipi_dsi_base, &rect);

	mipi_dsi_phy_config(dpufd, mipi_dsi_base, pinfo);

	if (is_mipi_cmd_panel(dpufd)) {
		/* config to command mode */
		set_reg(mipi_dsi_base + MIPIDSI_MODE_CFG_OFFSET, 0x1, 1, 0);
		/* ALLOWED_CMD_SIZE */
		set_reg(mipi_dsi_base + MIPIDSI_EDPI_CMD_SIZE_OFFSET, rect.w, 16, 0);

		/* cnt=2 in update-patial scene, cnt nees to be checked for different panels */
		if (pinfo->mipi.hs_wr_to_time == 0)
			set_reg(mipi_dsi_base + MIPIDSI_HS_WR_TO_CNT_OFFSET, 0x1000002, 25, 0);
		else
			set_reg(mipi_dsi_base + MIPIDSI_HS_WR_TO_CNT_OFFSET, (0x1 << 24) | (pinfo->mipi.hs_wr_to_time *
				pinfo->dsi_phy_ctrl.lane_byte_clk / 1000000000UL), 25, 0);
	}

	/* phy_stop_wait_time */
	set_reg(mipi_dsi_base + MIPIDSI_PHY_IF_CFG_OFFSET, pinfo->dsi_phy_ctrl.phy_stop_wait_time, 8, 8);

	/*--------------configuring the DPI packet transmission----------------*/
	/*
	 * 2. Configure the DPI Interface:
	 * This defines how the DPI interface interacts with the controller.
	 */
	mipi_dsi_config_dpi_interface(pinfo, mipi_dsi_base);

	/*
	 * 3. Select the Video Transmission Mode:
	 * This defines how the processor requires the video line to be
	 * transported through the DSI link.
	 */
	/* video mode: low power mode */
	mipi_dsi_video_mode_config(dpufd, mipi_dsi_base, pinfo, rect);
	/* for dsi read, BTA enable */
	set_reg(mipi_dsi_base + MIPIDSI_PCKHDL_CFG_OFFSET, 0x1, 1, 2);

	/*
	 * 4. Define the DPI Horizontal timing configuration:
	 *
	 * Hsa_time = HSA*(PCLK period/Clk Lane Byte Period);
	 * Hbp_time = HBP*(PCLK period/Clk Lane Byte Period);
	 * Hline_time = (HSA+HBP+HACT+HFP)*(PCLK period/Clk Lane Byte Period);
	 */
	mipi_dsi_horizontal_timing_config(dpufd, pinfo, mipi_dsi_base, rect, &hline_time);
	mipi_dsi_vfp_vsync_config(pinfo, mipi_dsi_base, rect);
	mipi_dsi_phy_timing_config(pinfo, mipi_dsi_base);

	/* 16~19bit:pclk_en, pclk_sel, dpipclk_en, dpipclk_sel */
	set_reg(mipi_dsi_base + MIPIDSI_CLKMGR_CFG_OFFSET, 0x5, 4, 16);
	if (is_mipi_cmd_panel(dpufd))
		mipi_dsi_auto_ulps_config(pinfo, mipi_dsi_base, hline_time);

	if (pinfo->mipi.phy_mode == CPHY_MODE)
		set_reg(mipi_dsi_base + PHY_MODE, 0x1, 1, 0); /* 1:cphy */
	else
		set_reg(mipi_dsi_base + PHY_MODE, 0x0, 1, 0); /* 0:dphy */
	/* Waking up Core */
	set_reg(mipi_dsi_base + MIPIDSI_PWR_UP_OFFSET, 0x1, 1, 0);
}

static bool is_clk_enable(const struct dpu_fb_data_type *dpufd, struct clk *clk_tmp, const char *msg)
{
	int ret;

	if (clk_tmp) {
		ret = clk_prepare(clk_tmp);
		if (ret != 0) {
			DPU_FB_ERR("fb%d %s clk_prepare failed, error=%d!\n", dpufd->index, msg, ret);
			return false;
		}

		ret = clk_enable(clk_tmp);
		if (ret != 0) {
			DPU_FB_ERR("fb%d %s clk_enable failed, error=%d!\n", dpufd->index, msg, ret);
			return false;
		}
	}

	return true;
}

int mipi_dsi_clk_enable(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		if (!is_clk_enable(dpufd, dpufd->dss_dphy0_ref_clk, "dss_dphy0_ref_clk"))
			return -EINVAL;

		if (!is_clk_enable(dpufd, dpufd->dss_dphy0_cfg_clk, "dss_dphy0_cfg_clk"))
			return -EINVAL;

		if (!is_clk_enable(dpufd, dpufd->dss_pclk_dsi0_clk, "dss_pclk_dsi0_clk"))
			return -EINVAL;
	}

#ifdef CONFIG_PCLK_PCTRL_USED
	if (!is_clk_enable(dpufd, dpufd->dss_pclk_pctrl_clk, "dss_pclk_pctrl_clk"))
		return -EINVAL;
#endif

	if (is_dual_mipi_panel(dpufd) || (dpufd->index == EXTERNAL_PANEL_IDX)) {
		if (!is_clk_enable(dpufd, dpufd->dss_dphy1_ref_clk, "dss_dphy1_ref_clk"))
			return -EINVAL;

		if (!is_clk_enable(dpufd, dpufd->dss_dphy1_cfg_clk, "dss_dphy1_cfg_clk"))
			return -EINVAL;

		if (!is_clk_enable(dpufd, dpufd->dss_pclk_dsi1_clk, "dss_pclk_dsi1_clk"))
			return -EINVAL;
	}

	return 0;
}

int mipi_dsi_clk_disable(struct dpu_fb_data_type *dpufd)
{
	struct clk *clk_tmp = NULL;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		clk_tmp = dpufd->dss_dphy0_ref_clk;
		if (clk_tmp) {
			clk_disable(clk_tmp);
			clk_unprepare(clk_tmp);
		}

		clk_tmp = dpufd->dss_dphy0_cfg_clk;
		if (clk_tmp) {
			clk_disable(clk_tmp);
			clk_unprepare(clk_tmp);
		}

		clk_tmp = dpufd->dss_pclk_dsi0_clk;
		if (clk_tmp) {
			clk_disable(clk_tmp);
			clk_unprepare(clk_tmp);
		}
	}

#ifdef CONFIG_PCLK_PCTRL_USED
	clk_tmp = dpufd->dss_pclk_pctrl_clk;
	if (clk_tmp) {
		clk_disable(clk_tmp);
		clk_unprepare(clk_tmp);
	}
#endif

	if (is_dual_mipi_panel(dpufd) || (dpufd->index == EXTERNAL_PANEL_IDX)) {
		clk_tmp = dpufd->dss_dphy1_ref_clk;
		if (clk_tmp) {
			clk_disable(clk_tmp);
			clk_unprepare(clk_tmp);
		}

		clk_tmp = dpufd->dss_dphy1_cfg_clk;
		if (clk_tmp) {
			clk_disable(clk_tmp);
			clk_unprepare(clk_tmp);
		}

		clk_tmp = dpufd->dss_pclk_dsi1_clk;
		if (clk_tmp) {
			clk_disable(clk_tmp);
			clk_unprepare(clk_tmp);
		}
	}

	return 0;
}

static int mipi_dsi_on_sub1(struct dpu_fb_data_type *dpufd, char __iomem *mipi_dsi_base)
{
	dpu_check_and_return((!mipi_dsi_base), 0, ERR, "mipi_dsi_base is NULL!\n");
	/* mipi init */
	mipi_init(dpufd, mipi_dsi_base);

	/* switch to cmd mode */
	set_reg(mipi_dsi_base + MIPIDSI_MODE_CFG_OFFSET, 0x1, 1, 0);
	/* cmd mode: low power mode */
	set_reg(mipi_dsi_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x7f, 7, 8);
	set_reg(mipi_dsi_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0xf, 4, 16);
	set_reg(mipi_dsi_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x1, 1, 24);
	/* disable generate High Speed clock */
	set_reg(mipi_dsi_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x0, 1, 0);

	return 0;
}

static void pctrl_dphytx_stopcnt_config(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	uint64_t pctrl_dphytx_stopcnt;
	uint32_t stopcnt_div;

	pinfo = &(dpufd->panel_info);

	stopcnt_div = is_dual_mipi_panel(dpufd) ? 2 : 1;
	/* init: wait DPHY 4 data lane stopstate */
	if (is_mipi_video_panel(dpufd))
		pctrl_dphytx_stopcnt = (uint64_t)(pinfo->ldi.h_back_porch +
			pinfo->ldi.h_front_porch + pinfo->ldi.h_pulse_width + pinfo->xres / stopcnt_div + 5) *
			dpufd->dss_vote_cmd.dss_pclk_pctrl_rate / (pinfo->pxl_clk_rate / stopcnt_div);
	else
		pctrl_dphytx_stopcnt = (uint64_t)(pinfo->ldi.h_back_porch +
			pinfo->ldi.h_front_porch + pinfo->ldi.h_pulse_width + 5) *
			dpufd->dss_vote_cmd.dss_pclk_pctrl_rate / (pinfo->pxl_clk_rate / stopcnt_div);

	outp32(dpufd->pctrl_base + PERI_CTRL29, (uint32_t)pctrl_dphytx_stopcnt);
	if (is_dual_mipi_panel(dpufd))
		outp32(dpufd->pctrl_base + PERI_CTRL32, (uint32_t)pctrl_dphytx_stopcnt);
}

static int mipi_dsi_on_sub2(struct dpu_fb_data_type *dpufd, char __iomem *mipi_dsi_base)
{
	struct dpu_panel_info *pinfo = NULL;

	dpu_check_and_return((!mipi_dsi_base), 0, ERR, "mipi_dsi_base is NULL!\n");
	pinfo = &(dpufd->panel_info);

	if (is_mipi_video_panel(dpufd))
		set_reg(mipi_dsi_base + MIPIDSI_MODE_CFG_OFFSET, 0x0, 1, 0); /* switch to video mode */

	if (is_mipi_cmd_panel(dpufd)) {
		/* cmd mode: high speed mode */
		set_reg(mipi_dsi_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x0, 7, 8);
		set_reg(mipi_dsi_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x0, 4, 16);
		set_reg(mipi_dsi_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x0, 1, 24);
	}

	/* enable EOTP TX */
	if (pinfo->mipi.phy_mode == DPHY_MODE) {
		/* Some vendors don't need eotp check. */
		if (pinfo->mipi.eotp_disable_flag == 1)
			set_reg(mipi_dsi_base + MIPIDSI_PCKHDL_CFG_OFFSET, 0x0, 1, 0);
		else
			set_reg(mipi_dsi_base + MIPIDSI_PCKHDL_CFG_OFFSET, 0x1, 1, 0);
	}

	/* enable generate High Speed clock, non continue */
	if (pinfo->mipi.non_continue_en)
		set_reg(mipi_dsi_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x3, 2, 0);
	else
		set_reg(mipi_dsi_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x1, 2, 0);

	if ((pinfo->mipi.dsi_version == DSI_1_2_VERSION) && is_ifbc_vesa_panel(dpufd))
		set_reg(mipi_dsi_base + MIPIDSI_DSC_PARAMETER_OFFSET, 0x01, 32, 0);

	pctrl_dphytx_stopcnt_config(dpufd);

	return 0;
}

static int mipi_dsi_off_sub(struct dpu_fb_data_type *dpufd, char __iomem *mipi_dsi_base)
{
	dpu_check_and_return(!mipi_dsi_base, -EINVAL, ERR, "mipi_dsi_base is NULL!\n");

	/* switch to cmd mode */
	set_reg(mipi_dsi_base + MIPIDSI_MODE_CFG_OFFSET, 0x1, 1, 0);
	/* cmd mode: low power mode */
	set_reg(mipi_dsi_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x7f, 7, 8);
	set_reg(mipi_dsi_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0xf, 4, 16);
	set_reg(mipi_dsi_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x1, 1, 24);

	/* disable generate High Speed clock */
	set_reg(mipi_dsi_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x0, 1, 0);
	udelay(10);

	/* shutdown d_phy */
	set_reg(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x0, 3, 0);

	return 0;
}

void mipi_dsi_reset(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL!\n");

	set_reg(dpufd->mipi_dsi0_base + MIPIDSI_PWR_UP_OFFSET, 0x0, 1, 0);
	msleep(2);
	set_reg(dpufd->mipi_dsi0_base + MIPIDSI_PWR_UP_OFFSET, 0x1, 1, 0);
}

/* MIPI DPHY GPIO for FPGA */
#define GPIO_MIPI_DPHY_PG_SEL_A_NAME "pg_sel_a"
#define GPIO_MIPI_DPHY_PG_SEL_B_NAME "pg_sel_b"
#define GPIO_MIPI_DPHY_TX_RX_A_NAME "tx_rx_a"
#define GPIO_MIPI_DPHY_TX_RX_B_NAME "tx_rx_b"

static uint32_t g_gpio_pg_sel_a = GPIO_PG_SEL_A;
static uint32_t g_gpio_tx_rx_a = GPIO_TX_RX_A;
static uint32_t g_gpio_pg_sel_b = GPIO_PG_SEL_B;
static uint32_t g_gpio_tx_rx_b = GPIO_TX_RX_B;

static struct gpio_desc g_mipi_dphy_gpio_request_cmds[] = {
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_PG_SEL_A_NAME, &g_gpio_pg_sel_a, 0 },
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_PG_SEL_B_NAME, &g_gpio_pg_sel_b, 0 },
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_TX_RX_A_NAME, &g_gpio_tx_rx_a, 0 },
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_TX_RX_B_NAME, &g_gpio_tx_rx_b, 0 },
};

static struct gpio_desc g_mipi_dphy_gpio_free_cmds[] = {
	{ DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_PG_SEL_A_NAME, &g_gpio_pg_sel_a, 0 },
	{ DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_PG_SEL_B_NAME, &g_gpio_pg_sel_b, 0 },
	{ DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_TX_RX_A_NAME, &g_gpio_tx_rx_a, 0 },
	{ DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_TX_RX_B_NAME, &g_gpio_tx_rx_b, 0 },
};

static struct gpio_desc g_mipi_dphy_gpio_normal_cmds[] = {
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_PG_SEL_A_NAME, &g_gpio_pg_sel_a, 0 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_TX_RX_A_NAME, &g_gpio_tx_rx_a, 1 },

	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_PG_SEL_B_NAME, &g_gpio_pg_sel_b, 0 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_TX_RX_B_NAME, &g_gpio_tx_rx_b, 1 },
};

static struct gpio_desc g_mipi_dphy_gpio_lowpower_cmds[] = {
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_PG_SEL_A_NAME, &g_gpio_pg_sel_a, 0 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_TX_RX_A_NAME, &g_gpio_tx_rx_a, 0 },

	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_PG_SEL_B_NAME, &g_gpio_pg_sel_b, 0 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_TX_RX_B_NAME, &g_gpio_tx_rx_b, 0 },
};

static int mipi_dsi_dphy_fastboot_fpga(struct dpu_fb_data_type *dpufd)
{
	void_unused(dpufd);
	if (g_fpga_flag == 1)
		/* mpi dphy gpio request */
		gpio_cmds_tx(g_mipi_dphy_gpio_request_cmds,
			ARRAY_SIZE(g_mipi_dphy_gpio_request_cmds));

	return 0;
}

static int mipi_dsi_dphy_on_fpga(const struct dpu_fb_data_type *dpufd)
{
	if (dpufd->index == EXTERNAL_PANEL_IDX)
		return 0;

	if (g_fpga_flag == 1) {
		/* mipi dphy gpio request */
		gpio_cmds_tx(g_mipi_dphy_gpio_request_cmds,
			ARRAY_SIZE(g_mipi_dphy_gpio_request_cmds));

		/* mipi dphy gpio normal */
		gpio_cmds_tx(g_mipi_dphy_gpio_normal_cmds,
			ARRAY_SIZE(g_mipi_dphy_gpio_normal_cmds));
	}

	return 0;
}

static int mipi_dsi_dphy_off_fpga(const struct dpu_fb_data_type *dpufd)
{
	if (dpufd->index == EXTERNAL_PANEL_IDX)
		return 0;

	if (g_fpga_flag == 1) {
		/* mipi dphy gpio lowpower */
		gpio_cmds_tx(g_mipi_dphy_gpio_lowpower_cmds,
			ARRAY_SIZE(g_mipi_dphy_gpio_lowpower_cmds));
		/* mipi dphy gpio free */
		gpio_cmds_tx(g_mipi_dphy_gpio_free_cmds,
			ARRAY_SIZE(g_mipi_dphy_gpio_free_cmds));
	}

	return 0;
}

int mipi_dsi_set_fastboot(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	int ret;

	dpu_check_and_return(!pdev, -EINVAL, ERR, "pdev is NULL\n");
	dpufd = platform_get_drvdata(pdev);
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");

	DPU_FB_DEBUG("fb%d, +.\n", dpufd->index);

	mipi_dsi_dphy_fastboot_fpga(dpufd);

	(void)mipi_dsi_clk_enable(dpufd);

	pinfo = &(dpufd->panel_info);
	memset(&(pinfo->dsi_phy_ctrl), 0, sizeof(pinfo->dsi_phy_ctrl));
	if (pinfo->mipi.phy_mode == CPHY_MODE)
		get_dsi_cphy_ctrl(dpufd, &(pinfo->dsi_phy_ctrl));
	else
		get_dsi_dphy_ctrl(dpufd, &(pinfo->dsi_phy_ctrl));

	ret = panel_next_set_fastboot(pdev);

	DPU_FB_DEBUG("fb%d, -.\n", dpufd->index);

	return ret;
}

int mipi_dsi_on(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;

	dpu_check_and_return(!pdev, -EINVAL, ERR, "pdev is NULL\n");
	dpufd = platform_get_drvdata(pdev);
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");

	DPU_FB_DEBUG("fb%d, +.\n", dpufd->index);
	mipi_dsi_dphy_on_fpga(dpufd);

	/* set LCD init step before LCD on */
	dpufd->panel_info.lcd_init_step = LCD_INIT_POWER_ON;
	panel_next_on(pdev);

	/* dis-reset ip_reset_dis_dsi0, ip_reset_dis_dsi1 */
	if (dpufd->index == PRIMARY_PANEL_IDX) {
		if (is_dual_mipi_panel(dpufd))
			outp32(dpufd->peri_crg_base + PERRSTDIS3, 0x30000000);
		else
			outp32(dpufd->peri_crg_base + PERRSTDIS3, 0x10000000);
	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		outp32(dpufd->peri_crg_base + PERRSTDIS3, 0x20000000);
	} else {
		DPU_FB_ERR("fb%d, not supported!\n", dpufd->index);
	}

	(void)mipi_dsi_clk_enable(dpufd);

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		mipi_dsi_on_sub1(dpufd, dpufd->mipi_dsi0_base);
		if (is_dual_mipi_panel(dpufd))
			mipi_dsi_on_sub1(dpufd, dpufd->mipi_dsi1_base);
		/* Here need to exit ulps when panel off bypass ddic power down. */
		if (panel_next_bypass_powerdown_ulps_support(pdev))
			mipi_dsi_ulps_cfg(dpufd, 1);
	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		mipi_dsi_on_sub1(dpufd, dpufd->mipi_dsi1_base);
	} else {
		DPU_FB_ERR("fb%d, not supported!\n", dpufd->index);
	}

	panel_next_on(pdev);

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		mipi_dsi_on_sub2(dpufd, dpufd->mipi_dsi0_base);
		if (is_dual_mipi_panel(dpufd))
			mipi_dsi_on_sub2(dpufd, dpufd->mipi_dsi1_base);
	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		mipi_dsi_on_sub2(dpufd, dpufd->mipi_dsi1_base);
	} else {
		DPU_FB_ERR("fb%d, not supported!\n", dpufd->index);
	}

	/* mipi hs video/command mode */
	panel_next_on(pdev);

	DPU_FB_DEBUG("fb%d, -.\n", dpufd->index);

	return 0;
}

int mipi_dsi_off(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;
	int ret;

	dpu_check_and_return(!pdev, -EINVAL, ERR, "pdev is NULL!\n");
	dpufd = platform_get_drvdata(pdev);
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL!\n");

	DPU_FB_DEBUG("fb%d, +.\n", dpufd->index);

	/* set LCD uninit step before LCD off */
	dpufd->panel_info.lcd_uninit_step = LCD_UNINIT_MIPI_HS_SEND_SEQUENCE;
	ret = panel_next_off(pdev);

	/* TODO: add MIPI LP mode here if necessary
	 * MIPI LP mode end
	 */
	if (dpufd->panel_info.lcd_uninit_step_support)
		ret = panel_next_off(pdev);

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		/* Here need to enter ulps when panel off bypass ddic power down. */
		if (panel_next_bypass_powerdown_ulps_support(pdev))
			mipi_dsi_ulps_cfg(dpufd, 0);
		mipi_dsi_off_sub(dpufd, dpufd->mipi_dsi0_base);
		if (is_dual_mipi_panel(dpufd))
			mipi_dsi_off_sub(dpufd, dpufd->mipi_dsi1_base);
	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		mipi_dsi_off_sub(dpufd, dpufd->mipi_dsi1_base);
	} else {
		DPU_FB_ERR("fb%d, not supported!\n", dpufd->index);
	}

	mipi_dsi_clk_disable(dpufd);

	mipi_dsi_dphy_off_fpga(dpufd);

	/* reset DSI */
	if (dpufd->index == PRIMARY_PANEL_IDX) {
		if (is_dual_mipi_panel(dpufd))
			outp32(dpufd->peri_crg_base + PERRSTEN3, 0x30000000);
		else
			outp32(dpufd->peri_crg_base + PERRSTEN3, 0x10000000);
	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		outp32(dpufd->peri_crg_base + PERRSTEN3, 0x20000000);
	} else {
		DPU_FB_ERR("fb%d, not supported!\n", dpufd->index);
	}

	if (dpufd->panel_info.lcd_uninit_step_support)
		ret = panel_next_off(pdev);

	DPU_FB_DEBUG("fb%d, -.\n", dpufd->index);

	return ret;
}

static int mipi_dsi_check_ulps_stopstate(const struct dpu_fb_data_type *dpufd,
	const char __iomem *mipi_dsi_base, uint32_t cmp_stopstate_val, uint32_t *need_pll_retry, bool enter_ulps)
{
	uint32_t tmp;
	uint32_t try_times;

	if (enter_ulps) { /* ulps enter */
		/* check DPHY data lane ulpsactivenot_status */
		try_times = 0;
		tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		while ((tmp & cmp_stopstate_val) != 0) {
			udelay(10);
			if (++try_times > 100) {  /* try 1ms */
				DPU_FB_ERR("fb%d, request phy data lane enter ulps failed! PHY_STATUS=0x%x.\n",
					dpufd->index, tmp);
				break;
			}

			tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		}
	} else { /* ulps exit */
		/* check DPHY data lane cmp_stopstate_val */
		try_times = 0;
		tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		while ((tmp & cmp_stopstate_val) != cmp_stopstate_val) {
			udelay(10);
			if (++try_times > 100) {  /* try 1ms */
				DPU_FB_ERR("fb%d, check phy data clk lane stop state failed! PHY_STATUS=0x%x.\n",
					dpufd->index, tmp);
				*need_pll_retry |= BIT(1);
				break;
			}

			tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		}
	}

	return 0;
}

static int mipi_dsi_data_clock_lane_enter_ulps(struct dpu_fb_data_type *dpufd,
	char __iomem *mipi_dsi_base, uint32_t cmp_ulpsactivenot_val, uint32_t cmp_stopstate_val)
{
	uint32_t tmp;
	uint32_t try_times;

	/* check DPHY data and clock lane stopstate */
	try_times = 0;
	tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	while ((tmp & cmp_stopstate_val) != cmp_stopstate_val) {
		udelay(10);
		if (++try_times > 100) {  /* try 1ms */
			DPU_FB_ERR("fb%d, check phy data and clk lane stop state failed! PHY_STATUS=0x%x.\n",
				dpufd->index, tmp);
			return -1;
		}
		if ((tmp & cmp_stopstate_val) == (cmp_stopstate_val & (~BIT(2)))) {
			DPU_FB_INFO("fb%d, datalanes are in stop state, pull down phy_txrequestclkhs.\n",
				dpufd->index);
			set_reg(mipi_dsi_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x0, 1, 1);
			set_reg(mipi_dsi_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x0, 1, 0);
		}

		tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	}

	/* disable DPHY clock lane's Hight Speed Clock */
	set_reg(mipi_dsi_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x0, 1, 0);

	/* request that data lane enter ULPS */
	set_reg(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0x4, 4, 0);

	mipi_dsi_check_ulps_stopstate(dpufd, mipi_dsi_base, cmp_ulpsactivenot_val, NULL, true);

	/* request that clock lane enter ULPS */
	if (dpufd->panel_info.mipi.phy_mode == DPHY_MODE) {
		set_reg(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0x5, 4, 0);

		/* check DPHY clock lane ulpsactivenot_status */
		try_times = 0;
		tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		while ((tmp & BIT(3)) != 0) {
			udelay(10);
			if (++try_times > 100) {  /* try 1ms */
				DPU_FB_ERR("fb%d, request phy clk lane enter ulps failed! PHY_STATUS=0x%x.\n",
					dpufd->index, tmp);
				break;
			}

			tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		}
	}

	return 0;
}

static void mipi_dsi_get_ulps_stopstate(
	const struct dpu_fb_data_type *dpufd,
	uint32_t *cmp_ulpsactivenot_val,
	uint32_t *cmp_stopstate_val,
	bool enter_ulps)
{
	if (enter_ulps) { /* ulps enter */
		if (dpufd->panel_info.mipi.lane_nums >= DSI_4_LANES) {
			*cmp_ulpsactivenot_val = (BIT(5) | BIT(8) | BIT(10) | BIT(12));
			*cmp_stopstate_val = (BIT(4) | BIT(7) | BIT(9) | BIT(11));
		} else if (dpufd->panel_info.mipi.lane_nums >= DSI_3_LANES) {
			*cmp_ulpsactivenot_val = (BIT(5) | BIT(8) | BIT(10));
			*cmp_stopstate_val = (BIT(4) | BIT(7) | BIT(9));
		} else if (dpufd->panel_info.mipi.lane_nums >= DSI_2_LANES) {
			*cmp_ulpsactivenot_val = (BIT(5) | BIT(8));
			*cmp_stopstate_val = (BIT(4) | BIT(7));
		} else {
			*cmp_ulpsactivenot_val = (BIT(5));
			*cmp_stopstate_val = (BIT(4));
		}
	} else { /* ulps exit */
		if (dpufd->panel_info.mipi.lane_nums >= DSI_4_LANES) {
			*cmp_ulpsactivenot_val = (BIT(3) | BIT(5) | BIT(8) | BIT(10) | BIT(12));
			*cmp_stopstate_val = (BIT(2) | BIT(4) | BIT(7) | BIT(9) | BIT(11));
		} else if (dpufd->panel_info.mipi.lane_nums >= DSI_3_LANES) {
			*cmp_ulpsactivenot_val = (BIT(3) | BIT(5) | BIT(8) | BIT(10));
			*cmp_stopstate_val = (BIT(2) | BIT(4) | BIT(7) | BIT(9));
		} else if (dpufd->panel_info.mipi.lane_nums >= DSI_2_LANES) {
			*cmp_ulpsactivenot_val = (BIT(3) | BIT(5) | BIT(8));
			*cmp_stopstate_val = (BIT(2) | BIT(4) | BIT(7));
		} else {
			*cmp_ulpsactivenot_val = (BIT(3) | BIT(5));
			*cmp_stopstate_val = (BIT(2) | BIT(4));
		}
		if (dpufd->panel_info.mipi.phy_mode == CPHY_MODE) {
			*cmp_ulpsactivenot_val &= (~BIT(3));
			*cmp_stopstate_val &= (~BIT(2));
		}
	}
}

static int mipi_dsi_ulps_enter(struct dpu_fb_data_type *dpufd,
	char __iomem *mipi_dsi_base, uint32_t mipi_idx)
{
	uint32_t cmp_ulpsactivenot_val = 0;
	uint32_t cmp_stopstate_val = 0;
	uint32_t tmp;

	dpu_check_and_return((!mipi_dsi_base), 0, ERR, "mipi_dsi_base is NULL\n");
	DPU_FB_DEBUG("fb%d, mipi_idx=%d, +!\n", dpufd->index, mipi_idx);
	mipi_dsi_get_ulps_stopstate(dpufd, &cmp_ulpsactivenot_val, &cmp_stopstate_val, true);

	tmp = (uint32_t)inp32(mipi_dsi_base + MIPIDSI_LPCLK_CTRL_OFFSET) & BIT(1);
	if (tmp && (dpufd->panel_info.mipi.phy_mode == DPHY_MODE))
		cmp_stopstate_val |= (BIT(2));

	if (mipi_dsi_data_clock_lane_enter_ulps(dpufd, mipi_dsi_base, cmp_ulpsactivenot_val, cmp_stopstate_val) != 0)
		return 0;

	/* bit13 lock sel enable (dual_mipi_panel bit29 set 1) ,colse clock gate */
	set_reg(dpufd->pctrl_base + PERI_CTRL33, 0x1, 1, 13);
	set_reg(dpufd->peri_crg_base + PERDIS3, 0x3, 4, 28);
	if (mipi_idx == 1) {
		set_reg(dpufd->pctrl_base + PERI_CTRL30, 0x1, 1, 29);
		set_reg(dpufd->peri_crg_base + PERDIS3, 0xf, 4, 28);
	}

	DPU_FB_DEBUG("fb%d, mipi_idx=%d, -!\n", dpufd->index, mipi_idx);

	return 0;
}

static void mipi_dsi_data_clock_lane_exit_ulps(const struct dpu_fb_data_type *dpufd,
	char __iomem *mipi_dsi_base, uint32_t cmp_ulpsactivenot_val, uint32_t *need_pll_retry)
{
	uint32_t tmp;
	uint32_t try_times;
	/* request that data lane and clock lane exit ULPS */

	outp32(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0xF);
	try_times = 0;
	tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	while ((tmp & cmp_ulpsactivenot_val) != cmp_ulpsactivenot_val) {
		udelay(10);
		if (++try_times > 100) { /* try 1ms */
			DPU_FB_ERR("fb%d, request data clock lane exit ulps fail!PHY_STATUS=0x%x.\n",
				dpufd->index, tmp);
			*need_pll_retry = BIT(0);
			break;
		}

		tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	}
}

static int mipi_dsi_ulps_exit(struct dpu_fb_data_type *dpufd,
	char __iomem *mipi_dsi_base, uint32_t mipi_idx)
{
	uint32_t cmp_ulpsactivenot_val = 0;
	uint32_t cmp_stopstate_val = 0;
	uint32_t need_pll_retry = 0;

	dpu_check_and_return(!mipi_dsi_base, -EINVAL, ERR, "mipi_dsi_base is NULL\n");

	DPU_FB_DEBUG("fb%d, mipi_idx=%d, +!\n", dpufd->index, mipi_idx);
	mipi_dsi_get_ulps_stopstate(dpufd, &cmp_ulpsactivenot_val, &cmp_stopstate_val, false);

	if (is_dual_mipi_panel(dpufd))
		set_reg(dpufd->peri_crg_base + PEREN3, 0xf, 4, 28);
	else
		set_reg(dpufd->peri_crg_base + PEREN3, 0x3, 4, 28);

	udelay(100); /* wait pll clk */
	/* bit13 lock sel enable (dual_mipi_panel bit29 set 1) ,colse clock gate */
	set_reg(dpufd->pctrl_base + PERI_CTRL33, 0x0, 1, 13);
	if (is_dual_mipi_panel(dpufd))
		set_reg(dpufd->pctrl_base + PERI_CTRL30, 0x0, 1, 29);

	mipi_dsi_data_clock_lane_exit_ulps(dpufd, mipi_dsi_base, cmp_ulpsactivenot_val, &need_pll_retry);

	/* mipi spec */
	mdelay(1);

	/* clear PHY_ULPS_CTRL */
	outp32(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0x0);

	mipi_dsi_check_ulps_stopstate(dpufd, mipi_dsi_base, cmp_stopstate_val, &need_pll_retry, false);

#if defined(CONFIG_DPU_FB_V410)
	if (need_pll_retry > 0) {
		DPU_FB_ERR("need_pll_retry = 0x%x .\n", need_pll_retry);
		mipi_dsi_pll_status_check_ec(dpufd, mipi_dsi_base);
	}
#endif

	/* enable DPHY clock lane's Hight Speed Clock */
	set_reg(mipi_dsi_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x1, 1, 0);
	if (dpufd->panel_info.mipi.non_continue_en)
		set_reg(mipi_dsi_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x1, 1, 1);

	/* reset dsi */
	outp32(mipi_dsi_base + MIPIDSI_PWR_UP_OFFSET, 0x0);
	udelay(5);
	/* Power_up dsi */
	outp32(mipi_dsi_base + MIPIDSI_PWR_UP_OFFSET, 0x1);

	DPU_FB_DEBUG("fb%d, mipi_idx=%d, -!\n", dpufd->index, mipi_idx);

	return 0;
}

int mipi_dsi_ulps_cfg(struct dpu_fb_data_type *dpufd, int enable)
{
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");

	DPU_FB_DEBUG("fb%d, +.\n", dpufd->index);

	if (enable) {
		mipi_dsi_ulps_exit(dpufd, dpufd->mipi_dsi0_base, 0);
		if (is_dual_mipi_panel(dpufd))
			mipi_dsi_ulps_exit(dpufd, dpufd->mipi_dsi1_base, 1);
	} else {
		mipi_dsi_ulps_enter(dpufd, dpufd->mipi_dsi0_base, 0);
		if (is_dual_mipi_panel(dpufd))
			mipi_dsi_ulps_enter(dpufd, dpufd->mipi_dsi1_base, 1);
	}

	DPU_FB_DEBUG("fb%d, -.\n", dpufd->index);

	return 0;
}

static void mipi_dsi_set_cdphy_bit_clk_upt_cmd(struct dpu_fb_data_type *dpufd,
	char __iomem *mipi_dsi_base, struct mipi_dsi_phy_ctrl *phy_ctrl)
{
	struct dpu_panel_info *pinfo = NULL;
	unsigned long dw_jiffies;
	bool is_ready = false;
	uint32_t tmp;

	dpu_check_and_no_retval((!phy_ctrl), ERR, "phy_ctrl is NULL\n");
	DPU_FB_DEBUG("fb%d +.\n", dpufd->index);

	pinfo = &(dpufd->panel_info);

	/* config parameter M, N, Q of PLL */
	if (pinfo->mipi.phy_mode == CPHY_MODE) {
		if (phy_ctrl->rg_pre_div > pinfo->dsi_phy_ctrl.rg_pre_div) {
			/* PLL configuration III N */
			mipi_config_phy_test_code(mipi_dsi_base, 0x00010049, phy_ctrl->rg_pre_div);
			/* PLL configuration IV M */
			mipi_config_phy_test_code(mipi_dsi_base, 0x0001004A, phy_ctrl->rg_div);
		} else {
			/* PLL configuration IV M */
			mipi_config_phy_test_code(mipi_dsi_base, 0x0001004A, phy_ctrl->rg_div);
			/* PLL configuration III N */
			mipi_config_phy_test_code(mipi_dsi_base, 0x00010049, phy_ctrl->rg_pre_div);
		}
		/* PLL configuration II Q */
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010048, phy_ctrl->rg_0p8v + (phy_ctrl->rg_2p5g << 1) +
			(phy_ctrl->rg_320m << 2) + (phy_ctrl->rg_band_sel << 3) + (phy_ctrl->rg_cphy_div << 4));

	} else {
		if (phy_ctrl->rg_pre_div > pinfo->dsi_phy_ctrl.rg_pre_div) {
			/* PLL configuration III N */
			mipi_config_phy_test_code(mipi_dsi_base, 0x00010049, phy_ctrl->rg_pre_div);
			/* PLL configuration IV M */
			mipi_config_phy_test_code(mipi_dsi_base, 0x0001004A, phy_ctrl->rg_div);
		} else {
			/* PLL configuration IV M */
			mipi_config_phy_test_code(mipi_dsi_base, 0x0001004A, phy_ctrl->rg_div);
			/* PLL configuration III N */
			mipi_config_phy_test_code(mipi_dsi_base, 0x00010049, phy_ctrl->rg_pre_div);
		}
		/* PLL configuration II Q */
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010048, phy_ctrl->rg_0p8v + (phy_ctrl->rg_2p5g << 1) +
			(phy_ctrl->rg_320m << 2) + (phy_ctrl->rg_band_sel << 3));
	}
	/* PLL update control */
	mipi_config_phy_test_code(mipi_dsi_base, 0x0001004B, 0x1);

	/* clk lane HS2LP/LP2HS */
	outp32(mipi_dsi_base + MIPIDSI_PHY_TMR_LPCLK_CFG_OFFSET,
		(phy_ctrl->clk_lane_lp2hs_time + (phy_ctrl->clk_lane_hs2lp_time << 16)));
	/* data lane HS2LP/LP2HS */
	outp32(mipi_dsi_base + MIPIDSI_PHY_TMR_CFG_OFFSET,
		(phy_ctrl->data_lane_lp2hs_time + (phy_ctrl->data_lane_hs2lp_time << 16)));

	/* escape clock dividor */
	set_reg(mipi_dsi_base + MIPIDSI_CLKMGR_CFG_OFFSET,
		(phy_ctrl->clk_division + (phy_ctrl->clk_division << 8)), 16, 0);

	is_ready = false;
	dw_jiffies = jiffies + HZ / 2; /* 500ms */
	do {
		tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		if ((tmp & 0x00000001) == 0x00000001) {
			is_ready = true;
			break;
		}
	} while (time_after(dw_jiffies, jiffies));
	if (!is_ready)
		DPU_FB_INFO("fb%d, phylock is not ready!MIPIDSI_PHY_STATUS_OFFSET=0x%x.\n", dpufd->index, tmp);

	DPU_FB_DEBUG("fb%d -.\n", dpufd->index);
}

static void mipi_dsi_cdphy_horizontal_timing_config(struct dpu_fb_data_type *dpufd,
	const struct dpu_panel_info *pinfo, char __iomem *mipi_dsi_base,
	const struct mipi_dsi_phy_ctrl *phy_ctrl, dss_rect_t rect)
{
	uint32_t pixel_clk;
	uint32_t hline_time;
	uint32_t hsa_time;
	uint32_t hbp_time;

	pixel_clk = mipi_pixel_clk(dpufd);
	dpu_check_and_no_retval((pixel_clk == 0), ERR, "The value of pixel-clk is 0\n");

	if (pinfo->mipi.phy_mode == DPHY_MODE) {
		hsa_time = round1(pinfo->ldi.h_pulse_width * phy_ctrl->lane_byte_clk, pixel_clk);
		hbp_time = round1(pinfo->ldi.h_back_porch * phy_ctrl->lane_byte_clk, pixel_clk);
		hline_time = round1((pinfo->ldi.h_pulse_width + pinfo->ldi.h_back_porch +
			rect.w + pinfo->ldi.h_front_porch) * phy_ctrl->lane_byte_clk, pixel_clk);
	} else {
		hsa_time = round1(pinfo->ldi.h_pulse_width * phy_ctrl->lane_word_clk, pixel_clk);
		hbp_time = round1(pinfo->ldi.h_back_porch * phy_ctrl->lane_word_clk, pixel_clk);
		hline_time = round1((pinfo->ldi.h_pulse_width + pinfo->ldi.h_back_porch +
			rect.w + pinfo->ldi.h_front_porch) * phy_ctrl->lane_word_clk, pixel_clk);
	}

	set_reg(mipi_dsi_base + MIPIDSI_VID_HSA_TIME_OFFSET, hsa_time, 12, 0);
	set_reg(mipi_dsi_base + MIPIDSI_VID_HBP_TIME_OFFSET, hbp_time, 12, 0);
	set_reg(mipi_dsi_base + MIPIDSI_VID_HLINE_TIME_OFFSET, hline_time, 15, 0);

	/* Configure core's phy parameters */
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_LPCLK_CFG_OFFSET, phy_ctrl->clk_lane_lp2hs_time, 10, 0);
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_LPCLK_CFG_OFFSET, phy_ctrl->clk_lane_hs2lp_time, 10, 16);

	outp32(mipi_dsi_base + MIPIDSI_PHY_TMR_CFG_OFFSET,
		(phy_ctrl->data_lane_lp2hs_time + (phy_ctrl->data_lane_hs2lp_time << 16)));
}

static void mipi_dsi_set_cdphy_bit_clk_upt_video(struct dpu_fb_data_type *dpufd,
	char __iomem *mipi_dsi_base, struct mipi_dsi_phy_ctrl *phy_ctrl)
{
	struct dpu_panel_info *pinfo = NULL;
	dss_rect_t rect;
	unsigned long dw_jiffies;
	uint32_t tmp;
	bool is_ready = false;

	dpu_check_and_no_retval((!phy_ctrl), ERR, "phy_ctrl is NULL\n");
	DPU_FB_DEBUG("fb%d +.\n", dpufd->index);

	pinfo = &(dpufd->panel_info);
	pinfo->dsi_phy_ctrl = *phy_ctrl;

	rect.x = 0;
	rect.y = 0;
	rect.w = pinfo->xres;
	rect.h = pinfo->yres;

	mipi_ifbc_get_rect(dpufd, &rect);

	if (pinfo->mipi.phy_mode == CPHY_MODE) {
		/* PLL configuration I */
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010046, phy_ctrl->rg_cp + (phy_ctrl->rg_lpf_r << 4));

		/* PLL configuration II */
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010048, phy_ctrl->rg_0p8v + (phy_ctrl->rg_2p5g << 1) +
			(phy_ctrl->rg_320m << 2) + (phy_ctrl->rg_band_sel << 3) + (phy_ctrl->rg_cphy_div << 4));

		/* PLL configuration III */
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010049, phy_ctrl->rg_pre_div);

		/* PLL configuration IV */
		mipi_config_phy_test_code(mipi_dsi_base, 0x0001004A, phy_ctrl->rg_div);

		/* PLL update control */
		mipi_config_phy_test_code(mipi_dsi_base, 0x0001004B, 0x1);

		/* set cphy spec parameter */
		mipi_config_cphy_spec1v0_parameter(mipi_dsi_base, pinfo);
	} else {
		/* PLL configuration I */
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010046, phy_ctrl->rg_cp + (phy_ctrl->rg_lpf_r << 4));

		/* PLL configuration II */
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010048, phy_ctrl->rg_0p8v + (phy_ctrl->rg_2p5g << 1) +
			(phy_ctrl->rg_320m << 2) + (phy_ctrl->rg_band_sel << 3));

		/* PLL configuration III */
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010049, phy_ctrl->rg_pre_div);

		/* PLL configuration IV */
		mipi_config_phy_test_code(mipi_dsi_base, 0x0001004A, phy_ctrl->rg_div);

		/* PLL update control */
		mipi_config_phy_test_code(mipi_dsi_base, 0x0001004B, 0x1);

		/* set dphy spec parameter */
		mipi_config_dphy_spec1v2_parameter(mipi_dsi_base, pinfo);
	}

	is_ready = false;
	dw_jiffies = jiffies + HZ / 2;
	do {
		tmp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		if ((tmp & 0x00000001) == 0x00000001) {
			is_ready = true;
			break;
		}
	} while (time_after(dw_jiffies, jiffies));

	if (!is_ready)
		DPU_FB_ERR("fb%d, phylock is not ready!MIPIDSI_PHY_STATUS_OFFSET=0x%x.\n", dpufd->index, tmp);
	/* phy_stop_wait_time */
	set_reg(mipi_dsi_base + MIPIDSI_PHY_IF_CFG_OFFSET, phy_ctrl->phy_stop_wait_time, 8, 8);

	/* 4. Define the DPI Horizontal timing configuration: */
	mipi_dsi_cdphy_horizontal_timing_config(dpufd, pinfo, mipi_dsi_base, phy_ctrl, rect);

	DPU_FB_DEBUG("fb%d -.\n", dpufd->index);
}

static bool check_pctrl_trstop_flag(struct dpu_fb_data_type *dpufd, int time_count)
{
	bool is_ready = false;
	int count;
	uint32_t tmp;

	if (is_dual_mipi_panel(dpufd)) {
		for (count = 0; count < time_count; count++) {
			tmp = inp32(dpufd->pctrl_base + PERI_STAT29);
			if ((tmp & 0x3) == 0x3) {
				is_ready = true;
				break;
			}
			udelay(2);
		}
	} else {
		for (count = 0; count < time_count; count++) {
			tmp = inp32(dpufd->pctrl_base + PERI_STAT29);
			if ((tmp & 0x1) == 0x1) {
				is_ready = true;
				break;
			}
			udelay(2);
		}
	}

	return is_ready;
}

#define VFP_TIME_MASK 0x7fff
#define VFP_TIME_OFFSET 10
#define VFP_DEF_TIME 80
#define MILLION_CONVERT 1000000
#define PCTRL_TRY_TIME 10
#define DSI_CLK_BW 1
#define DSI_CLK_BS 0

static void get_new_phy_ctrl_value(struct dpu_fb_data_type *dpufd, const struct dpu_panel_info *pinfo,
	struct mipi_dsi_phy_ctrl *phy_ctrl, uint32_t *stopstate_msk)
{
	if (dpufd->panel_info.mipi.lane_nums == DSI_4_LANES)
		*stopstate_msk = BIT(0);
	else if (dpufd->panel_info.mipi.lane_nums == DSI_3_LANES)
		*stopstate_msk = BIT(0) | BIT(4);
	else if (dpufd->panel_info.mipi.lane_nums == DSI_2_LANES)
		*stopstate_msk = BIT(0) | BIT(3) | BIT(4);
	else
		*stopstate_msk = BIT(0) | BIT(2) | BIT(3) | BIT(4);

	if (pinfo->mipi.phy_mode == CPHY_MODE)
		get_dsi_cphy_ctrl(dpufd, phy_ctrl);
	else
		get_dsi_dphy_ctrl(dpufd, phy_ctrl);
}

static int wait_stopstate_cnt(struct dpu_fb_data_type *dpufd, uint32_t stopstate_msk, uint32_t vfp_time,
	uint8_t esd_enable, timeval_compatible *tv0)
{
	bool is_ready = false;
	timeval_compatible tv1 = {0};
	uint32_t timediff = 0;

	set_reg(dpufd->pctrl_base + PERI_CTRL33, 1, 1, 0);
	set_reg(dpufd->pctrl_base + PERI_CTRL33, stopstate_msk, 5, 3);
	if (is_dual_mipi_panel(dpufd)) {
		set_reg(dpufd->pctrl_base + PERI_CTRL30, 1, 1, 16);
		set_reg(dpufd->pctrl_base + PERI_CTRL30, stopstate_msk, 5, 19);
	}

	while ((!is_ready) && (timediff < vfp_time)) {
		is_ready = check_pctrl_trstop_flag(dpufd, PCTRL_TRY_TIME);
		dpufb_get_timestamp(&tv1);
		timediff = dpufb_timestamp_diff(tv0, &tv1);
	}
	DPU_FB_INFO("timediff=%d us, vfp_time=%d us.\n", timediff, vfp_time);

	set_reg(dpufd->pctrl_base + PERI_CTRL33, 0, 1, 0);
	if (is_dual_mipi_panel(dpufd))
		set_reg(dpufd->pctrl_base + PERI_CTRL30, 0, 1, 16);

	if (!is_ready) {
		if (is_mipi_video_panel(dpufd)) {
			dpufd->panel_info.esd_enable = esd_enable;
			enable_ldi(dpufd);
		}
		DPU_FB_DEBUG("PERI_STAT0 is not ready.\n");
		return -1;
	}
	return 0;
}

static void mipi_dsi_bit_clk_upt_isr_handler_prepare(struct dpu_fb_data_type *dpufd,
	uint32_t *vfp_time, uint64_t *lane_byte_clk, uint8_t *esd_enable)
{
	*lane_byte_clk = dpufd->panel_info.dsi_phy_ctrl.lane_byte_clk;
	if (dpufd->panel_info.mipi.phy_mode == CPHY_MODE)
		*lane_byte_clk = dpufd->panel_info.dsi_phy_ctrl.lane_word_clk;
	*vfp_time = (uint32_t)inp32(dpufd->mipi_dsi0_base + MIPIDSI_VID_HLINE_TIME_OFFSET) & VFP_TIME_MASK;
	if (*lane_byte_clk >= MILLION_CONVERT) {
		*vfp_time = (*vfp_time) * (dpufd->panel_info.ldi.v_front_porch + VFP_TIME_OFFSET) /
			((uint32_t)(*lane_byte_clk / MILLION_CONVERT));
	} else {
		DPU_FB_ERR("vfp_time == 0.\n");
		*vfp_time = VFP_DEF_TIME; /* 80us */
	}

	*esd_enable = dpufd->panel_info.esd_enable;
	if (is_mipi_video_panel(dpufd)) {
		dpufd->panel_info.esd_enable = 0;
		disable_ldi(dpufd);
	}
}

int mipi_dsi_bit_clk_upt_isr_handler(struct dpu_fb_data_type *dpufd)
{
	struct mipi_dsi_phy_ctrl phy_ctrl = {0};
	struct dpu_panel_info *pinfo = NULL;
	uint32_t stopstate_msk = 0;
	uint8_t esd_enable = 0;
	timeval_compatible tv0 = {0};
	uint32_t vfp_time = 0;
	uint64_t lane_byte_clk = 0;
	uint32_t dsi_bit_clk_upt;

	dpu_check_and_return(!dpufd, 0, ERR, "dpufd is NULL\n");

	pinfo = &(dpufd->panel_info);
	dsi_bit_clk_upt = pinfo->mipi.dsi_bit_clk_upt;

	dpu_check_and_return((dpufd->index != PRIMARY_PANEL_IDX), 0, ERR, "fb%d, not support!\n", dpufd->index);

	if (dsi_bit_clk_upt == pinfo->mipi.dsi_bit_clk)
		return 0;

	DPU_FB_DEBUG("fb%d +.\n", dpufd->index);

	dpufb_get_timestamp(&tv0);
	mipi_dsi_bit_clk_upt_isr_handler_prepare(dpufd, &vfp_time, &lane_byte_clk, &esd_enable);

	/* get new phy_ctrl value according to dsi_bit_clk_next */
	get_new_phy_ctrl_value(dpufd, pinfo, &phy_ctrl, &stopstate_msk);

	/* 1. wait stopstate cnt:dphy_stopstate_cnt_en=1 (pctrl_dphy_ctrl[0])
	 * PERI_CTRL33[0:15]-PHY0, PERI_CTRL30[16:31]-PHY1.
	 */
	if (wait_stopstate_cnt(dpufd, stopstate_msk, vfp_time, esd_enable, &tv0) != 0)
		return 0;

	if (is_mipi_cmd_panel(dpufd)) {
		mipi_dsi_set_cdphy_bit_clk_upt_cmd(dpufd, dpufd->mipi_dsi0_base, &phy_ctrl);
		if (is_dual_mipi_panel(dpufd))
			mipi_dsi_set_cdphy_bit_clk_upt_cmd(dpufd, dpufd->mipi_dsi1_base, &phy_ctrl);
	} else {
		set_reg(dpufd->mipi_dsi0_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x0, DSI_CLK_BW, DSI_CLK_BS);
		mipi_dsi_set_cdphy_bit_clk_upt_video(dpufd, dpufd->mipi_dsi0_base, &phy_ctrl);
		set_reg(dpufd->mipi_dsi0_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x1, DSI_CLK_BW, DSI_CLK_BS);
		if (is_dual_mipi_panel(dpufd)) {
			set_reg(dpufd->mipi_dsi1_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x0, DSI_CLK_BW, DSI_CLK_BS);
			mipi_dsi_set_cdphy_bit_clk_upt_video(dpufd, dpufd->mipi_dsi1_base, &phy_ctrl);
			set_reg(dpufd->mipi_dsi1_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x1, DSI_CLK_BW, DSI_CLK_BS);
		}

		pinfo->esd_enable = esd_enable;
		enable_ldi(dpufd);
	}

	DPU_FB_INFO("Mipi clk successfully changed from [%d]M switch to [%d]M!\n",
		pinfo->mipi.dsi_bit_clk, dsi_bit_clk_upt);

	pinfo->dsi_phy_ctrl = phy_ctrl;
	pinfo->mipi.dsi_bit_clk = dsi_bit_clk_upt;

	DPU_FB_DEBUG("fb%d -.\n", dpufd->index);

	return 0;
}

int mipi_dsi_reset_underflow_clear(struct dpu_fb_data_type *dpufd)
{
	void_unused(dpufd);
	return 0;
}
