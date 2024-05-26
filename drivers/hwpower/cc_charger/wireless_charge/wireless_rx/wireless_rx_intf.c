// SPDX-License-Identifier: GPL-2.0
/*
 * wireless_rx_intf.c
 *
 * wireless rx interfaces
 *
 * Copyright (c) 2022-2022 Huawei Technologies Co., Ltd.
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

#include <chipset_common/hwpower/wireless_charge/wireless_rx_intf.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/common_module/power_dts.h>

#define HWLOG_TAG wireless_rx_intf
HWLOG_REGIST();

bool wlrx_support_wireless_charging(unsigned int drv_type)
{
	const char *status = NULL;

	if (power_dts_read_string_compatible(power_dts_tag(HWLOG_TAG),
		"huawei,wireless_charger", "status", &status))
		return false;

	if (!strcmp(status, "disabled"))
		return false;

	return true;
}
