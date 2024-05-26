/*
 * codec_probe.h -- da combine v5 codec driver
 *
 * Copyright (c) 2018 Huawei Technologies Co., Ltd.
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

#ifndef __DA_COMBINE_V5_CODEC_PROBE_H__
#define __DA_COMBINE_V5_CODEC_PROBE_H__

#include <sound/soc.h>

int da_combine_v5_codec_probe(struct snd_soc_component *codec);
void da_combine_v5_codec_remove(struct snd_soc_component *codec);

struct snd_soc_component *da_combine_v5_get_codec(void);

#endif
