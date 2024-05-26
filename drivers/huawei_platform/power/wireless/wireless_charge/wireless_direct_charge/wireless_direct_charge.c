/*
 * wireless_direct_charge.c
 *
 * wireless direct charge driver
 *
 * Copyright (c) 2018-2020 Huawei Technologies Co., Ltd.
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
#include <linux/workqueue.h>
#include <linux/notifier.h>
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/hardware_ic/boost_5v.h>
#include <chipset_common/hwpower/hardware_channel/wired_channel_switch.h>
#include <chipset_common/hwpower/common_module/power_event_ne.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <huawei_platform/power/huawei_charger.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_ic_intf.h>
#include <chipset_common/hwpower/wireless_charge/wireless_power_supply.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_status.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_pmode.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_acc.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_interfere.h>
#include <chipset_common/hwpower/hardware_ic/charge_pump.h>
#include <chipset_common/hwpower/direct_charge/direct_charge_error_handle.h>
#include <huawei_platform/power/wireless/wireless_direct_charger.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_alarm.h>
#include <chipset_common/hwpower/direct_charge/direct_charge_ic_manager.h>
#include <huawei_platform/hwpower/common_module/power_platform.h>

#define HWLOG_TAG wireless_dc
HWLOG_REGIST();

static struct wldc_dev_info *g_wldc_di;
static int g_wldc_need_wired_sw_on = WLDC_NEED_WIRED_SW_ON;

void wldc_set_di(struct wldc_dev_info *di)
{
	if (!di)
		hwlog_err("set_di: di null\n");
	else if (!g_wldc_di)
		g_wldc_di = di;
}

bool wldc_msleep_exit(void)
{
	return wldc_get_charge_stage() == WLDC_STAGE_STOP_CHARGING;
}

bool wldc_is_mode_para_valid(struct wldc_dev_info *di, int id)
{
	if (di->total_dc_mode <= 0)
		return false;

	if ((id < 0) || (id >= di->total_dc_mode))
		return false;

	return true;
}

int wldc_get_bat_voltage(void)
{
	int btb_vol;
	struct wldc_dev_info *di = g_wldc_di;

	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return -ENODEV;
	}

	btb_vol = dcm_get_ic_vbtb_with_comp(SC_MODE, di->ic_data.cur_type,
		di->comp_para.vbat_comp);
	if (btb_vol < 0) {
		di->stop_flag_error = true;
		hwlog_err("%s: error\n", __func__);
		dc_fill_eh_buf(di->wldc_err_dsm_buff,
			sizeof(di->wldc_err_dsm_buff),
			WLDC_EH_GET_VBATT, NULL);
		return -EINVAL;
	}

	return btb_vol;
}

static int wldc_get_bat_current(void)
{
	int ret;
	int bat_curr = 0;
	struct wldc_dev_info *di = g_wldc_di;

	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return -ENODEV;
	}
	ret = dcm_get_total_ibat(SC_MODE, di->ic_data.cur_type, &bat_curr);
	if (ret) {
		di->stop_flag_error = true;
		hwlog_err("%s: error\n", __func__);
		dc_fill_eh_buf(di->wldc_err_dsm_buff,
			sizeof(di->wldc_err_dsm_buff),
			WLDC_EH_GET_IBATT, NULL);
	}

	return bat_curr;
}

int wldc_get_ls_vbus(void)
{
	int ret;
	int vbus = 0;
	struct wldc_dev_info *di = g_wldc_di;

	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return -ENODEV;
	}
	ret = dcm_get_ic_vbus(SC_MODE, di->ic_data.cur_type, &vbus);
	if (ret < 0) {
		di->stop_flag_error = true;
		hwlog_err("%s: error\n", __func__);
		dc_fill_eh_buf(di->wldc_err_dsm_buff,
			sizeof(di->wldc_err_dsm_buff),
			WLDC_EH_GET_LS_VBUS, NULL);
	}
	return vbus;
}

int wldc_get_ls_ibus(void)
{
	int ret;
	int ibus = 0;
	struct wldc_dev_info *di = g_wldc_di;

	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return -ENODEV;
	}
	ret = dcm_get_ic_ibus(SC_MODE, di->ic_data.cur_type, &ibus);
	if (ret < 0) {
		di->stop_flag_error = true;
		hwlog_err("%s: error\n", __func__);
		dc_fill_eh_buf(di->wldc_err_dsm_buff,
			sizeof(di->wldc_err_dsm_buff),
			WLDC_EH_GET_LS_IBUS, NULL);
	}
	return ibus;
}

int wldc_get_error_cnt(void)
{
	struct wldc_dev_info *di = g_wldc_di;

	if (!di || !wldc_is_mode_para_valid(di, di->cur_dc_mode))
		return 0;

	return di->mode_para[di->cur_dc_mode].err_cnt;
}

int wldc_get_warning_cnt(void)
{
	struct wldc_dev_info *di = g_wldc_di;

	if (!di || !wldc_is_mode_para_valid(di, di->cur_dc_mode))
		return 0;

	return di->mode_para[di->cur_dc_mode].warn_cnt;
}

int wldc_get_tbatt_lth(void)
{
	struct wldc_dev_info *di = g_wldc_di;

	if (!di)
		return 0;

	return di->tbatt_lth;
}

int wldc_get_tbatt_hth(void)
{
	struct wldc_dev_info *di = g_wldc_di;

	if (!di)
		return 0;

	return di->tbatt_hth;
}

static void wldc_select_comp_param(struct wldc_dev_info *di)
{
	const char *ic_name = NULL;

	/* get vbat compensation of different ic */
	ic_name = dcm_get_ic_name(SC_MODE, CHARGE_IC_AUX);
	dc_get_vbat_comp_para(ic_name, &di->comp_para);
	memcpy(di->ic_check_info.vbat_comp, di->comp_para.vbat_comp,
		sizeof(di->comp_para.vbat_comp));
}

bool wldc_is_stop_charging_complete(void)
{
	struct wldc_dev_info *di = g_wldc_di;

	if (!di) {
		hwlog_err("%s: g_wldc_di null\n", __func__);
		return true;
	}

	return (di->dc_stop_flag == true);
}

