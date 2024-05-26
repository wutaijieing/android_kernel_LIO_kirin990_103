/* SPDX-License-Identifier: GPL-2.0 */
/*
 * wired_channel_manager.h
 *
 * wired_channel_manager module
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

#include <linux/slab.h>

#ifndef _WIRED_CHANNEL_MANAGER_H_
#define _WIRED_CHANNEL_MANAGER_H_

#define WDCM_DEV_ON      1
#define WDCM_DEV_OFF     0

enum wdcm_client {
	WDCM_CLIENT_BEGIN = 0,
	WDCM_CLIENT_WLS = WDCM_CLIENT_BEGIN,
	WDCM_CLIENT_WIRED,
	WDCM_CLIENT_OTG,
	WDCM_CLIENT_TX_OTG,
	WDCM_CLIENT_END,
};

#ifdef CONFIG_WIRED_CHANNEL_SWITCH
bool wdcm_dev_exist(void);
void wdcm_set_buck_channel_state(int client, int client_state);
#else
static inline bool wdcm_dev_exist(void)
{
	return false;
}

static inline void wdcm_set_buck_channel_state(int client, int client_state)
{
}
#endif /* CONFIG_WIRED_CHANNEL_SWITCH */

#endif /* _WIRED_CHANNEL_MANAGER_H_ */
