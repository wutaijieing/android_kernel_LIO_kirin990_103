

/* 头文件包含 */
#define HI11XX_LOG_MODULE_NAME     "[HI1103_BOARD]"
#define HI11XX_LOG_MODULE_NAME_VAR hi1103_board_loglevel
#include "board_hi1103.h"

#include "chr_user.h"
#include "plat_debug.h"
#include "bfgx_dev.h"
#include "plat_pm.h"
#include "plat_uart.h"
#include "plat_firmware.h"
#include "pcie_linux.h"

#ifdef BFGX_UART_DOWNLOAD_SUPPORT
#include "bfgx_data_parse.h"
#include "wireless_patch.h"
#endif

/* 全局变量定义 */
#ifdef PLATFORM_DEBUG_ENABLE
int32_t g_device_monitor_enable = 0;
#endif

STATIC int32_t g_ft_fail_powerdown_bypass = 0;

STATIC int32_t hi1103_board_power_on(uint32_t ul_subsystem)
{
    int32_t ret = SUCC;

    if (ul_subsystem == W_SYS) {
        if (bfgx_is_shutdown()) {
            ps_print_info("wifi pull up power_on_enable gpio!\n");
            board_chip_power_on();
            if (board_sys_enable(W_SYS)) {
                ret = WIFI_POWER_BFGX_OFF_PULL_WLEN_FAIL;
            }
            if (hi110x_firmware_download_mode() == MODE_COMBO) {
                board_sys_enable(B_SYS);
            }
        } else {
            if (board_sys_enable(W_SYS)) {
                ret = WIFI_POWER_BFGX_ON_PULL_WLEN_FAIL;
            }
        }
    } else if (ul_subsystem == B_SYS) {
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
        if (wlan_is_shutdown()) {
            ps_print_info("bfgx pull up power_on_enable gpio!\n");
            board_chip_power_on();
            ret = board_sys_enable(W_SYS);
        }
#else
        board_chip_power_on();
#endif
        board_sys_enable(B_SYS);
    } else {
        ps_print_err("power input system:%d error\n", ul_subsystem);
        ret = WIFI_POWER_FAIL;
    }

    return ret;
}

STATIC int32_t hi1103_board_power_off(uint32_t ul_subsystem)
{
    if (ul_subsystem == W_SYS) {
        board_sys_disable(W_SYS);
        if (bfgx_is_shutdown()) {
            ps_print_info("wifi pull down power_on_enable!\n");
            board_sys_disable(B_SYS);
            board_chip_power_off();
        }
    } else if (ul_subsystem == B_SYS) {
        board_sys_disable(B_SYS);
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
        if (wlan_is_shutdown()) {
            ps_print_info("bfgx pull down power_on_enable!\n");
            board_sys_disable(W_SYS);
            board_chip_power_off();
        }
#else
        board_chip_power_off();
#endif
    } else {
        ps_print_err("power input system:%d error\n", ul_subsystem);
        return -EFAIL;
    }

    return SUCC;
}

STATIC int32_t hi1103_board_power_reset(uint32_t ul_subsystem)
{
    int32_t ret = 0;
    board_sys_disable(B_SYS);
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    board_sys_disable(W_SYS);
#endif
    board_chip_power_off();
    board_chip_power_on();
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    ret = board_sys_enable(W_SYS);
#endif
    board_sys_enable(B_SYS);
    return ret;
}

void hi1103_bfgx_subsys_reset(void)
{
    // 维测, 判断之前的gpio状态
    ps_print_info("bfgx wkup host gpio val %d\n", board_get_dev_wkup_host_state(B_SYS));

    board_sys_disable(B_SYS);
    mdelay(BFGX_SUBSYS_RST_DELAY);
    board_sys_enable(B_SYS);
}

int32_t hi1103_wifi_subsys_reset(void)
{
    int32_t ret;
    board_sys_disable(W_SYS);
    mdelay(WIFI_SUBSYS_RST_DELAY);
    ret = board_sys_enable(W_SYS);
    return ret;
}

