/*
 * Copyright (c) 2013-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#include "dpu_mipi_dsi.h"

/* addr of dphy */
static uint32_t g_dphy_mipi_tab[PHY_TST_ENUM_MAX][DPHY_VER_MAX] = {
	{ 0x00010012, 0x00010062 }, /* PHY_TST_AFE_LPTX_CFG_I_ENUM */
	{ 0x00010013, 0x0001006A }, /* PHY_TST_AFE_LPTX_CFG_II_ENUM */
	{ 0x00010015, 0x00010063 }, /* PHY_TST_AFE_PLL_CFG_II_ENUM */
	{ 0x0001001C, 0x00010067 }, /* PHY_TST_AFE_REF_CFG_I_ENUM */
	{ 0x0001001D, 0x00010068 }, /* PHY_TST_AFE_REF_CFG_II_ENUM */
	{ 0x0001001E, 0x00010069 }, /* PHY_TST_MISC_AFE_CFG_ENUM */
	{ 0x00010020, 0x00010017 }, /* PHY_TST_CLK_PRE_DELAY_ENUM */
	{ 0x00010021, 0x00010018 }, /* PHY_TST_CLK_POST_DELAY_ENUM */
	{ 0x00010022, 0x00010010 }, /* PHY_TST_CLK_TLPX_ENUM */
	{ 0x00010023, 0x00010011 }, /* PHY_TST_CLK_PREPARE_ENUM */
	{ 0x00010024, 0x00010012 }, /* PHY_TST_CLK_ZERO_ENUM */
	{ 0x00010025, 0x00010013 }, /* PHY_TST_CLK_TRAIL_ENUM */
	{ 0x00010030, 0x00010028 }, /* PHY_TST_DATA_PRE_DELAY_ENUM */
	{ 0x00010031, 0x00010029 }, /* PHY_TST_DATA_POST_DELAY_ENUM */
	{ 0x00010032, 0x00010020 }, /* PHY_TST_DATA_TLPX_ENUM */
	{ 0x00010033, 0x00010021 }, /* PHY_TST_DATA_PREPARE_ENUM */
	{ 0x00010034, 0x00010022 }, /* PHY_TST_DATA_ZERO_ENUM */
	{ 0x00010035, 0x00010023 }, /* PHY_TST_DATA_TRAIL_ENUM */
	{ 0x00010036, 0x00010024 }, /* PHY_TST_DATA_TA_GO_ENUM */
	{ 0x00010037, 0x00010025 }  /* PHY_TST_DATA_TA_GET_ENUM */
};

static uint32_t phy_addr(int index)
{
	int ver = 0;

	if (g_mipi_dphy_version < DPHY_VER_MAX)
		ver = g_mipi_dphy_version;

	return g_dphy_mipi_tab[index][ver];
}

static void get_dsi_phy_ctrl_pll_para_v14(uint64_t *lane_clock,
	struct mipi_dsi_phy_ctrl *phy_ctrl, struct dpu_panel_info *pinfo)
{
	uint32_t vco_clk;

	if (*lane_clock > 2500 || *lane_clock < 80) {
		DPU_FB_ERR("80M<=lane_clock<=2500M, not support %llu M\n", *lane_clock);
		*lane_clock = 960;
	}

	phy_ctrl->rg_pll_prediv = 0;
	phy_ctrl->rg_pll_posdiv = 0;
	vco_clk = *lane_clock * 1000000;

	phy_ctrl->rg_pll_fbkdiv = round1(vco_clk, DEFAULT_MIPI_CLK_RATE);

	/* RG_PLL_FBKDIV[7:0], max value is 0xff */
	if (phy_ctrl->rg_pll_fbkdiv > 0xff)
		phy_ctrl->rg_pll_fbkdiv = 0xff;

	*lane_clock = phy_ctrl->rg_pll_fbkdiv * DEFAULT_MIPI_CLK_RATE;

	DPU_FB_INFO("rg_pll_prediv=%d, rg_pll_posdiv=%d, rg_pll_fbkdiv=%d, lane_clock = %llu\n",
		phy_ctrl->rg_pll_prediv,
		phy_ctrl->rg_pll_posdiv,
		phy_ctrl->rg_pll_fbkdiv, *lane_clock);
}

static void dsi_phy_ctrl_pll_para_v12_prepare(uint64_t *lane_clock,
	struct mipi_dsi_phy_ctrl *phy_ctrl, uint32_t *m_pll, uint32_t *n_pll)
{
	uint64_t vco_div = 1;  /* default clk division */
	uint64_t m_pll_remainder[2] = {0};
	uint32_t i;

	/* those magic number is chip spec */
	if ((*lane_clock >= 320) && (*lane_clock <= 2500)) {
		phy_ctrl->rg_band_sel = 0;  /* 0x1E[2] */
		vco_div = 1;
	} else if ((*lane_clock >= 80) && (*lane_clock < 320)) {
		phy_ctrl->rg_band_sel = 1;
		vco_div = 4;
	} else {
		DPU_FB_ERR("80M<=lane_clock<=2500M, not support %llu M\n", *lane_clock);
	}

	for (i = 0; i < 2; i++) {
		*n_pll = i + 1;
		m_pll_remainder[i] = ((uint64_t)(*n_pll * (*lane_clock) *
			1000000UL * 1000UL / DEFAULT_MIPI_CLK_RATE) % 1000) * 10 / 1000;
	}

	*n_pll = 1;
	if (m_pll_remainder[1] < m_pll_remainder[0])
		*n_pll = 2;

	*m_pll = *n_pll * (*lane_clock) * vco_div * 1000000UL / DEFAULT_MIPI_CLK_RATE;

	DPU_FB_DEBUG("m_pll = %d, n_pll = %d\n", *m_pll, *n_pll);

	*lane_clock = *m_pll * (DEFAULT_MIPI_CLK_RATE / *n_pll) / vco_div;
	DPU_FB_DEBUG("Config : lane_clock = %llu\n", *lane_clock);
}

static void get_dsi_phy_ctrl_pll_para_v12(uint64_t *lane_clock,
	struct mipi_dsi_phy_ctrl *phy_ctrl, struct dpu_panel_info *pinfo)
{
	uint32_t m_pll = 0;
	uint32_t n_pll = 0;

	dsi_phy_ctrl_pll_para_v12_prepare(lane_clock, phy_ctrl, &m_pll, &n_pll);

	/* test_code_0x14 other parameters config */
	phy_ctrl->rg_pll_pre_div1p = n_pll - 1;  /* 0x14[3] */
	phy_ctrl->rg_pll_chp = 0;  /* 0x14[1:0] */

	/* test_code_0x15 other parameters config */
	phy_ctrl->rg_div = m_pll; /* 0x15[7:0] */

	/* test_code_0x16 other parameters config, 0x16[3:2] reserved */
	phy_ctrl->rg_pll_cp = 0x3; /* 0x16[7:5] */

	phy_ctrl->reload_sel = 0; /* 0x1E[4] */
	if (is_mipi_cmd_panel_ext(pinfo))
		phy_ctrl->reload_sel = 1; /* 0x1E[4] */

	phy_ctrl->rg_phase_gen_en = 1; /* 0x1E[3] */
	phy_ctrl->pll_register_override = 0; /* 0x1E[1] */
	phy_ctrl->pll_power_down = 1; /* 0x1E[0] */

	phy_ctrl->rg_lptx_sri = 0x55;
	if (pinfo->mipi.rg_lptx_sri_adjust != 0)
		phy_ctrl->rg_lptx_sri = pinfo->mipi.rg_lptx_sri_adjust;

	phy_ctrl->rg_vrefsel_lptx = 0x25;
	if (pinfo->mipi.rg_vrefsel_lptx_adjust != 0)
		phy_ctrl->rg_vrefsel_lptx = pinfo->mipi.rg_vrefsel_lptx_adjust;

	/* HSTX select VCM VREF */
	phy_ctrl->rg_vrefsel_vcm = 0x55; /* 0x1D */
	if (pinfo->mipi.rg_vrefsel_vcm_clk_adjust != 0)
		phy_ctrl->rg_vrefsel_vcm = (phy_ctrl->rg_vrefsel_vcm & 0x0F) |
			((pinfo->mipi.rg_vrefsel_vcm_clk_adjust & 0x0F) << 4);

	if (pinfo->mipi.rg_vrefsel_vcm_data_adjust != 0)
		phy_ctrl->rg_vrefsel_vcm = (phy_ctrl->rg_vrefsel_vcm & 0xF0) |
			(pinfo->mipi.rg_vrefsel_vcm_data_adjust & 0x0F);

	/* if reload_sel = 1, need to set load_command */
	phy_ctrl->load_command = 0x5A;
}

static void get_dsi_phy_ctrl_lane_para_prepare(struct dpu_panel_info *pinfo,
	struct mipi_dsi_clk_data *clk_data, uint64_t lane_clock, uint32_t *out_ui)
{
	/* thos magic number is followed the chip spec at this function */
	clk_data->ui = 10 * 1000000000UL * clk_data->accuracy / lane_clock;
	*out_ui = clk_data->ui;

	/* unit of measurement */
	clk_data->unit_tx_byte_clk_hs = 8 * clk_data->ui;

	/* D-PHY Specification : 60ns + 52*UI <= clk_post */
	clk_data->clk_post = 600 * clk_data->accuracy + 52 * clk_data->ui + pinfo->mipi.clk_post_adjust * clk_data->ui;

	/* clocked by TXBYTECLKHS */
	clk_data->clk_pre_delay = 0 + pinfo->mipi.clk_pre_delay_adjust * clk_data->ui;

	/* D-PHY Specification : clk_t_hs_trial >= 60ns
	 * clocked by TXBYTECLKHS
	 */
	clk_data->clk_t_hs_trial = 600 * clk_data->accuracy + 3 * clk_data->unit_tx_byte_clk_hs +
		pinfo->mipi.clk_t_hs_trial_adjust * clk_data->ui;

	/* D-PHY Specification : 38ns <= clk_t_hs_prepare <= 95ns
	 * clocked by TXBYTECLKHS
	 */
	if (pinfo->mipi.clk_t_hs_prepare_adjust == 0)
		pinfo->mipi.clk_t_hs_prepare_adjust = 43;

	clk_data->clk_t_hs_prepare = ((380 * clk_data->accuracy +
		pinfo->mipi.clk_t_hs_prepare_adjust * clk_data->ui) <=
		(950 * clk_data->accuracy - 8 * clk_data->ui)) ?
		(380 * clk_data->accuracy + pinfo->mipi.clk_t_hs_prepare_adjust * clk_data->ui) :
		(950 * clk_data->accuracy - 8 * clk_data->ui);

	/* clocked by TXBYTECLKHS */
	clk_data->data_post_delay = 0 + pinfo->mipi.data_post_delay_adjust * clk_data->ui;

	/* D-PHY Specification : data_t_hs_trial >=
	 * max( n*8*UI, 60ns + n*4*UI ), n = 1
	 * clocked by TXBYTECLKHS
	 */
	clk_data->data_t_hs_trial = ((600 * clk_data->accuracy + 4 * clk_data->ui) >= (8 * clk_data->ui) ?
		(600 * clk_data->accuracy + 4 * clk_data->ui) : (8 * clk_data->ui)) + 8 * clk_data->ui +
		3 * clk_data->unit_tx_byte_clk_hs +
		pinfo->mipi.data_t_hs_trial_adjust * clk_data->ui;

	/* D-PHY Specification : 40ns + 4*UI <= data_t_hs_prepare <= 85ns + 6*UI
	 * clocked by TXBYTECLKHS
	 */
	if (pinfo->mipi.data_t_hs_prepare_adjust == 0)
		pinfo->mipi.data_t_hs_prepare_adjust = 35;

	clk_data->data_t_hs_prepare =  ((400  * clk_data->accuracy +
		4 * clk_data->ui + pinfo->mipi.data_t_hs_prepare_adjust * clk_data->ui) <=
		(850 * clk_data->accuracy + 6 * clk_data->ui - 8 * clk_data->ui)) ?
		(400  * clk_data->accuracy + 4 * clk_data->ui +
		pinfo->mipi.data_t_hs_prepare_adjust * clk_data->ui) :
		(850 * clk_data->accuracy + 6 * clk_data->ui - 8 * clk_data->ui);

	/* D-PHY chip spec : clk_t_lpx + clk_t_hs_prepare > 200ns
	 * D-PHY Specification : clk_t_lpx >= 50ns
	 * clocked by TXBYTECLKHS
	 */
	clk_data->clk_t_lpx = (((2000 * clk_data->accuracy - clk_data->clk_t_hs_prepare) >= 500 * clk_data->accuracy) ?
		((2000 * clk_data->accuracy - clk_data->clk_t_hs_prepare)) : (500 * clk_data->accuracy)) +
		pinfo->mipi.clk_t_lpx_adjust * clk_data->ui;

	/* D-PHY Specification : clk_t_hs_zero + clk_t_hs_prepare >= 300 ns
	 * clocked by TXBYTECLKHS
	 */
	clk_data->clk_t_hs_zero = 3000 * clk_data->accuracy - clk_data->clk_t_hs_prepare +
		3 * clk_data->unit_tx_byte_clk_hs +
		pinfo->mipi.clk_t_hs_zero_adjust * clk_data->ui;

	/* D-PHY chip spec : data_t_lpx + data_t_hs_prepare > 200ns
	 * D-PHY Specification : data_t_lpx >= 50ns
	 * clocked by TXBYTECLKHS
	 * 2000 * accuracy - data_t_hs_prepare;
	 */
	clk_data->data_t_lpx = clk_data->clk_t_lpx + pinfo->mipi.data_t_lpx_adjust * clk_data->ui;

	/* D-PHY Specification:
	 * data_t_hs_zero + data_t_hs_prepare >= 145ns + 10*UI
	 * clocked by TXBYTECLKHS
	 */
	clk_data->data_t_hs_zero = 1450 * clk_data->accuracy + 10 * clk_data->ui - clk_data->data_t_hs_prepare +
		3 * clk_data->unit_tx_byte_clk_hs + pinfo->mipi.data_t_hs_zero_adjust * clk_data->ui;
}

static void get_dsi_phy_ctrl_lane_para(uint64_t lane_clock,
	struct mipi_dsi_phy_ctrl *phy_ctrl, struct dpu_panel_info *pinfo, uint32_t *out_ui)
{
	struct mipi_dsi_clk_data clk_data = {0};

	clk_data.accuracy = 10;  /* magnification */

	dpu_check_and_no_retval((lane_clock == 0), ERR, "lan lock is 0.\n");

	get_dsi_phy_ctrl_lane_para_prepare(pinfo, &clk_data, lane_clock, out_ui);

	phy_ctrl->data_post_delay = round1(clk_data.data_post_delay, clk_data.unit_tx_byte_clk_hs);
	phy_ctrl->data_t_hs_prepare = round1(clk_data.data_t_hs_prepare, clk_data.unit_tx_byte_clk_hs);
	phy_ctrl->data_t_lpx = round1(clk_data.data_t_lpx, clk_data.unit_tx_byte_clk_hs);
	phy_ctrl->data_t_hs_zero = round1(clk_data.data_t_hs_zero, clk_data.unit_tx_byte_clk_hs);
	phy_ctrl->data_t_hs_trial = round1(clk_data.data_t_hs_trial, clk_data.unit_tx_byte_clk_hs);
	phy_ctrl->data_t_ta_go = 4;
	phy_ctrl->data_t_ta_get = 5;

