/*
 * phone_pcm_config.c
 *
 * hifi phone custom driver
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <sound/soc.h>
#include <sound/pcm.h>

#include "audio_log.h"
#include "audio_pcm_hifi.h"

#ifndef LOG_TAG
#define LOG_TAG "audio_pcm"
#endif

#define AUIDO_PCM_PB_FORMATS  (SNDRV_PCM_FMTBIT_S8 | \
		SNDRV_PCM_FMTBIT_U8 | \
		SNDRV_PCM_FMTBIT_S16_LE | \
		SNDRV_PCM_FMTBIT_S16_BE | \
		SNDRV_PCM_FMTBIT_U16_LE | \
		SNDRV_PCM_FMTBIT_U16_BE | \
		SNDRV_PCM_FMTBIT_S24_LE | \
		SNDRV_PCM_FMTBIT_S24_BE | \
		SNDRV_PCM_FMTBIT_U24_LE | \
		SNDRV_PCM_FMTBIT_U24_BE)

#define AUIDO_PCM_MMAP_FORMATS  (SNDRV_PCM_FMTBIT_S16_LE | \
		SNDRV_PCM_FMTBIT_S24_3LE | \
		SNDRV_PCM_FMTBIT_S32_LE)

/*
 * PLAYBACK SUPPORT RATES
 * 8/11.025/16/22.05/32/44.1/48/88.2/96kHz
 */
#ifdef CONFIG_USB_384K_AUDIO_ADAPTER_SUPPORT
#define AUIDO_PCM_PB_RATES    (SNDRV_PCM_RATE_8000_48000 | \
		SNDRV_PCM_RATE_44100 | \
		SNDRV_PCM_RATE_88200 | \
		SNDRV_PCM_RATE_96000 | \
		SNDRV_PCM_RATE_176400 | \
		SNDRV_PCM_RATE_192000 | \
		SNDRV_PCM_RATE_384000)
#else
#define AUIDO_PCM_PB_RATES    (SNDRV_PCM_RATE_8000_48000 | \
		SNDRV_PCM_RATE_44100 | \
		SNDRV_PCM_RATE_88200 | \
		SNDRV_PCM_RATE_96000 | \
		SNDRV_PCM_RATE_176400 | \
		SNDRV_PCM_RATE_192000)
#endif

#define AUIDO_PCM_PB_MIN_CHANNELS  1
#define AUIDO_PCM_PB_MAX_CHANNELS  4
/* Assume the FIFO size */
#define AUIDO_PCM_PB_FIFO_SIZE     16

/* CAPTURE SUPPORT FORMATS : SIGNED 16/24bit */
#define AUIDO_PCM_CP_FORMATS  (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE)

/* CAPTURE SUPPORT RATES : 48/96kHz */
#define AUIDO_PCM_CP_RATES    (SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000)

#define AUIDO_PCM_CP_MIN_CHANNELS  1
#define AUIDO_PCM_CP_MAX_CHANNELS  13
/* Assume the FIFO size */
#define AUIDO_PCM_CP_FIFO_SIZE     32
#define AUIDO_PCM_MODEM_RATES      (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 | \
	SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_48000)
#define AUIDO_PCM_FM_RATES         (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000)

#define AUIDO_PCM_MAX_BUFFER_SIZE  (192 * 1024)    /* 0x30000 */
#define AUIDO_PCM_MIN_BUFFER_SIZE  32
#define AUIDO_PCM_MAX_PERIODS      32
#define AUIDO_PCM_MIN_PERIODS      2

#undef NULL
#define NULL ((void *)0)

#ifdef CONFIG_HIFI_MEMORY_15M
/* sampling rate * time * bit width * channel num * buf num */
#define PCM_DMA_BUF_PLAYBACK_PRIMARY_LEN (48 * 20 * 4 * 2 * 2)
#define PCM_DMA_BUF_PLAYBACK_DIRECT_LEN (48 * 20 * 4 * 2 * 4)
#define PCM_DMA_BUF_CAPTURE_PRIMARY_LEN (48 * 20 * 2 * 4 * 8)
#define PCM_DMA_BUF_CAPTURE_DIRECT_LEN ((60 + 19) * 1024)
#else
#define PCM_DMA_BUF_PLAYBACK_PRIMARY_LEN (48 * 20 * 2 * 4 * 4)
#define PCM_DMA_BUF_PLAYBACK_DIRECT_LEN (384 * 20 * 4 * 2 * 4)
#define PCM_DMA_BUF_CAPTURE_PRIMARY_LEN (48 * 20 * 2 * 13 * 8)
#endif

