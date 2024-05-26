// SPDX-License-Identifier: GPL-2.0
/*
 * direct_charge_device_id.c
 *
 * device id for direct charge
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

#include <linux/module.h>
#include <chipset_common/hwpower/direct_charge/direct_charge_device_id.h>
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG direct_charge_devid
HWLOG_REGIST();

#define DC_DEVICE_NAME_LEN_MAX          32

struct dc_device_name_info {
	int id;
	char device_mode_name[DC_DEVICE_NAME_LEN_MAX];
	char device_name[DC_DEVICE_NAME_LEN_MAX];
};

static const struct dc_device_name_info g_dc_device_name[DC_DEVICE_ID_END] = {
	{ LOADSWITCH_RT9748, "lvc_rt9748", "rt9748" },
	{ LOADSWITCH_BQ25870, "lvc_bq25870", "bq25870" },
	{ LOADSWITCH_FAN54161, "lvc_fan54161", "fan54161" },
	{ LOADSWITCH_PCA9498, "lvc_pca9498", "pca9498" },
	{ LOADSWITCH_SCHARGERV600, "lvc_hi6526", "hi6526" },
	{ LOADSWITCH_FPF2283, "lvc_fpf2283", "fpf2283" },
	{ SWITCHCAP_BQ25970, "sc_bq25970", "bq25970" },
	{ SWITCHCAP_SCHARGERV600, "sc_hi6526", "hi6526" },
	{ SWITCHCAP_LTC7820, "sc_ltc7820", "ltc7820" },
	{ SWITCHCAP_MULTI_SC, "sc_multi_hl1506", "multi_hl1506" },
	{ SWITCHCAP_RT9759, "sc_rt9759", "rt9759" },
	{ SWITCHCAP_SM5450, "sc_sm5450", "sm5450" },
	{ SWITCHCAP_SC8551, "sc_sc8551", "sc8551" },
	{ SWITCHCAP_SC8551A, "sc_sc8551A", "sc8551A" },
	{ SWITCHCAP_HL1530, "sc_hl1530", "hl1530" },
	{ SWITCHCAP_HL7130, "sc_hl7130", "hl7130" },
	{ SWITCHCAP_SYH69637, "sc_syh69637", "syh69637" },
	{ SWITCHCAP_SYH69636, "sc_syh69636", "syh69636" },
	{ SWITCHCAP_SC8545, "sc_sc8545", "sc8545" },
	{ SWITCHCAP_SC8546, "sc_sc8546", "sc8546" },
	{ SWITCHCAP_CPS2021, "sc_cps2021", "cps2021" },
	{ SWITCHCAP_HL7139, "sc_hl7139", "hl7139" },
	{ SWITCHCAP_NU2105, "sc_nu2105", "nu2105" },
	{ SWITCHCAP_AW32280, "sc_aw32280", "aw32280" },
	{ SWITCHCAP_SC8562, "sc_sc8562", "sc8562" },
	{ SWITCHCAP_SGM41600,"sc_sgm41600","sgm41600" },
};

const char *dc_get_device_name_without_mode(int id)
{
	int i;

	for (i = DC_DEVICE_ID_BEGIN; i < DC_DEVICE_ID_END; i++) {
		if (id == g_dc_device_name[i].id)
			return g_dc_device_name[i].device_name;
	}

	hwlog_err("invalid id=%d\n", id);
	return "invalid";
}

const char *dc_get_device_name(int id)
{
	int i;

	for (i = DC_DEVICE_ID_BEGIN; i < DC_DEVICE_ID_END; i++) {
		if (id == g_dc_device_name[i].id)
			return g_dc_device_name[i].device_mode_name;
	}

	hwlog_err("invalid id=%d\n", id);
	return "invalid";
}

int dc_get_device_id(const char *name)
{
	int i;

	for (i = DC_DEVICE_ID_BEGIN; i < DC_DEVICE_ID_END; i++) {
		if (!strcmp(g_dc_device_name[i].device_mode_name, name))
			return g_dc_device_name[i].id;
	}

	return DC_DEVICE_ID_END;
}