	phy_ctrl->clk_post_delay = phy_ctrl->data_t_hs_trial + round1(clk_data.clk_post, clk_data.unit_tx_byte_clk_hs);

	phy_ctrl->clk_pre_delay = round1(clk_data.clk_pre_delay, clk_data.unit_tx_byte_clk_hs);
	phy_ctrl->clk_t_hs_prepare = round1(clk_data.clk_t_hs_prepare, clk_data.unit_tx_byte_clk_hs);
	phy_ctrl->clk_t_lpx = round1(clk_data.clk_t_lpx, clk_data.unit_tx_byte_clk_hs);
	phy_ctrl->clk_t_hs_zero = round1(clk_data.clk_t_hs_zero, clk_data.unit_tx_byte_clk_hs);
	phy_ctrl->clk_t_hs_trial = round1(clk_data.clk_t_hs_trial, clk_data.unit_tx_byte_clk_hs);
}

int mipi_dsi_ulps_cfg(struct dpu_fb_data_type *dpufd, int enable);

static void get_default_m_n_pll_value(uint32_t *m_pll, uint32_t *n_pll,
	uint64_t vco_div, uint64_t *lane_clock)
{
	uint32_t m_n_fract;
	uint32_t m_n_int;

	/* The following devil numbers from chip protocol
	 * It contains lots of fixed numbers
	 */
	m_n_int = *lane_clock * vco_div * 1000000UL / DEFAULT_MIPI_CLK_RATE;
	m_n_fract = ((*lane_clock * vco_div * 1000000UL * 1000UL / DEFAULT_MIPI_CLK_RATE) % 1000) * 10 / 1000;

	if (m_n_int % 2 == 0) {
		if (m_n_fract * 6 >= 50) {
			*n_pll = 2;
			*m_pll = (m_n_int + 1) * 2;
		} else if (m_n_fract * 6 >= 30) {
			*n_pll = 3;
			*m_pll = m_n_int * (*n_pll) + 2;
		} else {
			*n_pll = 1;
			*m_pll = m_n_int * (*n_pll);
		}
	} else {
		if (m_n_fract * 6 >= 50) {
			*n_pll = 1;
			*m_pll = (m_n_int + 1) * (*n_pll);
		} else if (m_n_fract * 6 >= 30) {
			*n_pll = 1;
			*m_pll = (m_n_int + 1) * (*n_pll);
		} else if (m_n_fract * 6 >= 10) {
			*n_pll = 3;
			*m_pll = m_n_int * (*n_pll) + 1;
		} else {
			*n_pll = 2;
			*m_pll = m_n_int * (*n_pll);
		}
	}
}

static void mipi_dsi_rg_pll_phy_config_prepare(struct dpu_panel_info *pinfo, uint64_t *lane_clock,
	uint32_t *m_pll, uint32_t *n_pll, uint64_t vco_div)
{
	uint32_t i;
	uint32_t temp_min;
	uint32_t m_pll_remainder[3] = {0};

	if (pinfo->mipi.phy_m_n_count_update) {
		for (i = 0; i < 3; i++) {
			*n_pll = i + 1;
			m_pll_remainder[i] = ((*n_pll * (*lane_clock) * 1000000UL * 1000UL /
				DEFAULT_MIPI_CLK_RATE) % 1000) * 10 / 1000;
		}
		temp_min = m_pll_remainder[0];
		*n_pll = 1;
		for (i = 1; i < 3; i++) {
			if (temp_min > m_pll_remainder[i]) {
				temp_min = m_pll_remainder[i];
				*n_pll = i + 1;
			}
		}
		*m_pll = (*n_pll) * (*lane_clock) * vco_div * 1000000UL / DEFAULT_MIPI_CLK_RATE;
		DPU_FB_DEBUG("m_pll = %d n_pll = %d\n", *m_pll, *n_pll);
	} else {
		get_default_m_n_pll_value(m_pll, n_pll, vco_div, lane_clock);
	}
}

static void set_rg_pll_phy_ctrl_data(struct mipi_dsi_phy_ctrl *phy_ctrl, uint32_t m_pll, uint32_t n_pll,
	uint64_t vco_div, uint64_t *lane_clock)
{
	/* if set rg_pll_enswc=1, rg_pll_fbd_s can't be 0
	 * The following devil numbers from chip protocol
	 * It contains lots of fixed numbers
	 */
	if (m_pll <= 8) {
		phy_ctrl->rg_pll_fbd_s = 1;
		phy_ctrl->rg_pll_enswc = 0;

		if (m_pll % 2 == 0) {
			phy_ctrl->rg_pll_fbd_p = m_pll / 2;
		} else {
			if (n_pll == 1) {
				n_pll *= 2;
				phy_ctrl->rg_pll_fbd_p = (m_pll  * 2) / 2;
			} else {
				DPU_FB_ERR("phy m_pll not support!m_pll = %d\n", m_pll);
				return;
			}
		}
	} else if (m_pll <= 300) {
		if (m_pll % 2 == 0)
			phy_ctrl->rg_pll_enswc = 0;
		else
			phy_ctrl->rg_pll_enswc = 1;

		phy_ctrl->rg_pll_fbd_s = 1;
		phy_ctrl->rg_pll_fbd_p = m_pll / 2;
	} else if (m_pll <= 315) {
		phy_ctrl->rg_pll_fbd_p = 150;
		phy_ctrl->rg_pll_fbd_s = m_pll - 2 * phy_ctrl->rg_pll_fbd_p;
		phy_ctrl->rg_pll_enswc = 1;
	} else {
		DPU_FB_ERR("phy m_pll not support!m_pll = %d\n", m_pll);
		return;
	}

	phy_ctrl->rg_pll_pre_p = n_pll;

	*lane_clock = m_pll * (DEFAULT_MIPI_CLK_RATE / n_pll) / vco_div;
	DPU_FB_DEBUG("Config : lane_clock = %llu\n", *lane_clock);

	phy_ctrl->rg_pll_cp = 1;  /* 0x16[7:5] */
	phy_ctrl->rg_pll_cp_p = 3;  /* 0x1E[7:5] */

	/* test_code_0x14 other parameters config */
	phy_ctrl->rg_pll_enbwt = 0;  /* 0x14[2] */
	phy_ctrl->rg_pll_chp = 0;  /* 0x14[1:0] */

	/* test_code_0x16 other parameters config,  0x16[3:2] reserved */
	phy_ctrl->rg_pll_lpf_cs = 0;  /* 0x16[4] */
	phy_ctrl->rg_pll_refsel = 1;  /* 0x16[1:0] */

	/* test_code_0x1E other parameters config */
	phy_ctrl->reload_sel = 1;  /* 0x1E[4] */
	phy_ctrl->rg_phase_gen_en = 1;  /* 0x1E[3] */
	phy_ctrl->pll_power_down = 0;  /* 0x1E[1] */
	phy_ctrl->pll_register_override = 1;  /* 0x1E[0] */
}

static void mipi_dsi_pll_phy_config(struct mipi_dsi_phy_ctrl *phy_ctrl, struct dpu_panel_info *pinfo,
	uint64_t *lane_clock)
{
	uint64_t vco_div = 1;  /* default clk division */
	uint32_t m_pll = 0;
	uint32_t n_pll = 0;

	if ((*lane_clock >= 320) && (*lane_clock <= 2500)) {
		phy_ctrl->rg_band_sel = 0;  /* 0x1E[2] */
		vco_div = 1;
	} else if ((*lane_clock >= 80) && (*lane_clock < 320)) {
		phy_ctrl->rg_band_sel = 1;
		vco_div = 4;
	} else {
		DPU_FB_ERR("80M <= lane_clock< = 2500M, not support lane_clock = %llu M\n", *lane_clock);
	}
	mipi_dsi_rg_pll_phy_config_prepare(pinfo, lane_clock, &m_pll, &n_pll, vco_div);
	set_rg_pll_phy_ctrl_data(phy_ctrl, m_pll, n_pll, vco_div, lane_clock);

	/* HSTX select VCM VREF */
	phy_ctrl->rg_vrefsel_vcm = 0x55;
	if (pinfo->mipi.rg_vrefsel_vcm_clk_adjust != 0)
		phy_ctrl->rg_vrefsel_vcm = (phy_ctrl->rg_vrefsel_vcm & 0x0F) |
			((pinfo->mipi.rg_vrefsel_vcm_clk_adjust & 0x0F) << 4);

	if (pinfo->mipi.rg_vrefsel_vcm_data_adjust != 0)
		phy_ctrl->rg_vrefsel_vcm = (phy_ctrl->rg_vrefsel_vcm & 0xF0) |
			(pinfo->mipi.rg_vrefsel_vcm_data_adjust & 0x0F);

	/* if reload_sel = 1, need to set load_command */
	phy_ctrl->load_command = 0x5A;
}

static void mipi_dsi_clk_data_lane_phy_config_prepare(struct dpu_panel_info *pinfo,
	uint64_t lane_clock, struct mipi_dsi_clk_data *clk_data)
{
	clk_data->accuracy = 10;  /* magnification */
	clk_data->ui =  10 * 1000000000UL * clk_data->accuracy / lane_clock;
	/* unit of measurement */
	clk_data->unit_tx_byte_clk_hs = 8 * clk_data->ui;

	/* D-PHY Specification : 60ns + 52*UI <= clk_post */
	clk_data->clk_post = 600 * clk_data->accuracy + 52 * clk_data->ui + pinfo->mipi.clk_post_adjust * clk_data->ui;

	/* D-PHY Specification : clk_pre >= 8*UI */
	clk_data->clk_pre = 8 * clk_data->ui + pinfo->mipi.clk_pre_adjust * clk_data->ui;

	/* D-PHY Specification : clk_t_hs_exit >= 100ns */
	clk_data->clk_t_hs_exit = 1000 * clk_data->accuracy + pinfo->mipi.clk_t_hs_exit_adjust * clk_data->ui;

	/* clocked by TXBYTECLKHS */
	clk_data->clk_pre_delay = 0 + pinfo->mipi.clk_pre_delay_adjust * clk_data->ui;

	/* D-PHY Specification : clk_t_hs_trial >= 60ns
	 * clocked by TXBYTECLKHS
	 */
	clk_data->clk_t_hs_trial = 600 * clk_data->accuracy + 3 * clk_data->unit_tx_byte_clk_hs +
		pinfo->mipi.clk_t_hs_trial_adjust * clk_data->ui;

	/* D-PHY Specification : 38ns <= clk_t_hs_prepare <= 95ns
	 * clocked by TXBYTECLKHS
	 */
	if (pinfo->mipi.clk_t_hs_prepare_adjust == 0)
		pinfo->mipi.clk_t_hs_prepare_adjust = 43;

	clk_data->clk_t_hs_prepare = ((380 * clk_data->accuracy + pinfo->mipi.clk_t_hs_prepare_adjust * clk_data->ui) <=
		(950 * clk_data->accuracy - 8 * clk_data->ui)) ?
		(380 * clk_data->accuracy + pinfo->mipi.clk_t_hs_prepare_adjust * clk_data->ui) :
		(950 * clk_data->accuracy - 8 * clk_data->ui);

	/* clocked by TXBYTECLKHS */
	clk_data->data_post_delay = 0 + pinfo->mipi.data_post_delay_adjust * clk_data->ui;

	/* D-PHY Specification : data_t_hs_trial >= max( n*8*UI, 60ns + n*4*UI ), n = 1
	 * clocked by TXBYTECLKHS
	 */
	clk_data->data_t_hs_trial = ((600 * clk_data->accuracy + 4 * clk_data->ui) >= (8 * clk_data->ui) ?
		(600 * clk_data->accuracy + 4 * clk_data->ui) : (8 * clk_data->ui)) + 8 * clk_data->ui +
		3 * clk_data->unit_tx_byte_clk_hs + pinfo->mipi.data_t_hs_trial_adjust * clk_data->ui;

	/* D-PHY Specification : 40ns + 4*UI <= data_t_hs_prepare <= 85ns + 6*UI
	 * clocked by TXBYTECLKHS
	 */
	if (pinfo->mipi.data_t_hs_prepare_adjust == 0)
		pinfo->mipi.data_t_hs_prepare_adjust = 35;

	clk_data->data_t_hs_prepare = ((400  * clk_data->accuracy + 4 * clk_data->ui +
		pinfo->mipi.data_t_hs_prepare_adjust * clk_data->ui) <= (850 * clk_data->accuracy +
		6 * clk_data->ui - 8 * clk_data->ui)) ?
		(400  * clk_data->accuracy + 4 * clk_data->ui + pinfo->mipi.data_t_hs_prepare_adjust * clk_data->ui) :
		(850 * clk_data->accuracy + 6 * clk_data->ui - 8 * clk_data->ui);

	/* D-PHY chip spec : clk_t_lpx + clk_t_hs_prepare > 200ns
	 * D-PHY Specification : clk_t_lpx >= 50ns
	 * clocked by TXBYTECLKHS
	 */
	clk_data->clk_t_lpx = (((2000 * clk_data->accuracy - clk_data->clk_t_hs_prepare) >= 500 * clk_data->accuracy) ?
		((2000 * clk_data->accuracy - clk_data->clk_t_hs_prepare)) : (500 * clk_data->accuracy)) +
		pinfo->mipi.clk_t_lpx_adjust * clk_data->ui;

	/* D-PHY Specification : clk_t_hs_zero + clk_t_hs_prepare >= 300 ns
	 * clocked by TXBYTECLKHS
	 */
	clk_data->clk_t_hs_zero = 3000 * clk_data->accuracy - clk_data->clk_t_hs_prepare +
	3 * clk_data->unit_tx_byte_clk_hs + pinfo->mipi.clk_t_hs_zero_adjust * clk_data->ui;

	/* D-PHY chip spec : data_t_lpx + data_t_hs_prepare > 200ns
	 * D-PHY Specification : data_t_lpx >= 50ns
	 * clocked by TXBYTECLKHS
	 * 2000 * accuracy - data_t_hs_prepare;
	 */
	clk_data->data_t_lpx = clk_data->clk_t_lpx + pinfo->mipi.data_t_lpx_adjust * clk_data->ui;

	/* D-PHY Specification : data_t_hs_zero + data_t_hs_prepare >= 145ns + 10*UI
	 * clocked by TXBYTECLKHS
	 */
	clk_data->data_t_hs_zero = 1450 * clk_data->accuracy + 10 * clk_data->ui - clk_data->data_t_hs_prepare +
		3 * clk_data->unit_tx_byte_clk_hs + pinfo->mipi.data_t_hs_zero_adjust * clk_data->ui;
}

static void set_phy_ctrl_data(struct mipi_dsi_phy_ctrl *phy_ctrl,
	struct dpu_panel_info *pinfo, struct mipi_dsi_clk_data *clk_data, uint64_t lane_clock)
{
	phy_ctrl->clk_pre_delay = round1(clk_data->clk_pre_delay, clk_data->unit_tx_byte_clk_hs);
	phy_ctrl->clk_t_hs_prepare = round1(clk_data->clk_t_hs_prepare, clk_data->unit_tx_byte_clk_hs);
	phy_ctrl->clk_t_lpx = round1(clk_data->clk_t_lpx, clk_data->unit_tx_byte_clk_hs);
	phy_ctrl->clk_t_hs_zero = round1(clk_data->clk_t_hs_zero, clk_data->unit_tx_byte_clk_hs);
	phy_ctrl->clk_t_hs_trial = round1(clk_data->clk_t_hs_trial, clk_data->unit_tx_byte_clk_hs);

