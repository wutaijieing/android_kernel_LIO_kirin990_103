/*
 * da_combine_machine.c
 *
 * is the machine driver of alsa which is used to registe sound card
 *
 * Copyright (c) 2014-2020 Huawei Technologies Co., Ltd.
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

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <sound/soc.h>
#include <linux/version.h>

#include "audio_log.h"

#define LOG_TAG "da_combine"

#define EXTERN_HIFI_CODEC_CODECLESS_NAME       "codecless"
#define EXTERN_HIFI_CODEC_WITHOUT_HIFI         "codec_without_hifi"
#define EXTERN_HIFI_CODEC_AK4376_NAME          "ak4376"

#define DA_COMBINE_V5_MACHINE_DRIVER_NAME      "da_combine_v5_machine"
#define DA_COMBINE_V5_MACHINE_CODECLESS_NAME   "da_combine_v5_machine_codecless_card"
#define DA_COMBINE_V5_MACHINE_CARD_NAME        "da_combine_v5_machine_card"

#define DAI_LINK_CODEC_NAME                    "da_combine_v5"
#define DAI_LINK_CODEC_DAI_NAME_AUDIO          "da_combine_v5-audio-dai"
#define DAI_LINK_CODEC_DAI_NAME_BT             "da_combine_v5-bt-dai"

#define DA_COMBINE_V3_MACHINE_DRIVER_NAME      "da_combine_v3-machine"
#define DA_COMBINE_V3_MACHINE_AK4376_CARD_NAME "da_combine_v3-machine_akxxxx_card"
#define DA_COMBINE_V3_MACHINE_CARD_NAME        "da_combine_v3-machine_card"

#define DAI_LINK_CODEC_NAME_V3                 "da_combine_v3"
#define DAI_LINK_CODEC_DAI_NAME_AUDIO_V3       "da_combine_v3-audio-dai"
#define DAI_LINK_CODEC_DAI_NAME_VOICE_V3       "da_combine_v3-voice-dai"
#define DAI_LINK_CODEC_DAI_NAME_FM_V3          "da_combine_v3-fm-dai"

/* pcm devices */
#define PLAYBACK_CHANNELS_MIN       1
#define PLAYBACK_CHANNELS_MAX       4
#define CAPTURE_CHANNELS_MIN        1
#define CAPTURE_CHANNELS_MAX        13

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
SND_SOC_DAILINK_DEFS(da_combine_pb_normal,
	DAILINK_COMP_ARRAY(COMP_CPU("extern-cpu-dai")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME, DAI_LINK_CODEC_DAI_NAME_AUDIO)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("asp-pcm-hifi")));

SND_SOC_DAILINK_DEFS(da_combine_voice,
	DAILINK_COMP_ARRAY(COMP_CPU("extern-cpu-dai")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME, DAI_LINK_CODEC_DAI_NAME_AUDIO)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("snd-soc-dummy")));

SND_SOC_DAILINK_DEFS(da_combine_fm1,
	DAILINK_COMP_ARRAY(COMP_CPU("extern-cpu-dai")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME, DAI_LINK_CODEC_DAI_NAME_AUDIO)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("snd-soc-dummy")));

SND_SOC_DAILINK_DEFS(da_combine_pb_dsp,
	DAILINK_COMP_ARRAY(COMP_CPU("extern-cpu-dai")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME, DAI_LINK_CODEC_DAI_NAME_AUDIO)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("asp-pcm-hifi")));

SND_SOC_DAILINK_DEFS(da_combine_pb_direct,
	DAILINK_COMP_ARRAY(COMP_CPU("extern-cpu-dai")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME, DAI_LINK_CODEC_DAI_NAME_AUDIO)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("asp-pcm-hifi")));

SND_SOC_DAILINK_DEFS(da_combine_lowlatency,
	DAILINK_COMP_ARRAY(COMP_CPU("extern-cpu-dai")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME, DAI_LINK_CODEC_DAI_NAME_AUDIO)),
#ifdef AUDIO_LOW_LATENCY_LEGACY
	DAILINK_COMP_ARRAY(COMP_PLATFORM("pcm-codec")));
#else
	DAILINK_COMP_ARRAY(COMP_PLATFORM("asp-pcm-hifi")));
