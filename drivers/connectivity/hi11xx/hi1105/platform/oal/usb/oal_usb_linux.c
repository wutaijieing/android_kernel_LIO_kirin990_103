
#include "oal_usb_linux.h"
#include "oal_hcc_host_if.h"
#include "oal_usb_host_if.h"

#define OAL_USB_OUT_DIR 0
#define OAL_USB_IN_DIR  1

static int32_t oal_usb_transfer_common(struct oal_usb_dev *usb_dev, struct urb *urb, uint32_t timeout,
                                       oal_completion *wait)
{
    int32_t ret;

    usb_anchor_urb(urb, &usb_dev->submitted);
    ret = usb_submit_urb(urb, GFP_KERNEL);
    if (ret < 0) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "submit tx urb failed [%d]", ret);
        goto error_unanchor;
    }
    if (timeout == 0) {
        return OAL_SUCC;
    }

    /* wakeup in urb->callback */
    ret = oal_wait_for_completion_timeout(wait, (uint32_t)oal_msecs_to_jiffies(timeout));
    if (ret == 0) {
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "wait for urb complete failed");
        return -OAL_EIO;
    }
    return OAL_SUCC;
error_unanchor:
    usb_unanchor_urb(urb);
    return OAL_FAIL;
}

static int32_t oal_usb_tx_common(struct oal_usb_dev *usb_dev, struct urb *urb, uint32_t timeout)
{
    int32_t ret;

    usb_dev->xfer_stat.tx_total++;
    ret = oal_usb_transfer_common(usb_dev, urb, timeout, &usb_dev->tx_wait);
    if (ret != OAL_SUCC) {
        usb_dev->xfer_stat.tx_miss++;
    }
    return ret;
}

static int32_t oal_usb_rx_common(struct oal_usb_dev *usb_dev, struct urb *urb, uint32_t timeout)
{
    int32_t ret;

    usb_dev->xfer_stat.rx_total++;
    ret = oal_usb_transfer_common(usb_dev, urb, timeout, &usb_dev->rx_wait);
    if (ret != OAL_SUCC) {
        usb_dev->xfer_stat.rx_miss++;
    }
    return ret;
}

static struct urb *oal_usb_parpare_bulk_urb(struct oal_usb_xfer_param *param, uint32_t is_in)
{
    struct oal_usb_dev *usb_dev = param->usb_dev;
    struct urb *urb = NULL;
    uint32_t pipe;

    urb = usb_alloc_urb(0, GFP_KERNEL);
    if (urb == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "alloc urb failed");
        return NULL;
    }
    if (is_in) {
        pipe = usb_rcvbulkpipe(usb_dev->udev, usb_dev->ep_info.in_ep_addr);
    } else {
        pipe = usb_sndbulkpipe(usb_dev->udev, usb_dev->ep_info.out_ep_addr);
    }

    usb_fill_bulk_urb(urb, usb_dev->udev, pipe, param->buf, param->len, param->complete, param);
    return urb;
}

static struct urb *oal_usb_parpare_intr_urb(struct oal_usb_xfer_param *param, uint32_t is_in)
{
    struct oal_usb_dev *usb_dev = param->usb_dev;
    struct urb *urb = NULL;
    uint32_t pipe;
    uint32_t interval;

    urb = usb_alloc_urb(0, GFP_KERNEL);
    if (urb == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "alloc urb failed");
        return NULL;
    }

    if (is_in) {
        pipe = usb_rcvintpipe(usb_dev->udev, usb_dev->ep_info.in_ep_addr);
        interval = usb_dev->ep_info.in_interval;
    } else {
        pipe = usb_sndintpipe(usb_dev->udev, usb_dev->ep_info.out_ep_addr);
        interval = usb_dev->ep_info.out_interval;
    }
    usb_fill_int_urb(urb, usb_dev->udev, pipe, param->buf, param->len, param->complete, param, interval);
    return urb;
}

static struct urb *oal_usb_parpare_isoc_urb(struct oal_usb_xfer_param *param, uint32_t is_in)
{
    struct oal_usb_dev *usb_dev = param->usb_dev;
    struct urb *urb = NULL;
    uint32_t npackets;
    uint32_t packet_size;
    uint32_t i;

