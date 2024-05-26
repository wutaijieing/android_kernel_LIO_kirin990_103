/*
 * hisi_charger.h
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

#ifndef _HISI_CHARGER_
#define _HISI_CHARGER_

#include <linux/device.h> /*for struct charge_device_info*/
#include <linux/notifier.h> /*for struct charge_device_info*/
#include <linux/workqueue.h> /*for struct charge_device_info*/
#include <linux/power_supply.h> /*for struct charge_device_info*/

/*************************marco define area***************************/
#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif

#define WEAKSOURCE_TRUE 1
#define WEAKSOURCE_FALSE 0
#define CHARGE_OTG_ENABLE 1
#define CHARGE_OTG_DISABLE 0

#define CHARGE_IC_GOOD 0
#define CHARGE_IC_BAD 1
/*fcp detect */
#define FCP_ADAPTER_DETECT_FAIL 1
#define FCP_ADAPTER_DETECT_SUCC 0
#define FCP_ADAPTER_DETECT_OTHER -1

#define ERROR_FCP_VOL_OVER_HIGH (10421)
#define ERROR_FCP_DETECT (10422)
#define ERROR_FCP_OUTPUT (10423)
#define ERROR_SWITCH_ATTACH (10424)
#define ERROR_ADAPTER_OVLT (10425)
#define ERROR_ADAPTER_OCCURRENT (10426)
#define ERROR_ADAPTER_OTEMP (10427)
#define get_index(x) (x - ERROR_FCP_VOL_OVER_HIGH)
#define ERR_NO_STRING_SIZE 128
#define CHARGELOG_SIZE (4096)
/*options of charge current(include input current & charge into battery current)*/
#define CHARGE_CURRENT_0000_MA (0)
#define CHARGE_CURRENT_0500_MA (500)
#define CHARGE_CURRENT_0800_MA (800)
#define CHARGE_CURRENT_1000_MA (1000)
#define CHARGE_CURRENT_1200_MA (1200)
#define CHARGE_CURRENT_1900_MA (1900)
#define CHARGE_CURRENT_2000_MA (2000)
#define CHARGE_CURRENT_4000_MA (4000)
#define CHARGE_CURRENT_MAX_MA (32767)

/*options of battery voltage*/
#define BATTERY_VOLTAGE_0000_MV (0)
#define BATTERY_VOLTAGE_0200_MV (200)

#define BATTERY_VOLTAGE_MIN_MV (-32767)
#define BATTERY_VOLTAGE_MAX_MV (32767)
#define BATTERY_VOLTAGE_3200_MV (3200)
#define BATTERY_VOLTAGE_3400_MV (3400)
#define BATTERY_VOLTAGE_4100_MV (4200)
#define BATTERY_VOLTAGE_4200_MV (4200)
#define BATTERY_VOLTAGE_4350_MV (4350)
#define BATTERY_VOLTAGE_4500_MV (4500)

/*options of NTC battery temperature*/
#define BATTERY_TEMPERATURE_MIN (-32767)
#define BATTERY_TEMPERATURE_MAX (32767)
#define BATTERY_TEMPERATURE_0_C (0)
#define BATTERY_TEMPERATURE_5_C (5)

#define CHARGING_WORK_TIMEOUT (30000)
#define MIN_CHARGING_CURRENT_OFFSET (-10)
#define BATTERY_FULL_CHECK_TIMIES (2)

#define WATCHDOG_TIMER_DISABLE 0
#define WATCHDOG_TIMER_40_S 40
#define WATCHDOG_TIMER_80_S 80

/*options of charge voltage (for dpm voltage setting,also ovp & uvp protect)*/
#define CHARGE_VOLTAGE_4360_MV (4360)
#define CHARGE_VOLTAGE_4520_MV (4520)
#define CHARGE_VOLTAGE_4600_MV (4600)
#define CHARGE_VOLTAGE_5000_MV (5000)
#define CHARGE_VOLTAGE_6300_MV (6300)
#define CHARGE_VOLTAGE_6500_MV (6500)

/*options of charge states from chip*/
#define CHAGRE_STATE_NORMAL (0x00)
#define CHAGRE_STATE_VBUS_OVP (0x01)
#define CHAGRE_STATE_NOT_PG (0x02)
#define CHAGRE_STATE_WDT_FAULT (0x04)
#define CHAGRE_STATE_BATT_OVP (0x08)
#define CHAGRE_STATE_CHRG_DONE (0x10)
#define CHAGRE_STATE_INPUT_DPM (0x20)
#define CHAGRE_STATE_NTC_FAULT (0x40)

