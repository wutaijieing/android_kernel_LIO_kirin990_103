/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Hisilicon USB Type-C framework.
 * Author: hisilicon
 *
 * This software is distributed under the terms of the GNU General
 * Public License ("GPL") as published by the Free Software Foundation,
 * either version 2 of that License or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/fb.h>

#ifdef CONFIG_ADAPTER_PROTOCOL_PD
#include <chipset_common/hwpower/protocol/adapter_protocol_pd.h>
#endif

#ifdef CONFIG_ADAPTER_PROTOCOL_UVDM
#include <chipset_common/hwpower/protocol/adapter_protocol_uvdm.h>
#endif

#ifdef CONFIG_HUAWEI_CHARGER_AP
#include <huawei_platform/power/huawei_charger.h>
#endif

#ifdef CONFIG_HUAWEI_DSM
#include <chipset_common/hwpower/common_module/power_dsm.h>
#endif

#ifdef CONFIG_TCPC_CLASS
#include <huawei_platform/usb/hw_pd_dev.h>
#endif

#ifdef CONFIG_HUAWEI_USB
#include <chipset_common/hwusb/hw_usb.h>
#endif

#ifdef CONFIG_DP_AUX_SWITCH
#include <huawei_platform/dp_aux_switch/dp_aux_switch.h>
#endif

#include <linux/platform_drivers/usb/hifi_usb.h>
#include <linux/platform_drivers/usb/hisi_tcpm.h>
#include <linux/platform_drivers/usb/hisi_typec.h>
#include "hisi-usb-typec.h"
#include "usb-typec.h"

#include <securec.h>

