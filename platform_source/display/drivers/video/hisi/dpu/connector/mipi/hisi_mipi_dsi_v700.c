/*
 * Copyright (c) 2012-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include <soc_dte_define.h>

#include "hisi_mipi_dsi.h"


#define VFP_TIME_MASK 0x7fff
#define VFP_TIME_OFFSET 10
#define VFP_DEF_TIME 80
#define MILLION_CONVERT 1000000
#define PCTRL_TRY_TIME 10
#define DSI_CLK_BW 1
#define DSI_CLK_BS 0

#define VCO_CLK_MIN_VALUE 2750

// TODO: read g_fpga_flag from dts, should define in a common place
uint32_t dpu_g_fpga_flag = 1;

static int mipi_dsi_ulps_cfg(struct hisi_connector_device *connector_dev, char __iomem *mipi_dsi_base, int enable);

static char __iomem *mipi_get_dsi_base(char __iomem *dss_base_addr, uint32_t index)
{
	char __iomem *mipi_dsi_base = NULL;

	if (index == DSI0_INDEX) {
		mipi_dsi_base = dss_base_addr + DPU_MIPI_DSI0_OFFSET;
	} else {
		mipi_dsi_base = dss_base_addr + DPU_MIPI_DSI1_OFFSET;
	}

	return mipi_dsi_base;
}

static uint32_t mipi_get_dsi_index(char __iomem *dss_base_addr, char __iomem *mipi_dsi_base)
{
	uint32_t dsi_idx = DSI0_INDEX;

	if (mipi_dsi_base == dss_base_addr + DPU_MIPI_DSI1_OFFSET)
		dsi_idx = DSI1_INDEX;

	return dsi_idx;
}

static uint32_t get_data_t_hs_prepare(struct hisi_panel_info *pinfo, uint32_t accuracy, uint32_t ui)
{
	uint32_t data_t_hs_prepare;
	uint32_t prepare_val1;
	uint32_t prepare_val2;

	disp_check_and_return(!pinfo, 0, err, "pinfo is NULL\n");

	/*
	 * D-PHY Specification : 40ns + 4*UI <= data_t_hs_prepare <= 85ns + 6*UI
	 * clocked by TXBYTECLKHS
	 * 35 is default adjust value
	 */
	if (pinfo->mipi.data_t_hs_prepare_adjust == 0)
		pinfo->mipi.data_t_hs_prepare_adjust = 35;

	prepare_val1 = 400 * accuracy + 4 * ui + pinfo->mipi.data_t_hs_prepare_adjust * ui;
	prepare_val2 = 850 * accuracy + 6 * ui - 8 * ui;
	data_t_hs_prepare = (prepare_val1 <= prepare_val2) ? prepare_val1 : prepare_val2;

	return data_t_hs_prepare;
}

static uint32_t get_data_pre_delay(uint32_t lp11_flag, struct mipi_dsi_phy_ctrl *phy_ctrl, uint32_t clk_pre)
{
	uint32_t data_pre_delay = 0;
	/* if use 1080 X 2160 resolution panel,need reduce the lp11 time, and disable noncontinue mode */
	if (lp11_flag != MIPI_SHORT_LP11)
		data_pre_delay = phy_ctrl->clk_pre_delay + 2 + phy_ctrl->clk_t_lpx +
			phy_ctrl->clk_t_hs_prepare + phy_ctrl->clk_t_hs_zero + 8 + clk_pre;

	return data_pre_delay;
}

static uint32_t get_data_pre_delay_reality(uint32_t lp11_flag, struct mipi_dsi_phy_ctrl *phy_ctrl)
{
	uint32_t data_pre_delay_reality = 0;
	/* if use 1080 X 2160 resolution panel,need reduce the lp11 time, and disable noncontinue mode */
	if (lp11_flag != MIPI_SHORT_LP11)
		data_pre_delay_reality = phy_ctrl->data_pre_delay + 5;
	return data_pre_delay_reality;
}

static uint32_t get_clk_post_delay_reality(uint32_t lp11_flag, struct mipi_dsi_phy_ctrl *phy_ctrl)
{
	uint32_t clk_post_delay_reality = 0;
	/* if use 1080 X 2160 resolution panel,need reduce the lp11 time, and disable noncontinue mode */
	if (lp11_flag != MIPI_SHORT_LP11)
		clk_post_delay_reality = phy_ctrl->clk_post_delay + 4;
	return clk_post_delay_reality;
}

static uint64_t get_default_lane_byte_clk(struct hisi_panel_info *pinfo)
{
	int rg_pll_posdiv = 0;
	uint32_t post_div[6] = { 1, 2, 4, 8, 16, 32 };  /* clk division */
	uint64_t lane_clock;
	uint64_t vco_clk;
	uint32_t rg_pll_fbkdiv;

	if (pinfo->mipi.dsi_bit_clk_default == 0) {
		disp_pr_warn("reset dsi_bit_clk_default %u M -> %u M\n",
			pinfo->mipi.dsi_bit_clk_default, pinfo->mipi.dsi_bit_clk);
		pinfo->mipi.dsi_bit_clk_default = pinfo->mipi.dsi_bit_clk;
	}

	lane_clock = (uint64_t)(pinfo->mipi.dsi_bit_clk_default);
	if (pinfo->mipi.phy_mode == DPHY_MODE)
		lane_clock = lane_clock * 2;

	disp_pr_debug("default lane_clock %llu M\n", lane_clock);

	vco_clk = lane_clock * post_div[rg_pll_posdiv];
	/* chip restrain, vco_clk_min and post_div index */
	while ((vco_clk <= 2000) && (rg_pll_posdiv < 5)) {
		rg_pll_posdiv++;
		vco_clk = lane_clock * post_div[rg_pll_posdiv];
	}
	vco_clk = vco_clk * 1000000;  /* MHZ to HZ */
	rg_pll_fbkdiv = vco_clk / DEFAULT_MIPI_CLK_RATE;
	lane_clock = rg_pll_fbkdiv * DEFAULT_MIPI_CLK_RATE / post_div[rg_pll_posdiv];

	disp_pr_debug("vco_clk %llu, rg_pll_fbkdiv %d, rg_pll_posdiv %d, lane_clock %llu\n",
		vco_clk, rg_pll_fbkdiv, rg_pll_posdiv, lane_clock);

	/* lanebyte clk formula which is stated in cdphy spec */
	if (pinfo->mipi.phy_mode == DPHY_MODE)
		return lane_clock / 8;
	else
		return lane_clock / 7;
}

static void mipi_dsi_pll_dphy_config(struct mipi_dsi_phy_ctrl *phy_ctrl, uint64_t *lane_clock,
	uint32_t *m_pll, uint32_t *n_pll)
{
	uint64_t vco_div = 1;  /* default clk division */
	uint64_t vco_clk = 0;
	uint32_t post_div[6] = { 1, 2, 4, 8, 16, 32 }; /* clk division */
	int post_div_idx = 0;

	if (dpu_g_fpga_flag) {
		/* D PHY Data rate range is from 2500 Mbps to 80 Mbps
		 * The following devil numbers from chip protocol
		 * It contains lots of fixed numbers
		 */
		if ((*lane_clock >= 320) && (*lane_clock <= 2500)) {
			phy_ctrl->rg_band_sel = 0;
			vco_div = 1;  /* clk division */
		} else if ((*lane_clock >= 80) && (*lane_clock < 320)) {
			phy_ctrl->rg_band_sel = 1;
			vco_div = 4; /* clk division */
		} else {
			disp_pr_err("80M <= lane_clock< = 2500M, not support lane_clock = %llu M\n", *lane_clock);
		}

		/* accord chip protocol, lane_clock from MHz to Hz */
		*n_pll = 2;
		*m_pll = (uint32_t)((*lane_clock) * vco_div * (*n_pll) * 1000000UL / DEFAULT_MIPI_CLK_RATE);

		*lane_clock = (*m_pll) * (DEFAULT_MIPI_CLK_RATE / (*n_pll)) / vco_div;
		if (*lane_clock > 750000000)  /* 750MHz */
			phy_ctrl->rg_cp = 3;
		/* 80M <= lane_clock <= 750M */
		else if ((*lane_clock >= 80000000) && (*lane_clock <= 750000000))
			phy_ctrl->rg_cp = 1;
		else
			disp_pr_err("80M <= lane_clock< = 2500M, not support lane_clock = %llu M\n", *lane_clock);

		phy_ctrl->rg_pre_div = *n_pll - 1;
		phy_ctrl->rg_div = *m_pll;
	} else {
		phy_ctrl->rg_pll_prediv = 0;
		vco_clk = (*lane_clock) * post_div[post_div_idx];
		/* vcc_clk_min and post_div index */
		while ((vco_clk <= VCO_CLK_MIN_VALUE) && (post_div_idx < 5)) {
			post_div_idx++;
			vco_clk = (*lane_clock) * post_div[post_div_idx];
		}
		vco_clk = vco_clk * 1000000; /* MHZ to HZ */
		phy_ctrl->rg_pll_posdiv = post_div_idx;
		phy_ctrl->rg_pll_fbkdiv = vco_clk / DEFAULT_MIPI_CLK_RATE;

		*lane_clock = phy_ctrl->rg_pll_fbkdiv * DEFAULT_MIPI_CLK_RATE / post_div[phy_ctrl->rg_pll_posdiv];
		disp_pr_info("rg_pll_prediv=%d, rg_pll_posdiv=%d, rg_pll_fbkdiv=%d\n",
			phy_ctrl->rg_pll_prediv, phy_ctrl->rg_pll_posdiv,
			phy_ctrl->rg_pll_fbkdiv);
	}

	/* The following devil numbers from chip protocol */
	phy_ctrl->rg_0p8v = 0;
	phy_ctrl->rg_2p5g = 1;
	phy_ctrl->rg_320m = 0;
	phy_ctrl->rg_lpf_r = 0;
}

static void mipi_dsi_clk_data_lane_dphy_config(struct hisi_panel_info *pinfo, struct mipi_dsi_phy_ctrl *phy_ctrl,
	uint64_t lane_clock)
{
	uint32_t accuracy;
	uint32_t ui;
	uint32_t unit_tx_byte_clk_hs;
	uint32_t clk_post;
	uint32_t clk_pre;
	uint32_t clk_t_hs_exit;
	uint32_t clk_pre_delay;
	uint32_t clk_t_hs_prepare;
	uint32_t clk_t_hs_trial;
	uint32_t data_post_delay;
	uint32_t data_t_hs_trial;
	uint32_t data_t_hs_prepare;
	uint32_t clk_t_lpx;
	uint32_t clk_t_hs_zero;
	uint32_t data_t_hs_zero;
	uint32_t data_t_lpx;

	/******************  clock/data lane parameters config  ******************/
	disp_check_and_no_retval((lane_clock == 0), err, "lane_clock is zero\n");

	accuracy = 10;  /* magnification */
	ui =  (uint32_t)(10 * 1000000000UL * accuracy / lane_clock);
	/* unit of measurement */
	unit_tx_byte_clk_hs = 8 * ui;
	disp_check_and_no_retval((unit_tx_byte_clk_hs == 0), err, "unit_tx_byte_clk_hs is zero\n");
	/* D-PHY Specification : 60ns + 52*UI <= clk_post */
	clk_post = 600 * accuracy + 52 * ui + unit_tx_byte_clk_hs + pinfo->mipi.clk_post_adjust * ui;

	/* D-PHY Specification : clk_pre >= 8*UI */
	clk_pre = 8 * ui + unit_tx_byte_clk_hs + pinfo->mipi.clk_pre_adjust * ui;

	/* D-PHY Specification : clk_t_hs_exit >= 100ns */
	clk_t_hs_exit = (uint32_t)(1000 * accuracy + 100 * accuracy + pinfo->mipi.clk_t_hs_exit_adjust * ui);

	/* clocked by TXBYTECLKHS */
	clk_pre_delay = 0 + pinfo->mipi.clk_pre_delay_adjust * ui;

	/* D-PHY Specification : clk_t_hs_trial >= 60ns clocked by TXBYTECLKHS */
	clk_t_hs_trial = 600 * accuracy + 3 * unit_tx_byte_clk_hs + pinfo->mipi.clk_t_hs_trial_adjust * ui;

	/* D-PHY Specification : 38ns <= clk_t_hs_prepare <= 95ns clocked by TXBYTECLKHS */
	clk_t_hs_prepare = 660 * accuracy;

	/* clocked by TXBYTECLKHS */
	data_post_delay = 0 + pinfo->mipi.data_post_delay_adjust * ui;

	/*
	 * D-PHY Specification : data_t_hs_trial >= max( n*8*UI, 60ns + n*4*UI ),
	 * n = 1. clocked by TXBYTECLKHS
	 */
	data_t_hs_trial = ((600 * accuracy + 4 * ui) >= (8 * ui) ?
		(600 * accuracy + 4 * ui) : (8 * ui)) + 8 * ui +
		3 * unit_tx_byte_clk_hs + pinfo->mipi.data_t_hs_trial_adjust * ui;

	/*
	 * D-PHY Specification : 40ns + 4*UI <= data_t_hs_prepare <= 85ns + 6*UI
	 * clocked by TXBYTECLKHS
	 */
	data_t_hs_prepare = get_data_t_hs_prepare(pinfo, accuracy, ui);
	/*
	 * D-PHY chip spec : clk_t_lpx + clk_t_hs_prepare > 200ns
	 * D-PHY Specification : clk_t_lpx >= 50ns
	 * clocked by TXBYTECLKHS
	 */
	clk_t_lpx = (uint32_t)(2000 * accuracy + 10 * accuracy +
		pinfo->mipi.clk_t_lpx_adjust * ui - clk_t_hs_prepare);
	/*
	 * D-PHY Specification : clk_t_hs_zero + clk_t_hs_prepare >= 300 ns
	 * clocked by TXBYTECLKHS
	 */
	clk_t_hs_zero = (uint32_t)(3000 * accuracy + 3 * unit_tx_byte_clk_hs +
		pinfo->mipi.clk_t_hs_zero_adjust * ui - clk_t_hs_prepare);
	/*
	 * D-PHY chip spec : data_t_lpx + data_t_hs_prepare > 200ns
	 * D-PHY Specification : data_t_lpx >= 50ns
	 * clocked by TXBYTECLKHS
	 */
	data_t_lpx = (uint32_t)(2000 * accuracy + 10 * accuracy +
		pinfo->mipi.data_t_lpx_adjust * ui - data_t_hs_prepare);
	/*
	 * D-PHY Specification : data_t_hs_zero + data_t_hs_prepare >= 145ns + 10*UI
	 * clocked by TXBYTECLKHS
	 */
	data_t_hs_zero = (uint32_t)(1450 * accuracy + 10 * ui +
		3 * unit_tx_byte_clk_hs + pinfo->mipi.data_t_hs_zero_adjust * ui -
		data_t_hs_prepare);

	/* The follow code from chip code, It contains lots of fixed numbers */
	phy_ctrl->clk_pre_delay = round1(clk_pre_delay, unit_tx_byte_clk_hs);
	phy_ctrl->clk_t_hs_prepare = round1(clk_t_hs_prepare, unit_tx_byte_clk_hs);
	phy_ctrl->clk_t_lpx = round1(clk_t_lpx, unit_tx_byte_clk_hs);
	phy_ctrl->clk_t_hs_zero = round1(clk_t_hs_zero, unit_tx_byte_clk_hs);
	phy_ctrl->clk_t_hs_trial = round1(clk_t_hs_trial, unit_tx_byte_clk_hs);

	phy_ctrl->data_post_delay = round1(data_post_delay, unit_tx_byte_clk_hs);
	phy_ctrl->data_t_hs_prepare = round1(data_t_hs_prepare, unit_tx_byte_clk_hs);
	phy_ctrl->data_t_lpx = round1(data_t_lpx, unit_tx_byte_clk_hs);
	phy_ctrl->data_t_hs_zero = round1(data_t_hs_zero, unit_tx_byte_clk_hs);
	phy_ctrl->data_t_hs_trial = round1(data_t_hs_trial, unit_tx_byte_clk_hs);

	phy_ctrl->clk_post_delay = phy_ctrl->data_t_hs_trial + round1(clk_post, unit_tx_byte_clk_hs);
	phy_ctrl->data_pre_delay = get_data_pre_delay(pinfo->mipi.lp11_flag,
		phy_ctrl, round1(clk_pre, unit_tx_byte_clk_hs));

	phy_ctrl->clk_lane_lp2hs_time = phy_ctrl->clk_pre_delay +
		phy_ctrl->clk_t_lpx + phy_ctrl->clk_t_hs_prepare +
		phy_ctrl->clk_t_hs_zero + 5 + 7;
	phy_ctrl->clk_lane_hs2lp_time = phy_ctrl->clk_t_hs_trial +
		phy_ctrl->clk_post_delay + 8 + 4;
	phy_ctrl->data_lane_lp2hs_time =
		get_data_pre_delay_reality(pinfo->mipi.lp11_flag, phy_ctrl) +
		phy_ctrl->data_t_lpx + phy_ctrl->data_t_hs_prepare +
		phy_ctrl->data_t_hs_zero + pinfo->mipi.data_lane_lp2hs_time_adjust + 7;

	phy_ctrl->data_lane_hs2lp_time = phy_ctrl->data_t_hs_trial + 8 + 5;

	phy_ctrl->phy_stop_wait_time =
		get_clk_post_delay_reality(pinfo->mipi.lp11_flag, phy_ctrl) +
		phy_ctrl->clk_t_hs_trial + round1(clk_t_hs_exit, unit_tx_byte_clk_hs) -
		(phy_ctrl->data_post_delay + 4 + phy_ctrl->data_t_hs_trial) + 3;

	phy_ctrl->lane_byte_clk = lane_clock / 8;
	phy_ctrl->clk_division =
		(((phy_ctrl->lane_byte_clk / 2) % pinfo->mipi.max_tx_esc_clk) > 0) ?
		(uint32_t)(phy_ctrl->lane_byte_clk / 2 / pinfo->mipi.max_tx_esc_clk + 1) :
		(uint32_t)(phy_ctrl->lane_byte_clk / 2 / pinfo->mipi.max_tx_esc_clk);

	phy_ctrl->lane_byte_clk_default = get_default_lane_byte_clk(pinfo);
}

