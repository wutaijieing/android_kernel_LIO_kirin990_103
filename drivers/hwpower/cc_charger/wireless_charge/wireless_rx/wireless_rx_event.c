// SPDX-License-Identifier: GPL-2.0
/*
 * wireless_rx_event.c
 *
 * handle events received from rx, acc and other modues for wireless charging
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

#include <chipset_common/hwpower/wireless_charge/wireless_trx_intf.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_common.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_status.h>
#include <chipset_common/hwpower/hardware_channel/charger_channel.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/common_module/power_wakeup.h>
#include <chipset_common/hwpower/common_module/power_event_ne.h>
#include <chipset_common/hwpower/common_module/power_dsm.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

#define HWLOG_TAG wireless_rx_evt
HWLOG_REGIST();

#define WLRX_OCP_CNT_MAX            3
#define WLRX_OVP_CNT_MAX            3
#define WLRX_OTP_CNT_MAX            3

struct wlrx_evt_dev {
	struct notifier_block rxic_nb;
	struct wakeup_source *rxic_wakelock;
	struct delayed_work rxic_work;
	unsigned int drv_type;
	unsigned long evt_type;
	unsigned int ocp_cnt;
	unsigned int ovp_cnt;
	unsigned int otp_cnt;
};

static struct wlrx_evt_dev *g_rx_evt_di[WLTRX_DRV_MAX];

static struct wlrx_evt_dev *wlrx_evt_get_di(unsigned int drv_type)
{
	if (!wltrx_is_drv_type_valid(drv_type)) {
		hwlog_err("get_di: drv_type=%u err\n", drv_type);
		return NULL;
	}

	return g_rx_evt_di[drv_type];
}

void wlrx_evt_clear_err_cnt(unsigned int drv_type)
{
	struct wlrx_evt_dev *di = wlrx_evt_get_di(drv_type);

	if (!di)
		return;

	di->ocp_cnt = 0;
	di->ovp_cnt = 0;
	di->otp_cnt = 0;
}

static void wlrx_evt_handle_rx_ocp_evt(struct wlrx_evt_dev *di)
{
	char dsm_buff[POWER_DSM_BUF_SIZE_0512] = { 0 };

	hwlog_info("[handle_rx_ocp_evt] drv_type:%u\n", di->drv_type);
	if (wlrx_get_charge_stage() < WLRX_STAGE_REGULATION)
		return;

	if (++di->ocp_cnt < WLRX_OCP_CNT_MAX)
		return;

	di->ocp_cnt = WLRX_OCP_CNT_MAX;
	wlrx_dsm_report(di->drv_type, POWER_DSM_ERROR_WIRELESS_RX_OCP,
		dsm_buff, sizeof(dsm_buff));
}

static void wlrx_evt_handle_rx_ovp_evt(struct wlrx_evt_dev *di)
{
	char dsm_buff[POWER_DSM_BUF_SIZE_0512] = { 0 };

	hwlog_info("[handle_rx_ovp_evt] drv_type:%u\n", di->drv_type);
	if (wlrx_get_charge_stage() < WLRX_STAGE_REGULATION)
		return;

	if (++di->ovp_cnt < WLRX_OVP_CNT_MAX)
		return;

	di->ovp_cnt = WLRX_OVP_CNT_MAX;
	wlrx_dsm_report(di->drv_type, POWER_DSM_ERROR_WIRELESS_RX_OVP,
		dsm_buff, sizeof(dsm_buff));
}

static void wlrx_evt_handle_rx_otp_evt(struct wlrx_evt_dev *di)
{
	char dsm_buff[POWER_DSM_BUF_SIZE_0512] = { 0 };

	hwlog_info("[handle_rx_otp_evt] drv_type:%u\n", di->drv_type);
	if (wlrx_get_charge_stage() < WLRX_STAGE_REGULATION)
		return;

	if (++di->otp_cnt < WLRX_OTP_CNT_MAX)
		return;

	di->otp_cnt = WLRX_OTP_CNT_MAX;
	wlrx_dsm_report(di->drv_type, POWER_DSM_ERROR_WIRELESS_RX_OTP,
		dsm_buff, sizeof(dsm_buff));
}

static void wlrx_evt_rxic_work(struct work_struct *work)
{
	struct wlrx_evt_dev *di = container_of(work, struct wlrx_evt_dev, rxic_work.work);

	if (!di) {
		power_wakeup_unlock(di->rxic_wakelock, false);
		return;
	}

	switch (di->evt_type) {
	case POWER_NE_WLRX_OCP:
		wlrx_evt_handle_rx_ocp_evt(di);
		break;
	case POWER_NE_WLRX_OVP:
		wlrx_evt_handle_rx_ovp_evt(di);
		break;
	case POWER_NE_WLRX_OTP:
		wlrx_evt_handle_rx_otp_evt(di);
		break;
	default:
		break;
	}
	power_wakeup_unlock(di->rxic_wakelock, false);
}

static void wlrx_evt_save_rxic_data(struct wlrx_evt_dev *di, unsigned long event, void *data)
{
	if (!data)
		return;

	switch (di->evt_type) {
	default:
		break;
	}
}

static int wlrx_evt_rxic_notifier_call(struct notifier_block *evt_nb,
	unsigned long event, void *data)
{
	struct wlrx_evt_dev *di = container_of(evt_nb, struct wlrx_evt_dev, rxic_nb);

	if (!di)
		return NOTIFY_OK;

	switch (event) {
	case POWER_NE_WLRX_PREV_READY:
		wlrx_cut_off_wired_channel(di->drv_type);
		charger_select_channel(CHARGER_CH_WLSIN);
		return NOTIFY_OK;
	case POWER_NE_WLRX_OCP:
	case POWER_NE_WLRX_OVP:
	case POWER_NE_WLRX_OTP:
		break;
	default:
		return NOTIFY_OK;
	}

	power_wakeup_lock(di->rxic_wakelock, false);
	di->evt_type = event;
	wlrx_evt_save_rxic_data(di, event, data);

	cancel_delayed_work_sync(&di->rxic_work);
	mod_delayed_work(system_wq, &di->rxic_work, msecs_to_jiffies(0));

	return NOTIFY_OK;
}

static int wlrx_evt_main_drv_register(struct wlrx_evt_dev *di)
{
	int ret;

	di->rxic_nb.notifier_call = wlrx_evt_rxic_notifier_call;
	ret = power_event_bnc_register(POWER_BNT_WLRX, &di->rxic_nb);
	if (ret) {
		hwlog_err("main_drv_register: register rxic_nb failed\n");
		return ret;
	}

	return 0;
}

static int wlrx_evt_register(unsigned int drv_type, struct wlrx_evt_dev *di)
{
	switch (drv_type) {
	case WLTRX_DRV_MAIN:
		di->drv_type = WLTRX_DRV_MAIN;
		return wlrx_evt_main_drv_register(di);
	default:
		return 0;
	}
}

static void wlrx_evt_kfree_dev(struct wlrx_evt_dev *di)
{
	if (!di)
		return;

	power_wakeup_source_unregister(di->rxic_wakelock);
	di->rxic_wakelock = NULL;
	kfree(di);
}

int wlrx_evt_init(unsigned int drv_type, struct device *dev)
{
	int ret;
	struct wlrx_evt_dev *di = NULL;

	if (!dev || !wltrx_is_drv_type_valid(drv_type))
		return -ENODEV;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	ret = wlrx_evt_register(drv_type, di);
	if (ret)
		goto exit;

	di->rxic_wakelock = power_wakeup_source_register(dev, "rxic_evt_wakelock");
	INIT_DELAYED_WORK(&di->rxic_work, wlrx_evt_rxic_work);
	g_rx_evt_di[drv_type] = di;
	return 0;

exit:
	wlrx_evt_kfree_dev(di);
	return ret;
}

void wlrx_evt_deinit(unsigned int drv_type)
{
	if (!wltrx_is_drv_type_valid(drv_type))
		return;

	wlrx_evt_kfree_dev(g_rx_evt_di[drv_type]);
	g_rx_evt_di[drv_type] = NULL;
}