/*
 * asp_codec_resource_widget.c -- asp codec resource widget define
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
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

#include"asp_codec_resource_widget.h"

#include <linux/delay.h>

#include "audio_log.h"
#include "asp_codec_regs.h"
#include "asp_codec.h"

#ifdef CONFIG_AUDIO_DEBUG
#define LOG_TAG "asp_codec"
#else
#define LOG_TAG "Analog_less_v1"
#endif

static int adc1_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_clr_reg_bits(CODEC_CLK_EN1_REG,
			0x1 << ADC1_0P5_PGA_CLKEN_OFFSET);
		asp_codec_set_reg_bits(CODEC_CLK_EN2_REG,
			0x1 << ADC1_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		asp_codec_clr_reg_bits(CODEC_CLK_EN2_REG,
			0x1 << ADC1_CLKEN_OFFSET);
		asp_codec_clr_reg_bits(CODEC_CLK_EN1_REG,
			0x1 << ADC1_0P5_PGA_CLKEN_OFFSET);
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static int adc2_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_clr_reg_bits(CODEC_CLK_EN1_REG,
			0x1 << ADC2_0P5_PGA_CLKEN_OFFSET);
		asp_codec_set_reg_bits(CODEC_CLK_EN2_REG,
			0x1 << ADC2_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		asp_codec_clr_reg_bits(CODEC_CLK_EN2_REG,
			0x1 << ADC2_CLKEN_OFFSET);
		asp_codec_clr_reg_bits(CODEC_CLK_EN1_REG,
			0x1 << ADC2_0P5_PGA_CLKEN_OFFSET);
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static int adc3_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_clr_reg_bits(CODEC_CLK_EN1_REG,
			0x1 << ADC3_0P5_PGA_CLKEN_OFFSET);
		asp_codec_set_reg_bits(CODEC_CLK_EN2_REG,
			0x1 << ADC3_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		asp_codec_clr_reg_bits(CODEC_CLK_EN2_REG,
			0x1 << ADC3_CLKEN_OFFSET);
		asp_codec_clr_reg_bits(CODEC_CLK_EN1_REG,
			0x1 << ADC3_0P5_PGA_CLKEN_OFFSET);
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static int adc4_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_clr_reg_bits(CODEC_CLK_EN1_REG,
			0x1 << ADC4_0P5_PGA_CLKEN_OFFSET);
		asp_codec_set_reg_bits(CODEC_CLK_EN2_REG,
			0x1 << ADC4_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		asp_codec_clr_reg_bits(CODEC_CLK_EN2_REG,
			0x1 << ADC4_CLKEN_OFFSET);
		asp_codec_clr_reg_bits(CODEC_CLK_EN1_REG,
			0x1 << ADC4_0P5_PGA_CLKEN_OFFSET);
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static int mdm5g_up_l_pga_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_set_reg_bits(CODEC_CLK_EN1_REG,
			0x1 << MDM_5G_L_UP_PGA_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		asp_codec_clr_reg_bits(CODEC_CLK_EN1_REG,
			0x1 << MDM_5G_L_UP_PGA_CLKEN_OFFSET);
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static int mdm5g_up_r_pga_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_set_reg_bits(CODEC_CLK_EN1_REG,
			0x1 << MDM_5G_R_UP_PGA_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		asp_codec_clr_reg_bits(CODEC_CLK_EN1_REG,
			0x1 << MDM_5G_R_UP_PGA_CLKEN_OFFSET);
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static int pll_supply_power_mode_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *codec = NULL;
	struct asp_codec_priv *priv = NULL;
	int ret = 0;

	codec = snd_soc_dapm_to_component(w->dapm);
	priv = snd_soc_component_get_drvdata(codec);
	if (priv->platform_type == UDP_PLATFORM) {
		switch (event) {
		case SND_SOC_DAPM_PRE_PMU:
			priv->have_dapm = true;
			break;
		case SND_SOC_DAPM_POST_PMD:
			priv->have_dapm = false;
			break;
		default:
			AUDIO_LOGE("power mode event err:%d", event);
			break;
		}
	} else {
		AUDIO_LOGI("fpga clk is always on");
	}

	return ret;
}

static int mdm5g_dn_l_supply_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << MDM_5G_L_DN_AFIFO_CLKEN_OFFSET);
		udelay(CLEAR_FIFO_DELAY_LEN_MS);
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << MDM_5G_L_DN_AFIFO_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* clear fifo */
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << MDM_5G_L_DN_AFIFO_CLKEN_OFFSET);
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static int mdm5g_dn_r_supply_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << MDM_5G_R_DN_AFIFO_CLKEN_OFFSET);
		udelay(CLEAR_FIFO_DELAY_LEN_MS);
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << MDM_5G_R_DN_AFIFO_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* clear fifo */
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << MDM_5G_R_DN_AFIFO_CLKEN_OFFSET);
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

