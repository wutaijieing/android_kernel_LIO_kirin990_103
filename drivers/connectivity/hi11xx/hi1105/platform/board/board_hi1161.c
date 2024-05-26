

/* 头文件包含 */
#ifdef _PRE_CONFIG_USE_DTS
#include <linux/of.h>
#include <linux/of_gpio.h>
#endif

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/fs.h>

#include "board_hi1161.h"
#include "board.h"
#include "plat_debug.h"
#include "bfgx_dev.h"
#include "oal_hcc_host_if.h"
#include "plat_pm.h"
#include "plat_uart.h"
#include "plat_firmware.h"
#include "plat_pm_gt.h"
#include "oam_ext_if.h"
#include "securec.h"
#include "pcie_linux.h"


STATIC int32_t hi1161_board_power_on(uint32_t ul_subsystem);
STATIC int32_t hi1161_board_power_off(uint32_t ul_subsystem);

#define OAL_WAKEUP_RX_SEM_DALAY 1000

oal_timer_list_stru g_wifi_rx_sem_timer;
oal_timer_list_stru g_gt_rx_sem_timer;
static void wifi_weekup_rx_sem_action(oal_timeout_func_para_t arg)
{
    struct hcc_handler *hcc = hcc_get_handler(HCC_EP_WIFI_DEV);
    hcc_bus* bus = NULL;

    if (hcc == NULL) {
        printk("week up rx sem timer, hcc is null\n");
        return;
    }

    bus = hcc_to_bus(hcc);
    if (bus == NULL) {
        printk("week up rx sem timer, bus is null\n");
        return;
    }
    up(&bus->rx_sema);
    oal_timer_start(&g_wifi_rx_sem_timer, OAL_WAKEUP_RX_SEM_DALAY);
}

static void gt_weekup_rx_sem_action(oal_timeout_func_para_t arg)
{
    struct hcc_handler *hcc = hcc_get_handler(HCC_EP_GT_DEV);
    hcc_bus* bus = NULL;

    if (hcc == NULL) {
        printk("week up rx sem timer, hcc is null\n");
        return;
    }

    bus = hcc_to_bus(hcc);
    if (bus == NULL) {
        printk("week up rx sem timer, bus is null\n");
        return;
    }
    up(&bus->rx_sema);
    oal_timer_start(&g_gt_rx_sem_timer, OAL_WAKEUP_RX_SEM_DALAY);
}

static void rx_sem_timer_init(uint8_t dev_id)
{
    if (dev_id == HCC_EP_WIFI_DEV) {
        oal_timer_init(&g_wifi_rx_sem_timer, 2 * OAL_WAKEUP_RX_SEM_DALAY, wifi_weekup_rx_sem_action, 0);
        oal_timer_add(&g_wifi_rx_sem_timer);
    } else {
        oal_timer_init(&g_gt_rx_sem_timer, 2 * OAL_WAKEUP_RX_SEM_DALAY, gt_weekup_rx_sem_action, 0);
        oal_timer_add(&g_gt_rx_sem_timer);
    }
    printk("rx sem timer init succ\n");
}

STATIC bool gt_is_shutdown(void)
{
    struct gt_pm_s *gt_pm_info = gt_pm_get_drv();
    if (gt_pm_info == NULL) {
        ps_print_err("gt_pm_info is NULL!\n");
        return OAL_TRUE;
    }

    return (gt_pm_info->gt_power_state == POWER_STATE_OPEN) ? OAL_FALSE : OAL_TRUE;
}

STATIC int32_t hi1161_bfgx_download(uint32_t sys)
{
    int32_t ret;
    uint32_t which_cfg = (sys == B_SYS) ? BFGX_CFG : GLE_CFG;

    ret = firmware_download_function(which_cfg, hcc_get_bus(HCC_EP_WIFI_DEV));
    if (ret != BFGX_POWER_SUCCESS) {
        ps_print_err("bfgx download firmware fail, cfg[%d]!\n", which_cfg);
        return (ret == -OAL_EINTR) ? BFGX_POWER_DOWNLOAD_FIRMWARE_INTERRUPT : BFGX_POWER_DOWNLOAD_FIRMWARE_FAIL;
    }
    ps_print_info("bfgx download firmware succ!\n");

    return BFGX_POWER_SUCCESS;
}

