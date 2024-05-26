/*
 * USB CDC serial (ACM) function driver
 *
 * Copyright (c) 2015-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * either version 2 of that License or (at your option) any later version.
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/stringify.h>
#include <securec.h>

#include "modem_acm.h"

#define MODEM_ACM_INTERFACE_SUBCLASS 0x13
#define NOTIFY_REQ_BUF_SIZE (sizeof(struct usb_cdc_notification) + 2)

/*
 * This CDC ACM function support just wraps control functions and
 * notifications around the generic serial-over-usb code.
 *
 * Because CDC ACM is standardized by the USB-IF, many host operating
 * systems have drivers for it.  Accordingly, ACM is the preferred
 * interop solution for serial-port type connections.  The control
 * models are often not necessary, and in any case don't do much in
 * this bare-bones implementation.
 *
 * Note that even MS-Windows has some support for ACM.  However, that
 * support is somewhat broken because when you use ACM in a composite
 * device, having multiple interfaces confuses the poor OS.  It doesn't
 * seem to understand CDC Union descriptors.  The new "association"
 * descriptors (roughly equivalent to CDC Unions) may sometimes help.
 */

struct f_acm {
	struct gserial          port;
	u8              ctrl_id;
	u8              data_id;
	u8              port_num;

	u8              pending;

	/*
	 * lock is mostly for pending and notify_req ... they get accessed
	 * by callbacks both from tty (open/close/break) under its spinlock,
	 * and notify_req.complete() which can't use that lock.
	 */
	spinlock_t          lock;

	int             support_notify;

	struct usb_ep           *notify;
	struct usb_request      *notify_req;

	int (*pending_notify)(struct f_acm *acm);
	/* cdc vendor flow control notify */
	u32             rx_is_on;
	u32             tx_is_on;

	struct usb_cdc_line_coding  port_line_coding;   /* 8-N-1 etc */

	/* SetControlLineState request -- CDC 1.1 section 6.2.14 (INPUT) */
	u16             port_handshake_bits;

	/* SerialState notification -- CDC 1.1 section 6.3.5 (OUTPUT) */
	u16             serial_state;
#define ACM_CTRL_BRK        (1 << 2)
#define ACM_CTRL_DSR        (1 << 1)
#define ACM_CTRL_DCD        (1 << 0)
};

static inline int is_support_notify(unsigned int port_num)
{
	return is_modem_port(port_num);
}

static inline struct f_acm *func_to_acm(struct usb_function *f)
{
	return container_of(f, struct f_acm, port.func);
}

static inline struct f_acm *port_to_acm(struct gserial *p)
{
	return container_of(p, struct f_acm, port);
}

/* notification endpoint uses smallish and infrequent fixed-size messages */

#define GS_NOTIFY_INTERVAL_MS    32
/* only notification + 2 bytes(10)isn't enough, flowctrol need 16 bytes  */
#define GS_NOTIFY_MAXPACKET   64
#define HS_MAXPACKET_SIZE 512
#define SS_MAXPACKET_SIZE 1024
#define REQUEST_TYPE_BYTE 8
/* follow CDC and USB Spec */
#define SET_COMM_FEATURE 0x02
#define MODEM_ACM_VENDOR_REQ 0xa3
#define MODEM_ACM_STD_REQ 0x81

/* interface and class descriptors: */

static struct usb_cdc_header_desc acm_header_desc = {
	.bLength =      sizeof(acm_header_desc),
	.bDescriptorType =  USB_DT_CS_INTERFACE,
	.bDescriptorSubType =   USB_CDC_HEADER_TYPE,
	.bcdCDC =       cpu_to_le16(0x0110), // 0x0110 CDC version 1.10
};

static struct usb_cdc_call_mgmt_descriptor
		acm_call_mgmt_descriptor = {
	.bLength =      sizeof(acm_call_mgmt_descriptor),
	.bDescriptorType =  USB_DT_CS_INTERFACE,
	.bDescriptorSubType =   USB_CDC_CALL_MANAGEMENT_TYPE,
	.bmCapabilities =   0,
};

static struct usb_cdc_acm_descriptor acm_descriptor = {
	.bLength =      sizeof(acm_descriptor),
	.bDescriptorType =  USB_DT_CS_INTERFACE,
	.bDescriptorSubType =   USB_CDC_ACM_TYPE,
	.bmCapabilities =   USB_CDC_CAP_LINE,
};