	phy_ctrl->data_post_delay = round1(clk_data->data_post_delay, clk_data->unit_tx_byte_clk_hs);
	phy_ctrl->data_t_hs_prepare = round1(clk_data->data_t_hs_prepare, clk_data->unit_tx_byte_clk_hs);
	phy_ctrl->data_t_lpx = round1(clk_data->data_t_lpx, clk_data->unit_tx_byte_clk_hs);
	phy_ctrl->data_t_hs_zero = round1(clk_data->data_t_hs_zero, clk_data->unit_tx_byte_clk_hs);
	phy_ctrl->data_t_hs_trial = round1(clk_data->data_t_hs_trial, clk_data->unit_tx_byte_clk_hs);
	phy_ctrl->data_t_ta_go = 4;
	phy_ctrl->data_t_ta_get = 5;

	clk_data->clk_pre_delay_reality = phy_ctrl->clk_pre_delay + 2;
	clk_data->clk_t_hs_zero_reality = phy_ctrl->clk_t_hs_zero + 8;
	clk_data->data_t_hs_zero_reality = phy_ctrl->data_t_hs_zero + 4;
	clk_data->data_post_delay_reality = phy_ctrl->data_post_delay + 4;

	phy_ctrl->clk_post_delay = phy_ctrl->data_t_hs_trial + round1(
		clk_data->clk_post, clk_data->unit_tx_byte_clk_hs);
	/* if use 1080 X 2160 resolution panel,need reduce the lp11 time,and disable noncontinue mode */
	if (pinfo->mipi.lp11_flag  == MIPI_SHORT_LP11)
		phy_ctrl->data_pre_delay = 0;
	else
		phy_ctrl->data_pre_delay = clk_data->clk_pre_delay_reality + phy_ctrl->clk_t_lpx +
			phy_ctrl->clk_t_hs_prepare + clk_data->clk_t_hs_zero_reality +
			round1(clk_data->clk_pre, clk_data->unit_tx_byte_clk_hs);

	clk_data->clk_post_delay_reality = phy_ctrl->clk_post_delay + 4;

	/* if use 1080 X 2160 resolution panel,need reduce the lp11 time,and disable noncontinue mode */
	if (pinfo->mipi.lp11_flag == MIPI_SHORT_LP11)
		clk_data->data_pre_delay_reality = 0;
	else
		clk_data->data_pre_delay_reality = phy_ctrl->data_pre_delay + 2;

	phy_ctrl->clk_lane_lp2hs_time = clk_data->clk_pre_delay_reality + phy_ctrl->clk_t_lpx +
		phy_ctrl->clk_t_hs_prepare + clk_data->clk_t_hs_zero_reality + 3;
	phy_ctrl->clk_lane_hs2lp_time = clk_data->clk_post_delay_reality + phy_ctrl->clk_t_hs_trial + 3;
	phy_ctrl->data_lane_lp2hs_time = clk_data->data_pre_delay_reality + phy_ctrl->data_t_lpx +
		phy_ctrl->data_t_hs_prepare + clk_data->data_t_hs_zero_reality + 3;
	phy_ctrl->data_lane_hs2lp_time = clk_data->data_post_delay_reality + phy_ctrl->data_t_hs_trial + 3;

	/* if use 1080 X 2160 resolution panel,need reduce the lp11 time,and disable noncontinue mode */
	if (pinfo->mipi.lp11_flag == MIPI_SHORT_LP11)
		phy_ctrl->phy_stop_wait_time = phy_ctrl->clk_t_hs_trial +
			round1(clk_data->clk_t_hs_exit, clk_data->unit_tx_byte_clk_hs) -
			(clk_data->data_post_delay_reality + phy_ctrl->data_t_hs_trial);
	else
		phy_ctrl->phy_stop_wait_time = clk_data->clk_post_delay_reality +
			phy_ctrl->clk_t_hs_trial + round1(clk_data->clk_t_hs_exit, clk_data->unit_tx_byte_clk_hs) -
			(clk_data->data_post_delay_reality + phy_ctrl->data_t_hs_trial) + 3;

	phy_ctrl->lane_byte_clk = lane_clock / 8;
	phy_ctrl->clk_division = (((phy_ctrl->lane_byte_clk / 2) % pinfo->mipi.max_tx_esc_clk) > 0) ?
		(phy_ctrl->lane_byte_clk / 2 / pinfo->mipi.max_tx_esc_clk + 1) :
		(phy_ctrl->lane_byte_clk / 2 / pinfo->mipi.max_tx_esc_clk);
}

static void mipi_dsi_clk_data_lane_phy_config(struct mipi_dsi_phy_ctrl *phy_ctrl,
	struct dpu_panel_info *pinfo, uint64_t lane_clock)
{
	struct mipi_dsi_clk_data clk_data = {0};

	mipi_dsi_clk_data_lane_phy_config_prepare(pinfo, lane_clock, &clk_data);
	set_phy_ctrl_data(phy_ctrl, pinfo, &clk_data, lane_clock);
}

static void get_dsi_phy_ctrl(struct dpu_fb_data_type *dpufd,
	struct mipi_dsi_phy_ctrl *phy_ctrl)
{
	struct dpu_panel_info *pinfo = NULL;
	uint32_t dsi_bit_clk;
	uint64_t lane_clock;

	if ((!phy_ctrl) || (!dpufd))
		return;

	pinfo = &(dpufd->panel_info);

	dsi_bit_clk = pinfo->mipi.dsi_bit_clk_upt;
	lane_clock = 2 * (uint64_t)dsi_bit_clk;
	DPU_FB_DEBUG("Expected : lane_clock = %llu M\n", lane_clock);

	/************************  PLL parameters config  *********************/
	/* chip spec :
	 * If the output data rate is below 320 Mbps, RG_BNAD_SEL should be set to 1.
	 * At this mode a post divider of 1/4 will be applied to VCO.
	 */
	mipi_dsi_pll_phy_config(phy_ctrl, pinfo, &lane_clock);

	/********************  clock/data lane parameters config  ******************/
	mipi_dsi_clk_data_lane_phy_config(phy_ctrl, pinfo, lane_clock);

	DPU_FB_DEBUG("PHY clock_lane and data_lane config :\n"
		"rg_vrefsel_vcm=%u, clk_pre_delay=%u, clk_post_delay=%u\n"
		"clk_t_hs_prepare=%u, clk_t_lpx=%u, clk_t_hs_zero=%u\n"
		"clk_t_hs_trial=%u, data_pre_delay=%u, data_post_delay=%u\n"
		"data_t_hs_prepare=%u, data_t_lpx=%u, data_t_hs_zero=%u\n"
		"data_t_hs_trial=%u, data_t_ta_go=%u, data_t_ta_get=%u\n",
		phy_ctrl->rg_vrefsel_vcm, phy_ctrl->clk_pre_delay, phy_ctrl->clk_post_delay,
		phy_ctrl->clk_t_hs_prepare, phy_ctrl->clk_t_lpx, phy_ctrl->clk_t_hs_zero,
		phy_ctrl->clk_t_hs_trial, phy_ctrl->data_pre_delay, phy_ctrl->data_post_delay,
		phy_ctrl->data_t_hs_prepare, phy_ctrl->data_t_lpx, phy_ctrl->data_t_hs_zero,
		phy_ctrl->data_t_hs_trial, phy_ctrl->data_t_ta_go, phy_ctrl->data_t_ta_get);

	DPU_FB_DEBUG("clk_lane_lp2hs_time=%u, clk_lane_hs2lp_time=%u\n"
		"data_lane_lp2hs_time=%u, data_lane_hs2lp_time=%u\n"
		"phy_stop_wait_time=%u\n",
		phy_ctrl->clk_lane_lp2hs_time, phy_ctrl->clk_lane_hs2lp_time,
		phy_ctrl->data_lane_lp2hs_time, phy_ctrl->data_lane_hs2lp_time,
		phy_ctrl->phy_stop_wait_time);
}

static void get_dsi_phy_ctrl_clk_timing_config(struct mipi_dsi_clk_data *clk_data,
	struct mipi_dsi_phy_ctrl *phy_ctrl, struct dpu_panel_info *pinfo, uint64_t lane_clock)
{
	clk_data->clk_pre_delay_reality = phy_ctrl->clk_pre_delay + 2;
	clk_data->clk_t_hs_zero_reality = phy_ctrl->clk_t_hs_zero + 8;
	clk_data->data_t_hs_zero_reality = phy_ctrl->data_t_hs_zero + 4;
	clk_data->data_post_delay_reality = phy_ctrl->data_post_delay + 4;

	/* if use 1080 X 2160 resolution panel,need reduce the lp11 time,
	 * and disable noncontinue mode
	 */
	if (pinfo->mipi.lp11_flag == MIPI_SHORT_LP11) {
		phy_ctrl->data_pre_delay = 0;
	} else {
		phy_ctrl->data_pre_delay = clk_data->clk_pre_delay_reality +
			phy_ctrl->clk_t_lpx + phy_ctrl->clk_t_hs_prepare +
			clk_data->clk_t_hs_zero_reality + round1(clk_data->clk_pre, clk_data->unit_tx_byte_clk_hs);
	}

	clk_data->clk_post_delay_reality = phy_ctrl->clk_post_delay + 4;

	/* if use 1080 X 2160 resolution panel,
	 * need reduce the lp11 time,
	 * and disable noncontinue mode
	 */
	if (pinfo->mipi.lp11_flag == MIPI_SHORT_LP11)
		clk_data->data_pre_delay_reality = 0;
	else
		clk_data->data_pre_delay_reality = phy_ctrl->data_pre_delay + 2;

	phy_ctrl->clk_lane_lp2hs_time = clk_data->clk_pre_delay_reality +
		phy_ctrl->clk_t_lpx + phy_ctrl->clk_t_hs_prepare +
		clk_data->clk_t_hs_zero_reality + 3;

	phy_ctrl->clk_lane_hs2lp_time = clk_data->clk_post_delay_reality +
		phy_ctrl->clk_t_hs_trial + 3;

	phy_ctrl->data_lane_lp2hs_time = clk_data->data_pre_delay_reality +
		phy_ctrl->data_t_lpx + phy_ctrl->data_t_hs_prepare +
		clk_data->data_t_hs_zero_reality + 3;

	phy_ctrl->data_lane_hs2lp_time = clk_data->data_post_delay_reality +
		phy_ctrl->data_t_hs_trial + 3;

	/* if use 1080 X 2160 resolution panel,
	 * need reduce the lp11 time,
	 * and disable noncontinue mode
	 */
	if (pinfo->mipi.lp11_flag == MIPI_SHORT_LP11) {
		phy_ctrl->phy_stop_wait_time = phy_ctrl->clk_t_hs_trial +
			round1(clk_data->clk_t_hs_exit, clk_data->unit_tx_byte_clk_hs) -
			(clk_data->data_post_delay_reality + phy_ctrl->data_t_hs_trial);
	} else {
		phy_ctrl->phy_stop_wait_time = clk_data->clk_post_delay_reality +
			phy_ctrl->clk_t_hs_trial +
			round1(clk_data->clk_t_hs_exit, clk_data->unit_tx_byte_clk_hs) -
			(clk_data->data_post_delay_reality + phy_ctrl->data_t_hs_trial) + 3;
	}

	phy_ctrl->lane_byte_clk = lane_clock / 8;
	phy_ctrl->clk_division =
		(((phy_ctrl->lane_byte_clk / 2) % pinfo->mipi.max_tx_esc_clk) > 0) ?
		(phy_ctrl->lane_byte_clk / 2 / pinfo->mipi.max_tx_esc_clk + 1) :
		(phy_ctrl->lane_byte_clk / 2 / pinfo->mipi.max_tx_esc_clk);
}

static void get_dsi_phy_ctrl_v320(struct dpu_fb_data_type *dpufd,
	struct mipi_dsi_phy_ctrl *phy_ctrl, uint32_t mipi_dsi_bit_clk)
{
	struct dpu_panel_info *pinfo = &(dpufd->panel_info);
	uint32_t dsi_bit_clk;
	uint64_t lane_clock;
	struct mipi_dsi_clk_data clk_data = {0};

	dpu_check_and_no_retval(!pinfo, ERR, "pinfo is NULL\n");

	clk_data.accuracy = 10;  /* magnification */
	dsi_bit_clk = mipi_dsi_bit_clk;

	lane_clock = 2 * (uint64_t)dsi_bit_clk;
	DPU_FB_DEBUG("Expected : lane_clock = %llu M\n", lane_clock);
	dpu_check_and_no_retval((lane_clock == 0), ERR, "lan lock is 0.\n");
	/* PLL parameters config
	 * phy-tx v12 chip spec :
	 * If the output data rate is below 320 Mbps,
	 * RG_BNAD_SEL should be set to 1.
	 * At this mode a post divider of 1/4 will be applied to VCO.
	 */
	if (g_mipi_dphy_version == DPHY_VER_12)
		get_dsi_phy_ctrl_pll_para_v12(&lane_clock, phy_ctrl, pinfo);
	else
		get_dsi_phy_ctrl_pll_para_v14(&lane_clock, phy_ctrl, pinfo);

	/* clock/data lane parameters config */
	get_dsi_phy_ctrl_lane_para(lane_clock, phy_ctrl, pinfo, &clk_data.ui);

	/* D-PHY Specification : clk_pre >= 8*UI */
	clk_data.clk_pre = 8 * clk_data.ui + pinfo->mipi.clk_pre_adjust * clk_data.ui;

	/* D-PHY Specification : clk_t_hs_exit >= 100ns */
	clk_data.clk_t_hs_exit = 1000 * clk_data.accuracy + pinfo->mipi.clk_t_hs_exit_adjust * clk_data.ui;

	/* unit of measurement */
	clk_data.unit_tx_byte_clk_hs = 8 * clk_data.ui;
	dpu_check_and_no_retval((clk_data.unit_tx_byte_clk_hs == 0), ERR, "unit_tx_byte_clk_hs is 0.\n");

	/* clock/data timing config */
	get_dsi_phy_ctrl_clk_timing_config(&clk_data, phy_ctrl, pinfo, lane_clock);

	DPU_FB_DEBUG("PHY lane config: rg_vrefsel_vcm=%u,clk_pre_delay=%u\n"
		"clk_post_delay=%u, clk_t_hs_prepare=%u, clk_t_lpx=%u\n"
		"clk_t_hs_zero=%u clk_t_hs_trial=%u\n, data_pre_delay=%u\n"
		"data_post_delay=%u data_t_hs_prepare=%u, data_t_lpx=%u\n"
		"data_t_hs_zero=%u data_t_hs_trial=%u, data_t_ta_go=%u\n"
		"data_t_ta_get=%u\n",
		phy_ctrl->rg_vrefsel_vcm, phy_ctrl->clk_pre_delay,
		phy_ctrl->clk_post_delay, phy_ctrl->clk_t_hs_prepare,
		phy_ctrl->clk_t_lpx, phy_ctrl->clk_t_hs_zero,
		phy_ctrl->clk_t_hs_trial, phy_ctrl->data_pre_delay,
		phy_ctrl->data_post_delay, phy_ctrl->data_t_hs_prepare,
		phy_ctrl->data_t_lpx, phy_ctrl->data_t_hs_zero,
		phy_ctrl->data_t_hs_trial, phy_ctrl->data_t_ta_go,
		phy_ctrl->data_t_ta_get);

