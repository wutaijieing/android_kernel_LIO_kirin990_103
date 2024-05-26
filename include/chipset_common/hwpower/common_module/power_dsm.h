/* SPDX-License-Identifier: GPL-2.0 */
/*
 * power_dsm.h
 *
 * dsm for power module
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

#ifndef _POWER_DSM_H_
#define _POWER_DSM_H_

#include <dsm/dsm_pub.h>

/* define dsm new id */
#ifdef CONFIG_HUAWEI_DSM
#define POWER_DSM_ERROR_BATT_ACR_OVER_THRESHOLD           ERROR_BATT_ACR_OVER_THRESHOLD
#define POWER_DSM_ERROR_LOW_VOL_INT                       ERROR_LOW_VOL_INT
#define POWER_DSM_ERROR_NO_WATER_CHECK_BASE               ERROR_NO_WATER_CHECK_BASE
#define POWER_DSM_ERROR_NO_TYPEC_4480_OVP                 ERROR_NO_TYPEC_4480_OVP
#define POWER_DSM_ERROR_SWITCH_ATTACH                     ERROR_SWITCH_ATTACH
#define POWER_DSM_ERROR_FCP_OUTPUT                        ERROR_FCP_OUTPUT
#define POWER_DSM_ERROR_FCP_DETECT                        ERROR_FCP_DETECT
#define POWER_DSM_ERROR_BATT_TEMP_LOW                     ERROR_BATT_TEMP_LOW
#define POWER_DSM_ERROR_NO_TYPEC_CC_OVP                   ERROR_NO_TYPEC_CC_OVP
#define POWER_DSM_ERROR_WIRELESS_TX_STATISTICS            ERROR_WIRELESS_TX_STATISTICS
#define POWER_DSM_ERROR_NO_SMPL                           ERROR_NO_SMPL
#define POWER_DSM_ERROR_NON_STANDARD_CHARGER_PLUGGED      ERROR_NON_STANDARD_CHARGER_PLUGGED
#define POWER_DSM_ERROR_NO_WATER_CHECK_IN_USB             ERROR_NO_WATER_CHECK_IN_USB
#define POWER_DSM_ERROR_BATTERY_POLAR_ISHORT              ERROR_BATTERY_POLAR_ISHORT
#define POWER_DSM_ERROR_NO_CPU_BUCK_BASE                  ERROR_NO_CPU_BUCK_BASE
#define POWER_DSM_ERROR_REFRESH_FCC_OUTSIDE               ERROR_REFRESH_FCC_OUTSIDE
#define POWER_DSM_ERROR_BATT_TERMINATE_TOO_EARLY          ERROR_BATT_TERMINATE_TOO_EARLY
#define POWER_DSM_ERROR_CHARGE_I2C_RW                     ERROR_CHARGE_I2C_RW
#define POWER_DSM_ERROR_WEAKSOURCE_STOP_CHARGE            ERROR_WEAKSOURCE_STOP_CHARGE
#define POWER_DSM_ERROR_BOOST_OCP                         ERROR_BOOST_OCP
#define POWER_DSM_ERROR_NO_USB_SHORT_PROTECT_NTC          ERROR_NO_USB_SHORT_PROTECT_NTC
#define POWER_DSM_ERROR_NO_USB_SHORT_PROTECT              ERROR_NO_USB_SHORT_PROTECT
#define POWER_DSM_ERROR_NO_USB_SHORT_PROTECT_HIZ          ERROR_NO_USB_SHORT_PROTECT_HIZ
#define POWER_DSM_ERROR_WIRELESS_BOOSTING_FAIL            ERROR_WIRELESS_BOOSTING_FAIL
#define POWER_DSM_ERROR_WIRELESS_CERTI_COMM_FAIL          ERROR_WIRELESS_CERTI_COMM_FAIL
#define POWER_DSM_ERROR_WIRELESS_CHECK_TX_ABILITY_FAIL    ERROR_WIRELESS_CHECK_TX_ABILITY_FAIL
#define POWER_DSM_ERROR_WIRELESS_RX_OCP                   ERROR_WIRELESS_RX_OCP
#define POWER_DSM_ERROR_WIRELESS_RX_OVP                   ERROR_WIRELESS_RX_OVP
#define POWER_DSM_ERROR_WIRELESS_RX_OTP                   ERROR_WIRELESS_RX_OTP
#define POWER_DSM_ERROR_WIRELESS_TX_POWER_SUPPLY_FAIL     ERROR_WIRELESS_TX_POWER_SUPPLY_FAIL
#define POWER_DSM_ERROR_WIRELESS_TX_BATTERY_OVERHEAT      ERROR_WIRELESS_TX_BATTERY_OVERHEAT
#define POWER_DSM_DIRECT_CHARGE_ADAPTER_OTP               DSM_DIRECT_CHARGE_ADAPTER_OTP
#define POWER_DSM_DIRECT_CHARGE_VOL_ACCURACY              DSM_DIRECT_CHARGE_VOL_ACCURACY
#define POWER_DSM_DIRECT_CHARGE_USB_PORT_LEAKAGE_CURRENT  DSM_DIRECT_CHARGE_USB_PORT_LEAKAGE_CURRENT
#define POWER_DSM_DIRECT_CHARGE_FULL_PATH_RESISTANCE      DSM_DIRECT_CHARGE_FULL_PATH_RESISTANCE
#define POWER_DSM_DIRECT_CHARGE_VBUS_OVP                  DSM_DIRECT_CHARGE_VBUS_OVP
#define POWER_DSM_DIRECT_CHARGE_REVERSE_OCP               DSM_DIRECT_CHARGE_REVERSE_OCP
#define POWER_DSM_DIRECT_CHARGE_OTP                       DSM_DIRECT_CHARGE_OTP
#define POWER_DSM_DIRECT_CHARGE_INPUT_OCP                 DSM_DIRECT_CHARGE_INPUT_OCP
#define POWER_DSM_DIRECT_CHARGE_SC_FAULT_VBUS_OVP         DSM_DIRECT_CHARGE_SC_FAULT_VBUS_OVP
#define POWER_DSM_DIRECT_CHARGE_SC_FAULT_TSBAT_OTP        DSM_DIRECT_CHARGE_SC_FAULT_TSBAT_OTP
#define POWER_DSM_DIRECT_CHARGE_SC_FAULT_TSBUS_OTP        DSM_DIRECT_CHARGE_SC_FAULT_TSBUS_OTP
#define POWER_DSM_DIRECT_CHARGE_SC_FAULT_TDIE_OTP         DSM_DIRECT_CHARGE_SC_FAULT_TDIE_OTP
#define POWER_DSM_DIRECT_CHARGE_SC_FAULT_AC_OVP           DSM_DIRECT_CHARGE_SC_FAULT_AC_OVP
#define POWER_DSM_DIRECT_CHARGE_SC_FAULT_VBAT_OVP         DSM_DIRECT_CHARGE_SC_FAULT_VBAT_OVP
#define POWER_DSM_DIRECT_CHARGE_SC_FAULT_IBAT_OCP         DSM_DIRECT_CHARGE_SC_FAULT_IBAT_OCP
#define POWER_DSM_DIRECT_CHARGE_SC_FAULT_IBUS_OCP         DSM_DIRECT_CHARGE_SC_FAULT_IBUS_OCP
#define POWER_DSM_DIRECT_CHARGE_SC_FAULT_CONV_OCP         DSM_DIRECT_CHARGE_SC_FAULT_CONV_OCP
#define POWER_DSM_DIRECT_CHARGE_FULL_PATH_RESISTANCE_2ND  DSM_DIRECT_CHARGE_FULL_PATH_RESISTANCE_2ND
#define POWER_DSM_SOFT_RECHARGE_NO                        DSM_SOFT_RECHARGE_NO
#define POWER_DSM_BATTERY_HEATING                         DSM_BATTERY_HEATING
#define POWER_DSM_POWER_DEVICES_INFO                      DSM_POWER_DEVICES_INFO
#define POWER_DSM_BATTERY_DETECT_ERROR_NO                 DSM_BATTERY_DETECT_ERROR_NO
#define POWER_DSM_OTG_VBUS_SHORT                          DSM_OTG_VBUS_SHORT
#define POWER_DSM_LIGHTSTRAP_STATUS                       DSM_LIGHTSTRAP_STATUS
#define POWER_DSM_PD_TYPEC_VBUS_VALID                     DSM_PD_TYPEC_VBUS_VALID
#define POWER_DSM_PD_TYPEC_VBUS_STILL_VALID               DSM_PD_TYPEC_VBUS_STILL_VALID
#define POWER_DSM_DA_BATTERY_ACR_ERROR_NO                 DA_BATTERY_ACR_ERROR_NO
#define POWER_DSM_BATTERY_ROM_ID_CERTIFICATION_FAIL       DSM_BATTERY_ROM_ID_CERTIFICATION_FAIL
#define POWER_DSM_BATTERY_IC_EEPROM_STATE_ERROR           DSM_BATTERY_IC_EEPROM_STATE_ERROR
#define POWER_DSM_BATTERY_IC_KEY_CERTIFICATION_FAIL       DSM_BATTERY_IC_KEY_CERTIFICATION_FAIL
#define POWER_DSM_OLD_BOARD_AND_OLD_BATTERY_UNMATCH       DSM_OLD_BOARD_AND_OLD_BATTERY_UNMATCH
#define POWER_DSM_OLD_BOARD_AND_NEW_BATTERY_UNMATCH       DSM_OLD_BOARD_AND_NEW_BATTERY_UNMATCH
#define POWER_DSM_NEW_BOARD_AND_OLD_BATTERY_UNMATCH       DSM_NEW_BOARD_AND_OLD_BATTERY_UNMATCH
#define POWER_DSM_BATTERY_NV_DATA_READ_FAIL               DSM_BATTERY_NV_DATA_READ_FAIL
#define POWER_DSM_CERTIFICATION_SERVICE_IS_NOT_RESPONDING DSM_CERTIFICATION_SERVICE_IS_NOT_RESPONDING
#define POWER_DSM_UNMATCH_BATTERYS                        DSM_UNMATCH_BATTERYS
#define POWER_DSM_MULTI_CHARGE_CURRENT_RATIO_INFO         DSM_MULTI_CHARGE_CURRENT_RATIO_INFO
#define POWER_DSM_MULTI_CHARGE_CURRENT_RATIO_WARNING      DSM_MULTI_CHARGE_CURRENT_RATIO_WARNING
#define POWER_DSM_MULTI_CHARGE_CURRENT_RATIO_ERROR        DSM_MULTI_CHARGE_CURRENT_RATIO_ERROR
#define POWER_DSM_DIRECT_CHARGE_ERR_WITH_STD_CABLE        DSM_DIRECT_CHARGE_ERR_WITH_STD_CABLE
#define POWER_DSM_DIRECT_CHARGE_ERR_WITH_NONSTD_CABLE     DSM_DIRECT_CHARGE_ERR_WITH_NONSTD_CABLE
#define POWER_DSM_DUAL_BATTERY_CURRENT_BIAS_DETECT        DSM_DUAL_BATTERY_CURRENT_BIAS_DETECT
#else
#define POWER_DSM_ERROR_BATT_ACR_OVER_THRESHOLD           0
#define POWER_DSM_ERROR_LOW_VOL_INT                       0
#define POWER_DSM_ERROR_NO_WATER_CHECK_BASE               0
#define POWER_DSM_ERROR_NO_TYPEC_4480_OVP                 0
#define POWER_DSM_ERROR_SWITCH_ATTACH                     0
#define POWER_DSM_ERROR_FCP_OUTPUT                        0
#define POWER_DSM_ERROR_FCP_DETECT                        0
#define POWER_DSM_ERROR_BATT_TEMP_LOW                     0
#define POWER_DSM_ERROR_NO_TYPEC_CC_OVP                   0
#define POWER_DSM_ERROR_WIRELESS_TX_STATISTICS            0
#define POWER_DSM_ERROR_NO_SMPL                           0
#define POWER_DSM_ERROR_NON_STANDARD_CHARGER_PLUGGED      0
#define POWER_DSM_ERROR_NO_WATER_CHECK_IN_USB             0
#define POWER_DSM_ERROR_BATTERY_POLAR_ISHORT              0
#define POWER_DSM_ERROR_NO_CPU_BUCK_BASE                  0
#define POWER_DSM_ERROR_REFRESH_FCC_OUTSIDE               0
#define POWER_DSM_ERROR_BATT_TERMINATE_TOO_EARLY          0
#define POWER_DSM_ERROR_CHARGE_I2C_RW                     0
#define POWER_DSM_ERROR_WEAKSOURCE_STOP_CHARGE            0
#define POWER_DSM_ERROR_BOOST_OCP                         0
#define POWER_DSM_ERROR_NO_USB_SHORT_PROTECT_NTC          0
#define POWER_DSM_ERROR_NO_USB_SHORT_PROTECT              1
#define POWER_DSM_ERROR_NO_USB_SHORT_PROTECT_HIZ          2
#define POWER_DSM_ERROR_WIRELESS_BOOSTING_FAIL            0
#define POWER_DSM_ERROR_WIRELESS_CERTI_COMM_FAIL          0
#define POWER_DSM_ERROR_WIRELESS_CHECK_TX_ABILITY_FAIL    0
#define POWER_DSM_ERROR_WIRELESS_RX_OCP                   0
#define POWER_DSM_ERROR_WIRELESS_RX_OVP                   0
#define POWER_DSM_ERROR_WIRELESS_RX_OTP                   0
#define POWER_DSM_ERROR_WIRELESS_TX_POWER_SUPPLY_FAIL     0
#define POWER_DSM_ERROR_WIRELESS_TX_BATTERY_OVERHEAT      0
#define POWER_DSM_DIRECT_CHARGE_ADAPTER_OTP               0
#define POWER_DSM_DIRECT_CHARGE_VOL_ACCURACY              0
#define POWER_DSM_DIRECT_CHARGE_USB_PORT_LEAKAGE_CURRENT  0
#define POWER_DSM_DIRECT_CHARGE_FULL_PATH_RESISTANCE      0
#define POWER_DSM_DIRECT_CHARGE_VBUS_OVP                  0
#define POWER_DSM_DIRECT_CHARGE_REVERSE_OCP               0
#define POWER_DSM_DIRECT_CHARGE_OTP                       0
#define POWER_DSM_DIRECT_CHARGE_INPUT_OCP                 0
#define POWER_DSM_DIRECT_CHARGE_SC_FAULT_VBUS_OVP         0
#define POWER_DSM_DIRECT_CHARGE_SC_FAULT_TSBAT_OTP        0
#define POWER_DSM_DIRECT_CHARGE_SC_FAULT_TSBUS_OTP        0
#define POWER_DSM_DIRECT_CHARGE_SC_FAULT_TDIE_OTP         0
#define POWER_DSM_DIRECT_CHARGE_SC_FAULT_AC_OVP           0
#define POWER_DSM_DIRECT_CHARGE_SC_FAULT_VBAT_OVP         0
#define POWER_DSM_DIRECT_CHARGE_SC_FAULT_IBAT_OCP         0
#define POWER_DSM_DIRECT_CHARGE_SC_FAULT_IBUS_OCP         0
#define POWER_DSM_DIRECT_CHARGE_SC_FAULT_CONV_OCP         0
#define POWER_DSM_DIRECT_CHARGE_FULL_PATH_RESISTANCE_2ND  0
#define POWER_DSM_SOFT_RECHARGE_NO                        0
#define POWER_DSM_BATTERY_HEATING                         0
#define POWER_DSM_POWER_DEVICES_INFO                      0
#define POWER_DSM_BATTERY_DETECT_ERROR_NO                 0
#define POWER_DSM_OTG_VBUS_SHORT                          0
#define POWER_DSM_LIGHTSTRAP_STATUS                       0
#define POWER_DSM_PD_TYPEC_VBUS_VALID                     0
#define POWER_DSM_PD_TYPEC_VBUS_STILL_VALID               0
#define POWER_DSM_DA_BATTERY_ACR_ERROR_NO                 0
#define POWER_DSM_BATTERY_ROM_ID_CERTIFICATION_FAIL       0
#define POWER_DSM_BATTERY_IC_EEPROM_STATE_ERROR           0
#define POWER_DSM_BATTERY_IC_KEY_CERTIFICATION_FAIL       0
#define POWER_DSM_OLD_BOARD_AND_OLD_BATTERY_UNMATCH       0
#define POWER_DSM_OLD_BOARD_AND_NEW_BATTERY_UNMATCH       0
#define POWER_DSM_NEW_BOARD_AND_OLD_BATTERY_UNMATCH       0
#define POWER_DSM_BATTERY_NV_DATA_READ_FAIL               0
#define POWER_DSM_CERTIFICATION_SERVICE_IS_NOT_RESPONDING 0
#define POWER_DSM_UNMATCH_BATTERYS                        0
#define POWER_DSM_MULTI_CHARGE_CURRENT_RATIO_INFO         0
#define POWER_DSM_MULTI_CHARGE_CURRENT_RATIO_WARNING      0
#define POWER_DSM_MULTI_CHARGE_CURRENT_RATIO_ERROR        0
#define POWER_DSM_DIRECT_CHARGE_ERR_WITH_STD_CABLE        0
#define POWER_DSM_DIRECT_CHARGE_ERR_WITH_NONSTD_CABLE     0
#define POWER_DSM_DUAL_BATTERY_CURRENT_BIAS_DETECT        0
#endif /* CONFIG_HUAWEI_DSM */

