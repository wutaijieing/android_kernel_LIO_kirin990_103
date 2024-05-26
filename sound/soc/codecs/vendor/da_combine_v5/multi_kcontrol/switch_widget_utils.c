/*
 * switch_widget_utils.c -- da combine v5 codec driver
 *
 * Copyright (c) 2018 Huawei Technologies Co., Ltd.
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

#include "switch_widget_utils.h"

#include <sound/core.h>
#include <sound/tlv.h>

#include "linux/platform_drivers/codec_bus/codec_bus.h"
#include "audio_log.h"
#include "linux/platform_drivers/da_combine/da_combine_v5.h"
#include "linux/platform_drivers/da_combine/da_combine_v5_regs.h"
#include "linux/platform_drivers/da_combine/da_combine_v5_type.h"
#include "linux/platform_drivers/da_combine/da_combine_utils.h"
#include "codec_dai.h"
#include "path_widget.h"

#ifdef CONFIG_HUAWEI_AUDIO
#include "huawei_platform/power/vsys_switch/vsys_switch.h"
#endif

#define PA_2_IV_PARAM_CHANNEL  2

void u12_select_pga(struct snd_soc_component *codec)
{
	da_combine_update_bits(codec, SC_CODEC_MUX_CTRL25_REG,
		0x3 << SLIM_UP12_DATA_SEL_OFFSET,
		0x0 << SLIM_UP12_DATA_SEL_OFFSET);
}

void u12_select_dspif(struct snd_soc_component *codec)
{
	da_combine_update_bits(codec, SC_CODEC_MUX_CTRL25_REG,
		0x3 << SLIM_UP12_DATA_SEL_OFFSET,
		BIT(SLIM_UP12_DATA_SEL_OFFSET));
}

void u56_select_dspif(struct snd_soc_component *codec)
{
	/* U5 U6 selesct dspif for wakeup app may died */
	da_combine_update_bits(codec, SC_CODEC_MUX_CTRL22_REG,
		0x3 << SLIM_UP6_DATA_SEL_OFFSET,
		BIT(SLIM_UP6_DATA_SEL_OFFSET));
	da_combine_update_bits(codec, SC_CODEC_MUX_CTRL25_REG,
		0x3 << SLIM_UP5_DATA_SEL_OFFSET,
		BIT(SLIM_UP5_DATA_SEL_OFFSET));
}

void u56_select_pga(struct snd_soc_component *codec)
{
	da_combine_update_bits(codec, SC_CODEC_MUX_CTRL22_REG,
		0x3 << SLIM_UP6_DATA_SEL_OFFSET,
		0x0 << SLIM_UP6_DATA_SEL_OFFSET);
	da_combine_update_bits(codec, SC_CODEC_MUX_CTRL25_REG,
		0x3 << SLIM_UP5_DATA_SEL_OFFSET,
		0x0 << SLIM_UP5_DATA_SEL_OFFSET);
}

void u10_select_mic_src(struct snd_soc_component *codec)
{
	da_combine_update_bits(codec, SC_CODEC_MUX_CTRL25_REG,
		0x3 << SLIM_UP10_DATA_SEL_OFFSET,
		0x3 << SLIM_UP10_DATA_SEL_OFFSET);
}

void u10_select_dspif(struct snd_soc_component *codec)
{
	da_combine_update_bits(codec, SC_CODEC_MUX_CTRL25_REG,
		0x3 << SLIM_UP10_DATA_SEL_OFFSET,
		BIT(SLIM_UP10_DATA_SEL_OFFSET));
}

int voice_codec_bus_active(struct snd_soc_component *codec,
	struct da_combine_v5_platform_data *platform_data)
{
	int ret;

	u56_select_pga(codec);
	if (platform_data->voice_up_params.channels == 4)
		da_combine_update_bits(codec, S1_MIXER_EQ_CLK_EN_REG,
			BIT(RST_5MIC_S3_ACCESS_IRQ_OFFSET),
			BIT(RST_5MIC_S3_ACCESS_IRQ_OFFSET));

	ret = codec_bus_activate("VOICE_UP", &platform_data->voice_up_params);
	if (ret != 0)
		AUDIO_LOGE("activate voice up err: %d", ret);

	if (platform_data->voice_up_params.channels == 4)
		da_combine_update_bits(codec, S1_MIXER_EQ_CLK_EN_REG,
			BIT(RST_5MIC_S3_ACCESS_IRQ_OFFSET), 0);

	platform_data->voiceup_state = TRACK_STARTUP;

	return ret;
}

int voice_codec_bus_deactive(struct snd_soc_component *codec,
	struct da_combine_v5_platform_data *platform_data)
{
	int ret;

	/* voice deactive */
	ret = codec_bus_deactivate("VOICE_UP", &platform_data->voice_up_params);
	if (ret != 0)
		AUDIO_LOGE("deactivate voice up err: %d", ret);

	platform_data->voiceup_state = TRACK_FREE;
	u56_select_dspif(codec);

	return ret;
}

static int codec_bus_param_pass(struct snd_soc_component *codec,
	const char* name, struct scene_param *params, int event)
{
	struct da_combine_v5_platform_data *platform_data = snd_soc_component_get_drvdata(codec);
	struct codec_dai_sample_rate_params hwparms;
	int ret = 0;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		params_sample_rate(hwparms, PORT_D1_D2, params->rate);
		platform_data->codec_dai->config_sample_rate(codec, &hwparms);

		ret = codec_bus_activate(name, params);
		if (ret != 0)
			AUDIO_LOGE("activate err: %d", ret);
		break;
	case SND_SOC_DAPM_POST_PMD:
		ret = codec_bus_deactivate(name, params);
		if (ret != 0)
			AUDIO_LOGE("deactivate err: %d", ret);

		params_sample_rate(hwparms, PORT_D1_D2, SAMPLE_RATE_48K);
		platform_data->codec_dai->config_sample_rate(codec, &hwparms);
		break;
	default:
		AUDIO_LOGW("power event err: %d", event);
		ret = -1;
		break;
	}

	return ret;
}

static int play48k_path_config(struct snd_soc_component *codec, int event)
{
	int ret = 0;

	AUDIO_LOGD("begin");

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		da_combine_update_bits(codec, S1_DP_CLK_EN_REG,
			BIT(S1_IL_SRC_CLK_EN_OFFSET) | BIT(S1_IR_SRC_CLK_EN_OFFSET),
			BIT(S1_IL_SRC_CLK_EN_OFFSET) | BIT(S1_IR_SRC_CLK_EN_OFFSET));
		da_combine_update_bits(codec, SC_CODEC_MUX_CTRL9_REG,
			BIT(DAC_MIXER4_VLD_SEL_OFFSET) | BIT(DAC_PRE_MIXER2_VLD_SEL_OFFSET),
			BIT(DAC_MIXER4_VLD_SEL_OFFSET) | BIT(DAC_PRE_MIXER2_VLD_SEL_OFFSET));
		break;
	case SND_SOC_DAPM_POST_PMD:
		da_combine_update_bits(codec, SC_CODEC_MUX_CTRL9_REG,
			BIT(DAC_MIXER4_VLD_SEL_OFFSET)  | BIT(DAC_PRE_MIXER2_VLD_SEL_OFFSET), 0);
		da_combine_update_bits(codec, S1_DP_CLK_EN_REG,
			BIT(S1_IL_SRC_CLK_EN_OFFSET) | BIT(S1_IR_SRC_CLK_EN_OFFSET), 0);
		break;
	default:
		AUDIO_LOGW("power event err: %d", event);
		break;
	}

	AUDIO_LOGD("end");

	return ret;
}