#define PCM_DMA_BUF_0_PLAYBACK_LEN  (PCM_DMA_BUF_PLAYBACK_PRIMARY_LEN)
#define PCM_DMA_BUF_0_CAPTURE_BASE  (PCM_DMA_BUF_0_PLAYBACK_BASE + PCM_DMA_BUF_0_PLAYBACK_LEN)
#define PCM_DMA_BUF_0_CAPTURE_LEN   (PCM_DMA_BUF_CAPTURE_PRIMARY_LEN)

#define PCM_DMA_BUF_1_PLAYBACK_BASE (PCM_DMA_BUF_0_CAPTURE_BASE + PCM_DMA_BUF_0_CAPTURE_LEN)
#define PCM_DMA_BUF_1_PLAYBACK_LEN  (PCM_DMA_BUF_PLAYBACK_DIRECT_LEN)
#define PCM_DMA_BUF_1_CAPTURE_BASE  (PCM_DMA_BUF_1_PLAYBACK_BASE + PCM_DMA_BUF_1_PLAYBACK_LEN)
#ifdef CONFIG_HIFI_MEMORY_15M
#define PCM_DMA_BUF_1_CAPTURE_LEN   (PCM_DMA_BUF_CAPTURE_DIRECT_LEN)
#else
#define PCM_DMA_BUF_1_CAPTURE_LEN   (193 * 1024)
#endif

#define PCM_DMA_BUF_2_PLAYBACK_BASE (PCM_DMA_BUF_1_CAPTURE_BASE + PCM_DMA_BUF_1_CAPTURE_LEN)
#define PCM_DMA_BUF_2_PLAYBACK_LEN  (PCM_DMA_BUF_PLAYBACK_PRIMARY_LEN)
#define PCM_DMA_BUF_2_CAPTURE_BASE  (PCM_DMA_BUF_2_PLAYBACK_BASE + PCM_DMA_BUF_2_PLAYBACK_LEN)
#define PCM_DMA_BUF_2_CAPTURE_LEN   (PCM_DMA_BUF_PLAYBACK_PRIMARY_LEN)

/* mmap device */
#define PCM_DMA_BUF_3_PLAYBACK_BASE (PCM_DMA_BUF_2_CAPTURE_BASE + PCM_DMA_BUF_2_CAPTURE_LEN)
#define PCM_DMA_BUF_3_PLAYBACK_LEN  (PCM_DMA_BUF_MMAP_MAX_SIZE)
#define PCM_DMA_BUF_3_CAPTURE_BASE  (PCM_DMA_BUF_3_PLAYBACK_BASE + PCM_DMA_BUF_3_PLAYBACK_LEN)
#define PCM_DMA_BUF_3_CAPTURE_LEN   (PCM_DMA_BUF_MMAP_MAX_SIZE)

#define PCM_DMA_BUF_END_ADDR        (PCM_DMA_BUF_3_CAPTURE_BASE + PCM_DMA_BUF_3_CAPTURE_LEN)

enum pcm_device {
	PCM_DEVICE_NORMAL = 0,
	PCM_DEVICE_MODEM,
	PCM_DEVICE_FM,
	PCM_DEVICE_OFFLOAD,
	PCM_DEVICE_DIRECT,
	PCM_DEVICE_LOW_LATENCY,
	PCM_DEVICE_MMAP,
	PCM_DEVICE_TOTAL
};

static const struct pcm_dma_buf_config g_pcm_dma_buf_config[PCM_DEVICE_TOTAL][PCM_STREAM_MAX] = {
	{ /* normal */
		{PCM_DMA_BUF_0_PLAYBACK_BASE, PCM_DMA_BUF_0_PLAYBACK_LEN},
		{PCM_DMA_BUF_0_CAPTURE_BASE, PCM_DMA_BUF_0_CAPTURE_LEN}
	},
	{ {0, 0}, {0, 0} }, /* modem */
	{ {0, 0}, {0, 0} }, /* fm */
	{ {0, 0}, {0, 0} }, /* offload */
	{ /* direct */
		{PCM_DMA_BUF_1_PLAYBACK_BASE, PCM_DMA_BUF_1_PLAYBACK_LEN},
		{PCM_DMA_BUF_1_CAPTURE_BASE, PCM_DMA_BUF_1_CAPTURE_LEN}
	},
	{ /* lowlatency */
		{PCM_DMA_BUF_2_PLAYBACK_BASE, PCM_DMA_BUF_2_PLAYBACK_LEN},
		{PCM_DMA_BUF_2_CAPTURE_BASE, PCM_DMA_BUF_2_CAPTURE_LEN}
	},
	{ /* mmap */
		{PCM_DMA_BUF_3_PLAYBACK_BASE, PCM_DMA_BUF_3_PLAYBACK_LEN},
		{PCM_DMA_BUF_3_CAPTURE_BASE, PCM_DMA_BUF_3_CAPTURE_LEN}
	},
};

