// SPDX-License-Identifier: GPL-2.0
/*
 * direct_charge_ic_interface.c
 *
 * common interface, variables, definition etc for direct charge module
 *
 * Copyright (c) 2020-2021 Huawei Technologies Co., Ltd.
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

#include <chipset_common/hwpower/common_module/power_calibration.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/direct_charge/direct_charge_ic_interface.h>
#include <chipset_common/hwpower/direct_charge/direct_charge_ic.h>

#define HWLOG_TAG direct_charge_ic_interface
HWLOG_REGIST();

int dc_init_ic(int mode, unsigned int index)
{
	struct dc_ic_ops *temp_ops = dc_ic_get_ic_ops(mode, index);

	hwlog_info("init ic index is %u\n", index);

	if (!temp_ops || !temp_ops->ic_init)
		return -EPERM;
	return temp_ops->ic_init(temp_ops->dev_data);
}

int dc_exit_ic(int mode, unsigned int index)
{
	struct dc_ic_ops *temp_ops = dc_ic_get_ic_ops(mode, index);

	hwlog_info("exit ic index is %u\n", index);

	if (!temp_ops || !temp_ops->ic_exit)
		return -EPERM;
	return temp_ops->ic_exit(temp_ops->dev_data);
}

int dc_enable_ic(int mode, unsigned int index, int enable)
{
	struct dc_ic_ops *temp_ops = dc_ic_get_ic_ops(mode, index);

	hwlog_info("enable ic index is %u, enable %d\n", index, enable);

	if (!temp_ops || !temp_ops->ic_enable)
		return -EPERM;
	return temp_ops->ic_enable(enable, temp_ops->dev_data);
}

int dc_enable_irq(int mode, unsigned int index, int enable)
{
	struct dc_ic_ops *temp_ops = dc_ic_get_ic_ops(mode, index);

	hwlog_info("enable irq index is %u, enable %d\n", index, enable);

	if (!temp_ops || !temp_ops->irq_enable)
		return -EPERM;
	return temp_ops->irq_enable(enable, temp_ops->dev_data);
}

int dc_enable_ic_adc(int mode, unsigned int index, int enable)
{
	struct dc_ic_ops *temp_ops = dc_ic_get_ic_ops(mode, index);

	if (!temp_ops || !temp_ops->ic_adc_enable)
		return -EPERM;
	return temp_ops->ic_adc_enable(enable, temp_ops->dev_data);
}

int dc_discharge_ic(int mode, unsigned int index, int enable)
{
	struct dc_ic_ops *temp_ops = dc_ic_get_ic_ops(mode, index);

	if (!temp_ops || !temp_ops->ic_discharge)
		return -EPERM;
	return temp_ops->ic_discharge(enable, temp_ops->dev_data);
}

int dc_is_ic_close(int mode, unsigned int index)
{
	struct dc_ic_ops *temp_ops = dc_ic_get_ic_ops(mode, index);

	if (!temp_ops || !temp_ops->is_ic_close)
		return -EPERM;
	return temp_ops->is_ic_close(temp_ops->dev_data);
}

bool dc_is_ic_support_prepare(int mode, unsigned int index)
{
	struct dc_ic_ops *temp_ops = dc_ic_get_ic_ops(mode, index);

	if (temp_ops && temp_ops->ic_enable_prepare)
		return true;
	return false;
}

int dc_prepare_enable_ic(int mode, unsigned int index)
{
	struct dc_ic_ops *temp_ops = dc_ic_get_ic_ops(mode, index);

	if (!temp_ops || !temp_ops->ic_enable_prepare)
		return -EPERM;
	return temp_ops->ic_enable_prepare(temp_ops->dev_data);
}

int dc_config_ic_watchdog(int mode, unsigned int index, int time)
{
	struct dc_ic_ops *temp_ops = dc_ic_get_ic_ops(mode, index);

	if (!temp_ops || !temp_ops->config_ic_watchdog)
		return -EPERM;
	return temp_ops->config_ic_watchdog(time, temp_ops->dev_data);
}

int dc_kick_ic_watchdog(int mode, unsigned int index)
{
	struct dc_ic_ops *temp_ops = dc_ic_get_ic_ops(mode, index);

	if (!temp_ops || !temp_ops->kick_ic_watchdog)
		return -EPERM;
	return temp_ops->kick_ic_watchdog(temp_ops->dev_data);
}

int dc_get_ic_id(int mode, unsigned int index)
{
	struct dc_ic_ops *temp_ops = dc_ic_get_ic_ops(mode, index);

	if (temp_ops && temp_ops->get_ic_id)
		return temp_ops->get_ic_id(temp_ops->dev_data);
	return -EPERM;
}

int dc_get_ic_status(int mode, unsigned int index)
{
	struct dc_ic_ops *temp_ops = dc_ic_get_ic_ops(mode, index);

	if (!temp_ops)
		return -EPERM;
	if (!temp_ops->get_ic_status)
		return 0;
	return temp_ops->get_ic_status(temp_ops->dev_data);
}

int dc_set_ic_buck_enable(int mode, unsigned int index, int enable)
{
	struct dc_ic_ops *temp_ops = dc_ic_get_ic_ops(mode, index);

	if (!temp_ops || !temp_ops->set_ic_buck_enable)
		return -EPERM;
	return temp_ops->set_ic_buck_enable(enable, temp_ops->dev_data);
}

int dc_reset_and_init_ic_reg(int mode, unsigned int index)
{
	struct dc_ic_ops *temp_ops = dc_ic_get_ic_ops(mode, index);

	if (!temp_ops || !temp_ops->ic_reg_reset_and_init)
		return -EPERM;
	return temp_ops->ic_reg_reset_and_init(temp_ops->dev_data);
}

int dc_get_ic_freq(int mode, unsigned int index)
{
	struct dc_ic_ops *temp_ops = dc_ic_get_ic_ops(mode, index);

	if (!temp_ops || !temp_ops->get_ic_freq)
		return -EPERM;
	return temp_ops->get_ic_freq(temp_ops->dev_data);
}

int dc_set_ic_freq(int mode, unsigned int index, int freq)
{
	struct dc_ic_ops *temp_ops = dc_ic_get_ic_ops(mode, index);

	if (!temp_ops || !temp_ops->set_ic_freq)
		return 0;
	return temp_ops->set_ic_freq(freq, temp_ops->dev_data);
}

const char *dc_get_ic_name(int mode, unsigned int index)
{
	struct dc_ic_ops *temp_ops = dc_ic_get_ic_ops(mode, index);

	if (!temp_ops || !temp_ops->dev_name)
		return "invalid ic";
	return temp_ops->dev_name;
}

int dc_init_batinfo(int mode, unsigned int index)
{
	struct dc_batinfo_ops *temp_ops = dc_ic_get_battinfo_ops(index);

	if (!temp_ops || !temp_ops->init)
		return -EPERM;
	return temp_ops->init(temp_ops->dev_data);
}

int dc_exit_batinfo(int mode, unsigned int index)
{
	struct dc_batinfo_ops *temp_ops = dc_ic_get_battinfo_ops(index);

	if (!temp_ops || !temp_ops->exit)
		return -EPERM;
	return temp_ops->exit(temp_ops->dev_data);
}

int dc_get_ic_vbtb(unsigned int index)
{
	struct dc_batinfo_ops *temp_ops = dc_ic_get_battinfo_ops(index);

	if (!temp_ops || !temp_ops->get_bat_btb_voltage)
		return -EPERM;
	return temp_ops->get_bat_btb_voltage(temp_ops->dev_data);
}

int dc_get_ic_vpack(unsigned int index)
{
	struct dc_batinfo_ops *temp_ops = dc_ic_get_battinfo_ops(index);

	if (!temp_ops || !temp_ops->get_bat_package_voltage)
		return -EPERM;
	return temp_ops->get_bat_package_voltage(temp_ops->dev_data);
}

int dc_get_ic_vbus(unsigned int index, int *vbus)
{
	struct dc_batinfo_ops *temp_ops = dc_ic_get_battinfo_ops(index);

	if (!vbus)
		return -EPERM;

	if (!temp_ops || !temp_ops->get_vbus_voltage)
		return -EPERM;
	return temp_ops->get_vbus_voltage(vbus, temp_ops->dev_data);
}

int dc_get_ic_ibat(unsigned int index, int *ibat)
{
	s64 temp;
	int c_offset_a = 0;
	int c_offset_b = 0;
	struct dc_batinfo_ops *temp_ops = dc_ic_get_battinfo_ops(index);

	if (!ibat)
		return -EPERM;

	if (!temp_ops || !temp_ops->get_bat_current ||
		temp_ops->get_bat_current(ibat, temp_ops->dev_data))
		return -EPERM;

	hwlog_info("ic[%u] cali_b: bat_cur=%d\n", index, *ibat);
	switch (index) {
	case IC_ONE:
		/* 0:CUR_A_OFFSET 1:CUR_B_OFFSET */
		power_cali_common_get_data("dc", 0, &c_offset_a);
		power_cali_common_get_data("dc", 1, &c_offset_b);
		break;
	case IC_TWO:
		/* 2:AUX_CUR_A_OFFSET 3:AUX_CUR_B_OFFSET */
		power_cali_common_get_data("dc", 2, &c_offset_a);
		power_cali_common_get_data("dc", 3, &c_offset_b);
		break;
	default:
		break;
	}

	if (c_offset_a) {
		temp = (*ibat * (s64)c_offset_a) + c_offset_b;
		*ibat = (int)div_s64(temp, 1000000); /* base 1000000 */
		hwlog_info("cali_a: bat_cur=%d,c_offset_a=%d,c_offset_b=%d\n",
			*ibat, c_offset_a, c_offset_b);
	}
	return 0;
}

