/*
 * Chip Adp USB function
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


#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/module.h>

#include <platform_include/basicplatform/linux/usb/bsp_acm.h>

#include "usb_vendor.h"

#define ERROR (-1)
#define OK 0

#define USB_ENABLE_CB_MAX 32

struct usb_udi_enable_cb_wrap {
	USB_UDI_ENABLE_CB_T enable_cb;
};

struct usb_udi_disable_cb_wrap {
	USB_UDI_DISABLE_CB_T disable_cb;
};

struct usb_udi_enable_cb_wrap g_usb_enum_done_cbs[USB_ENABLE_CB_MAX];
struct usb_udi_disable_cb_wrap g_usb_enum_dis_cbs[USB_ENABLE_CB_MAX];

static unsigned int g_usb_enum_done_cur;
static unsigned int g_usb_enum_dis_cur;

static int g_usb_enum_done_notify_complete;
static int g_usb_disable_notify_complete;

/* External Interface */
unsigned int BSP_USB_RegUdiEnableCB(USB_UDI_ENABLE_CB_T pFunc)
{
	pr_info("%s: %pK\n", __func__, pFunc);
	if (g_usb_enum_done_cur >= USB_ENABLE_CB_MAX) {
		pr_err("%s error:%pK\n", __func__, pFunc);
		return (unsigned int)ERROR;
	}

	g_usb_enum_done_cbs[g_usb_enum_done_cur].enable_cb = pFunc;
	g_usb_enum_done_cur++;

	if (g_usb_enum_done_notify_complete) {
		if (pFunc)
			pFunc();
	}

	return OK;
}

/* External Interface */
unsigned int BSP_USB_RegUdiDisableCB(USB_UDI_DISABLE_CB_T pFunc)
{
	pr_info("%s: %pK\n", __func__, pFunc);
	if (g_usb_enum_dis_cur >= USB_ENABLE_CB_MAX) {
		pr_err("%s error:%pK\n", __func__, pFunc);
		return (unsigned int)ERROR;
	}

	g_usb_enum_dis_cbs[g_usb_enum_dis_cur].disable_cb = pFunc;
	g_usb_enum_dis_cur++;

	return OK;
}

static int gs_usb_adp_notifier_cb(struct notifier_block *nb,
				unsigned long event, void *priv)
{
	int loop;

	switch (event) {
	case USB_MODEM_DEVICE_INSERT:
		g_usb_disable_notify_complete = 0;
		break;
	case USB_MODEM_ENUM_DONE:
		/* enum done */
		g_usb_disable_notify_complete = 0;
		if (g_usb_enum_done_notify_complete)
			break;
		pr_info("%s: enumdone\n", __func__);
		for (loop = 0; loop < USB_ENABLE_CB_MAX; loop++) {
			if (g_usb_enum_done_cbs[loop].enable_cb)
				g_usb_enum_done_cbs[loop].enable_cb();
		}
		g_usb_enum_done_notify_complete = 1;
		break;
	case USB_MODEM_DEVICE_DISABLE:
	case USB_MODEM_DEVICE_REMOVE:
		/* notify other cb */
		g_usb_enum_done_notify_complete = 0;
		if (g_usb_disable_notify_complete)
			break;
		pr_info("%s: disable\n", __func__);
		for (loop = 0; loop < USB_ENABLE_CB_MAX; loop++) {
			if (g_usb_enum_dis_cbs[loop].disable_cb)
				g_usb_enum_dis_cbs[loop].disable_cb();
		}
		g_usb_disable_notify_complete = 1;
		break;
	default:
		break;
	}
	return 0;
}

static struct notifier_block gs_adp_usb_nb;
static struct notifier_block *gs_adp_usb_nb_ptr;

int __init adp_usb_init(void)
{
	if (gs_adp_usb_nb_ptr == NULL) {
		gs_adp_usb_nb_ptr = &gs_adp_usb_nb;
		gs_adp_usb_nb.priority = USB_NOTIF_PRIO_ADP;
		gs_adp_usb_nb.notifier_call = gs_usb_adp_notifier_cb;
		bsp_usb_register_notify(gs_adp_usb_nb_ptr);
	}
	return 0;
}
module_init(adp_usb_init);

static void __exit adp_usb_exit(void)
{
	if (gs_adp_usb_nb_ptr != NULL) {
		bsp_usb_unregister_notify(gs_adp_usb_nb_ptr);
		gs_adp_usb_nb_ptr = NULL;
	}
}

module_exit(adp_usb_exit);

