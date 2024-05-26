

/* 头文件包含 */
#define HI11XX_LOG_MODULE_NAME     "[BOARD_DTS]"
#define HI11XX_LOG_MODULE_NAME_VAR board_dts_loglevel
#include <linux/clk.h>
#include <linux/platform_device.h>

#include "board_dts.h"
#include "chr_user.h"
#include "plat_debug.h"
#include "plat_pm.h"
#include "plat_uart.h"

#ifdef BFGX_UART_DOWNLOAD_SUPPORT
#include "bfgx_data_parse.h"
#include "wireless_patch.h"
#endif

STATIC void request_ssi_gpio(void)
{
    int32_t ret;
    int32_t physical_gpio = 0;

    ret = get_board_gpio(DTS_NODE_HISI_HI110X, DTS_PROP_GPIO_HI110X_GPIO_SSI_CLK, &physical_gpio);
    if (ret != BOARD_SUCC) {
        ps_print_info("get dts prop %s failed, gpio-ssi don't support\n", DTS_PROP_GPIO_HI110X_GPIO_SSI_CLK);
        return;
    }
    g_st_board_info.ssi_gpio_clk = physical_gpio;

    ret = get_board_gpio(DTS_NODE_HISI_HI110X, DTS_PROP_GPIO_HI110X_GPIO_SSI_DATA, &physical_gpio);
    if (ret != BOARD_SUCC) {
        ps_print_info("get dts prop %s failed, gpio-ssi don't support\n", DTS_PROP_GPIO_HI110X_GPIO_SSI_DATA);
        g_st_board_info.ssi_gpio_clk = 0;
        return;
    }
    g_st_board_info.ssi_gpio_data = physical_gpio;

    ret = ssi_request_gpio(g_st_board_info.ssi_gpio_clk, g_st_board_info.ssi_gpio_data);
    if (ret != BOARD_SUCC) {
        ps_print_err("request ssi gpio fail\n");
        g_st_board_info.ssi_gpio_clk = 0;
        g_st_board_info.ssi_gpio_data = 0;
        return;
    }
}

STATIC void free_ssi_gpio(void)
{
    ssi_free_gpio();
    g_st_board_info.ssi_gpio_clk = 0;
    g_st_board_info.ssi_gpio_data = 0;
}

STATIC int32_t get_pcie_rst_gpio(struct platform_device *pdev)
{
    int32_t ret;
    int physical_gpio = 0;

    ret = get_board_gpio(DTS_NODE_HISI_HI110X, DTS_PROP_GPIO_HI110X_PCIE0_RST, &physical_gpio);
    if (ret == BOARD_SUCC) {
        g_st_board_info.ep_pcie0_rst = physical_gpio;
        ret = gpio_request(physical_gpio, "ep_pcie0_rst");
        if (ret != BOARD_SUCC) {
            ps_print_err("ep_pcie0_rst %d gpio_request failed ret=%d\n", physical_gpio, ret);
            return BOARD_FAIL;
        }
        gpio_direction_output(g_st_board_info.ep_pcie0_rst, GPIO_LOWLEVEL);
        ps_print_info("ep_pcie0_rst=%d\n", g_st_board_info.ep_pcie0_rst);
    } else {
        ps_print_info("get dts prop %s failed\n", DTS_PROP_GPIO_HI110X_PCIE0_RST);
        g_st_board_info.ep_pcie0_rst = 0;
    }
    ps_print_info("ep_pcie0_rst gpio=%d", g_st_board_info.ep_pcie0_rst);

    ret = get_board_gpio(DTS_NODE_HISI_HI110X, DTS_PROP_GPIO_HI110X_PCIE1_RST, &physical_gpio);
    if (ret == BOARD_SUCC) {
        g_st_board_info.ep_pcie1_rst = physical_gpio;
        ret = gpio_request(physical_gpio, "ep_pcie1_rst");
        if (ret != BOARD_SUCC) {
            ps_print_err("ep_pcie1_rst %d gpio_request failed ret=%d\n", physical_gpio, ret);
            oal_gpio_free(g_st_board_info.ep_pcie0_rst);

            return BOARD_FAIL;
        }
        gpio_direction_output(g_st_board_info.ep_pcie1_rst, GPIO_LOWLEVEL);
        ps_print_info("ep_pcie1_rst=%d\n", g_st_board_info.ep_pcie1_rst);
    } else {
        ps_print_info("get dts prop %s failed\n", DTS_PROP_GPIO_HI110X_PCIE1_RST);
        g_st_board_info.ep_pcie1_rst = 0;
    }
    ps_print_info("ep_pcie1_rst gpio=%d", g_st_board_info.ep_pcie1_rst);

    return BOARD_SUCC;
}

STATIC void free_pcie_rst_gpio(void)
{
    oal_gpio_free(g_st_board_info.ep_pcie1_rst);
    oal_gpio_free(g_st_board_info.ep_pcie0_rst);
}