#ifdef CONFIG_HISI_USB_TYPEC_DBG
#define D(format, arg...) \
		pr_info("[oem_typec] [%s]" format, __func__, ##arg)
#else
#define D(format, arg...) do {} while (0)
#endif
#define I(format, arg...) \
		pr_info("[oem_typec] [%s]" format, __func__, ##arg)
#define E(format, arg...) \
		pr_err("[ERR][oem_typec] [%s]" format, __func__, ##arg)

struct oem_typec {
	struct notifier_block wakelock_control_nb;
};

static struct oem_typec _oem_typec;

#define ANALOG_SINGLE_CC_STS_MASK 0xF
#define ANALOG_CC2_STS_OFFSET 4
#define ANALOG_CC_WITH_UNDEFINED_RESISTANCE 1

static int oem_typec_otg_pwr_src(void)
{
	return pd_dpm_get_otg_channel();
}

#ifdef CONFIG_ADAPTER_PROTOCOL
static bool adapter_type_qtr_typec(int adapter_type)
{
	if (adapter_type == ADAPTER_TYPE_QTR_C_20V3A ||
			adapter_type == ADAPTER_TYPE_QTR_C_10V4A)
		return true;

	return false;
}
#endif

static bool oem_typec_force_vconn(void)
{
#ifdef CONFIG_ADAPTER_PROTOCOL
	int adapter_type = ADAPTER_TYPE_UNKNOWN;
	int ret = adapter_get_adp_type(ADAPTER_PROTOCOL_SCP, &adapter_type);
	if (ret) {
		D("get adapter type failed\n");
		return false;
	}

	if (adapter_type_qtr_typec(adapter_type)) {
		D("Force Vconn\n");
		return true;
	}
#endif
	return false;
}

/*
 * Turn on/off vconn power.
 * enable:0 - off, 1 - on
 */
static void oem_typec_set_vconn(int enable)
{
	I(" Vconn enable: %d\n", enable);
	pd_dpm_handle_pe_event(PD_DPM_PE_EVT_SOURCE_VCONN, &enable);
}

static int oem_pd_dp_state_notify(uint32_t irq_type, uint32_t mode_type,
		uint32_t dev_type, uint32_t typec_orien)
{
	int ret = 0;
	struct pd_dpm_combphy_event event;

	event.irq_type = irq_type;
	event.mode_type = mode_type;
	event.dev_type = dev_type;
	event.typec_orien = typec_orien;
	ret = pd_dpm_handle_combphy_event(event);

	return ret;
}

static void oem_set_combphy_status(enum tcpc_mux_ctrl_type mode)
{
	pd_dpm_set_combphy_status(mode);
}

static void oem_set_last_hpd_status(bool status)
{
	pd_dpm_set_last_hpd_status(status);
}

static void oem_usb_send_event(enum usb_cable_event event)
{
#ifdef CONFIG_HUAWEI_USB
	if (event == USB_CABLE_DP_IN)
		hw_usb_send_event(DP_CABLE_IN_EVENT);
	else if (event == USB_CABLE_DP_OUT)
		hw_usb_send_event(DP_CABLE_OUT_EVENT);
	else
#endif
		D("not dp\n");
}

/* Monitor cc status which pass through CCDebounce */
static void oem_typec_cc_status_change(uint8_t cc1, uint8_t cc2)
{
#ifdef CONFIG_TCPC_CLASS
	pd_dpm_handle_pe_event(PD_DPM_PE_ABNORMAL_CC_CHANGE_HANDLER, NULL);
#endif
}

/*
 * transfer pid/vid to hw_pd
 * payload[0]: reserved
 * payload[1]: id header vdo
 * payload[2]: reserved
 * payload[3]: product vdo
 */
static void oem_typec_dfp_inform_id(uint32_t *payload, uint32_t size)
{
	uint32_t bcd = 0;
	uint32_t vid = 0;
	uint32_t pid = 0;

	if (payload && size >= 4) { /* 4: bcd or vid or pid */
		bcd = *(payload + 3) & PD_DPM_HW_PDO_MASK; /* 3: bcd */
		vid = *(payload + 1) & PD_DPM_HW_PDO_MASK; /* 1: vid */
		pid = (*(payload + 3) >> PD_DPM_PDT_VID_OFFSET) & /* 3: pid */
			PD_DPM_HW_PDO_MASK;
	}

	pd_set_product_id_info(vid, pid, bcd);
}

static bool oem_typec_charger_type_pd(void)
{
	return pd_dpm_get_pd_finish_flag();
}

static int oem_typec_wake_lock_call(struct notifier_block *dpm_nb,
		unsigned long event, void *data)
{
	switch (event) {
	case PD_WAKE_LOCK:
		hisi_usb_typec_update_wake_lock(true);
		break;

	case PD_WAKE_UNLOCK:
		hisi_usb_typec_update_wake_lock(false);
		break;

	default:
		E("unknown event (%lu)\n", event);
		break;
	}

	return NOTIFY_OK;
}

#ifdef CONFIG_ADAPTER_PROTOCOL_PPS
static bool oem_pd_is_support_pps(void)
{
	return hisi_usb_typec_is_support_pps();
}

static int oem_pd_set_pps_start(int mv, int ma)
{
	return hisi_usb_typec_set_pps_start(mv, ma);
}

static int oem_pd_set_pps_end(void *data)
{
	return hisi_usb_typec_set_pps_end();
}

static int oem_pd_get_max_current(int *cur, void *data)
{
	if (!cur)
		return -EINVAL;

	*cur = hisi_usb_typec_get_max_current();
	return 0;
}

static int oem_pd_get_temp(int *temp, void *data)
{
	if (!temp)
		return -EINVAL;

	*temp = hisi_usb_typec_get_temp();
	return 0;
}

static int oem_pd_get_pps_adapter_output(int *mv, int *ma, void * data)
{
	if (!mv || !ma)
		return -EINVAL;

	return hisi_usb_typec_get_pps_adapter_output(mv, ma);
}

static int oem_pd_get_product_id(int *pid, void *data)
{
	if (!pid)
		return -EINVAL;

	*pid = hisi_typec_usb_get_product_id();
	return 0;
}

static int oem_pd_get_vendor_id(int *vid, void *data)
{
	if (!vid)
		return -EINVAL;

	*vid = hisi_typec_usb_get_vendor_id();
	return 0;
}

static int oem_pd_get_version(int *fw_ver, int *hw_ver, void *data);
{
	if (!fw_ver || !hw_ver)
		return -EINVAL;

	return hisi_typec_usb_get_version(fw_ver, hw_ver);
}

static int oem_pd_get_source_apdo_cap(struct adapter_cur_para *para, void *data)
{
	int i;
	struct hisi_usb_apdo_info pd_apdo = {0};
	int ret = hisi_usb_typec_get_source_apdo_cap(&pd_apdo);
	if (ret || !para) {
		E("null params\n");
		return -1;
	}

	para->num = pd_apdo.num;

	for (i = 0; i < pd_apdo.num; i++) {
		para->min_volt[i] = pd_apdo.min_volt[i];
		para->max_volt[i] = pd_apdo.max_volt[i];
		para->cur[i] = pd_apdo.cur[i];
	}

	return 0;
}
#endif

static void oem_typec_cc_ovp_dmd_report(const void *dmd_msg)
{
#ifdef CONFIG_HUAWEI_DSM
	power_dsm_report_dmd(POWER_DSM_BATTERY,
			POWER_DSM_ERROR_NO_TYPEC_CC_OVP, dmd_msg);
#endif
}

static struct pd_dpm_ops hisi_device_pd_dpm_ops = {
	.pd_dpm_get_hw_dock_svid_exist = hisi_usb_pd_get_hw_dock_svid_exist,
	.pd_dpm_notify_direct_charge_status =
			hisi_usb_typec_mark_direct_charging,
	.pd_dpm_get_cc_state = hisi_usb_get_cc_state,
	.pd_dpm_set_cc_mode = hisi_usb_set_cc_mode,
	.pd_dpm_enable_drp = hisi_usb_force_enable_drp,
	.pd_dpm_disable_pd = hisi_usb_disable_pd,
	.pd_dpm_check_cc_vbus_short = hisi_usb_check_cc_vbus_short,
	.pd_dpm_detect_emark_cable = hisi_usb_detect_emark_cable,
	.data_role_swap = hisi_usb_pd_dpm_data_role_swap,
};

#ifdef CONFIG_ADAPTER_PROTOCOL_PD
static struct hwpd_ops hisi_device_pd_protocol_ops = {
	.chip_name = "scharger_v600",
	.hard_reset_master = hisi_usb_typec_issue_hardreset,
	.set_output_voltage = hisi_usb_typec_set_pd_adapter_voltage,
};
#endif

#ifdef CONFIG_ADAPTER_PROTOCOL_UVDM
static struct hwuvdm_ops hisi_device_uvdm_protocol_ops = {
	.chip_name = "scharger_v600",
	.send_data = hisi_usb_typec_send_uvdm,
};
#endif

#ifdef CONFIG_ADAPTER_PROTOCOL_PPS
static struct pps_protocol_ops hisi_device_pps_protocol_ops = {
	.chip_name = "scharger_v600",
	.set_cap = oem_pd_set_pps_start,
	.set_pps_end_cap = oem_pd_set_pps_end,
	.get_temp = oem_pd_get_temp,
	.get_output = oem_pd_get_pps_adapter_output,
	.get_source_apdo_cap = oem_pd_get_source_apdo_cap,
	.get_max_current = oem_pd_get_max_current,
	.is_support_pps = oem_pd_is_support_pps,
	.get_vendor_id = oem_pd_get_vendor_id,
	.get_product_id = oem_pd_get_product_id,
	.get_version = oem_pd_get_version,
};
#endif

/*
 * Check the cable for direct charge or not.
 */
static struct cc_check_ops direct_charge_cable_check_ops = {
	.is_cable_for_direct_charge = hisi_usb_typec_direct_charge_cable_detect,
};

static int oem_typec_handle_pe_event(uint32_t evt_type, void *state)
{
	int ret = 0;
	struct pd_dpm_typec_state dpm_tc_state = {0};
	struct tcp_ny_typec_state *tcp_tc_state = NULL;
	struct pd_dpm_vbus_state dpm_vbus_state = {0};
	struct tcp_ny_vbus_state *tcp_vbus_state = NULL;
	struct pd_dpm_swap_state dpm_swap_state = {0};
	struct tcp_ny_swap_state *tcp_swap_state = NULL;
	struct pd_dpm_pd_state dpm_pd_state = {0};
	struct tcp_ny_pd_state *tcp_pd_state = NULL;
	struct pd_dpm_cable_info dpm_cable_vdo = {0};
	struct tcp_ny_cable_vdo *tcp_cable_vdo = NULL;

	switch (evt_type) {
	case TCP_NOTIFY_DIS_VBUS_CTRL:
		tcp_vbus_state = (struct tcp_ny_vbus_state *)state;
		dpm_vbus_state.mv = tcp_vbus_state->mv;
		dpm_vbus_state.ma = tcp_vbus_state->ma;
		dpm_vbus_state.vbus_type = tcp_vbus_state->type;
		ret = pd_dpm_handle_pe_event(PD_DPM_PE_EVT_DIS_VBUS_CTRL, (void *)&dpm_vbus_state);
		break;

	case TCP_NOTIFY_SOURCE_VBUS:
		tcp_vbus_state = (struct tcp_ny_vbus_state *)state;
		dpm_vbus_state.mv = tcp_vbus_state->mv;
		dpm_vbus_state.ma = tcp_vbus_state->ma;
		dpm_vbus_state.vbus_type = tcp_vbus_state->type;
		ret = pd_dpm_handle_pe_event(PD_DPM_PE_EVT_SOURCE_VBUS, (void *)&dpm_vbus_state);
		break;

	case TCP_NOTIFY_SINK_VBUS:
		tcp_vbus_state = (struct tcp_ny_vbus_state *)state;
		dpm_vbus_state.mv = tcp_vbus_state->mv;
		dpm_vbus_state.ma = tcp_vbus_state->ma;
		dpm_vbus_state.vbus_type = tcp_vbus_state->type;
		dpm_vbus_state.ext_power = tcp_vbus_state->ext_power;
		dpm_vbus_state.remote_rp_level = tcp_vbus_state->remote_rp_level;
		ret = pd_dpm_handle_pe_event(PD_DPM_PE_EVT_SINK_VBUS, (void *)&dpm_vbus_state);
		break;

	case TCP_NOTIFY_PR_SWAP:
		tcp_swap_state = (struct tcp_ny_swap_state *)state;
		dpm_swap_state.new_role = tcp_swap_state->new_role;
		ret = pd_dpm_handle_pe_event(PD_DPM_PE_EVT_PR_SWAP, (void *)&dpm_swap_state);
		break;

	case TCP_NOTIFY_DR_SWAP:
		tcp_swap_state = (struct tcp_ny_swap_state *)state;
		dpm_swap_state.new_role = tcp_swap_state->new_role;
		ret = pd_dpm_handle_pe_event(PD_DPM_PE_EVT_DR_SWAP, (void *)&dpm_swap_state);
		break;

	case TCP_NOTIFY_TYPEC_STATE:
		tcp_tc_state = (struct tcp_ny_typec_state *)state;
		dpm_tc_state.polarity = tcp_tc_state->polarity;
		dpm_tc_state.old_state = tcp_tc_state->old_state;
		dpm_tc_state.new_state = tcp_tc_state->new_state;
		ret = pd_dpm_handle_pe_event(PD_DPM_PE_EVT_TYPEC_STATE, (void *)&dpm_tc_state);
		break;

	case TCP_NOTIFY_PD_STATE:
		tcp_pd_state = (struct tcp_ny_pd_state *)state;
		dpm_pd_state.connected = tcp_pd_state->connected;
		ret = pd_dpm_handle_pe_event(PD_DPM_PE_EVT_PD_STATE, (void *)&dpm_pd_state);
		break;

	case TCP_NOTIFY_CABLE_VDO:
		tcp_cable_vdo = (struct tcp_ny_cable_vdo *)state;
		dpm_cable_vdo.cable_vdo = tcp_cable_vdo->vdo;
		dpm_cable_vdo.cable_vdo_ext = tcp_cable_vdo->vdo_ext;
		ret = pd_dpm_handle_pe_event(PD_DPM_PE_CABLE_VDO, (void *)&dpm_cable_vdo);
		break;

	default:
		break;
	}

	return ret;
}

/*
 * Record optional_max_power by max_power value om pdo.
 * max_power:max power value from pdo.
 */
static void oem_typec_max_power(int max_power)
{
	pd_dpm_set_max_power(max_power);
}

/*
 * Reset hw_pd product type when src startup.
 */
static void oem_typec_reset_product(void)
{
	pd_set_product_type(PD_DPM_INVALID_VAL);
}

static int oem_typec_register_pd_dpm(void)
{
	int ret;
	void *data = (void *)&_oem_typec;

	D("+\n");
	ret = pd_dpm_ops_register(&hisi_device_pd_dpm_ops, data);
	if (ret) {
		I("Need not hisi pd\n");
		return -EBUSY;
	}

#ifdef CONFIG_ADAPTER_PROTOCOL_PD
	hisi_device_pd_protocol_ops.dev_data = data;
	ret = hwpd_ops_register(&hisi_device_pd_protocol_ops);
	if (ret) {
			I("pd protocol register failed\n");
			return -EBUSY;
	}
#endif

#ifdef CONFIG_ADAPTER_PROTOCOL_UVDM
	hisi_device_uvdm_protocol_ops.dev_data = data;
	ret = hwuvdm_ops_register(&hisi_device_uvdm_protocol_ops);
	if (ret)
		E("uvdm protocol register failed\n");
#endif

#ifdef HISI_USB_PD_PPS
	hisi_device_pps_protocol_ops.dev_data = data;
	ret = pps_protocol_ops_register(&hisi_device_pps_protocol_ops);
	if (ret)
		E("pps protocol register failed\n");
#endif

	ret = cc_check_ops_register(&direct_charge_cable_check_ops);
	if (ret) {
		E("cc_check_ops register failed!\n");
		return ret;
	}

	_oem_typec.wakelock_control_nb.notifier_call = oem_typec_wake_lock_call;
	ret = register_pd_wake_unlock_notifier(&_oem_typec.wakelock_control_nb);
	if (ret) {
		E("register_pd_wake_unlock_notifier ret %d\n", ret);
		return ret;
	}

	D("-\n");
	return 0;
}

static struct hisi_usb_typec_adapter_ops oem_typec_ops = {
	.set_vconn = oem_typec_set_vconn,
	.otg_pwr_src = oem_typec_otg_pwr_src,
	.force_vconn = oem_typec_force_vconn,
	.monitor_cc_change = oem_typec_cc_status_change,
	.dfp_inform_id = oem_typec_dfp_inform_id,
	.charger_type_pd = oem_typec_charger_type_pd,
	.cc_ovp_dmd_report = oem_typec_cc_ovp_dmd_report,
	.handle_pe_event = oem_typec_handle_pe_event,
	.max_power = oem_typec_max_power,
	.reset_product = oem_typec_reset_product,
	.pd_dp_state_notify = oem_pd_dp_state_notify,
	.set_combphy_status = oem_set_combphy_status,
	.set_last_hpd_status = oem_set_last_hpd_status,
	.usb_send_event = oem_usb_send_event,
	.register_pd_dpm = oem_typec_register_pd_dpm,
};

static int _oem_typec_init(void)
{
	int ret;
	ret = hisi_usb_typec_register_ops(&oem_typec_ops);
	if (ret) {
		I("May not hipd\n");
		return ret;
	}
	return 0;
}

static int __init oem_typec_init(void)
{
	D("oem typec init\n");
	return _oem_typec_init();
}

static void __exit oem_typec_exit(void)
{
	D("oem typec exit\n");
}

subsys_initcall_sync(oem_typec_init);
module_exit(oem_typec_exit);

MODULE_AUTHOR("Xiaowei Song <songxiaowei@hisilicon.com>");
MODULE_DESCRIPTION("Hisilicon USB Type-C Driver");
MODULE_LICENSE("GPL");
