/* SPDX-License-Identifier: GPL-2.0 */
/*
 * power_event_ne.h
 *
 * notifier event for power module
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

#ifndef _POWER_EVENT_NE_H_
#define _POWER_EVENT_NE_H_

#include <linux/notifier.h>
#include <linux/kobject.h>

#define POWER_EVENT_NOTIFY_SIZE 1024
#define POWER_EVENT_NOTIFY_NUM  2

/* define blocking notifier type for power module */
enum power_event_blocking_notifier_type {
	POWER_BNT_BEGIN = 0,
	POWER_BNT_CONNECT = POWER_BNT_BEGIN,
	POWER_BNT_CHARGING,
	POWER_BNT_SOC_DECIMAL,
	POWER_BNT_WD,
	POWER_BNT_UVDM,
	POWER_BNT_DC,
	POWER_BNT_LIGHTSTRAP,
	POWER_BNT_OTG,
	POWER_BNT_WLC,
	POWER_BNT_WLTX_AUX,
	POWER_BNT_WLRX,
	POWER_BNT_WLTX,
	POWER_BNT_CHG,
	POWER_BNT_COUL,
	POWER_BNT_TYPEC,
	POWER_BNT_THIRDPLAT_CHARGING,
	POWER_BNT_THIRDPLAT_PD,
	POWER_BNT_HW_PD,
	POWER_BNT_HW_USB,
	POWER_BNT_BATTERY,
	POWER_BNT_BUCK_CHARGE,
	POWER_BNT_UFCS,
	POWER_BNT_USB_EXT_MODEM,
	POWER_BNT_END,
};

/* define atomic notifier type for power module */
enum power_event_atomic_notifier_type {
	POWER_ANT_BEGIN = 0,
	POWER_ANT_CHARGE_FAULT = POWER_ANT_BEGIN,
	POWER_ANT_LVC_FAULT,
	POWER_ANT_SC_FAULT,
	POWER_ANT_SC4_FAULT,
	POWER_ANT_UVDM_FAULT,
	POWER_ANT_END,
};