    if (is_in) {
        packet_size = usb_dev->ep_info.in_max_packet;
    } else {
        packet_size = usb_dev->ep_info.out_max_packet;
    }
    npackets = param->len / packet_size;

    urb = usb_alloc_urb(npackets, GFP_KERNEL);
    if (urb == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "alloc urb failed");
        return NULL;
    }

    urb->dev = usb_dev->udev;
    urb->context = usb_dev;
    urb->transfer_buffer = param->buf;
    urb->transfer_buffer_length = param->len;
    if (is_in) {
        urb->pipe = usb_rcvisocpipe(usb_dev->udev, usb_dev->ep_info.in_ep_addr);
        urb->interval = 1 << (usb_dev->ep_info.in_interval - 1);
    } else {
        urb->pipe = usb_sndisocpipe(usb_dev->udev, usb_dev->ep_info.out_ep_addr);
        urb->interval = 1 << (usb_dev->ep_info.out_interval - 1);
    }

    urb->transfer_flags = URB_ISO_ASAP | URB_NO_TRANSFER_DMA_MAP;
    urb->complete = param->complete;
    urb->number_of_packets = npackets;
    for (i = 0; i < npackets; ++i) {
        urb->iso_frame_desc[i].offset = packet_size * i;
        urb->iso_frame_desc[i].length = packet_size;
    }
    return urb;
}

static struct urb *oal_usb_parpare_urb(struct oal_usb_xfer_param *param, uint32_t xfer_type, uint32_t xfer_dir)
{
    struct oal_usb_dev *usb_dev = param->usb_dev;
    struct urb *urb = NULL;
    dma_addr_t dma_addr;
    enum dma_data_direction dir;

    switch (xfer_type) {
        case OAL_USB_INTERFACE_ISOC:
            urb = oal_usb_parpare_isoc_urb(param, xfer_dir);
            break;
        case OAL_USB_INTERFACE_BULK:
            urb = oal_usb_parpare_bulk_urb(param, xfer_dir);
            break;
        case OAL_USB_INTERFACE_INTR:
            urb = oal_usb_parpare_intr_urb(param, xfer_dir);
            break;
        default:
            break;
    }

    if (urb == NULL) {
        return NULL;
    }

    if (usb_get_xfer_flag(param->flag, USB_XFER_DMA)) {
        dir = xfer_dir ? DMA_FROM_DEVICE : DMA_TO_DEVICE;
        dma_addr = dma_map_single(&usb_dev->udev->dev, param->buf, param->len, dir);
        if (dma_mapping_error(&usb_dev->udev->dev, dma_addr)) {
            usb_free_urb(urb);
            return NULL;
        }
        urb->transfer_dma = dma_addr;
        urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
    }

    return urb;
}

int32_t oal_usb_bulk_tx(struct oal_usb_xfer_param *param)
{
    struct oal_usb_dev *usb_dev = param->usb_dev;
    struct urb *urb = NULL;
    int32_t ret;

    if (usb_dev == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "usb_dev is null");
        return -OAL_ENODEV;
    }

    urb = oal_usb_parpare_urb(param, OAL_USB_INTERFACE_BULK, OAL_USB_OUT_DIR);
    if (urb == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "alloc urb failed");
        return -OAL_EFAIL;
    }

    ret = oal_usb_tx_common(usb_dev, urb, param->timeout);
    if (ret != OAL_SUCC) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "tx fail");
        usb_kill_urb(urb);
    }
    return ret;
}

int32_t oal_usb_bulk_rx(struct oal_usb_xfer_param *param)
{
    struct oal_usb_dev *usb_dev = param->usb_dev;
    struct urb *urb = NULL;
    int32_t ret;

    if (usb_dev == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "usb_dev is null");
        return -OAL_ENODEV;
    }

    urb = oal_usb_parpare_urb(param, OAL_USB_INTERFACE_BULK, OAL_USB_IN_DIR);
    if (urb == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "alloc urb failed");
        return -OAL_EFAIL;
    }

    ret = oal_usb_rx_common(usb_dev, urb, param->timeout);
    if (ret != OAL_SUCC) {
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "rx fail");
        usb_kill_urb(urb);
    }
    return ret;
}

