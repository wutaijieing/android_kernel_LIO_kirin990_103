/*
 * f_sourcesink_ext_ext.c
 *
 * USB peripheral source/sink extensive configuration driver
 *
 * Copyright (c) 2018-2020 Huawei Technologies Co., Ltd.
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

#include <linux/device.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/usb/composite.h>
#include <linux/workqueue.h>
#include "securec.h"

#undef pr_fmt
#define pr_fmt(fmt) "[SSE_FUNC]%s: " fmt, __func__

/*
 * SOURCE/SINK EXT FUNCTION ... a primary testing vehicle for USB peripheral
 * controller drivers.
 */
struct f_ss_ext_statistics {
	uint32_t		reqs_count;
	uint32_t		pattern;
	uint64_t		data_len;
	uint32_t		data_check_err_count;
	uint32_t		status_err_count;
	/* just for isoc */
	uint32_t		isoc_reqs_count_since_err;
	uint32_t		isoc_err_reqs_count;
};

struct f_ss_ext_opts {
	struct usb_function_instance func_inst;
	unsigned pattern;
	unsigned isoc_interval;
	unsigned isoc_maxpacket;
	unsigned isoc_mult;
	unsigned isoc_maxburst;
	unsigned isoc_qlen;

	unsigned bulk_buflen;
	unsigned bulk_qlen;
	unsigned bulk_maxpacket;

	unsigned intr_interval;
	unsigned intr_maxpacket;
	unsigned intr_buflen;
	unsigned intr_qlen;

#define SS_TEST_MODE_ISOC_BULK_INTR 0 /* defalut */
#define SS_TEST_MODE_BULK_ONLY 1
#define SS_TEST_MODE_ISOC_ONLY 2
#define SS_TEST_MODE_INTR_ONLY 3

	unsigned test_mode;
	unsigned char inst_num;

	/*
	 * Read/write access to configfs attributes is handled by configfs.
	 *
	 * This is to protect the data from concurrent access by read/write
	 * and create symlink/remove symlink.
	 */
	struct mutex lock;
	int refcnt;
};

struct f_sourcesink_ext {
	struct usb_function	function;

#define SS_EP_NUM 6
#define SS_BULK_IN_EP_IDX 0
#define SS_BULK_OUT_EP_IDX 1
#define SS_INTR_IN_EP_IDX 2
#define SS_INTR_OUT_EP_IDX 3
#define SS_ISOC_IN_EP_IDX 4
#define SS_ISOC_OUT_EP_IDX 5
	struct usb_ep *ep[SS_EP_NUM];
	int			cur_alt;

	unsigned pattern;
	unsigned isoc_interval;
	unsigned isoc_maxpacket;
	unsigned isoc_mult;
	unsigned isoc_maxburst;
	unsigned isoc_buflen;
	unsigned isoc_qlen;

	unsigned bulk_buflen;
	unsigned bulk_qlen;
	unsigned bulk_maxpacket;

	unsigned intr_interval;
	unsigned intr_maxpacket;
	unsigned intr_buflen;
	unsigned intr_qlen;

	unsigned test_mode;

	unsigned func_num;

	/* support dequeue, stop/start ep */
	/* input */
	unsigned ep_idx;
	u8 brequest;
	struct work_struct handler_work;

	struct f_ss_ext_statistics stat[SS_EP_NUM];

#define SS_MAX_REQ_NUM 16
	struct usb_request *reqs[SS_EP_NUM][SS_MAX_REQ_NUM];

#define SS_MAX_DEQUEUE_NUM 8
	struct usb_request *dequeue_reqs[SS_EP_NUM][SS_MAX_DEQUEUE_NUM];
};

static void handler_work(struct work_struct *work);

static inline struct f_sourcesink_ext *func_to_ss(struct usb_function *f)
{
	return container_of(f, struct f_sourcesink_ext, function);
}

/*-------------------------------------------------------------------------*/

static struct usb_interface_descriptor source_sink_intf_alt0 = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,

	.bAlternateSetting =	0,
	.bNumEndpoints =	2,
	.bInterfaceClass =	USB_CLASS_VENDOR_SPEC,
	/* .iInterface		= DYNAMIC */
};

static struct usb_interface_descriptor source_sink_intf_alt0_for_isoc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,

	.bAlternateSetting =	0,
	.bNumEndpoints =	0,
	.bInterfaceClass =	USB_CLASS_VENDOR_SPEC,
	/* .iInterface		= DYNAMIC */
};

static struct usb_interface_descriptor source_sink_intf_alt1 = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,

	.bAlternateSetting =	1,
	.bNumEndpoints =	2,
	.bInterfaceClass =	USB_CLASS_VENDOR_SPEC,
	/* .iInterface		= DYNAMIC */
};

static struct usb_interface_descriptor source_sink_intf_alt2 = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,

	.bAlternateSetting =	2,
	.bNumEndpoints =	2,
	.bInterfaceClass =	USB_CLASS_VENDOR_SPEC,
	/* .iInterface		= DYNAMIC */
};

static struct usb_interface_descriptor source_sink_intf_alt3 = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,

	.bAlternateSetting =	3,
	.bNumEndpoints =	6,
	.bInterfaceClass =	USB_CLASS_VENDOR_SPEC,
	/* .iInterface		= DYNAMIC */
};


/* full speed support: */

static struct usb_endpoint_descriptor fs_bulk_source_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor fs_bulk_sink_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor fs_isoc_source_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize =	cpu_to_le16(1023),
	.bInterval =		4,
};

static struct usb_endpoint_descriptor fs_isoc_sink_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize =	cpu_to_le16(1023),
	.bInterval =		4,
};

static struct usb_endpoint_descriptor fs_intr_source_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	cpu_to_le16(64),
	.bInterval =		4,
};

static struct usb_endpoint_descriptor fs_intr_sink_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	cpu_to_le16(64),
	.bInterval =		4,
};


static struct usb_descriptor_header *fs_source_sink_descs[] = {
	(struct usb_descriptor_header *) &source_sink_intf_alt0,
	(struct usb_descriptor_header *) &fs_bulk_sink_desc,
	(struct usb_descriptor_header *) &fs_bulk_source_desc,

	(struct usb_descriptor_header *) &source_sink_intf_alt1,
#define FS_ALT_IFC_1_OFFSET	3
	(struct usb_descriptor_header *) &fs_intr_sink_desc,
	(struct usb_descriptor_header *) &fs_intr_source_desc,

	(struct usb_descriptor_header *) &source_sink_intf_alt2,
#define FS_ALT_IFC_2_OFFSET	6
	(struct usb_descriptor_header *) &fs_isoc_sink_desc,
	(struct usb_descriptor_header *) &fs_isoc_source_desc,

	(struct usb_descriptor_header *) &source_sink_intf_alt2,
#define FS_ALT_IFC_3_OFFSET	9
	(struct usb_descriptor_header *) &fs_bulk_sink_desc,
	(struct usb_descriptor_header *) &fs_bulk_source_desc,
	(struct usb_descriptor_header *) &fs_intr_sink_desc,
	(struct usb_descriptor_header *) &fs_intr_source_desc,
	(struct usb_descriptor_header *) &fs_isoc_sink_desc,
	(struct usb_descriptor_header *) &fs_isoc_source_desc,
	NULL,
};

/* high speed support: */

static struct usb_endpoint_descriptor hs_bulk_source_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),
};

static struct usb_endpoint_descriptor hs_bulk_sink_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),
};

static struct usb_endpoint_descriptor hs_isoc_source_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize =	cpu_to_le16(1024),
	.bInterval =		4,
};

static struct usb_endpoint_descriptor hs_isoc_sink_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize =	cpu_to_le16(1024),
	.bInterval =		4,
};

static struct usb_endpoint_descriptor hs_intr_source_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	cpu_to_le16(512),
	.bInterval =		4,
};

static struct usb_endpoint_descriptor hs_intr_sink_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	cpu_to_le16(512),
	.bInterval =		4,
};

static struct usb_descriptor_header *hs_source_sink_descs[] = {
	(struct usb_descriptor_header *) &source_sink_intf_alt0,
	(struct usb_descriptor_header *) &hs_bulk_source_desc,
	(struct usb_descriptor_header *) &hs_bulk_sink_desc,

	(struct usb_descriptor_header *) &source_sink_intf_alt1,
#define HS_ALT_IFC_1_OFFSET	3
	(struct usb_descriptor_header *) &hs_intr_source_desc,
	(struct usb_descriptor_header *) &hs_intr_sink_desc,

	(struct usb_descriptor_header *) &source_sink_intf_alt2,
#define HS_ALT_IFC_2_OFFSET	6
	(struct usb_descriptor_header *) &hs_isoc_source_desc,
	(struct usb_descriptor_header *) &hs_isoc_sink_desc,

	(struct usb_descriptor_header *) &source_sink_intf_alt2,
#define HS_ALT_IFC_3_OFFSET	9
	(struct usb_descriptor_header *) &hs_bulk_source_desc,
	(struct usb_descriptor_header *) &hs_bulk_sink_desc,
	(struct usb_descriptor_header *) &hs_intr_source_desc,
	(struct usb_descriptor_header *) &hs_intr_sink_desc,
	(struct usb_descriptor_header *) &hs_isoc_source_desc,
	(struct usb_descriptor_header *) &hs_isoc_sink_desc,
	NULL,
};

/* super speed support: */

static struct usb_endpoint_descriptor ss_bulk_source_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(1024),
};

static struct usb_ss_ep_comp_descriptor ss_bulk_source_comp_desc = {
	.bLength =		USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType =	USB_DT_SS_ENDPOINT_COMP,

	.bMaxBurst =		0,
	.bmAttributes =		0,
	.wBytesPerInterval =	0,
};

static struct usb_endpoint_descriptor ss_bulk_sink_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(1024),
};

static struct usb_ss_ep_comp_descriptor ss_bulk_sink_comp_desc = {
	.bLength =		USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType =	USB_DT_SS_ENDPOINT_COMP,

