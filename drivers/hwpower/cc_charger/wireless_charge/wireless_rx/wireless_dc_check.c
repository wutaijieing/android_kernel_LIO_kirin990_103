// SPDX-License-Identifier: GPL-2.0
/*
 * wireless_dc_check.c
 *
 * wireless direct charging check driver
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

#include <chipset_common/hwpower/wireless_charge/wireless_dc_check.h>
#include <huawei_platform/power/wireless/wireless_charger.h>
#include <huawei_platform/power/wireless/wireless_direct_charger.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_ic_intf.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_status.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_pmode.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_interfere.h>
#include <chipset_common/hwpower/direct_charge/direct_charge_ic_manager.h>
#include <chipset_common/hwpower/direct_charge/direct_charge_error_handle.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/common_module/power_delay.h>

#define HWLOG_TAG wireless_dc_check
HWLOG_REGIST();

static bool g_in_wldc_check;

static void wldc_enable_mode_priority(struct wldc_dev_info *di, int mode)
{
	if (!wldc_is_mode_para_valid(di, mode))
		return;

	di->mode_para[mode].dis_flag &= ~WLDC_DIS_BY_PRIORITY;
}

static void wldc_disable_mode_priority(struct wldc_dev_info *di, int mode)
{
	if (!wldc_is_mode_para_valid(di, mode))
		return;

	di->mode_para[mode].dis_flag |= WLDC_DIS_BY_PRIORITY;
}

static void wldc_check_priority_dis_flag(struct wldc_dev_info *di, int mode)
{
	int pmode_id;
	const char *cur_mode_name = NULL;

	if (!wldc_is_mode_para_valid(di, mode))
		return;

	if (!(di->mode_para[mode].dis_flag & WLDC_DIS_BY_PRIORITY))
		return;

	cur_mode_name = di->mode_para[mode].init_para.dc_name;
	pmode_id = wlrx_pmode_get_pid_by_name(WLTRX_DRV_MAIN, cur_mode_name);
	if (!wlrx_pmode_judge(WLTRX_DRV_MAIN, pmode_id + 1, WLDC_PMODE_FINAL_JUDGE))
		wldc_enable_mode_priority(di, mode);
}

static bool can_err_cnt_do_wldc_charge(struct wldc_dev_info *di, int mode)
{
	int pmode_id;
	const char *cur_mode_name = NULL;

	if (!wldc_is_mode_para_valid(di, mode))
		return false;

	if (di->mode_para[mode].err_cnt < WLDC_ERR_CNT_MAX)
		return true;

	if (di->mode_para[mode].dmd_report_flag ||
		(di->mode_para[mode].dc_open_retry_cnt >
		WLDC_OPEN_RETRY_CNT_MAX))
		return false;

	dc_show_eh_buf(di->wldc_err_dsm_buff);
	if (wlc_is_pwr_good())
		dc_report_eh_buf(di->wldc_err_dsm_buff, ERROR_WIRELESS_ERROR);
	dc_clean_eh_buf(di->wldc_err_dsm_buff, sizeof(di->wldc_err_dsm_buff));
	di->mode_para[mode].dmd_report_flag = true;
	cur_mode_name = di->mode_para[mode].init_para.dc_name;
	pmode_id = wlrx_pmode_get_pid_by_name(WLTRX_DRV_MAIN, cur_mode_name);
	wlc_clear_icon_pmode(pmode_id);
	wireless_charge_icon_display(WLRX_PMODE_NORMAL_JUDGE);

	return false;
}

static bool can_warning_cnt_do_wldc_charge(struct wldc_dev_info *di, int mode)
{
	if (!wldc_is_mode_para_valid(di, mode))
		return false;

	if (di->mode_para[mode].warn_cnt < WLDC_WARNING_CNT_MAX)
		return true;

	return false;
}

static bool can_support_mode_do_wldc_charge(struct wldc_dev_info *di, int mode)
{
	u8 rx_support_mode = wlc_get_rx_support_mode();

	if (!wldc_is_mode_para_valid(di, mode))
		return false;
	if (!(rx_support_mode & di->mode_para[mode].init_para.dc_type))
		return false;

	return true;
}

static bool can_dis_flag_do_wldc_charge(struct wldc_dev_info *di, int mode)
{
	if (!wldc_is_mode_para_valid(di, mode))
		return false;

	wldc_check_priority_dis_flag(di, mode);

	if (di->mode_para[mode].dis_flag)
		return false;

	return true;
}

static bool can_vbatt_do_wldc_charge(struct wldc_dev_info *di, int dc_mode)
{
	int vbatt, ls_vbatt, vbatt_min, vbatt_max;

	if (!wldc_is_mode_para_valid(di, dc_mode))
		return false;

	/* ignore vbatt check in this way while dc-charging */
	if (wldc_get_charge_stage() >= WLDC_STAGE_CHARGING)
		return true;

	vbatt = power_supply_app_get_bat_voltage_now();
	ls_vbatt = wldc_get_bat_voltage();
	if ((ls_vbatt > 0) && (ls_vbatt < vbatt))
		vbatt = ls_vbatt;

	vbatt_min = di->mode_para[dc_mode].init_para.vbatt_min;
	vbatt_max = di->mode_para[dc_mode].init_para.vbatt_max;
	if ((vbatt < vbatt_min) || (vbatt > vbatt_max))
		return false;

	return true;
}