static struct snd_soc_dai_driver g_audio_pcm_dai[] = {
	{
		.name = "asp-pcm-mm",
		.playback = {
			.stream_name  = "asp-pcm-mm Playback",
			.channels_min = AUIDO_PCM_PB_MIN_CHANNELS,
			.channels_max = AUIDO_PCM_PB_MAX_CHANNELS,
			.rates        = AUIDO_PCM_PB_RATES,
			.formats      = AUIDO_PCM_PB_FORMATS
		},
		.capture = {
			.stream_name  = "asp-pcm-mm Capture",
			.channels_min = AUIDO_PCM_CP_MIN_CHANNELS,
			.channels_max = AUIDO_PCM_CP_MAX_CHANNELS,
			.rates        = AUIDO_PCM_CP_RATES,
			.formats      = AUIDO_PCM_CP_FORMATS
		},
	},
	{
		.name = "asp-pcm-modem",
		.playback = {
			.stream_name  = "asp-pcm-modem Playback",
			.channels_min = AUIDO_PCM_PB_MIN_CHANNELS,
			.channels_max = AUIDO_PCM_PB_MAX_CHANNELS,
			.rates        = AUIDO_PCM_MODEM_RATES,
			.formats      = AUIDO_PCM_PB_FORMATS
		},
		.capture = {
			.stream_name  = "asp-pcm-modem Capture",
			.channels_min = AUIDO_PCM_CP_MIN_CHANNELS,
			.channels_max = AUIDO_PCM_CP_MAX_CHANNELS,
			.rates        = AUIDO_PCM_MODEM_RATES,
			.formats      = AUIDO_PCM_CP_FORMATS
		},
	},
	{
		.name = "asp-pcm-fm",
		.playback = {
			.stream_name  = "asp-pcm-fm Playback",
			.channels_min = AUIDO_PCM_PB_MIN_CHANNELS,
			.channels_max = AUIDO_PCM_PB_MAX_CHANNELS,
			.rates        = AUIDO_PCM_FM_RATES,
			.formats      = AUIDO_PCM_PB_FORMATS
		},
	},
	{
		.name = "asp-pcm-lpp",
		.playback = {
			.stream_name  = "asp-pcm-lpp Playback",
			.channels_min = AUIDO_PCM_PB_MIN_CHANNELS,
			.channels_max = AUIDO_PCM_PB_MAX_CHANNELS,
			.rates        = AUIDO_PCM_PB_RATES,
			.formats      = AUIDO_PCM_PB_FORMATS
		},
	},
	{
		.name = "asp-pcm-direct",
		.playback = {
			.stream_name  = "asp-pcm-direct Playback",
			.channels_min = AUIDO_PCM_PB_MIN_CHANNELS,
			.channels_max = AUIDO_PCM_PB_MAX_CHANNELS,
			.rates        = AUIDO_PCM_PB_RATES,
			.formats      = AUIDO_PCM_PB_FORMATS
		},
	},
	{
		.name = "asp-pcm-fast",
		.playback = {
			.stream_name  = "asp-pcm-fast Playback",
			.channels_min = AUIDO_PCM_PB_MIN_CHANNELS,
			.channels_max = AUIDO_PCM_PB_MAX_CHANNELS,
			.rates        = AUIDO_PCM_PB_RATES,
			.formats      = AUIDO_PCM_PB_FORMATS
		},
		.capture = {
			.stream_name  = "asp-pcm-fast Capture",
			.channels_min = AUIDO_PCM_CP_MIN_CHANNELS,
			.channels_max = AUIDO_PCM_CP_MAX_CHANNELS,
			.rates        = AUIDO_PCM_CP_RATES,
			.formats      = AUIDO_PCM_CP_FORMATS
		},
	},
	{
		.name = "audio-pcm-mmap",
		.playback = {
			.stream_name  = "audio-pcm-mmap Playback",
			.channels_min = AUIDO_PCM_PB_MIN_CHANNELS,
			.channels_max = AUIDO_PCM_PB_MAX_CHANNELS,
			.rates        = AUIDO_PCM_PB_RATES,
			.formats      = AUIDO_PCM_MMAP_FORMATS
		},
		.capture = {
			.stream_name  = "audio-pcm-mmap Capture",
			.channels_min = AUIDO_PCM_CP_MIN_CHANNELS,
			.channels_max = AUIDO_PCM_CP_MAX_CHANNELS,
			.rates        = AUIDO_PCM_CP_RATES,
			.formats      = AUIDO_PCM_CP_FORMATS
		},
	},
};