	.bMaxBurst =		0,
	.bmAttributes =		0,
	.wBytesPerInterval =	0,
};

static struct usb_endpoint_descriptor ss_isoc_source_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize =	cpu_to_le16(1024),
	.bInterval =		4,
};

static struct usb_ss_ep_comp_descriptor ss_isoc_source_comp_desc = {
	.bLength =		USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType =	USB_DT_SS_ENDPOINT_COMP,

	.bMaxBurst =		0,
	.bmAttributes =		0,
	.wBytesPerInterval =	cpu_to_le16(1024),
};

static struct usb_endpoint_descriptor ss_isoc_sink_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize =	cpu_to_le16(1024),
	.bInterval =		4,
};

static struct usb_ss_ep_comp_descriptor ss_isoc_sink_comp_desc = {
	.bLength =		USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType =	USB_DT_SS_ENDPOINT_COMP,

	.bMaxBurst =		0,
	.bmAttributes =		0,
	.wBytesPerInterval =	cpu_to_le16(1024),
};

static struct usb_endpoint_descriptor ss_intr_source_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	cpu_to_le16(1024),
	.bInterval =		4,
};

static struct usb_ss_ep_comp_descriptor ss_intr_source_comp_desc = {
	.bLength =		USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType =	USB_DT_SS_ENDPOINT_COMP,

	.bMaxBurst =		0,
	.bmAttributes =		0,
	.wBytesPerInterval =	cpu_to_le16(1024),
};

static struct usb_endpoint_descriptor ss_intr_sink_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	cpu_to_le16(1024),
	.bInterval =		4,
};

static struct usb_ss_ep_comp_descriptor ss_intr_sink_comp_desc = {
	.bLength =		USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType =	USB_DT_SS_ENDPOINT_COMP,

	.bMaxBurst =		0,
	.bmAttributes =		0,
	.wBytesPerInterval =	cpu_to_le16(1024),
};

static struct usb_descriptor_header *ss_source_sink_descs[] = {
	(struct usb_descriptor_header *) &source_sink_intf_alt0,
	(struct usb_descriptor_header *) &ss_bulk_source_desc,
	(struct usb_descriptor_header *) &ss_bulk_source_comp_desc,
	(struct usb_descriptor_header *) &ss_bulk_sink_desc,
	(struct usb_descriptor_header *) &ss_bulk_sink_comp_desc,

	(struct usb_descriptor_header *) &source_sink_intf_alt1,
#define SS_ALT_IFC_1_OFFSET	5
	(struct usb_descriptor_header *) &ss_intr_source_desc,
	(struct usb_descriptor_header *) &ss_intr_source_comp_desc,
	(struct usb_descriptor_header *) &ss_intr_sink_desc,
	(struct usb_descriptor_header *) &ss_intr_sink_comp_desc,

#define SS_ALT_IFC_2_OFFSET	10
	(struct usb_descriptor_header *) &source_sink_intf_alt1,
	(struct usb_descriptor_header *) &ss_isoc_source_desc,
	(struct usb_descriptor_header *) &ss_isoc_source_comp_desc,
	(struct usb_descriptor_header *) &ss_isoc_sink_desc,
	(struct usb_descriptor_header *) &ss_isoc_sink_comp_desc,

	(struct usb_descriptor_header *) &source_sink_intf_alt2,
#define SS_ALT_IFC_3_OFFSET	15
	(struct usb_descriptor_header *) &ss_bulk_source_desc,
	(struct usb_descriptor_header *) &ss_bulk_source_comp_desc,
	(struct usb_descriptor_header *) &ss_bulk_sink_desc,
	(struct usb_descriptor_header *) &ss_bulk_sink_comp_desc,
	(struct usb_descriptor_header *) &ss_intr_source_desc,
	(struct usb_descriptor_header *) &ss_intr_source_comp_desc,
	(struct usb_descriptor_header *) &ss_intr_sink_desc,
	(struct usb_descriptor_header *) &ss_intr_sink_comp_desc,
	(struct usb_descriptor_header *) &ss_isoc_source_desc,
	(struct usb_descriptor_header *) &ss_isoc_source_comp_desc,
	(struct usb_descriptor_header *) &ss_isoc_sink_desc,
	(struct usb_descriptor_header *) &ss_isoc_sink_comp_desc,
	NULL,
};

static struct usb_endpoint_descriptor *fs_source_desc_p = &fs_bulk_source_desc;
static struct usb_endpoint_descriptor *fs_sink_desc_p = &fs_bulk_sink_desc;

/* function-specific strings: */
static struct usb_string strings_sourcesink[] = {
	[0].s = "source and sink data",
	{  } /* end of list */
};

static struct usb_gadget_strings stringtab_sourcesink = {
	.language	= 0x0409, /* en-us */
	.strings	= strings_sourcesink,
};

static struct usb_gadget_strings *sourcesink_strings[] = {
	&stringtab_sourcesink,
	NULL,
};

/*-------------------------------------------------------------------------*/

static struct usb_request *_ss_ext_alloc_ep_req(struct usb_ep *ep,
						unsigned int len)
{
	struct usb_request      *req = NULL;

	req = usb_ep_alloc_request(ep, GFP_KERNEL);
	if (req) {
		req->length = len;
		req->buf = kmalloc(req->length, GFP_KERNEL);
		if (!req->buf) {
			usb_ep_free_request(ep, req);
			req = NULL;
		}
	}
	return req;
}

static int ss_ext_disable_endpoint(struct usb_ep *ep)
{
	int value = 0;

	if (ep) {
		value = usb_ep_disable(ep);
		if (value < 0)
			pr_err("disable %s --> %d\n", ep->name, value);
	}

	return value;
}

static void isoc_para_sanity_check(struct f_sourcesink_ext *ss)
{
	if (ss->isoc_interval < 1)
		ss->isoc_interval = 1;
	if (ss->isoc_interval > 16)
		ss->isoc_interval = 16;
	if (ss->isoc_mult > 2)
		ss->isoc_mult = 2;
	if (ss->isoc_maxburst > 15)
		ss->isoc_maxburst = 15;
	if (ss->isoc_maxpacket > 1024)
		ss->isoc_maxpacket = 1024;
}

static void fs_isoc_desc_update(struct f_sourcesink_ext *ss)
{
	fs_isoc_source_desc.wMaxPacketSize = (ss->isoc_maxpacket > 1023) ?
						1023 : ss->isoc_maxpacket;
	fs_isoc_source_desc.bInterval = ss->isoc_interval;
	fs_isoc_sink_desc.wMaxPacketSize = (ss->isoc_maxpacket > 1023) ?
						1023 : ss->isoc_maxpacket;
	fs_isoc_sink_desc.bInterval = ss->isoc_interval;
}

static void fs_intr_desc_update(struct f_sourcesink_ext *ss)
{
	fs_intr_source_desc.wMaxPacketSize = (ss->intr_maxpacket > 64) ?
						64 : ss->intr_maxpacket;
	fs_intr_source_desc.bInterval = ss->intr_interval;
	fs_intr_sink_desc.wMaxPacketSize = (ss->intr_maxpacket > 64) ?
						64 : ss->intr_maxpacket;
	fs_intr_sink_desc.bInterval = ss->intr_interval;
}

static void fs_bulk_desc_update(struct f_sourcesink_ext *ss)
{
	fs_bulk_source_desc.wMaxPacketSize = (ss->bulk_maxpacket > 64) ?
						64 : ss->bulk_maxpacket;
	fs_bulk_sink_desc.wMaxPacketSize = (ss->bulk_maxpacket > 64) ?
						64 : ss->bulk_maxpacket;
}

static void hs_desc_update(struct f_sourcesink_ext *ss)
{
	hs_bulk_source_desc.bEndpointAddress = fs_bulk_source_desc.bEndpointAddress;
	hs_bulk_sink_desc.bEndpointAddress = fs_bulk_sink_desc.bEndpointAddress;
	hs_isoc_source_desc.bEndpointAddress = fs_isoc_source_desc.bEndpointAddress;
	hs_isoc_sink_desc.bEndpointAddress = fs_isoc_sink_desc.bEndpointAddress;
	hs_intr_source_desc.bEndpointAddress = fs_intr_source_desc.bEndpointAddress;
	hs_intr_sink_desc.bEndpointAddress = fs_intr_sink_desc.bEndpointAddress;

	/*
	 * Fill in the HS isoc descriptors from the module parameters.
	 * We assume that the user knows what they are doing and won't
	 * give parameters that their UDC doesn't support.
	 */
	hs_isoc_source_desc.wMaxPacketSize = ss->isoc_maxpacket;
	hs_isoc_source_desc.wMaxPacketSize |= ss->isoc_mult << 11;
	hs_isoc_source_desc.bInterval = ss->isoc_interval;

	hs_isoc_sink_desc.wMaxPacketSize = ss->isoc_maxpacket;
	hs_isoc_sink_desc.wMaxPacketSize |= ss->isoc_mult << 11;
	hs_isoc_sink_desc.bInterval = ss->isoc_interval;

	hs_intr_source_desc.wMaxPacketSize = ss->intr_maxpacket;
	hs_intr_source_desc.bInterval = ss->intr_interval;
	hs_intr_sink_desc.wMaxPacketSize = ss->intr_maxpacket;
	hs_intr_sink_desc.bInterval = ss->intr_interval;

	hs_bulk_source_desc.wMaxPacketSize = (ss->bulk_maxpacket > 512) ?
			512 : ss->bulk_maxpacket;
	hs_bulk_sink_desc.wMaxPacketSize = (ss->bulk_maxpacket > 512) ?
			512 : ss->bulk_maxpacket;
}

