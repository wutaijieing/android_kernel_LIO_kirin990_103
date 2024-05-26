/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#include <linux/platform_device.h>
#include <linux/platform_drivers/usb/tca.h>
#include <linux/of_irq.h>

#include "hisi_connector_utils.h"
#include "hisi_disp_debug.h"
#include "hisi_drv_dp.h"
#include "hisi_disp_composer.h"
#include "hisi_drv_dp_chrdev.h"
#include "hisi_disp_config.h"

#include "hisi_dss_context.h"

#include "link/dp_link_layer_interface.h"
#include "link/dp_link_training.h"
#include "hisi_dp_struct.h"
#include "controller/hidptx/hidptx_reg.h"
#include "controller/hidptx/hidptx_dp_core.h"
#include "controller/dp_avgen_base.h"
#include <securec.h>

#if defined(CONFIG_PERI_DVFS)
//#include "peri_volt_poll.h"
#endif

#define DTS_HISI_DP "hisilicon,dpu_dp_swing"
#define DTS_COMP_SSC_PPM_VALUE "hisilicon,hisi_dp_ssc_ppm"
#define DTS_DP_AUX_SWITCH "huawei,dp_aux_switch"

struct platform_device *g_dp_pdev[DP_CTRL_NUMS_MAX];
static bool bpress_powerkey;
static bool btrigger_timeout;

uint32_t g_fpga_flag = 1;
uint32_t g_sdp_reg_num = 0;

/* 27M, 720*480 */
/* uint32_t g_timing_code = TIMING_480P;
uint32_t g_pixel_clock = 27000;
uint32_t g_video_code = 3;
uint32_t g_hblank = 138; */

/* 74.25M, 1920*1080 */
uint32_t g_timing_code = TIMING_1080P;
uint32_t g_pixel_clock = 74250;
uint32_t g_video_code = 16;
uint32_t g_hblank = 280;

/* 100M, 1280*720 */
/*uint32_t g_pixel_clock = 100000;
uint32_t g_video_code = 4;
uint32_t g_hblank = 942;*/

uint32_t g_dp_id = 0;
void select_dp(uint32_t id)
{
	if (id >= DP_CTRL_NUMS_MAX)
		id = 0;
	g_dp_id = id;
}
EXPORT_SYMBOL_GPL(select_dp);

uint32_t g_dp_mode = DP_MODE;
void set_dp_mode(uint32_t mode)
{
	if (mode >= DP_MODE_MAX)
		mode = DP_MODE;
	g_dp_mode = mode;

	struct hisi_connector_device *connector_dev = NULL;
	struct hisi_dp_info *dp_info = NULL;
	struct dp_ctrl *dptx = NULL;

	disp_check_and_no_retval((g_dp_pdev[g_dp_id] == NULL), err, "[DP] g_dp_pdev is NULL!\n");

	connector_dev = platform_get_drvdata(g_dp_pdev[g_dp_id]);
	disp_check_and_no_retval((connector_dev == NULL), err, "[DP] connector_dev is NULL!\n");
	dp_info = (struct hisi_dp_info *)connector_dev->master.connector_info;
	disp_check_and_no_retval((dp_info == NULL), err, "[DP] dp_info is NULL!\n");

	dptx = &(dp_info->dp);
	dptx->mode = g_dp_mode;
}
EXPORT_SYMBOL_GPL(set_dp_mode);