#endif

SND_SOC_DAILINK_DEFS(da_combine_mmap,
	DAILINK_COMP_ARRAY(COMP_CPU("extern-cpu-dai")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME, DAI_LINK_CODEC_DAI_NAME_AUDIO)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("asp-pcm-hifi")));

SND_SOC_DAILINK_DEFS(da_combine_v3_pb_normal,
	DAILINK_COMP_ARRAY(COMP_CPU("extern-cpu-dai")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME_V3, DAI_LINK_CODEC_DAI_NAME_AUDIO_V3)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("asp-pcm-hifi")));

SND_SOC_DAILINK_DEFS(da_combine_v3_voice,
	DAILINK_COMP_ARRAY(COMP_CPU("extern-cpu-dai")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME_V3, DAI_LINK_CODEC_DAI_NAME_VOICE_V3)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("snd-soc-dummy")));

SND_SOC_DAILINK_DEFS(da_combine_v3_fm1,
	DAILINK_COMP_ARRAY(COMP_CPU("extern-cpu-dai")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME_V3, DAI_LINK_CODEC_DAI_NAME_FM_V3)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("snd-soc-dummy")));

SND_SOC_DAILINK_DEFS(da_combine_v3_pb_dsp,
	DAILINK_COMP_ARRAY(COMP_CPU("extern-cpu-dai")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME_V3, DAI_LINK_CODEC_DAI_NAME_AUDIO_V3)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("asp-pcm-hifi")));

SND_SOC_DAILINK_DEFS(da_combine_v3_pb_direct,
	DAILINK_COMP_ARRAY(COMP_CPU("extern-cpu-dai")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME_V3, DAI_LINK_CODEC_DAI_NAME_AUDIO_V3)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("asp-pcm-hifi")));

SND_SOC_DAILINK_DEFS(da_combine_v3_lowlatency,
	DAILINK_COMP_ARRAY(COMP_CPU("extern-cpu-dai")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME_V3, DAI_LINK_CODEC_DAI_NAME_AUDIO_V3)),
#ifdef AUDIO_LOW_LATENCY_LEGACY
	DAILINK_COMP_ARRAY(COMP_PLATFORM("pcm-codec")));
#else
	DAILINK_COMP_ARRAY(COMP_PLATFORM("asp-pcm-hifi")));
#endif

SND_SOC_DAILINK_DEFS(da_combine_v3_mmap,
	DAILINK_COMP_ARRAY(COMP_CPU("extern-cpu-dai")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME_V3, DAI_LINK_CODEC_DAI_NAME_AUDIO_V3)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("asp-pcm-hifi")));

SND_SOC_DAILINK_DEFS(CODECLESS_OUTPUT,
#ifdef CONFIG_HIFI_VIR_DAI
	DAILINK_COMP_ARRAY(COMP_CPU("hifi-vir-dai")),
#else
	DAILINK_COMP_ARRAY(COMP_CPU("extern-cpu-dai")),
#endif
	DAILINK_COMP_ARRAY(COMP_CODEC("asp-codec-codecless", "asp-codec-codecless-dai")),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("asp-pcm-hifi")));

SND_SOC_DAILINK_DEFS(AK4376_PB_OUTPUT,
	DAILINK_COMP_ARRAY(COMP_CPU("hifi-vir-dai")),
	DAILINK_COMP_ARRAY(COMP_CODEC("ak4376-codec", "ak4376-AIF1")),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("asp-pcm-hifi")));

SND_SOC_DAILINK_DEFS(da_combine_without_hifi_pb_normal,
	DAILINK_COMP_ARRAY(COMP_CPU("extern-cpu-dai")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME, DAI_LINK_CODEC_DAI_NAME_AUDIO)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("pcm-codec")));

SND_SOC_DAILINK_DEFS(da_combine_without_hifi_bluetooth,
	DAILINK_COMP_ARRAY(COMP_CPU("extern-cpu-dai")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME, DAI_LINK_CODEC_DAI_NAME_BT)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("pcm-codec")));
#endif