static bool can_tbatt_do_wldc_charge(struct wldc_dev_info *di)
{
	int tbatt = 0;

	bat_temp_get_temperature(BAT_TEMP_MIXED, &tbatt);
	if (tbatt <= di->tbatt_lth) {
		di->tbatt_lth = di->tbat_limit.lth_back;
		return false;
	}
	di->tbatt_lth = di->tbat_limit.lth;
	if (tbatt >= di->tbatt_hth) {
		di->tbatt_hth = di->tbat_limit.hth_back;
		return false;
	}
	di->tbatt_hth = di->tbat_limit.hth;

	return true;
}

static bool can_pmode_crit_support_wldc_charge(void)
{
	int cur_pid;

	if (wldc_get_charge_stage() < WLDC_STAGE_CHARGING)
		return true;

	cur_pid = wlrx_pmode_get_curr_pid(WLTRX_DRV_MAIN);
	if (!wlrx_pmode_judge(WLTRX_DRV_MAIN, cur_pid, WLDC_PMODE_FINAL_JUDGE)) {
		hwlog_err("can_pmode_crit_support: failed\n");
		return false;
	}

	return true;
}

bool wldc_is_tmp_forbidden(struct wldc_dev_info *di, int mode)
{
	if (!can_support_mode_do_wldc_charge(di, mode))
		return true;
	if (!can_dis_flag_do_wldc_charge(di, mode))
		return true;
	if (!can_vbatt_do_wldc_charge(di, mode))
		return true;
	if (!can_tbatt_do_wldc_charge(di))
		return true;
	if (!can_pmode_crit_support_wldc_charge())
		return true;

	return false;
}

static bool wldc_is_forever_forbidden(struct wldc_dev_info *di, int mode)
{
	if (!can_err_cnt_do_wldc_charge(di, mode) ||
		!can_warning_cnt_do_wldc_charge(di, mode))
		return true;

	return false;
}

/*
 * wldc_prev_entry_check - check prev-entry for wireless direct charge
 * @di: device info
 * @dc_mode: direct charge mode to check
 * Returns zero if check successfully, non-zero otherwise.
 *
 * Forever and tmp conditions will be checked to determine priority:
 *     If cur mode is forbidden, lower power in priority must be permitted.
 *
 * e.g., (A is cur mode,  B is lower power mode)
 *    If A forverer mismatch,
 *        then B permitted by priority, return -1;
 *    else if A forverer and tmp match
 *        then B forbidden by priority, return 0;
 *    else, i.e., A forverer match but tmp mismatch
 *        then B permitted by priority, return -1.
 */
