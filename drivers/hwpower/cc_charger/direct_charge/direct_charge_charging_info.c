// SPDX-License-Identifier: GPL-2.0
/*
 * direct_charge_charging_info.c
 *
 * charging info for direct charge module
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

#include <chipset_common/hwpower/battery/battery_temp.h>
#include <chipset_common/hwpower/common_module/power_supply_interface.h>
#include <chipset_common/hwpower/direct_charge/direct_charge_ic_manager.h>
#include <chipset_common/hwpower/wireless_charge/wireless_tx_ic_intf.h>
#include <huawei_platform/power/wireless/wireless_transmitter.h>
#include <huawei_platform/hwpower/common_module/power_platform.h>

#define HWLOG_TAG direct_charge_charging_info
HWLOG_REGIST();

static const char *coul_psy[] = { "battery_gauge", "battery_gauge_aux" };

static int dc_check_coul_status(void)
{
	int i;
	int coul_num = 0;
	struct power_supply *psy = NULL;

	for (i = 0; i < COUL_IC_NUM; i++) {
		psy = NULL;
		if (power_supply_check_psy_available(coul_psy[i], &psy))
			coul_num |= BIT(i);
	}
	return coul_num;
}

void dc_update_charging_info(int mode, struct dc_charge_info *info, int *vbat_comp)
{
	int i;
	int ret;
	int coul_ibat_max = 0;
	int coul_vbat_max = 0;
	int ibat[CHARGE_IC_TYPE_MAX] = { 0 };
	int ibus[CHARGE_IC_TYPE_MAX] = { 0 };
	int coul_ibat[COUL_IC_NUM] = { 0 };
	int coul_vbat[COUL_IC_NUM] = { 0 };

	if (!info || !vbat_comp)
		return;

	coul_ibat_max = -power_platform_get_battery_current();
	if (coul_ibat_max < info->coul_ibat_max)
		return;

	for (i = 0; i < info->channel_num; i++) {
		ret = dcm_get_ic_ibat(mode, BIT(i), &ibat[i]);
		ret += dcm_get_ic_ibus(mode, BIT(i), &ibus[i]);
		if (ret || (ibus[i] < info->ibus[i]) || (ibat[i] < info->ibat[i]))
			return;
	}

	info->coul_check_flag = dc_check_coul_status();
	ret = power_supply_get_int_property_value("battery",
		POWER_SUPPLY_PROP_VOLTAGE_NOW, &coul_vbat_max);
	for (i = 0; i < COUL_IC_NUM; i++) {
		if (!(info->coul_check_flag & BIT(i)))
			continue;
		ret += power_supply_get_int_property_value(coul_psy[i],
			POWER_SUPPLY_PROP_CURRENT_NOW, &coul_ibat[i]);
		ret += power_supply_get_int_property_value(coul_psy[i],
			POWER_SUPPLY_PROP_VOLTAGE_NOW, &coul_vbat[i]);
		if (ret || (coul_ibat[i] < info->coul_ibat[i]))
			return;
	}

	direct_charge_get_bat_current(&info->ibat_max);
	for (i = 0; i < info->channel_num; i++) {
		info->ibat[i] = ibat[i];
		info->ibus[i] = ibus[i];
		info->ic_name[i] = dcm_get_ic_name(mode, BIT(i));
		info->vbat[i] = dcm_get_ic_vbtb_with_comp(mode, BIT(i), vbat_comp);
		dcm_get_ic_vout(mode, BIT(i), &info->vout[i]);
		bat_temp_get_temperature(i, &info->tbat[i]);
	}

	info->coul_ibat_max = coul_ibat_max;
	info->coul_vbat_max = coul_vbat_max;
	for (i = 0; i < COUL_IC_NUM; i++) {
		if (!(info->coul_check_flag & BIT(i)))
			continue;
		info->coul_vbat[i] = coul_vbat[i];
		info->coul_ibat[i] = coul_ibat[i];
	}
}

void dc_show_realtime_charging_info(int mode, unsigned int path)
{
	int vbat;
	int vadp = 0;
	int iadp = 0;
	int vbus = 0;
	int ibus = 0;
	int ibat = 0;

	dc_get_adapter_voltage(&vadp);
	dc_get_adapter_current(&iadp);
	dcm_get_ic_vbus(mode, path, &vbus);
	dcm_get_ic_ibus(mode, path, &ibus);
	vbat = dcm_get_ic_vbtb(mode, path);
	dcm_get_total_ibat(mode, path, &ibat);

	hwlog_info("Vadp=%d, Iadp=%d, Vbus=%d, Ibus=%d, Vbat=%d, Ibat=%d\n",
		vadp, iadp, vbus, ibus, vbat, ibat);
}

int dc_get_gain_ibus(void)
{
	int gain_ibus = 0;
	u16 itx = 0;
	int iuem;
	struct direct_charge_device *di = direct_charge_get_di();

	if (!di)
		goto exit;

	if (di->need_check_tx_iin && wireless_tx_get_tx_open_flag()) {
		(void)wltx_ic_get_iin(WLTRX_IC_MAIN, &itx);
		hwlog_info("Itx=%u\n", itx);
		gain_ibus += itx;
	}

	iuem = (int)power_platform_get_uem_leak_current();
	hwlog_info("Iuem=%u\n", iuem);
	gain_ibus += iuem;
exit:
	return gain_ibus;
}

void dc_set_stop_charging_flag(bool flag)
{
	struct direct_charge_device *l_di = direct_charge_get_di();

	if (!l_di)
		return;

	hwlog_info("set stop_charging_flag_error %s\n", flag ? "true" : "false");
	l_di->stop_charging_flag_error = flag;
}

int dc_get_stop_charging_flag(void)
{
	struct direct_charge_device *l_di = direct_charge_get_di();

	if (!l_di)
		return -1;

	if (l_di->stop_charging_flag_error) {
		hwlog_info("stop_charging_flag_error is true\n");
		return -1;
	}

	return 0;
}
