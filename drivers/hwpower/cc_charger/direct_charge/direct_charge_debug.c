// SPDX-License-Identifier: GPL-2.0
/*
 * direct_charge_debug.c
 *
 * debug for direct charge
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

#include <linux/types.h>
#include <chipset_common/hwpower/common_module/power_common_macro.h>
#include <chipset_common/hwpower/common_module/power_temp.h>
#include <chipset_common/hwpower/common_module/power_debug.h>
#include <huawei_platform/power/direct_charger/direct_charger.h>
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG direct_charge_debug
HWLOG_REGIST();

#define OUT_BUF_SIZE    256
#define TMP_BUF_SIZE    32
#define DC_LVC_MOS_MAX  3

static ssize_t dc_dbg_lvc_mos_para_store(void *dev_data, const char *buf, size_t size)
{
	struct direct_charge_device *di = (struct direct_charge_device *)dev_data;

	if (!di)
		return -EINVAL;

	if (sscanf(buf, "%d %d %d", &di->lvc_mos_dts.is_support, &di->lvc_mos_dts.ibat_th,
		&di->lvc_mos_dts.curr_offset) != DC_LVC_MOS_MAX)
		return -EINVAL;

	hwlog_info("[lvc_mos_para] is_support:%d ibat_th:%d curr_offset:%d\n",
		di->lvc_mos_dts.is_support, di->lvc_mos_dts.ibat_th, di->lvc_mos_dts.curr_offset);

	return size;
}

static ssize_t dc_dbg_lvc_mos_para_show(void *dev_data, char *buf, size_t size)
{
	struct direct_charge_device *di = (struct direct_charge_device *)dev_data;

	if (!di)
		return -EINVAL;

	return scnprintf(buf, size, "[lvc_mos_para] is_support:%d ibat_th:%d curr_offset:%d\n",
		di->lvc_mos_dts.is_support, di->lvc_mos_dts.ibat_th, di->lvc_mos_dts.curr_offset);
}

static ssize_t dc_dbg_volt_para_test_flag_store(void *dev_data, const char *buf, size_t size)
{
	int val = 0;
	struct direct_charge_device *di = (struct direct_charge_device *)dev_data;

	if (!di)
		return -EINVAL;

	if (kstrtoint(buf, 0, &val)) {
		hwlog_err("unable to parse input:%s\n", buf);
		return -EINVAL;
	}

	di->volt_para_test_flag = val;
	return size;
}

static ssize_t dc_dbg_volt_para_test_flag_show(void *dev_data, char *buf, size_t size)
{
	struct direct_charge_device *di = (struct direct_charge_device *)dev_data;

	if (!di)
		return -EINVAL;

	return scnprintf(buf, size, "volt_para_test_flag is %d\n",
		di->volt_para_test_flag);
}

static ssize_t dc_dbg_volt_para_store(void *dev_data, const char *buf, size_t size)
{
	int i;
	int paras = DC_VOLT_LEVEL * DC_PARA_VOLT_TOTAL;
	struct direct_charge_volt_para para[DC_VOLT_LEVEL];
	char out_buf[OUT_BUF_SIZE] = { 0 };
	char tmp_buf[TMP_BUF_SIZE] = { 0 };
	struct direct_charge_device *di = (struct direct_charge_device *)dev_data;

	if (!di || !di->volt_para_test_flag)
		return -EINVAL;

	/* [0] [1] [2] [3] [4] [5] [6] [7]: eight group parameters */
	if (sscanf(buf, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
		&para[0].vol_th, &para[0].cur_th_high, &para[0].cur_th_low,
		&para[1].vol_th, &para[1].cur_th_high, &para[1].cur_th_low,
		&para[2].vol_th, &para[2].cur_th_high, &para[2].cur_th_low,
		&para[3].vol_th, &para[3].cur_th_high, &para[3].cur_th_low,
		&para[4].vol_th, &para[4].cur_th_high, &para[4].cur_th_low,
		&para[5].vol_th, &para[5].cur_th_high, &para[5].cur_th_low,
		&para[6].vol_th, &para[6].cur_th_high, &para[6].cur_th_low,
		&para[7].vol_th, &para[7].cur_th_high, &para[7].cur_th_low) != paras) {
		di->volt_para_test_flag = 0;
		return -EINVAL;
	}

	for (i = 0; i < DC_VOLT_LEVEL; i++) {
		if (para[i].vol_th == 0) {
			di->stage_size = i;
			break;
		}
		di->volt_para[i].vol_th = para[i].vol_th;
		di->volt_para[i].cur_th_high = para[i].cur_th_high;
		di->volt_para[i].cur_th_low = para[i].cur_th_low;
		memset(tmp_buf, 0, TMP_BUF_SIZE);
		scnprintf(tmp_buf, TMP_BUF_SIZE, "%4d %5d %5d\n",
			di->volt_para[i].vol_th, di->volt_para[i].cur_th_high, di->volt_para[i].cur_th_low);
		strncat(out_buf, tmp_buf, strlen(tmp_buf));
	}

	hwlog_info("dc_volt_para:\n%s\n", out_buf);
	return size;
}