void wldc_extra_power_supply(bool flag)
{
	int cur_dc_mode;
	enum wldc_ext_pwr_type pwr_type;
	struct wldc_dev_info *di = g_wldc_di;

	if (!di) {
		hwlog_err("%s: g_wldc_di null\n", __func__);
		return;
	}

	cur_dc_mode = di->cur_dc_mode;
	hwlog_info("[%s] cur dc_mode = 0x%x\n", __func__, cur_dc_mode);

	if ((cur_dc_mode < 0) || (cur_dc_mode >= di->total_dc_mode)) {
		hwlog_err("%s: cur_dc_mode 0x%x out of range [0, 0x%x)\n",
			__func__, cur_dc_mode, di->total_dc_mode);
		return;
	}

	pwr_type = di->mode_para[cur_dc_mode].init_para.ext_pwr_type;
	switch (pwr_type) {
	case WLDC_EXT_PWR_TO_SC_IC:
		boost_5v_enable(flag ? BOOST_5V_ENABLE : BOOST_5V_DISABLE, BOOST_CTRL_WLDC);
		break;
	case WLDC_EXT_PWR_TO_RX_IC:
		wlps_control(WLTRX_IC_MAIN, WLPS_RX_EXT_PWR, flag);
		break;
	default:
		break;
	}
}

static int wldc_restore_normal_charge_path(void)
{
	return charge_set_hiz_enable_by_direct_charge(HIZ_MODE_DISABLE);
}

int wldc_cut_off_normal_charge_path(void)
{
	return charge_set_hiz_enable_by_direct_charge(HIZ_MODE_ENABLE);
}

int wldc_turn_on_direct_charge_channel(void)
{
	if (g_wldc_need_wired_sw_on)
		return wired_chsw_set_wired_reverse_channel(
			WIRED_REVERSE_CHANNEL_RESTORE);
	else
		return 0;
}

static int wldc_turn_off_direct_charge_channel(void)
{
	if (g_wldc_need_wired_sw_on)
		return wired_chsw_set_wired_reverse_channel(
			WIRED_REVERSE_CHANNEL_CUTOFF);
	else
		return 0;
}

static bool wldc_is_volt_param_match(struct wldc_dev_info *di, int id)
{
	int tbatt = 0;
	const char *batt_brand = NULL;

	if (!wldc_is_mode_para_valid(di, di->cur_dc_mode))
		return false;

	batt_brand = power_supply_app_get_bat_brand();
	bat_temp_get_temperature(BAT_TEMP_MIXED, &tbatt);

	if (!di->orig_volt_para_p[id].bat_info.parse_ok)
		return false;
	if (!strstr(batt_brand,
		di->orig_volt_para_p[id].bat_info.batid))
		return false;
	if (tbatt < di->orig_volt_para_p[id].bat_info.tbatt_lth)
		return false;
	if (tbatt >= di->orig_volt_para_p[id].bat_info.tbatt_hth)
		return false;
	if (di->mode_para[di->cur_dc_mode].init_para.dc_type !=
		di->orig_volt_para_p[id].bat_info.dc_type)
		return false;
	return true;
}

static bool wldc_is_dflt_volt_param_match(struct wldc_dev_info *di, int id)
{
	if (strstr(di->orig_volt_para_p[id].bat_info.batid, "default") &&
		(di->mode_para[di->cur_dc_mode].init_para.dc_type ==
		di->orig_volt_para_p[id].bat_info.dc_type))
		return true;

	return false;
}

static void wldc_select_group_volt_param(struct wldc_dev_info *di)
{
	int i;
	int tbatt = 0;

	if (!wldc_is_mode_para_valid(di, di->cur_dc_mode))
		return;
	bat_temp_get_temperature(BAT_TEMP_MIXED, &tbatt);
	hwlog_info("[%s] tbatt:%d cur_dc_type:0x%x\n", __func__, tbatt,
		di->mode_para[di->cur_dc_mode].init_para.dc_type);

	for (i = 0; i < di->stage_group_size; i++) {
		if (wldc_is_volt_param_match(di, i))
			break;
	}
	if (i >= di->stage_group_size) {
		for (i = 0; i < di->stage_group_size; i++) {
			if (wldc_is_dflt_volt_param_match(di, i))
				break;
		}
	}
	if ((i >= di->stage_group_size) || (i < 0))
		i = 0;

	di->stage_size = di->orig_volt_para_p[i].stage_size;
	memcpy(di->volt_para, di->orig_volt_para_p[i].volt_info,
		sizeof(di->volt_para));
	memcpy(di->orig_volt_para, di->orig_volt_para_p[i].volt_info,
		sizeof(di->orig_volt_para));

	for (i = 0; i < di->stage_size; i++)
		hwlog_info("[%s] volt_para[%d], vbatt_th:%d icp_th:%d,%d\n",
			__func__, i, di->volt_para[i].vbatt_hth,
			di->volt_para[i].cp_iout_hth,
			di->volt_para[i].cp_iout_lth);
}

static void wldc_get_imax_by_tx_ability(struct wldc_dev_info *di)
{
	s64 tmp;
	int rx_ratio;
	int tx_eff;
	struct wlprot_acc_cap *acc_cap = wlrx_acc_get_cap(WLTRX_DRV_MAIN);

	if (!acc_cap) {
		hwlog_err("%s: acc_cap null\n", __func__);
		return;
	}
	rx_ratio = di->mode_para[di->cur_dc_mode].init_para.rx_ratio;
	tmp = (s64)(acc_cap->vmax / POWER_PERCENT) * acc_cap->imax * WLRX_ACC_TX_PWR_RATIO;
	tx_eff = wlrx_acc_get_tx_eff(WLTRX_DRV_MAIN);
	di->tx_imax = (int)tmp / (WLDC_VBAT_OVP_TH * rx_ratio * di->volt_ratio) * tx_eff / POWER_PERCENT;
	hwlog_info("[%s] tx_imax = %dmA\n", __func__, di->tx_imax);
}

static int wldc_get_imax_by_pwr_limit(struct wldc_dev_info *di)
{
	int vmax;
	int rx_ratio;

	rx_ratio = di->mode_para[di->cur_dc_mode].init_para.rx_ratio;
	vmax = WLDC_VBAT_OVP_TH * rx_ratio * di->volt_ratio;
	if (vmax > 0)
		return di->sysfs_data.pwr_limit / vmax;

	return 0;
}

static void wldc_notify_start_charging_evt(struct wldc_dev_info *di)
{
	int dc_type;

	if (!wldc_is_mode_para_valid(di, di->cur_dc_mode))
		return;

	dc_type = di->mode_para[di->cur_dc_mode].init_para.dc_type;
	power_event_bnc_notify(POWER_BNT_WLC, POWER_NE_WLC_DC_START_CHARGING, &dc_type);
}

void wldc_start_charging(struct wldc_dev_info *di)
{
	wldc_select_comp_param(di);
	wldc_select_group_volt_param(di);
	wldc_set_charge_stage(WLDC_STAGE_CHARGING);
	wldc_notify_start_charging_evt(di);
	wldc_get_imax_by_tx_ability(di);
	dcm_config_ic_watchdog(SC_MODE, di->ic_data.cur_type, WLDC_WDT_TIMEOUT);
	mod_delayed_work(system_wq, &di->wldc_ctrl_work,
		msecs_to_jiffies(di->ctrl_interval));
	mod_delayed_work(system_wq, &di->wldc_calc_work,
		msecs_to_jiffies(di->calc_interval));
}

