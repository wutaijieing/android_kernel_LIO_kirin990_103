// SPDX-License-Identifier: GPL-2.0
/*
 * charge_manager.c
 *
 * charge manager module
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

#include <chipset_common/hwpower/charger/charge_manager.h>
#include <chipset_common/hwpower/common_module/power_cmdline.h>
#include <chipset_common/hwpower/common_module/power_common_macro.h>
#include <chipset_common/hwpower/common_module/power_icon.h>
#include <chipset_common/hwpower/common_module/power_supply_interface.h>
#include <chipset_common/hwpower/common_module/power_time.h>
#include <chipset_common/hwpower/common_module/power_wakeup.h>
#include <chipset_common/hwpower/hardware_channel/vbus_channel.h>
#include <chipset_common/hwpower/hvdcp_charge/hvdcp_charge.h>
#include <huawei_platform/hwpower/common_module/power_platform.h>
#include <huawei_platform/hwpower/power_proxy.h>
#include <huawei_platform/power/direct_charger/direct_charger.h>
#include <huawei_platform/power/wireless/wireless_charger.h>
#include <huawei_platform/usb/hw_pd_dev.h>

#define HWLOG_TAG charge_manager
HWLOG_REGIST();

#define MONITOR_CHARGING_DELAY_TIME         10000
#define MONITOR_CHARGING_QUICKEN_DELAY_TIME 1000
#define MONITOR_CHARGING_WAITPD_TIMEOUT     2000
#define MONITOR_CHARGING_QUICKEN_TIMES      1
#define CHG_WAIT_PD_TIME                    6
#define PD_TO_SCP_MAX_COUNT                 5

static struct charge_manager_info *g_di;
struct completion g_emark_detect_comp;
static time64_t g_boot_time;
static struct power_supply_desc g_usb_psy_desc;

static void charge_update_usb_psy_type(unsigned int type)
{
	if (g_usb_psy_desc.type == type)
		return;

	hwlog_info("%s: update usb_psy_type = %d\n", __func__, type);
	g_usb_psy_desc.type = type;
	power_supply_changed(g_di->usb_psy);
}

#ifdef SNDJ
static void water_detection_entrance(void)
{
#ifdef CONFIG_HUAWEI_WATER_CHECK
	power_event_bnc_notify(POWER_BNT_WD, POWER_NE_WD_DETECT_BY_USB_GPIO, NULL);
#endif /* CONFIG_HUAWEI_WATER_CHECK */
	power_event_bnc_notify(POWER_BNT_WD, POWER_NE_WD_DETECT_BY_USB_ID, NULL);
	power_event_bnc_notify(POWER_BNT_WD, POWER_NE_WD_DETECT_BY_AUDIO_DP_DN, NULL);
}
#endif

static bool charger_event_check(struct charge_manager_info *di,
	enum charger_event_type new_event)
{
	bool ret = false;

	if (di->event == CHARGER_MAX_EVENT)
		return true;

	switch (new_event) {
	case START_SINK:
		if ((di->event == STOP_SINK) || (di->event == STOP_SOURCE) ||
			(di->event == START_SINK_WIRELESS) || (di->event == STOP_SINK_WIRELESS))
			ret = true;
		break;
	case STOP_SINK:
		if (di->event == START_SINK)
			ret = true;
		break;
	case START_SOURCE:
		if ((di->event == STOP_SINK) || (di->event == STOP_SOURCE) ||
			(di->event == START_SINK_WIRELESS) || (di->event == STOP_SINK_WIRELESS))
			ret = true;
		break;
	case STOP_SOURCE:
		if ((di->event == START_SOURCE) ||
			(di->event == START_SINK_WIRELESS) || (di->event == STOP_SINK_WIRELESS))
			ret = true;
		break;
	case START_SINK_WIRELESS:
		if ((di->event == START_SOURCE) || (di->event == STOP_SOURCE) ||
			(di->event == STOP_SINK) || (di->event == STOP_SINK_WIRELESS))
			ret = true;
		break;
	case STOP_SINK_WIRELESS:
		if ((di->event == START_SINK_WIRELESS) ||
			(di->event == START_SOURCE) || (di->event == STOP_SOURCE))
			ret = true;
		break;
	default:
		hwlog_err("%s: error event:%d\n", __func__, new_event);
		break;
	}

