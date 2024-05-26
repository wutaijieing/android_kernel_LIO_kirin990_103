/*
 * media_monitor.h
 *
 * media_monitor
 *
 * Copyright (C) 2017-2020 Huawei Technologies Co., Ltd.
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
#ifndef __MEDIA_MONITOR_H__
#define __MEDIA_MONITOR_H__

#include <linux/platform_device.h>
#include <linux/types.h>

enum {
	INFO_IP_NUM = 0,
	INFO_MONITOR_FREQ,
	INFO_IP_MASK,
	INFO_MAX,
};

enum {
	NO_READ = 0,
	CAN_READ,
};

#define MEDIA_INFO_LEN	2

extern u32 g_media_info[INFO_MAX];
extern struct platform_driver g_media_dpm_driver;
int media_monitor_read(unsigned int dpm_period_media);

#endif
