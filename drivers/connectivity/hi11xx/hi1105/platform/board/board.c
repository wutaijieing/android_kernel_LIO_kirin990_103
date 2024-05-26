

/* 头文件包含 */
#ifdef _PRE_CONFIG_USE_DTS
#include <linux/of.h>
#include <linux/of_gpio.h>
#endif

#ifdef _PRE_PRODUCT_HI1620S_KUNPENG
#include <linux/acpi.h>
#include <linux/gpio/consumer.h>
#endif

/*lint -e322*/ /*lint -e7*/
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/delay.h>
#ifdef CONFIG_PINCTRL
#include <linux/pinctrl/consumer.h>
#endif

#include <linux/fs.h>
/*lint +e322*/ /*lint +e7*/

#if defined(CONFIG_LOG_EXCEPTION) && !defined(CONFIG_HI110X_KERNEL_MODULES_BUILD_SUPPORT)
#include <log/log_usertype.h>
#endif
#ifdef CONFIG_HWCONNECTIVITY_PC
#include "board_hi1103_kunpeng.h"
#endif
#include "board_bisheng.h"
#include "board_hi1161.h"
#include "plat_debug.h"
#include "plat_firmware.h"
#include "plat_pm.h"
#include "oam_ext_if.h"
#include "platform_common_clk.h"
#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
#include "pcie_linux.h"
#endif
#ifdef PLATFORM_SSI_DEBUG_CMD
#include "ssi_dbg_cmd.h"
#endif

/* 全局变量定义 */
hi110x_board_info g_st_board_info = { .ssi_gpio_clk = 0, .ssi_gpio_data = 0 };
EXPORT_SYMBOL(g_st_board_info);

OAL_STATIC int32_t g_board_probe_ret = 0;
OAL_STATIC struct completion g_board_driver_complete;
int g_emu_latency_times = 10; // 简单处理, 处理时间乘10
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
oal_debug_module_param(g_emu_latency_times, int, S_IRUGO | S_IWUSR);
#endif

OAL_STATIC device_board_version g_hi110x_board_version_list[] = {
    { .index = BOARD_VERSION_HI1102,  .name = BOARD_VERSION_NAME_HI1102 },
    { .index = BOARD_VERSION_HI1103,  .name = BOARD_VERSION_NAME_HI1103 },
    { .index = BOARD_VERSION_HI1102A, .name = BOARD_VERSION_NAME_HI1102A },
    { .index = BOARD_VERSION_HI1105,  .name = BOARD_VERSION_NAME_HI1105 },
    { .index = BOARD_VERSION_SHENKUO,  .name = BOARD_VERSION_NAME_SHENKUO },
    { .index = BOARD_VERSION_BISHENG, .name = BOARD_VERSION_NAME_BISHENG },
    { .index = BOARD_VERSION_HI1131K, .name = BOARD_VERSION_NAME_HI113K },
    { .index = BOARD_VERSION_HI1161, .name = BOARD_VERSION_NAME_HI1161 },
};

OAL_STATIC download_channel g_hi110x_download_channel_list[CHANNEL_DOWNLOAD_BUTT] = {
    { .index = CHANNEL_SDIO, .name = DOWNLOAD_CHANNEL_SDIO },
    { .index = CHANNEL_PCIE, .name = DOWNLOAD_CHANNEL_PCIE },
    { .index = CHANNEL_UART, .name = DOWNLOAD_CHANNEL_UART },
    { .index = CHANNEL_USB,  .name = DOWNLOAD_CHANNEL_USB },
};

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
OAL_STATIC int g_hi11xx_os_build_variant = HI1XX_OS_BUILD_VARIANT_USER; /* default user mode */
#endif

uint32_t g_ssi_dump_en = 0;

/* 函数定义 */
inline hi110x_board_info *get_hi110x_board_info(void)
{
    return &g_st_board_info;
}
EXPORT_SYMBOL_GPL(get_hi110x_board_info);

int32_t hi110x_firmware_download_mode(void)
{
    return g_st_board_info.download_mode;
}

int hi110x_is_asic(void)
{
    if (g_st_board_info.is_asic == VERSION_ASIC) {
        return VERSION_ASIC;
    } else {
        return VERSION_FPGA;
    }
}
EXPORT_SYMBOL_GPL(hi110x_is_asic);

int hi110x_is_emu(void)
{
    return g_st_board_info.is_emu;
}
EXPORT_SYMBOL(hi110x_is_emu);

/*
 * 函 数 名  : hi110x_cmu_is_tcxo_dll
 * 功能描述  : [TCXO时钟是38.4M还是76.8M]
 * 返 回 值  : 1:TCXO_FREQ_DET_38P4M 38.4M
 *             0:TCXO_FREQ_DET_76P8M 76.8M
 */
uint16_t hi110x_cmu_is_tcxo_dll(void)
{
    /* gpio高低电平代表的tcxo频率与芯片强相关,不同芯片可能选用不同gpio */
    if ((get_hi110x_subchip_type() == BOARD_VERSION_SHENKUO) ||
        (get_hi110x_subchip_type() == BOARD_VERSION_BISHENG)) {
        // 0:tcxo 76.8M  1:tcxo 38.4M
        if (g_st_board_info.gpio_tcxo_detect_level == 0) {
            return TCXO_FREQ_DET_76P8M;
        }
    } else {
        // 1:tcxo 76.8M  0:tcxo 38.4M
        if (g_st_board_info.gpio_tcxo_detect_level == 1) {
            return TCXO_FREQ_DET_76P8M;
        }
    }
    return TCXO_FREQ_DET_38P4M;
}
EXPORT_SYMBOL(hi110x_cmu_is_tcxo_dll);

void hi110x_emu_mdelay(int msec)
{
    if (hi110x_is_emu() == OAL_TRUE) {
        ps_print_info("emu delay %d msec\n", msec);
        msleep(msec);
    }
}
EXPORT_SYMBOL(hi110x_emu_mdelay);

unsigned int hi110x_get_emu_timeout(unsigned int timeout)
{
    if (hi110x_is_emu() == OAL_TRUE) {
        unsigned int ret = timeout * (unsigned int)g_emu_latency_times;
        ps_print_info("emu timeout %u to %u\n", timeout, ret);
        return ret;
    }
    return timeout;
}
EXPORT_SYMBOL(hi110x_get_emu_timeout);

int is_wifi_support(void)
{
    if (g_st_board_info.is_wifi_disable == 0) {
        return OAL_TRUE;
    } else {
        return OAL_FALSE;
    }
}
EXPORT_SYMBOL_GPL(is_wifi_support);

int is_bfgx_support(void)
{
    if (g_st_board_info.is_bfgx_disable == 0) {
        return OAL_TRUE;
    } else {
        return OAL_FALSE;
    }
}
EXPORT_SYMBOL_GPL(is_bfgx_support);

