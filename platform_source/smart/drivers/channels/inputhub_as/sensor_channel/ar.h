/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020. All rights reserved.
 * Description: ar.h
 * Create: 2020/2/17
 */

#ifndef __AR_H
#define __AR_H

#include <linux/types.h>
#include "device_manager.h"
#include "ar_type.h"

struct sensorlist_info *get_ar_sensor_list_info(int shb_type);

struct ar_status {
	int ar_shb_type;
	bool support_timeout;
	bool enable;
	int report_latency; // latency after event happened. ar not support sample interval
	int default_report_latency;// just for offbody dts cfg is int max, others is 0 now
	int non_wakeup;
	int care_event; // 1 for enter, 2 for exit, 3 for all
	u32 flags;
	char *dts_name;
	struct sensorlist_info sensor_info;
};

/*
 * Function    : get_ar_status
 * Description : get ar status pointer from global array
 * Input       : [shb_type] ar sensor type
 * Output      : none
 * Return      : null is shb_type invalid, else return the corresponding pointer
 */
struct ar_status *get_ar_status(int shb_type);

#endif