static void get_dsi_dphy_ctrl(struct hisi_panel_info *pinfo)
{
	uint32_t m_pll = 0;
	uint32_t n_pll = 0;
	uint32_t dsi_bit_clk;
	uint64_t lane_clock;
	struct mipi_dsi_phy_ctrl *phy_ctrl = NULL;

	dsi_bit_clk = pinfo->mipi.dsi_bit_clk_upt;
	phy_ctrl = &(pinfo->dsi_phy_ctrl);

	lane_clock = (uint64_t)(2 * dsi_bit_clk);
	disp_pr_info("Expected : lane_clock = %llu M\n", lane_clock);

	/************************  PLL parameters config  *********************/
	/* chip spec :
	 * If the output data rate is below 320 Mbps, RG_BNAD_SEL should be set to 1.
	 * At this mode a post divider of 1/4 will be applied to VCO.
	 */
	mipi_dsi_pll_dphy_config(phy_ctrl, &lane_clock, &m_pll, &n_pll);

	/* HSTX select VCM VREF */
	phy_ctrl->rg_vrefsel_vcm = 0;
	if (pinfo->mipi.rg_vrefsel_vcm_adjust != 0) {
		phy_ctrl->rg_vrefsel_vcm = pinfo->mipi.rg_vrefsel_vcm_adjust;
		disp_pr_info("rg_vrefsel_vcm=0x%x\n", phy_ctrl->rg_vrefsel_vcm);
	}

	mipi_dsi_clk_data_lane_dphy_config(pinfo, phy_ctrl, lane_clock);

	disp_pr_info("DPHY clock_lane and data_lane config :\n"
		"lane_clock = %llu, n_pll=%u, m_pll=%u\n"
		"rg_cp=%u\n"
		"rg_band_sel=%u\n"
		"rg_vrefsel_vcm=%u\n"
		"clk_pre_delay=%u\n"
		"clk_post_delay=%u\n"
		"clk_t_hs_prepare=%u\n"
		"clk_t_lpx=%u\n"
		"clk_t_hs_zero=%u\n"
		"clk_t_hs_trial=%u\n"
		"data_pre_delay=%u\n"
		"data_post_delay=%u\n"
		"data_t_hs_prepare=%u\n"
		"data_t_lpx=%u\n"
		"data_t_hs_zero=%u\n"
		"data_t_hs_trial=%u\n"
		"clk_lane_lp2hs_time=%u\n"
		"clk_lane_hs2lp_time=%u\n"
		"data_lane_lp2hs_time=%u\n"
		"data_lane_hs2lp_time=%u\n"
		"phy_stop_wait_time=%u\n",
		lane_clock, n_pll, m_pll,
		pinfo->dsi_phy_ctrl.rg_cp,
		pinfo->dsi_phy_ctrl.rg_band_sel,
		phy_ctrl->rg_vrefsel_vcm,
		phy_ctrl->clk_pre_delay,
		phy_ctrl->clk_post_delay,
		phy_ctrl->clk_t_hs_prepare,
		phy_ctrl->clk_t_lpx,
		phy_ctrl->clk_t_hs_zero,
		phy_ctrl->clk_t_hs_trial,
		phy_ctrl->data_pre_delay,
		phy_ctrl->data_post_delay,
		phy_ctrl->data_t_hs_prepare,
		phy_ctrl->data_t_lpx,
		phy_ctrl->data_t_hs_zero,
		phy_ctrl->data_t_hs_trial,
		phy_ctrl->clk_lane_lp2hs_time,
		phy_ctrl->clk_lane_hs2lp_time,
		phy_ctrl->data_lane_lp2hs_time,
		phy_ctrl->data_lane_hs2lp_time,
		phy_ctrl->phy_stop_wait_time);
}

static void mipi_dsi_get_cphy_div(struct mipi_dsi_phy_ctrl *phy_ctrl, uint64_t lane_clock, uint64_t *vco_div)
{
	/* C PHY Data rate range is from 1500 Mbps to 40 Mbps
	* The following devil numbers from chip protocol
	* It contains lots of fixed numbers
	*/
	if ((lane_clock >= 320) && (lane_clock <= 1500)) {
		phy_ctrl->rg_cphy_div = 0;
		*vco_div = 1;  /* clk division */
	} else if ((lane_clock >= 160) && (lane_clock < 320)) {
		phy_ctrl->rg_cphy_div = 1;
		*vco_div = 2;  /* clk division */
	} else if ((lane_clock >= 80) && (lane_clock < 160)) {
		phy_ctrl->rg_cphy_div = 2;
		*vco_div = 4;  /* clk division */
	} else if ((lane_clock >= 40) && (lane_clock < 80)) {
		phy_ctrl->rg_cphy_div = 3;
		*vco_div = 8;  /* clk division */
	} else {
		disp_pr_err("40M <= lane_clock< = 1500M, not support lane_clock = %llu M\n", lane_clock);
	}
}

static void mipi_dsi_pll_cphy_config(struct mipi_dsi_phy_ctrl *phy_ctrl, uint64_t *lane_clock,
	uint32_t *m_pll, uint32_t *n_pll)
{
	uint64_t vco_div = 1;  /* default clk division */
	uint64_t vco_clk = 0;
	uint32_t post_div[6] = { 1, 2, 4, 8, 16, 32 };  /* clk division */
	int post_div_idx = 0;

	if (dpu_g_fpga_flag) {
		mipi_dsi_get_cphy_div(phy_ctrl, *lane_clock, &vco_div);

		/* accord chip protocol, lane_clock from MHz to Hz */
		*n_pll = 2;
		*m_pll = (uint32_t)((*lane_clock) * vco_div * (*n_pll) * 1000000UL / DEFAULT_MIPI_CLK_RATE);
		if (vco_div)
			*lane_clock = (*m_pll) * (DEFAULT_MIPI_CLK_RATE / (*n_pll)) / vco_div;
		if (*lane_clock > 750000000)  /* 750Mhz */
			phy_ctrl->rg_cp = 3;
		/* 40M <= lane_clock <= 750M */
		else if ((*lane_clock >= 40000000) && (*lane_clock <= 750000000))
			phy_ctrl->rg_cp = 1;
		else
			disp_pr_err("40M <= lane_clock< = 1500M, not support lane_clock = %llu M\n", *lane_clock);

		phy_ctrl->rg_pre_div = *n_pll - 1;
		phy_ctrl->rg_div = *m_pll;
	} else {
		phy_ctrl->rg_pll_prediv = 0;
		vco_clk = (*lane_clock) * post_div[post_div_idx];
		while ((vco_clk <= VCO_CLK_MIN_VALUE) && (post_div_idx < 5)) {
			post_div_idx++;
			vco_clk = (*lane_clock) * post_div[post_div_idx];
		}
		vco_clk = vco_clk * 1000000;  /* MHz to Hz */
		phy_ctrl->rg_pll_posdiv = post_div_idx;
		phy_ctrl->rg_pll_fbkdiv = vco_clk / DEFAULT_MIPI_CLK_RATE;
		*lane_clock = phy_ctrl->rg_pll_fbkdiv * DEFAULT_MIPI_CLK_RATE /
			post_div[phy_ctrl->rg_pll_posdiv];
		disp_pr_info("rg_pll_prediv=%d, rg_pll_posdiv=%d, rg_pll_fbkdiv=%d\n",
			phy_ctrl->rg_pll_prediv, phy_ctrl->rg_pll_posdiv,
			phy_ctrl->rg_pll_fbkdiv);
	}

	/* The following devil numbers from chip protocol */
	phy_ctrl->rg_0p8v = 0;
	phy_ctrl->rg_2p5g = 1;
	phy_ctrl->rg_320m = 0;
	phy_ctrl->rg_lpf_r = 0;
}

static void mipi_dsi_clk_data_lane_cphy_config(struct hisi_panel_info *pinfo, struct mipi_dsi_phy_ctrl *phy_ctrl,
	uint64_t lane_clock)
{
	uint32_t accuracy;
	uint32_t ui;
	uint32_t unit_tx_word_clk_hs;

	/********************  data lane parameters config  ******************/
	disp_check_and_no_retval((lane_clock == 0), err, "lane_clock is zero\n");
	accuracy = 10;  /* magnification */
	ui = (uint32_t)(10 * 1000000000UL * accuracy / lane_clock);
	/* unit of measurement */
	unit_tx_word_clk_hs = 7 * ui;
	disp_check_and_no_retval((unit_tx_word_clk_hs == 0), err, "unit_tx_word_clk_hs is zero\n");

	if (pinfo->mipi.mininum_phy_timing_flag == 1) {
		/* CPHY Specification: 38ns <= t3_prepare <= 95ns */
		phy_ctrl->t_prepare = MIN_T3_PREPARE_PARAM * accuracy;

		/* CPHY Specification: 50ns <= t_lpx */
		phy_ctrl->t_lpx = MIN_T3_LPX_PARAM * accuracy + 8 * ui - unit_tx_word_clk_hs;

		/* CPHY Specification: 7*UI <= t_prebegin <= 448UI */
		phy_ctrl->t_prebegin = MIN_T3_PREBEGIN_PARAM * ui - unit_tx_word_clk_hs;

		/* CPHY Specification: 7*UI <= t_post <= 224*UI */
		phy_ctrl->t_post = MIN_T3_POST_PARAM * ui - unit_tx_word_clk_hs;
	} else {
		/* CPHY Specification: 38ns <= t3_prepare <= 95ns */
		/* 380 * accuracy - unit_tx_word_clk_hs; */
		phy_ctrl->t_prepare = T3_PREPARE_PARAM * accuracy;

		/* CPHY Specification: 50ns <= t_lpx */
		phy_ctrl->t_lpx =  T3_LPX_PARAM * accuracy + 8 * ui - unit_tx_word_clk_hs;

		/* CPHY Specification: 7*UI <= t_prebegin <= 448UI */
		phy_ctrl->t_prebegin =  T3_PREBEGIN_PARAM * ui - unit_tx_word_clk_hs;

		/* CPHY Specification: 7*UI <= t_post <= 224*UI */
		phy_ctrl->t_post = T3_POST_PARAM * ui - unit_tx_word_clk_hs;
	}

	/* The follow code from chip code, It contains lots of fixed numbers */
	phy_ctrl->t_prepare = round1(phy_ctrl->t_prepare, unit_tx_word_clk_hs);
	phy_ctrl->t_lpx = round1(phy_ctrl->t_lpx, unit_tx_word_clk_hs);
	phy_ctrl->t_prebegin = round1(phy_ctrl->t_prebegin, unit_tx_word_clk_hs);
	phy_ctrl->t_post = round1(phy_ctrl->t_post, unit_tx_word_clk_hs);

	phy_ctrl->data_lane_lp2hs_time = phy_ctrl->t_lpx + phy_ctrl->t_prepare +
		phy_ctrl->t_prebegin + 5 + 17;
	phy_ctrl->data_lane_hs2lp_time = phy_ctrl->t_post + 8 + 5;

	phy_ctrl->lane_word_clk = lane_clock / 7;
	phy_ctrl->clk_division =
		(((phy_ctrl->lane_word_clk / 2) % pinfo->mipi.max_tx_esc_clk) > 0) ?
		(uint32_t)(phy_ctrl->lane_word_clk / 2 / pinfo->mipi.max_tx_esc_clk + 1) :
		(uint32_t)(phy_ctrl->lane_word_clk / 2 / pinfo->mipi.max_tx_esc_clk);

	phy_ctrl->phy_stop_wait_time = phy_ctrl->t_post + 8 + 5;
	phy_ctrl->lane_byte_clk_default = get_default_lane_byte_clk(pinfo);
}