static ssize_t dc_dbg_volt_para_show(void *dev_data, char *buf, size_t size)
{
	struct direct_charge_device *di = (struct direct_charge_device *)dev_data;
	char out_buf[OUT_BUF_SIZE] = { 0 };
	char tmp_buf[TMP_BUF_SIZE] = { 0 };
	int i;

	if (!di || !di->volt_para_test_flag)
		return 0;

	for (i = 0; i < DC_VOLT_LEVEL; i++) {
		memset(tmp_buf, 0, TMP_BUF_SIZE);
		scnprintf(tmp_buf, TMP_BUF_SIZE, "%4d %5d %5d\n",
			di->volt_para[i].vol_th, di->volt_para[i].cur_th_high, di->volt_para[i].cur_th_low);
		strncat(out_buf, tmp_buf, strlen(tmp_buf));
	}

	return scnprintf(buf, size, "dc_volt_para:\n%s\n", out_buf);
}

static ssize_t dc_dbg_vstep_para_store(void *dev_data,
	const char *buf, size_t size)
{
	struct direct_charge_device *di = dev_data;
	int paras = DC_VSTEP_PARA_LEVEL * DC_VSTEP_INFO_MAX;
	char out_buf[OUT_BUF_SIZE] = { 0 };
	char tmp_buf[TMP_BUF_SIZE] = { 0 };
	int i;

	if (!di)
		return -EINVAL;

	/* [0] [1] [2] [3]: four group parameters */
	if (sscanf(buf, "%d %d %d %d %d %d %d %d",
		&di->vstep_para[0].curr_gap, &di->vstep_para[0].vstep,
		&di->vstep_para[1].curr_gap, &di->vstep_para[1].vstep,
		&di->vstep_para[2].curr_gap, &di->vstep_para[2].vstep,
		&di->vstep_para[3].curr_gap, &di->vstep_para[3].vstep) != paras)
		return -EINVAL;

	for (i = 0; i < DC_VSTEP_PARA_LEVEL; i++) {
		memset(tmp_buf, 0, TMP_BUF_SIZE);
		scnprintf(tmp_buf, TMP_BUF_SIZE, "%d   %d\n",
			di->vstep_para[i].curr_gap, di->vstep_para[i].vstep);
		strncat(out_buf, tmp_buf, strlen(tmp_buf));
	}

	hwlog_info("dc_vstep_para:\n%s\n", out_buf);
	return size;
}

static ssize_t dc_dbg_vstep_para_show(void *dev_data, char *buf, size_t size)
{
	struct direct_charge_device *di = dev_data;
	char out_buf[OUT_BUF_SIZE] = { 0 };
	char tmp_buf[TMP_BUF_SIZE] = { 0 };
	int i;

	if (!di)
		return 0;

	for (i = 0; i < DC_VSTEP_PARA_LEVEL; i++) {
		memset(tmp_buf, 0, TMP_BUF_SIZE);
		scnprintf(tmp_buf, TMP_BUF_SIZE, "%d   %d\n",
			di->vstep_para[i].curr_gap, di->vstep_para[i].vstep);
		strncat(out_buf, tmp_buf, strlen(tmp_buf));
	}

	return scnprintf(buf, size, "dc_vstep_para:\n%s\n", out_buf);
}