static void bypass_dac_fir21(struct snd_soc_component *codec, bool flag)
{
	if (flag) {
		da_combine_update_bits(codec, DACL_CTRL_REG,
			BIT(DACL_IR2I_BYPASS_EN_OFFSET), BIT(DACL_IR2I_BYPASS_EN_OFFSET));
		da_combine_update_bits(codec, DACR_CTRL_REG,
			BIT(DACR_IR2I_BYPASS_EN_OFFSET), BIT(DACR_IR2I_BYPASS_EN_OFFSET));
	} else {
		da_combine_update_bits(codec, DACL_CTRL_REG,
			BIT(DACL_IR2I_BYPASS_EN_OFFSET), 0);
		da_combine_update_bits(codec, DACR_CTRL_REG,
			BIT(DACR_IR2I_BYPASS_EN_OFFSET), 0);
	}
}

static int play96k_path_config(struct snd_soc_component *codec, int event)
{
	int ret = 0;

	AUDIO_LOGD("begin");

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		da_combine_update_bits(codec, SC_CODEC_MUX_CTRL9_REG,
			BIT(DAC_MIXER4_VLD_SEL_OFFSET) | BIT(DAC_PRE_MIXER2_VLD_SEL_OFFSET),
			BIT(DAC_MIXER4_VLD_SEL_OFFSET) | BIT(DAC_PRE_MIXER2_VLD_SEL_OFFSET));
		bypass_dac_fir21(codec, true);
		break;
	case SND_SOC_DAPM_POST_PMD:
		bypass_dac_fir21(codec, false);
		da_combine_update_bits(codec, SC_CODEC_MUX_CTRL9_REG,
			BIT(DAC_MIXER4_VLD_SEL_OFFSET) | BIT(DAC_PRE_MIXER2_VLD_SEL_OFFSET), 0);
		break;
	default:
		AUDIO_LOGW("power event err: %d", event);
		break;
	}

	AUDIO_LOGD("end");

	return ret;
}

static int play192k_path_config(struct snd_soc_component *codec, int event)
{
	int ret = 0;

	AUDIO_LOGD("begin");

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		da_combine_update_bits(codec, SC_CODEC_MUX_CTRL34_REG,
			BIT(DACL_POST_MIXER2_DVLD_SEL_OFFSET) | BIT(DACR_POST_MIXER2_DVLD_SEL_OFFSET),
			BIT(DACL_POST_MIXER2_DVLD_SEL_OFFSET) | BIT(DACR_POST_MIXER2_DVLD_SEL_OFFSET));
		bypass_dac_fir21(codec, true);
		break;
	case SND_SOC_DAPM_POST_PMD:
		bypass_dac_fir21(codec, false);
		da_combine_update_bits(codec, SC_CODEC_MUX_CTRL34_REG,
			BIT(DACL_POST_MIXER2_DVLD_SEL_OFFSET) | BIT(DACR_POST_MIXER2_DVLD_SEL_OFFSET), 0);
		break;
	default:
		AUDIO_LOGW("power event err: %d", event);
		break;
	}

	AUDIO_LOGD("end");

	return ret;
}

struct play_power_event_config {
	unsigned int rate;
	unsigned int sample_rate;
	const char* scene_name;
	int (*play_path_config)(struct snd_soc_component *codec, int event);
};

static const struct play_power_event_config play_power_event_config_list[SAMPLE_RATE_INDEX_MAX] = {
	{ SAMPLE_RATE_INDEX_48K, SAMPLE_RATE_48K, "AUDIO_PLAY", play48k_path_config },
	{ SAMPLE_RATE_INDEX_96K, SAMPLE_RATE_96K, "DIRECT_PLAY", play96k_path_config },
	{ SAMPLE_RATE_INDEX_192K, SAMPLE_RATE_192K, "DIRECT_PLAY", play192k_path_config },
	{ SAMPLE_RATE_INDEX_384K, SAMPLE_RATE_384K, "DIRECT_PLAY", play192k_path_config },
	{ SAMPLE_RATE_INDEX_44K1, SAMPLE_RATE_44K1, "DIRECT_PLAY", play48k_path_config },
	{ SAMPLE_RATE_INDEX_88K2, SAMPLE_RATE_88K2, "DIRECT_PLAY", play96k_path_config },
	{ SAMPLE_RATE_INDEX_176K4, SAMPLE_RATE_176K4, "DIRECT_PLAY", play192k_path_config }
};

int play_config_power_event(unsigned int rate, struct snd_soc_component *codec, int event)
{
	struct da_combine_v5_platform_data *priv = snd_soc_component_get_drvdata(codec);

	unsigned int i;
	int ret;

	AUDIO_LOGD("begin");

	for (i = 0; i < SAMPLE_RATE_INDEX_MAX; i++) {
		if (rate == play_power_event_config_list[i].rate)
			break;
	}

	if (i == SAMPLE_RATE_INDEX_MAX) {
		AUDIO_LOGW("error sample rate: %d", rate);
		return -EINVAL;
	}
	AUDIO_LOGI("sample rate idx: %d, rate: %u",
				i, play_power_event_config_list[i].rate);

	if (play_power_event_config_list[i].play_path_config) {
		ret = play_power_event_config_list[i].play_path_config(codec, event);
		if (ret != 0) {
			AUDIO_LOGW("path config err: %d", ret);
			return ret;
		}
	}

	priv->play_params.ports[0] = PORT_TYPE_D1;
	priv->play_params.ports[1] = PORT_TYPE_D2;
	priv->play_params.rate = play_power_event_config_list[i].sample_rate;

	ret = codec_bus_param_pass(codec,
		play_power_event_config_list[i].scene_name, &priv->play_params, event);

	AUDIO_LOGD("end");

	return ret;
}

void hp_high_level_enable(struct snd_soc_component *codec)
{
	struct da_combine_v5_platform_data *priv = snd_soc_component_get_drvdata(codec);

	/* hpdac high performance */
	unsigned int old_mode = (unsigned int)priv->rcv_hp_classH_state;

	priv->rcv_hp_classH_state = old_mode & (~HP_CLASSH_STATE);
	da_combine_v5_set_classH_config(codec, priv->rcv_hp_classH_state);
}

void hp_high_level_disable(struct snd_soc_component *codec)
{
	struct da_combine_v5_platform_data *priv = snd_soc_component_get_drvdata(codec);

	/* hpdac lower power */
	unsigned int old_mode = (unsigned int)priv->rcv_hp_classH_state;

	priv->rcv_hp_classH_state = old_mode | HP_CLASSH_STATE;
	da_combine_v5_set_classH_config(codec, priv->rcv_hp_classH_state);
}