static struct usb_cdc_union_desc acm_union_desc = {
	.bLength =      sizeof(acm_union_desc),
	.bDescriptorType =  USB_DT_CS_INTERFACE,
	.bDescriptorSubType =   USB_CDC_UNION_TYPE,
};


static struct usb_interface_descriptor acm_single_interface_desc = {
	.bLength =      USB_DT_INTERFACE_SIZE,
	.bDescriptorType =  USB_DT_INTERFACE,
	.bInterfaceClass =  0xff,
	.bInterfaceSubClass =   0x02,
	.bInterfaceProtocol =   0x01,
};

/* full speed support: */
static struct usb_endpoint_descriptor acm_fs_notify_desc = {
	.bLength =      USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =  USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes =     USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =   cpu_to_le16(GS_NOTIFY_MAXPACKET),
	.bInterval =        GS_NOTIFY_INTERVAL_MS,
};

static struct usb_endpoint_descriptor acm_fs_in_desc = {
	.bLength =      USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =  USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes =     USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor acm_fs_out_desc = {
	.bLength =      USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =  USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes =     USB_ENDPOINT_XFER_BULK,
};

static struct usb_descriptor_header **acm_fs_cur_function;

static struct usb_descriptor_header *acm_fs_function_single[] = {
	(struct usb_descriptor_header *) &acm_single_interface_desc,
	(struct usb_descriptor_header *) &acm_header_desc,
	(struct usb_descriptor_header *) &acm_descriptor,
	(struct usb_descriptor_header *) &acm_call_mgmt_descriptor,
	(struct usb_descriptor_header *) &acm_union_desc,
	(struct usb_descriptor_header *) &acm_fs_in_desc,
	(struct usb_descriptor_header *) &acm_fs_out_desc,
	NULL,
};

static struct usb_descriptor_header *acm_fs_function_single_notify[] = {
	(struct usb_descriptor_header *) &acm_single_interface_desc,
	(struct usb_descriptor_header *) &acm_header_desc,
	(struct usb_descriptor_header *) &acm_descriptor,
	(struct usb_descriptor_header *) &acm_call_mgmt_descriptor,
	(struct usb_descriptor_header *) &acm_union_desc,
	(struct usb_descriptor_header *) &acm_fs_notify_desc,
	(struct usb_descriptor_header *) &acm_fs_in_desc,
	(struct usb_descriptor_header *) &acm_fs_out_desc,
	NULL,
};

/* high speed support: */
static struct usb_endpoint_descriptor acm_hs_notify_desc = {
	.bLength =      USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =  USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes =     USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =   cpu_to_le16(GS_NOTIFY_MAXPACKET),
	.bInterval =        USB_MS_TO_HS_INTERVAL(GS_NOTIFY_INTERVAL_MS),
};

static struct usb_endpoint_descriptor acm_hs_in_desc = {
	.bLength =      USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =  USB_DT_ENDPOINT,
	.bmAttributes =     USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =   cpu_to_le16(HS_MAXPACKET_SIZE),
};

static struct usb_endpoint_descriptor acm_hs_out_desc = {
	.bLength =      USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =  USB_DT_ENDPOINT,
	.bmAttributes =     USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =   cpu_to_le16(HS_MAXPACKET_SIZE),
};

static struct usb_descriptor_header **acm_hs_cur_function;

static struct usb_descriptor_header *acm_hs_function_single[] = {
	(struct usb_descriptor_header *) &acm_single_interface_desc,
	(struct usb_descriptor_header *) &acm_header_desc,
	(struct usb_descriptor_header *) &acm_descriptor,
	(struct usb_descriptor_header *) &acm_call_mgmt_descriptor,
	(struct usb_descriptor_header *) &acm_union_desc,
	(struct usb_descriptor_header *) &acm_hs_in_desc,
	(struct usb_descriptor_header *) &acm_hs_out_desc,
	NULL,
};

static struct usb_descriptor_header *acm_hs_function_single_notify[] = {
	(struct usb_descriptor_header *) &acm_single_interface_desc,
	(struct usb_descriptor_header *) &acm_header_desc,
	(struct usb_descriptor_header *) &acm_descriptor,
	(struct usb_descriptor_header *) &acm_call_mgmt_descriptor,
	(struct usb_descriptor_header *) &acm_union_desc,
	(struct usb_descriptor_header *) &acm_hs_notify_desc,
	(struct usb_descriptor_header *) &acm_hs_in_desc,
	(struct usb_descriptor_header *) &acm_hs_out_desc,
	NULL,
};

static struct usb_endpoint_descriptor acm_ss_in_desc = {
	.bLength =      USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =  USB_DT_ENDPOINT,
	.bmAttributes =     USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =   cpu_to_le16(SS_MAXPACKET_SIZE),
};

static struct usb_endpoint_descriptor acm_ss_out_desc = {
	.bLength =      USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =  USB_DT_ENDPOINT,
	.bmAttributes =     USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =   cpu_to_le16((SS_MAXPACKET_SIZE)),
};

static struct usb_ss_ep_comp_descriptor acm_ss_bulk_comp_desc = {
	.bLength =              sizeof(acm_ss_bulk_comp_desc),
	.bDescriptorType =      USB_DT_SS_ENDPOINT_COMP,
};


static struct usb_descriptor_header **acm_ss_cur_function;

static struct usb_descriptor_header *acm_ss_function_single[] = {
	(struct usb_descriptor_header *) &acm_single_interface_desc,
	(struct usb_descriptor_header *) &acm_header_desc,
	(struct usb_descriptor_header *) &acm_descriptor,
	(struct usb_descriptor_header *) &acm_call_mgmt_descriptor,
	(struct usb_descriptor_header *) &acm_union_desc,
	(struct usb_descriptor_header *) &acm_ss_in_desc,
	(struct usb_descriptor_header *) &acm_ss_bulk_comp_desc,
	(struct usb_descriptor_header *) &acm_ss_out_desc,
	(struct usb_descriptor_header *) &acm_ss_bulk_comp_desc,
	NULL,
};

static struct usb_descriptor_header *acm_ss_function_single_notify[] = {
	(struct usb_descriptor_header *) &acm_single_interface_desc,
	(struct usb_descriptor_header *) &acm_header_desc,
	(struct usb_descriptor_header *) &acm_descriptor,
	(struct usb_descriptor_header *) &acm_call_mgmt_descriptor,
	(struct usb_descriptor_header *) &acm_union_desc,
	(struct usb_descriptor_header *) &acm_hs_notify_desc,
	(struct usb_descriptor_header *) &acm_ss_bulk_comp_desc,
	(struct usb_descriptor_header *) &acm_ss_in_desc,
	(struct usb_descriptor_header *) &acm_ss_bulk_comp_desc,
	(struct usb_descriptor_header *) &acm_ss_out_desc,
	(struct usb_descriptor_header *) &acm_ss_bulk_comp_desc,
	NULL,
};

/*
 * ACM control ... data handling is delegated to tty library code.
 * The main task of this function is to activate and deactivate
 * that code based on device state; track parameters like line
 * speed, handshake state, and so on; and issue notifications.
 */
static void acm_complete_set_line_coding(struct usb_ep *ep,
			struct usb_request *req)
{
	struct f_acm  *acm = ep->driver_data;
	struct usb_composite_dev *cdev = acm->port.func.config->cdev;