STATIC int32_t request_gcore_gpio(void)
{
    int32_t ret;
    int physical_gpio = 0;

    ret = get_board_gpio(DTS_NODE_HISI_HI110X, DTS_PROP_GPIO_G_POWEN_ON_ENABLE, &physical_gpio);
    if (ret == BOARD_SUCC) {
        g_st_board_info.sys_enable[G_SYS] = physical_gpio;
        ret = gpio_request(physical_gpio, PROC_NAME_GPIO_G_POWEN_ON);
        if (ret != BOARD_SUCC) {
            ps_print_err("g core %d gpio_request failed ret=%d\n", physical_gpio, ret);
            return BOARD_FAIL;
        }
        gpio_direction_output(g_st_board_info.sys_enable[G_SYS], GPIO_LOWLEVEL);
        ps_print_info("g core gpio =%d\n", g_st_board_info.sys_enable[G_SYS]);
    } else {
        g_st_board_info.sys_enable[G_SYS] = 0;
    }

    return BOARD_SUCC;
}

STATIC int32_t hi110x_gpio_request(const char *dts_node, const char *dts_prop,
                                   const char *gpio_name, uint32_t direction)
{
    int32_t ret;
    int32_t physical_gpio = 0;

    ret = get_board_gpio(dts_node, dts_prop, &physical_gpio);
    if (ret != BOARD_SUCC) {
        ps_print_info("get dts prop %s failed\n", dts_prop);
        return BOARD_SUCC;
    }

    ret = board_gpio_request(physical_gpio, gpio_name, direction);
    if (ret != BOARD_SUCC) {
        ps_print_err("request gpio %s failed\n", gpio_name);
        return BOARD_FAIL;
    }

    return physical_gpio;
}

STATIC void hi110x_get_gt_board_power_gpio(void)
{
    int32_t physical_gpio;
    /* gt enable */
    physical_gpio = hi110x_gpio_request(DTS_NODE_HISI_HI110X, DTS_PROP_GPIO_GT_POWEN_ON_ENABLE,
                                        PROC_NAME_GPIO_GT_POWEN_ON, GPIO_DIRECTION_OUTPUT);
    if (physical_gpio < 0) {
        ps_print_info("gt power gpio not find\n");
    } else {
        g_st_board_info.sys_enable[GT_SYS] = physical_gpio;
    }
}

STATIC int32_t hi110x_get_board_power_gpio(struct platform_device *pdev)
{
    int32_t ret;
    int32_t physical_gpio;

    request_ssi_gpio();

    /* power on enable */
    physical_gpio = hi110x_gpio_request(DTS_NODE_HISI_HI110X, DTS_PROP_GPIO_HI110X_POWEN_ON,
                                        PROC_NAME_GPIO_POWEN_ON, GPIO_DIRECTION_OUTPUT);
    if (physical_gpio < 0) {
        return BOARD_FAIL;
    }
    g_st_board_info.power_on_enable = physical_gpio;

    /* bfgn enable */
    physical_gpio = hi110x_gpio_request(DTS_NODE_HISI_HI110X, DTS_PROP_GPIO_BFGX_POWEN_ON_ENABLE,
                                        PROC_NAME_GPIO_BFGX_POWEN_ON, GPIO_DIRECTION_OUTPUT);
    if (physical_gpio < 0) {
        goto err_get_bfgx_power_gpio;
    }
    g_st_board_info.sys_enable[B_SYS] = physical_gpio;

#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    /* wlan enable */
    physical_gpio = hi110x_gpio_request(DTS_NODE_HISI_HI110X, DTS_PROP_GPIO_WLAN_POWEN_ON_ENABLE,
                                        PROC_NAME_GPIO_WLAN_POWEN_ON, GPIO_DIRECTION_OUTPUT);
    if (physical_gpio < 0) {
        goto err_get_wlan_power_gpio;
    }
    g_st_board_info.sys_enable[W_SYS] = physical_gpio;
#endif

    ret = get_pcie_rst_gpio(pdev);
    if (ret != BOARD_SUCC) {
        goto err_get_pcie_rst_gpio;
    }

    ret = request_gcore_gpio();
    if (ret != BOARD_SUCC) {
        goto err_get_g_gpio;
    }
    hi110x_get_gt_board_power_gpio();
    return BOARD_SUCC;
err_get_g_gpio:
    free_pcie_rst_gpio();
err_get_pcie_rst_gpio:
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    oal_gpio_free(g_st_board_info.sys_enable[W_SYS]);
#endif
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
err_get_wlan_power_gpio:
    oal_gpio_free(g_st_board_info.sys_enable[B_SYS]);
    oal_gpio_free(g_st_board_info.sys_enable[G_SYS]);
#endif
err_get_bfgx_power_gpio:
    oal_gpio_free(g_st_board_info.power_on_enable);
    free_ssi_gpio();

    return BOARD_FAIL;
}

STATIC void hi110x_free_board_power_gpio(void)
{
    free_pcie_rst_gpio();
    oal_gpio_free(g_st_board_info.power_on_enable);
    oal_gpio_free(g_st_board_info.sys_enable[B_SYS]);
    oal_gpio_free(g_st_board_info.sys_enable[G_SYS]);
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    oal_gpio_free(g_st_board_info.sys_enable[W_SYS]);
#endif
    free_ssi_gpio();
}

