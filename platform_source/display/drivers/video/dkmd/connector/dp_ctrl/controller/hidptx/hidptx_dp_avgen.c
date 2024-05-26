/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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

#include "dp_ctrl.h"
#include "hidptx_dp_avgen.h"
#include "dp_avgen_base.h"
#include "hidptx_dp_core.h"
#include "hidptx_reg.h"
#include "dp_core_interface.h"
#include "link/dp_mst_topology.h"
#include "link/dp_dsc_algorithm.h"
#include "link/dp_edid.h"
#include "link/dp_aux.h"
#include "link/drm_dp_helper_additions.h"
#include "dsc/dsc_algorithm_manager.h"
#include "dsc/dsc_config_base.h"

#define OFFSET_FRACTIONAL_BITS 11
#define FLOAT_TU_CARRY_PROTECT 9
#define FLOAT_ACCURACY_CONVERT 100 /* The accuracy of tu need map to 0.00 */
#define FEC_COST_BANDWIDTH_ACCURACY_RECOVER 1000 /* The accuracy of tu need map to 0.00 */
#define FEC_COST_BANDWIDTH_MULTIPLY_THOUSAND 976 /* (0.976 * 1000) fec cost 2.4% bandwidth */

#define SSC_CAUSED_HORIZONTAL_CHANGE_CONVERTOR 10000
#define SSC_CAUSED_HORIZONTAL_CHANGE_FACTOR 9947 /* sdp size with ssc enabled: htotal_int=htotal*0.9947 */
#define SSC_CAUSED_HBLANK_CHANGE_FACTOR 53 /* sdp size with ssc enabled: hblank_int=hblank-hactive*0.0053 */
#define SYMBOLS_PER_LINK_CLOCK 4

/*
 * This Function is used by audio team.
 */
void dptx_audio_infoframe_sdp_send(struct dp_ctrl *dptx)
{
	uint32_t sdp_reg = 0;
	uint8_t sample_freq_cfg = 0;
	uint8_t data_width_cfg = 0;
	uint8_t num_channels_cfg = 0;
	uint8_t speaker_map_cfg = 0;
	uint32_t audio_infoframe_header = AUDIO_INFOFREAME_HEADER;
	uint32_t audio_infoframe_data[3] = {0x00000710, 0x0, 0x0};

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	sample_freq_cfg = dptx_audio_get_sample_freq_cfg(&dptx->aparams);
	audio_infoframe_data[0] |= sample_freq_cfg << DPTX_AUDIO_SAMPLE_FREQ_SHIFT;

	data_width_cfg = dptx_audio_get_data_width_cfg(&dptx->aparams);
	audio_infoframe_data[0] |= data_width_cfg << DPTX_AUDIO_SAMPLE_SIZE_SHIFT;

	num_channels_cfg = dptx_audio_get_num_channels_cfg(&dptx->aparams);
	audio_infoframe_data[0] |= num_channels_cfg << DPTX_AUDIO_CHANNEL_CNT_SHIFT;

	speaker_map_cfg = dptx_audio_get_speaker_map_cfg(&dptx->aparams);
	audio_infoframe_data[0] |= speaker_map_cfg << DPTX_AUDIO_SPEAKER_MAPPING_SHIFT;
	dpu_pr_debug("[DP] audio_infoframe_data[0] after = %x", audio_infoframe_data[0]);

	dptx->sdp_list[0].payload[0] = audio_infoframe_header;
	sdp_reg = 0;
	dptx_writel(dptx, dptx_stream_audio_infoframe_sdp(sdp_reg, 0), audio_infoframe_header);
	dptx_writel(dptx, dptx_stream_audio_infoframe_sdp(sdp_reg, 0) + 4, audio_infoframe_data[0]);
	dptx_writel(dptx, dptx_stream_audio_infoframe_sdp(sdp_reg, 0) + 8, audio_infoframe_data[1]);
	dptx_writel(dptx, dptx_stream_audio_infoframe_sdp(sdp_reg, 0) + 12, audio_infoframe_data[2]);

	if (dptx->dptx_sdp_config)
		dptx->dptx_sdp_config(dptx, 0, SDP_CONFIG_TYPE_AUDIO);
}

void dptx_en_audio_channel(struct dp_ctrl *dptx, int ch_num, int enable)
{
}

/* depends on dptx_dp_colorbar_config DPTX_COLOR_BAR_AUDIO_TEST_EN */
#ifdef AUDIO_TEST
static void dptx_audio_test_ctrl_config(struct dp_ctrl *dptx,  int stream)
{
	uint32_t reg;

	/* Configure audio test ctrl register */
	reg = 0;
	reg |= 1 << 24;
	dptx_writel(dptx, dptx_audio_test_ctrl0_stream(stream), reg);
	dptx_writel(dptx, dptx_audio_test_ctrl1_stream(stream), reg);
}

