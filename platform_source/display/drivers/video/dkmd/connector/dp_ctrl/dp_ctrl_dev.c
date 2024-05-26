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
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/switch.h>
#include <dpu/soc_dpu_define.h>
#include "peri/dkmd_peri.h"
#include "dp_aux.h"
#include "dp_drv.h"
#include "drm_dp_helper_additions.h"
#include "dp_link_layer_interface.h"
#include "dpu_dp_dbg.h"
#include "dp_ctrl_dev.h"
#include "hidptx/hidptx_reg.h"
#include "hidptx/hidptx_dp_core.h"
#include "controller/dp_avgen_base.h"

int dpu_dptx_get_spec(void *data, unsigned int size, unsigned int *ext_acount)
{
	struct dp_private *dp_priv = NULL;
	struct dkmd_connector_info *pinfo = NULL;
	struct dp_ctrl *dptx = NULL;
	struct edid_audio *audio_info = NULL;

	dpu_check_and_return(!g_dkmd_dp_devive, -EINVAL, err, "[DP] dp device is null pointer!");
	pinfo = platform_get_drvdata(g_dkmd_dp_devive);
	dpu_check_and_return(!pinfo, -EINVAL, err, "[DP] get connector info failed!");
	dp_priv = to_dp_private(pinfo);
	dpu_check_and_return(!dp_priv, -EINVAL, err, "[DP] dp_priv is null pointer\n");
	dptx = &dp_priv->dp;
	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is null pointer");

	audio_info = &dptx->edid_info.audio;
	if (size < sizeof(struct edid_audio_info) * audio_info->ext_acount) {
		dpu_pr_err("[DP] size is error!size %d ext_acount %d", size, audio_info->ext_acount);
		return -EINVAL;
	}

	memcpy(data, audio_info->spec, sizeof(struct edid_audio_info) * audio_info->ext_acount);
	*ext_acount = audio_info->ext_acount;

	dpu_pr_info("[DP] get spec success");

	return 0;
}

int dpu_dptx_set_aparam(unsigned int channel_num, unsigned int data_width, unsigned int sample_rate)
{
	struct dp_private *dp_priv = NULL;
	struct dkmd_connector_info *pinfo = NULL;
	struct dp_ctrl *dptx = NULL;
	uint8_t orig_sample_freq = 0;
	uint8_t sample_freq = 0;
	struct audio_params *aparams = NULL;

	dpu_check_and_return(!g_dkmd_dp_devive, -EINVAL, err, "[DP] dp device is null pointer!");
	pinfo = platform_get_drvdata(g_dkmd_dp_devive);
	dpu_check_and_return(!pinfo, -EINVAL, err, "[DP] get connector info failed!");
	dp_priv = to_dp_private(pinfo);
	dpu_check_and_return(!dp_priv, -EINVAL, err, "[DP] dp_priv is null pointer\n");
	dptx = &dp_priv->dp;
	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is null pointer");

	if (channel_num > DPTX_CHANNEL_NUM_MAX || data_width > DPTX_DATA_WIDTH_MAX) {
		dpu_pr_err("[DP] input param is invalid. channel_num=%d data_width=%d", channel_num, data_width);
		return -EINVAL;
	}
	aparams = &dptx->aparams;

	dpu_pr_info("[DP] set aparam. channel_num=%d data_width=%d sample_rate=%d",
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
		dpu_pr_info("[DP] invalid sample_rate");
		return -EINVAL;
	}

	aparams->iec_samp_freq = sample_freq;
	aparams->iec_orig_samp_freq = orig_sample_freq;
	aparams->num_channels = (uint8_t)channel_num;
	aparams->data_width = (uint8_t)data_width;

	if (dptx->dptx_audio_config)
		dptx->dptx_audio_config(dptx);

	dpu_pr_info("[DP] set aparam success");

	return 0;
}
EXPORT_SYMBOL(dpu_dptx_set_aparam);

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
		dpu_pr_err("[DP] Invalid rate 0x%x", rate);
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
		dpu_pr_err("[DP] Invalid bw 0x%x", bw);
		return -EINVAL;
	}
}

/*
 * audio/video Parameters Reset
 */