	DPU_FB_DEBUG("clk_lane_lp2hs_time=%u, clk_lane_hs2lp_time=%u\n"
		"data_lane_lp2hs_time=%u data_lane_hs2lp_time=%u\n"
		"phy_stop_wait_time=%u\n",
		phy_ctrl->clk_lane_lp2hs_time, phy_ctrl->clk_lane_hs2lp_time,
		phy_ctrl->data_lane_lp2hs_time, phy_ctrl->data_lane_hs2lp_time,
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

void mipi_config_phy_test_code(char __iomem *mipi_dsi_base,
	uint32_t test_code_addr, uint32_t test_code_parameter)
{
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL1_OFFSET, test_code_addr);
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL0_OFFSET, 0x00000002);
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL0_OFFSET, 0x00000000);
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL1_OFFSET, test_code_parameter);
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL0_OFFSET, 0x00000002);
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL0_OFFSET, 0x00000000);
}

static void mipi_config_dphy_spec_parameter(char __iomem *mipi_dsi_base,
	struct dpu_panel_info *pinfo)
{
	uint32_t i;
	uint32_t addr;

	/* pre_delay of clock lane request setting */
	mipi_config_phy_test_code(mipi_dsi_base,
		phy_addr(PHY_TST_CLK_PRE_DELAY_ENUM),
		dss_reduce(pinfo->dsi_phy_ctrl.clk_pre_delay));

	/* post_delay of clock lane request setting */
	mipi_config_phy_test_code(mipi_dsi_base,
		phy_addr(PHY_TST_CLK_POST_DELAY_ENUM),
		dss_reduce(pinfo->dsi_phy_ctrl.clk_post_delay));

	/* clock lane timing ctrl - t_lpx */
	mipi_config_phy_test_code(mipi_dsi_base,
		phy_addr(PHY_TST_CLK_TLPX_ENUM),
		dss_reduce(pinfo->dsi_phy_ctrl.clk_t_lpx));

	/* clock lane timing ctrl - t_hs_prepare */
	mipi_config_phy_test_code(mipi_dsi_base,
		phy_addr(PHY_TST_CLK_PREPARE_ENUM),
		dss_reduce(pinfo->dsi_phy_ctrl.clk_t_hs_prepare));

	/* clock lane timing ctrl - t_hs_zero */
	mipi_config_phy_test_code(mipi_dsi_base,
		phy_addr(PHY_TST_CLK_ZERO_ENUM),
		dss_reduce(pinfo->dsi_phy_ctrl.clk_t_hs_zero));

	/* clock lane timing ctrl - t_hs_trial */
	mipi_config_phy_test_code(mipi_dsi_base,
		phy_addr(PHY_TST_CLK_TRAIL_ENUM),
		pinfo->dsi_phy_ctrl.clk_t_hs_trial);

	for (i = 0; i <= pinfo->mipi.lane_nums; i++) {
		/* data lane pre_delay */

		addr = phy_addr(PHY_TST_DATA_PRE_DELAY_ENUM) + (i << 4);
		mipi_config_phy_test_code(mipi_dsi_base, addr, dss_reduce(pinfo->dsi_phy_ctrl.data_pre_delay));

		/* data lane post_delay */
		addr = phy_addr(PHY_TST_DATA_POST_DELAY_ENUM) + (i << 4);
		mipi_config_phy_test_code(mipi_dsi_base, addr, dss_reduce(pinfo->dsi_phy_ctrl.data_post_delay));

		/* data lane timing ctrl - t_lpx */
		addr = phy_addr(PHY_TST_DATA_TLPX_ENUM) + (i << 4);
		mipi_config_phy_test_code(mipi_dsi_base, addr, dss_reduce(pinfo->dsi_phy_ctrl.data_t_lpx));

		/* data lane timing ctrl - t_hs_prepare */
		addr = phy_addr(PHY_TST_DATA_PREPARE_ENUM) + (i << 4);
		mipi_config_phy_test_code(mipi_dsi_base, addr, dss_reduce(pinfo->dsi_phy_ctrl.data_t_hs_prepare));

		/* data lane timing ctrl - t_hs_zero */
		addr = phy_addr(PHY_TST_DATA_ZERO_ENUM) + (i << 4);
		mipi_config_phy_test_code(mipi_dsi_base, addr, dss_reduce(pinfo->dsi_phy_ctrl.data_t_hs_zero));

		/* data lane timing ctrl - t_hs_trial */
		addr = phy_addr(PHY_TST_DATA_TRAIL_ENUM) + (i << 4);
		mipi_config_phy_test_code(mipi_dsi_base, addr, pinfo->dsi_phy_ctrl.data_t_hs_trial);

		/* data lane timing ctrl - t_ta_go */
		addr = phy_addr(PHY_TST_DATA_TA_GO_ENUM) + (i << 4);
		mipi_config_phy_test_code(mipi_dsi_base, addr, dss_reduce(pinfo->dsi_phy_ctrl.data_t_ta_go));

		/* data lane timing ctrl - t_ta_get */
		addr = phy_addr(PHY_TST_DATA_TA_GET_ENUM) + (i << 4);
		mipi_config_phy_test_code(mipi_dsi_base, addr, dss_reduce(pinfo->dsi_phy_ctrl.data_t_ta_get));
	}
}

static void mipi_phy_init_config_v14(struct dpu_fb_data_type *dpufd, char __iomem *mipi_dsi_base)
{
	struct dpu_panel_info *pinfo = &(dpufd->panel_info);

	DPU_FB_INFO("mipi phy init v14\n");

	/* AFE LPTX Configuration II */
	mipi_config_phy_test_code(mipi_dsi_base, 0x0001006A, 0x55);

	/* PLL in D-PHY TX Configuration III */
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010065,
		(pinfo->dsi_phy_ctrl.rg_pll_posdiv << 4) | pinfo->dsi_phy_ctrl.rg_pll_prediv);

	/* PLL in D-PHY TX Configuration I */
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010063, pinfo->dsi_phy_ctrl.rg_pll_fbkdiv);

	/* AFE V Reference Configuration I */
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010067, 0x25);
	/* Change dphy mipi drive voltage VCM */
	if (pinfo->mipi.rg_vrefsel_vcm_adjust != 0) {
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010068, pinfo->mipi.rg_vrefsel_vcm_adjust);
		DPU_FB_INFO("rg_vrefsel_vcm = 0x%x\n", pinfo->mipi.rg_vrefsel_vcm_adjust);
	} else {
		/* AFE V Reference Configuration II */
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010068, 0x05);
	}
	/* MISC AFE Configuration */
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010069, 0x21);
}

static void mipi_phy_init_config(struct dpu_fb_data_type *dpufd,
	char __iomem *mipi_dsi_base, int step)
{
	struct dpu_panel_info *pinfo = NULL;

	pinfo = &(dpufd->panel_info);

	mipi_config_phy_test_code(mipi_dsi_base, 0x00010013, pinfo->dsi_phy_ctrl.rg_lptx_sri);

	if (step == 1) {
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010014,
			(1 << 4) + (pinfo->dsi_phy_ctrl.rg_pll_pre_div1p << 3) + 0x0);
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010016,
			(0x3 << 5) + (pinfo->dsi_phy_ctrl.rg_pll_lpf_cs << 4) + pinfo->dsi_phy_ctrl.rg_pll_refsel);
		DPU_FB_DEBUG("step1: 0x00010014=0x%x, 0x00010016=0x%x\n",
			(1 << 4) + (pinfo->dsi_phy_ctrl.rg_pll_pre_div1p << 3) + 0x0,
			(0x3 << 5) + (pinfo->dsi_phy_ctrl.rg_pll_lpf_cs << 4) + pinfo->dsi_phy_ctrl.rg_pll_refsel);
	} else if (step == 2) {
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010014,
			(1 << 4) + (pinfo->dsi_phy_ctrl.rg_pll_pre_div1p << 3) + 0x2);
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010016,
			(0x7 << 5) + (pinfo->dsi_phy_ctrl.rg_pll_lpf_cs << 4) + pinfo->dsi_phy_ctrl.rg_pll_refsel);
		DPU_FB_DEBUG("step2: 0x00010014=0x%x, 0x00010016=0x%x\n",
			(1 << 4) + (pinfo->dsi_phy_ctrl.rg_pll_pre_div1p << 3) + 0x2,
			(0x7 << 5) + (pinfo->dsi_phy_ctrl.rg_pll_lpf_cs << 4) + pinfo->dsi_phy_ctrl.rg_pll_refsel);
	} else {
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010014,
			(1 << 4) + (pinfo->dsi_phy_ctrl.rg_pll_pre_div1p << 3) + pinfo->dsi_phy_ctrl.rg_pll_chp);
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010016,
			(pinfo->dsi_phy_ctrl.rg_pll_cp << 5) + (pinfo->dsi_phy_ctrl.rg_pll_lpf_cs << 4) +
			pinfo->dsi_phy_ctrl.rg_pll_refsel);
		DPU_FB_DEBUG("normal: 0x00010014=0x%x, 0x00010016=0x%x\n",
			(1 << 4) + (pinfo->dsi_phy_ctrl.rg_pll_pre_div1p << 3) + pinfo->dsi_phy_ctrl.rg_pll_chp,
			(pinfo->dsi_phy_ctrl.rg_pll_cp << 5) + (pinfo->dsi_phy_ctrl.rg_pll_lpf_cs << 4) +
			pinfo->dsi_phy_ctrl.rg_pll_refsel);
	}

	/* physical configuration PLL II, M */
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010015, pinfo->dsi_phy_ctrl.rg_div);

	mipi_config_phy_test_code(mipi_dsi_base, 0x0001001C, pinfo->dsi_phy_ctrl.rg_vrefsel_lptx);

	/* sets the analog characteristic of V reference in D-PHY TX */
	mipi_config_phy_test_code(mipi_dsi_base, 0x0001001D, pinfo->dsi_phy_ctrl.rg_vrefsel_vcm);

	/* MISC AFE Configuration */
	mipi_config_phy_test_code(mipi_dsi_base, 0x0001001E, (3 << 5) + (pinfo->dsi_phy_ctrl.reload_sel << 4) +
		(pinfo->dsi_phy_ctrl.rg_phase_gen_en << 3) + (pinfo->dsi_phy_ctrl.rg_band_sel << 2) +
		(pinfo->dsi_phy_ctrl.pll_register_override << 1) + pinfo->dsi_phy_ctrl.pll_power_down);

	/* reload_command */
	mipi_config_phy_test_code(mipi_dsi_base, 0x0001001F, pinfo->dsi_phy_ctrl.load_command);

}

static void mipi_phy_pll_start_enhance_config_v14(
	struct dpu_fb_data_type *dpufd, char __iomem *mipi_dsi_base)
{
	uint32_t mipi_dsi_bit_clk;
	struct dpu_panel_info *pinfo = &(dpufd->panel_info);

	mipi_dsi_bit_clk = pinfo->mipi.dsi_bit_clk_upt;
	get_dsi_phy_ctrl_v320(dpufd, &(pinfo->dsi_phy_ctrl), mipi_dsi_bit_clk);
	mipi_phy_init_config_v14(dpufd, mipi_dsi_base);  /* normal rate */
	udelay(100);
	outp32(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x00000001);
}

static void mipi_phy_pll_start_enhance_config_v12(
	struct dpu_fb_data_type *dpufd, char __iomem *mipi_dsi_base)
{
	uint32_t mipi_dsi_bit_clk = 0;
	struct dpu_panel_info *pinfo = NULL;

	pinfo = &(dpufd->panel_info);

	mipi_dsi_bit_clk = 200;  /* default dsi_bit_clk */
	get_dsi_phy_ctrl_v320(dpufd, &(pinfo->dsi_phy_ctrl), mipi_dsi_bit_clk);
	mipi_phy_init_config(dpufd, mipi_dsi_base, 1);  /* 400M */
	set_reg(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x1, 1, 0);
	udelay(100);

	mipi_dsi_bit_clk = pinfo->mipi.dsi_bit_clk_upt;
	get_dsi_phy_ctrl_v320(dpufd, &(pinfo->dsi_phy_ctrl), mipi_dsi_bit_clk);
	mipi_phy_init_config(dpufd, mipi_dsi_base, 2);  /* normal rate */
	udelay(100);

	mipi_config_phy_test_code(mipi_dsi_base, 0x00010014,
		(1 << 4) + (pinfo->dsi_phy_ctrl.rg_pll_pre_div1p << 3) + pinfo->dsi_phy_ctrl.rg_pll_chp);
	mipi_config_phy_test_code(mipi_dsi_base, 0x00010016, (pinfo->dsi_phy_ctrl.rg_pll_cp << 5) +
		(pinfo->dsi_phy_ctrl.rg_pll_lpf_cs << 4) + pinfo->dsi_phy_ctrl.rg_pll_refsel);
	mipi_config_phy_test_code(mipi_dsi_base, 0x0001001F, pinfo->dsi_phy_ctrl.load_command);
}

static void mipi_phy_pll_start_enhance_config(
	struct dpu_fb_data_type *dpufd,
	char __iomem *mipi_dsi_base)
{
	if (g_mipi_dphy_version == DPHY_VER_14)
		mipi_phy_pll_start_enhance_config_v14(dpufd, mipi_dsi_base);
	else
		mipi_phy_pll_start_enhance_config_v12(dpufd, mipi_dsi_base);
}

static void mipi_phy_mask_ctrl(char __iomem *mipi_dsi_base, bool mask, uint8_t txoff_rxulps_en)
{
	if (!txoff_rxulps_en) {
		if (mask) {
			mipi_config_phy_test_code(mipi_dsi_base, phy_addr(PHY_TST_AFE_LPTX_CFG_I_ENUM), 0x00000020);
			mipi_config_phy_test_code(mipi_dsi_base, phy_addr(PHY_TST_AFE_LPTX_CFG_I_ENUM), 0x00000030);
		} else {
			mipi_config_phy_test_code(mipi_dsi_base, phy_addr(PHY_TST_AFE_LPTX_CFG_I_ENUM), 0x00000020);
			mipi_config_phy_test_code(mipi_dsi_base, phy_addr(PHY_TST_AFE_LPTX_CFG_I_ENUM), 0x00000000);
		}
	}
}

static void mipi_cdphy_init_config(struct dpu_fb_data_type *dpufd, struct dpu_panel_info *pinfo,
	char __iomem *mipi_dsi_base)
{
	if ((g_dss_version_tag & FB_ACCEL_DSSV320) && (g_fpga_flag == 0)) {
		mipi_phy_mask_ctrl(mipi_dsi_base, true, 0);

		mipi_phy_pll_start_enhance_config(dpufd, mipi_dsi_base);
		/* set dphy spec parameter */
		mipi_config_dphy_spec_parameter(mipi_dsi_base, pinfo);
	} else if (g_mipi_dphy_version == DPHY_VER_12) {
		/* physical configuration PLL I */
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010014, (pinfo->dsi_phy_ctrl.rg_pll_fbd_s << 4) +
			(pinfo->dsi_phy_ctrl.rg_pll_enswc << 3) + (pinfo->dsi_phy_ctrl.rg_pll_enbwt << 2) +
			pinfo->dsi_phy_ctrl.rg_pll_chp);

		/* physical configuration PLL II, M */
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010015, pinfo->dsi_phy_ctrl.rg_pll_fbd_p);

		/* physical configuration PLL III */
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010016, (pinfo->dsi_phy_ctrl.rg_pll_cp << 5) +
			(pinfo->dsi_phy_ctrl.rg_pll_lpf_cs << 4) + pinfo->dsi_phy_ctrl.rg_pll_refsel);

		/* physical configuration PLL IV, N */
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010017, pinfo->dsi_phy_ctrl.rg_pll_pre_p);

		/* sets the analog characteristic of V reference in D-PHY TX */
		mipi_config_phy_test_code(mipi_dsi_base, 0x0001001D, pinfo->dsi_phy_ctrl.rg_vrefsel_vcm);

		/* MISC AFE Configuration */
		mipi_config_phy_test_code(mipi_dsi_base, 0x0001001E, (pinfo->dsi_phy_ctrl.rg_pll_cp_p << 5) +
			(pinfo->dsi_phy_ctrl.reload_sel << 4) + (pinfo->dsi_phy_ctrl.rg_phase_gen_en << 3) +
			(pinfo->dsi_phy_ctrl.rg_band_sel << 2) + (pinfo->dsi_phy_ctrl.pll_power_down << 1) +
			pinfo->dsi_phy_ctrl.pll_register_override);

		/* reload_command */
		mipi_config_phy_test_code(mipi_dsi_base, 0x0001001F, pinfo->dsi_phy_ctrl.load_command);

		/* set dphy spec parameter */
		mipi_config_dphy_spec_parameter(mipi_dsi_base, pinfo);
	}
}

