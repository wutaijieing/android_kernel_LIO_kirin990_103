

#include "oal_usb_msg.h"
#include "oal_usb_driver.h"
#include "oal_usb_linux.h"

#define OAL_USB_MSG_MAX_SIZE 4

void usb_msg_dev_init(struct oal_usb_dev *usb_dev)
{
    usb_dev->rx_buf_max_size = OAL_USB_MSG_MAX_SIZE;
}

static void oal_usb_tx_msg_complete(struct urb *urb)
{
    struct oal_usb_xfer_param *param = NULL;
    struct oal_usb_dev *usb_dev = NULL;

    param = (struct oal_usb_xfer_param *)urb->context;
    usb_dev = param->usb_dev;
    dma_unmap_single(&usb_dev->udev->dev, urb->transfer_dma, urb->transfer_buffer_length, DMA_TO_DEVICE);
    oal_usb_tx_complete(urb, param);
}

static void oal_usb_rx_msg_complete(struct urb *urb)
{
    struct oal_usb_xfer_param *param = NULL;
    struct oal_usb_dev *usb_dev = NULL;

    param = (struct oal_usb_xfer_param *)urb->context;
    usb_dev = param->usb_dev;
    dma_unmap_single(&usb_dev->udev->dev, urb->transfer_dma, urb->transfer_buffer_length, DMA_TO_DEVICE);
    oal_usb_rx_complete(urb, param);
}

static int32_t oal_usb_send_message_to_dev(struct oal_usb_dev *usb_dev, uint32_t message)
{
    struct oal_usb_xfer_param *param = NULL;
    int32_t ret = -OAL_ENOMEM;
    uint32_t *buf = NULL;
    int32_t buf_len;

    /* alloc buffer, free in callback function */
    buf_len = sizeof(message);
    buf = oal_memalloc(buf_len);
    if (buf == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "alloc buffer failed");
        return ret;
    }
    buf[0] = message;

    param = oal_usb_alloc_xfer_param(usb_dev, (uint8_t *)buf, buf_len, TRANSFER_DEFAULT_TIMEOUT, USB_XFER_DMA,
                                     oal_usb_tx_msg_complete, NULL);
    ret = oal_usb_intr_tx(param);
    if (ret != OAL_SUCC) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "tx fail");
    }
    oal_free(buf);
    return ret;
}

int32_t oal_usb_send_msg(hcc_bus *bus, uint32_t val)
{
    int32_t ret;
    struct oal_usb_dev_container *usb_dev_container = NULL;
    struct oal_usb_dev *usb_dev = NULL;

    if (val >= H2D_MSG_COUNT) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "invaild param[%u]", val);
        return -OAL_EINVAL;
    }

    usb_dev_container = (struct oal_usb_dev_container *)bus->data;
    if (usb_dev_container == NULL) {
        return -OAL_ENODEV;
    }

    usb_dev = usb_dev_container->usb_devices[OAL_USB_INTERFACE_INTR][OAL_USB_CHAN_0];
    if (usb_dev == NULL) {
        return -OAL_ENODEV;
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "send msg to device [%u]", val);

    hcc_bus_wake_lock(bus);
    ret = oal_usb_send_message_to_dev(usb_dev, val);
    hcc_bus_wake_unlock(bus);

    return ret;
}

static int32_t oal_usb_read_d2h_message(struct oal_usb_dev *usb_dev, uint32_t *message)
{
    struct oal_usb_xfer_param *param = NULL;
    int32_t ret;
    uint32_t *buf = NULL;
    uint32_t buf_len = sizeof(*message);

    buf = oal_memalloc(buf_len);
    if (buf == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "alloc buffer failed");
        return -OAL_ENOMEM;
    }

    param = oal_usb_alloc_xfer_param(usb_dev, (uint8_t *)buf, buf_len, TRANSFER_DEFAULT_TIMEOUT, USB_XFER_DMA,
                                     oal_usb_rx_msg_complete, message);

    ret = oal_usb_intr_rx(param);
    if (ret != OAL_SUCC) {
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "rx fail");
    } else {
        *message = *buf;
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "rx succ msg[%u]", *message);
    }
    oal_free(buf);
    return ret;
}

int32_t oal_usb_gpio_rx_data(hcc_bus *bus)
{
    int32_t ret;
    uint32_t message;
    struct oal_usb_dev_container *usb_dev_container = NULL;
    struct oal_usb_dev *usb_dev = NULL;

    usb_dev_container = (struct oal_usb_dev_container *)bus->data;
    if (usb_dev_container == NULL) {
        return -OAL_EINVAL;
    }

    usb_dev = usb_dev_container->usb_devices[OAL_USB_INTERFACE_INTR][OAL_USB_CHAN_0];
    if (usb_dev == NULL) {
        return -OAL_EINVAL;
    }

    forever_loop()
    {
        hcc_bus_wake_lock(bus);
        ret = oal_usb_read_d2h_message(usb_dev, &message);
        hcc_bus_wake_unlock(bus);

        if (ret != OAL_SUCC) {
            break;
        }

        oal_print_hi11xx_log(HI11XX_LOG_INFO, "recv message: %u", message);

        if (message < D2H_MSG_COUNT) {
            /* standard message */
            bus->msg[message].count++;
            bus->last_msg = message;
            bus->msg[message].cpu_time = cpu_clock(UINT_MAX);
            if (bus->msg[message].msg_rx) {
                bus->msg[message].msg_rx(bus->msg[message].data);
            } else {
                oal_print_hi11xx_log(HI11XX_LOG_INFO, "usb message :%u no callback functions", message);
            }
        }
    }

    return OAL_SUCC;
}