#ifndef BFGX_UART_DOWNLOAD_SUPPORT
STATIC int32_t hi1103_bfgx_download_by_wifi(void)
{
    int32_t ret;
    int32_t error = BFGX_POWER_SUCCESS;

    if (wlan_is_shutdown() || (hi110x_firmware_download_mode() == MODE_SEPEREATE)) {
        ret = firmware_download_function(BFGX_CFG, hcc_get_bus(HCC_EP_WIFI_DEV));
        if (ret != BFGX_POWER_SUCCESS) {
            ps_print_err("bfgx download firmware fail!\n");
            error = (ret == -OAL_EINTR) ? BFGX_POWER_DOWNLOAD_FIRMWARE_INTERRUPT : BFGX_POWER_DOWNLOAD_FIRMWARE_FAIL;
            return error;
        }
        ps_print_info("bfgx download firmware succ!\n");
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
STATIC int32_t hi1103_bfgx_download_by_uart(struct ps_core_s *ps_core_d)
{
    int32_t error;

    st_tty_recv = ps_recv_patch;
    error = ps_core_d->pm_data->download_patch(ps_core_d);
    if (error) { /* if download patch err,and close uart */
        ps_print_err(" download_patch is failed!\n");
        return error;
    }

    ps_print_suc("download_patch is successfully!\n");
    return error;
}
#endif

STATIC int32_t hi1103_bfgx_dev_power_on(uint32_t sys)
{
    int32_t ret;
    int32_t error = BFGX_POWER_SUCCESS;
    struct pm_drv_data *pm_data = NULL;
    uint32_t uart = (sys == B_SYS) ? BUART : GUART;

    struct ps_core_s *ps_core_d = ps_get_core_reference(uart);
    if ((ps_core_d == NULL) || (ps_core_d->pm_data == NULL)) {
        ps_print_err("ps_core_d is err\n");
        return BFGX_POWER_FAILED;
    }

    pm_data = ps_core_d->pm_data;

    bfgx_gpio_intr_enable(pm_data, OAL_TRUE);

    ret = hi1103_board_power_on(sys);
    if (ret) {
        ps_print_err("hi1103_board_power_on bfg failed, ret=%d\n", ret);
        error = BFGX_POWER_PULL_POWER_GPIO_FAIL;
        goto bfgx_power_on_fail;
    }

    if (open_tty_drv(ps_core_d) != BFGX_POWER_SUCCESS) {
        ps_print_err("open tty fail!\n");
        error = BFGX_POWER_TTY_OPEN_FAIL;
        goto bfgx_power_on_fail;
    }

#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    error = hi1103_bfgx_download_by_wifi();
    if (wlan_is_shutdown()) {
        struct pm_top* pm_top_data = pm_get_top();
        hcc_bus_disable_state(pm_top_data->wlan_pm_info->pst_bus, OAL_BUS_STATE_ALL);
 /* eng support monitor */
#ifdef PLATFORM_DEBUG_ENABLE
        if (!g_device_monitor_enable) {
#endif
            board_sys_disable(W_SYS);
#ifdef PLATFORM_DEBUG_ENABLE
        }
#endif
    }
#else
    error = hi1103_bfgx_download_by_uart(ps_core_d);
#endif
    if (error != BFGX_POWER_SUCCESS) {
        release_tty_drv(ps_core_d);
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
    (void)hi1103_board_power_off(B_SYS);
    return error;
}

STATIC int32_t hi1103_bfgx_dev_power_off(uint32_t sys)
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

    if (!wlan_is_shutdown()) {
        ps_print_info("wifi shutdown bcpu\n");
        if (wlan_pm_shutdown_bcpu_cmd() != SUCCESS) {
            ps_print_err("wifi shutdown bcpu fail\n");
            chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                                 CHR_PLT_DRV_EVENT_CLOSE, CHR_PLAT_DRV_ERROR_WIFI_CLOSE_BCPU);
            error = BFGX_POWER_FAILED;
        }
    }

    pm_data->bfgx_dev_state = BFGX_SLEEP;
    pm_data->uart_state = UART_NOT_READY;

    (void)hi1103_board_power_off(sys);

    return error;
}

STATIC int32_t hi1103_wlan_power_off(void)
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

    (void)hi1103_board_power_off(W_SYS);

    if (wlan_pm_info != NULL) {
        wlan_pm_info->wlan_power_state = POWER_STATE_SHUTDOWN;
    }
    return SUCCESS;
}

STATIC int32_t hi1103_wlan_power_on(void)
{
    int32_t ret;
    int32_t error = WIFI_POWER_SUCCESS;
    struct wlan_pm_s *wlan_pm_info = wlan_pm_get_drv();
    if (wlan_pm_info == NULL) {
        ps_print_err("wlan_pm_info is NULL!\n");
        return -FAILURE;
    }

    ret = hi1103_board_power_on(W_SYS);
    if (ret) {
        ps_print_err("hi1103_board_power_on wlan failed ret=%d\n", ret);
        return -FAILURE;
    }

    hcc_bus_power_action(hcc_get_bus(HCC_EP_WIFI_DEV), HCC_BUS_POWER_PATCH_LOAD_PREPARE);
    /* 混合加载模式下，才一起加载bfg */
    if (bfgx_is_shutdown() && (hi110x_firmware_download_mode() == MODE_COMBO)) {
        error = firmware_download_function(BFGX_AND_WIFI_CFG, hcc_get_bus(HCC_EP_WIFI_DEV));
        board_sys_disable(B_SYS);
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
        ps_print_err("wlan_poweron HCC_BUS_POWER_PATCH_LAUCH by gpio fail ret=%d", ret);
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


#if defined(_PRE_S4_FEATURE)
STATIC void hi1103_suspend_power_gpio(void)
{
    int physical_gpio;

    physical_gpio = g_st_board_info.power_on_enable;
    if (physical_gpio == 0) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "power_on_enable suspend fail.\n");
        return;
    }
    oal_gpio_free(physical_gpio);

    physical_gpio = g_st_board_info.sys_enable[B_SYS];
    if (physical_gpio == 0) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "bfgn_power_on_enable suspend fail.\n");
        return;
    }
    oal_gpio_free(physical_gpio);