static int wldc_prev_entry_check(struct wldc_dev_info *di, int dc_mode)
{
	if (!wldc_is_mode_para_valid(di, dc_mode))
		return -EINVAL;

	if (wldc_is_forever_forbidden(di, dc_mode)) {
		wldc_disable_mode_priority(di, dc_mode);
		wldc_enable_mode_priority(di, dc_mode - 1);
		return -EPERM;
	}
	if (!wldc_is_tmp_forbidden(di, dc_mode)) {
		wldc_disable_mode_priority(di, dc_mode - 1);
		return 0;
	}

	wldc_enable_mode_priority(di, dc_mode - 1);
	return -EPERM;
}

static int wldc_prev_check(struct wldc_dev_info *di, const char *mode_name)
{
	int i;

	for (i = 0; i < di->total_dc_mode; i++) {
		if (!strncmp(mode_name, di->mode_para[i].init_para.dc_name,
			strlen(di->mode_para[i].init_para.dc_name)))
			break;
	}
	if ((i >= di->total_dc_mode) || (i < 0))
		return -EINVAL;

	if (wlrx_get_wired_channel_state() == WIRED_CHANNEL_ON) {
		hwlog_err("prev_check: wired vbus connect\n");
		return -EPERM;
	}
	if (wldc_get_charge_stage() == WLDC_STAGE_STOP_CHARGING) {
		hwlog_err("prev_check: tx disconnect\n");
		return -EPERM;
	}
	if (!di->sysfs_data.enable_charger || di->force_disable) {
		hwlog_debug("prev_check: wireless_sc is disabled\n");
		return -EPERM;
	}
	if (wldc_prev_entry_check(di, i))
		return -EPERM;

	return 0;
}

static int wldc_vout_accuracy_check(struct wldc_dev_info *di)
{
	int rx_vout = 0;
	int rx_iout = 0;
	int i, rx_init_vout, rx_vout_err_th;
	char tmp_buf[ERR_NO_STRING_SIZE] = {0};
	char dsm_buf[POWER_DSM_BUF_SIZE_1024] = {0};

	if (!wldc_is_mode_para_valid(di, di->cur_dc_mode))
		return -EINVAL;

	if (!power_msleep(DT_MSLEEP_1S, DT_MSLEEP_25MS, wldc_msleep_exit))
		return -EPERM;

	rx_init_vout = di->mode_para[di->cur_dc_mode].init_para.rx_vout;
	rx_vout_err_th = di->mode_para[di->cur_dc_mode].init_para.rx_vout_th;
	for (i = 0; i < WLDC_VOUT_ACCURACY_CHECK_CNT; i++) {
		if (wldc_get_charge_stage() == WLDC_STAGE_STOP_CHARGING) {
			hwlog_err("vout_accuracy_check: wireless tx disconnect\n");
			return -EPERM;
		}
		(void)wlrx_ic_get_vout(WLTRX_IC_MAIN, &rx_vout);
		(void)wlrx_ic_get_iout(WLTRX_IC_MAIN, &rx_iout);
		snprintf(tmp_buf, sizeof(tmp_buf), "vset:%dmV, vout:%dmV, iout:%dmA\n",
			rx_init_vout, rx_vout, rx_iout);
		strncat(dsm_buf, tmp_buf, strlen(tmp_buf));
		hwlog_info("[vout_accuracy_check] vset:%dmV vrx:%dmV err_th:%dmV irx:%dmA\n",
			rx_init_vout, rx_vout, rx_vout_err_th, rx_iout);
		if (abs(rx_vout - rx_init_vout) <= rx_vout_err_th) {
			if (!power_msleep(DT_MSLEEP_100MS, DT_MSLEEP_25MS, wldc_msleep_exit))
				return -EPERM;
			continue;
		}
		dc_fill_eh_buf(di->wldc_err_dsm_buff, sizeof(di->wldc_err_dsm_buff),
			WLDC_EH_VOUT_ACCURACY, dsm_buf);
		return -EPERM;
	}

	hwlog_info("[vout_accuracy_check] succ\n");
	return 0;
}

