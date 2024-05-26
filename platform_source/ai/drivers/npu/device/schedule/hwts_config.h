/*
 * hwts_config.h
 *
 * about hwts config
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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

#ifndef _HWTS_CONFIG_H_
#define _HWTS_CONFIG_H_

#include <linux/types.h>

enum {
	HWTS_CHANNEL_GROUP0,
	HWTS_CHANNEL_GROUP1,
	HWTS_CHANNEL_GROUP2,
	HWTS_CHANNEL_GROUP3,
	HWTS_CHANNEL_GROUP4,
	HWTS_CHANNEL_GROUP5,
	HWTS_CHANNEL_GROUP6,
	HWTS_CHANNEL_GROUP7,
	HWTS_CHANNEL_GROUP8,
	HWTS_CHANNEL_GROUP9,
	HWTS_CHANNEL_GROUP10,
	HWTS_CHANNEL_GROUP11,
	HWTS_CHANNEL_GROUP12,
	HWTS_CHANNEL_GROUP13,
	HWTS_CHANNEL_GROUP14,
	HWTS_CHANNEL_GROUP15
};

enum {
	HWTS_HOST_TYPE_ISP = 0,
	HWTS_HOST_TYPE_ACPU = 1,
	HWTS_HOST_TYPE_SENSORHUB = 2,
	HWTS_HOST_TYPE_TSCPU = 3,
	HWTS_HOST_TYPE_RSV
};

enum {
	HWTS_PRIORITY_GRP_PLATFORM0 = 0,
	HWTS_PRIORITY_GRP_PLATFORM1,
	HWTS_PRIORITY_GRP_PLATFORM2,
	HWTS_PRIORITY_GRP_HIGHEST,
	HWTS_PRIORITY_GRP_HIGHER,
	HWTS_PRIORITY_GRP_HIGH,
	HWTS_PRIORITY_GRP_MIDDLE,
	HWTS_PRIORITY_GRP_LOW,
	HWTS_PRIORITY_GRP_MAX
};

enum {
	HWTS_RTSQ_TYPE_NS_AIC = 0,
	HWTS_RTSQ_TYPE_NS_AIV,
	HWTS_RTSQ_TYPE_TINY,
	HWTS_RTSQ_TYPE_ISPNN,
	HWTS_RTSQ_TYPE_SEC,
	HWTS_RTSQ_TYPE_RSV
};

struct hwts_group_item {
	uint16_t group_id;
	uint16_t host_type;
	uint16_t start_channel_id;
	uint16_t channel_count;
};

struct hwts_priority_item {
	uint16_t rtsq_type;
	uint16_t priority;
	/* This parameter is not used by software */
	/* It is easy to understand that the non-preemption priority uses the same group */
	bool     is_preempty;
	uint16_t group_id;
};

uint16_t hwts_config_get_group_channel_count(uint16_t group_id);
uint16_t hwts_config_get_group_start_channel_id(uint16_t group_id);
uint16_t hwts_config_get_group_id(uint16_t rtsq_type, uint16_t priority);

#endif