#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    physical_gpio = g_st_board_info.sys_enable[W_SYS];
    if (physical_gpio == 0) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "wlan_power_on_enable suspend fail.\n");
        return;
    }
    oal_gpio_free(physical_gpio);
#endif
}

STATIC void hi1103_suspend_wakeup_gpio(void)
{
    int physical_gpio;

    physical_gpio = g_st_board_info.dev_wakeup_host[B_SYS];
    if (physical_gpio == 0) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "bfgn_wakeup_host suspend fail.\n");
        return;
    }
    oal_gpio_free(physical_gpio);
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    physical_gpio = g_st_board_info.dev_wakeup_host[W_SYS];
    if (physical_gpio == 0) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "wlan_wakeup_host suspend fail.\n");
        return;
    }
    oal_gpio_free(physical_gpio);

    physical_gpio = g_st_board_info.host_wakeup_dev[W_SYS];
    if (physical_gpio == 0) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "host_wakeup_wlan suspend fail.\n");
        return;
    }
    oal_gpio_free(physical_gpio);
#endif
#ifdef _PRE_H2D_GPIO_WKUP
    physical_gpio = g_st_board_info.host_wakeup_dev[B_SYS];
    if (physical_gpio == 0) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "host_wakeup_bfg suspend fail.\n");
        return;
    }
    gpio_free(physical_gpio);
#endif
}

STATIC void hi1103_suspend_tas_gpio(void)
{
    oal_gpio_free(g_st_board_info.rf_wifi_tas);
}

STATIC void hi1103_suspend_gpio(void)
{
    hi1103_suspend_tas_gpio();
    hi1103_suspend_wakeup_gpio();
    hi1103_suspend_power_gpio();
}

STATIC int32_t hi1103_resume_gpio_reuqest_in(int32_t physical_gpio, const char *gpio_name)
{
    int32_t ret;

    oal_print_hi11xx_log(HI11XX_LOG_INFO, " physical_gpio is %d, gpio_name is %s.\n",
        physical_gpio, gpio_name);

    if ((physical_gpio == 0) || (gpio_name == NULL)) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "physical_gpio is %d.\n", physical_gpio);
        return BOARD_FAIL;
    }

    ret = board_gpio_request(physical_gpio, gpio_name, GPIO_DIRECTION_INPUT);
    if (ret != BOARD_SUCC) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "hi1103_board_gpio(%s)_request_in fail.\n", gpio_name);
        return BOARD_FAIL;
    }

    return BOARD_SUCC;
}

STATIC int32_t hi1103_resume_gpio_reuqest_out(int32_t physical_gpio, const char *gpio_name)
{
    int32_t ret;

    oal_print_hi11xx_log(HI11XX_LOG_INFO, " physical_gpio is %d, gpio_name is %s.\n",
        physical_gpio, gpio_name);

    if ((physical_gpio == 0) || (gpio_name == NULL)) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "physical_gpio is %d.\n", physical_gpio);
        return BOARD_FAIL;
    }

    ret = board_gpio_request(physical_gpio, gpio_name, GPIO_DIRECTION_OUTPUT);
    if (ret != BOARD_SUCC) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "hi1103_board_gpio(%s)_request_out fail.\n", gpio_name);
        return BOARD_FAIL;
    }

    return BOARD_SUCC;
}

STATIC void hi1103_resume_power_gpio(void)
{
    int32_t ret;
    int32_t physical_gpio;

    physical_gpio = g_st_board_info.power_on_enable;
    ret = hi1103_resume_gpio_reuqest_out(physical_gpio, PROC_NAME_GPIO_POWEN_ON);
    if (ret != BOARD_SUCC) {
        return;
    }

    physical_gpio = g_st_board_info.sys_enable[B_SYS];
    ret = hi1103_resume_gpio_reuqest_out(physical_gpio, PROC_NAME_GPIO_BFGX_POWEN_ON);
    if (ret != BOARD_SUCC) {
        return;
    }

    physical_gpio = g_st_board_info.sys_enable[W_SYS];
    ret = hi1103_resume_gpio_reuqest_out(physical_gpio, PROC_NAME_GPIO_WLAN_POWEN_ON);
    if (ret != BOARD_SUCC) {
        return;
    }

    return;
}

