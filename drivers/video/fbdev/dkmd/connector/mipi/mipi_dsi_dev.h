/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __MIPI_DSI_DEV_H__
#define __MIPI_DSI_DEV_H__

#define DPHYTX_TRSTOP_FLAG_TIMEOUT_TIMES (10)
#define DSI_ADDR_TO_OFFSET (0)

/* mipi dsi panel */
enum {
	DSI_1_1_VERSION = 0,
	DSI_1_2_VERSION,
};

enum {
	DSI_LANE_NUMS_DEFAULT = 0,
	DSI_1_LANES_SUPPORT = BIT(0),
	DSI_2_LANES_SUPPORT = BIT(1),
	DSI_3_LANES_SUPPORT = BIT(2),
	DSI_4_LANES_SUPPORT = BIT(3),
};

enum {
	DSI_16BITS_1 = 0,
	DSI_16BITS_2,
	DSI_16BITS_3,
	DSI_18BITS_1,
	DSI_18BITS_2,
	DSI_24BITS_1,
	DSI_24BITS_2,
	DSI_24BITS_3,
	DSI_DSC24_COMPRESSED_DATA = 0xF,
};

enum {
	DSI_NON_BURST_SYNC_PULSES = 0,
	DSI_NON_BURST_SYNC_EVENTS,
	DSI_BURST_SYNC_PULSES_1,
	DSI_BURST_SYNC_PULSES_2,
};

enum {
	EN_DSI_TX_NORMAL_MODE = 0x0,
	EN_DSI_TX_LOW_PRIORITY_DELAY_MODE = 0x1,
	EN_DSI_TX_HIGH_PRIORITY_DELAY_MODE = 0x2,
	EN_DSI_TX_AUTO_MODE = 0xF,
};

enum MIPI_LP11_MODE {
	MIPI_NORMAL_LP11 = 0,
	MIPI_SHORT_LP11 = 1,
	MIPI_DISABLE_LP11 = 2,
};

enum V320_DPHY_VER {
	DPHY_VER_12 = 0,
	DPHY_VER_14,
	DPHY_VER_MAX
};

enum DSI_TE_TYPE {
	DSI0_TE0_TYPE = 0,  /* include video lcd */
	DSI1_TE0_TYPE = 1,
	DSI1_TE1_TYPE = 2,
};

#define DSI_VIDEO_DST_FORMAT_RGB565 0
#define DSI_VIDEO_DST_FORMAT_RGB666 1
#define DSI_VIDEO_DST_FORMAT_RGB666_LOOSE 2
#define DSI_VIDEO_DST_FORMAT_RGB888 3

#define DSI_CMD_DST_FORMAT_RGB565 0
#define DSI_CMD_DST_FORMAT_RGB666 1
#define DSI_CMD_DST_FORMAT_RGB888 2

struct mipi_dsi_timing {
	uint32_t hsa;
	uint32_t hbp;
	uint32_t dpi_hsize;
	uint32_t width;
	uint32_t hline_time;

	uint32_t vsa;
	uint32_t vbp;
	uint32_t vactive_line;
	uint32_t vfp;
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
	int32_t frame_rate;
	int32_t take_effect_delayed_frm_cnt;

	uint32_t hsa;
	uint32_t hbp;
	uint32_t dpi_hsize;
	uint32_t width;
	uint32_t hline_time;

	uint32_t vsa;
	uint32_t vbp;
	uint32_t vactive_line;
	uint32_t vfp;

	uint32_t dsi_bit_clk_upt_support;
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
	int32_t clk_t_hs_exit_adjust;
	int32_t clk_t_hs_trial_adjust;
	uint32_t clk_t_hs_prepare_adjust;
	int32_t clk_t_lpx_adjust;
	uint32_t clk_t_hs_zero_adjust;
	uint32_t data_post_delay_adjust;
	int32_t data_t_lpx_adjust;
	uint32_t data_t_hs_prepare_adjust;
	uint32_t data_t_hs_zero_adjust;
	int32_t data_t_hs_trial_adjust;
	uint32_t rg_vrefsel_vcm_adjust;
	uint32_t support_de_emphasis;
	uint32_t rg_vrefsel_lptx_adjust;
	uint32_t rg_lptx_sri_adjust;
	int32_t data_lane_lp2hs_time_adjust;

	uint32_t phy_mode;  /* 0: DPHY, 1:CPHY */
	uint32_t lp11_flag; /* 0: nomal_lp11, 1:short_lp11, 2:disable_lp11 */
	uint32_t phy_m_n_count_update;  /* 0:old ,1:new can get 988.8M */
	uint32_t eotp_disable_flag; /* 0: eotp enable, 1:eotp disable */

	uint8_t mininum_phy_timing_flag; /* 1:support entering lp11 with minimum clock */

	uint32_t dynamic_dsc_en; /* used for dfr */
	uint32_t dsi_te_type; /* 0: dsi0&te0, 1: dsi1&te0, 2: dsi1&te1 */

	uint64_t pxl_clk_rate;
	uint32_t pxl_clk_rate_div;
};

struct dpu_connector;
struct dkmd_connector_info;

void mipi_init(struct dpu_connector *connector);
int32_t mipi_dsi_cmds_tx(struct dsi_cmd_desc *cmds, int32_t cnt, char __iomem *dsi_base);

void mipi_dsi_default_setup(struct dpu_connector *connector);
bool mipi_phy_status_check(const char __iomem *mipi_dsi_base, uint32_t expected_value);
void mipi_dsi_ulps_cfg(struct dpu_connector *connector, bool is_ulps);

void mipi_dsi_auto_ulps_config(struct mipi_dsi_timing *timing, struct dpu_connector *connector,
	char __iomem *mipi_dsi_base);
void mipi_auto_ulps_ctrl(struct dpu_connector *connector, bool is_auto_ulps);

#endif