	acm_dbg("+\n");

	if (req->status != 0) {
		DBG(cdev, "acm ttyGS%u completion, err %d\n",
			acm->port_num, req->status);
		return;
	}

	/* normal completion */
	if (req->actual != sizeof(acm->port_line_coding)) {
		DBG(cdev, "acm ttyGS%u short resp, len %u\n",
			acm->port_num, req->actual);
		usb_ep_set_halt(ep);
	} else {
		struct usb_cdc_line_coding  *value = req->buf;

		/*
		 * REVISIT:  we currently just remember this data.
		 * If we change that, (a) validate it first, then
		 * (b) update whatever hardware needs updating,
		 * (c) worry about locking.  This is information on
		 * the order of 9600-8-N-1 ... most of which means
		 * nothing unless we control a real RS232 line.
		 */
		acm->port_line_coding = *value;
	}
	acm_dbg("-\n");
}

static void acm_complete_nop(struct usb_ep *ep,
				struct usb_request *req)
{
}

static int acm_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct f_acm  *acm = func_to_acm(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request  *req = cdev->req;
	int value = -EOPNOTSUPP;
	u16 w_index = le16_to_cpu(ctrl->wIndex);
	u16 w_value = le16_to_cpu(ctrl->wValue);
	u16 w_length = le16_to_cpu(ctrl->wLength);

	acm_dbg("+\n");
	acm_dbg("ctrl: %02x %02x %04x %04x %04x\n", ctrl->bRequestType,
			ctrl->bRequest, ctrl->wValue, ctrl->wIndex,
			ctrl->wLength);

	/*
	 * clear the complete function since it may be assigned
	 * other interface's callback complete function before
	 */
	req->complete = acm_complete_nop;

	/*
	 * composite driver infrastructure handles everything except
	 * CDC class messages; interface activation uses set_alt().
	 *
	 * Note CDC spec table 4 lists the ACM request profile.  It requires
	 * encapsulated command support ... we don't handle any, and respond
	 * to them by stalling.  Options include get/set/clear comm features
	 * (not that useful) and SEND_BREAK.
	 */
	switch ((ctrl->bRequestType << REQUEST_TYPE_BYTE) | ctrl->bRequest) {
	case ((USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_INTERFACE) << REQUEST_TYPE_BYTE)
			| MODEM_ACM_VENDOR_REQ:
	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << REQUEST_TYPE_BYTE)
			| SET_COMM_FEATURE:
	case ((USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_INTERFACE) << REQUEST_TYPE_BYTE)
			| MODEM_ACM_STD_REQ:
		pr_debug("f_acm request: wValue: 0x%04x, wIndex: 0x%04x, wLength: 0x%04x\n",
			 w_value, w_index, w_length);
		value = w_length;
		cdev->gadget->ep0->driver_data = acm;
		break;

		/* SET_LINE_CODING ... just read and save what the host sends */
	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << REQUEST_TYPE_BYTE)
			| USB_CDC_REQ_SET_LINE_CODING:
		if (w_length != sizeof(struct usb_cdc_line_coding)
			|| w_index != acm->ctrl_id)
			goto invalid;