/* define the capability of playback channel */
static const struct snd_pcm_hardware g_audio_pcm_hardware_playback = {
	.info             = SNDRV_PCM_INFO_INTERLEAVED
		| SNDRV_PCM_INFO_NONINTERLEAVED
		| SNDRV_PCM_INFO_MMAP
		| SNDRV_PCM_INFO_MMAP_VALID
		| SNDRV_PCM_INFO_PAUSE,
	.formats          = SNDRV_PCM_FMTBIT_S24_LE,
	.channels_min     = AUIDO_PCM_PB_MIN_CHANNELS,
	.channels_max     = AUIDO_PCM_PB_MAX_CHANNELS,
	.buffer_bytes_max = PCM_DMA_BUF_PLAYBACK_PRIMARY_LEN,
	.period_bytes_min = AUIDO_PCM_MIN_BUFFER_SIZE,
	.period_bytes_max = PCM_DMA_BUF_PLAYBACK_PRIMARY_LEN,
	.periods_min      = AUIDO_PCM_MIN_PERIODS,
	.periods_max      = AUIDO_PCM_MAX_PERIODS,
	.fifo_size        = AUIDO_PCM_PB_FIFO_SIZE,
};

/* define the capability of capture channel */
static const struct snd_pcm_hardware g_audio_pcm_hardware_capture = {
	.info             = SNDRV_PCM_INFO_INTERLEAVED,
	.formats          = SNDRV_PCM_FMTBIT_S16_LE,
	.rates            = SNDRV_PCM_RATE_48000,
	.channels_min     = AUIDO_PCM_CP_MIN_CHANNELS,
	.channels_max     = AUIDO_PCM_CP_MAX_CHANNELS,
	.buffer_bytes_max = AUIDO_PCM_MAX_BUFFER_SIZE,
	.period_bytes_min = AUIDO_PCM_MIN_BUFFER_SIZE,
	.period_bytes_max = AUIDO_PCM_MAX_BUFFER_SIZE,
	.periods_min      = AUIDO_PCM_MIN_PERIODS,
	.periods_max      = AUIDO_PCM_MAX_PERIODS,
	.fifo_size        = AUIDO_PCM_CP_FIFO_SIZE,
};

/* define the capability of playback channel for direct */
static const struct snd_pcm_hardware g_audio_pcm_hardware_direct_playback = {
	.info             = SNDRV_PCM_INFO_INTERLEAVED
		| SNDRV_PCM_INFO_NONINTERLEAVED
		| SNDRV_PCM_INFO_MMAP
		| SNDRV_PCM_INFO_MMAP_VALID
		| SNDRV_PCM_INFO_PAUSE,
	.formats          = SNDRV_PCM_FMTBIT_S24_LE,
	.channels_min     = AUIDO_PCM_PB_MIN_CHANNELS,
	.channels_max     = AUIDO_PCM_PB_MAX_CHANNELS,
	.buffer_bytes_max = PCM_DMA_BUF_PLAYBACK_DIRECT_LEN,
	.period_bytes_min = AUIDO_PCM_MIN_BUFFER_SIZE,
	.period_bytes_max = PCM_DMA_BUF_PLAYBACK_DIRECT_LEN,
	.periods_min      = AUIDO_PCM_MIN_PERIODS,
	.periods_max      = AUIDO_PCM_MAX_PERIODS,
	.fifo_size        = AUIDO_PCM_PB_FIFO_SIZE,
};