static int wldc_leak_current_check(struct wldc_dev_info *di)
{
	int i, ileak_th;
	int iout = 0;
	char tmp_buf[POWER_DSM_BUF_SIZE_0128] = {0};

	if (!wldc_is_mode_para_valid(di, di->cur_dc_mode))
		return -EINVAL;

	ileak_th = di->mode_para[di->cur_dc_mode].init_para.ileak_th;
	for (i = 0; i < WLDC_LEAK_CURRENT_CHECK_CNT; i++) {
		(void)wlrx_ic_get_iout(WLTRX_IC_MAIN, &iout);
		hwlog_info("[leak_current_check] leak_current:%dmA\n", iout);
		if (iout > ileak_th) {
			snprintf(tmp_buf, sizeof(tmp_buf), "ileak:%d\n", iout);
			dc_fill_eh_buf(di->wldc_err_dsm_buff, sizeof(di->wldc_err_dsm_buff),
				WLDC_EH_ILEAK, tmp_buf);
			return -EPERM;
		}
		if (wldc_get_charge_stage() == WLDC_STAGE_STOP_CHARGING) {
			hwlog_err("leak_current_check: tx disconnect\n");
			return -EPERM;
		}
		(void)power_msleep(DT_MSLEEP_50MS, DT_MSLEEP_25MS, wldc_msleep_exit);
	}

	hwlog_info("[leak_current_check] succ\n");
	return 0;
}

static int wldc_vrect_vout_diff_check(struct wldc_dev_info *di)
{
	int i, vdiff_th;
	int vrect = 0;
	int vout = 0;
	char tmp_buf[ERR_NO_STRING_SIZE] = {0};
	char dsm_buf[POWER_DSM_BUF_SIZE_0256 * WLDC_VDIFF_CHECK_CNT] = {0};

	if (!wldc_is_mode_para_valid(di, di->cur_dc_mode))
		return -EINVAL;

	vdiff_th = di->mode_para[di->cur_dc_mode].init_para.vdiff_th;
	for (i = 0; i < WLDC_VDIFF_CHECK_CNT; i++) {
		(void)wlrx_ic_get_vrect(WLTRX_IC_MAIN, &vrect);
		(void)wlrx_ic_get_vout(WLTRX_IC_MAIN, &vout);
		hwlog_info("[vrect_vout_diff_check] vrect:%dmV vout:%dmV vdiff_th:%dmV\n",
			vrect, vout, vdiff_th);
		if (vrect - vout > vdiff_th) {
			hwlog_info("[vrect_vout_diff_check] succ\n");
			return 0;
		}
		snprintf(tmp_buf, sizeof(tmp_buf), "vrect:%dmV vout:%dmV\n", vrect, vout);
		strncat(dsm_buf, tmp_buf, strlen(tmp_buf));
		if (wldc_get_charge_stage() == WLDC_STAGE_STOP_CHARGING) {
			hwlog_err("vrect_vout_diff_check: tx disconnect\n");
			return -EPERM;
		}
		(void)power_msleep(DT_MSLEEP_100MS, DT_MSLEEP_25MS, wldc_msleep_exit);
	}
	dc_fill_eh_buf(di->wldc_err_dsm_buff, sizeof(di->wldc_err_dsm_buff),
		WLDC_EH_VDIFF, dsm_buf);
	return -EPERM;
}