int is_gt_support(void)
{
    if (g_st_board_info.is_gt_disable == 0) {
        return OAL_TRUE;
    } else {
        return OAL_FALSE;
    }
}
EXPORT_SYMBOL_GPL(is_gt_support);

int pmu_clk_request_enable(void)
{
    if (g_st_board_info.pmu_clk_share_enable == PMU_CLK_REQ_ENABLE) {
        return PMU_CLK_REQ_ENABLE;
    } else {
        return PMU_CLK_REQ_DISABLE;
    }
}

int is_hi110x_debug_type(void)
{
#if defined(CONFIG_LOG_EXCEPTION) && !defined(CONFIG_HI110X_KERNEL_MODULES_BUILD_SUPPORT)
    if (get_logusertype_flag() == BETA_USER) {
        ps_print_info("current soft version is beta user\n");
        return OAL_TRUE;
    }
#endif

    if (hi11xx_get_os_build_variant() == HI1XX_OS_BUILD_VARIANT_USER) {
        return OAL_FALSE;
    }

    return OAL_TRUE;
}
EXPORT_SYMBOL_GPL(is_hi110x_debug_type);

int32_t get_hi110x_subchip_type(void)
{
    hi110x_board_info *bd_info = NULL;

    bd_info = get_hi110x_board_info();
    if (unlikely(bd_info == NULL)) {
        ps_print_err("board info is err\n");
        return -EFAIL;
    }

    return bd_info->chip_nr;
}
EXPORT_SYMBOL_GPL(get_hi110x_subchip_type);

#ifdef _PRE_CONFIG_USE_DTS
int32_t get_board_dts_node(struct device_node **np, const char *node_prop)
{
    if (np == NULL || node_prop == NULL) {
        ps_print_err("func has NULL input param!!!, np=%p, node_prop=%p\n", np, node_prop);
        return BOARD_FAIL;
    }

    *np = of_find_compatible_node(NULL, NULL, node_prop);
    if (*np == NULL) {
        ps_print_err("No compatible node %s found.\n", node_prop);
        return BOARD_FAIL;
    }

    return BOARD_SUCC;
}

int32_t get_board_dts_prop(struct device_node *np, const char *dts_prop, const char **prop_val)
{
    int32_t ret;

    if (np == NULL || dts_prop == NULL || prop_val == NULL) {
        ps_print_err("func has NULL input param!!!, np=%p, dts_prop=%p, prop_val=%p\n", np, dts_prop, prop_val);
        return BOARD_FAIL;
    }

    ret = of_property_read_string(np, dts_prop, prop_val);
    if (ret) {
        ps_print_err("can't get dts_prop value: dts_prop=%s\n", dts_prop);
        return ret;
    }

    return BOARD_SUCC;
}

int32_t get_board_dts_gpio_prop(struct device_node *np, const char *dts_prop, int32_t *prop_val)
{
    int32_t ret;

    if (np == NULL || dts_prop == NULL || prop_val == NULL) {
        ps_print_err("func has NULL input param!!!, np=%p, dts_prop=%p, prop_val=%p\n", np, dts_prop, prop_val);
        return BOARD_FAIL;
    }

    ret = of_get_named_gpio(np, dts_prop, 0);
    if (ret < 0) {
        ps_print_err("can't get dts_prop value: dts_prop=%s, ret=%d\n", dts_prop, ret);
        return ret;
    }

    *prop_val = ret;
    ps_print_suc("have get dts_prop and prop_val: %s=%d\n", dts_prop, *prop_val);

    return BOARD_SUCC;
}

#endif

int32_t board_gpio_request(int32_t physical_gpio, const char *gpio_name, uint32_t direction)
{
    int32_t ret;
    if (direction == GPIO_DIRECTION_INPUT) {
        ret = gpio_request_one(physical_gpio, GPIOF_IN, gpio_name);
    } else {
#if (defined _PRE_HI375X_PCIE) || (defined _PRE_HI5652H_BOARD)
        ret = gpio_request_one(physical_gpio, GPIOF_OUT_INIT_HIGH, gpio_name);
#else
        ret = gpio_request_one(physical_gpio, GPIOF_OUT_INIT_LOW, gpio_name);
#endif
    }

    if (ret != BOARD_SUCC) {
        ps_print_err("%s[%d] gpio_request failed\n", gpio_name, physical_gpio);
        return BOARD_FAIL;
    }

    return BOARD_SUCC;
}

int32_t get_board_gpio(const char *gpio_node, const char *gpio_prop, int32_t *physical_gpio)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32_t ret;
    struct device_node *np = NULL;

    ret = get_board_dts_node(&np, gpio_node);
    if (ret != BOARD_SUCC) {
        return BOARD_FAIL;
    }

    ret = get_board_dts_gpio_prop(np, gpio_prop, physical_gpio);
    if (ret != BOARD_SUCC) {
        return BOARD_FAIL;
    }

    return BOARD_SUCC;
#else
    return BOARD_SUCC;
#endif
}

int32_t get_board_custmize(const char *cust_node, const char *cust_prop, const char **cust_prop_val)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32_t ret;
    struct device_node *np = NULL;

    if (cust_node == NULL || cust_prop == NULL || cust_prop_val == NULL) {
        ps_print_err("func has NULL input param!!!\n");
        return BOARD_FAIL;
    }

    ret = get_board_dts_node(&np, cust_node);
    if (ret != BOARD_SUCC) {
        return BOARD_FAIL;
    }

    ret = get_board_dts_prop(np, cust_prop, cust_prop_val);
    if (ret != BOARD_SUCC) {
        return BOARD_FAIL;
    }

    ps_print_info("get board customize info %s=%s\n", cust_prop, *cust_prop_val);

    return BOARD_SUCC;
#else
    return BOARD_FAIL;
#endif
}

#if (defined _PRE_PRODUCT_HI1620S_KUNPENG) || (defined _PRE_HI375X_NO_DTS) || (defined _PRE_WINDOWS_SUPPORT)
int32_t enable_board_pmu_clk32k(void)
{
    return BOARD_SUCC;
}
int32_t disable_board_pmu_clk32k(void)
{
    return BOARD_SUCC;
}
#else
int32_t enable_board_pmu_clk32k(void)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32_t ret;
    struct clk *clk = NULL;

    clk = g_st_board_info.clk_32k;

    if (clk != NULL) {
        if (!__clk_is_enabled(clk)) {
            ret = clk_prepare_enable(clk);
            if (unlikely(ret < 0)) {
                ps_print_err("enable 32K clk failed!!!\n");
                chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                    CHR_PLT_DRV_EVENT_CLK, CHR_PLAT_DRV_ERROR_32K_CLK_EN);
                return BOARD_FAIL;
            }
        }
        ps_print_info("enable 32K clk success!\n");
        return BOARD_SUCC;
    } else {
        ps_print_err("clk32k not config!\n");
        return BOARD_FAIL;
    }
