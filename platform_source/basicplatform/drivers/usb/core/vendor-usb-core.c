/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: driver for usb-core
 * Create: 2019-03-04
 *
 * This software is distributed under the terms of the GNU General
 * Public License ("GPL") as published by the Free Software Foundation,
 * either version 2 of that License or (at your option) any later version.
 */
#include <linux/time64.h>
#include <linux/err.h>
#include <linux/usb.h>
#include "vendor-usb-core.h"
#include "hub.h"
#ifdef CONFIG_HUAWEI_USB
#include <chipset_common/hwusb/hw_usb.h>
#endif

#ifdef CONFIG_FMEA_FAULT_INJECTION
static unsigned int g_control_msg_type = 0;
static int g_urb_status = 0;

#define URB_TRANSFER_EPROT 0x1
#define URB_TRANSFER_ETIMEDOUT 0x2
#define URB_TRANSFER_EILSEQ 0x3
#define URB_TRANSFER_ENOSR 0x4

void usb_stub_transfer(unsigned int request)
{
	g_control_msg_type = request;

	pr_err("[%s] stub control msg type %u\n", __func__, g_control_msg_type);
}

void usb_stub_control_msg(int *ret, __u8 request)
{
	if (ret == NULL) {
		pr_err("[%s]ret is NULL\n", __func__);
		return;
	}

	if (g_control_msg_type == 0)
		return;

	switch (request) {
	case USB_REQ_SET_ADDRESS:
		if (g_control_msg_type == USB_REQ_SET_ADDRESS) {
			pr_err("[%s] stub usb set address\n", __func__);
			*ret = -ETIMEDOUT;
		}
		break;
	case USB_REQ_GET_DESCRIPTOR:
		if (g_control_msg_type == USB_REQ_GET_DESCRIPTOR) {
			pr_err("[%s] stub usb get descriptor\n", __func__);
			*ret =  -ETIMEDOUT;
		}
		break;
	default:
		pr_err("[%s] stub usb request %u\n", __func__, request);
		break;
	}
}

void stub_usb_urb_complete(int status)
{
	switch (status) {
	case URB_TRANSFER_EPROT:
		g_urb_status = -EPROTO;
		break;
	case URB_TRANSFER_ETIMEDOUT:
		g_urb_status = -ETIMEDOUT;
		break;
	case URB_TRANSFER_EILSEQ:
		g_urb_status = -EILSEQ;
		break;
	case URB_TRANSFER_ENOSR:
		g_urb_status = -ENOSR;
		break;
	default:
		g_urb_status = 0;
		break;
	}

	pr_err("[%s] stub urb complete transfer %d\n", __func__, g_urb_status);
}

void test_usb_urb_complete_fault(struct urb *urb, int *status)
{
	if (g_urb_status == 0)
		return;
	*status = g_urb_status;
	urb->status = g_urb_status;
}
#endif

#ifdef CONFIG_USB_DWC3_CHIP
/* register usb fault notify list */
static BLOCKING_NOTIFIER_HEAD(usb_fault_notifier_list);

void usb_fault_register_notify(struct notifier_block *nb)
{
	blocking_notifier_chain_register(&usb_fault_notifier_list, nb);
}

void usb_fault_unregister_notify(struct notifier_block *nb)
{
	blocking_notifier_chain_unregister(&usb_fault_notifier_list, nb);
}

void usb_notify_set_address_err(struct usb_bus *ubus)
{
	blocking_notifier_call_chain(&usb_fault_notifier_list, USB_SET_ADDRESS_ERR, ubus);
}

void usb_notify_get_descriptor_err(struct usb_bus *ubus)
{
	blocking_notifier_call_chain(&usb_fault_notifier_list, USB_GET_DESCRIPTOR_ERR, ubus);
}

void usb_notify_enum_dev_unknown_err(struct usb_bus *ubus)
{
	blocking_notifier_call_chain(&usb_fault_notifier_list, USB_ENUM_UNKNOWN_ERR, ubus);
}

void usb_notify_transfer_err(struct usb_bus *ubus)
{
	blocking_notifier_call_chain(&usb_fault_notifier_list, USB_TRANSFER_ERR, ubus);
}

#define FAULT_THRESHOLD   5
#define FAULT_INTERVAL    120
#define NOTIFY_INTERVAL   30

struct usb_trans_fault_record {
	unsigned long start_time;
	unsigned long last_time;
	unsigned long notify_time;
	int fault_count;
	unsigned int enum_err_type;
};
static struct usb_trans_fault_record g_record = {0};

void ueb_set_enum_err_type(unsigned int enum_err_type)
{
	g_record.enum_err_type = enum_err_type;
}

