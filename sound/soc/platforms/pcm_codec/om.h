/*
 * om.h
 *
 * om module for pcm codec
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

#ifndef __OM_H__
#define __OM_H__

#ifdef AUDIO_PCM_CODEC_DEBUG

enum DUMP_DATA_TYPE {
	PLAY_AP_DATA,
	PLAY_DMA_DATA,
	CAPTURE_AP_DATA,
	CAPTURE_DMA_DATA,
	DUMP_CNT
};

void pcm_codec_set_dump_flag(int flag);
void pcm_codec_dump_data(char *buf, unsigned int size, int device, enum DUMP_DATA_TYPE dump_type);

#endif
#endif