#endif
    return BOARD_SUCC;
}
int32_t disable_board_pmu_clk32k(void)
{
#ifdef _PRE_CONFIG_USE_DTS
    struct clk *clk = NULL;

    clk = g_st_board_info.clk_32k;

    if (clk != NULL) {
        if (__clk_is_enabled(clk)) {
            clk_disable_unprepare(clk);
        }
        ps_print_info("disable 32K clk success!\n");
        return BOARD_SUCC;
    } else {
        ps_print_err("clk32k not config!\n");
        return BOARD_FAIL;
    }
#endif
    return BOARD_SUCC;
}
#endif

void board_flowctrl_irq_init(void)
{
    if (g_st_board_info.flowctrl_gpio) {
        g_st_board_info.flowctrl_irq = oal_gpio_to_irq(g_st_board_info.flowctrl_gpio);
    } else {
        g_st_board_info.flowctrl_irq = 0;
    }
}

static int32_t board_gpio_init(struct platform_device *pdev)
{
#ifdef _PRE_WINDOWS_SUPPORT
    return BOARD_SUCC;
#else
    if (g_st_board_info.bd_ops.board_gpio_init) {
        return  g_st_board_info.bd_ops.board_gpio_init(pdev);
    } else {
        return BOARD_SUCC;
    }
#endif
}

static void board_gpio_free(struct platform_device *pdev)
{
#ifdef _PRE_WINDOWS_SUPPORT
#else
    if (g_st_board_info.bd_ops.board_gpio_free) {
        g_st_board_info.bd_ops.board_gpio_free(pdev);
    }
#endif
}

static int32_t board_sys_attr_init(struct platform_device *pdev)
{
    if (g_st_board_info.bd_ops.board_sys_attr_init) {
        return g_st_board_info.bd_ops.board_sys_attr_init(pdev);
    } else {
        return BOARD_SUCC;
    }
}

static int32_t board_irq_init(struct platform_device *pdev)
{
    if (g_st_board_info.bd_ops.board_irq_init != NULL) {
        return g_st_board_info.bd_ops.board_irq_init(pdev);
    } else {
        return BOARD_SUCC;
    }
}

static int32_t board_product_attr_init(struct platform_device *pdev)
{
    if (g_st_board_info.bd_ops.board_product_attr_init) {
        return g_st_board_info.bd_ops.board_product_attr_init(pdev);
    } else {
        return BOARD_SUCC;
    }
}

#ifdef _PRE_CONFIG_HISI_S3S4_POWER_STATE
int board_get_sys_enable_gpio_val(uint8_t sys_en)
{
    if (sys_en >= SYS_BUTT) {
        return -1;
    }

    return gpio_get_value(g_st_board_info.sys_enable[sys_en]);
}
#endif

void power_state_change(int32_t gpio, int32_t flag)
{
#ifdef _PRE_PRODUCT_HI3751V811
#else
    if (unlikely(gpio == 0)) {
        ps_print_warning("gpio is 0, flag=%d\n", flag);
        return;
    }
#endif
    ps_print_info("power_state_change gpio %d current state %d, to %s\n", gpio, oal_gpio_get_value(gpio),
                  (flag == BOARD_POWER_ON) ? "low2high" : "low");

    if (flag == BOARD_POWER_ON) {
        oal_gpio_direction_output(gpio, GPIO_LOWLEVEL);
        mdelay(10); /* delay 10ms */
        oal_gpio_direction_output(gpio, GPIO_HIGHLEVEL);
        mdelay(20); /* delay 20ms */
    } else if (flag == BOARD_POWER_OFF) {
        oal_gpio_direction_output(gpio, GPIO_LOWLEVEL);
        mdelay(5); /* delay 5ms */
    }
}

static int32_t check_download_channel_pcie(void)
{
    int32_t ret;
    uint8_t download_channel[DOWNLOAD_CHANNEL_LEN] = {0};

    ret = get_cust_conf_string(INI_MODU_PLAT, INI_WLAN_DOWNLOAD_CHANNEL, download_channel, sizeof(download_channel));
    if (ret != INI_SUCC) {
        ps_print_warning("%s can not find download_channel"NEWLINE,  __func__);
        return BOARD_FAIL;
    }

    if (oal_strncmp(DOWNLOAD_CHANNEL_PCIE, download_channel, strlen(DOWNLOAD_CHANNEL_PCIE)) != 0) {
        ps_print_warning("check download channel is %s, not %s\n", download_channel, DOWNLOAD_CHANNEL_PCIE);
        return BOARD_FAIL;
    }

    ps_print_dbg("%s check download channel match %s\n", __func__, DOWNLOAD_CHANNEL_PCIE);
    return BOARD_SUCC;
}

int32_t board_wlan_gpio_power_on(void *data)
{
    int32_t gpio = (int32_t)(uintptr_t)(data);

    if (g_st_board_info.host_wakeup_dev[W_SYS]) {
        /* host wakeup dev gpio pinmux to dft jtag when w boot,
          must gpio low when bootup */
        board_host_wakeup_dev_set(0);
    }
    power_state_change(gpio, BOARD_POWER_ON);

    /* 如果总线不是pcie,不进行phy初始化; 此时bus_type未初始化,直接从ini读取 */
    if (check_download_channel_pcie() == BOARD_SUCC) {
        if (g_st_board_info.ep_pcie0_rst) {
            /* 拉高pcie_rst */
            oal_gpio_direction_output(g_st_board_info.ep_pcie0_rst, GPIO_LOWLEVEL);
            mdelay(1);
            oal_gpio_direction_output(g_st_board_info.ep_pcie0_rst, GPIO_HIGHLEVEL);
            mdelay(10); /* delay 10ms */
            ps_print_info("pcie0_rst %d pull up\n", g_st_board_info.ep_pcie0_rst);
        }

        hi110x_emu_mdelay(50000); // 50000 msec, 等待慢速侧PCIE建链, 时间取决于EMU平台的性能
#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
        pcie_linkup_prepare_fixup(0);
#endif
    }

    board_host_wakeup_dev_set(1);
    return 0;
}

int32_t board_wlan_gpio_power_off(void *data)
{
    int32_t gpio = (int32_t)(uintptr_t)(data);
    if (g_st_board_info.ep_pcie0_rst) {
        /* 拉低pcie_rst */
        oal_gpio_direction_output(g_st_board_info.ep_pcie0_rst, GPIO_LOWLEVEL);
        mdelay(1);
        ps_print_info("pcie0_rst %d pull down\n", g_st_board_info.ep_pcie0_rst);
    }
    power_state_change(gpio, BOARD_POWER_OFF);
    hi110x_emu_mdelay(10000); // 10000:5ms * 1000 * 2
    return 0;
}

