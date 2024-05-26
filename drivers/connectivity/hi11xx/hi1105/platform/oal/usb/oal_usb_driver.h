

#ifndef __OAL_USB_DRIVER_H__
#define __OAL_USB_DRIVER_H__

#include "oal_schedule.h"
#include "oal_usb_host_if.h"
#include "plat_type.h"

struct oal_usb_dev_cb {
    void (*oal_usb_dev_register)(struct oal_usb_dev *usb_dev, uint32_t type, uint32_t chan);
    void (*oal_usb_dev_deregister)(uint32_t type, uint32_t chan);
    void (*oal_usb_dev_probe_complete)(void);
};

int32_t oal_usb_driver_init(void);
void oal_usb_set_dev_cb(struct oal_usb_dev_cb *usb_dev_cb);
void oal_usb_create_rx_thread(struct oal_usb_dev *usb_dev);
void oal_usb_distory_rx_thread(struct oal_usb_dev *usb_dev);

/* 调度接收线程，补充rx dr描述符 */
static inline void oal_usb_sched_rx_thread(struct oal_usb_dev *usb_dev)
{
    oal_atomic_set(&usb_dev->rx_cond, 1);
    oal_wait_queue_wake_up_interrupt(&usb_dev->rx_wq);
}

static inline int32_t oal_usb_rx_thread_condtion(oal_atomic *pst_ato)
{
    int32_t ret = oal_atomic_read(pst_ato);
    if (oal_likely(ret == 1)) {
        oal_atomic_set(pst_ato, 0);
    }
    return ret;
}

#endif