int32_t oal_usb_intr_tx(struct oal_usb_xfer_param *param)
{
    struct oal_usb_dev *usb_dev = param->usb_dev;
    struct urb *urb = NULL;
    int32_t ret;

    if (usb_dev == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "usb_dev is null");
        return -OAL_ENODEV;
    }

    urb = oal_usb_parpare_urb(param, OAL_USB_INTERFACE_INTR, OAL_USB_OUT_DIR);
    if (urb == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "alloc urb failed");
        return -OAL_EFAIL;
    }

    ret = oal_usb_tx_common(usb_dev, urb, param->timeout);
    if (ret != OAL_SUCC) {
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "tx fail");
        usb_kill_urb(urb);
    }
    return ret;
}

int32_t oal_usb_intr_rx(struct oal_usb_xfer_param *param)
{
    struct oal_usb_dev *usb_dev = param->usb_dev;
    struct urb *urb = NULL;
    int32_t ret;

    if (usb_dev == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "usb_dev is null");
        return -OAL_ENODEV;
    }

    urb = oal_usb_parpare_urb(param, OAL_USB_INTERFACE_INTR, OAL_USB_IN_DIR);
    if (urb == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "alloc urb failed");
        return -OAL_EFAIL;
    }

    mutex_lock(&usb_dev->rx_mutex);
    ret = oal_usb_rx_common(usb_dev, urb, param->timeout);
    if (ret != OAL_SUCC) {
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "rx fail");
        usb_kill_urb(urb);
    }
    mutex_unlock(&usb_dev->rx_mutex);
    return ret;
}

int32_t oal_usb_isoc_tx(struct oal_usb_xfer_param *param)
{
    struct oal_usb_dev *usb_dev = param->usb_dev;
    struct urb *urb = NULL;
    int32_t ret;

    if (usb_dev == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "usb_dev is null");
        return -OAL_ENODEV;
    }

    urb = oal_usb_parpare_urb(param, OAL_USB_INTERFACE_ISOC, OAL_USB_OUT_DIR);
    if (urb == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "alloc urb failed");
        return -OAL_EFAIL;
    }

    ret = oal_usb_tx_common(usb_dev, urb, param->timeout);
    if (ret != OAL_SUCC) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "tx fail");
        usb_kill_urb(urb);
        return ret;
    }
    return OAL_SUCC;
}

int32_t oal_usb_isoc_rx(struct oal_usb_xfer_param *param)
{
    struct oal_usb_dev *usb_dev = param->usb_dev;
    struct urb *urb = NULL;
    int32_t ret;

    if (usb_dev == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "usb_dev is null");
        return -OAL_ENODEV;
    }

    urb = oal_usb_parpare_urb(param, OAL_USB_INTERFACE_ISOC, OAL_USB_IN_DIR);
    if (urb == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "alloc urb failed");
        return -OAL_EFAIL;
    }

    ret = oal_usb_rx_common(usb_dev, urb, param->timeout);
    if (ret != OAL_SUCC) {
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "rx fail");
        usb_kill_urb(urb);
    }
    mutex_unlock(&usb_dev->rx_mutex);

    return ret;
}

void oal_usb_rx_complete(struct urb *urb, struct oal_usb_xfer_param *param)
{
    struct oal_usb_dev *usb_dev = (struct oal_usb_dev *)param->usb_dev;

    if (urb->status) {
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "urb->status[%d]", urb->status);
        goto exit;
    }
    usb_dev->xfer_stat.rx_cmpl++;
    oal_complete(&usb_dev->rx_wait);
exit:
    oal_free(param);
    usb_free_urb(urb);
}

void oal_usb_tx_complete(struct urb *urb, struct oal_usb_xfer_param *param)
{
    struct oal_usb_dev *usb_dev = (struct oal_usb_dev *)param->usb_dev;

    if (urb->status) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "urb->status[%d]", urb->status);
        goto exit;
    }
    usb_dev->xfer_stat.tx_cmpl++;
    oal_complete(&usb_dev->tx_wait);
exit:
    oal_free(param);
    usb_free_urb(urb);
}