	return ret;
}

static void charge_direct_charge_check(unsigned int type, int flag)
{
	if (type != CHARGER_TYPE_STANDARD)
		return;

	if (charge_get_pd_init_flag())
		return;

	if (charge_get_monitor_work_flag() == CHARGE_MONITOR_WORK_NEED_STOP)
		return;

	if (!direct_charge_get_stop_charging_complete_flag())
		return;

	direct_charge_check(flag);
}

#ifdef CONFIG_TCPC_CLASS
static int charger_disable_usbpd(bool disable)
{
	(void)charge_set_vbus_vset(ADAPTER_5V);

	pd_dpm_disable_pd(disable);
	adapter_set_usbpd_enable(ADAPTER_PROTOCOL_SCP, !disable);
	hwlog_info("%s\n", __func__);
	return 0;
}

static void charger_switch_type_to_standard(void)
{
	if (charge_get_monitor_work_flag() == CHARGE_MONITOR_WORK_NEED_STOP)
		return;

	charge_set_pd_charge_flag(false);
	charge_set_charger_type(CHARGER_TYPE_STANDARD);
	charge_update_usb_psy_type(POWER_SUPPLY_TYPE_USB_DCP);
	hwlog_info("%s CHARGER_TYPE_STANDARD\n", __func__);
}

static void charge_try_pd2scp(struct charge_manager_info *di)
{
	if (!pd_dpm_get_ctc_cable_flag())
		return;

	if (di->try_pd_to_scp_counter <= 0)
		return;

	charge_set_pd_init_flag(false);

	if (charge_get_monitor_work_flag() == CHARGE_MONITOR_WORK_NEED_STOP) {
		di->try_pd_to_scp_counter = 0;
		return;
	}

	hwlog_info("%s try_pd_to_scp\n", __func__);
	if (direct_charge_in_charging_stage() == DC_IN_CHARGING_STAGE) {
		hwlog_info("ignore pre_check\n");
		charger_switch_type_to_standard();
		di->try_pd_to_scp_counter = 0;
	} else if (!direct_charge_pre_check()) {
		if (pd_dpm_get_emark_detect_enable()) {
			pd_dpm_detect_emark_cable();
			/* wait 200ms to get cable vdo */
			if (!wait_for_completion_timeout(
				&g_emark_detect_comp,
				msecs_to_jiffies(200)))
				hwlog_info("emark timeout\n");

			reinit_completion(&g_emark_detect_comp);
			pd_dpm_detect_emark_cable_finish();
		}

		if (direct_charge_in_charging_stage() ==
			DC_NOT_IN_CHARGE_DONE_STAGE)
			charger_disable_usbpd(true);
		msleep(800); /* Wait 800ms for BC1.2 complete */
		charger_switch_type_to_standard();
		di->try_pd_to_scp_counter = 0;
	} else {
		di->try_pd_to_scp_counter--;
	}

	if (di->try_pd_to_scp_counter < 0)
		di->try_pd_to_scp_counter = 0;
}
#else
static inline void charge_try_pd2scp(struct charge_manager_info *di)
{
}
#endif


