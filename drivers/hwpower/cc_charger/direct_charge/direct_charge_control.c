// SPDX-License-Identifier: GPL-2.0
/*
 * direct_charge_control.c
 *
 * control module for direct charge
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

#include <huawei_platform/power/direct_charger/direct_charger.h>
#include <chipset_common/hwpower/common_module/power_delay.h>

#define HWLOG_TAG direct_charge_control
HWLOG_REGIST();

void dc_preparation_before_switch_to_singlepath(int working_mode, int ratio, int vdelt)
{
	int ibus = 0;
	int vbat = 0;
	int vadp = 0;
	int ibat_th = 0;
	int retry = 30; /* 30 : max retry times */

	if (!ratio)
		return;

	dcm_get_ic_max_ibat(working_mode, CHARGE_MULTI_IC, &ibat_th);
	if (!ibat_th)
		ibat_th = DC_SINGLEIC_CURRENT_LIMIT;

	direct_charge_get_device_ibus(&ibus);
	direct_charge_get_bat_sys_voltage(&vbat);
	if (ibus > ibat_th / ratio) {
		if (dc_get_adapter_voltage(&vadp))
			return;

		do {
			vadp = vadp - 200; /* voltage decreases by 200mv each time */
			dc_set_adapter_voltage(vadp);
			power_usleep(DT_USLEEP_5MS);
			direct_charge_get_device_ibus(&ibus);
			hwlog_info("[%d] set vadp=%d, ibus=%d\n", retry, vadp, ibus);
			retry--;
		} while ((ibus >= ibat_th / ratio) && (retry != 0) && (vadp > (vbat * ratio + vdelt)));
	}
}

void dc_preparation_before_stop(void)
{
	int vadp = 0;
	struct direct_charge_device *l_di = direct_charge_get_di();

	if (!l_di)
		return;

	/*
	 * fix a sc adapter hardware issue:
	 * adapter has not output voltage when direct charger charge done
	 * we will set the adapter voltage to 5500mv
	 * when adapter voltage up to 7500mv on charge done stage
	 */
	if (l_di->reset_adap_volt_enabled && (l_di->dc_stage == DC_STAGE_CHARGE_DONE) &&
		(l_di->adaptor_vset > 7500)) {
		l_di->adaptor_vset = 5500;
		dc_set_adapter_voltage(l_di->adaptor_vset);
		power_msleep(DT_MSLEEP_200MS, 0, NULL); /* delay 200ms at least */
		dc_get_adapter_voltage(&vadp);
		hwlog_info("set adaptor_vset=%d when charge done, vadp=%d\n", l_di->adaptor_vset, vadp);
		return;
	}

	if (l_di->cur_mode == CHARGE_MULTI_IC)
		dc_preparation_before_switch_to_singlepath(l_di->working_mode,
			l_di->dc_volt_ratio, l_di->init_delt_vset);
}