static void dptx_audio_test_core_config(struct dp_ctrl *dptx)
{
	uint32_t reg;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	/* Configure audio ctrl register */
	reg = 0;
	reg |= 0x1 << DPTX_AUDIO_IN_ENABLE_SHIFT;
	reg |= 0x1 << DPTX_AUDIO_CH_NUM_SHIFT;
	reg |= 0x12 << DPTX_AUDIO_VERSION_NUM_SHIFT;

	dptx_writel(dptx, dptx_audio_ctrl_stream(0), reg);
}
#endif

void dptx_audio_core_config(struct dp_ctrl *dptx)
{
	struct audio_params *aparams = NULL;
	uint32_t reg = 0;
	uint32_t ch_num_cfg;
	uint32_t sdp_header = 0;
	uint32_t version_num = 0x12;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	aparams = &dptx->aparams;

	if (aparams->num_channels <= 2)
		reg |= 0x1;
	else if (aparams->num_channels <= 4)
		reg |= 0x3;
	else if (aparams->num_channels <= 6)
		reg |= 0x7;
	else if (aparams->num_channels <= 8)
		reg |= 0xf;

	if (aparams->data_width == 24)
		reg |= 1 << DPTX_AUDIO_DATA_WIDTH_SHIFT;

	reg &= ~DPTX_AUDIO_HBR_MODE_EN;

	if (aparams->num_channels > 0 && aparams->num_channels <= 8)
		ch_num_cfg = aparams->num_channels - 1;
	else
		ch_num_cfg = 1;
	reg |= ch_num_cfg << DPTX_AUDIO_CH_NUM_SHIFT;

	if (aparams->mute)
		reg |= DPTX_AUDIO_MUTE_EN;
	else
		reg &= ~DPTX_AUDIO_MUTE_EN;

	reg |= sdp_header << DPTX_AUDIO_HB0_SHIFT;

	reg |= version_num << DPTX_AUDIO_VERSION_NUM_SHIFT;

	dptx_writel(dptx, dptx_audio_ctrl_stream(0), reg);
}

void dptx_audio_config(struct dp_ctrl *dptx)
{
	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

#ifndef AUDIO_TEST
	dpu_pr_info("[DP] audio config");
	dptx_audio_core_config(dptx);
	if (dptx->dptx_audio_infoframe_sdp_send)
		dptx->dptx_audio_infoframe_sdp_send(dptx);
#else
	dpu_pr_info("[DP] audio test");
	dptx_audio_test_ctrl_config(dptx, 0);
	dptx_audio_test_core_config(dptx);
#endif
}

void dptx_hdr_infoframe_set_reg(struct dp_ctrl *dptx, uint8_t enable)
{
	int i;
	uint32_t reg;
	struct sdp_full_data hdr_sdp_data;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");
	dpu_check_and_no_retval(!dptx->dptx_enable, err, "[DP] dptx has already off");

	dptx_config_hdr_payload(&hdr_sdp_data, &dptx->hdr_infoframe, enable);

	for (i = 0; i < DPTX_SDP_LEN; i++) {
		dpu_pr_debug("[DP] hdr_sdp_data.payload[%d]: %x", i, hdr_sdp_data.payload[i]);
		dptx_writel(dptx, dptx_stream_audio_infoframe_sdp(0, 0) +  4 * i, hdr_sdp_data.payload[i]);
		reg = (uint32_t)dptx_readl(dptx, dptx_stream_audio_infoframe_sdp(0, 0) + 4 * i);
	}

	if (dptx->dptx_sdp_config)
		dptx->dptx_sdp_config(dptx, 0, SDP_CONFIG_TYPE_HDR);
}

static void dptx_mst_vhs_config(struct dp_ctrl *dptx, int stream)
{
	uint32_t reg;
	struct video_params *vparams = &dptx->vparams;
	struct dtd *mdtd = &vparams->mdtd;

	reg = dptx_readl(dptx, dptx_video_horizontal_size(stream));
	reg &= ~GENMASK(31, 0); /* clear to zero */
	reg |= (mdtd->h_blanking * 24 / 8) * 9947 / 10000; /* 24: if pinfo->bpp is rgb888 than bpp is 24 */
	reg |= ((mdtd->h_active + mdtd->h_blanking) * 24 / 8 * 9947 / 10000) << 16;
	dptx_writel(dptx, dptx_video_horizontal_size(stream), reg);
}