STATIC void hi1103_resume_wakeup_gpio(void)
{
    int32_t ret;
    int32_t physical_gpio;

    physical_gpio = g_st_board_info.dev_wakeup_host[B_SYS];
    ret = hi1103_resume_gpio_reuqest_in(physical_gpio, PROC_NAME_GPIO_BFGX_WAKEUP_HOST);
    if (ret != BOARD_SUCC) {
        return;
    }

    physical_gpio = g_st_board_info.dev_wakeup_host[W_SYS];
    ret = hi1103_resume_gpio_reuqest_in(physical_gpio, PROC_NAME_GPIO_WLAN_WAKEUP_HOST);
    if (ret != BOARD_SUCC) {
        return;
    }

    physical_gpio = g_st_board_info.host_wakeup_dev[W_SYS];
    ret = hi1103_resume_gpio_reuqest_out(physical_gpio, PROC_NAME_GPIO_HOST_WAKEUP_WLAN);
    if (ret != BOARD_SUCC) {
        return;
    }

#ifdef _PRE_H2D_GPIO_WKUP
    physical_gpio = g_st_board_info.host_wakeup_dev[B_SYS];
    ret = hi1103_resume_gpio_reuqest_out(physical_gpio, PROC_NAME_GPIO_HOST_WAKEUP_BFG);
    if (ret != BOARD_SUCC) {
        return;
    }
#endif
    return;
}

STATIC void hi1103_resume_tas_gpio(void)
{
    int32_t ret;
    if (g_st_board_info.wifi_tas_enable == WIFI_TAS_DISABLE) {
        return;
    }

    if (g_st_board_info.wifi_tas_state) {
        ret = gpio_request_one(g_st_board_info.rf_wifi_tas, GPIOF_OUT_INIT_HIGH, PROC_NAME_GPIO_WIFI_TAS);
    } else {
        ret = gpio_request_one(g_st_board_info.rf_wifi_tas, GPIOF_OUT_INIT_LOW, PROC_NAME_GPIO_WIFI_TAS);
    }
    if (ret) {
        ps_print_err("%s gpio_request failed\n", PROC_NAME_GPIO_WIFI_TAS);
        return;
    }
}

STATIC void hi1103_resume_gpio(void)
{
    hi1103_resume_power_gpio();
    hi1103_resume_wakeup_gpio();
    hi1103_resume_tas_gpio();
}
#endif

static int32_t hi1103_board_irq_init(struct platform_device *pdev)
{
#ifdef CONFIG_GPIOLIB
    int32_t gpio;
    int32_t irq;
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

void board_callback_init_hi1103_power(void)
{
    g_st_board_info.bd_ops.bfgx_dev_power_on = hi1103_bfgx_dev_power_on;
    g_st_board_info.bd_ops.bfgx_dev_power_off = hi1103_bfgx_dev_power_off;
    g_st_board_info.bd_ops.wlan_power_off = hi1103_wlan_power_off;
    g_st_board_info.bd_ops.wlan_power_on = hi1103_wlan_power_on;
    g_st_board_info.bd_ops.board_power_on = hi1103_board_power_on;
    g_st_board_info.bd_ops.board_power_off = hi1103_board_power_off;
    g_st_board_info.bd_ops.board_power_reset = hi1103_board_power_reset;
    g_st_board_info.bd_ops.board_irq_init = hi1103_board_irq_init;
#if defined(_PRE_S4_FEATURE)
    g_st_board_info.bd_ops.suspend_board_gpio_in_s4 = hi1103_suspend_gpio;
    g_st_board_info.bd_ops.resume_board_gpio_in_s4 = hi1103_resume_gpio;
#endif
}

/* 110x系列芯片board ops初始化 */
void board_info_init_hi1103(void)
{
    board_callback_init_dts();

    board_callback_init_hi1103_power();
}

/* factory test, wifi power on, do some test under bootloader mode */
STATIC void hi1103_dump_gpio_regs(void)
{
    uint16_t value;
    int32_t ret;
    value = 0;
    ret = read_device_reg16(GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR, &value);
    if (ret) {
        return;
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "gpio reg 0x%x = 0x%x", GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR, value);

    value = 0;
    ret = read_device_reg16(GPIO_BASE_ADDR + GPIO_LEVEL_GET_REGADDR, &value);
    if (ret) {
        return;
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "gpio reg 0x%x = 0x%x", GPIO_BASE_ADDR + GPIO_LEVEL_GET_REGADDR, value);
}

STATIC int32_t hi1103_check_device_ready(void)
{
    uint16_t value;
    int32_t ret;

    value = 0;
    ret = read_device_reg16(CHECK_DEVICE_RDY_ADDR, &value);
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "read 0x%x reg failed, ret=%d", CHECK_DEVICE_RDY_ADDR, ret);
        return -OAL_EFAIL;
    }

    /* 读到0x101表示成功 */
    if (value != 0x101) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "device sysctrl reg error, value=0x%x", value);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}