static void charge_monitor_work(struct work_struct *work)
{
	struct charge_manager_info *di = container_of(work, struct charge_manager_info,
		charge_work.work);
	unsigned int charge_type = charge_get_charger_type();

	if (charge_get_pd_init_flag()) {
		/* wait a moment for PD init and detect PD adaptor */
		if (power_get_monotonic_boottime() - g_boot_time > CHG_WAIT_PD_TIME)
			charge_set_pd_init_flag(false);
	}

	hwlog_info("%s enter, charge_type = %d\n", __func__, charge_type);
	charge_set_monitor_work_flag(CHARGE_MONITOR_WORK_NEED_START);

	/* if support both SCP and PD, try to control adapter by SCP */
	charge_try_pd2scp(di);

/* todo
	rc = power_supply_get_property_value_with_psy(info->bk_batt_psy,
		POWER_SUPPLY_PROP_FLASH_ACTIVE, &val);
	if (rc == 0 && val.intval)
		info->enable_hv_charging = 0;
	hwlog_info("%s flash val.intval= %d  info->enable_hv_charging = %d\n", __func__,
		val.intval, info->enable_hv_charging);
*/

	charge_type = charge_get_charger_type();
	hwlog_info("%s di->enable_hv_charging = %d, charge_type=%d\n", __func__,
		di->enable_hv_charging, charge_type);
	if (direct_charge_in_charging_stage() == DC_NOT_IN_CHARGING_STAGE)
		charge_direct_charge_check(charge_type, di->enable_hv_charging);

	if (direct_charge_in_charging_stage() == DC_NOT_IN_CHARGING_STAGE)
		buck_charge_entry();

	hwlog_err("%s charge_type=%d, charging_stage=%d, standard=%d\n", __func__,
		charge_type, direct_charge_in_charging_stage(),
		CHARGER_TYPE_STANDARD);

	/* here implement jeita standard */
	if ((charge_get_quicken_work_flag() == MONITOR_CHARGING_QUICKEN_TIMES) ||
		(di->try_pd_to_scp_counter > 0))
		schedule_delayed_work(&di->charge_work,
			msecs_to_jiffies(MONITOR_CHARGING_QUICKEN_DELAY_TIME));
	else if (charge_get_pd_init_flag())
		schedule_delayed_work(&di->charge_work,
			msecs_to_jiffies(MONITOR_CHARGING_WAITPD_TIMEOUT));
	else
		schedule_delayed_work(&di->charge_work,
			msecs_to_jiffies(MONITOR_CHARGING_DELAY_TIME));
}

static void charge_start_charging(struct charge_manager_info *di)
{
	hwlog_info("---->START CHARGING\n");
	power_wakeup_lock(di->charge_lock, false);

	buck_charge_init_chip();

	mod_delayed_work(system_power_efficient_wq, &di->charge_work, 0);

	/* notify start charging event */
	power_event_bnc_notify(POWER_BNT_CHARGING, POWER_NE_CHARGING_START, NULL);
	(void)direct_charge_set_stage_status_default();
	power_ui_int_event_notify(POWER_UI_NE_MAX_POWER, 0);
}

static void charge_stop_charging(struct charge_manager_info *di)
{
	hwlog_info("---->STOP CHARGING\n");
	charge_set_monitor_work_flag(CHARGE_MONITOR_WORK_NEED_STOP);
	power_wakeup_lock(di->charge_lock, false);

	power_icon_notify(ICON_TYPE_INVALID);
	/* notify stop charging event */
	power_event_bnc_notify(POWER_BNT_CHARGING, POWER_NE_CHARGING_STOP, NULL);
	/* notify event to power ui */
	power_ui_event_notify(POWER_UI_NE_DEFAULT, NULL);

	if (adapter_set_default_param(ADAPTER_PROTOCOL_FCP))
		hwlog_err("fcp set default param failed\n");

	if (di->force_disable_dc_path && (direct_charge_in_charging_stage() ==
		DC_IN_CHARGING_STAGE))
		direct_charge_force_disable_dc_path();
	if (!direct_charge_get_cutoff_normal_flag())
		direct_charge_set_stage_status_default();

	buck_charge_stop_charging();

	charge_otg_mode_enable(CHARGE_OTG_DISABLE, VBUS_CH_USER_WIRED_OTG);

	cancel_delayed_work_sync(&di->charge_work);
	charge_set_monitor_work_flag(CHARGE_MONITOR_WORK_NEED_START);

	if (!direct_charge_get_cutoff_normal_flag())
		direct_charge_exit();
	direct_charge_update_cutoff_normal_flag();
	dc_set_adapter_default_param();

	power_wakeup_unlock(di->charge_lock, false);
}