static void wldc_rx_vset_volt_check(struct wldc_dev_info *di,
	int rx_vout, int rx_ratio, int rx_vmax)
{
	if ((di->max_vgap > 0) &&
		(di->rx_vout_set > rx_vout + di->max_vgap * rx_ratio))
		di->rx_vout_set = rx_vout + di->max_vgap * rx_ratio;
	if ((rx_vmax > 0) && (di->rx_vout_set > rx_vmax))
		di->rx_vout_set = rx_vmax;
}

static int wldc_update_regulation_data(struct wldc_dev_info *di,
	struct wldc_regulation_data *data)
{
	if (!wldc_is_mode_para_valid(di, di->cur_dc_mode))
		return -EINVAL;

	data->rx_ratio = di->mode_para[di->cur_dc_mode].init_para.rx_ratio;
	(void)wlrx_ic_get_vrect(WLTRX_IC_MAIN, &data->rx_vrect);
	(void)wlrx_ic_get_vout(WLTRX_IC_MAIN, &data->rx_vout);
	(void)wlrx_ic_get_iout(WLTRX_IC_MAIN, &data->rx_iout);
	wlrx_ic_get_imax(WLTRX_IC_MAIN, &data->rx_imax);
	wlrx_ic_get_iavg(WLTRX_IC_MAIN, &data->rx_iavg);
	data->cp_iout = wldc_cp_get_iout();
	data->cp_iout_avg = wldc_cp_get_iavg();
	data->ls_vbus = wldc_get_ls_vbus();
	data->ls_ibus = wldc_get_ls_ibus();
	data->ls_vbatt = wldc_get_bat_voltage();
	data->ls_ibatt = wldc_get_bat_current();
	data->rx_vmax = di->mode_para[di->cur_dc_mode].init_para.rx_vmax;

	hwlog_info("cur_stage=%d, ls: type=%d vbus=%d ibus=%d vbat=%d ibat=%d,\t"
		"rx: ratio=%d vrect=%d vout=%d iout=%d iavg=%d,\t"
		"cp: type=%d iout=%d iavg=%d vbat_th=%d cp_iout_hth=%d\t"
		"cp_iout_lth=%d cp_iout_err_th=%d\n",
		di->cur_stage, di->ic_data.cur_type, data->ls_vbus, data->ls_ibus,
		data->ls_vbatt, data->ls_ibatt, data->rx_ratio, data->rx_vrect,
		data->rx_vout, data->rx_iout, data->rx_iavg, di->cp_data.cur_type,
		data->cp_iout, data->cp_iout_avg, di->cur_vbat_hth,
		di->cur_cp_iout_hth, di->cur_cp_iout_lth, di->cp_iout_err_hth);

	return 0;
}

static void wldc_charge_regulation(struct wldc_dev_info *di)
{
	int ret;
	struct wldc_regulation_data data = { 0 };

	if (wldc_update_regulation_data(di, &data))
		return;

	if ((data.rx_iout > data.rx_imax + WLDC_DFT_IMAX_ERR_TH) ||
		(data.rx_iout > di->tx_imax + WLDC_DFT_IMAX_ERR_TH)) {
		di->rx_vout_set = data.rx_vout - di->vstep * data.rx_ratio;
		hwlog_info("[%s] rx_iout:%dmA>hth:%dmA or tx_imax:%dmA, decrease rx_vout\n",
			__func__, data.rx_iout, data.rx_imax, di->tx_imax);
		goto set_vout;
	}
	if (data.cp_iout_avg > di->cur_cp_iout_hth + di->cp_iout_err_hth) {
		di->rx_vout_set -= di->vstep * data.rx_ratio;
		hwlog_info("[%s] cp_iout_avg:%dmA>%dmA, decrease rx_vout\n",
			__func__, data.cp_iout_avg,
			di->cur_cp_iout_hth + di->cp_iout_err_hth);
		goto set_vout;
	}
	if (data.ls_vbatt > di->cur_vbat_hth) {
		di->rx_vout_set -= data.rx_ratio * di->volt_ratio *
			(data.ls_vbatt - di->cur_vbat_hth);
		hwlog_info("[%s] ls_vbatt:%dmV>%dmV, decrease rx_vout\n",
			__func__, data.ls_vbatt, di->cur_vbat_hth);
		goto set_vout;
	}
	if (data.ls_ibatt > di->ibat_max_hth) {
		di->rx_vout_set -= di->vstep * data.rx_ratio;
		hwlog_info("[%s] ls_ibatt:%dmA>%dmA, decrease rx_vout\n",
			__func__, data.ls_ibatt, di->ibat_max_hth);
		goto set_vout;
	}
	if (data.cp_iout_avg > di->cur_cp_iout_hth - di->cp_iout_err_hth)
		return;
	if (data.cp_iout > di->cur_cp_iout_hth - di->cp_iout_err_hth)
		return;
	if (data.ls_ibatt > di->ibat_min_lth)
		return;
	if (data.cp_iout_avg < di->cur_cp_iout_hth - di->cp_iout_err_hth) {
		di->rx_vout_set += di->vstep * data.rx_ratio;
		hwlog_info("[%s] cp_iout_avg:%dmA<%dmA, increase rx_vout\n",
			__func__, data.cp_iout_avg,
			di->cur_cp_iout_hth - di->cp_iout_err_hth);
		goto set_vout;
	}

set_vout:
	wldc_rx_vset_volt_check(di, data.rx_vout, data.rx_ratio, data.rx_vmax);
	ret = wireless_charge_set_rx_vout(di->rx_vout_set);
	if (ret)
		hwlog_err("%s: set_rx_vout fail\n", __func__);
}

static void wldc_select_charge_stage(struct wldc_dev_info *di)
{
	int vbatt;
	int cp_iout;
	int i;
	int cur_stage;
	int vbat_th;

	vbatt = wldc_get_bat_voltage();
	cp_iout = wldc_cp_get_iout();
	di->pre_stage = di->cur_stage;

	for (i = di->stage_size - 1; i >= 0; --i) {
		vbat_th = di->volt_para[i].vbatt_hth;
		if ((vbatt >= vbat_th) &&
			(cp_iout <= di->volt_para[i].cp_iout_lth)) {
			/* cc stage */
			cur_stage =
				WLDC_BATT_STG_TOTAL * (i + 1) + WLDC_CC_STAGE;
			break;
		} else if (vbatt >= vbat_th) {
			/* cv stage */
			cur_stage =
				WLDC_BATT_STG_TOTAL * i + WLDC_CV_STAGE;
			break;
		}
	}
	if (i < 0)
		cur_stage = 0;
	if (cur_stage >= di->pre_stage)
		di->cur_stage = cur_stage;
}

