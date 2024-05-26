

#include "oal_usb_host_if.h"

#ifndef __OAL_USB_NETBUF_H__
#define __OAL_USB_NETBUF_H__

void usb_netbuf_dev_init(struct oal_usb_dev *usb_dev);
int32_t oal_usb_rx_netbuf(hcc_bus *bus, oal_netbuf_head_stru *head);
int32_t oal_usb_tx_netbuf(hcc_bus *bus, oal_netbuf_head_stru *head, hcc_netbuf_queue_type qtype);

#endif
