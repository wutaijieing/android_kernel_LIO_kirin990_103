/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: usb host interface header
 * Author: platform
 * Create: 2021-03-29
 * History:
 */
#ifndef __OAL_USB_HOST_IF_H__
#define __OAL_USB_HOST_IF_H__

#include "oal_hcc_bus.h"

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include <linux/usb.h>

#define USB_FPGA_DEBUG

#define USB_INTERFACE_CLASS    0xFF
#define USB_INTERFACE_SUBCLASS 0x42

#define USB_INTERFACE_TYPE_MASK  0xC0
#define USB_INTERFACE_TYPE_SHIFT 0x6
#define USB_INTERFACE_CHAN_MASK  0x3C
#define USB_INTERFACE_CHAN_SHIFT 0x2

enum OAL_USB_INTERFACE {
    OAL_USB_INTERFACE_CTRL = 0,
    OAL_USB_INTERFACE_ISOC = 1,
    OAL_USB_INTERFACE_BULK = 2,
    OAL_USB_INTERFACE_INTR = 3,
    OAL_USB_INTERFACE_NUMS = 4,
};

enum OAL_USB_CHAN_INDEX {
    OAL_USB_CHAN_0 = 0,
    OAL_USB_CHAN_1,
    OAL_USB_CHAN_2,
    OAL_USB_CHAN_3,
    OAL_USB_CHAN_NUMS
};

struct endpoint_info {
    uint8_t in_ep_addr; /* D2H */
    uint8_t in_interval;
    uint16_t in_max_packet;

    uint8_t out_ep_addr; /* H2D */
    uint8_t out_interval;
    uint16_t out_max_packet;
};

struct oal_usb_stat {
    uint32_t rx_total;
    uint32_t tx_total;

    uint32_t rx_cmpl;
    uint32_t tx_cmpl;

    uint32_t rx_miss;
    uint32_t tx_miss;
};

typedef int32_t (*usb_rx_thread)(void *);

struct oal_usb_dev {
    struct oal_usb_dev_container *usb_dev_container;
    struct usb_device *udev;
    struct usb_interface *interface;
    struct endpoint_info ep_info;
    struct usb_anchor submitted; /* maintain submitted urb */
    struct mutex rx_mutex;
    oal_completion rx_wait; /* 同步接收 */
    oal_completion tx_wait; /* 同步发送 */
    struct oal_usb_stat xfer_stat;
    uint32_t rx_threshold;    /* rx深度 */
    uint32_t rx_buf_max_size; /* rx最大长度 */

    struct task_struct *rx_task;
    char *task_name;
    oal_wait_queue_head_stru rx_wq;
    oal_atomic rx_cond;
    usb_rx_thread rx_thread;
};

struct oal_usb_dev_ops {
    void (*usb_dev_init)(struct oal_usb_dev *);
};

struct oal_usb_dev_container {
    hcc_bus *bus;
    uint32_t state;
    struct oal_usb_dev *usb_devices[OAL_USB_INTERFACE_NUMS][OAL_USB_CHAN_NUMS];
    struct oal_usb_dev_ops *usb_dev_ops[OAL_USB_INTERFACE_NUMS][OAL_USB_CHAN_NUMS];
};

int32_t oal_wifi_platform_load_usb(void);
#endif

#endif
