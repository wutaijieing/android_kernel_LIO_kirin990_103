// SPDX-License-Identifier: GPL-2.0
/*
 * direct_charge_uevent.c
 *
 * uevent handle for direct charge
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

#include <huawei_platform/power/direct_charger/direct_charger.h>
#include <huawei_platform/power/battery_voltage.h>
#include <chipset_common/hwpower/common_module/power_common_macro.h>
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG direct_charge_uevent
HWLOG_REGIST();

#define CTC_EMARK_CURR_5A         5000

static int dc_get_max_current(void)
{
	int adp_limit;
	int cable_limit;
	int bat_limit;
	int max_cur;
	int bat_vol = hw_battery_get_series_num() * BAT_RATED_VOLT;
	struct direct_charge_device *l_di = direct_charge_get_di();

	if (!l_di)
		return 0;

	/* get max current by adapter */
	adp_limit = dc_get_adapter_max_current(bat_vol * l_di->dc_volt_ratio);
	adp_limit *= l_di->dc_volt_ratio;

	/* get max current by battery */
	if (l_di->product_max_pwr)
		bat_limit = l_di->product_max_pwr * POWER_UW_PER_MW / bat_vol;
	else
		bat_limit = direct_charge_get_battery_max_current();

	/* get max current by cable */
	cable_limit = dc_get_cable_max_current();
	/* avoid 55W/66W display issues on 5A/6A c2c cable */
	if (pd_dpm_get_ctc_cable_flag() && (cable_limit >= CTC_EMARK_CURR_5A * l_di->dc_volt_ratio))
		cable_limit = cable_limit * 11 / POWER_BASE_DEC; /* amplified 1.1 times. */
	if (l_di->cc_cable_detect_enable && cable_limit)
		max_cur = (bat_limit < cable_limit) ? bat_limit : cable_limit;
	else
		max_cur = bat_limit;

	if (adp_limit)
		max_cur = (max_cur < adp_limit) ? max_cur : adp_limit;

	l_di->max_pwr = max_cur * bat_vol;
	l_di->max_pwr /= POWER_UW_PER_MW;

	hwlog_info("l_adp=%d, l_cable=%d, l_bat=%d, m_cur=%d, m_pwr=%d\n",
		adp_limit, cable_limit, bat_limit, max_cur, l_di->max_pwr);

	return max_cur;
}

void dc_send_icon_uevent(void)
{
	struct direct_charge_device *l_di = direct_charge_get_di();
	int max_cur = dc_get_max_current();

	if (!l_di)
		return;

	if (direct_charge_is_priority_inversion()) {
		hwlog_info("icon type already send\n");
		return;
	}

	if (max_cur >= l_di->super_ico_current)
		wired_connect_send_icon_uevent(ICON_TYPE_SUPER);
	else
		wired_connect_send_icon_uevent(ICON_TYPE_QUICK);
}

void dc_send_max_power_uevent(void)
{
	struct direct_charge_device *l_di = direct_charge_get_di();

	if (!l_di || l_di->ui_max_pwr <= 0)
		return;

	if (l_di->max_pwr >= l_di->ui_max_pwr)
		power_ui_event_notify(POWER_UI_NE_MAX_POWER, &l_di->max_pwr);
}

void dc_send_soc_decimal_uevent(void)
{
	struct direct_charge_device *l_di = direct_charge_get_di();

	if (!l_di)
		return;

	power_event_bnc_notify(POWER_BNT_SOC_DECIMAL, POWER_NE_SOC_DECIMAL_DC, &l_di->max_pwr);
}

static bool dc_check_cable_type_send_done(void)
{
	struct direct_charge_device *lvc_di = NULL;
	struct direct_charge_device *sc_di = NULL;
	struct direct_charge_device *sc4_di = NULL;

	lvc_get_di(&lvc_di);
	sc_get_di(&sc_di);
	sc4_get_di(&sc4_di);

	if ((lvc_di && lvc_di->cable_type_send_flag) ||
		(sc_di && sc_di->cable_type_send_flag) ||
		(sc4_di && sc4_di->cable_type_send_flag))
		return true;

	return false;
}

void dc_send_cable_type_uevent(void)
{
	struct direct_charge_device *l_di = direct_charge_get_di();

	if (!l_di || l_di->is_send_cable_type == 0)
		return;

	if (dc_check_cable_type_send_done())
		return;

	power_ui_event_notify(POWER_UI_NE_CABLE_TYPE, &l_di->cable_type);
	l_di->cable_type_send_flag = true;
}