/* define the capability of playback channel for lowlatency */
static const struct snd_pcm_hardware g_audio_pcm_hardware_lowlatency_playback = {
	.info             = SNDRV_PCM_INFO_INTERLEAVED
		| SNDRV_PCM_INFO_NONINTERLEAVED
		| SNDRV_PCM_INFO_MMAP
		| SNDRV_PCM_INFO_MMAP_VALID
		| SNDRV_PCM_INFO_PAUSE,
	.formats          = SNDRV_PCM_FMTBIT_S24_LE,
	.channels_min     = AUIDO_PCM_PB_MIN_CHANNELS,
	.channels_max     = AUIDO_PCM_PB_MAX_CHANNELS,
	.buffer_bytes_max = PCM_DMA_BUF_PLAYBACK_PRIMARY_LEN,
	.period_bytes_min = AUIDO_PCM_MIN_BUFFER_SIZE,
	.period_bytes_max = PCM_DMA_BUF_PLAYBACK_PRIMARY_LEN,
	.periods_min      = AUIDO_PCM_MIN_PERIODS,
	.periods_max      = AUIDO_PCM_MAX_PERIODS,
	.fifo_size        = AUIDO_PCM_PB_FIFO_SIZE,
};

/* define the capability of capture channel for lowlatency */
static const struct snd_pcm_hardware g_audio_pcm_hardware_lowlatency_capture = {
	.info             = SNDRV_PCM_INFO_INTERLEAVED
		| SNDRV_PCM_INFO_NONINTERLEAVED
		| SNDRV_PCM_INFO_MMAP
		| SNDRV_PCM_INFO_MMAP_VALID
		| SNDRV_PCM_INFO_PAUSE,
	.formats          = SNDRV_PCM_FMTBIT_S16_LE,
	.rates            = SNDRV_PCM_RATE_48000,
	.channels_min     = AUIDO_PCM_CP_MIN_CHANNELS,
	.channels_max     = AUIDO_PCM_CP_MAX_CHANNELS,
	.buffer_bytes_max = AUIDO_PCM_MAX_BUFFER_SIZE,
	.period_bytes_min = AUIDO_PCM_MIN_BUFFER_SIZE,
	.period_bytes_max = AUIDO_PCM_MAX_BUFFER_SIZE,
	.periods_min      = AUIDO_PCM_MIN_PERIODS,
	.periods_max      = AUIDO_PCM_MAX_PERIODS,
	.fifo_size        = AUIDO_PCM_CP_FIFO_SIZE,
};

/* define the capability of playback channel for mmap device */
static const struct snd_pcm_hardware g_audio_pcm_hardware_mmap_playback = {
	.info             = SNDRV_PCM_INFO_INTERLEAVED
		| SNDRV_PCM_INFO_NONINTERLEAVED
		| SNDRV_PCM_INFO_MMAP
		| SNDRV_PCM_INFO_MMAP_VALID
		| SNDRV_PCM_INFO_PAUSE,
	.formats          = AUIDO_PCM_MMAP_FORMATS,
	.channels_min     = AUIDO_PCM_PB_MIN_CHANNELS,
	.channels_max     = AUIDO_PCM_PB_MAX_CHANNELS,
	.buffer_bytes_max = PCM_DMA_BUF_MMAP_MAX_SIZE,
	.period_bytes_min = AUIDO_PCM_MIN_BUFFER_SIZE,
	.period_bytes_max = PCM_DMA_BUF_MMAP_MAX_SIZE,
	.periods_min      = AUIDO_PCM_MIN_PERIODS,
	.periods_max      = AUIDO_PCM_MAX_PERIODS,
	.fifo_size        = AUIDO_PCM_PB_FIFO_SIZE,
};

/* define the capability of capture channel for mmap device */
static const struct snd_pcm_hardware g_audio_pcm_hardware_mmap_capture = {
	.info             = SNDRV_PCM_INFO_INTERLEAVED
		| SNDRV_PCM_INFO_NONINTERLEAVED
		| SNDRV_PCM_INFO_MMAP
		| SNDRV_PCM_INFO_MMAP_VALID
		| SNDRV_PCM_INFO_PAUSE,
	.formats          = SNDRV_PCM_FMTBIT_S16_LE,
	.rates            = SNDRV_PCM_RATE_48000,
	.channels_min     = AUIDO_PCM_CP_MIN_CHANNELS,
	.channels_max     = AUIDO_PCM_CP_MAX_CHANNELS,
	.buffer_bytes_max = PCM_DMA_BUF_MMAP_MAX_SIZE,
	.period_bytes_min = AUIDO_PCM_MIN_BUFFER_SIZE,
	.period_bytes_max = PCM_DMA_BUF_MMAP_MAX_SIZE,
	.periods_min      = AUIDO_PCM_MIN_PERIODS,
	.periods_max      = AUIDO_PCM_MAX_PERIODS,
	.fifo_size        = AUIDO_PCM_CP_FIFO_SIZE,
};