void iv_dspif_switch_enable(struct snd_soc_component *codec)
{
	/* on bit match */
	da_combine_update_bits(codec, SC_CODEC_MUX_CTRL1_REG,
		BIT(S4_O_BITMATCH_BYP_EN_OFFSET), 0);

	/* src 96k-48k */
	da_combine_update_bits(codec, S4_DP_CLK_EN_REG,
		BIT(S4_OL_SRC_CLK_EN_OFFSET) | BIT(S4_OR_SRC_CLK_EN_OFFSET),
		BIT(S4_OL_SRC_CLK_EN_OFFSET) | BIT(S4_OR_SRC_CLK_EN_OFFSET));
	da_combine_update_bits(codec, SRC_VLD_CTRL8_REG,
		BIT(S4_OL_SRC_DIN_VLD_SEL_OFFSET) | BIT(S4_OR_SRC_DIN_VLD_SEL_OFFSET),
		BIT(S4_OL_SRC_DIN_VLD_SEL_OFFSET) | BIT(S4_OR_SRC_DIN_VLD_SEL_OFFSET));

	/* 32bit */
	da_combine_update_bits(codec, SC_S4_IF_H_REG,
		MASK_ON_BIT(S4_CODEC_IO_WORDLENGTH_LEN, S4_CODEC_IO_WORDLENGTH_OFFSET), 0xff);
}

void iv_dspif_switch_disable(struct snd_soc_component *codec)
{
	da_combine_update_bits(codec, SC_CODEC_MUX_CTRL37_REG,
		MASK_ON_BIT(SPA_OL_SRC_DIN_SEL_LEN, SPA_OL_SRC_DIN_SEL_OFFSET) |
		MASK_ON_BIT(SPA_OR_SRC_DIN_SEL_LEN, SPA_OR_SRC_DIN_SEL_OFFSET),
		BIT(SPA_OL_SRC_DIN_SEL_OFFSET) | BIT(SPA_OR_SRC_DIN_SEL_OFFSET));
	da_combine_update_bits(codec, SC_S4_IF_H_REG,
		MASK_ON_BIT(S4_CODEC_IO_WORDLENGTH_LEN,
		S4_CODEC_IO_WORDLENGTH_OFFSET),
		0x0);
	da_combine_update_bits(codec, S4_DP_CLK_EN_REG,
		BIT(S4_OL_SRC_CLK_EN_OFFSET) | BIT(S4_OR_SRC_CLK_EN_OFFSET),
		0x0);
	da_combine_update_bits(codec, SRC_VLD_CTRL8_REG,
		BIT(S4_OL_SRC_DIN_VLD_SEL_OFFSET) |
		BIT(S4_OR_SRC_DIN_VLD_SEL_OFFSET),
		0x0);
	da_combine_update_bits(codec, SC_CODEC_MUX_CTRL1_REG,
		BIT(S4_O_BITMATCH_BYP_EN_OFFSET),
		BIT(S4_O_BITMATCH_BYP_EN_OFFSET));
}

static const struct reg_seq_config iv_2pa_up_regs[] = {
	/* on bit match */
	{ { SC_CODEC_MUX_CTRL1_REG, BIT(S4_O_BITMATCH_BYP_EN_OFFSET), 0x0, true }, 0, 0 },
	/* src 96k-48k */
	{ { S4_DP_CLK_EN_REG, BIT(S4_OL_SRC_CLK_EN_OFFSET) |
		BIT(S4_OR_SRC_CLK_EN_OFFSET),
		BIT(S4_OL_SRC_CLK_EN_OFFSET) |
		BIT(S4_OR_SRC_CLK_EN_OFFSET), true }, 0, 0 },
	{ { SRC_VLD_CTRL8_REG, BIT(S4_OL_SRC_DIN_VLD_SEL_OFFSET) |
		BIT(S4_OR_SRC_DIN_VLD_SEL_OFFSET),
		BIT(S4_OL_SRC_DIN_VLD_SEL_OFFSET) |
		BIT(S4_OR_SRC_DIN_VLD_SEL_OFFSET), true }, 0, 0 },
	/* 32bit */
	{ { SC_S4_IF_H_REG, MASK_ON_BIT(S4_CODEC_IO_WORDLENGTH_LEN,
		S4_CODEC_IO_WORDLENGTH_OFFSET),
		0x3 << S4_CODEC_IO_WORDLENGTH_OFFSET, true }, 0, 0 },
	/* sel iv */
	{ { SC_CODEC_MUX_CTRL37_REG, 0, 0xA5, 0 }, 0, 0 },
};

static const struct reg_seq_config iv_2pa_dn_regs[] = {
	{ { SC_CODEC_MUX_CTRL37_REG, 0, 0x5, 0 }, 0, 0 },
	{ { SC_S4_IF_H_REG, MASK_ON_BIT(S4_CODEC_IO_WORDLENGTH_LEN,
		S4_CODEC_IO_WORDLENGTH_OFFSET), 0x0, true }, 0, 0 },
	{ { S4_DP_CLK_EN_REG, BIT(S4_OL_SRC_CLK_EN_OFFSET) |
		BIT(S4_OR_SRC_CLK_EN_OFFSET), 0x0, true }, 0, 0 },
	{ { SRC_VLD_CTRL8_REG, BIT(S4_OL_SRC_DIN_VLD_SEL_OFFSET) |
		BIT(S4_OR_SRC_DIN_VLD_SEL_OFFSET), 0x0, true }, 0, 0 },
	{ { SC_CODEC_MUX_CTRL1_REG, BIT(S4_O_BITMATCH_BYP_EN_OFFSET),
		BIT(S4_O_BITMATCH_BYP_EN_OFFSET), true }, 0, 0 },
};

void iv_2pa_switch_enable(struct snd_soc_component *codec)
{
	int ret;
	struct da_combine_v5_platform_data *priv = snd_soc_component_get_drvdata(codec);

	priv->pa_iv_params.channels = PA_2_IV_PARAM_CHANNEL;
	priv->pa_iv_params.ports[0] = PORT_TYPE_U7;
	priv->pa_iv_params.ports[1] = PORT_TYPE_U8;

	write_reg_seq_array(codec,
		iv_2pa_up_regs, ARRAY_SIZE(iv_2pa_up_regs));

	ret = codec_bus_activate("2PA_IV", &priv->pa_iv_params);
	if (ret != 0)
		AUDIO_LOGE("track activate 2pa iv err: %d", ret);
}

void iv_2pa_switch_disable(struct snd_soc_component *codec)
{
	int ret;
	struct da_combine_v5_platform_data *priv = snd_soc_component_get_drvdata(codec);

	priv->pa_iv_params.channels = PA_2_IV_PARAM_CHANNEL;
	priv->pa_iv_params.ports[0] = PORT_TYPE_U7;
	priv->pa_iv_params.ports[1] = PORT_TYPE_U8;

	write_reg_seq_array(codec,
		iv_2pa_dn_regs, ARRAY_SIZE(iv_2pa_dn_regs));

	ret = codec_bus_deactivate("2PA_IV", &priv->pa_iv_params);
	if (ret != 0)
		AUDIO_LOGE("track deactivate 2pa iv err: %d", ret);
}