STATIC int32_t hi110x_bfgx_wakeup_gpio_init(void)
{
    int32_t physical_gpio;

    /* bfgx wake host gpio request */
    physical_gpio = hi110x_gpio_request(DTS_NODE_HI110X_BFGX, DTS_PROP_HI110X_GPIO_BFGX_WAKEUP_HOST,
                                        PROC_NAME_GPIO_BFGX_WAKEUP_HOST, GPIO_DIRECTION_INPUT);
    if (physical_gpio < 0) {
        return BOARD_FAIL;
    }
    g_st_board_info.dev_wakeup_host[B_SYS] = physical_gpio;
    ps_print_info("hi110x bfgx wkup host gpio is %d\n", g_st_board_info.dev_wakeup_host[B_SYS]);

    if (get_hi110x_subchip_type() == BOARD_VERSION_SHENKUO) {
        /* gcpu wake host gpio request */
        physical_gpio = hi110x_gpio_request(DTS_NODE_HI110X_ME, DTS_PROP_HI110X_GPIO_GCPU_WAKEUP_HOST,
                                            PROC_NAME_GPIO_GCPU_WAKEUP_HOST, GPIO_DIRECTION_INPUT);
        if (physical_gpio < 0) {
            goto err_get_gcpu_wkup_host_gpio;
        }
        g_st_board_info.dev_wakeup_host[G_SYS] = physical_gpio;
        ps_print_info("hi110x gcpu wkup host gpio is %d\n", g_st_board_info.dev_wakeup_host[G_SYS]);
    } else if (get_hi110x_subchip_type() == BOARD_VERSION_BISHENG ||
               get_hi110x_subchip_type() == BOARD_VERSION_HI1161) {
        /* gcpu wake host gpio request */
        physical_gpio = hi110x_gpio_request(DTS_NODE_HI110X_GLE, DTS_PROP_HI110X_GPIO_GCPU_WAKEUP_HOST,
                                            PROC_NAME_GPIO_GCPU_WAKEUP_HOST, GPIO_DIRECTION_INPUT);
        if (physical_gpio < 0) {
            goto err_get_gcpu_wkup_host_gpio;
        }
        g_st_board_info.dev_wakeup_host[G_SYS] = physical_gpio;
        ps_print_info("hi110x gle wkup host gpio is %d\n", g_st_board_info.dev_wakeup_host[G_SYS]);
    } else {
        g_st_board_info.dev_wakeup_host[G_SYS] = 0;
    }

#ifdef _PRE_H2D_GPIO_WKUP
    /* host wake bfgx gpio request */
    physical_gpio = hi110x_gpio_request(DTS_NODE_HI110X_BFGX, DTS_PROP_HI110X_GPIO_HOST_WAKEUP_BFGX,
                                        PROC_NAME_GPIO_HOST_WAKEUP_BFG, GPIO_DIRECTION_INPUT);
    if (physical_gpio < 0) {
        goto err_get_host_wkup_bfgx_gpio;
    }
    g_st_board_info.host_wakeup_dev[B_SYS] = physical_gpio;
    ps_print_info("hi110x host wkup bfgn gpio is %d\n", g_st_board_info.host_wakeup_dev[B_SYS]);
#else
    g_st_board_info.host_wakeup_dev[B_SYS] = 0;
#endif

    return BOARD_SUCC;

#ifdef _PRE_H2D_GPIO_WKUP
err_get_host_wkup_bfgx_gpio:
    oal_gpio_free(g_st_board_info.dev_wakeup_host[G_SYS]);
#endif
err_get_gcpu_wkup_host_gpio:
    oal_gpio_free(g_st_board_info.dev_wakeup_host[B_SYS]);

    return BOARD_FAIL;
}

STATIC void hi110x_bfgx_wakeup_gpio_free(void)
{
#ifdef _PRE_H2D_GPIO_WKUP
    oal_gpio_free(g_st_board_info.host_wakeup_dev[B_SYS]);
#endif

    if ((get_hi110x_subchip_type() == BOARD_VERSION_SHENKUO) || (get_hi110x_subchip_type() == BOARD_VERSION_BISHENG)) {
        oal_gpio_free(g_st_board_info.dev_wakeup_host[G_SYS]);
    }

    oal_gpio_free(g_st_board_info.dev_wakeup_host[B_SYS]);
}

STATIC int32_t hi110x_wifi_wakeup_gpio_init(void)
{
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    int32_t physical_gpio;

    /* wifi wake host gpio request */
    physical_gpio = hi110x_gpio_request(DTS_NODE_HI110X_WIFI, DTS_PROP_HI110X_GPIO_WLAN_WAKEUP_HOST,
                                        PROC_NAME_GPIO_WLAN_WAKEUP_HOST, GPIO_DIRECTION_INPUT);
    if (physical_gpio < 0) {
        return BOARD_FAIL;
    }
    g_st_board_info.dev_wakeup_host[W_SYS] = physical_gpio;
    ps_print_info("hi110x wifi wkup host gpio is %d\n", g_st_board_info.dev_wakeup_host[W_SYS]);

    /* host wake wlan gpio request */
    physical_gpio = hi110x_gpio_request(DTS_NODE_HI110X_WIFI, DTS_PROP_GPIO_HOST_WAKEUP_WLAN,
                                        PROC_NAME_GPIO_HOST_WAKEUP_WLAN, GPIO_DIRECTION_INPUT);
    if (physical_gpio < 0) {
        oal_gpio_free(g_st_board_info.dev_wakeup_host[W_SYS]);
        return BOARD_FAIL;
    }
    g_st_board_info.host_wakeup_dev[W_SYS] = physical_gpio;
    ps_print_info("hi110x host wkup wifi gpio is %d\n", g_st_board_info.host_wakeup_dev[W_SYS]);
#endif

    return BOARD_SUCC;
}

STATIC void hi110x_wifi_wakeup_gpio_free(void)
{
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    oal_gpio_free(g_st_board_info.dev_wakeup_host[W_SYS]);
    oal_gpio_free(g_st_board_info.host_wakeup_dev[W_SYS]);
#endif
}