static void get_dsi_cphy_ctrl(struct hisi_panel_info *pinfo)
{

	uint32_t m_pll = 0;
	uint32_t n_pll = 0;
	uint64_t lane_clock;
	struct mipi_dsi_phy_ctrl *phy_ctrl = NULL;

	lane_clock = pinfo->mipi.dsi_bit_clk_upt;
	phy_ctrl = &(pinfo->dsi_phy_ctrl);

	disp_pr_info("Expected : lane_clock = %llu M\n", lane_clock);

	/************************  PLL parameters config  *********************/
	mipi_dsi_pll_cphy_config(phy_ctrl, &lane_clock, &m_pll, &n_pll);

	/* HSTX select VCM VREF */
	phy_ctrl->rg_vrefsel_vcm = 0x51;

	mipi_dsi_clk_data_lane_cphy_config(pinfo, phy_ctrl, lane_clock);

	disp_pr_info("CPHY clock_lane and data_lane config :\n"
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

static uint32_t mipi_pixel_clk(struct hisi_connector_device *connector_dev)
{
	struct hisi_panel_info *pinfo = NULL;

	pinfo = connector_dev->fix_panel_info;
	disp_check_and_return(!pinfo, -1, err, "pinfo is NULL\n");

	if ((pinfo->timing_info.pxl_clk_rate_div == 0) || (dpu_g_fpga_flag == 1))
		return (uint32_t)pinfo->timing_info.pxl_clk_rate;

	if ((pinfo->ifbc_type == IFBC_TYPE_NONE) && (connector_dev->slaver_count == 0))
		pinfo->timing_info.pxl_clk_rate_div = 1;

	return (uint32_t)pinfo->timing_info.pxl_clk_rate / pinfo->timing_info.pxl_clk_rate_div;
}

static void mipi_config_phy_test_code(char __iomem *mipi_dsi_base,
	uint32_t test_code_addr, uint32_t test_code_parameter)
{
	outp32(DPU_DSI_CDPHY_TEST_CTRL_1_ADDR(mipi_dsi_base), test_code_addr);
	outp32(DPU_DSI_CDPHY_TEST_CTRL_0_ADDR(mipi_dsi_base), 0x00000002);
	outp32(DPU_DSI_CDPHY_TEST_CTRL_0_ADDR(mipi_dsi_base), 0x00000000);
	outp32(DPU_DSI_CDPHY_TEST_CTRL_1_ADDR(mipi_dsi_base), test_code_parameter);
	outp32(DPU_DSI_CDPHY_TEST_CTRL_0_ADDR(mipi_dsi_base), 0x00000002);
	outp32(DPU_DSI_CDPHY_TEST_CTRL_0_ADDR(mipi_dsi_base), 0x00000000);
}

static void mipi_config_cphy_spec1v0_parameter(char __iomem *mipi_dsi_base,
	struct hisi_panel_info *pinfo, const struct mipi_dsi_phy_ctrl *phy_ctrl)
{
	uint32_t i;
	uint32_t addr = 0;

	for (i = 0; i <= pinfo->mipi.lane_nums; i++) {
		if (dpu_g_fpga_flag) {
			/* Lane Transmission Property */
			addr = MIPIDSI_PHY_TST_LANE_TRANSMISSION_PROPERTY + (i << 5);
			mipi_config_phy_test_code(mipi_dsi_base, addr, 0x43);
		}

		/* Lane Timing Control - DPHY: THS-PREPARE/CPHY: T3-PREPARE */
		addr = MIPIDSI_PHY_TST_DATA_PREPARE + (i << 5);

		mipi_config_phy_test_code(mipi_dsi_base, addr, disp_reduce(phy_ctrl->t_prepare));

		/* Lane Timing Control - TLPX */
		addr = MIPIDSI_PHY_TST_DATA_TLPX + (i << 5);
		mipi_config_phy_test_code(mipi_dsi_base, addr, disp_reduce(phy_ctrl->t_lpx));
	}
}

static void mipi_config_dphy_spec1v2_parameter(char __iomem *mipi_dsi_base,
	struct hisi_panel_info *pinfo, const struct mipi_dsi_phy_ctrl *phy_ctrl)
{
	uint32_t i;
	uint32_t addr = 0;

	for (i = 0; i <= (pinfo->mipi.lane_nums + 1); i++) {
		/* Lane Transmission Property */
		addr = MIPIDSI_PHY_TST_LANE_TRANSMISSION_PROPERTY + (i << 5);
		mipi_config_phy_test_code(mipi_dsi_base, addr, 0x43);
	}

	/* pre_delay of clock lane request setting */
	mipi_config_phy_test_code(mipi_dsi_base, MIPIDSI_PHY_TST_CLK_PRE_DELAY,
		disp_reduce(phy_ctrl->clk_pre_delay));

	/* post_delay of clock lane request setting */
	mipi_config_phy_test_code(mipi_dsi_base, MIPIDSI_PHY_TST_CLK_POST_DELAY,
		disp_reduce(phy_ctrl->clk_post_delay));

	/* clock lane timing ctrl - t_lpx */
	mipi_config_phy_test_code(mipi_dsi_base, MIPIDSI_PHY_TST_CLK_TLPX,
		disp_reduce(phy_ctrl->clk_t_lpx));

	/* clock lane timing ctrl - t_hs_prepare */
	mipi_config_phy_test_code(mipi_dsi_base, MIPIDSI_PHY_TST_CLK_PREPARE,
		disp_reduce(phy_ctrl->clk_t_hs_prepare));

	/* clock lane timing ctrl - t_hs_zero */
	mipi_config_phy_test_code(mipi_dsi_base, MIPIDSI_PHY_TST_CLK_ZERO,
		disp_reduce(phy_ctrl->clk_t_hs_zero));

	/* clock lane timing ctrl - t_hs_trial */
	mipi_config_phy_test_code(mipi_dsi_base, MIPIDSI_PHY_TST_CLK_TRAIL,
		disp_reduce(phy_ctrl->clk_t_hs_trial));

	for (i = 0; i <= (pinfo->mipi.lane_nums + 1); i++) {
		if (i == 2)
			i++;  /* addr: lane0:0x60; lane1:0x80; lane2:0xC0; lane3:0xE0 */

		/* data lane pre_delay */
		addr = MIPIDSI_PHY_TST_DATA_PRE_DELAY + (i << 5);
		mipi_config_phy_test_code(mipi_dsi_base, addr, disp_reduce(phy_ctrl->data_pre_delay));

		/* data lane post_delay */
		addr = MIPIDSI_PHY_TST_DATA_POST_DELAY + (i << 5);
		mipi_config_phy_test_code(mipi_dsi_base, addr, disp_reduce(phy_ctrl->data_post_delay));

		/* data lane timing ctrl - t_lpx */
		addr = MIPIDSI_PHY_TST_DATA_TLPX + (i << 5);
		mipi_config_phy_test_code(mipi_dsi_base, addr, disp_reduce(phy_ctrl->data_t_lpx));

		/* data lane timing ctrl - t_hs_prepare */
		addr = MIPIDSI_PHY_TST_DATA_PREPARE + (i << 5);
		mipi_config_phy_test_code(mipi_dsi_base, addr, disp_reduce(phy_ctrl->data_t_hs_prepare));

		/* data lane timing ctrl - t_hs_zero */
		addr = MIPIDSI_PHY_TST_DATA_ZERO + (i << 5);
		mipi_config_phy_test_code(mipi_dsi_base, addr, disp_reduce(phy_ctrl->data_t_hs_zero));

		/* data lane timing ctrl - t_hs_trial */
		addr = MIPIDSI_PHY_TST_DATA_TRAIL + (i << 5);
		mipi_config_phy_test_code(mipi_dsi_base, addr, disp_reduce(phy_ctrl->data_t_hs_trial));


		disp_pr_info("DPHY spec1v2 config :\n"
			"addr=0x%x\n"
			"clk_pre_delay=%u\n"
			"clk_t_hs_trial=%u\n"
			"data_t_hs_zero=%u\n"
			"data_t_lpx=%u\n"
			"data_t_hs_prepare=%u\n",
			addr,
			phy_ctrl->clk_pre_delay,
			phy_ctrl->clk_t_hs_trial,
			phy_ctrl->data_t_hs_zero,
			phy_ctrl->data_t_lpx,
			phy_ctrl->data_t_hs_prepare);
	}
}

static void mipi_cdphy_PLL_configuration(char __iomem *mipi_dsi_base, struct hisi_panel_info *pinfo,
	uint32_t rg_cphy_div_param)
{
		if (dpu_g_fpga_flag) {
			/* PLL configuration I */
			mipi_config_phy_test_code(mipi_dsi_base, 0x00010046,
				pinfo->dsi_phy_ctrl.rg_cp + (pinfo->dsi_phy_ctrl.rg_lpf_r << 4));

			/* PLL configuration II */
			mipi_config_phy_test_code(mipi_dsi_base, 0x00010048,
				pinfo->dsi_phy_ctrl.rg_0p8v +
				(pinfo->dsi_phy_ctrl.rg_2p5g << 1) +
				(pinfo->dsi_phy_ctrl.rg_320m << 2) +
				(pinfo->dsi_phy_ctrl.rg_band_sel << 3) +
				rg_cphy_div_param);

			/* PLL configuration III */
			mipi_config_phy_test_code(mipi_dsi_base, 0x00010049,
				pinfo->dsi_phy_ctrl.rg_pre_div);

			/* PLL configuration IV */
			mipi_config_phy_test_code(mipi_dsi_base, 0x0001004A,
				pinfo->dsi_phy_ctrl.rg_div);
		} else {
			mipi_config_phy_test_code(mipi_dsi_base, 0x00010048,
				pinfo->dsi_phy_ctrl.rg_pll_posdiv);
			mipi_config_phy_test_code(mipi_dsi_base, 0x00010049,
				(pinfo->dsi_phy_ctrl.rg_pll_prediv << 4) |
				(pinfo->dsi_phy_ctrl.rg_pll_fbkdiv >> 8));
			mipi_config_phy_test_code(mipi_dsi_base, 0x0001004A,
				(pinfo->dsi_phy_ctrl.rg_pll_fbkdiv & 0xFF));
		}
}

static void mipi_cdphy_init_config(char __iomem *mipi_dsi_base, struct hisi_panel_info *pinfo)
{
	if (pinfo->mipi.phy_mode == CPHY_MODE) {
		if (pinfo->mipi.mininum_phy_timing_flag == 1) {
			/* T3-PREBEGIN */
			mipi_config_phy_test_code(mipi_dsi_base, 0x00010001, MIN_T3_PREBEGIN_PHY_TIMING);
			/* T3-POST */
			mipi_config_phy_test_code(mipi_dsi_base, 0x00010002, MIN_T3_POST_PHY_TIMING);
		} else {
			/* T3-PREBEGIN */
			mipi_config_phy_test_code(mipi_dsi_base, 0x00010001, T3_PREBEGIN_PHY_TIMING);
			/* T3-POST */
			mipi_config_phy_test_code(mipi_dsi_base, 0x00010002, T3_POST_PHY_TIMING);
		}

		mipi_cdphy_PLL_configuration(mipi_dsi_base, pinfo, (pinfo->dsi_phy_ctrl.rg_cphy_div << 4));

		if (dpu_g_fpga_flag) {
			mipi_config_phy_test_code(mipi_dsi_base, 0x0001004F, 0xf0);
			mipi_config_phy_test_code(mipi_dsi_base, 0x00010052, 0xa8);
			mipi_config_phy_test_code(mipi_dsi_base, 0x00010053, 0xc2);
		}

		mipi_config_phy_test_code(mipi_dsi_base, 0x00010058,
			(0x4 + pinfo->mipi.lane_nums) << 4 | 0);
		mipi_config_phy_test_code(mipi_dsi_base, 0x0001005B, 0x19);
		/* PLL update control */
		mipi_config_phy_test_code(mipi_dsi_base, 0x0001004B, 0x1);

		/* set cphy spec parameter */
		mipi_config_cphy_spec1v0_parameter(mipi_dsi_base, pinfo, &pinfo->dsi_phy_ctrl);
	} else {
		mipi_cdphy_PLL_configuration(mipi_dsi_base, pinfo, 0);

		if (dpu_g_fpga_flag) {
			mipi_config_phy_test_code(mipi_dsi_base, 0x0001004F, 0xf0);
			mipi_config_phy_test_code(mipi_dsi_base, 0x00010050, 0xc0);
			mipi_config_phy_test_code(mipi_dsi_base, 0x00010051, 0x22);
			mipi_config_phy_test_code(mipi_dsi_base, 0x00010053,
				pinfo->dsi_phy_ctrl.rg_vrefsel_vcm);
		}
		/* PLL update control */
		mipi_config_phy_test_code(mipi_dsi_base, 0x0001004B, 0x1);

		/* set dphy spec parameter */
		mipi_config_dphy_spec1v2_parameter(mipi_dsi_base, pinfo, &pinfo->dsi_phy_ctrl);
	}
}

static uint32_t mipi_get_cmp_stopstate_value(struct hisi_panel_info *pinfo)
{
	uint32_t cmp_stopstate_val;

	if (pinfo->mipi.lane_nums >= DSI_4_LANES)
		cmp_stopstate_val = (BIT(4) | BIT(7) | BIT(9) | BIT(11));
	else if (pinfo->mipi.lane_nums >= DSI_3_LANES)
		cmp_stopstate_val = (BIT(4) | BIT(7) | BIT(9));
	else if (pinfo->mipi.lane_nums >= DSI_2_LANES)
		cmp_stopstate_val = (BIT(4) | BIT(7));
	else
		cmp_stopstate_val = (BIT(4));

	return cmp_stopstate_val;
}

static bool mipi_phy_status_check(const char __iomem *mipi_dsi_base, uint32_t expected_value)
{
	bool is_ready = false;
	uint32_t temp = 0;
	unsigned long dw_jiffies;

	dw_jiffies = jiffies + HZ / 2;  /* HZ / 2 = 0.5s */
	do {
		temp = inp32(DPU_DSI_CDPHY_STATUS_ADDR(mipi_dsi_base));
		if ((temp & expected_value) == expected_value) {
			is_ready = true;
			break;
		}
	} while (time_after(dw_jiffies, jiffies));

	disp_pr_debug("DPU_DSI_CDPHY_STATUS_ADDR=0x%x.\n", temp);

	return is_ready;
}

static void get_mipi_dsi_timing(struct hisi_connector_device *connector_dev)
{
	dss_rect_t rect;
	uint64_t pixel_clk;
	uint64_t lane_byte_clk;
	struct hisi_panel_info *pinfo = NULL;

	pinfo = connector_dev->fix_panel_info;
	disp_check_and_no_retval(!pinfo, err, "pinfo is NULL\n");

	rect.x = 0;
	rect.y = 0;
	rect.w = pinfo->xres;
	rect.h = pinfo->yres;
	// TODO:
	/* mipi_ifbc_get_rect(hisifd, &rect); */

	pinfo->mipi.width = rect.w;
	pinfo->mipi.vactive_line = pinfo->yres;

	if (pinfo->mipi.dsi_timing_support)
		return;

	pixel_clk = mipi_pixel_clk(connector_dev);
	if (pixel_clk == 0)
		return;

	lane_byte_clk = (pinfo->mipi.phy_mode == DPHY_MODE) ?
		pinfo->dsi_phy_ctrl.lane_byte_clk : pinfo->dsi_phy_ctrl.lane_word_clk;

	pinfo->mipi.hsa = round1(pinfo->ldi.h_pulse_width * lane_byte_clk, pixel_clk);
	pinfo->mipi.hbp = round1(pinfo->ldi.h_back_porch * lane_byte_clk, pixel_clk);
	pinfo->mipi.hline_time = round1((pinfo->ldi.h_pulse_width +
		pinfo->ldi.h_back_porch + pinfo->mipi.width +
		pinfo->ldi.h_front_porch) * lane_byte_clk,
		pixel_clk);
	pinfo->mipi.dpi_hsize = round1(pinfo->mipi.width * lane_byte_clk, pixel_clk);

	pinfo->mipi.vsa = pinfo->ldi.v_pulse_width;
	pinfo->mipi.vbp = pinfo->ldi.v_back_porch;
	pinfo->mipi.vfp = pinfo->ldi.v_front_porch;

	disp_pr_debug("lane_byte_clk_default %llu M, htiming: %d, %d, %d, %d\n",
		pinfo->dsi_phy_ctrl.lane_byte_clk_default, pinfo->mipi.hsa,
		pinfo->mipi.hbp, pinfo->mipi.hline_time, pinfo->mipi.dpi_hsize);
}

#if 0
uint32_t get_mipi_timing_hline_time(struct hisi_fb_data_type *hisifd, uint32_t object_hline_time)
{
	uint32_t tmp_hline_time = 0;
	uint64_t lane_byte_clk;

	struct hisi_panel_info *pinfo = &(hisifd->panel_info);

	lane_byte_clk = (pinfo->mipi.phy_mode == DPHY_MODE) ?
		pinfo->dsi_phy_ctrl.lane_byte_clk : pinfo->dsi_phy_ctrl.lane_word_clk;
	if (lane_byte_clk == pinfo->dsi_phy_ctrl.lane_byte_clk_default) {
		tmp_hline_time = object_hline_time;  /* pinfo->mipi.hline_time; */
	} else {
		if (pinfo->dsi_phy_ctrl.lane_byte_clk_default == 0) {
			pinfo->dsi_phy_ctrl.lane_byte_clk_default = get_default_lane_byte_clk(hisifd);
			disp_pr_err("change lane_byte_clk_default to %llu M\n",
				pinfo->dsi_phy_ctrl.lane_byte_clk_default);
		}

		tmp_hline_time = (uint32_t)round1(object_hline_time * lane_byte_clk,
			pinfo->dsi_phy_ctrl.lane_byte_clk_default);
	}

	disp_pr_info("hline_time = %d\n", tmp_hline_time);

	return tmp_hline_time;
}
#endif

static void get_mipi_dsi_timing_config_para(
	struct hisi_panel_info *pinfo, struct mipi_dsi_phy_ctrl *phy_ctrl,
	struct mipi_dsi_timing *timing)
{
	uint32_t tmp_hline_time = 0;
	uint64_t lane_byte_clk;

	disp_check_and_no_retval((!timing || !pinfo), err, "timing or pinfo is NULL\n");

	// TODO:
	/* if (pinfo->dfr_support) {
		if (pinfo->fps == FPS_60HZ)
			tmp_hline_time = pinfo->mipi.hline_time;
		else
			tmp_hline_time = hisifd->panel_info.mipi_updt.hline_time;
	} else {
		tmp_hline_time = pinfo->mipi.hline_time;
	} */
	tmp_hline_time = pinfo->mipi.hline_time;

	lane_byte_clk = (pinfo->mipi.phy_mode == DPHY_MODE) ? phy_ctrl->lane_byte_clk : phy_ctrl->lane_word_clk;

	if (lane_byte_clk == pinfo->dsi_phy_ctrl.lane_byte_clk_default) {
		timing->hsa = pinfo->mipi.hsa;
		timing->hbp = pinfo->mipi.hbp;
		timing->hline_time = tmp_hline_time;
	} else {
		if (pinfo->dsi_phy_ctrl.lane_byte_clk_default == 0) {
			pinfo->dsi_phy_ctrl.lane_byte_clk_default = get_default_lane_byte_clk(pinfo);
			disp_pr_err("change lane_byte_clk_default to %llu M\n",
				pinfo->dsi_phy_ctrl.lane_byte_clk_default);
		}

		timing->hsa = (uint32_t)round1(pinfo->mipi.hsa * lane_byte_clk,
			pinfo->dsi_phy_ctrl.lane_byte_clk_default);
		timing->hbp = (uint32_t)round1(pinfo->mipi.hbp * lane_byte_clk,
			pinfo->dsi_phy_ctrl.lane_byte_clk_default);
		timing->hline_time = (uint32_t)round1(tmp_hline_time * lane_byte_clk,
			pinfo->dsi_phy_ctrl.lane_byte_clk_default);
	}

	timing->dpi_hsize = pinfo->mipi.dpi_hsize;
	timing->width = pinfo->mipi.width;
	timing->vsa = pinfo->mipi.vsa;
	timing->vbp = pinfo->mipi.vbp;
	timing->vfp = pinfo->mipi.vfp;
	timing->vactive_line = pinfo->mipi.vactive_line;

#if 0
	if (pinfo->dfr_support)
		pinfo->frm_rate_ctrl.current_hline_time = timing->hline_time;
#endif

	disp_pr_debug("lanebyteclk: %llu M, %llu M, htiming: %d, %d, %d, %d "
		"new: %d, %d, %d, %d\n",
		lane_byte_clk, pinfo->dsi_phy_ctrl.lane_byte_clk_default,
		pinfo->mipi.hsa, pinfo->mipi.hbp, tmp_hline_time,
		pinfo->mipi.dpi_hsize, timing->hsa, timing->hbp, timing->hline_time,
		timing->dpi_hsize);
}

static void disable_mipi_ldi(struct hisi_connector_device *connector_dev, char __iomem *mipi_dsi_base)
{
	char __iomem *mipi_ldi_base = NULL;
	char __iomem *dss_base_addr = NULL;

	dss_base_addr = connector_dev->base_addr.dpu_base_addr;
	disp_check_and_no_retval(!dss_base_addr, err, "dss_base_addr is NULL\n");

	if (is_dual_mipi_panel(connector_dev->fix_panel_info->type)) {
		mipi_ldi_base = mipi_get_dsi_base(dss_base_addr, DSI0_INDEX);
		dpu_set_reg(DPU_DSI_LDI_CTRL_ADDR(mipi_ldi_base), 0x0, 1, 5);

		return;
	}

	mipi_ldi_base = mipi_dsi_base;
	dpu_set_reg(DPU_DSI_LDI_CTRL_ADDR(mipi_ldi_base), 0x0, 1, 0);

}

static void mipi_ldi_init(struct hisi_connector_device *connector_dev, char __iomem *mipi_dsi_base)
{
	struct hisi_panel_info *pinfo = NULL;
	uint64_t lane_byte_clk;
	uint32_t dsi_idx;
	struct mipi_dsi_timing timing;

	pinfo = connector_dev->fix_panel_info;
	disp_check_and_no_retval(!pinfo, err, "pinfo is NULL\n");
	dsi_idx = mipi_get_dsi_index(connector_dev->base_addr.dpu_base_addr, mipi_dsi_base);

	lane_byte_clk = (pinfo->mipi.phy_mode == CPHY_MODE) ?
		pinfo->dsi_phy_ctrl.lane_word_clk : pinfo->dsi_phy_ctrl.lane_byte_clk;

	memset(&timing, 0, sizeof(timing));
	get_mipi_dsi_timing_config_para(pinfo, &(pinfo->dsi_phy_ctrl), &timing);

	dpu_set_reg(DPU_DSI_LDI_DPI0_HRZ_CTRL3_ADDR(mipi_dsi_base), disp_reduce(timing.dpi_hsize), 12, 0);
	dpu_set_reg(DPU_DSI_LDI_DPI0_HRZ_CTRL2_ADDR(mipi_dsi_base), disp_reduce(timing.width), 12, 0);
	dpu_set_reg(DPU_DSI_LDI_VRT_CTRL2_ADDR(mipi_dsi_base), disp_reduce(timing.vactive_line), 12, 0);

	disable_mipi_ldi(connector_dev, mipi_dsi_base);
	if (is_mipi_video_panel(pinfo->type)) {
		dpu_set_reg(DPU_DSI_LDI_FRM_MSK_ADDR(mipi_dsi_base), 0x0, 1, 0);
		dpu_set_reg(DPU_DSI_CMD_MOD_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 1);
	}

	if (is_dual_mipi_panel(pinfo->type))
		dpu_set_reg(DPU_DSI_LDI_CTRL_ADDR(mipi_dsi_base), ((dsi_idx == DSI0_INDEX) ? 0 : 1), 1, 13);

	if (is_mipi_cmd_panel(pinfo->type)) {
		if (dsi_idx == DSI0_INDEX)
			dpu_set_reg(DPU_DSI_TE_CTRL_ADDR(mipi_dsi_base), (0x1 << 17) | (0x1 << 6) | 0x1, 18, 0);

		dpu_set_reg(DPU_DSI_TE_HS_NUM_ADDR(mipi_dsi_base), 0x0, 32, 0);
		dpu_set_reg(DPU_DSI_TE_HS_WD_ADDR(mipi_dsi_base), 0x24024, 32, 0);

		if (pinfo->mipi.dsi_te_type == DSI1_TE1_TYPE)
			dpu_set_reg(DPU_DSI_TE_VS_WD_ADDR(mipi_dsi_base), ((2 * lane_byte_clk / MILLION_CONVERT) << 12) |
				0x3FC, 32, 0);
		else
			dpu_set_reg(DPU_DSI_TE_VS_WD_ADDR(mipi_dsi_base), (0x3FC << 12) | (2 * lane_byte_clk / MILLION_CONVERT), 32, 0);

		dpu_set_reg(DPU_DSI_SHADOW_REG_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 0);

		/* enable vsync delay when dirty region update */
		dpu_set_reg(DPU_DSI_VSYNC_DELAY_CTRL_ADDR(mipi_dsi_base), 0x2, 2, 0);
		dpu_set_reg(DPU_DSI_VSYNC_DELAY_TIME_ADDR(mipi_dsi_base), 0x0, 32, 0);

		// TODO: enable single frame update
		/* dpu_set_reg(mipi_dsi_base + MIPI_LDI_FRM_MSK, (hisifd->frame_update_flag == 1) ? 0x0 : 0x1, 1, 0); */
	} else if (is_mipi_video_panel(pinfo->type) && pinfo->dfr_support) {
		/* enable vsync delay */
		dpu_set_reg(DPU_DSI_VSYNC_DELAY_CTRL_ADDR(mipi_dsi_base), 0x2, 2, 0);

		/*
		 * If vsync delay is enable, the vsync_delay_count can not be set to 0.
		 * It is recommended to set to a small value to avoid the abnormality of vsync_delay function
		 */
		dpu_set_reg(DPU_DSI_VSYNC_DELAY_TIME_ADDR(mipi_dsi_base), 0x5, 32, 0);
	}
}

static void mipi_dsi_phy_config(char __iomem *mipi_dsi_base,
	struct hisi_panel_info *pinfo)
{
	bool is_ready = false;

	/*************************Configure the PHY start*************************/

	dpu_set_reg(DPU_DSI_CDPHY_LANE_NUM_ADDR(mipi_dsi_base),
		pinfo->mipi.lane_nums, 2, 0);
	dpu_set_reg(DPU_DSI_CLK_DIV_CTRL_ADDR(mipi_dsi_base),
		pinfo->dsi_phy_ctrl.clk_division, 8, 0);
	dpu_set_reg(DPU_DSI_CLK_DIV_CTRL_ADDR(mipi_dsi_base),
		pinfo->dsi_phy_ctrl.clk_division, 8, 8);

	outp32(DPU_DSI_CDPHY_RST_CTRL_ADDR(mipi_dsi_base), 0x00000000);

	outp32(DPU_DSI_CDPHY_TEST_CTRL_0_ADDR(mipi_dsi_base), 0x00000000);
	outp32(DPU_DSI_CDPHY_TEST_CTRL_0_ADDR(mipi_dsi_base), 0x00000001);
	outp32(DPU_DSI_CDPHY_TEST_CTRL_0_ADDR(mipi_dsi_base), 0x00000000);

	mipi_cdphy_init_config(mipi_dsi_base, pinfo);

	outp32(DPU_DSI_CDPHY_RST_CTRL_ADDR(mipi_dsi_base), 0x0000000F);

	is_ready = mipi_phy_status_check(mipi_dsi_base, 0x01);
	if (!is_ready)
		disp_pr_info("phylock is not ready!\n");

	is_ready = mipi_phy_status_check(mipi_dsi_base,
		mipi_get_cmp_stopstate_value(pinfo));
	if (!is_ready)
		disp_pr_info("phystopstateclklane is not ready!\n");

	/*************************Configure the PHY end*************************/
}

static void mipi_dsi_config_dpi_interface(struct hisi_panel_info *pinfo, char __iomem *mipi_dsi_base)
{
	DPU_DSI_VIDEO_POL_CTRL_UNION pol_ctrl = {
		.reg = {
			.dataen_active_low = pinfo->ldi.data_en_plr,
			.vsync_active_low = pinfo->ldi.vsync_plr,
			.hsync_active_low = pinfo->ldi.hsync_plr,
			.shutd_active_low = 0,
			.colorm_active_low = 0,
			.reserved = 0,
		},
	};

	dpu_set_reg(DPU_DSI_VIDEO_VCID_ADDR(mipi_dsi_base), pinfo->mipi.vc, 2, 0);
	dpu_set_reg(DPU_DSI_VIDEO_COLOR_FORMAT_ADDR(mipi_dsi_base), pinfo->mipi.color_mode, 4, 0);

	outp32(DPU_DSI_VIDEO_POL_CTRL_ADDR(mipi_dsi_base), pol_ctrl.value);

}

static void mipi_dsi_video_mode_config(char __iomem *mipi_dsi_base,
	struct hisi_panel_info *pinfo, struct mipi_dsi_timing *timing)
{
	/* video mode: low power mode */
	if (pinfo->mipi.lp11_flag == MIPI_DISABLE_LP11)
		dpu_set_reg(DPU_DSI_VIDEO_MODE_CTRL_ADDR(mipi_dsi_base), 0x0f, 6, 8);
	else
		dpu_set_reg(DPU_DSI_VIDEO_MODE_CTRL_ADDR(mipi_dsi_base), 0x3f, 6, 8);

	if (is_mipi_video_panel(pinfo->type)) {
		dpu_set_reg(DPU_DSI_VIDEO_MODE_LP_CMD_TIMING_ADDR(mipi_dsi_base), 0x4, 8, 16);
		/* video mode: send read cmd by lp mode */
		dpu_set_reg(DPU_DSI_VIDEO_MODE_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 15);
	}

	if ((pinfo->mipi.dsi_version == DSI_1_2_VERSION) &&
		(is_mipi_video_panel(pinfo->type)) &&
		((pinfo->ifbc_type == IFBC_TYPE_VESA3X_SINGLE)
			|| (pinfo->ifbc_type == IFBC_TYPE_VESA3X_DUAL))) {
		dpu_set_reg(DPU_DSI_VIDEO_PKT_LEN_ADDR(mipi_dsi_base),
			timing->width * pinfo->timing_info.pxl_clk_rate_div, 14, 0);
		/* video vase3x must be set BURST mode */
		if (pinfo->mipi.burst_mode < DSI_BURST_SYNC_PULSES_1) {
			disp_pr_info("pinfo->mipi.burst_mode = %d, video need config "
				"BURST mode\n", pinfo->mipi.burst_mode);
			pinfo->mipi.burst_mode = DSI_BURST_SYNC_PULSES_1;
		}
	} else {
		dpu_set_reg(DPU_DSI_VIDEO_PKT_LEN_ADDR(mipi_dsi_base), timing->width, 14, 0);
	}

	/* burst mode */
	dpu_set_reg(DPU_DSI_VIDEO_MODE_CTRL_ADDR(mipi_dsi_base), pinfo->mipi.burst_mode, 2, 0);
}

static void mipi_dsi_horizontal_timing_config(struct mipi_dsi_timing *timing, char __iomem *mipi_dsi_base)
{
	if (timing->hline_time < (timing->hsa + timing->hbp + timing->dpi_hsize))
		disp_pr_err("wrong hfp\n");

	dpu_set_reg(DPU_DSI_VIDEO_HSA_NUM_ADDR(mipi_dsi_base), timing->hsa, 12, 0);
	dpu_set_reg(DPU_DSI_VIDEO_HBP_NUM_ADDR(mipi_dsi_base), timing->hbp, 12, 0);
	dpu_set_reg(DPU_DSI_VIDEO_HLINE_NUM_ADDR(mipi_dsi_base), timing->hline_time, 15, 0);

	dpu_set_reg(DPU_DSI_VIDEO_VSA_NUM_ADDR(mipi_dsi_base), timing->vsa, 10, 0);
	dpu_set_reg(DPU_DSI_VIDEO_VBP_NUM_ADDR(mipi_dsi_base), timing->vbp, 10, 0);
}

static void mipi_dsi_vfp_vsync_config(struct mipi_dsi_timing *timing,  char __iomem *mipi_dsi_base)
{
	uint32_t ldi_vrt_ctrl0 = 0;
	uint32_t vsync_delay_cnt = 0;

	if (timing->vfp > V_FRONT_PORCH_MAX) {
		ldi_vrt_ctrl0 = V_FRONT_PORCH_MAX;
		vsync_delay_cnt = (timing->vfp - V_FRONT_PORCH_MAX) * timing->hline_time;
		disp_pr_warn("vfp %d > 1023", timing->vfp);
	} else {
		ldi_vrt_ctrl0 = timing->vfp;
		vsync_delay_cnt = 0;
	}

	dpu_set_reg(DPU_DSI_VIDEO_VFP_NUM_ADDR(mipi_dsi_base), ldi_vrt_ctrl0, 10, 0);
	dpu_set_reg(DPU_DSI_VSYNC_DELAY_TIME_ADDR(mipi_dsi_base), vsync_delay_cnt, 32, 0);
	dpu_set_reg(DPU_DSI_VIDEO_VACT_NUM_ADDR(mipi_dsi_base), timing->vactive_line, 14, 0);
	dpu_set_reg(DPU_DSI_TO_TIME_CTRL_ADDR(mipi_dsi_base), 0x7FF, 16, 0);
}

static void mipi_dsi_phy_timing_config(struct hisi_panel_info *pinfo, char __iomem *mipi_dsi_base)
{
	/* Configure core's phy parameters */
	dpu_set_reg(DPU_DSI_CLKLANE_TRANS_TIME_ADDR(mipi_dsi_base), pinfo->dsi_phy_ctrl.clk_lane_lp2hs_time, 10, 0);
	dpu_set_reg(DPU_DSI_CLKLANE_TRANS_TIME_ADDR(mipi_dsi_base), pinfo->dsi_phy_ctrl.clk_lane_hs2lp_time, 10, 16);

	dpu_set_reg(DPU_DSI_CDPHY_MAX_RD_TIME_ADDR(mipi_dsi_base), 0x7FFF, 15, 0);
	dpu_set_reg(DPU_DSI_DATALANE_TRNAS_TIME_ADDR(mipi_dsi_base), pinfo->dsi_phy_ctrl.data_lane_lp2hs_time, 10, 0);
	dpu_set_reg(DPU_DSI_DATALANE_TRNAS_TIME_ADDR(mipi_dsi_base), pinfo->dsi_phy_ctrl.data_lane_hs2lp_time, 10, 16);
}

static void mipi_dsi_auto_ulps_config(struct mipi_dsi_timing *timing, struct hisi_panel_info *pinfo,
	char __iomem *mipi_dsi_base)
{
	uint32_t twakeup_cnt = 0;
	uint32_t auto_ulps_enter_delay;
	uint32_t twakeup_clk_div;

	auto_ulps_enter_delay = timing->hline_time * 3 / 2;  /* chip protocol */
	twakeup_clk_div = 8;
	/* twakeup_cnt*twakeup_clk_div*t_lanebyteclk>1ms */
	if (pinfo->mipi.phy_mode == CPHY_MODE)
		twakeup_cnt = pinfo->dsi_phy_ctrl.lane_word_clk;
	else
		twakeup_cnt = pinfo->dsi_phy_ctrl.lane_byte_clk;

	/* chip protocol */
	twakeup_cnt = twakeup_cnt / 1000 * 3 / 2 / twakeup_clk_div;

	dpu_set_reg(DPU_DSI_AUTO_ULPS_ENTRY_DELAY_ADDR(mipi_dsi_base), auto_ulps_enter_delay, 32, 0);
	dpu_set_reg(DPU_DSI_AUTO_ULPS_WAKEUP_TIME_ADDR(mipi_dsi_base), twakeup_clk_div, 16, 0);
	dpu_set_reg(DPU_DSI_AUTO_ULPS_WAKEUP_TIME_ADDR(mipi_dsi_base), twakeup_cnt, 16, 16);
}

static void mipi_init(struct hisi_connector_device *connector_dev, char __iomem *mipi_dsi_base)
{
	struct hisi_panel_info *pinfo = NULL;
	struct mipi_dsi_timing timing;

	disp_check_and_no_retval(!connector_dev, err, "connector_dev is NULL\n");

	pinfo = connector_dev->fix_panel_info;
	disp_check_and_no_retval(!pinfo, err, "pinfo is NULL\n");

	if (pinfo->mipi.max_tx_esc_clk == 0) {
		disp_pr_err("max_tx_esc_clk is invalid!\n");
		pinfo->mipi.max_tx_esc_clk = DEFAULT_MAX_TX_ESC_CLK;
	}

	memset(&(pinfo->dsi_phy_ctrl), 0, sizeof(struct mipi_dsi_phy_ctrl));

	if (pinfo->mipi.phy_mode == CPHY_MODE)
		get_dsi_cphy_ctrl(pinfo);
	else
		get_dsi_dphy_ctrl(pinfo);

	get_mipi_dsi_timing(connector_dev);
	memset(&timing, 0, sizeof(timing));
	get_mipi_dsi_timing_config_para(pinfo, &(pinfo->dsi_phy_ctrl), &timing);

	mipi_dsi_phy_config(mipi_dsi_base, pinfo);

	dpu_set_reg(DPU_DSI_MODE_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 1);
	if (is_mipi_cmd_panel(pinfo->type)) {
		dpu_set_reg(DPU_DSI_MODE_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 0);
		dpu_set_reg(DPU_DSI_EDPI_CMD_SIZE_ADDR(mipi_dsi_base),
			timing.width, 16, 0);

		/* cnt=2 in update-patial scene, cnt nees to be checked for different panels */
		if (pinfo->mipi.hs_wr_to_time == 0)
			dpu_set_reg(DPU_DSI_HS_WR_TO_TIME_CTRL_ADDR(mipi_dsi_base), 0x1000002, 25, 0);
		else
			dpu_set_reg(DPU_DSI_HS_WR_TO_TIME_CTRL_ADDR(mipi_dsi_base),
				(0x1 << 24) | (pinfo->mipi.hs_wr_to_time *
				pinfo->dsi_phy_ctrl.lane_byte_clk / 1000000000UL), 25, 0);
	}

	/* phy_stop_wait_time */
	dpu_set_reg(DPU_DSI_CDPHY_LANE_NUM_ADDR(mipi_dsi_base), pinfo->dsi_phy_ctrl.phy_stop_wait_time, 8, 8);

	/* --------------configuring the DPI packet transmission---------------- */
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
	mipi_dsi_video_mode_config(mipi_dsi_base, pinfo, &timing);

	/* for dsi read, BTA enable */
	dpu_set_reg(DPU_DSI_PERIP_CHAR_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 2);

	/*
	 * 4. Define the DPI Horizontal timing configuration:
	 *
	 * Hsa_time = HSA*(PCLK period/Clk Lane Byte Period);
	 * Hbp_time = HBP*(PCLK period/Clk Lane Byte Period);
	 * Hline_time = (HSA+HBP+HACT+HFP)*(PCLK period/Clk Lane Byte Period);
	 */
	mipi_dsi_horizontal_timing_config(&timing, mipi_dsi_base);

	mipi_dsi_vfp_vsync_config(&timing, mipi_dsi_base);

	mipi_dsi_phy_timing_config(pinfo, mipi_dsi_base);

	/* 16~19bit:pclk_en, pclk_sel, dpipclk_en, dpipclk_sel */
	dpu_set_reg(DPU_DSI_CLK_DIV_CTRL_ADDR(mipi_dsi_base), 0x5, 4, 16);

	if (is_mipi_cmd_panel(pinfo->type))
		mipi_dsi_auto_ulps_config(&timing, pinfo, mipi_dsi_base);

	if (pinfo->mipi.phy_mode == CPHY_MODE)
		dpu_set_reg(DPU_DSI_CPHY_OR_DPHY_ADDR(mipi_dsi_base), 0x1, 1, 0);
	else
		dpu_set_reg(DPU_DSI_CPHY_OR_DPHY_ADDR(mipi_dsi_base), 0x0, 1, 0);

	mipi_ldi_init(connector_dev, mipi_dsi_base);

	/* Waking up Core */
	dpu_set_reg(DPU_DSI_POR_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 0);
}

static void mipi_dsi_on_sub1(struct hisi_connector_device *connector_dev, char __iomem *mipi_dsi_base)
{
	/* mipi init */
	mipi_init(connector_dev, mipi_dsi_base);

	/* switch to cmd mode */
	dpu_set_reg(DPU_DSI_MODE_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 0);
	/* cmd mode: low power mode */
	dpu_set_reg(DPU_DSI_CMD_MODE_CTRL_ADDR(mipi_dsi_base), 0x7f, 7, 8);
	dpu_set_reg(DPU_DSI_CMD_MODE_CTRL_ADDR(mipi_dsi_base), 0xf, 4, 16);
	dpu_set_reg(DPU_DSI_CMD_MODE_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 24);
	/* disable generate High Speed clock */
	dpu_set_reg(DPU_DSI_LP_CLK_CTRL_ADDR(mipi_dsi_base), 0x0, 1, 0);
}

#if 0
static void pctrl_dphytx_stopcnt_config(
	struct hisi_panel_info *pinfo, char __iomem *mipi_dsi_base)
{
	uint64_t lane_byte_clk = 0;
	uint64_t pctrl_dphytx_stopcnt = 0;
	char __iomem *mipi_dsi_base = NULL;
	uint32_t stopcnt_div;
	struct mipi_dsi_timing timing;

	memset(&timing, 0, sizeof(timing));
	get_mipi_dsi_timing_config_para(pinfo, &(pinfo->dsi_phy_ctrl), &timing);

	stopcnt_div = is_dual_mipi_panel(pinfo) ? 2 : 1;
	/* init: wait DPHY 4 data lane stopstate */
	hisi_check_and_no_retval((pinfo->pxl_clk_rate == 0), err, "pxl_clk_rate is zero\n");
	if (is_mipi_video_panel(pinfo)) {
		pctrl_dphytx_stopcnt = (uint64_t)(pinfo->ldi.h_back_porch +
			pinfo->ldi.h_front_porch + pinfo->ldi.h_pulse_width +
			pinfo->xres / stopcnt_div + 5) *
			hisifd->dss_vote_cmd.dss_pclk_dss_rate /
			(pinfo->pxl_clk_rate / stopcnt_div);
	} else {
		pctrl_dphytx_stopcnt = (uint64_t)(pinfo->ldi.h_back_porch +
			pinfo->ldi.h_front_porch + pinfo->ldi.h_pulse_width + 5) *
			hisifd->dss_vote_cmd.dss_pclk_dss_rate /
			(pinfo->pxl_clk_rate / stopcnt_div);
	}

	if (pinfo->mipi.dsi_timing_support) {
		lane_byte_clk = (pinfo->mipi.phy_mode == DPHY_MODE) ?
			pinfo->dsi_phy_ctrl.lane_byte_clk :
			pinfo->dsi_phy_ctrl.lane_word_clk;
		pctrl_dphytx_stopcnt = round1(((uint64_t)timing.hline_time *
			hisifd->dss_vote_cmd.dss_pclk_dss_rate), lane_byte_clk);
		disp_pr_info("pctrl_dphytx_stopcnt = %llu, pclk = %lu\n",
			pctrl_dphytx_stopcnt, hisifd->dss_vote_cmd.dss_pclk_dss_rate);
	}

	mipi_dsi_base = get_mipi_dsi_base(hisifd);
	outp32(mipi_dsi_base + MIPIDSI_DPHYTX_STOPSNT_OFFSET, (uint32_t)pctrl_dphytx_stopcnt);
	if (is_dual_mipi_panel(hisifd))
		outp32(hisifd->mipi_dsi1_base + MIPIDSI_DPHYTX_STOPSNT_OFFSET,
			(uint32_t)pctrl_dphytx_stopcnt);
}
#endif

static void mipi_dsi_on_sub2(struct hisi_connector_device *connector_dev, char __iomem *mipi_dsi_base)
{
	struct hisi_panel_info *pinfo = NULL;

	disp_check_and_no_retval(!connector_dev, err, "connector_dev is NULL\n");
	disp_check_and_no_retval(!mipi_dsi_base, err, "mipi_dsi_base is NULL\n");

	pinfo = connector_dev->fix_panel_info;
	disp_check_and_no_retval(!pinfo, err, "pinfo is NULL\n");

	if (is_mipi_video_panel(pinfo->type))
		/* switch to video mode */
		dpu_set_reg(DPU_DSI_MODE_CTRL_ADDR(mipi_dsi_base), 0x0, 1, 0);

	if (is_mipi_cmd_panel(pinfo->type)) {
		/* cmd mode: high speed mode */
		dpu_set_reg(DPU_DSI_CMD_MODE_CTRL_ADDR(mipi_dsi_base), 0x0, 7, 8);
		dpu_set_reg(DPU_DSI_CMD_MODE_CTRL_ADDR(mipi_dsi_base), 0x0, 4, 16);
		dpu_set_reg(DPU_DSI_CMD_MODE_CTRL_ADDR(mipi_dsi_base), 0x0, 1, 24);
	}

	/* enable EOTP TX */
	if (pinfo->mipi.phy_mode == DPHY_MODE) {
		/* Some vendors don't need eotp check */
		if (pinfo->mipi.eotp_disable_flag == 1)
			dpu_set_reg(DPU_DSI_PERIP_CHAR_CTRL_ADDR(mipi_dsi_base), 0x0, 1, 0);
		else
			dpu_set_reg(DPU_DSI_PERIP_CHAR_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 0);
	}

	/* enable generate High Speed clock, non continue */
	if (pinfo->mipi.non_continue_en)
		dpu_set_reg(DPU_DSI_LP_CLK_CTRL_ADDR(mipi_dsi_base), 0x3, 2, 0);
	else
		dpu_set_reg(DPU_DSI_LP_CLK_CTRL_ADDR(mipi_dsi_base), 0x1, 2, 0);

// TODO:
#if 0
	if ((pinfo->mipi.dsi_version == DSI_1_2_VERSION)
		&& is_ifbc_vesa_panel(hisifd))
		dpu_set_reg(DPU_DSI_DSC_PARA_SET_ADDR(mipi_dsi_base), 0x01, 32, 0);
	pctrl_dphytx_stopcnt_config(hisifd);
#endif
}


static void mipi_check_itf_status(struct hisi_connector_device *connector_dev)
{
#if 0
	int delay_count = 0;
	bool is_timeout = true;
	char __iomem *mctl_sys_base = NULL;
	int max_delay_count = 100;  /* 100ms */
	uint32_t temp;
// TODO:
#if 0
	if (hisifd->index != PRIMARY_PANEL_IDX)
		return;
#endif

	mctl_sys_base =  connector_dev->base_addr.dpu_base_addr + DSS_MCTRL_SYS_OFFSET;

	while (1) {
		temp = inp32(mctl_sys_base + MCTL_MOD17_STATUS);
		if (((temp & 0x10) == 0x10) || (delay_count > max_delay_count)) {
			is_timeout = (delay_count > max_delay_count) ? true : false;
			break;
		}
		mdelay(1);  /* 1ms */
		++delay_count;
	}

	if (is_timeout)
		disp_pr_info("mctl_itf not in idle status, ints=0x%x\n", temp);
#endif
}

void mipi_dsi_off_sub2(struct hisi_connector_device *connector_dev, struct hisi_connector_info *connector_info)
{
	char __iomem *mipi_dsi_base = NULL;

	disp_check_and_no_retval(!connector_dev, err, "connector_dev is NULL\n");
	disp_check_and_no_retval(!connector_info, err, "connector_info is NULL\n");

	mipi_dsi_base = connector_info->connector_base;
	disp_check_and_no_retval(!mipi_dsi_base, err, "mipi_dsi_base is NULL\n");

	/* switch to cmd mode */
	dpu_set_reg(DPU_DSI_MODE_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 0);
	/* cmd mode: low power mode */
	dpu_set_reg(DPU_DSI_CMD_MODE_CTRL_ADDR(mipi_dsi_base), 0x7f, 7, 8);
	dpu_set_reg(DPU_DSI_CMD_MODE_CTRL_ADDR(mipi_dsi_base), 0xf, 4, 16);
	dpu_set_reg(DPU_DSI_CMD_MODE_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 24);

	/* disable generate High Speed clock */
	dpu_set_reg(DPU_DSI_LP_CLK_CTRL_ADDR(mipi_dsi_base), 0x0, 1, 0);
	udelay(10);  /* 10us */

	/* shutdown d_phy */
	dpu_set_reg(DPU_DSI_CDPHY_RST_CTRL_ADDR(mipi_dsi_base), 0x0, 3, 0);
}

static void mipi_dsi_reset(struct hisi_connector_device *connector_dev)
{
	char __iomem *dss_base_addr = NULL;
	char __iomem *mipi_dsi_base = NULL;

	disp_check_and_no_retval(!connector_dev, err, "connector_dev is NULL\n");

	dss_base_addr = connector_dev->base_addr.dpu_base_addr;
	disp_check_and_no_retval(!dss_base_addr, err, "dss_base_addr is NULL\n");
	disp_check_and_no_retval(!connector_dev->master.connector_info, err, "connector_info is NULL\n");

	mipi_dsi_base = mipi_get_dsi_base(dss_base_addr, connector_dev->master.connector_id);

	dpu_set_reg(DPU_DSI_POR_CTRL_ADDR(mipi_dsi_base), 0x0, 1, 0);
	msleep(2); /* 2ms */
	dpu_set_reg(DPU_DSI_POR_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 0);
}

/*******************************************************************************
 * MIPI DPHY GPIO for FPGA
 */
#define GPIO_MIPI_DPHY_PG_SEL_A_NAME "pg_sel_a"
#define GPIO_MIPI_DPHY_PG_SEL_B_NAME "pg_sel_b"
#define GPIO_MIPI_DPHY_TX_RX_A_NAME "tx_rx_a"
#define GPIO_MIPI_DPHY_TX_RX_B_NAME "tx_rx_b"

// GPIO
#ifdef CONFIG_DKMD_DPU_V720
// gpio_6[0], 6x32+0=192
// gpio_6[2], 6x32+2=194
// gpio_3[5], 3x32+5=101
// gpio_3[7], 3x32+7=103
#define GPIO_PG_SEL_A (192)    /* gpout_32_1[24] */
#define GPIO_TX_RX_A (194)     /* gpout_32_1[26] */
#define GPIO_PG_SEL_B (101)    /* gpout_32_1[5] */
#define GPIO_TX_RX_B (103)     /* gpout_32_1[7] */
#else
#define GPIO_PG_SEL_A (56)    /* gpout_32_1[24] */
#define GPIO_TX_RX_A (58)     /* gpout_32_1[26] */
#define GPIO_PG_SEL_B (37)    /* gpout_32_1[5] */
#define GPIO_TX_RX_B (39)     /* gpout_32_1[7] */
#endif

static uint32_t gpio_pg_sel_a = GPIO_PG_SEL_A;
static uint32_t gpio_tx_rx_a = GPIO_TX_RX_A;
static uint32_t gpio_pg_sel_b = GPIO_PG_SEL_B;
static uint32_t gpio_tx_rx_b = GPIO_TX_RX_B;

static struct gpio_desc mipi_dphy_gpio_request_cmds[] = {
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_PG_SEL_A_NAME, &gpio_pg_sel_a, 0 },
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_PG_SEL_B_NAME, &gpio_pg_sel_b, 0 },
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_TX_RX_A_NAME, &gpio_tx_rx_a, 0 },
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_TX_RX_B_NAME, &gpio_tx_rx_b, 0 },
};

