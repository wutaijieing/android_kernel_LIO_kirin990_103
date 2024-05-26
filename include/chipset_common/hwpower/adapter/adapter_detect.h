/* SPDX-License-Identifier: GPL-2.0 */
/*
 * adapter_detect.h
 *
 * detect of adapter driver
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
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

#ifndef _ADAPTER_DETECT_H_
#define _ADAPTER_DETECT_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/bitops.h>
#include <linux/notifier.h>
#include <chipset_common/hwpower/protocol/adapter_protocol.h>

#define ADAPTER_DETECT_PING_TIMES      3
#define ADAPTER_DETECT_RETRY_TIMES     3

typedef int (*adapter_detect_ping_cb)(int *);

struct adapter_detect_priority_data {
	unsigned int prot_type;
	const char *prot_name;
	adapter_detect_ping_cb ping_cb;
	bool compatible;
};

struct adapter_detect_dev {
	struct notifier_block plugged_nb;
	struct mutex plugged_lock;
	bool plugged_state;
	struct mutex ping_lock;
	unsigned int ping_times;
	unsigned int init_prot_type;
	unsigned int rt_prot_type;
	unsigned int sysfs_prot_type;
	bool init_prot_type_flag;
	bool ufcs_ping_fail_flag;
};

#ifdef CONFIG_ADAPTER_PROTOCOL
void adapter_detect_init_protocol_type(void);
bool adapter_detect_check_protocol_type(unsigned int prot_type);
unsigned int adapter_detect_get_runtime_protocol_type(void);
int adapter_detect_ping_ufcs_type(int *adp_mode);
int adapter_detect_ping_scp_type(int *adp_mode);
int adapter_detect_ping_fcp_type(int *adp_mode);
unsigned int adapter_detect_ping_protocol_type(void);
#else
static inline void adapter_detect_init_protocol_type(void)
{
}

static inline bool adapter_detect_check_protocol_type(unsigned int prot_type)
{
	return false;
}

static inline unsigned int adapter_detect_get_runtime_protocol_type(void)
{
	return 0;
}

static inline int adapter_detect_ping_ufcs_type(int *adp_mode)
{
	return ADAPTER_DETECT_FAIL;
}

static inline int adapter_detect_ping_scp_type(int *adp_mode)
{
	return ADAPTER_DETECT_FAIL;
}

static inline int adapter_detect_ping_fcp_type(int *adp_mode)
{
	return ADAPTER_DETECT_FAIL;
}

static inline unsigned int adapter_detect_ping_protocol_type(void)
{
	return 0;
}
#endif /* CONFIG_ADAPTER_PROTOCOL */

#endif /* _ADAPTER_DETECT_H_ */
