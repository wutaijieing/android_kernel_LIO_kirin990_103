

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>

#include "oal_hcc_host_if.h"
#include "oal_usb_driver.h"
#include "oal_usb_host.h"
#include "oal_usb_host_if.h"
#include "oal_usb_linux.h"
#include "oal_usb_msg.h"
#include "oal_usb_netbuf.h"
#include "oal_usb_patch.h"

#define OAL_USB_TRANSFER_SIZE 2048

static oal_completion g_usb_probe_complete; /* probe complete flag */
static struct oal_usb_dev_container *g_oal_usb_dev_container = NULL;

static int32_t oal_usb_power_action(hcc_bus *bus, hcc_bus_power_action_type action);
static int32_t oal_usb_reinit(hcc_bus *bus);

static int32_t oal_usb_wakeup_complete(hcc_bus *bus);
static void oal_usb_task_init(struct oal_usb_dev_container *usb_dev_container);

static void oal_usb_probe_complete(void);

static hcc_bus_opt_ops g_usb_opt_ops = { .get_bus_state = NULL,
                                         .disable_bus_state = NULL,
                                         .enable_bus_state = NULL,
                                         .rx_netbuf_list = oal_usb_rx_netbuf,
                                         .tx_netbuf_list = oal_usb_tx_netbuf,
                                         .send_msg = oal_usb_send_msg,
                                         .lock = NULL,
                                         .unlock = NULL,
                                         .sleep_request = NULL,
                                         .sleep_request_host = NULL,
                                         .wakeup_request = NULL,
                                         .get_sleep_state = NULL,
                                         .wakeup_complete = oal_usb_wakeup_complete,
                                         .rx_int_mask = NULL,
                                         .power_action = oal_usb_power_action,
                                         .reinit = oal_usb_reinit,
                                         .deinit = NULL,
                                         .wlan_gpio_handler = NULL,
                                         .flowctrl_gpio_handler = NULL,
                                         .wlan_gpio_rxdata_proc = oal_usb_gpio_rx_data,
                                         .patch_read = oal_usb_patch_read,
                                         .patch_write = oal_usb_patch_write,
                                         .bindcpu = NULL,
                                         .chip_info = NULL,
                                         .print_trans_info = NULL,
                                         .reset_trans_info = NULL,
                                         .pending_signal_check = NULL,
                                         .pending_signal_process = NULL };

struct oal_usb_dev_ops_map {
    uint32_t type;
    uint32_t chan;
    struct oal_usb_dev_ops dev_ops;
};

static struct oal_usb_dev_ops_map g_usb_dev_ops_map[] = {
    {
        .type = OAL_USB_INTERFACE_BULK,
        .chan = OAL_USB_CHAN_1,
        .dev_ops = {
            .usb_dev_init = usb_netbuf_dev_init,
        }
    },
    {
        .type = OAL_USB_INTERFACE_INTR,
        .chan = OAL_USB_CHAN_0,
        .dev_ops = {
            .usb_dev_init = usb_msg_dev_init,
        }
    }
};

static void oal_usb_device_register(struct oal_usb_dev *usb_dev, enum OAL_USB_INTERFACE type,
                                    enum OAL_USB_CHAN_INDEX chan)
{
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "type[%u] chan[%u] dev_addr[0x%p]", type, chan, usb_dev);
    g_oal_usb_dev_container->usb_devices[type][chan] = usb_dev;
    usb_dev->usb_dev_container = g_oal_usb_dev_container;

    if (g_oal_usb_dev_container->usb_dev_ops[type][chan] != NULL) {
        if (g_oal_usb_dev_container->usb_dev_ops[type][chan]->usb_dev_init != NULL) {
            g_oal_usb_dev_container->usb_dev_ops[type][chan]->usb_dev_init(usb_dev);
        }
    }
}

static void oal_usb_device_deregister(enum OAL_USB_INTERFACE type, enum OAL_USB_CHAN_INDEX chan)
{
    g_oal_usb_dev_container->usb_devices[type][chan] = NULL;
}

