

/* 头文件包含 */
#define HI11XX_LOG_MODULE_NAME     "[BISHENG_BOARD]"
#define HI11XX_LOG_MODULE_NAME_VAR bisheng_board_loglevel
#include "board_bisheng.h"

#include "chr_user.h"
#include "plat_debug.h"
#include "bfgx_dev.h"
#include "plat_pm.h"
#include "plat_uart.h"
#include "plat_firmware.h"

#ifdef BFGX_UART_DOWNLOAD_SUPPORT
#include "bfgx_data_parse.h"
#include "wireless_patch.h"
#endif

STATIC int32_t bisheng_board_w_power_on(void)
{
    int32_t ret = SUCC;
    if (bfgx_is_shutdown() && gle_is_shutdown()) {
        ps_print_info("wifi pull up power_on_enable gpio!\n");
        board_chip_power_on();
        if (board_sys_enable(W_SYS)) {
            ret = WIFI_POWER_BFGX_OFF_PULL_WLEN_FAIL;
        }
        board_sys_enable(B_SYS);
        board_sys_enable(G_SYS);
    } else {
        if (board_sys_enable(W_SYS)) {
            ret = WIFI_POWER_BFGX_ON_PULL_WLEN_FAIL;
        }
    }
    return ret;
}

STATIC int32_t bisheng_board_b_power_on(void)
{
    int32_t ret = SUCC;
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    if (wlan_is_shutdown() && gle_is_shutdown()) {
        ps_print_info("bfgx pull up power_on_enable gpio!\n");
        board_chip_power_on();
        ret = board_sys_enable(W_SYS);
    }
#else
    board_chip_power_on();
#endif
    board_sys_enable(B_SYS);
    return  ret;
}

STATIC int32_t bisheng_board_g_power_on(void)
{
    int32_t ret = SUCC;
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    if (wlan_is_shutdown() && bfgx_is_shutdown()) {
        ps_print_info("bfgx pull up power_on_enable gpio!\n");
        board_chip_power_on();
        ret = board_sys_enable(W_SYS);
    }
#else
    board_chip_power_on();
#endif
    board_sys_enable(G_SYS);
    return ret;
}

STATIC int32_t bisheng_board_power_on(uint32_t ul_subsystem)
{
    if (ul_subsystem == W_SYS) {
        return bisheng_board_w_power_on();
    } else if (ul_subsystem == B_SYS) {
        return bisheng_board_b_power_on();
    } else if (ul_subsystem == G_SYS) {
        return bisheng_board_g_power_on();
    } else {
        ps_print_err("power input system:%d error\n", ul_subsystem);
        return WIFI_POWER_FAIL;
    }
}

STATIC void bisheng_board_w_power_off(void)
{
    board_sys_disable(W_SYS);
    if (bfgx_is_shutdown() && gle_is_shutdown()) {
        ps_print_info("wifi pull down power_on_enable!\n");
        board_sys_disable(B_SYS);
        board_sys_disable(G_SYS);
        board_chip_power_off();
    }
}

STATIC void bisheng_board_b_power_off(void)
{
    board_sys_disable(B_SYS);
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    if (wlan_is_shutdown() && gle_is_shutdown()) {
        ps_print_info("bfgx pull down power_on_enable!\n");
        board_sys_disable(W_SYS);
        board_sys_disable(G_SYS);
        board_chip_power_off();
    }
#else
    board_chip_power_off();
#endif
}

STATIC void bisheng_board_g_power_off(void)
{
    board_sys_disable(G_SYS);
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    if (wlan_is_shutdown() && gle_is_shutdown()) {
        ps_print_info("bfgx pull down power_on_enable!\n");
        board_sys_disable(B_SYS);
        board_sys_disable(W_SYS);
        board_chip_power_off();
    }
#else
    board_chip_power_off();
#endif
}

