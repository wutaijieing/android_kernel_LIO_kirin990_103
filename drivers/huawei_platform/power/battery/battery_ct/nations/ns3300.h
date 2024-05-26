/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ns3300.h
 *
 * ns3300 driver head file for battery checker
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

#ifndef _NS3300_H_
#define _NS3300_H_
#include <linux/types.h>
#include <linux/device.h>
#include <huawei_platform/power/power_mesg_srv.h>
#include "../batt_early_param.h"
#include "../batt_aut_checker.h"

#define LOW_VOLTAGE                         0
#define HIGH_VOLTAGE                        1
#define ASCII_0                             0x30
#define NS3300_UID_LEN                      12
#define NS3300_BATTTYP_LEN                  2
#define NS3300_SN_ASC_LEN                   16
#define NS3300_ODC_LEN                      48
#define NS3300_ACT_LEN                      60
#define NS3300_DEFAULT_TAU                  7  /* default: 7us */
#define NS3300_DFT_FULL_CYC                 0xffff
#define NS3300_IC_NAME                      "ns3300"

struct ns3300_memory {
	uint8_t uid[NS3300_UID_LEN];
	bool uid_ready;
	uint8_t batt_type[NS3300_BATTTYP_LEN];
	bool batttp_ready;
	uint8_t sn[NS3300_SN_ASC_LEN];
	bool sn_ready;
	uint8_t res_ct[NS3300_ODC_LEN];
	bool res_ct_ready;
	uint8_t act_sign[NS3300_ACT_LEN];
	bool act_sig_ready;
	enum batt_source source;
	bool ecce_pass;
	enum batt_ic_type ic_type;
	enum batt_cert_state cet_rdy;
};

struct ns3300_dev {
	struct device *dev;
	int onewire_gpio;
	spinlock_t onewire_lock;
	struct ns3300_memory mem;
	uint32_t tau;
	uint16_t cyc_full;
	bool sysfs_mode;
};

#endif /* _NS3300_H_ */