static void dptx_sst_vhs_config(struct dp_ctrl *dptx, int stream)
{
	uint32_t reg;
	uint32_t val;
	uint32_t link_rate;
	uint32_t htotal_int;
	uint32_t hblank_int;
	struct video_params *vparams = &dptx->vparams;
	struct dtd *mdtd = &vparams->mdtd;

	/* Configure video_horizontal_size register */
	switch (dptx->link.rate) {
	case DPTX_PHYIF_CTRL_RATE_RBR:
		link_rate = 162;
		break;
	case DPTX_PHYIF_CTRL_RATE_HBR:
		link_rate = 270;
		break;
	case DPTX_PHYIF_CTRL_RATE_HBR2:
		link_rate = 540;
		break;
	case DPTX_PHYIF_CTRL_RATE_HBR3:
		link_rate = 810;
		break;
	default:
		link_rate = 162;
		break;
	}

	reg = 0;
	/*
	 * horizontal size set
	 * For 4 16 (53/10000) (9947/10000), pls refer to 0x124 set in register manual
	 */
	hblank_int = mdtd->h_blanking - mdtd->h_active *
		SSC_CAUSED_HBLANK_CHANGE_FACTOR / SSC_CAUSED_HORIZONTAL_CHANGE_CONVERTOR;
	val = (uint32_t)(hblank_int * link_rate);
	reg |= (val / (SYMBOLS_PER_LINK_CLOCK * mdtd->pixel_clock / 1000)) * /* 1000 : pixel_clock unit from K -> M */
		SSC_CAUSED_HORIZONTAL_CHANGE_FACTOR / SSC_CAUSED_HORIZONTAL_CHANGE_CONVERTOR;
	htotal_int = (mdtd->h_active + mdtd->h_blanking) *
		SSC_CAUSED_HORIZONTAL_CHANGE_FACTOR / SSC_CAUSED_HORIZONTAL_CHANGE_CONVERTOR;
	val = (uint32_t)(htotal_int * link_rate);
	reg |= (val / (SYMBOLS_PER_LINK_CLOCK * mdtd->pixel_clock / 1000)) << DPTX_VIDEO_HORIZONTAL_SIZE_HTOTAL_OFFSET;

	dptx_writel(dptx, dptx_video_horizontal_size(0), reg); /* stream 0 */
}

static void dptx_pps_sdp_config(struct dp_ctrl *dptx, int stream)
{
	uint32_t reg;

	/* Configure sdp_ctrl */
	reg = dptx_readl(dptx, dptx_sdp_ctrl(stream));
	reg &= ~DPTX_CFG_STREAM_PPS_SDP_TX_MODE; /* auto mode */
	/* reg |= DPTX_CFG_STREAM_PPS_SDP_TX_MODE; manual mode */
	dptx_writel(dptx, dptx_sdp_ctrl(stream), reg);
}

static void dptx_audio_sdp_config(struct dp_ctrl *dptx, int stream)
{
	uint32_t reg;

	/* Configure sdp_ctrl */
	reg = (uint32_t)dptx_readl(dptx, dptx_sdp_ctrl(0));
	reg &= ~DPTX_CFG_STREAM_AUDIO_SDP_TX_MODE;
	dptx_writel(dptx, dptx_sdp_ctrl(0), reg);
}

static void dptx_infoframe_sdp_config(struct dp_ctrl *dptx, int stream)
{
	uint32_t reg;

	reg = (uint32_t)dptx_readl(dptx, dptx_sdp_ctrl(0));
	reg &= ~DPTX_CFG_STREAM_INFORMFRAME0_SDP_TX_MODE;
	dptx_writel(dptx, dptx_sdp_ctrl(0), reg);
}

void dptx_sdp_config(struct dp_ctrl *dptx, int stream, enum dptx_sdp_config_type sdp_type)
{
	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	if (dptx->mst)
		dptx_mst_vhs_config(dptx, stream);
	else
		dptx_sst_vhs_config(dptx, stream);

	if (sdp_type == SDP_CONFIG_TYPE_DSC) {
		dptx_pps_sdp_config(dptx, stream); /* dsc mode */
	} else if (sdp_type == SDP_CONFIG_TYPE_HDR) {
		dptx_infoframe_sdp_config(dptx, stream); /* hdr mode */
	} else if (sdp_type == SDP_CONFIG_TYPE_AUDIO) {
		dptx_audio_sdp_config(dptx, stream); /* audio mode */
		dptx_infoframe_sdp_config(dptx, stream);
	} else {
		dpu_pr_err("[DP] sdp type error");
	}
}

/*
 * video Generation
 */
void dptx_video_timing_change(struct dp_ctrl *dptx, int stream)
{
	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	if (dptx->dptx_disable_default_video_stream)
		dptx->dptx_disable_default_video_stream(dptx, stream);
	if (dptx->dptx_video_core_config)
		dptx->dptx_video_core_config(dptx, stream);
	if (dptx->dptx_video_ts_change)
		dptx->dptx_video_ts_change(dptx, stream);
	if (dptx->dsc && dptx->dptx_sdp_config)
		dptx->dptx_sdp_config(dptx, stream, SDP_CONFIG_TYPE_DSC);

	if (dptx->dptx_enable_default_video_stream)
		dptx->dptx_enable_default_video_stream(dptx, stream);
}

