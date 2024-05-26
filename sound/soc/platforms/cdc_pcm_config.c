/*
 * cdc_pcm_config.c
 *
 * hifi cdc custom driver
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
#define LOG_TAG "cdc_pcm"
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
		SNDRV_PCM_FMTBIT_U24_BE | \
		SNDRV_PCM_FMTBIT_S32_LE | \
		SNDRV_PCM_FMTBIT_S32_BE | \
		SNDRV_PCM_FMTBIT_U32_LE | \
		SNDRV_PCM_FMTBIT_U32_BE)

/*
 * PLAYBACK SUPPORT RATES
 * 8/11.025/16/22.05/32/44.1/48/88.2/96kHz
 */
#define AUIDO_PCM_PB_RATES    (SNDRV_PCM_RATE_8000_48000 | \
		SNDRV_PCM_RATE_44100 | \
		SNDRV_PCM_RATE_88200 | \
		SNDRV_PCM_RATE_96000 | \
		SNDRV_PCM_RATE_176400 | \
		SNDRV_PCM_RATE_192000)

#define AUIDO_PCM_PB_MIN_CHANNELS  1
#define AUIDO_PCM_PB_MAX_CHANNELS  4
#define AUIDO_PRI_PB_MAX_CHANNELS  12
/* Assume the FIFO size */
#define AUIDO_PCM_PB_FIFO_SIZE     16

#define PCM_BT_PB_MIN_CHANNELS 2
#define PCM_BT_PB_MAX_CHANNELS 2
#define PCM_BT_CP_MIN_CHANNELS 2
#define PCM_BT_CP_MAX_CHANNELS 2
#define PCM_FM_CP_MIN_CHANNELS 2
#define PCM_FM_CP_MAX_CHANNELS 2

/* CAPTURE SUPPORT FORMATS : SIGNED 16/24bit */
#define AUIDO_PCM_CP_FORMATS  (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE)

/* CAPTURE SUPPORT RATES : 48/96kHz */
#define AUIDO_PCM_CP_RATES    SNDRV_PCM_RATE_8000_384000

#define AUIDO_PCM_CP_MIN_CHANNELS  1
#define AUIDO_PCM_CP_MAX_CHANNELS  13
#define AUIDO_PRI_CP_MAX_CHANNELS  30
/* Assume the FIFO size */
#define AUIDO_PCM_CP_FIFO_SIZE     32
#define AUIDO_PCM_FM_RATES         (SNDRV_PCM_RATE_8000_48000)

#define AUIDO_PCM_MAX_BUFFER_SIZE  (192 * 1024)    /* 0x30000 */
#define AUIDO_PCM_MIN_BUFFER_SIZE  32
#define AUIDO_PCM_MAX_PERIODS      32
#define AUIDO_PCM_MIN_PERIODS      2

#undef NULL
#define NULL ((void *)0)

/* sampling rate * time * bit width * channel num * buf num */
#define PCM_DMA_BUF_PLAYBACK_PRIMARY_LEN (48 * 20 * 4 * 12 * 2)
#define PCM_DMA_BUF_CAPTURE_PRIMARY_LEN (48 * 20 * 2 * 13 * 8)
#define PCM_DMA_BUF_CAPTURE_FM_LEN (48 * 20 * 4 * 4 * 2)
#define PCM_DMA_BUF_PLAYBACK_BT_LEN (48 * 20 * 4 * 4 * 2)
#define PCM_DMA_BUF_PLAYBACK_FAST_LEN (48 * 5 * 4 * 12 * 2)
#define PCM_DMA_BUF_CAPTURE_BT_LEN (48 * 20 * 4 * 4 * 2)