void *oal_usb_device_get(enum OAL_USB_INTERFACE type, enum OAL_USB_CHAN_INDEX chan)
{
    return g_oal_usb_dev_container->usb_devices[type][chan];
}

static int32_t oal_usb_hcc_bus_power_patch_launch(hcc_bus *bus)
{
    struct oal_usb_dev_container *usb_dev_container = (struct oal_usb_dev_container *)bus->data;
    unsigned int timeout = hi110x_get_emu_timeout(HOST_WAIT_BOTTOM_INIT_TIMEOUT);

    oal_wlan_gpio_intr_enable(hbus_to_dev(bus), OAL_TRUE);

    /* 第一个中断有可能在中断使能之前上报，强制调度一次RX Thread */
    up(&bus->rx_sema);

    if (oal_wait_for_completion_timeout(&bus->st_device_ready,
                                        (uint32_t)oal_msecs_to_jiffies(timeout)) == 0) {
        oal_io_print(KERN_ERR "wait device ready timeout... %d ms \n", timeout);
        up(&bus->rx_sema);
        if (oal_wait_for_completion_timeout(&bus->st_device_ready,
                                            (uint32_t)oal_msecs_to_jiffies(5000)) == 0) {
            oal_io_print(KERN_ERR "retry 5 second hold, still timeout");
            return -OAL_ETIMEDOUT;
        } else {
            /* 强制调度成功，说明有可能是GPIO中断未响应 */
            oal_io_print(KERN_WARNING "[E]retry succ, maybe gpio interrupt issue");
            declare_dft_trace_key_info("usb gpio int issue", OAL_DFT_TRACE_FAIL);
        }
    }

    hcc_enable(hbus_to_hcc(bus), OAL_TRUE);
    oal_usb_task_init(usb_dev_container);
    return OAL_SUCC;
}

static int32_t oal_usb_power_action(hcc_bus *bus, hcc_bus_power_action_type action)
{
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "action type [%d]", action);

    if (action == HCC_BUS_POWER_PATCH_LOAD_PREPARE) {
        /* close hcc */
        hcc_disable(hbus_to_hcc(bus), OAL_TRUE);
        oal_init_completion(&bus->st_device_ready);
        oal_wlan_gpio_intr_enable(hbus_to_dev(bus), OAL_FALSE);
    }

    if (action == HCC_BUS_POWER_PATCH_LAUCH) {
        if (oal_usb_hcc_bus_power_patch_launch(bus) != OAL_SUCC) {
            return -OAL_ETIMEDOUT;
        }
    }
    return OAL_SUCC;
}

static int32_t oal_usb_reinit(hcc_bus *bus)
{
    struct oal_usb_dev_container *usb_dev_container = NULL;

    usb_dev_container = (struct oal_usb_dev_container *)bus->data;
    if (usb_dev_container == NULL) {
        return -OAL_EINVAL;
    }

    sema_init(&bus->sr_wake_sema, 1);
    hcc_bus_disable_state(bus, OAL_BUS_STATE_ALL);

    /* trigger device usb reset / link down - up */
    hcc_bus_enable_state(bus, OAL_BUS_STATE_ALL);
    return OAL_SUCC;
}

static void oal_usb_host_init(void)
{
    struct oal_usb_dev_container *usb_dev_container = NULL;
    struct oal_usb_dev_cb usb_dev_cb;
    int32_t i;

    usb_dev_container = oal_memalloc(sizeof(*usb_dev_container));
    if (usb_dev_container == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "usb_dev_container is null");
        return;
    }
    memset_s(usb_dev_container, sizeof(*usb_dev_container), 0, sizeof(*usb_dev_container));

    g_oal_usb_dev_container = usb_dev_container;

    /* 注册rx thread补充处理函数 */
    for (i = 0; i < oal_array_size(g_usb_dev_ops_map); ++i) {
        uint32_t type = g_usb_dev_ops_map[i].type;
        uint32_t chan = g_usb_dev_ops_map[i].chan;
        usb_dev_container->usb_dev_ops[type][chan] = &g_usb_dev_ops_map[i].dev_ops;
    }

    usb_dev_cb.oal_usb_dev_register = oal_usb_device_register;
    usb_dev_cb.oal_usb_dev_deregister = oal_usb_device_deregister;
    usb_dev_cb.oal_usb_dev_probe_complete = oal_usb_probe_complete;
    oal_usb_set_dev_cb(&usb_dev_cb);
}