int dc_get_ic_ibus(unsigned int index, int *ibus)
{
	struct dc_batinfo_ops *temp_ops = dc_ic_get_battinfo_ops(index);

	if (!ibus)
		return -EPERM;

	if (!temp_ops || !temp_ops->get_ic_ibus)
		return -EPERM;
	return temp_ops->get_ic_ibus(ibus, temp_ops->dev_data);
}

int dc_get_ic_temp(unsigned int index, int *temp)
{
	struct dc_batinfo_ops *temp_ops = dc_ic_get_battinfo_ops(index);

	if (!temp)
		return -EPERM;

	if (!temp_ops || !temp_ops->get_ic_temp)
		return 0;
	return temp_ops->get_ic_temp(temp, temp_ops->dev_data);
}

int dc_get_ic_vusb(unsigned int index, int *vusb)
{
	struct dc_batinfo_ops *temp_ops = dc_ic_get_battinfo_ops(index);

	if (!vusb)
		return -EPERM;

	if (!temp_ops || !temp_ops->get_ic_vusb)
		return -EPERM;
	return temp_ops->get_ic_vusb(vusb, temp_ops->dev_data);
}

int dc_get_ic_vout(unsigned int index, int *vout)
{
	struct dc_batinfo_ops *temp_ops = dc_ic_get_battinfo_ops(index);

	if (!vout)
		return -EPERM;

	if (!temp_ops || !temp_ops->get_ic_vout)
		return -EPERM;
	return temp_ops->get_ic_vout(vout, temp_ops->dev_data);
}

int dc_set_ic_threshold_enable(int mode, unsigned int index, int enable)
{
	struct dc_ic_ops *temp_ops = dc_ic_get_ic_ops(mode, index);

	if (!temp_ops)
		return -EPERM;

	if (!temp_ops->set_ic_thld)
		return 0;

	return temp_ops->set_ic_thld(enable, temp_ops->dev_data);
}