static struct gpio_desc mipi_dphy_gpio_free_cmds[] = {
	{ DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_PG_SEL_A_NAME, &gpio_pg_sel_a, 0 },
	{ DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_PG_SEL_B_NAME, &gpio_pg_sel_b, 0 },
	{ DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_TX_RX_A_NAME, &gpio_tx_rx_a, 0 },
	{ DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_TX_RX_B_NAME, &gpio_tx_rx_b, 0 },

};

static struct gpio_desc mipi_dphy_gpio_normal_cmds[] = {
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_PG_SEL_A_NAME, &gpio_pg_sel_a, 0 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_TX_RX_A_NAME, &gpio_tx_rx_a, 1 },

	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_PG_SEL_B_NAME, &gpio_pg_sel_b, 0 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_TX_RX_B_NAME, &gpio_tx_rx_b, 1 },
};

static struct gpio_desc mipi_dphy_gpio_lowpower_cmds[] = {
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_PG_SEL_A_NAME, &gpio_pg_sel_a, 0 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_TX_RX_A_NAME, &gpio_tx_rx_a, 0 },

	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_PG_SEL_B_NAME, &gpio_pg_sel_b, 0 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_TX_RX_B_NAME, &gpio_tx_rx_b, 0 },
};

static int mipi_dsi_dphy_fastboot_fpga(void)
{
	if (dpu_g_fpga_flag == 1)
		/* mpi dphy gpio request */
		dpu_gpio_cmds_tx(mipi_dphy_gpio_request_cmds, ARRAY_SIZE(mipi_dphy_gpio_request_cmds));

	return 0;
}