STATIC int32_t bisheng_board_power_off(uint32_t ul_subsystem)
{
    if (ul_subsystem == W_SYS) {
        bisheng_board_w_power_off();
    } else if (ul_subsystem == B_SYS) {
        bisheng_board_b_power_off();
    } else if (ul_subsystem == G_SYS) {
        bisheng_board_g_power_off();
    } else {
        ps_print_err("power input system:%d error\n", ul_subsystem);
        return -EFAIL;
    }

    return SUCC;
}

STATIC int32_t bisheng_board_power_reset(uint32_t ul_subsystem)
{
    int32_t ret;
    board_sys_disable(B_SYS);
    board_sys_disable(G_SYS);
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    board_sys_disable(W_SYS);
#endif
    board_chip_power_off();
    board_chip_power_on();
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    ret = board_sys_enable(W_SYS);
#endif
    ret = board_sys_enable(B_SYS);
    ret = board_sys_enable(G_SYS);
    return ret;
}

void bisheng_bfgx_subsys_reset(void)
{
    // 维测, 判断之前的gpio状态
    ps_print_info("bfgx wkup host gpio val %d\n", board_get_dev_wkup_host_state(B_SYS));

    board_sys_disable(B_SYS);
    mdelay(100);
    board_sys_enable(B_SYS);
}

int32_t bisheng_wifi_subsys_reset(void)
{
    int32_t ret;
    board_sys_disable(W_SYS);
    mdelay(10);
    ret = board_sys_enable(W_SYS);
    return ret;
}

#ifndef BFGX_UART_DOWNLOAD_SUPPORT
STATIC int32_t bisheng_bfgx_download_by_wifi(void)
{
    int32_t ret;
    int32_t error = BFGX_POWER_SUCCESS;
    struct pm_top* pm_top_data = pm_get_top();

    if (wlan_is_shutdown()) {
        ret = firmware_download_function(BFGX_CFG, hcc_get_bus(HCC_EP_WIFI_DEV));
        if (ret != BFGX_POWER_SUCCESS) {
            hcc_bus_disable_state(pm_top_data->wlan_pm_info->pst_bus, OAL_BUS_STATE_ALL);
            ps_print_err("bfgx download firmware fail!\n");
            error = (ret == -OAL_EINTR) ? BFGX_POWER_DOWNLOAD_FIRMWARE_INTERRUPT : BFGX_POWER_DOWNLOAD_FIRMWARE_FAIL;
            return error;
        }
        hcc_bus_disable_state(pm_top_data->wlan_pm_info->pst_bus, OAL_BUS_STATE_ALL);

        board_sys_disable(W_SYS);
    } else {
        /* 此时BFGX 需要解复位BCPU */
        ps_print_info("wifi dereset bcpu\n");
        if (wlan_pm_open_bcpu() != BFGX_POWER_SUCCESS) {
            ps_print_err("wifi dereset bcpu fail!\n");
            error = BFGX_POWER_WIFI_DERESET_BCPU_FAIL;
            chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                                 CHR_PLT_DRV_EVENT_OPEN, CHR_PLAT_DRV_ERROR_BFGX_PWRON_BY_WIFI);
            return error;
        }
    }
    return error;
}
#else
STATIC int32_t bisheng_bfgx_download_by_uart(struct ps_core_s *ps_core_d)
{
    int32_t error;
    if (ps_core_d->pm_data->index == BUART) {
        st_tty_recv = ps_recv_patch;
    } else {
        me_tty_recv = ps_recv_patch;
    }
    error = ps_core_d->pm_data->download_patch(ps_core_d);
    if (error) { /* if download patch err,and close uart */
        release_tty_drv(ps_core_d);
        ps_print_err(" download_patch is failed!\n");
        return error;
    }

    ps_print_suc("download_patch is successfully!\n");
    return error;
}
#endif