static struct snd_soc_dai_link g_da_combine_dai_link[] = {
	{
		.name           = "da_combine_pb_normal",
		.stream_name    = "da_combine_pb_normal",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(da_combine_pb_normal),
#else
		.codec_name     = DAI_LINK_CODEC_NAME,
		.cpu_dai_name   = "extern-cpu-dai",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME_AUDIO,
		.platform_name  = "asp-pcm-hifi",
#endif
	},
	{
		.name           = "da_combine_voice",
		.stream_name    = "da_combine_voice",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(da_combine_voice),
#else
		.codec_name     = DAI_LINK_CODEC_NAME,
		.cpu_dai_name   = "extern-cpu-dai",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME_AUDIO,
		.platform_name  = "snd-soc-dummy",
#endif
	},
	{
		.name           = "da_combine_fm1",
		.stream_name    = "da_combine_fm1",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(da_combine_fm1),
#else
		.codec_name     = DAI_LINK_CODEC_NAME,
		.cpu_dai_name   = "extern-cpu-dai",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME_AUDIO,
		.platform_name  = "snd-soc-dummy",
#endif
	},
	{
		.name           = "da_combine_pb_dsp",
		.stream_name    = "da_combine_pb_dsp",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(da_combine_pb_dsp),
#else
		.codec_name     = DAI_LINK_CODEC_NAME,
		.cpu_dai_name   = "extern-cpu-dai",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME_AUDIO,
		.platform_name  = "asp-pcm-hifi",
#endif
	},
	{
		.name           = "da_combine_pb_direct",
		.stream_name    = "da_combine_pb_direct",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(da_combine_pb_direct),
#else
		.codec_name     = DAI_LINK_CODEC_NAME,
		.cpu_dai_name   = "extern-cpu-dai",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME_AUDIO,
		.platform_name  = "asp-pcm-hifi",
#endif
	},
	{
		.name           = "da_combine_lowlatency",
		.stream_name    = "da_combine_lowlatency",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(da_combine_lowlatency),
#else
		.codec_name     = DAI_LINK_CODEC_NAME,
		.cpu_dai_name   = "extern-cpu-dai",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME_AUDIO,
#ifdef AUDIO_LOW_LATENCY_LEGACY
		.platform_name  = "pcm-codec",
#else
		.platform_name  = "asp-pcm-hifi",
#endif
#endif
	},
	{
		.name           = "da_combine_mmap",
		.stream_name    = "da_combine_mmap",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(da_combine_mmap),
#else
		.codec_name     = DAI_LINK_CODEC_NAME,
		.cpu_dai_name   = "extern-cpu-dai",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME_AUDIO,
		.platform_name  = "asp-pcm-hifi",
#endif
	},
};

static struct snd_soc_dai_link g_da_combine_v3_dai_link[] = {
	{
		.name           = "da_combine_v3_pb_normal",
		.stream_name    = "da_combine_v3_pb_normal",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(da_combine_v3_pb_normal),
#else
		.codec_name     = DAI_LINK_CODEC_NAME_V3,
		.cpu_dai_name   = "extern-cpu-dai",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME_AUDIO_V3,
		.platform_name  = "asp-pcm-hifi",
#endif
	},
	{
		.name           = "da_combine_v3_voice",
		.stream_name    = "da_combine_v3_voice",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(da_combine_v3_voice),
#else
		.codec_name     = DAI_LINK_CODEC_NAME_V3,
		.cpu_dai_name   = "extern-cpu-dai",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME_VOICE_V3,
		.platform_name  = "snd-soc-dummy",
#endif
	},
	{
		.name           = "da_combine_v3_fm1",
		.stream_name    = "da_combine_v3_fm1",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(da_combine_v3_fm1),
#else
		.codec_name     = DAI_LINK_CODEC_NAME_V3,
		.cpu_dai_name   = "extern-cpu-dai",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME_FM_V3,
		.platform_name  = "snd-soc-dummy",
#endif
	},
	{
		.name           = "da_combine_v3_pb_dsp",
		.stream_name    = "da_combine_v3_pb_dsp",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(da_combine_v3_pb_dsp),
#else
		.codec_name     = DAI_LINK_CODEC_NAME_V3,
		.cpu_dai_name   = "extern-cpu-dai",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME_AUDIO_V3,
		.platform_name  = "asp-pcm-hifi",
#endif
	},
	{
		.name           = "da_combine_v3_pb_direct",
		.stream_name    = "da_combine_v3_pb_direct",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(da_combine_v3_pb_direct),
#else
		.codec_name     = DAI_LINK_CODEC_NAME_V3,
		.cpu_dai_name   = "extern-cpu-dai",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME_AUDIO_V3,
		.platform_name  = "asp-pcm-hifi",
#endif
	},
	{
		.name           = "da_combine_v3_lowlatency",
		.stream_name    = "da_combine_v3_lowlatency",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(da_combine_v3_lowlatency),
#else
		.codec_name     = DAI_LINK_CODEC_NAME_V3,
		.cpu_dai_name   = "extern-cpu-dai",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME_AUDIO_V3,
#ifdef AUDIO_LOW_LATENCY_LEGACY
		.platform_name  = "pcm-codec",
#else
		.platform_name  = "asp-pcm-hifi",
#endif
#endif
	},
	{
		.name           = "da_combine_v3_mmap",
		.stream_name    = "da_combine_v3_mmap",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(da_combine_v3_mmap),
#else
		.codec_name     = DAI_LINK_CODEC_NAME_V3,
		.cpu_dai_name   = "extern-cpu-dai",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME_AUDIO_V3,
		.platform_name  = "asp-pcm-hifi",
#endif
	},
};