STATIC int32_t hi1161_bfgx_dev_power_on(uint32_t sys)
{
    int32_t ret;
    struct ps_core_s *ps_core_d = NULL;
    int32_t error = BFGX_POWER_SUCCESS;
    struct pm_drv_data *pm_data = NULL;
    uint32_t uart = (sys == B_SYS) ? BUART : GUART;

    ps_print_info("%s %d\n", __func__, sys);

    ps_core_d = ps_get_core_reference(uart);
    if ((ps_core_d == NULL) || (ps_core_d->pm_data == NULL)) {
        ps_print_err("ps_core_d is err\n");
        return BFGX_POWER_FAILED;
    }

    pm_data = ps_core_d->pm_data;

    bfgx_gpio_intr_enable(pm_data, OAL_TRUE);

    ret = hi1161_board_power_on(sys);
    if (ret) {
        ps_print_err("hi1161_board_power_on bfg[%d] failed, ret=%d\n", sys, ret);
        error = BFGX_POWER_PULL_POWER_GPIO_FAIL;
        goto board_power_on_fail;
    }

    if (open_tty_drv(ps_core_d) != BFGX_POWER_SUCCESS) {
        ps_print_err("open tty fail!\n");
        error = BFGX_POWER_TTY_OPEN_FAIL;
        goto open_tty_fail;
    }

    error = hi1161_bfgx_download(sys);
    if (error != BFGX_POWER_SUCCESS) {
        goto bfgx_download_fail;
    }

    goto bfgx_power_on_succ;

bfgx_download_fail:
    release_tty_drv(ps_core_d);
open_tty_fail:
    (void)hi1161_board_power_off(sys);
board_power_on_fail:
    bfgx_gpio_intr_enable(pm_data, OAL_FALSE);

bfgx_power_on_succ:
    if (gt_is_shutdown() == OAL_TRUE) {
        board_sys_disable(GT_SYS);
    }

    if (wlan_is_shutdown() == OAL_TRUE) {
        board_sys_disable(W_SYS);
    }
    return error;
}

STATIC int32_t hi1161_bfgx_dev_power_off(uint32_t sys)
{
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data *pm_data = NULL;
    uint32_t uart = (sys == B_SYS) ? BUART : GUART;

    ps_print_info("%s %d\n", __func__, sys);

    ps_core_d = ps_get_core_reference(uart);
    if ((ps_core_d == NULL) || (ps_core_d->pm_data == NULL)) {
        ps_print_err("ps_core_d is err\n");
        return BFGX_POWER_FAILED;
    }

    if (uart_bfgx_close_cmd(ps_core_d) != SUCCESS) {
        /* bfgx self close fail 了，后面也要通过wifi shutdown bcpu */
        ps_print_err("bfgx self close[%d] fail\n", sys);
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_CLOSE, CHR_PLAT_DRV_ERROR_CLOSE_BCPU);
    }

    pm_data = ps_core_d->pm_data;
    bfgx_gpio_intr_enable(pm_data, OAL_FALSE);

    if (wait_bfgx_memdump_complete() != EXCEPTION_SUCCESS) {
        ps_print_err("wait memdump complete failed\n");
    }

    if (release_tty_drv(ps_core_d) != SUCCESS) {
        /* 代码执行到此处，说明六合一所有业务都已经关闭，无论tty是否关闭成功，device都要下电 */
        ps_print_err("wifi off, close tty is err!");
    }

    pm_data->bfgx_dev_state = BFGX_SLEEP;
    pm_data->uart_state = UART_NOT_READY;

    (void)hi1161_board_power_off(sys);

    return BFGX_POWER_SUCCESS;
}

