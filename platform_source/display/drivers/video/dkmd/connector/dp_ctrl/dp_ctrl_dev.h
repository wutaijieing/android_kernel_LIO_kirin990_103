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

#ifndef __DP_CTRL_DEV_H__
#define __DP_CTRL_DEV_H__

#define DEFAULT_MIDIA_PPLL7_CLOCK_FREQ 1290000000UL
#define VCO_MIN_FREQ_OUPUT         800000000UL /* 800 * 1000 *1000 */
#define SYS_FREQ   38400000UL /* 38.4 * 1000 * 1000 */
#define MIDIA_PPLL7_CTRL0	0x838
#define MIDIA_PPLL7_CTRL1	0x83c

#define MIDIA_PPLL7_FREQ_DEVIDER_MASK	GENMASK(25, 2)
#define MIDIA_PPLL7_FRAC_MODE_MASK	GENMASK(25, 0)

#define DP_PLUG_TYPE_NORMAL 0
#define DP_PLUG_TYPE_FLIPPED 1

#define DPTX_CHANNEL_NUM_MAX 8
#define DPTX_DATA_WIDTH_MAX 24

#define DPTX_COMBOPHY_PARAM_NUM				20
#define DPTX_COMBOPHY_SSC_PPM_PARAM_NUM		12

#define DSC_DEFAULT_DECODER 2

struct dpu_connector;
struct dkmd_connector_info;
struct dp_ctrl;
struct video_params;
struct audio_params;
struct dtd;

struct ldi_panel_info {
	uint32_t h_back_porch;
	uint32_t h_front_porch;
	uint32_t h_pulse_width;

	uint32_t v_back_porch;
	uint32_t v_front_porch;
	uint32_t v_pulse_width;

	uint8_t hsync_plr;
	uint8_t vsync_plr;

	uint64_t pxl_clk_rate;
	uint32_t pxl_clk_rate_div;
};

struct dptx_combophy_ctrl {
	uint32_t combophy_pree_swing[DPTX_COMBOPHY_PARAM_NUM];
	uint32_t combophy_ssc_ppm[DPTX_COMBOPHY_SSC_PPM_PARAM_NUM];
};

int dptx_bw_to_phy_rate(uint8_t bw);
int dptx_phy_rate_to_bw(uint8_t rate);
void dp_send_cable_notification(struct dp_ctrl *dptx, int val);
void dptx_audio_params_reset(struct audio_params *aparams);
void dptx_video_params_reset(struct video_params *params);
bool dptx_check_low_temperature(struct dp_ctrl *dptx);

void dp_ctrl_default_setup(struct dpu_connector *connector);

#endif
