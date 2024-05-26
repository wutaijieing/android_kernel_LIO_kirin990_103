/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ns3300_eng.h
 *
 * ns3300_eng driver head for factory or debug
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

#ifndef _NS3300_ENG_H_
#define _NS3300_ENG_H_

#include <linux/types.h>
#include <linux/device.h>
#include "ns3300.h"
#include <huawei_platform/log/hw_log.h>

#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif
#define HWLOG_TAG battct_ns3300
HWLOG_REGIST();

#if defined(BATTBD_FORCE_MATCH) || defined(ONEWIRE_STABILITY_DEBUG)
int ns3300_eng_get_batt_sn(struct ns3300_dev *di);
int ns3300_eng_set_act_signature(struct ns3300_dev *di,
	enum res_type type, const struct power_genl_attr *res);
int ns3300_eng_set_batt_as_org(struct ns3300_dev *di);
int ns3300_eng_set_cert_ready(struct ns3300_dev *di);
int ns3300_eng_act_read(struct ns3300_dev *di);
int ns3300_eng_init(struct ns3300_dev *di);
int ns3300_eng_deinit(struct ns3300_dev *di);
#else
static inline int ns3300_eng_get_batt_sn(struct ns3300_dev *di)
{
	return -1;
}

static inline int ns3300_eng_set_act_signature(struct ns3300_dev *di,
	enum res_type type, const struct power_genl_attr *res)
{
	return -1;
}

static inline int ns3300_eng_set_batt_as_org(struct ns3300_dev *di)
{
	hwlog_info("[%s] operation banned in user mode\n", __func__);
	return 0;
}

static inline int ns3300_eng_set_cert_ready(struct ns3300_dev *di)
{
	hwlog_info("[%s] operation banned in user mode\n", __func__);
	return 0;
}

static inline int ns3300_eng_act_read(struct ns3300_dev *di)
{
	return -1;
}

int ns3300_eng_init(struct ns3300_dev *di)
{
	return 0;
}

int ns3300_eng_deinit(struct ns3300_dev *di)
{
	return 0;
}
#endif

#endif /* _NS3300_H_ */