static const struct reg_seq_config audioup_2mic_up_regs[] = {
	/* src in && out sample rate */
	{ { SRC_VLD_CTRL2_REG, 0, 0x1C, 0 }, 0, 0 },
	/* src disable clk */
	{ { S1_DP_CLK_EN_REG, BIT(S1_MIC2_SRC_CLK_EN_OFFSET) | BIT(S1_MIC1_SRC_CLK_EN_OFFSET),
		0x0, true }, 0, 0 },
	/* src down 2 mode */
	{ { MIC1_SRC_CTRL_REG, 0x7 << MIC1_SRC_MODE_OFFSET,
		SRC_MODE_2 << MIC1_SRC_MODE_OFFSET, true }, 0, 0 },
	{ { MIC2_SRC_CTRL_REG, 0x7 << MIC2_SRC_MODE_OFFSET,
		SRC_MODE_2 << MIC2_SRC_MODE_OFFSET, true }, 0, 0 },
	/* src enable clk */
	{ { S1_DP_CLK_EN_REG, BIT(S1_MIC2_SRC_CLK_EN_OFFSET) | BIT(S1_MIC1_SRC_CLK_EN_OFFSET),
		BIT(S1_MIC2_SRC_CLK_EN_OFFSET) | BIT(S1_MIC1_SRC_CLK_EN_OFFSET), true }, 0, 0 },
};

static const struct reg_seq_config audioup_2mic_dn_regs[] = {
	/* src power off */
	{ { S1_DP_CLK_EN_REG, BIT(S1_MIC2_SRC_CLK_EN_OFFSET) | BIT(S1_MIC1_SRC_CLK_EN_OFFSET), 0x0, true }, 0, 0 },
	{ { MIC2_SRC_CTRL_REG, 0x7 << MIC2_SRC_MODE_OFFSET, 0x0, true }, 0, 0 },
	{ { MIC1_SRC_CTRL_REG, 0x7 << MIC1_SRC_MODE_OFFSET, 0x0, true }, 0, 0 },
	{ { SRC_VLD_CTRL2_REG, 0, 0x0, false }, 0, 0 },
};

int audioup_2mic_enable(struct snd_soc_component *codec)
{
	int ret;
	struct da_combine_v5_platform_data *platform_data = snd_soc_component_get_drvdata(codec);

	AUDIO_LOGD("begin");

	platform_data->capture_params.channels = 2;
	platform_data->capture_params.ports[0] = PORT_TYPE_U1;
	platform_data->capture_params.ports[1] = PORT_TYPE_U2;

	write_reg_seq_array(codec, audioup_2mic_up_regs,
		ARRAY_SIZE(audioup_2mic_up_regs));

	u12_select_pga(codec);
	ret = codec_bus_activate("AUDIO_CAPTURE", &platform_data->capture_params);
	if (ret != 0)
		AUDIO_LOGE("track_activate capture err: %d", ret);

	AUDIO_LOGD("end");
	return ret;
}

int audioup_2mic_disable(struct snd_soc_component *codec)
{
	int ret = 0;
	struct da_combine_v5_platform_data *platform_data = snd_soc_component_get_drvdata(codec);

	AUDIO_LOGD("begin");

	if (platform_data->audioup_4mic_state == TRACK_FREE ||
		platform_data->audioup_5mic_state == TRACK_FREE) {
		u12_select_dspif(codec);
		ret = codec_bus_deactivate("AUDIO_CAPTURE", &platform_data->capture_params);
		if (ret != 0)
			AUDIO_LOGE("track_deactivate capture err: %d", ret);
		write_reg_seq_array(codec, audioup_2mic_dn_regs,
			ARRAY_SIZE(audioup_2mic_dn_regs));
	}

	AUDIO_LOGD("end");
	return ret;
}

static const struct reg_seq_config audioup_4mic_up_regs[] = {
	{ { S1_MIXER_EQ_CLK_EN_REG, BIT(RST_5MIC_S1_ACCESS_IRQ_OFFSET),
		BIT(RST_5MIC_S1_ACCESS_IRQ_OFFSET), true }, 0, 0 },
	/* src in && out sample rate */
	{ { SRC_VLD_CTRL2_REG, 0, 0x1C, false }, 0, 0 },
	/* src clk */
	{ { S1_DP_CLK_EN_REG, BIT(S1_MIC1_SRC_CLK_EN_OFFSET) |
		BIT(S1_MIC2_SRC_CLK_EN_OFFSET), 0x0, true }, 0, 0 },
	/* src down 2 mode */
	{ { MIC1_SRC_CTRL_REG, 0x7 << MIC1_SRC_MODE_OFFSET,
		SRC_MODE_2 << MIC1_SRC_MODE_OFFSET, true }, 0, 0 },
	{ { MIC2_SRC_CTRL_REG, 0x7 << MIC2_SRC_MODE_OFFSET,
		SRC_MODE_2 << MIC2_SRC_MODE_OFFSET, true }, 0, 0 },
	/* src clk */
	{ { S1_DP_CLK_EN_REG, BIT(S1_MIC1_SRC_CLK_EN_OFFSET) |
		BIT(S1_MIC2_SRC_CLK_EN_OFFSET), BIT(S1_MIC1_SRC_CLK_EN_OFFSET) |
		BIT(S1_MIC2_SRC_CLK_EN_OFFSET), true }, 0, 0 },
	{ { SRC_VLD_CTRL3_REG, 0, 0xC, false }, 0, 0 },
	{ { SRC_VLD_CTRL4_REG, 0, 0xC, false }, 0, 0 },
	{ { S1_DP_CLK_EN_REG, BIT(S1_MIC3_SRC_CLK_EN_OFFSET) |
		BIT(S1_MIC4_SRC_CLK_EN_OFFSET), 0x0, true }, 0, 0 },
	{ { MIC3_SRC_CTRL_REG, 0x7 << MIC3_SRC_MODE_OFFSET,
		SRC_MODE_2 << MIC3_SRC_MODE_OFFSET, true }, 0, 0 },
	{ { MIC4_SRC_CTRL_REG, 0x7 << MIC4_SRC_MODE_OFFSET,
		SRC_MODE_2 << MIC4_SRC_MODE_OFFSET, true }, 0, 0 },
	{ { S1_DP_CLK_EN_REG, BIT(S1_MIC3_SRC_CLK_EN_OFFSET) |
		BIT(S1_MIC4_SRC_CLK_EN_OFFSET), BIT(S1_MIC3_SRC_CLK_EN_OFFSET) |
		BIT(S1_MIC4_SRC_CLK_EN_OFFSET), true }, 0, 0 },
};