int32_t board_host_wakeup_dev_set(int value)
{
    if (g_st_board_info.host_wakeup_dev[W_SYS] == 0) {
        ps_print_info("host_wakeup_wlan gpio is 0\n");
        return 0;
    }
    ps_print_info("host_wakeup_wlan set %s\n", value ? "high" : "low");
    if (value) {
        return oal_gpio_direction_output(g_st_board_info.host_wakeup_dev[W_SYS], GPIO_HIGHLEVEL);
    } else {
        return oal_gpio_direction_output(g_st_board_info.host_wakeup_dev[W_SYS], GPIO_LOWLEVEL);
    }
}

int32_t board_host_wakeup_bfg_set(int value)
{
    if (g_st_board_info.host_wakeup_dev[B_SYS] == 0) {
        ps_print_info("host_wakeup_bfg gpio is 0\n");
        return 0;
    }

    if (value) {
        return oal_gpio_direction_output(g_st_board_info.host_wakeup_dev[B_SYS], GPIO_HIGHLEVEL);
    } else {
        return oal_gpio_direction_output(g_st_board_info.host_wakeup_dev[B_SYS], GPIO_LOWLEVEL);
    }
}

int32_t board_set_pcie_reset(int32_t is_master, int32_t is_poweron)
{
    int32_t value = (is_poweron == OAL_TRUE) ? GPIO_HIGHLEVEL : GPIO_LOWLEVEL;
    int32_t gpio  = (is_master == OAL_TRUE) ? g_st_board_info.ep_pcie0_rst : g_st_board_info.ep_pcie1_rst;

    ps_print_info("set pcie reset gpio=%d value=%d\n", gpio, value);

    if (gpio == 0) {
        return -OAL_ENODEV;
    }

    oal_gpio_direction_output(gpio, value);
    return OAL_SUCC;
}

int32_t board_get_host_wakeup_dev_state(uint8_t sys)
{
    if (g_st_board_info.host_wakeup_dev[sys] == 0) {
        ps_print_info("host_wakeup sys[%d] gpio is 0\n", sys);
        return 0;
    }
    return oal_gpio_get_value(g_st_board_info.host_wakeup_dev[sys]);
}

#ifdef CONFIG_HWCONNECTIVITY_PC
/* wlan_wakeup_host_have_reverser  判断取反器是否存在 */
int32_t board_get_dev_wkup_host_state(uint8_t sys)
{
    bool ret = 0;
    if (sys >= SYS_BUTT || g_st_board_info.dev_wakeup_host[sys] == 0) {
        return  -EFAIL;
    }

    ret = gpio_get_value(g_st_board_info.dev_wakeup_host[sys]);

    if (sys == W_SYS && g_st_board_info.wlan_wakeup_host_have_reverser == 1) {
        return !ret;
    }

    return ret;
}
#else
int32_t board_get_dev_wkup_host_state(uint8_t sys)
{
    if (sys >= SYS_BUTT || g_st_board_info.dev_wakeup_host[sys] == 0) {
        return  -EFAIL;
    }
    return oal_gpio_get_value(g_st_board_info.dev_wakeup_host[sys]);
}
#endif

int32_t board_wifi_tas_set(int value)
{
    if (g_st_board_info.wifi_tas_enable == WIFI_TAS_DISABLE) {
        return 0;
    }

    ps_print_dbg("wifi tas gpio set %s %pF\n", value ? "high" : "low", (void *)_RET_IP_);

    if (value) {
        return gpio_direction_output(g_st_board_info.rf_wifi_tas, GPIO_HIGHLEVEL);
    } else {
        return gpio_direction_output(g_st_board_info.rf_wifi_tas, GPIO_LOWLEVEL);
    }
}

EXPORT_SYMBOL(board_wifi_tas_set);

int32_t board_get_wifi_tas_gpio_state(void)
{
    if (g_st_board_info.wifi_tas_enable == WIFI_TAS_DISABLE) {
        return 0;
    }

    return gpio_get_value(g_st_board_info.rf_wifi_tas);
}

EXPORT_SYMBOL(board_get_wifi_tas_gpio_state);

int32_t board_power_on(uint32_t ul_subsystem)
{
    return g_st_board_info.bd_ops.board_power_on(ul_subsystem);
}

int32_t board_power_off(uint32_t ul_subsystem)
{
    return g_st_board_info.bd_ops.board_power_off(ul_subsystem);
}

int32_t board_power_reset(uint32_t ul_subsystem)
{
    return g_st_board_info.bd_ops.board_power_reset(ul_subsystem);
}
EXPORT_SYMBOL(board_wlan_gpio_power_off);
EXPORT_SYMBOL(board_wlan_gpio_power_on);

/* DTS_PROP_HI110X_GPIO_TCXO_FREQ_DETECT： GPIO管脚
   DTS_PROP_GPIO_TCXO_LEVEL： 0:tcxo 76.8M  1:tcxo 38.4M
  上电前先配置该GPIO告知芯片外部TCXO的频率
*/
STATIC void prepare_to_power_on(void)
{
    int32_t ret;
    hi110x_board_info *bd_info = NULL;

    bd_info = get_hi110x_board_info();
    if (bd_info->need_power_prepare == NO_NEED_POWER_PREPARE) {
        return;
    }
    ret = gpio_request_one(bd_info->tcxo_freq_detect, GPIOF_OUT_INIT_LOW, PROC_NAME_HI110X_GPIO_TCXO_FREQ);
    if (ret != BOARD_SUCC) {
        ps_print_err("%s gpio_request failed\n", PROC_NAME_HI110X_GPIO_TCXO_FREQ);
        return;
    }

    gpio_direction_output(bd_info->tcxo_freq_detect, bd_info->gpio_tcxo_detect_level);

    return;
}

STATIC void post_to_power_on(void)
{
    hi110x_board_info *bd_info = NULL;

    bd_info = get_hi110x_board_info();
    if (bd_info->need_power_prepare == NO_NEED_POWER_PREPARE) {
        return;
    }

    oal_gpio_free(bd_info->tcxo_freq_detect);

    return;
}

void board_chip_power_on(void)
{
    int32_t ret;

    ps_print_info("chip_power on called by %pF\n", (void*)(uintptr_t)_RET_IP_);

    if (oal_gpio_get_value(g_st_board_info.dev_wakeup_host[B_SYS]) == GPIO_HIGHLEVEL) {
        ps_print_err("bfg wakeup host gpio in wrong status\n");
    }

    ret = enable_board_pmu_clk32k();
    if (ret != BOARD_SUCC) {
        ps_print_err("hi1103 enable 32k clk fail.\n");
    }

    buck_power_ctrl(OAL_TRUE, PLAT_SUBSYS);

    board_host_wakeup_dev_set(0);
    prepare_to_power_on();
    power_state_change(g_st_board_info.power_on_enable, BOARD_POWER_ON);
    post_to_power_on();
}

