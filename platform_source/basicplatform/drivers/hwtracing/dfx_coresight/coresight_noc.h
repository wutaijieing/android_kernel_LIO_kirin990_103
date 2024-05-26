/*
 * coresight-noc.h
 *
 * Coresight NOC Trace driver module
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

#ifndef _CORESIGHT_CORESIGHT_NOC_H
#define _CORESIGHT_CORESIGHT_NOC_H

#include <linux/spinlock.h>

#define CFGCTRL_DISABLE 0x0
#define STPV2EN_DISABLE 0x0
#define ATBEN_DISABLE 0x0
#define TRACE_ALARMCLR 0x7
#define SECURITY_BASE 0x0
#define SECURITY_MASK 0x0
#define NOC_TRACE_STATUS 0x3
#define NOC_TRACE_LENGTH 0xF
#define URGENCY 0x0
#define USER_BASE 0x0
#define USER_MASK 0x0
#define ATBEN_ENABLE 0x1
#define ASYNC_PERIOD_ENABLE 0x17
#define STPV2EN_ENABLE 0x1
#define MAINCTL_ENABLE 0x0F
#define CFGCTL_ENABLE 0x1

#define TRACEID_WIDE 0x7f
#define NUM_NOC 3

#define FILTER_NUM_MAX 3
#define NUM_DISABLE_CFG_OFFSET 4
#define NUM_FLITER_CFG_OFFSET 13
#define NUM_ENABLE_CFG_OFFSET 7
#define NUM_FLITER_CFG 6

struct disable_configuration_offset {
	u32 cfgctrl_offset;
	u32 stpv2en_offset;
	u32 atben_offset;
	u32 tracealarmclr_offset;
};

struct filter_configuration_offset {
	u32 routeidbase_offset;
	u32 routeidmask_offset;
	u32 addrbase_low_offset;
	u32 addrbase_high_offset;
	u32 windowsize_offset;
	u32 securitybase_offset;
	u32 securitymask_offset;
	u32 opcode_offset;
	u32 status_offset;
	u32 length_offset;
	u32 urgency_offset;
	u32 userbase_offset;
	u32 usermask_offset;
};

struct enable_configuration_offset {
	u32 filterlut_offset;
	u32 atbid_offset;
	u32 atben_offset;
	u32 asyncperiod_offset;
	u32 stpv2en_offset;
	u32 mainctl_offset;
	u32 cfgctrl_offset;
};

struct filter_configuration {
	u32 routeidbase;
	u32 routeidmask;
	u32 addrbase_low;
	u32 addrbase_high;
	u32 windowsize;
	u32 opcode;
};

/**
 * struct noc_trace_drvdata - specifics associated to an noc component
 * @base:		   memory mapped base address for this component.
 * @dev:		   the device entity associated to this component.
 * @csdev:		   component vitals needed by the framework.
 * @spinlock:	   only one at a time pls.
 * @enable:		   Is this noc trace currently tracing.
 * @boot_enable:   True if we should start tracing at boot time.
 * @trace_id;:	   value of the current ID for this component.
 * @filter_nums:   the number of the filter.
 * @filterlut:    look up table for filter selection.
 * @disable_cfg_offset:   register offset of disable configuration.
 * @filter_cfg_offset;:	   register offset of filter configuration.
 * @enable_cfg_offset:   register offset of enable configuration.
 * @filter_cfg:    the configuration of filter.
 */
struct noc_trace_drvdata {
	void __iomem *base;
	struct device *dev;
	struct coresight_device *csdev;
	spinlock_t spinlock;
	bool enable;
	bool boot_enable;
	u32 trace_id;
	u32 filter_nums;
	u32 filterlut;
	struct disable_configuration_offset *disable_cfg_offset;
	struct filter_configuration_offset *filter_cfg_offset[FILTER_NUM_MAX];
	struct enable_configuration_offset *enable_cfg_offset;
	struct filter_configuration *filter_cfg[FILTER_NUM_MAX];
};

#endif