/*options of vbus voltage*/
#define VBUS_VOLTAGE_FCP_MIN_MV (7000)
#define VBUS_VOLTAGE_NON_FCP_MAX_MV (6500)
#define VBUS_VOLTAGE_ABNORMAL_MAX_COUNT (2)

#define ADAPTER_0V 0
#define ADAPTER_5V 5
#define ADAPTER_9V 9
#define ADAPTER_12V 12

#define STRTOL_DECIMAL_BASE     10

/*************************struct define area***************************/
enum huawei_usb_charger_type {
	CHARGER_TYPE_USB = 0, /*SDP*/
	CHARGER_TYPE_BC_USB, /*CDP*/
	CHARGER_TYPE_NON_STANDARD, /*UNKNOW*/
	CHARGER_TYPE_STANDARD, /*DCP*/
	CHARGER_TYPE_FCP, /*FCP*/
	CHARGER_REMOVED, /*not connected*/
	USB_EVENT_OTG_ID,
};

enum charge_fault_type {
	CHARGE_FAULT_NON = 0,
	CHARGE_FAULT_BOOST_OCP,
	CHARGE_FAULT_TOTAL,
};

enum charge_sysfs_type {
	CHARGE_SYSFS_IIN_THERMAL = 0,
	CHARGE_SYSFS_ICHG_THERMAL,
	CHARGE_SYSFS_IIN_RUNNINGTEST,
	CHARGE_SYSFS_ICHG_RUNNINGTEST,
	CHARGE_SYSFS_ENABLE_CHARGER,
	CHARGE_SYSFS_LIMIT_CHARGING,
	CHARGE_SYSFS_REGULATION_VOLTAGE,
	CHARGE_SYSFS_BATFET_DISABLE,
	CHARGE_SYSFS_WATCHDOG_DISABLE,
	CHARGE_SYSFS_CHARGELOG,
	CHARGE_SYSFS_CHARGELOG_HEAD,
	CHARGE_SYSFS_IBUS,
	CHARGE_SYSFS_HIZ,
	CHARGE_SYSFS_CHARGE_TYPE,
	CHARGE_SYSFS_CHARGE_DONE_STATUS,
	CHARGE_SYSFS_CHARGE_DONE_SLEEP_STATUS,
	CHARGE_SYSFS_INPUTCURRENT,
};
enum charge_done_type {
	CHARGE_DONE_NON = 0,
	CHARGE_DONE,
};
enum charge_done_sleep_type {
	CHARGE_DONE_SLEEP_DISABLED = 0,
	CHARGE_DONE_SLEEP_ENABLED,
};
enum fcp_check_stage_type {
	FCP_STAGE_DEFAUTL,
	FCP_STAGE_SUPPORT_DETECT,
	FCP_STAGE_ADAPTER_DETECT,
	FCP_STAGE_ADAPTER_ENABLE,
	FCP_STAGE_SUCESS,
	FCP_STAGE_CHARGE_DONE,
};
enum fcp_retry_operate_type {
	FCP_RETRY_OPERATE_DEFAUTL,
	FCP_RETRY_OPERATE_RESET_ADAPTER,
	FCP_RETRY_OPERATE_RESET_FSA9688,
	FCP_RETRY_OPERATE_UNVALID,
};
static const char *const fcp_check_stage[] = {
	[0] = "FCP_STAGE_DEFAUTL",	[1] = "FCP_STAGE_SUPPORT_DETECT",
	[2] = "FCP_STAGE_ADAPTER_DETECT", [3] = "FCP_STAGE_ADAPTER_ENABLE",
	[4] = "FCP_STAGE_SUCESS",
};

struct charge_sysfs_data {
	unsigned int iin_thl;
	unsigned int ichg_thl;
	unsigned int iin_rt;
	unsigned int ichg_rt;
	unsigned int vterm_rt;
	unsigned int charge_limit;
	unsigned int wdt_disable;
	unsigned int charge_enable;
	unsigned int batfet_disable;
	unsigned int hiz_enable;
	enum charge_done_type charge_done_status;
	enum charge_done_sleep_type charge_done_sleep_status;
	int ibus;
	int inputcurrent;
	struct mutex dump_reg_lock;
	struct mutex dump_reg_head_lock;
	char reg_value[CHARGELOG_SIZE];
	char reg_head[CHARGELOG_SIZE];
};

