

/* 头文件包含 */
#include "board.h"
#include "chr_user.h"
#include "plat_debug.h"
#include "bfgx_dev.h"
#include "securec.h"

#ifdef _PRE_HI_DRV_GPIO
#ifdef _PRE_PRODUCT_HI3751_PLATO
#define POWER_ON_ENABLE_GPIO (21 * 8 + 0) /* GPIO21_0 */
#define WLAN_POWER_ON_ENABLE_GPIO (9 * 8 + 2) /* GPIO9_2 */
#define BFGN_POWER_ON_ENABLE_GPIO (19 * 8 + 6) /* GPIO19_6 */
#define WLAN_WAKEUP_HOST_GPIO (21 * 8 + 1) /* GPIOP21_1 */
#define HOST_WAKEUP_WLAN_GPIO (9 * 8 + 1) /* STB_GPIO9_1 */
#else
#define POWER_ON_ENABLE_GPIO (20 * 8 + 5) /* PMU ON STB_GPIOPWM20_5 */
#define WLAN_POWER_ON_ENABLE_GPIO (18 * 8 + 5) /* S_ADC2/STB_GPIO18_5 */
#define BFGN_POWER_ON_ENABLE_GPIO (18 * 8 + 4) /* LS_ADC1/STB_GPIO18_4 */
#define WLAN_WAKEUP_HOST_GPIO (19 * 8 + 3) /* STB_GPIOPWM20_5/UART4_CTSN */
#define HOST_WAKEUP_WLAN_GPIO (19 * 8 + 7) /* HDMI3_DET/STB_GPIOPWM19_7/TSI3_D7 */
#endif
#define BFGN_WAKEUP_HOST_GPIO (19 * 8 + 1) /* STB_GPIOPWM19_1/UART3_RXD */
#define HOST_WAKEUP_BFGN_GPIO (20 * 8 + 6)
#define HOST_WAKEUP_BFGN_GPIO_HEGEL (20 * 8 + 1)
#define WIFI_PRODUCT_VER_OSCA_550 0
#define WIFI_PRODUCT_VER_OSCA_550A 1

#define SSI_CLK_HISI_GPIO 7*8
#define SSI_DATA_HISI_GPIO (6 * 8 + 7)

STATIC void hitv_ssi_gpio_init(void)
{
    hitv_gpio_request(SSI_CLK_HISI_GPIO, 0);
    hitv_gpio_direction_output(SSI_CLK_HISI_GPIO, GPIO_HIGHLEVEL);
    hitv_gpio_request(SSI_DATA_HISI_GPIO, 0);
    hitv_gpio_direction_output(SSI_DATA_HISI_GPIO, GPIO_HIGHLEVEL);
}

STATIC int32_t hitv_get_board_power_gpio(struct platform_device *pdev)
{
    /* use hi3751 raw gpio api */
    g_st_board_info.power_on_enable = POWER_ON_ENABLE_GPIO;
    g_st_board_info.sys_enable[W_SYS] = WLAN_POWER_ON_ENABLE_GPIO;
    hitv_gpio_request(g_st_board_info.power_on_enable, 0);
    hitv_gpio_direction_output(g_st_board_info.power_on_enable, 1);
    hitv_gpio_request(g_st_board_info.sys_enable[W_SYS], 0);
    hitv_gpio_direction_output(g_st_board_info.sys_enable[W_SYS], 1);

    g_st_board_info.sys_enable[B_SYS] = BFGN_POWER_ON_ENABLE_GPIO;

    return BOARD_SUCC;
}