void board_chip_power_off(void)
{
    int32_t ret;

    ps_print_info("chip_power off called by %pF\n", (void*)(uintptr_t)_RET_IP_);

    power_state_change(g_st_board_info.power_on_enable, BOARD_POWER_OFF);

    buck_power_ctrl(OAL_FALSE, PLAT_SUBSYS);
    board_host_wakeup_dev_set(0);
    ret = disable_board_pmu_clk32k();
    if (ret != BOARD_SUCC) {
        ps_print_err("hi1103 disable 32k clk fail.\n");
    }
}

static int32_t wifi_enable(void)
{
    int32_t ret;
    uintptr_t gpio = (uintptr_t)g_st_board_info.sys_enable[W_SYS];

    /* 第一次枚举时BUS 还未初始化 */
    ret = hcc_bus_power_ctrl_register(hcc_get_bus(HCC_EP_WIFI_DEV), HCC_BUS_CTRL_POWER_UP,
                                      board_wlan_gpio_power_on, (void *)gpio);
    if (ret) {
        ps_print_info("power ctrl down reg failed, ret=%d\n", ret);
    }
    ret = hcc_bus_power_action(hcc_get_bus(HCC_EP_WIFI_DEV), HCC_BUS_POWER_UP);

    return ret;
}

static int32_t wifi_disable(void)
{
    int32_t ret;
    uintptr_t gpio = (uintptr_t)g_st_board_info.sys_enable[W_SYS];

    ret = hcc_bus_power_ctrl_register(hcc_get_bus(HCC_EP_WIFI_DEV), HCC_BUS_CTRL_POWER_DOWN,
                                      board_wlan_gpio_power_off, (void *)gpio);
    if (ret) {
        ps_print_info("power ctrl down reg failed, ret=%d", ret);
    }
    ret = hcc_bus_power_action(hcc_get_bus(HCC_EP_WIFI_DEV), HCC_BUS_POWER_DOWN);
    return ret;
}

#ifndef _PRE_KIRIN_BOARD
static int32_t board_gt_gpio_power_on(void *data)
{
    int32_t gpio = (int32_t)(uintptr_t)(data);

    power_state_change(gpio, BOARD_POWER_ON);

    /* 如果总线不是pcie,不进行phy初始化; 此时bus_type未初始化,直接从ini读取 */
    if (check_download_channel_pcie() == BOARD_SUCC) {
        if (g_st_board_info.ep_pcie0_rst) {
            /* 拉高pcie_rst */
            oal_gpio_direction_output(g_st_board_info.ep_pcie0_rst, GPIO_LOWLEVEL);
            mdelay(1);
            oal_gpio_direction_output(g_st_board_info.ep_pcie0_rst, GPIO_HIGHLEVEL);
            mdelay(10); /* delay 10ms */
            ps_print_info("pcie0_rst %d pull up\n", g_st_board_info.ep_pcie0_rst);
        }

        hi110x_emu_mdelay(50000); // 50000 msec, 等待慢速侧PCIE建链, 时间取决于EMU平台的性能
    }
    return 0;
}

static int32_t board_gt_gpio_power_off(void *data)
{
    int32_t gpio = (int32_t)(uintptr_t)(data);
    if (g_st_board_info.ep_pcie0_rst) {
        /* 拉低pcie_rst */
        oal_gpio_direction_output(g_st_board_info.ep_pcie0_rst, GPIO_LOWLEVEL);
        mdelay(1);
        ps_print_info("pcie0_rst %d pull down\n", g_st_board_info.ep_pcie0_rst);
    }
    power_state_change(gpio, BOARD_POWER_OFF);
    hi110x_emu_mdelay(10000); // 10000:5ms * 1000 * 2
    return 0;
}

static int32_t gt_enable(void)
{
    int32_t ret;
    uintptr_t gpio = (uintptr_t)g_st_board_info.sys_enable[GT_SYS];

    /* 第一次枚举时BUS 还未初始化 */
    ret = hcc_bus_power_ctrl_register(hcc_get_bus(HCC_EP_GT_DEV), HCC_BUS_CTRL_POWER_UP, board_gt_gpio_power_on,
                                      (void *)gpio);
    if (ret) {
        ps_print_info("power ctrl down reg failed, ret=%d\n", ret);
    }
    ret = hcc_bus_power_action(hcc_get_bus(HCC_EP_GT_DEV), HCC_BUS_POWER_UP);
    return ret;
}

static int32_t gt_disable(void)
{
    int32_t ret;
    uintptr_t gpio = (uintptr_t)g_st_board_info.sys_enable[GT_SYS];

    ret = hcc_bus_power_ctrl_register(hcc_get_bus(HCC_EP_GT_DEV), HCC_BUS_CTRL_POWER_DOWN,
                                      board_gt_gpio_power_off, (void *)gpio);
    if (ret) {
        ps_print_info("power ctrl down reg failed, ret=%d", ret);
    }
    ret = hcc_bus_power_action(hcc_get_bus(HCC_EP_GT_DEV), HCC_BUS_POWER_DOWN);
    return ret;
}
#endif

int32_t board_sys_enable(uint8_t sys)
{
    if (sys >= SYS_BUTT) {
        return -EFAIL;
    }

    if (sys == W_SYS) {
        return wifi_enable();
#ifndef _PRE_KIRIN_BOARD
    } else if (sys == GT_SYS) {
        return gt_enable();
#endif
    } else {
        if (g_st_board_info.sys_enable[sys]) {
            power_state_change(g_st_board_info.sys_enable[sys], BOARD_POWER_ON);
        }
        return SUCC;
    }
}

int32_t board_sys_disable(uint8_t sys)
{
    if (sys >= SYS_BUTT) {
        return -EFAIL;
    }

    if (sys == W_SYS) {
        return  wifi_disable();
#ifndef _PRE_KIRIN_BOARD
    } else if (sys == GT_SYS) {
        return gt_disable();
#endif
    } else {
        if (g_st_board_info.sys_enable[sys]) {
            power_state_change(g_st_board_info.sys_enable[sys], BOARD_POWER_OFF);
        }
        return SUCC;
    }
}

EXPORT_SYMBOL(board_chip_power_on);
EXPORT_SYMBOL(board_chip_power_off);
EXPORT_SYMBOL(board_sys_disable);
EXPORT_SYMBOL(board_sys_enable);

