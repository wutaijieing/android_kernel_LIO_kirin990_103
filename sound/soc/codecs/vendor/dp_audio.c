/*
 * DP audio codec driver.
 *
 * Copyright (c) 2015 Huawei Technologies Co., Ltd.
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

#include "dp_audio.h"

#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/jack.h>

#include "audio/include/audio_log.h"
#include "audio_pcm_dp.h"

#define DP_AUDIO_MIN_CHANNELS 1
#define DP_AUDIO_MAX_CHANNELS 8
#define DP_AUDIO_FORMATS     (SNDRV_PCM_FMTBIT_S16_LE | \
		SNDRV_PCM_FMTBIT_S16_BE | \
		SNDRV_PCM_FMTBIT_S24_LE | \
		SNDRV_PCM_FMTBIT_S24_BE | \
		SNDRV_PCM_FMTBIT_S32_LE | \
		SNDRV_PCM_FMTBIT_S32_BE)

#define DP_AUDIO_RATES    SNDRV_PCM_RATE_8000_192000

#define AUDIO_DP_NAME "audio-dp"

#define LOG_TAG "audio dp"

struct dp_audio_platform_data {
	bool is_dp_plug_in;
	bool enable_set_audio_config_with_kernel;
	bool report_enable;
	struct snd_jack *jack;
};

static struct dp_audio_platform_data *dp_audio_pdata;

static void get_dp_audio_cfg(const struct device_node *node,
	struct dp_audio_platform_data *pdata)
{
	pdata->enable_set_audio_config_with_kernel =
		of_property_read_bool(node, "enable_set_audio_config_with_kernel");
	if (pdata->enable_set_audio_config_with_kernel)
		AUDIO_LOGI("enable_set_audio_config_with_kernel");
	else
		AUDIO_LOGI("should set dp audio params by user");
}

extern int dpu_dptx_set_aparam(uint32_t channel_num, uint32_t data_width, uint32_t sample_rate);

static void set_dp_audio_params(struct snd_pcm_hw_params *params)
{
	unsigned int channels = params_channels(params);
	unsigned int data_width = hw_param_interval(params, SNDRV_PCM_HW_PARAM_SAMPLE_BITS)->min;
	unsigned int sample_rate = params_rate(params);

	if (dp_audio_pdata->is_dp_plug_in && dp_audio_pdata->enable_set_audio_config_with_kernel) {
		dpu_dptx_set_aparam(channels, data_width, sample_rate);
		AUDIO_LOGI("channels %u, data_width %u, sample_rate %u",
			channels, data_width, sample_rate);
	}
}

static int dp_audio_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	set_dp_audio_params(params);

	return 0;
}

static int dp_audio_hw_free(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	return 0;
}

struct snd_soc_dai_ops dp_audio_dai_ops = {
	.hw_params  = dp_audio_hw_params,
	.hw_free = dp_audio_hw_free,
};

struct snd_soc_dai_driver dp_audio_dai[] = {
	{
		.name = "audio-dp-dai",
		.playback = {
			.stream_name    = "dp-playback",
			.channels_min   = DP_AUDIO_MIN_CHANNELS,
			.channels_max   = DP_AUDIO_MAX_CHANNELS,
			.rates          = DP_AUDIO_RATES,
			.formats        = DP_AUDIO_FORMATS,
		},
		.ops = &dp_audio_dai_ops,
	},
};

static const struct of_device_id dp_audio_of_match[] = {
	{
		.compatible = "hisilicon,dp-audio",
	},
	{ },
};

MODULE_DEVICE_TABLE(of, dp_audio_of_match);

/**
 * audio_dp_jack_report - Report the current status of dp audio jack
 * @is_plug_in: false if plug out, true is plug in
 *
 * We can enable the jack report events in device tree source.
 */
void audio_dp_jack_report(bool is_plug_in)
{
	int ret;

	if (dp_audio_pdata == NULL) {
		AUDIO_LOGE("dp audio platform data is null");
		return;
	}
	if (!dp_audio_pdata->report_enable) {
		AUDIO_LOGI("dp audio jack report is not enabled");
		return;
	}
	if (dp_audio_pdata->jack == NULL) {
		AUDIO_LOGE("dp audio jack is null");
		return;
	}

	ret = stop_pcm_dp_substream();
	if (ret != 0) {
		AUDIO_LOGE("substream stop failed, err code: %d", ret);
		return;
	}

	if (is_plug_in)
		snd_jack_report(dp_audio_pdata->jack, SND_JACK_AVOUT);
	else
		snd_jack_report(dp_audio_pdata->jack, 0);

	dp_audio_pdata->is_dp_plug_in = is_plug_in;
	AUDIO_LOGI("hdmi/dp cable is %s", is_plug_in ? "plug in" : "plug out");
}

static int dp_audio_jack_new(struct snd_soc_component *codec,
	struct snd_jack **jjack)
{
	int ret = snd_jack_new(codec->card->snd_card, "HDMI/DP Jack",
		SND_JACK_AVOUT, jjack, true, false);
	if (ret != 0) {
		AUDIO_LOGE("dp audio jack new failed with error %d", ret);
		return ret;
	}

	audio_dp_jack_report(false);
	return 0;
}

static int dp_audio_codec_probe(struct snd_soc_component *codec)
{
	int ret;
	struct dp_audio_platform_data *pdata = snd_soc_component_get_drvdata(codec);

	if (!pdata->report_enable)
		return 0;

	ret = dp_audio_jack_new(codec, &pdata->jack);
	if (ret != 0)
		AUDIO_LOGE("probe failed with error %d", ret);

	return ret;
}

static void dp_audio_codec_remove(struct snd_soc_component *codec)
{
	return;
}

static struct snd_soc_component_driver dp_audio_dev = {
	.name		= AUDIO_DP_NAME,
	.probe		= dp_audio_codec_probe,
	.remove		= dp_audio_codec_remove,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	.idle_bias_on = true,
#endif
};

static void get_jack_report_cfg(const struct device_node *node,
	bool *report_enable)
{
	if (of_property_read_bool(node, "jack_report_enable"))
		*report_enable = true;
	else
		*report_enable = false;

	AUDIO_LOGI("read jack report enable status is %d", *report_enable);
}

static int dp_audio_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *node = pdev->dev.of_node;
	struct dp_audio_platform_data *pdata =
		devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (pdata == NULL) {
		AUDIO_LOGE("malloc dp audio platform data failed");
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, pdata);
	dp_audio_pdata = pdata;

	get_jack_report_cfg(node, &pdata->report_enable);

	get_dp_audio_cfg(node, pdata);

	dev_set_name(dev, AUDIO_DP_NAME);
	ret = devm_snd_soc_register_component(dev, &dp_audio_dev,
			dp_audio_dai, ARRAY_SIZE(dp_audio_dai));
	if (ret != 0)
		AUDIO_LOGE("register failed with error %d", ret);

	AUDIO_LOGI("register ok");

	return ret;
}

static struct platform_driver dp_audio_driver = {
	.driver = {
		.name  = AUDIO_DP_NAME,
		.owner = THIS_MODULE,
		.of_match_table = dp_audio_of_match,
	},
	.probe  = dp_audio_probe,
};

module_platform_driver(dp_audio_driver);

MODULE_DESCRIPTION("ASoC dp audio driver");
MODULE_LICENSE("GPL");
