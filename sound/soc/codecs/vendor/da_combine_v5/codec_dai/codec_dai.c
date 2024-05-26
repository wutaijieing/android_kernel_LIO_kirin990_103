/*
 * codec_dai.c -- codec dai driver
 *
 * Copyright (c) 2020 Huawei Technologies Co., Ltd.
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
#include "codec_dai.h"

#include <sound/soc-dai.h>

#include <linux/platform_drivers/da_combine/da_combine_v5.h>
#include <linux/platform_drivers/da_combine/da_combine_v5_regs.h>
#include <linux/platform_drivers/da_combine/da_combine_resmgr.h>
#include "codec_bus.h"
#include "audio_log.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "codec_dai"

int codec_dai_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	int ret = 0;
	int rate;

	if (params == NULL) {
		AUDIO_LOGE("pcm params is null");
		return -EINVAL;
	}

	if (dai == NULL) {
		AUDIO_LOGE("soc dai is null");
		return -EINVAL;
	}

	rate = params_rate(params);
	switch (rate) {
	case 8000:
	case 11025:
	case 16000:
	case 22050:
	case 32000:
	case 44100:
	case 48000:
	case 88200:
	case 96000:
	case 176400:
	case 192000:
		break;
	case 384000:
		AUDIO_LOGE("rate: %d", rate);
		break;
	default:
		AUDIO_LOGE("unknown rate: %d", rate);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static void codec_dai_update_dac_power_state(struct snd_soc_component *codec, bool on)
{
	if (on) {
		snd_soc_component_write_adapter(codec, CODEC_ANA_RWREG_09, 0x43);
		snd_soc_component_write_adapter(codec, CODEC_ANA_RWREG_010, 0x3);
	} else {
		snd_soc_component_write_adapter(codec, CODEC_ANA_RWREG_09, 0x7f);
		snd_soc_component_write_adapter(codec, CODEC_ANA_RWREG_010, 0x7f);
	}
}

int codec_dai_digital_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_component *codec = dai->component;

	if (!of_property_read_bool(codec->dev->of_node, "hisilicon,digital_mute_enable"))
		return 0;

	if (mute == 1)
		codec_dai_update_dac_power_state(codec, false);
	else
		codec_dai_update_dac_power_state(codec, true);

	return 0;
}

int codec_dai_mute_stream(struct snd_soc_dai *dai, int mute, int stream)
{
	return codec_dai_digital_mute(dai, mute);
}

int codec_dai_probe(struct snd_soc_dai *dai)
{
	struct da_combine_v5_platform_data *priv = snd_soc_dai_get_drvdata(dai);
	struct snd_soc_component *codec = priv->codec;
	int ret;

	ret = codec_bus_runtime_get();
	if (ret != 0) {
		AUDIO_LOGE("get runtime failed");
		return ret;
	}

	/* open codec pll and asp clk to make sure codec framer be enumerated */
	ret = da_combine_resmgr_request_pll(priv->resmgr, PLL_HIGH);
	if (ret != 0) {
		AUDIO_LOGE("request pll failed");
		codec_bus_runtime_put();
		return ret;
	}

	priv->codec_dai->codec_dai_init(codec);
	ret = codec_bus_enum_device();
	if (ret != 0)
		AUDIO_LOGE("enum device failed, ret = %d", ret);

	ret = da_combine_resmgr_release_pll(priv->resmgr, PLL_HIGH);
	if (ret != 0)
		AUDIO_LOGE("release pll failed");

	codec_bus_runtime_put();

	return ret;
}