static bool mipi_phy_status_check(const char __iomem *mipi_dsi_base, uint32_t expected_value)
{
	bool is_ready = false;
	unsigned long dw_jiffies;
	uint32_t temp = 0;

	dw_jiffies = jiffies + HZ / 2;
	do {
		temp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		if ((temp & expected_value) == expected_value) {
			is_ready = true;
			break;
		}
	} while (time_after(dw_jiffies, jiffies));

	DPU_FB_DEBUG("MIPIDSI_PHY_STATUS_OFFSET=0x%x.\n", temp);

	return is_ready;
}

static uint32_t mipi_get_cmp_stopstate_value(struct dpu_panel_info *pinfo)
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

static void mipi_dsi_phy_config(struct dpu_fb_data_type *dpufd, struct dpu_panel_info *pinfo,
	char __iomem *mipi_dsi_base)
{
	uint32_t mipi_dsi_bit_clk = 0;
	bool is_ready = false;

	if ((g_dss_version_tag & FB_ACCEL_DSSV320) && (g_fpga_flag == 0)) {
		mipi_dsi_bit_clk = pinfo->mipi.dsi_bit_clk_upt;
		get_dsi_phy_ctrl_v320(dpufd, &(pinfo->dsi_phy_ctrl), mipi_dsi_bit_clk);
	} else {
		get_dsi_phy_ctrl(dpufd, &(pinfo->dsi_phy_ctrl));
	}

	if (dpufd->panel_info.mipi.txoff_rxulps_en)
		set_reg(mipi_dsi_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x0, 2, 0);

	set_reg(mipi_dsi_base + MIPIDSI_PHY_IF_CFG_OFFSET, pinfo->mipi.lane_nums, 2, 0);
	set_reg(mipi_dsi_base + MIPIDSI_CLKMGR_CFG_OFFSET, pinfo->dsi_phy_ctrl.clk_division, 8, 0);
	set_reg(mipi_dsi_base + MIPIDSI_CLKMGR_CFG_OFFSET, pinfo->dsi_phy_ctrl.clk_division, 8, 8);
	outp32(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x00000000);
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL0_OFFSET, 0x00000000);
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL0_OFFSET, 0x00000001);
	outp32(mipi_dsi_base + MIPIDSI_PHY_TST_CTRL0_OFFSET, 0x00000000);

	mipi_cdphy_init_config(dpufd, pinfo, mipi_dsi_base);
	outp32(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x0000000F);

	is_ready = mipi_phy_status_check(mipi_dsi_base, 0x00000001);
	if (!is_ready)
		DPU_FB_INFO("fb%d, phylock is not ready!\n", dpufd->index);

	is_ready = mipi_phy_status_check(mipi_dsi_base, mipi_get_cmp_stopstate_value(pinfo));
	if (!is_ready)
		DPU_FB_INFO("fb%d, phystopstateclklane is not ready!\n", dpufd->index);
}

static void mipi_dsi_config_dpi_interface(struct dpu_panel_info *pinfo, char __iomem *mipi_dsi_base)
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
	/* video mode: low power mode */
	if (pinfo->mipi.lp11_flag == MIPI_DISABLE_LP11) {
		set_reg(mipi_dsi_base + MIPIDSI_VID_MODE_CFG_OFFSET, 0x0f, 6, 8);
		DPU_FB_INFO("set_reg MIPIDSI_VID_MODE_CFG_OFFSET 0x0f\n");
	} else {
		set_reg(mipi_dsi_base + MIPIDSI_VID_MODE_CFG_OFFSET, 0x3f, 6, 8);
		DPU_FB_INFO("set_reg MIPIDSI_VID_MODE_CFG_OFFSET 0x3f\n");
	}

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
	/* for dsi read, BTA enable */
	set_reg(mipi_dsi_base + MIPIDSI_PCKHDL_CFG_OFFSET, 0x1, 1, 2);
}

static void mipi_dsi_horizontal_timing_config(struct dpu_fb_data_type *dpufd,
	char __iomem *mipi_dsi_base, struct dpu_panel_info *pinfo, dss_rect_t rect)
{
	uint32_t hline_time;
	uint32_t hsa_time;
	uint32_t hbp_time;
	uint64_t pixel_clk;

	pixel_clk = mipi_pixel_clk(dpufd);
	DPU_FB_INFO("pixel_clk = %llu\n", pixel_clk);

	if (pinfo->mipi.phy_mode == DPHY_MODE) {
		hsa_time = pinfo->ldi.h_pulse_width * pinfo->dsi_phy_ctrl.lane_byte_clk / pixel_clk;
		hbp_time = pinfo->ldi.h_back_porch * pinfo->dsi_phy_ctrl.lane_byte_clk / pixel_clk;
		hline_time = (pinfo->ldi.h_pulse_width + pinfo->ldi.h_back_porch +
			rect.w + pinfo->ldi.h_front_porch) * pinfo->dsi_phy_ctrl.lane_byte_clk / pixel_clk;
	} else {
		hsa_time = pinfo->ldi.h_pulse_width * pinfo->dsi_phy_ctrl.lane_word_clk / pixel_clk;
		hbp_time = pinfo->ldi.h_back_porch * pinfo->dsi_phy_ctrl.lane_word_clk / pixel_clk;
		hline_time = (pinfo->ldi.h_pulse_width + pinfo->ldi.h_back_porch +
			rect.w + pinfo->ldi.h_front_porch) * pinfo->dsi_phy_ctrl.lane_word_clk / pixel_clk;
	}
	DPU_FB_INFO("hsa_time = %d, hbp_time = %d, hline_time = %d\n", hsa_time, hbp_time, hline_time);

	set_reg(mipi_dsi_base + MIPIDSI_VID_HSA_TIME_OFFSET, hsa_time, 12, 0);
	set_reg(mipi_dsi_base + MIPIDSI_VID_HBP_TIME_OFFSET, hbp_time, 12, 0);
	set_reg(mipi_dsi_base + MIPIDSI_VID_HLINE_TIME_OFFSET, hline_time, 15, 0);
}

static void mipi_dsi_vfp_vsync_config(struct dpu_panel_info *pinfo,
	char __iomem *mipi_dsi_base, dss_rect_t rect)
{
	/* Define the Vertical line configuration */
	set_reg(mipi_dsi_base + MIPIDSI_VID_VSA_LINES_OFFSET, pinfo->ldi.v_pulse_width, 10, 0);
	set_reg(mipi_dsi_base + MIPIDSI_VID_VBP_LINES_OFFSET, pinfo->ldi.v_back_porch, 10, 0);
	set_reg(mipi_dsi_base + MIPIDSI_VID_VFP_LINES_OFFSET, pinfo->ldi.v_front_porch, 10, 0);
	set_reg(mipi_dsi_base + MIPIDSI_VID_VACTIVE_LINES_OFFSET, rect.h, 14, 0);
	set_reg(mipi_dsi_base + MIPIDSI_TO_CNT_CFG_OFFSET, 0x7FF, 16, 0);
}

static void mipi_dsi_phy_timing_config(struct dpu_panel_info *pinfo,
	char __iomem *mipi_dsi_base)
{
	/* Configure core's phy parameters */
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_LPCLK_CFG_OFFSET, pinfo->dsi_phy_ctrl.clk_lane_lp2hs_time, 10, 0);
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_LPCLK_CFG_OFFSET, pinfo->dsi_phy_ctrl.clk_lane_hs2lp_time, 10, 16);
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_RD_CFG_OFFSET, 0x7FFF, 15, 0);
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_CFG_OFFSET, pinfo->dsi_phy_ctrl.data_lane_lp2hs_time, 10, 0);
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_CFG_OFFSET, pinfo->dsi_phy_ctrl.data_lane_hs2lp_time, 10, 16);
}

void mipi_init(struct dpu_fb_data_type *dpufd, char __iomem *mipi_dsi_base)
{
	struct dpu_panel_info *pinfo = NULL;
	dss_rect_t rect;

	pinfo = &(dpufd->panel_info);

	if (pinfo->mipi.max_tx_esc_clk == 0) {
		DPU_FB_ERR("fb%d, max_tx_esc_clk is invalid!", dpufd->index);
		pinfo->mipi.max_tx_esc_clk = DEFAULT_MAX_TX_ESC_CLK;
	}

	memset(&(pinfo->dsi_phy_ctrl), 0, sizeof(pinfo->dsi_phy_ctrl));

	rect.x = 0;
	rect.y = 0;
	rect.w = pinfo->xres;
	rect.h = pinfo->yres;

	mipi_ifbc_get_rect(dpufd, &rect);

	mipi_dsi_phy_config(dpufd, pinfo, mipi_dsi_base);

	if (is_mipi_cmd_panel(dpufd)) {
		/* config to command mode */
		set_reg(mipi_dsi_base + MIPIDSI_MODE_CFG_OFFSET, 0x1, 1, 0);
		/* ALLOWED_CMD_SIZE */
		set_reg(mipi_dsi_base + MIPIDSI_EDPI_CMD_SIZE_OFFSET, rect.w, 16, 0);

		/* cnt=2 in update-patial scene, cnt nees to be checked for different panels */
		if (pinfo->mipi.hs_wr_to_time == 0)
			set_reg(mipi_dsi_base + MIPIDSI_HS_WR_TO_CNT_OFFSET, 0x1000002, 25, 0);
		else
			set_reg(mipi_dsi_base + MIPIDSI_HS_WR_TO_CNT_OFFSET,
				(0x1 << 24) | (pinfo->mipi.hs_wr_to_time *
				pinfo->dsi_phy_ctrl.lane_byte_clk / 1000000000UL), 25, 0);
	}

	/* phy_stop_wait_time */
	set_reg(mipi_dsi_base + MIPIDSI_PHY_IF_CFG_OFFSET, pinfo->dsi_phy_ctrl.phy_stop_wait_time, 8, 8);

	/* configuring the DPI packet transmission
	 * 2. Configure the DPI Interface:
	 * This defines how the DPI interface interacts with the controller.
	 */
	mipi_dsi_config_dpi_interface(pinfo, mipi_dsi_base);

	/*
	 * 3. Select the Video Transmission Mode:
	 * This defines how the processor requires the video line to be
	 * transported through the DSI link.
	 */
	mipi_dsi_video_mode_config(dpufd, mipi_dsi_base, pinfo, rect);

	/*
	 * 4. Define the DPI Horizontal timing configuration:
	 * Hsa_time = HSA*(PCLK period/Clk Lane Byte Period);
	 * Hbp_time = HBP*(PCLK period/Clk Lane Byte Period);
	 * Hline_time = (HSA+HBP+HACT+HFP)*(PCLK period/Clk Lane Byte Period);
	 */
	mipi_dsi_horizontal_timing_config(dpufd, mipi_dsi_base, pinfo, rect);

	mipi_dsi_vfp_vsync_config(pinfo, mipi_dsi_base, rect);

	mipi_dsi_phy_timing_config(pinfo, mipi_dsi_base);

	/* Waking up Core */
	set_reg(mipi_dsi_base + MIPIDSI_PWR_UP_OFFSET, 0x1, 1, 0);
}

static int dsi_clk_enable_prepare(struct dpu_fb_data_type *dpufd, struct clk *dss_clk, const char *msg)
{
	int ret;

	if (dss_clk) {
		ret = clk_prepare(dss_clk);
		if (ret != 0) {
			DPU_FB_ERR("fb%d %s clk_prepare failed, error=%d!\n", dpufd->index, msg, ret);
			return -EINVAL;
		}


		ret = clk_enable(dss_clk);
		if (ret != 0) {
			DPU_FB_ERR("fb%d %s clk_enable failed, error=%d!\n", dpufd->index, msg, ret);
			return -EINVAL;
		}
	}

	return 0;
}

int mipi_dsi_clk_enable(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL!\n");

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		if (dsi_clk_enable_prepare(dpufd, dpufd->dss_dphy0_ref_clk, "dss_dphy0_ref_clk") < 0)
			return -EINVAL;
		if (dsi_clk_enable_prepare(dpufd, dpufd->dss_dphy0_cfg_clk, "dss_dphy0_cfg_clk") < 0)
			return -EINVAL;
		if (dsi_clk_enable_prepare(dpufd, dpufd->dss_pclk_dsi0_clk, "dss_pclk_dsi0_clk") < 0)
			return -EINVAL;
	}

#ifdef CONFIG_PCLK_PCTRL_USED
	if (dsi_clk_enable_prepare(dpufd, dpufd->dss_pclk_pctrl_clk, "dss_pclk_pctrl_clk") < 0)
		return -EINVAL;
#endif

	if (is_dual_mipi_panel(dpufd) || (dpufd->index == EXTERNAL_PANEL_IDX)) {
		if (dsi_clk_enable_prepare(dpufd, dpufd->dss_dphy1_ref_clk, "dss_dphy1_ref_clk") < 0)
			return -EINVAL;
		if (dsi_clk_enable_prepare(dpufd, dpufd->dss_dphy1_cfg_clk, "dss_dphy1_cfg_clk") < 0)
			return -EINVAL;
		if (dsi_clk_enable_prepare(dpufd, dpufd->dss_pclk_dsi1_clk, "dss_pclk_dsi1_clk") < 0)
			return -EINVAL;
	}

	return 0;
}

static inline void dsi_clk_disable_unprepare(struct clk *dss_clk)
{
	if (dss_clk) {
		clk_disable(dss_clk);
		clk_unprepare(dss_clk);
	}
}

int mipi_dsi_clk_disable(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		dsi_clk_disable_unprepare(dpufd->dss_dphy0_ref_clk);
		dsi_clk_disable_unprepare(dpufd->dss_dphy0_cfg_clk);
		dsi_clk_disable_unprepare(dpufd->dss_pclk_dsi0_clk);
	}

#ifdef CONFIG_PCLK_PCTRL_USED
	dsi_clk_disable_unprepare(dpufd->dss_pclk_pctrl_clk);
#endif

	if (is_dual_mipi_panel(dpufd) || (dpufd->index == EXTERNAL_PANEL_IDX)) {
		dsi_clk_disable_unprepare(dpufd->dss_dphy1_ref_clk);
		dsi_clk_disable_unprepare(dpufd->dss_dphy1_cfg_clk);
		dsi_clk_disable_unprepare(dpufd->dss_pclk_dsi1_clk);
	}

	return 0;
}

