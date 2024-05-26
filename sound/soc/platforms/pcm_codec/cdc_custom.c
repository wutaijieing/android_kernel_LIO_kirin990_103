/*
 * cdc_custom.c
 *
 * pcm cdc custom driver
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <sound/soc.h>
#include <sound/pcm.h>

#include "pcm_codec.h"
#include "format.h"
#include "platform_io.h"
#include "audio_log.h"

#ifndef LOG_TAG
#define LOG_TAG "cdc_custom"
#endif

#define BIT_SHIFT_FOR_MICIN_MERGE 16
#define DEFAULT_INTERLACED_CHANNEL 2

#define PCM_PB_FORMATS (SNDRV_PCM_FMTBIT_S8 | \
		SNDRV_PCM_FMTBIT_U8 | \
		SNDRV_PCM_FMTBIT_S16_LE | \
		SNDRV_PCM_FMTBIT_S16_BE | \
		SNDRV_PCM_FMTBIT_U16_LE | \
		SNDRV_PCM_FMTBIT_U16_BE | \
		SNDRV_PCM_FMTBIT_S24_LE | \
		SNDRV_PCM_FMTBIT_S24_BE | \
		SNDRV_PCM_FMTBIT_U24_LE | \
		SNDRV_PCM_FMTBIT_U24_BE)

/*
 * PLAYBACK SUPPORT RATES
 * 8/11.025/16/22.05/32/44.1/48/88.2/96kHz
 */
#ifdef CONFIG_USB_384K_AUDIO_ADAPTER_SUPPORT
#define PCM_PB_RATES (SNDRV_PCM_RATE_8000_48000 | \
		SNDRV_PCM_RATE_44100 | \
		SNDRV_PCM_RATE_88200 | \
		SNDRV_PCM_RATE_96000 | \
		SNDRV_PCM_RATE_176400 | \
		SNDRV_PCM_RATE_192000 | \
		SNDRV_PCM_RATE_384000)
#else
#define PCM_PB_RATES (SNDRV_PCM_RATE_8000_48000 | \
		SNDRV_PCM_RATE_44100 | \
		SNDRV_PCM_RATE_88200 | \
		SNDRV_PCM_RATE_96000 | \
		SNDRV_PCM_RATE_176400 | \
		SNDRV_PCM_RATE_192000)
#endif

/* Assume the FIFO size */
#define PCM_PB_FIFO_SIZE 16

/* CAPTURE SUPPORT FORMATS : SIGNED 16/24bit */
#define PCM_CP_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE)

/* CAPTURE SUPPORT RATES : 48/96kHz */
#define PCM_CP_RATES (SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000)

#define CDC_MICIN_CHANNELS 4
#define CDC_EC_CHANNELS 4

#define PCM_PB_MIN_CHANNELS 1
#define PCM_PB_MAX_CHANNELS 6
#define PCM_BT_PB_MIN_CHANNELS 2
#define PCM_BT_PB_MAX_CHANNELS 2

#define PCM_CP_MIN_CHANNELS 1
#define PCM_CP_MAX_CHANNELS 8
#define PCM_BT_CP_MIN_CHANNELS 2
#define PCM_BT_CP_MAX_CHANNELS 2
#define PCM_FM_CP_MIN_CHANNELS 2
#define PCM_FM_CP_MAX_CHANNELS 2

#define PB_PERIOD_BYTES_MAX (3840 * 3)
#define PB_PERIODS_MAX       4
#define PB_BUFF_BYTES_MAX   (PB_PERIOD_BYTES_MAX * PB_PERIODS_MAX)

#define CP_PERIOD_BYTES_MAX (3840 * 4)
#define CP_PERIODS_MAX       2
#define CP_BUFF_BYTES_MAX   (CP_PERIOD_BYTES_MAX * CP_PERIODS_MAX)