void dptx_mst_enable(struct dp_ctrl *dptx)
{
	uint32_t ret;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	ret = dptx_readl(dptx, DPTX_MST_CTRL);
	ret |= DPTX_CFG_STREAM_ENABLE0;
	ret |= DPTX_CFG_STREAM_ENABLE1;
	dptx_writel(dptx, DPTX_MST_CTRL, ret);

	/* enable sst1 clock */
	ret = dptx_readl(dptx, DPTX_CLK_CTRL);
	ret |= DPTX_CFG_MST_SST1_CLK_EN;
	dptx_writel(dptx, DPTX_CLK_CTRL, ret);

	/* enable mst */
	ret = dptx_readl(dptx, DPTX_GCTL0);
	ret |= DPTX_CFG_MST_ENABLE;
	dptx_writel(dptx, DPTX_GCTL0, ret);
}

int dptx_fec_enable(struct dp_ctrl *dptx, bool fec_enable)
{
	uint32_t retval_32b;
	uint8_t retval_8b = 0;

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] null pointer");

	if (fec_enable) {
		/* Forward Error Correction flow */
		(void)dptx_read_dpcd(dptx, DP_FEC_CONFIGURATION, &retval_8b);
		retval_8b |= DP_FEC_READY;
		(void)dptx_write_dpcd(dptx, DP_FEC_CONFIGURATION, retval_8b);

		/* Enable forward error correction */
		retval_32b = dptx_readl(dptx, DPTX_GCTL0);
		retval_32b |= DPTX_CFG_PHY_FEC_ENABLE;
		dptx_writel(dptx, DPTX_GCTL0, retval_32b);
	} else {
		/* Disable forward error correction */
		retval_32b = dptx_readl(dptx, DPTX_GCTL0);
		retval_32b &= ~DPTX_CFG_PHY_FEC_ENABLE;
		dptx_writel(dptx, DPTX_GCTL0, retval_32b);

		msleep(100); // wait 100ms to close fec
	}

	return 0;
}

void dptx_video_core_config(struct dp_ctrl *dptx, int stream)
{
	struct video_params *vparams = NULL;
	struct dtd *mdtd = NULL;
	uint32_t reg;
	uint8_t vmode;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	vparams = &dptx->vparams;
	mdtd = &vparams->mdtd;
	vmode = vparams->mode;

	/* Configure video_config0 register */
	reg = 0;
	reg |= mdtd->h_active << DPTX_VIDEO_HACTIVE_SHIFT;
	reg |= mdtd->h_blanking << DPTX_VIDEO_HBLANK_SHIFT;
	dptx_writel(dptx, dptx_video_config0_stream(stream), reg);

	/* Configure video_config1 register */
	reg = 0;
	reg |= mdtd->v_active << DPTX_VIDEO_VACTIVE_SHIFT;
	reg |= mdtd->v_blanking << DPTX_VIDEO_VBLANK_SHIFT;
	dptx_writel(dptx, dptx_video_config1_stream(stream), reg);

	/* Configure video_config2 register */
	reg = 0;
	reg |= mdtd->h_sync_offset << DPTX_VIDEO_HFRONT_PORCH;
	reg |= mdtd->h_sync_pulse_width << DPTX_VIDEO_HSYNC_WIDTH;
	dptx_writel(dptx, dptx_video_config2_stream(stream), reg);

	/* Configure video_config3 register */
	reg = 0;
	reg |= mdtd->v_sync_offset << DPTX_VIDEO_VFRONT_PORCH;
	reg |= mdtd->v_sync_pulse_width << DPTX_VIDEO_VSYNC_WIDTH;
	dptx_writel(dptx, dptx_video_config3_stream(stream), reg);

	if (dptx->dptx_video_ts_change)
		dptx->dptx_video_ts_change(dptx, stream);

	/* Configure video_msa0 register */
	reg = 0;
	reg |= (mdtd->h_blanking - mdtd->h_sync_offset) << DPTX_VIDEO_MSA0_HSTART_SHIFT;
	reg |= (mdtd->v_blanking - mdtd->v_sync_offset) << DPTX_VIDEO_MSA0_VSTART_SHIFT;
	dptx_writel(dptx, dptx_video_msa0_stream(stream), reg);

	/* Configure video_msa1-2 register */
	dptx_video_set_sink_bpc(dptx, stream);

	/* Configure video_ctrl */
	/* configure video mapping */
	dptx_video_set_core_bpc(dptx, stream);

	reg = dptx_readl(dptx, dptx_video_ctrl_stream(stream));
	/* configure polarity */
	reg &= ~DPTX_POL_CTRL_SYNC_POL_MASK;
	if (((1 - mdtd->h_sync_polarity) < 0) || ((1 - mdtd->v_sync_polarity) < 0)) {
		dpu_pr_err("[DP] polarity can't be negative");
		return;
	}
	reg |= ((1 - mdtd->h_sync_polarity) << DPTX_POL_CTRL_H_SYNC_POL_SHIFT);
	reg |= ((1 - mdtd->v_sync_polarity) << DPTX_POL_CTRL_V_SYNC_POL_SHIFT);
	dptx_writel(dptx, dptx_video_ctrl_stream(stream), reg);
}