static void oal_usb_sched_rx_threads(struct oal_usb_dev_container *usb_dev_container)
{
    struct oal_usb_dev *usb_dev = NULL;
    int32_t i, j;

    for (i = 0; i < OAL_USB_INTERFACE_NUMS; ++i) {
        for (j = 0; j < OAL_USB_CHAN_NUMS; ++i) {
            usb_dev = usb_dev_container->usb_devices[i][j];
            if (usb_dev == NULL || usb_dev->rx_task == NULL) {
                continue;
            }
            oal_usb_sched_rx_thread(usb_dev);
        }
    }
}

static int32_t oal_usb_wakeup_complete(hcc_bus *bus)
{
    oal_usb_sched_rx_threads((struct oal_usb_dev_container *)bus->data);
    return OAL_SUCC;
}

static void oal_usb_task_init(struct oal_usb_dev_container *usb_dev_container)
{
    struct oal_usb_dev *usb_dev = NULL;
    int32_t i, j;

    for (i = 0; i < OAL_USB_INTERFACE_NUMS; ++i) {
        for (j = 0; j < OAL_USB_CHAN_NUMS; ++j) {
            usb_dev = usb_dev_container->usb_devices[i][j];
            if (usb_dev == NULL) {
                continue;
            }
            oal_usb_create_rx_thread(usb_dev);
        }
    }
}

static void oal_usb_init(void)
{
    oal_usb_host_init();
    oal_usb_driver_init();
}

static hcc_bus *oal_usb_bus_init(struct oal_usb_dev_container *usb_dev_container)
{
    int32_t ret;
    hcc_bus *bus = NULL;

    bus = hcc_alloc_bus();
    if (bus == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "alloc usb hcc bus failed, size:%u", (uint32_t)sizeof(hcc_bus));
        return NULL;
    }

    bus->bus_type = HCC_BUS_USB;
    bus->bus_id = 0x0;
    bus->dev_id = HCC_EP_GT_DEV;

    bus->opt_ops = &g_usb_opt_ops;

    bus->cap.align_size[HCC_TX] = sizeof(uint32_t);
    bus->cap.align_size[HCC_RX] = sizeof(uint32_t);
    bus->cap.max_trans_size = OAL_USB_TRANSFER_SIZE;

    bus->data = (void *)usb_dev_container;

    ret = hcc_add_bus(bus, "usb");
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "add usb bus failed, ret=%d", ret);
        hcc_free_bus(bus);
        return NULL;
    }

    usb_dev_container->bus = bus;
    return bus;
}

static void oal_usb_probe_complete(void)
{
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "probe complete");
    oal_complete(&g_usb_probe_complete);
}

static int32_t oal_usb_trigger_probe(void)
{
    hcc_bus *bus = NULL;

    init_completion(&g_usb_probe_complete);
    oal_usb_init();

    bus = oal_usb_bus_init(g_oal_usb_dev_container);

    hcc_bus_enable_state(bus, OAL_BUS_STATE_ALL);

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "start to register usb module");

    /* wakeup in probe */
    if (wait_for_completion_timeout(&g_usb_probe_complete, 10 * HZ) == 0) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "usb enum timeout");
        return -OAL_EFAIL;
    }
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hisi usb load sucuess, usb enum done.");
    return OAL_SUCC;
}

int32_t oal_wifi_platform_load_usb(void)
{
    return oal_usb_trigger_probe();
}