		value = w_length;
		cdev->gadget->ep0->driver_data = acm;
		req->complete = acm_complete_set_line_coding;
		break;

		/*
		 * GET_LINE_CODING ...
		 * return what host sent, or initial value
		 */
	case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << REQUEST_TYPE_BYTE)
			| USB_CDC_REQ_GET_LINE_CODING:
		if (w_index != acm->ctrl_id)
			goto invalid;

		value = (int)min_t(unsigned int, w_length,
					sizeof(struct usb_cdc_line_coding));
		memcpy(req->buf, &acm->port_line_coding, value);
		break;

		/* SET_CONTROL_LINE_STATE ... save what the host sent */
	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << REQUEST_TYPE_BYTE)
			| USB_CDC_REQ_SET_CONTROL_LINE_STATE:
		if (w_index != (u16)acm->ctrl_id)
			goto invalid;

		value = 0;

		/*
		 * we should not allow data to flow until the
		 * host sets the ACM_CTRL_DTR bit; and when it clears
		 * that bit, we should return to that no-flow state.
		 */
		acm->port_handshake_bits = w_value;
		gacm_cdev_line_state(&acm->port, acm->port_num, (u32)w_value);
		break;

	default:
invalid:
		VDBG(cdev, "invalid control req%02x.%02x v%04x i%04x l%u\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
	}

	/* respond with data transfer or status phase? */
	if (value >= 0) {
		DBG(cdev, "acm ttyGS%d req%02x.%02x v%04x i%04x l%u\n",
			acm->port_num, ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
		req->zero = 0;
		req->length = (unsigned int)value;
		value = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
		if (value < 0)
			ERROR(cdev, "acm response on ttyGS%u, err %d\n",
				acm->port_num, value);
	}

	/* device either stalls (value < 0) or reports success */
	acm_dbg("-\n");
	return value;
}

static void acm_suspend(struct usb_function *f)
{
	struct f_acm *acm = func_to_acm(f);

	gacm_cdev_suspend_notify(&acm->port);
}

static void acm_resume(struct usb_function *f)
{
	struct f_acm *acm = func_to_acm(f);

	gacm_cdev_resume_notify(&acm->port);
}