static bool wldc_abnormal_iout_check(struct wldc_dev_info *di)
{
	static int cnt;
	int cp_iout;
	int cp_iout_avg;
	int rx_ratio;

	rx_ratio = di->mode_para[di->cur_dc_mode].init_para.rx_ratio;
	cp_iout = wldc_cp_get_iout();
	cp_iout_avg = wldc_cp_get_iavg();
	if ((rx_ratio > WLDC_RATION_MODE) && (cp_iout < WLDC_IOUT_LTH) &&
		(cp_iout_avg < WLDC_IOUT_LTH)) {
		cnt++;
		hwlog_err("cp_iout_avg:%d cp_iout:%d\n", cp_iout_avg, cp_iout);
	} else {
		cnt = 0;
	}

	if (cnt >= WLDC_IOUT_AVG_LOW_CNT) {
		cnt = 0;
		return true;
	}

	return false;
}

static void wldc_check_exit_condition(struct wldc_dev_info *di)
{
	int cur_pid;
	int exp_pid;

	wireless_charge_update_max_vout_and_iout(true);
	if (wldc_is_tmp_forbidden(di, di->cur_dc_mode)) {
		di->stop_flag_info = true;
		return;
	}
	cur_pid = wlrx_pmode_get_curr_pid(WLTRX_DRV_MAIN);
	exp_pid = wlrx_pmode_get_exp_pid(WLTRX_DRV_MAIN, cur_pid);
	if (wlrx_pmode_judge(WLTRX_DRV_MAIN, exp_pid, WLRX_PMODE_FINAL_JUDGE)) {
		hwlog_err("%s: switch to expected pmode\n", __func__);
		wlrx_pmode_set_curr_pid(WLTRX_DRV_MAIN, exp_pid);
		di->stop_flag_info = true;
		return;
	}
	if (dcm_is_ic_close(SC_MODE, di->ic_data.cur_type)) {
		hwlog_err("%s: ls is close\n", __func__);
		di->stop_flag_warning = true;
		return;
	}
	if (wldc_abnormal_iout_check(di)) {
		hwlog_err("wldc abnormal iout_avg is low\n");
		di->stop_flag_warning = true;
		return;
	}
}

static void wldc_check_imax_by_multi_ic(struct wldc_dev_info *di)
{
	if ((di->ic_check_info.limit_current > 0) &&
		(di->ic_data.cur_type == CHARGE_MULTI_IC) &&
		(di->cur_cp_iout_hth > di->ic_check_info.limit_current))
		di->cur_cp_iout_hth = di->ic_check_info.limit_current;

	if ((di->ic_data.cur_type == CHARGE_IC_MAIN) &&
		(di->ic_data.single_ic_cp_iout_th > 0) &&
		(di->cur_cp_iout_hth > di->ic_data.single_ic_cp_iout_th))
		di->cur_cp_iout_hth = di->ic_data.single_ic_cp_iout_th;
}

static void wldc_check_imax_by_multi_cp(struct wldc_dev_info *di)
{
	if (!di->cp_data.support_multi_cp ||
		(di->cp_data.cur_type == CP_TYPE_MULTI))
		return;

	if ((di->cp_data.single_cp_iout_th > 0) &&
		(di->cur_cp_iout_hth > di->cp_data.single_cp_iout_th))
		di->cur_cp_iout_hth = di->cp_data.single_cp_iout_th;
}

static void wldc_check_intfr_imax(struct wldc_dev_info *di)
{
	int plimit_iout;

	if (wlrx_intfr_get_fixed_fop(WLTRX_DRV_MAIN) <= 0)
		return;

	plimit_iout = wlrx_intfr_get_irx(WLTRX_DRV_MAIN);
	if ((plimit_iout > 0) &&
		(di->cur_cp_iout_hth > plimit_iout * di->volt_ratio))
		di->cur_cp_iout_hth = plimit_iout * di->volt_ratio;
}

static void wldc_select_charge_param(struct wldc_dev_info *di)
{
	int stage_id;
	int tx_evt_ilim;
	int cp_iout_low;
	int rx_imax = 0;
	int cp_iin_thermal;
	int rx_ratio;
	int i_limit_by_pwr;
	static int original_iout_hth = 0;
	unsigned int idx;

	stage_id = di->cur_stage / WLDC_BATT_STG_TOTAL;
	if ((stage_id < 0) || (stage_id >= WLDC_VOLT_LEVEL)) {
		hwlog_err("%s: stage_id %d out of range\n", __func__, stage_id);
		return;
	}
	if ((di->stage_size < 1) || (di->stage_size > WLDC_VOLT_LEVEL)) {
		hwlog_err("%s: stage_size %d out of range\n",
			__func__, di->stage_size);
		return;
	}

	di->cur_vbat_hth = di->volt_para[stage_id].vbatt_hth;
	di->cur_cp_iout_hth = di->volt_para[stage_id].cp_iout_hth;
	di->cur_cp_iout_lth = di->volt_para[stage_id].cp_iout_lth;

	cp_iout_low = di->volt_para[di->stage_size - 1].cp_iout_lth;
	idx = (di->ic_data.cur_type == CHARGE_MULTI_IC) ?
		WLDC_DUAL_CHANNEL : WLDC_SINGLE_CHANNEL;
	cp_iin_thermal = di->sysfs_data.cp_iin_thermal_array[idx];
	if (cp_iin_thermal < cp_iout_low)
		cp_iin_thermal = cp_iout_low;

	wlrx_ic_get_imax(WLTRX_IC_MAIN, &rx_imax);
	rx_ratio = di->mode_para[di->cur_dc_mode].init_para.rx_ratio;
	if (di->cur_cp_iout_hth > rx_imax * rx_ratio)
		di->cur_cp_iout_hth = rx_imax * rx_ratio;
	if ((di->cur_cp_iout_hth > cp_iin_thermal) && !wlc_get_high_pwr_test_flag())
		di->cur_cp_iout_hth = cp_iin_thermal;
	if (di->cur_cp_iout_hth > di->tx_imax * rx_ratio)
		di->cur_cp_iout_hth = di->tx_imax * rx_ratio;

	wldc_check_intfr_imax(di);

	tx_evt_ilim = wlrx_get_alarm_ilim(WLDC_VBAT_OVP_TH * di->volt_ratio);
	if (tx_evt_ilim && (di->cur_cp_iout_hth > tx_evt_ilim))
		di->cur_cp_iout_hth = tx_evt_ilim;

	i_limit_by_pwr = wldc_get_imax_by_pwr_limit(di);
	di->cur_cp_iout_hth = di->cur_cp_iout_hth > i_limit_by_pwr ?
		i_limit_by_pwr : di->cur_cp_iout_hth;

	wldc_check_imax_by_multi_ic(di);
	wldc_check_imax_by_multi_cp(di);

	if (original_iout_hth != di->cur_cp_iout_hth) {
		hwlog_info("[select_param] stage_vbat_hth=%d stage_cp_iout_hth=%d stage_cp_iout_lth=%d "
			"rx_imax=%d cp_iin_thermal=%d tx_imax=%d tx_evt_ilim=%d i_limit_by_pwr=%d\n",
			di->volt_para[stage_id].vbatt_hth, di->volt_para[stage_id].cp_iout_hth,
			di->volt_para[stage_id].cp_iout_lth, rx_imax * rx_ratio, cp_iin_thermal,
			di->tx_imax * rx_ratio, tx_evt_ilim, i_limit_by_pwr);
		original_iout_hth = di->cur_cp_iout_hth;
	}
}

