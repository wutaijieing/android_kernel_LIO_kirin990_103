

#include "oal_usb_driver.h"
#include "oal_usb_host_if.h"

/* Define these values to match your devices */
#define USB_VENDOR_ID_HI1161     0x12d1
#define USB_PRODUCT_ID_1161_HISI 0x1443

#define oal_usb_get_type(protocol) (((protocol)&USB_INTERFACE_TYPE_MASK) >> USB_INTERFACE_TYPE_SHIFT)
#define oal_usb_get_chan(protocol) (((protocol)&USB_INTERFACE_CHAN_MASK) >> USB_INTERFACE_CHAN_SHIFT)

/* table of devices that work with this driver */
static const struct usb_device_id g_oal_usb_ids[] = { { USB_DEVICE(USB_VENDOR_ID_HI1161, USB_PRODUCT_ID_1161_HISI) },
                                                      {} };
MODULE_DEVICE_TABLE(usb, g_oal_usb_ids);

static char *g_usb_xfer_type[OAL_USB_INTERFACE_NUMS] = {
    [OAL_USB_INTERFACE_CTRL] = "ctrl",
    [OAL_USB_INTERFACE_ISOC] = "isoc",
    [OAL_USB_INTERFACE_BULK] = "bulk",
    [OAL_USB_INTERFACE_INTR] = "intr",
};

static struct oal_usb_dev_cb g_oal_usb_dev_cb;

void oal_usb_set_dev_cb(struct oal_usb_dev_cb *usb_dev_cb)
{
    g_oal_usb_dev_cb.oal_usb_dev_register = usb_dev_cb->oal_usb_dev_register;
    g_oal_usb_dev_cb.oal_usb_dev_deregister = usb_dev_cb->oal_usb_dev_deregister;
    g_oal_usb_dev_cb.oal_usb_dev_probe_complete = usb_dev_cb->oal_usb_dev_probe_complete;
}

static void pr_interface(struct usb_interface_descriptor *desc)
{
    oal_io_print("====INTERFACE====" NEWLINE);
    oal_io_print("bLength [0x%x]" NEWLINE, desc->bLength);
    oal_io_print("bDescriptorType [0x%x]" NEWLINE, desc->bDescriptorType);
    oal_io_print("bInterfaceNumber [0x%x]" NEWLINE, desc->bInterfaceNumber);
    oal_io_print("bAlternateSetting [0x%x]" NEWLINE, desc->bAlternateSetting);
    oal_io_print("bNumEndpoints [0x%x]" NEWLINE, desc->bNumEndpoints);
    oal_io_print("bInterfaceClass [0x%x]" NEWLINE, desc->bInterfaceClass);
    oal_io_print("bInterfaceSubClass [0x%x]" NEWLINE, desc->bInterfaceSubClass);
    oal_io_print("bInterfaceProtocol [0x%x]" NEWLINE, desc->bInterfaceProtocol);
    oal_io_print("iInterface [0x%x]" NEWLINE, desc->iInterface);
    oal_io_print("=================" NEWLINE);
}

static void pr_endpoint(struct usb_endpoint_descriptor *desc)
{
    oal_io_print("====EDNPOINT====" NEWLINE);
    oal_io_print("bLength [0x%x]" NEWLINE, desc->bLength);
    oal_io_print("bDescriptorType [0x%x]" NEWLINE, desc->bDescriptorType);
    oal_io_print("bEndpointAddress [0x%x]" NEWLINE, desc->bEndpointAddress);
    oal_io_print("bmAttributes [0x%x]" NEWLINE, desc->bmAttributes);
    oal_io_print("wMaxPacketSize [0x%x]" NEWLINE, desc->wMaxPacketSize);
    oal_io_print("bInterval [0x%x]" NEWLINE, desc->bInterval);
    oal_io_print("================" NEWLINE);
}