STATIC int32_t hi110x_board_wakeup_gpio_init(struct platform_device *pdev)
{
    int32_t ret;

    ret = hi110x_bfgx_wakeup_gpio_init();
    if (ret != BOARD_SUCC) {
        return BOARD_FAIL;
    }

    ret = hi110x_wifi_wakeup_gpio_init();
    if (ret != BOARD_SUCC) {
        hi110x_bfgx_wakeup_gpio_free();
        return BOARD_FAIL;
    }

    return BOARD_SUCC;
}

STATIC void hi110x_free_board_wakeup_gpio(void)
{
    hi110x_bfgx_wakeup_gpio_free();
    hi110x_wifi_wakeup_gpio_free();
}

STATIC int32_t hi110x_board_wifi_tas_gpio_init(void)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32_t ret;
    int32_t physical_gpio = 0;
    struct device_node *np = NULL;

    ret = get_board_dts_node(&np, DTS_NODE_HI110X_WIFI);
    if (ret != BOARD_SUCC) {
        ps_print_err("DTS read node %s fail!!!\n", DTS_NODE_HI110X_WIFI);
        return BOARD_SUCC;
    }

    ret = of_property_read_bool(np, DTS_PROP_WIFI_TAS_EN);
    if (ret) {
        ps_print_info("Hi110x wifi tas enable\n");
        g_st_board_info.wifi_tas_enable = WIFI_TAS_ENABLE;
    } else {
        ps_print_info("Hi110x wifi tas not enable\n");
        g_st_board_info.wifi_tas_enable = WIFI_TAS_DISABLE;
        return BOARD_SUCC;
    }

    /* wifi tas control gpio request */
    ret = get_board_gpio(DTS_NODE_HI110X_WIFI, DTS_PROP_GPIO_WIFI_TAS, &physical_gpio);
    if (ret != BOARD_SUCC) {
        ps_print_err("get dts prop %s failed\n", DTS_PROP_GPIO_WIFI_TAS);
        return BOARD_SUCC;
    }

    g_st_board_info.rf_wifi_tas = physical_gpio;
    ps_print_info("hi110x wifi tas gpio is %d\n", g_st_board_info.rf_wifi_tas);

    ret = of_property_read_u32(np, DTS_PROP_HI110X_WIFI_TAS_STATE, &g_st_board_info.wifi_tas_state);
    if (ret) {
        ps_print_err("read prop [%s] fail, ret=%d\n", DTS_PROP_HI110X_WIFI_TAS_STATE, ret);
    }
    if (g_st_board_info.wifi_tas_state) {
        ret = gpio_request_one(physical_gpio, GPIOF_OUT_INIT_HIGH, PROC_NAME_GPIO_WIFI_TAS);
    } else {
        ret = gpio_request_one(physical_gpio, GPIOF_OUT_INIT_LOW, PROC_NAME_GPIO_WIFI_TAS);
    }
    if (ret) {
        ps_print_err("%s gpio_request failed\n", PROC_NAME_GPIO_WIFI_TAS);
        return BOARD_FAIL;
    }
#endif

    return BOARD_SUCC;
}

STATIC void hi110x_free_board_wifi_tas_gpio(void)
{
    oal_gpio_free(g_st_board_info.rf_wifi_tas);
}

/*
 * 函 数 名  : hi110x_board_flowctrl_gpio_init
 * 功能描述  : 注册gpio流控中断
 */
STATIC int32_t hi110x_board_flowctrl_gpio_init(void)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32_t ret;
    int32_t physical_gpio;

    ret = get_board_gpio(DTS_NODE_HI110X_WIFI, DTS_PROP_GPIO_WLAN_FLOWCTRL, &physical_gpio);
    if (ret != BOARD_SUCC) {
        ps_print_err("NO dts prop %s\n", DTS_PROP_GPIO_WLAN_FLOWCTRL);
        return BOARD_SUCC;
    }

    ret = board_gpio_request(physical_gpio, PROC_NAME_GPIO_WLAN_FLOWCTRL, GPIO_DIRECTION_INPUT);
    if (ret != BOARD_SUCC) {
        ps_print_err("gpio requeset %s\n", PROC_NAME_GPIO_WLAN_FLOWCTRL);
        return BOARD_FAIL;
    }

    g_st_board_info.flowctrl_gpio = physical_gpio;
    ps_print_info("hi110x flow ctrl gpio is %d\n", g_st_board_info.flowctrl_gpio);
#endif
    return BOARD_SUCC;
}

STATIC void hi110x_free_board_flowctrl_gpio(void)
{
    oal_gpio_free(g_st_board_info.flowctrl_gpio);
}


STATIC int32_t hi110x_get_board_pmu_clk32k(struct platform_device *pdev)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32_t ret;
    const char *clk_name = NULL;
    struct clk *clk = NULL;
    struct device *dev = NULL;

    ret = get_board_custmize(DTS_NODE_HISI_HI110X, DTS_PROP_CLK_32K, &clk_name);
    if (ret != BOARD_SUCC) {
        ps_print_err("NO dts prop %s\n", DTS_PROP_CLK_32K);
        return BOARD_SUCC;
    }
    g_st_board_info.clk_32k_name = clk_name;

    ps_print_info("hi110x 32k clk name is %s\n", g_st_board_info.clk_32k_name);

    dev = &pdev->dev;
    clk = devm_clk_get(dev, clk_name);
    if (clk == NULL) {
        ps_print_err("Get 32k clk %s failed!!!\n", clk_name);
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_DTS, CHR_PLAT_DRV_ERROR_32K_CLK_DTS);
        return BOARD_FAIL;
    }
    g_st_board_info.clk_32k = clk;
    return BOARD_SUCC;