static void ss_desc_update(struct f_sourcesink_ext *ss)
{
	ss_bulk_source_desc.bEndpointAddress = fs_bulk_source_desc.bEndpointAddress;
	ss_bulk_sink_desc.bEndpointAddress = fs_bulk_sink_desc.bEndpointAddress;
	ss_isoc_source_desc.bEndpointAddress = fs_isoc_source_desc.bEndpointAddress;
	ss_isoc_sink_desc.bEndpointAddress = fs_isoc_sink_desc.bEndpointAddress;
	ss_intr_source_desc.bEndpointAddress = fs_intr_source_desc.bEndpointAddress;
	ss_intr_sink_desc.bEndpointAddress = fs_intr_sink_desc.bEndpointAddress;

	/*
	 * Fill in the SS isoc descriptors from the module parameters.
	 * We assume that the user knows what they are doing and won't
	 * give parameters that their UDC doesn't support.
	 */
	ss_isoc_source_desc.wMaxPacketSize = ss->isoc_maxpacket;
	ss_isoc_source_desc.bInterval = ss->isoc_interval;
	ss_isoc_source_comp_desc.bmAttributes = ss->isoc_mult;
	ss_isoc_source_comp_desc.bMaxBurst = ss->isoc_maxburst;
	ss_isoc_source_comp_desc.wBytesPerInterval = ss->isoc_maxpacket *
		(ss->isoc_mult + 1) * (ss->isoc_maxburst + 1);

	ss_isoc_sink_desc.wMaxPacketSize = ss->isoc_maxpacket;
	ss_isoc_sink_desc.bInterval = ss->isoc_interval;
	ss_isoc_sink_comp_desc.bmAttributes = ss->isoc_mult;
	ss_isoc_sink_comp_desc.bMaxBurst = ss->isoc_maxburst;
	ss_isoc_sink_comp_desc.wBytesPerInterval = ss->isoc_maxpacket *
		(ss->isoc_mult + 1) * (ss->isoc_maxburst + 1);

	ss_intr_source_desc.wMaxPacketSize = ss->intr_maxpacket;
	ss_intr_source_desc.bInterval = ss->intr_interval;
	ss_intr_sink_desc.wMaxPacketSize = ss->intr_maxpacket;
	ss_intr_sink_desc.bInterval = ss->intr_interval;

	ss_bulk_source_desc.wMaxPacketSize = ss->bulk_maxpacket;
	ss_bulk_sink_desc.wMaxPacketSize = ss->bulk_maxpacket;
}

static void _re_config_isoc_bulk_intr_desc_fs(void)
{
	int i = 0;

	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&source_sink_intf_alt0;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&fs_bulk_sink_desc;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&fs_bulk_source_desc;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&source_sink_intf_alt1;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&fs_intr_sink_desc;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&fs_intr_source_desc;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&source_sink_intf_alt2;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&fs_isoc_sink_desc;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&fs_isoc_source_desc;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&source_sink_intf_alt3;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&fs_bulk_sink_desc;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&fs_bulk_source_desc;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&fs_intr_sink_desc;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&fs_intr_source_desc;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&fs_isoc_sink_desc;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&fs_isoc_source_desc;
	fs_source_sink_descs[i++] = NULL;
}

static void _re_config_isoc_bulk_intr_desc_hs(void)
{
	int i = 0;

	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&source_sink_intf_alt0;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&hs_bulk_sink_desc;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&hs_bulk_source_desc;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&source_sink_intf_alt1;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&hs_intr_sink_desc;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&hs_intr_source_desc;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&source_sink_intf_alt2;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&hs_isoc_sink_desc;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&hs_isoc_source_desc;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&source_sink_intf_alt3;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&hs_bulk_sink_desc;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&hs_bulk_source_desc;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&hs_intr_sink_desc;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&hs_intr_source_desc;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&hs_isoc_sink_desc;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&hs_isoc_source_desc;
	hs_source_sink_descs[i++] = NULL;
}

static void _re_config_isoc_bulk_intr_desc_ss(void)
{
	int i = 0;

	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&source_sink_intf_alt0;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_bulk_sink_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_bulk_sink_comp_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_bulk_source_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_bulk_source_comp_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&source_sink_intf_alt1;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_intr_sink_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_intr_sink_comp_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_intr_source_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_intr_source_comp_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&source_sink_intf_alt2;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_isoc_sink_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_isoc_sink_comp_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_isoc_source_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_isoc_source_comp_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&source_sink_intf_alt3;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_bulk_sink_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_bulk_sink_comp_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_bulk_source_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_bulk_source_comp_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_intr_sink_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_intr_sink_comp_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_intr_source_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_intr_source_comp_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_isoc_sink_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_isoc_sink_comp_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_isoc_source_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_isoc_source_comp_desc;
	ss_source_sink_descs[i++] = NULL;
}

static void re_config_isoc_bulk_intr_desc(void)
{
	_re_config_isoc_bulk_intr_desc_fs();
	_re_config_isoc_bulk_intr_desc_hs();
	_re_config_isoc_bulk_intr_desc_ss();

	fs_source_desc_p = &fs_bulk_source_desc;
	fs_sink_desc_p = &fs_bulk_sink_desc;
}

static int isoc_bulk_intr_mode_ep_config(struct usb_composite_dev *cdev,
					struct usb_function *f)
{
	struct f_sourcesink_ext *ss = func_to_ss(f);

	re_config_isoc_bulk_intr_desc();

	/* allocate bulk endpoints */
	ss->ep[SS_BULK_IN_EP_IDX] = usb_ep_autoconfig(cdev->gadget, &fs_bulk_source_desc);
	if (!ss->ep[SS_BULK_IN_EP_IDX]) {
autoconf_fail:
		pr_err("%s: can't autoconfigure on %s\n",
			f->name, cdev->gadget->name);
		return -ENODEV;
	}

	ss->ep[SS_BULK_OUT_EP_IDX] = usb_ep_autoconfig(cdev->gadget, &fs_bulk_sink_desc);
	if (!ss->ep[SS_BULK_OUT_EP_IDX])
		goto autoconf_fail;

	/* allocate intr endpoints */
	ss->ep[SS_INTR_IN_EP_IDX] = usb_ep_autoconfig(cdev->gadget, &fs_intr_source_desc);
	if (!ss->ep[SS_INTR_IN_EP_IDX])
		goto autoconf_fail;

	ss->ep[SS_INTR_OUT_EP_IDX] = usb_ep_autoconfig(cdev->gadget, &fs_intr_sink_desc);
	if (!ss->ep[SS_INTR_OUT_EP_IDX])
		goto autoconf_fail;

	/* allocate isoc endpoints */
	ss->ep[SS_ISOC_IN_EP_IDX] = usb_ep_autoconfig(cdev->gadget, &fs_isoc_source_desc);
	if (!ss->ep[SS_ISOC_IN_EP_IDX])
		goto autoconf_fail;

	ss->ep[SS_ISOC_OUT_EP_IDX] = usb_ep_autoconfig(cdev->gadget, &fs_isoc_sink_desc);
	if (!ss->ep[SS_ISOC_OUT_EP_IDX])
		goto autoconf_fail;

	/* fill in the FS isoc descriptors from the module parameters */
	fs_isoc_desc_update(ss);

	/* fill in the FS intr descriptors from the module parameters */
	fs_intr_desc_update(ss);

	/* fill in the FS bulk descriptors from the module parameters */
	fs_bulk_desc_update(ss);

	return 0;
}

static void re_config_bulk_only_desc(void)
{
	int i;

	/* re-config bulk only */
	i = 0;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&source_sink_intf_alt0;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&fs_bulk_sink_desc;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&fs_bulk_source_desc;
	fs_source_sink_descs[i++] = NULL;

	i = 0;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&source_sink_intf_alt0;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&hs_bulk_sink_desc;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&hs_bulk_source_desc;
	hs_source_sink_descs[i++] = NULL;

	i = 0;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&source_sink_intf_alt0;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_bulk_sink_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_bulk_sink_comp_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_bulk_source_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_bulk_source_comp_desc;
	ss_source_sink_descs[i++] = NULL;

	fs_source_desc_p = &fs_bulk_source_desc;
	fs_sink_desc_p = &fs_bulk_sink_desc;
}

static int bulk_only_mode_ep_config(struct usb_composite_dev *cdev,
					struct usb_function *f)
{
	struct f_sourcesink_ext *ss = func_to_ss(f);
	re_config_bulk_only_desc();

	ss->ep[SS_BULK_IN_EP_IDX] = usb_ep_autoconfig(cdev->gadget, fs_source_desc_p);
	if (!ss->ep[SS_BULK_IN_EP_IDX]) {
autoconf_fail:
		pr_err("%s: can't autoconfigure on %s\n",
			f->name, cdev->gadget->name);
		return -ENODEV;
	}

	ss->ep[SS_BULK_OUT_EP_IDX] = usb_ep_autoconfig(cdev->gadget, fs_sink_desc_p);
	if (!ss->ep[SS_BULK_OUT_EP_IDX])
		goto autoconf_fail;

	/* fill in the FS bulk descriptors from the module parameters */
	fs_bulk_desc_update(ss);

	return 0;
}

static void re_config_intr_only_desc(void)
{
	int i;

	/* re-config isoc only */
	i = 0;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&source_sink_intf_alt0;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&fs_intr_sink_desc;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&fs_intr_source_desc;
	fs_source_sink_descs[i++] = NULL;

	i = 0;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&source_sink_intf_alt0;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&hs_intr_sink_desc;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&hs_intr_source_desc;
	hs_source_sink_descs[i++] = NULL;

	i = 0;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&source_sink_intf_alt0;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_intr_sink_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_intr_sink_comp_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_intr_source_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_intr_source_comp_desc;
	ss_source_sink_descs[i++] = NULL;

	fs_source_desc_p = &fs_intr_source_desc;
	fs_sink_desc_p = &fs_intr_sink_desc;
}

static int intr_only_mode_ep_config(struct usb_composite_dev *cdev,
					struct usb_function *f)
{
	struct f_sourcesink_ext *ss = func_to_ss(f);

	re_config_intr_only_desc();

	ss->ep[SS_INTR_IN_EP_IDX] = usb_ep_autoconfig(cdev->gadget, fs_source_desc_p);
	if (!ss->ep[SS_INTR_IN_EP_IDX]) {
autoconf_fail:
		pr_err("%s: can't autoconfigure on %s\n",
			f->name, cdev->gadget->name);
		return -ENODEV;
	}