static int32_t oal_usb_match_interface(struct usb_interface *interface)
{
    uint32_t type = oal_usb_get_type(interface->cur_altsetting->desc.bInterfaceProtocol);
    uint32_t chan = oal_usb_get_chan(interface->cur_altsetting->desc.bInterfaceProtocol);

    if (interface->cur_altsetting->desc.bInterfaceClass != USB_INTERFACE_CLASS) {
        return -OAL_EINVAL;
    }

    if (type >= OAL_USB_INTERFACE_NUMS || type == OAL_USB_INTERFACE_CTRL) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "unknow type[%u]", type);
        return -OAL_EINVAL;
    }
    if (chan >= OAL_USB_CHAN_NUMS) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "invaild chan[%u]", chan);
        return -OAL_EINVAL;
    }

    return OAL_SUCC;
}

static int32_t oal_usb_bulk_alloc(struct oal_usb_dev *usb_dev, struct usb_host_interface *iface_desc)
{
    struct usb_endpoint_descriptor *endpoint = NULL;
    int32_t i;

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "bNumEndpoints[%u]", iface_desc->desc.bNumEndpoints);
    for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
        endpoint = &iface_desc->endpoint[i].desc;

        if (usb_dev->ep_info.in_ep_addr == 0 && usb_endpoint_is_bulk_in(endpoint)) {
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "discover bulk in endpoint, addr[0x%x]", endpoint->bEndpointAddress);
            usb_dev->ep_info.in_ep_addr = endpoint->bEndpointAddress;
            usb_dev->ep_info.in_max_packet = usb_endpoint_maxp(endpoint);
        }
        if (usb_dev->ep_info.out_ep_addr == 0 && usb_endpoint_is_bulk_out(endpoint)) {
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "discover bulk out endpoint addr[0x%x]", endpoint->bEndpointAddress);
            usb_dev->ep_info.out_ep_addr = endpoint->bEndpointAddress;
            usb_dev->ep_info.out_max_packet = usb_endpoint_maxp(endpoint);
        }
    }
    return OAL_SUCC;
}

static int32_t oal_usb_intr_alloc(struct oal_usb_dev *usb_dev, struct usb_host_interface *iface_desc)
{
    struct usb_endpoint_descriptor *endpoint = NULL;
    int32_t i;

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "bNumEndpoints[%u]", iface_desc->desc.bNumEndpoints);
    for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
        endpoint = &iface_desc->endpoint[i].desc;

        if (usb_dev->ep_info.in_ep_addr == 0 && usb_endpoint_is_int_in(endpoint)) {
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "discover intr in endpoint addr[0x%x]", endpoint->bEndpointAddress);
            usb_dev->ep_info.in_ep_addr = endpoint->bEndpointAddress;
            usb_dev->ep_info.in_interval = endpoint->bInterval;
            usb_dev->ep_info.in_max_packet = usb_endpoint_maxp(endpoint);
        }
        if (usb_dev->ep_info.out_ep_addr == 0 && usb_endpoint_is_int_out(endpoint)) {
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "discover intr out endpoint addr[0x%x]", endpoint->bEndpointAddress);
            usb_dev->ep_info.out_ep_addr = endpoint->bEndpointAddress;
            usb_dev->ep_info.out_interval = endpoint->bInterval;
            usb_dev->ep_info.out_max_packet = usb_endpoint_maxp(endpoint);
        }
    }
    return OAL_SUCC;
}

static int32_t oal_usb_isoc_alloc(struct oal_usb_dev *usb_dev, struct usb_host_interface *iface_desc)
{
    struct usb_endpoint_descriptor *endpoint = NULL;
    int32_t i;

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "bNumEndpoints[%u]", iface_desc->desc.bNumEndpoints);
    for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
        endpoint = &iface_desc->endpoint[i].desc;

        if (usb_dev->ep_info.in_ep_addr == 0 && usb_endpoint_is_isoc_in(endpoint)) {
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "discover isoc in endpoint addr[0x%x]", endpoint->bEndpointAddress);
            usb_dev->ep_info.in_ep_addr = endpoint->bEndpointAddress;
            usb_dev->ep_info.in_interval = endpoint->bInterval;
            usb_dev->ep_info.in_max_packet = usb_endpoint_maxp(endpoint);
        }
        if (usb_dev->ep_info.out_ep_addr == 0 && usb_endpoint_is_isoc_out(endpoint)) {
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "discover isoc out endpoint addr[0x%x]", endpoint->bEndpointAddress);
            usb_dev->ep_info.out_ep_addr = endpoint->bEndpointAddress;
            usb_dev->ep_info.out_interval = endpoint->bInterval;
            usb_dev->ep_info.out_max_packet = usb_endpoint_maxp(endpoint);
        }
    }
    return OAL_SUCC;
}