struct charge_core_data {
	unsigned int iin;
	unsigned int ichg;
	unsigned int vterm;
	unsigned int iin_ac;
	unsigned int ichg_ac;
	unsigned int iin_usb;
	unsigned int ichg_usb;
	unsigned int iin_nonstd;
	unsigned int ichg_nonstd;
	unsigned int iin_bc_usb;
	unsigned int ichg_bc_usb;
	unsigned int iin_fcp;
	unsigned int ichg_fcp;
	unsigned int iterm;
	unsigned int vdpm;
	unsigned int iin_max;
	unsigned int ichg_max;
	unsigned int otg_curr;
};

struct charge_init_data {
	unsigned int charger_type;
	int vbus;
};

struct charge_device_ops {
	int (*chip_init)(struct charge_init_data *init_crit);
	int (*set_input_current)(int value);
	int (*set_charge_current)(int value);
	int (*dev_check)(void);
	int (*set_terminal_voltage)(int value);
	int (*set_dpm_voltage)(int value);
	int (*set_terminal_current)(int value);
	int (*set_charge_enable)(int enable);

	int (*set_otg_enable)(int enable);
	int (*set_term_enable)(int enable);
	int (*get_charge_state)(unsigned int *state);
	int (*reset_watchdog_timer)(void);
	int (*set_watchdog_timer)(int value);
	int (*set_batfet_disable)(int disable);
	int (*get_ibus)(void);
	int (*get_vbus)(unsigned int *value);

	int (*check_input_dpm_state)(void);
	int (*check_input_vdpm_state)(void);
	int (*check_input_idpm_state)(void);
	int (*set_charger_hiz)(int enable);
	int (*set_otg_current)(int value);
	int (*stop_charge_config)(void);
	int (*set_otg_switch_mode_enable)(int enable);
	int (*get_vbat_sys)(void);
	int (*set_vbus_vset)(u32);
	int (*set_uvp_ovp)(void);

	int (*set_force_term_enable)(int enable);
	int (*get_charger_state)(void);
	int (*soft_vbatt_ovp_protect)(void);
	int (*rboost_buck_limit)(void);
	int (*get_charge_current)(void);
	int (*get_iin_set)(void);
	int (*get_dieid)(char *dieid, unsigned int len);
	int (*get_vbat)(void);
	int (*get_terminal_voltage)(void);
	int (*get_vusb)(int *value);
	int (*set_pretrim_code)(int val);
	int (*get_dieid_for_nv)(u8 *dieid, unsigned int len);

	int (*get_register_head)(char *reg_head, int size);
	int (*dump_register)(char *reg_value, int size);
	int (*fcp_chip_init)(void);

	int (*get_charge_enable_status)(void);
	int (*set_covn_start)(int enable);
	int (*check_charger_plugged)(void);
};

struct fcp_adapter_device_ops {
	int (*get_adapter_output_current)(void);
	int (*set_adapter_output_vol)(int *);
	int (*detect_adapter)(void);
	int (*is_support_fcp)(void);
	int (*switch_chip_reset)(void);
	int (*fcp_adapter_reset)(void);
	int (*stop_charge_config)(void);
	int (*is_fcp_charger_type)(void);
	int (*fcp_read_adapter_status)(void);
	int (*fcp_read_switch_status)(void);
};

struct charge_device_info {
	struct device *dev;
	struct notifier_block usb_nb;
	struct notifier_block fault_nb;
	struct delayed_work charge_work;
	struct delayed_work otg_work;
	struct work_struct usb_work;
	struct work_struct fault_work;
	struct charge_device_ops *ops;
	struct fcp_adapter_device_ops *fcp_ops;
	struct charge_core_data *core_data;
	struct charge_sysfs_data sysfs_data;

	enum huawei_usb_charger_type charger_type;
	enum power_supply_type charger_source;
	enum charge_fault_type charge_fault;
	unsigned int charge_enable;
	unsigned int input_current;
	unsigned int charge_current;
	unsigned int check_full_count;
};

struct sysfs_store_handle {
	u8 name;
	int (*func)(struct charge_device_info *di, const char *buf);
};

