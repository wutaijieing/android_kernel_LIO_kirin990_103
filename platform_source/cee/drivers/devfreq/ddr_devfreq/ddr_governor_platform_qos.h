/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 */
#ifndef __DDR_GOVERNOR_PLATFORM_QOS_H__
#define __DDR_GOVERNOR_PLATFORM_QOS_H__

struct devfreq_platform_qos_data {
	unsigned int bytes_per_sec_per_hz;
	unsigned int bd_utilization;
	int platform_qos_class;
	unsigned long freq;
};

#endif /* __DDR_GOVERNOR_PLATFORM_QOS_H__ */