static struct snd_soc_dai_driver pcm_codec_dai[] = {
	{
		.name = "asp-pcm-mm",
		.playback = {
			.stream_name  = "asp-pcm-mm Playback",
			.channels_min = PCM_PB_MIN_CHANNELS,
			.channels_max = PCM_PB_MAX_CHANNELS,
			.rates        = PCM_PB_RATES,
			.formats      = PCM_PB_FORMATS
		},
		.capture = {
			.stream_name  = "asp-pcm-mm Capture",
			.channels_min = PCM_CP_MIN_CHANNELS,
			.channels_max = PCM_CP_MAX_CHANNELS,
			.rates        = PCM_CP_RATES,
			.formats      = PCM_CP_FORMATS
		},
	},
	{
		.name = "asp-pcm-bt",
		.playback = {
			.stream_name  = "asp-pcm-bt Playback",
			.channels_min = PCM_BT_PB_MIN_CHANNELS,
			.channels_max = PCM_BT_PB_MAX_CHANNELS,
			.rates        = PCM_PB_RATES,
			.formats      = PCM_PB_FORMATS
		},
		.capture = {
			.stream_name  = "asp-pcm-bt Capture",
			.channels_min = PCM_BT_CP_MIN_CHANNELS,
			.channels_max = PCM_BT_CP_MAX_CHANNELS,
			.rates        = PCM_CP_RATES,
			.formats      = PCM_CP_FORMATS
		},
	},
	{
		.name = "asp-pcm-fm",
		.capture = {
			.stream_name  = "asp-pcm-fm Capture",
			.channels_min = PCM_FM_CP_MIN_CHANNELS,
			.channels_max = PCM_FM_CP_MAX_CHANNELS,
			.rates        = PCM_CP_RATES,
			.formats      = PCM_CP_FORMATS
		},
	},
};

static const struct snd_pcm_hardware cdc_hardware_playback = {
	.info               = SNDRV_PCM_INFO_INTERLEAVED,
	.formats            = SNDRV_PCM_FMTBIT_S16_LE,
	.period_bytes_min   = 32,
	.period_bytes_max   = PB_PERIOD_BYTES_MAX,
	.periods_min        = 2,
	.periods_max        = PB_PERIODS_MAX,
	.buffer_bytes_max   = PB_BUFF_BYTES_MAX,
	.rates              = 48000,
	.channels_min       = PCM_PB_MIN_CHANNELS,
	.channels_max       = PCM_PB_MAX_CHANNELS,
};

static const struct snd_pcm_hardware cdc_hardware_capture = {
	.info               = SNDRV_PCM_INFO_INTERLEAVED,
	.formats            = SNDRV_PCM_FMTBIT_S16_LE,
	.period_bytes_min   = 32,
	.period_bytes_max   = CP_PERIOD_BYTES_MAX,
	.periods_min        = 2,
	.periods_max        = CP_PERIODS_MAX,
	.buffer_bytes_max   = CP_BUFF_BYTES_MAX,
	.rates              = 48000,
	.channels_min       = PCM_CP_MIN_CHANNELS,
	.channels_max       = PCM_CP_MAX_CHANNELS,
};

static const struct snd_pcm_hardware cdc_hardware_bt_playback = {
	.info               = SNDRV_PCM_INFO_INTERLEAVED,
	.formats            = SNDRV_PCM_FMTBIT_S16_LE,
	.period_bytes_min   = 32,
	.period_bytes_max   = 3840 * 2,
	.periods_min        = 2,
	.periods_max        = 2,
	.buffer_bytes_max   = 4 * 3840 * 2,
	.rates              = 48000,
	.channels_min       = PCM_BT_PB_MIN_CHANNELS,
	.channels_max       = PCM_BT_PB_MAX_CHANNELS,
};

