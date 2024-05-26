// SPDX-License-Identifier: GPL-2.0
/*
 * wireless_rx_common.c
 *
 * rx common interface of wireless charging
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

#include <chipset_common/hwpower/wireless_charge/wireless_rx_common.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_ic_intf.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_acc.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_status.h>
#include <chipset_common/hwpower/accessory/wireless_lightstrap.h>
#include <chipset_common/hwpower/hardware_channel/wired_channel_switch.h>
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_dsm.h>
#include <chipset_common/hwpower/battery/battery_temp.h>
#include <huawei_platform/usb/usb_extra_modem.h>
#include <linux/kernel.h>
#include <linux/of.h>

#define HWLOG_TAG wireless_rx_common
HWLOG_REGIST();

/* existing definitions mustn't be modified */
#define WLRX_WIRED_CHSW_ALL_CUTOFF            0
#define WLRX_WIRED_CHSW_NON_BUCK_CUTOFF       1

struct wlrx_common_dts {
	int wired_chsw_cutoff_type;
};

struct wlrx_common_dev {
	struct wlrx_common_dts *dts;
};

static struct wlrx_common_dev *g_rx_common_di[WLTRX_DRV_MAX];

static struct wlrx_common_dev *wlrx_common_get_di(unsigned int drv_type)
{
	if (!wltrx_is_drv_type_valid(drv_type)) {
		hwlog_err("get_di: drv_type=%u err\n", drv_type);
		return NULL;
	}

	return g_rx_common_di[drv_type];
}

enum wlrx_scene wlrx_get_scene(void)
{
	if (lightstrap_online_state())
		return WLRX_SCN_LIGHTSTRAP;
	if (uem_check_online_status())
		return WLRX_SCN_UEM;

	return WLRX_SCN_NORMAL;
}

static void wlrx_cut_off_all_wired_channel(void)
{
	if (!wdcm_dev_exist()) {
		wired_chsw_set_wired_channel(WIRED_CHANNEL_ALL, WIRED_CHANNEL_CUTOFF);
		return;
	}

	wired_chsw_set_other_wired_channel(WIRED_CHANNEL_BUCK, WIRED_CHANNEL_CUTOFF);
	wdcm_set_buck_channel_state(WDCM_CLIENT_WLS, WDCM_DEV_ON);
}

static void wlrx_cut_off_nonbuck_wired_channel(void)
{
	if (!wdcm_dev_exist()) {
		wired_chsw_set_other_wired_channel(WIRED_CHANNEL_BUCK, WIRED_CHANNEL_CUTOFF);
		return;
	}

	wired_chsw_set_other_wired_channel(WIRED_CHANNEL_BUCK, WIRED_CHANNEL_CUTOFF);
	wdcm_set_buck_channel_state(WDCM_CLIENT_WLS, WDCM_DEV_ON);
}

void wlrx_cut_off_wired_channel(unsigned int drv_type)
{
	struct wlrx_common_dev *di = wlrx_common_get_di(drv_type);

	if (!di)
		return;

	switch (di->dts->wired_chsw_cutoff_type) {
	case WLRX_WIRED_CHSW_ALL_CUTOFF:
		hwlog_info("[cut_off_wired_channel] all cut_off\n");
		wlrx_cut_off_all_wired_channel();
		break;
	case WLRX_WIRED_CHSW_NON_BUCK_CUTOFF:
		hwlog_info("[cut_off_wired_channel] non_buck cut_off\n");
		wlrx_cut_off_nonbuck_wired_channel();
		break;
	default:
		break;
	}
}

static void wlrx_dsm_dump(unsigned int drv_type, char *dsm_buff, size_t buff_size)
{
	int tbatt = 0;
	char buf[ERR_NO_STRING_SIZE] = { 0 };
	int soc = power_supply_app_get_bat_capacity();
	int vrect = 0;
	int vout = 0;
	int iout = 0;
	int iavg = 0;
	unsigned int ic_type;
	size_t len;

	ic_type = wltrx_get_dflt_ic_type(drv_type);
	(void)wlrx_ic_get_vrect(ic_type, &vrect);
	(void)wlrx_ic_get_vout(ic_type, &vout);
	(void)wlrx_ic_get_iout(ic_type, &iout);
	wlrx_ic_get_iavg(ic_type, &iavg);
	(void)bat_temp_get_temperature(BAT_TEMP_MIXED, &tbatt);

	snprintf(buf, sizeof(buf), "soc=%d,vrect=%dmV,vout=%dmV,iout=%dmA,iavg=%dmA,tbat=%d\n",
		soc, vrect, vout, iout, iavg, tbatt);
	len = buff_size < (strlen(buf) + 1) ? buff_size : (strlen(buf) + 1);
	strncat(dsm_buff, buf, len);
}

void wlrx_dsm_report(unsigned int drv_type, int err_no, char *dsm_buff, size_t buff_size)
{
	if (!wltrx_is_drv_type_valid(drv_type) || !dsm_buff)
		return;

	if (wlrx_is_qc_adp(drv_type))
		return;

	(void)power_msleep(DT_MSLEEP_100MS, 0, NULL);
	if (wlrx_get_wireless_channel_state() == WIRELESS_CHANNEL_ON) {
		wlrx_dsm_dump(drv_type, dsm_buff, buff_size);
		power_dsm_report_dmd(POWER_DSM_BATTERY, err_no, dsm_buff);
	}
}

static int wlrx_common_parse_dts(const struct device_node *np, struct wlrx_common_dev *di)
{
	di->dts = kzalloc(sizeof(*di->dts), GFP_KERNEL);
	if (!di->dts)
		return -ENOMEM;

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "wired_chsw_cutoff_type",
		(u32 *)&di->dts->wired_chsw_cutoff_type, WLRX_WIRED_CHSW_ALL_CUTOFF);

	return 0;
}

static void wlrx_common_kfree_dev(struct wlrx_common_dev *di)
{
	if (!di)
		return;

	kfree(di->dts);
	kfree(di);
}

int wlrx_common_init(unsigned int drv_type, struct device *dev)
{
	int ret;
	struct wlrx_common_dev *di = NULL;

	if (!dev || !wltrx_is_drv_type_valid(drv_type))
		return -ENODEV;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	ret = wlrx_common_parse_dts(dev->of_node, di);
	if (ret)
		goto exit;

	g_rx_common_di[drv_type] = di;
	return 0;

exit:
	wlrx_common_kfree_dev(di);
	return ret;
}

void wlrx_common_deinit(unsigned int drv_type)
{
	if (!wltrx_is_drv_type_valid(drv_type))
		return;

	wlrx_common_kfree_dev(g_rx_common_di[drv_type]);
	g_rx_common_di[drv_type] = NULL;
}