static struct oal_usb_dev *oal_usb_alloc(struct usb_interface *interface, enum OAL_USB_INTERFACE type)
{
    struct oal_usb_dev *usb_dev = NULL;
    struct usb_host_interface *iface_desc = NULL;
    int32_t ret;

    usb_dev = oal_memalloc(sizeof(struct oal_usb_dev));
    if (usb_dev == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "alloc oal_usb_dev failed [%d]", (int32_t)sizeof(struct oal_usb_dev));
        return NULL;
    }
    memset_s((void *)usb_dev, sizeof(struct oal_usb_dev), 0, sizeof(struct oal_usb_dev));

    usb_dev->udev = usb_get_dev(interface_to_usbdev(interface));
    usb_dev->interface = interface;

    pr_interface(&interface->cur_altsetting->desc);
    pr_endpoint(&interface->cur_altsetting->endpoint[0].desc);
    pr_endpoint(&interface->cur_altsetting->endpoint[1].desc);

    iface_desc = interface->cur_altsetting;
    switch (type) {
        case OAL_USB_INTERFACE_BULK:
            ret = oal_usb_bulk_alloc(usb_dev, iface_desc);
            break;
        case OAL_USB_INTERFACE_INTR:
            ret = oal_usb_intr_alloc(usb_dev, iface_desc);
            break;
        case OAL_USB_INTERFACE_ISOC:
            ret = oal_usb_isoc_alloc(usb_dev, iface_desc);
            break;
        default:
            ret = -OAL_EINVAL;
    }
    if (ret != OAL_SUCC) {
        goto error;
    }

    usb_set_intfdata(interface, usb_dev);
    return usb_dev;
error:
    oal_free(usb_dev);
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "alloc fail");

    return NULL;
}

void oal_usb_create_rx_thread(struct oal_usb_dev *usb_dev)
{
    uint32_t type = oal_usb_get_type(usb_dev->interface->cur_altsetting->desc.bInterfaceProtocol);
    uint32_t chan = oal_usb_get_chan(usb_dev->interface->cur_altsetting->desc.bInterfaceProtocol);
    char *task_name = NULL;

    if (usb_dev->rx_task != NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "usb rx thread already exist, type [%s] chan [%d]", g_usb_xfer_type[type],
                             chan);
        return;
    }
    if (usb_dev->rx_thread == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "usb rx thread is NULL, type [%s] chan [%d]", g_usb_xfer_type[type], chan);
        return;
    }

    oal_atomic_set(&usb_dev->rx_cond, 0);
    oal_wait_queue_init_head(&usb_dev->rx_wq);

    task_name = oal_memalloc(TASK_COMM_LEN);
    if (task_name == NULL) {
        task_name = "usb_rx_unknow";
    } else {
        usb_dev->task_name = task_name;
        snprintf_s(task_name, TASK_COMM_LEN, TASK_COMM_LEN - 1, "usb_rx_%s_%d", g_usb_xfer_type[type], chan);
    }

    usb_dev->rx_task = oal_thread_create(usb_dev->rx_thread, (void *)usb_dev, NULL, task_name, SCHED_RR,
                                         OAL_BUS_TEST_INIT_PRIORITY, -1);
    if (usb_dev->rx_task == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "usb rx task create fail, type [%s] chan [%d]", g_usb_xfer_type[type],
                             chan);
    }
    oal_usb_sched_rx_thread(usb_dev);
}

void oal_usb_distory_rx_thread(struct oal_usb_dev *usb_dev)
{
    uint32_t type = oal_usb_get_type(usb_dev->interface->cur_altsetting->desc.bInterfaceProtocol);
    uint32_t chan = oal_usb_get_chan(usb_dev->interface->cur_altsetting->desc.bInterfaceProtocol);

    if (usb_dev->rx_task != NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "stop rx thread type [%s] chan [%d]", g_usb_xfer_type[type], chan);
        oal_thread_stop(usb_dev->rx_task, NULL);
        usb_dev->rx_task = NULL;
    }
    if (usb_dev->task_name != NULL) {
        oal_free(usb_dev->task_name);
        usb_dev->task_name = NULL;
    }
}