static int wldc_open_dc_path_regulate_vrx(struct wldc_dev_info *di)
{
	int ls_vbatt, ls_ibus;
	int rx_vrect = 0;
	int rx_vout = 0;
	char tmp_buf[ERR_NO_STRING_SIZE] = {0};

	ls_ibus = wldc_get_ls_ibus();
	ls_vbatt = wldc_get_bat_voltage();
	if (ls_vbatt > di->mode_para[di->cur_dc_mode].init_para.vbatt_max) {
		hwlog_err("open_dc_path_regulate_vrx: ls_vbatt:%dmV high, ls_ibus:%d\n",
			ls_vbatt, ls_ibus);
		snprintf(tmp_buf, sizeof(tmp_buf), "ls_vbatt:%dmV high, ls_ibus:%dmA",
			ls_vbatt, ls_ibus);
		dc_fill_eh_buf(di->wldc_err_dsm_buff, sizeof(di->wldc_err_dsm_buff),
			WLDC_EH_OPEN_DC_PATH, tmp_buf);
		return -EPERM;
	}
	di->rx_vout_set += WLDC_DEFAULT_VSTEP;
	if (wireless_charge_set_rx_vout(di->rx_vout_set)) {
		hwlog_err("open_dc_path_regulate_vrx: set rx vout failed\n");
		snprintf(tmp_buf, sizeof(tmp_buf), "set rx_vout");
		dc_fill_eh_buf(di->wldc_err_dsm_buff, sizeof(di->wldc_err_dsm_buff),
			WLDC_EH_OPEN_DC_PATH, tmp_buf);
		return -EPERM;
	}
	(void)wlrx_ic_get_vrect(WLTRX_IC_MAIN, &rx_vrect);
	(void)wlrx_ic_get_vout(WLTRX_IC_MAIN, &rx_vout);
	hwlog_info("[open_dc_path_regulate_vrx] ls_ibus:%dmA vrect:%dmV vout:%dmV\n",
		ls_ibus, rx_vrect, rx_vout);
	return 0;
}

static int wldc_open_dc_path_check(struct wldc_dev_info *di)
{
	int i, ls_ibus;
	char tmp_buf[ERR_NO_STRING_SIZE] = {0};

	for (i = 0; i < WLDC_OPEN_PATH_CNT; i++) {
		if (!power_msleep(DT_MSLEEP_500MS, DT_MSLEEP_25MS, wldc_msleep_exit))
			return -EPERM;

		ls_ibus = wldc_get_ls_ibus();
		if (ls_ibus > WLDC_OPEN_PATH_IOUT_MIN)
			return 0;

		if (wldc_open_dc_path_regulate_vrx(di) < 0)
			return -EPERM;

		(void)dcm_enable_ic(SC_MODE, di->ic_data.cur_type, DC_IC_ENABLE);
		if (dcm_is_ic_close(SC_MODE, di->ic_data.cur_type))
			hwlog_err("open_dc_path_check: ls is close\n");

		if (wldc_get_charge_stage() == WLDC_STAGE_STOP_CHARGING) {
			hwlog_err("open_dc_path_check: wireless tx disconnect\n");
			return -EPERM;
		}
	}
	if (i >= WLDC_OPEN_PATH_CNT) {
		hwlog_err("open_dc_path_check: failed, ls_ibus:%d\n", ls_ibus);
		snprintf(tmp_buf, sizeof(tmp_buf), "open dc_path, ls_ibus:%dmA", ls_ibus);
		dc_fill_eh_buf(di->wldc_err_dsm_buff, sizeof(di->wldc_err_dsm_buff),
			WLDC_EH_OPEN_DC_PATH, tmp_buf);
		if (power_supply_app_get_bat_capacity() >= 95) { /* maybe normal, when capacity>=95 */
			++di->mode_para[di->cur_dc_mode].dc_open_retry_cnt;
			hwlog_info("[open_dc_path_check] failed caused by high soc\n");
		}
		return -EPERM;
	}

	return 0;
}