STATIC int32_t hitv_board_wakeup_gpio_init(struct platform_device *pdev)
{
    g_st_board_info.dev_wakeup_host[W_SYS] = WLAN_WAKEUP_HOST_GPIO;
    hitv_gpio_request(g_st_board_info.dev_wakeup_host[W_SYS], 1);
    gpio_request(g_st_board_info.dev_wakeup_host[W_SYS], "wlan_wakeup_host");
    g_st_board_info.dev_wakeup_host[B_SYS] = BFGN_WAKEUP_HOST_GPIO;
    hitv_gpio_request(g_st_board_info.dev_wakeup_host[B_SYS], 1);

    g_st_board_info.host_wakeup_dev[W_SYS] = HOST_WAKEUP_WLAN_GPIO;
    hitv_gpio_request(g_st_board_info.host_wakeup_dev[W_SYS], 0);
    hitv_gpio_direction_output(g_st_board_info.host_wakeup_dev[W_SYS], 0);

#ifdef _PRE_PRODUCT_HI3751_PLATO
    g_st_board_info.host_wakeup_dev[B_SYS] = HOST_WAKEUP_BFGN_GPIO_HEGEL;
    hitv_gpio_request(g_st_board_info.host_wakeup_dev[B_SYS], GPIO_LOWLEVEL); /* Req PCIE_rst GPIO */
    hitv_gpio_direction_output(g_st_board_info.host_wakeup_dev[B_SYS], 0);
    ps_print_info("bfg plato wakeup gpio 20-1 \n");
#else
    /* when is HEGEL,use GPIO 20_1,set output mode,other use default 20_6  */
    /* fix it when dts is ready */
    if (PdmGetHwVer() == WIFI_PRODUCT_VER_OSCA_550 || PdmGetHwVer() == WIFI_PRODUCT_VER_OSCA_550A) {
        g_st_board_info.host_wakeup_dev[B_SYS] = HOST_WAKEUP_BFGN_GPIO;
        hitv_gpio_request(g_st_board_info.host_wakeup_dev[B_SYS], GPIO_LOWLEVEL); /* Req PCIE_rst GPIO */
        ps_print_info("bfg no hegel wakeup gpio still is 20-6 \n");
    } else {
        /* 1.if is hegel,will change pcie_perst gpio port to MHL_CD_SENSE/STB_GPIOPWM20_1 */
        g_st_board_info.host_wakeup_dev[B_SYS] = HOST_WAKEUP_BFGN_GPIO_HEGEL;
        hitv_gpio_request(g_st_board_info.host_wakeup_dev[B_SYS], GPIO_LOWLEVEL); /* Req PCIE_rst GPIO */
        /* 2.in hegel must change port mode to output. */
        hitv_gpio_direction_output(g_st_board_info.host_wakeup_dev[B_SYS], 0);
        ps_print_info("bfg hegel wakeup gpio change to 20-1 \n");
    }
#endif

    return BOARD_SUCC;
}

STATIC int32_t hitv_gpio_init(struct platform_device *pdev)
{
    int32_t ret;

    hitv_ssi_gpio_init();

    /* power on gpio request */
    ret = hitv_get_board_power_gpio(pdev);
    if (ret != BOARD_SUCC) {
        ps_print_err("hitv_get_board_power_gpio failed\n");
        return BOARD_FAIL;
    }

    ret = hitv_board_wakeup_gpio_init(pdev);
    if (ret != BOARD_SUCC) {
        ps_print_err("hitv_board_wakeup_gpio_init\n");
        return BOARD_FAIL;
    }

    return BOARD_SUCC;
}

void board_info_init_tv(void)
{
    /* GPIO部分初始化接口替换DTS接口 */
    g_st_board_info.bd_ops.board_gpio_init = hitv_gpio_init;
    g_st_board_info.bd_ops.board_gpio_free = NULL;
}
#endif

#ifdef _PRE_SUSPORT_OEMINFO
int32_t hi1103_get_wifi_calibrate_in_oeminfo_state(const char *node, const char *prop, int32_t *prop_val)
{
    int32_t ret;
    struct device_node *np = NULL;

    if (node == NULL || prop == NULL || prop_val == NULL) {
        ps_print_suc("func has NULL input param!\n");
        return BOARD_FAIL;
    }

    ret = get_board_dts_node(&np, node);
    if (ret != BOARD_SUCC) {
        return BOARD_FAIL;
    }

    ret = of_property_read_u32(np, prop, prop_val);
    if (ret) {
        ps_print_suc("can't get dts prop val: prop=%s\n", prop);
        return ret;
    }

    return BOARD_SUCC;
}

int32_t is_hitv_miniproduct(void)
{
    int32_t wifi_calibrate_in_oeminfo = 0;
    int32_t ret;
    ret = hi1103_get_wifi_calibrate_in_oeminfo_state(DTS_NODE_HI110X_WIFI, DTS_WIFI_CALIBRATE_IN_OEMINFO,
                                                     &wifi_calibrate_in_oeminfo);
    if (ret != BOARD_SUCC) {
        return ret;
    }

    if (wifi_calibrate_in_oeminfo == 1) {
        ps_print_info("hitv is mini product.\n");
        return BOARD_SUCC;
    }
    return BOARD_FAIL;
}
#endif