STATIC int32_t hi1161_wlan_power_on(void)
{
    int32_t ret;
    int32_t error;
    struct wlan_pm_s *wlan_pm_info = wlan_pm_get_drv();
    if (wlan_pm_info == NULL) {
        ps_print_err("wlan_pm_info is NULL!\n");
        return -FAILURE;
    }

    ret = hi1161_board_power_on(W_SYS);
    if (ret) {
        ps_print_err("hi1161_board_power_on wlan failed ret=%d\n", ret);
        return -FAILURE;
    }

    hcc_bus_power_action(hcc_get_bus(HCC_EP_WIFI_DEV), HCC_BUS_POWER_PATCH_LOAD_PREPARE);
    error = firmware_download_function(WIFI_CFG, hcc_get_bus(HCC_EP_WIFI_DEV));
    if (error != WIFI_POWER_SUCCESS) {
        ps_print_err("firmware download fail\n");
        if (error == -OAL_EINTR) {
            error = WIFI_POWER_ON_FIRMWARE_DOWNLOAD_INTERRUPT;
        } else if (error == -OAL_ENOPERMI) {
            error = WIFI_POWER_ON_FIRMWARE_FILE_OPEN_FAIL;
        } else {
            error = WIFI_POWER_BFGX_OFF_FIRMWARE_DOWNLOAD_FAIL;
        }
        return error;
    }
    wlan_pm_info->wlan_power_state = POWER_STATE_OPEN;
    hcc_bus_power_action(hcc_get_bus(HCC_EP_WIFI_DEV), HCC_BUS_POWER_PATCH_LAUCH);

    if (gt_is_shutdown() == OAL_TRUE) {
        board_sys_disable(GT_SYS);
    }
    return WIFI_POWER_SUCCESS;
}

STATIC int32_t hi1161_wlan_power_off(void)
{
    struct wlan_pm_s *wlan_pm_info = wlan_pm_get_drv();

    /* 先关闭TX通道 */
    hcc_bus_disable_state(hcc_get_bus(HCC_EP_WIFI_DEV), OAL_BUS_STATE_TX);

    /* wakeup dev,send poweroff cmd to wifi */
    if (wlan_pm_poweroff_cmd() != OAL_SUCC) {
        /* wifi self close 失败了也继续往下执行，uart关闭WCPU，异常恢复推迟到wifi下次open的时候执行 */
        declare_dft_trace_key_info("wlan_poweroff_cmd_fail", OAL_DFT_TRACE_FAIL);
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_CLOSE, CHR_PLAT_DRV_ERROR_CLOSE_WCPU);
    }
    hcc_bus_disable_state(hcc_get_bus(HCC_EP_WIFI_DEV), OAL_BUS_STATE_ALL);

    (void)hi1161_board_power_off(W_SYS);

    if (wlan_pm_info != NULL) {
        wlan_pm_info->wlan_power_state = POWER_STATE_SHUTDOWN;
    }
    return BOARD_SUCC;
}

STATIC int32_t hi1161_gt_power_on(void)
{
    int32_t error;
    struct gt_pm_s *gt_pm_info = gt_pm_get_drv();
    if (gt_pm_info == NULL) {
        ps_print_err("wlan_pm_info is NULL!\n");
        return -FAILURE;
    }
    hcc_bus_power_action(hcc_get_bus(HCC_EP_GT_DEV), HCC_BUS_POWER_PATCH_LOAD_PREPARE);
    error = firmware_download_function(GT_CFG, hcc_get_bus(HCC_EP_GT_DEV));
    if (error != GT_POWER_SUCCESS) {
        ps_print_err("firmware download fail\n");
        if (error == -OAL_EINTR) {
            error = GT_POWER_ON_FIRMWARE_DOWNLOAD_INTERRUPT;
        } else if (error == -OAL_ENOPERMI) {
            error = GT_POWER_ON_FIRMWARE_FILE_OPEN_FAIL;
        } else {
            error = GT_POWER_BFGX_OFF_FIRMWARE_DOWNLOAD_FAIL;
        }
        return error;
    }
    gt_pm_info->gt_power_state = POWER_STATE_OPEN;
    // fpga平台没有gpio, 临时采用timer方式唤醒host消息线程
    rx_sem_timer_init(HCC_EP_GT_DEV);
    hcc_bus_power_action(hcc_get_bus(HCC_EP_GT_DEV), HCC_BUS_POWER_PATCH_LAUCH);
    return GT_POWER_SUCCESS;
}

STATIC int32_t hi1161_gt_power_off(void)
{
    return BOARD_SUCC;
}

STATIC int32_t hi1161_board_w_power_on(void)
{
    int32_t ret;
    if ((bfgx_is_shutdown() == OAL_TRUE) &&
        (gle_is_shutdown() == OAL_TRUE) &&
        (gt_is_shutdown() == OAL_TRUE)) {
        ps_print_info("wifi pull up power_on_enable gpio!\n");
        board_chip_power_on();
    }

    if (gt_is_shutdown() == OAL_TRUE) {
        board_sys_enable(GT_SYS);
    }

    ret = board_sys_enable(W_SYS);
    return ret;
}

