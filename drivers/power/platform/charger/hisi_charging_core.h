/*
 * hisi_charging_core.c
 *
 * hisi charger driver
 *
 * Copyright (c) 2012-2021 Huawei Technologies Co., Ltd.
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

#ifndef _HISI_CHARGING_CORE_
#define _HISI_CHARGING_CORE_

#include <linux/device.h> /* for struct charge_core_info */
#include <platform_include/basicplatform/linux/power/platform/battery_data.h> /* for struct charge_core_info */
#include "hisi_charger.h" /* for struct charge_core_info */

/* ************************marco define area************************** */
#define VDPM_PARA_LEVEL (4)
#define VDPM_CBAT_MIN (-32767)
#define VDPM_CBAT_MAX (32767)
#define VDPM_VOLT_MIN (3880)
#define VDPM_VOLT_MAX (5080)
#define VDPM_CAP_DETA (100)

#define INDUCTANCE_PARA_LEVEL (4)
#define INDUCTANCE_CBAT_MIN (-32767)
#define INDUCTANCE_CBAT_MAX (32767)
#define INDUCTANCE_IIN_MIN (0)
#define INDUCTANCE_IIN_MAX (5000)
#define INDUCTANCE_CAP_DETA (5)
/* ************************struct define area************************** */
enum vdpm_para_info {
	VDPM_PARA_CAP_MIN = 0,
	VDPM_PARA_CAP_MAX,
	VDPM_PARA_DPM,
	VDPM_PARA_CAP_BACK,
	VDPM_PARA_TOTAL,
};
enum inductance_para_info {
	INDUCTANCE_PARA_CAP_MIN = 0,
	INDUCTANCE_PARA_CAP_MAX,
	INDUCTANCE_PARA_IIN,
	INDUCTANCE_PARA_CAP_BACK,
	INDUCTANCE_PARA_TOTAL,
};

struct charge_temp_data {
	int temp_min;
	int temp_max;
	int iin_temp;
	int ichg_temp;
	int vterm_temp;
	int temp_back;
};

struct charge_volt_data {
	int vbat_min;
	int vbat_max;
	int iin_volt;
	int ichg_volt;
	int volt_back;
};

struct charge_vdpm_data {
	int cap_min;
	int cap_max;
	int vin_dpm;
	int cap_back;
};

struct charge_segment_data {
	int vbat_min;
	int vbat_max;
	int ichg_segment;
	int vterm_segment;
	int volt_back;
};

struct charge_inductance_data {
	int cap_min;
	int cap_max;
	int iin_inductance;
	int cap_back;
};

struct charge_core_info {
	struct device *dev;
	struct charge_temp_data temp_para[TEMP_PARA_LEVEL];
	struct charge_volt_data volt_para[VOLT_PARA_LEVEL];
	struct charge_vdpm_data vdpm_para[VDPM_PARA_LEVEL];
	struct charge_segment_data segment_para[SEGMENT_PARA_LEVEL];
	struct charge_inductance_data inductance_para[VDPM_PARA_LEVEL];
	struct charge_core_data data;
};

/* ***************variable and function declarationn area***************** */
struct charge_core_data *charge_core_get_params(void);
extern enum fcp_check_stage_type fcp_get_stage_status(void);

#endif
