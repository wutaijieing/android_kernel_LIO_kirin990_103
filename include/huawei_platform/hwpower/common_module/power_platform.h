/* SPDX-License-Identifier: GPL-2.0 */
/*
 * power_platform.h
 *
 * differentiated interface related to chip platform
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
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

#ifndef _POWER_PLATFORM_H_
#define _POWER_PLATFORM_H_

#include <huawei_platform/power/huawei_charger.h>
#include <huawei_platform/power/direct_charger/direct_charger.h>
#include <chipset_common/hwpower/wireless_charge/wireless_tx_pwr_ctrl.h>
#include <chipset_common/hwpower/direct_charge/direct_charge_ic_manager.h>
#include <huawei_platform/usb/hw_pogopin.h>
#include <huawei_platform/usb/hw_pd_dev.h>
#include <linux/errno.h>
#include <platform_include/basicplatform/linux/power/platform/coul/coul_drv.h>
#include <platform_include/basicplatform/linux/power/platform/coul/coul_event.h>
#include <platform_include/basicplatform/linux/power/platform/bci_battery.h>
#include <platform_include/basicplatform/linux/power/platform/soh/hisi_soh_interface.h>
#include <platform_include/basicplatform/linux/power/platform/battery_data.h>
#include <linux/adc.h>
#include <linux/hisi/usb/chip_usb.h>
#include <linux/hisi/usb/hisi_tcpm.h>
#include <platform_include/basicplatform/linux/mfd/pmic_platform.h>
#ifdef CONFIG_USE_CAMERA3_ARCH
#include <../../../../platform_source/camera/drivers/media/native/camera/pmic/hw_pmic.h>
#endif
#include <huawei_platform/power/batt_info_pub.h>
#ifdef CONFIG_POWERKEY_SPMI
#include <platform_include/basicplatform/linux/powerkey_event.h>
#endif
#include <chipset_common/hwpower/common_module/power_supply_application.h>
#include <huawei_platform/usb/usb_extra_modem.h>

static inline int power_platform_get_filter_soc(int base)
{
	return bci_capacity_get_filter_sum(base);
}

static inline void power_platform_sync_filter_soc(int rep_soc,
	int round_soc, int base)
{
	bci_capacity_sync_filter(rep_soc, round_soc, base);
}

static inline void power_platform_cancle_capacity_work(void)
{
}

static inline void power_platform_restart_capacity_work(void)
{
}

static inline int power_platform_get_adc_sample(int adc_channel)
{
	return lpm_adc_get_adc(adc_channel);
}

static inline int power_platform_get_adc_voltage(int adc_channel)
{
	return lpm_adc_get_value(adc_channel);
}

static inline int power_platform_get_battery_id_voltage(void)
{
	return coul_drv_battery_id_voltage();
}

static inline int power_platform_get_battery_ui_capacity(void)
{
	return coul_drv_battery_capacity();
}

static inline int power_platform_get_battery_temperature(void)
{
	return coul_drv_battery_temperature();
}

static inline int power_platform_get_rt_battery_temperature(void)
{
	return coul_drv_battery_temperature_for_charger();
}

static inline int power_platform_get_battery_current(void)
{
	return coul_drv_battery_current();
}

static inline int power_platform_is_battery_removed(void)
{
	return coul_drv_battery_removed_before_boot();
}

static inline int power_platform_is_battery_changed(void)
{
	int changed_flag = get_batt_changed_on_boot();

	if (changed_flag >= 0)
		return changed_flag;
	return coul_drv_battery_removed_before_boot();
}

static inline int power_platform_is_battery_exit(void)
{
	return coul_drv_is_battery_exist();
}

static inline int power_platform_get_vbus_status(void)
{
	return pmic_get_vbus_status();
}

#ifdef CONFIG_USE_CAMERA3_ARCH
static inline int power_platform_enable_ext_pmic_boost(int value)
{
	return pmic_enable_boost(value);
}
#else
static inline int power_platform_enable_ext_pmic_boost(int value)
{
	return 0;
}
#endif /* CONFIG_USE_CAMERA3_ARCH */

static inline int power_platform_enable_int_pmic_boost(int value)
{
	return 0;
}

