/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020. All rights reserved.
 * Description: pedometer.h
 * Create: 2020/2/17
 */

#ifndef __PEDOMETER_H
#define __PEDOMETER_H

#include <linux/types.h>
#include <platform_include/smart/linux/base/ap/protocol.h>
#include "pedo_type.h"
#include "device_manager.h"

struct sensorlist_info *get_pedo_sensor_list_info(int shb_type);

struct pedometer_status {
	s32 pedometer_shb_type;
	bool enable;
	u32 interval; // only use for step counter wakeup, use as config steps
	u32 latency;
	s32 non_wakeup;
	u32 flags;
	char *dts_name; // used by reading dts
	struct sensorlist_info sensor_info;
};

/*
 * Function    : get_pedo_status
 * Description : get pedo status pointer from global array
 * Input       : [shb_type] pedo sensor type
 * Output      : none
 * Return      : null is shb_type invalid, else return the corresponding pointer
 */
struct pedometer_status *get_pedo_status(int shb_type);
#endif
