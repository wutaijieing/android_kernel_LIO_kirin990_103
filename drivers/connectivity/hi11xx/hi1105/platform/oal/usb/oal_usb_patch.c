
#include "oal_usb_patch.h"
#include "oal_usb_driver.h"
#include "oal_usb_linux.h"

static void oal_usb_tx_patch_complete(struct urb *urb)
{
    struct oal_usb_xfer_param *param = NULL;

    param = (struct oal_usb_xfer_param *)urb->context;
    oal_usb_tx_complete(urb, param);
}

static void oal_usb_rx_patch_complete(struct urb *urb)
{
    struct oal_usb_xfer_param *param = NULL;

    param = (struct oal_usb_xfer_param *)urb->context;
    oal_usb_rx_complete(urb, param);
}

int32_t oal_usb_patch_read(hcc_bus *bus, uint8_t *buff, int32_t len, uint32_t timeout)
{
    struct oal_usb_dev_container *usb_dev_container = NULL;
    struct oal_usb_dev *usb_dev = NULL;
    struct oal_usb_xfer_param *param = NULL;
    int32_t ret;

    usb_dev_container = (struct oal_usb_dev_container *)bus->data;
    if (usb_dev_container == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "usb_dev_container is null");
        return -OAL_ENODEV;
    }

    usb_dev = usb_dev_container->usb_devices[OAL_USB_INTERFACE_BULK][OAL_USB_CHAN_0];
    if (usb_dev == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "usb_dev_loader is null");
        return -OAL_ENODEV;
    }
    param = oal_usb_alloc_xfer_param(usb_dev, buff, len, timeout, USB_XFER_NO_DMA, oal_usb_rx_patch_complete, NULL);
    ret = oal_usb_bulk_rx(param);
    if (ret < 0) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "rx fail");
    }
    return ret;
}

int32_t oal_usb_patch_write(hcc_bus *bus, uint8_t *buff, int32_t len)
{
    struct oal_usb_dev_container *usb_dev_container = NULL;
    struct oal_usb_dev *usb_dev = NULL;
    struct oal_usb_xfer_param *param = NULL;
    int32_t ret;

    usb_dev_container = (struct oal_usb_dev_container *)bus->data;
    if (usb_dev_container == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "usb_dev_container is null");
        return -OAL_ENODEV;
    }

    usb_dev = usb_dev_container->usb_devices[OAL_USB_INTERFACE_BULK][OAL_USB_CHAN_0];
    if (usb_dev == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "usb_dev_loader is null");
        return -OAL_ENODEV;
    }
    param = oal_usb_alloc_xfer_param(usb_dev, buff, len, TRANSFER_DEFAULT_TIMEOUT, USB_XFER_NO_DMA,
                                     oal_usb_tx_patch_complete, NULL);
    ret = oal_usb_bulk_tx(param);
    if (ret != OAL_SUCC) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "tx fail");
    }
    return ret;
}
