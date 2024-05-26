// SPDX-License-Identifier: GPL-2.0
/*
 * power_icon.c
 *
 * icon interface for power module
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

#include <chipset_common/hwpower/common_module/power_supply_interface.h>
#include <chipset_common/hwpower/common_module/power_ui_ne.h>
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG power_icon
HWLOG_REGIST();

static int g_icon_type;

void power_icon_notify(int icon_type)
{
	hwlog_info("notify icon_type=%d\n", icon_type);

	g_icon_type = icon_type;
	power_ui_event_notify(POWER_UI_NE_ICON_TYPE, &icon_type);
	power_supply_sync_changed("usb");
	power_supply_sync_changed("battery");
	power_supply_sync_changed("Battery");
}

int power_icon_inquire(void)
{
	return g_icon_type;
}