uint32_t dptx_select_timing(uint8_t id, uint32_t timing_code)
{
	disp_check_and_return(id >= DP_CTRL_NUMS_MAX, -EINVAL, err, "invalid dptx id\n");
	disp_check_and_return(timing_code >= TIMING_MAX, -EINVAL, err, "invalid timing code\n");

	disp_pr_info("set dptx%d timing to %d\n", id, timing_code);

	g_timing_code = timing_code;
	if (timing_code == TIMING_480P) {
		g_pixel_clock = 27000;
		g_video_code = 3;
		g_hblank = 138;

	} else {
		g_pixel_clock = 74250;
		g_video_code = 16;
		g_hblank = 280;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(dptx_select_timing);

void set_clk_hblank_code(uint32_t clock, uint32_t hblank, uint32_t code)
{
	g_pixel_clock = clock;
	g_hblank = hblank;
	g_video_code = code;

	g_timing_code = (g_pixel_clock == 74250) ? TIMING_1080P : TIMING_480P;
}
EXPORT_SYMBOL_GPL(set_clk_hblank_code);

int dp_debug_read_dpcd(uint32_t addr)
{
	uint8_t byte = 0;
	int retval;
	struct hisi_connector_device *connector_dev = NULL;
	struct hisi_dp_info *dp_info = NULL;
	struct dp_ctrl *dptx = NULL;

	disp_check_and_return((g_dp_pdev[g_dp_id] == NULL), -EINVAL, err, "[DP] g_dp_pdev is NULL!\n");

	connector_dev = platform_get_drvdata(g_dp_pdev[g_dp_id]);
	disp_check_and_return((connector_dev == NULL), -EINVAL, err, "[DP] connector_dev is NULL!\n");
	dp_info = (struct hisi_dp_info *)connector_dev->master.connector_info;
	disp_check_and_return((dp_info == NULL), -EINVAL, err, "[DP] dp_info is NULL!\n");

	dptx = &(dp_info->dp);

	retval = dptx_read_dpcd(dptx, addr, &byte);
	if (retval) {
	       disp_pr_err("[DP] read dpcd fail\n");
	       return retval;
	}
	return (int)byte;
}
EXPORT_SYMBOL_GPL(dp_debug_read_dpcd);

int dp_debug_write_dpcd(uint32_t addr, uint8_t byte)
{
	int retval;
	struct hisi_connector_device *connector_dev = NULL;
	struct hisi_dp_info *dp_info = NULL;
	struct dp_ctrl *dptx = NULL;

	disp_check_and_return((g_dp_pdev[g_dp_id] == NULL), -EINVAL, err, "[DP] g_dp_pdev is NULL!\n");

	connector_dev = platform_get_drvdata(g_dp_pdev[g_dp_id]);
	disp_check_and_return((connector_dev == NULL), -EINVAL, err, "[DP] connector_dev is NULL!\n");
	dp_info = (struct hisi_dp_info *)connector_dev->master.connector_info;
	disp_check_and_return((dp_info == NULL), -EINVAL, err, "[DP] dp_info is NULL!\n");

	dptx = &(dp_info->dp);

	retval = dptx_write_dpcd(dptx, addr, byte);
	if (retval) {
	       disp_pr_err("[DP] write dpcd fail\n");
	       return retval;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(dp_debug_write_dpcd);

void test_fast_lk(uint32_t open)
{
	int retval;
	struct hisi_connector_device *connector_dev = NULL;
	struct hisi_dp_info *dp_info = NULL;
	struct dp_ctrl *dptx = NULL;

	disp_check_and_no_retval((g_dp_pdev[g_dp_id] == NULL), err, "[DP] g_dp_pdev is NULL!\n");

	connector_dev = platform_get_drvdata(g_dp_pdev[g_dp_id]);
	disp_check_and_no_retval((connector_dev == NULL), err, "[DP] connector_dev is NULL!\n");
	dp_info = (struct hisi_dp_info *)connector_dev->master.connector_info;
	disp_check_and_no_retval((dp_info == NULL), err, "[DP] dp_info is NULL!\n");

	dptx = &(dp_info->dp);

	mutex_lock(&dptx->dptx_mutex);
	if (open == 0) {
		retval = dptx_write_dpcd(dptx, DP_SET_POWER, DP_SET_POWER_D3);
		if (retval) {
			disp_pr_err("dptx_write_dpcd return value: %d.\n", retval);
			mutex_unlock(&dptx->dptx_mutex);
			return;
		}
		if (dptx->dptx_disable_default_video_stream)
			dptx->dptx_disable_default_video_stream(dptx, 0);
	} else {
		retval = dptx_write_dpcd(dptx, DP_SET_POWER, DP_SET_POWER_D0);
		if (retval) {
			disp_pr_err("dptx_write_dpcd return value: %d.\n", retval);
			mutex_unlock(&dptx->dptx_mutex);
			return;
		}
		mdelay(10);
		test_fast_link_training(dptx);
		if (dptx->dptx_enable_default_video_stream)
			dptx->dptx_enable_default_video_stream(dptx, 0);
	}

	mutex_unlock(&dptx->dptx_mutex);
}
EXPORT_SYMBOL_GPL(test_fast_lk);

struct sdp_type {
	enum dptx_sdp_config_type config_type;
	uint8_t packet_type;
};

static struct sdp_type g_config2type_fmt[] = {
	{SDP_CONFIG_TYPE_DSC,       0x10}, // 0
	{SDP_CONFIG_TYPE_HDR,       0x87}, // 1
	{SDP_CONFIG_TYPE_AUDIO,     0x84}, // 2
	{SDP_CONFIG_TYPE_PSR,       0x0}, // 3
	{AUDIO_TIMESTAMP,           0x1}, // 4
	{AUDIO_STREAM,              0x2}, // 5
	{EXTENSION_SDP,             0x4}, // 6
	{AUDIO_COPY_MANAGEMENT,     0x5}, // 7
	{ISRC,                      0x6}, // 8
	{VSC,                       0x7}, // 9
	{VSC_EXT_VESA,              0x20}, // 10
	{VSC_EXT_CTA,               0x21}, // 11
	{ADAPTIVE_SYNC,             0x22}, // 12
	{VENDOR_SPECIFIC_INFOFRAME, 0x81}, // 13
	{AVI_INFOFRAME,             0x82}, // 14
	{SPD_INFOFRAME,             0x83}, // 15
	{MPEG_SOURCE_INFOFRAME,     0x85}, // 16
	{NTSC_VBI_INFOFRAME,        0x86}, // 17
};

void test_config_sdp_payload(struct sdp_full_data *hdr_sdp_data, struct hdr_infoframe *hdr_infoframe,
	uint8_t enable, uint8_t packet_type)
{
	disp_check_and_no_retval((hdr_sdp_data == NULL), err, "[DP] hdr_sdp_data is NULL\n");
	disp_check_and_no_retval((hdr_infoframe == NULL), err, "[DP] hdr_infoframe is NULL\n");

	hdr_sdp_data->en = enable;
	hdr_sdp_data->payload[0] = 0x4C1D0000 | (g_config2type_fmt[packet_type].packet_type << 8);
	hdr_sdp_data->payload[1] = (hdr_infoframe->data[1] << 24) | (hdr_infoframe->data[0] << 16) |
		(HDR_INFOFRAME_LENGTH << 8) | HDR_INFOFRAME_VERSION;

	dptx_config_hdr_info_data(hdr_sdp_data, hdr_infoframe);
}

void test_sdp_infoframe_set_reg(struct dp_ctrl *dptx, uint8_t enable, uint8_t packet_type)
{
	int i;
	uint32_t reg;
	struct sdp_full_data hdr_sdp_data;

	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");
	disp_check_and_no_retval((!dptx->dptx_enable), err, "[DP] dptx has already off.\n");

	memset_s(&hdr_sdp_data, sizeof(struct sdp_full_data), 0, sizeof(struct sdp_full_data));
	test_config_sdp_payload(&hdr_sdp_data, &dptx->hdr_infoframe, enable, packet_type);

	for (i = 0; i < DPTX_SDP_LEN; i++) {
		disp_pr_info("[DP] hdr_sdp_data.payload[%d]: %x\n", i, hdr_sdp_data.payload[i]);
		dptx_writel(dptx, DPTX_STREAM_AUDIO_INFOFRAME_SDP(0, 0) +  4 * i, hdr_sdp_data.payload[i]);
	}

	if (dptx->dptx_sdp_config)
		dptx->dptx_sdp_config(dptx, 0, SDP_CONFIG_TYPE_AUDIO);
}

int test_infoframe_sdp_send(uint8_t packet_type)
{
	int i;
	struct hisi_connector_device *connector_dev = NULL;
	struct hisi_dp_info *dp_info = NULL;
	struct dp_ctrl *dptx = NULL;

	disp_check_and_return((g_dp_pdev[g_dp_id] == NULL), -EINVAL, err, "[DP] g_dp_pdev is NULL!\n");

	connector_dev = platform_get_drvdata(g_dp_pdev[g_dp_id]);
	disp_check_and_return((connector_dev == NULL), -EINVAL, err, "[DP] connector_dev is NULL!\n");
	dp_info = (struct hisi_dp_info *)connector_dev->master.connector_info;
	disp_check_and_return((dp_info == NULL), -EINVAL, err, "[DP] dp_info is NULL!\n");

	dptx = &(dp_info->dp);

	dptx->hdr_infoframe.type_code = INFOFRAME_PACKET_TYPE_HDR;
	dptx->hdr_infoframe.version_number = HDR_INFOFRAME_VERSION;
	dptx->hdr_infoframe.length = HDR_INFOFRAME_LENGTH;

	memset_s(dptx->hdr_infoframe.data, HDR_INFOFRAME_LENGTH, 0x00, HDR_INFOFRAME_LENGTH);
	dptx->hdr_metadata.red_primary_x = 6400;
	dptx->hdr_metadata.red_primary_y = 3300;
	dptx->hdr_metadata.green_primary_x = 3000;
	dptx->hdr_metadata.green_primary_y = 6000;
	dptx->hdr_metadata.blue_primary_x = 1500;
	dptx->hdr_metadata.blue_primary_y = 600;
	dptx->hdr_metadata.white_point_x = 2;
	dptx->hdr_metadata.white_point_y = 3290;

	dptx_hdr_calculate_infoframe_data(dptx);

	for (i = 0; i < HDR_INFOFRAME_LENGTH; i++)
		disp_pr_info("[DP] hdr_infoframe->data[%d] = 0x%02x", i, dptx->hdr_infoframe.data[i]);

	mutex_lock(&dptx->dptx_mutex);

	test_sdp_infoframe_set_reg(dptx, 1, packet_type);

	mutex_unlock(&dptx->dptx_mutex);

	return 0;
}
EXPORT_SYMBOL_GPL(test_infoframe_sdp_send);

static void hisi_connector_init_base_addr(struct hisi_connector_base_addr *base_addr)
{
	base_addr->dpu_base_addr = hisi_config_get_ip_base(DISP_IP_BASE_DPU);
	base_addr->peri_crg_base = hisi_config_get_ip_base(DISP_IP_BASE_PERI_CRG);
	base_addr->hsdt1_crg_base = hisi_config_get_ip_base(DISP_IP_BASE_HSDT1_CRG);

	disp_pr_info("[DP] dpu base = 0x%x", base_addr->dpu_base_addr);
	disp_pr_info("[DP] peri base = 0x%x", base_addr->peri_crg_base);
	disp_pr_info("[DP] hsdt1 base = 0x%x", base_addr->hsdt1_crg_base);
}


/*******************************************************************************
 *
 */
int dpu_dptx_get_spec(void *data, unsigned int size, unsigned int *ext_acount)
{
	struct hisi_connector_device *connector_dev = NULL;
	struct hisi_dp_info *dp_info = NULL;
	struct dp_ctrl *dptx = NULL;
	struct edid_audio *audio_info = NULL;

	disp_check_and_return((g_dp_pdev[DPTX_DP0] == NULL), -EINVAL, err, "[DP] g_dp_pdev is NULL!\n");
	connector_dev = platform_get_drvdata(g_dp_pdev[DPTX_DP0]);
	disp_check_and_return((connector_dev == NULL), -EINVAL, err, "[DP] connector_dev is NULL!\n");
	dp_info = (struct hisi_dp_info *)connector_dev->master.connector_info;
	disp_check_and_return((dp_info == NULL), -EINVAL, err, "[DP] dp_info is NULL!\n");

	dptx = &(dp_info->dp);
	audio_info = &dptx->edid_info.Audio;

	if (size < sizeof(struct edid_audio_info) * audio_info->extACount) {
		disp_pr_err("[DP] size is error!size %d extACount %d\n", size, audio_info->extACount);
		return -EINVAL;
	}

	memcpy(data, audio_info->spec, sizeof(struct edid_audio_info) * audio_info->extACount);
	*ext_acount = audio_info->extACount;

	disp_pr_info("[DP] get spec success\n");

	return 0;
}

int dpu_dptx_set_aparam(unsigned int channel_num, unsigned int data_width, unsigned int sample_rate)
{
	uint8_t orig_sample_freq = 0;
	uint8_t sample_freq = 0;
	struct audio_params *aparams = NULL;
	struct hisi_connector_device *connector_dev = NULL;
	struct hisi_dp_info *dp_info = NULL;
	struct dp_ctrl *dptx = NULL;

	if (channel_num > DPTX_CHANNEL_NUM_MAX || data_width > DPTX_DATA_WIDTH_MAX) {
		disp_pr_err("[DP] input param is invalid. channel_num(%d) data_width(%d)\n", channel_num, data_width);
		return -EINVAL;
	}

	disp_check_and_return((g_dp_pdev[DPTX_DP0] == NULL), -EINVAL, err, "[DP] g_dp_pdev is NULL!\n");
	connector_dev = platform_get_drvdata(g_dp_pdev[DPTX_DP0]);
	disp_check_and_return((connector_dev == NULL), -EINVAL, err, "[DP] connector_dev is NULL!\n");
	dp_info = (struct hisi_dp_info *)connector_dev->master.connector_info;
	disp_check_and_return((dp_info == NULL), -EINVAL, err, "[DP] dp_info is NULL!\n");

	dptx = &(dp_info->dp);
	aparams = &dptx->aparams;

	disp_pr_info("[DP] set aparam. channel_num(%d) data_width(%d) sample_rate(%d)\n",
		channel_num, data_width, sample_rate);

	switch (sample_rate) {
	case 32000:
		orig_sample_freq = IEC_ORIG_SAMP_FREQ_32K;
		sample_freq = IEC_SAMP_FREQ_32K;
		break;
	case 44100:
		orig_sample_freq = IEC_ORIG_SAMP_FREQ_44K;
		sample_freq = IEC_SAMP_FREQ_44K;
		break;
	case 48000:
		orig_sample_freq = IEC_ORIG_SAMP_FREQ_48K;
		sample_freq = IEC_SAMP_FREQ_48K;
		break;
	case 88200:
		orig_sample_freq = IEC_ORIG_SAMP_FREQ_88K;
		sample_freq = IEC_SAMP_FREQ_88K;
		break;
	case 96000:
		orig_sample_freq = IEC_ORIG_SAMP_FREQ_96K;
		sample_freq = IEC_SAMP_FREQ_96K;
		break;
	case 176400:
		orig_sample_freq = IEC_ORIG_SAMP_FREQ_176K;
		sample_freq = IEC_SAMP_FREQ_176K;
		break;
	case 192000:
		orig_sample_freq = IEC_ORIG_SAMP_FREQ_192K;
		sample_freq = IEC_SAMP_FREQ_192K;
		break;
	default:
		disp_pr_info("[DP] invalid sample_rate\n");
		return -EINVAL;
	}

	aparams->iec_samp_freq = sample_freq;
	aparams->iec_orig_samp_freq = orig_sample_freq;
	aparams->num_channels = (uint8_t)channel_num;
	aparams->data_width = (uint8_t)data_width;

	if (dptx->dptx_audio_config)
		dptx->dptx_audio_config(dptx);

	disp_pr_info("[DP] set aparam success\n");

	return 0;
}

int dptx_phy_rate_to_bw(uint8_t rate)
{
	switch (rate) {
	case DPTX_PHYIF_CTRL_RATE_RBR:
		return DP_LINK_BW_1_62;
	case DPTX_PHYIF_CTRL_RATE_HBR:
		return DP_LINK_BW_2_7;
	case DPTX_PHYIF_CTRL_RATE_HBR2:
		return DP_LINK_BW_5_4;
	case DPTX_PHYIF_CTRL_RATE_HBR3:
		return DP_LINK_BW_8_1;
	default:
		disp_pr_err("[DP] Invalid rate 0x%x\n", rate);
		return -EINVAL;
	}
}

int dptx_bw_to_phy_rate(uint8_t bw)
{
	switch (bw) {
	case DP_LINK_BW_1_62:
		return DPTX_PHYIF_CTRL_RATE_RBR;
	case DP_LINK_BW_2_7:
		return DPTX_PHYIF_CTRL_RATE_HBR;
	case DP_LINK_BW_5_4:
		return DPTX_PHYIF_CTRL_RATE_HBR2;
	case DP_LINK_BW_8_1:
		return DPTX_PHYIF_CTRL_RATE_HBR3;
	default:
		disp_pr_err("[DP] Invalid bw 0x%x\n", bw);
		return -EINVAL;
	}
}

/*
 * Audio/Video Parameters Reset
 */
void dptx_audio_params_reset(struct audio_params *params)
{
	disp_check_and_no_retval((params == NULL), err, "[DP] NULL Pointer!\n");

	memset(params, 0x0, sizeof(struct audio_params));
	params->iec_channel_numcl0 = 8;
	params->iec_channel_numcr0 = 4;
	params->use_lut = 1;
	params->iec_samp_freq = 3;
	params->iec_word_length = 11;
	params->iec_orig_samp_freq = 12;
	params->data_width = 16;
	params->num_channels = 2;
	params->inf_type = 1;
	params->ats_ver = 17;
	params->mute = 0;
}

uint32_t g_bpc = COLOR_DEPTH_8;
void dp_set_bpc(uint32_t bpc)
{
	g_bpc = bpc;
}
EXPORT_SYMBOL_GPL(dp_set_bpc);

void dptx_video_params_reset(struct video_params *params)
{
	disp_check_and_no_retval((params == NULL), err, "[DP] NULL Pointer!\n");

	memset(params, 0x0, sizeof(struct video_params));

	/* 6 bpc should be default - use 8 bpc for MST calculation */
	params->bpc = g_bpc;
	params->dp_dsc_info.dsc_info.dsc_bpp = 8; /* DPTX_BITS_PER_PIXEL */
	params->pix_enc = RGB;
	params->mode = 1;
	params->colorimetry = ITU601;
	params->dynamic_range = CEA;
	params->aver_bytes_per_tu = 30;
	params->aver_bytes_per_tu_frac = 0;
	params->init_threshold = 15;
	params->pattern_mode = RAMP;
	params->refresh_rate = 60000;
	params->video_format = VCEA;
}

/*
 * DTD reset
 */
void dwc_dptx_dtd_reset(struct dtd *mdtd)
{
	disp_check_and_no_retval((mdtd == NULL), err, "[DP] NULL Pointer!\n");

	mdtd->pixel_repetition_input = 0;
	mdtd->pixel_clock  = 0;
	mdtd->h_active = 0;
	mdtd->h_blanking = 0;
	mdtd->h_sync_offset = 0;
	mdtd->h_sync_pulse_width = 0;
	mdtd->h_image_size = 0;
	mdtd->v_active = 0;
	mdtd->v_blanking = 0;
	mdtd->v_sync_offset = 0;
	mdtd->v_sync_pulse_width = 0;
	mdtd->v_image_size = 0;
	mdtd->interlaced = 0;
	mdtd->v_sync_polarity = 0;
	mdtd->h_sync_polarity = 0;
}

bool dptx_check_low_temperature(struct dp_ctrl *dptx)
{
	uint32_t perictrl4;

	if (g_fpga_flag == 1)
		return 0;

	disp_check_and_return((dptx == NULL), false, err, "[DP] NULL Pointer!\n");

#if DP_TO_DO
	perictrl4 = inp32(hisifd->pmctrl_base + MIDIA_PERI_CTRL4);//lint !e529
	perictrl4 &= PMCTRL_PERI_CTRL4_TEMPERATURE_MASK;
	perictrl4 = (perictrl4 >> PMCTRL_PERI_CTRL4_TEMPERATURE_SHIFT);
	HISI_FB_INFO("[DP] Get current temperature: %d\n", perictrl4);

	if (perictrl4 != NORMAL_TEMPRATURE)
		return true;
	else
		return false;
#endif
	return false; // tmp
}

void dptx_notify(struct dp_ctrl *dptx)
{
	disp_check_and_no_retval((dptx == NULL), err, "[DP] dptx is NULL!\n");

	wake_up_interruptible(&dptx->waitq);
}

void dptx_notify_shutdown(struct dp_ctrl *dptx)
{
	disp_check_and_no_retval((dptx == NULL), err, "[DP] dptx is NULL!\n");

	atomic_set(&dptx->shutdown, 1);
	wake_up_interruptible(&dptx->waitq);
}

int dp_ceil(uint64_t a, uint64_t b)
{
	if (b == 0)
		return -1;

	if (a % b != 0)
		return a / b + 1;

	return a / b;
}

void dp_send_cable_notification(struct dp_ctrl *dptx, int val)
{
	int state = 0;
	struct dtd *mdtd = NULL;
	struct video_params *vparams = NULL;

	disp_check_and_no_retval((dptx == NULL), err, "[DP] dptx is NULL!\n");

#if CONFIG_DP_ENABLE
	state = dptx->sdev.state;
	switch_set_state(&dptx->sdev, val);
	if (dptx->edid_info.Audio.basicAudio == 0x1) {
		if (val == HOT_PLUG_OUT || val == HOT_PLUG_OUT_VR)
			switch_set_state(&dptx->dp_switch, 0);
		else if (val == HOT_PLUG_IN || val == HOT_PLUG_IN_VR)
			switch_set_state(&dptx->dp_switch, 1);
	} else {
		HISI_FB_WARNING("[DP] basicAudio(%ud) no support!\n", dptx->edid_info.Audio.basicAudio);
	}

	vparams = &(dptx->vparams);
	mdtd = &(dptx->vparams.mdtd);

	huawei_dp_update_external_display_timming_info(mdtd->h_active, mdtd->v_active, vparams->m_fps);

	HISI_FB_INFO("[DP] cable state %s %d\n",
		dptx->sdev.state == state ? "is same" : "switched to", dptx->sdev.state);
#endif
}

int hisi_dptx_switch_source(uint32_t user_mode, uint32_t user_format)
{
	int ret = 0;
	struct hisi_connector_device *connector_dev = NULL;
	struct hisi_dp_info *dp_info = NULL;
	struct dp_ctrl *dptx = NULL;
	struct hisi_panel_info *pinfo = NULL;

	disp_check_and_return((g_dp_pdev[DPTX_DP0] == NULL), -EINVAL, err, "[DP] g_dp_pdev is NULL!\n");

	connector_dev = platform_get_drvdata(g_dp_pdev[DPTX_DP0]);
	disp_check_and_return((connector_dev == NULL), -EINVAL, err, "[DP] connector_dev is NULL!\n");
	dp_info = (struct hisi_dp_info *)connector_dev->master.connector_info;
	disp_check_and_return((dp_info == NULL), -EINVAL, err, "[DP] dp_info is NULL!\n");

	dptx = &(dp_info->dp);
	disp_pr_info("[DP] dp%d +\n", dptx->id);

	mutex_lock(&dptx->dptx_mutex);

	if (!dptx->dptx_enable) {
		disp_pr_err("[DP] dptx has already off\n");
		ret = -EINVAL;
		goto fail;
	}

	if (!dptx->video_transfer_enable) {
		disp_pr_err("[DP] dptx never transfer video\n");
		ret = -EINVAL;
		goto fail;
	}

#if CONFIG_DP_ENABLE
	if ((dptx->same_source == huawei_dp_get_current_dp_source_mode()) && (!user_mode) && (!user_format)) {
		disp_pr_err("[DP] dptx don't switch source when the dest mode is same as current!!!\n");
		ret = -EINVAL;
		goto fail;
	}

	dptx->user_mode = user_mode;
	dptx->user_mode_format = (enum video_format_type) user_format;
	dptx->same_source = huawei_dp_get_current_dp_source_mode();
#endif
	disp_pr_info("[DP] dptx user switch: mode (%d); format (%d); same_source (%d).\n",
		dptx->user_mode, dptx->user_mode_format, dptx->same_source);

	pinfo = connector_dev->fix_panel_info;

	/*PC mode change to 1080p*/
	if (((pinfo->xres >= MAX_DIFF_SOURCE_X_SIZE) && (dptx->max_edid_timing_hactive > MAX_DIFF_SOURCE_X_SIZE))
		|| (dptx->user_mode != 0)) {
		/* DP plug out */
		if (dptx->handle_hotunplug)
			dptx->handle_hotunplug(dptx);
		mutex_unlock(&dptx->dptx_mutex);

		msleep(10);

		mutex_lock(&dptx->dptx_mutex);
		if (dptx->handle_hotplug)
			ret = dptx->handle_hotplug(dptx);
		huawei_dp_imonitor_set_param(DP_PARAM_HOTPLUG_RETVAL, &ret);
	}
fail:
	mutex_unlock(&dptx->dptx_mutex);

	disp_pr_info("[DP] dp%d -\n", dptx->id);

	return ret;
}
EXPORT_SYMBOL_GPL(hisi_dptx_switch_source);

static void dp_hpd_status_init(struct dp_ctrl *dptx, TYPEC_PLUG_ORIEN_E typec_orien)
{
	disp_check_and_no_retval((dptx == NULL), err, "[DP] dptx is NULL!\n");

#if CONFIG_DP_ENABLE
	dptx->same_source = huawei_dp_get_current_dp_source_mode();
#endif
	dptx->user_mode = 0;
	dptx->dptx_plug_type = typec_orien;
	dptx->user_mode_format = VCEA;

	/* DP HPD event must be delayed when system is booting*/
	if (!dptx->dptx_gate)
		wait_event_interruptible_timeout(dptx->dptxq, dptx->dptx_gate, msecs_to_jiffies(20000)); /*lint !e666 !e578 */
}

static int dp_get_lanes_mode(TCPC_MUX_CTRL_TYPE mode, uint8_t *dp_lanes)
{
	disp_check_and_return((dp_lanes == NULL), -EINVAL, err, "[DP] dp_lanes is NULL!\n");

	switch (mode) {
	case TCPC_DP:
		*dp_lanes = 4;
		break;
	case TCPC_USB31_AND_DP_2LINE:
		*dp_lanes = 2;
		break;
	default:
		disp_pr_err("[DP] not supported tcpc_mux_ctrl_type=%d\n", mode);
		return -EINVAL;
	}

	return 0;
}

int dpu_dptx_hpd_trigger(TCA_IRQ_TYPE_E irq_type, TCPC_MUX_CTRL_TYPE mode, TYPEC_PLUG_ORIEN_E typec_orien)
{
	struct hisi_connector_device *connector_dev = NULL;
	struct hisi_dp_info *dp_info = NULL;
	struct dp_ctrl *dptx = NULL;
	uint8_t dp_lanes = 0;
	int ret = 0;

	disp_check_and_return((g_dp_id >= DP_CTRL_NUMS_MAX), -EINVAL, err, "[DP] Invalid dp id!\n");
	disp_check_and_return((g_dp_pdev[g_dp_id] == NULL), -EINVAL, err, "[DP] g_dp_pdev is NULL!\n");

	connector_dev = platform_get_drvdata(g_dp_pdev[g_dp_id]);
	disp_check_and_return((connector_dev == NULL), -EINVAL, err, "[DP] connector_dev is NULL!\n");
	dp_info = (struct hisi_dp_info *)connector_dev->master.connector_info;
	disp_check_and_return((dp_info == NULL), -EINVAL, err, "[DP] dp_info is NULL!\n");

	dptx = &(dp_info->dp);
	disp_pr_info("[DP] dp%d +\n", dptx->id);

	if (dptx->dp_shutdown == true) {
		disp_pr_info("[DP] dp already shutdown, just return");
		return 0;
	}

	if (!dptx->dptx_enable && irq_type != TCA_IRQ_HPD_IN) {
		disp_pr_info("[DP] dptx has already off\n");
		return -EINVAL;
	}

	switch (irq_type) {
	case TCA_IRQ_HPD_OUT:
		ret = dp_off(dptx);
		if (ret) {
			disp_pr_err("[DP] dp off fail\n");
		}
		return ret;
		break;
	case TCA_IRQ_HPD_IN:
		if (dptx->video_transfer_enable)
			return 0;
		// FIXME: clear active flag in composer on
		hisi_vactive0_set_event(0);
		ret = dp_on(dptx);
		if (ret) {
			disp_pr_err("[DP] dp on fail\n");
			return ret;
		}
		break;
	default:
		break;
	}

	mutex_lock(&dptx->dptx_mutex);

	dptx->dptx_gate = 1;
	disp_pr_info("[DP] DP HPD Type: %d, Mode: %d, Gate: %d\n", irq_type, mode, dptx->dptx_gate);

	dp_hpd_status_init(dptx, typec_orien);
	ret = dp_get_lanes_mode(mode, &dp_lanes);
	if (ret) {
		mutex_unlock(&dptx->dptx_mutex);
		return ret;
	}
	dptx->max_lanes = dp_lanes;

	switch (irq_type) {
	case TCA_IRQ_HPD_OUT:
		if (dptx->dptx_hpd_handler)
			dptx->dptx_hpd_handler(dptx, false, dp_lanes);
		break;
	case TCA_IRQ_HPD_IN:
		if (dptx->dptx_hpd_handler)
			dptx->dptx_hpd_handler(dptx, true, dp_lanes);
		break;
	case TCA_IRQ_SHORT:
		if (dptx->dptx_hpd_irq_handler)
			dptx->dptx_hpd_irq_handler(dptx);
		break;
	default:
		disp_pr_err("[DP] not supported tca_irq_type=%d\n", irq_type);
		ret = -EINVAL;
	}

	mutex_unlock(&dptx->dptx_mutex);

	disp_pr_info("[DP] dp%d -\n", dptx->id);
	return ret;
}
EXPORT_SYMBOL_GPL(dpu_dptx_hpd_trigger);

static struct dp_ctrl * dptx_get_ctrl(uint32_t local_id)
{
	struct platform_device *pdev;
	struct hisi_connector_device *connector_data = NULL;
	struct hisi_dp_info *dp_info = NULL;

	pdev = g_dp_pdev[local_id];
	if (!pdev) {
		disp_pr_err("dptx%d not probed yet\n", local_id);
		return NULL;
	}

	connector_data = platform_get_drvdata(pdev);
	dp_info = (struct hisi_dp_info *)connector_data->master.connector_info;
	disp_check_and_return(!dp_info , NULL, err, "[DP] dp infor is NULL!\n");
	disp_check_and_return((dp_info->dp.id >= DP_CTRL_NUMS_MAX), NULL, err, "[DP] Invalid dp id!\n");

	return &dp_info->dp;
}

void set_dp_rx_tx_mode(uint32_t dp_rx_tx_mode)
{
	uint32_t i;
	struct dp_ctrl *dptx;
	for (i = 0; i < DP_CTRL_NUMS_MAX; i++) {
		dptx = dptx_get_ctrl(i);
		if (!dptx)
			continue;

		disp_pr_info("set DPRX-DPTX Mode %s for dptx%d\n", dp_rx_tx_mode ? "on" : "off", i);
		dptx->source_is_dprx = dp_rx_tx_mode;
		dptx->source_is_external = dp_rx_tx_mode;
	}
}
EXPORT_SYMBOL_GPL(set_dp_rx_tx_mode);

void set_dp_hdmirx_tx_mode(uint32_t dp_rx_tx_mode)
{
	uint32_t i;
	struct dp_ctrl *dptx;
	for (i = 0; i < DP_CTRL_NUMS_MAX; i++) {
		dptx = dptx_get_ctrl(i);
		if (!dptx)
			continue;

		disp_pr_info("set hdmiRX-DPTX Mode %s for dptx%d\n", dp_rx_tx_mode ? "on" : "off", i);
		dptx->source_is_dprx = 0;
		dptx->source_is_external = dp_rx_tx_mode;
	}
}
EXPORT_SYMBOL_GPL(set_dp_hdmirx_tx_mode);

static bool dss_is_dprx(uint32_t inputsource)
{
	return inputsource == INPUT_SOURCE_DPRX0 || inputsource == INPUT_SOURCE_DPRX1;
}

static bool dss_is_hdmirx(uint32_t inputsource)
{
	return inputsource == INPUT_SOURCE_HDMIIN;
}

static void dptx_connect(tx_context_t tx_ctx, uint32_t inputsource_id, notify_dsscontext_func event_notify)
{
	struct dp_ctrl *dptx = (struct dp_ctrl*)tx_ctx;

	disp_pr_info("dptx_connect dptx%d, source is %d\n", dptx->id, inputsource_id);

	if (!dptx) {
		disp_pr_err("invalid context\n");
		return;
	}

	dptx->source_is_dprx = dss_is_dprx(inputsource_id);
	dptx->source_is_external = dptx->source_is_dprx || dss_is_hdmirx(inputsource_id);
	dptx->notify_dsscontext = event_notify;

	select_dp(dptx->id);

	dpu_dptx_hpd_trigger(TCA_IRQ_HPD_IN, TCPC_DP, DP_PLUG_TYPE_NORMAL);
}

static void dptx_disconnect(tx_context_t tx_ctx)
{
	struct dp_ctrl *dptx = (struct dp_ctrl*)tx_ctx;

	if (!dptx) {
		disp_pr_err("invalid context\n");
		return;
	}

	disp_pr_info("dptx_disconnect %d\n", dptx->id);

	select_dp(dptx->id);
	dpu_dptx_hpd_trigger(TCA_IRQ_HPD_OUT, TCPC_DP, DP_PLUG_TYPE_NORMAL);
}

static void dptx_get_information(tx_context_t tx_ctx, dss_intfinfo_t *info)
{
	struct dp_ctrl *dptx = (struct dp_ctrl*)tx_ctx;

	if (!dptx) {
		disp_pr_err("invalid context\n");
		return;
	}

	disp_pr_info("dptx%d get information\n", dptx->id);

	if (!info) {
		disp_pr_err("invalid parameter\n");
		return;
	}

	info->timing_code = g_timing_code;
	if (g_timing_code == TIMING_480P) {
		info->xres = 720;
		info->yres = 480;
		info->framerate = 60;
	} else {
		info->xres = 1920;
		info->yres = 1080;
		info->framerate = 30;
	}
}

static void dptx_notify_dss(struct dp_ctrl *dptx, uint32_t local_event_id)
{
	uint32_t msg_id;
	if (!dptx->notify_dsscontext) {
		disp_pr_info("none event notify registered\n");
		return;
	}

	if (local_event_id == CONNECTED) {
		msg_id = DISPLAYSINK_CONNECTED;
	} else if (local_event_id == VIDEO_ACTIVATED) {
		msg_id = DISPLAYSINK_VIDEO_ACTIVATED;
	} else {
		msg_id = DISPLAYSINK_DISCONNECTED;
	}

	disp_pr_info("dptx%d notify %d\n", dptx->id, msg_id);
	dptx->notify_dsscontext(dptx->id + DISPLAY_SINK_DPTX0, msg_id);
}

static void dptx_register_displaysink(struct dp_ctrl *dptx)
{
	dss_displaysink_t sink_data = {
		.tx_ctx = (tx_context_t)dptx,
		.enable_displaysink = dptx_connect,
		.disable_displaysink = dptx_disconnect,
		.get_information = dptx_get_information,
	};

	dss_register_displaysink(dptx->id + DISPLAY_SINK_DPTX0, &sink_data);
}

/*******************************************************************************
 *
 */

static void dp_on_params_init(struct dp_ctrl *dptx)
{
	disp_check_and_no_retval((dptx == NULL), err, "[DP] dptx is NULL!\n");

	dptx->dptx_enable = true;
	dptx->detect_times = 0;
	dptx->current_link_rate = dptx->max_rate;
	dptx->current_link_lanes = dptx->max_lanes;
	bpress_powerkey = false;
	btrigger_timeout = false;
}

int dp_on(struct dp_ctrl *dptx)
{
	int ret = 0;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");
	disp_check_and_return((dptx->id >= DP_CTRL_NUMS_MAX), -EINVAL, err, "[DP] Invalid dp id!\n");

	disp_pr_info("[DP] dp%d, +\n", dptx->id);

	mutex_lock(&dptx->dptx_mutex);

	if (dptx->dptx_enable) {
		disp_pr_info("[DP] dptx has already on\n");
		mutex_unlock(&dptx->dptx_mutex);
		return 0;
	}

	if (dptx->dp_dis_reset)
		ret = dptx->dp_dis_reset(dptx, true);
	if (ret) {
		disp_pr_err("[DP] DPTX dis reset failed !!!\n");
		ret = -ENODEV;
		goto err_out;
	}

	if (!g_fpga_flag)
		hisi_disp_pm_enable_clks(dptx->connector_dev->master.connector_info->clk_bits);

	if (dptx->dptx_core_on)
		ret = dptx->dptx_core_on(dptx);
	if (ret) {
		ret = -ENODEV;
		goto err_out;
	}

	dp_on_params_init(dptx);

err_out:
	mutex_unlock(&dptx->dptx_mutex);

	disp_pr_info("[DP] dp%d, -\n", dptx->id);

	return ret;
}

static void dp_off_params_reset(struct dp_ctrl *dptx)
{
	disp_check_and_no_retval((dptx == NULL), err, "[DP] dptx is NULL!\n");

	dptx->detect_times = 0;
	dptx->dptx_vr = false;
	dptx->dptx_enable = false;
	dptx->video_transfer_enable = false;
	dptx->dptx_plug_type = DP_PLUG_TYPE_NORMAL;
	bpress_powerkey = false;
	btrigger_timeout = false;
}

int dp_off(struct dp_ctrl *dptx)
{
	int ret = 0;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");
	disp_check_and_return((dptx->id >= DP_CTRL_NUMS_MAX), -EINVAL, err, "[DP] Invalid dp id!\n");
	disp_pr_info("[DP] dp%d, +\n", dptx->id);

	mutex_lock(&dptx->dptx_mutex);

	if (!dptx->dptx_enable) {
		disp_pr_info("[DP] dptx has already off.\n");
		mutex_unlock(&dptx->dptx_mutex);
		return 0;
	}

	if (dptx->video_transfer_enable) {
		if (dptx->handle_hotunplug)
			dptx->handle_hotunplug(dptx);
		mdelay(10);
	}

	if (dptx->dptx_core_off)
		dptx->dptx_core_off(dptx);
#if DP_TO_DO
	hisi_disp_pm_disable_clks(dptx->connector_dev->master.connector_info->clk_bits);
#endif

	if (dptx->dp_dis_reset)
		ret = dptx->dp_dis_reset(dptx, false);
	if (ret) {
		disp_pr_err("[DP] DPTX dis reset failed !!!\n");
		ret = -ENODEV;
	}

	dp_off_params_reset(dptx);

	mutex_unlock(&dptx->dptx_mutex);

	disp_pr_info("[DP] dp%d, -\n", dptx->id);
	return ret;
}

#if DP_TO_DO
static int dp_clock_setup(struct dp_ctrl *dptx)
{
	struct hisi_disp_pm_clk *pm_clk = NULL;
	struct clk **disp_clks = hisi_disp_config_get_clks();
	int ret;

	if (dptx->id == DPTX_DP0 || dptx->id == DPTX_DP1) {
		pm_clk = disp_clks[PM_CLK_DP_CTRL_16M];
		if (pm_clk) {
			pm_clk->default_clk_rate = DEFAULT_AUXCLK_DPCTRL_RATE;
			ret = hisi_disp_pm_set_clk_default_rate(PM_CLK_DP_CTRL_16M);
			if (ret)
				return ret;
		}

		pm_clk = disp_clks[PM_CLK_ACLK_DP_CTRL];
		if (pm_clk) {
			pm_clk->default_clk_rate = DEFAULT_ACLK_DPCTRL_RATE;
			ret = hisi_disp_pm_set_clk_default_rate(PM_CLK_ACLK_DP_CTRL);
			if (ret)
				return ret;
		}
	} else {
		disp_pr_err("[DP] Wrong dp index: %u!\n", dptx->id);
		return -EINVAL;
	}

	return 0;
}
#endif

static int dp_parse_dts_params(struct dp_ctrl *dptx)
{
	struct device_node *np = NULL;
	int i;
	int ret;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

#if CONFIG_DP_ENABLE
	/* parse pre swing configure values */
	np = of_find_compatible_node(NULL, NULL, DTS_HISI_DP);
	if (np == NULL) {
		disp_pr_err("[DP] NOT FOUND device node %s!\n", DTS_HISI_DP);
		return -EINVAL;
	}
	for (i = 0; i < DPTX_COMBOPHY_PARAM_NUM; i++) {
		ret = of_property_read_u32_index(np, "preemphasis_swing", i, &(dptx->combophy_pree_swing[i]));
		if (ret) {
			disp_pr_err("[DP] preemphasis_swing[%d] is got fail\n", i);
			return -EINVAL;
		}
	}

	/* parse ssc ppm configure values */
	for (i = 0; i < DPTX_COMBOPHY_SSC_PPM_PARAM_NUM; i++) {
		ret = of_property_read_u32_index(np, "ssc_ppm", i, &(dptx->combophy_ssc_ppm[i]));
		if (ret) {
			disp_pr_err("[DP] combophy_ssc_ppm[%d] is got fail\n", i);
			return -EINVAL;
		}
	}
#endif

	np = of_find_compatible_node(NULL, NULL, DTS_DP_AUX_SWITCH);
	if (np == NULL) {
		dptx->edid_try_count = MAX_AUX_RETRY_COUNT;
		dptx->edid_try_delay = AUX_RETRY_DELAY_TIME;
	} else {
		ret = of_property_read_u32(np, "edid_try_count", &dptx->edid_try_count);
		if (ret < 0)
			dptx->edid_try_count = MAX_AUX_RETRY_COUNT;

		ret = of_property_read_u32(np, "edid_try_delay", &dptx->edid_try_delay);
		if (ret < 0)
			dptx->edid_try_delay = AUX_RETRY_DELAY_TIME;
	}
	disp_pr_info("[DP] edid try count=%d, delay=%d ms.\n", dptx->edid_try_count, dptx->edid_try_delay);

	return 0;
}

static int dp_device_params_init(struct dp_ctrl *dptx)
{
	int ret;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	mutex_init(&dptx->dptx_mutex);
	init_waitqueue_head(&dptx->dptxq);
	init_waitqueue_head(&dptx->waitq);
	atomic_set(&(dptx->sink_request), 0);
	atomic_set(&(dptx->shutdown), 0);
	atomic_set(&(dptx->c_connect), 0);

	dptx->dp_shutdown = false;
	dptx->dummy_dtds_present = false;
	dptx->selected_est_timing = NONE;
	dptx->dptx_vr = false;
	dptx->dptx_gate = false;
	dptx->dptx_enable = false;
	dptx->video_transfer_enable = false;
	dptx->dptx_detect_inited = false;
	dptx->user_mode = 0;
	dptx->detect_times = 0;
	dptx->dptx_plug_type = DP_PLUG_TYPE_NORMAL;
	dptx->user_mode_format = VCEA;
	dptx->dsc_decoders = DSC_DEFAULT_DECODER;
	if (g_fpga_flag == 1)
		dptx->dsc_ifbc_type = IFBC_TYPE_VESA2X_DUAL;
	else
		dptx->dsc_ifbc_type = IFBC_TYPE_VESA3X_DUAL;

#if CONFIG_DP_ENABLE
	dptx->same_source = huawei_dp_get_current_dp_source_mode();
#endif
	dptx->max_edid_timing_hactive = 0;
	dptx->bstatus = 0;

	ret = dp_parse_dts_params(dptx);
	if (ret)
		ret = -ENOMEM;

#if CONFIG_DP_ENABLE
	huawei_dp_debug_init_combophy_pree_swing(dptx->combophy_pree_swing, DPTX_COMBOPHY_PARAM_NUM);
#endif

	return ret;
}

static int dp_device_buf_alloc(struct dp_ctrl *dptx)
{
	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");

	memset(&(dptx->edid_info), 0, sizeof(struct edid_information));

	dptx->edid = kzalloc(DPTX_DEFAULT_EDID_BUFLEN, GFP_KERNEL);
	if (dptx->edid == NULL) {
		disp_pr_err("[DP] dptx base is NULL!\n");
		return -ENOMEM;
	}
	memset(dptx->edid, 0, DPTX_DEFAULT_EDID_BUFLEN);

	return 0;
}

static int dp_irq_setup(struct dp_ctrl *dptx)
{
	int ret;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL!\n");
	disp_check_and_return((dptx->dptx_irq == NULL), -EINVAL, err, "[DP] dptx->dptx_irq is NULL!\n");
	disp_check_and_return((dptx->dptx_threaded_irq == NULL), -EINVAL, err,
		"[DP] dptx->dptx_threaded_irq is NULL!\n");

	ret = devm_request_threaded_irq(dptx->dev,
		dptx->irq, dptx->dptx_irq, dptx->dptx_threaded_irq,
		IRQF_SHARED | IRQ_LEVEL, "dwc_dptx", (void *)dptx->connector_dev);//lint !e747
	if (ret) {
		disp_pr_err("[DP] Request for irq %d failed!\n", dptx->irq);
		return -EINVAL;
	}
	disable_irq(dptx->irq);

	return 0;
}

static int dp_config_irq_signal(struct dp_ctrl *dptx)
{
	const uint32_t *irq_signals;

#if DP_TO_DO
	dptx->irq = 0; //hisifd->dp_irq;
	if (!dptx->irq) {
		disp_pr_err("[DP] dptx irq is NULL!\n");
		return -EINVAL;
	}
#else
	irq_signals = hisi_disp_config_get_irqs();
	if (dptx->id == DPTX_DP0)
		dptx->irq = irq_signals[DISP_IRQ_SIGNAL_DPTX0];
	else if (dptx->id == DPTX_DP1)
		dptx->irq = irq_signals[DISP_IRQ_SIGNAL_DPTX1];
	if (!dptx->irq) {
		disp_pr_err("[DP] dptx irq is NULL!\n");
		return -EINVAL;
	}
#endif

	return 0;
}

static struct dp_ctrl *get_dp_ctrl_info(struct platform_device *pdev)
{
	struct hisi_connector_device *connector_dev = NULL;
	struct hisi_dp_info *dp_info = NULL;
	struct dp_ctrl *dptx = NULL;
	int ret;

	disp_check_and_return((pdev == NULL), NULL, err, "[DP] pdev is NULL!\n");

	connector_dev = platform_get_drvdata(pdev);
	disp_check_and_return((connector_dev == NULL), NULL, err, "[DP] connector_dev is NULL!\n");
	dp_info = (struct hisi_dp_info *)connector_dev->master.connector_info;
	disp_check_and_return((dp_info == NULL), NULL, err, "[DP] dp_info is NULL!\n");

	dptx = &(dp_info->dp);
	disp_pr_info("[DP] dp%d +.\n", dptx->id);

	if (dptx->id == DPTX_DP0) {
		dp_info->base.connector_base = hisi_config_get_ip_base(DISP_IP_BASE_HIDPTX0);
		dp_info->base.clk_bits = BIT(PM_CLK_DP_CTRL_16M) | BIT(PM_CLK_PCLK_DP_CTRL) | BIT(PM_CLK_ACLK_DP_CTRL);
	} else if (dptx->id == DPTX_DP1) {
		dp_info->base.connector_base = hisi_config_get_ip_base(DISP_IP_BASE_HIDPTX1);
		dp_info->base.clk_bits = BIT(PM_CLK_DP_CTRL_16M) | BIT(PM_CLK_PCLK_DP_CTRL) | BIT(PM_CLK_ACLK_DP_CTRL);
	}

	dptx->source_is_dprx = false;
	dptx->source_is_external = false;
	dptx->notify_dsscontext = NULL;
	dptx->local_notify_func = dptx_notify_dss;

#if DP_TO_DO
	ret = dp_clock_setup(dptx);
	if (ret) {
		disp_pr_err("[DP] DP clock setup failed!\n");
		return NULL;
	}
#endif

	dptx->dev = &pdev->dev;
	dptx->base = dp_info->base.connector_base;
	disp_pr_info("dp_base_addrs:%d\n", dptx->base);
	dptx->connector_dev = connector_dev;
	if (IS_ERR(dptx->base)) {
		disp_pr_err("[DP] dptx base is NULL!\n");
		return NULL;
	}

	return dptx;
}

static int dp_device_init(struct platform_device *pdev)
{
	struct dp_ctrl *dptx = NULL;
	int ret;

	dptx = get_dp_ctrl_info(pdev);
	if (dptx == NULL)
		return -EINVAL;

	ret = dp_config_irq_signal(dptx);
	if (ret)
		return ret;

	ret = dp_device_params_init(dptx);
	if (ret)
		return ret;

	dptx_link_layer_init(dptx);
	if (dptx->dptx_default_params_from_core)
		dptx->dptx_default_params_from_core(dptx);

	ret = dp_device_buf_alloc(dptx);
	if (ret)
		goto err_edid_alloc;

#if CONFIG_DP_ENABLE
	dptx->sdev.name = "dpu-dp";
	if (switch_dev_register(&dptx->sdev) < 0) {
		disp_pr_err("[DP] dp switch registration failed!\n");
		ret = -ENODEV;
		goto err_edid_alloc;
	}

	dptx->dp_switch.name = "hdmi_audio";
	ret = switch_dev_register(&dptx->dp_switch);
	if (ret < 0) {
		disp_pr_err("[DP] hdmi_audio switch device register error %d\n", ret);
		goto err_sdev_register;
	}
#endif

	ret = dp_irq_setup(dptx);
	if (ret)
		goto err_sdev_hdmi_register;
#if DP_TO_DO
	hisifd->dp_device_srs = dp_device_srs;
	hisifd->dp_get_color_bit_mode = dp_get_color_bit_mode;
	hisifd->dp_get_source_mode = dp_get_source_mode;
	hisifd->dp_pxl_ppll7_init = dp_pxl_ppll7_init;
	hisifd->dp_wakeup = dp_wakeup;
#endif

#ifdef CONFIG_DP_GRAPHIC_DEBUG_TOOL
	dp_graphic_debug_node_init(dptx);
#endif
#if DP_TO_DO
	hdcp_register();
#endif
	disp_pr_info("[DP] dp%d +.\n", dptx->id);

	return 0;
err_sdev_hdmi_register: /*lint !e563 */
#if CONFIG_DP_ENABLE
	switch_dev_unregister(&dptx->dp_switch);
#endif

err_sdev_register: /*lint !e563 */
#if CONFIG_DP_ENABLE
	switch_dev_unregister(&dptx->sdev);
#endif

err_edid_alloc: /*lint !e563 */
	if (dptx->edid != NULL) {
		kfree(dptx->edid);
		dptx->edid = NULL;
	}

	return ret;
}

static int hisi_drv_tmp_fb2_create(struct platform_device *pdev)
{
	struct hisi_connector_device *connector_dev = NULL;
	int ret;

	disp_pr_info("++++");

	connector_dev = platform_get_drvdata(pdev);
	disp_assert_if_cond(connector_dev == NULL);

	// create overlay device, will generate overlay platform device
	ret = hisi_drv_tmp_fb2_create_device(connector_dev);
	if (ret) {
		disp_pr_err("create device fail");
		return -1;
	}

	// register connector to overlay device
	hisi_drv_tmp_fb2_register_connector(connector_dev);
	hisi_drv_dp_create_chrdev(connector_dev);
	disp_pr_info("----");
	return 0;
}

static int hisi_drv_dp_probe(struct platform_device *pdev)
{
	struct hisi_connector_device *connector_dev = NULL;
	struct hisi_dp_info *dp_info = NULL;
	int ret;

	disp_pr_info("[DP] enter ++++++++++++");

	connector_dev = platform_get_drvdata(pdev);
	disp_assert_if_cond(connector_dev == NULL);
	dp_info = (struct hisi_dp_info *)connector_dev->master.connector_info;
	disp_check_and_return((dp_info == NULL), -EINVAL, err, "[DP] dp_info is NULL!\n");
	disp_check_and_return((dp_info->dp.id >= DP_CTRL_NUMS_MAX), -EINVAL, err, "[DP] Invalid dp id!\n");

	disp_pr_info("[DP] pdev=%p, connector_data=%p \n", pdev, connector_dev);

	ret = dp_device_init(pdev);
	if (ret) {
		disp_pr_err("[DP] dp_irq_clk_setup failed, error=%d!\n", ret);
		return -EINVAL;
	}

	/* create overlay device, will generate overlay platform device */
	ret = hisi_drv_external_composer_create_device(connector_dev);
	if (ret)
		return -1;

	hisi_connector_init_base_addr(&connector_dev->base_addr);

	/* registe dp master */
	hisi_dp_init_component(connector_dev);

	/* registe connector to overlay device */
	hisi_drv_external_composer_register_connector(connector_dev);
	hisi_drv_dp_create_chrdev(connector_dev);

	hisi_drv_tmp_fb2_create(pdev);

	g_dp_pdev[dp_info->dp.id] = pdev;

	dptx_register_displaysink(&dp_info->dp);

	disp_pr_info("[DP] exit -------------");
	return 0;
}

static int hisi_drv_dp_remove(struct platform_device *pdev)
{
	struct hisi_connector_device *connector_data = NULL;
	struct hisi_dp_info *dp_info = NULL;

	disp_assert_if_cond(pdev == NULL);

	connector_data = platform_get_drvdata(pdev);
	dp_info = (struct hisi_dp_info *)connector_data->master.connector_info;
	disp_check_and_return((dp_info == NULL), -EINVAL, err, "[DP] dp_info is NULL!\n");
	disp_check_and_return((dp_info->dp.id >= DP_CTRL_NUMS_MAX), -EINVAL, err, "[DP] Invalid dp id!\n");

	hisi_drv_dp_destroy_chrdev(connector_data);

	g_dp_pdev[dp_info->dp.id] = NULL;

	return 0;
}

static struct platform_driver g_hisi_dp_driver = {
	.probe = hisi_drv_dp_probe,
	.remove = hisi_drv_dp_remove,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = HISI_CONNECTOR_EXTERNAL_DP_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init hisi_drv_dp_init(void)
{
	int ret;

	ret = platform_driver_register(&g_hisi_dp_driver);
	if (ret) {
		disp_pr_err("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	return ret;
}

module_init(hisi_drv_dp_init);

MODULE_DESCRIPTION("Hisilicon Dptx Driver");
MODULE_LICENSE("GPL v2");