/* just for shenkuo fpga test, delete later */
static void hi110x_subchip_check_tmp(void)
{
#ifdef _PRE_WINDOWS_SUPPORT
    return;
#else
#ifdef _PRE_CONFIG_USE_DTS
    int32_t ret;
    struct device_node *np = NULL;

    ret = get_board_dts_node(&np, DTS_NODE_HISI_HI110X);
    if (ret != BOARD_SUCC) {
        ps_print_err("DTS read node %s fail!!!\n", DTS_NODE_HISI_HI110X);
        return;
    }

    ret = of_property_read_bool(np, BOARD_VERSION_NAME_HI1105);
    if (ret) {
        ps_print_info("hisi subchip type is 05\n");
        g_st_board_info.chip_type = BOARD_VERSION_NAME_HI1105;
    }

    ret = of_property_read_bool(np, BOARD_VERSION_NAME_SHENKUO);
    if (ret) {
        ps_print_info("hisi subchip type is shenkuo for fpga\n");
        g_st_board_info.chip_type = BOARD_VERSION_NAME_SHENKUO;
    }
    ret = of_property_read_bool(np, BOARD_VERSION_NAME_HI1161);
    if (ret) {
        ps_print_info("hisi subchip type is 61 for fpga\n");
        g_st_board_info.chip_type = BOARD_VERSION_NAME_HI1161;
    }

    return;
#else
    return;
#endif
#endif
}

#if defined(_PRE_PRODUCT_HI1620S_KUNPENG) || defined(_PRE_WINDOWS_SUPPORT)
int32_t find_device_board_version(void)
{
    g_st_board_info.chip_type = BOARD_VERSION_NAME_HI1103;
    return BOARD_SUCC;
}
#else
int32_t find_device_board_version(void)
{
    int32_t ret;
    const char *device_version = NULL;

    ret = get_board_custmize(DTS_NODE_HISI_HI110X, DTS_PROP_SUBCHIP_TYPE_VERSION, &device_version);
    if (ret != BOARD_SUCC) {
        return BOARD_FAIL;
    }

    g_st_board_info.chip_type = device_version;

    ps_print_info("board chip_type:%s\n", g_st_board_info.chip_type);
    return BOARD_SUCC;
}
#endif

static int32_t check_device_board_name(void)
{
    int32_t i = 0;
    for (i = 0; i < oal_array_size(g_hi110x_board_version_list); i++) {
        if (strncmp(g_hi110x_board_version_list[i].name, g_st_board_info.chip_type, HI11XX_SUBCHIP_NAME_LEN_MAX) ==
            0) {
            g_st_board_info.chip_nr = (int32_t)g_hi110x_board_version_list[i].index;
            return BOARD_SUCC;
        }
    }

    return BOARD_FAIL;
}

const uint8_t* get_device_board_name(void)
{
    if (oal_unlikely(g_st_board_info.chip_type == NULL)) {
        return NULL;
    }

    return g_st_board_info.chip_type;
}

int32_t board_chiptype_init(void)
{
    int32_t ret;

    ret = find_device_board_version();
    if (ret != BOARD_SUCC) {
        ps_print_err("can not find device_board_version\n");
        return BOARD_FAIL;
    }

    /* just for hi110x fpga test, delete later */
    hi110x_subchip_check_tmp();

    ret = check_device_board_name();
    if (ret != BOARD_SUCC) {
        ps_print_err("check device name fail\n");
        return BOARD_FAIL;
    }

    return BOARD_SUCC;
}

#if defined(_PRE_S4_FEATURE)
void suspend_board_gpio_in_s4(void)
{
    ps_print_info("s4 suspend gpio..\n");
    if (g_st_board_info.bd_ops.suspend_board_gpio_in_s4 == NULL) {
        ps_print_err("suspend_board_gpio_in_s4 is null.\n");
        return;
    }
    g_st_board_info.bd_ops.suspend_board_gpio_in_s4();
    return;
}
void resume_board_gpio_in_s4(void)
{
    ps_print_info("s4 resume gpio..\n");
    if (g_st_board_info.bd_ops.resume_board_gpio_in_s4 == NULL) {
        ps_print_err("suspend_board_gpio_in_s4 is null.\n");
        return;
    }
    g_st_board_info.bd_ops.resume_board_gpio_in_s4();
    return;
}
#endif

static int32_t board_func_init(void)
{
    int32_t ret;

    memset_s(&g_st_board_info, sizeof(g_st_board_info), 0, sizeof(g_st_board_info));

    g_st_board_info.is_wifi_disable = 0;
    g_st_board_info.is_bfgx_disable = 0;
    g_st_board_info.is_gt_disable = 1;

    ret = board_chiptype_init();
    if (ret != BOARD_SUCC) {
        ps_print_err("sub chiptype init fail\n");
        return BOARD_FAIL;
    }

    ps_print_info("g_st_board_info.chip_nr=%d \n", g_st_board_info.chip_nr);

    if (get_hi110x_subchip_type() == BOARD_VERSION_BISHENG) {
        board_info_init_bisheng();
        ps_print_info("%s load bisheng board ops\n", __func__);
    } else if (get_hi110x_subchip_type() == BOARD_VERSION_HI1161) {
        board_info_init_hi1161();
        ps_print_info("%s load hi1161 board ops\n", __func__);
    } else {
        board_info_init_hi1103();
#ifdef _PRE_HI_DRV_GPIO
        board_info_init_tv();
#endif

#ifdef _PRE_PRODUCT_HI1620S_KUNPENG
        board_info_init_kunpeng();
#endif
        ps_print_info("%s load hi110x board ops\n", __func__);
    }

    return BOARD_SUCC;
}

static int32_t check_download_channel_name(const char *wlan_buff, int32_t *index)
{
    int32_t i = 0;
    for (i = 0; i < CHANNEL_DOWNLOAD_BUTT; i++) {
        if (strncmp(g_hi110x_download_channel_list[i].name, wlan_buff,
                    strlen(g_hi110x_download_channel_list[i].name)) == 0) {
            *index = i;
            return BOARD_SUCC;
        }
    }
    return BOARD_FAIL;
}

