/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Hisilicon USB Type-C framework.
 * Author: hisilicon
 *
 * This software is distributed under the terms of the GNU General
 * Public License ("GPL") as published by the Free Software Foundation,
 * either version 2 of that License or (at your option) any later version.
 */

#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/spinlock.h>
#include <linux/notifier.h>
#include <linux/fb.h>
#include <linux/debugfs.h>

#ifdef CONFIG_DP_AUX_SWITCH
#include <huawei_platform/dp_aux_switch/dp_aux_switch.h>
#endif

#include <linux/platform_drivers/usb/hifi_usb.h>
#include <linux/platform_drivers/usb/chip_usb.h>
#include <linux/platform_drivers/usb/tca.h>
#include <linux/platform_drivers/usb/hisi_tcpm.h>
#include <linux/platform_drivers/usb/hisi_typec.h>
#include "usb-typec.h"

#include <securec.h>


#ifdef CONFIG_HISI_USB_TYPEC_DBG
#define D(format, arg...) \
		pr_info("[vendor_typec]" format, ##arg)
#else
#define D(format, arg...) do {} while (0)
#endif
#define I(format, arg...) \
		pr_info("[vendor_typec]" format, ##arg)
#define E(format, arg...) \
		pr_err("[ERR][vendor_typec]" format, ##arg)

struct vendor_typec {
	uint32_t usb_pending_state;
	uint32_t usb_pre_state;
	struct workqueue_struct *usb_wq;
	struct delayed_work usb_state_update_work;
	struct blocking_notifier_head pd_evt_nh;
	uint32_t usb_event;

	int source_vbus;
	int orient;

	struct notifier_block pwr_nb;
};

static struct vendor_typec *_vendor_typec;

/* UDP Board, otg_src: hi6526 always */
static int vendor_typec_otg_pwr_src(void)
{
	return 0;
}

static int vendor_typec_otg_en(int enable)
{
	if (enable)
		I("Notify Charger to En Otg Pwr\n");
	else
		I("Notify Charger to Dis Otg Pwr\n");

	return 0;
}

/*
 * Turn on/off vconn power.
 * enable:0 - off, 1 - on
 */
static void vendor_typec_set_vconn(int enable)
{
	if (enable)
		I("Notify Boost to En Vconn\n");
	else
		I("Notify Boost to Dis Vconn\n");
}

static int vendor_pd_dp_state_notify(uint32_t irq_type, uint32_t mode_type,
		uint32_t dev_type, uint32_t typec_orien)
{
	return pd_event_notify(irq_type, mode_type, dev_type, typec_orien);
}

static bool vendor_typec_charger_type_pd(void)
{
	return false;
}

static int vendor_typec_pwr_notifier_call(struct notifier_block *nb,
			unsigned long action, void *data)
{
	struct vendor_typec *typec = container_of(nb, struct vendor_typec, pwr_nb);
	struct tcp_ny_vbus_state *vbus_state = (struct tcp_ny_vbus_state *)data;
	int ret = NOTIFY_OK;

	switch(action) {
	case TCP_NOTIFY_SOURCE_VBUS:
		I("source vbus\n");
		if (vbus_state->mv) {
			typec->source_vbus = 1;
			ret = vendor_typec_otg_en(1);
		} else {
			ret = vendor_typec_otg_en(0);
			typec->source_vbus = 0;
		}
		break;

	case TCP_NOTIFY_DIS_VBUS_CTRL:
		I("dis vbus\n");
		if (typec->source_vbus)
			ret = vendor_typec_otg_en(0);
		else
			D("Stop charging\n");
		break;

	case TCP_NOTIFY_SINK_VBUS:
		typec->source_vbus = 0;
		I("sink vbus\n");
		break;

	default:
		break;
	}

	return 0;
}

static void vendor_typec_report_detach(struct vendor_typec *typec)
{
	if (typec->usb_pre_state == TCPC_TYPEC_DEVICE_ATTACHED)
		pd_event_notify(TCA_IRQ_HPD_OUT, TCPC_NC, TCA_CHARGER_DISCONNECT_EVENT, typec->orient);
	else
		pd_event_notify(TCA_IRQ_HPD_OUT, TCPC_NC, TCA_ID_RISE_EVENT, typec->orient);
}

static void vendor_typec_report_attach(struct vendor_typec *typec)
{
	if (typec->usb_pending_state == TCPC_TYPEC_HOST_ATTACHED)
		pd_event_notify(TCA_IRQ_HPD_IN, TCPC_USB31_CONNECTED, TCA_ID_FALL_EVENT, typec->orient);
	else
		pd_event_notify(TCA_IRQ_HPD_IN, TCPC_USB31_CONNECTED, TCA_CHARGER_CONNECT_EVENT, typec->orient);
}

static void vendor_typec_state_update_state(struct work_struct *work)
{
	struct vendor_typec *typec = _vendor_typec;

	I("usb pre_state[%u], pending_state[%u]\n",
		typec->usb_pre_state, typec->usb_pending_state);

	if (typec->usb_pending_state == typec->usb_pre_state)
		return;

	switch (typec->usb_pending_state) {
	case TCPC_TYPEC_DETACHED:
		vendor_typec_report_detach(typec);
		break;

	case TCPC_TYPEC_DEVICE_ATTACHED:
	case TCPC_TYPEC_HOST_ATTACHED:
		if ((typec->usb_pre_state != TCPC_TYPEC_DETACHED) &&
				(typec->usb_pre_state != TCPC_TYPEC_NONE))
			vendor_typec_report_detach(typec);
		vendor_typec_report_attach(typec);
		break;

	default:
		return;
	}
}

int register_vendor_typec_vbus_notifier(struct notifier_block *nb)
{
	struct vendor_typec *typec = _vendor_typec;
	int ret = 0;

	if (!nb)
		return -EINVAL;

	ret = blocking_notifier_chain_register(&typec->pd_evt_nh, nb);

	return ret;
}
EXPORT_SYMBOL(register_vendor_typec_vbus_notifier);

int unregister_vendor_typec_vbus_notifier(struct notifier_block *nb)
{
	struct vendor_typec *typec = _vendor_typec;

	return blocking_notifier_chain_unregister(&typec->pd_evt_nh, nb);
}
EXPORT_SYMBOL(unregister_vendor_typec_vbus_notifier);

static int vendor_typec_vbus_notifier_call(struct vendor_typec *typec, unsigned long event, void *data)
{
	return blocking_notifier_call_chain(&typec->pd_evt_nh, event, data);
}

static int vendor_typec_handle_pe_event(uint32_t event, void *data)
{
	struct vendor_typec *typec = _vendor_typec;
	struct tcp_ny_typec_state *typec_state = NULL;
	struct tcp_ny_swap_state *swap_state = NULL;
	int usb_event = TCPC_TYPEC_NONE;

	if (!data) {
		E("NULL Data\n");
		return -1;
	}

	switch (event) {
	case TCP_NOTIFY_TYPEC_STATE:
		typec_state = (struct tcp_ny_typec_state *)data;
		typec->orient = typec_state->polarity;
		switch (typec_state->new_state) {
		case TYPEC_ATTACHED_SRC:
			usb_event = TCPC_TYPEC_HOST_ATTACHED;
			break;

		case TYPEC_ATTACHED_AUDIO:
			usb_event = TCPC_TYPEC_AUDIO_ATTACHED;
			break;

		case TYPEC_ATTACHED_SNK:
		case TYPEC_ATTACHED_DBGACC_SNK:
		case TYPEC_ATTACHED_CUSTOM_SRC:
		case TYPEC_ATTACHED_DEBUG:
		case TYPEC_ATTACHED_VBUS_ONLY:
			usb_event = TCPC_TYPEC_DEVICE_ATTACHED;
			break;

		case TYPEC_UNATTACHED:
		case TYPEC_DETTACHED_VBUS_ONLY:
			usb_event = TCPC_TYPEC_DETACHED;
			break;

		default:
			D("%s can not detect typec state\r\n", __func__);
			break;
		}
		break;

	case TCP_NOTIFY_DIS_VBUS_CTRL:
	case TCP_NOTIFY_SINK_VBUS:
	case TCP_NOTIFY_SOURCE_VBUS:
		vendor_typec_vbus_notifier_call(typec, event, data);
		break;

	case TCP_NOTIFY_DR_SWAP:
		swap_state = (struct tcp_ny_swap_state *)data;
		if (swap_state->new_role == PD_ROLE_DFP)
			usb_event = TCPC_TYPEC_HOST_ATTACHED;
		else
			usb_event = TCPC_TYPEC_DEVICE_ATTACHED;
		break;

	case TCP_NOTIFY_PR_SWAP:
		break;

	case TCP_NOTIFY_PD_STATE:
		break;

	default:
		D("%s reserved event[%u] \n", __func__, event);
		break;
	};

	if ((usb_event != TCPC_TYPEC_NONE)) {
		I("usb_event: %u\n", usb_event);
		typec->usb_pre_state = typec->usb_pending_state;
		if (typec->usb_pending_state != usb_event) {
			typec->usb_pending_state = usb_event;
			queue_delayed_work(typec->usb_wq,
				&typec->usb_state_update_work,
				msecs_to_jiffies(0));\
		}
	}

	return 0;
}

static void vendor_typec_max_power(int max_power)
{
	D("Notify max power to Charger\n");
}

static int vendor_typec_register_pd_dpm(void)
{
	struct vendor_typec *typec = _vendor_typec;
	int ret;

	if (!typec) {
		E("Vendor typec is not ready\n");
		return 0;
	}

	typec->pwr_nb.notifier_call = vendor_typec_pwr_notifier_call;
	ret = register_vendor_typec_vbus_notifier(&typec->pwr_nb);
	if (ret)
		E("register vbus state notifier failed ret %d\n", ret);

	return 0;
}

struct hisi_usb_typec_adapter_ops  vendor_typec_ops = {
	.set_vconn = vendor_typec_set_vconn,
	.otg_pwr_src = vendor_typec_otg_pwr_src,
	.charger_type_pd = vendor_typec_charger_type_pd,
	.handle_pe_event = vendor_typec_handle_pe_event,
	.pd_dp_state_notify = vendor_pd_dp_state_notify,
	.max_power = vendor_typec_max_power,
	.register_pd_dpm = vendor_typec_register_pd_dpm,
};

static int _vendor_typec_init(void)
{
	struct vendor_typec *typec = NULL;
	int ret;

	I("+\n");
	typec = kzalloc(sizeof(*typec), GFP_KERNEL);
	if (!typec)
		return -ENOMEM;

	typec->usb_wq = create_workqueue("vendor typec_state_update_wq");
	INIT_DELAYED_WORK(&typec->usb_state_update_work, vendor_typec_state_update_state);
	BLOCKING_INIT_NOTIFIER_HEAD(&typec->pd_evt_nh);

	typec->usb_pending_state = TCPC_TYPEC_NONE;
	typec->usb_pre_state = TCPC_TYPEC_NONE;
	_vendor_typec = typec;

	ret = hisi_usb_typec_register_ops(&vendor_typec_ops);
	if (ret) {
		E("vendor ops register failed\n");
		return ret;
	}

	I("-\n");
	return 0;
}

static int __init vendor_typec_init(void)
{
	return _vendor_typec_init();
}

static void __exit vendor_typec_exit(void)
{
	if (_vendor_typec) {
		kfree(_vendor_typec);
		_vendor_typec = NULL;
	}
}

subsys_initcall(vendor_typec_init);
module_exit(vendor_typec_exit);

MODULE_AUTHOR("Xiaowei Song <songxiaowei@hisilicon.com>");
MODULE_DESCRIPTION("Vendor TypeC Driver");
MODULE_LICENSE("GPL");