static inline bool power_platform_usb_state_is_host(void)
{
	return chip_usb_state_is_host();
}

static inline bool power_platform_pogopin_is_support(void)
{
	return pogopin_is_support();
}

static inline bool power_platform_pogopin_otg_from_buckboost(void)
{
	return pogopin_otg_from_buckboost();
}

#ifdef CONFIG_POWERKEY_SPMI
static inline int power_platform_powerkey_register_notifier(struct notifier_block *nb)
{
	return powerkey_register_notifier(nb);
}

static inline int power_platform_powerkey_unregister_notifier(struct notifier_block *nb)
{
	return powerkey_unregister_notifier(nb);
}

static inline bool power_platform_is_powerkey_up(unsigned long event)
{
	return event == PRESS_KEY_UP;
}
#else
static inline int power_platform_powerkey_register_notifier(struct notifier_block *nb)
{
	return 0;
}

static inline int power_platform_powerkey_unregister_notifier(struct notifier_block *nb)
{
	return 0;
}

static inline bool power_platform_is_powerkey_up(unsigned long event)
{
	return false;
}
#endif /* CONFIG_POWERKEY_SPMI */

extern bool get_sysfs_wdt_disable_flag(void);
static inline bool power_platform_get_sysfs_wdt_disable_flag(void)
{
	return get_sysfs_wdt_disable_flag();
}

#ifdef CONFIG_CHARGER_SYS_WDG
extern void charge_enable_sys_wdt(void);
extern void charge_stop_sys_wdt(void);
extern void charge_feed_sys_wdt(unsigned int timeout);

static inline void power_platform_charge_enable_sys_wdt(void)
{
	charge_enable_sys_wdt();
}

static inline void power_platform_charge_stop_sys_wdt(void)
{
	charge_stop_sys_wdt();
}

static inline void power_platform_charge_feed_sys_wdt(unsigned int timeout)
{
	charge_feed_sys_wdt(timeout);
}
#else
static inline void power_platform_charge_enable_sys_wdt(void)
{
}

static inline void power_platform_charge_stop_sys_wdt(void)
{
}

static inline void power_platform_charge_feed_sys_wdt(unsigned int timeout)
{
}
#endif /* CONFIG_CHARGER_SYS_WDG */

static inline int power_platform_set_max_input_current(void)
{
	return charge_set_input_current_max();
}

static inline void power_platform_start_acr_calc(void)
{
	soh_acr_low_precision_cal_start();
}

static inline int power_platform_get_acr_resistance(int *acr_value)
{
	struct bat_acr_info acr_data;
	int ret;

	if (!acr_value)
		return -EPERM;

	memset(&acr_data, 0, sizeof(acr_data));
	ret = soh_get_acr_resistance(&acr_data, ACR_L_PRECISION);
	*acr_value = acr_data.batt_acr;
	return ret;
}

#ifdef CONFIG_DIRECT_CHARGER
static inline bool power_platform_in_dc_charging_stage(void)
{
	return direct_charge_in_charging_stage() == DC_IN_CHARGING_STAGE;
}
#else
static inline bool power_platform_in_dc_charging_stage(void)
{
	return false;
}
#endif /* CONFIG_DIRECT_CHARGER */

static inline void power_platform_set_charge_hiz(int enable)
{
	charge_set_hiz_enable(enable);
}

#ifdef CONFIG_SCHARGER_V300
extern bool is_hi6523_cv_limit(void);
static inline int power_platform_get_cv_limit(void)
{
	return is_hi6523_cv_limit() ? 150 : 0; /* 150 : cv limit 150mv */
}
#else
static inline int power_platform_get_cv_limit(void)
{
	return 0;
}
#endif /* CONFIG_SCHARGER_V300 */

extern void charger_vbus_init_handler(int vbus);
static inline void power_platform_buck_vbus_change_handler(int vbus)
{
	charger_vbus_init_handler(vbus);
}

static inline unsigned int power_platform_get_uem_leak_current(void)
{
	return uem_get_charge_leak_current();
}

static inline unsigned int power_platform_get_uem_resistance(void)
{
	return uem_get_charge_resistance();
}

static bool power_platform_check_online_status(void)
{
	return uem_check_online_status();
}
#endif /* _POWER_PLATFORM_H_ */
