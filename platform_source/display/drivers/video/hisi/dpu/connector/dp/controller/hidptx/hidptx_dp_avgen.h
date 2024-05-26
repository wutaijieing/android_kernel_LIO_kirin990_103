/* Copyright (c) 2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __HIDPTX_DP_AVGEN_H__
#define __HIDPTX_DP_AVGEN_H__

#include <linux/kernel.h>
#include "hisi_drv_dp.h"
struct dp_ctrl;

void dptx_audio_config(struct dp_ctrl *dptx);
void dptx_audio_core_config(struct dp_ctrl *dptx);
void dptx_en_audio_channel(struct dp_ctrl *dptx, int ch_num, int enable);
void dptx_audio_mute(struct dp_ctrl *dptx);

void dptx_audio_inf_type_change(struct dp_ctrl *dptx);
void dptx_audio_data_width_change(struct dp_ctrl *dptx);

void dptx_enable_default_video_stream(struct dp_ctrl *dptx, int stream);
void dptx_disable_default_video_stream(struct dp_ctrl *dptx, int stream);
int dptx_triger_media_transfer(struct dp_ctrl *dptx, bool benable);
void dptx_enable_dpu_pipeline(struct dp_ctrl *dptx);
int dptx_resolution_switch(struct dp_ctrl *dptx, enum dptx_hot_plug_type etype);
int dptx_video_ts_calculate(struct dp_ctrl *dptx, int lane_num, int rate,
	int bpc, int encoding, int pixel_clock);
/* disable stream when hotunplug, and config dss and notify hwc */
void dptx_link_close_stream(struct dp_ctrl *dptx);

void dptx_hdr_infoframe_set_reg(struct dp_ctrl *dptx, uint8_t enable);
void dptx_audio_infoframe_sdp_send(struct dp_ctrl *dptx);

void dptx_video_core_config(struct dp_ctrl *dptx, int stream);
void dptx_sdp_config(struct dp_ctrl *dptx, int stream, enum dptx_sdp_config_type sdp_type);
void dptx_video_set_core_bpc(struct dp_ctrl *dptx, int stream);
void dptx_video_set_sink_bpc(struct dp_ctrl *dptx, int stream);
void dptx_video_set_sink_col(struct dp_ctrl *dptx, int stream);
void dptx_video_bpc_change(struct dp_ctrl *dptx, int stream);
void dptx_video_pattern_change(struct dp_ctrl *dptx, int stream);
//void dptx_audio_clock_freq_change(struct dp_ctrl *dptx, uint8_t freq_id);

void dptx_program_dsc_bits_perpixel(struct dp_ctrl *dptx);

int dptx_link_timing_config(struct dp_ctrl *dptx);
void dptx_video_timing_change(struct dp_ctrl *dptx, int stream);
void dptx_link_set_lane_after_training(struct dp_ctrl *dptx);
void dptx_video_ts_change(struct dp_ctrl *dptx, int stream);
void dptx_phy_set_pattern(struct dp_ctrl *dptx, uint32_t pattern);

/* #define DPTX_COMBO_PHY */
/* To enable audio during hotplug */
void dptx_audio_sdp_en(struct dp_ctrl *dptx);
void dptx_audio_timestamp_sdp_en(struct dp_ctrl *dptx);

int dptx_fec_enable(struct dp_ctrl *dptx, bool fec_enable);
#endif /* HIDPTX_DP_AVGEN_H */