	ss->ep[SS_INTR_OUT_EP_IDX] = usb_ep_autoconfig(cdev->gadget, fs_sink_desc_p);
	if (!ss->ep[SS_INTR_OUT_EP_IDX])
		goto autoconf_fail;

	/* fill in the FS intr descriptors from the module parameters */
	fs_intr_desc_update(ss);

	return 0;
}

static void re_config_isoc_only_desc(void)
{
	int i;

	/* re-config isoc only */
	i = 0;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&source_sink_intf_alt0_for_isoc;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&source_sink_intf_alt1;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&fs_isoc_sink_desc;
	fs_source_sink_descs[i++] = (struct usb_descriptor_header *)&fs_isoc_source_desc;
	fs_source_sink_descs[i++] = NULL;

	i = 0;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&source_sink_intf_alt0_for_isoc;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&source_sink_intf_alt1;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&hs_isoc_sink_desc;
	hs_source_sink_descs[i++] = (struct usb_descriptor_header *)&hs_isoc_source_desc;
	hs_source_sink_descs[i++] = NULL;

	i = 0;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&source_sink_intf_alt0_for_isoc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&source_sink_intf_alt1;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_isoc_sink_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_isoc_sink_comp_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_isoc_source_desc;
	ss_source_sink_descs[i++] = (struct usb_descriptor_header *)&ss_isoc_source_comp_desc;
	ss_source_sink_descs[i++] = NULL;

	fs_source_desc_p = &fs_isoc_source_desc;
	fs_sink_desc_p = &fs_isoc_sink_desc;
}

static int isoc_only_mode_ep_config(struct usb_composite_dev *cdev,
					struct usb_function *f)
{
	struct f_sourcesink_ext	*ss = func_to_ss(f);

	re_config_isoc_only_desc();

	ss->ep[SS_ISOC_IN_EP_IDX] = usb_ep_autoconfig(cdev->gadget, fs_source_desc_p);
	if (!ss->ep[SS_ISOC_IN_EP_IDX]) {
autoconf_fail:
		pr_err("%s: can't autoconfigure on %s\n",
			f->name, cdev->gadget->name);
		return -ENODEV;
	}

	ss->ep[SS_ISOC_OUT_EP_IDX] = usb_ep_autoconfig(cdev->gadget, fs_sink_desc_p);
	if (!ss->ep[SS_ISOC_OUT_EP_IDX])
		goto autoconf_fail;

	/* fill in the FS isoc descriptors from the module parameters */
	fs_isoc_desc_update(ss);

	return 0;
}

static void ss_ext_free_req(struct f_sourcesink_ext *ss,
			int ep_idx, struct usb_request *req)
{
	kfree(req->buf);
	req->buf = NULL;
	usb_ep_free_request(ss->ep[ep_idx], req);
}

static int _ss_ext_alloc_req(struct f_sourcesink_ext *ss,
				unsigned int ep_idx,
				unsigned int qlen,
				unsigned int len)
{
	unsigned int i;

	for (i = 0; i < qlen; i++) {
		ss->reqs[ep_idx][i] = _ss_ext_alloc_ep_req(ss->ep[ep_idx], len);
		if (!ss->reqs[ep_idx][i])
			goto exit;
	}
	return 0;
exit:
	if (i) {
		for (i--; i >= 0; i--) {
			ss_ext_free_req(ss, ep_idx, ss->reqs[ep_idx][i]);
			ss->reqs[ep_idx][i] = NULL;
		}
	}
	return -ENOMEM;
}

static void ss_ext_free_reqs(struct f_sourcesink_ext *ss,
			int ep_idx, unsigned int qlen)
{
	unsigned int i;
	struct usb_request *req = NULL;

	for (i = 0; i < qlen; i++) {
		req = ss->reqs[ep_idx][i];
		if (req) {
			ss_ext_free_req(ss, ep_idx, req);
			ss->reqs[ep_idx][i] = NULL;
		}
	}
}

static int ss_ext_alloc_ep_reqs(struct f_sourcesink_ext *ss)
{
	int i, ret;
	unsigned int qlen[SS_EP_NUM] = {
		ss->bulk_qlen, ss->bulk_qlen,
		ss->intr_qlen, ss->intr_qlen,
		ss->isoc_qlen, ss->isoc_qlen
	};
	unsigned int buflen[SS_EP_NUM] = {
		ss->bulk_buflen, ss->bulk_buflen,
		ss->intr_buflen, ss->intr_buflen,
		ss->isoc_buflen, ss->isoc_buflen
	};

	for (i = 0; i < SS_EP_NUM; i++) {
		if (ss->ep[i]) {
			ret = _ss_ext_alloc_req(ss, i, qlen[i], buflen[i]);
			if (ret)
				goto exit;
		}
	}

	return 0;

exit:
	if (i) {
		for (i--; i >= 0; i--)
			ss_ext_free_reqs(ss, i, qlen[i]);
	}

	return ret;
}

static int __isoc_buflen(struct f_sourcesink_ext *ss, int speed)
{
	int size;

	switch (speed) {
	case USB_SPEED_SUPER:
		size = ss->isoc_maxpacket *
				(ss->isoc_mult + 1) *
				(ss->isoc_maxburst + 1);
		break;
	case USB_SPEED_HIGH:
		size = (int)(ss->isoc_maxpacket * (ss->isoc_mult + 1));
		break;
	default:
		size = (ss->isoc_maxpacket > 1023) ?
				1023 : (int)(ss->isoc_maxpacket);
		break;
	}

	return size;
}

