/*
 * analog_less_machine.c
 *
 * analog_less_machine driver
 *
 * Copyright (c) 2018-2020 Huawei Technologies Co., Ltd.
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
#include <linux/version.h>
#include <linux/of.h>
#include <sound/soc.h>

#include "audio_log.h"

#define LOG_TAG "analog_less_machine"

#define DAI_LINK_CODECLESS_NAME          "asp-codec-codecless"
#define DAI_LINK_CODECLESS_DAI_NAME      "asp-codec-codecless-dai"
#define DAI_LINK_CODEC_NAME      "asp-codec"
#define DAI_LINK_CODEC_HIFI_NAME      "asp-codec-hifi"
#define DAI_LINK_CODEC_DAI_NAME  "asp-codec-dai"
#define ANALOG_LESS_MACHINE_DRIVER_NAME "analog-less-machine"
#define ANALOG_LESS_MACHINE_CARD_NAME   "analog-less-machine-card"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
SND_SOC_DAILINK_DEFS(analog_less_machine_pb_normal,
	DAILINK_COMP_ARRAY(COMP_CPU("asp-pcm-mm")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODECLESS_NAME, DAI_LINK_CODECLESS_DAI_NAME)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("asp-pcm-hifi")));

SND_SOC_DAILINK_DEFS(analog_less_machine_voice,
	DAILINK_COMP_ARRAY(COMP_CPU("asp-pcm-modem")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODECLESS_NAME, DAI_LINK_CODECLESS_DAI_NAME)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("snd-soc-dummy")));

SND_SOC_DAILINK_DEFS(analog_less_machine_fm1,
	DAILINK_COMP_ARRAY(COMP_CPU("asp-pcm-fm")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODECLESS_NAME, DAI_LINK_CODECLESS_DAI_NAME)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("snd-soc-dummy")));

SND_SOC_DAILINK_DEFS(analog_less_machine_pb_dsp,
	DAILINK_COMP_ARRAY(COMP_CPU("asp-pcm-lpp")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODECLESS_NAME, DAI_LINK_CODECLESS_DAI_NAME)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("asp-pcm-hifi")));

SND_SOC_DAILINK_DEFS(analog_less_machine_pb_direct,
	DAILINK_COMP_ARRAY(COMP_CPU("asp-pcm-direct")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODECLESS_NAME, DAI_LINK_CODECLESS_DAI_NAME)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("asp-pcm-hifi")));

SND_SOC_DAILINK_DEFS(analog_less_machine_lowlatency,
	DAILINK_COMP_ARRAY(COMP_CPU("asp-pcm-fast")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODECLESS_NAME, DAI_LINK_CODECLESS_DAI_NAME)),
#ifdef AUDIO_LOW_LATENCY_LEGACY
	DAILINK_COMP_ARRAY(COMP_PLATFORM("pcm-codec")));
#else
	DAILINK_COMP_ARRAY(COMP_PLATFORM("asp-pcm-hifi")));
#endif

SND_SOC_DAILINK_DEFS(analog_less_machine_mmap,
	DAILINK_COMP_ARRAY(COMP_CPU("audio-pcm-mmap")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODECLESS_NAME, DAI_LINK_CODECLESS_DAI_NAME)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("asp-pcm-hifi")));

SND_SOC_DAILINK_DEFS(analog_less_hifi_machine_pb_normal,
	DAILINK_COMP_ARRAY(COMP_CPU("asp-pcm-mm")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME, DAI_LINK_CODEC_DAI_NAME)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("asp-pcm-hifi")));

SND_SOC_DAILINK_DEFS(analog_codec_hifi_machine_bt,
	DAILINK_COMP_ARRAY(COMP_CPU("asp-pcm-bt")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME, DAI_LINK_CODEC_DAI_NAME)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("asp-pcm-hifi")));

SND_SOC_DAILINK_DEFS(analog_codec_hifi_machine_fm,
	DAILINK_COMP_ARRAY(COMP_CPU("asp-pcm-fm")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME, DAI_LINK_CODEC_DAI_NAME)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("asp-pcm-hifi")));

SND_SOC_DAILINK_DEFS(analog_less_hifi_machine_offload,
	DAILINK_COMP_ARRAY(COMP_CPU("asp-pcm-offload")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME, DAI_LINK_CODEC_DAI_NAME)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("asp-pcm-hifi")));

SND_SOC_DAILINK_DEFS(analog_less_hifi_machine_direct,
	DAILINK_COMP_ARRAY(COMP_CPU("asp-pcm-direct")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME, DAI_LINK_CODEC_DAI_NAME)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("asp-pcm-hifi")));

SND_SOC_DAILINK_DEFS(analog_less_hifi_machine_lowlatency,
	DAILINK_COMP_ARRAY(COMP_CPU("asp-pcm-fast")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME, DAI_LINK_CODEC_DAI_NAME)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("asp-pcm-hifi")));

SND_SOC_DAILINK_DEFS(analog_less_hifi_machine_mmap,
	DAILINK_COMP_ARRAY(COMP_CPU("audio-pcm-mmap")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME, DAI_LINK_CODEC_DAI_NAME)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("asp-pcm-hifi")));

SND_SOC_DAILINK_DEFS(analog_codec_machine_pb_normal,
	DAILINK_COMP_ARRAY(COMP_CPU("asp-pcm-mm")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME, DAI_LINK_CODEC_DAI_NAME)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("pcm-codec")));

SND_SOC_DAILINK_DEFS(analog_codec_machine_bt,
	DAILINK_COMP_ARRAY(COMP_CPU("asp-pcm-bt")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME, DAI_LINK_CODEC_DAI_NAME)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("pcm-codec")));

SND_SOC_DAILINK_DEFS(analog_codec_machine_fm,
	DAILINK_COMP_ARRAY(COMP_CPU("asp-pcm-fm")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DAI_LINK_CODEC_NAME, DAI_LINK_CODEC_DAI_NAME)),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("pcm-codec")));
#endif

static struct snd_soc_dai_link g_analog_less_dai_link[] = {
	{
		.name           = "analog_less_machine_pb_normal",
		.stream_name    = "analog_less_machine_pb_normal",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(analog_less_machine_pb_normal),
#else
		.codec_name     = DAI_LINK_CODECLESS_NAME,
		.cpu_dai_name   = "asp-pcm-mm",
		.codec_dai_name = DAI_LINK_CODECLESS_DAI_NAME,
		.platform_name  = "asp-pcm-hifi",
#endif
	},
	{
		.name           = "analog_less_machine_voice",
		.stream_name    = "analog_less_machine_voice",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(analog_less_machine_voice),
#else
		.codec_name     = DAI_LINK_CODECLESS_NAME,
		.cpu_dai_name   = "asp-pcm-modem",
		.codec_dai_name = DAI_LINK_CODECLESS_DAI_NAME,
		.platform_name  = "snd-soc-dummy",
#endif
	},
	{
		.name           = "analog_less_machine_fm1",
		.stream_name    = "analog_less_machine_fm1",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(analog_less_machine_fm1),
#else
		.codec_name     = DAI_LINK_CODECLESS_NAME,
		.cpu_dai_name   = "asp-pcm-fm",
		.codec_dai_name = DAI_LINK_CODECLESS_DAI_NAME,
		.platform_name  = "snd-soc-dummy",
#endif
	},
	{
		.name           = "analog_less_machine_pb_dsp",
		.stream_name    = "analog_less_machine_pb_dsp",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(analog_less_machine_pb_dsp),
#else
		.codec_name     = DAI_LINK_CODECLESS_NAME,
		.cpu_dai_name   = "asp-pcm-lpp",
		.codec_dai_name = DAI_LINK_CODECLESS_DAI_NAME,
		.platform_name  = "asp-pcm-hifi",
#endif
	},
	{
		.name           = "analog_less_machine_pb_direct",
		.stream_name    = "analog_less_machine_pb_direct",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(analog_less_machine_pb_direct),
#else
		.codec_name     = DAI_LINK_CODECLESS_NAME,
		.cpu_dai_name   = "asp-pcm-direct",
		.codec_dai_name = DAI_LINK_CODECLESS_DAI_NAME,
		.platform_name  = "asp-pcm-hifi",
#endif
	},
	{
		.name           = "analog_less_machine_lowlatency",
		.stream_name    = "analog_less_machine_lowlatency",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(analog_less_machine_lowlatency),
#else
		.codec_name     = DAI_LINK_CODECLESS_NAME,
		.cpu_dai_name   = "asp-pcm-fast",
		.codec_dai_name = DAI_LINK_CODECLESS_DAI_NAME,
#ifdef AUDIO_LOW_LATENCY_LEGACY
		.platform_name  = "pcm-codec",
#else
		.platform_name  = "asp-pcm-hifi",
#endif
#endif
	},
	{
		.name           = "analog_less_machine_mmap",
		.stream_name    = "analog_less_machine_mmap",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(analog_less_machine_mmap),
#else
		.codec_name     = DAI_LINK_CODECLESS_NAME,
		.cpu_dai_name   = "audio-pcm-mmap",
		.codec_dai_name = DAI_LINK_CODECLESS_DAI_NAME,
		.platform_name  = "asp-pcm-hifi",
#endif
	},
};

static struct snd_soc_dai_link g_hifi_codec_dai_link[] = {
	{
		.name           = "analog_less_machine_pb_normal",
		.stream_name    = "analog_less_machine_pb_normal",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(analog_less_hifi_machine_pb_normal),
#else
		.codec_name     = DAI_LINK_CODEC_NAME,
		.cpu_dai_name   = "asp-pcm-mm",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME,
		.platform_name  = "asp-pcm-hifi",
#endif
	},
	{
		.name           = "analog_codec_machine_bt",
		.stream_name    = "analog_codec_machine_bt",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(analog_codec_hifi_machine_bt),
#else
		.codec_name     = DAI_LINK_CODEC_NAME,
		.cpu_dai_name   = "asp-pcm-bt",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME,
		.platform_name  = "asp-pcm-hifi",
#endif
	},
	{
		.name           = "analog_codec_machine_fm",
		.stream_name    = "analog_codec_machine_fm",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(analog_codec_hifi_machine_fm),
#else
		.codec_name     = DAI_LINK_CODEC_NAME,
		.cpu_dai_name   = "asp-pcm-fm",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME,
		.platform_name  = "asp-pcm-hifi",
#endif
	},
	{
		.name           = "analog_less_machine_offload",
		.stream_name    = "analog_less_machine_offload",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(analog_less_hifi_machine_offload),
#else
		.codec_name     = DAI_LINK_CODEC_NAME,
		.cpu_dai_name   = "asp-pcm-offload",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME,
		.platform_name  = "asp-pcm-hifi",
#endif
	},
	{
		.name           = "analog_less_machine_direct",
		.stream_name    = "analog_less_machine_direct",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(analog_less_hifi_machine_direct),
#else
		.codec_name     = DAI_LINK_CODEC_NAME,
		.cpu_dai_name   = "asp-pcm-direct",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME,
		.platform_name  = "asp-pcm-hifi",
#endif
	},
	{
		.name           = "analog_less_machine_lowlatency",
		.stream_name    = "analog_less_machine_lowlatency",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(analog_less_hifi_machine_lowlatency),
#else
		.codec_name     = DAI_LINK_CODEC_NAME,
		.cpu_dai_name   = "asp-pcm-fast",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME,
		.platform_name  = "asp-pcm-hifi",
#endif
	},
	{
		.name           = "analog_less_machine_mmap",
		.stream_name    = "analog_less_machine_mmap",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(analog_less_hifi_machine_mmap),
#else
		.codec_name     = DAI_LINK_CODEC_NAME,
		.cpu_dai_name   = "audio-pcm-mmap",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME,
		.platform_name  = "asp-pcm-hifi",
#endif
	},
};

static struct snd_soc_dai_link g_analog_codec_dai_link[] = {
	{
		.name           = "analog_codec_machine_pb_normal",
		.stream_name    = "analog_codec_machine_pb_normal",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(analog_codec_machine_pb_normal),
#else
		.codec_name     = DAI_LINK_CODEC_NAME,
		.cpu_dai_name   = "asp-pcm-mm",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME,
		.platform_name  = "pcm-codec",
#endif
		.nonatomic      = true,
	},
	{
		.name           = "analog_codec_machine_bt",
		.stream_name    = "analog_codec_machine_bt",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(analog_codec_machine_bt),
#else
		.codec_name     = DAI_LINK_CODEC_NAME,
		.cpu_dai_name   = "asp-pcm-bt",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME,
		.platform_name  = "pcm-codec",
#endif
		.nonatomic      = true,
	},
	{
		.name           = "analog_codec_machine_fm",
		.stream_name    = "analog_codec_machine_fm",
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		SND_SOC_DAILINK_REG(analog_codec_machine_fm),
#else
		.codec_name     = DAI_LINK_CODEC_NAME,
		.cpu_dai_name   = "asp-pcm-fm",
		.codec_dai_name = DAI_LINK_CODEC_DAI_NAME,
		.platform_name  = "pcm-codec",
#endif
		.nonatomic      = true,
	},
};

static struct snd_soc_card g_analog_less_card = {
	.name      = ANALOG_LESS_MACHINE_CARD_NAME,
	.owner     = THIS_MODULE,
	.dai_link  = g_analog_less_dai_link,
	.num_links = ARRAY_SIZE(g_analog_less_dai_link),
};

static struct snd_soc_card g_hifi_codec_card = {
	.name      = ANALOG_LESS_MACHINE_CARD_NAME,
	.owner     = THIS_MODULE,
	.dai_link  = g_hifi_codec_dai_link,
	.num_links = ARRAY_SIZE(g_hifi_codec_dai_link),
};

static struct snd_soc_card g_analog_codec_card = {
	.name      = ANALOG_LESS_MACHINE_CARD_NAME,
	.owner     = THIS_MODULE,
	.dai_link  = g_analog_codec_dai_link,
	.num_links = ARRAY_SIZE(g_analog_codec_dai_link),
};

static int analog_less_probe(struct platform_device *pdev)
{
	int ret;
	struct snd_soc_card *card = &g_analog_less_card;
	const char *ptr = NULL;

	if (!pdev) {
		AUDIO_LOGE("pdev is null, fail");
		return -EINVAL;
	}

	AUDIO_LOGI("analog less probe");

	ret = of_property_read_string(pdev->dev.of_node, "dai-link-codec-name", &ptr);
	if (ret == 0) {
		AUDIO_LOGI("dai_link_codec_name: %s in dt node", ptr);
		if (strcmp(ptr, DAI_LINK_CODEC_NAME) == 0) {
			card = &g_analog_codec_card;
		} else if (strcmp(ptr, DAI_LINK_CODEC_HIFI_NAME) == 0) {
			card = &g_hifi_codec_card;
		}
	}

	card->dev = &pdev->dev;

	ret = snd_soc_register_card(card);
	if (ret != 0)
		AUDIO_LOGE("sound card register failed %d", ret);

	return ret;
}

static int analog_less_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	if (card)
		snd_soc_unregister_card(card);

	return 0;
}

static const struct of_device_id g_analog_less_of_match[] = {
	{ .compatible = "hisilicon,analog-less-machine", },
	{ },
};
MODULE_DEVICE_TABLE(of, g_analog_less_of_match);

static struct platform_driver g_analog_less_driver = {
	.driver  = {
		.name           = ANALOG_LESS_MACHINE_DRIVER_NAME,
		.owner          = THIS_MODULE,
		.of_match_table = g_analog_less_of_match,
	},
	.probe  = analog_less_probe,
	.remove = analog_less_remove,
};

static int __init analog_less_init(void)
{
	AUDIO_LOGI("analog less init");

	return platform_driver_register(&g_analog_less_driver);
}
late_initcall(analog_less_init);

static void __exit analog_less_exit(void)
{
	platform_driver_unregister(&g_analog_less_driver);
}
module_exit(analog_less_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ALSA SoC for analog less machine driver");