STATIC int32_t bisheng_bfgx_dev_power_on(uint32_t sys)
{
    int32_t ret;
    int32_t error = BFGX_POWER_SUCCESS;
    uint32_t uart = (sys == B_SYS) ? BUART : GUART;

    struct ps_core_s *ps_core_d = ps_get_core_reference(uart);
    if ((ps_core_d == NULL) || (ps_core_d->pm_data == NULL)) {
        ps_print_err("ps_core_d is err\n");
        return BFGX_POWER_FAILED;
    }

    bfgx_gpio_intr_enable(ps_core_d->pm_data, OAL_TRUE);

    ret = bisheng_board_power_on(sys);
    if (ret) {
        ps_print_err("bisheng_board_power_on bfg failed, ret=%d\n", ret);
        error = BFGX_POWER_PULL_POWER_GPIO_FAIL;
        goto bfgx_power_on_fail;
    }

    if (open_tty_drv(ps_core_d) != BFGX_POWER_SUCCESS) {
        ps_print_err("open tty fail!\n");
        error = BFGX_POWER_TTY_OPEN_FAIL;
        goto bfgx_power_on_fail;
    }

#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    error = bisheng_bfgx_download_by_wifi();
#else
    error = bisheng_bfgx_download_by_uart(ps_core_d);
#endif
    if (error != BFGX_POWER_SUCCESS) {
        goto bfgx_power_on_fail;
    }

    return BFGX_POWER_SUCCESS;

bfgx_power_on_fail:
#ifdef CONFIG_HUAWEI_DSM
    if (error != BFGX_POWER_DOWNLOAD_FIRMWARE_INTERRUPT) {
        hw_110x_dsm_client_notify(SYSTEM_TYPE_PLATFORM, DSM_1103_DOWNLOAD_FIRMWARE,
                                  "bcpu download firmware failed,wifi %s,ret=%d,process:%s\n",
                                  wlan_is_shutdown() ? "off" : "on", error, current->comm);
    }
#endif
    (void)bisheng_board_power_off(sys);
    return error;
}

STATIC int32_t bisheng_bfgx_dev_power_off(uint32_t sys)
{
    int32_t error = BFGX_POWER_SUCCESS;
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data *pm_data = NULL;
    uint32_t uart = (sys == B_SYS) ? BUART : GUART;

    ps_print_info("%s\n", __func__);

    ps_core_d = ps_get_core_reference(uart);
    if ((ps_core_d == NULL) || (ps_core_d->pm_data == NULL)) {
        ps_print_err("ps_core_d is err\n");
        return BFGX_POWER_FAILED;
    }

    if (uart_bfgx_close_cmd(ps_core_d) != SUCCESS) {
        /* bfgx self close fail 了，后面也要通过wifi shutdown bcpu */
        ps_print_err("bfgx self close fail\n");
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

    (void)bisheng_board_power_off(sys);

    return error;
}

STATIC int32_t bisheng_wlan_power_off(void)
{
    struct wlan_pm_s *wlan_pm_info = wlan_pm_get_drv();

    /* 先关闭SDIO TX通道 */
    hcc_bus_disable_state(hcc_get_bus(HCC_EP_WIFI_DEV), OAL_BUS_STATE_TX);

    /* wakeup dev,send poweroff cmd to wifi */
    if (wlan_pm_poweroff_cmd() != OAL_SUCC) {
        /* wifi self close 失败了也继续往下执行，uart关闭WCPU，异常恢复推迟到wifi下次open的时候执行 */
        declare_dft_trace_key_info("wlan_poweroff_cmd_fail", OAL_DFT_TRACE_FAIL);
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_CLOSE, CHR_PLAT_DRV_ERROR_CLOSE_WCPU);
    }
    hcc_bus_disable_state(hcc_get_bus(HCC_EP_WIFI_DEV), OAL_BUS_STATE_ALL);

    (void)bisheng_board_power_off(W_SYS);

    if (wlan_pm_info != NULL) {
        wlan_pm_info->wlan_power_state = POWER_STATE_SHUTDOWN;
    }
    return SUCCESS;
}