static void dptx_video_mst_ts_calculate(struct dp_ctrl *dptx, int link_rate, int *tu, int *tu_frac)
{
	uint32_t mst_tu_temp;
	uint32_t mst_tu_frac;
	uint32_t mst_tu_frac_mod;
	int bpp = 24; /* if rgb888 than bpp is 24 rgb101010 : 30 */

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	if ((dptx->link.lanes == 0) || (link_rate == 0)) {
		dpu_pr_err("[DP] lanes or rate cannot be zero");
		return;
	}

	if (!dptx->fec) {
		mst_tu_temp = (64 * (dptx->vparams.mdtd.pixel_clock / 1000) *
			(uint32_t)bpp * FLOAT_ACCURACY_CONVERT) /
			(8 * dptx->link.lanes * (uint32_t)link_rate);
	} else {
		mst_tu_temp = (64 * (dptx->vparams.mdtd.pixel_clock / 1000) *
			(uint32_t)bpp * FLOAT_ACCURACY_CONVERT * FEC_COST_BANDWIDTH_ACCURACY_RECOVER) /
			(8 * dptx->link.lanes * (uint32_t)link_rate *
			FEC_COST_BANDWIDTH_MULTIPLY_THOUSAND);
	}

	*tu = (int)(mst_tu_temp / FLOAT_ACCURACY_CONVERT);

	mst_tu_frac = mst_tu_temp - (*tu) * FLOAT_ACCURACY_CONVERT;

	mst_tu_frac_mod = (mst_tu_frac * 64) % 100;

	*tu_frac = (int)((mst_tu_frac * 64) / FLOAT_ACCURACY_CONVERT);

	if (mst_tu_frac_mod != 0)
		*tu_frac += 1;

	if (*tu_frac == 64) {
		*tu += 1;
		*tu_frac = 0;
	}
}

int dptx_video_ts_calculate(struct dp_ctrl *dptx, int lane_num, int rate,
	int bpc, int encoding, int pixel_clock)
{
	struct video_params *vparams = NULL;
	struct dtd *mdtd = NULL;
	int link_rate;
	int retval = 0;
	int ts;
	int tu;
	int tu_frac;
	int color_dep;
	uint32_t dsc_tu_temp;

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] null pointer");

	vparams = &dptx->vparams;
	mdtd = &vparams->mdtd;

	link_rate = dptx_br_to_link_rate(rate);
	if (link_rate < 0) {
		dpu_pr_debug("Invalid rate param=%d", rate);
		return -EINVAL;
	}

	color_dep = dptx_get_color_depth(bpc, encoding);

	if (lane_num * link_rate == 0) {
		dpu_pr_err("[DP] lane_num=%d, link_rate=%d", lane_num, link_rate);
		return -EINVAL;
	}

	ts = (8 * color_dep * pixel_clock) / (lane_num * link_rate);
	tu = ts / 1000;

	dp_imonitor_set_param(DP_PARAM_TU, &tu);

	tu_frac = ts / 100 - tu * 10;

	if (dptx->dsc) {
		dsc_tu_temp = ((64 * 6 * (dptx->vparams.mdtd.pixel_clock / 1000)) *
			FLOAT_ACCURACY_CONVERT * FEC_COST_BANDWIDTH_ACCURACY_RECOVER) /
			(uint32_t)(lane_num * dptx_dsc_get_clock_div(dptx) * link_rate *
			FEC_COST_BANDWIDTH_MULTIPLY_THOUSAND);

		dsc_tu_temp += FLOAT_TU_CARRY_PROTECT;

		tu = (int)(dsc_tu_temp / FLOAT_ACCURACY_CONVERT);

		tu_frac = ((int32_t)dsc_tu_temp - tu * FLOAT_ACCURACY_CONVERT) / 10;
	}

	/* mst: recalc tu and tu_frac */
	if (dptx->mst)
		dptx_video_mst_ts_calculate(dptx, link_rate, &tu, &tu_frac);

	vparams->init_threshold = 0xf;

	dpu_pr_info("[DP] tu=%d", tu);

	dpu_check_and_return((tu >= 65), -EINVAL, info, "[DP] tu: %d > 65", tu);

	vparams->aver_bytes_per_tu = (uint8_t)tu;
	vparams->aver_bytes_per_tu_frac = (uint8_t)tu_frac;

	return retval;
}

void dptx_video_ts_change(struct dp_ctrl *dptx, int stream)
{
	uint32_t reg;
	struct video_params *vparams = NULL;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	vparams = &dptx->vparams;

	if (dptx->mst) {
		reg = dptx_readl(dptx, DPTX_MST_CTRL);
		reg &= ~GENMASK(11, 0);
		reg |= vparams->aver_bytes_per_tu;
		reg |= vparams->aver_bytes_per_tu_frac << 6;
		dptx_writel(dptx, DPTX_MST_CTRL, reg);
	} else {
		reg = (uint32_t)dptx_readl(dptx, dptx_video_packet_stream(stream));
		reg = reg & (~DPTX_VIDEO_PACKET_TU_MASK);
		reg = reg | (vparams->aver_bytes_per_tu <<
			DPTX_VIDEO_PACKET_TU_SHIFT);

		reg = reg & (~DPTX_VIDEO_PACKET_TU_FRAC_MASK_SST);
		reg = reg | (vparams->aver_bytes_per_tu_frac <<
			DPTX_VIDEO_PACKET_TU_FRAC_SHIFT_SST);

		reg = reg & (~DPTX_VIDEO_PACKET_INIT_THRESHOLD_MASK);
		reg = reg | (vparams->init_threshold <<
			DPTX_VIDEO_PACKET_INIT_THRESHOLD_SHIFT);

		dptx_writel(dptx, dptx_video_packet_stream(stream), reg);
	}
}

