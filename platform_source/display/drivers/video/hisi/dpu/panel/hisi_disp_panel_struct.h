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
#ifndef HISI_DISP_PANEL_STRUCT_H
#define HISI_DISP_PANEL_STRUCT_H

#include "hisi_disp_panel_enum.h"
#include "hisi_disp_bl.h"
#include "dsc/dsc_algorithm.h"

/* DSI PHY configuration */
struct mipi_dsi_phy_ctrl {
	uint64_t lane_byte_clk;
	uint64_t lane_word_clk;
	uint64_t lane_byte_clk_default;
	uint32_t clk_division;

	uint32_t clk_lane_lp2hs_time;
	uint32_t clk_lane_hs2lp_time;
	uint32_t data_lane_lp2hs_time;
	uint32_t data_lane_hs2lp_time;
	uint32_t clk2data_delay;
	uint32_t data2clk_delay;

	uint32_t clk_pre_delay;
	uint32_t clk_post_delay;
	uint32_t clk_t_lpx;
	uint32_t clk_t_hs_prepare;
	uint32_t clk_t_hs_zero;
	uint32_t clk_t_hs_trial;
	uint32_t clk_t_wakeup;
	uint32_t data_pre_delay;
	uint32_t data_post_delay;
	uint32_t data_t_lpx;
	uint32_t data_t_hs_prepare;
	uint32_t data_t_hs_zero;
	uint32_t data_t_hs_trial;
	uint32_t data_t_ta_go;
	uint32_t data_t_ta_get;
	uint32_t data_t_wakeup;

	uint32_t phy_stop_wait_time;

	uint32_t rg_lptx_sri;
	uint32_t rg_vrefsel_lptx;
	uint32_t rg_vrefsel_vcm;
	uint32_t rg_hstx_ckg_sel;
	uint32_t rg_pll_fbd_div5f;
	uint32_t rg_pll_fbd_div1f;
	uint32_t rg_pll_fbd_2p;
	uint32_t rg_pll_enbwt;
	uint32_t rg_pll_fbd_p;
	uint32_t rg_pll_fbd_s;
	uint32_t rg_pll_pre_div1p;
	uint32_t rg_pll_pre_p;
	uint32_t rg_pll_vco_750m;
	uint32_t rg_pll_lpf_rs;
	uint32_t rg_pll_lpf_cs;
	uint32_t rg_pll_enswc;
	uint32_t rg_pll_chp;

	/* only for 3660 use */
	uint32_t pll_register_override;
	uint32_t pll_power_down;
	uint32_t rg_band_sel;
	uint32_t rg_phase_gen_en;
	uint32_t reload_sel;
	uint32_t rg_pll_cp_p;
	uint32_t rg_pll_refsel;
	uint32_t rg_pll_cp;
	uint32_t load_command;

	/* for CDPHY */
	uint32_t rg_cphy_div;
	uint32_t rg_div;
	uint32_t rg_pre_div;
	uint32_t rg_320m;
	uint32_t rg_2p5g;
	uint32_t rg_0p8v;
	uint32_t rg_lpf_r;
	uint32_t rg_cp;
	uint32_t rg_pll_fbkdiv;
	uint32_t rg_pll_prediv;
	uint32_t rg_pll_posdiv;
	uint32_t t_prepare;
	uint32_t t_lpx;
	uint32_t t_prebegin;
	uint32_t t_post;
};

/* for sensorhub_aod, keep 32-bit aligned */
struct ldi_panel_info {
	uint32_t h_back_porch;
	uint32_t h_front_porch;
	uint32_t h_pulse_width;

	/*
	 * note: vbp > 8 if used overlay compose,
	 * also lcd vbp > 8 in lcd power on sequence
	 */
	uint32_t v_back_porch;
	uint32_t v_front_porch;
	uint32_t v_pulse_width;

	uint32_t hbp_store_vid;
	uint32_t hfp_store_vid;
	uint32_t hpw_store_vid;
	uint32_t vbp_store_vid;
	uint32_t vfp_store_vid;
	uint32_t vpw_store_vid;
	uint64_t pxl_clk_store_vid;

	uint32_t hbp_store_cmd;
	uint32_t hfp_store_cmd;
	uint32_t hpw_store_cmd;
	uint32_t vbp_store_cmd;
	uint32_t vfp_store_cmd;
	uint32_t vpw_store_cmd;
	uint64_t pxl_clk_store_cmd;

	uint8_t hsync_plr;
	uint8_t vsync_plr;
	uint8_t pixelclk_plr;
	uint8_t data_en_plr;

	/* for cabc */
	uint8_t dpi0_overlap_size;
	uint8_t dpi1_overlap_size;
};

struct mipi_panel_info {
	uint8_t dsi_version;
	uint8_t vc;
	uint8_t lane_nums;
	uint8_t lane_nums_select_support;
	uint8_t color_mode;
	uint32_t dsi_bit_clk; /* clock lane(p/n) */
	uint32_t dsi_bit_clk_default;
	uint32_t burst_mode;
	uint32_t max_tx_esc_clk;
	uint8_t non_continue_en;
	uint8_t txoff_rxulps_en;
	int frame_rate;
	int take_effect_delayed_frm_cnt;