enum charge_wakelock_flag {
	CHARGE_NEED_WAKELOCK,
	CHARGE_NO_NEED_WAKELOCK,
};
#define WDT_TIME_40_S 40
#define WDT_STOP 0
/****************variable and function declarationn area******************/
extern struct atomic_notifier_head fault_notifier_list;
int charge_ops_register(struct charge_device_ops *ops);
int charge_check_charger_plugged(void);
int charge_check_input_dpm_state(void);
int fcp_adapter_ops_register(struct fcp_adapter_device_ops *ops);
int charge_set_charge_state(int state);
void charge_type_dcp_detected_notify(void);
bool charge_check_charger_ts(void);
bool charge_check_charger_otg_state(void);
unsigned int charge_get_charger_type(void);

#ifndef CONFIG_HUAWEI_CHARGER_AP
enum power_event_atomic_notifier_type {
	POWER_ANT_BEGIN = 0,
	POWER_ANT_CHARGE_FAULT = POWER_ANT_BEGIN,
	POWER_ANT_LVC_FAULT,
	POWER_ANT_SC_FAULT,
	POWER_ANT_UVDM_FAULT,
	POWER_ANT_END,
};
enum power_event_notifier_list {
	POWER_NE_BEGIN = 0,
	/* section: for connect */
	POWER_NE_USB_DISCONNECT = POWER_NE_BEGIN,
	POWER_NE_USB_CONNECT,
	POWER_NE_WIRELESS_DISCONNECT,
	POWER_NE_WIRELESS_CONNECT,
	POWER_NE_WIRELESS_TX_START,
	POWER_NE_WIRELESS_TX_STOP, /* num:5 */
	/* section: for charging */
	POWER_NE_CHARGING_START,
	POWER_NE_CHARGING_STOP,
	POWER_NE_CHARGING_SUSPEND,
	/* section: for soc decimal */
	POWER_NE_SOC_DECIMAL_DC,
	POWER_NE_SOC_DECIMAL_WL_DC, /* num:10 */
	/* section: for water detect */
	POWER_NE_WD_REPORT_DMD,
	POWER_NE_WD_REPORT_UEVENT,
	POWER_NE_WD_DETECT_BY_USB_DP_DN,
	POWER_NE_WD_DETECT_BY_USB_ID,
	POWER_NE_WD_DETECT_BY_USB_GPIO, /* num:15 */
	POWER_NE_WD_DETECT_BY_AUDIO_DP_DN,
	/* section: for uvdm */
	POWER_NE_UVDM_RECEIVE,
	/* section: for direct charger */
	POWER_NE_DC_CHECK_START,
	POWER_NE_DC_CHECK_SUCC,
	POWER_NE_DC_LVC_CHARGING, /* num:20 */
	POWER_NE_DC_SC_CHARGING,
	POWER_NE_DC_STOP_CHARGE,
	/* section: for lightstrap */
	POWER_NE_LIGHTSTRAP_ON,
	POWER_NE_LIGHTSTRAP_OFF,
	POWER_NE_LIGHTSTRAP_GET_PRODUCT_TYPE, /* num:25 */
	POWER_NE_LIGHTSTRAP_EPT,
	/* section: for otg */
	POWER_NE_OTG_SC_CHECK_STOP,
	POWER_NE_OTG_SC_CHECK_START,
	POWER_NE_OTG_OCP_HANDLE,
	/* section: for wireless charge */
	POWER_NE_WLC_CHARGER_VBUS, /* num:30 */
	POWER_NE_WLC_ICON_TYPE,
	POWER_NE_WLC_TX_VSET,
	POWER_NE_WLC_READY,
	POWER_NE_WLC_HS_SUCC,
	POWER_NE_WLC_TX_CAP_SUCC, /* num:35 */
	POWER_NE_WLC_CERT_SUCC,
	POWER_NE_WLC_DC_START_CHARGING,
	POWER_NE_WLC_VBUS_CONNECT,
	POWER_NE_WLC_VBUS_DISCONNECT,
	POWER_NE_WLC_WIRED_VBUS_CONNECT, /* num:40 */
	POWER_NE_WLC_WIRED_VBUS_DISCONNECT,
	/* section: for wireless tx */
	POWER_NE_WLTX_GET_CFG,
	POWER_NE_WLTX_HANDSHAKE_SUCC,
	POWER_NE_WLTX_CHARGEDONE,
	POWER_NE_WLTX_CEP_TIMEOUT, /* num:45 */
	POWER_NE_WLTX_EPT_CMD,
	POWER_NE_WLTX_OVP,
	POWER_NE_WLTX_OCP,
	POWER_NE_WLTX_PING_RX,
	POWER_NE_WLTX_HALL_APPROACH, /* num:50 */
	POWER_NE_WLTX_AUX_PEN_HALL_APPROACH,
	POWER_NE_WLTX_AUX_KB_HALL_APPROACH,
	POWER_NE_WLTX_HALL_AWAY_FROM,
	POWER_NE_WLTX_AUX_PEN_HALL_AWAY_FROM,
	POWER_NE_WLTX_AUX_KB_HALL_AWAY_FROM, /* num:55 */
	POWER_NE_WLTX_ACC_DEV_CONNECTED,
	POWER_NE_WLTX_RCV_DPING,
	POWER_NE_WLTX_ASK_SET_VTX,
	POWER_NE_WLTX_GET_TX_CAP,
	POWER_NE_WLTX_TX_FOD, /* num:60 */
	POWER_NE_WLTX_RP_DM_TIMEOUT,
	POWER_NE_WLTX_TX_INIT,
	POWER_NE_WLTX_TX_AP_ON,
	POWER_NE_WLTX_IRQ_SET_VTX,
	POWER_NE_WLTX_GET_RX_PRODUCT_TYPE, /* num:65 */
	POWER_NE_WLTX_GET_RX_MAX_POWER,
	POWER_NE_WLTX_ASK_RX_EVT,
	/* section: for wireless rx */
	POWER_NE_WLRX_PWR_ON,
	POWER_NE_WLRX_READY,
	POWER_NE_WLRX_OCP, /* num:70 */
	POWER_NE_WLRX_OVP,
	POWER_NE_WLRX_OTP,
	POWER_NE_WLRX_LDO_OFF,
	POWER_NE_WLRX_TX_ALARM,
	POWER_NE_WLRX_TX_BST_ERR, /* num:75 */
	/* section: for charger */
	POWER_NE_CHG_START_CHARGING,
	POWER_NE_CHG_STOP_CHARGING,
	POWER_NE_CHG_CHARGING_DONE,
	POWER_NE_CHG_PRE_STOP_CHARGING,
	/* section: for coul */
	POWER_NE_COUL_LOW_VOL, /* num:80 */
	/* section: for charger fault */
	POWER_NE_CHG_FAULT_NON,
	POWER_NE_CHG_FAULT_BOOST_OCP,
	POWER_NE_CHG_FAULT_VBAT_OVP,
	POWER_NE_CHG_FAULT_SCHARGER,
	POWER_NE_CHG_FAULT_I2C_ERR, /* num:85 */
	POWER_NE_CHG_FAULT_WEAKSOURCE,
	POWER_NE_CHG_FAULT_CHARGE_DONE,
	/* section: for direct charger fault */
	POWER_NE_DC_FAULT_NON,
	POWER_NE_DC_FAULT_VBUS_OVP,
	POWER_NE_DC_FAULT_REVERSE_OCP, /* num:90 */
	POWER_NE_DC_FAULT_OTP,
	POWER_NE_DC_FAULT_TSBUS_OTP,
	POWER_NE_DC_FAULT_TSBAT_OTP,
	POWER_NE_DC_FAULT_TDIE_OTP,
	POWER_NE_DC_FAULT_INPUT_OCP, /* num:95 */
	POWER_NE_DC_FAULT_VDROP_OVP,
	POWER_NE_DC_FAULT_AC_OVP,
	POWER_NE_DC_FAULT_VBAT_OVP,
	POWER_NE_DC_FAULT_IBAT_OCP,
	POWER_NE_DC_FAULT_IBUS_OCP, /* num:100 */
	POWER_NE_DC_FAULT_CONV_OCP,
	POWER_NE_DC_FAULT_LTC7820,
	POWER_NE_DC_FAULT_INA231,
	POWER_NE_DC_FAULT_CC_SHORT,
	/* section: for uvdm charger fault */
	POWER_NE_UVDM_FAULT_OTG, /* num:105 */
	POWER_NE_UVDM_FAULT_COVER_ABNORMAL,
	/* section: for typec */
	POWER_NE_TYPEC_CURRENT_CHANGE,
	POWER_NE_END,
};

static void inline power_event_anc_notify(unsigned int type,
					  unsigned long event, void *data){};

#endif

#endif