void dpu_mipi_dsi_dphy_on_fpga(struct hisi_connector_device *connector_dev)
{
	if (dpu_g_fpga_flag == 1) {
		/* mipi dphy gpio request */
		dpu_gpio_cmds_tx(mipi_dphy_gpio_request_cmds, ARRAY_SIZE(mipi_dphy_gpio_request_cmds));

		/* mipi dphy gpio normal */
		dpu_gpio_cmds_tx(mipi_dphy_gpio_normal_cmds, ARRAY_SIZE(mipi_dphy_gpio_normal_cmds));
	}
}

void dpu_mipi_dsi_dphy_off_fpga(struct hisi_connector_device *connector_dev)
{
	if (dpu_g_fpga_flag == 1) {
		/* mipi dphy gpio lowpower */
		dpu_gpio_cmds_tx(mipi_dphy_gpio_lowpower_cmds, ARRAY_SIZE(mipi_dphy_gpio_lowpower_cmds));
		/* mipi dphy gpio free */
		dpu_gpio_cmds_tx(mipi_dphy_gpio_free_cmds, ARRAY_SIZE(mipi_dphy_gpio_free_cmds));
	}
}

/*
int mipi_dsi_set_fastboot(struct hisi_connector_device *connector_dev)
{
	struct hisi_connector_info *connector_info = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	disp_check_and_return(!connector_dev, -EINVAL, err, "connector_dev is NULL\n");

	connector_info = connector_dev->connector_info;
	pinfo = connector_dev->fix_panel_info;
	disp_check_and_return(!connector_info, -EINVAL, err, "connector_info is NULL\n");
	disp_check_and_return(!pinfo, -EINVAL, err, "pinfo is NULL\n");
	disp_check_and_return(!connector_dev->base_addr.dss_base_addr, -EINVAL, err, "connector_dev->base_addr.dss_base_addr is NULL\n");

	disp_pr_debug("fb%d, +\n", connector_info->connector_id);

	mipi_dsi_dphy_fastboot_fpga();

	mipi_dsi_clk_enable(hisifd);

	// bugfix for access dsi reg noc before apb clock enable
	if (connector_info->connector_id == PRIMARY_PANEL_IDX) {
		// disp core dsi0 clk
		dpu_set_reg(connector_dev->base_addr.dss_base_addr + DSS_DISP_GLB_OFFSET + MODULE_CORE_CLK_SEL, 0x1, 1, 16);
		dpu_set_reg(connector_dev->base_addr.dss_base_addr + GLB_MODULE_CLK_SEL, 0x1, 1, 23);
		dpu_set_reg(connector_dev->base_addr.dss_base_addr + GLB_MODULE_CLK_EN, 0x1, 1, 23);
	} else if (connector_info->connector_id == EXTERNAL_PANEL_IDX) {
		// disp core dsi1 clk
		dpu_set_reg(connector_dev->base_addr.dss_base_addr + DSS_DISP_GLB_OFFSET + MODULE_CORE_CLK_SEL, 0x1, 1, 17);
		dpu_set_reg(connector_dev->base_addr.dss_base_addr + GLB_MODULE_CLK_SEL, 0x1, 1, 24);
		dpu_set_reg(connector_dev->base_addr.dss_base_addr + GLB_MODULE_CLK_EN, 0x1, 1, 24);
	}

	memset(&(pinfo->dsi_phy_ctrl), 0, sizeof(struct mipi_dsi_phy_ctrl));
	if (pinfo->mipi.phy_mode == CPHY_MODE)
		get_dsi_cphy_ctrl(pinfo);
	else
		get_dsi_dphy_ctrl(pinfo);

	get_mipi_dsi_timing(hisifd);

	ret = panel_next_set_fastboot(pdev);

	disp_pr_debug("fb%d, -\n", connector_info->connector_id);

	return ret;
}
*/