static ssize_t dc_dbg_time_para_store(void *dev_data,
	const char *buf, size_t size)
{
	struct direct_charge_device *di = dev_data;
	int paras = DC_TIME_PARA_LEVEL * DC_TIME_INFO_MAX;
	char out_buf[OUT_BUF_SIZE] = { 0 };
	char tmp_buf[TMP_BUF_SIZE] = { 0 };
	int i;

	if (!di)
		return -EINVAL;

	/* [0] [1] [2] [3] [4]: five group parameters */
	if (sscanf(buf, "%d %d %d %d %d %d %d %d %d %d",
		&di->time_para[0].time_th, &di->time_para[0].ibat_max,
		&di->time_para[1].time_th, &di->time_para[1].ibat_max,
		&di->time_para[2].time_th, &di->time_para[2].ibat_max,
		&di->time_para[3].time_th, &di->time_para[3].ibat_max,
		&di->time_para[4].time_th, &di->time_para[4].ibat_max) != paras)
		return -EINVAL;

	for (i = 0; i < DC_TIME_PARA_LEVEL; i++) {
		memset(tmp_buf, 0, TMP_BUF_SIZE);
		scnprintf(tmp_buf, TMP_BUF_SIZE, "%d   %d\n",
			di->time_para[i].time_th, di->time_para[i].ibat_max);
		strncat(out_buf, tmp_buf, strlen(tmp_buf));
	}

	di->time_para_parse_ok = 1; /* set parse ok flag */
	hwlog_info("dc_time_para:\n%s\n", out_buf);
	return size;
}

static ssize_t dc_dbg_time_para_show(void *dev_data, char *buf, size_t size)
{
	struct direct_charge_device *di = dev_data;
	char out_buf[OUT_BUF_SIZE] = { 0 };
	char tmp_buf[TMP_BUF_SIZE] = { 0 };
	int i;

	if (!di)
		return 0;

	for (i = 0; i < DC_TIME_PARA_LEVEL; i++) {
		memset(tmp_buf, 0, TMP_BUF_SIZE);
		scnprintf(tmp_buf, TMP_BUF_SIZE, "%d   %d\n",
			di->time_para[i].time_th, di->time_para[i].ibat_max);
		strncat(out_buf, tmp_buf, strlen(tmp_buf));
	}

	return scnprintf(buf, size, "dc_time_para:\n%s\n", out_buf);
}

static ssize_t dc_dbg_total_ibus_show(void *dev_data, char *buf, size_t size)
{
	struct direct_charge_device *di = dev_data;
	int ibus = 0;

	if (!di)
		return -EINVAL;

	(void)dcm_get_ic_ibus(di->local_mode, CHARGE_MULTI_IC, &ibus);
	return scnprintf(buf, size, "%d\n", ibus);
}

static ssize_t dc_dbg_main_ibus_show(void *dev_data, char *buf, size_t size)
{
	struct direct_charge_device *di = dev_data;
	int ibus = 0;

	if (!di)
		return -EINVAL;

	(void)dcm_get_ic_ibus(di->local_mode, CHARGE_IC_MAIN, &ibus);
	return scnprintf(buf, size, "%d\n", ibus);
}

static ssize_t dc_dbg_aux_ibus_show(void *dev_data, char *buf, size_t size)
{
	struct direct_charge_device *di = dev_data;
	int ibus = 0;

	if (!di)
		return -EINVAL;

	(void)dcm_get_ic_ibus(di->local_mode, CHARGE_IC_AUX, &ibus);
	return scnprintf(buf, size, "%d\n", ibus);
}

static ssize_t dc_dbg_main_vbus_show(void *dev_data, char *buf, size_t size)
{
	struct direct_charge_device *di = dev_data;
	int vbus = 0;

	if (!di)
		return -EINVAL;

	(void)dcm_get_ic_vbus(di->local_mode, CHARGE_IC_MAIN, &vbus);
	return scnprintf(buf, size, "%d\n", vbus);
}

static ssize_t dc_dbg_aux_vbus_show(void *dev_data, char *buf, size_t size)
{
	struct direct_charge_device *di = dev_data;
	int vbus = 0;

	if (!di)
		return -EINVAL;

	(void)dcm_get_ic_vbus(di->local_mode, CHARGE_IC_AUX, &vbus);
	return scnprintf(buf, size, "%d\n", vbus);
}

static ssize_t dc_dbg_main_vout_show(void *dev_data, char *buf, size_t size)
{
	struct direct_charge_device *di = dev_data;
	int vout = 0;

	if (!di)
		return -EINVAL;

	(void)dcm_get_ic_vout(di->local_mode, CHARGE_IC_MAIN, &vout);
	return scnprintf(buf, size, "%d\n", vout);
}

static ssize_t dc_dbg_aux_vout_show(void *dev_data, char *buf, size_t size)
{
	struct direct_charge_device *di = dev_data;
	int vout = 0;

	if (!di)
		return -EINVAL;

	(void)dcm_get_ic_vout(di->local_mode, CHARGE_IC_AUX, &vout);
	return scnprintf(buf, size, "%d\n", vout);
}