static int ss_ext_bind(struct usb_configuration *c,
				struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct f_sourcesink_ext	*ss = func_to_ss(f);
	int	id;
	int ret;

	/* allocate interface ID(s) */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;

	source_sink_intf_alt0.bInterfaceNumber = (__u8)id;
	source_sink_intf_alt0_for_isoc.bInterfaceNumber = (__u8)id;
	source_sink_intf_alt1.bInterfaceNumber = (__u8)id;
	source_sink_intf_alt2.bInterfaceNumber = (__u8)id;
	source_sink_intf_alt3.bInterfaceNumber = (__u8)id;

	/* sanity check the isoc module parameters */
	isoc_para_sanity_check(ss);

	ss->isoc_buflen = ss->isoc_maxpacket *
			(ss->isoc_mult + 1) *
			(ss->isoc_maxburst + 1);

	switch (ss->test_mode) {
	case SS_TEST_MODE_ISOC_BULK_INTR:
		ret = isoc_bulk_intr_mode_ep_config(cdev, f);
		break;
	case SS_TEST_MODE_BULK_ONLY:
		ret = bulk_only_mode_ep_config(cdev, f);
		break;
	case SS_TEST_MODE_ISOC_ONLY:
		ret = isoc_only_mode_ep_config(cdev, f);
		break;
	case SS_TEST_MODE_INTR_ONLY:
		ret = intr_only_mode_ep_config(cdev, f);
		break;
	default:
		return -EINVAL;
	}

	if (ret)
		return ret;

	/* support high speed hardware */
	hs_desc_update(ss);

	/* support super speed hardware */
	ss_desc_update(ss);

	ret = usb_assign_descriptors(f, fs_source_sink_descs,
			hs_source_sink_descs, ss_source_sink_descs, NULL);
	if (ret)
		return ret;

	ret = ss_ext_alloc_ep_reqs(ss);
	if (ret)
		return ret;

	INIT_WORK(&ss->handler_work, handler_work);

	pr_err("%s speed %s: BULK-IN/%s, BULK-OUT/%s, INTR-IN/%s, INTR-OUT/%s, "
		"ISOC-IN/%s, ISOC-OUT/%s\n",
	    (gadget_is_superspeed(c->cdev->gadget) ? "super" :
	     (gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full")),
		f->name,
		ss->ep[SS_BULK_IN_EP_IDX] ? ss->ep[SS_BULK_IN_EP_IDX]->name : "<none>",
		ss->ep[SS_BULK_OUT_EP_IDX] ? ss->ep[SS_BULK_OUT_EP_IDX]->name : "<none>",
		ss->ep[SS_INTR_IN_EP_IDX] ? ss->ep[SS_INTR_IN_EP_IDX]->name : "<none>",
		ss->ep[SS_INTR_OUT_EP_IDX] ? ss->ep[SS_INTR_OUT_EP_IDX]->name : "<none>",
		ss->ep[SS_ISOC_IN_EP_IDX] ? ss->ep[SS_ISOC_IN_EP_IDX]->name : "<none>",
		ss->ep[SS_ISOC_OUT_EP_IDX] ? ss->ep[SS_ISOC_OUT_EP_IDX]->name : "<none>");

	return 0;
}

static void ss_ext_unbind(struct usb_configuration *c,
				struct usb_function *f)
{
	struct f_sourcesink_ext	*ss = func_to_ss(f);
	int i;

	cancel_work_sync(&ss->handler_work);
	for (i = 0; i < SS_EP_NUM; i++)
		ss_ext_free_reqs(ss, i, SS_MAX_REQ_NUM);
}

static void sourcesink_ext_free_func(struct usb_function *f)
{
	struct f_ss_ext_opts *opts = NULL;
	opts = container_of(f->fi, struct f_ss_ext_opts, func_inst);

	mutex_lock(&opts->lock);
	opts->refcnt--;
	mutex_unlock(&opts->lock);

	usb_free_all_descriptors(f);
	kfree(func_to_ss(f));
}

/* optionally require specific source/sink data patterns  */
static int check_read_data(struct f_sourcesink_ext *ss, int ep_idx,
					struct usb_request *req)
{
	unsigned		i;
	u8 *buf = req->buf;

	int mps = usb_endpoint_maxp(ss->ep[ep_idx]->desc);

	if (ss->pattern > 1)
		return 0;

	if (usb_endpoint_dir_in(ss->ep[ep_idx]->desc))
		return 0;

	if (mps == 0)
		return 0;

	for (i = 0; i < req->actual; i++, buf++) {
		switch (ss->pattern) {
		/* all-zeroes has no synchronization issues */
		case 0:
			if (*buf == 0)
				continue;
			break;

		/* "mod63" stays in sync with short-terminated transfers,
		 * OR otherwise when host and gadget agree on how large
		 * each usb transfer request should be.  Resync is done
		 * with set_interface or set_config.  (We *WANT* it to
		 * get quickly out of sync if controllers or their drivers
		 * stutter for any reason, including buffer duplication...)
		 */
		case 1:
			if (*buf == (u8)((i % (unsigned int)mps) % 63))
				continue;
			break;

		default:
			break;
		}
		pr_err("bad OUT byte, buf[%d] = %d\n", i, *buf);
		ss->stat[ep_idx].data_check_err_count++;
		usb_ep_set_halt(ss->ep[ep_idx]);
		return -EINVAL;
	}
	return 0;
}

static void reinit_write_data(struct usb_ep *ep, struct usb_request *req)
{
	unsigned	i;
	u8 *buf = req->buf;
	int mps = usb_endpoint_maxp(ep->desc);
	struct f_sourcesink_ext *ss = ep->driver_data;

	switch (ss->pattern) {
	case 0:
		memset(req->buf, 0, req->length);
		break;
	case 1:
		if (mps == 0)
			break;

		for  (i = 0; i < req->length; i++)
			*buf++ = (u8) ((i % (unsigned int)mps) % 63);
		break;
	default:
		break;
	}
}

static int ss_ext_get_ep_idx(const struct f_sourcesink_ext *ss,
			const struct usb_ep *ep)
{
	int i;
	for (i = 0; i < SS_EP_NUM; i++)
		if (ss->ep[i] == ep)
			return i;
	return -1;
}

static void ss_ext_clear_dequeue_req(struct f_sourcesink_ext *ss,
					u16 epidx, struct usb_request *req)
{
	int i;
	for (i = 0; i < SS_MAX_DEQUEUE_NUM; i++) {
		if (ss->dequeue_reqs[epidx][i] == req) {
			ss->dequeue_reqs[epidx][i] = NULL;
			break;
		}
	}
}

static void ss_ext_req_buflen_update(struct f_sourcesink_ext *ss,
				int ep_idx, struct usb_request *req)
{
	unsigned int len;
	if (ep_idx == SS_BULK_IN_EP_IDX
			|| ep_idx == SS_BULK_OUT_EP_IDX)
		len = ss->bulk_buflen;
	else if (ep_idx == SS_INTR_IN_EP_IDX
			|| ep_idx == SS_INTR_OUT_EP_IDX)
		len = ss->intr_buflen;
	else
		len = ss->isoc_buflen;

	if (req->length != len)
		req->length = len;
}

static void ss_ext_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct usb_composite_dev *cdev = NULL;
	struct f_sourcesink_ext *ss = ep->driver_data;
	int				status = req->status;
	int ep_idx;

	/* driver_data will be null if ep has been disabled */
	if (!ss)
		return;

	ep_idx = ss_ext_get_ep_idx(ss, ep);
	if (ep_idx == -1) {
		pr_err("idx err\n");
		return;
	}

	if (status != 0)
		ss->stat[ep_idx].status_err_count++;

	cdev = ss->function.config->cdev;

	switch (status) {
	case 0: /* normal completion? */

		check_read_data(ss, ep_idx, req);

		ss->stat[ep_idx].reqs_count++;
		ss->stat[ep_idx].data_len +=
					(unsigned long)req->actual;

		if (ep_idx == SS_ISOC_IN_EP_IDX
				|| ep_idx == SS_ISOC_OUT_EP_IDX) {
			ss->stat[ep_idx].isoc_reqs_count_since_err++;
			if (req->length != req->actual) {
				ss->stat[ep_idx].isoc_reqs_count_since_err = 0;
				ss->stat[ep_idx].isoc_err_reqs_count++;
			}
		}

		break;

	/* this endpoint is normally active while we're configured */
	case -ECONNABORTED: /* hardware forced ep reset */
	case -ECONNRESET: /* request dequeued */
	case -ESHUTDOWN: /* disconnect from host */
		pr_err("%s gone %d, %d/%d\n", ep->name, status,
				req->actual, req->length);

		ss_ext_clear_dequeue_req(ss, ep_idx, req);
		return;

	case -EOVERFLOW: /* buffer overrun on read means that
					 * we didn't provide a big enough
					 * buffer.
					 */
	default:
		pr_debug("%s complete --> %d, %d/%d\n", ep->name,
				status, req->actual, req->length);

	case -EREMOTEIO: /* short read */
		break;
	}

	ss_ext_req_buflen_update(ss, ep_idx, req);
	status = usb_ep_queue(ep, req, GFP_ATOMIC);
	if (status) {
		ERROR(cdev, "kill %s:  resubmit %d bytes --> %d\n",
				ep->name, req->length, status);
		usb_ep_set_halt(ep);
	}
}

static int __ss_ext_start_ep(struct f_sourcesink_ext *ss,
	struct usb_ep *ep, int idx, bool is_in, int qlen, unsigned int buflen)
{
	struct usb_request *req = NULL;
	int i;
	int status = 0;

	for (i = 0; i < qlen; i++) {
		req = ss->reqs[idx][i];
		req->complete = ss_ext_complete;
		req->length = buflen;

		if (is_in)
			reinit_write_data(ep, req);

		status = usb_ep_queue(ep, req, GFP_ATOMIC);
		if (status) {
			struct usb_composite_dev *cdev;

			cdev = ss->function.config->cdev;
			pr_err("start %s --> %d\n", ep->name, status);
			return status;
		}
		ss->dequeue_reqs[idx][i % SS_MAX_DEQUEUE_NUM] = req;
	}

	return status;
}

static void _ss_ext_disable(struct f_sourcesink_ext *ss)
{
	int i;

	for (i = 0; i < SS_EP_NUM; i++) {
		ss_ext_disable_endpoint(ss->ep[i]);
		memset(&ss->stat[i], 0, sizeof(ss->stat[i]));
	}
	pr_err("%s disabled\n", ss->function.name);
}

static int ss_ext_common_start_ep(struct usb_composite_dev *cdev,
			struct f_sourcesink_ext *ss,
			struct usb_ep *ep, int idx, bool is_enable)
{
	int					result;
	bool					is_in = false;
	unsigned				qlen;
	unsigned				buflen;

	if (is_enable) {
		result = config_ep_by_speed(cdev->gadget, &(ss->function), ep);
		if (result) {
			pr_err("config ep fail %d\n", result);
			return result;
		}
	}

	is_in = !!usb_endpoint_dir_in(ep->desc);

	if (usb_endpoint_xfer_isoc(ep->desc)) {
		qlen = ss->isoc_qlen;
		buflen = ss->isoc_buflen;
	} else if (usb_endpoint_xfer_int(ep->desc)) {
		qlen = ss->intr_qlen;
		buflen = ss->intr_buflen;
	} else {
		qlen = ss->bulk_qlen;
		buflen = ss->bulk_buflen;
	}

	pr_info("%s is_in %d, qlen %u, buflen %u\n", ep->name,
			is_in, qlen, buflen);

	if (is_enable) {
		result = usb_ep_enable(ep);
		if (result) {
			pr_err("ep enable fail %d\n", result);
			return result;
		}
	}

	ep->driver_data = ss;

	return __ss_ext_start_ep(ss, ep, idx, is_in, qlen, buflen);
}

static int ss_ext_start_bulk_ep(struct usb_composite_dev *cdev,
					struct f_sourcesink_ext *ss)
{
	struct usb_ep *ep = NULL;
	int result = 0;

	/* one bulk endpoint writes (sources) zeroes IN (to the host) */
	ep = ss->ep[SS_BULK_IN_EP_IDX];
	result = config_ep_by_speed(cdev->gadget, &(ss->function), ep);
	if (result)
		return result;

	result = usb_ep_enable(ep);
	if (result)
		return result;

	ep->driver_data = ss;
	memset(&ss->stat[SS_BULK_IN_EP_IDX], 0,
		sizeof(ss->stat[SS_BULK_IN_EP_IDX]));

	result = __ss_ext_start_ep(ss, ep, SS_BULK_IN_EP_IDX, true,
			ss->bulk_qlen, ss->bulk_buflen);
	if (result)
		goto exit_disable_in;

	ep = ss->ep[SS_BULK_OUT_EP_IDX];
	result = config_ep_by_speed(cdev->gadget, &(ss->function), ep);
	if (result)
		goto exit_disable_in;

	result = usb_ep_enable(ep);
	if (result)
		goto exit_disable_in;

	ep->driver_data = ss;

	memset(&ss->stat[SS_BULK_OUT_EP_IDX], 0,
		sizeof(ss->stat[SS_BULK_OUT_EP_IDX]));
	result = __ss_ext_start_ep(ss, ep, SS_BULK_OUT_EP_IDX, false,
			ss->bulk_qlen, ss->bulk_buflen);
	if (result)
		goto exit_disable_out;

	return result;

exit_disable_out:
	usb_ep_disable(ss->ep[SS_BULK_OUT_EP_IDX]);

exit_disable_in:
	usb_ep_disable(ss->ep[SS_BULK_IN_EP_IDX]);
	return result;
}

static int ss_ext_start_isoc_ep(struct usb_composite_dev *cdev,
					struct f_sourcesink_ext *ss)
{
	struct usb_ep *ep = NULL;
	int result = 0;

	/* one iso endpoint writes (sources) zeroes IN (to the host) */
	ep = ss->ep[SS_ISOC_IN_EP_IDX];
	result = config_ep_by_speed(cdev->gadget, &(ss->function), ep);
	if (result)
		return result;

	result = usb_ep_enable(ep);
	if (result)
		return result;

	ep->driver_data = ss;
	memset(&ss->stat[SS_ISOC_IN_EP_IDX], 0,
		sizeof(ss->stat[SS_ISOC_IN_EP_IDX]));

	result  = __ss_ext_start_ep(ss, ep, SS_ISOC_IN_EP_IDX, true,
			ss->isoc_qlen, __isoc_buflen(ss, cdev->gadget->speed));
	if (result)
		goto exit_disable_in;

	ep = ss->ep[SS_ISOC_OUT_EP_IDX];
	result = config_ep_by_speed(cdev->gadget, &(ss->function), ep);
	if (result)
		goto exit_disable_in;

	result = usb_ep_enable(ep);
	if (result)
		goto exit_disable_in;

	ep->driver_data = ss;
	memset(&ss->stat[SS_ISOC_OUT_EP_IDX], 0,
		sizeof(ss->stat[SS_ISOC_OUT_EP_IDX]));

	result  = __ss_ext_start_ep(ss, ep, SS_ISOC_OUT_EP_IDX, false,
			ss->isoc_qlen, __isoc_buflen(ss, cdev->gadget->speed));
	if (result)
		goto exit_disable_out;

	ss->isoc_buflen = (unsigned int)__isoc_buflen(ss, cdev->gadget->speed);
	return result;

exit_disable_out:
	usb_ep_disable(ss->ep[SS_ISOC_OUT_EP_IDX]);

exit_disable_in:
	usb_ep_disable(ss->ep[SS_ISOC_IN_EP_IDX]);
	return result;
}

static int ss_ext_start_intr_ep(struct usb_composite_dev *cdev,
					struct f_sourcesink_ext *ss)
{
	struct usb_ep *ep = NULL;
	int result;

	/* one endpoint writes (sources) data IN (to the host) */
	ep = ss->ep[SS_INTR_IN_EP_IDX];
	result = config_ep_by_speed(cdev->gadget, &(ss->function), ep);
	if (result)
		return result;

	result = usb_ep_enable(ep);
	if (result)
		return result;

	ep->driver_data = ss;
	memset(&ss->stat[SS_INTR_IN_EP_IDX], 0,
		sizeof(ss->stat[SS_INTR_IN_EP_IDX]));

	result = __ss_ext_start_ep(ss, ep, SS_INTR_IN_EP_IDX, true,
			ss->intr_qlen, ss->intr_buflen);
	if (result)
		goto exit_disable_in;

	ep = ss->ep[SS_INTR_OUT_EP_IDX];
	result = config_ep_by_speed(cdev->gadget, &(ss->function), ep);
	if (result)
		goto exit_disable_in;

	result = usb_ep_enable(ep);
	if (result)
		goto exit_disable_in;

	ep->driver_data = ss;
	memset(&ss->stat[SS_INTR_OUT_EP_IDX], 0,
		sizeof(ss->stat[SS_INTR_OUT_EP_IDX]));

	result = __ss_ext_start_ep(ss, ep, SS_INTR_OUT_EP_IDX, false,
			ss->intr_qlen, ss->intr_buflen);
	if (result)
		goto exit_disable_out;

	return result;

exit_disable_out:
	usb_ep_disable(ss->ep[SS_INTR_OUT_EP_IDX]);

exit_disable_in:
	usb_ep_disable(ss->ep[SS_INTR_IN_EP_IDX]);
	return result;
}

static int ss_ext_start_isoc_bulk_intr_ep(
					struct usb_composite_dev *cdev,
					struct f_sourcesink_ext *ss, int alt)
{
	int					result = 0;

	if (alt == 0 || alt == 3) {
		result = ss_ext_start_bulk_ep(cdev, ss);
		if (result)
			return result;

		if (alt == 0)
			return result;
	}

	if (alt == 1 || alt == 3) {
		result = ss_ext_start_intr_ep(cdev, ss);
		if (result)
			goto exit_start_intr;

		if (alt == 1)
			return result;
	}

	if (alt == 2 || alt == 3) {
		result = ss_ext_start_isoc_ep(cdev, ss);
		if (result)
			goto exit_start_isoc;

		if (alt == 2)
			return result;
	}
	return result;

exit_start_isoc:
	if (alt == 3) {
		usb_ep_disable(ss->ep[SS_INTR_IN_EP_IDX]);
		usb_ep_disable(ss->ep[SS_INTR_OUT_EP_IDX]);
	}

exit_start_intr:
	if (alt == 3) {
		usb_ep_disable(ss->ep[SS_BULK_IN_EP_IDX]);
		usb_ep_disable(ss->ep[SS_BULK_OUT_EP_IDX]);
	}
	return result;
}

static int ss_ext_enable(struct usb_composite_dev *cdev,
			struct f_sourcesink_ext *ss, int alt)
{
	int	result;

	switch (ss->test_mode) {
	case SS_TEST_MODE_BULK_ONLY:
		result = ss_ext_start_bulk_ep(cdev, ss);
		break;

	case SS_TEST_MODE_INTR_ONLY:
		result = ss_ext_start_intr_ep(cdev, ss);
		break;

	case SS_TEST_MODE_ISOC_ONLY:
		result = ss_ext_start_isoc_ep(cdev, ss);
		break;

	case SS_TEST_MODE_ISOC_BULK_INTR:
		result = ss_ext_start_isoc_bulk_intr_ep(cdev, ss, alt);
		break;

	default:
		result = -EINVAL;
		break;
	}

	ss->cur_alt = alt;

	pr_info("%s enabled, alt intf %d\n", ss->function.name, alt);
	return result;
}

static int ss_ext_set_alt(struct usb_function *f,
		unsigned intf, unsigned alt)
{
	struct f_sourcesink_ext *ss = func_to_ss(f);
	struct usb_composite_dev *cdev = f->config->cdev;

	pr_info("function %s, test_mode %u, alt %u\n",
			f->name, ss->test_mode, alt);

	_ss_ext_disable(ss);
	return ss_ext_enable(cdev, ss, alt);
}

static int ss_ext_get_alt(struct usb_function *f, unsigned intf)
{
	struct f_sourcesink_ext *ss = func_to_ss(f);

	return ss->cur_alt;
}

static void ss_ext_disable(struct usb_function *f)
{
	struct f_sourcesink_ext *ss = func_to_ss(f);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	cancel_work_sync(&ss->handler_work);
#else
	cancel_work(&ss->handler_work);
#endif
	_ss_ext_disable(ss);
}

/*-------------------------------------------------------------------------*/

#define SS_CONTROL_WRITE 0x5b
#define SS_CONTROL_READ 0x5c
#define SS_CONTROL_WRITE_BABBLE 0x5d
#define SS_CONTROL_READ_BABBLE 0x5e

#define SS_GET_STATISTICS 0x50
#define SS_RESET_STATISTICS 0x51
#define SS_GET_BUFLEN 0x52
#define SS_SET_PATTERN 0x53
#define SS_SET_BUFLEN 0x54

#define SS_STOP_EP 0x41
#define SS_START_EP 0x42
#define SS_DEQUEUE_REQ 0x43
#define SS_QUEUE_REQ 0x44

static int _setup_ctrl_handle(const struct usb_ctrlrequest *ctrl,
					struct usb_request *req, int r)
{
	u16 w_length = le16_to_cpu(ctrl->wLength);
	u8 requesttype;

	if (r)
		requesttype = USB_DIR_OUT |  USB_TYPE_VENDOR;
	else
		requesttype = USB_DIR_IN | USB_TYPE_VENDOR;

	if (ctrl->bRequestType != requesttype)
		return -EOPNOTSUPP;
	/* just read that many bytes into the buffer */
	if (w_length > req->length)
		return -EOPNOTSUPP;

	return w_length;
}

static int _setup_get_stat_handle(const struct f_sourcesink_ext *ss,
					const struct usb_ctrlrequest *ctrl,
					struct usb_request *req)
{
	u16 w_length = le16_to_cpu(ctrl->wLength);
	u16 w_value = le16_to_cpu(ctrl->wValue);
	int value;

	if (ctrl->bRequestType != (USB_RECIP_INTERFACE
			| USB_DIR_IN | USB_TYPE_VENDOR))
		return -EOPNOTSUPP;

	if (w_length > req->length)
		return -EOPNOTSUPP;

	if (w_value >= SS_EP_NUM)
		return -EOPNOTSUPP;

	value = (int)min_t(size_t, w_length, sizeof(ss->stat[w_value]));
	memcpy(req->buf, &ss->stat[w_value], value);

	return value;
}

static int _setup_reset_stat_handle(struct f_sourcesink_ext *ss,
					const struct usb_ctrlrequest *ctrl)
{
	u16 w_length = le16_to_cpu(ctrl->wLength);
	u16 w_value = le16_to_cpu(ctrl->wValue);

	if (w_length != 0)
		return -EOPNOTSUPP;

	if (w_value >= SS_EP_NUM)
		return -EOPNOTSUPP;

	memset(&ss->stat[w_value], 0, sizeof(ss->stat[w_value]));

	return 0;
}

static int _setup_set_pattern_handle(struct f_sourcesink_ext *ss,
					const struct usb_ctrlrequest *ctrl)
{
	u16 w_length = le16_to_cpu(ctrl->wLength);
	u16 w_value = le16_to_cpu(ctrl->wValue);

	if (w_length != 0)
		return -EOPNOTSUPP;

	ss->pattern = w_value;

	return 0;
}

/* 8 is less than all maxpacket size, fs hs ss */
#define BABBLE_TEST_BUF_LEN 8
static int _setup_set_buflen_handle(struct f_sourcesink_ext *ss,
					const struct usb_ctrlrequest *ctrl,
					struct usb_request *req)
{
	u16 w_length = le16_to_cpu(ctrl->wLength);
	u16 w_value = le16_to_cpu(ctrl->wValue);

	if (ctrl->bRequestType != (USB_RECIP_INTERFACE
			| USB_DIR_OUT | USB_TYPE_VENDOR))
		return -EOPNOTSUPP;

	if (w_length != 0)
		return -EOPNOTSUPP;

	if (w_value >= SS_EP_NUM)
		return -EOPNOTSUPP;

	if (w_value == SS_BULK_IN_EP_IDX
			|| w_value == SS_BULK_OUT_EP_IDX)
		ss->bulk_buflen = BABBLE_TEST_BUF_LEN;
	else if (w_value == SS_INTR_IN_EP_IDX
			|| w_value == SS_INTR_OUT_EP_IDX)
		ss->intr_buflen = BABBLE_TEST_BUF_LEN;
	else
		ss->isoc_buflen = BABBLE_TEST_BUF_LEN;

	return 0;
}

static int _setup_get_buflen_handle(const struct f_sourcesink_ext *ss,
					const struct usb_ctrlrequest *ctrl,
					struct usb_request *req)
{
	u16 w_length = le16_to_cpu(ctrl->wLength);
	u16 w_value = le16_to_cpu(ctrl->wValue);
	int value;
	uint32_t buflen;

	if (ctrl->bRequestType != (USB_RECIP_INTERFACE
			| USB_DIR_IN | USB_TYPE_VENDOR))
		return -EOPNOTSUPP;

	if (w_length > req->length)
		return -EOPNOTSUPP;

	if (w_value >= SS_EP_NUM)
		return -EOPNOTSUPP;

	if (w_value == SS_BULK_IN_EP_IDX
			|| w_value == SS_BULK_OUT_EP_IDX)
		buflen = ss->bulk_buflen;
	else if (w_value == SS_INTR_IN_EP_IDX
			|| w_value == SS_INTR_OUT_EP_IDX)
		buflen = ss->intr_buflen;
	else
		return -EOPNOTSUPP;

	value = (int)min_t(size_t, w_length, sizeof(buflen));
	memcpy(req->buf, &buflen, value);

	return value;
}

static int _setup_common_cmd_handle(struct f_sourcesink_ext *ss,
			const struct usb_ctrlrequest *ctrl)
{
	u16 w_length = le16_to_cpu(ctrl->wLength);
	u16 w_value = le16_to_cpu(ctrl->wValue);

	if (w_length != 0)
		return -EOPNOTSUPP;

	if (w_value >= SS_EP_NUM)
		return -EOPNOTSUPP;

	ss->ep_idx = w_value;
	ss->brequest = ctrl->bRequest;

	if (!queue_work(system_power_efficient_wq, &ss->handler_work))
		return -EBUSY;

	return 0;
}

static void handler_work(struct work_struct *work)
{
	int i;
	struct f_sourcesink_ext *ss = container_of(work,
				    struct f_sourcesink_ext, handler_work);
	struct usb_configuration *c = ss->function.config;
	struct usb_composite_dev *cdev = c->cdev;

	switch (ss->brequest) {
	case SS_STOP_EP:
		pr_info("SS_STOP_EP\n");
		ss_ext_disable_endpoint(ss->ep[ss->ep_idx]);
		break;

	case SS_START_EP:
		pr_info("SS_START_EP\n");
		ss_ext_common_start_ep(cdev, ss,
				ss->ep[ss->ep_idx], ss->ep_idx, true);
		break;

	case SS_DEQUEUE_REQ:
		pr_info("SS_DEQUEUE_REQ\n");
		for (i = 0; i < SS_MAX_DEQUEUE_NUM; i++) {
			if (ss->dequeue_reqs[ss->ep_idx][i])
				usb_ep_dequeue(ss->ep[ss->ep_idx],
					ss->dequeue_reqs[ss->ep_idx][i]);
		}
		break;

	case SS_QUEUE_REQ:
		pr_info("SS_QUEUE_REQ\n");
		ss_ext_common_start_ep(cdev, ss,
				ss->ep[ss->ep_idx], ss->ep_idx, false);
		break;
	default:
		break;
	}
}

static int ss_ext_setup(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
	struct usb_configuration *c = f->config;
	struct f_sourcesink_ext *ss = func_to_ss(f);
	struct usb_request	*req = c->cdev->req;
	int			value = -EOPNOTSUPP;
	u16			w_index = le16_to_cpu(ctrl->wIndex);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u16			w_length = le16_to_cpu(ctrl->wLength);

	req->length = USB_COMP_EP0_BUFSIZ;

	/* multi interface */
	if ((ctrl->bRequestType & USB_RECIP_MASK) == USB_RECIP_INTERFACE) {
		if (w_index <  MAX_CONFIG_INTERFACES) {
			if (c->interface[w_index] != f)
				return value;
		}
	}

	/* composite driver infrastructure handles everything except
	 * the two control test requests.
	 */
	switch (ctrl->bRequest) {
	/*
	 * These are the same vendor-specific requests supported by
	 * Intel's USB 2.0 compliance test devices.  We exceed that
	 * device spec by allowing multiple-packet requests.
	 *
	 * NOTE:  the Control-OUT data stays in req->buf ... better
	 * would be copying it into a scratch buffer, so that other
	 * requests may safely intervene.
	 */
	case SS_CONTROL_WRITE: /* control WRITE test -- fill the buffer */
		value = _setup_ctrl_handle(ctrl, req, 1);
		break;

	case SS_CONTROL_READ: /* control READ test -- return the buffer */
		value = _setup_ctrl_handle(ctrl, req, 0);
		break;

	case SS_CONTROL_WRITE_BABBLE: /* control WRITE babble test -- fill the buffer */
		value = _setup_ctrl_handle(ctrl, req, 1);
		if (value)
			value--;
		break;

	case SS_CONTROL_READ_BABBLE: /* control READ babble test -- return the buffer */
		value = _setup_ctrl_handle(ctrl, req, 0);
		if (value < USB_COMP_EP0_BUFSIZ)
			value++;
		break;

	case SS_GET_STATISTICS:
		pr_info("%s: SS_GET_STATISTICS\n", __func__);
		value = _setup_get_stat_handle(ss, ctrl, req);
		break;

	case SS_RESET_STATISTICS:
		pr_info("%s: SS_RESET_STATISTICS\n", __func__);
		value = _setup_reset_stat_handle(ss, ctrl);
		break;

	case SS_SET_PATTERN:
		pr_info("%s: SS_SET_PATTERN\n", __func__);
		value = _setup_set_pattern_handle(ss, ctrl);
		break;

	case SS_GET_BUFLEN:
		pr_info("%s: SS_GET_BUFLEN\n", __func__);
		value = _setup_get_buflen_handle(ss, ctrl, req);
		break;

	case SS_SET_BUFLEN:
		pr_info("%s: SS_SET_BUFLEN\n", __func__);
		value = _setup_set_buflen_handle(ss, ctrl, req);
		break;

	case SS_STOP_EP:
	case SS_START_EP:
	case SS_DEQUEUE_REQ:
	case SS_QUEUE_REQ:
		value = _setup_common_cmd_handle(ss, ctrl);
		break;

	default:
		pr_info(
			"unknown control req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
	}

	/* respond with data transfer or status phase? */
	if (value >= 0) {
		pr_debug("source/sink req%x %x v%x i%x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
		req->zero = 0;
		req->length = (unsigned int)value;
		value = usb_ep_queue(c->cdev->gadget->ep0, req, GFP_ATOMIC);
		if (value < 0)
			pr_err("source/sink response, err %d\n", value);
	}

	/* device either stalls (value < 0) or reports success */
	return value;
}

static struct usb_function *ss_ext_alloc_func(
		struct usb_function_instance *fi)
{
	struct f_sourcesink_ext *ss = NULL;
	struct f_ss_ext_opts *ss_opts = NULL;

	ss = kzalloc(sizeof(*ss), GFP_KERNEL);
	if (!ss)
		return ERR_PTR(-ENOMEM);

	ss_opts =  container_of(fi, struct f_ss_ext_opts, func_inst);

	mutex_lock(&ss_opts->lock);
	ss_opts->refcnt++;
	mutex_unlock(&ss_opts->lock);

	ss->pattern = ss_opts->pattern;
	ss->isoc_interval = ss_opts->isoc_interval;
	ss->isoc_maxpacket = ss_opts->isoc_maxpacket;
	ss->isoc_mult = ss_opts->isoc_mult;
	ss->isoc_maxburst = ss_opts->isoc_maxburst;
	ss->isoc_qlen = ss_opts->isoc_qlen;

	ss->bulk_buflen = ss_opts->bulk_buflen;
	ss->bulk_qlen = ss_opts->bulk_qlen;
	ss->bulk_maxpacket = ss_opts->bulk_maxpacket;

	ss->intr_interval = ss_opts->intr_interval;
	ss->intr_maxpacket = ss_opts->intr_maxpacket;
	ss->intr_buflen = ss_opts->intr_buflen;
	ss->intr_qlen = ss_opts->intr_qlen;

	ss->test_mode = ss_opts->test_mode;
	ss->func_num = ss_opts->inst_num;
	pr_info("func_num %d , test_mode %d\n",
			ss->func_num, ss->test_mode);

	ss->function.name = "source/sink/ext";
	ss->function.bind = ss_ext_bind;
	ss->function.unbind = ss_ext_unbind;
	ss->function.set_alt = ss_ext_set_alt;
	ss->function.get_alt = ss_ext_get_alt;
	ss->function.disable = ss_ext_disable;
	ss->function.setup = ss_ext_setup;
	ss->function.strings = sourcesink_strings;

	ss->function.free_func = sourcesink_ext_free_func;

	return &ss->function;
}

static inline struct f_ss_ext_opts *to_f_ss_ext_opts(struct config_item *item)
{
	struct config_group *g = to_config_group(item);
	return container_of(g, struct f_ss_ext_opts, func_inst.group);
}

static void ss_attr_release(struct config_item *item)
{
	struct f_ss_ext_opts *ss_opts = to_f_ss_ext_opts(item);

	usb_put_function_instance(&ss_opts->func_inst);
}

static struct configfs_item_operations ss_item_ops = {
	.release		= ss_attr_release,
};

static ssize_t f_ss_ext_opts_show(struct config_item *item,
					char *page, u32 target)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);
	int result;

	mutex_lock(&opts->lock);
	result = sprintf_s(page, PAGE_SIZE, "%u\n", target);
	if (result == -1)
		result = -ENOMEM;
	mutex_unlock(&opts->lock);

	return result;
}

static ssize_t f_ss_ext_opts_store(struct config_item *item,
					 const char *page, size_t len,
					 u32 *ptarget)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);
	int ret;
	u32 num;

	mutex_lock(&opts->lock);
	if (opts->refcnt) {
		ret = -EBUSY;
		goto end;
	}

	ret = kstrtou32(page, 0, &num);
	if (ret)
		goto end;

	*ptarget = num;
	ret = (ssize_t)len;

