/* SPDX-License-Identifier: GPL-2.0 */
/*
 * charge_manager.h
 *
 * charge manager module
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

#ifndef _CHARGE_MANAGER_H_
#define _CHARGE_MANAGER_H_

#include <linux/device.h>
#include <linux/notifier.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/jiffies.h>
#include <chipset_common/hwpower/charger/charger_event.h>

struct pd_dpm_vbus_state;

struct charge_manager_info {
	struct device *dev;
	struct power_supply *charge_psy;
	struct power_supply *usb_psy;
	struct delayed_work charge_work;
	struct mutex event_type_lock;
	struct work_struct event_work;
	spinlock_t event_spin_lock;
	enum charger_event_type event;
	struct charger_event_queue event_queue;
	struct wakeup_source *charge_lock;
	int support_usb_psy;
	int charger_pd_support;
	int try_pd_to_scp_counter;
	int usb_online;
	int force_disable_dc_path;
	int enable_hv_charging;
#ifdef CONFIG_TCPC_CLASS
	struct notifier_block tcpc_nb;
	struct pd_dpm_vbus_state *vbus_state;
#endif
};

#ifdef CONFIG_CHARGE_MANAGER
void charger_source_sink_event(enum charger_event_type event);
void emark_detect_complete(void);
#else
static inline void charger_source_sink_event(enum charger_event_type event)
{
}

static inline void emark_detect_complete(void)
{
}
#endif /* CONFIG_CHARGE_MANAGER */
#endif /* _CHARGE_MANAGER_H_ */