static int wldc_stop_charge_status_check(struct wldc_dev_info *di)
{
	int cur_mode = di->cur_dc_mode;
	int state = wlrx_get_wired_channel_state();

	if (!wldc_is_mode_para_valid(di, cur_mode)) {
		wldc_set_charge_stage(WLDC_STAGE_DEFAULT);
		return -1;
	}
	if (di->stop_flag_error)
		++di->mode_para[cur_mode].err_cnt;
	if (di->stop_flag_warning)
		++di->mode_para[cur_mode].warn_cnt;

	if ((state == WIRED_CHANNEL_ON) ||
		(wldc_get_charge_stage() == WLDC_STAGE_STOP_CHARGING)) {
		wldc_set_charge_stage(WLDC_STAGE_STOP_CHARGING);
	} else if (di->stop_flag_error || di->stop_flag_info ||
		di->stop_flag_warning || !di->sysfs_data.enable_charger ||
		di->force_disable) {
		wldc_set_charge_stage(WLDC_STAGE_DEFAULT);
	} else {
		wldc_set_charge_stage(WLDC_STAGE_CHARGE_DONE);
		di->mode_para[cur_mode].dis_flag |= WLDC_DIS_BY_CHRG_DONE;
	}

	return 0;
}

static void wldc_stop_limit_output_current(struct wldc_dev_info *di)
{
	int i;
	int rx_iout = 0;
	int rx_vout = 0;
	int rx_ratio = di->mode_para[di->cur_dc_mode].init_para.rx_ratio;

	(void)wlrx_ic_get_vout(WLTRX_IC_MAIN, &rx_vout);
	for (i = 0; i < 8; i++) { /* 8: limit current cnt */
		if (wlrx_get_wired_channel_state() == WIRED_CHANNEL_ON)
			break;
		rx_vout -= 150 * rx_ratio; /* 150mv: limit current step volt */
		if (wireless_charge_set_rx_vout(rx_vout))
			break;
		(void)power_msleep(DT_MSLEEP_50MS, 0, NULL);
		(void)wlrx_ic_get_vout(WLTRX_IC_MAIN, &rx_vout);
		(void)wlrx_ic_get_iout(WLTRX_IC_MAIN, &rx_iout);
		hwlog_info("[%s] [%d] rx_iout = %dmA rx_vout = %dmV\n",
			__func__, i, rx_iout, rx_vout);
		if (rx_iout < di->iout_drop_th)
			break;
	}
}

static void wldc_stop_output_check(struct wldc_dev_info *di)
{
	int vfc_reg = 0;
	int rx_iout = 0;
	int rx_init_vout;
	int state = wlrx_get_wired_channel_state();

	(void)wlrx_ic_get_iout(WLTRX_IC_MAIN, &rx_iout);
	if ((state != WIRED_CHANNEL_ON) && (di->iout_drop_th > 0) &&
		(rx_iout > di->iout_drop_th))
		wldc_stop_limit_output_current(di);

	(void)wlrx_ic_get_vfc_reg(WLTRX_IC_MAIN, &vfc_reg);
	rx_init_vout = di->mode_para[di->cur_dc_mode].init_para.rx_vout;
	if (vfc_reg > rx_init_vout)
		vfc_reg = rx_init_vout;
	wireless_charge_set_rx_vout(vfc_reg); /* in charging plugin otg ovp */
	power_usleep(DT_USLEEP_15MS); /* fall to target vout_reg volt */
}

void wldc_stop_charging(struct wldc_dev_info *di)
{
	int ret;

	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return;
	}

	hwlog_info("%s: ++\n", __func__);
	if (wldc_stop_charge_status_check(di))
		return;

	wldc_stop_output_check(di);
	ret = dcm_enable_ic(SC_MODE, di->ic_data.cur_type, DC_IC_DISABLE);
	if (ret)
		hwlog_err("%s: ls enable fail\n", __func__);
	ret = wldc_restore_normal_charge_path();
	if (ret)
		hwlog_err("%s: retore_normal_charge fail\n", __func__);
	ret = wldc_turn_off_direct_charge_channel();
	if (ret)
		hwlog_err("%s: close wired_reverse_channel fail\n", __func__);
	cancel_delayed_work_sync(&di->wldc_calc_work);
	if (wldc_get_charge_stage() != WLDC_STAGE_STOP_CHARGING)
		wireless_charge_restart_charging(WLRX_STAGE_REGULATION);
	dcm_config_ic_watchdog(SC_MODE, di->ic_data.cur_type, 0);
	ret = dcm_exit_ic(SC_MODE, di->ic_data.cur_type);
	if (ret)
		hwlog_err("%s: ls exit fail\n", __func__);
	ret = dcm_exit_batinfo(SC_MODE, di->ic_data.cur_type);
	if (ret)
		hwlog_err("%s: bi exit fail\n", __func__);
	wlps_control(WLTRX_IC_MAIN, WLPS_SC_SW2, false);
	wldc_extra_power_supply(false);
	wlc_ignore_vbus_only_event(false);
	wldc_ic_para_reset(di);
	wldc_cp_reset(di);
	di->stop_flag_error = 0;
	di->stop_flag_warning = 0;
	di->stop_flag_info = 0;
	di->cur_stage = 0;
	di->dc_stop_flag = true;
	hwlog_info("%s: --\n", __func__);
}

