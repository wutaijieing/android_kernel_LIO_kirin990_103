/*
 * codec_bus.h -- codec_bus driver
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
#ifndef __CODEC_BUS_H__
#define __CODEC_BUS_H__

#include <linux/types.h>

enum {
	SAMPLE_RATE_8K = 8000,
	SAMPLE_RATE_16K = 16000,
	SAMPLE_RATE_32K = 32000,
	SAMPLE_RATE_48K = 48000,
	SAMPLE_RATE_96K = 96000,
	SAMPLE_RATE_192K = 192000,
	SAMPLE_RATE_384K = 384000,
	SAMPLE_RATE_768K = 768000,
	SAMPLE_RATE_44K1 = 44100,
	SAMPLE_RATE_88K2 = 88200,
	SAMPLE_RATE_176K4 = 176400
};

enum framer_type {
	FRAMER_SOC,
	FRAMER_CODEC,
	FRAMER_NUM
};

enum data_port_type {
	PORT_TYPE_NONE,
	PORT_TYPE_U1,
	PORT_TYPE_U2,
	PORT_TYPE_U3,
	PORT_TYPE_U4,
	PORT_TYPE_U5,
	PORT_TYPE_U6,
	PORT_TYPE_U7,
	PORT_TYPE_U8,
	PORT_TYPE_U9,
	PORT_TYPE_U10,
	PORT_TYPE_U11,
	PORT_TYPE_U12,
	PORT_TYPE_U13,
	PORT_TYPE_U14,
	PORT_TYPE_U15,
	PORT_TYPE_U16,
	PORT_TYPE_U17,
	PORT_TYPE_U18,
	PORT_TYPE_D1,
	PORT_TYPE_D2,
	PORT_TYPE_D3,
	PORT_TYPE_D4,
	PORT_TYPE_D5,
	PORT_TYPE_D6,
	PORT_TYPE_D7,
	PORT_TYPE_D8,
	PORT_TYPE_D9,
	PORT_TYPE_D10,
	PORT_TYPE_MAX
};

enum priority {
	LOW_PRIORITY,
	NORMAL_PRIORITY,
	HIGH_PRIORITY
};

typedef int (*scene_callback_t)(const void *param);

#define DATA_PORTS_MAX 10
struct scene_param {
	enum data_port_type ports[DATA_PORTS_MAX];
	unsigned int channels;
	unsigned int rate;
	unsigned int bits_per_sample;
	enum priority priority;
	scene_callback_t callback;
};

int codec_bus_activate(const char* scene_name, struct scene_param *params);
int codec_bus_deactivate(const char* scene_name, struct scene_param *params);
int codec_bus_switch_framer(enum framer_type framer_type);
int codec_bus_runtime_get(void);
void codec_bus_runtime_put(void);
int codec_bus_enum_device(void);
enum framer_type codec_bus_get_framer_type(void);

#endif