end:
	mutex_unlock(&opts->lock);
	return ret;
}

static ssize_t f_ss_ext_opts_pattern_show(
					struct config_item *item,
					char *page)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_show(item, page, opts->pattern);
}

static ssize_t f_ss_ext_opts_pattern_store(
					struct config_item *item,
					 const char *page, size_t len)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_store(item, page, len, &opts->pattern);
}
CONFIGFS_ATTR(f_ss_ext_opts_, pattern);

static ssize_t f_ss_ext_opts_isoc_interval_show(
					struct config_item *item,
					char *page)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_show(item, page, opts->isoc_interval);
}

static ssize_t f_ss_ext_opts_isoc_interval_store(
					struct config_item *item,
					 const char *page, size_t len)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_store(item, page, len, &opts->isoc_interval);
}
CONFIGFS_ATTR(f_ss_ext_opts_, isoc_interval);

static ssize_t f_ss_ext_opts_isoc_maxpacket_show(
					struct config_item *item,
					char *page)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_show(item, page, opts->isoc_maxpacket);
}

static ssize_t f_ss_ext_opts_isoc_maxpacket_store(
					struct config_item *item,
					 const char *page, size_t len)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_store(item, page, len, &opts->isoc_maxpacket);
}
CONFIGFS_ATTR(f_ss_ext_opts_, isoc_maxpacket);