/* primary */
#define PCM_DMA_BUF_0_PLAYBACK_LEN  (PCM_DMA_BUF_PLAYBACK_PRIMARY_LEN)
#define PCM_DMA_BUF_0_CAPTURE_BASE  (PCM_DMA_BUF_0_PLAYBACK_BASE + PCM_DMA_BUF_0_PLAYBACK_LEN)
#define PCM_DMA_BUF_0_CAPTURE_LEN   (PCM_DMA_BUF_CAPTURE_PRIMARY_LEN)
/* BT device */
#define PCM_DMA_BUF_1_PLAYBACK_BASE (PCM_DMA_BUF_0_CAPTURE_BASE + PCM_DMA_BUF_0_CAPTURE_LEN)
#define PCM_DMA_BUF_1_PLAYBACK_LEN  (PCM_DMA_BUF_PLAYBACK_BT_LEN)
#define PCM_DMA_BUF_1_CAPTURE_BASE  (PCM_DMA_BUF_1_PLAYBACK_BASE + PCM_DMA_BUF_1_PLAYBACK_LEN)
#define PCM_DMA_BUF_1_CAPTURE_LEN   (PCM_DMA_BUF_CAPTURE_BT_LEN)
/* FM device */
#define PCM_DMA_BUF_2_CAPTURE_BASE (PCM_DMA_BUF_1_CAPTURE_BASE + PCM_DMA_BUF_1_CAPTURE_LEN)
#define PCM_DMA_BUF_2_CAPTURE_LEN  (PCM_DMA_BUF_CAPTURE_FM_LEN)
/* lowlatency */
#define PCM_DMA_BUF_3_PLAYBACK_BASE (PCM_DMA_BUF_2_CAPTURE_BASE + PCM_DMA_BUF_2_CAPTURE_LEN)
#define PCM_DMA_BUF_3_PLAYBACK_LEN  (PCM_DMA_BUF_PLAYBACK_FAST_LEN)
#define PCM_DMA_BUF_3_CAPTURE_BASE  (PCM_DMA_BUF_3_PLAYBACK_BASE + PCM_DMA_BUF_3_PLAYBACK_LEN)
#define PCM_DMA_BUF_3_CAPTURE_LEN   (PCM_DMA_BUF_PLAYBACK_BT_LEN)
/* mmap device */
#define PCM_DMA_BUF_4_PLAYBACK_BASE (PCM_DMA_BUF_3_CAPTURE_BASE + PCM_DMA_BUF_3_CAPTURE_LEN)
#define PCM_DMA_BUF_4_PLAYBACK_LEN  (PCM_DMA_BUF_MMAP_MAX_SIZE)
#define PCM_DMA_BUF_4_CAPTURE_BASE  (PCM_DMA_BUF_4_PLAYBACK_BASE + PCM_DMA_BUF_4_PLAYBACK_LEN)
#define PCM_DMA_BUF_4_CAPTURE_LEN   (PCM_DMA_BUF_MMAP_MAX_SIZE)

#define PCM_DMA_BUF_END_ADDR        (PCM_DMA_BUF_4_CAPTURE_BASE + PCM_DMA_BUF_4_CAPTURE_LEN)

enum pcm_device {
	PCM_DEVICE_NORMAL = 0,
	PCM_DEVICE_BT,
	PCM_DEVICE_FM,
	PCM_DEVICE_OFFLOAD,
	PCM_DEVICE_DIRECT,
	PCM_DEVICE_LOW_LATENCY,
	PCM_DEVICE_MMAP,
	PCM_DEVICE_TOTAL
};

static const struct pcm_dma_buf_config g_cdc_dma_buf_config[PCM_DEVICE_TOTAL][PCM_STREAM_MAX] = {
	{ /* normal */
		{PCM_DMA_BUF_0_PLAYBACK_BASE, PCM_DMA_BUF_0_PLAYBACK_LEN},
		{PCM_DMA_BUF_0_CAPTURE_BASE, PCM_DMA_BUF_0_CAPTURE_LEN}
	},
	{ /* BT */
		{PCM_DMA_BUF_1_PLAYBACK_BASE, PCM_DMA_BUF_1_PLAYBACK_LEN},
		{PCM_DMA_BUF_1_CAPTURE_BASE, PCM_DMA_BUF_1_CAPTURE_LEN}
	},
	{ /* fm */
		{0, 0}, {PCM_DMA_BUF_2_CAPTURE_BASE, PCM_DMA_BUF_2_CAPTURE_LEN}
	},
	{ {0, 0}, {0, 0} }, /* offload */
	{ {0, 0}, {0, 0} }, /* direct */
	{ /* lowlatency */
		{PCM_DMA_BUF_3_PLAYBACK_BASE, PCM_DMA_BUF_3_PLAYBACK_LEN},
		{PCM_DMA_BUF_3_CAPTURE_BASE, PCM_DMA_BUF_3_CAPTURE_LEN}
	},
	{ /* mmap */
		{PCM_DMA_BUF_4_PLAYBACK_BASE, PCM_DMA_BUF_4_PLAYBACK_LEN},
		{PCM_DMA_BUF_4_CAPTURE_BASE, PCM_DMA_BUF_4_CAPTURE_LEN}
	},
};