void dptx_video_bpc_change(struct dp_ctrl *dptx, int stream)
{
	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	dptx_video_set_core_bpc(dptx, stream);
	dptx_video_set_sink_bpc(dptx, stream);
}

void dptx_video_set_core_bpc(struct dp_ctrl *dptx, int stream)
{
	uint32_t reg;
	uint8_t bpc_mapping = 0, bpc = 0;
	enum pixel_enc_type pix_enc;
	struct video_params *vparams = NULL;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	vparams = &dptx->vparams;
	bpc = vparams->bpc;
	pix_enc = vparams->pix_enc;

	reg = dptx_readl(dptx, dptx_video_ctrl_stream(stream));
	reg &= ~DPTX_VIDEO_CTRL_VMAP_BPC_MASK;

	switch (pix_enc) {
	case RGB:
		if (bpc == COLOR_DEPTH_6)
			bpc_mapping = 8;
		else if (bpc == COLOR_DEPTH_8)
			bpc_mapping = 0;
		else if (bpc == COLOR_DEPTH_10)
			bpc_mapping = 1;
		break;
	case YCBCR444:
		if (bpc == COLOR_DEPTH_8)
			bpc_mapping = 2;
		else if (bpc == COLOR_DEPTH_10)
			bpc_mapping = 3;
		break;
	case YCBCR422:
		if (bpc == COLOR_DEPTH_8)
			bpc_mapping = 4;
		else if (bpc == COLOR_DEPTH_10)
			bpc_mapping = 5;
		break;
	case YCBCR420:
		if (bpc == COLOR_DEPTH_8)
			bpc_mapping = 6;
		else if (bpc == COLOR_DEPTH_10)
			bpc_mapping = 7;
		break;
	default:
		break;
	}

	reg |= (bpc_mapping << DPTX_VIDEO_CTRL_VMAP_BPC_SHIFT);
	dptx_writel(dptx, dptx_video_ctrl_stream(stream), reg);
}

void dptx_video_set_sink_col(struct dp_ctrl *dptx, int stream)
{
	uint32_t reg_msa1;
	uint8_t col_mapping;
	uint8_t colorimetry;
	uint8_t dynamic_range;
	struct video_params *vparams = NULL;
	enum pixel_enc_type pix_enc;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	vparams = &dptx->vparams;
	pix_enc = vparams->pix_enc;
	colorimetry = vparams->colorimetry;
	dynamic_range = vparams->dynamic_range;

	reg_msa1 = dptx_readl(dptx, dptx_video_msa1_stream(stream));
	reg_msa1 &= ~DPTX_VIDEO_VMSA1_COL_MASK;

	col_mapping = 0;

	/* According to Table 2-94 of DisplayPort spec 1.3 */
	switch (pix_enc) {
	case RGB:
		if (dynamic_range == CEA)
			col_mapping = 4;
		else if (dynamic_range == VESA)
			col_mapping = 0;
		break;
	case YCBCR422:
		if (colorimetry == ITU601)
			col_mapping = 5;
		else if (colorimetry == ITU709)
			col_mapping = 13;
		break;
	case YCBCR444:
		if (colorimetry == ITU601)
			col_mapping = 6;
		else if (colorimetry == ITU709)
			col_mapping = 14;
		break;
	case RAW:
		col_mapping = 1;
		break;
	case YCBCR420:
	case YONLY:
		break;
	default:
		break;
	}

	reg_msa1 |= (col_mapping << DPTX_VIDEO_VMSA1_COL_SHIFT);
	dptx_writel(dptx, dptx_video_msa1_stream(stream), reg_msa1);
}