STATIC int32_t bisheng_wlan_power_on(void)
{
    int32_t ret, error;
    struct wlan_pm_s *wlan_pm_info = wlan_pm_get_drv();
    if (wlan_pm_info == NULL) {
        return -FAILURE;
    }

    ret = bisheng_board_power_on(W_SYS);
    if (ret) {
        return -FAILURE;
    }

    hcc_bus_power_action(hcc_get_bus(HCC_EP_WIFI_DEV), HCC_BUS_POWER_PATCH_LOAD_PREPARE);

    if (bfgx_is_shutdown() && gle_is_shutdown()) {
        error = firmware_download_function(BFGX_AND_WIFI_CFG, hcc_get_bus(HCC_EP_WIFI_DEV));
        board_sys_disable(B_SYS);
        board_sys_disable(G_SYS);
    } else {
        error = firmware_download_function(WIFI_CFG, hcc_get_bus(HCC_EP_WIFI_DEV));
    }

    if (error != WIFI_POWER_SUCCESS) {
        ps_print_err("firmware download fail\n");
        if (error == -OAL_EINTR) {
            error = WIFI_POWER_ON_FIRMWARE_DOWNLOAD_INTERRUPT;
        } else if (error == -OAL_ENOPERMI) {
            error = WIFI_POWER_ON_FIRMWARE_FILE_OPEN_FAIL;
        } else {
            error = WIFI_POWER_BFGX_OFF_FIRMWARE_DOWNLOAD_FAIL;
        }
        goto wifi_power_fail;
    } else {
        wlan_pm_info->wlan_power_state = POWER_STATE_OPEN;
    }

    ret = hcc_bus_power_action(hcc_get_bus(HCC_EP_WIFI_DEV), HCC_BUS_POWER_PATCH_LAUCH);
    if (ret != 0) {
        declare_dft_trace_key_info("wlan_poweron HCC_BUS_POWER_PATCH_LAUCH by gpio_fail", OAL_DFT_TRACE_FAIL);
        error = WIFI_POWER_BFGX_OFF_BOOT_UP_FAIL;
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_OPEN, CHR_PLAT_DRV_ERROR_WCPU_BOOTUP);
        goto wifi_power_fail;
    }

    return WIFI_POWER_SUCCESS;
wifi_power_fail:
#ifdef CONFIG_HUAWEI_DSM
    if (error != WIFI_POWER_ON_FIRMWARE_DOWNLOAD_INTERRUPT && error != WIFI_POWER_ON_FIRMWARE_FILE_OPEN_FAIL) {
        hw_110x_dsm_client_notify(SYSTEM_TYPE_PLATFORM, DSM_1103_DOWNLOAD_FIRMWARE,
                                  "%s: failed to download firmware, bfgx %s, error=%d\n",
                                  __FUNCTION__, bfgx_is_shutdown() ? "off" : "on", error);
    }
#endif
    return error;
}

static int32_t bisheng_board_irq_init(struct platform_device *pdev)
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

    if ((get_hi110x_subchip_type() == BOARD_VERSION_SHENKUO) ||
        (get_hi110x_subchip_type() == BOARD_VERSION_BISHENG)) {
        gpio = g_st_board_info.dev_wakeup_host[G_SYS];
        irq = gpio_to_irq(gpio);
        g_st_board_info.wakeup_irq[G_SYS] = irq;

        ps_print_info("gcpu_irq is %d\n", g_st_board_info.wakeup_irq[G_SYS]);
    }
#endif
    return BOARD_SUCC;
}

STATIC void board_callback_init_bisheng_power(void)
{
    g_st_board_info.bd_ops.bfgx_dev_power_on = bisheng_bfgx_dev_power_on;
    g_st_board_info.bd_ops.bfgx_dev_power_off = bisheng_bfgx_dev_power_off;
    g_st_board_info.bd_ops.wlan_power_off = bisheng_wlan_power_off;
    g_st_board_info.bd_ops.wlan_power_on = bisheng_wlan_power_on;
    g_st_board_info.bd_ops.board_power_on = bisheng_board_power_on;
    g_st_board_info.bd_ops.board_power_off = bisheng_board_power_off;
    g_st_board_info.bd_ops.board_power_reset = bisheng_board_power_reset;
    g_st_board_info.bd_ops.board_irq_init = bisheng_board_irq_init;
}

void board_info_init_bisheng(void)
{
    board_callback_init_dts();

    board_callback_init_bisheng_power();
}