void mipi_dsi_peri_dis_reset(struct hisi_connector_device *connector_dev)
{
	struct hisi_connector_component *master = NULL;

	disp_check_and_no_retval(!connector_dev, err, "connector_dev is NULL\n");
	disp_check_and_no_retval(!connector_dev->base_addr.peri_crg_base, err,
			"connector_dev->base_addr.peri_crg_base is NULL\n");

	master = &connector_dev->master;

	/* dis-reset ip_reset_dis_dsi0, ip_reset_dis_dsi1 */
	if (master->connector_id == DSI0_INDEX) {
		if (is_dual_mipi_panel(connector_dev->fix_panel_info->type))
			outp32(SOC_CRGPERIPH_PERRSTDIS3_ADDR(connector_dev->base_addr.peri_crg_base), 0x30000000);
		else
			outp32(SOC_CRGPERIPH_PERRSTDIS3_ADDR(connector_dev->base_addr.peri_crg_base), 0x10000000);
	} else if (master->connector_id == DSI1_INDEX) {
		outp32(SOC_CRGPERIPH_PERRSTDIS3_ADDR(connector_dev->base_addr.peri_crg_base), 0x20000000);
	} else {
		disp_pr_err("dsi%d, not supported!\n", master->connector_id);
	}
}

void mipi_dsi_peri_reset(struct hisi_connector_device *connector_dev)
{
	struct hisi_connector_component *master = NULL;

	disp_check_and_no_retval(!connector_dev, err, "connector_dev is NULL\n");
	disp_check_and_no_retval(!connector_dev->base_addr.peri_crg_base, err,
		"connector_dev->base_addr.peri_crg_base is NULL\n");

	master = &connector_dev->master;

	/* reset DSI */
	if (master->connector_id == DSI0_INDEX) {
		if (is_dual_mipi_panel(connector_dev->fix_panel_info->type))
			outp32(SOC_CRGPERIPH_PERRSTEN3_ADDR(connector_dev->base_addr.peri_crg_base), 0x30000000);
		else
			outp32(SOC_CRGPERIPH_PERRSTEN3_ADDR(connector_dev->base_addr.peri_crg_base), 0x10000000);
	} else if (master->connector_id == DSI1_INDEX) {
		outp32(SOC_CRGPERIPH_PERRSTEN3_ADDR(connector_dev->base_addr.peri_crg_base), 0x20000000);
	} else {
		disp_pr_err("not supported!\n");
	}
}

static void mipi_dsi0_clk_enable(char __iomem *base)
{
	/*
	dpu_set_reg(dss_base_addr + DSS_DISP_GLB_OFFSET + MODULE_CORE_CLK_SEL, 0x1, 1, 16);
	dpu_set_reg(dss_base_addr + GLB_MODULE_CLK_SEL, 0x1, 1, 23);
	dpu_set_reg(dss_base_addr + GLB_MODULE_CLK_EN, 0x1, 1, 23);
	*/
	dpu_set_reg(DPU_GLB_DISP_MODULE_CLK_SEL_ADDR(base), 0x1, 1, 4); // clk_dss_core_dsi0_sel
	dpu_set_reg(DPU_GLB_DISP_MODULE_CLK_EN_ADDR(base), 0x1, 1, 4); // clk_dss_core_dsi0_en
	udelay(10);  /* 10us */
}

static void mipi_dsi1_clk_enable(char __iomem *base)
{
	/*
	dpu_set_reg(dss_base_addr + DSS_DISP_GLB_OFFSET + MODULE_CORE_CLK_SEL, 0x1, 1, 17);
	dpu_set_reg(dss_base_addr + GLB_MODULE_CLK_SEL, 0x1, 1, 24);
	dpu_set_reg(dss_base_addr + GLB_MODULE_CLK_EN, 0x1, 1, 24);
	*/
	dpu_set_reg(DPU_GLB_DISP_MODULE_CLK_SEL_ADDR(base), 0x1, 1, 3); // clk_dss_core_dsi0_sel
	dpu_set_reg(DPU_GLB_DISP_MODULE_CLK_EN_ADDR(base), 0x1, 1, 3); // clk_dss_core_dsi0_en
	udelay(10);  /* 10us */
}

void mipi_dsi_set_lp_mode(struct hisi_connector_device *connector_dev, struct hisi_connector_info *connector_info)
{
	char __iomem *mipi_dsi_base = NULL;
	char __iomem *dss_base_addr = NULL;
	disp_pr_info(" ++++ ");
	disp_check_and_no_retval(!connector_dev, err, "connector_dev is NULL\n");
	disp_check_and_no_retval(!connector_info, err, "connector_info is NULL\n");

	dss_base_addr = connector_dev->base_addr.dpu_base_addr;
	disp_check_and_no_retval(!dss_base_addr, err, "dss_base_addr is NULL\n");

	mipi_dsi_base = connector_info->connector_base;
	disp_check_and_no_retval(!mipi_dsi_base, err, "mipi_dsi_base is NULL\n");

	if (connector_dev->master.connector_id == DSI0_INDEX) {
		/* disp core dsi0 clk */
		//mipi_dsi0_clk_enable(dss_base_addr);
		mipi_dsi_on_sub1(connector_dev, mipi_dsi_base);
	} else {
		/* disp core dsi1 clk */
		//mipi_dsi1_clk_enable(dss_base_addr);
		mipi_dsi_on_sub1(connector_dev, mipi_dsi_base);
	}
}

void mipi_dsi_set_hs_mode(struct hisi_connector_device *connector_dev, struct hisi_connector_info *connector_info)
{
	char __iomem *mipi_dsi_base = NULL;
	disp_pr_info(" ++++ ");
	disp_check_and_no_retval(!connector_dev, err, "connector_dev is NULL\n");
	disp_check_and_no_retval(!connector_info, err, "connector_info is NULL\n");

	mipi_dsi_base = connector_info->connector_base;
	disp_check_and_no_retval(!mipi_dsi_base, err, "mipi_dsi_base is NULL\n");

	mipi_dsi_on_sub2(connector_dev, mipi_dsi_base);
}

void mipi_tx_off_rx_ulps_config(struct hisi_connector_device *connector_dev, struct hisi_connector_info *connector_info)
{
	int delay_count = 0;
	int delay_count_max = 16;
	char __iomem *mipi_dsi_base = NULL;

	disp_check_and_no_retval(!connector_dev, err, "connector_dev is NULL\n");
	disp_check_and_no_retval(!connector_info, err, "connector_info is NULL\n");

	mipi_dsi_base = connector_info->connector_base;

// TODO: check here
#if 0
	if (!panel_next_bypass_powerdown_ulps_support(pdev) &&
		!hisifd->panel_info.skip_power_on_off)
		return;

#endif
	disable_mipi_ldi(connector_dev, mipi_dsi_base);
	/*
	 * Read register status, maximum waiting time is 16ms
	 * 0x7FF--The lower 11 bits of the register 0x1--Register value
	 */

	while (((uint32_t)inp32(DPU_DSI_VSTATE_ADDR(mipi_dsi_base)) & 0x7FF) != 0x1) {  /*lint -e529*/
		if (++delay_count > delay_count_max) {
			disp_pr_err("wait ldi vstate idle timeout\n");
			break;
		}
		msleep(1);  /* 1ms */
	}
	/* 0--enable 1--not enabled */
	mipi_dsi_ulps_cfg(connector_dev, mipi_dsi_base, 0);
}

void mipi_dsi_off_sub1(struct hisi_connector_device *connector_dev, struct hisi_connector_info *connector_info)
{
	disp_check_and_no_retval(!connector_dev, err, "connector_dev is NULL\n");
	disp_check_and_no_retval(!connector_info, err, "connector_info is NULL\n");
	disp_check_and_no_retval(!connector_info->connector_base, err, "mipi_dsi_base is NULL\n");

	disable_mipi_ldi(connector_dev, connector_info->connector_base);

	/* Here need to enter ulps when panel off bypass ddic power down */
	mipi_tx_off_rx_ulps_config(connector_dev, connector_info);

}

static void mipi_dsi_get_ulps_stopstate(struct hisi_panel_info *pinfo, uint32_t *cmp_ulpsactivenot_val,
	uint32_t *cmp_stopstate_val, bool enter_ulps)
{
	/* ulps enter */
	if (enter_ulps) {
		if (pinfo->mipi.lane_nums >= DSI_4_LANES) {
			*cmp_ulpsactivenot_val = (BIT(5) | BIT(8) | BIT(10) | BIT(12));
			*cmp_stopstate_val = (BIT(4) | BIT(7) | BIT(9) | BIT(11));
		} else if (pinfo->mipi.lane_nums >= DSI_3_LANES) {
			*cmp_ulpsactivenot_val = (BIT(5) | BIT(8) | BIT(10));
			*cmp_stopstate_val = (BIT(4) | BIT(7) | BIT(9));
		} else if (pinfo->mipi.lane_nums >= DSI_2_LANES) {
			*cmp_ulpsactivenot_val = (BIT(5) | BIT(8));
			*cmp_stopstate_val = (BIT(4) | BIT(7));
		} else {
			*cmp_ulpsactivenot_val = (BIT(5));
			*cmp_stopstate_val = (BIT(4));
		}
	} else { /* ulps exit */
		if (pinfo->mipi.lane_nums >= DSI_4_LANES) {
			*cmp_ulpsactivenot_val = (BIT(3) | BIT(5) | BIT(8) | BIT(10) | BIT(12));
			*cmp_stopstate_val = (BIT(2) | BIT(4) | BIT(7) | BIT(9) | BIT(11));
		} else if (pinfo->mipi.lane_nums >= DSI_3_LANES) {
			*cmp_ulpsactivenot_val = (BIT(3) | BIT(5) | BIT(8) | BIT(10));
			*cmp_stopstate_val = (BIT(2) | BIT(4) | BIT(7) | BIT(9));
		} else if (pinfo->mipi.lane_nums >= DSI_2_LANES) {
			*cmp_ulpsactivenot_val = (BIT(3) | BIT(5) | BIT(8));
			*cmp_stopstate_val = (BIT(2) | BIT(4) | BIT(7));
		} else {
			*cmp_ulpsactivenot_val = (BIT(3) | BIT(5));
			*cmp_stopstate_val = (BIT(2) | BIT(4));
		}
		if (pinfo->mipi.phy_mode == CPHY_MODE) {
			*cmp_ulpsactivenot_val &= ~(BIT(3));
			*cmp_stopstate_val &= ~(BIT(2));
		}
	}
}

static int mipi_dsi_check_ulps_stopstate(char __iomem *mipi_dsi_base,
	uint32_t cmp_stopstate_val, bool enter_ulps)
{
	uint32_t try_times = 0;
	uint32_t temp = 0;

	if (enter_ulps) {
		/* check DPHY data and clock lane stopstate */
		temp = inp32(DPU_DSI_CDPHY_STATUS_ADDR(mipi_dsi_base));  /*lint -e529*/
		while ((temp & cmp_stopstate_val) != cmp_stopstate_val) {
			udelay(10);  /* 10us */
			if (++try_times > 100) {  /* try 1ms */
				disp_pr_err("dsi, check phy data and clk lane stop state failed! "
					"PHY_STATUS=0x%x\n", temp);
				return -1;
			}
			if ((temp & cmp_stopstate_val) == (cmp_stopstate_val & ~(BIT(2)))) {
				disp_pr_info("dsi datalanes are in stop state, pull down phy_txrequestclkhs\n");
				dpu_set_reg(DPU_DSI_LP_CLK_CTRL_ADDR(mipi_dsi_base), 0x0, 1, 1);
				dpu_set_reg(DPU_DSI_LP_CLK_CTRL_ADDR(mipi_dsi_base), 0x0, 1, 0);
			}
			temp = inp32(DPU_DSI_CDPHY_STATUS_ADDR(mipi_dsi_base));  /*lint -e529*/
		}
	} else {
		/* check DPHY data lane cmp_stopstate_val */
		try_times = 0;
		temp = inp32(DPU_DSI_CDPHY_STATUS_ADDR(mipi_dsi_base)); /*lint -e529*/
		while ((temp & cmp_stopstate_val) != cmp_stopstate_val) {
			udelay(10); /* 10us */
			if (++try_times > 100) { /* try 1ms */
				disp_pr_err("dsi, check phy data clk lane stop state failed! "
					"PHY_STATUS=0x%x\n", temp);
				break;
			}

			temp = inp32(DPU_DSI_CDPHY_STATUS_ADDR(mipi_dsi_base)); /*lint -e529*/
		}
	}

	return 0;
}