#else
    return BOARD_SUCC;
#endif
}

STATIC int32_t hi110x_get_board_uart_port(void)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32_t ret;
    struct device_node *np = NULL;
    const char *uart_port = NULL;

    ret = get_board_dts_node(&np, DTS_NODE_HI110X_BFGX);
    if (ret != BOARD_SUCC) {
        ps_print_err("DTS read node %s fail!!!\n", DTS_NODE_HI110X_BFGX);
        return BOARD_SUCC;
    }

    /* 使用uart4，需要在dts里新增DTS_PROP_HI110X_UART_PCLK项，指明uart4不依赖sensorhub */
    ret = of_property_read_bool(np, DTS_PROP_HI110X_UART_PCLK);
    if (ret) {
        ps_print_info("uart pclk normal\n");
        g_st_board_info.uart_pclk = UART_PCLK_NORMAL;
    } else {
        ps_print_info("uart pclk from sensorhub\n");
        g_st_board_info.uart_pclk = UART_PCLK_FROM_SENSORHUB;
    }

    ret = get_board_custmize(DTS_NODE_HI110X_BFGX, DTS_PROP_HI110X_UART_POART, &uart_port);
    if (ret != BOARD_SUCC) {
        ps_print_info("no %s\n", DTS_PROP_HI110X_UART_POART);
    }
    g_st_board_info.uart_port[BUART] = uart_port;

    if ((get_hi110x_subchip_type() == BOARD_VERSION_SHENKUO)) {
        ret = get_board_custmize(DTS_NODE_HI110X_ME, DTS_PROP_HI110X_GUART_POART, &uart_port);
        if (ret != BOARD_SUCC) {
            ps_print_info("no %s\n", DTS_PROP_HI110X_GUART_POART);
        }
        g_st_board_info.uart_port[GUART] = uart_port;
    } else if ((get_hi110x_subchip_type() == BOARD_VERSION_BISHENG) ||
                get_hi110x_subchip_type() == BOARD_VERSION_HI1161) {
        ret = get_board_custmize(DTS_NODE_HI110X_GLE, DTS_PROP_HI110X_GUART_POART, &uart_port);
        if (ret != BOARD_SUCC) {
            ps_print_info("no %s\n", DTS_PROP_HI110X_GUART_POART);
        }
        g_st_board_info.uart_port[GUART] = uart_port;
    }

    return BOARD_SUCC;
#else
    return BOARD_SUCC;
#endif
}


#ifdef HAVE_HISI_IR
STATIC int32_t hi110x_board_ir_ctrl_gpio_init(void)
{
    int32_t ret;
    int32_t physical_gpio = 0;

    ret = get_board_gpio(DTS_NODE_HI110X_BFGX, DTS_PROP_GPIO_BFGX_IR_CTRL, &physical_gpio);
    if (ret != BOARD_SUCC) {
        ps_print_info("dts prop %s not exist\n", DTS_PROP_GPIO_BFGX_IR_CTRL);
        g_st_board_info.bfgx_ir_ctrl_gpio = 0;
    } else {
        g_st_board_info.bfgx_ir_ctrl_gpio = physical_gpio;

        ret = gpio_request_one(physical_gpio, GPIOF_OUT_INIT_LOW, PROC_NAME_GPIO_BFGX_IR_CTRL);
        if (ret) {
            ps_print_err("%s gpio_request failed\n", PROC_NAME_GPIO_BFGX_IR_CTRL);
        }
    }

    return BOARD_SUCC;
}

STATIC int32_t hi110x_board_ir_ctrl_pmic_init(struct platform_device *pdev)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32_t ret = BOARD_FAIL;
    struct device_node *np = NULL;
    int32_t irled_voltage = 0;
    if (pdev == NULL) {
        ps_print_err("board pmu pdev is NULL!\n");
        return ret;
    }

    ret = get_board_dts_node(&np, DTS_NODE_HI110X_BFGX);
    if (ret != BOARD_SUCC) {
        ps_print_err("DTS read node %s fail!!!\n", DTS_NODE_HI110X_BFGX);
        return ret;
    }

    g_st_board_info.bfgn_ir_ctrl_ldo = regulator_get(&pdev->dev, DTS_PROP_HI110X_IRLED_LDO_POWER);

    if (IS_ERR(g_st_board_info.bfgn_ir_ctrl_ldo)) {
        ps_print_err("board_ir_ctrl_pmic_init get ird ldo failed\n");
        return ret;
    }

    ret = of_property_read_u32(np, DTS_PROP_HI110X_IRLED_VOLTAGE, &irled_voltage);
    if (ret == BOARD_SUCC) {
        ps_print_info("set irled voltage %d mv\n", irled_voltage / VOLATAGE_V_TO_MV); /* V to mV */
        ret = regulator_set_voltage(g_st_board_info.bfgn_ir_ctrl_ldo, (int)irled_voltage, (int)irled_voltage);
        if (ret) {
            ps_print_err("board_ir_ctrl_pmic_init set voltage ldo failed\n");
            return ret;
        }
    } else {
        ps_print_err("get irled voltage failed ,use default\n");
    }

    ret = regulator_set_mode(g_st_board_info.bfgn_ir_ctrl_ldo, REGULATOR_MODE_NORMAL);
    if (ret) {
        ps_print_err("board_ir_ctrl_pmic_init set ldo mode failed\n");
        return ret;
    }
    ps_print_info("board_ir_ctrl_pmic_init success\n");
    return BOARD_SUCC;
