// SPDX-License-Identifier: GPL-2.0
/*
 * charger_type.c
 *
 * charger type for charger module
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

#include <chipset_common/hwpower/charger/charger_type.h>
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG charger_type
HWLOG_REGIST();

static const char * const g_charge_type_name[CHARGER_TYPE_END] = {
	[CHARGER_TYPE_USB] = "usb",
	[CHARGER_TYPE_BC_USB] = "bc_usb",
	[CHARGER_TYPE_NON_STANDARD] = "non_stdard",
	[CHARGER_TYPE_STANDARD] = "standard",
	[CHARGER_TYPE_FCP] = "fcp",
	[CHARGER_REMOVED] = "removed",
	[USB_EVENT_OTG_ID] = "otg_id",
	[CHARGER_TYPE_VR] = "vr",
	[CHARGER_TYPE_TYPEC] = "typec",
	[CHARGER_TYPE_PD] = "pd",
	[CHARGER_TYPE_SCP] = "scp",
	[CHARGER_TYPE_WIRELESS] = "wireless",
	[CHARGER_TYPE_POGOPIN] = "pogopin",
	[CHARGER_TYPE_APPLE_2_1A] = "apple_2_1a",
	[CHARGER_TYPE_APPLE_1_0A] = "apple_1_0a",
	[CHARGER_TYPE_APPLE_0_5A] = "apple_0_5a",
	[CHARGER_TYPE_BATTERY] = "battery",
	[CHARGER_TYPE_UPS] = "ups",
	[CHARGER_TYPE_MAINS] = "mains",
	[CHARGER_TYPE_ACA] = "aca",
	[CHARGER_TYPE_PD_DRP] = "pd_drp",
	[CHARGER_TYPE_APPLE_BRICK_ID] = "apple_brick_id",
	[CHARGER_TYPE_HVDCP] = "hvdcp",
	[CHARGER_TYPE_HVDCP_3] = "hvdcp_3",
	[CHARGER_TYPE_BMS] = "bms",
	[CHARGER_TYPE_PARALLEL] = "parallel",
	[CHARGER_TYPE_MAIN] = "main",
	[CHARGER_TYPE_UFP] = "ufp",
	[CHARGER_TYPE_DFP] = "dfp",
	[CHARGER_TYPE_CHARGE_PUMP] = "charge_pump",
};

const char *charge_get_charger_type_name(unsigned int type)
{
	if (type < CHARGER_TYPE_END)
		return g_charge_type_name[type];

	hwlog_err("invalid type=%u\n", type);
	return "invalid";
}