static int acm_set_alt(struct usb_function *f, unsigned int intf, unsigned int alt)
{
	struct f_acm  *acm = func_to_acm(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	bool is_setting = 0;
	int ret;

	acm_dbg("+\n");
	/* we know alt == 0, so this is an activation or a reset */

	/*
	 * if it is single interface, intf, acm->ctrl_id and acm->data_id
	 * are the same, so we can setting data and notify interface
	 * in the same time.
	 *
	 * if it is multi interface, acm->ctrl_id and
	 * acm->data_id are different,
	 * so the setting is go ahead in different times.
	 */
	if (intf == acm->ctrl_id) {
		is_setting = 1;
		if (acm->notify != NULL) {
			VDBG(cdev, "reset acm control interface %u\n", intf);
			usb_ep_disable(acm->notify);
			VDBG(cdev, "init acm ctrl interface %u\n", intf);
			if (config_ep_by_speed(cdev->gadget, f, acm->notify))
				return -EINVAL;
			ret = usb_ep_enable(acm->notify);
			if (ret < 0) {
				ERROR(cdev, "Enable acm interface ep failed\n");
				return ret;
			}
		}
	}

	if (intf == acm->data_id) {
		is_setting = 1;
		if (acm->port.in->enabled) {
			DBG(cdev, "reset acm ttyGS%u\n", acm->port_num);
			gacm_cdev_disconnect(&acm->port);
		}
		if (acm->port.in->desc == NULL || acm->port.out->desc == NULL) {
			DBG(cdev, "activate acm ttyGS%u\n", acm->port_num);
			if (config_ep_by_speed(cdev->gadget, f,
				acm->port.in) ||
				config_ep_by_speed(cdev->gadget, f,
					acm->port.out)) {
				acm->port.in->desc = NULL;
				acm->port.out->desc = NULL;
				return -EINVAL;
			}
		}
		gacm_cdev_connect(&acm->port, acm->port_num);

		bsp_usb_set_enum_stat(acm->data_id, 1);
	}

	if (!is_setting)
		return -EINVAL;

	acm_dbg("-\n");
	return 0;
}
static void acm_disable(struct usb_function *f)
{
	struct f_acm  *acm = func_to_acm(f);
	struct usb_composite_dev *cdev = f->config->cdev;

	acm_dbg("+\n");

	DBG(cdev, "acm ttyGS%u deactivated\n", acm->port_num);

	gacm_cdev_disconnect(&acm->port);

	if (acm->notify != NULL)
		usb_ep_disable(acm->notify);

	bsp_usb_set_enum_stat(acm->data_id, 0);

	acm_dbg("-\n");
}

/*
 * acm_cdc_notify - issue CDC notification to host
 * @acm: wraps host to be notified
 * @type: notification type
 * @value: Refer to cdc specs, wValue field.
 * @data: data to be sent
 * @length: size of data
 * Context: irqs blocked, acm->lock held, acm_notify_req non-null
 *
 * Returns zero on success or a negative errno.
 *
 * See section 6.3.5 of the CDC 1.1 specification for information
 * about the only notification we issue:  SerialState change.
 */
static int acm_cdc_notify(struct f_acm *acm, u8 type, u16 value,
				void *data, unsigned int length, bool is_vendor)
{
	struct usb_ep  *ep = acm->notify;
	struct usb_request *req = NULL;
	struct usb_cdc_notification *notify = NULL;
	const unsigned int len = sizeof(*notify) + length;
	void *buf = NULL;
	int status;

	acm_dbg("+\n");

	req = acm->notify_req;
	acm->notify_req = NULL;
	acm->pending = false;

	req->length = len;
	notify = req->buf;
	buf = notify + 1;

	notify->bmRequestType = USB_DIR_IN |
			(is_vendor ? USB_TYPE_VENDOR : USB_TYPE_CLASS) |
			USB_RECIP_INTERFACE;
	notify->bNotificationType = type;
	notify->wValue = cpu_to_le16(value);
	notify->wIndex = cpu_to_le16(acm->ctrl_id);
	notify->wLength = cpu_to_le16(length);

	if (length  && data != NULL)
		memcpy(buf, data, length);

	/* ep_queue() can complete immediately if it fills the fifo... */
	spin_unlock(&acm->lock);
	status = usb_ep_queue(ep, req, GFP_ATOMIC);
	spin_lock(&acm->lock);

	if (status < 0) {
		ERROR(acm->port.func.config->cdev,
			"acm ttyGS%u can't notify serial state, %d\n",
			acm->port_num, status);
		acm->notify_req = req;
	}

	acm_dbg("-\n");
	return status;
}

static int acm_notify_serial_state(struct f_acm *acm)
{
	struct usb_composite_dev *cdev = acm->port.func.config->cdev;
	int status;
	unsigned long flags = 0;

	acm_dbg("+\n");

	spin_lock_irqsave(&acm->lock, flags);
	if (acm->notify_req != NULL) {
		DBG(cdev, "acm ttyGS%u serial state %04x\n",
			acm->port_num, acm->serial_state);
		status = acm_cdc_notify(acm, USB_CDC_NOTIFY_SERIAL_STATE,
				0, &acm->serial_state, sizeof(acm->serial_state), 0);
	} else {
		acm->pending = true;
		acm->pending_notify = acm_notify_serial_state;
		status = 0;
	}
	spin_unlock_irqrestore(&acm->lock, flags);
	acm_dbg("-\n");
	return status;
}

static int acm_notify_flow_control(struct f_acm *acm)
{
	int status;
	u16 value;
	unsigned long flags = 0;

	acm_dbg("+\n");

	spin_lock_irqsave(&acm->lock, flags);
	if (acm->notify_req != NULL) {
		/* send flow control messages adapt JUNGO host vcom driver */
		value = (acm->rx_is_on ? 0x1 : 0x0) | (acm->tx_is_on ? 0x2 : 0x0);
		status = acm_cdc_notify(acm, USB_CDC_VENDOR_NTF_FLOW_CONTROL,
				value, NULL, 0, 1);
	} else {
		acm->pending = true;
		acm->pending_notify = acm_notify_flow_control;
		status = 0;
	}
	spin_unlock_irqrestore(&acm->lock, flags);
	acm_dbg("-\n");
	return status;
}

static void acm_cdc_notify_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_acm *acm = req->context;
	u8 doit = 0;

	acm_dbg("+\n");

	/*
	 * on this call path we do NOT hold the port spinlock,
	 * which is why ACM needs its own spinlock
	 */
	spin_lock(&acm->lock);
	if (req->status != -ESHUTDOWN)
		doit = acm->pending;
	acm->notify_req = req;
	spin_unlock(&acm->lock);

	if (doit && acm->pending_notify != NULL)
		acm->pending_notify(acm);
	acm_dbg("-\n");
}

