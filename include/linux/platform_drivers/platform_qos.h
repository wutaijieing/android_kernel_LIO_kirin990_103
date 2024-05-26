/*
 * platform_qos.h
 *
 * platform qos module
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
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

#ifndef _PLATFORM_QOS_H
#define _PLATFORM_QOS_H

#include <linux/plist.h>
#include <linux/notifier.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/types.h>

/* Note : When you modify this struct, you need to notify Modem */
struct platform_qos_request {
	struct plist_node node;
	int platform_qos_class;
	struct delayed_work work; /* for platform_qos_update_request_timeout */
};

struct platform_qos_flags_request {
	struct list_head node;
	u32 flags; /* Do not change to 64 bit */
};

enum platform_qos_type {
	PLATFORM_QOS_UNITIALIZED,
	PLATFORM_QOS_MAX, /* return the largest value */
	PLATFORM_QOS_MIN, /* return the smallest value */
	PLATFORM_QOS_SUM /* return the sum */
};

enum {
	PLATFORM_QOS_RESERVED = 0,
#ifdef CONFIG_ONSYS_AVS
	PLATFORM_QOS_ONSYS_AVS_LEVEL,
#endif
#ifdef CONFIG_DEVFREQ_GOV_PLATFORM_QOS
	PLATFORM_QOS_MEMORY_LATENCY,
	PLATFORM_QOS_MEMORY_THROUGHPUT,
	PLATFORM_QOS_MEMORY_THROUGHPUT_UP_THRESHOLD,
#endif
#ifdef CONFIG_DEVFREQ_L1BUS_LATENCY_PLATFORM
	PLATFORM_QOS_L1BUS_LATENCY,
#endif
#ifdef CONFIG_GPUTOP_FREQ
	PLATFORM_QOS_GPUTOP_FREQ_DNLIMIT,
	PLATFORM_QOS_GPUTOP_FREQ_UPLIMIT,
#endif
#ifdef CONFIG_NPUFREQ_PLATFORM_QOS
	PLATFORM_QOS_NPU_FREQ_DNLIMIT,
	PLATFORM_QOS_NPU_FREQ_UPLIMIT,
#endif
#ifdef CONFIG_NETWORK_LATENCY_PLATFORM_QOS
	PLATFORM_QOS_NETWORK_LATENCY,
#endif
	PLATFORM_QOS_NUM_CLASSES,
};
#ifdef CONFIG_GPUTOP_FREQ
#define PLATFORM_QOS_GPUTOP_FREQ_DNLIMIT_DEFAULT_VALUE	0
#define PLATFORM_QOS_GPUTOP_FREQ_UPLIMIT_DEFAULT_VALUE	(20 * 1000)
#endif
#ifdef CONFIG_NPUFREQ_PLATFORM_QOS
#define PLATFORM_QOS_NPU_FREQ_DNLIMIT_DEFAULT_VALUE	0
#define PLATFORM_QOS_NPU_FREQ_UPLIMIT_DEFAULT_VALUE	(20 * 1000)
#endif
#ifdef CONFIG_NETWORK_LATENCY_PLATFORM_QOS
#define PLATFORM_QOS_NETWORK_LAT_DEFAULT_VALUE		(2000 * USEC_PER_SEC)
#endif

/*
 * Note: The lockless read path depends on the CPU accessing target_value
 * or effective_flags atomically.  Atomic access is only guaranteed on all CPU
 * types linux supports for 32 bit quantites
 */
struct platform_qos_constraints {
	struct plist_head list;
	s32 target_value; /* Do not change to 64 bit */
	s32 default_value;
	s32 no_constraint_value;
	enum platform_qos_type type;
	struct blocking_notifier_head *notifiers;
};

struct platform_qos_flags {
	struct list_head list;
	u32 effective_flags; /* Do not change to 64 bit */
};

/* Action requested to platform_qos_update_target */
enum platform_qos_req_action {
	PLATFORM_QOS_ADD_REQ, /* Add a new request */
	PLATFORM_QOS_UPDATE_REQ, /* Update an existing request */
	PLATFORM_QOS_REMOVE_REQ /* Remove an existing request */
};

int platform_qos_request(int platform_qos_class);
int platform_qos_request_active(struct platform_qos_request *req);
s32 platform_qos_read_value(struct platform_qos_constraints *c);
void platform_qos_add_request(struct platform_qos_request *req,
			      int platform_qos_class, s32 value);
void platform_qos_update_request(struct platform_qos_request *req,
				 s32 new_value);
void platform_qos_update_request_timeout(struct platform_qos_request *req,
					 s32 new_value,
					 unsigned long timeout_us);
void platform_qos_remove_request(struct platform_qos_request *req);

int platform_qos_add_notifier(int platform_qos_class, struct notifier_block *notifier);
int platform_qos_remove_notifier(int platform_qos_class, struct notifier_block *notifier);
#endif