void dptx_audio_params_reset(struct audio_params *params)
{
	dpu_check_and_no_retval((params == NULL), err, "[DP] null pointer!");

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

void dptx_video_params_reset(struct video_params *params)
{
	dpu_check_and_no_retval((params == NULL), err, "[DP] null pointer!");

	memset(params, 0x0, sizeof(struct video_params));

	/* 6 bpc should be default - use 8 bpc for MST calculation */
	params->bpc = COLOR_DEPTH_6;
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

bool dptx_check_low_temperature(struct dp_ctrl *dptx)
{
	return false;
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

	dpu_check_and_no_retval(!dptx, err, "[DP] dptx is NULL!");

	state = dptx->sdev.state;
	switch_set_state(&dptx->sdev, val);

	if (dptx->edid_info.audio.basic_audio == 0x1) {
		if (val == HOT_PLUG_OUT || val == HOT_PLUG_OUT_VR)
			switch_set_state(&dptx->dp_switch, 0);
		else if (val == HOT_PLUG_IN || val == HOT_PLUG_IN_VR)
			switch_set_state(&dptx->dp_switch, 1);
	} else {
		dpu_pr_info("[DP] basic_audio(%ud) no support!", dptx->edid_info.audio.basic_audio);
	}

	vparams = &(dptx->vparams);
	mdtd = &(dptx->vparams.mdtd);
	dp_update_external_display_timming_info(mdtd->h_active, mdtd->v_active, vparams->m_fps);

	dpu_pr_info("[DP] cable state %s %d",
		dptx->sdev.state == state ? "is same" : "switched to", dptx->sdev.state);
}

struct ppll7_cfg_param {
	uint64_t refdiv;
	uint64_t fbdiv;
	uint64_t frac;
	uint64_t postdiv1;
	uint64_t postdiv2;
	int post_div;
};

static uint64_t dp_pixel_ppll7_param_calc(uint64_t pixel_clock_cur, struct ppll7_cfg_param *cfg)
{
	int i, ceil_temp;
	uint64_t ppll7_freq_divider, vco_freq_output;
	int freq_divider_list[22] = {
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
		12, 14, 15, 16, 20, 21, 24,
		25, 30, 36, 42, 49
	};
	int postdiv1_list[22] = {
		1, 2, 3, 4, 5, 6, 7, 4, 3, 5, 4,
		7, 5, 4, 5, 7, 6, 5, 6, 6, 7, 7
	};
	int postdiv2_list[22] = {
		1, 1, 1, 1, 1, 1, 1, 2, 3, 2, 3,
		2, 3, 4, 4, 3, 4, 5, 5, 6, 6, 7
	};

	/* Fractional PLL can not output the so small clock */
	cfg->post_div = 1;
	if (pixel_clock_cur * (uint64_t)freq_divider_list[21] < VCO_MIN_FREQ_OUPUT) {
		cfg->post_div = 2;
		pixel_clock_cur *= 2; /* multiple frequency */
	}

	ceil_temp = dp_ceil(VCO_MIN_FREQ_OUPUT, pixel_clock_cur);
	if (ceil_temp < 0)
		return pixel_clock_cur;

	ppll7_freq_divider = (uint64_t)ceil_temp;
	for (i = 0; i < 22; i++) {
		if (freq_divider_list[i] >= (int)ppll7_freq_divider) {
			ppll7_freq_divider = (uint64_t)freq_divider_list[i];
			cfg->postdiv1 = (uint64_t)postdiv1_list[i] - 1;
			cfg->postdiv2 = (uint64_t)postdiv2_list[i] - 1;
			dpu_pr_info("[DP] postdiv1=0x%llx, postdiv2=0x%llx\n", cfg->postdiv1, cfg->postdiv2);
			break;
		}
	}

	vco_freq_output = ppll7_freq_divider * pixel_clock_cur;
	if (vco_freq_output == 0)
		return pixel_clock_cur;

	dpu_pr_info("[DP] vco_freq_output=%llu\n", vco_freq_output);
	ceil_temp = dp_ceil(400000000UL, vco_freq_output);
	if (ceil_temp < 0)
		return pixel_clock_cur;

	cfg->refdiv = ((vco_freq_output * (uint64_t)ceil_temp) >= 494000000UL) ? 1 : 2;
	dpu_pr_info("[DP] refdiv=0x%llx\n", cfg->refdiv);

	cfg->fbdiv = vco_freq_output * (uint64_t)ceil_temp * cfg->refdiv / SYS_FREQ;
	dpu_pr_info("[DP] fbdiv=0x%llx\n", cfg->fbdiv);

	cfg->frac = ((uint64_t)ceil_temp * vco_freq_output - SYS_FREQ / \
		cfg->refdiv * cfg->fbdiv) * cfg->refdiv * 0x1000000; /* 0x1000000 is 2^24 */
	cfg->frac = (uint64_t)cfg->frac / SYS_FREQ;

	dpu_pr_info("[DP] frac=0x%llx\n", cfg->frac);

	return pixel_clock_cur;
}

static void dp_pixel_ppll7_set_reg(char __iomem *peri_crg_base, struct ppll7_cfg_param *cfg)
{
	uint32_t ppll7ctrl0, ppll7ctrl1;
	uint32_t ppll7ctrl0_val, ppll7ctrl1_val;

	ppll7ctrl0 = inp32(peri_crg_base + MIDIA_PPLL7_CTRL0);
	ppll7ctrl0 &= ~MIDIA_PPLL7_FREQ_DEVIDER_MASK;

	ppll7ctrl0_val = 0x0;
	ppll7ctrl0_val |= (uint32_t)((cfg->postdiv2 << 23) | (cfg->postdiv1 << 20) | \
		(cfg->fbdiv << 8) | (cfg->refdiv << 2));
	ppll7ctrl0_val &= MIDIA_PPLL7_FREQ_DEVIDER_MASK;

	ppll7ctrl0 |= ppll7ctrl0_val;
	outp32(peri_crg_base + MIDIA_PPLL7_CTRL0, ppll7ctrl0);

	ppll7ctrl1 = inp32(peri_crg_base + MIDIA_PPLL7_CTRL1);
	ppll7ctrl1 &= ~MIDIA_PPLL7_FRAC_MODE_MASK;

	ppll7ctrl1_val = 0x0;
	ppll7ctrl1_val |= (uint32_t)(1 << 25 | 0 << 24 | cfg->frac);
	ppll7ctrl1_val &= MIDIA_PPLL7_FRAC_MODE_MASK;

	ppll7ctrl1 |= ppll7ctrl1_val;
	outp32(peri_crg_base + MIDIA_PPLL7_CTRL1, ppll7ctrl1);
}

static int dp_pxl_ppll7_init(struct dp_private *dp_priv, struct dpu_connector *connector, uint64_t pixel_clock)
{
	int ret = 0;
	uint64_t pixel_clock_cur;
	struct ppll7_cfg_param cfg;

	dpu_check_and_return(!connector || !pixel_clock, -EINVAL, err, "[DP] input err!\n");

	pixel_clock_cur = dp_pixel_ppll7_param_calc(pixel_clock, &cfg);
	dp_pixel_ppll7_set_reg(connector->peri_crg_base, &cfg);

	/* need vote voltage */
	ret = clk_set_rate(dp_priv->clk_dpctrl_pixel, DEFAULT_MIDIA_PPLL7_CLOCK_FREQ / cfg.post_div);
	if (ret < 0)
		dpu_pr_err("[DP] %s clk_dpctrl_pixel clk_set_rate(%llu) failed, error=%d!\n",
			connector->conn_info->base.name, pixel_clock_cur, ret);

	ret = clk_prepare_enable(dp_priv->clk_dpctrl_pixel);
	if (ret) {
		dpu_pr_err("%s clk_dpctrl_pixel enable failed, error=%d!\n",
			connector->conn_info->base.name, pixel_clock_cur, ret);
		return -EINVAL;
	}
	dpu_pr_info("clk_dpctrl_pixel:ori[%llu]->[%llu]->[%llu]\n",
		pixel_clock, pixel_clock_cur, (uint64_t)clk_get_rate(dp_priv->clk_dpctrl_pixel));

	return ret;
}

int dptx_dss_plugin(struct dp_ctrl *dptx, enum dptx_hot_plug_type etype)
{
	struct dp_private *dp_priv = NULL;

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is null pointer\n");
	dp_priv = to_dp_private(dptx->connector->conn_info);
	dpu_check_and_return(!dp_priv, -EINVAL, err, "[DP] dp_priv is null pointer\n");

	/* config pll */
	dp_pxl_ppll7_init(dp_priv, dptx->connector, dptx->connector->ldi.pxl_clk_rate);

	dptx->video_transfer_enable = true;

	/* for dp test, would be deleted */
	dpu_pr_info("[DP] enable dp colorbar!");
	dptx_writel(dptx, dptx_dp_color_bar_stream(0), DPTX_COLOR_BAR_STREAM_EN);

	dp_send_cable_notification(dptx, etype);

	return 0;
}

static int dp_ctrl_on(struct dkmd_connector_info *pinfo)
{
	struct dp_private *dp_priv = to_dp_private(pinfo);
	struct dp_ctrl *dptx = NULL;

	dpu_check_and_return(!dp_priv, -EINVAL, err, "[DP] dp_priv is null pointer\n");
	dptx = &dp_priv->dp;

	dpu_pr_info("[DP] enter!");

	mutex_lock(&dptx->dptx_mutex);
	if (dptx->power_saving_mode && dptx->dptx_power_handler) {
		dpu_pr_info("[DP] dptx has already blank, power on for unblank!");
		dptx->dptx_power_handler(dptx, true);
		dptx->power_saving_mode = false;
	}
	mutex_unlock(&dptx->dptx_mutex);

	dpu_pr_info("[DP] exit!");

	return 0;
}

static int dp_ctrl_off(struct dkmd_connector_info *pinfo)
{
	struct dp_private *dp_priv = to_dp_private(pinfo);
	struct dp_ctrl *dptx = NULL;

	dpu_check_and_return(!dp_priv, -EINVAL, err, "[DP] dp_priv is null pointer\n");
	dptx = &dp_priv->dp;

	dpu_pr_info("[DP] enter!");
	mutex_lock(&dptx->dptx_mutex);
	if (!dptx->power_saving_mode && dptx->video_transfer_enable) {
		dpu_pr_info("[DP] dptx has already connected, power off for blank!");
		if (dptx->dptx_triger_media_transfer)
			dptx->dptx_triger_media_transfer(dptx, false);

		msleep(100);
		if (dptx->dptx_power_handler)
			dptx->dptx_power_handler(dptx, false);

		dptx->power_saving_mode = true;
	}
	mutex_unlock(&dptx->dptx_mutex);

	dpu_pr_info("[DP] exit!");

	return 0;
}

static irqreturn_t dpu_dp_isr(int32_t irq, void *ptr)
{
	uint32_t timing_gen_irq_status, intr_status, sdp_intr_status;
	struct dpu_connector *connector = NULL;
	struct dkmd_isr *isr_ctrl = (struct dkmd_isr *)ptr;
	struct dp_private *dp_priv = NULL;
	struct dp_ctrl *dptx = NULL;
	uint32_t val = 0;

	dpu_check_and_return(!isr_ctrl, IRQ_NONE, err, "[DP] isr_ctrl is null pointer");
	connector = (struct dpu_connector *)isr_ctrl->parent;
	dp_priv = to_dp_private(connector->conn_info);
	dpu_check_and_return(!dp_priv, IRQ_NONE, err, "[DP] dp_priv is null pointer");
	dptx = &dp_priv->dp;

	timing_gen_irq_status = dptx_readl(dptx, DPTX_INTR_ACPU_TIMING_GEN_MASKED_STATUS) &
		dptx_readl(dptx, DPTX_INTR_ACPU_TIMING_GEN_ORIGINAL_STATUS);
	dptx_writel(dptx, DPTX_INTR_ACPU_TIMING_GEN_ORIGINAL_STATUS, timing_gen_irq_status);

	intr_status = dptx_readl(dptx, DPTX_INTR_MASKED_STATUS) & dptx_readl(dptx, DPTX_INTR_ORIGINAL_STATUS);
	dptx_writel(dptx, DPTX_INTR_ORIGINAL_STATUS, intr_status);

	sdp_intr_status = dptx_readl(dptx, DPTX_SDP_INTR_MASKED_STATUS) & dptx_readl(dptx, DPTX_SDP_INTR_ORIGINAL_STATUS);
	dptx_writel(dptx, DPTX_SDP_INTR_ORIGINAL_STATUS, sdp_intr_status);

	if (timing_gen_irq_status & DPTX_IRQ_VSYNC)
		dkmd_isr_notify_listener(isr_ctrl, DSI_INT_VSYNC);

	if ((timing_gen_irq_status & DPTX_IRQ_STREAM0_UNDERFLOW) && !dptx->dptx_underflow_clear) {
		dpu_pr_warn("[DP-IRQ] DPTX_IRQ_STREAM0_UNDERFLOW");
		dptx->dptx_underflow_clear = true;
		val = dptx_readl(dptx, DPTX_INTR_ACPU_TIMING_GEN_ENABLE);
		val &= ~DPTX_IRQ_STREAM0_UNDERFLOW;
		dptx_writel(dptx, DPTX_INTR_ACPU_TIMING_GEN_ENABLE, val);
		dptx_global_intr_clear(dptx);
		dkmd_isr_notify_listener(isr_ctrl, DSI_INT_UNDER_FLOW);
	}

	if (intr_status & DPTX_IRQ_STREAM0_FRAME) {
		val = dptx_readl(dptx, DPTX_INTR_ACPU_TIMING_GEN_ENABLE);
		val |= DPTX_IRQ_STREAM0_UNDERFLOW;
		dptx_writel(dptx, DPTX_INTR_ACPU_TIMING_GEN_ENABLE, val);
		dkmd_isr_notify_listener(isr_ctrl, DSI_INT_VACT0_START);
	}

	return IRQ_HANDLED;
}

static void dp_ctrl_isr_handle(struct dpu_connector *connector, struct dkmd_isr *isr_ctrl, bool enable)
{
	struct dp_private *dp_priv = to_dp_private(connector->conn_info);
	struct dp_ctrl *dptx = &dp_priv->dp;

	mutex_lock(&dptx->dptx_mutex);
	if (!dptx->video_transfer_enable) {
		dpu_pr_info("[DP] dptx has already off\n");
		mutex_unlock(&dptx->dptx_mutex);
		return;
	}
	/* 1. interrupt mask */
	dptx_global_intr_dis(dptx);
	if (enable) {
		/* 2. enable irq */
		isr_ctrl->handle_func(isr_ctrl, DKMD_ISR_ENABLE);

		/* 3. interrupt clear */
		dptx_global_intr_clear(dptx);

		/* 4. interrupt umask, enable all top-level interrupts */
		dptx_global_intr_en(dptx);
	} else {
		/* 2. disable irq */
		isr_ctrl->handle_func(isr_ctrl, DKMD_ISR_DISABLE);

		/* 3. interrupt clear */
		dptx_global_intr_clear(dptx);
	}
	mutex_unlock(&dptx->dptx_mutex);
}

static int32_t dp_ctrl_isr_enable(struct dpu_connector *connector, const void *value)
{
	struct dkmd_isr *isr_ctrl = (struct dkmd_isr *)value;

	dp_ctrl_isr_handle(connector, isr_ctrl, true);

	return 0;
}

static int32_t dp_ctrl_isr_disable(struct dpu_connector *connector, const void *value)
{
	struct dkmd_isr *isr_ctrl = (struct dkmd_isr *)value;

	dp_ctrl_isr_handle(connector, isr_ctrl, false);

	return 0;
}

static int32_t dp_ctrl_isr_setup(struct dpu_connector *connector, const void *value)
{
	struct dkmd_isr *isr_ctrl = (struct dkmd_isr *)value;

	(void)snprintf(isr_ctrl->irq_name, sizeof(isr_ctrl->irq_name), "irq_dp_%d", connector->pipe_sw_post_chn_idx);
	isr_ctrl->irq_no = connector->connector_irq;
	isr_ctrl->isr_fnc = dpu_dp_isr;
	isr_ctrl->parent = connector;
	/* fake int mask for dptx */
	isr_ctrl->unmask = ~(DSI_INT_VSYNC | DSI_INT_VACT0_START | DSI_INT_UNDER_FLOW);

	return 0;
}

static void enable_dptx_timing_gen(struct dkmd_connector_info *pinfo)
{
	uint32_t val;
	struct dpu_connector *connector = get_primary_connector(pinfo);
	struct dp_private *dp_priv = to_dp_private(connector->conn_info);
	struct dp_ctrl *dptx = &dp_priv->dp;

	dptx_writel(dptx, dptx_dp_color_bar_stream(0), 0);
	if (dptx->dptx_triger_media_transfer)
		dptx->dptx_triger_media_transfer(dptx, true);

	val = dptx_readl(dptx, DPTX_GCTL0);
	val |= DPTX_CFG_TIMING_GEN_ENABLE;
	dptx_writel(dptx, DPTX_GCTL0, val);
}

static void disable_dptx_timing_gen(struct dkmd_connector_info *pinfo)
{
	struct dpu_connector *connector = get_primary_connector(pinfo);
	struct dp_private *dp_priv = to_dp_private(connector->conn_info);
	struct dp_ctrl *dptx = &dp_priv->dp;

	if (dptx->dptx_triger_media_transfer)
		dptx->dptx_triger_media_transfer(dptx, false);
}

static int32_t dp_ctrl_dptx_reset(struct dpu_connector *connector, const void *value)
{
	struct dp_private *dp_priv = to_dp_private(connector->conn_info);
	struct dp_ctrl *dptx = &dp_priv->dp;

	dpu_pr_info("[DP] clear enter");

	mutex_lock(&dptx->dptx_mutex);
	if (!dptx->video_transfer_enable) {
		dpu_pr_info("[DP] dptx has already off");
		dptx->dptx_underflow_clear = false;
		mutex_unlock(&dptx->dptx_mutex);
		return 0;
	}
	msleep(100);
	enable_dptx_timing_gen(connector->conn_info);
	dptx->dptx_underflow_clear = false;
	mutex_unlock(&dptx->dptx_mutex);

	dpu_pr_info("[DP] clear exit");

	return 0;
}

static struct connector_ops_handle_data dp_ops_table[] = {
	{ SETUP_ISR, dp_ctrl_isr_setup },
	{ ENABLE_ISR, dp_ctrl_isr_enable },
	{ DISABLE_ISR, dp_ctrl_isr_disable },
	{ DO_CLEAR, dp_ctrl_dptx_reset },
};

static int32_t dp_ctrl_ops_handle(struct dkmd_connector_info *pinfo, uint32_t ops_cmd_id, void *value)
{
	struct dpu_connector *connector = NULL;

	dpu_check_and_return(!pinfo, -EINVAL, err, "pinfo is NULL!");
	connector = get_primary_connector(pinfo);
	dpu_check_and_return(!connector, -EINVAL, err, "connector is NULL!");

	return dkdm_connector_hanlde_func(dp_ops_table, ARRAY_SIZE(dp_ops_table), ops_cmd_id, connector, value);
}

static void dp_ctrl_ldi_setup(struct dpu_connector *connector)
{
	struct dkmd_connector_info *pinfo = connector->conn_info;

	/* set default ldi parameter */
	pinfo->base.width = 16000;
	pinfo->base.height = 9000;
	pinfo->base.fps = 60;
	pinfo->ifbc_type = IFBC_TYPE_NONE;
	if (pinfo->base.fpga_flag == 1) {
		pinfo->base.xres = 1280;
		pinfo->base.yres = 720;

		connector->ldi.h_back_porch = 220;
		connector->ldi.h_front_porch = 110;
		connector->ldi.h_pulse_width = 40;
		connector->ldi.hsync_plr = 1;

		connector->ldi.v_back_porch = 20;
		connector->ldi.v_front_porch = 30;
		connector->ldi.v_pulse_width = 5;
		connector->ldi.vsync_plr = 1;

		connector->ldi.pxl_clk_rate = 27000000UL;
		connector->ldi.pxl_clk_rate_div = 1;
	} else {
		pinfo->base.xres = 1920;
		pinfo->base.yres = 1080;

		connector->ldi.h_back_porch = 148;
		connector->ldi.h_front_porch = 88;
		connector->ldi.h_pulse_width = 44;
		connector->ldi.hsync_plr = 1;

		connector->ldi.v_back_porch = 36;
		connector->ldi.v_front_porch = 4;
		connector->ldi.v_pulse_width = 5;
		connector->ldi.vsync_plr = 1;

		connector->ldi.pxl_clk_rate = 148500000UL;
		connector->ldi.pxl_clk_rate_div = 1;
	}
}

static void dp_ctrl_clk_setup(struct dpu_connector *connector)
{
	int ret = 0;

	ret = clk_set_rate(connector->connector_clk[CLK_DPCTRL_16M], 16000000UL);
	if (ret < 0)
		dpu_pr_err("[DP] CLK_DPCTRL_16M clk_set_rate failed, error=%d!\n", ret);

	ret = clk_set_rate(connector->connector_clk[CLK_DPCTRL_PCLK], 288000000UL);
	if (ret < 0)
		dpu_pr_err("[DP] CLK_DPCTRL_PCLK clk_set_rate failed, error=%d!\n", ret);
}

static void dp_device_params_init(struct dpu_connector *connector, struct dp_ctrl *dptx)
{
	mutex_init(&dptx->dptx_mutex);
	init_waitqueue_head(&dptx->dptxq);
	init_waitqueue_head(&dptx->waitq);
	atomic_set(&(dptx->sink_request), 0);
	atomic_set(&(dptx->shutdown), 0);
	atomic_set(&(dptx->c_connect), 0);

	dptx->dummy_dtds_present = false;
	dptx->selected_est_timing = NONE;
	dptx->dptx_vr = false;
	dptx->dptx_enable = false;
	dptx->video_transfer_enable = false;
	dptx->power_saving_mode = false;
	dptx->dptx_detect_inited = false;
	dptx->user_mode = 0;
	dptx->detect_times = 0;
	dptx->dptx_plug_type = DP_PLUG_TYPE_NORMAL;
	dptx->user_mode_format = VCEA;
	dptx->dsc_decoders = DSC_DEFAULT_DECODER;
	dptx->dsc_ifbc_type = IFBC_TYPE_VESA3X_DUAL; /* IFBC_TYPE_VESA2X_DUAL for fpga */
	dptx->same_source = false;
	dptx->max_edid_timing_hactive = 0;
	dptx->bstatus = 0;
	dptx->edid_try_count = MAX_AUX_RETRY_COUNT;
	dptx->edid_try_delay = AUX_RETRY_DELAY_TIME;

	dptx->base = connector->connector_base;
	memcpy(dptx->combophy_pree_swing,
		connector->combophy_ctrl.combophy_pree_swing, sizeof(dptx->combophy_pree_swing));
	memcpy(dptx->combophy_ssc_ppm,
		connector->combophy_ctrl.combophy_ssc_ppm, sizeof(dptx->combophy_ssc_ppm));
}

static int dp_device_buf_alloc(struct dp_ctrl *dptx)
{
	memset(&(dptx->edid_info), 0, sizeof(struct edid_information));

	dptx->edid = kzalloc(DPTX_DEFAULT_EDID_BUFLEN, GFP_KERNEL);
	if (dptx->edid == NULL) {
		dpu_pr_err("[DP] dptx base is NULL!\n");
		return -ENOMEM;
	}
	memset(dptx->edid, 0, DPTX_DEFAULT_EDID_BUFLEN);

	return 0;
}

void dp_ctrl_default_setup(struct dpu_connector *connector)
{
	int ret = 0;
	struct dp_private *dp_priv = to_dp_private(connector->conn_info);
	struct dp_ctrl *dptx = NULL;

	dpu_check_and_no_retval(!dp_priv, err, "[DP] dp_priv is null pointer\n");
	dptx = &dp_priv->dp;
	dptx->connector = connector;
	dptx->irq = (uint32_t)(connector->connector_irq);

	connector->connector_base = dp_priv->hidptx_base;

	dp_ctrl_ldi_setup(connector);
	dp_ctrl_clk_setup(connector);

	dp_device_params_init(connector, dptx);

	dptx_link_layer_init(dptx);
	if (dptx->dptx_default_params_from_core)
		dptx->dptx_default_params_from_core(dptx);

	ret = dp_device_buf_alloc(dptx);
	if (ret != 0)
		dpu_pr_err("[DP] dp device alloc failed!\n");

	dptx->sdev.name = "gfx_dp";
	if (switch_dev_register(&dptx->sdev) < 0)
		dpu_pr_err("[DP] dp switch registration failed!\n");

	dptx->dp_switch.name = "hdmi_audio";
	ret = switch_dev_register(&dptx->dp_switch);
	if (ret < 0)
		dpu_pr_err("[DP] hdmi_audio switch device register error %d\n", ret);

	dp_graphic_debug_node_init(dptx);

	connector->on_func = dp_ctrl_on;
	connector->off_func = dp_ctrl_off;
	connector->ops_handle_func = dp_ctrl_ops_handle;

	connector->conn_info->enable_ldi = enable_dptx_timing_gen;
	connector->conn_info->disable_ldi = disable_dptx_timing_gen;
}

MODULE_LICENSE("GPL");