/* define notifier event for power module */
enum power_event_notifier_list {
	POWER_NE_BEGIN = 0,
	/* section: for connect */
	POWER_NE_USB_DISCONNECT = POWER_NE_BEGIN,
	POWER_NE_USB_CONNECT,
	POWER_NE_WIRELESS_DISCONNECT,
	POWER_NE_WIRELESS_CONNECT,
	POWER_NE_WIRELESS_TX_START,
	POWER_NE_WIRELESS_TX_STOP, /* num:5 */
	POWER_NE_WIRELESS_AUX_TX_START,
	POWER_NE_WIRELESS_AUX_TX_STOP,
	/* section: for charging */
	POWER_NE_CHARGING_START,
	POWER_NE_CHARGING_STOP,
	POWER_NE_CHARGING_SUSPEND, /* num:10 */
	/* section: for soc decimal */
	POWER_NE_SOC_DECIMAL_DC,
	POWER_NE_SOC_DECIMAL_WL_DC,
	/* section: for water detect */
	POWER_NE_WD_REPORT_DMD,
	POWER_NE_WD_REPORT_UEVENT,
	POWER_NE_WD_DETECT_BY_USB_DP_DN, /* num:15 */
	POWER_NE_WD_DETECT_BY_USB_ID,
	POWER_NE_WD_DETECT_BY_USB_GPIO,
	POWER_NE_WD_DETECT_BY_AUDIO_DP_DN,
	/* section: for uvdm */
	POWER_NE_UVDM_RECEIVE,
	/* section: for direct charger */
	POWER_NE_DC_CHECK_START, /* num:20 */
	POWER_NE_DC_SWITCH_PATH,
	POWER_NE_DC_CHECK_SUCC,
	POWER_NE_DC_LVC_CHARGING,
	POWER_NE_DC_SC_CHARGING,
	POWER_NE_DC_STOP_CHARGE, /* num:25 */
	/* section: for lightstrap */
	POWER_NE_LIGHTSTRAP_ON,
	POWER_NE_LIGHTSTRAP_OFF,
	POWER_NE_LIGHTSTRAP_GET_PRODUCT_INFO,
	POWER_NE_LIGHTSTRAP_EPT,
	/* section: for otg */
	POWER_NE_OTG_OCP_CHECK_STOP, /* num:30 */
	POWER_NE_OTG_OCP_CHECK_START,
	POWER_NE_OTG_SCP_CHECK_STOP,
	POWER_NE_OTG_SCP_CHECK_START,
	POWER_NE_OTG_OCP_HANDLE,
	/* section: for wireless charge */
	POWER_NE_WLC_CHARGER_VBUS, /* num:35 */
	POWER_NE_WLC_ICON_TYPE,
	POWER_NE_WLC_TX_VSET,
	POWER_NE_WLC_READY,
	POWER_NE_WLC_HS_SUCC,
	POWER_NE_WLC_TX_CAP_SUCC, /* num:40 */
	POWER_NE_WLC_AUTH_SUCC,
	POWER_NE_WLC_DC_START_CHARGING,
	POWER_NE_WLC_VBUS_CONNECT,
	POWER_NE_WLC_VBUS_DISCONNECT,
	POWER_NE_WLC_WIRED_VBUS_CONNECT, /* num:45 */
	POWER_NE_WLC_WIRED_VBUS_DISCONNECT,
	/* section: for wireless tx */
	POWER_NE_WLTX_GET_CFG,
	POWER_NE_WLTX_HANDSHAKE_SUCC,
	POWER_NE_WLTX_CHARGEDONE,
	POWER_NE_WLTX_CEP_TIMEOUT, /* num:50 */
	POWER_NE_WLTX_EPT_CMD,
	POWER_NE_WLTX_OVP,
	POWER_NE_WLTX_OCP,
	POWER_NE_WLTX_PING_RX,
	POWER_NE_WLTX_HALL_APPROACH, /* num:55 */
	POWER_NE_WLTX_AUX_PEN_HALL_APPROACH,
	POWER_NE_WLTX_AUX_KB_HALL_APPROACH,
	POWER_NE_WLTX_HALL_AWAY_FROM,
	POWER_NE_WLTX_AUX_PEN_HALL_AWAY_FROM,
	POWER_NE_WLTX_AUX_KB_HALL_AWAY_FROM, /* num:60 */
	POWER_NE_WLTX_ACC_DEV_CONNECTED,
	POWER_NE_WLTX_RCV_DPING,
	POWER_NE_WLTX_ASK_SET_VTX,
	POWER_NE_WLTX_GET_TX_CAP,
	POWER_NE_WLTX_TX_FOD, /* num:65 */
	POWER_NE_WLTX_TX_PING_OCP,
	POWER_NE_WLTX_RP_DM_TIMEOUT,
	POWER_NE_WLTX_TX_INIT,
	POWER_NE_WLTX_TX_AP_ON,
	POWER_NE_WLTX_IRQ_SET_VTX, /* num:70 */
	POWER_NE_WLTX_GET_RX_PRODUCT_TYPE,
	POWER_NE_WLTX_GET_RX_MAX_POWER,
	POWER_NE_WLTX_ASK_RX_EVT,
	/* section: for wireless rx */
	POWER_NE_WLRX_PWR_ON,
	POWER_NE_WLRX_PREV_READY, /* num:75 */
	POWER_NE_WLRX_READY,
	POWER_NE_WLRX_OCP,
	POWER_NE_WLRX_OVP,
	POWER_NE_WLRX_OTP,
	POWER_NE_WLRX_LDO_OFF, /* num:80 */
	POWER_NE_WLRX_TX_ALARM,
	POWER_NE_WLRX_TX_BST_ERR,
	/* section: for charger */
	POWER_NE_CHG_START_CHARGING,
	POWER_NE_CHG_STOP_CHARGING,
	POWER_NE_CHG_CHARGING_DONE, /* num:85 */
	POWER_NE_CHG_PRE_STOP_CHARGING,
	POWER_NE_CHG_WAKE_UNLOCK,
	/* section: for coul */
	POWER_NE_COUL_LOW_VOL,
	/* section: for charger fault */
	POWER_NE_CHG_FAULT_NON,
	POWER_NE_CHG_FAULT_BOOST_OCP, /* num:90 */
	POWER_NE_CHG_FAULT_VBAT_OVP,
	POWER_NE_CHG_FAULT_SCHARGER,
	POWER_NE_CHG_FAULT_I2C_ERR,
	POWER_NE_CHG_FAULT_WEAKSOURCE,
	POWER_NE_CHG_FAULT_CHARGE_DONE, /* num:95 */
	/* section: for direct charger fault */
	POWER_NE_DC_FAULT_NON,
	POWER_NE_DC_FAULT_VBUS_OVP,
	POWER_NE_DC_FAULT_REVERSE_OCP,
	POWER_NE_DC_FAULT_OTP,
	POWER_NE_DC_FAULT_TSBUS_OTP, /* num:100 */
	POWER_NE_DC_FAULT_TSBAT_OTP,
	POWER_NE_DC_FAULT_TDIE_OTP,
	POWER_NE_DC_FAULT_INPUT_OCP,
	POWER_NE_DC_FAULT_VDROP_OVP,
	POWER_NE_DC_FAULT_AC_OVP, /* num:105 */
	POWER_NE_DC_FAULT_VBAT_OVP,
	POWER_NE_DC_FAULT_IBAT_OCP,
	POWER_NE_DC_FAULT_IBUS_OCP,
	POWER_NE_DC_FAULT_CONV_OCP,
	POWER_NE_DC_FAULT_LTC7820, /* num:110 */
	POWER_NE_DC_FAULT_INA231,
	POWER_NE_DC_FAULT_CC_SHORT,
	/* section: for uvdm charger fault */
	POWER_NE_UVDM_FAULT_OTG,
	POWER_NE_UVDM_FAULT_COVER_ABNORMAL,
	/* section: for typec */
	POWER_NE_TYPEC_CURRENT_CHANGE, /* num:115 */
	/* section: for third platform charging */
	POWER_NE_THIRDPLAT_CHARGING_EOC,
	POWER_NE_THIRDPLAT_CHARGING_START,
	POWER_NE_THIRDPLAT_CHARGING_STOP,
	POWER_NE_THIRDPLAT_CHARGING_ERROR,
	POWER_NE_THIRDPLAT_CHARGING_NORMAL, /* num:120 */
	/* section: for third platform pd */
	POWER_NE_THIRDPLAT_PD_START,
	POWER_NE_THIRDPLAT_PD_STOP,
	/* section: for pd dpm */
	POWER_NE_HW_PD_ORIENTATION_CC,
	POWER_NE_HW_PD_CHARGER,
	POWER_NE_HW_PD_SOURCE_VCONN, /* num:125 */
	POWER_NE_HW_PD_SOURCE_VBUS,
	POWER_NE_HW_PD_LOW_POWER_VBUS,
	POWER_NE_HW_PD_QUCIK_CHARGE,
	/* section: for usb */
	POWER_NE_HW_USB_SPEED,
	POWER_NE_HW_DP_STATE, /* num:130 */
	/* section: for battery */
	POWER_NE_BATTERY_LOW_WARNING,
	POWER_NE_BATTERY_LOW_SHUTDOWN,
	POWER_NE_BATTERY_MOVE,
	/* section: for buck charge */
	POWER_NE_BUCK_FFC_CHARGE,
	/* section: for direct charger ufcs irq */
	POWER_NE_UFCS_REC_UNSOLICITED_DATA,
	/* section: for usb extra modem */
	POWER_NE_UEM_RECEIVE_UVDM_DATA,
	POWER_NE_END,
};

struct power_event_notify_data {
	const char *event;
	int event_len;
};

int power_event_bnc_cond_register(unsigned int type, struct notifier_block *nb);
int power_event_bnc_register(unsigned int type, struct notifier_block *nb);
int power_event_bnc_unregister(unsigned int type, struct notifier_block *nb);
void power_event_bnc_notify(unsigned int type, unsigned long event, void *data);
int power_event_anc_register(unsigned int type, struct notifier_block *nb);
int power_event_anc_unregister(unsigned int type, struct notifier_block *nb);
void power_event_anc_notify(unsigned int type, unsigned long event, void *data);
void power_event_report_uevent(const struct power_event_notify_data *n_data);
void power_event_notify_sysfs(struct kobject *kobj, const char *dir, const char *attr);
void power_event_sync(void);

#endif /* _POWER_EVENT_NE_H_ */