static void wldc_update_soh_para(struct wldc_dev_info *di)
{
	int i;
	int volt_max;
	int cur_max;
	bool print = false;

	if (!di)
		return;

	volt_max = di->orig_volt_para[di->stage_size - 1].vbatt_hth -
		di->vterm_dec;
	if (di->volt_para[di->stage_size - 1].vbatt_hth != volt_max) {
		di->volt_para[di->stage_size - 1].vbatt_hth = volt_max;
		print = true;
	}

	for (i = 0; i < di->stage_size - 1; i++)
		di->volt_para[i].vbatt_hth =
			(di->orig_volt_para[i].vbatt_hth < volt_max) ?
			di->orig_volt_para[i].vbatt_hth : volt_max;

	if (di->ichg_ratio == 0)
		di->ichg_ratio = BASP_PARA_SCALE;
	cur_max = di->orig_volt_para[0].cp_iout_hth *
		di->ichg_ratio / BASP_PARA_SCALE;
	if (di->volt_para[0].cp_iout_hth != cur_max) {
		di->volt_para[0].cp_iout_hth = cur_max;
		print = true;
	}
	di->volt_para[0].cp_iout_lth =
		di->orig_volt_para[0].cp_iout_lth;

	for (i = 1; i < di->stage_size; i++) {
		di->volt_para[i].cp_iout_hth =
			(di->orig_volt_para[i].cp_iout_hth <= cur_max) ?
			di->orig_volt_para[i].cp_iout_hth : cur_max;
		di->volt_para[i].cp_iout_lth =
			di->orig_volt_para[i].cp_iout_lth;
	}

	if (!print)
		return;

	hwlog_info("dc_volt_dec:%d, dc_cur_ratio:%d\n",
		di->vterm_dec, di->ichg_ratio);
	for (i = 0; i < di->stage_size; i++)
		hwlog_info("volt_para[%d],vbatt_th:%d icp_th:%d,%d\n",
			i, di->volt_para[i].vbatt_hth,
			di->volt_para[i].cp_iout_hth,
			di->volt_para[i].cp_iout_lth);
}

static int wldc_multi_ic_check(struct wldc_dev_info *di)
{
	int rx_ratio;

	if (!wldc_is_mode_para_valid(di, di->cur_dc_mode))
		return -1;

	rx_ratio = di->mode_para[di->cur_dc_mode].init_para.rx_ratio;
	if (mulit_ic_check(SC_MODE, di->ic_data.cur_type, &di->ic_check_info, rx_ratio) < 0) {
		di->ic_data.multi_ic_err_cnt++;
		return -1;
	}
	return 0;
}

static bool wldc_need_stop_work(struct wldc_dev_info *di)
{
	if (di->stop_flag_error || di->stop_flag_info ||
		di->stop_flag_warning || !di->sysfs_data.enable_charger ||
		di->force_disable) {
		hwlog_err("%s: stop charging, force_disable=%u\n",
			__func__, di->force_disable);
		return true;
	}
	return false;
}

void wldc_calc_work(struct work_struct *work)
{
	struct wldc_dev_info *di = container_of(work,
		struct wldc_dev_info, wldc_calc_work.work);

	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return;
	}
	if (wldc_need_stop_work(di)) {
		hwlog_err("%s: stop calc work\n", __func__);
		return;
	}
	wldc_check_exit_condition(di);
	wldc_update_soh_para(di);
	wldc_select_charge_stage(di);
	wldc_select_charge_param(di);
	if (di->cur_stage == di->stage_size * WLDC_BATT_STG_TOTAL) {
		hwlog_info("%s: direct charge done\n", __func__);
		return;
	}
	if (wldc_get_charge_stage() == WLDC_STAGE_STOP_CHARGING) {
		hwlog_info("%s: tx disconnect\n", __func__);
		return;
	}
	mod_delayed_work(system_wq, &di->wldc_calc_work,
		msecs_to_jiffies(di->calc_interval));
}

void wldc_control_work(struct work_struct *work)
{
	struct wldc_dev_info *di = container_of(work,
		struct wldc_dev_info, wldc_ctrl_work.work);

	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return;
	}

	if (wldc_need_stop_work(di)) {
		wldc_stop_charging(di);
		return;
	}
	if (wldc_get_charge_stage() == WLDC_STAGE_STOP_CHARGING) {
		hwlog_info("%s: tx disconnect\n", __func__);
		di->stop_flag_info = true;
		wldc_stop_charging(di);
		return;
	}
	if (di->cur_stage == di->stage_size * WLDC_BATT_STG_TOTAL) {
		hwlog_info("%s: wireless direct charge done\n", __func__);
		wldc_stop_charging(di);
		return;
	}
	if (wldc_multi_ic_check(di) < 0) {
		di->stop_flag_error = true;
		wldc_stop_charging(di);
		return;
	}

	wldc_ic_select_charge_path(di);
	wldc_cp_select_charge_path(di);
	wldc_charge_regulation(di);
	dcm_kick_ic_watchdog(SC_MODE, di->ic_data.cur_type);

	mod_delayed_work(system_wq, &di->wldc_ctrl_work,
		msecs_to_jiffies(di->ctrl_interval));
}

static void wldc_para_reset(struct wldc_dev_info *di)
{
	int i;

	for (i = 0; i < di->total_dc_mode; i++) {
		di->mode_para[i].dis_flag = WLDC_EN;
		di->mode_para[i].dmd_report_flag = false;
		di->mode_para[i].dc_open_retry_cnt = 0;
		di->mode_para[i].err_cnt = 0;
		di->mode_para[i].warn_cnt = 0;
	}
	di->cur_stage = 0;
	di->tbatt_lth = di->tbat_limit.lth;
	di->tbatt_hth = di->tbat_limit.hth;
	dc_clean_eh_buf(di->wldc_err_dsm_buff, sizeof(di->wldc_err_dsm_buff));
	wldc_set_charge_stage(WLDC_STAGE_STOP_CHARGING);
}

void wldc_tx_disconnect_handler(void)
{
	struct wldc_dev_info *sc_di = NULL;

	wireless_sc_get_di(&sc_di);

	if (!sc_di) {
		hwlog_err("sc_di null\n");
		return;
	}
	wldc_para_reset(sc_di);
}

static int wldc_parse_volt_para(struct device_node *np,
	struct wldc_dev_info *di, int group)
{
	int i;
	int row;
	int col;
	int array_len;
	int idata;
	int j = group;
	const char *tmp_str = NULL;
	char *volt_para = NULL;

	volt_para = di->orig_volt_para_p[j].bat_info.volt_para_id;

	array_len = power_dts_read_count_strings(power_dts_tag(HWLOG_TAG), np,
		volt_para, WLDC_VOLT_LEVEL, WLDC_VOLT_INFO_TOTAL);
	if (array_len < 0)
		return -EINVAL;
	di->orig_volt_para_p[j].stage_size = array_len / WLDC_VOLT_INFO_TOTAL;

	for (i = 0; i < array_len; i++) {
		if (power_dts_read_string_index(power_dts_tag(HWLOG_TAG),
			np, volt_para, i, &tmp_str))
			return -EINVAL;

		if (kstrtoint(tmp_str, POWER_BASE_DEC, &idata)) {
			hwlog_err("%s: get kstrtoint fail\n", __func__);
			return -EINVAL;
		}

		row = i / WLDC_VOLT_INFO_TOTAL;
		col = i % WLDC_VOLT_INFO_TOTAL;

		switch (col) {
		case WLDC_PARA_VBATT_HTH:
			di->orig_volt_para_p[j].volt_info[row].vbatt_hth =
				idata;
			break;

		case WLDC_PARA_CP_IOUT_HTH:
			di->orig_volt_para_p[j].volt_info[row].cp_iout_hth =
				idata;
			break;

		case WLDC_PARA_CP_IOUT_LTH:
			di->orig_volt_para_p[j].volt_info[row].cp_iout_lth =
				idata;
			break;
		default:
			break;
		}
	}

