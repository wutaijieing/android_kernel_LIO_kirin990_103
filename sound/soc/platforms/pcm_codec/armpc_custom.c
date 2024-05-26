/*
 * armpc_custom.c
 *
 * pcm armpc custom driver
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
#define LOG_TAG "armpc_custom"
#endif

static int32_t *ec_convert_buf;
#define PB_PERIOD_BYTES_MAX (3840 * 2)
#define PB_PERIODS_MAX       2
#define PB_BUFF_BYTES_MAX   (PB_PERIOD_BYTES_MAX * PB_PERIODS_MAX)

#define CP_PERIOD_BYTES_MAX (3840 * 3)
#define CP_PERIODS_MAX       2
#define CP_BUFF_BYTES_MAX   (CP_PERIOD_BYTES_MAX * CP_PERIODS_MAX)

static const struct snd_pcm_hardware armpc_hardware_playback = {
	.info               = SNDRV_PCM_INFO_INTERLEAVED,
	.formats            = SNDRV_PCM_FMTBIT_S16_LE,
	.period_bytes_min   = 32,
	.period_bytes_max   = PB_PERIOD_BYTES_MAX,
	.periods_min        = 2,
	.periods_max        = PB_PERIODS_MAX,
	.buffer_bytes_max   = PB_BUFF_BYTES_MAX,
	.rates              = 48000,
	.channels_min       = 2,
	.channels_max       = 4,
};

static const struct snd_pcm_hardware armpc_hardware_capture = {
	.info               = SNDRV_PCM_INFO_INTERLEAVED,
	.formats            = SNDRV_PCM_FMTBIT_S16_LE,
	.period_bytes_min   = 32,
	.period_bytes_max   = CP_PERIOD_BYTES_MAX,
	.periods_min        = 2,
	.periods_max        = CP_PERIODS_MAX,
	.buffer_bytes_max   = CP_BUFF_BYTES_MAX,
	.rates              = 48000,
	.channels_min       = 1,
	.channels_max       = 6,
};

static const struct snd_pcm_hardware armpc_hardware_playback_bt = {
	.info               = SNDRV_PCM_INFO_INTERLEAVED,
	.formats            = SNDRV_PCM_FMTBIT_S16_LE,
	.period_bytes_min   = 32,
	.period_bytes_max   = PB_PERIOD_BYTES_MAX,
	.periods_min        = 2,
	.periods_max        = PB_PERIODS_MAX,
	.buffer_bytes_max   = PB_BUFF_BYTES_MAX,
	.rates              = 48000,
	.channels_min       = 2,
	.channels_max       = 2,
};

static const struct snd_pcm_hardware armpc_hardware_capture_bt = {
	.info               = SNDRV_PCM_INFO_INTERLEAVED,
	.formats            = SNDRV_PCM_FMTBIT_S16_LE,
	.period_bytes_min   = 32,
	.period_bytes_max   = CP_PERIOD_BYTES_MAX,
	.periods_min        = 2,
	.periods_max        = CP_PERIODS_MAX,
	.buffer_bytes_max   = CP_BUFF_BYTES_MAX,
	.rates              = 48000,
	.channels_min       = 1,
	.channels_max       = 2,
};

static const struct snd_pcm_hardware *armpc_hardware_config[PCM_DEVICE_TOTAL][PCM_STREAM_MAX] = {
		{ &armpc_hardware_playback, &armpc_hardware_capture },
		{ &armpc_hardware_playback_bt, &armpc_hardware_capture_bt },
};

static const struct snd_pcm_hardware *_get_pcm_hw_config(int device, int stream)
{
	if (!is_device_valid(device) || !is_stream_valid(stream))
		return NULL;

	return armpc_hardware_config[device][stream];
}

static unsigned int pcm_get_hw_buffer_bytes_max(void)
{
	int device, stream;
	unsigned int ret = 0;

	for (device = 0; device < PCM_DEVICE_TOTAL; device++) {
		for (stream = 0; stream < PCM_STREAM_MAX; stream++) {
			if (armpc_hardware_config[device][stream] != NULL)
				ret = armpc_hardware_config[device][stream]->buffer_bytes_max > ret ?
					armpc_hardware_config[device][stream]->buffer_bytes_max : ret;
		}
	}

	return ret;
}

static void _convert_ec_format(void *dma_addr,
	unsigned int record_channel_cnt, uint32_t period_size)
{
	int32_t *record_data = NULL;
	int16_t *ec_data = NULL;
	int32_t position;
	unsigned int i, j;

	record_data = (int32_t *)dma_addr;
	for (i = 0; i < period_size * record_channel_cnt; i++)
		ec_convert_buf[i] = record_data[i];

	ec_data = (int16_t *)((int32_t *)dma_addr + period_size * record_channel_cnt);
	for (i = 0; i < period_size * EC_CHANNEL_CNT;) {
		for (j = 0; j < EC_CHANNEL_CNT; j++) {
			position = period_size * (record_channel_cnt + j) + i / EC_CHANNEL_CNT;
			ec_convert_buf[position] = ec_data[i++] << SHORT_BITS;
		}
	}
}

static int _devm_alloc_ec_buff(struct device *dev)
{
	unsigned int max = pcm_get_hw_buffer_bytes_max();

	if (max == 0) {
		AUDIO_LOGE("get hardware buffer max bytes is 0");
		return -EINVAL;
	}

	ec_convert_buf = devm_kzalloc(dev, max, GFP_KERNEL);
	if (ec_convert_buf == NULL) {
		AUDIO_LOGE("malloc memory fail, request size is %u", max);
		return -ENOMEM;
	}

	return 0;
}

static void _capture_data_process(struct data_convert_info *info, int device)
{
	if (info->channels == RECORD_EC_CHANNEL_CNT && device == 0) {
		_convert_ec_format(info->src_cfg.addr,
						   RECORD_CHANNEL_CNT_MAX,
						   info->period_size);
		info->src_cfg.addr = ec_convert_buf;
	}

	default_data_process(info);
}

static void _data_process(struct data_convert_info *info, int device, int stream)
{
	if (info == NULL || !is_device_valid(device) || !is_stream_valid(stream))
		return;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		default_data_process(info);
	else
		_capture_data_process(info, device);
}

static void _init_custom_interface(struct cust_priv *cust_privdata)
{
	if (cust_privdata == NULL)
		return;

	cust_privdata->get_pcm_hw_config = _get_pcm_hw_config;
	cust_privdata->data_process = _data_process;
}

static int armpc_custom_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct cust_priv *priv_data = NULL;

	AUDIO_LOGI("begin");

	priv_data = devm_kzalloc(dev, sizeof(struct cust_priv), GFP_KERNEL);
	if (priv_data == NULL) {
		AUDIO_LOGE("priv data alloc failed");
		return -ENOMEM;
	}

	ret = _devm_alloc_ec_buff(dev);
	if (ret != 0) {
		AUDIO_LOGE("ec buff alloc failed, result: %d", ret);
		return -EFAULT;
	}

	platform_set_drvdata(pdev, priv_data);

	_init_custom_interface(priv_data);

	pcm_codec_register_cust_interface(priv_data);

	AUDIO_LOGI("end");
	return ret;
}

static const struct of_device_id armpc_custom_match[] = {
	{ .compatible = "hisilicon,armpc-custom", },
	{},
};

static struct platform_driver armpc_custom_driver = {
	.driver = {
		.name  = "armpc-custom",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(armpc_custom_match),
	},
	.probe = armpc_custom_probe,
};

static int __init armpc_custom_init(void)
{
	return platform_driver_register(&armpc_custom_driver);
}
fs_initcall(armpc_custom_init);

static void __exit armpc_custom_exit(void)
{
	platform_driver_unregister(&armpc_custom_driver);
}
module_exit(armpc_custom_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ASoC armpc custom driver");