static void mipi_dsi_get_ulps_stopstate(struct dpu_fb_data_type *dpufd,
	uint32_t *cmp_ulpsactivenot_val, uint32_t *cmp_stopstate_val, bool enter_ulps)
{
	if (enter_ulps) {  /* ulps enter */
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
	} else {  /* ulps exit */
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
	}
}

static int check_dphy_data_and_clock_stopstate(struct dpu_fb_data_type *dpufd,
	char __iomem *mipi_dsi_base, uint32_t cmp_stopstate_val)
{
	uint32_t try_times;
	uint32_t temp;

	/* check DPHY data and clock lane stopstate */
	try_times = 0;
	temp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	while ((temp & cmp_stopstate_val) != cmp_stopstate_val) {
		udelay(40);
		if (++try_times > 100) {  /* try 1ms */
			DPU_FB_INFO("fb%d, check1, check phy data clk lane stop state fail, PHY_STATUS=0x%x!\n",
				dpufd->index, temp);

			set_reg(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0x0, 32, 0);
			set_reg(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x0, 1, 0);
			udelay(5);
			mipi_phy_pll_start_enhance_config(dpufd, mipi_dsi_base);
			set_reg(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x1, 1, 0);
			return -1;
		}

		temp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	}

	return 0;
}

static int check_dphy_data_lane_ulpsactivenot_status(struct dpu_fb_data_type *dpufd,
	char __iomem *mipi_dsi_base, uint32_t cmp_ulpsactivenot_val)
{
	uint32_t try_times;
	uint32_t temp;

	/* check DPHY data lane ulpsactivenot_status */
	try_times = 0;
	temp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	while ((temp & cmp_ulpsactivenot_val) != 0) {
		if (++try_times > 4) {  /* try 20us */
			DPU_FB_INFO("fb%d, check2, request data lane enter ulps fail, PHY_STATUS=0x%x!\n",
				dpufd->index, temp);

			set_reg(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0x0, 32, 0);
			set_reg(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x0, 2, 0);
			set_reg(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x3, 2, 0);
			return -1;
		}

		udelay(5);
		temp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	}

	return 0;
}

static int check_dphy_data_clock_lane_exit_ulps(struct dpu_fb_data_type *dpufd,
	char __iomem *mipi_dsi_base, uint32_t cmp_ulpsactivenot_val)
{
	uint32_t try_times;
	uint32_t temp;

	try_times = 0;
	temp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	while ((temp & cmp_ulpsactivenot_val) != cmp_ulpsactivenot_val) {
		udelay(10);
		if (++try_times > 3) {  /* try 15us */
			DPU_FB_INFO("fb%d, check3, request data clock lane exit ulps fail, PHY_STATUS=0x%x!\n",
				dpufd->index, temp);

			set_reg(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0x0, 32, 0);
			set_reg(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x0, 1, 0);
			udelay(5);
			mipi_phy_pll_start_enhance_config(dpufd, mipi_dsi_base);
			set_reg(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x1, 1, 0);
			return -1;
		}

		temp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	}

	return 0;
}

static int check_dphy_data_lane_cmp_stopstate_val(struct dpu_fb_data_type *dpufd,
	char __iomem *mipi_dsi_base, uint32_t cmp_stopstate_val)
{
	uint32_t try_times;
	uint32_t temp;

	try_times = 0;
	temp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	while ((temp & cmp_stopstate_val) != cmp_stopstate_val) {
		udelay(10);
		if (++try_times > 3) {  /* try 30us */
			DPU_FB_INFO("fb%d, check4, wait data clock lane stop state fail, PHY_STATUS=0x%x!\n",
				dpufd->index, temp);

			set_reg(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0x0, 32, 0);
			set_reg(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x0, 1, 0);
			udelay(5);
			mipi_phy_pll_start_enhance_config(dpufd, mipi_dsi_base);
			set_reg(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0x1, 1, 0);
			return -1;
		}

		temp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	}

	return 0;
}

static int mipi_dsi_pll_status_check_ec(struct dpu_fb_data_type *dpufd,
	char __iomem *mipi_dsi_base)
{
	uint32_t cmp_ulpsactivenot_val = 0;
	uint32_t cmp_stopstate_val = 0;
	timeval_compatible tv0;
	timeval_compatible tv1;
	uint32_t redo_count = 0;

	dpu_check_and_return((!mipi_dsi_base), 0, ERR, "mipi_dsi_base is null.\n");

	DPU_FB_DEBUG("fb%d, +!\n", dpufd->index);

	dpufb_get_timestamp(&tv0);

	/* enter ulps */
	mipi_dsi_get_ulps_stopstate(dpufd, &cmp_ulpsactivenot_val, &cmp_stopstate_val, true);

REDO:
	dpu_check_and_return((redo_count > 100), 0, ERR, "mipi phy pll retry fail, phy is not ready.\n");
	redo_count++;

	if (check_dphy_data_and_clock_stopstate(dpufd, mipi_dsi_base, cmp_stopstate_val) < 0)
		goto REDO;

	/* request that data lane enter ULPS */
	set_reg(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0x4, 4, 0);

	if (check_dphy_data_lane_ulpsactivenot_status(dpufd, mipi_dsi_base, cmp_ulpsactivenot_val) < 0)
		goto REDO;

	/* enable DPHY PLL, force_pll = 1 */
	outp32(mipi_dsi_base + MIPIDSI_PHY_RSTZ_OFFSET, 0xF);

	/* exit ulps
	 * request that data lane  exit ULPS
	 */
	outp32(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0xC);

	if (check_dphy_data_clock_lane_exit_ulps(dpufd, mipi_dsi_base, cmp_ulpsactivenot_val) < 0)
		goto REDO;

	/* clear PHY_ULPS_CTRL */
	outp32(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0x0);

	if (check_dphy_data_lane_cmp_stopstate_val(dpufd, mipi_dsi_base, cmp_stopstate_val) < 0)
		goto REDO;

	mipi_phy_mask_ctrl(mipi_dsi_base, false, dpufd->panel_info.mipi.txoff_rxulps_en);

	dpufb_get_timestamp(&tv1);

	DPU_FB_INFO("fb%d, wait data clock lane stop state SUCC, redo_count=%d, cost time %u us!\n",
		dpufd->index, redo_count, dpufb_timestamp_diff(&tv0, &tv1));

	DPU_FB_DEBUG("fb%d, -!\n", dpufd->index);

	return 0;
}

static int mipi_dsi_on_sub1(struct dpu_fb_data_type *dpufd, char __iomem *mipi_dsi_base)
{
	dpu_check_and_return((!mipi_dsi_base), 0, ERR, "mipi_dsi_base is null.\n");
	/* mipi init */
	mipi_init(dpufd, mipi_dsi_base);

	/* dsi memory init */
	if (g_dss_version_tag == FB_ACCEL_DPUV410)
		outp32(mipi_dsi_base + DSI_MEM_CTRL, 0x02600008);

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
	if (is_mipi_video_panel(dpufd)) {
		pctrl_dphytx_stopcnt = (uint64_t)(pinfo->ldi.h_back_porch +
			pinfo->ldi.h_front_porch + pinfo->ldi.h_pulse_width + pinfo->xres / stopcnt_div + 5) *
			dpufd->dss_vote_cmd.dss_pclk_pctrl_rate / (pinfo->pxl_clk_rate / stopcnt_div);
	} else {
		pctrl_dphytx_stopcnt = (uint64_t)(pinfo->ldi.h_back_porch +
			pinfo->ldi.h_front_porch + pinfo->ldi.h_pulse_width + 5) *
			dpufd->dss_vote_cmd.dss_pclk_pctrl_rate / (pinfo->pxl_clk_rate / stopcnt_div);
	}

	outp32(dpufd->pctrl_base + PERI_CTRL29, (uint32_t)pctrl_dphytx_stopcnt);
	if (is_dual_mipi_panel(dpufd))
		outp32(dpufd->pctrl_base + PERI_CTRL32, (uint32_t)pctrl_dphytx_stopcnt);
}

static int mipi_dsi_on_sub2(struct dpu_fb_data_type *dpufd, char __iomem *mipi_dsi_base)
{
	struct dpu_panel_info *pinfo = NULL;

	dpu_check_and_return((!mipi_dsi_base), 0, ERR, "mipi_dsi_base is null.\n");
	pinfo = &(dpufd->panel_info);

	if (is_mipi_video_panel(dpufd))
		/* switch to video mode */
		set_reg(mipi_dsi_base + MIPIDSI_MODE_CFG_OFFSET, 0x0, 1, 0);

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
	dpu_check_and_return((!mipi_dsi_base), -EINVAL, ERR, "mipi_dsi_base is null.\n");

	/* switch to cmd mode */
	set_reg(mipi_dsi_base + MIPIDSI_MODE_CFG_OFFSET, 0x1, 1, 0);
	/* cmd mode: low power mode */
	set_reg(mipi_dsi_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x7f, 7, 8);
	set_reg(mipi_dsi_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0xf, 4, 16);
	set_reg(mipi_dsi_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x1, 1, 24);

	/* disable generate High Speed clock */
	set_reg(mipi_dsi_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x0, 1, 0);

	/* shutdown d_phy */
	set_reg(mipi_dsi_base +  MIPIDSI_PHY_RSTZ_OFFSET, 0x0, 3, 0);

	return 0;
}

static int mipi_dsi_check_ulps_stopstate(struct dpu_fb_data_type *dpufd, const char __iomem *mipi_dsi_base,
	uint32_t cmp_stopstate_val, bool enter_ulps)
{
	uint32_t try_times;
	uint32_t temp;

	if (enter_ulps) {
		/* check DPHY data and clock lane stopstate */
		try_times = 0;
		temp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		while ((temp & cmp_stopstate_val) != cmp_stopstate_val) {
			udelay(40);
			if (++try_times > 100) {  /* try 4ms */
				DPU_FB_ERR("fb%d, check phy data and clk lane stop state failed! PHY_STATUS=0x%x.\n",
					dpufd->index, temp);
				return -1;
			}

			temp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		}
	} else {
		/* check DPHY data lane cmp_stopstate_val */
		try_times = 0;
		temp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		while ((temp & cmp_stopstate_val) != cmp_stopstate_val) {
			udelay(10);
			if (++try_times > 400) {  /* ulps exit try_time must be greater than 400 */
				DPU_FB_ERR("fb%d, check phy data clk lane stop state failed! PHY_STATUS=0x%x.\n",
					dpufd->index, temp);
				break;
			}

			temp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		}
	}

	return 0;
}

static void mipi_dsi_data_clock_lane_enter_ulps(struct dpu_fb_data_type *dpufd,
	char __iomem *mipi_dsi_base, uint32_t cmp_ulpsactivenot_val)
{
	uint32_t try_times;
	uint32_t temp;

	/* request that data lane enter ULPS */
	set_reg(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0x4, 4, 0);

	/* check DPHY data lane ulpsactivenot_status */
	try_times = 0;
	temp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	while ((temp & cmp_ulpsactivenot_val) != 0) {
		udelay(40);
		if (++try_times > 100) {  /* try 4ms */
			DPU_FB_ERR("fb%d, request phy data lane enter ulps failed! PHY_STATUS=0x%x.\n",
				dpufd->index, temp);
			break;
		}

		temp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	}

	/* request that clock lane enter ULPS */
	if (dpufd->panel_info.mipi.phy_mode == DPHY_MODE) {
		set_reg(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0x5, 4, 0);

		/* check DPHY clock lane ulpsactivenot_status */
		try_times = 0;
		temp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		while ((temp & BIT(3)) != 0) {
			udelay(40);
			if (++try_times > 100) {  /* try 4ms */
				DPU_FB_ERR("fb%d, request phy clk lane enter ulps failed! PHY_STATUS=0x%x.\n",
					dpufd->index, temp);
				break;
			}

			temp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		}
	}
}


static int mipi_dsi_ulps_enter(struct dpu_fb_data_type *dpufd,
	char __iomem *mipi_dsi_base)
{
	uint32_t temp;
	uint32_t cmp_ulpsactivenot_val = 0;
	uint32_t cmp_stopstate_val = 0;

	dpu_check_and_return(!mipi_dsi_base, -EINVAL, ERR, "mipi_dsi_base is null.\n");

	DPU_FB_DEBUG("fb%d, +!\n", dpufd->index);

	mipi_dsi_get_ulps_stopstate(dpufd, &cmp_ulpsactivenot_val, &cmp_stopstate_val, true);

	temp = (uint32_t)inp32(mipi_dsi_base + MIPIDSI_LPCLK_CTRL_OFFSET) & BIT(1);
	if (temp && (dpufd->panel_info.mipi.phy_mode == DPHY_MODE))
		cmp_stopstate_val |= (BIT(2));

	if (mipi_dsi_check_ulps_stopstate(dpufd, mipi_dsi_base, cmp_stopstate_val, true) < 0)
		return 0;

	/* disable DPHY clock lane's Hight Speed Clock */
	set_reg(mipi_dsi_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x0, 1, 0);

	mipi_dsi_data_clock_lane_enter_ulps(dpufd, mipi_dsi_base, cmp_ulpsactivenot_val);

	/* bit13 lock sel enable (dual_mipi_panel bit29 set 1) ,colse clock gate */
	set_reg(dpufd->pctrl_base + PERI_CTRL33, 0x1, 1, 13);
	if (is_dual_mipi_panel(dpufd))
		set_reg(dpufd->pctrl_base + PERI_CTRL30, 0x1, 1, 29);

	if (is_dual_mipi_panel(dpufd))
		set_reg(dpufd->peri_crg_base + PERDIS3, 0xf, 4, 28);
	else
		set_reg(dpufd->peri_crg_base + PERDIS3, 0x3, 4, 28);

	DPU_FB_DEBUG("fb%d, -!\n", dpufd->index);

	return 0;
}

static void mipi_dsi_get_ulps_stopstate_for_video(struct dpu_fb_data_type *dpufd,
	uint32_t *cmp_ulpsactivenot_val, uint32_t *cmp_stopstate_val)
{
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
}

static int mipi_dsi_ulps_exit_for_video(struct dpu_fb_data_type *dpufd,
	char __iomem *mipi_dsi_base)
{
	uint32_t temp;
	uint32_t cmp_ulpsactivenot_val = 0;
	uint32_t cmp_stopstate_val = 0;
	uint32_t try_times;

	dpu_check_and_return(!mipi_dsi_base, -EINVAL, ERR, "mipi_dsi_base is NULL\n");

	DPU_FB_DEBUG("fb%d, +!\n", dpufd->index);

	mipi_dsi_get_ulps_stopstate_for_video(dpufd, &cmp_ulpsactivenot_val, &cmp_stopstate_val);

	/* bit28, bit29, bit30, bit31 set 0x3
	 * txdphy0_cfg, txdphy0_ref clock enable, txdphy1_cfg, txdphy1_ref clock no effect
	 */
	set_reg(dpufd->peri_crg_base + PEREN3, 0x3, 4, 28);

	udelay(100);  /* wait pll clk */
	/* bit13 lock sel enable (dual_mipi_panel bit29 set 1) ,colse clock gate */
	set_reg(dpufd->pctrl_base + PERI_CTRL33, 0x0, 1, 13);

	/* request that data lane and clock lane exit ULPS */
	outp32(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0xF);
	try_times = 0;
	temp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	while ((temp & cmp_ulpsactivenot_val) != cmp_ulpsactivenot_val) {
		udelay(10);
		if (++try_times > 400) { /* ulps exit try_time must be greater than 400 */
			DPU_FB_ERR("fb%d, request data clock lane exit ulps fail!PHY_STATUS=0x%x.\n",
				dpufd->index, temp);
			break;
		}
		temp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	}

	/* mipi spec */
	mdelay(1);

	/* clear PHY_ULPS_CTRL */
	outp32(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0x0);

	/* check DPHY data lane cmp_stopstate_val */
	try_times = 0;
	temp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	while ((temp & cmp_stopstate_val) != cmp_stopstate_val) {
		udelay(10);
		if (++try_times > 400) {  /* ulps exit try_time must be greater than 400 */
			DPU_FB_ERR("fb%d, check phy data clk lane stop state failed! PHY_STATUS=0x%x.\n",
				dpufd->index, temp);
			break;
		}
		temp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	}
	/* reset dsi */
	outp32(mipi_dsi_base + MIPIDSI_PWR_UP_OFFSET, 0x0);
	udelay(5);
	/* Power_up dsi */
	outp32(mipi_dsi_base + MIPIDSI_PWR_UP_OFFSET, 0x1);
	DPU_FB_DEBUG("fb%d, -!\n", dpufd->index);

	return 0;
}

