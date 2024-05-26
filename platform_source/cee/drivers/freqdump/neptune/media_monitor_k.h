/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: monitor device status file
 * Author: duyouxuan
 * Create: 2019-03-29
 */

#ifndef __MEDIA_MONITOR_K_H__
#define __MEDIA_MONITOR_K_H__
#include <loadmonitor_plat.h>

enum {
	NO_READ = 0,
	CAN_READ,
};

int media_monitor_read(unsigned int dpm_period_media);
int media0_monitor_read(unsigned int dpm_period_media);
int media1_monitor_read(unsigned int dpm_period_media);

#endif