static int32_t get_download_channel(void)
{
    int32_t ret;
    uint8_t wlan_channel[DOWNLOAD_CHANNEL_LEN] = {0};
    uint8_t bfgn_channel[DOWNLOAD_CHANNEL_LEN] = {0};
    int32_t mode_value = 0;

    /* wlan channel */
    ret = get_cust_conf_string(INI_MODU_PLAT, INI_WLAN_DOWNLOAD_CHANNEL, wlan_channel, sizeof(wlan_channel));
    if (ret != INI_SUCC) {
        /* 兼容1102,1102无此配置项 */
#if defined(_PRE_WLAN_PCIE_ONLY) || defined(_PRE_PRODUCT_HI1620S_KUNPENG) || defined(_PRE_WINDOWS_SUPPORT)
        g_st_board_info.wlan_download_channel = CHANNEL_PCIE;
        ps_print_warning("can not find wlan_download_channel ,choose default:%s\n", "pcie");
        hcc_bus_cap_init(HCC_EP_WIFI_DEV, "pcie");
#else
        g_st_board_info.wlan_download_channel = CHANNEL_SDIO;
        ps_print_warning("can not find wlan_download_channel ,choose default:%s\n",
                         g_hi110x_download_channel_list[0].name);
        hcc_bus_cap_init(HCC_EP_WIFI_DEV, NULL);
#endif
    } else {
        if (check_download_channel_name(wlan_channel, &(g_st_board_info.wlan_download_channel)) != BOARD_SUCC) {
            ps_print_err("check wlan download channel:%s error\n", bfgn_channel);
            return BOARD_FAIL;
        }
        hcc_bus_cap_init(HCC_EP_WIFI_DEV, wlan_channel);
    }

    /* bfgn channel */
    ret = get_cust_conf_string(INI_MODU_PLAT, INI_BFGX_DOWNLOAD_CHANNEL, bfgn_channel, sizeof(bfgn_channel));
    if (ret != INI_SUCC) {
        /* 如果不存在该项，则默认保持和wlan一致 */
        g_st_board_info.bfgn_download_channel = g_st_board_info.wlan_download_channel;
        ps_print_warning("can not find bfgn_download_channel ,choose default:%s\n",
                         g_hi110x_download_channel_list[0].name);
        return BOARD_SUCC;
    }

    if (check_download_channel_name(bfgn_channel, &(g_st_board_info.bfgn_download_channel)) != BOARD_SUCC) {
        ps_print_err("check bfgn download channel:%s error\n", bfgn_channel);
        return BOARD_FAIL;
    }

    ps_print_info("wlan_download_channel index:%d, bfgn_download_channel index:%d\n",
                  g_st_board_info.wlan_download_channel, g_st_board_info.bfgn_download_channel);

    ret = get_cust_conf_int32(INI_MODU_PLAT, INI_FIRMWARE_DOWNLOAD_MODE, &mode_value);
    if (ret != INI_SUCC) {
        // 前向兼容，默认混合加载方式
        g_st_board_info.download_mode = MODE_COMBO;
    } else {
        g_st_board_info.download_mode = mode_value;
    }
    ps_print_info("firmware_download_mode:%d\n", g_st_board_info.download_mode);
    return BOARD_SUCC;
}

static int32_t get_ssi_dump_cfg(void)
{
    int32_t l_cfg_value = 0;
    int32_t l_ret;

#ifdef PLATFORM_SSI_DEBUG_CMD
    ssi_cmd_create_dev_node();
#endif
    /* 获取ini的配置值 */
    l_ret = get_cust_conf_int32(INI_MODU_PLAT, INI_SSI_DUMP_EN, &l_cfg_value);
    if (l_ret == INI_FAILED) {
        ps_print_err("get_ssi_dump_cfg: fail to get ini, keep disable\n");
        return BOARD_SUCC;
    }

    g_ssi_dump_en = (uint32_t)l_cfg_value;

    ps_print_info("get_ssi_dump_cfg: 0x%x\n", g_ssi_dump_en);

    return BOARD_SUCC;
}

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
STATIC void board_cci_bypass_init(void)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32_t ret;
    struct device_node *np = NULL;

    if (g_pcie_memcopy_type == -1) {
        ps_print_info("skip pcie mem burst control\n");
        return;
    }

    ret = get_board_dts_node(&np, DTS_NODE_HISI_CCIBYPASS);
    if (ret != BOARD_SUCC) {
        /* cci enable */
        g_pcie_memcopy_type = 0;
        ps_print_info("cci enable, pcie use mem burst 8 bytes\n");
    } else {
        /* cci bypass */
        g_pcie_memcopy_type = 1;
        ps_print_info("cci bypass, pcie use mem burst 4 bytes\n");
    }
#endif
}
#endif

STATIC void buck_param_init_by_ini(void)
{
    int32_t l_cfg_value = 0;
    int32_t l_ret;

    /* 获取ini的配置值 */
    l_ret = get_cust_conf_int32(INI_MODU_PLAT, "buck_mode", &l_cfg_value);
    if (l_ret == INI_FAILED) {
        ps_print_err("get_ssi_dump_cfg: fail to get ini, keep disable\n");
        return;
    }

    g_st_board_info.buck_param = (uint16_t)l_cfg_value;

    ps_print_info("buck_param_init_by_ini: 0x%x\n",  g_st_board_info.buck_param);

    return;
}

/*
*  函 数 名  : buck_param_init
*  功能描述  : 从定制化ini文件中提取buck模式的配置参数。
*   110x支持内置buck和外置buck,firmware CFG文件中仅有关键字，实际值从ini文件中获取，方便定制化。
*   ini定制化格式[buck_mode=2,BUCK_CUSTOM_REG,value],符合CFG文件的WRITEM的语法,0x50001850为1105的BUCK_CUSTOM_REG
*   地址, value根据实际要求配置:
*   （1）   BIT[15,14]：表示是否采用外置buck
*           2'b00:  全内置buck
*           2'b01:  I2C控制独立外置buck
*           2'b10:  GPIO控制独立外置buck
*           2'b11:  host控制共享外置buck电压
*   （2）   BIT[8]：表示先调电压，才能buck_en.
*   （3）   BIT[7，0]: 代表不同的Buck器件型号
*/
STATIC void buck_param_init(void)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32_t ret;
    struct device_node *np = NULL;
    int32_t buck_mode = 0;

    g_st_board_info.buck_param = 0;
    g_st_board_info.buck_control_flag = 0;

    ret = get_board_dts_node(&np, DTS_NODE_HISI_HI110X);
    if (ret != BOARD_SUCC) {
        ps_print_err("DTS read node %s fail!!!\n", DTS_NODE_HISI_HI110X);
        return  buck_param_init_by_ini();
    }

    ret = of_property_read_u32(np, DTS_PROP_HI110X_BUCK_MODE, &buck_mode);
    if (ret == BOARD_SUCC) {
        ps_print_info("buck_param_init get dts config:0x%x\n", buck_mode);
        g_st_board_info.buck_param = (uint16_t)buck_mode;
    } else {
        ps_print_err("buck_param_init fail,get from ini\n");
        return  buck_param_init_by_ini();
    }

    ps_print_info("buck_param_init success,buck_param[0x%x],buck_control_flag[0x%x]\n",
                  g_st_board_info.buck_param, g_st_board_info.buck_control_flag);
    return ;
#else
    g_st_board_info.buck_param = 0;
    g_st_board_info.buck_control_flag = 0;

    return  buck_param_init_by_ini();