void emark_detect_complete(void)
{
	complete(&g_emark_detect_comp);
}

static void charger_type_handler(unsigned long type, void *data)
{
	struct charge_manager_info *di = g_di;
	struct pd_dpm_vbus_state *vbus_state = NULL;
	int need_resched_work = 0;
	unsigned int charger_type = charge_get_charger_type();

	if (!di)
		return;

	hwlog_info("%s type = %lu\n", __func__, type);
	switch (type) {
	case CHARGER_TYPE_USB:
		if ((charger_type == CHARGER_TYPE_USB) ||
			(charger_type == CHARGER_TYPE_WIRELESS) ||
			(charger_type == CHARGER_REMOVED)) {
			charge_set_charger_type(CHARGER_TYPE_USB);
			charge_update_usb_psy_type(POWER_SUPPLY_TYPE_USB);
			hwlog_info("%s case = CHARGER_TYPE_SDP\n", __func__);
			need_resched_work = 1;
		}
		break;
	case CHARGER_TYPE_BC_USB:
		if ((charger_type == CHARGER_TYPE_NON_STANDARD) ||
			(charger_type == CHARGER_TYPE_WIRELESS) ||
			(charger_type == CHARGER_REMOVED) ||
			(charger_type == CHARGER_TYPE_USB)) {
			/*
			 * factory equipment has usb reset issues
			 * when bc1.2 type is CDP
			 */
			if (power_cmdline_is_factory_mode())
				charge_set_charger_type(CHARGER_TYPE_USB);
			else
				charge_set_charger_type(CHARGER_TYPE_BC_USB);

			charge_update_usb_psy_type(POWER_SUPPLY_TYPE_USB_CDP);
			hwlog_info("%s case = CHARGER_TYPE_CDP\n", __func__);
			need_resched_work = 1;
		}
		break;
	case CHARGER_TYPE_STANDARD:
		if ((charger_type != CHARGER_TYPE_STANDARD) &&
			(charger_type != CHARGER_TYPE_PD) &&
			(charger_type != CHARGER_TYPE_FCP)) {
			charge_set_charger_type(CHARGER_TYPE_STANDARD);
			charge_update_usb_psy_type(POWER_SUPPLY_TYPE_USB_DCP);
			need_resched_work = 1;
			hwlog_info("%s case = CHARGER_TYPE_DCP\n", __func__);
		}
		if (charger_type == CHARGER_TYPE_PD) {
			need_resched_work = 1;
			di->try_pd_to_scp_counter = PD_TO_SCP_MAX_COUNT;
		}
		break;
	case CHARGER_TYPE_NON_STANDARD:
		if ((charger_type == CHARGER_TYPE_WIRELESS) ||
			(charger_type == CHARGER_TYPE_USB) ||
			(charger_type == CHARGER_REMOVED)) {
			charge_set_charger_type(CHARGER_TYPE_NON_STANDARD);
			charge_update_usb_psy_type(POWER_SUPPLY_TYPE_USB_DCP);
			need_resched_work = 1;
			hwlog_info("%s case = CHARGER_TYPE_NONSTANDARD\n", __func__);
		}
		break;

	case PD_DPM_VBUS_TYPE_PD:
		vbus_state = (struct pd_dpm_vbus_state *) data;
		buck_charge_set_pd_vbus_state(vbus_state);
		charge_set_charger_type(CHARGER_TYPE_PD);
		if (vbus_state->ext_power)
			charge_update_usb_psy_type(POWER_SUPPLY_TYPE_USB_PD);
		else
			charge_update_usb_psy_type(POWER_SUPPLY_TYPE_USB);

		if (charger_type == CHARGER_TYPE_STANDARD)
			di->try_pd_to_scp_counter = PD_TO_SCP_MAX_COUNT;
		need_resched_work = 1;
		hwlog_info("%s case = CHARGER_TYPE_PD\n", __func__);
		break;
	case CHARGER_TYPE_WIRELESS:
		charge_set_charger_type(CHARGER_TYPE_WIRELESS);
		charge_update_usb_psy_type(POWER_SUPPLY_TYPE_WIRELESS);
		need_resched_work = 1;
		hwlog_info("%s case = CHARGER_TYPE_WIRELESS\n", __func__);
		break;
	default:
		hwlog_info("%s ignore type = %lu\n", __func__, type);
		return;
	}

	if (need_resched_work) {
		hwlog_info("%s mod charge work\n", __func__);
		mod_delayed_work(system_power_efficient_wq, &di->charge_work, 0);
	}
}