/* RX for headset/headphone with audio mode */
static struct snd_soc_dai_link g_codecless_dai_link[] = {
	{
		 .name           = "CODECLESS_OUTPUT",
		 .stream_name    = "Playback",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(CODECLESS_OUTPUT),
#else
		 .codec_name     = "asp-codec-codecless",
#ifdef CONFIG_HIFI_VIR_DAI
		 .cpu_dai_name   = "hifi-vir-dai",
#else
		 .cpu_dai_name   = "extern-cpu-dai",
#endif
		 .platform_name  = "asp-pcm-hifi",
		 .codec_dai_name = "asp-codec-codecless-dai",
#endif
	},
};

static struct snd_soc_dai_link g_ak4376_dai_link[] = {
	{
		 .name           = "AK4376_PB_OUTPUT",
		 .stream_name    = "Audio Playback",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(AK4376_PB_OUTPUT),
#else
		 .codec_name     = "ak4376-codec",
		 .cpu_dai_name   = "hifi-vir-dai",
		 .platform_name  = "asp-pcm-hifi",
		 .codec_dai_name = "ak4376-AIF1",
#endif
	},
};

static struct snd_soc_dai_link g_da_combine_dai_link_without_hifi[] = {
	{
		.name           = "da_combine_pb_normal",
		.stream_name    = "da_combine_pb_normal",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(da_combine_without_hifi_pb_normal),
#else
		.codec_name     = DAI_LINK_CODEC_NAME,
		.cpu_dai_name   = "extern-cpu-dai",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME_AUDIO,
		.platform_name  = "pcm-codec",
#endif
		.nonatomic      = true,
	},
	{
		.name           = "da_combine_bluetooth",
		.stream_name    = "da_combine_bluetooth",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(da_combine_without_hifi_bluetooth),
#else
		.codec_name     = DAI_LINK_CODEC_NAME,
		.cpu_dai_name   = "extern-cpu-dai",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME_BT,
		.platform_name  = "pcm-codec",
#endif
		.nonatomic      = true,
	},
};

static struct snd_soc_dai_link g_da_combine_codecless_dai_links[
	ARRAY_SIZE(g_da_combine_dai_link) + ARRAY_SIZE(g_codecless_dai_link)];

static struct snd_soc_card g_da_combine_card = {
	.name      = DA_COMBINE_V5_MACHINE_CARD_NAME,
	.owner     = THIS_MODULE,
	.dai_link  = g_da_combine_dai_link,
	.num_links = ARRAY_SIZE(g_da_combine_dai_link),
};

struct snd_soc_card g_da_combine_codecless_card = {
	.name      = DA_COMBINE_V5_MACHINE_CODECLESS_NAME,
	.owner     = THIS_MODULE,
	.dai_link  = g_da_combine_codecless_dai_links,
	.num_links = ARRAY_SIZE(g_da_combine_codecless_dai_links),
};