STATIC int32_t hi1103_ssi_set_gpio_direction(uint32_t address, uint16_t gpio_bit, uint32_t direction)
{
    uint16_t value;
    int32_t ret;

    value = 0;
    ret = read_device_reg16(address, &value);
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "read 0x%x reg failed, ret=%d", address, ret);
        return -OAL_EFAIL;
    }

    if (direction == GPIO_DIRECTION_OUTPUT) {
        value |= gpio_bit;
    } else {
        value &= (uint16_t)(~gpio_bit);
    }

    ret = write_device_reg16(address, value);
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "write 0x%x reg failed,value=0x%x, ret=%d", address, value, ret);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}

STATIC int32_t hi1103_ssi_set_gpio_value(int32_t gpio, int32_t gpio_lvl, uint32_t address, uint16_t value)
{
    int32_t ret;

    ret = write_device_reg16(address, value);
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "write 0x%x reg failed,value=0x%x, ret=%d", address, value, ret);
        return -OAL_EFAIL;
    }

    oal_msleep(1);

    if (oal_gpio_get_value(gpio) != gpio_lvl) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "pull gpio to %d failed!", gpio_lvl);
        hi1103_dump_gpio_regs();
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}

STATIC int32_t hi1103_ssi_check_gpio_value(int32_t gpio, int32_t gpio_lvl, uint32_t address, uint16_t check_bit)
{
    uint16_t read;
    int32_t ret;

    gpio_direction_output(gpio, gpio_lvl);
    oal_msleep(1);

    read = 0;
    ret = read_device_reg16(address, &read);
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "read 0x%x reg failed, ret=%d", address, ret);
        return -OAL_EFAIL;
    }

    oal_print_hi11xx_log(HI11XX_LOG_DBG, "read 0x%x reg=0x%x", address, read);

    read &= check_bit;

    if (((gpio_lvl == GPIO_HIGHLEVEL) && (read == 0)) || ((gpio_lvl == GPIO_LOWLEVEL) && (read != 0))) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "pull gpio to %d failed!", gpio_lvl);
        hi1103_dump_gpio_regs();
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}

STATIC int32_t hi1103_check_wlan_wakeup_host(void)
{
    int32_t i;
    int32_t ret;
    int32_t gpio_num;
    uint32_t ssi_address;
    const uint32_t ul_test_times = 2;

    gpio_num = g_st_board_info.dev_wakeup_host[W_SYS];
    if (gpio_num == 0) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "wlan_wakeup_host gpio is zero!");
        return -OAL_EIO;
    }

    ssi_address = GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR;
    ret = hi1103_ssi_set_gpio_direction(ssi_address, WLAN_DEV2HOST_GPIO, GPIO_DIRECTION_OUTPUT);
    if (ret < 0) {
        return ret;
    }

    ssi_address = GPIO_BASE_ADDR + GPIO_LEVEL_CONFIG_REGADDR;
    for (i = 0; i < ul_test_times; i++) {
        ret = hi1103_ssi_set_gpio_value(gpio_num, GPIO_HIGHLEVEL, ssi_address, WLAN_DEV2HOST_GPIO);
        if (ret < 0) {
            return ret;
        }

        ret = hi1103_ssi_set_gpio_value(gpio_num, GPIO_LOWLEVEL, ssi_address, 0);
        if (ret < 0) {
            return ret;
        }

        oal_print_hi11xx_log(HI11XX_LOG_INFO, "check d2h wakeup io %d times ok", i + 1);
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "check d2h wakeup io done");
    return OAL_SUCC;
}

STATIC int32_t hi1103_check_host_wakeup_wlan(void)
{
    int32_t i;
    int32_t ret;
    int32_t gpio_num;
    uint32_t ssi_address;
    const uint32_t ul_test_times = 2;

    gpio_num = g_st_board_info.host_wakeup_dev[W_SYS];
    if (gpio_num == 0) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "host_wakeup_wlan gpio is zero!");
        return -OAL_EIO;
    }

    ssi_address = GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR;
    ret = hi1103_ssi_set_gpio_direction(ssi_address, WLAN_HOST2DEV_GPIO, GPIO_DIRECTION_INPUT);
    if (ret < 0) {
        return ret;
    }

    ssi_address = GPIO_BASE_ADDR + GPIO_LEVEL_GET_REGADDR;
    for (i = 0; i < ul_test_times; i++) {
        ret = hi1103_ssi_check_gpio_value(gpio_num, GPIO_HIGHLEVEL, ssi_address, WLAN_HOST2DEV_GPIO);
        if (ret < 0) {
            return ret;
        }

        ret = hi1103_ssi_check_gpio_value(gpio_num, GPIO_LOWLEVEL, ssi_address, WLAN_HOST2DEV_GPIO);
        if (ret < 0) {
            return ret;
        }
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "check h2d wakeup io %d times ok", i + 1);
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "check h2d wakeup io done");
    return 0;
}