static const struct reg_seq_config audioup_4_2mic_up_regs[] = {
	/* src in && out sample rate */
	{ { SRC_VLD_CTRL2_REG, 0, 0x1C, false }, 0, 0 },
	/* src clk */
	{ { S1_DP_CLK_EN_REG, BIT(S1_MIC1_SRC_CLK_EN_OFFSET) |
		BIT(S1_MIC2_SRC_CLK_EN_OFFSET), 0x0, true }, 0, 0 },
	/* src down 2 mode */
	{ { MIC1_SRC_CTRL_REG, 0x7 << MIC1_SRC_MODE_OFFSET,
		SRC_MODE_2 << MIC1_SRC_MODE_OFFSET, true }, 0, 0 },
	{ { MIC2_SRC_CTRL_REG, 0x7 << MIC2_SRC_MODE_OFFSET,
		SRC_MODE_2 << MIC2_SRC_MODE_OFFSET, true }, 0, 0 },
	/* src clk */
	{ { S1_DP_CLK_EN_REG, BIT(S1_MIC1_SRC_CLK_EN_OFFSET) |
		BIT(S1_MIC2_SRC_CLK_EN_OFFSET), BIT(S1_MIC1_SRC_CLK_EN_OFFSET) |
		BIT(S1_MIC2_SRC_CLK_EN_OFFSET), true }, 0, 0 },
};

static const struct reg_seq_config audioup_4mic_dn_regs[] = {
	/* src power off */
	{ { S1_DP_CLK_EN_REG, BIT(S1_MIC1_SRC_CLK_EN_OFFSET) |
		BIT(S1_MIC2_SRC_CLK_EN_OFFSET), 0x0, true }, 0, 0 },
	{ { MIC1_SRC_CTRL_REG, 0x7 << MIC1_SRC_MODE_OFFSET, 0x0, true }, 0, 0 },
	{ { MIC2_SRC_CTRL_REG, 0x7 << MIC2_SRC_MODE_OFFSET, 0x0, true }, 0, 0 },
	{ { SRC_VLD_CTRL2_REG, 0, 0x0, false }, 0, 0 },
	{ { S1_DP_CLK_EN_REG, BIT(S1_MIC3_SRC_CLK_EN_OFFSET) |
		BIT(S1_MIC4_SRC_CLK_EN_OFFSET), 0x0, true }, 0, 0 },
	{ { MIC3_SRC_CTRL_REG, 0x7 << MIC3_SRC_MODE_OFFSET, 0x0, true }, 0, 0 },
	{ { MIC4_SRC_CTRL_REG, 0x7 << MIC4_SRC_MODE_OFFSET, 0x0, true }, 0, 0 },
	{ { SRC_VLD_CTRL3_REG, 0, 0x0, false }, 0, 0 },
	{ { SRC_VLD_CTRL4_REG, 0, 0x0, false }, 0, 0 },
};

static const struct reg_seq_config audioup_4_2mic_dn_regs[] = {
	{ { S1_DP_CLK_EN_REG, BIT(S1_MIC3_SRC_CLK_EN_OFFSET) |
		BIT(S1_MIC4_SRC_CLK_EN_OFFSET), 0x0, true }, 0, 0 },
	{ { MIC3_SRC_CTRL_REG, 0x7 << MIC3_SRC_MODE_OFFSET, 0x0, true }, 0, 0 },
	{ { MIC4_SRC_CTRL_REG, 0x7 << MIC4_SRC_MODE_OFFSET, 0x0, true }, 0, 0 },
	{ { SRC_VLD_CTRL3_REG, 0, 0x0, false }, 0, 0 },
	{ { SRC_VLD_CTRL4_REG, 0, 0x0, false }, 0, 0 },
};

int audioup_4mic_enable(struct snd_soc_component *codec)
{
	int ret;
	struct da_combine_v5_platform_data *platform_data = snd_soc_component_get_drvdata(codec);
	struct codec_dai_sample_rate_params hwparms;

	AUDIO_LOGD("begin");

	/* When calling and recording are concurrent, channels drops from 4 to 2. */
	if (platform_data->voiceup_state && (platform_data->voice_up_params.channels == 4))
		platform_data->capture_params.channels = 2;
	else
		platform_data->capture_params.channels = 4;

	platform_data->capture_params.ports[0] = PORT_TYPE_U1;
	platform_data->capture_params.ports[1] = PORT_TYPE_U2;

	params_sample_rate(hwparms, PORT_U1_U2, SAMPLE_RATE_48K);
	platform_data->codec_dai->config_sample_rate(codec, &hwparms);

	if (platform_data->capture_params.channels == 4) {
		params_sample_rate(hwparms, PORT_U3_U4, SAMPLE_RATE_48K);
		platform_data->codec_dai->config_sample_rate(codec, &hwparms);
		platform_data->capture_params.ports[2] = PORT_TYPE_U3;
		platform_data->capture_params.ports[3] = PORT_TYPE_U4;
		write_reg_seq_array(codec, audioup_4mic_up_regs,
			ARRAY_SIZE(audioup_4mic_up_regs));
	} else if (platform_data->capture_params.channels == 2) {
		write_reg_seq_array(codec, audioup_4_2mic_up_regs,
			ARRAY_SIZE(audioup_4_2mic_up_regs));
	}

	u12_select_pga(codec);

	ret = codec_bus_activate("AUDIO_CAPTURE", &platform_data->capture_params);
	if (ret != 0)
		AUDIO_LOGE("activate capture err: %d", ret);
	if (platform_data->capture_params.channels == 4)
		da_combine_update_bits(codec, S1_MIXER_EQ_CLK_EN_REG,
			BIT(RST_5MIC_S1_ACCESS_IRQ_OFFSET), 0);

	platform_data->audioup_4mic_state = TRACK_STARTUP;

	AUDIO_LOGD("end");
	return ret;
}

int audioup_4mic_disable(struct snd_soc_component *codec)
{
	int ret = 0;
	struct da_combine_v5_platform_data *platform_data = snd_soc_component_get_drvdata(codec);

	AUDIO_LOGD("begin");

	/* When calling and recording are concurrent, channels drops from 4 to 2. */
	if (platform_data->voiceup_state && (platform_data->voice_up_params.channels == 4))
		platform_data->capture_params.channels = 2;
	else
		platform_data->capture_params.channels = 4;

	platform_data->capture_params.ports[0] = PORT_TYPE_U1;
	platform_data->capture_params.ports[1] = PORT_TYPE_U2;

	u12_select_dspif(codec);

	ret = codec_bus_deactivate("AUDIO_CAPTURE", &platform_data->capture_params);
	if (ret != 0)
		AUDIO_LOGE("deactivate capture err: %d", ret);
	if (platform_data->capture_params.channels == 4) {
		platform_data->capture_params.ports[2] = PORT_TYPE_U3;
		platform_data->capture_params.ports[3] = PORT_TYPE_U4;
		write_reg_seq_array(codec, audioup_4mic_dn_regs,
			ARRAY_SIZE(audioup_4mic_dn_regs));
	} else if (platform_data->capture_params.channels == 2) {
		write_reg_seq_array(codec, audioup_4_2mic_dn_regs,
			ARRAY_SIZE(audioup_4_2mic_dn_regs));
	}

	platform_data->audioup_4mic_state = TRACK_FREE;

	AUDIO_LOGD("end");
	return ret;
}