static struct snd_soc_card g_da_combine_card_without_hifi = {
	.name      = DA_COMBINE_V5_MACHINE_CARD_NAME,
	.owner     = THIS_MODULE,
	.dai_link  = g_da_combine_dai_link_without_hifi,
	.num_links = ARRAY_SIZE(g_da_combine_dai_link_without_hifi),
};

static struct snd_soc_dai_link g_da_combine_v3_ak4376_dai_links[
	ARRAY_SIZE(g_da_combine_v3_dai_link) + ARRAY_SIZE(g_ak4376_dai_link)];

static struct snd_soc_card g_da_combine_v3_card = {
	.name      = DA_COMBINE_V3_MACHINE_CARD_NAME,
	.owner     = THIS_MODULE,
	.dai_link  = g_da_combine_v3_dai_link,
	.num_links = ARRAY_SIZE(g_da_combine_v3_dai_link),
};

struct snd_soc_card g_da_combine_v3_ak4376_card = {
	.name      = DA_COMBINE_V3_MACHINE_AK4376_CARD_NAME,
	.owner     = THIS_MODULE,
	.dai_link  = g_da_combine_v3_ak4376_dai_links,
	.num_links = ARRAY_SIZE(g_da_combine_v3_ak4376_dai_links),
};

static void populate_extern_snd_card_dailinks(struct platform_device *pdev)
{
	AUDIO_LOGI("probe");

	memcpy(g_da_combine_codecless_dai_links, g_da_combine_dai_link,
		sizeof(g_da_combine_dai_link));
	memcpy(g_da_combine_codecless_dai_links + ARRAY_SIZE(g_da_combine_dai_link),
		g_codecless_dai_link, sizeof(g_codecless_dai_link));
}

static void populate_extern_snd_card_dailinks_v3(struct platform_device *pdev)
{
	AUDIO_LOGI("probe");

	memcpy(g_da_combine_v3_ak4376_dai_links, g_da_combine_v3_dai_link,
		sizeof(g_da_combine_v3_dai_link));
	memcpy(g_da_combine_v3_ak4376_dai_links + ARRAY_SIZE(g_da_combine_v3_dai_link),
		g_ak4376_dai_link, sizeof(g_ak4376_dai_link));
}

static struct snd_soc_dai_driver g_extern_cpu_dai = {
	.name     = "extern-cpu-dai",
	.playback = {
		.channels_min = PLAYBACK_CHANNELS_MIN,
		.channels_max = PLAYBACK_CHANNELS_MAX,
		.rates        = SNDRV_PCM_RATE_8000_384000,
		.formats      = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S16_BE |
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S24_BE | SNDRV_PCM_FMTBIT_S24_3LE | SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture = {
		.channels_min = CAPTURE_CHANNELS_MIN,
		.channels_max = CAPTURE_CHANNELS_MAX,
		.rates        = SNDRV_PCM_RATE_8000_96000,
		.formats      = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
	},
};

static const struct snd_soc_component_driver g_extern_cpu_dai_component = {
	.name = "extern-cpu-dai",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	.idle_bias_on = true,
#endif
};

static int da_combine_probe(struct platform_device *pdev)
{
	int ret;
	struct snd_soc_card *card = &g_da_combine_card;
	const char *extern_codec_type = "extern_codec_type";
	const char *ptr = NULL;

	if (!pdev) {
		AUDIO_LOGE("pdev is null, fail");
		return -EINVAL;
	}

	AUDIO_LOGI("probe");

	ret = of_property_read_string(pdev->dev.of_node, extern_codec_type, &ptr);
	if (ret == 0) {
		AUDIO_LOGI("extern type: %s in dt node", extern_codec_type);
		if (!strncmp(ptr, EXTERN_HIFI_CODEC_CODECLESS_NAME,
			strlen(EXTERN_HIFI_CODEC_CODECLESS_NAME))) {
			populate_extern_snd_card_dailinks(pdev);
			card = &g_da_combine_codecless_card;
		} else if (!strncmp(ptr, EXTERN_HIFI_CODEC_WITHOUT_HIFI,
			strlen(EXTERN_HIFI_CODEC_WITHOUT_HIFI))) {
			card = &g_da_combine_card_without_hifi;
		} else {
			card = &g_da_combine_card;
		}
	} else {
		card = &g_da_combine_card;
	}

	card->dev = &pdev->dev;

	ret = devm_snd_soc_register_component(&pdev->dev, &g_extern_cpu_dai_component,
		&g_extern_cpu_dai, 1);

	/* card is static, so needn't free it */
	ret = snd_soc_register_card(card);
	if (ret != 0) {
		AUDIO_LOGE("sound card register failed %d", ret);
		return ret;
	}

	ret = snd_component_add(card->snd_card, "hi_codec");
	if (ret < 0) {
		AUDIO_LOGE("sound card add component failed %d", ret);
		snd_soc_unregister_card(card);
	}

	return ret;
}

static int da_combine_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	if (card)
		snd_soc_unregister_card(card);