static void charger_handle_event(struct charge_manager_info *di, enum charger_event_type event)
{
	switch (event) {
	case START_SINK:
		power_event_bnc_notify(POWER_BNT_CONNECT, POWER_NE_USB_CONNECT, NULL);
		power_icon_notify(ICON_TYPE_NORMAL);
		charger_type_handler(CHARGER_TYPE_SDP, NULL);
		charge_start_charging(di);
		break;
	case START_SINK_WIRELESS:
		power_event_bnc_notify(POWER_BNT_CONNECT, POWER_NE_WIRELESS_CONNECT, NULL);
		power_icon_notify(ICON_TYPE_WIRELESS_NORMAL);
		mutex_lock(&di->event_type_lock);
		charge_update_usb_psy_type(POWER_SUPPLY_TYPE_WIRELESS);
		charge_set_charger_type(CHARGER_TYPE_WIRELESS);
		hwlog_info("%s start display wireless charge\n", __func__);
		charge_send_uevent(NO_EVENT);
		mutex_unlock(&di->event_type_lock);
		charge_start_charging(di);
		break;
	case STOP_SINK:
		power_event_bnc_notify(POWER_BNT_CONNECT, POWER_NE_USB_DISCONNECT, NULL);
		power_event_bnc_notify(POWER_BNT_CHG, POWER_NE_CHG_PRE_STOP_CHARGING, NULL);
		wireless_charge_wired_vbus_disconnect_handler();
		/* fall-through */
	case STOP_SINK_WIRELESS:
		power_event_bnc_notify(POWER_BNT_CONNECT, POWER_NE_WIRELESS_DISCONNECT, NULL);
		mutex_lock(&di->event_type_lock);
		charge_update_usb_psy_type(POWER_SUPPLY_TYPE_BATTERY);
		charge_set_charger_type(CHARGER_REMOVED);
		/* update the icon first when charger removed */
		power_icon_notify(ICON_TYPE_INVALID);
		charge_send_uevent(NO_EVENT);
		charge_stop_charging(di);
		mutex_unlock(&di->event_type_lock);
		break;
	case START_SOURCE:
		power_wakeup_unlock(di->charge_lock, false);
		charge_otg_mode_enable(CHARGE_OTG_ENABLE, VBUS_CH_USER_WIRED_OTG);
		break;
	case STOP_SOURCE:
		charge_stop_charging(di);
		break;
	default:
		hwlog_err("%s: error event:%d\n", __func__, event);
		break;
	}
}

static void charger_event_work(struct work_struct *work)
{
	unsigned long flags;
	enum charger_event_type event;
	struct charge_manager_info *di = g_di;

	hwlog_info("%s+\n", __func__);

	while (!charger_event_queue_isempty(&di->event_queue)) {
		spin_lock_irqsave(&(di->event_spin_lock), flags);
		event = charger_event_dequeue(&di->event_queue);
		spin_unlock_irqrestore(&(di->event_spin_lock), flags);

		charger_handle_event(di, event);
	}

	spin_lock_irqsave(&(di->event_spin_lock), flags);
	charger_event_queue_clear_overlay(&di->event_queue);
	spin_unlock_irqrestore(&(di->event_spin_lock), flags);
	hwlog_info("%s-\n", __func__);
}