void dptx_video_set_sink_bpc(struct dp_ctrl *dptx, int stream)
{
	uint32_t reg_msa1, reg_msa2;
	uint8_t bpc_mapping = 0, bpc = 0;
	struct video_params *vparams = NULL;
	enum pixel_enc_type pix_enc;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	vparams = &dptx->vparams;
	pix_enc = vparams->pix_enc;
	bpc = vparams->bpc;

	reg_msa1 = dptx_readl(dptx, dptx_video_msa1_stream(stream));
	reg_msa2 = dptx_readl(dptx, dptx_video_msa2_stream(stream));

	reg_msa1 &= ~DPTX_VIDEO_VMSA1_BPC_MASK;
	reg_msa2 &= ~DPTX_VIDEO_VMSA2_PIX_ENC_MASK;

	bpc_mapping = dptx_get_sink_bpc_mapping(pix_enc, bpc);

	switch (pix_enc) {
	case RGB:
	case YCBCR444:
	case YCBCR422:
		break;
	case YCBCR420:
		reg_msa2 |= 1 << DPTX_VIDEO_VMSA2_PIX_ENC_YCBCR420_SHIFT;
		break;
	case YONLY:
		/* According to Table 2-94 of DisplayPort spec 1.3 */
		reg_msa2 |= 1 << DPTX_VIDEO_VMSA2_PIX_ENC_SHIFT;
		break;
	case RAW:
		 /* According to Table 2-94 of DisplayPort spec 1.3 */
		reg_msa2 |= (1 << DPTX_VIDEO_VMSA2_PIX_ENC_SHIFT);
		break;
	default:
		break;
	}
	reg_msa1 |= (bpc_mapping << DPTX_VIDEO_VMSA1_BPC_SHIFT);

	dptx_writel(dptx, dptx_video_msa1_stream(stream), reg_msa1);
	dptx_writel(dptx, dptx_video_msa2_stream(stream), reg_msa2);
	dptx_video_set_sink_col(dptx, stream);
}

void dptx_disable_default_video_stream(struct dp_ctrl *dptx, int stream)
{
	uint32_t vsamplectrl;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	vsamplectrl = dptx_readl(dptx, dptx_video_ctrl_stream(stream));
	vsamplectrl &= ~DPTX_VIDEO_CTRL_STREAM_EN;
	dptx_writel(dptx, dptx_video_ctrl_stream(stream), vsamplectrl);

	if ((dptx->dptx_vr) && (dptx->dptx_detect_inited)) {
		dpu_pr_info("[DP] Cancel dptx detect err count when disable video stream");
		hrtimer_cancel(&dptx->dptx_hrtimer);
	}
}

void dptx_enable_default_video_stream(struct dp_ctrl *dptx, int stream)
{
	uint32_t vsamplectrl;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	vsamplectrl = dptx_readl(dptx, dptx_video_ctrl_stream(stream));
	vsamplectrl |= DPTX_VIDEO_CTRL_STREAM_EN;
	dptx_writel(dptx, dptx_video_ctrl_stream(stream), vsamplectrl);

	if ((dptx->dptx_vr) && (dptx->dptx_detect_inited)) {
		dpu_pr_info("[DP] restart dptx detect err count when enable video stream");
		hrtimer_restart(&dptx->dptx_hrtimer);
	}
}

static void dptx_timing_config(struct dp_ctrl *dptx)
{
	/* video data can' be sent before DPTX timing configure. */
	if (dptx->dptx_triger_media_transfer)
		dptx->dptx_triger_media_transfer(dptx, false);

	/* Update DP reg configure */
	if (dptx->dptx_video_timing_change)
		dptx->dptx_video_timing_change(dptx, 0); /* dptx video reg depends on dss pixel clock */

	if (dptx->mst) {
		if (dptx->dptx_video_timing_change)
			dptx->dptx_video_timing_change(dptx, 1); /* dptx video reg depends on dss pixel clock */
	}

	if (dptx->dsc)
		dptx_dsc_enable(dptx, 0);

	if (dptx->dptx_triger_media_transfer)
		dptx->dptx_triger_media_transfer(dptx, true);
}

int dptx_link_timing_config(struct dp_ctrl *dptx)
{
	int retval = 0;

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is NULL");

	/* enable fec */
	if (dptx->fec && dptx->dptx_fec_enable) {
		retval = dptx_fec_enable(dptx, true);
		if (retval) {
			dpu_pr_err("[DP] fec enable failed!");
			return retval;
		}
	}

	if (dptx->mst) {
		retval = dptx_mst_payload_setting(dptx);
		if (retval)
			return retval;
	}

	if (dptx->dsc) {
		if (dptx->dptx_dsc_check_rx_cap)
			dptx->dptx_dsc_check_rx_cap(dptx);

		if (dptx->dptx_dsc_para_init)
			dptx->dptx_dsc_para_init(dptx);

		if (dptx->dptx_dsc_cfg)
			dptx->dptx_dsc_cfg(dptx);
	}

	/* dp update device to hwc and configue dpu */
	retval = dptx_update_dss_and_hwc(dptx);
	if (retval)
		return retval;

	dptx_timing_config(dptx);

	return 0;
}