static void mipi_dsi_data_clock_lane_enter_ulps(struct hisi_panel_info *pinfo,
	uint32_t dsi_idx, char __iomem *mipi_dsi_base, uint32_t cmp_ulpsactivenot_val)
{
	uint32_t try_times;
	uint32_t temp;

	/* request that data lane enter ULPS */
	dpu_set_reg(DPU_DSI_CDPHY_ULPS_CTRL_ADDR(mipi_dsi_base), 0x4, 4, 0);

	/* check DPHY data lane ulpsactivenot_status */
	try_times = 0;
	temp = inp32(DPU_DSI_CDPHY_STATUS_ADDR(mipi_dsi_base)); /*lint -e529*/
	while ((temp & cmp_ulpsactivenot_val) != 0) {
		udelay(10);  /* 10us */
		if (++try_times > 100) {  /* try 1ms */
			disp_pr_err("dsi%d, request phy data lane enter ulps failed! "
				"PHY_STATUS=0x%x.\n", dsi_idx, temp);
			break;
		}

		temp = inp32(DPU_DSI_CDPHY_STATUS_ADDR(mipi_dsi_base)); /*lint -e529*/
	}

	/* request that clock lane enter ULPS */
	if (pinfo->mipi.phy_mode == DPHY_MODE) {
		dpu_set_reg(DPU_DSI_CDPHY_ULPS_CTRL_ADDR(mipi_dsi_base), 0x5, 4, 0);

		/* check DPHY clock lane ulpsactivenot_status */
		try_times = 0;
		temp = inp32(DPU_DSI_CDPHY_STATUS_ADDR(mipi_dsi_base)); /*lint -e529*/
		while ((temp & BIT(3)) != 0) {
			udelay(10); /* 10us */
			if (++try_times > 100) { /* try 1ms */
				disp_pr_err("dsi%d, request phy clk lane enter ulps failed! "
					"PHY_STATUS=0x%x\n", dsi_idx, temp);
				break;
			}

			temp = inp32(DPU_DSI_CDPHY_STATUS_ADDR(mipi_dsi_base)); /*lint -e529*/
		}
	}
}

static int mipi_dsi_ulps_enter(struct hisi_connector_device *connector_dev, uint32_t dsi_idx)
{
	uint32_t cmp_ulpsactivenot_val = 0;
	uint32_t cmp_stopstate_val = 0;
	bool is_ready = false;
	uint32_t temp;
	char __iomem *mipi_dsi_base = NULL;
	char __iomem *mipi_dsi1_base = NULL;
	struct hisi_panel_info *pinfo = NULL;

	disp_pr_debug("dsi_idx=%d, %s +!\n", dsi_idx, __func__);
	mipi_dsi_base = mipi_get_dsi_base(connector_dev->base_addr.dpu_base_addr, dsi_idx);
	pinfo = connector_dev->fix_panel_info;

	mipi_dsi_get_ulps_stopstate(pinfo, &cmp_ulpsactivenot_val, &cmp_stopstate_val, true);

	temp = (uint32_t)inp32(DPU_DSI_LP_CLK_CTRL_ADDR(mipi_dsi_base)) & BIT(1);
	if (temp && (pinfo->mipi.phy_mode == DPHY_MODE))
		cmp_stopstate_val |= (BIT(2));

	if (mipi_dsi_check_ulps_stopstate(mipi_dsi_base, cmp_stopstate_val, true))
		return 0;

	/* disable DPHY clock lane's Hight Speed Clock */
	dpu_set_reg(DPU_DSI_LP_CLK_CTRL_ADDR(mipi_dsi_base), 0x0, 1, 0);

	/* force_pll = 0 */
	dpu_set_reg(DPU_DSI_CDPHY_RST_CTRL_ADDR(mipi_dsi_base), 0x0, 1, 3);

	mipi_dsi_data_clock_lane_enter_ulps(pinfo, dsi_idx, mipi_dsi_base, cmp_ulpsactivenot_val);

	/* check phy lock == 0? */
	is_ready = mipi_phy_status_check(mipi_dsi_base, 0x0);
	if (!is_ready)
		disp_pr_debug("dsi%d, phylock == 1!\n", dsi_idx);

	/* bit13 lock sel enable (dual_mipi_panel mipi_dsi1_base+bit13 set 1), colse clock gate */

	dpu_set_reg(DPU_DSI_DPHYTX_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 13);
	dpu_set_reg(SOC_CRGPERIPH_PERDIS3_ADDR(connector_dev->base_addr.peri_crg_base), 0x3, 4, 28);

	if (dsi_idx == DSI1_INDEX) {
		mipi_dsi1_base = mipi_get_dsi_base(connector_dev->base_addr.dpu_base_addr, DSI1_INDEX);
		dpu_set_reg(DPU_DSI_DPHYTX_CTRL_ADDR(mipi_dsi1_base), 0x1, 1, 13);
		dpu_set_reg(SOC_CRGPERIPH_PERDIS3_ADDR(connector_dev->base_addr.peri_crg_base), 0xf, 4, 28);
	}

	disp_pr_debug("mipi_idx=%d, %s -!\n", dsi_idx, __func__);

	return 0;
}

static void mipi_dsi_data_clock_lane_exit_ulps(uint32_t dsi_idx, char __iomem *mipi_dsi_base,
	uint32_t cmp_ulpsactivenot_val)
{
	uint32_t try_times;
	uint32_t temp;

	/* request that data lane and clock lane exit ULPS */
	outp32(DPU_DSI_CDPHY_ULPS_CTRL_ADDR(mipi_dsi_base), 0xF);
	try_times = 0;
	temp = inp32(DPU_DSI_CDPHY_STATUS_ADDR(mipi_dsi_base)); /*lint -e529*/
	while ((temp & cmp_ulpsactivenot_val) != cmp_ulpsactivenot_val) {
		udelay(10);  /* delay 10us */
		if (++try_times > 100) {  /* try 1ms */
			disp_pr_err("dsi%d, request data clock lane exit ulps fail! "
				"PHY_STATUS=0x%x\n", dsi_idx, temp);
			break;
		}

		temp = inp32(DPU_DSI_CDPHY_STATUS_ADDR(mipi_dsi_base)); /*lint -e529*/
	}
}

static int mipi_dsi_ulps_exit(struct hisi_connector_device *connector_dev, uint32_t dsi_idx)
{
	uint32_t cmp_ulpsactivenot_val = 0;
	uint32_t cmp_stopstate_val = 0;
	bool is_ready = false;
	char __iomem *mipi_dsi_base = NULL;
	char __iomem *mipi_dsi1_base = NULL;
	struct hisi_panel_info *pinfo = NULL;

	disp_pr_debug("dsi_idx=%d, %s +!\n", dsi_idx, __func__);
	mipi_dsi_base = mipi_get_dsi_base(connector_dev->base_addr.dpu_base_addr, dsi_idx);
	pinfo = connector_dev->fix_panel_info;

	mipi_dsi_get_ulps_stopstate(pinfo, &cmp_ulpsactivenot_val, &cmp_stopstate_val, false);

	if (is_dual_mipi_panel(pinfo->type) || (dsi_idx == DSI1_INDEX))
		dpu_set_reg(SOC_CRGPERIPH_PEREN3_ADDR(connector_dev->base_addr.peri_crg_base), 0xf, 4, 28);
	else
		dpu_set_reg(SOC_CRGPERIPH_PEREN3_ADDR(connector_dev->base_addr.peri_crg_base), 0x3, 4, 28);

	udelay(10); /* 10us */
	/* force pll = 1 */
	dpu_set_reg(DPU_DSI_CDPHY_RST_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 3);

	udelay(100); /* wait pll clk */

	/* check phy lock == 1? */
	is_ready = mipi_phy_status_check(mipi_dsi_base, 0x1);
	if (!is_ready)
		disp_pr_debug("dsi%d, phylock == 0, phylock is not ready!\n", dsi_idx);

	/* bit13 lock sel enable (dual_mipi_panel mipi_dsi1_base+bit13 set 1), colse clock gate */

	dpu_set_reg(DPU_DSI_DPHYTX_CTRL_ADDR(mipi_dsi_base), 0x0, 1, 13);
	if (is_dual_mipi_panel(pinfo->type)) {
		mipi_dsi1_base = mipi_get_dsi_base(connector_dev->base_addr.dpu_base_addr, DSI1_INDEX);
		dpu_set_reg(DPU_DSI_DPHYTX_CTRL_ADDR(mipi_dsi1_base), 0x0, 1, 13);
	}

	mipi_dsi_data_clock_lane_exit_ulps(dsi_idx, mipi_dsi_base, cmp_ulpsactivenot_val);

	/* mipi spec */
	mdelay(1);

	/* clear PHY_ULPS_CTRL */
	outp32(DPU_DSI_CDPHY_ULPS_CTRL_ADDR(mipi_dsi_base), 0x0);

	mipi_dsi_check_ulps_stopstate(mipi_dsi_base, cmp_stopstate_val, false);

	/* enable DPHY clock lane's Hight Speed Clock */
	dpu_set_reg(DPU_DSI_LP_CLK_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 0);
	if (pinfo->mipi.non_continue_en)
		dpu_set_reg(DPU_DSI_LP_CLK_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 1);

	/* reset dsi */
	outp32(DPU_DSI_POR_CTRL_ADDR(mipi_dsi_base), 0x0);
	udelay(5);
	/* Power_up dsi */
	outp32(DPU_DSI_POR_CTRL_ADDR(mipi_dsi_base), 0x1);

	disp_pr_debug("dsi_idx=%d, %s -!\n", dsi_idx, __func__);
	return 0;
}

static int mipi_dsi_ulps_cfg(struct hisi_connector_device *connector_dev, char __iomem *mipi_dsi_base, int enable)
{
	uint32_t dsi_idx;

	disp_check_and_return(!connector_dev, -EINVAL, err, "connector_dev is NULL\n");
	disp_check_and_return(!mipi_dsi_base, -EINVAL, err, "mipi_dsi_base is NULL\n");

	dsi_idx = mipi_get_dsi_index(connector_dev->base_addr.dpu_base_addr, mipi_dsi_base);

	disp_pr_debug("dsi%d, +\n", dsi_idx);
	if (enable) {
		mipi_dsi_ulps_exit(connector_dev, dsi_idx);
		if (is_dual_mipi_panel(connector_dev->fix_panel_info->type))
			mipi_dsi_ulps_exit(connector_dev, DSI1_INDEX);
	} else {
		mipi_dsi_ulps_enter(connector_dev, dsi_idx);
		if (is_dual_mipi_panel(connector_dev->fix_panel_info->type))
			mipi_dsi_ulps_enter(connector_dev, DSI1_INDEX);
	}

	disp_pr_debug("dsi%d, -\n", dsi_idx);

	return 0;
}

#if 0
static void mipi_pll_cfg_for_clk_upt(
	char __iomem *mipi_dsi_base, struct mipi_dsi_phy_ctrl *phy_ctrl)
{
#ifdef CONFIG_DPU_FB_V600
	mipi_config_phy_test_code(mipi_dsi_base, PLL_PRE_DIV_ADDR, phy_ctrl->rg_pll_posdiv);
	mipi_config_phy_test_code(mipi_dsi_base, PLL_POS_DIV_ADDR,
		(phy_ctrl->rg_pll_prediv << SHIFT_4BIT) |
		(phy_ctrl->rg_pll_fbkdiv >> SHIFT_8BIT));
#else
	/* PLL configuration III */
	mipi_config_phy_test_code(mipi_dsi_base, PLL_POS_DIV_ADDR,
		(phy_ctrl->rg_pll_posdiv << SHIFT_4BIT) | phy_ctrl->rg_pll_prediv);
#endif
	/* PLL configuration IV */
	mipi_config_phy_test_code(mipi_dsi_base, PLL_FBK_DIV_ADDR,
		(phy_ctrl->rg_pll_fbkdiv & PLL_FBK_DIV_MAX_VALUE));
}

static void mipi_pll_cfg_for_clk_upt_cmd(
	struct hisi_panel_info *pinfo, char __iomem *mipi_dsi_base,
	struct mipi_dsi_phy_ctrl *phy_ctrl)
{
	if (dpu_g_fpga_flag) {
		if (pinfo->mipi.phy_mode == CPHY_MODE) {
			/* PLL configuration I */
			mipi_config_phy_test_code(mipi_dsi_base, 0x00010046,
				phy_ctrl->rg_cp + (phy_ctrl->rg_lpf_r << 4));
			/* PLL configuration II */
			mipi_config_phy_test_code(mipi_dsi_base, 0x00010048,
				phy_ctrl->rg_0p8v + (phy_ctrl->rg_2p5g << 1) +
				(pinfo->dsi_phy_ctrl.rg_320m << 2) +
				(phy_ctrl->rg_band_sel << 3) + (phy_ctrl->rg_cphy_div << 4));
			/* PLL configuration III */
			mipi_config_phy_test_code(mipi_dsi_base, 0x00010049,
				phy_ctrl->rg_pre_div);
			/* PLL configuration IV */
			mipi_config_phy_test_code(mipi_dsi_base, 0x0001004A,
				phy_ctrl->rg_div);
		} else {
			/* PLL configuration I */
			mipi_config_phy_test_code(mipi_dsi_base, 0x00010046,
				phy_ctrl->rg_cp + (phy_ctrl->rg_lpf_r << 4));
			/* PLL configuration II */
			mipi_config_phy_test_code(mipi_dsi_base, 0x00010048,
				phy_ctrl->rg_0p8v + (phy_ctrl->rg_2p5g << 1) +
				(phy_ctrl->rg_320m << 2) + (phy_ctrl->rg_band_sel << 3));
			/* PLL configuration III */
			mipi_config_phy_test_code(mipi_dsi_base, 0x00010049,
				phy_ctrl->rg_pre_div);
			/* PLL configuration IV */
			mipi_config_phy_test_code(mipi_dsi_base, 0x0001004A,
				phy_ctrl->rg_div);
		}
	} else {
		/* PLL configuration */
		mipi_pll_cfg_for_clk_upt(mipi_dsi_base, phy_ctrl);
	}
}

static void mipi_dsi_set_cdphy_bit_clk_upt_cmd(
	struct hisi_fb_data_type *hisifd, char __iomem *mipi_dsi_base,
	struct mipi_dsi_phy_ctrl *phy_ctrl)
{
	struct hisi_panel_info *pinfo = NULL;
	bool is_ready = false;

	disp_pr_info("fb%d +\n", hisifd->index);
	pinfo = &(hisifd->panel_info);

	/* PLL configuration */
	mipi_pll_cfg_for_clk_upt_cmd(pinfo, mipi_dsi_base, phy_ctrl);

	/* PLL update control */
	mipi_config_phy_test_code(mipi_dsi_base, PLL_UPT_CTRL_ADDR, 0x1);

	/* clk lane HS2LP/LP2HS */
	outp32(DPU_DSI_CLKLANE_TRANS_TIME_ADDR(mipi_dsi_base),
		(phy_ctrl->clk_lane_lp2hs_time +
		(phy_ctrl->clk_lane_hs2lp_time << SHIFT_16BIT)));
	/* data lane HS2LP/ LP2HS */
	outp32(DPU_DSI_DATALANE_TRNAS_TIME_ADDR(mipi_dsi_base),
		(phy_ctrl->data_lane_lp2hs_time +
		(phy_ctrl->data_lane_hs2lp_time << SHIFT_16BIT)));

	/* escape clock dividor */
	dpu_set_reg(DPU_DSI_CLK_DIV_CTRL_ADDR(mipi_dsi_base),
		(phy_ctrl->clk_division + (phy_ctrl->clk_division << 8)), 16, 0);

	is_ready = mipi_phy_status_check(mipi_dsi_base, PHY_LOCK_STANDARD_STATUS);
	if (!is_ready)
		disp_pr_info("fb%d, phylock is not ready!\n", hisifd->index);

	disp_pr_debug("fb%d -\n", hisifd->index);
}

static void mipi_dsi_set_cdphy_bit_clk_upt_video(
	struct hisi_fb_data_type *hisifd, char __iomem *mipi_dsi_base,
	struct mipi_dsi_phy_ctrl *phy_ctrl)
{
	struct hisi_panel_info *pinfo = NULL;
	dss_rect_t rect;
	bool is_ready = false;
	struct mipi_dsi_timing timing;

	disp_pr_debug("fb%d +\n", hisifd->index);

	pinfo = &(hisifd->panel_info);
	pinfo->dsi_phy_ctrl = *phy_ctrl;

	rect.x = 0;
	rect.y = 0;
	rect.w = pinfo->xres;
	rect.h = pinfo->yres;

	mipi_ifbc_get_rect(hisifd, &rect);

	/* PLL configuration */
	mipi_pll_cfg_for_clk_upt(mipi_dsi_base, phy_ctrl);

	/* PLL update control */
	mipi_config_phy_test_code(mipi_dsi_base, PLL_UPT_CTRL_ADDR, 0x1);

	if (pinfo->mipi.phy_mode == CPHY_MODE)
		mipi_config_cphy_spec1v0_parameter(mipi_dsi_base, pinfo,
			&pinfo->dsi_phy_ctrl);
	else
		mipi_config_dphy_spec1v2_parameter(mipi_dsi_base, pinfo,
			&pinfo->dsi_phy_ctrl);