STATIC int32_t hi1103_test_device_io(void)
{
    int32_t ret;
    struct wlan_pm_s *wlan_pm_info = wlan_pm_get_drv();
    if (wlan_pm_info == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "wlan_pm_info is NULL!");
        return -FAILURE;
    }

    hcc_bus_and_wake_lock(wlan_pm_info->pst_bus);
    /* power on wifi, need't download firmware */
    ret = hi1103_board_power_on(W_SYS);
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "power on wlan failed=%d", ret);
        hcc_bus_and_wake_unlock(wlan_pm_info->pst_bus);
        return ret;
    }

    hcc_bus_power_action(hcc_get_bus(HCC_EP_WIFI_DEV), HCC_BUS_POWER_PATCH_LOAD_PREPARE);
    ret = hcc_bus_reinit(wlan_pm_info->pst_bus);
    if (ret != OAL_SUCC) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "reinit bus %d failed, ret=%d", wlan_pm_info->pst_bus->bus_type, ret);
        goto error;
    }

    wlan_pm_init_dev();

    ret = hi1103_check_device_ready();
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "check_device_ready failed, ret=%d", ret);
        goto error;
    }

    /* check io */
    ret = hi1103_check_host_wakeup_wlan();
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "check_host_wakeup_wlan failed, ret=%d", ret);
        goto error;
    }

    ret = hi1103_check_wlan_wakeup_host();
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "check_wlan_wakeup_host failed, ret=%d", ret);
        goto error;
    }

    (void)hi1103_board_power_off(W_SYS);
    hcc_bus_and_wake_unlock(wlan_pm_info->pst_bus);
    return OAL_SUCC;

error:
    if (!g_ft_fail_powerdown_bypass) {
        hi1103_board_power_off(W_SYS);
    }
    hcc_bus_and_wake_unlock(wlan_pm_info->pst_bus);
    return ret;
}

int32_t hi1103_dev_io_test(void)
{
    int32_t ret;
    declare_time_cost_stru(cost);
    struct pm_top* pm_top_data = pm_get_top();

    if (oal_mutex_trylock(&pm_top_data->host_mutex) == 0) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "get lock fial!");
        return -EINVAL;
    }

    if (!bfgx_is_shutdown()) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "bfgx is open, test abort!");
        bfgx_print_subsys_state();
        oal_mutex_unlock(&pm_top_data->host_mutex);
        return -OAL_ENODEV;
    }

    if (!wlan_is_shutdown()) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "wlan is open, test abort!");
        oal_mutex_unlock(&pm_top_data->host_mutex);
        return -OAL_ENODEV;
    }

    oal_get_time_cost_start(cost);

    ret = hi1103_test_device_io();
    if (ret < 0) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "dev io test fail!");
    }
    oal_get_time_cost_end(cost);
    oal_calc_time_cost_sub(cost);
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hi1103 device io test cost %llu us", time_cost_var_sub(cost));

    oal_mutex_unlock(&pm_top_data->host_mutex);
    return ret;
}
EXPORT_SYMBOL(hi1103_dev_io_test);

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
OAL_STATIC int32_t pcie_ip_test_pre(hcc_bus **old_bus)
{
    if (!bfgx_is_shutdown()) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "bfgx is open, test abort!");
        bfgx_print_subsys_state();
        return -OAL_ENODEV;
    }

    if (!wlan_is_shutdown()) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "wlan is open, test abort!");
        return -OAL_ENODEV;
    }

    *old_bus = hcc_get_bus(HCC_EP_WIFI_DEV);
    if (*old_bus == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "hi110x_bus is null, test abort!");
        return -OAL_ENODEV;
    }

    return OAL_SUCC;
}