static void mipi_dsi_data_clock_lane_exit_ulps(struct dpu_fb_data_type *dpufd,
	char __iomem *mipi_dsi_base, uint32_t cmp_ulpsactivenot_val)
{
	uint32_t temp;
	uint32_t try_times;

	/* request that data lane and clock lane exit ULPS */
	outp32(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0xF);
	try_times = 0;
	temp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	while ((temp & cmp_ulpsactivenot_val) != cmp_ulpsactivenot_val) {
		udelay(10);
		if (++try_times > 400) { /* ulps exit try_time must be greater than 400 */
			DPU_FB_ERR("fb%d, request data clock lane exit ulps fail!PHY_STATUS=0x%x.\n",
				dpufd->index, temp);
			break;
		}

		temp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
	}
}

static int mipi_dsi_ulps_exit(struct dpu_fb_data_type *dpufd,
	char __iomem *mipi_dsi_base)
{
	uint32_t cmp_ulpsactivenot_val = 0;
	uint32_t cmp_stopstate_val = 0;

	dpu_check_and_return(!mipi_dsi_base, -EINVAL, ERR, "mipi_dsi_base is null.\n");

	DPU_FB_DEBUG("fb%d, +!\n", dpufd->index);

	mipi_dsi_get_ulps_stopstate(dpufd, &cmp_ulpsactivenot_val, &cmp_stopstate_val, false);

	if (dpufd->panel_info.mipi.phy_mode == CPHY_MODE) {
		cmp_ulpsactivenot_val &= (~(BIT(3)));
		cmp_stopstate_val &= (~(BIT(2)));
	}
	if (is_dual_mipi_panel(dpufd))
		set_reg(dpufd->peri_crg_base + PEREN3, 0xf, 4, 28);
	else
		set_reg(dpufd->peri_crg_base + PEREN3, 0x3, 4, 28);

	udelay(100); /* wait pll clk */
	/* bit13 lock sel enable (dual_mipi_panel bit29 set 1) ,colse clock gate */
	set_reg(dpufd->pctrl_base + PERI_CTRL33, 0x0, 1, 13);
	if (is_dual_mipi_panel(dpufd))
		set_reg(dpufd->pctrl_base + PERI_CTRL30, 0x0, 1, 29);

	mipi_dsi_data_clock_lane_exit_ulps(dpufd, mipi_dsi_base, cmp_ulpsactivenot_val);

	/* mipi spec */
	mdelay(1);

	/* clear PHY_ULPS_CTRL */
	outp32(mipi_dsi_base + MIPIDSI_PHY_ULPS_CTRL_OFFSET, 0x0);

	mipi_dsi_check_ulps_stopstate(dpufd, mipi_dsi_base, cmp_stopstate_val, false);

	/* enable DPHY clock lane's Hight Speed Clock */
	set_reg(mipi_dsi_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x1, 1, 0);

	/* reset dsi */
	outp32(mipi_dsi_base + MIPIDSI_PWR_UP_OFFSET, 0x0);
	udelay(5);
	/* Power_up dsi */
	outp32(mipi_dsi_base + MIPIDSI_PWR_UP_OFFSET, 0x1);
	DPU_FB_DEBUG("fb%d, -!\n", dpufd->index);

	return 0;
}

int mipi_dsi_ulps_cfg(struct dpu_fb_data_type *dpufd, int enable)
{
	int ret = 0;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is null.\n");

	DPU_FB_DEBUG("fb%d, +.\n", dpufd->index);

	if (enable) {
		mipi_dsi_ulps_exit(dpufd, dpufd->mipi_dsi0_base);
		if (is_dual_mipi_panel(dpufd))
			mipi_dsi_ulps_exit(dpufd, dpufd->mipi_dsi1_base);
	} else {
		mipi_dsi_ulps_enter(dpufd, dpufd->mipi_dsi0_base);
		if (is_dual_mipi_panel(dpufd))
			mipi_dsi_ulps_enter(dpufd, dpufd->mipi_dsi1_base);
	}

	DPU_FB_DEBUG("fb%d, -.\n", dpufd->index);

	return ret;
}

void mipi_dsi_reset(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is null.\n");

	set_reg(dpufd->mipi_dsi0_base + MIPIDSI_PWR_UP_OFFSET, 0x0, 1, 0);
	msleep(2);
	set_reg(dpufd->mipi_dsi0_base + MIPIDSI_PWR_UP_OFFSET, 0x1, 1, 0);
}

/*
 * MIPI DPHY GPIO for FPGA
 */
#define GPIO_MIPI_DPHY_PG_SEL_A_NAME "pg_sel_a"
#define GPIO_MIPI_DPHY_PG_SEL_B_NAME "pg_sel_b"
#define GPIO_MIPI_DPHY_TX_RX_A_NAME "tx_rx_a"
#define GPIO_MIPI_DPHY_TX_RX_B_NAME "tx_rx_b"

static uint32_t g_gpio_pg_sel_a = GPIO_PG_SEL_A;
static uint32_t g_gpio_tx_rx_a = GPIO_TX_RX_A;
static uint32_t g_gpio_pg_sel_b = GPIO_PG_SEL_B;
static uint32_t g_gpio_tx_rx_b = GPIO_TX_RX_B;

static struct gpio_desc g_mipi_dphy_gpio_request_cmds[] = {
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_PG_SEL_A_NAME, &g_gpio_pg_sel_a, 0},
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_PG_SEL_B_NAME, &g_gpio_pg_sel_b, 0},
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_TX_RX_A_NAME, &g_gpio_tx_rx_a, 0},
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_TX_RX_B_NAME, &g_gpio_tx_rx_b, 0},
};

static struct gpio_desc g_mipi_dphy_gpio_free_cmds[] = {
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_PG_SEL_A_NAME, &g_gpio_pg_sel_a, 0},
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_PG_SEL_B_NAME, &g_gpio_pg_sel_b, 0},
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_TX_RX_A_NAME, &g_gpio_tx_rx_a, 0},
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_TX_RX_B_NAME, &g_gpio_tx_rx_b, 0},

};

static struct gpio_desc g_mipi_dphy_gpio_normal_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_PG_SEL_A_NAME, &g_gpio_pg_sel_a, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_TX_RX_A_NAME, &g_gpio_tx_rx_a, 1},

	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_PG_SEL_B_NAME, &g_gpio_pg_sel_b, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_TX_RX_B_NAME, &g_gpio_tx_rx_b, 1},
};

static struct gpio_desc g_mipi_dphy_gpio_lowpower_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_PG_SEL_A_NAME, &g_gpio_pg_sel_a, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_TX_RX_A_NAME, &g_gpio_tx_rx_a, 0},

	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_PG_SEL_B_NAME, &g_gpio_pg_sel_b, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_MIPI_DPHY_TX_RX_B_NAME, &g_gpio_tx_rx_b, 0},
};

static int mipi_dsi_dphy_fastboot_fpga(struct dpu_fb_data_type *dpufd)
{
	if (g_fpga_flag == 1)
		/* mpi dphy gpio request */
		gpio_cmds_tx(g_mipi_dphy_gpio_request_cmds,
			ARRAY_SIZE(g_mipi_dphy_gpio_request_cmds));

	return 0;
}

static int mipi_dsi_dphy_on_fpga(struct dpu_fb_data_type *dpufd)
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

static int mipi_dsi_dphy_off_fpga(struct dpu_fb_data_type *dpufd)
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

	dpu_check_and_return(!pdev, -EINVAL, ERR, "pdev is null.\n");
	dpufd = platform_get_drvdata(pdev);
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is null.\n");

	DPU_FB_DEBUG("fb%d, +.\n", dpufd->index);

	mipi_dsi_dphy_fastboot_fpga(dpufd);

	(void)mipi_dsi_clk_enable(dpufd);

	pinfo = &(dpufd->panel_info);
	if (pinfo) {
		memset(&(pinfo->dsi_phy_ctrl), 0, sizeof(pinfo->dsi_phy_ctrl));
		if ((g_dss_version_tag & FB_ACCEL_DSSV320) && (g_fpga_flag == 0))
			get_dsi_phy_ctrl_v320(dpufd, &(pinfo->dsi_phy_ctrl), pinfo->mipi.dsi_bit_clk_upt);
		else
			get_dsi_phy_ctrl(dpufd, &(pinfo->dsi_phy_ctrl));
	}

	ret = panel_next_set_fastboot(pdev);

	DPU_FB_DEBUG("fb%d, -.\n", dpufd->index);

	return ret;
}


static void mipi_reset_dsi(struct dpu_fb_data_type *dpufd)
{
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
}

static int mipi_dsi_is_phy_special(void)
{
	if (g_mipi_dphy_version == DPHY_VER_14 && g_mipi_dphy_opt == 0)
		return TRUE;

	return FALSE;
}

static void mipi_dsi_module_clk_sel(struct dpu_fb_data_type *dpufd)
{
	if (dpufd->index == PRIMARY_PANEL_IDX) {
		if (dpufd->panel_info.mipi.txoff_rxulps_en) {
			/* bit28, bit29, bit30, bit31 set 0x3
			 * txdphy0_cfg, txdphy0_ref clock enable, txdphy1_cfg, txdphy1_ref clock no effect
			 */
			set_reg(dpufd->peri_crg_base + PEREN3, 0x3, 4, 28);
			udelay(100);  /* wait pll clk */
			/* bit13 lock sel enable (dual_mipi_panel bit29 set 1) ,colse clock gate */
			set_reg(dpufd->pctrl_base + PERI_CTRL33, 0x0, 1, 13);
		}

		mipi_dsi_on_sub1(dpufd, dpufd->mipi_dsi0_base);
		if (is_dual_mipi_panel(dpufd))
			mipi_dsi_on_sub1(dpufd, dpufd->mipi_dsi1_base);

		mipi_dsi_pll_status_check_ec(dpufd, dpufd->mipi_dsi0_base);
		if (is_dual_mipi_panel(dpufd))
			mipi_dsi_pll_status_check_ec(dpufd, dpufd->mipi_dsi1_base);
	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		mipi_dsi_on_sub1(dpufd, dpufd->mipi_dsi1_base);
		mipi_dsi_pll_status_check_ec(dpufd, dpufd->mipi_dsi1_base);
	} else {
		DPU_FB_ERR("fb%d, not supported!\n", dpufd->index);
	}

	/* only used for v320 */
	if (dpufd->panel_info.mipi.txoff_rxulps_en) {
		mipi_dsi_ulps_cfg(dpufd, false);
		mdelay(1);
		mipi_phy_mask_ctrl(dpufd->mipi_dsi0_base, false, 0);
		mipi_dsi_ulps_exit_for_video(dpufd, dpufd->mipi_dsi0_base);
	}
}

int mipi_dsi_on(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;

	dpu_check_and_return(!pdev, -EINVAL, ERR, "pdev is NULL\n");

	dpufd = platform_get_drvdata(pdev);
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");

	DPU_FB_DEBUG("fb%d, +.\n", dpufd->index);

	if (mipi_dsi_is_phy_special())
		mipi_reset_dsi(dpufd);

	mipi_dsi_dphy_on_fpga(dpufd);

	/* set LCD init step before LCD on */
	dpufd->panel_info.lcd_init_step = LCD_INIT_POWER_ON;
	panel_next_on(pdev);

	/* dis-reset
	 * ip_reset_dis_dsi0, ip_reset_dis_dsi1
	 */
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

	udelay(1000);  /* delay 1ms */

	if (mipi_dsi_is_phy_special()) {
		/* C50 Avoiding leakage
		 * restore pll to open
		 */
		mipi_config_phy_test_code(dpufd->mipi_dsi0_base, 0x00010070, 0x0);
		mipi_config_phy_test_code(dpufd->mipi_dsi0_base, 0x00010069, 0x61);
		mipi_config_phy_test_code(dpufd->mipi_dsi0_base, 0x00010069, 0x1);
	}

	mipi_dsi_module_clk_sel(dpufd);

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

static void mipi_tx_off_rx_ulps_config(struct dpu_fb_data_type *dpufd)
{
	int delay_count = 0;
	int delay_count_max = 16;  /* Read register status, maximum waiting time is 16ms */

	if (!dpufd->panel_info.mipi.txoff_rxulps_en)
		return;

	if (mipi_dsi_is_phy_special()) {
		mipi_dsi_ulps_cfg(dpufd, 0);
		return;
	}

	disable_ldi(dpufd);
	/* Read register status, maximum waiting time is 16ms
	 * 0x7FF--The lower 11 bits of the register 0x1--Register value
	 */
	while ((((uint32_t)inp32(dpufd->dss_base + DSS_LDI0_OFFSET + LDI_VSTATE)) & 0x7FF) != 0x1) {
		if (++delay_count > delay_count_max) {
			DPU_FB_ERR("wait ldi vstate idle timeout\n");
			break;
		}
		msleep(1);
	}
	mipi_dsi_ulps_cfg(dpufd, 0);
}

int mipi_dsi_off(struct platform_device *pdev)
{
	int ret;
	struct dpu_fb_data_type *dpufd = NULL;

	dpu_check_and_return(!pdev, -EINVAL, ERR, "pdev is NULL\n");

	dpufd = platform_get_drvdata(pdev);
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");

	DPU_FB_DEBUG("fb%d, +.\n", dpufd->index);

	/* set LCD uninit step before LCD off */
	dpufd->panel_info.lcd_uninit_step = LCD_UNINIT_MIPI_HS_SEND_SEQUENCE;
	ret = panel_next_off(pdev);

	if (dpufd->panel_info.lcd_uninit_step_support)
		ret = panel_next_off(pdev);  /* MIPI LP mode end */

	if (mipi_dsi_is_phy_special()) {
		/* C50 Avoiding leakage
		 * 0x72 & 0x73 Manually control pll
		 * 0x70 Toggle manual control
		 * 0x63 down frequenccy
		 * 0x62 control RG_LOAD_BAND_EN
		 * 0x60 & 0x61 control RG_BAND
		 * 0x67 & 0x68 control RG_VREFSEL_XXX
		 */
		mipi_config_phy_test_code(dpufd->mipi_dsi0_base, 0x00010072, 0x1f);
		mipi_config_phy_test_code(dpufd->mipi_dsi0_base, 0x00010073, 0x0);

		mipi_config_phy_test_code(dpufd->mipi_dsi0_base, 0x00010070, 0x61);
		mipi_config_phy_test_code(dpufd->mipi_dsi0_base, 0x00010072, 0x18);
		mipi_config_phy_test_code(dpufd->mipi_dsi0_base, 0x00010072, 0x1f);

		mipi_config_phy_test_code(dpufd->mipi_dsi0_base, 0x00010063, 0x12);
		mipi_config_phy_test_code(dpufd->mipi_dsi0_base, 0x00010062, 0x2);
		mipi_config_phy_test_code(dpufd->mipi_dsi0_base, 0x00010061, 0x0);
		mipi_config_phy_test_code(dpufd->mipi_dsi0_base, 0x00010060, 0x0);
	}

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		mipi_tx_off_rx_ulps_config(dpufd);
		mipi_dsi_off_sub(dpufd, dpufd->mipi_dsi0_base);
		if (is_dual_mipi_panel(dpufd))
			mipi_dsi_off_sub(dpufd, dpufd->mipi_dsi1_base);
	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		mipi_dsi_off_sub(dpufd, dpufd->mipi_dsi1_base);
	} else {
		DPU_FB_ERR("fb%d, not supported!\n", dpufd->index);
	}

	if (mipi_dsi_is_phy_special()) {
		/* C50 Avoiding leakage
		 * rg_load_band_en eq 1
		 * kband eq 0000000000
		 * pll open and low
		 */
		mipi_config_phy_test_code(dpufd->mipi_dsi0_base, 0x00010072, 0x7);
		mipi_config_phy_test_code(dpufd->mipi_dsi0_base, 0x00010067, 0x0);
		mipi_config_phy_test_code(dpufd->mipi_dsi0_base, 0x00010068, 0x0);
	}

	mipi_dsi_clk_disable(dpufd);

	mipi_dsi_dphy_off_fpga(dpufd);

	if (!mipi_dsi_is_phy_special())
		mipi_reset_dsi(dpufd);

	if (dpufd->panel_info.lcd_uninit_step_support)
		ret = panel_next_off(pdev);

	DPU_FB_DEBUG("fb%d, -.\n", dpufd->index);

	return ret;
}

