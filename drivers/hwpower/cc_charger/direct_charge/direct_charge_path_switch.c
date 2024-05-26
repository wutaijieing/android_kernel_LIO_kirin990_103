// SPDX-License-Identifier: GPL-2.0
/*
 * direct_charge_path_switch.c
 *
 * path switch for direct charge
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
#include <chipset_common/hwpower/common_module/power_cmdline.h>
#include <chipset_common/hwpower/common_module/power_common_macro.h>
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_dsm.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/common_module/power_time.h>
#include <chipset_common/hwpower/hardware_ic/charge_pump.h>

#define HWLOG_TAG direct_charge_path
HWLOG_REGIST();

#define MULTI_IC_CHECK_COUNT     3

static int dc_enable_multi_ic(struct direct_charge_device *di, int enable)
{
	int mode;

	if (di->cur_mode == CHARGE_IC_AUX)
		mode = CHARGE_IC_MAIN;
	else
		mode = CHARGE_IC_AUX;

	if (enable && dcm_init_ic(di->working_mode, mode)) {
		di->multi_ic_error_cnt++;
		return -EPERM;
	}
	if (enable && dcm_init_batinfo(di->working_mode, mode)) {
		di->multi_ic_error_cnt++;
		return -EPERM;
	}

	if (dcm_enable_ic(di->working_mode, mode, enable)) {
		di->multi_ic_error_cnt++;
		return -EPERM;
	}

	return 0;
}

static int dc_check_multi_ic_current(struct direct_charge_device *di)
{
	int main_ibus = 0;
	int aux_ibus = 0;
	int count = 10; /* 10 : retry times */

	while (count) {
		if (dcm_get_ic_ibus(di->working_mode, CHARGE_IC_MAIN, &main_ibus))
			return -EPERM;
		if (dcm_get_ic_ibus(di->working_mode, CHARGE_IC_AUX, &aux_ibus))
			return -EPERM;
		hwlog_info("check multi curr, main ibus=%d, aux_ibus=%d\n",
			main_ibus, aux_ibus);

		if ((main_ibus > MIN_CURRENT_FOR_MULTI_IC) &&
			(aux_ibus > MIN_CURRENT_FOR_MULTI_IC))
			return 0;
		msleep(DT_MSLEEP_30MS); /* adc cycle 20+ms */
		count--;
	}

	return -EPERM;
}

/* rt charge mode test select single charge path */
static int dc_rt_select_charge_path(struct direct_charge_device *di)
{
	if (power_cmdline_is_factory_mode() &&
		(di->sysfs_mainsc_enable_charger ^ di->sysfs_auxsc_enable_charger)) {
		hwlog_info("RT single sc charge mode test\n");
		return -EPERM;
	}

	return 0;
}

static void dc_change2singlepath(struct direct_charge_device *di)
{
	int count = MULTI_IC_CHECK_COUNT;
	int ibus = 0;
	int volt_ratio = di->dc_volt_ratio;

	if (di->cur_mode != CHARGE_MULTI_IC)
		return;

	if (dcm_use_two_stage() && (di->working_mode == SC4_MODE))
		volt_ratio /= SC_IN_OUT_VOLT_RATE;

	while (count) {
		if (di->multi_ic_check_info.force_single_path_flag) {
			dc_preparation_before_switch_to_singlepath(di->working_mode,
				di->dc_volt_ratio, di->init_delt_vset);
			break;
		}
		if (dcm_get_ic_ibus(di->working_mode, di->cur_mode, &ibus))
			return;
		hwlog_info("charge2single check ibat is %d\n", ibus);
		if (ibus > (di->multi_ic_ibat_th / volt_ratio))
			return;
		msleep(DT_MSLEEP_30MS); /* adc cycle 20+ms */
		count--;
	}

	if (dcm_enable_ic(di->working_mode, CHARGE_IC_AUX, DC_IC_DISABLE)) {
		di->stop_charging_flag_error = 1; /* set sc or lvc error flag */
		return;
	}

	dc_close_aux_wired_channel();
	hwlog_info("charge to single sc done\n");
	di->cur_mode = CHARGE_IC_MAIN;
}