static const struct snd_pcm_hardware *g_audio_pcm_hw_info[PCM_DEVICE_TOTAL][PCM_STREAM_MAX] = {
	{ &g_audio_pcm_hardware_playback, &g_audio_pcm_hardware_capture }, /* PCM_DEVICE_NORMAL */
	{ NULL, NULL }, /* PCM_DEVICE_MODEM */
	{ NULL, NULL }, /* PCM_DEVICE_FM */
	{ NULL, NULL }, /* PCM_DEVICE_OFFLOAD */
	{ &g_audio_pcm_hardware_direct_playback, NULL }, /* PCM_DEVICE_DIRECT */
	{ &g_audio_pcm_hardware_lowlatency_playback, &g_audio_pcm_hardware_lowlatency_capture }, /* PCM_DEVICE_LOW_LATENCY */
	{ &g_audio_pcm_hardware_mmap_playback, &g_audio_pcm_hardware_mmap_capture }, /* PCM_DEVICE_MMAP */
};

static bool is_valid_device(int pcm_device)
{
	switch (pcm_device) {
	case PCM_DEVICE_NORMAL:
	case PCM_DEVICE_DIRECT:
	case PCM_DEVICE_LOW_LATENCY:
	case PCM_DEVICE_MMAP:
		return true;
	default:
		return false;
	}
}

static const struct snd_pcm_hardware *get_hw_config(int device, int stream)
{
	if ((device < 0 || device >= PCM_DEVICE_TOTAL) || (stream < 0 || stream >= PCM_STREAM_MAX))
		return NULL;

	return g_audio_pcm_hw_info[device][stream];
}

static const struct pcm_dma_buf_config *get_buf_config(int device, int stream)
{
	if ((device < 0 || device >= PCM_DEVICE_TOTAL) || (stream < 0 || stream >= PCM_STREAM_MAX))
		return NULL;

	return &g_pcm_dma_buf_config[device][stream];
}

static void init_cust_interface(struct cust_priv *priv_data)
{
	priv_data->pcm_dai = g_audio_pcm_dai;
	priv_data->dai_num = ARRAY_SIZE(g_audio_pcm_dai);
	priv_data->get_pcm_hw_config = get_hw_config;
	priv_data->get_pcm_buf_config = get_buf_config;
	priv_data->end_addr = PCM_DMA_BUF_END_ADDR;
	priv_data->is_valid_pcm_device = is_valid_device;
	priv_data->dev_normal = PCM_DEVICE_NORMAL;
	priv_data->dev_lowlatency = PCM_DEVICE_LOW_LATENCY;
	priv_data->dev_mmap = PCM_DEVICE_MMAP;
	priv_data->dev_num = PCM_DEVICE_TOTAL;
	priv_data->is_cdc_device = false;
}

static int phone_pcm_platform_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cust_priv *priv_data = NULL;

	AUDIO_LOGI("begin");

	priv_data = devm_kzalloc(dev, sizeof(struct cust_priv), GFP_KERNEL);
	if (priv_data == NULL) {
		AUDIO_LOGE("priv data alloc failed");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, priv_data);

	init_cust_interface(priv_data);

	codec_register_cust_interface(priv_data);

	AUDIO_LOGI("end");
	return 0;
}

static const struct of_device_id audio_pcm_match[] = {
	{.compatible = "soc-dsp-phone-pcm-config", },
	{ },
};
static struct platform_driver phone_pcm_driver = {
	.driver = {
		.name  = "soc-dsp-phone-pcm-config",
		.owner = THIS_MODULE,
		.of_match_table = audio_pcm_match,
	},
	.probe  = phone_pcm_platform_probe,
};

static int __init phone_pcm_init(void)
{
	return platform_driver_register(&phone_pcm_driver);
}
fs_initcall(phone_pcm_init);

static void __exit phone_pcm_exit(void)
{
	platform_driver_unregister(&phone_pcm_driver);
}
module_exit(phone_pcm_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ASoC audio pcm driver");