#endif
}


STATIC int32_t hi110x_board_probe(struct platform_device *pdev)
{
    int ret;

    ret = board_func_init();
    if (ret != BOARD_SUCC) {
        goto err_init;
    }

    ret = ini_cfg_init();
    if (ret != BOARD_SUCC) {
        goto err_init;
    }
#ifndef _PRE_PRODUCT_HI1620S_KUNPENG
    read_tcxo_dcxo_ini_file(); // 读取共时钟配置
#endif

    ret = get_download_channel();
    if (ret != BOARD_SUCC) {
        goto err_init;
    }

    ret = get_ssi_dump_cfg();
    if (ret != BOARD_SUCC) {
        goto err_init;
    }

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
    board_cci_bypass_init();
#endif

    ret = board_sys_attr_init(pdev);
    if (ret != BOARD_SUCC) {
        goto err_init;
    }

    ret = board_gpio_init(pdev);
    if (ret != BOARD_SUCC) {
        goto err_init;
    }

    ret = board_product_attr_init(pdev);
    if (ret != BOARD_SUCC) {
        goto err_init;
    }

    ret = board_irq_init(pdev);
    if (ret != BOARD_SUCC) {
        goto err_init;
    }

    buck_param_init();

    ps_print_info("board init ok\n");

    g_board_probe_ret = BOARD_SUCC;
    complete(&g_board_driver_complete);

    return BOARD_SUCC;

err_init:
    board_gpio_free(pdev);
    g_board_probe_ret = BOARD_FAIL;
    complete(&g_board_driver_complete);
    chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                         CHR_PLT_DRV_EVENT_INIT, CHR_PLAT_DRV_ERROR_BOARD_DRV_PROB);
    return BOARD_FAIL;
}

STATIC int32_t hi110x_board_remove(struct platform_device *pdev)
{
    ps_print_info("hi110x board exit\n");
    board_gpio_free(pdev);
    return BOARD_SUCC;
}

static int32_t hi110x_board_suspend(struct platform_device *pdev, oal_pm_message_t state)
{
    return BOARD_SUCC;
}

static int32_t hi110x_board_resume(struct platform_device *pdev)
{
    return BOARD_SUCC;
}

#ifdef _PRE_CONFIG_USE_DTS
STATIC struct of_device_id g_hi110x_board_match_table[] = {
    {
        .compatible = DTS_COMP_HISI_HI110X_BOARD_NAME,
        .data = NULL,
    },
#ifdef _PRE_WINDOWS_SUPPORT
    // {}, This format is not supported in windows
#else
    {},
#endif
};
#endif

#if defined(_PRE_PRODUCT_HI1620S_KUNPENG) || defined(_PRE_WINDOWS_SUPPORT)
static const struct acpi_device_id g_hi110x_board_acpi_match[] = {
    { "HISI1103", 0 },
    { "HISI110X", 0 },
#ifdef _PRE_WINDOWS_SUPPORT
    // {}, This format is not supported in windows
#else
    {},
#endif
};
#endif

STATIC struct platform_driver g_hi110x_board_driver = {
    .probe = hi110x_board_probe,
    .remove = hi110x_board_remove,
    .suspend = hi110x_board_suspend,
    .resume = hi110x_board_resume,
    .driver = {
        .name = "hi110x_board",
        .owner = THIS_MODULE,
#ifdef _PRE_CONFIG_USE_DTS
        .of_match_table = g_hi110x_board_match_table,
#endif
#if defined(_PRE_PRODUCT_HI1620S_KUNPENG) || defined(_PRE_WINDOWS_SUPPORT)
        .acpi_match_table = ACPI_PTR(g_hi110x_board_acpi_match),
#endif
    },
};

#if defined(_PRE_PRODUCT_HI1620S_KUNPENG) || defined(_PRE_WINDOWS_SUPPORT)
MODULE_DEVICE_TABLE(acpi, g_hi110x_board_acpi_match);
#endif

int32_t hi110x_board_init(void)
{
    int32_t ret;

    g_board_probe_ret = BOARD_FAIL;
    init_completion(&g_board_driver_complete);

#ifdef OS_HI1XX_BUILD_VERSION
    g_hi11xx_os_build_variant = OS_HI1XX_BUILD_VERSION;
    ps_print_info("hi11xx_os_build_variant=%d\n", g_hi11xx_os_build_variant);
#endif

#ifdef _PRE_WINDOWS_SUPPORT
    oal_spin_lock_init(get_ssi_lock_glb_addr());
    ret = hi110x_board_probe(&g_hi110x_board_driver);
#else
    ret = platform_driver_register(&g_hi110x_board_driver);
    if (ret) {
        ps_print_err("Unable to register hisi connectivity board driver.\n");
        return ret;
    }

    if (wait_for_completion_timeout(&g_board_driver_complete, 10 * HZ)) { /* 10 */
        /* completed */
        if (g_board_probe_ret != BOARD_SUCC) {
            platform_driver_unregister(&g_hi110x_board_driver);
            ps_print_err("hi110x_board probe failed=%d\n", g_board_probe_ret);
            return g_board_probe_ret;
        }
    } else {
        /* timeout */
        platform_driver_unregister(&g_hi110x_board_driver);
        ps_print_err("hi110x_board probe timeout\n");
        return BOARD_FAIL;
    }
#endif
    ps_print_info("hi110x_board probe succ\n");

    return ret;
}

void hi110x_board_exit(void)
{
    platform_driver_unregister(&g_hi110x_board_driver);
}

int32_t hi11xx_get_os_build_variant(void)
{
    return g_hi11xx_os_build_variant;
}
EXPORT_SYMBOL(hi11xx_get_os_build_variant);

hi110x_release_vtype hi110x_get_release_type(void)
{
    hi110x_release_vtype vtype = HI110X_VTYPE_DEBUG;

    /* check the version release or debug */
    if (hi11xx_get_os_build_variant() == HI1XX_OS_BUILD_VARIANT_ROOT) {
        ps_print_dbg("root user\n");
        return vtype;
    }

#if defined(CONFIG_LOG_EXCEPTION) && !defined(CONFIG_HI110X_KERNEL_MODULES_BUILD_SUPPORT)
    if (get_logusertype_flag() == BETA_USER) {
        /* beta user */
        ps_print_dbg("beta user\n");
        return HI110X_VTYPE_RELEASE_DEBUG;
    } else {
        /* release(user & non beta user) */
        ps_print_dbg("release user\n");
        return HI110X_VTYPE_RELEASE;
    }
#else
    /* kernel module or no beta user function */
    ps_print_dbg("default user\n");
    vtype = HI110X_VTYPE_DEBUG;
#endif

    return vtype;
}
EXPORT_SYMBOL(hi110x_get_release_type);