static int oal_usb_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    struct oal_usb_dev *usb_dev = NULL;
    uint32_t type;
    uint32_t chan;

    if (oal_usb_match_interface(interface) < 0) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "match fail");
        return -EINVAL;
    }
    type = oal_usb_get_type(interface->cur_altsetting->desc.bInterfaceProtocol);
    chan = oal_usb_get_chan(interface->cur_altsetting->desc.bInterfaceProtocol);
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "oal_usb_probe type[%s] chan[%u]" NEWLINE, g_usb_xfer_type[type], chan);

    usb_dev = oal_usb_alloc(interface, type);
    if (usb_dev == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "failed to alloc usb_dev!");
        return -EINVAL;
    }

    mutex_init(&usb_dev->rx_mutex);
    init_usb_anchor(&usb_dev->submitted);
    oal_init_completion(&usb_dev->rx_wait);
    oal_init_completion(&usb_dev->tx_wait);

    if (g_oal_usb_dev_cb.oal_usb_dev_register != NULL) {
        g_oal_usb_dev_cb.oal_usb_dev_register(usb_dev, type, chan);
    }
    if (g_oal_usb_dev_cb.oal_usb_dev_probe_complete != NULL) {
        g_oal_usb_dev_cb.oal_usb_dev_probe_complete();
    }
    oal_usb_create_rx_thread(usb_dev);
    return 0;
}

static void oal_usb_free(struct oal_usb_dev *usb_dev)
{
    usb_kill_anchored_urbs(&usb_dev->submitted);
    usb_put_dev(usb_dev->udev);
}

static void oal_usb_disconnect(struct usb_interface *interface)
{
    struct oal_usb_dev *usb_dev = NULL;
    uint32_t type = oal_usb_get_type(interface->cur_altsetting->desc.bInterfaceProtocol);
    uint32_t chan = oal_usb_get_chan(interface->cur_altsetting->desc.bInterfaceProtocol);

    usb_dev = usb_get_intfdata(interface);
    usb_set_intfdata(interface, NULL);
    if (usb_dev == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "usb_dev is null");
        return;
    }

    if (g_oal_usb_dev_cb.oal_usb_dev_deregister != NULL) {
        g_oal_usb_dev_cb.oal_usb_dev_deregister(type, chan);
    }
    oal_usb_distory_rx_thread(usb_dev);
    usb_dev->interface = NULL;
    oal_usb_free(usb_dev);
}

static void oal_usb_draw_down(struct oal_usb_dev *usb_dev)
{
    int time;

    time = usb_wait_anchor_empty_timeout(&usb_dev->submitted, 1000);
    if (time == 0) {
        usb_kill_anchored_urbs(&usb_dev->submitted);
    }
}

static int32_t oal_usb_suspend(struct usb_interface *interface, pm_message_t message)
{
    struct oal_usb_dev *usb_dev = NULL;

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "usb suspend");
    usb_dev = usb_get_intfdata(interface);
    if (usb_dev == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "usb_dev is null");
        return OAL_SUCC;
    }

    /* wait urb complete or timeout */
    oal_usb_draw_down(usb_dev);

    return OAL_SUCC;
}

static int32_t oal_usb_resume(struct usb_interface *interface)
{
    struct oal_usb_dev *usb_dev = NULL;

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "usb resume");
    usb_dev = usb_get_intfdata(interface);
    if (usb_dev == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "usb_dev is null");
        return OAL_SUCC;
    }

    return OAL_SUCC;
}

static struct usb_driver g_oal_usb_driver = {
    .name = "oal_usb",
    .probe = oal_usb_probe,
    .disconnect = oal_usb_disconnect,
    .suspend = oal_usb_suspend,
    .resume = oal_usb_resume,
    .id_table = g_oal_usb_ids,
};

int32_t oal_usb_driver_init(void)
{
    int32_t ret;

    ret = usb_register(&g_oal_usb_driver);
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "register usb driver failed ret=%d", ret);
    }
    return ret;
}
