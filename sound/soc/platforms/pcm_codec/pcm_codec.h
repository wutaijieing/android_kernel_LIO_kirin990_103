/*
 * pcm_codec.h
 *
 * pcm codec driver
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

#ifndef __PCM_CODEC_H__
#define __PCM_CODEC_H__

#include <sound/soc.h>
#include <sound/pcm.h>

#include "format.h"

#define MAX_STREAM_TYPES (SNDRV_PCM_STREAM_LAST + 1)
#define SHORT_BITS 16

#define MAX_FILE_NAME_LEN 256

struct cust_priv {
	char name[MAX_FILE_NAME_LEN];
	struct snd_soc_dai_driver *pcm_dai;
	unsigned int  dai_num;
	const struct snd_pcm_hardware* (*get_pcm_hw_config)(int device, int stream);
	void (*data_process)(struct data_convert_info *info, int device, int stream);
};

void pcm_codec_register_cust_interface(struct cust_priv *cust);
void default_data_process(struct data_convert_info *convert_info);

#endif /* PCM_CODEC_H */