void charger_source_sink_event(enum charger_event_type event)
{
	unsigned long flags;
	struct charge_manager_info *di = g_di;

	if (!di) {
		hwlog_info("%s di is NULL\n", __func__);
		return;
	}

	if (!charger_event_check(di, event)) {
		hwlog_err("last event: [%s], event [%s] was rejected\n",
			charger_event_type_string(di->event),
			charger_event_type_string(event));
		return;
	}

	spin_lock_irqsave(&(di->event_spin_lock), flags);

	if ((event == START_SINK) || (event == START_SINK_WIRELESS))
		charge_set_charger_online(1);
	else
		charge_set_charger_online(0);

	hwlog_info("case = %s\n", charger_event_type_string(event));
	di->event = event;

	if (!charger_event_enqueue(&di->event_queue, event)) {
		if (!queue_work(system_power_efficient_wq, &di->event_work))
			hwlog_err("schedule event_work wait:%d\n", event);
		power_wakeup_lock(di->charge_lock, false);
	} else {
		hwlog_err("%s: can't enqueue event:%d\n", __func__, event);
	}

	if ((event == STOP_SOURCE) || (event == STOP_SINK) ||
		(event == STOP_SINK_WIRELESS))
		charger_event_queue_set_overlay(&di->event_queue);

	spin_unlock_irqrestore(&(di->event_spin_lock), flags);
}

#ifdef CONFIG_TCPC_CLASS
int is_pd_supported(void)
{
	if (!g_di)
		return 0;

	return g_di->charger_pd_support;
}

static int pd_dpm_notifier_call(struct notifier_block *tcpc_nb, unsigned long event, void *data)
{
	struct charge_manager_info *di = container_of(tcpc_nb, struct charge_manager_info, tcpc_nb);

	hwlog_info("%s ++ , event = %ld\n", __func__, event);

	if ((di->event == START_SINK) && (event == CHARGER_TYPE_DCP) &&
		(charge_get_charger_type() == CHARGER_TYPE_PD)) {
		di->try_pd_to_scp_counter = PD_TO_SCP_MAX_COUNT;
		hwlog_info("%s try_pd_to_scp\n", __func__);
		mod_delayed_work(system_power_efficient_wq, &di->charge_work, 0);
		return NOTIFY_OK;
	}

	if (event == PD_DPM_VBUS_TYPE_PD)
		charge_set_pd_charge_flag(false);

	mutex_lock(&di->event_type_lock);
	if (charge_get_charger_online())
		charger_type_handler(event, data);
	mutex_unlock(&di->event_type_lock);
	return NOTIFY_OK;
}
#endif

static int set_charge_manager_property(struct power_supply *psy,
	enum power_supply_property psp, const union power_supply_propval *val)
{
	enum charger_event_type event;
	int charger_type = CHARGER_REMOVED;

	hwlog_info("%s prop = %d, val->intval = %d\n", __func__, psp, val->intval);

	switch (psp) {
	case POWER_SUPPLY_PROP_CHG_PLUGIN:
		hwlog_info("%s POWER_SUPPLY_PROP_CHG_PLUGIN\n", __func__);
		event = (enum charger_event_type)val->intval;
		charger_source_sink_event(event);
		return 0;
	case POWER_SUPPLY_PROP_CHG_TYPE:
		hwlog_info("%s POWER_SUPPLY_PROP_CHG_TYPE type = %d, standard=%d\n", __func__,
			val->intval, CHARGER_TYPE_STANDARD);
		charger_type = val->intval;
		if (charge_get_charger_online())
			charger_type_handler(charger_type, NULL);
		return 0;
	default:
		return 0;
	}
}

static int get_charge_manager_property(struct power_supply *psy,
	enum power_supply_property psp, union power_supply_propval *val)
{
	return 0;
}

static int charge_manager_property_is_writeable(struct power_supply *psy,
	enum power_supply_property prop)
{
	return 0;
}

static enum power_supply_property power_supply_charge_manager_props[] = {
	POWER_SUPPLY_PROP_CHG_PLUGIN,
	POWER_SUPPLY_PROP_CHG_TYPE,
};