	uint32_t hsa;
	uint32_t hbp;
	uint32_t dpi_hsize;
	uint32_t width;
	uint32_t hline_time;

	uint32_t vsa;
	uint32_t vbp;
	uint32_t vactive_line;
	uint32_t vfp;
	uint8_t dsi_timing_support;

	uint32_t dsi_bit_clk_val1;
	uint32_t dsi_bit_clk_val2;
	uint32_t dsi_bit_clk_val3;
	uint32_t dsi_bit_clk_val4;
	uint32_t dsi_bit_clk_val5;
	uint32_t dsi_bit_clk_upt;

	uint32_t hs_wr_to_time;

	/* dphy config parameter adjust */
	uint32_t clk_post_adjust;
	uint32_t clk_pre_adjust;
	uint32_t clk_pre_delay_adjust;
	int clk_t_hs_exit_adjust;
	int clk_t_hs_trial_adjust;
	uint32_t clk_t_hs_prepare_adjust;
	int clk_t_lpx_adjust;
	uint32_t clk_t_hs_zero_adjust;
	uint32_t data_post_delay_adjust;
	int data_t_lpx_adjust;
	uint32_t data_t_hs_prepare_adjust;
	uint32_t data_t_hs_zero_adjust;
	int data_t_hs_trial_adjust;
	uint32_t rg_vrefsel_vcm_adjust;
	uint32_t rg_vrefsel_lptx_adjust;
	uint32_t rg_lptx_sri_adjust;
	int data_lane_lp2hs_time_adjust;

	/* only for 3660 use */
	uint32_t rg_vrefsel_vcm_clk_adjust;
	uint32_t rg_vrefsel_vcm_data_adjust;

	uint32_t phy_mode;  /* 0: DPHY, 1:CPHY */
	uint32_t lp11_flag; /* 0: nomal_lp11, 1:short_lp11, 2:disable_lp11 */
	uint32_t phy_m_n_count_update;  /* 0:old ,1:new can get 988.8M */
	uint32_t eotp_disable_flag; /* 0: eotp enable, 1:eotp disable */

	uint8_t mininum_phy_timing_flag; /* 1:support entering lp11 with minimum clock */

	uint32_t dynamic_dsc_en; /* used for dfr */
	uint32_t dsi_te_type; /* 0: dsi0&te0, 1: dsi1&te0, 2: dsi1&te1 */
};

struct panel_dsc_info {
	enum pixel_format format;
	uint16_t dsc_version;
	uint16_t native_422;
	uint32_t idata_422;
	uint32_t convert_rgb;
	uint32_t adjustment_bits;
	uint32_t adj_bits_per_grp;
	uint32_t bits_per_grp;
	uint32_t slices_per_line;
	uint32_t pic_line_grp_num;
	uint32_t dsc_insert_byte_num;
	uint32_t dual_dsc_en;
	uint32_t dsc_en;
	struct dsc_info dsc_info;
};

struct panel_timing_info {
	uint32_t h_active;
	uint32_t v_active;

	uint32_t h_back_porch;
	uint32_t h_front_porch;
	uint32_t h_pulse_width;

	uint32_t v_back_porch;
	uint32_t v_front_porch;
	uint32_t v_pulse_width;

	uint8_t h_sync_polarity;
	uint8_t v_sync_polarity;

	uint64_t pxl_clk_rate;
	uint32_t pxl_clk_rate_div;
};

struct hisi_panel_info {
	uint32_t type;

	uint32_t xres;
	uint32_t yres;
	uint32_t fps;

	uint32_t width; /* mm */
	uint32_t height;
	uint32_t orientation;

	uint32_t bgr_fmt;
	uint32_t bpp;

	uint32_t vsync_ctrl_type;
	uint32_t ifbc_type;
	uint32_t dfr_support;

	uint8_t lcd_init_step;
	uint8_t lcd_uninit_step;
	uint8_t lcd_uninit_step_support;

	struct panel_timing_info timing_info;
	/* backlight basic info get from panel */
	struct hisi_panel_bl_info  bl_info;
	struct ldi_panel_info ldi;
	struct panel_dsc_info panel_dsc_info;

	struct mipi_panel_info mipi;
	struct mipi_dsi_phy_ctrl dsi_phy_ctrl;
	int (*set_fastboot)(struct platform_device *pdev);
};

/* vcc desc */
struct vcc_desc {
	int dtype;
	char *id;
	struct regulator **regulator;
	int min_uV;
	int max_uV;
	int waittype;
	int wait;
};

/* pinctrl data */
struct pinctrl_data {
	struct pinctrl *p;
	struct pinctrl_state *pinctrl_def;
	struct pinctrl_state *pinctrl_idle;
};

struct pinctrl_cmd_desc {
	int dtype;
	struct pinctrl_data *pctrl_data;
	int mode;
};

/* gpio desc */
struct gpio_desc {
	int dtype;
	int waittype;
	int wait;
	char *label;
	uint32_t *gpio;
	int value;
};

#endif /* HISI_DISP_PANEL_STRUCT_H */