void up_src_pre_pmu(struct snd_soc_component *codec, unsigned int channel, unsigned int sample_rate,
	unsigned int sample_rate_reg_offset, unsigned int src_mode)
{
	struct da_combine_v5_platform_data *platform_data = snd_soc_component_get_drvdata(codec);
	struct codec_dai_sample_rate_params hwparms;

	/* src in && out sample rate */
	da_combine_update_bits(codec, SRC_VLD_CTRL7_REG,
		BIT(S3_OR_SRC_DIN_VLD_SEL_OFFSET) | BIT(S3_OL_SRC_DIN_VLD_SEL_OFFSET),
		BIT(S3_OR_SRC_DIN_VLD_SEL_OFFSET) | BIT(S3_OL_SRC_DIN_VLD_SEL_OFFSET));
	da_combine_update_bits(codec, SRC_VLD_CTRL7_REG,
		0x7 << S3_O_SRC_DOUT_VLD_SEL_OFFSET,
		sample_rate_reg_offset << S3_O_SRC_DOUT_VLD_SEL_OFFSET);
	/* src clk */
	da_combine_update_bits(codec, S3_DP_CLK_EN_REG,
		BIT(S3_OL_SRC_CLK_EN_OFFSET) | BIT(S3_OR_SRC_CLK_EN_OFFSET), 0x0);
	/* src down xx mode */
	da_combine_update_bits(codec, S3_OL_SRC_CTRL_REG,
		0x7 << S3_OL_SRC_MODE_OFFSET, src_mode << S3_OL_SRC_MODE_OFFSET);
	da_combine_update_bits(codec, S3_OR_SRC_CTRL_REG,
		0x7 << S3_OR_SRC_MODE_OFFSET, src_mode << S3_OR_SRC_MODE_OFFSET);
	/* src clk */
	da_combine_update_bits(codec, S3_DP_CLK_EN_REG,
		BIT(S3_OL_SRC_CLK_EN_OFFSET) | BIT(S3_OR_SRC_CLK_EN_OFFSET),
		BIT(S3_OL_SRC_CLK_EN_OFFSET) | BIT(S3_OR_SRC_CLK_EN_OFFSET));
	/* slimbus sample rate */
	params_sample_rate(hwparms, PORT_U5_U6, sample_rate);
	platform_data->codec_dai->config_sample_rate(codec, &hwparms);

	if (channel == 4) {
		/* mic3/4 in&&out sample rate */
		da_combine_update_bits(codec, SRC_VLD_CTRL3_REG,
			BIT(MIC3_SRC_DIN_VLD_SEL_OFFSET) | (0x7 << MIC3_SRC_DOUT_VLD_SEL_OFFSET),
			(unsigned int)BIT(MIC3_SRC_DIN_VLD_SEL_OFFSET) | (sample_rate_reg_offset << MIC3_SRC_DOUT_VLD_SEL_OFFSET));
		da_combine_update_bits(codec, SRC_VLD_CTRL4_REG,
			BIT(MIC4_SRC_DIN_VLD_SEL_OFFSET) | (0x7 << MIC4_SRC_DOUT_VLD_SEL_OFFSET),
			(unsigned int)BIT(MIC4_SRC_DIN_VLD_SEL_OFFSET) | (sample_rate_reg_offset << MIC4_SRC_DOUT_VLD_SEL_OFFSET));
		/* u3/u4 clk clk */
		da_combine_update_bits(codec, S1_DP_CLK_EN_REG,
			BIT(S1_MIC4_SRC_CLK_EN_OFFSET) | BIT(S1_MIC3_SRC_CLK_EN_OFFSET), 0x0);
		/* u3/u4 src mode */
		da_combine_update_bits(codec, MIC3_SRC_CTRL_REG,
			0x7 << MIC3_SRC_MODE_OFFSET,
			src_mode << MIC3_SRC_MODE_OFFSET);
		da_combine_update_bits(codec, MIC4_SRC_CTRL_REG,
			0x7 << MIC4_SRC_MODE_OFFSET,
			src_mode << MIC4_SRC_MODE_OFFSET);
		/* u3/u4 clk clk */
		da_combine_update_bits(codec, S1_DP_CLK_EN_REG,
			BIT(S1_MIC4_SRC_CLK_EN_OFFSET) | BIT(S1_MIC3_SRC_CLK_EN_OFFSET),
			BIT(S1_MIC4_SRC_CLK_EN_OFFSET) | BIT(S1_MIC3_SRC_CLK_EN_OFFSET));

		/* u3/u4 slimbus sample rate */
		params_sample_rate(hwparms, PORT_U3_U4, sample_rate);
		platform_data->codec_dai->config_sample_rate(codec, &hwparms);
	}
}

void up_src_post_pmu(struct snd_soc_component *codec, unsigned int channel)
{
	struct da_combine_v5_platform_data *platform_data = snd_soc_component_get_drvdata(codec);
	struct codec_dai_sample_rate_params hwparms;

	/* slimbus sample rate */
	params_sample_rate(hwparms, PORT_U5_U6, SAMPLE_RATE_8K);
	platform_data->codec_dai->config_sample_rate(codec, &hwparms);
	/* src power off */
	da_combine_update_bits(codec, S3_DP_CLK_EN_REG,
		BIT(S3_OL_SRC_CLK_EN_OFFSET) | BIT(S3_OR_SRC_CLK_EN_OFFSET), 0x0);
	da_combine_update_bits(codec, S3_OL_SRC_CTRL_REG,
		0x7 << S3_OL_SRC_MODE_OFFSET, 0x0);
	da_combine_update_bits(codec, S3_OR_SRC_CTRL_REG,
		0x7 << S3_OR_SRC_MODE_OFFSET, 0x0);
	da_combine_update_bits(codec, SRC_VLD_CTRL7_REG,
		0x7 << S3_O_SRC_DOUT_VLD_SEL_OFFSET, 0);
	da_combine_update_bits(codec, SRC_VLD_CTRL7_REG,
		BIT(S3_OR_SRC_DIN_VLD_SEL_OFFSET) | BIT(S3_OL_SRC_DIN_VLD_SEL_OFFSET), 0x0);
	if (channel == 4) {
		da_combine_update_bits(codec, S1_DP_CLK_EN_REG,
			BIT(S1_MIC4_SRC_CLK_EN_OFFSET) | BIT(S1_MIC3_SRC_CLK_EN_OFFSET), 0x0);
		params_sample_rate(hwparms, PORT_U3_U4, SAMPLE_RATE_48K);
		platform_data->codec_dai->config_sample_rate(codec, &hwparms);
		da_combine_update_bits(codec, SRC_VLD_CTRL3_REG,
			BIT(MIC3_SRC_DIN_VLD_SEL_OFFSET) | (0x7 << MIC3_SRC_DOUT_VLD_SEL_OFFSET), 0x0);
		da_combine_update_bits(codec, SRC_VLD_CTRL4_REG,
			BIT(MIC4_SRC_DIN_VLD_SEL_OFFSET) | (0x7 << MIC4_SRC_DOUT_VLD_SEL_OFFSET), 0x0);
		da_combine_update_bits(codec, MIC3_SRC_CTRL_REG,
			0x7 << MIC3_SRC_MODE_OFFSET, 0x0);
		da_combine_update_bits(codec, MIC4_SRC_CTRL_REG,
			0x7 << MIC4_SRC_MODE_OFFSET, 0x0);
	}
}