static ssize_t f_ss_ext_opts_isoc_maxburst_show(
					struct config_item *item,
					char *page)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_show(item, page, opts->isoc_maxburst);
}

static ssize_t f_ss_ext_opts_isoc_maxburst_store(
					struct config_item *item,
					 const char *page, size_t len)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_store(item, page, len, &opts->isoc_maxburst);
}
CONFIGFS_ATTR(f_ss_ext_opts_, isoc_maxburst);

static ssize_t f_ss_ext_opts_isoc_mult_show(
					struct config_item *item,
					char *page)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_show(item, page, opts->isoc_mult);
}

static ssize_t f_ss_ext_opts_isoc_mult_store(
					struct config_item *item,
					 const char *page, size_t len)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_store(item, page, len, &opts->isoc_mult);
}
CONFIGFS_ATTR(f_ss_ext_opts_, isoc_mult);

static ssize_t f_ss_ext_opts_isoc_qlen_show(
					struct config_item *item,
					char *page)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_show(item, page, opts->isoc_qlen);
}

static ssize_t f_ss_ext_opts_isoc_qlen_store(
					struct config_item *item,
					 const char *page, size_t len)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);
	int result;

	result = f_ss_ext_opts_store(item, page, len, &opts->isoc_qlen);
	if (opts->isoc_qlen > SS_MAX_REQ_NUM)
		opts->isoc_qlen = SS_MAX_REQ_NUM;

	return result;
}
CONFIGFS_ATTR(f_ss_ext_opts_, isoc_qlen);

