// SPDX-License-Identifier: GPL-2.0
/*
 * wireless_rx_monitor.c
 *
 * monitor info for wireless charging
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

#include <chipset_common/hwpower/wireless_charge/wireless_rx_monitor.h>
#include <huawei_platform/hwpower/wireless_charge/wireless_rx_platform.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_status.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_acc.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_pmode.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_plim.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_ic_intf.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_alarm.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_interfere.h>
#include <huawei_platform/power/wireless/wireless_direct_charger.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <huawei_platform/hwpower/common_module/power_platform.h>
#include <chipset_common/hwpower/battery/battery_temp.h>

#define HWLOG_TAG wireless_rx_monitor
HWLOG_REGIST();

#define WLRX_MON_LOG_INTERVAL        5000 /* 5s */

struct wlrx_mon_info {
	/* for rx_ic */
	int vrect;
	int vout;
	int iout;
	int fop;
	int temp;
	int cep;
	int imax;
	int iavg;
	/* for system */
	int soc;
	int tbatt;
	int pid; /* pmode index */
	unsigned int plim_src;
	/* for dc */
	int dc_err;
	int dc_warning;
	int tbatt_lth;
	int tbatt_hth;
	/* for buck */
	int buck_iin_reg;
	/* for acc */
	int palarm;
	/* for interference setting */
	unsigned int intfr_src;
	int intfr_vtx;
	int intfr_vrx;
	int intfr_irx;
	int intfr_fop;
};

static int g_wlrx_mon_interval = WLRX_MON_INTERVAL;

int wlrx_get_monitor_interval(void)
{
	return g_wlrx_mon_interval;
}

static unsigned int wlrx_mon_get_ic_type(unsigned int drv_type)
{
	return wltrx_get_dflt_ic_type(drv_type);
}

static void wlrx_mon_update_sys_info(unsigned int drv_type, struct wlrx_mon_info *info)
{
	(void)bat_temp_get_temperature(BAT_TEMP_MIXED, &info->tbatt);
	info->soc = power_platform_get_battery_ui_capacity();
	info->pid = wlrx_pmode_get_curr_pid(drv_type);
	info->plim_src = wlrx_plim_get_src(drv_type);
}

static void wlrx_mon_update_ic_info(unsigned int drv_type, struct wlrx_mon_info *info)
{
	unsigned int ic_type = wlrx_mon_get_ic_type(drv_type);

	wlrx_ic_get_imax(ic_type, &info->imax);
	wlrx_ic_get_iavg(ic_type, &info->iavg);
	(void)wlrx_ic_get_vrect(ic_type, &info->vrect);
	(void)wlrx_ic_get_vout(ic_type, &info->vout);
	(void)wlrx_ic_get_iout(ic_type, &info->iout);
	(void)wlrx_ic_get_fop(ic_type, &info->fop);
	(void)wlrx_ic_get_temp(ic_type, &info->temp);
	(void)wlrx_ic_get_cep(ic_type, &info->cep);
}

static void wlrx_mon_update_acc_info(unsigned int drv_type, struct wlrx_mon_info *info)
{
	info->palarm = wlrx_get_alarm_plim();
}

static void wlrx_mon_update_buck_info(unsigned int drv_type, struct wlrx_mon_info *info)
{
	info->buck_iin_reg = wlrx_buck_get_iin_regval(drv_type);
}

static void wlrx_mon_update_dc_info(unsigned int drv_type, struct wlrx_mon_info *info)
{
	info->dc_err = wldc_get_error_cnt();
	info->dc_warning = wldc_get_warning_cnt();
	info->tbatt_lth = wldc_get_tbatt_lth();
	info->tbatt_hth = wldc_get_tbatt_hth();
}

static void wlrx_mon_update_sysfs_info(unsigned int drv_type, struct wlrx_mon_info *info)
{
	info->intfr_src = wlrx_intfr_get_src(WLTRX_DRV_MAIN);
	info->intfr_vtx = wlrx_intfr_get_vtx(WLTRX_DRV_MAIN);
	info->intfr_vrx = wlrx_intfr_get_vrx(WLTRX_DRV_MAIN);
	info->intfr_irx = wlrx_intfr_get_irx(WLTRX_DRV_MAIN);
	info->intfr_fop = wlrx_intfr_get_fixed_fop(WLTRX_DRV_MAIN);
}

static void wlrx_mon_print_info(struct wlrx_mon_info *info)
{
	hwlog_info("[monitor_info] [sys]soc:%-3d tbatt:%d pid:%d plim_src:0x%02x\t"
		"[dc] warn:%d err:%d tbatt_lth:%d tbatt_hth:%d [tx] palarm:%d\t"
		"[rx] vrect:%-5d vout:%-5d iout:%-4d fop:%-3d cep:%-3d imax:%-4d iavg:%-4d temp:%-3d\t"
		"[buck] iin_reg:%-4d [intfr] src:0x%02x vtx:%-5d vrx:%-5d irx:%-4d fop:%-3d\n",
		info->soc, info->tbatt, info->pid, info->plim_src,
		info->dc_warning, info->dc_err, info->tbatt_lth, info->tbatt_hth, info->palarm,
		info->vrect, info->vout, info->iout, info->fop, info->cep,
		info->imax, info->iavg, info->temp, info->buck_iin_reg,
		info->intfr_src, info->intfr_vtx, info->intfr_vrx, info->intfr_irx, info->intfr_fop);
}

static bool wlrx_mon_need_show_info(unsigned int drv_type)
{
	int buck_ireg;
	static int buck_ireg_last;
	static int cnt;

	if (wlrx_is_fac_tx(WLTRX_DRV_MAIN))
		return true;
	if (wlrx_get_charge_stage() < WLRX_STAGE_CHARGING)
		return true;
	if (!wlrx_bst_rst_completed())
		return true;

	if (++cnt == WLRX_MON_LOG_INTERVAL / g_wlrx_mon_interval) {
		cnt = 0;
		return true;
	}

	buck_ireg = wlrx_buck_get_iin_regval(drv_type);
	if (buck_ireg_last != buck_ireg) {
		buck_ireg_last = buck_ireg;
		return true;
	}

	return false;
}

void wlrx_mon_show_info(unsigned int drv_type)
{
	struct wlrx_mon_info info;

	if (!wlrx_mon_need_show_info(drv_type))
		return;

	memset(&info, 0, sizeof(info));
	wlrx_mon_update_sys_info(drv_type, &info);
	wlrx_mon_update_ic_info(drv_type, &info);
	wlrx_mon_update_acc_info(drv_type, &info);
	wlrx_mon_update_buck_info(drv_type, &info);
	wlrx_mon_update_dc_info(drv_type, &info);
	wlrx_mon_update_sysfs_info(drv_type, &info);
	wlrx_mon_print_info(&info);
}