void up_src_pre_pmu_v2(struct snd_soc_component *codec, unsigned int channel,
	unsigned int sample_rate, unsigned int sample_rate_reg_offset, unsigned int src_mode)
{
	struct da_combine_v5_platform_data *platform_data = snd_soc_component_get_drvdata(codec);
	struct codec_dai_sample_rate_params hwparms;

	/* mic3 in&&out sample rate */
	da_combine_update_bits(codec, SRC_VLD_CTRL3_REG,
		BIT(MIC3_SRC_DIN_VLD_SEL_OFFSET) | (0x7 << MIC3_SRC_DOUT_VLD_SEL_OFFSET),
		(unsigned int)BIT(MIC3_SRC_DIN_VLD_SEL_OFFSET) | (sample_rate_reg_offset << MIC3_SRC_DOUT_VLD_SEL_OFFSET));
	/* mic4 in&&out sample rate */
	da_combine_update_bits(codec, SRC_VLD_CTRL4_REG,
		BIT(MIC4_SRC_DIN_VLD_SEL_OFFSET) | (0x7 << MIC4_SRC_DOUT_VLD_SEL_OFFSET),
		(unsigned int)BIT(MIC4_SRC_DIN_VLD_SEL_OFFSET) | (sample_rate_reg_offset << MIC4_SRC_DOUT_VLD_SEL_OFFSET));
	/* s1_mic3/mic4 src clk close */
	da_combine_update_bits(codec, S1_DP_CLK_EN_REG,
		BIT(S1_MIC4_SRC_CLK_EN_OFFSET) | BIT(S1_MIC3_SRC_CLK_EN_OFFSET), 0x0);
	/* mic3/mic4 src mode */
	da_combine_update_bits(codec, MIC3_SRC_CTRL_REG, 0x7 << MIC3_SRC_MODE_OFFSET,
		src_mode << MIC3_SRC_MODE_OFFSET);
	da_combine_update_bits(codec, MIC4_SRC_CTRL_REG, 0x7 << MIC4_SRC_MODE_OFFSET,
		src_mode << MIC4_SRC_MODE_OFFSET);
	/* s1_mic3/mic4 src clk open */
	da_combine_update_bits(codec, S1_DP_CLK_EN_REG,
		BIT(S1_MIC4_SRC_CLK_EN_OFFSET) | BIT(S1_MIC3_SRC_CLK_EN_OFFSET),
		BIT(S1_MIC4_SRC_CLK_EN_OFFSET) | BIT(S1_MIC3_SRC_CLK_EN_OFFSET));
	/* u3/u4 slimbus sample rate */
	params_sample_rate(hwparms, PORT_U3_U4, sample_rate);
	platform_data->codec_dai->config_sample_rate(codec, &hwparms);

	if (channel == 4) {
		/* u10 data source: mic_src */
		da_combine_update_bits(codec, SC_CODEC_MUX_CTRL25_REG, 0x3 << SLIM_UP10_DATA_SEL_OFFSET,
			0x3 << SLIM_UP10_DATA_SEL_OFFSET);
		/* mic5 in&&out sample rate */
		da_combine_update_bits(codec, SRC_VLD_CTRL5_REG,
			BIT(MIC5_SRC_DIN_VLD_SEL_OFFSET) | (0x7 << MIC5_SRC_DOUT_VLD_SEL_OFFSET),
			(unsigned int)BIT(MIC5_SRC_DIN_VLD_SEL_OFFSET) | (sample_rate_reg_offset << MIC5_SRC_DOUT_VLD_SEL_OFFSET));
		/* mic5 src clk close */
		da_combine_update_bits(codec, S4_DP_CLK_EN_REG, BIT(S1_MIC5_SRC_CLK_EN_OFFSET),
			0x0 << S1_MIC5_SRC_CLK_EN_OFFSET);
		/* mic5 src mode */
		da_combine_update_bits(codec, MIC5_SRC_CTRL_REG, 0x7 << MIC5_SRC_MODE_OFFSET,
			src_mode << MIC5_SRC_MODE_OFFSET);
		/* mic5 src clk open */
		da_combine_update_bits(codec, S4_DP_CLK_EN_REG, BIT(S1_MIC5_SRC_CLK_EN_OFFSET),
			BIT(S1_MIC5_SRC_CLK_EN_OFFSET));
		if (platform_data->cdc_ctrl->bus_sel == BUSTYPE_SELECT_SLIMBUS) {
			/* 0x20007033 0x0, Mic5-Port7, Port6 and Port7 not use the same ACK */
			da_combine_update_bits(codec, SLIM_CTRL3_REG, BIT(SLIM_DPORT6_DPORT7_ACK_OFFSET),
				0 << BIT(SLIM_DPORT6_DPORT7_ACK_OFFSET));
			/* u10 slimbus sample rate */
			params_sample_rate(hwparms, PORT_U10, sample_rate);
			platform_data->codec_dai->config_sample_rate(codec, &hwparms);
		} else {
			/* u11 soundwire sample rate */
			params_sample_rate(hwparms, PORT_U11_U12, sample_rate);
			platform_data->codec_dai->config_sample_rate(codec, &hwparms);
		}
	}
}

void up_src_post_pmu_v2(struct snd_soc_component *codec, unsigned int channel)
{
	struct da_combine_v5_platform_data *platform_data = snd_soc_component_get_drvdata(codec);
	struct codec_dai_sample_rate_params hwparms;

	/* u3/u4 slimbus sample rate revert to init value 48k */
	params_sample_rate(hwparms, PORT_U3_U4, SAMPLE_RATE_48K);
	platform_data->codec_dai->config_sample_rate(codec, &hwparms);
	/* s1_mic3/mic4 src clk close */
	da_combine_update_bits(codec, S1_DP_CLK_EN_REG,
		BIT(S1_MIC4_SRC_CLK_EN_OFFSET) | BIT(S1_MIC3_SRC_CLK_EN_OFFSET), 0x0);
	/* mic3 in&&out sample rate revert to default value in: 48k, out: 8k */
	da_combine_update_bits(codec, SRC_VLD_CTRL3_REG,
		BIT(MIC3_SRC_DIN_VLD_SEL_OFFSET) | (0x7 << MIC3_SRC_DOUT_VLD_SEL_OFFSET), 0x0);
	/* mic4 in&&out sample rate revert to default value in: 48k, out: 8k */
	da_combine_update_bits(codec, SRC_VLD_CTRL4_REG,
		BIT(MIC4_SRC_DIN_VLD_SEL_OFFSET) | (0x7 << MIC4_SRC_DOUT_VLD_SEL_OFFSET), 0x0);
	/* mic3 src mode: 3times */
	da_combine_update_bits(codec, MIC3_SRC_CTRL_REG, 0x7 << MIC3_SRC_MODE_OFFSET, 0x0);
	da_combine_update_bits(codec, MIC4_SRC_CTRL_REG, 0x7 << MIC4_SRC_MODE_OFFSET, 0x0);

	if (channel == 4) {
		if (platform_data->cdc_ctrl->bus_sel == BUSTYPE_SELECT_SLIMBUS) {
			/* 0x20007033  Mic5-Port7, Port6 and Port7 revert to use the same ACK */
			da_combine_update_bits(codec, SLIM_CTRL3_REG, BIT(SLIM_DPORT6_DPORT7_ACK_OFFSET),
				BIT(SLIM_DPORT6_DPORT7_ACK_OFFSET));
			/* u10 slimbus sample rate revert to init value 48k */
			params_sample_rate(hwparms, PORT_U10, SAMPLE_RATE_48K);
			platform_data->codec_dai->config_sample_rate(codec, &hwparms);
		} else {
			/* u11 soundwire sample rate revert to init value 48k */
			params_sample_rate(hwparms, PORT_U11_U12, SAMPLE_RATE_48K);
			platform_data->codec_dai->config_sample_rate(codec, &hwparms);
		}
		/* mic5 src clk close */
		da_combine_update_bits(codec, S4_DP_CLK_EN_REG, BIT(S1_MIC5_SRC_CLK_EN_OFFSET),
			0x0 << S1_MIC5_SRC_CLK_EN_OFFSET);
		/* mic5 in&&out sample rate revert to default value in: 48k, out: 8k */
		da_combine_update_bits(codec, SRC_VLD_CTRL5_REG,
			BIT(MIC5_SRC_DIN_VLD_SEL_OFFSET) | (0x7 << MIC5_SRC_DOUT_VLD_SEL_OFFSET), 0x0);
		/* mic5 src mode: 3times */
		da_combine_update_bits(codec, MIC5_SRC_CTRL_REG, 0x7 << MIC5_SRC_MODE_OFFSET, 0x0);
	}
}