static void dc_change2multipath(struct direct_charge_device *di)
{
	int ibus = 0;
	int count = MULTI_IC_CHECK_COUNT;
	int volt_ratio = di->dc_volt_ratio;

	if (di->cur_mode == CHARGE_MULTI_IC)
		return;

	if (di->multi_ic_check_info.force_single_path_flag)
		return;

	if (multi_ic_check_ic_status(&di->multi_ic_mode_para)) {
		hwlog_err("ic status error, can not enter multi ic charge\n");
		return;
	}

	if (dcm_use_two_stage() && (di->working_mode == SC4_MODE))
		volt_ratio /= SC_IN_OUT_VOLT_RATE;

	while (count) {
		if (dcm_get_ic_ibus(di->working_mode, di->cur_mode, &ibus))
			return;
		hwlog_info("change2multi check ibus is %d\n", ibus);
		if (ibus < (di->multi_ic_ibat_th / volt_ratio))
			return;
		msleep(DT_MSLEEP_30MS); /* adc cycle 20+ms */
		count--;
	}

	if (dc_enable_multi_ic(di, DC_IC_ENABLE)) {
		di->multi_ic_error_cnt++;
		return;
	}

	di->cur_mode = CHARGE_MULTI_IC;
	msleep(DT_MSLEEP_250MS); /* need 200+ms to wait for sc open */

	if (dc_check_multi_ic_current(di)) {
		dc_enable_multi_ic(di, DC_IC_DISABLE);
		multi_ic_check_set_ic_error_flag(CHARGE_IC_AUX, &di->multi_ic_mode_para);
		di->cur_mode = CHARGE_IC_MAIN;
		di->multi_ic_error_cnt++;
		return;
	}

	hwlog_info("charge to multi sc done\n");
	di->multi_ic_check_info.multi_ic_start_time = power_get_current_kernel_time().tv_sec;
}

static int dc_get_dc_channel(struct direct_charge_device *di)
{
	if (!di)
		return WIRED_CHANNEL_END;

	switch (di->working_mode) {
	case LVC_MODE:
		return WIRED_CHANNEL_LVC;
	case SC_MODE:
		return WIRED_CHANNEL_SC;
	case SC4_MODE:
		return WIRED_CHANNEL_SC4;
	default:
		return WIRED_CHANNEL_END;
	}
}

int dc_close_dc_channel(void)
{
	int ret = 0;
	int path;
	struct direct_charge_device *l_di = direct_charge_get_di();

	if (!l_di)
		return -EPERM;

	path = dc_get_dc_channel(l_di);
	if ((path >= WIRED_CHANNEL_END) || (path < WIRED_CHANNEL_BEGIN))
		return -EPERM;

	if (l_di->need_wired_sw_off)
		ret = wired_chsw_set_wired_channel(WIRED_CHANNEL_BUCK, WIRED_CHANNEL_RESTORE);
	power_msleep(DT_MSLEEP_200MS, 0, NULL);
	ret += wired_chsw_set_wired_channel(path, WIRED_CHANNEL_CUTOFF);

	return ret;
}

int dc_open_dc_channel(void)
{
	int ret;
	int path;
	struct direct_charge_device *l_di = direct_charge_get_di();

	if (!l_di)
		return -EPERM;

	path = dc_get_dc_channel(l_di);
	if ((path >= WIRED_CHANNEL_END) || (path < WIRED_CHANNEL_BEGIN))
		return -EPERM;

	ret = wired_chsw_set_wired_channel(path, WIRED_CHANNEL_RESTORE);
	if (dcm_use_two_stage() && (l_di->working_mode == SC4_MODE)) {
		ret += dc_init_adapter(ADAPTER_9V * POWER_MV_PER_V);
		power_msleep(DT_MSLEEP_200MS, 0, NULL);
		ret += dcm_init_first_level_ic();
		if (ret) {
			dcm_enable_first_level_ic(CP_TYPE_MAIN, true);
			dcm_enable_first_level_ic(CP_TYPE_AUX, true);
			ret = dcm_init_first_level_ic();
			if (ret)
				return ret;
		}
		dcm_enable_first_level_ic(CP_TYPE_MAIN, true);
		power_msleep(DT_MSLEEP_200MS, 0, NULL);
	}

	if (l_di->need_wired_sw_off)
		ret += wired_chsw_set_wired_channel(WIRED_CHANNEL_BUCK, WIRED_CHANNEL_CUTOFF);
	if (dcm_use_two_stage() && (l_di->working_mode == SC4_MODE)) {
		power_msleep(DT_MSLEEP_200MS, 0, NULL);
		ret += dcm_enable_irq(l_di->working_mode, CHARGE_IC_MAIN, DC_IRQ_DISABLE);
		ret += dcm_set_first_level_ic_mode(DC_IC_CP_MODE);
		dcm_enable_first_level_ic(CP_TYPE_AUX, true);
	}

	return ret;
}

void dc_open_aux_wired_channel(void)
{
	struct direct_charge_device *l_di = direct_charge_get_di();

	if (!l_di)
		return;

	if (l_di->multi_ic_mode_para.support_multi_ic &&
		(l_di->multi_ic_error_cnt < MULTI_IC_CHECK_ERR_CNT_MAX)) {
		wired_chsw_set_wired_channel(WIRED_CHANNEL_SC_AUX, WIRED_CHANNEL_RESTORE);
		power_usleep(DT_USLEEP_5MS); /* delay 5ms, gennerally 0.1ms */
	}
}

void dc_close_aux_wired_channel(void)
{
	struct direct_charge_device *l_di = direct_charge_get_di();

	if (!l_di || !l_di->multi_ic_mode_para.support_multi_ic)
		return;

	wired_chsw_set_wired_channel(WIRED_CHANNEL_SC_AUX, WIRED_CHANNEL_CUTOFF);
}