	return 0;
}

static int da_combine_v3_probe(struct platform_device *pdev)
{
	int ret;
	struct snd_soc_card *card = &g_da_combine_v3_card;
	const char *extern_codec_type = "extern_codec_type";
	const char *ptr = NULL;

	if (!pdev) {
		AUDIO_LOGE("pdev is null, fail");
		return -EINVAL;
	}

	AUDIO_LOGI("probe");

	ret = of_property_read_string(pdev->dev.of_node, extern_codec_type, &ptr);
	if (ret == 0) {
		AUDIO_LOGI("extern type: %s in dt node", extern_codec_type);
		if (!strncmp(ptr, EXTERN_HIFI_CODEC_AK4376_NAME,
			strlen(EXTERN_HIFI_CODEC_AK4376_NAME))) {
			populate_extern_snd_card_dailinks_v3(pdev);
			card = &g_da_combine_v3_ak4376_card;
		} else {
			card = &g_da_combine_v3_card;
		}
	} else {
		card = &g_da_combine_v3_card;
	}
	card->dev = &pdev->dev;

	ret = devm_snd_soc_register_component(&pdev->dev, &g_extern_cpu_dai_component,
		&g_extern_cpu_dai, 1);

	ret = snd_soc_register_card(card);
	if (ret != 0)
		AUDIO_LOGE("sound card register failed %d", ret);

	return ret;
}

static int da_combine_v3_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	if (card)
		snd_soc_unregister_card(card);

	return 0;
}

static const struct of_device_id g_da_combine_of_match[] = {
	{ .compatible = "hisilicon,da_combine_v5-machine", },
	{ },
};
MODULE_DEVICE_TABLE(of, g_da_combine_of_match);

static const struct of_device_id g_da_combine_v3_of_match[] = {
	{ .compatible = "hisilicon,da_combine_v3-machine", },
	{ },
};
MODULE_DEVICE_TABLE(of, g_da_combine_v3_of_match);

static struct platform_driver g_da_combine_driver = {
	.driver = {
		.name           = DA_COMBINE_V5_MACHINE_DRIVER_NAME,
		.owner          = THIS_MODULE,
		.of_match_table = g_da_combine_of_match,
	},
	.probe  = da_combine_probe,
	.remove = da_combine_remove,
};

static struct platform_driver g_da_combine_v3_driver = {
	.driver = {
		.name           = DA_COMBINE_V3_MACHINE_DRIVER_NAME,
		.owner          = THIS_MODULE,
		.of_match_table = g_da_combine_v3_of_match,
	},
	.probe  = da_combine_v3_probe,
	.remove = da_combine_v3_remove,
};

static int __init da_combine_init(void)
{
	AUDIO_LOGI("init");

	return platform_driver_register(&g_da_combine_driver);
}
late_initcall(da_combine_init);

static int __init da_combine_v3_init(void)
{
	AUDIO_LOGI("init");

	return platform_driver_register(&g_da_combine_v3_driver);
}
late_initcall(da_combine_v3_init);

static void __exit da_combine_exit(void)
{
	platform_driver_unregister(&g_da_combine_driver);
}
module_exit(da_combine_exit);

static void __exit da_combine_v3_exit(void)
{
	platform_driver_unregister(&g_da_combine_v3_driver);
}
module_exit(da_combine_v3_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ALSA SoC for da combine machine driver");