void update_callswitch_state(int event,
	struct da_combine_v5_platform_data *platform_data)
{
	struct da_combine_v5_board_cfg board_cfg = platform_data->board_config;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		platform_data->is_callswitch_on = true;
		if (board_cfg.mic_control_sc_freq_enable) {
#ifdef CONFIG_HUAWEI_AUDIO
			vsys_switch_set_sc_frequency_mode(1); /* auto mode */
#endif
			AUDIO_LOGI("callswitch event update, set sc frequency auto mode");
		}
		break;
	case SND_SOC_DAPM_POST_PMD:
		platform_data->is_callswitch_on = false;
		break;
	default:
		AUDIO_LOGW("power event err: %d", event);
		break;
	}
}

int voice_v2_power_event(struct snd_soc_dapm_widget *w,
	int event, uint32_t rate)
{
	struct snd_soc_component *codec = snd_soc_dapm_to_component(w->dapm);
	struct da_combine_v5_platform_data *platform_data = snd_soc_component_get_drvdata(codec);
	uint32_t rate_cfg;
	uint32_t rate_mode;
	int ret = 0;

	/* update channels to 2 */
	platform_data->voice_up_params.channels = 2;
	platform_data->voice_up_params.ports[0] = PORT_TYPE_U3;
	platform_data->voice_up_params.ports[1] = PORT_TYPE_U4;

	platform_data->voice_up_params.rate = rate;
	if (rate == SAMPLE_RATE_8K) {
		rate_cfg = SAMPLE_RATE_REG_CFG_8K;
		rate_mode = SRC_MODE_12;
	} else if (rate == SAMPLE_RATE_16K) {
		rate_cfg = SAMPLE_RATE_REG_CFG_16K;
		rate_mode = SRC_MODE_6;
	} else {
		rate_cfg = SAMPLE_RATE_REG_CFG_32K;
		rate_mode = SRC_MODE_3;
	}

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		up_src_pre_pmu_v2(codec, 2, rate, rate_cfg, rate_mode);

		ret = codec_bus_activate("VOICE_UP", &platform_data->voice_up_params);
		if (ret != 0)
			AUDIO_LOGE("activate voice up v2 err: %d", ret);

		platform_data->voiceup_v2_state = TRACK_STARTUP;
		break;
	case SND_SOC_DAPM_POST_PMD:
		ret = codec_bus_deactivate("VOICE_UP", &platform_data->voice_up_params);
		if (ret != 0)
			AUDIO_LOGE("deactivate voice up v2 err: %d", ret);

		platform_data->voiceup_v2_state = TRACK_FREE;

		up_src_post_pmu_v2(codec, 2);
		break;
	default:
		AUDIO_LOGW("power event err: %d", event);
		break;
	}

	return ret;
}

void u9_select_dspif(struct snd_soc_component *codec)
{
	da_combine_update_bits(codec, SC_CODEC_MUX_CTRL25_REG,
		0x3 << SLIM_UP9_DATA_SEL_OFFSET,
		BIT(SLIM_UP9_DATA_SEL_OFFSET));
}

void u9_select_pga(struct snd_soc_component *codec)
{
	da_combine_update_bits(codec, SC_CODEC_MUX_CTRL25_REG,
		0x3 << SLIM_UP9_DATA_SEL_OFFSET,
		0x0 << SLIM_UP9_DATA_SEL_OFFSET);
}

void ec_switch_enable(struct snd_soc_component *codec)
{
	struct da_combine_v5_platform_data *platform_data = snd_soc_component_get_drvdata(codec);
	struct codec_dai_sample_rate_params hwparms;
	int ret;

	/* close u7 src clk */
	da_combine_update_bits(codec, S4_DP_CLK_EN_REG,
		BIT(S4_SPA_R_SRC_CLK_EN_OFFSET), 0x0);

	/* sel u7 data source */
	da_combine_update_bits(codec, SC_CODEC_MUX_CTRL37_REG,
		0x3 << SLIM_SW_UP7_DATA_SEL_OFFSET, 0x0);

	/* config u7 48k */
	params_sample_rate(hwparms, PORT_U7_U8, SAMPLE_RATE_48K);
	platform_data->codec_dai->config_sample_rate(codec, &hwparms);

	/* active ec */
	ret = codec_bus_activate("ECREF", &platform_data->ec_params);
	if (ret != 0)
		AUDIO_LOGE("track activate ec err: %d", ret);
}

void ec_switch_disable(struct snd_soc_component *codec)
{
	struct da_combine_v5_platform_data *platform_data = snd_soc_component_get_drvdata(codec);
	struct codec_dai_sample_rate_params hwparms;
	int ret;

	ret = codec_bus_deactivate("ECREF", &platform_data->ec_params);
	if (ret != 0)
		AUDIO_LOGE("track deactivate ec err: %d", ret);

	/* sel reg default config */
	params_sample_rate(hwparms, PORT_U7_U8, SAMPLE_RATE_48K);
	platform_data->codec_dai->config_sample_rate(codec, &hwparms);

	/* sel reg default config */
	da_combine_update_bits(codec, SC_CODEC_MUX_CTRL37_REG,
		0x3 << SLIM_SW_UP7_DATA_SEL_OFFSET, 0x0);

	/* sel reg default config */
	da_combine_update_bits(codec, S4_DP_CLK_EN_REG,
		BIT(S4_SPA_R_SRC_CLK_EN_OFFSET), 0x0);
}