static int wldc_open_dc_path_set_inital_vrx(struct wldc_dev_info *di)
{
	int vbatt;
	int ls_vbatt = wldc_get_bat_voltage();
	char tmp_buf[ERR_NO_STRING_SIZE] = {0};

	vbatt = power_supply_app_get_bat_voltage_now();
	if (ls_vbatt < 0) {
		hwlog_err("open_dc_path_set_inital_vrx: get ls_vbatt failed\n");
		return -EIO;
	}

	hwlog_info("[open_dc_path_set_inital_vrx] vbatt ls:%dmV,coul:%dmV\n", ls_vbatt, vbatt);
	di->rx_vout_set = di->mode_para[di->cur_dc_mode].init_para.rx_ratio *
		(ls_vbatt * di->volt_ratio + di->mode_para[di->cur_dc_mode].init_para.vdelt);
	if (wireless_charge_set_rx_vout(di->rx_vout_set)) {
		hwlog_err("open_dc_path_set_inital_vrx: set vrx failed\n");
		snprintf(tmp_buf, sizeof(tmp_buf), "set rx_vout\n");
		dc_fill_eh_buf(di->wldc_err_dsm_buff, sizeof(di->wldc_err_dsm_buff),
			WLDC_EH_OPEN_DC_PATH, tmp_buf);
		return -EIO;
	}

	/* delay for vout stability, generally 500ms */
	if (!power_msleep(DT_MSLEEP_500MS, DT_MSLEEP_25MS, wldc_msleep_exit))
		return -EPERM;

	if (dcm_enable_ic(SC_MODE, di->ic_data.cur_type, DC_IC_ENABLE)) {
		hwlog_err("open_dc_path_set_inital_vrx: ls enable fail\n");
		snprintf(tmp_buf, sizeof(tmp_buf), "ls_en\n");
		dc_fill_eh_buf(di->wldc_err_dsm_buff, sizeof(di->wldc_err_dsm_buff),
			WLDC_EH_OPEN_DC_PATH, tmp_buf);
		return -EPERM;
	}

	return 0;
}

static int wldc_open_direct_charge_path(struct wldc_dev_info *di)
{
	if (!wldc_is_mode_para_valid(di, di->cur_dc_mode))
		return -EINVAL;

	if (multi_ic_check_select_init_mode(&di->ic_mode_para, &di->ic_data.cur_type))
		return -EPERM;

	if (wldc_open_dc_path_set_inital_vrx(di) < 0)
		return -EPERM;

	if (wldc_open_dc_path_check(di) < 0)
		return -EPERM;

	hwlog_info("[open_direct_charge_path] succ\n");
	return 0;
}

static int wldc_security_check(struct wldc_dev_info *di)
{
	int ret;

	ret = wldc_vout_accuracy_check(di);
	if (ret) {
		hwlog_err("security_check: voltage_accuracy_check failed\n");
		return ret;
	}
	ret = wldc_leak_current_check(di);
	if (ret) {
		hwlog_err("security_check: leak_current_check failed\n");
		return ret;
	}
	ret = wldc_vrect_vout_diff_check(di);
	if (ret) {
		hwlog_err("security_check: vrect_vout_delta_check failed\n");
		return ret;
	}
	ret = wldc_open_direct_charge_path(di);
	if (ret) {
		hwlog_err("security_check: open_direct_charge_path failed\n");
		return ret;
	}
	return 0;
}

static int wldc_set_rx_init_vout(struct wldc_dev_info *di)
{
	int ret;
	int vtx;
	int vrx;

	if (!di) {
		hwlog_err("set_rx_init_vout: di null\n");
		return -ENODEV;
	}

	if (!wldc_is_mode_para_valid(di, di->cur_dc_mode))
		return -EINVAL;

	vtx = di->mode_para[di->cur_dc_mode].init_para.tx_vout;
	vrx = di->mode_para[di->cur_dc_mode].init_para.rx_vout;
	hwlog_info("[set_rx_init_vout] vtx=%d vrx=%d\n", vtx, vrx);
	ret = wldc_set_trx_vout(vtx, vrx);
	if (ret) {
		hwlog_err("set_rx_init_vout: set trx init vout failed\n");
		dc_fill_eh_buf(di->wldc_err_dsm_buff,
			sizeof(di->wldc_err_dsm_buff),
			WLDC_EH_SET_TRX_INIT_VOUT, NULL);
		return ret;
	}
	di->rx_vout_set = vrx;
	return 0;
}

static int wldc_chip_init(struct wldc_dev_info *di)
{
	int ret;

	ret = dcm_init_ic(SC_MODE, di->ic_data.cur_type);
	if (ret) {
		hwlog_err("chip_init: ls init failed\n");
		dc_fill_eh_buf(di->wldc_err_dsm_buff,
			sizeof(di->wldc_err_dsm_buff),
			WLDC_EH_LS_INIT, NULL);
		return ret;
	}
	ret = dcm_init_batinfo(SC_MODE, di->ic_data.cur_type);
	if (ret) {
		hwlog_err("chip_init: bi init failed\n");
		dc_fill_eh_buf(di->wldc_err_dsm_buff,
			sizeof(di->wldc_err_dsm_buff),
			WLDC_EH_BI_INIT, NULL);
		return ret;
	}
	hwlog_info("[chip_init] succ\n");
	return 0;
}