#define SUPPLY_WIDGET \
	SND_SOC_DAPM_SUPPLY_S("DMIC1_SUPPLY", \
		0, CODEC_CLK_EN2_REG, DMIC1_CLKEN_OFFSET, 0, NULL, 0),\
	SND_SOC_DAPM_SUPPLY_S("DMIC2_SUPPLY", \
		0, CODEC_CLK_EN2_REG, DMIC2_CLKEN_OFFSET, 0, NULL, 0),\
	SND_SOC_DAPM_SUPPLY_S("AU_IN_SUPPLY", \
		0, CODEC_CLK_EN2_REG, AU_49152K_CLKEN_OFFSET, 0, NULL, 0),\
	SND_SOC_DAPM_SUPPLY_S("S1_SUPPLY", \
		0, CODEC_CLK_EN2_REG, I2S1_TDM_CLKEN_OFFSET, 0, NULL, 0),\
	SND_SOC_DAPM_SUPPLY_S("S1_RX_SUPPLY", \
		0, I2S1_CTRL_REG, I2S1_IF_RX_ENA_OFFSET, 0, NULL, 0),\
	SND_SOC_DAPM_SUPPLY_S("S2_SUPPLY", \
		0, CODEC_CLK_EN2_REG, I2S2_PCM_CLKEN_OFFSET, 0, NULL, 0),\
	SND_SOC_DAPM_SUPPLY_S("S2_RX_SUPPLY", \
		0, I2S2_PCM_CTRL_REG, I2S2_IF_RX_ENA_OFFSET, 0, NULL, 0),\
	SND_SOC_DAPM_SUPPLY_S("S3_SUPPLY", \
		0, CODEC_CLK_EN2_REG, I2S3_PCM_CLKEN_OFFSET, 0, NULL, 0),\
	SND_SOC_DAPM_SUPPLY_S("S3_RX_SUPPLY", \
		0, I2S3_PCM_CTRL_REG, I2S3_IF_RX_ENA_OFFSET, 0, NULL, 0),\
	SND_SOC_DAPM_SUPPLY_S("S4_SUPPLY", \
		0, CODEC_CLK_EN2_REG, I2S4_TDM_CLKEN_OFFSET, 0, NULL, 0),\
	SND_SOC_DAPM_SUPPLY_S("S4_RX_SUPPLY", \
		0, I2S4_CTRL_REG, I2S4_IF_RX_ENA_OFFSET, 0, NULL, 0),\
	SND_SOC_DAPM_SUPPLY_S("MDM5G_DN_L_SUPPLY", \
		0, SND_SOC_NOPM, 0, 0, mdm5g_dn_l_supply_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SUPPLY_S("MDM5G_DN_R_SUPPLY", \
		0, SND_SOC_NOPM, 0, 0, mdm5g_dn_r_supply_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SUPPLY_S("PLL", \
		0, SND_SOC_NOPM, 0, 0, pll_supply_power_mode_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)) \

#define PGA_WIDGET \
	SND_SOC_DAPM_PGA_S("AUDIO_DN_L_PGA", 0, CODEC_CLK_EN0_REG, \
		AUDIO_DN_L_PGA_CLKEN_OFFSET, 0, NULL, 0), \
	SND_SOC_DAPM_PGA_S("AUDIO_DN_R_PGA", 0, CODEC_CLK_EN0_REG, \
		AUDIO_DN_R_PGA_CLKEN_OFFSET, 0, NULL, 0), \
	SND_SOC_DAPM_PGA_S("CODEC3_DN_L_PGA", 0, CODEC_CLK_EN0_REG, \
		CODEC3_DN_L_PGA_CLKEN_OFFSET, 0, NULL, 0), \
	SND_SOC_DAPM_PGA_S("CODEC3_DN_R_PGA", 0, CODEC_CLK_EN0_REG, \
		CODEC3_DN_R_PGA_CLKEN_OFFSET, 0, NULL, 0), \
	SND_SOC_DAPM_PGA_S("I2S2_RX_L_PGA", 0, CODEC_CLK_EN1_REG, \
		I2S2_RX_L_PGA_CLKEN_OFFSET, 0, NULL, 0), \
	SND_SOC_DAPM_PGA_S("I2S2_RX_R_PGA", 0, CODEC_CLK_EN1_REG, \
		I2S2_RX_R_PGA_CLKEN_OFFSET, 0, NULL, 0), \
	SND_SOC_DAPM_PGA_S("SIDETONE_PGA", 0, CODEC_CLK_EN1_REG, \
		SIDETONE_PGA_CLKEN_OFFSET, 0, NULL, 0), \
	SND_SOC_DAPM_PGA_S("ADC1_0P5_PGA", 0, CODEC_CLK_EN1_REG, \
		ADC1_0P5_PGA_CLKEN_OFFSET, 0, NULL, 0), \
	SND_SOC_DAPM_PGA_S("MIC3_UP_PGA", 0, CODEC_CLK_EN0_REG, \
		MIC3_UP_PGA_CLKEN_OFFSET, 0, NULL, 0), \
	SND_SOC_DAPM_PGA_S("MIC4_UP_PGA", 0, CODEC_CLK_EN1_REG, \
		MIC4_UP_PGA_CLKEN_OFFSET, 0, NULL, 0), \
	SND_SOC_DAPM_PGA_S("ADC1", 1, SND_SOC_NOPM, 0, 0, adc1_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_PGA_S("ADC2", 1, SND_SOC_NOPM, 0, 0, adc2_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_PGA_S("ADC3", 1, SND_SOC_NOPM, 0, 0, adc3_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_PGA_S("ADC4", 1, SND_SOC_NOPM, 0, 0, adc4_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_PGA_S("MDM5G_UP_L_PGA", 1, SND_SOC_NOPM, 0, 0, \
		mdm5g_up_l_pga_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_PGA_S("MDM5G_UP_R_PGA", 1, SND_SOC_NOPM, 0, 0, \
		mdm5g_up_r_pga_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)) \

static const struct snd_soc_dapm_widget resource_widgets[] = {
	SUPPLY_WIDGET,
	PGA_WIDGET,
};

int asp_codec_add_resource_widgets(struct snd_soc_dapm_context *dapm)
{
	if (dapm == NULL) {
		AUDIO_LOGE("input dapm is null");
		return -EINVAL;
	}

	return snd_soc_dapm_new_controls(dapm, resource_widgets,
		ARRAY_SIZE(resource_widgets));
}