static void acm_connect(struct gserial *port)
{
	struct f_acm *acm = port_to_acm(port);
	int ret;

	acm_dbg("+\n");
	acm->serial_state |= ACM_CTRL_DSR | ACM_CTRL_DCD;
	ret = acm_notify_serial_state(acm);
	if (ret < 0)
		pr_err("%s:faild\n", __func__);
	acm_dbg("-\n");
}

static void acm_disconnect(struct gserial *port)
{
	struct f_acm *acm = port_to_acm(port);
	int ret;

	acm_dbg("+\n");
	acm->serial_state &= ~(ACM_CTRL_DSR | ACM_CTRL_DCD);
	ret = acm_notify_serial_state(acm);
	if (ret < 0)
		pr_err("%s:faild\n", __func__);
	acm_dbg("-\n");
}

static void acm_notify_state(struct gserial *port, u16 state)
{
	struct f_acm  *acm = port_to_acm(port);
	int ret;

	acm_dbg("+\n");
	acm->serial_state = state;
	ret = acm_notify_serial_state(acm);
	if (ret < 0)
		pr_err("%s:faild\n", __func__);
	acm_dbg("-\n");
}

static void acm_flow_control(struct gserial *port, u32 rx_is_on, u32 tx_is_on)
{
	struct f_acm  *acm = port_to_acm(port);

	acm_dbg("+\n");
	acm->rx_is_on = rx_is_on;
	acm->tx_is_on = tx_is_on;
	(void)acm_notify_flow_control(acm);
	acm_dbg("-\n");
}

static int acm_send_break(struct gserial *port, int duration)
{
	struct f_acm  *acm = port_to_acm(port);
	u16 state;

	acm_dbg("+\n");
	state = acm->serial_state;
	state &= ~ACM_CTRL_BRK;
	if (duration)
		state |= ACM_CTRL_BRK;

	acm->serial_state = state;

	acm_dbg("-\n");

	return acm_notify_serial_state(acm);
}