STATIC int32_t hi1161_board_b_power_on(void)
{
    if ((wlan_is_shutdown() == OAL_TRUE) &&
        (gle_is_shutdown() == OAL_TRUE) &&
        (gt_is_shutdown() == OAL_TRUE)) {
        ps_print_info("bfgx pull up power_on_enable gpio!\n");
        board_chip_power_on();
    }

    if (wlan_is_shutdown() == OAL_TRUE) {
        if (gt_is_shutdown() == OAL_TRUE) {
            board_sys_enable(GT_SYS);
        }
        board_sys_enable(W_SYS);
    }

    board_sys_enable(B_SYS);
    return  SUCC;
}

STATIC int32_t hi1161_board_g_power_on(void)
{
    if ((wlan_is_shutdown() == OAL_TRUE) &&
        (bfgx_is_shutdown() == OAL_TRUE) &&
        (gt_is_shutdown() == OAL_TRUE)) {
        ps_print_info("bfgx pull up power_on_enable gpio!\n");
        board_chip_power_on();
    }

    if (wlan_is_shutdown() == OAL_TRUE) {
        if (gt_is_shutdown() == OAL_TRUE) {
            board_sys_enable(GT_SYS);
        }
        board_sys_enable(W_SYS);
    }

    board_sys_enable(G_SYS);
    return SUCC;
}


STATIC int32_t hi1161_board_gt_power_on(void)
{
    return board_sys_enable(GT_SYS);
}

STATIC int32_t hi1161_board_power_on(uint32_t ul_subsystem)
{
    if (ul_subsystem == W_SYS) {
        return hi1161_board_w_power_on();
    } else if (ul_subsystem == B_SYS) {
        return hi1161_board_b_power_on();
    } else if (ul_subsystem == G_SYS) {
        return hi1161_board_g_power_on();
    } else if (ul_subsystem == GT_SYS) {
        return hi1161_board_gt_power_on();
    } else {
        ps_print_err("power input system:%d error\n", ul_subsystem);
        return BOARD_FAIL;
    }
}

STATIC void hi1161_board_b_power_off(void)
{
    board_sys_disable(B_SYS);
    if (wlan_is_shutdown() == OAL_TRUE) {
        board_sys_disable(W_SYS);
    }
    if (gle_is_shutdown() == OAL_TRUE) {
        board_sys_disable(G_SYS);
    }
    if (gt_is_shutdown() == OAL_TRUE) {
        board_sys_disable(GT_SYS);
    }

    if ((wlan_is_shutdown() == OAL_TRUE) &&
        (gle_is_shutdown() == OAL_TRUE) &&
        (gt_is_shutdown() == OAL_TRUE)) {
        board_chip_power_off();
        ps_print_info("bfg pull down power_on_enable!\n");
    }
}

STATIC void hi1161_board_g_power_off(void)
{
    const uint32_t gle_soft_en = 0x400007b8;

    if (ssi_write_value32_test(gle_soft_en, 0x0) != BOARD_SUCC) {
        ps_print_err("gle soft en cfg fail\n");
    }

    if (bfgx_is_shutdown() == OAL_TRUE) {
        board_sys_disable(B_SYS);
    }
    if (wlan_is_shutdown() == OAL_TRUE) {
        board_sys_disable(W_SYS);
    }
    if (gt_is_shutdown() == OAL_TRUE) {
        board_sys_disable(GT_SYS);
    }

    if ((wlan_is_shutdown() == OAL_TRUE) &&
        (bfgx_is_shutdown() == OAL_TRUE) &&
        (gt_is_shutdown() == OAL_TRUE)) {
        board_chip_power_off();
        ps_print_info("gle pull down power_on_enable!\n");
    }
}

STATIC void hi1161_board_gt_power_off(void)
{
    board_sys_disable(GT_SYS);
}

STATIC void hi1161_board_w_power_off(void)
{
    board_sys_disable(W_SYS);

    if (gt_is_shutdown() == OAL_TRUE) {
        board_sys_disable(GT_SYS);
    }
    if ((bfgx_is_shutdown() == OAL_TRUE) &&
        (gle_is_shutdown() == OAL_TRUE) &&
        (gt_is_shutdown() == OAL_TRUE)) {
        board_chip_power_off();
        ps_print_info("wifi pull down power_on_enable!\n");
    }
}

