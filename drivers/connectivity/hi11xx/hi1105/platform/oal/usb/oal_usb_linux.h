
#ifndef __OAL_USB_LINUX_H__
#define __OAL_USB_LINUX_H__

#include "oal_usb_host_if.h"

#define TRANSFER_DEFAULT_TIMEOUT 100 /* default transfer timeout (ms) */
#define TRANSFER_NOT_TIMEOUT     0

struct oal_usb_xfer_param {
    struct oal_usb_dev *usb_dev;
    uint8_t *buf;
    uint32_t len;
    uint32_t timeout;
    uint32_t flag;
    usb_complete_t complete;
    void *context;
};

static inline struct oal_usb_xfer_param *oal_usb_alloc_xfer_param(struct oal_usb_dev *usb_dev, uint8_t *buf,
                                                                  uint32_t len, uint32_t timeout, uint32_t flag,
                                                                  usb_complete_t complete, void *context)
{
    struct oal_usb_xfer_param *param = (struct oal_usb_xfer_param *)oal_memalloc(sizeof(struct oal_usb_xfer_param));
    param->usb_dev = usb_dev;
    param->buf = buf;
    param->len = len;
    param->timeout = timeout;
    param->flag = flag;
    param->complete = complete;
    param->context = context;
    return param;
}

#define USB_XFER_DMA    1U
#define USB_XFER_NO_DMA 0U

static inline uint32_t usb_get_xfer_flag(uint32_t flag, uint32_t mask)
{
    return (flag & mask) == mask;
}

int32_t oal_usb_bulk_tx(struct oal_usb_xfer_param *param);
int32_t oal_usb_bulk_rx(struct oal_usb_xfer_param *param);
int32_t oal_usb_intr_tx(struct oal_usb_xfer_param *param);
int32_t oal_usb_intr_rx(struct oal_usb_xfer_param *param);
int32_t oal_usb_isoc_tx(struct oal_usb_xfer_param *param);
int32_t oal_usb_isoc_rx(struct oal_usb_xfer_param *param);

void oal_usb_rx_complete(struct urb *urb, struct oal_usb_xfer_param *param);
void oal_usb_tx_complete(struct urb *urb, struct oal_usb_xfer_param *param);

#endif
