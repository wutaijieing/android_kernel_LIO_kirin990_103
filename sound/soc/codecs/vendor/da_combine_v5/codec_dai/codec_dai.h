/*
 * codec_dai.h -- codec_dai driver
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
#ifndef __CODEC_DAI_H__
#define __CODEC_DAI_H__

#include <linux/version.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>

#include "asoc_adapter.h"

#define FS_VALUE_8K    0x0
#define FS_VALUE_16K   0x1
#define FS_VALUE_32K   0x2
#define FS_VALUE_48K   0x4
#define FS_VALUE_96K   0x5
#define FS_VALUE_192K  0x6
#define FS_VALUE_44K1  0x4
#define FS_VALUE_88K2  0x5
#define FS_VALUE_176K4 0x6

#define params_sample_rate(params, data_port, sample_rate) \
	do { \
		(params).port = data_port; \
		(params).rate = sample_rate; \
	} while (0)

enum data_port {
	PORT_D1_D2,
	PORT_D3_D4,
	PORT_D5_D6,
	PORT_D7_D8,
	PORT_D9_D10,
	PORT_U1_U2,
	PORT_U3_U4,
	PORT_U5_U6,
	PORT_U7_U8,
	PORT_U9,
	PORT_U10,
	PORT_U9_U10,
	PORT_U11_U12,
	PORT_U13_U14,
	PORT_U15_U16,
	PORT_U17_U18
};

struct port_config {
	enum data_port port;
	unsigned int reg;
};

struct rate_reg_config {
	unsigned int rate;
	unsigned int up_bit_val;
	unsigned int dn_bit_val;
};

struct codec_dai_sample_rate_params {
	enum data_port port;
	unsigned int rate;
};

struct da_combine_v5_codec_dai {
	struct snd_soc_dai_driver *dai_driver;
	unsigned int dai_nums;
	int (*config_sample_rate)(struct snd_soc_component *codec,
			 struct codec_dai_sample_rate_params *hwparms);
	void (*codec_dai_init)(struct snd_soc_component *codec);
};

int codec_dai_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params, struct snd_soc_dai *dai);

int codec_dai_digital_mute(struct snd_soc_dai *dai, int mute);

int codec_dai_mute_stream(struct snd_soc_dai *dai, int mute, int stream);

int codec_dai_probe(struct snd_soc_dai *dai);

#ifdef CONFIG_PLATFORM_SLIMBUS
struct da_combine_v5_codec_dai *get_slimbus_codec_dai(void);
#endif

#ifdef CONFIG_PLATFORM_SOUNDWIRE
struct da_combine_v5_codec_dai *get_soundwire_codec_dai(void);
#endif

#endif

