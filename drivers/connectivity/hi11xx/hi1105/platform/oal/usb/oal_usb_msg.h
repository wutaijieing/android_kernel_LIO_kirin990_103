

#include "oal_usb_host_if.h"

#ifndef __OAL_USB_MSG_H__
#define __OAL_USB_MSG_H__

void usb_msg_dev_init(struct oal_usb_dev *usb_dev);
int32_t oal_usb_send_msg(hcc_bus *bus, uint32_t val);
int32_t oal_usb_gpio_rx_data(hcc_bus *bus);

#endif
