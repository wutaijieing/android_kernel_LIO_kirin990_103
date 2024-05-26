/*
 * loadmonitor_media_kernel.h
 *
 * media monitor module
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MEDIA_MONITOR_K_H__
#define __MEDIA_MONITOR_K_H__
#include <loadmonitor_plat.h>

enum {
	NO_READ = 0,
	CAN_READ,
};

int media0_monitor_read(unsigned int dpm_period_media);
int media1_monitor_read(unsigned int dpm_period_media);

#endif
