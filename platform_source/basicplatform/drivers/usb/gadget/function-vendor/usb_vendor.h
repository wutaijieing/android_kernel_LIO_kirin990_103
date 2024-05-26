/*
 * usb_vendor.c
 *
 * Chip usb notifier
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
#ifndef __USB_VENDOR_H__
#define __USB_VENDOR_H__

#include <linux/notifier.h>

/* The usb_vendor declarations */

#define USB_NOTIF_PRIO_ADP		0       /* adp has lowest priority */
#define USB_NOTIF_PRIO_FD		100     /* function drvier */
#define USB_NOTIF_PRIO_CORE		200     /* usb core */
#define USB_NOTIF_PRIO_HAL		300     /* hardware has highest priority */

#define USB_MODEM_DEVICE_INSERT	1
#define USB_MODEM_CHARGER_IDEN		2
#define USB_MODEM_ENUM_DONE		3
#define USB_MODEM_PERIP_INSERT		4
#define USB_MODEM_PERIP_REMOVE		5
#define USB_MODEM_DEVICE_REMOVE	0
#define USB_MODEM_DEVICE_DISABLE	0xF1

/* notify interface */
void bsp_usb_register_notify(struct notifier_block *nb);
void bsp_usb_unregister_notify(struct notifier_block *nb);

/* usb status change interface */
void bsp_usb_status_change(int status);

/* usb enum done interface */
void bsp_usb_add_setup_dev_fdname(unsigned int intf_id, const char *fd_name);
void bsp_usb_remove_setup_dev_fdname(void);
void bsp_usb_set_enum_stat(unsigned int intf_id, int enum_stat);

#endif /* __USB_VENDOR_H__ */