/* ------------------------------------------------------------------------- */
static void acm_assign_ep_address(struct f_acm *acm)
{
	acm_dbg("do desc\n");
	acm_hs_in_desc.bEndpointAddress = acm_fs_in_desc.bEndpointAddress;
	acm_hs_out_desc.bEndpointAddress = acm_fs_out_desc.bEndpointAddress;

	if (acm->support_notify)
		acm_hs_notify_desc.bEndpointAddress =
			acm_fs_notify_desc.bEndpointAddress;

	acm_ss_in_desc.bEndpointAddress = acm_fs_in_desc.bEndpointAddress;
	acm_ss_out_desc.bEndpointAddress = acm_fs_out_desc.bEndpointAddress;
}

static void acm_assign_desc(struct f_acm *acm)
{
	acm_dbg("to assign desc\n");
	/*
	 * choose descriptors according to single interface or not,
	 * and support notify or not.
	 */
	if (acm->support_notify) {
		/* bulk in + bulk out + interrupt in */
		acm_single_interface_desc.bNumEndpoints = 3;
		acm_fs_cur_function = acm_fs_function_single_notify;
		acm_hs_cur_function = acm_hs_function_single_notify;
		acm_ss_cur_function = acm_ss_function_single_notify;
	} else {
		/* bulk in + bulk out */
		acm_single_interface_desc.bNumEndpoints = 2;
		acm_fs_cur_function = acm_fs_function_single;
		acm_hs_cur_function = acm_hs_function_single;
		acm_ss_cur_function = acm_ss_function_single;
	}

	acm_single_interface_desc.bInterfaceClass = 0xFF;
	acm_single_interface_desc.bInterfaceSubClass = MODEM_ACM_INTERFACE_SUBCLASS;
	acm_single_interface_desc.bInterfaceProtocol = get_acm_protocol_type(acm->port_num);
}

static int ep_autoconfig_notify(struct f_acm *acm, struct usb_composite_dev *cdev)
{
	struct usb_ep *ep = NULL;

	if (acm->support_notify) {
		ep = usb_ep_autoconfig(cdev->gadget, &acm_fs_notify_desc);
		if (ep == NULL)
			return -1;
		acm->notify = ep;

		/* allocate notification */
		acm->notify_req = gs_acm_cdev_alloc_req(ep,
					NOTIFY_REQ_BUF_SIZE, GFP_KERNEL);
		if (acm->notify_req == NULL)
			return -1;

		acm->notify_req->complete = acm_cdc_notify_complete;
		acm->notify_req->context = acm;
	} else {
		acm->notify = NULL;
		acm->notify_req = NULL;
	}
	return 0;
}


/* ACM function driver setup/binding */
static int acm_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct f_acm  *acm = func_to_acm(f);
	int status;
	struct usb_ep *ep = NULL;

	acm_dbg("+\n");

	/* allocate instance-specific interface IDs, and patch descriptors */
	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;

	acm_dbg("interface id: %d\n", status);

	acm_dbg("single interface\n");
	acm->ctrl_id = acm->data_id = (u8)status;
	acm_single_interface_desc.bInterfaceNumber = (u8)status;
	acm_call_mgmt_descriptor.bDataInterface = (u8)status;

	bsp_usb_add_setup_dev_fdname((unsigned int)acm->data_id, "f_modem_acm");

	status = -ENODEV;

	/* allocate instance-specific endpoints */
	acm_dbg("to ep autoconfig\n");
	ep = usb_ep_autoconfig(cdev->gadget, &acm_fs_in_desc);
	if (ep == NULL)
		goto fail;
	acm->port.in = ep;
	ep = usb_ep_autoconfig(cdev->gadget, &acm_fs_out_desc);
	if (ep == NULL)
		goto fail;
	acm->port.out = ep;

	status = ep_autoconfig_notify(acm, cdev);
	if (status < 0)
		goto fail;

	/*
	 * support all relevant hardware speeds... we expect that when
	 * hardware is dual speed, all bulk-capable endpoints work at
	 * both speeds
	 */
	acm_assign_ep_address(acm);
	acm_assign_desc(acm);

	status = usb_assign_descriptors(f,
			acm_fs_cur_function, acm_hs_cur_function,
			acm_ss_cur_function, acm_ss_cur_function);
	if (status)
		goto fail;

	DBG(cdev, "acm_cdev%d: %s speed IN/%s OUT/%s NOTIFY/%s\n",
		acm->port_num,
		gadget_is_superspeed(c->cdev->gadget) ? "super" :
		gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full",
		acm->port.in->name, acm->port.out->name,
		acm->notify ? acm->notify->name : "null");

	return 0;