#else
    return BOARD_SUCC;
#endif
}
#endif

STATIC int32_t hi110x_board_ir_ctrl_init(struct platform_device *pdev)
{
#if (defined HAVE_HISI_IR) && (defined _PRE_CONFIG_USE_DTS)
    int ret;
    struct device_node *np = NULL;

    ret = get_board_dts_node(&np, DTS_NODE_HI110X_BFGX);
    if (ret != BOARD_SUCC) {
        ps_print_err("DTS read node %s fail!!!\n", DTS_NODE_HI110X_BFGX);
        return BOARD_SUCC;
    }
    g_st_board_info.have_ir = of_property_read_bool(np, "have_ir");
    if (!g_st_board_info.have_ir) {
        ps_print_info("board has no Ir");
    } else {
        ret = of_property_read_u32(np, DTS_PROP_HI110X_IR_LDO_TYPE, &g_st_board_info.irled_power_type);
        ps_print_info("read property ret is %d, irled_power_type is %d\n", ret, g_st_board_info.irled_power_type);
        if (ret != BOARD_SUCC) {
            ps_print_err("get dts prop %s failed\n", DTS_PROP_HI110X_IR_LDO_TYPE);
            goto err_get_ir_ctrl_gpio;
        }

        if (g_st_board_info.irled_power_type == IR_GPIO_CTRL) {
            ret = hi110x_board_ir_ctrl_gpio_init();
            if (ret != BOARD_SUCC) {
                ps_print_err("ir_ctrl_gpio init failed\n");
                goto err_get_ir_ctrl_gpio;
            }
        } else if (g_st_board_info.irled_power_type == IR_LDO_CTRL) {
            ret = hi110x_board_ir_ctrl_pmic_init(pdev);
            if (ret != BOARD_SUCC) {
                ps_print_err("ir_ctrl_pmic init failed\n");
                goto err_get_ir_ctrl_gpio;
            }
        } else {
            ps_print_err("get ir_ldo_type failed!err num is %d\n", g_st_board_info.irled_power_type);
            goto err_get_ir_ctrl_gpio;
        }
    }
    return BOARD_SUCC;

err_get_ir_ctrl_gpio:
    return BOARD_FAIL;
#else
    return BOARD_SUCC;
#endif
}

void hi110x_free_board_ir_gpio(void)
{
#if (defined HAVE_HISI_IR)
    oal_gpio_free(g_st_board_info.bfgx_ir_ctrl_gpio);
#endif
}

#define EMU_INI_BUF_SIZE 50
static void hi110x_emu_init(void)
{
    char buff[EMU_INI_BUF_SIZE];
    g_st_board_info.is_emu = OAL_FALSE;

    memset_s(buff, sizeof(buff), 0, sizeof(buff));
    if (get_cust_conf_string(INI_MODU_PLAT, "emu_enable", buff, sizeof(buff) - 1) == INI_SUCC) {
        if (!oal_strncmp("yes", buff, OAL_STRLEN("yes"))) {
            g_st_board_info.is_emu = OAL_TRUE;
            ps_print_info("emu enable\n");
        }
    }
}

STATIC int32_t hi110x_check_evb_or_fpga(void)
{
#ifdef _PRE_HI375X_NO_DTS
        ps_print_info("HI11xx ASIC VERSION\n");
        g_st_board_info.is_asic = VERSION_ASIC;
#else
#ifdef _PRE_CONFIG_USE_DTS
    int32_t ret;
    struct device_node *np = NULL;

    ret = get_board_dts_node(&np, DTS_NODE_HISI_HI110X);
    if (ret != BOARD_SUCC) {
        ps_print_err("DTS read node %s fail!!!\n", DTS_NODE_HISI_HI110X);
        return BOARD_FAIL;
    }

    ret = of_property_read_bool(np, DTS_PROP_HI110X_VERSION);
    if (ret) {
        ps_print_info("HI11xx ASIC VERSION\n");
        g_st_board_info.is_asic = VERSION_ASIC;
    } else {
        ps_print_info("HI11xx FPGA VERSION\n");
        g_st_board_info.is_asic = VERSION_FPGA;
    }
#endif
#endif
    return BOARD_SUCC;
}

STATIC int32_t hi110x_check_hi110x_subsystem_support(void)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32_t ret;
    struct device_node *np = NULL;

    ret = get_board_dts_node(&np, DTS_NODE_HISI_HI110X);
    if (ret != BOARD_SUCC) {
        ps_print_err("DTS read node %s fail!!!\n", DTS_NODE_HISI_HI110X);
        return BOARD_FAIL;
    }

    ret = of_property_read_bool(np, DTS_PROP_HI110X_WIFI_DISABLE);
    if (ret) {
        g_st_board_info.is_wifi_disable = 1;
    } else {
        g_st_board_info.is_wifi_disable = 0;
    }

    ret = of_property_read_bool(np, DTS_PROP_HI110X_BFGX_DISABLE);
    if (ret) {
        g_st_board_info.is_bfgx_disable = 1;
    } else {
        g_st_board_info.is_bfgx_disable = 0;
    }

    hi110x_emu_init();

    if (hi110x_is_emu() == OAL_TRUE) {
        // non-bfgx in emu
        g_st_board_info.is_bfgx_disable = 1;
    }

    ps_print_info("wifi %s, bfgx %s\n",
                  (g_st_board_info.is_wifi_disable == 0) ? "enabled" : "disabled",
                  (g_st_board_info.is_bfgx_disable == 0) ? "enabled" : "disabled");

    return BOARD_SUCC;