static ssize_t dc_dbg_main_ibat_show(void *dev_data, char *buf, size_t size)
{
	struct direct_charge_device *di = dev_data;
	int ibat = 0;

	if (!di)
		return -EINVAL;

	/* DBC sc bat current calibration use, ibat need be calibrated */
	(void)dcm_get_ic_ibat(di->local_mode, CHARGE_IC_MAIN, &ibat);
	return scnprintf(buf, size, "%d\n", ibat);
}

static ssize_t dc_dbg_aux_ibat_show(void *dev_data, char *buf, size_t size)
{
	struct direct_charge_device *di = dev_data;
	int ibat = 0;

	if (!di)
		return -EINVAL;

	/* DBC sc bat current calibration use, ibat need be calibrated */
	(void)dcm_get_ic_ibat(di->local_mode, CHARGE_IC_AUX, &ibat);
	return scnprintf(buf, size, "%d\n", ibat);
}

static ssize_t dc_dbg_main_vbat_show(void *dev_data, char *buf, size_t size)
{
	struct direct_charge_device *di = dev_data;
	int vbat;

	if (!di)
		return -EINVAL;

	vbat = dcm_get_ic_vbtb(di->local_mode, CHARGE_IC_MAIN);
	return scnprintf(buf, size, "%d\n", vbat);
}

static ssize_t dc_dbg_aux_vbat_show(void *dev_data, char *buf, size_t size)
{
	struct direct_charge_device *di = dev_data;
	int vbat;

	if (!di)
		return -EINVAL;

	vbat = dcm_get_ic_vbtb(di->local_mode, CHARGE_IC_AUX);
	return scnprintf(buf, size, "%d\n", vbat);
}

static ssize_t dc_dbg_main_ic_temp_show(void *dev_data, char *buf, size_t size)
{
	struct direct_charge_device *di = dev_data;
	int ic_temp = 0;

	if (!di)
		return -EINVAL;

	(void)dcm_get_ic_temp(di->local_mode, CHARGE_IC_MAIN, &ic_temp);
	return scnprintf(buf, size, "%d\n", ic_temp);
}

static ssize_t dc_dbg_aux_ic_temp_show(void *dev_data, char *buf, size_t size)
{
	struct direct_charge_device *di = dev_data;
	int ic_temp = 0;

	if (!di)
		return -EINVAL;

	(void)dcm_get_ic_temp(di->local_mode, CHARGE_IC_AUX, &ic_temp);
	return scnprintf(buf, size, "%d\n", ic_temp);
}

static ssize_t dc_dbg_main_ic_id_show(void *dev_data, char *buf, size_t size)
{
	struct direct_charge_device *di = dev_data;
	int ic_id;

	if (!di)
		return -EINVAL;

	ic_id = dcm_get_ic_id(di->local_mode, CHARGE_IC_MAIN);
	return scnprintf(buf, size, "%d\n", ic_id);
}

static ssize_t dc_dbg_aux_ic_id_show(void *dev_data, char *buf, size_t size)
{
	struct direct_charge_device *di = dev_data;
	int ic_id;

	if (!di)
		return -EINVAL;

	ic_id = dcm_get_ic_id(di->local_mode, CHARGE_IC_AUX);
	return scnprintf(buf, size, "%d\n", ic_id);
}

static ssize_t dc_dbg_main_ic_name_show(void *dev_data, char *buf, size_t size)
{
	struct direct_charge_device *di = dev_data;
	const char *ic_name = NULL;

	if (!di)
		return -EINVAL;

	ic_name = dcm_get_ic_name(di->local_mode, CHARGE_IC_MAIN);
	return scnprintf(buf, PAGE_SIZE, "%s\n", ic_name);
}

static ssize_t dc_dbg_aux_ic_name_show(void *dev_data, char *buf, size_t size)
{
	struct direct_charge_device *di = dev_data;
	const char *ic_name = NULL;

	if (!di)
		return -EINVAL;

	ic_name = dcm_get_ic_name(di->local_mode, CHARGE_IC_AUX);
	return scnprintf(buf, PAGE_SIZE, "%s\n", ic_name);
}

static ssize_t dc_dbg_aux_tbat_show(void *dev_data, char *buf, size_t size)
{
	int aux_tbat = power_temp_get_raw_value(POWER_TEMP_BTB_NTC_AUX);

	return scnprintf(buf, PAGE_SIZE, "%d\n", aux_tbat);
}