	di->orig_volt_para_p[j].bat_info.parse_ok = 1;

	for (i = 0; i < di->orig_volt_para_p[j].stage_size; i++)
		hwlog_info("%s[%d] vbatt_th:%dmV icp_th:%d,%d mA\n", volt_para,
			i, di->orig_volt_para_p[j].volt_info[i].vbatt_hth,
			di->orig_volt_para_p[j].volt_info[i].cp_iout_hth,
			di->orig_volt_para_p[j].volt_info[i].cp_iout_lth);

	return 0;
}

static void wldc_parse_group_volt_para(struct device_node *np,
	struct wldc_dev_info *di)
{
	int i;

	di->stage_size = 0;

	for (i = 0; i < di->stage_group_size; i++) {
		if (wldc_parse_volt_para(np, di, i))
			return;
	}

	di->stage_size = di->orig_volt_para_p[0].stage_size;
}

static void wldc_parse_batt_para(struct device_node *np,
	struct wldc_dev_info *di)
{
	int i;
	int row;
	int col;
	int ret;
	int array_len;
	int idata;
	const char *tmp_str = NULL;

	array_len = power_dts_read_count_strings(power_dts_tag(HWLOG_TAG), np,
		"bat_para", WLDC_VOLT_GROUP_MAX, WLDC_BAT_INFO_TOTAL);
	if (array_len < 0) {
		di->stage_group_size = 1;
		di->orig_volt_para_p[0].bat_info.tbatt_hth = di->tbat_limit.hth;
		di->orig_volt_para_p[0].bat_info.tbatt_lth = di->tbat_limit.lth;
		strncpy(di->orig_volt_para_p[0].bat_info.batid,
			"default", WLDC_BAT_BRAND_LEN_MAX - 1);
		strncpy(di->orig_volt_para_p[0].bat_info.volt_para_id,
			"volt_para00", WLDC_VOLT_NODE_LEN_MAX - 1);
		return;
	}

	di->stage_group_size = array_len / WLDC_BAT_INFO_TOTAL;

	for (i = 0; i < array_len; i++) {
		if (power_dts_read_string_index(power_dts_tag(HWLOG_TAG),
			np, "bat_para", i, &tmp_str))
			return;

		row = i / WLDC_BAT_INFO_TOTAL;
		col = i % WLDC_BAT_INFO_TOTAL;

		switch (col) {
		case WLDC_BAT_ID:
			strncpy(di->orig_volt_para_p[row].bat_info.batid,
				tmp_str, WLDC_BAT_BRAND_LEN_MAX - 1);
			break;

		case WLDC_BAT_TEMP_LOW:
			ret = kstrtoint(tmp_str, POWER_BASE_DEC, &idata);
			if (ret) {
				hwlog_err("%s: kstrtoint tbatt_lth fail\n",
					__func__);
				return;
			}
			if ((idata < di->tbat_limit.lth) ||
				(idata > di->tbat_limit.hth)) {
				hwlog_err("%s:tbatt_low %d invalid\n",
					__func__, idata);
				return;
			}
			di->orig_volt_para_p[row].bat_info.tbatt_lth = idata;
			break;

		case WLDC_BAT_TEMP_HIGH:
			ret = kstrtoint(tmp_str, POWER_BASE_DEC, &idata);
			if (ret) {
				hwlog_err("%s: kstrtoint tbatt_hth fail\n",
					__func__);
				return;
			}
			if ((idata < di->tbat_limit.lth) ||
				(idata > di->tbat_limit.hth)) {
				hwlog_err("%s:tbatt_high %d invalid\n",
					__func__, idata);
				return;
			}
			di->orig_volt_para_p[row].bat_info.tbatt_hth = idata;
			break;

		case WLDC_BAT_DC_TYPE:
			ret = kstrtoint(tmp_str, POWER_BASE_HEX, &idata);
			if (ret) {
				hwlog_err("%s: kstrtoint dc_type fail\n",
					__func__);
				return;
			}
			if ((idata < WLRX_SUPP_PMODE_BUCK) ||
				(idata > WLRX_SUPP_PMODE_ALL)) {
				hwlog_err("%s:dc_type %d invalid\n",
					__func__, idata);
				return;
			}
			di->orig_volt_para_p[row].bat_info.dc_type = idata;
			break;
		case WLDC_BAT_PARA_INDEX:
			strncpy(di->orig_volt_para_p[row].bat_info.volt_para_id,
				tmp_str, WLDC_VOLT_NODE_LEN_MAX - 1);
			break;
		default:
			break;
		}
	}

	for (i = 0; i < di->stage_group_size; i++)
		hwlog_info("bat_para[%d]=%d %d 0x%x %s\n", i,
			di->orig_volt_para_p[i].bat_info.tbatt_lth,
			di->orig_volt_para_p[i].bat_info.tbatt_hth,
			di->orig_volt_para_p[i].bat_info.dc_type,
			di->orig_volt_para_p[i].bat_info.volt_para_id);
}

static int wldc_pmode_cfg_str2int(const char *str, int *pcfg, int offset, int col)
{
	if (kstrtoint(str, 0, &pcfg[offset % col]))
		return -EINVAL;

	return 0;
}

static void wldc_print_mode_para(struct wldc_dev_info *di)
{
	int i;
	struct wldc_init_para *para = NULL;

	for (i = 0; i < di->total_dc_mode; i++) {
		para = &di->mode_para[i].init_para;
		hwlog_info("mode_para[%d] type:0x%x name:%s ext_pwr_type:0x%x\t"
			"ratio:%d vbatt_min:%-4d vbatt_max:%-4d rx_vout:%-5d\t"
			"rx_vout_th:%-3d vdiff_th:%-4d ileak_th:%-3d\t"
			"vdelt:%-3d rx_vmax:%-3d tx_vout:%-5d\n", i,
			para->dc_type, para->dc_name, para->ext_pwr_type,
			para->rx_ratio, para->vbatt_min, para->vbatt_max,
			para->rx_vout, para->rx_vout_th, para->vdiff_th,
			para->ileak_th, para->vdelt, para->rx_vmax, para->tx_vout);
	}
}

