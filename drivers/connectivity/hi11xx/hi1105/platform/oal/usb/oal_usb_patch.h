

#include "oal_usb_host_if.h"

#ifndef __OAL_USB_PATCH_H__
#define __OAL_USB_PATCH_H__

int32_t oal_usb_patch_write(hcc_bus *bus, uint8_t *buff, int32_t len);
int32_t oal_usb_patch_read(hcc_bus *bus, uint8_t *buff, int32_t len, uint32_t timeout);

#endif
