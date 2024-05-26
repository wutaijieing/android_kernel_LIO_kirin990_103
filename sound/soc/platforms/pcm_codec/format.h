/*
 * format.h
 *
 * format converter for pcm codec
 *
 * Copyright (c) 2019 Huawei Technologies Co., Ltd.
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

#ifndef __FORMAT_H__
#define __FORMAT_H__

#include <linux/types.h>
#include <sound/pcm.h>

/* Bit formats */
enum pcm_format {
	PCM_FORMAT_S16_LE = 0,
	PCM_FORMAT_S24_LE_LA,
	PCM_FORMAT_S24_LE_RA,
	PCM_FORMAT_S32_LE,
	PCM_FORMAT_S24_3LE,
	PCM_FORMAT_MAX,
};

/* data formats */
enum data_format {
	DATA_FORMAT_INTERLACED = 0,
	DATA_FORMAT_NONE_INTERLACED,
};

struct data_format_conversion_cfg {
	void *addr;
	enum data_format data_format;
	enum pcm_format pcm_format;
};

struct data_convert_info {
	struct data_format_conversion_cfg dest_cfg;
	struct data_format_conversion_cfg src_cfg;
	unsigned int period_size;
	unsigned int channels;
	unsigned int merge_interlace_channel;
};

uint32_t audio_bytes_per_sample(enum pcm_format format);
unsigned int get_bytes_per_sample(snd_pcm_format_t format);
void convert_format(struct data_convert_info *info);
int pcm_format_init(struct device *dev, unsigned int bytes);
#endif