static ssize_t dc_dbg_tadapt_show(void *dev_data, char *buf, size_t size)
{
	struct direct_charge_device *di = dev_data;

	if (!di)
		return -EINVAL;

	return scnprintf(buf, PAGE_SIZE, "%d\n", di->tadapt);
}

static ssize_t dc_dbg_vadapt_show(void *dev_data, char *buf, size_t size)
{
	struct direct_charge_device *di = dev_data;

	if (!di)
		return -EINVAL;

	return scnprintf(buf, PAGE_SIZE, "%d\n", di->vadapt);
}

static ssize_t dc_dbg_tusb_show(void *dev_data, char *buf, size_t size)
{
	int tusb = power_temp_get_average_value(POWER_TEMP_USB_PORT) / POWER_MC_PER_C;

	return scnprintf(buf, PAGE_SIZE, "%d\n", tusb);
}

void sc_dbg_register(void *dev_data)
{
	power_dbg_ops_register("sc", "volt_para_test_flag", dev_data,
		dc_dbg_volt_para_test_flag_show, dc_dbg_volt_para_test_flag_store);
	power_dbg_ops_register("sc", "volt_para", dev_data,
		dc_dbg_volt_para_show, dc_dbg_volt_para_store);
	power_dbg_ops_register("sc", "time_para", dev_data,
		dc_dbg_time_para_show, dc_dbg_time_para_store);
	power_dbg_ops_register("sc", "vstep_para", dev_data,
		dc_dbg_vstep_para_show, dc_dbg_vstep_para_store);
	power_dbg_ops_register("sc", "total_ibus", dev_data,
		dc_dbg_total_ibus_show, NULL);
	power_dbg_ops_register("sc", "main_ibus", dev_data,
		dc_dbg_main_ibus_show, NULL);
	power_dbg_ops_register("sc", "main_vbus", dev_data,
		dc_dbg_main_vbus_show, NULL);
	power_dbg_ops_register("sc", "main_vout", dev_data,
		dc_dbg_main_vout_show, NULL);
	power_dbg_ops_register("sc", "main_ibat", dev_data,
		dc_dbg_main_ibat_show, NULL);
	power_dbg_ops_register("sc", "main_vbat", dev_data,
		dc_dbg_main_vbat_show, NULL);
	power_dbg_ops_register("sc", "main_ic_temp", dev_data,
		dc_dbg_main_ic_temp_show, NULL);
	power_dbg_ops_register("sc", "main_ic_id", dev_data,
		dc_dbg_main_ic_id_show, NULL);
	power_dbg_ops_register("sc", "main_ic_name", dev_data,
		dc_dbg_main_ic_name_show, NULL);
	power_dbg_ops_register("sc", "aux_ibus", dev_data,
		dc_dbg_aux_ibus_show, NULL);
	power_dbg_ops_register("sc", "aux_vbus", dev_data,
		dc_dbg_aux_vbus_show, NULL);
	power_dbg_ops_register("sc", "aux_vout", dev_data,
		dc_dbg_aux_vout_show, NULL);
	power_dbg_ops_register("sc", "aux_ibat", dev_data,
		dc_dbg_aux_ibat_show, NULL);
	power_dbg_ops_register("sc", "aux_vbat", dev_data,
		dc_dbg_aux_vbat_show, NULL);
	power_dbg_ops_register("sc", "aux_ic_temp", dev_data,
		dc_dbg_aux_ic_temp_show, NULL);
	power_dbg_ops_register("sc", "aux_ic_id", dev_data,
		dc_dbg_aux_ic_id_show, NULL);
	power_dbg_ops_register("sc", "aux_ic_name", dev_data,
		dc_dbg_aux_ic_name_show, NULL);
	power_dbg_ops_register("sc", "aux_tbat", dev_data,
		dc_dbg_aux_tbat_show, NULL);
	power_dbg_ops_register("sc", "tadapt", dev_data,
		dc_dbg_tadapt_show, NULL);
	power_dbg_ops_register("sc", "vadapt", dev_data,
		dc_dbg_vadapt_show, NULL);
	power_dbg_ops_register("sc", "tusb", dev_data,
		dc_dbg_tusb_show, NULL);
}