STATIC int32_t hi1161_board_power_off(uint32_t ul_subsystem)
{
    if (ul_subsystem == W_SYS) {
        hi1161_board_w_power_off();
    } else if (ul_subsystem == B_SYS) {
        hi1161_board_b_power_off();
    } else if (ul_subsystem == G_SYS) {
        hi1161_board_g_power_off();
    } else if (ul_subsystem == GT_SYS) {
        hi1161_board_gt_power_off();
    } else {
        ps_print_err("power input system:%d error\n", ul_subsystem);
        return -EFAIL;
    }
    return SUCC;
}

STATIC int32_t hi1161_board_power_reset(uint32_t ul_subsystem)
{
    int32_t ret;
    board_sys_disable(B_SYS);
    board_sys_disable(G_SYS);
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    board_sys_disable(W_SYS);
#endif
    board_sys_disable(GT_SYS);
    board_chip_power_off();
    board_chip_power_on();
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    ret = board_sys_enable(W_SYS);
    if (ret != SUCC) {
        return ret;
    }
#endif
    ret = board_sys_enable(B_SYS);
    if (ret != SUCC) {
        return ret;
    }
    ret = board_sys_enable(G_SYS);
    if (ret != SUCC) {
        return ret;
    }
    ret = board_sys_enable(GT_SYS);
    return ret;
}

static int32_t hi1161_board_irq_init(struct platform_device *pdev)
{
#ifdef CONFIG_GPIOLIB
    int32_t irq;
    int32_t gpio;
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    gpio = g_st_board_info.dev_wakeup_host[W_SYS];
    irq = gpio_to_irq(gpio);
    g_st_board_info.wakeup_irq[W_SYS] = irq;

    ps_print_info("wlan_irq is %d\n", g_st_board_info.wakeup_irq[W_SYS]);
#endif

    gpio = g_st_board_info.dev_wakeup_host[B_SYS];
    irq = gpio_to_irq(gpio);
    g_st_board_info.wakeup_irq[B_SYS] = irq;

    ps_print_info("bfgx_irq is %d\n", g_st_board_info.wakeup_irq[B_SYS]);

    gpio = g_st_board_info.dev_wakeup_host[G_SYS];
    irq = gpio_to_irq(gpio);
    g_st_board_info.wakeup_irq[G_SYS] = irq;
    ps_print_info("gle_irq is %d\n", g_st_board_info.wakeup_irq[G_SYS]);
#endif
    return BOARD_SUCC;
}

STATIC void board_callback_init_hi1161_power(void)
{
    g_st_board_info.bd_ops.bfgx_dev_power_on = hi1161_bfgx_dev_power_on;
    g_st_board_info.bd_ops.bfgx_dev_power_off = hi1161_bfgx_dev_power_off;
    g_st_board_info.bd_ops.wlan_power_on = hi1161_wlan_power_on;
    g_st_board_info.bd_ops.wlan_power_off = hi1161_wlan_power_off;
    g_st_board_info.bd_ops.gt_power_on = hi1161_gt_power_on;
    g_st_board_info.bd_ops.gt_power_off = hi1161_gt_power_off;
    g_st_board_info.bd_ops.board_power_on = hi1161_board_power_on;
    g_st_board_info.bd_ops.board_power_off = hi1161_board_power_off;
    g_st_board_info.bd_ops.board_power_reset = hi1161_board_power_reset;
    g_st_board_info.bd_ops.board_irq_init = hi1161_board_irq_init;
#ifndef _PRE_GT_SUPPORT
    g_st_board_info.is_gt_disable = 1;
#else
    g_st_board_info.is_gt_disable = 0;
#endif
#ifndef _PRE_WIFI_SUPPORT
    g_st_board_info.is_wifi_disable = 1;
#endif
#if (defined _PRE_WLAN_CHIP_VERSION) && (_PRE_WLAN_CHIP_VERSION == _PRE_WLAN_CHIP_ASIC)
    g_st_board_info.is_asic = VERSION_ASIC;
#else
    g_st_board_info.is_asic = VERSION_FPGA;
#endif
}

void board_info_init_hi1161(void)
{
    board_callback_init_dts();
    board_callback_init_hi1161_power();
}