static struct power_supply_desc g_charge_manager_psy_desc = {
	.name = "charge_manager",
	.type = POWER_SUPPLY_TYPE_UNKNOWN,
	.properties = power_supply_charge_manager_props,
	.num_properties = ARRAY_SIZE(power_supply_charge_manager_props),
	.get_property = get_charge_manager_property,
	.set_property = set_charge_manager_property,
	.property_is_writeable = charge_manager_property_is_writeable,
};

static int set_usb_property(struct power_supply *psy,
	enum power_supply_property psp, const union power_supply_propval *val)
{
	struct charge_manager_info *di = g_di;

	if (!di) {
		hwlog_err("null point in [%s]\n", __func__);
		return -EINVAL;
	}

	hwlog_info("%s prop = %d, val->intval = %d\n", __func__, psp, val->intval);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		hwlog_info("%s set_usb_property online=%d\n", __func__, val->intval);
		di->usb_online = val->intval;
		return 0;
	default:
		return 0;
	}
}

static int get_usb_property(struct power_supply *psy,
	enum power_supply_property psp, union power_supply_propval *val)
{
	struct charge_manager_info *di = g_di;

	if (!di) {
		hwlog_err("null point in [%s]\n", __func__);
		return -EINVAL;
	}

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = di->usb_online;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = charge_get_vusb() * POWER_UV_PER_MV;
		break;
	default:
		return 0;
	}

	return 0;
}

static int usb_property_is_writeable(struct power_supply *psy,
	enum power_supply_property prop)
{
	return 0;
}

static enum power_supply_property power_supply_usb_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
};

static struct power_supply_desc g_usb_psy_desc = {
	.name = "usb",
	.type = POWER_SUPPLY_TYPE_USB,
	.properties = power_supply_usb_props,
	.num_properties = ARRAY_SIZE(power_supply_usb_props),
	.get_property = get_usb_property,
	.set_property = set_usb_property,
	.property_is_writeable = usb_property_is_writeable,
};

static int charge_manager_init_psy(struct charge_manager_info *di)
{
	di->charge_psy = power_supply_register(di->dev,
		&g_charge_manager_psy_desc, NULL);
	if (IS_ERR(di->charge_psy))
		return -EPERM;

	if (di->support_usb_psy) {
		di->usb_psy = power_supply_register(di->dev,
			&g_usb_psy_desc, NULL);
		if (IS_ERR(di->usb_psy))
			return -EPERM;
	}

	return 0;
}

static void charge_manger_parse_dts(struct device_node *np,
	struct charge_manager_info *di)
{
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"support_usb_psy", &di->support_usb_psy, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"pd_support", &di->charger_pd_support, 0);
}