void sc4_dbg_register(void *dev_data)
{
	power_dbg_ops_register("sc4", "volt_para_test_flag", dev_data,
		dc_dbg_volt_para_test_flag_show, dc_dbg_volt_para_test_flag_store);
	power_dbg_ops_register("sc4", "volt_para", dev_data,
		dc_dbg_volt_para_show, dc_dbg_volt_para_store);
	power_dbg_ops_register("sc4", "time_para", dev_data,
		dc_dbg_time_para_show, dc_dbg_time_para_store);
	power_dbg_ops_register("sc4", "vstep_para", dev_data,
		dc_dbg_vstep_para_show, dc_dbg_vstep_para_store);
	power_dbg_ops_register("sc4", "total_ibus", dev_data,
		dc_dbg_total_ibus_show, NULL);
	power_dbg_ops_register("sc4", "main_ibus", dev_data,
		dc_dbg_main_ibus_show, NULL);
	power_dbg_ops_register("sc4", "main_vbus", dev_data,
		dc_dbg_main_vbus_show, NULL);
	power_dbg_ops_register("sc4", "main_vout", dev_data,
		dc_dbg_main_vout_show, NULL);
	power_dbg_ops_register("sc4", "main_ibat", dev_data,
		dc_dbg_main_ibat_show, NULL);
	power_dbg_ops_register("sc4", "main_vbat", dev_data,
		dc_dbg_main_vbat_show, NULL);
	power_dbg_ops_register("sc4", "main_ic_temp", dev_data,
		dc_dbg_main_ic_temp_show, NULL);
	power_dbg_ops_register("sc4", "main_ic_id", dev_data,
		dc_dbg_main_ic_id_show, NULL);
	power_dbg_ops_register("sc4", "main_ic_name", dev_data,
		dc_dbg_main_ic_name_show, NULL);
	power_dbg_ops_register("sc4", "aux_ibus", dev_data,
		dc_dbg_aux_ibus_show, NULL);
	power_dbg_ops_register("sc4", "aux_vbus", dev_data,
		dc_dbg_aux_vbus_show, NULL);
	power_dbg_ops_register("sc4", "aux_vout", dev_data,
		dc_dbg_aux_vout_show, NULL);
	power_dbg_ops_register("sc4", "aux_ibat", dev_data,
		dc_dbg_aux_ibat_show, NULL);
	power_dbg_ops_register("sc4", "aux_vbat", dev_data,
		dc_dbg_aux_vbat_show, NULL);
	power_dbg_ops_register("sc4", "aux_ic_temp", dev_data,
		dc_dbg_aux_ic_temp_show, NULL);
	power_dbg_ops_register("sc4", "aux_ic_id", dev_data,
		dc_dbg_aux_ic_id_show, NULL);
	power_dbg_ops_register("sc4", "aux_ic_name", dev_data,
		dc_dbg_aux_ic_name_show, NULL);
	power_dbg_ops_register("sc4", "tadapt", dev_data,
		dc_dbg_tadapt_show, NULL);
	power_dbg_ops_register("sc4", "vadapt", dev_data,
		dc_dbg_vadapt_show, NULL);
}

void lvc_dbg_register(void *dev_data)
{
	power_dbg_ops_register("lvc", "lvc_mos_para", dev_data,
		dc_dbg_lvc_mos_para_show, dc_dbg_lvc_mos_para_store);
	power_dbg_ops_register("lvc", "volt_para_test_flag", dev_data,
		dc_dbg_volt_para_test_flag_show, dc_dbg_volt_para_test_flag_store);
	power_dbg_ops_register("lvc", "volt_para", dev_data,
		dc_dbg_volt_para_show, dc_dbg_volt_para_store);
	power_dbg_ops_register("lvc", "time_para", dev_data,
		dc_dbg_time_para_show, dc_dbg_time_para_store);
	power_dbg_ops_register("lvc", "ibus", dev_data,
		dc_dbg_main_ibus_show, NULL);
	power_dbg_ops_register("lvc", "vbus", dev_data,
		dc_dbg_main_vbus_show, NULL);
	power_dbg_ops_register("lvc", "ibat", dev_data,
		dc_dbg_main_ibat_show, NULL);
	power_dbg_ops_register("lvc", "vbat", dev_data,
		dc_dbg_main_vbat_show, NULL);
	power_dbg_ops_register("lvc", "ic_id", dev_data,
		dc_dbg_main_ic_id_show, NULL);
	power_dbg_ops_register("lvc", "ic_name", dev_data,
		dc_dbg_main_ic_name_show, NULL);
	power_dbg_ops_register("lvc", "vadapt", dev_data,
		dc_dbg_vadapt_show, NULL);
}