	is_ready = mipi_phy_status_check(mipi_dsi_base, PHY_LOCK_STANDARD_STATUS);
	if (!is_ready)
		disp_pr_info("fb%d, phylock is not ready!\n", hisifd->index);

	/* phy_stop_wait_time */
	dpu_set_reg(DPU_DSI_CDPHY_LANE_NUM_ADDR(mipi_dsi_base),
		phy_ctrl->phy_stop_wait_time, 8, 8);

	/*
	 * 4. Define the DPI Horizontal timing configuration:
	 *
	 * Hsa_time = HSA*(PCLK period/Clk Lane Byte Period);
	 * Hbp_time = HBP*(PCLK period/Clk Lane Byte Period);
	 * Hline_time = (HSA+HBP+HACT+HFP)*(PCLK period/Clk Lane Byte Period);
	 */
	memset(&timing, 0, sizeof(timing));
	get_mipi_dsi_timing_config_para(hisifd, phy_ctrl, &timing);

	if (timing.hline_time < (timing.hsa + timing.hbp + timing.dpi_hsize))
		disp_pr_err("wrong hfp\n");

	dpu_set_reg(DPU_DSI_VIDEO_HSA_NUM_ADDR(mipi_dsi_base), timing.hsa, 12, 0);
	dpu_set_reg(DPU_DSI_VIDEO_HBP_NUM_ADDR(mipi_dsi_base), timing.hbp, 12, 0);
	dpu_set_reg(mipi_dsi_base + MIPI_LDI_DPI0_HRZ_CTRL3,
		disp_reduce(timing.dpi_hsize), 12, 0);
	dpu_set_reg(DPU_DSI_VIDEO_HLINE_NUM_ADDR(mipi_dsi_base),
		timing.hline_time, 15, 0);

	/* Configure core's phy parameters */
	dpu_set_reg(DPU_DSI_CLKLANE_TRANS_TIME_ADDR(mipi_dsi_base),
		phy_ctrl->clk_lane_lp2hs_time, 10, 0);
	dpu_set_reg(DPU_DSI_CLKLANE_TRANS_TIME_ADDR(mipi_dsi_base),
		phy_ctrl->clk_lane_hs2lp_time, 10, 16);

	outp32(DPU_DSI_DATALANE_TRNAS_TIME_ADDR(mipi_dsi_base),
		(phy_ctrl->data_lane_lp2hs_time +
		(phy_ctrl->data_lane_hs2lp_time << 16)));

	disp_pr_debug("fb%d -\n", hisifd->index);
}

static bool check_pctrl_trstop_flag(
	struct hisi_fb_data_type *hisifd, int time_count)
{
	bool is_ready = false;
	int count;
	uint32_t temp = 0;
	uint32_t tmp1 = 0;

	if (is_dual_mipi_panel(hisifd)) {
		for (count = 0; count < time_count; count++) {
			temp = inp32(hisifd->mipi_dsi0_base +
					MIPIDSI_DPHYTX_TRSTOP_FLAG_OFFSET);
			tmp1 = inp32(hisifd->mipi_dsi1_base +
					MIPIDSI_DPHYTX_TRSTOP_FLAG_OFFSET);
			if ((temp & tmp1 & 0x1) == 0x1) {
				is_ready = true;
				break;
			}
			udelay(2); /* 2us delay each time  */
		}
	} else {
		for (count = 0; count < time_count; count++) {
			temp = inp32(hisifd->mipi_dsi0_base +
					MIPIDSI_DPHYTX_TRSTOP_FLAG_OFFSET);
			if ((temp & 0x1) == 0x1) {
				is_ready = true;
				break;
			}
			udelay(2); /* 2us delay each time  */
		}
	}

	return is_ready;
}

static uint32_t get_stopstate_msk_value(uint8_t lane_nums)
{
	uint32_t stopstate_msk;

	if (lane_nums == DSI_4_LANES)
		stopstate_msk = BIT(0);
	else if (lane_nums == DSI_3_LANES)
		stopstate_msk = BIT(0) | BIT(4);
	else if (lane_nums == DSI_2_LANES)
		stopstate_msk = BIT(0) | BIT(3) | BIT(4);
	else
		stopstate_msk = BIT(0) | BIT(2) | BIT(3) | BIT(4);

	return stopstate_msk;
}

bool mipi_dsi_wait_vfp_end(struct hisi_fb_data_type *hisifd,
	uint64_t lane_byte_clk, struct timespec64 *tv0)
{
	uint32_t vfp_time = 0;
	uint32_t timediff = 0;
	bool is_ready = false;
	uint32_t hline_time;
	uint32_t vfp_line;
	uint32_t stopstate_msk;
	struct timespec64 tv1;

	hline_time = (uint32_t)inp32(DPU_DSI_VIDEO_HLINE_NUM_ADDR(hisifd->mipi_dsi0_base)) & VFP_TIME_MASK;

	vfp_line = (uint32_t)inp32(DPU_DSI_VIDEO_VFP_NUM_ADDR(hisifd->mipi_dsi0_base)) & 0x3FF;

	if (lane_byte_clk >= MILLION_CONVERT) {
		vfp_time = (vfp_line + VFP_TIME_OFFSET) * hline_time / ((uint32_t)(lane_byte_clk / MILLION_CONVERT));
	} else {
		disp_pr_err("vfp_time == 0\n");
		vfp_time = VFP_DEF_TIME;
	}

	disp_pr_debug("hline_time = %u, vfp_line = %u, lane_byte_clk = %llu\n",
		hline_time, vfp_line, lane_byte_clk);

	stopstate_msk = get_stopstate_msk_value(hisifd->panel_info.mipi.lane_nums);

	dpu_set_reg(hisifd->mipi_dsi0_base + MIPIDSI_DPHYTX_CTRL_OFFSET, 1, 1, 0);
	dpu_set_reg(hisifd->mipi_dsi0_base + MIPIDSI_DPHYTX_CTRL_OFFSET, stopstate_msk, 5, 3);
	if (is_dual_mipi_panel(hisifd)) {
		dpu_set_reg(hisifd->mipi_dsi1_base + MIPIDSI_DPHYTX_CTRL_OFFSET, 1, 1, 0);
		dpu_set_reg(hisifd->mipi_dsi1_base + MIPIDSI_DPHYTX_CTRL_OFFSET, stopstate_msk, 5, 3);
	}
	disp_pr_debug("is_ready == %d, timediff = %d , vfp_time = %d\n", is_ready, timediff, vfp_time);
	while ((!is_ready) && (timediff < vfp_time)) {
		is_ready = check_pctrl_trstop_flag(hisifd, PCTRL_TRY_TIME);
		disp_pr_info("is_ready == %d\n", is_ready);
		hisifb_get_timestamp(&tv1);
		timediff = hisifb_timestamp_diff(tv0, &tv1);
	}
	disp_pr_info("timediff = %d us, vfp_time = %d us\n", timediff, vfp_time);
	dpu_set_reg(hisifd->mipi_dsi0_base + MIPIDSI_DPHYTX_CTRL_OFFSET, 0, 1, 0);
	if (is_dual_mipi_panel(hisifd))
		dpu_set_reg(hisifd->mipi_dsi1_base + MIPIDSI_DPHYTX_CTRL_OFFSET, 0, 1, 0);

	return is_ready;
}

static bool mipi_dsi_check_ldi_vstate(struct hisi_fb_data_type *hisifd, uint64_t lane_byte_clk)
{
	bool is_ready = false;
	uint32_t count = 0;
	uint32_t vfp_time = 0;
	uint32_t ldi_vstate;
	uint32_t hline_time;
	uint32_t vfp_line;

	hline_time = (uint32_t)inp32(DPU_DSI_VIDEO_HLINE_NUM_ADDR(hisifd->mipi_dsi0_base)) & VFP_TIME_MASK;

	vfp_line = (uint32_t)inp32(DPU_DSI_VIDEO_VFP_NUM_ADDR(hisifd->mipi_dsi0_base)) & 0x3FF; /* mask bit0:9 */

	if (lane_byte_clk >= MILLION_CONVERT) {
		vfp_time = (vfp_line + VFP_TIME_OFFSET) *
		hline_time / ((uint32_t)(lane_byte_clk / MILLION_CONVERT)); /*lint -e414*/
	} else {
		disp_pr_err("vfp_time == 0\n");
		vfp_time = VFP_DEF_TIME;
	}

	disp_pr_debug("hline_time = %d, vfp_line = %d, vfp_time = %d\n", hline_time, vfp_line, vfp_time);

	/* read ldi vstate reg value and mask bit0:15 */
	ldi_vstate = inp32(hisifd->mipi_dsi0_base + MIPI_LDI_VSTATE) & 0xFFFF; /*lint -e529*/
	while ((ldi_vstate == LDI_VSTATE_VFP) && (count < vfp_time)) {
		udelay(1);  /* 1us delay each time  */
		ldi_vstate = inp32(hisifd->mipi_dsi0_base + MIPI_LDI_VSTATE) & 0xFFFF; /*lint -e529*/
		count++;
	}
	if ((ldi_vstate == LDI_VSTATE_IDLE) || (ldi_vstate == LDI_VSTATE_V_WAIT_GPU))
		is_ready = true;

	if (!is_ready)
		disp_pr_info("ldi_vstate = 0x%x\n", ldi_vstate);

	return is_ready;
}

int mipi_dsi_reset_underflow_clear(struct hisi_fb_data_type *hisifd)
{
	uint32_t vfp_time = 0;
	bool is_ready = false;
	uint32_t timediff = 0;
	char __iomem *mipi_dsi_base;
	uint64_t lane_byte_clk;
	struct timespec64 tv0;

	if (is_dp_panel(hisifd))
		goto reset_exit;

	if (hisifd->index == PRIMARY_PANEL_IDX)
		mipi_dsi_base = get_mipi_dsi_base(hisifd);
	else if (hisifd->index == EXTERNAL_PANEL_IDX)
		mipi_dsi_base = hisifd->mipi_dsi1_base;
	else
		goto reset_exit;

	hisifb_get_timestamp(&tv0);

	lane_byte_clk = (hisifd->panel_info.mipi.phy_mode == DPHY_MODE) ?
		hisifd->panel_info.dsi_phy_ctrl.lane_byte_clk :
		hisifd->panel_info.dsi_phy_ctrl.lane_word_clk;

	is_ready = mipi_dsi_wait_vfp_end(hisifd, lane_byte_clk, &tv0);
	if (!is_ready)
		disp_pr_err("check_pctrl_trstop_flag fail, vstate = 0x%x\n",
			inp32(mipi_dsi_base + MIPI_LDI_VSTATE));

	disp_pr_info("timediff=%d us, vfp_time=%d us\n", timediff, vfp_time);

	dpu_set_reg(mipi_dsi_base + MIPIDSI_PWR_UP_OFFSET, 0x0, 1, 0);
	if (is_dual_mipi_panel(hisifd))
		dpu_set_reg(hisifd->mipi_dsi1_base + MIPIDSI_PWR_UP_OFFSET, 0x0, 1, 0);
	udelay(5); /* timing constraint */
	dpu_set_reg(mipi_dsi_base + MIPIDSI_PWR_UP_OFFSET, 0x1, 1, 0);
	if (is_dual_mipi_panel(hisifd))
		dpu_set_reg(hisifd->mipi_dsi1_base + MIPIDSI_PWR_UP_OFFSET, 0x1, 1, 0);

	ldi_data_gate(hisifd, true);
	enable_ldi(hisifd);

reset_exit:
	hisifd->underflow_flag = 0;
	return 0;
}

static void mipi_set_cdphy_bit_clk(
	struct hisi_fb_data_type *hisifd, struct mipi_dsi_phy_ctrl *phy_ctrl,
	struct hisi_panel_info *pinfo, uint8_t esd_enable)
{
	if (is_mipi_cmd_panel(hisifd)) {
		mipi_dsi_set_cdphy_bit_clk_upt_cmd(hisifd, hisifd->mipi_dsi0_base, phy_ctrl);
		if (is_dual_mipi_panel(hisifd))
			mipi_dsi_set_cdphy_bit_clk_upt_cmd(hisifd, hisifd->mipi_dsi1_base, phy_ctrl);
	} else {
		dpu_set_reg(DPU_DSI_LP_CLK_CTRL_ADDR(hisifd->mipi_dsi0_base), 0x0, DSI_CLK_BW, DSI_CLK_BS);
		mipi_dsi_set_cdphy_bit_clk_upt_video(hisifd, hisifd->mipi_dsi0_base, phy_ctrl);
		dpu_set_reg(DPU_DSI_LP_CLK_CTRL_ADDR(hisifd->mipi_dsi0_base), 0x1, DSI_CLK_BW, DSI_CLK_BS);
		if (is_dual_mipi_panel(hisifd)) {
			dpu_set_reg(DPU_DSI_LP_CLK_CTRL_ADDR(hisifd->mipi_dsi1_base), 0x0, DSI_CLK_BW, DSI_CLK_BS);
			mipi_dsi_set_cdphy_bit_clk_upt_video(hisifd, hisifd->mipi_dsi1_base, phy_ctrl);
			dpu_set_reg(DPU_DSI_LP_CLK_CTRL_ADDR(hisifd->mipi_dsi1_base),
				0x1, DSI_CLK_BW, DSI_CLK_BS);
			disp_pr_debug("end, phy1 status: 0x%x, vstate: 0x%x,\n",
				inp32(DPU_DSI_CDPHY_STATUS_ADDR(hisifd->mipi_dsi1_bas)),
				inp32(hisifd->mipi_dsi1_base + MIPI_LDI_VSTATE));
		}
		pinfo->esd_enable = esd_enable;
		enable_ldi(hisifd);
	}
	disp_pr_debug("end, phy status: 0x%x, vstate: 0x%x,\n",
		inp32(DPU_DSI_CDPHY_STATUS_ADDR(hisifd->mipi_dsi0_bas)),
		inp32(hisifd->mipi_dsi0_base + MIPI_LDI_VSTATE));
}

int mipi_dsi_bit_clk_upt_isr_handler(
	struct hisi_fb_data_type *hisifd)
{
	struct mipi_dsi_phy_ctrl phy_ctrl = {0};
	struct hisi_panel_info *pinfo = NULL;
	bool is_ready = false;
	uint8_t esd_enable;
	uint64_t lane_byte_clk;
	uint32_t dsi_bit_clk_upt;

	disp_check_and_return(!hisifd, 0, err, "hisifd is NULL!\n");

	pinfo = &(hisifd->panel_info);
	dsi_bit_clk_upt = pinfo->mipi.dsi_bit_clk_upt;

	disp_check_and_return((hisifd->index != PRIMARY_PANEL_IDX), 0, err, "fb%d, not support!\n", hisifd->index);

	if (!spin_trylock(&hisifd->mipi_resource_lock)) {
		disp_pr_info("dsi_bit_clk %u will update in next frame\n", dsi_bit_clk_upt);
		return 0;
	}

	hisifd->mipi_dsi_bit_clk_update = 1;
	esd_enable = pinfo->esd_enable;
	if (is_mipi_video_panel(hisifd)) {
		pinfo->esd_enable = 0;
		disable_ldi(hisifd);
	}

	spin_unlock(&hisifd->mipi_resource_lock);

	if (dsi_bit_clk_upt == pinfo->mipi.dsi_bit_clk) {
		hisifd->mipi_dsi_bit_clk_update = 0;
		return 0;
	}

	if (pinfo->mipi.phy_mode == CPHY_MODE)
		get_dsi_cphy_ctrl(hisifd, &phy_ctrl);
	else
		get_dsi_dphy_ctrl(hisifd, &phy_ctrl);

	lane_byte_clk = hisifd->panel_info.dsi_phy_ctrl.lane_byte_clk;
	if (hisifd->panel_info.mipi.phy_mode == CPHY_MODE)
		lane_byte_clk = hisifd->panel_info.dsi_phy_ctrl.lane_word_clk;

	is_ready = mipi_dsi_check_ldi_vstate(hisifd, lane_byte_clk);
	if (!is_ready) {
		if (is_mipi_video_panel(hisifd)) {
			pinfo->esd_enable = esd_enable;
			enable_ldi(hisifd);
		}
		hisifd->mipi_dsi_bit_clk_update = 0;
		disp_pr_info("PERI_STAT0 or ldi vstate is not ready\n");
		return 0;
	}

	mipi_set_cdphy_bit_clk(hisifd, &phy_ctrl, pinfo, esd_enable);

	disp_pr_info("Mipi clk success changed from %d M switch to %d M\n", pinfo->mipi.dsi_bit_clk, dsi_bit_clk_upt);

	pinfo->dsi_phy_ctrl = phy_ctrl;
	pinfo->mipi.dsi_bit_clk = dsi_bit_clk_upt;
	hisifd->mipi_dsi_bit_clk_update = 0;

	return 0;
}
#endif