int32_t hi1103_pcie_ip_test(int32_t test_count)
{
    int32_t ret;
    hcc_bus *pst_bus = NULL;
    hcc_bus *old_bus = NULL;
    declare_time_cost_stru(cost);

    if (oal_pcie_110x_working_check() != OAL_TRUE) {
        /* 不支持PCIe,直接返回成功 */
        oal_print_hi11xx_log(HI11XX_LOG_WARN, "do not support PCIe!");
        return OAL_SUCC;
    }

    ret = pcie_ip_test_pre(&old_bus);
    if (ret != OAL_SUCC) {
        return -OAL_EFAIL;
    }

    if (old_bus->bus_type != HCC_BUS_PCIE) {
        /* 尝试切换到PCIE */
        ret = hcc_switch_bus(HCC_EP_WIFI_DEV, HCC_BUS_PCIE);
        if (ret) {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "switch to PCIe failed, ret=%d", ret);
            return -OAL_ENODEV;
        }
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "switch to PCIe ok.");
    } else {
        old_bus = NULL;
    }

    oal_get_time_cost_start(cost);

    pst_bus = hcc_get_bus(HCC_EP_WIFI_DEV);

    hcc_bus_and_wake_lock(pst_bus);
    /* power on wifi, need't download firmware */
    ret = hi1103_board_power_on(W_SYS);
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "power on wlan failed=%d", ret);
        hcc_bus_and_wake_unlock(pst_bus);
        return ret;
    }

    hcc_bus_power_action(pst_bus, HCC_BUS_POWER_PATCH_LOAD_PREPARE);
    ret = hcc_bus_reinit(pst_bus);
    if (ret != OAL_SUCC) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "reinit bus %s failed, ret=%d", pst_bus->name, ret);
        ssi_dump_device_regs(SSI_MODULE_MASK_COMM | SSI_MODULE_MASK_PCIE_CFG);
        if (!g_ft_fail_powerdown_bypass) {
            hi1103_board_power_off(W_SYS);
        }
        hcc_bus_and_wake_unlock(pst_bus);
        return -OAL_EFAIL;
    }

    wlan_pm_init_dev();

    ret = hi1103_check_device_ready();
    if (ret) {
        ssi_dump_device_regs(SSI_MODULE_MASK_COMM | SSI_MODULE_MASK_PCIE_CFG);
        if (!g_ft_fail_powerdown_bypass) {
            hi1103_board_power_off(W_SYS);
        }
        hcc_bus_and_wake_unlock(pst_bus);
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "check_device_ready failed, ret=%d", ret);
        return ret;
    }

    ret = oal_pcie_ip_factory_test(pst_bus, test_count);
    if (ret) {
        ssi_dump_device_regs(SSI_MODULE_MASK_COMM | SSI_MODULE_MASK_PCIE_CFG);
        if (!g_ft_fail_powerdown_bypass) {
            hi1103_board_power_off(W_SYS);
        }
        hcc_bus_and_wake_unlock(pst_bus);
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "pcie_ip_factory_test failed, ret=%d", ret);
        return ret;
    }

    (void)hi1103_board_power_off(W_SYS);

    hcc_bus_and_wake_unlock(pst_bus);

    if (old_bus != NULL) {
        ret = hcc_switch_bus(HCC_EP_WIFI_DEV, old_bus->bus_type);
        if (ret) {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "restore to bus %s failed, ret=%d", old_bus->name, ret);
            return -OAL_ENODEV;
        }
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "resotre to bus %s ok.", old_bus->name);
    }

    oal_get_time_cost_end(cost);
    oal_calc_time_cost_sub(cost);
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hi1103 pcie ip test %llu us", time_cost_var_sub(cost));

    return OAL_SUCC;
}

EXPORT_SYMBOL(hi1103_pcie_ip_test);

OAL_STATIC uint32_t g_slt_pcie_status = 0;
OAL_STATIC hcc_bus *g_slt_old_bus = NULL;
/* for kirin slt test */
int32_t hi110x_pcie_chip_poweron(void *data)
{
    int32_t ret;
    hcc_bus *pst_bus = NULL;
    hcc_bus *old_bus = NULL;

    if (oal_pcie_110x_working_check() != OAL_TRUE) {
        /* 不支持PCIe,直接返回成功 */
        oal_print_hi11xx_log(HI11XX_LOG_WARN, "do not support PCIe!");
        return -OAL_ENODEV;
    }

    ret = pcie_ip_test_pre(&old_bus);
    if (ret != OAL_SUCC) {
        return -OAL_EFAIL;
    }

    if (old_bus->bus_type != HCC_BUS_PCIE) {
        /* 尝试切换到PCIE */
        ret = hcc_switch_bus(HCC_EP_WIFI_DEV, HCC_BUS_PCIE);
        if (ret) {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "switch to PCIe failed, ret=%d", ret);
            return -OAL_ENODEV;
        }
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "switch to PCIe ok.");
    } else {
        old_bus = NULL;
    }

    g_slt_old_bus = old_bus;

    pst_bus = hcc_get_bus(HCC_EP_WIFI_DEV);

    hcc_bus_and_wake_lock(pst_bus);
    /* power on wifi, need't download firmware */
    ret = hi1103_board_power_on(W_SYS);
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "power on wlan failed=%d", ret);
        hcc_bus_and_wake_unlock(pst_bus);
        return ret;
    }

    hcc_bus_power_action(pst_bus, HCC_BUS_POWER_PATCH_LOAD_PREPARE);
    ret = hcc_bus_reinit(pst_bus);
    if (ret != OAL_SUCC) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "reinit bus %d failed, ret=%d", pst_bus->bus_type, ret);
        ssi_dump_device_regs(SSI_MODULE_MASK_COMM | SSI_MODULE_MASK_PCIE_CFG);
        if (!g_ft_fail_powerdown_bypass) {
            hi1103_board_power_off(W_SYS);
        }
        hcc_bus_and_wake_unlock(pst_bus);
        return -OAL_EFAIL;
    }

    wlan_pm_init_dev();

    ret = hi1103_check_device_ready();
    if (ret) {
        ssi_dump_device_regs(SSI_MODULE_MASK_COMM | SSI_MODULE_MASK_PCIE_CFG);
        if (!g_ft_fail_powerdown_bypass) {
            hi1103_board_power_off(W_SYS);
        }
        hcc_bus_and_wake_unlock(pst_bus);
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "check_device_ready failed, ret=%d", ret);
        return ret;
    }

    ret = oal_pcie_ip_init(pst_bus);
    if (ret) {
        ssi_dump_device_regs(SSI_MODULE_MASK_COMM | SSI_MODULE_MASK_PCIE_CFG);
        if (!g_ft_fail_powerdown_bypass) {
            hi1103_board_power_off(W_SYS);
        }
        hcc_bus_and_wake_unlock(pst_bus);
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "oal_pcie_ip_init failed, ret=%d", ret);
        return ret;
    }

    ret = oal_pcie_ip_voltage_bias_init(pst_bus);
    if (ret) {
        if (!g_ft_fail_powerdown_bypass) {
            hi1103_board_power_off(W_SYS);
        }
        hcc_bus_and_wake_unlock(pst_bus);
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "voltage_bias_init failed, ret=%d", ret);
        return ret;
    }

    g_slt_pcie_status = 1;

    return 0;
}