static void dc_open_lvc_mos_channel(void)
{
	struct direct_charge_device *l_di = direct_charge_get_di();

	if (!l_di || !l_di->lvc_mos_dts.is_support)
		return;

	wired_chsw_set_wired_channel(WIRED_CHANNEL_LVC_MOS, WIRED_CHANNEL_RESTORE);
	power_usleep(DT_USLEEP_5MS); /* delay 5ms, gennerally 0.1ms */
	if (wired_chsw_get_wired_channel(WIRED_CHANNEL_LVC_MOS) == WIRED_CHANNEL_RESTORE) {
		l_di->lvc_mos_dts.enable = true;
		hwlog_info("open %s charging path succ\n",
			wired_chsw_get_channel_type_string(WIRED_CHANNEL_LVC_MOS));
	}
}

void dc_close_lvc_mos_channel(void)
{
	struct direct_charge_device *l_di = direct_charge_get_di();

	if (!l_di || !l_di->lvc_mos_dts.is_support)
		return;

	wired_chsw_set_wired_channel(WIRED_CHANNEL_LVC_MOS, WIRED_CHANNEL_CUTOFF);
	l_di->lvc_mos_dts.enable = false;
}

static bool dc_lvc_mos_channel_security_check(int ibat, int ibus,
	struct dc_lvc_mos_para *para)
{
	char tmp_buf[POWER_DSM_BUF_SIZE_0128] = { 0 };

	if (!power_cmdline_is_factory_mode() || (ibat <= 0) || (ibus <= 0))
		return false;

	if (!para->enable)
		return para->err_cnt >= para->err_cnt_th;

	if (ibat / ibus >= para->err_th)
		return false;

	para->err_cnt++;
	hwlog_info("check lvc_mos_channel error, ibat=%d, ibus=%d, ratio=%d, cnt=%u\n",
		ibat, ibus, ibat / ibus, para->err_cnt);
	if (para->err_cnt >= para->err_cnt_th) {
		snprintf(tmp_buf, sizeof(tmp_buf),
			"check lvc_mos_channel error, cnt=%d,  ratio=%d\n",
			para->err_cnt, ibat / ibus);
		power_dsm_report_dmd(POWER_DSM_BATTERY,
			POWER_DSM_MULTI_CHARGE_CURRENT_RATIO_ERROR, tmp_buf);
		dc_close_lvc_mos_channel();
		return true;
	}

	return false;
}

static void dc_check_lvc_mos_channel(struct direct_charge_device *di)
{
	int ibus = 0;
	int ibat = 0;
	int volt_ratio;

	if (!di->lvc_mos_dts.is_support)
		return;

	if (dcm_get_ic_ibus(di->working_mode, di->cur_mode, &ibus) ||
		dcm_get_ic_ibat(di->working_mode, di->cur_mode, &ibat))
		return;

	if (dc_lvc_mos_channel_security_check(ibat, ibus, &di->lvc_mos_dts))
		return;

	volt_ratio = di->dc_volt_ratio;
	if (ibus >= ((di->lvc_mos_dts.ibat_th + di->lvc_mos_dts.curr_offset) / volt_ratio))
		dc_open_lvc_mos_channel();
	else if (ibus <= ((di->lvc_mos_dts.ibat_th - di->lvc_mos_dts.curr_offset) / volt_ratio))
		dc_close_lvc_mos_channel();
}

void dc_select_charge_path(void)
{
	int ibus = 0;
	struct direct_charge_device *l_di = direct_charge_get_di();
	int volt_ratio;

	if (!l_di)
		return;

	dc_check_lvc_mos_channel(l_di);

	if (!l_di->multi_ic_mode_para.support_multi_ic ||
		(l_di->multi_ic_error_cnt > MULTI_IC_CHECK_ERR_CNT_MAX))
		return;

	if (dc_rt_select_charge_path(l_di))
		return;

	if (l_di->multi_ic_check_info.force_single_path_flag) {
		dc_change2singlepath(l_di);
		return;
	}

	if (dcm_get_ic_ibus(l_di->working_mode, l_di->cur_mode, &ibus))
		return;

	volt_ratio = l_di->dc_volt_ratio;
	if (dcm_use_two_stage() && (l_di->working_mode == SC4_MODE))
		volt_ratio /= SC_IN_OUT_VOLT_RATE;
	if (ibus >= ((l_di->multi_ic_ibat_th + l_di->curr_offset) / volt_ratio))
		dc_change2multipath(l_di);
	else if (ibus <= ((l_di->multi_ic_ibat_th - l_di->curr_offset) / volt_ratio))
		dc_change2singlepath(l_di);
}

int dc_switch_charging_path(unsigned int path)
{
	hwlog_info("switch to %d,%s charging path\n",
		path, wired_chsw_get_channel_type_string(path));

	switch (path) {
	case WIRED_CHANNEL_BUCK:
		return direct_charge_switch_path_to_normal_charging();
	case WIRED_CHANNEL_LVC:
	case WIRED_CHANNEL_SC:
	case WIRED_CHANNEL_SC4:
		return direct_charge_switch_path_to_dc_charging();
	default:
		return -EPERM;
	}
}