void usb_check_enum_dev_state(struct usb_bus *ubus)
{
	switch (g_record.enum_err_type) {
	case  USB_SET_ADDRESS_ERR:
		pr_err("[%s] usb host enum dev set address fialed\n", __func__);
		usb_notify_set_address_err(ubus);
		break;
	case USB_GET_DESCRIPTOR_ERR:
		pr_err("[%s] usb host enum dev get descriptor fialed\n", __func__);
		usb_notify_get_descriptor_err(ubus);
		break;
	default:
		pr_err("[%s] usb host enum dev unknown error fialed\n", __func__);
		usb_notify_enum_dev_unknown_err(ubus);
		break;
	}

	g_record.enum_err_type = 0;
}

static unsigned long get_timestame(void)
{
	struct timespec64 ts;

	ktime_get_real_ts64(&ts);
	return ts.tv_sec;
}

void vendor_usb_urb_complete(struct usb_bus *bus, struct urb *urb,
	int status)
{
#ifdef CONFIG_FMEA_FAULT_INJECTION
	test_usb_urb_complete_fault(urb, &status);
#endif
	if (status != -EPROTO && status != -ETIMEDOUT
		&& status != -EILSEQ && status != -ENOSR)
		return;

	if (g_record.start_time == 0)
		g_record.start_time = get_timestame();

	g_record.last_time = get_timestame();

	/* if time interval over 120s, restart record */
	if (g_record.last_time - g_record.start_time > FAULT_INTERVAL) {
		g_record.start_time = get_timestame();
		g_record.last_time = get_timestame();
		g_record.fault_count = 1;
	}

	/* transfer err, count++ */
	g_record.fault_count ++;

	/* if transfer failed over 5 times in 120s, notify event */
	if (g_record.fault_count >= FAULT_THRESHOLD &&
			get_timestame() - g_record.notify_time >= NOTIFY_INTERVAL) {
		pr_err("[%s] urb transfer failed, status %d\n", __func__, status);
		usb_notify_transfer_err(bus);
		g_record.notify_time  = get_timestame();
		g_record.start_time = 0;
		g_record.last_time = 0;
		g_record.fault_count = 0;
	}
}
#endif

void notify_hub_too_deep(void)
{
#ifdef CONFIG_HUAWEI_USB
	hw_usb_host_abnormal_event_notify(USB_HOST_EVENT_HUB_TOO_DEEP);
#endif
}

void notify_power_insufficient(void)
{
#ifdef CONFIG_HUAWEI_USB
	hw_usb_host_abnormal_event_notify(USB_HOST_EVENT_POWER_INSUFFICIENT);
#endif
}

int usb_device_read_mutex_trylock(void)
{
	unsigned long jiffies_expire = jiffies + HZ;

	while (!mutex_trylock(&usb_bus_idr_lock)) {
		/*
		 * If we can't acquire the lock after waiting one second,
		 * we're probably deadlocked
		 */
		if (time_after(jiffies, jiffies_expire)) {
			pr_err("%s:get usb_bus_idr_lock timeout, probably deadlocked\n",
				__func__);
			return -EFAULT;
		}
		msleep(20); /* according to system design */
	}

	return 0;
}

int usb_device_read_usb_trylock_device(struct usb_device *udev)
{
	/* wait 6 seconds for usb device enumerate */
	unsigned long jiffies_expire = jiffies + 6 * HZ;

	if (!udev)
		return -EFAULT;

	while (!usb_trylock_device(udev)) {
		/*
		 * If we can't acquire the lock after waiting one second,
		 * we're probably deadlocked
		 */
		if (time_after(jiffies, jiffies_expire))
			return -EFAULT;

		msleep(20); /* according to system design */
	}

	return 0;
}

#ifdef CONFIG_USB_DOCK_HEADSET_QUIRK

#include <huawei_platform/usb/hw_pd_dev.h>

#define VENDOR_DOCK_VID 0x0bda
#define VENDOR_DOCK_PID 0x5411

bool check_vendor_dock_quirk(struct usb_device *hdev,
	struct usb_hub *hub, int port1)
{
	struct usb_port *port2_dev = NULL;

	if (!hdev || !hub)
		return false;

	/* port1 is 3 just fix dock quirk condition */
	if (unlikely(port1 == 3 &&
		hdev->descriptor.idVendor == VENDOR_DOCK_VID &&
				hdev->descriptor.idProduct == VENDOR_DOCK_PID &&
				pd_dpm_get_hw_dock_svid_exist())) {
		/* dock quirk check dev(hub port1) */
		port2_dev = hub->ports[1];
		if (port2_dev->child)
			return false;
	}

	return true;
}

#endif /* CONFIG_USB_DOCK_HEADSET_QUIRK */