int32_t hi110x_pcie_chip_poweroff(void *data)
{
    int32_t ret;
    hcc_bus *pst_bus;

    pst_bus = hcc_get_bus(HCC_EP_WIFI_DEV);
    if (pst_bus == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "can't find any wifi bus");
        return -OAL_ENODEV;
    }

    if (pst_bus->bus_type != HCC_BUS_PCIE) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "current bus is %s , not PCIe", pst_bus->name);
        return -OAL_ENODEV;
    }

    if (g_slt_pcie_status != 1) {
        oal_print_hi11xx_log(HI11XX_LOG_WARN, "pcie slt is not power on");
        return -OAL_EBUSY;
    }

    g_slt_pcie_status = 0;

    /* SLT下电之前打印链路信息 */
    hcc_bus_chip_info(pst_bus, OAL_FALSE, OAL_TRUE);

    (void)hi1103_board_power_off(W_SYS);

    hcc_bus_and_wake_unlock(pst_bus);

    if (g_slt_old_bus != NULL) {
        ret = hcc_switch_bus(HCC_EP_WIFI_DEV, g_slt_old_bus->bus_type);
        if (ret) {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "restore to bus %s failed, ret=%d", g_slt_old_bus->name, ret);
            return -OAL_ENODEV;
        }
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "resotre to bus %s ok.", g_slt_old_bus->name);
    }

    return 0;
}

int32_t hi110x_pcie_chip_transfer(void *ddr_address, uint32_t data_size, uint32_t direction)
{
    hcc_bus *pst_bus;

    pst_bus = hcc_get_bus(HCC_EP_WIFI_DEV);
    if (pst_bus == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "can't find any wifi bus");
        return -OAL_ENODEV;
    }

    if (pst_bus->bus_type != HCC_BUS_PCIE) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "current bus is %s , not PCIe", pst_bus->name);
        return -OAL_ENODEV;
    }

    return oal_pcie_rc_slt_chip_transfer(pst_bus, ddr_address, data_size, (int32_t)direction);
}
EXPORT_SYMBOL(hi110x_pcie_chip_poweron);
EXPORT_SYMBOL(hi110x_pcie_chip_transfer);
EXPORT_SYMBOL(hi110x_pcie_chip_poweroff);
#endif

#if (defined(CONFIG_PCIE_KIRIN_SLT_HI110X)|| defined(CONFIG_PCIE_KPORT_SLT_DEVICE)) && defined(CONFIG_HISI_DEBUG_FS)
int pcie_slt_hook_register(u32 rc_id, u32 device_type, int (*init)(void *), int (*on)(void *),
                           int (*off)(void *), int (*setup)(void *), int (*data_transfer)(void *, u32, u32));
int32_t hi1103_pcie_chip_init(void *data)
{
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "slt pcie init");
    return 0;
}

int32_t hi1103_pcie_chip_rc_slt_register(void)
{
#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
    return pcie_slt_hook_register(g_kirin_rc_idx, 0x2,
                                  hi1103_pcie_chip_init, hi110x_pcie_chip_poweron,
                                  hi110x_pcie_chip_poweroff, hi1103_pcie_chip_init,
                                  hi110x_pcie_chip_transfer);
#else
    return 0;
#endif
}

int32_t hi1103_pcie_chip_rc_slt_unregister(void)
{
#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
    return pcie_slt_hook_register(g_kirin_rc_idx, 0x2,
                                  NULL, NULL,
                                  NULL, NULL,
                                  NULL);
#else
    return 0;
#endif
}
EXPORT_SYMBOL(hi1103_pcie_chip_rc_slt_register);
EXPORT_SYMBOL(hi1103_pcie_chip_rc_slt_unregister);
#endif