/* define dsm buffer size */
#define POWER_DSM_BUF_SIZE_0016                           16
#define POWER_DSM_BUF_SIZE_0128                           128
#define POWER_DSM_BUF_SIZE_0256                           256
#define POWER_DSM_BUF_SIZE_0512                           512
#define POWER_DSM_BUF_SIZE_1024                           1024
#define POWER_DSM_BUF_SIZE_2048                           2048
#define POWER_DSM_RESERVED_SIZE                           16

enum power_dsm_type {
	POWER_DSM_TYPE_BEGIN = 0,
	POWER_DSM_CPU_BUCK = POWER_DSM_TYPE_BEGIN,
	POWER_DSM_USB,
	POWER_DSM_BATTERY_DETECT,
	POWER_DSM_BATTERY,
	POWER_DSM_CHARGE_MONITOR,
	POWER_DSM_SUPERSWITCH,
	POWER_DSM_SMPL,
	POWER_DSM_PD_RICHTEK,
	POWER_DSM_PD,
	POWER_DSM_USCP,
	POWER_DSM_PMU_OCP,
	POWER_DSM_PMU_IRQ,
	POWER_DSM_VIBRATOR_IRQ,
	POWER_DSM_DIRECT_CHARGE_SC,
	POWER_DSM_FCP_CHARGE,
	POWER_DSM_MTK_SWITCH_CHARGE2,
	POWER_DSM_LIGHTSTRAP,
	POWER_DSM_TYPE_END,
};