static ssize_t f_ss_ext_opts_bulk_qlen_show(
					struct config_item *item,
					char *page)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_show(item, page, opts->bulk_qlen);
}

static ssize_t f_ss_ext_opts_bulk_qlen_store(
					struct config_item *item,
					 const char *page, size_t len)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);
	int result;

	result = f_ss_ext_opts_store(item, page, len, &opts->bulk_qlen);
	if (opts->bulk_qlen > SS_MAX_REQ_NUM)
		opts->bulk_qlen = SS_MAX_REQ_NUM;

	return result;
}
CONFIGFS_ATTR(f_ss_ext_opts_, bulk_qlen);

static ssize_t f_ss_ext_opts_bulk_buflen_show(
					struct config_item *item,
					char *page)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_show(item, page, opts->bulk_buflen);
}

static ssize_t f_ss_ext_opts_bulk_buflen_store(
					struct config_item *item,
					 const char *page, size_t len)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_store(item, page, len, &opts->bulk_buflen);
}
CONFIGFS_ATTR(f_ss_ext_opts_, bulk_buflen);

static ssize_t f_ss_ext_opts_bulk_maxpacket_show(
					struct config_item *item,
					char *page)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_show(item, page, opts->bulk_maxpacket);
}

static ssize_t f_ss_ext_opts_bulk_maxpacket_store(
					struct config_item *item,
					 const char *page, size_t len)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_store(item, page, len, &opts->bulk_maxpacket);
}
CONFIGFS_ATTR(f_ss_ext_opts_, bulk_maxpacket);

static ssize_t f_ss_ext_opts_intr_qlen_show(
					struct config_item *item,
					char *page)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_show(item, page, opts->intr_qlen);
}

static ssize_t f_ss_ext_opts_intr_qlen_store(
					struct config_item *item,
					 const char *page, size_t len)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);
	int result;

	result = f_ss_ext_opts_store(item, page, len, &opts->intr_qlen);
	if (opts->intr_qlen > SS_MAX_REQ_NUM)
		opts->intr_qlen = SS_MAX_REQ_NUM;

	return result;
}
CONFIGFS_ATTR(f_ss_ext_opts_, intr_qlen);

static ssize_t f_ss_ext_opts_intr_buflen_show(
					struct config_item *item,
					char *page)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_show(item, page, opts->intr_buflen);
}

static ssize_t f_ss_ext_opts_intr_buflen_store(
					struct config_item *item,
					 const char *page, size_t len)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_store(item, page, len, &opts->intr_buflen);
}
CONFIGFS_ATTR(f_ss_ext_opts_, intr_buflen);

static ssize_t f_ss_ext_opts_intr_interval_show(
					struct config_item *item,
					char *page)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_show(item, page, opts->intr_interval);
}

static ssize_t f_ss_ext_opts_intr_interval_store(
					struct config_item *item,
					 const char *page, size_t len)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_store(item, page, len, &opts->intr_interval);
}
CONFIGFS_ATTR(f_ss_ext_opts_, intr_interval);

static ssize_t f_ss_ext_opts_intr_maxpacket_show(
					struct config_item *item,
					char *page)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_show(item, page, opts->intr_maxpacket);
}

static ssize_t f_ss_ext_opts_intr_maxpacket_store(
					struct config_item *item,
					 const char *page, size_t len)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_store(item, page, len, &opts->intr_maxpacket);
}
CONFIGFS_ATTR(f_ss_ext_opts_, intr_maxpacket);

static ssize_t f_ss_ext_opts_test_mode_show(
					struct config_item *item,
					char *page)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_show(item, page, opts->test_mode);
}

static ssize_t f_ss_ext_opts_test_mode_store(
					struct config_item *item,
					 const char *page, size_t len)
{
	struct f_ss_ext_opts *opts = to_f_ss_ext_opts(item);

	return f_ss_ext_opts_store(item, page, len, &opts->test_mode);
}
CONFIGFS_ATTR(f_ss_ext_opts_, test_mode);

static struct configfs_attribute *ss_attrs[] = {
	&f_ss_ext_opts_attr_pattern,
	&f_ss_ext_opts_attr_isoc_interval,
	&f_ss_ext_opts_attr_isoc_maxpacket,
	&f_ss_ext_opts_attr_isoc_mult,
	&f_ss_ext_opts_attr_isoc_maxburst,
	&f_ss_ext_opts_attr_isoc_qlen,
	&f_ss_ext_opts_attr_bulk_buflen,
	&f_ss_ext_opts_attr_bulk_qlen,
	&f_ss_ext_opts_attr_bulk_maxpacket,
	&f_ss_ext_opts_attr_intr_buflen,
	&f_ss_ext_opts_attr_intr_interval,
	&f_ss_ext_opts_attr_intr_qlen,
	&f_ss_ext_opts_attr_intr_maxpacket,
	&f_ss_ext_opts_attr_test_mode,
	NULL,
};

static struct config_item_type ss_func_type = {
	.ct_item_ops    = &ss_item_ops,
	.ct_attrs	= ss_attrs,
	.ct_owner       = THIS_MODULE,
};

static unsigned ss_ext_inst_num;

static void ss_ext_free_instance(struct usb_function_instance *fi)
{
	struct f_ss_ext_opts *ss_opts = NULL;

	ss_opts = container_of(fi, struct f_ss_ext_opts, func_inst);
	pr_info("nst_num %d\n", ss_opts->inst_num);
	kfree(ss_opts);
	ss_ext_inst_num--;
}

static struct usb_function_instance *ss_ext_alloc_inst(void)
{
	struct f_ss_ext_opts *ss_opts = NULL;

	ss_opts = kzalloc(sizeof(*ss_opts), GFP_KERNEL);
	if (!ss_opts)
		return ERR_PTR(-ENOMEM);

	mutex_init(&ss_opts->lock);
	ss_opts->func_inst.free_func_inst = ss_ext_free_instance;
	ss_opts->pattern = 1; /* mod63 */

	ss_opts->isoc_interval = 4;
	ss_opts->isoc_maxpacket = 1024;
	ss_opts->isoc_qlen = 8;

	ss_opts->bulk_buflen = 4096;
	ss_opts->bulk_qlen = 1;
	ss_opts->bulk_maxpacket = 1024;

	ss_opts->intr_interval = 4;
	ss_opts->intr_maxpacket = 1024;
	ss_opts->intr_buflen = 4096;
	ss_opts->intr_qlen = 1;

	ss_opts->test_mode = SS_TEST_MODE_ISOC_BULK_INTR;
	ss_opts->inst_num = ss_ext_inst_num++;
	pr_info("inst_num %d\n", ss_opts->inst_num);

	config_group_init_type_name(&ss_opts->func_inst.group, "",
				    &ss_func_type);

	return &ss_opts->func_inst;
}

DECLARE_USB_FUNCTION_INIT(SourceSinkExt, ss_ext_alloc_inst, ss_ext_alloc_func);

MODULE_LICENSE("GPL");