static void dptx_enable_timing_gen(struct dp_ctrl *dptx, bool benable)
{
	uint32_t reg;
	uint32_t val;
	struct ldi_panel_info *ldi = NULL;
	struct dkmd_connector_info *pinfo = dptx->connector->conn_info;

	dpu_check_and_no_retval(!pinfo, err, "[DP] pinfo is null pointer");
	ldi = &dptx->connector->ldi;

	val = dptx_readl(dptx, DPTX_GCTL0);
	if (benable) {
		reg = 0;
		if (dptx->dsc && (pinfo->ifbc_type != IFBC_TYPE_NONE))
			reg |= (dptx->vparams.dp_dsc_info.h_active_new << DPTX_TIMING_GEN_HACTIVE_SHIFT);
		else
			reg |= (pinfo->base.xres << DPTX_TIMING_GEN_HACTIVE_SHIFT);
		reg |= (ldi->h_back_porch + ldi->h_front_porch + ldi->h_pulse_width);

		dptx_writel(dptx, dptx_timing_gen_config0_stream(0), reg);
		if (dptx->mst)
			dptx_writel(dptx, dptx_timing_gen_config0_stream(1), reg);

		reg = 0;
		reg |= (ldi->h_front_porch << DPTX_TIMING_GEN_HFP_SHIFT);
		reg |= ldi->h_pulse_width;

		dptx_writel(dptx, dptx_timing_gen_config1_stream(0), reg);
		if (dptx->mst)
			dptx_writel(dptx, dptx_timing_gen_config1_stream(1), reg);
	} else {
		val &= ~DPTX_CFG_TIMING_GEN_ENABLE;
	}
	dptx_writel(dptx, DPTX_GCTL0, val);
}

int dptx_triger_media_transfer(struct dp_ctrl *dptx, bool benable)
{
	struct audio_params *aparams = NULL;

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] null pointer");
	aparams = &(dptx->aparams);

	if (dptx->video_transfer_enable) {
		if (benable) {
			dptx_enable_timing_gen(dptx, true);
			dptx_en_audio_channel(dptx, aparams->num_channels, 1);
		} else {
			dptx_en_audio_channel(dptx, aparams->num_channels, 0);
			dptx_enable_timing_gen(dptx, false);
		}
	}

	return 0;
}

int dptx_resolution_switch(struct dp_ctrl *dptx, enum dptx_hot_plug_type etype)
{
	struct dtd *mdtd = NULL;
	struct video_params *vparams = NULL;
	struct ldi_panel_info *ldi = NULL;
	int ret;

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is NULL");

	vparams = &(dptx->vparams);
	mdtd = &(dptx->vparams.mdtd);
	ldi = &(dptx->connector->ldi);

	ret = dptx_update_panel_info(dptx);
	if (ret < 0)
		return ret;

	ldi->hsync_plr = 1 - mdtd->h_sync_polarity;
	ldi->vsync_plr = 1 - mdtd->v_sync_polarity;

	if (dptx->dsc) {
		ldi->pxl_clk_rate = ldi->pxl_clk_rate / (uint64_t)dptx_dsc_get_clock_div(dptx);
		dptx_dsc_dss_config(dptx);
	} else {
		ldi->pxl_clk_rate = ldi->pxl_clk_rate / 2;
	}
	dptx_debug_resolution_info(dptx);

	ret = dptx_dss_plugin(dptx, etype);

	return ret;
}

void dptx_link_set_lane_after_training(struct dp_ctrl *dptx)
{
	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	if (dptx->dptx_phy_enable_xmit)
		dptx->dptx_phy_enable_xmit(dptx, dptx->link.lanes, true);

	dptx->link.trained = true;

	if (dptx->dptx_video_ts_change)
		dptx->dptx_video_ts_change(dptx, 0);

	dpu_pr_info("[DP] Link training succeeded rate=%d lanes=%d",
		dptx->link.rate, dptx->link.lanes);
}

void dptx_link_close_stream(struct dp_ctrl *dptx)
{
	dpu_check_and_no_retval(!dptx, err, "[DP] dptx is NULL");

	dpu_pr_info("[DP] +");

	dptx_video_params_reset(&dptx->vparams);
	dptx_audio_params_reset(&dptx->aparams);

	/* Notify hwc */
	dp_send_cable_notification(dptx, (dptx->dptx_vr) ? HOT_PLUG_OUT_VR : HOT_PLUG_OUT);
	msleep(100);

	if (dptx->fec && dptx->dptx_fec_enable)
		dptx->dptx_fec_enable(dptx, false);

	/* Disable DPTX video */
	if (dptx->dptx_disable_default_video_stream)
		dptx->dptx_disable_default_video_stream(dptx, 0);

	/* set 0xA0 to 0 */
	dptx_phy_data_lane_clear(dptx);

	dptx_phy_lane_reset(dptx, false);
	dptx_phy_set_lanes_power(dptx, false);

	/* set 0x100 to 0x40 */
	dptx_writel(dptx, dptx_video_ctrl_stream(0), 0x40);

	/* set 0x600 to 0x20 in dptx_core_deinit */

	/* set 0x708 to 0x10 */
	dptx_writel(dptx, DPTX_GCTL0, 0x10);

	/* set 0x700 to 0x3F */
	dptx_writel(dptx, DPTX_RST_CTRL, 0x3F);

	if (dptx->dptx_aux_disreset)
		dptx->dptx_aux_disreset(dptx, false);

	dpu_pr_info("[DP] -");
}