static void wldc_revise_mode_para(struct wldc_dev_info *di)
{
	int i;
	struct wldc_init_para *p_init = NULL;

	for (i = 0; i < di->total_dc_mode; i++) {
		di->mode_para[i].dis_flag = WLDC_EN;
		p_init = &di->mode_para[i].init_para;
		if (p_init->tx_vout <= 0)
			p_init->tx_vout = p_init->rx_vout;
	}
}

static void wldc_parse_mode_para(struct device_node *np, struct wldc_dev_info *di)
{
	int i, row, col, len;
	const char *tmp_str = NULL;

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"init_para_col", (u32 *)&col, WLDC_PMODE_CFG_COL);
	len = power_dts_read_count_strings(power_dts_tag(HWLOG_TAG), np,
		"init_para", WLDC_PMODE_CFG_ROW, col);
	if (len <= 0)
		return;

	di->mode_para = kcalloc(len / col, sizeof(*(di->mode_para)), GFP_KERNEL);
	if (!di->mode_para) {
		hwlog_err("%s: alloc wldc_mode_para fail\n", __func__);
		return;
	}
	di->total_dc_mode = len / col;
	for (i = 0; i < len; i++) {
		if (power_dts_read_string_index(power_dts_tag(HWLOG_TAG), np,
			"init_para", i, &tmp_str))
			goto parse_err;
		row = i / col;
		if ((i % col) == 0) { /* 0: pmode type */
			if (kstrtoint(tmp_str, 0, &di->mode_para[row].init_para.dc_type))
				goto parse_err;
			continue;
		}
		if ((i % col) == 1) { /* 1: pmode name */
			di->mode_para[row].init_para.dc_name = tmp_str;
			continue;
		}
		if (wldc_pmode_cfg_str2int(tmp_str, /* length: 1char*=2int */
			(int *)&di->mode_para[row].init_para.ext_pwr_type, i - 2, col))
			goto parse_err;
	}

	wldc_revise_mode_para(di);
	wldc_print_mode_para(di);
	return;

parse_err:
	kfree(di->mode_para);
	di->mode_para = NULL;
}

static void wldc_parse_basic_para(struct device_node *np,
	struct wldc_dev_info *di)
{
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"need_wired_sw_on", &g_wldc_need_wired_sw_on,
		WLDC_NEED_WIRED_SW_ON);

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"cp_iout_err_hth", &di->cp_iout_err_hth,
		WLDC_DEFAULT_IOUT_ERR_TH);

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "ibat_max_hth",
		&di->ibat_max_hth, WLDC_DEFAULT_IBAT_MAX_TH);

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "ibat_min_lth",
		&di->ibat_min_lth, WLDC_DEFAULT_IBAT_MIN_TH);

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "volt_ratio",
		&di->volt_ratio, SC_DEFAULT_VOLT_RATIO);

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "ctrl_interval",
		&di->ctrl_interval, WLDC_DEFAULT_CTRL_INTERVAL);

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "calc_interval",
		&di->calc_interval, WLDC_DEFAULT_CALC_INTERVAL);

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "vstep",
		&di->vstep, WLDC_DEFAULT_VSTEP);

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "max_vgap",
		&di->max_vgap, 0);

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "iout_drop_th",
		&di->iout_drop_th, 0);
}

static void wldc_parse_tbat_limit_para(struct device_node *np,
	struct wldc_dev_info *di)
{
	u32 temp[WLDC_TBAT_LIMIT_TOTAL] = { 0 };

	if (power_dts_read_u32_array(power_dts_tag(HWLOG_TAG), np,
		"tbat_limit_para", temp, WLDC_TBAT_LIMIT_TOTAL)) {
		di->tbat_limit.lth = 10; /* 10: tbatt limit low threshold */
		di->tbat_limit.lth_back = 13; /* 13: low back threshold */
		di->tbat_limit.hth = 45; /* 45: tbatt limit high threshold */
		di->tbat_limit.hth_back = 42; /* 42: high back threshold */
		hwlog_info("[tbat_limit] use default para\n");
	} else {
		di->tbat_limit.lth = (int)temp[WLDC_TBAT_LIMIT_LTH];
		di->tbat_limit.lth_back = (int)temp[WLDC_TBAT_LIMIT_LTH_BACK];
		di->tbat_limit.hth = (int)temp[WLDC_TBAT_LIMIT_HTH];
		di->tbat_limit.hth_back = (int)temp[WLDC_TBAT_LIMIT_HTH_BACK];
	}

	di->tbatt_lth = di->tbat_limit.lth;
	di->tbatt_hth = di->tbat_limit.hth;

	hwlog_info("[tbat_limit] lth:%d, lth_back:%d, hth:%d, hth_back:%d\n",
		di->tbat_limit.lth, di->tbat_limit.lth_back,
		di->tbat_limit.hth, di->tbat_limit.hth_back);
}

static void wldc_parse_multi_ic_para(struct device_node *np,
	struct wldc_dev_info *di)
{
	u32 tmp_para[WLDC_MULTI_IC_TOTAL] = { 0 };

	if (power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "support_multi_ic",
		(u32 *)&di->ic_mode_para.support_multi_ic, 0))
		return;

	power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "single_ic_cp_iout_th",
		(u32 *)&di->ic_data.single_ic_cp_iout_th, 0);

	if (!di->ic_mode_para.support_multi_ic)
		return;

	if (power_dts_read_u32_array(power_dts_tag(HWLOG_TAG), np,
		"multi_ic_para", tmp_para, WLDC_MULTI_IC_TOTAL)) {
		di->ic_mode_para.support_multi_ic = 0;
		return;
	}

	di->ic_data.ibus_para.hth = (int)tmp_para[WLDC_MULTI_IC_IBAT_HTH];
	di->ic_data.ibus_para.h_time = (int)tmp_para[WLDC_MULTI_IC_HTIME];
	di->ic_data.ibus_para.lth = (int)tmp_para[WLDC_MULTI_IC_IBAT_LTH];
	di->ic_data.ibus_para.l_time = (int)tmp_para[WLDC_MULTI_IC_LTIME];
	hwlog_info("ic_para ibus_hth:%dmA h_time:%d ibus_lth:%dmA l_time:%d\n",
		di->ic_data.ibus_para.hth, di->ic_data.ibus_para.h_time,
		di->ic_data.ibus_para.lth, di->ic_data.ibus_para.l_time);

	multi_ic_parse_check_para(np, &di->ic_check_info);
}

void wldc_parse_dts(struct device_node *np, struct wldc_dev_info *di)
{
	if (!np || !di)
		return;

	wldc_parse_basic_para(np, di);
	wldc_parse_mode_para(np, di);
	wldc_parse_tbat_limit_para(np, di);
	wldc_parse_batt_para(np, di);
	wldc_parse_group_volt_para(np, di);
	wldc_parse_multi_ic_para(np, di);
	wldc_parse_multi_cp_para(np, di);
	dc_comp_parse_dts(np, &di->comp_para);
}