static struct snd_soc_dai_driver g_cdc_pcm_dai[] = {
	{
		.name = "asp-pcm-mm",
		.playback = {
			.stream_name  = "asp-pcm-mm Playback",
			.channels_min = AUIDO_PCM_PB_MIN_CHANNELS,
			.channels_max = AUIDO_PRI_PB_MAX_CHANNELS,
			.rates        = AUIDO_PCM_PB_RATES,
			.formats      = AUIDO_PCM_PB_FORMATS
		},
		.capture = {
			.stream_name  = "asp-pcm-mm Capture",
			.channels_min = AUIDO_PCM_CP_MIN_CHANNELS,
			.channels_max = AUIDO_PRI_CP_MAX_CHANNELS,
			.rates        = AUIDO_PCM_CP_RATES,
			.formats      = AUIDO_PCM_CP_FORMATS
		},
	},
	{
		.name = "asp-pcm-bt",
		.playback = {
			.stream_name  = "asp-pcm-bt Playback",
			.channels_min = AUIDO_PCM_PB_MIN_CHANNELS,
			.channels_max = AUIDO_PCM_PB_MAX_CHANNELS,
			.rates        = AUIDO_PCM_PB_RATES,
			.formats      = AUIDO_PCM_PB_FORMATS
		},
		.capture = {
			.stream_name  = "asp-pcm-bt Capture",
			.channels_min = AUIDO_PCM_PB_MIN_CHANNELS,
			.channels_max = AUIDO_PCM_PB_MAX_CHANNELS,
			.rates        = AUIDO_PCM_CP_RATES,
			.formats      = AUIDO_PCM_CP_FORMATS
		},
	},
	{
		.name = "asp-pcm-fm",
		.capture = {
			.stream_name  = "asp-pcm-fm Capture",
			.channels_min = AUIDO_PCM_PB_MIN_CHANNELS,
			.channels_max = AUIDO_PCM_PB_MAX_CHANNELS,
			.rates        = AUIDO_PCM_FM_RATES,
			.formats      = AUIDO_PCM_PB_FORMATS
		},
	},
	{
		.name =  "asp-pcm-offload",
		.capture = {
			.stream_name  = "asp-pcm-offload Capture",
			.channels_min = AUIDO_PCM_PB_MIN_CHANNELS,
			.channels_max = AUIDO_PCM_PB_MAX_CHANNELS,
			.rates        = AUIDO_PCM_PB_RATES,
			.formats      = AUIDO_PCM_PB_FORMATS
		},
	},
	{
		.name =  "asp-pcm-direct",
		.capture = {
			.stream_name  = "asp-pcm-direct Capture",
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
			.channels_max = AUIDO_PRI_PB_MAX_CHANNELS,
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
			.formats      = AUIDO_PCM_PB_FORMATS
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
	.channels_max     = AUIDO_PRI_PB_MAX_CHANNELS,
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
	.channels_max     = AUIDO_PRI_CP_MAX_CHANNELS,
	.buffer_bytes_max = AUIDO_PCM_MAX_BUFFER_SIZE,
	.period_bytes_min = AUIDO_PCM_MIN_BUFFER_SIZE,
	.period_bytes_max = AUIDO_PCM_MAX_BUFFER_SIZE,
	.periods_min      = AUIDO_PCM_MIN_PERIODS,
	.periods_max      = AUIDO_PCM_MAX_PERIODS,
	.fifo_size        = AUIDO_PCM_CP_FIFO_SIZE,
};

static const struct snd_pcm_hardware g_cdc_hardware_fm_capture = {
	.info               = SNDRV_PCM_INFO_INTERLEAVED,
	.formats            = SNDRV_PCM_FMTBIT_S16_LE,
	.period_bytes_min   = 32,
	.period_bytes_max   = 3840 * 2, /* sampling rate * time * bit width * channel num */
	.periods_min        = 2,
	.periods_max        = 2,
	.buffer_bytes_max   = 4 * 3840,
	.rates              = 48000,
	.channels_min       = PCM_FM_CP_MIN_CHANNELS,
	.channels_max       = PCM_FM_CP_MAX_CHANNELS,
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
	.channels_max     = AUIDO_PRI_PB_MAX_CHANNELS,
	.buffer_bytes_max = PCM_DMA_BUF_PLAYBACK_FAST_LEN,
	.period_bytes_min = AUIDO_PCM_MIN_BUFFER_SIZE,
	.period_bytes_max = PCM_DMA_BUF_PLAYBACK_FAST_LEN,
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
	.buffer_bytes_max = PCM_DMA_BUF_PLAYBACK_BT_LEN,
	.period_bytes_min = AUIDO_PCM_MIN_BUFFER_SIZE,
	.period_bytes_max = PCM_DMA_BUF_PLAYBACK_BT_LEN,
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
	.formats          = SNDRV_PCM_FMTBIT_S16_LE,
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

static const struct snd_pcm_hardware g_cdc_hardware_bt_playback = {
	.info               = SNDRV_PCM_INFO_INTERLEAVED,
	.formats            = SNDRV_PCM_FMTBIT_S24_LE,
	.period_bytes_min   = 32,
	.period_bytes_max   = 3840 * 2,
	.periods_min        = 2,
	.periods_max        = 2,
	.buffer_bytes_max   = 4 * 3840 * 2,
	.rates              = 48000,
	.channels_min       = PCM_BT_PB_MIN_CHANNELS,
	.channels_max       = PCM_BT_PB_MAX_CHANNELS,
};

static const struct snd_pcm_hardware g_cdc_hardware_bt_capture = {
	.info               = SNDRV_PCM_INFO_INTERLEAVED,
	.formats            = SNDRV_PCM_FMTBIT_S16_LE,
	.period_bytes_min   = 32,
	.period_bytes_max   = 3840 * 2,
	.periods_min        = 2,
	.periods_max        = 2,
	.buffer_bytes_max   = 4 * 3840,
	.rates              = 48000,
	.channels_min       = PCM_BT_CP_MIN_CHANNELS,
	.channels_max       = PCM_BT_CP_MAX_CHANNELS,
};

static bool is_valid_device(int pcm_device)
{
	switch (pcm_device) {
	case PCM_DEVICE_NORMAL:
	case PCM_DEVICE_FM:
	case PCM_DEVICE_BT:
	case PCM_DEVICE_LOW_LATENCY:
	case PCM_DEVICE_MMAP:
		return true;
	default:
		return false;
	}
}

static const struct snd_pcm_hardware *g_cdc_pcm_hw_info[PCM_DEVICE_TOTAL][PCM_STREAM_MAX] = {
	{ &g_audio_pcm_hardware_playback, &g_audio_pcm_hardware_capture }, /* PCM_DEVICE_NORMAL */
	{ &g_cdc_hardware_bt_playback, &g_cdc_hardware_bt_capture }, /* PCM_DEVICE_BT */
	{ NULL, &g_cdc_hardware_fm_capture }, /* PCM_DEVICE_FM */
	{ NULL, NULL }, /* PCM_DEVICE_OFFLOAD */
	{ NULL, NULL }, /* PCM_DEVICE_DIRECT */
	{ &g_audio_pcm_hardware_lowlatency_playback, &g_audio_pcm_hardware_lowlatency_capture }, /* PCM_DEVICE_LOW_LATENCY */
	{ &g_audio_pcm_hardware_mmap_playback, &g_audio_pcm_hardware_mmap_capture }, /* PCM_DEVICE_MMAP */
};

static const struct snd_pcm_hardware *get_hw_config(int device, int stream)
{
	if (!is_valid_device(device) || (stream < 0 || stream >= PCM_STREAM_MAX))
		return NULL;

	return g_cdc_pcm_hw_info[device][stream];
}

static const struct pcm_dma_buf_config *get_buf_config(int device, int stream)
{
	if (!is_valid_device(device) || (stream < 0 || stream >= PCM_STREAM_MAX))
		return NULL;

	return &g_cdc_dma_buf_config[device][stream];
}

static void init_cust_interface(struct cust_priv *priv_data)
{
	priv_data->pcm_dai = g_cdc_pcm_dai;
	priv_data->dai_num = ARRAY_SIZE(g_cdc_pcm_dai);
	priv_data->get_pcm_hw_config = get_hw_config;
	priv_data->get_pcm_buf_config = get_buf_config;
	priv_data->end_addr = PCM_DMA_BUF_END_ADDR;
	priv_data->is_valid_pcm_device = is_valid_device;
	priv_data->dev_normal = PCM_DEVICE_NORMAL;
	priv_data->dev_lowlatency = PCM_DEVICE_LOW_LATENCY;
	priv_data->dev_mmap = PCM_DEVICE_MMAP;
	priv_data->dev_num = PCM_DEVICE_TOTAL;
	priv_data->is_cdc_device = true;
}

static int cdc_pcm_probe(struct platform_device *pdev)
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


static const struct of_device_id cdc_pcm_match[] = {
	{ .compatible = "soc-dsp-cdc-pcm-config", },
	{},
};

static struct platform_driver cdc_pcm_driver = {
	.driver = {
		.name  = "soc-dsp-cdc-pcm-config",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(cdc_pcm_match),
	},
	.probe = cdc_pcm_probe,
};

static int __init cdc_pcm_init(void)
{
	return platform_driver_register(&cdc_pcm_driver);
}
fs_initcall(cdc_pcm_init);

static void __exit cdc_pcm_exit(void)
{
	platform_driver_unregister(&cdc_pcm_driver);
}
module_exit(cdc_pcm_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ASoC cdc pcm driver");

