// SPDX-License-Identifier: GPL-2.0
/*
 * wireless_tx_client.c
 *
 * tx client for wireless reverse charging
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

#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/wireless_charge/wireless_tx_client.h>

#define HWLOG_TAG wireless_tx_client
HWLOG_REGIST();

static struct {
	enum wltx_client client;
} g_wltx_client;

enum wltx_client wltx_get_client(void)
{
	return g_wltx_client.client;
}

void wltx_set_client(enum wltx_client client)
{
	g_wltx_client.client = client;
	hwlog_info("[set_client] client=%d\n", g_wltx_client.client);
}

bool wltx_client_is_hall(void)
{
	return g_wltx_client.client == WLTX_CLIENT_HALL;
}