static const struct snd_pcm_hardware cdc_hardware_bt_capture = {
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

static const struct snd_pcm_hardware cdc_hardware_fm_capture = {
	.info               = SNDRV_PCM_INFO_INTERLEAVED,
	.formats            = SNDRV_PCM_FMTBIT_S16_LE,
	.period_bytes_min   = 32,
	.period_bytes_max   = 3840 * 2,
	.periods_min        = 2,
	.periods_max        = 2,
	.buffer_bytes_max   = 4 * 3840,
	.rates              = 48000,
	.channels_min       = PCM_FM_CP_MIN_CHANNELS,
	.channels_max       = PCM_FM_CP_MAX_CHANNELS,
};

static const struct snd_pcm_hardware *cdc_hardware_config[PCM_DEVICE_TOTAL][PCM_STREAM_MAX] = {
		{ &cdc_hardware_playback, &cdc_hardware_capture},
		{ &cdc_hardware_bt_playback, &cdc_hardware_bt_capture},
		{ NULL, &cdc_hardware_fm_capture},
};

static const struct snd_pcm_hardware *_get_pcm_hw_config(int device, int stream)
{
	if (!is_device_valid(device) || !is_stream_valid(stream))
		return NULL;

	return cdc_hardware_config[device][stream];
}

static void _playback_data_process(struct data_convert_info *info)
{
	convert_format(info);
}

static void convert_hi_lo_word(uint32_t *out_buffer, uint32_t *in_buffer, unsigned int runtime_period_size)
{
	uint32_t i, j, ch1_data, ch2_data, ch3_data, ch4_data;

	for (i = 0; i < runtime_period_size; i++) {
		j = i * DEFAULT_INTERLACED_CHANNEL;
		ch1_data = in_buffer[j] & 0xFFFF0000;
		ch2_data = in_buffer[j] & 0x0000FFFF;
		ch3_data = in_buffer[j + 1] & 0xFFFF0000;
		ch4_data = in_buffer[j + 1] & 0x0000FFFF;

		out_buffer[j] = (ch2_data << BIT_SHIFT_FOR_MICIN_MERGE) | (ch1_data >> BIT_SHIFT_FOR_MICIN_MERGE);
		out_buffer[j + 1] = (ch4_data << BIT_SHIFT_FOR_MICIN_MERGE) | (ch3_data >> BIT_SHIFT_FOR_MICIN_MERGE);
	}
}
static void cdc_micin_data_process(struct data_convert_info *info)
{
	convert_hi_lo_word((uint32_t *)info->dest_cfg.addr, (uint32_t *)info->src_cfg.addr, info->period_size);
}

static unsigned int get_micin_ap_frame_bytes(struct data_convert_info *info)
{
	unsigned int sample_bytes = audio_bytes_per_sample(info->dest_cfg.pcm_format);
	return info->period_size * sample_bytes * CDC_MICIN_CHANNELS;
}

static unsigned int get_micin_dma_frame_bytes(struct data_convert_info *info)
{
	/* cdc using 1 dma to deal with the micin data */
	return info->period_size * INTERLACED_DMA_BYTES;
}

static void cdc_micin_ec_data_process(struct data_convert_info *info)
{
	struct data_convert_info new_info;

	cdc_micin_data_process(info);
	/* deal with the ec data */
	new_info = *info;
	new_info.dest_cfg.addr = info->dest_cfg.addr + get_micin_ap_frame_bytes(info);
	new_info.src_cfg.addr = info->src_cfg.addr + get_micin_dma_frame_bytes(info);
	new_info.channels = CDC_EC_CHANNELS;
	new_info.merge_interlace_channel = DEFAULT_INTERLACED_CHANNEL;
	convert_format(&new_info);
}

static void _capture_data_process(struct data_convert_info *info, int device)
{
	unsigned int channels = info->channels;

	if (device == PCM_DEVICE_NORMAL) {
		if(channels == RECORD_CHANNEL_CNT_MAX) {
			cdc_micin_data_process(info);
		} else if (channels == PCM_CP_MAX_CHANNELS) {
			cdc_micin_ec_data_process(info);
		} else {
			AUDIO_LOGE("error param channels: %d, device: %d", channels, device);
		}
	} else if ((device == PCM_DEVICE_BT) || (device == PCM_DEVICE_FM)) {
		convert_format(info);
	} else {
		AUDIO_LOGE("error param device: %d", device);
	}
}

static void _cdc_data_process(struct data_convert_info *info, int device, int stream)
{
	if (info == NULL || !is_device_valid(device) || !is_stream_valid(stream))
		return;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		_playback_data_process(info);
	else
		_capture_data_process(info, device);
}

static void _init_cust_interface(struct cust_priv *priv_data)
{
	if (priv_data == NULL)
		return;

	priv_data->pcm_dai = pcm_codec_dai;
	priv_data->dai_num = ARRAY_SIZE(pcm_codec_dai);
	priv_data->get_pcm_hw_config = _get_pcm_hw_config;
	priv_data->data_process = _cdc_data_process;
}

static int cdc_custom_probe(struct platform_device *pdev)
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

	_init_cust_interface(priv_data);

	pcm_codec_register_cust_interface(priv_data);

	AUDIO_LOGI("end");
	return 0;
}


static const struct of_device_id cdc_custom_match[] = {
	{ .compatible = "hisilicon,cdc-custom", },
	{},
};

static struct platform_driver cdc_custom_driver = {
	.driver = {
		.name  = "cdc-custom",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(cdc_custom_match),
	},
	.probe = cdc_custom_probe,
};

static int __init cdc_custom_init(void)
{
	return platform_driver_register(&cdc_custom_driver);
}
fs_initcall(cdc_custom_init);

static void __exit cdc_custom_exit(void)
{
	platform_driver_unregister(&cdc_custom_driver);
}
module_exit(cdc_custom_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ASoC cdc custom driver");