static int charge_manager_probe(struct platform_device *pdev)
{
	struct charge_manager_info *di = NULL;
	struct device_node *np = NULL;
#ifdef CONFIG_TCPC_CLASS
	unsigned long local_event;
	struct pd_dpm_vbus_state local_state;
#endif
	int ret = 0;

	if (!pdev || !pdev->dev.of_node)
		return -ENODEV;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	di->dev = &pdev->dev;
	np = di->dev->of_node;

	charge_manger_parse_dts(np, di);
	if (charge_manager_init_psy(di))
		goto fail_register_psy;

	di->charge_lock = power_wakeup_source_register(di->dev, "charge_wakelock");
	init_completion(&g_emark_detect_comp);

	if (charger_event_queue_create(&di->event_queue, MAX_EVENT_COUNT))
		goto fail_create_event_queue;

	spin_lock_init(&di->event_spin_lock);
	di->event = CHARGER_MAX_EVENT;
	mutex_init(&di->event_type_lock);
	INIT_WORK(&di->event_work, charger_event_work);
	INIT_DELAYED_WORK(&di->charge_work, charge_monitor_work);

#ifdef CONFIG_TCPC_CLASS
	if (di->charger_pd_support) {
		di->tcpc_nb.notifier_call = pd_dpm_notifier_call;
		ret = register_pd_dpm_notifier(&di->tcpc_nb);
		if (ret < 0)
			hwlog_err("register_pd_dpm_notifier failed\n");
		else
			hwlog_info("register_pd_dpm_notifier OK\n");
	}
#endif

	charge_set_charger_type(CHARGER_REMOVED);
	charge_set_pd_init_flag(true);
	if (di->event == CHARGER_MAX_EVENT) {
#ifdef CONFIG_WIRELESS_CHARGER
		if (wlrx_ic_is_tx_exist(WLTRX_IC_MAIN)) {
			charger_source_sink_event(START_SINK_WIRELESS);
		} else {
#endif
#ifdef CONFIG_TCPC_CLASS
			if (di->charger_pd_support) {
				charger_source_sink_event(pd_dpm_get_source_sink_state());
				pd_dpm_get_charge_event(&local_event, &local_state);
				pd_dpm_notifier_call(&(di->tcpc_nb), local_event, &local_state);
			}
#endif
#ifdef CONFIG_WIRELESS_CHARGER
		}
#endif
	}

	g_boot_time = power_get_monotonic_boottime();
	di->enable_hv_charging = 1;
	platform_set_drvdata(pdev, di);
	g_di = di;

	hwlog_info("charger manager probe ok\n");
	return ret;

fail_create_event_queue:
	power_wakeup_source_unregister(di->charge_lock);
	charger_event_queue_destroy(&di->event_queue);
fail_register_psy:
	if (di->charge_psy)
		power_supply_unregister(di->charge_psy);
	if (di->usb_psy)
		power_supply_unregister(di->charge_psy);

	platform_set_drvdata(pdev, NULL);
	kfree(di);
	g_di = NULL;
	return ret;
}

static int charge_manager_remove(struct platform_device *pdev)
{
	struct charge_manager_info *di = platform_get_drvdata(pdev);

	if (!di)
		return -ENODEV;

	cancel_delayed_work(&di->charge_work);
	if (di->charge_psy)
		power_supply_unregister(di->charge_psy);
	if (di->usb_psy)
		power_supply_unregister(di->charge_psy);

	kfree(di);
	g_di = NULL;
	return 0;
}

static void charge_manager_shutdown(struct platform_device *pdev)
{
	struct charge_manager_info *di = platform_get_drvdata(pdev);

	if (!di)
		return;

	hwlog_info("%s ++\n", __func__);
	if (!di) {
		hwlog_err("[%s]di is NULL\n", __func__);
		return;
	}

	cancel_delayed_work(&di->charge_work);

	hwlog_info("%s --\n", __func__);

	return;
}

#ifdef CONFIG_PM
static int charge_manager_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct charge_manager_info *di = platform_get_drvdata(pdev);

	if (!di)
		return -ENODEV;

	hwlog_info("%s ++\n", __func__);

	cancel_delayed_work(&di->charge_work);

	hwlog_info("%s --\n", __func__);

	return 0;
}

static int charge_manager_resume(struct platform_device *pdev)
{
	/* todo suspend when chargedone and charging when resume */
	hwlog_info("%s\n", __func__);

	return 0;
}
#endif /* CONFIG_PM */

static const struct of_device_id charge_manager_match_table[] = {
	{
		.compatible = "huawei,charge_manager",
		.data = NULL,
	},
	{},
};

static struct platform_driver charge_manager_driver = {
	.probe = charge_manager_probe,
	.remove = charge_manager_remove,
#ifdef CONFIG_PM
	.suspend = charge_manager_suspend,
	.resume = charge_manager_resume,
#endif
	.shutdown = charge_manager_shutdown,
	.driver = {
		.name = "huawei,charge_manager",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(charge_manager_match_table),
	},
};

static int __init charge_manager_init(void)
{
	return platform_driver_register(&charge_manager_driver);
}

static void __exit charge_manager_exit(void)
{
	platform_driver_unregister(&charge_manager_driver);
}

device_initcall_sync(charge_manager_init);
module_exit(charge_manager_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("charge manager driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