#else
    return BOARD_SUCC;
#endif
}

STATIC int32_t hi110x_check_pmu_clk_share(void)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32_t ret;
    struct device_node *np = NULL;
    int32_t pmu_clk_request = PMU_CLK_REQ_DISABLE;

    ret = get_board_dts_node(&np, DTS_NODE_HISI_HI110X);
    if (ret != BOARD_SUCC) {
        ps_print_err("DTS read node %s fail!!!\n", DTS_NODE_HISI_HI110X);
        return BOARD_FAIL;
    }

    ret = of_property_read_u32(np, DTS_PROP_HI110X_PMU_CLK, &pmu_clk_request);
    if (ret != BOARD_SUCC) {
        ps_print_info("get dts prop %s failed, hi110x pmu clk request disable\n", DTS_PROP_HI110X_PMU_CLK);
        g_st_board_info.pmu_clk_share_enable = PMU_CLK_REQ_DISABLE;
    } else {
        ps_print_info("hi110x PMU clk request is %s\n", (pmu_clk_request ? "enable" : "disable"));
        g_st_board_info.pmu_clk_share_enable = pmu_clk_request;
    }

    return BOARD_SUCC;
#else
    return BOARD_SUCC;
#endif
}

STATIC int32_t hi110x_board_get_power_pinctrl(void)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32_t ret;
    int32_t physical_gpio = 0;
    struct device_node *np = NULL;

    ret = get_board_dts_node(&np, DTS_NODE_HISI_HI110X);
    if (ret != BOARD_SUCC) {
        ps_print_err("DTS read node %s fail!!!\n", DTS_NODE_HISI_HI110X);
        return BOARD_FAIL;
    }

    ret = of_property_read_bool(np, DTS_PROP_HI110X_POWER_PREPARE);
    if (ret) {
        ps_print_info("need prepare before board power on\n");
        g_st_board_info.need_power_prepare = NEED_POWER_PREPARE;
    } else {
        ps_print_info("no need prepare before board power on\n");
        g_st_board_info.need_power_prepare = NO_NEED_POWER_PREPARE;
        return BOARD_SUCC;
    }

    ret = get_board_gpio(DTS_NODE_HISI_HI110X, DTS_PROP_HI110X_GPIO_TCXO_FREQ_DETECT, &physical_gpio);
    if (ret != BOARD_SUCC) {
        ps_print_err("get dts prop %s failed\n", DTS_PROP_HI110X_GPIO_TCXO_FREQ_DETECT);
        return BOARD_FAIL;
    }

    g_st_board_info.tcxo_freq_detect = physical_gpio;

    if (get_cust_conf_int32(INI_MODU_PLAT, "tcxo_gpio_level", &physical_gpio) ==  INI_FAILED) {
        ps_print_info("get tcxo value from dts\n");
        ret = of_property_read_u32(np, DTS_PROP_GPIO_TCXO_LEVEL, &physical_gpio);
        if (ret != BOARD_SUCC) {
            ps_print_err("get dts prop %s failed\n", DTS_PROP_GPIO_TCXO_LEVEL);
            return BOARD_FAIL;
        }
    }

    // 0:tcxo 76.8M  1:tcxo 38.4M
    ps_print_info("have get dts_prop and prop_val: %s=%d\n", DTS_PROP_GPIO_TCXO_LEVEL, physical_gpio);
    g_st_board_info.gpio_tcxo_detect_level = physical_gpio;
#endif

    return BOARD_SUCC;
}

int32_t hi110x_get_ini_file_name_from_dts(char *dts_prop, char *prop_value, uint32_t size)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32_t ret;
    struct device_node *np = NULL;
    int32_t len;
    char out_str[HISI_CUST_NVRAM_LEN] = { 0 };

    np = of_find_compatible_node(NULL, NULL, COST_HI110X_COMP_NODE);
    if (np == NULL) {
        ini_error("dts node %s not found", COST_HI110X_COMP_NODE);
        return INI_FAILED;
    }

    len = of_property_count_u8_elems(np, dts_prop);
    if (len < 0) {
        ini_error("can't get len of dts prop(%s)", dts_prop);
        return INI_FAILED;
    }

    len = ini_min(len, (int32_t)sizeof(out_str));
    ini_debug("read len of dts prop %s is:%d", dts_prop, len);
    ret = of_property_read_u8_array(np, dts_prop, out_str, len);
    if (ret < 0) {
        ini_error("read dts prop (%s) fail", dts_prop);
        return INI_FAILED;
    }

    len = ini_min(len, (int32_t)size);
    if (memcpy_s(prop_value, (size_t)size, out_str, (size_t)len) != EOK) {
        ini_error("memcpy_s error, destlen=%d, srclen=%d\n ", size, len);
        return INI_FAILED;
    }
    ini_debug("dts prop %s value is:%s", dts_prop, prop_value);
#endif
    return INI_SUCC;
}