fail:
	if (acm->notify_req != NULL)
		gs_acm_cdev_free_req(acm->notify, acm->notify_req);
	ERROR(cdev, "%s/%pK: can't bind, err %d\n", f->name, f, status);

	acm_dbg("-\n");
	return status;
}

static void acm_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_acm *acm = func_to_acm(f);

	acm_dbg("+\n");
	usb_free_all_descriptors(f);
	if (acm->notify_req != NULL)
		gs_acm_cdev_free_req(acm->notify, acm->notify_req);
	bsp_usb_remove_setup_dev_fdname();
	acm_dbg("-\n");
}

static void acm_free_func(struct usb_function *f)
{
	struct f_acm *acm = func_to_acm(f);

	acm_dbg("+\n");
	kfree(acm);
	acm_dbg("-\n");
}

static struct usb_function *acm_alloc_func(struct usb_function_instance *fi)
{
	struct modem_acm_instance *inst = NULL;
	struct f_acm *acm = NULL;

	acm_dbg("+\n");
	acm = kzalloc(sizeof(*acm), GFP_KERNEL);
	if (acm == NULL)
		return ERR_PTR(-ENOMEM);

	spin_lock_init(&acm->lock);

	inst = container_of(fi, struct modem_acm_instance, func_inst);
	acm->support_notify = is_support_notify(inst->port_num);
	if (acm->support_notify) {
		acm->port.connect = acm_connect;
		acm->port.disconnect = acm_disconnect;

		acm->port.notify_state = acm_notify_state;
		acm->port.flow_control = acm_flow_control;

		acm->port.send_break = acm_send_break;

		acm->port.func.suspend = acm_suspend;
		acm->port.func.resume = acm_resume;
	}

	acm->port.func.name = "modem_acm";
	acm->port.func.bind = acm_bind;
	acm->port.func.set_alt = acm_set_alt;
	acm->port.func.setup = acm_setup;
	acm->port.func.disable = acm_disable;

	acm->port.func.unbind = acm_unbind;
	acm->port.func.free_func = acm_free_func;

	acm->port_num = inst->port_num;

	acm_dbg("-\n");
	return &acm->port.func;
}

static inline struct modem_acm_instance *to_modem_acm_instance(
						struct config_item *item)
{
	return container_of(to_config_group(item), struct modem_acm_instance,
			func_inst.group);
}

static void modem_acm_attr_release(struct config_item *item)
{
	struct modem_acm_instance *inst = to_modem_acm_instance(item);

	usb_put_function_instance(&inst->func_inst);
}

static struct configfs_item_operations modem_acm_item_ops = {
	.release = modem_acm_attr_release,
};

static ssize_t f_modem_acm_port_num_show(struct config_item *item, char *page)
{
	return snprintf(page, PAGE_SIZE, "%u\n",
			to_modem_acm_instance(item)->port_num);
}

CONFIGFS_ATTR_RO(f_modem_acm_port_, num);

static struct configfs_attribute *modem_acm_attrs[] = {
	&f_modem_acm_port_attr_num,
	NULL,
};

static struct config_item_type modem_acm_func_type = {
	.ct_item_ops = &modem_acm_item_ops,
	.ct_attrs = modem_acm_attrs,
	.ct_owner = THIS_MODULE,
};

static void acm_free_instance(struct usb_function_instance *fi)
{
	struct modem_acm_instance *inst = NULL;

	acm_dbg("+\n");
	inst = container_of(fi, struct modem_acm_instance, func_inst);
	gs_acm_cdev_free_line(inst->port_num);
	kfree(inst);
	acm_dbg("-\n");
}

static struct usb_function_instance *acm_alloc_instance(void)
{
	struct modem_acm_instance *inst = NULL;
	int ret;

	acm_dbg("+\n");
	inst = kzalloc(sizeof(*inst), GFP_KERNEL);
	if (inst == NULL)
		return ERR_PTR(-ENOMEM);
	inst->func_inst.free_func_inst = acm_free_instance;

	ret = gs_acm_cdev_alloc_line(&inst->port_num);
	if (ret) {
		kfree(inst);
		return ERR_PTR(ret);
	}
	config_group_init_type_name(&inst->func_inst.group, "",
		&modem_acm_func_type);
	acm_dbg("-\n");

	return &inst->func_inst;
}
DECLARE_USB_FUNCTION_INIT(modem_acm, acm_alloc_instance, acm_alloc_func);
MODULE_LICENSE("GPL v2");