static int wldc_check_prev_init(struct wldc_dev_info *di)
{
	int ret;

	ret = wldc_set_rx_init_vout(di);
	if (ret) {
		di->stop_flag_error = true;
		hwlog_err("prev_init: set rx_init_vout failed\n");
		wldc_stop_charging(di);
		return ret;
	}
	ret = wldc_chip_init(di);
	if (ret) {
		di->stop_flag_error = true;
		hwlog_err("prev_init: dc_chip_init failed\n");
		wldc_stop_charging(di);
		return ret;
	}
	wldc_extra_power_supply(true);
	wireless_charge_chip_init(WILREESS_SC_CHIP_INIT);
	ret = wldc_cut_off_normal_charge_path();
	if (ret) {
		di->stop_flag_error = true;
		hwlog_err("prev_init: cutt_off_normal_charge_path failed\n");
		dc_fill_eh_buf(di->wldc_err_dsm_buff, sizeof(di->wldc_err_dsm_buff),
			WLDC_EH_CUT_OFF_NORMAL_PATH, NULL);
		wldc_stop_charging(di);
		return ret;
	}
	wlc_ignore_vbus_only_event(true);
	ret = wldc_turn_on_direct_charge_channel();
	if (ret) {
		di->stop_flag_error = true;
		hwlog_err("prev_init: turn_on_dc_channel failed\n");
		dc_fill_eh_buf(di->wldc_err_dsm_buff, sizeof(di->wldc_err_dsm_buff),
			WLDC_EH_TURN_ON_DC_PATH, NULL);
		wldc_stop_charging(di);
	}

	return ret;
}

static int wireless_sc_charge_check(struct wldc_dev_info *di)
{
	if (wldc_check_prev_init(di))
		return -EPERM;

	if (wldc_security_check(di)) {
		di->stop_flag_error = true;
		wldc_stop_charging(di);
		return -EPERM;
	}
	return 0;
}

static int wldc_formal_check(struct wldc_dev_info *di, const char *mode_name)
{
	int i;
	int ret = -EPERM;

	if (!di->dc_stop_flag) {
		hwlog_err("formal_check: in wldc, ignore\n");
		return ret;
	}

	for (i = 0; i < di->total_dc_mode; i++) {
		di->cur_dc_mode = i;
		if (strncmp(mode_name, di->mode_para[i].init_para.dc_name,
			strlen(di->mode_para[i].init_para.dc_name)))
			continue;
		wldc_set_charge_stage(WLDC_STAGE_CHECK);
		di->dc_stop_flag = false;
		ret =  wireless_sc_charge_check(di);
		if (!ret) {
			wldc_set_charge_stage(WLDC_STAGE_SUCCESS);
			wldc_start_charging(di);
		}
		return ret;
	}

	return ret;
}

bool wlrx_pmode_dc_judge_crit(const char *mode_name, int pid)
{
	struct wldc_dev_info *di = NULL;

	wireless_sc_get_di(&di);
	if (!di) {
		hwlog_err("get di failed\n");
		return false;
	}

	wldc_set_di(di);

	if (wldc_prev_check(di, mode_name))
		return false;

	if ((wlrx_get_charge_stage() == WLRX_STAGE_REGULATION_DC) || g_in_wldc_check)
		return true;

	g_in_wldc_check = true;
	if (wldc_formal_check(di, mode_name)) {
		g_in_wldc_check = false;
		return false;
	}

	wlrx_set_charge_stage(WLRX_STAGE_REGULATION_DC);
	wlrx_pmode_set_curr_pid(WLTRX_DRV_MAIN, pid);
	g_in_wldc_check = false;
	return true;
}