#ifdef CONFIG_HUAWEI_DSM
struct power_dsm_client {
	unsigned int type;
	const char *name;
	struct dsm_client *client;
	struct dsm_dev *dev;
};
#endif /* CONFIG_HUAWEI_DSM */

struct power_dsm_dump {
	unsigned int type;
	int error_no;
	bool dump_enable;
	bool dump_always;
	const char *pre_buf;
	bool (*support)(void);
	void (*dump)(char *buf, unsigned int buf_len);
	bool (*check_error)(char *buf, unsigned int buf_len);
};

#ifdef CONFIG_HUAWEI_DSM
struct dsm_client *power_dsm_get_dclient(unsigned int type);
int power_dsm_report_dmd(unsigned int type, int err_no, const char *buf);

#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
int power_dsm_report_fmd(unsigned int type, int err_no,
	const void *msg);
#else
static inline int power_dsm_report_fmd(unsigned int type,
	int err_no, const void *msg)
{
	return 0;
}
#endif /* CONFIG_HUAWEI_DATA_ACQUISITION */

#define power_dsm_report_format_dmd(type, err_no, fmt, args...) do { \
	if (power_dsm_get_dclient(type)) { \
		if (!dsm_client_ocuppy(power_dsm_get_dclient(type))) { \
			dsm_client_record(power_dsm_get_dclient(type), fmt, ##args); \
			dsm_client_notify(power_dsm_get_dclient(type), err_no); \
			pr_info("report type:%d, err_no:%d\n", type, err_no); \
		} else { \
			pr_err("power dsm client is busy\n"); \
		} \
	} \
} while (0)

void power_dsm_reset_dump_enable(struct power_dsm_dump *data, unsigned int size);
void power_dsm_dump_data(struct power_dsm_dump *data, unsigned int size);
void power_dsm_dump_data_with_error_no(struct power_dsm_dump *data,
	unsigned int size, int err_no);
#else
static inline struct dsm_client *power_dsm_get_dclient(unsigned int type)
{
	return NULL;
}

static inline int power_dsm_report_dmd(unsigned int type,
	int err_no, const char *buf)
{
	return 0;
}

static inline int power_dsm_report_fmd(unsigned int type,
	int err_no, const void *msg)
{
	return 0;
}

#define power_dsm_report_format_dmd(type, err_no, fmt, args...)

static inline void power_dsm_reset_dump_enable(
	struct power_dsm_dump *data, unsigned int size)
{
}

static inline void power_dsm_dump_data(
	struct power_dsm_dump *data, unsigned int size)
{
}

static inline void power_dsm_dump_data_with_error_no(
	struct power_dsm_dump *data, unsigned int size, int err_no)
{
}
#endif /* CONFIG_HUAWEI_DSM */

#endif /* _POWER_DSM_H_ */