STATIC int32_t hi110x_gpio_dts_init(struct platform_device *pdev)
{
    int32_t ret;

    /* power on gpio request */
    ret = hi110x_get_board_power_gpio(pdev);
    if (ret != BOARD_SUCC) {
        ps_print_err("get power_on dts prop failed\n");
        goto err_get_power_on_gpio;
    }

    ret = hi110x_board_wakeup_gpio_init(pdev);
    if (ret != BOARD_SUCC) {
        ps_print_err("get wakeup prop failed\n");
        goto oal_board_wakup_gpio_fail;
    }

    ret = hi110x_board_wifi_tas_gpio_init();
    if (ret != BOARD_SUCC) {
        ps_print_err("get wifi tas prop failed\n");
        goto oal_board_wifi_tas_gpio_fail;
    }

    ret = hi110x_board_flowctrl_gpio_init();
    if (ret != BOARD_SUCC) {
        ps_print_err("get wifi tas prop failed\n");
        goto oal_board_flowctrl_gpio_fail;
    }

    return BOARD_SUCC;

oal_board_flowctrl_gpio_fail:
    hi110x_free_board_wifi_tas_gpio();
oal_board_wifi_tas_gpio_fail:
    hi110x_free_board_wakeup_gpio();
oal_board_wakup_gpio_fail:
    hi110x_free_board_power_gpio();
err_get_power_on_gpio:
    chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                         CHR_PLT_DRV_EVENT_INIT, CHR_PLAT_DRV_ERROR_BOARD_GPIO_INIT);

    return BOARD_FAIL;
}

STATIC void hi110x_gpio_dts_free(struct platform_device *pdev)
{
    hi110x_free_board_ir_gpio();
    hi110x_free_board_flowctrl_gpio();
    hi110x_free_board_wifi_tas_gpio();
    hi110x_free_board_wakeup_gpio();
    hi110x_free_board_power_gpio();
}

STATIC void hi110x_coldboot_partion_init(void)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32_t ret;
    const char *coldboot_partion = NULL;

    ret = get_board_custmize(DTS_NODE_HI110X_BFGX, DTS_PROP_HI110X_COLDBOOT_PARTION, &coldboot_partion);
    if (ret == BOARD_SUCC) {
        g_st_board_info.coldboot_partion = coldboot_partion;
        ps_print_info("coldboot_partion: %s\n",  coldboot_partion);
    }
#endif
}


STATIC int32_t hi110x_sys_attr_dts_init(struct platform_device *pdev)
{
    int32_t ret;
    ret = hi110x_check_hi110x_subsystem_support();
    if (ret != BOARD_SUCC) {
        ps_print_err("hi110x_check_hi110x_subsystem_support failed\n");
        goto sys_cfg_fail;
    }

    ret = hi110x_check_evb_or_fpga();
    if (ret != BOARD_SUCC) {
        ps_print_err("hi110x_check_evb_or_fpga failed\n");
        goto sys_cfg_fail;
    }

    ret = hi110x_check_pmu_clk_share();
    if (ret != BOARD_SUCC) {
        ps_print_err("hi110x_check_pmu_clk_share failed\n");
        goto sys_cfg_fail;
    }

    ret = hi110x_get_board_pmu_clk32k(pdev);
    if (ret != BOARD_SUCC) {
        ps_print_err("hi110x_check_pmu_clk_share failed\n");
        goto sys_cfg_fail;
    }

    ret = hi110x_get_board_uart_port();
    if (ret != BOARD_SUCC) {
        ps_print_err("get uart port failed\n");
        goto sys_cfg_fail;
    }

    ret = hi110x_board_get_power_pinctrl();
    if (ret != BOARD_SUCC) {
        ps_print_err("hi110x_board_get_power_pinctrl failed\n");
        goto sys_cfg_fail;
    }

    ret = hi110x_board_ir_ctrl_init(pdev);
    if (ret != BOARD_SUCC) {
        ps_print_err("get ir dts prop failed\n");
        goto sys_cfg_fail;
    }

    hi110x_coldboot_partion_init();

    return BOARD_SUCC;

sys_cfg_fail:
    return  BOARD_FAIL;
}

STATIC void hi110x_wlan_wakeup_host_property_init(void)
{
    int32_t ret;
    int32_t cfg_val;

    ret = get_cust_conf_int32(INI_MODU_PLAT, INI_WLAN_WAKEUP_HOST_REVERSE, &cfg_val);
    if (ret == INI_FAILED) {
        ps_print_err("get %s failed\n", INI_WLAN_WAKEUP_HOST_REVERSE);
    }

    g_st_board_info.wlan_wakeup_host_have_reverser = cfg_val;
    ps_print_info("wlan_wakeup_host have reverser is or not: [%d]\n", cfg_val);
}

/* 产品特殊属性配置 */
STATIC int32_t hi110x_product_attr_init(struct platform_device *pdev)
{
    hi110x_wlan_wakeup_host_property_init();
    return BOARD_SUCC;
}


void board_callback_init_dts(void)
{
    g_st_board_info.bd_ops.board_gpio_init = hi110x_gpio_dts_init;
    g_st_board_info.bd_ops.board_gpio_free = hi110x_gpio_dts_free;
    g_st_board_info.bd_ops.board_sys_attr_init = hi110x_sys_attr_dts_init;
    g_st_board_info.bd_ops.board_product_attr_init = hi110x_product_attr_init;
}