static void set_dphy_bit_clk_upt_video(struct dpu_fb_data_type *dpufd,
	char __iomem *mipi_dsi_base, struct mipi_dsi_phy_ctrl *phy_ctrl, struct dpu_panel_info *pinfo)
{
	bool is_ready = false;
	unsigned long dw_jiffies;
	uint32_t temp;

	set_reg(mipi_dsi_base + MIPIDSI_CLKMGR_CFG_OFFSET, phy_ctrl->clk_division, 8, 0);
	set_reg(mipi_dsi_base + MIPIDSI_CLKMGR_CFG_OFFSET, phy_ctrl->clk_division, 8, 8);
	if (g_mipi_dphy_version == DPHY_VER_12) {
		/* physical configuration I, Q
		 * physical configuration PLL I
		 */
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010014,  (1 << 4) +
		     (phy_ctrl->rg_pll_pre_div1p << 3) + phy_ctrl->rg_pll_chp);

		/* physical configuration PLL II, M */
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010015, phy_ctrl->rg_div);

		/* sets the analog characteristic of V reference in D-PHY TX */
		mipi_config_phy_test_code(mipi_dsi_base, 0x0001001D, phy_ctrl->rg_vrefsel_vcm);

		/* MISC AFE Configuration */
		mipi_config_phy_test_code(mipi_dsi_base, 0x0001001E, (3 << 5) + (phy_ctrl->reload_sel << 4) +
			(phy_ctrl->rg_phase_gen_en << 3) + (phy_ctrl->rg_band_sel << 2) +
			(phy_ctrl->pll_register_override << 1) + phy_ctrl->pll_power_down);

		/* reload_command */
		mipi_config_phy_test_code(mipi_dsi_base, 0x0001001F, phy_ctrl->load_command);
	} else {
		mipi_phy_init_config_v14(dpufd, mipi_dsi_base);
	}
	/* set dphy spec parameter */
	mipi_config_dphy_spec_parameter(mipi_dsi_base, pinfo);

	is_ready = false;
	dw_jiffies = jiffies + HZ / 2;
	do {
		temp = inp32(mipi_dsi_base + MIPIDSI_PHY_STATUS_OFFSET);
		if ((temp & 0x00000001) == 0x00000001) {
			is_ready = true;
			break;
		}
	} while (time_after(dw_jiffies, jiffies));

	if (!is_ready)
		DPU_FB_ERR("fb%d, phylock is not ready!MIPIDSI_PHY_STATUS_OFFSET=0x%x.\n", dpufd->index, temp);
}

static void mipi_dsi_bit_clk_upt_set_video(struct dpu_fb_data_type *dpufd,
	char __iomem *mipi_dsi_base, struct mipi_dsi_phy_ctrl *phy_ctrl)
{
	struct dpu_panel_info *pinfo = NULL;
	uint32_t hline_time = 0;
	uint32_t hsa_time = 0;
	uint32_t hbp_time = 0;
	uint32_t pixel_clk;
	dss_rect_t rect = {0};

	pinfo = &(dpufd->panel_info);
	pinfo->dsi_phy_ctrl = *phy_ctrl;

	rect.w = pinfo->xres;
	rect.h = pinfo->yres;

	mipi_ifbc_get_rect(dpufd, &rect);

	set_dphy_bit_clk_upt_video(dpufd, mipi_dsi_base, phy_ctrl, pinfo);

	/* phy_stop_wait_time */
	set_reg(mipi_dsi_base + MIPIDSI_PHY_IF_CFG_OFFSET, phy_ctrl->phy_stop_wait_time, 8, 8);

	/* Define the DPI Horizontal timing configuration:
	 * Hsa_time = HSA*(PCLK period/Clk Lane Byte Period);
	 * Hbp_time = HBP*(PCLK period/Clk Lane Byte Period);
	 * Hline_time = (HSA+HBP+HACT+HFP)*(PCLK period/Clk Lane Byte Period);
	 */
	pixel_clk = mipi_pixel_clk(dpufd);
	if (pixel_clk != 0) {
		hsa_time = pinfo->ldi.h_pulse_width * phy_ctrl->lane_byte_clk / pixel_clk;
		hbp_time = pinfo->ldi.h_back_porch * phy_ctrl->lane_byte_clk / pixel_clk;
		hline_time = (pinfo->ldi.h_pulse_width + pinfo->ldi.h_back_porch +
		rect.w + pinfo->ldi.h_front_porch) * phy_ctrl->lane_byte_clk / pixel_clk;
	} else {
		DPU_FB_ERR("pixel_clk is zero\n");
	}

	set_reg(mipi_dsi_base + MIPIDSI_VID_HSA_TIME_OFFSET, hsa_time, 12, 0);
	set_reg(mipi_dsi_base + MIPIDSI_VID_HBP_TIME_OFFSET, hbp_time, 12, 0);
	set_reg(mipi_dsi_base + MIPIDSI_VID_HLINE_TIME_OFFSET, hline_time, 15, 0);

	/* Configure core's phy parameters */
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_LPCLK_CFG_OFFSET, phy_ctrl->clk_lane_lp2hs_time, 10, 0);
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_LPCLK_CFG_OFFSET, phy_ctrl->clk_lane_hs2lp_time, 10, 16);
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_CFG_OFFSET, phy_ctrl->data_lane_lp2hs_time, 10, 0);
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_CFG_OFFSET, phy_ctrl->data_lane_hs2lp_time, 10, 16);
}

static void mipi_dsi_bit_clk_upt_set_cmd(struct dpu_fb_data_type *dpufd,
	char __iomem *mipi_dsi_base, struct mipi_dsi_phy_ctrl *phy_ctrl)
{
	struct dpu_panel_info *pinfo = NULL;

	pinfo = &(dpufd->panel_info);
	if (g_mipi_dphy_version == DPHY_VER_12) {
		/* physical configuration PLL I */
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010014,  (1 << 4) +
			(phy_ctrl->rg_pll_pre_div1p << 3) + phy_ctrl->rg_pll_chp);
		/* physical configuration PLL II, M */
		mipi_config_phy_test_code(mipi_dsi_base, 0x00010015, pinfo->dsi_phy_ctrl.rg_div);
	} else {
		mipi_phy_init_config_v14(dpufd, mipi_dsi_base);
	}
	/* Configure core's phy parameters */
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_LPCLK_CFG_OFFSET,
		pinfo->dsi_phy_ctrl.clk_lane_lp2hs_time, 10, 0);
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_LPCLK_CFG_OFFSET,
		pinfo->dsi_phy_ctrl.clk_lane_hs2lp_time, 10, 16);

	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_RD_CFG_OFFSET, 0x7FFF, 15, 0);
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_CFG_OFFSET,
		pinfo->dsi_phy_ctrl.data_lane_lp2hs_time, 10, 0);
	set_reg(mipi_dsi_base + MIPIDSI_PHY_TMR_CFG_OFFSET,
		pinfo->dsi_phy_ctrl.data_lane_hs2lp_time, 10, 16);

	/* escape clock dividor */
	outp32(mipi_dsi_base + MIPIDSI_CLKMGR_CFG_OFFSET,
		(phy_ctrl->clk_division + (phy_ctrl->clk_division << 8)));
}

static bool check_pctrl_trstop_flag(struct dpu_fb_data_type *dpufd)
{
	bool is_ready = false;
	int count;
	uint32_t tmp = 0;
	int time_count = 10;  /* wait 20us */

	if (is_dual_mipi_panel(dpufd)) {
		for (count = 0; count < time_count; count++) {
			tmp = inp32(dpufd->pctrl_base + PERI_STAT0);
			if ((tmp & 0xC0000000) == 0xC0000000) {
				is_ready = true;
				break;
			}
			udelay(2);
		}
	} else {
		for (count = 0; count < time_count; count++) {
			tmp = inp32(dpufd->pctrl_base + PERI_STAT0);
			if ((tmp & 0x80000000) == 0x80000000) {
				is_ready = true;
				break;
			}
			udelay(2);
		}
	}

	return is_ready;
}

static void mipi_dsi_bit_clk_upt_isr_handler_prepare(struct dpu_fb_data_type *dpufd,
	uint32_t *vfp_time, uint64_t *lane_byte_clk, uint8_t *esd_enable)
{
	*lane_byte_clk = dpufd->panel_info.dsi_phy_ctrl.lane_byte_clk;
	*vfp_time = (uint32_t)inp32(dpufd->mipi_dsi0_base + MIPIDSI_VID_HLINE_TIME_OFFSET) & 0x7fff;
	if (*lane_byte_clk != 0) {
		*vfp_time = *vfp_time * (dpufd->panel_info.ldi.v_front_porch + 10) /
			((uint32_t)(*lane_byte_clk / 1000000));
	} else {
		DPU_FB_ERR("vfp_time == 0.\n");
		*vfp_time = 80;  /* 80us */
	}

	*esd_enable = dpufd->panel_info.esd_enable;
	if (is_mipi_video_panel(dpufd)) {
		dpufd->panel_info.esd_enable = 0;
		disable_ldi(dpufd);
	}
}

static int wait_stopstate_cnt(struct dpu_fb_data_type *dpufd, uint32_t vfp_time, uint8_t esd_enable,
	timeval_compatible *tv0)
{
	bool is_ready = false;
	uint32_t timediff = 0;
	timeval_compatible tv1;

	if (g_dss_version_tag == FB_ACCEL_DSSV320) {
		set_reg(dpufd->pctrl_base + PERI_CTRL33, 1, 1, 0);
		set_reg(dpufd->pctrl_base + PERI_CTRL33, 1, 1, 3);
	} else {
		set_reg(dpufd->pctrl_base + PERI_CTRL30, 1, 1, 0);
		set_reg(dpufd->pctrl_base + PERI_CTRL30, 1, 1, 3);
		if (is_dual_mipi_panel(dpufd)) {
			set_reg(dpufd->pctrl_base + PERI_CTRL30, 1, 1, 16);
			set_reg(dpufd->pctrl_base + PERI_CTRL30, 1, 1, 19);
		}
	}

	while ((!is_ready) && (timediff < vfp_time)) {
		is_ready = check_pctrl_trstop_flag(dpufd);
		dpufb_get_timestamp(&tv1);
		timediff = dpufb_timestamp_diff(tv0, &tv1);
	}
	DPU_FB_INFO("timediff=%d us, vfp_time=%d us.\n", timediff, vfp_time);

	if (g_dss_version_tag == FB_ACCEL_DSSV320) {
		set_reg(dpufd->pctrl_base + PERI_CTRL33, 0, 1, 0);
	} else {
		set_reg(dpufd->pctrl_base + PERI_CTRL30, 0, 1, 0);
		if (is_dual_mipi_panel(dpufd))
			set_reg(dpufd->pctrl_base + PERI_CTRL30, 0, 1, 16);
	}

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

static void mipi_set_cdphy_bit_clk(struct dpu_fb_data_type *dpufd, struct dpu_panel_info *pinfo,
	struct mipi_dsi_phy_ctrl *phy_ctrl, uint8_t esd_enable)
{
	if (is_mipi_cmd_panel(dpufd)) {
		mipi_dsi_bit_clk_upt_set_cmd(dpufd, dpufd->mipi_dsi0_base, phy_ctrl);
		if (is_dual_mipi_panel(dpufd))
			mipi_dsi_bit_clk_upt_set_cmd(dpufd, dpufd->mipi_dsi1_base, phy_ctrl);
	} else {
		set_reg(dpufd->mipi_dsi0_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x0, 1, 0);
		mipi_dsi_bit_clk_upt_set_video(dpufd, dpufd->mipi_dsi0_base, phy_ctrl);
		set_reg(dpufd->mipi_dsi0_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x1, 1, 0);
		if (is_dual_mipi_panel(dpufd)) {
			set_reg(dpufd->mipi_dsi0_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x0, 1, 0);
			mipi_dsi_bit_clk_upt_set_video(dpufd, dpufd->mipi_dsi1_base, phy_ctrl);
			set_reg(dpufd->mipi_dsi0_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x1, 1, 0);
		}

		pinfo->esd_enable = esd_enable;
		enable_ldi(dpufd);
	}
}

int mipi_dsi_bit_clk_upt_isr_handler(struct dpu_fb_data_type *dpufd)
{
	struct mipi_dsi_phy_ctrl phy_ctrl = {0};
	struct dpu_panel_info *pinfo = NULL;
	uint32_t dsi_bit_clk_upt;
	uint8_t esd_enable;
	timeval_compatible tv0;
	uint32_t vfp_time = 0;
	uint64_t lane_byte_clk = 0;

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
	if ((g_dss_version_tag & FB_ACCEL_DSSV320) && (g_fpga_flag == 0))
		get_dsi_phy_ctrl_v320(dpufd, &phy_ctrl, pinfo->mipi.dsi_bit_clk_upt);
	else
		get_dsi_phy_ctrl(dpufd, &phy_ctrl);

	/* wait stopstate cnt:dphy_stopstate_cnt_en=1 (pctrl_dphy_ctrl[0]) */
	if (wait_stopstate_cnt(dpufd, vfp_time, esd_enable, &tv0) < 0)
		return 0;

	mipi_set_cdphy_bit_clk(dpufd, pinfo, &phy_ctrl, esd_enable);

	DPU_FB_INFO("Mipi clk success changed from %d M switch to %d M\n", pinfo->mipi.dsi_bit_clk, dsi_bit_clk_upt);

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

