
#include "factory.h"
#include "board.h"
#include "plat_type.h"
#include "oal_schedule.h"
#include "plat_debug.h"
#include "plat_pm.h"
#include "plat_firmware.h"
#include "oam_ext_if.h"

typedef struct {
    int32_t addr;
    int32_t len;
    int32_t en;
} factory_wlan_memdump_t;

#define DEVICE_MEM_DUMP_MAX_BUFF_SIZE 100

#define TSENSOR_VALID_HIGHEST_TEMP 125
#define TSENSOR_VALID_LOWEST_TEMP  (-40)

#define DEVICE_MEM_CHECK_SUCC 0x000f
#define RESULT_DETAIL_REG     "0x5000001c,4"
#define RESULT_TIME_LOW_REG   "0x50000010,4"
#define RESULT_TIME_HIGH_REG  "0x50000014,4"
#define RESULT_TSENSOR_C0_REG "0x5000348c,4"
#define RESULT_TSENSOR_C1_REG "0x500034cc,4"
#define GET_MEM_CHECK_FLAG    "0x50000018,4"
// 06寄存器
#define RESULT_DETAIL_REG06     "0x4000140c,4"
#define RESULT_TIME_LOW_REG06   "0x40001400,4"
#define RESULT_TIME_HIGH_REG06  "0x40001404,4"
#define RESULT_TSENSOR_C0_REG06 "0x4000e48c,4"
#define RESULT_TSENSOR_C1_REG06 "0x4000e4cc,4"
#define GET_MEM_CHECK_FLAG06    "0x40001408,4"

#define RAM_TEST_RUN_VOLTAGE_BIAS_HIGH 0x0 /* 拉偏高压 */
#define RAM_TEST_RUN_VOLTAGE_BIAS_LOW  0x1 /* 拉偏低压 */

#define RAM_TEST_RUN_VOLTAGE_REG_ADDR     0x50002010
#define RAM_TEST_RUN_PROCESS_SEL_REG_ADDR 0x50002014

#define RAM_TEST_RUN_VOLTAGE_REG_ADDR06     0x40002010
#define RAM_TEST_RUN_PROCESS_SEL_REG_ADDR06 0x40002014

/* device soc test专用宏 */
#define SOC_TEST_RUN_FINISH     0
#define SOC_TEST_RUN_BEGIN      1

#define SOC_TEST_STATUS_REG     0x50000014
#define SOC_TEST_MODE_REG       0x50000018
#define SOC_TEST_RST_FLAG       "0x5000001c,4"

/* device soc test mode */
#define TIMER_TEST_FLAG         BIT0
#define REG_MEM_CHECK_TEST_FLAG BIT1
#define INTERRUPT_TEST_FLAG     BIT2
#define MEM_MONITOR_TEST_FLAG   BIT3
#define DMA_TEST_FLAG           BIT4
#define CPU_TRACE_TEST_FLAG     BIT5
#define PATCH_TEST_FLAG         BIT6
#define SSI_MASTER_TEST_FLAG    BIT7
#define EFUSE_TEST_FLAG         BIT8
#define WDT_TEST_FLAG           BIT9
#define MPU_TEST_FLAG           BIT10
#define IPC_TEST_FLAG           BIT11
#define MBIST_TEST_FLAG         BIT12
#define SCHEDULE_FPU_TEST_FLAG  BIT13
#define I3C_SLAVE_TEST_FLAG     BIT14

#define PRO_RAM_TEST_CASE_NONE                                   0x0
#define PRO_RAM_TEST_CASE_TCM_SCAN                               0x1
#define PRO_RAM_TEST_CASE_RAM_ALL_SCAN                           0x2
#define PRO_RAM_TEST_CASE_REG_SCAN                               0x3
#define PRO_RAM_TEST_CASE_BT_EM_SCAN                             0x4
#define PRO_RAM_TEST_CASE_MBIST                                  0x5
#define PRO_RAM_TEST_CASE_MBIST_CLDO1_WL0_MAJORITY_FULL_TEST_NEW 0x6
#define PRO_RAM_TEST_CASE_MBIST_CLDO2_FULL_TEST_NEW              0x7
#define PRO_RAM_TEST_CASE_MBIST_CLDO1_OTHER_FULL_TEST_NEW        0x8
#define PRO_RAM_TEST_CASE_MBIST_CLDO1_WL0_320_FULL_TEST_NEW      0x9
#define PRO_RAM_TEST_CASE_MBIST_CLDO1_WL0_MAJORITY_TEST_NEW      0xa

#define PRO_DETAIL_RESULT_NOT_START                   0x0000
#define PRO_DETAIL_RESULT_SUCC                        0xf000
#define PRO_DETAIL_RESULT_RUNING                      0xffff
#define PRO_DETAIL_RESULT_CASE_0                      0x1
#define PRO_DETAIL_RESULT_CASE_1                      0x2
#define PRO_DETAIL_RESULT_CASE_2_1                    0x3
#define PRO_DETAIL_RESULT_CASE_2_2                    0x4
#define PRO_DETAIL_RESULT_CASE_3                      0x5
#define pro_detail_error_result(main_case, test_case) (0xf000 | ((main_case) << 8) | ((test_case)&0xff))
#define pro_get_detail_error_result_main(error)       (((error) >> 8) & 0xf)
#define pro_get_detail_error_result_sub(error)        (((error) >> 0) & 0xff)

// 产测结果
enum product_test_result_enum {
    RESULT_DETAIL = 0, /* 产测结果数组索引 */
    RESULT_TIME_LOW,   /* 产测运行时间低16bit索引 */
    RESULT_TIME_HIGH,  /* 产测运行时间高16bit索引 */
    RESULT_TSENSOR_C0, /* 产线TSENSOR_C0结果索引 */
    RESULT_TSENSOR_C1, /* 产线TSENSOR_C1结果索引 */
    MEM_CHECK_FLAG, /* 产线GET_MEM_CHECK_FLAG 索引 */
    RESULT_TOTAL
};

static uint8_t *g_shenkuo_product_test_result[RESULT_TOTAL] = {
    RESULT_DETAIL_REG06,
    RESULT_TIME_LOW_REG06,
    RESULT_TIME_HIGH_REG06,
    RESULT_TSENSOR_C0_REG06,
    RESULT_TSENSOR_C1_REG06,
    GET_MEM_CHECK_FLAG06,
};

// 03&&05 寄存器地址一样
static uint8_t *g_hi110x_product_test_result[RESULT_TOTAL] = {
    RESULT_DETAIL_REG,
    RESULT_TIME_LOW_REG,
    RESULT_TIME_HIGH_REG,
    RESULT_TSENSOR_C0_REG,
    RESULT_TSENSOR_C1_REG,
    GET_MEM_CHECK_FLAG,
};

typedef union {
    /* Define the struct bits */
    struct {
        unsigned int tsensor_c0_auto_clr : 1;   /* [0]  */
        unsigned int tsensor_c0_rdy_auto : 1;   /* [1]  */
        unsigned int tsensor_c0_data_auto : 10; /* [11..2]  */
        unsigned int reserved : 4;              /* [15..12]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;
} tensor_auto;

typedef struct _mem_test_type_str_ {
    uint32_t id;
    char *name;
} mem_test_type_str;

static mem_test_type_str g_mem_test_main_type[] = {
    { PRO_DETAIL_RESULT_CASE_0,   "SYSTEM_INIT" },
    { PRO_DETAIL_RESULT_CASE_1,   "CASE_1" },
    { PRO_DETAIL_RESULT_CASE_2_1, "CASE_2_1" },
    { PRO_DETAIL_RESULT_CASE_2_2, "CASE_2_2" },
    { PRO_DETAIL_RESULT_CASE_3,   "CASE_3" },
};

static mem_test_type_str g_mem_test_sub_type[] = {
    { PRO_RAM_TEST_CASE_NONE,                                   "NOT_OVER" },
    { PRO_RAM_TEST_CASE_TCM_SCAN,                               "TCM_SCAN" },
    { PRO_RAM_TEST_CASE_RAM_ALL_SCAN,                           "RAM_ALL_SCAN" },
    { PRO_RAM_TEST_CASE_REG_SCAN,                               "REG_SCAN" },
    { PRO_RAM_TEST_CASE_BT_EM_SCAN,                             "BT_EM_SCAN" },
    { PRO_RAM_TEST_CASE_MBIST,                                  "MBIST_INIT" },
    { PRO_RAM_TEST_CASE_MBIST_CLDO1_WL0_MAJORITY_FULL_TEST_NEW, "CLDO1_WL0_MAJORITY_FULL_TEST_NEW" },
    { PRO_RAM_TEST_CASE_MBIST_CLDO2_FULL_TEST_NEW,              "CLDO2_FULL_TEST_NEW" },
    { PRO_RAM_TEST_CASE_MBIST_CLDO1_OTHER_FULL_TEST_NEW,        "CLDO1_OTHER_FULL_TEST_NEW" },
    { PRO_RAM_TEST_CASE_MBIST_CLDO1_WL0_320_FULL_TEST_NEW,      "CLDO1_WL0_320_FULL_TEST_NEW" },
    { PRO_RAM_TEST_CASE_MBIST_CLDO1_WL0_MAJORITY_TEST_NEW,      "CLDO1_WL0_MAJORITY_TEST_NEW" },
};

int g_ram_test_ssi_error_dump = 0;
oal_debug_module_param(g_ram_test_ssi_error_dump, int, S_IRUGO | S_IWUSR);
int g_ram_test_ssi_pass_dump = 0;
oal_debug_module_param(g_ram_test_ssi_pass_dump, int, S_IRUGO | S_IWUSR);
int g_ram_test_detail_result_dump = 1;
oal_debug_module_param(g_ram_test_detail_result_dump, int, S_IRUGO | S_IWUSR);
int g_ram_test_detail_tsensor_dump = 1;
oal_debug_module_param(g_ram_test_detail_tsensor_dump, int, S_IRUGO | S_IWUSR);
int g_ram_test_mem_pass_dump = 0;
oal_debug_module_param(g_ram_test_mem_pass_dump, int, S_IRUGO | S_IWUSR);

/*
 * 0 表示 用例全部跑完,
 * 1表示 case1跑完返回，
 * 2表示 case2跑完返回 类推
 */
int g_ram_test_run_process_sel = 0x0;
oal_debug_module_param(g_ram_test_run_process_sel, int, S_IRUGO | S_IWUSR);

int g_ram_test_run_voltage_bias_sel = RAM_TEST_RUN_VOLTAGE_BIAS_HIGH;
oal_debug_module_param(g_ram_test_run_voltage_bias_sel, int, S_IRUGO | S_IWUSR);

int g_ram_test_wifi_hold_time = 0; /* after done the test, we hold the process to test signal(ms) */
oal_debug_module_param(g_ram_test_wifi_hold_time, int, S_IRUGO | S_IWUSR);

int g_ram_test_bfgx_hold_time = 0; /* after done the test, we hold the process to test signal(ms) */
oal_debug_module_param(g_ram_test_bfgx_hold_time, int, S_IRUGO | S_IWUSR);

int32_t g_ram_reg_test_result = OAL_SUCC;
unsigned long long g_ram_reg_test_time = 0;
/* 03产测delay W、B的delay分别是3s和5s，05产测delay W、B的delay暂定是5s和4s */
uint32_t g_wlan_mem_check_mdelay = 3000;
uint32_t g_bfgx_mem_check_mdelay = 5000;

static factory_wlan_memdump_t g_factory_wlan_memdump_cfg = { 0x60000000, 0x1000 };

static char *get_memcheck_error_main_string(uint32_t result)
{
    uint16_t main_type;
    uint32_t i;
    uint32_t num = sizeof(g_mem_test_main_type) / sizeof(mem_test_type_str);
    main_type = pro_get_detail_error_result_main(result);

    for (i = 0; i < num; i++) {
        if (main_type == g_mem_test_main_type[i].id) {
            return g_mem_test_main_type[i].name;
        }
    }

    return NULL;
}

static char *get_memcheck_error_sub_string(uint32_t result)
{
    uint16_t sub_type;
    uint32_t i;
    uint32_t num = sizeof(g_mem_test_sub_type) / sizeof(mem_test_type_str);
    sub_type = pro_get_detail_error_result_sub(result);
    for (i = 0; i < num; i++) {
        if (sub_type == g_mem_test_sub_type[i].id) {
            return g_mem_test_sub_type[i].name;
        }
    }

    return NULL;
}

static int32_t wait_wcpu_mem_test(void)
{
    unsigned long timeout;
    unsigned long timeout_hold;
    uint32_t hold_time = 100; /* 拉高维持100ms */
    int32_t gpio_num;
    declare_time_cost_stru(cost);

    gpio_num = g_st_board_info.dev_wakeup_host[W_SYS];

    timeout = jiffies + msecs_to_jiffies(g_wlan_mem_check_mdelay);
    ps_print_info("wifi memcheck gpio level check start,timeout=%d ms\n", g_wlan_mem_check_mdelay);
    oal_get_time_cost_start(cost);

    while (1) {
        // wait gpio high
        if (wait_gpio_level(gpio_num, GPIO_HIGHLEVEL, timeout) != SUCCESS) {
            ps_print_err("[E]wait wakeup gpio to high timeout [%u] ms[%lu:%lu]\n",
                         g_wlan_mem_check_mdelay, jiffies, timeout);
            return -FAILURE;
        }
        oal_get_time_cost_end(cost);
        oal_calc_time_cost_sub(cost);
        ps_print_info("wcpu wake up host gpio to high %llu us", time_cost_var_sub(cost));

        // high level hold time
        timeout_hold = jiffies + msecs_to_jiffies(hold_time);
        oal_msleep(SLEEP_10_MSEC);

        // wait high level hold time
        if (wait_gpio_level(gpio_num, GPIO_LOWLEVEL, timeout_hold) == SUCCESS) {
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "[E]gpio pull down again, retry");
            oal_msleep(SLEEP_10_MSEC);
            continue;
        }

        // gpio high and hold enough time.
        oal_get_time_cost_end(cost);
        oal_calc_time_cost_sub(cost);
        ps_print_info("wcpu wake up host gpio to high %llu us", time_cost_var_sub(cost));
        break;
    }

    return OAL_SUCC;
}

int32_t is_device_mem_test_succ(void)
{
    int32_t ret;
    int32_t test_flag = 0;
    uint8_t **result_reg = NULL;

    if (get_hi110x_subchip_type() == BOARD_VERSION_SHENKUO) {
        result_reg = g_shenkuo_product_test_result;
        /* 等待wcpu拉wkup gpio */
        ret = wait_wcpu_mem_test();
        if (ret < 0) {
            ps_print_err("[E]wait wcpu device mem test timeout!\n");
            return -OAL_EFAIL;
        }
    } else {
        result_reg = g_hi110x_product_test_result;
    }

    ret = number_type_cmd_send(RMEM_CMD_KEYWORD, result_reg[MEM_CHECK_FLAG]);
    if (ret < 0) {
        ps_print_warning("send cmd %s:%s fail,ret = %d\n", RMEM_CMD_KEYWORD, result_reg[MEM_CHECK_FLAG], ret);
        return -OAL_EFAIL;
    }

    ret = read_msg((uint8_t *)&test_flag, sizeof(test_flag));
    if (ret < 0) {
        ps_print_warning("read device test flag fail, read_len = %d, return = %d\n", (int32_t)sizeof(test_flag), ret);
        return -OAL_EFAIL;
    }
    ps_print_warning("get device test flag:0x%x\n", test_flag);
    if (test_flag == DEVICE_MEM_CHECK_SUCC) {
        return OAL_SUCC;
    }

    return -OAL_EFAIL;
}

static void print_device_ram_test_detail_result(int32_t is_wcpu, uint32_t result)
{
    char *case_name = NULL;
    char *main_str = NULL;
    char *sub_str = NULL;
    if (is_wcpu == true) {
        case_name = "[wcpu_memcheck]";
    } else {
        case_name = "[bcpu_memcheck]";
    }
    ps_print_warning("%s detail result=0x%x\n", case_name, result);
    if (result == PRO_DETAIL_RESULT_NOT_START) {
        ps_print_warning("%s didn't start run\n", case_name);
        return;
    } else if (result == PRO_DETAIL_RESULT_SUCC) {
        ps_print_warning("%s run succ\n", case_name);
        return;
    } else if (result == PRO_DETAIL_RESULT_RUNING) {
        ps_print_warning("%s still running\n", case_name);
        return;
    }

    main_str = get_memcheck_error_main_string(result);
    sub_str = get_memcheck_error_sub_string(result);

    ps_print_err("%s error found [%s:%s]\n",
                 case_name,
                 (main_str == NULL) ? "unkown" : main_str,
                 (sub_str == NULL) ? "unkown" : sub_str);
}

static int32_t number_cmd_send_and_read(const char *key, const char *value, uint32_t *result)
{
    int32_t ret;
    ret = number_type_cmd_send(key, value);
    if (ret < 0) {
        ps_print_warning("send cmd %s:%s fail,ret = %d\n", key, value, ret);
        return -OAL_EFAIL;
    }

    ret = read_msg((uint8_t *)result, sizeof(uint32_t));
    if (ret < 0) {
        ps_print_warning("send cmd %s:%s read result failed,ret = %d\n", key, value, ret);
        return -OAL_EFAIL;
    }
    return OAL_SUCC;
}

static void tsensor_check(int32_t sensor_num, uint16_t reg_tsensor)
{
    tensor_auto *tsensor_auto_sts = (tensor_auto *)&reg_tsensor;
    uint16_t temp_data = tsensor_auto_sts->bits.tsensor_c0_data_auto;
    int32_t temp_val = ((((int32_t)temp_data - 118) * 165) / 815) - 40; /* 温度码转换公式l_val=(us-118)*165)/815)-40 */
    ps_print_info("[memcheck]TsensorC%d, val 0x%x data,%u rdy,%u temp %d\n", sensor_num,
                  reg_tsensor, tsensor_auto_sts->bits.tsensor_c0_data_auto,
                  tsensor_auto_sts->bits.tsensor_c0_rdy_auto, temp_val);
    if ((tsensor_auto_sts->bits.tsensor_c0_rdy_auto) &&
        ((temp_val < TSENSOR_VALID_LOWEST_TEMP) || (temp_val > TSENSOR_VALID_HIGHEST_TEMP))) {
        ps_print_info("[memcheck]TsensorC%d, invalid", sensor_num);
    }
}

void get_device_ram_test_result(int32_t is_wcpu, uint32_t *cost)
{
    uint32_t time_cost;
    uint32_t result = 0;
    uint8_t **result_reg = NULL;
    hcc_bus *pst_bus = hcc_get_bus(HCC_EP_WIFI_DEV);
    if (pst_bus == NULL) {
        ps_print_err("pst_bus is null");
        return;
    }

    if (pst_bus->bus_type != HCC_BUS_PCIE) {
        if (is_wcpu != true) {
            /* bcpu 运行wmbist 会导致sdio接口无法回读，这种场景不回读 */
            ps_print_err("bcpu ram test can't read detail result");
            return;
        }
    }

    if (get_hi110x_subchip_type() == BOARD_VERSION_SHENKUO) {
        result_reg = g_shenkuo_product_test_result;
    } else {
        result_reg = g_hi110x_product_test_result;
    }
    /* 失败后读取详细的结果 */
    if (number_cmd_send_and_read(RMEM_CMD_KEYWORD, result_reg[RESULT_DETAIL], &result) < 0) {
        return;
    }

    print_device_ram_test_detail_result(is_wcpu, result);

    if (number_cmd_send_and_read(RMEM_CMD_KEYWORD, result_reg[RESULT_TIME_LOW], &result) < 0) {
        return;
    }
    time_cost = (result & 0xffff);

    if (number_cmd_send_and_read(RMEM_CMD_KEYWORD, result_reg[RESULT_TIME_HIGH], &result) < 0) {
        return;
    }
    time_cost |= ((result & 0xffff) << 16); /* 左移16位 */
    ps_print_warning("%s_ram_test_time_cost tick:%u  %u us\n",
                     (is_wcpu == true) ? "wcpu" : "bcpu", time_cost, time_cost * 31); /* time from 31+1 k */
    *cost = time_cost * 31; /* time_cost * 31 */

    /* tsensor read */
    if (g_ram_test_detail_tsensor_dump) {
        uint32_t tsensor_c0 = 0;
        uint32_t tsensor_c1 = 0;

        if (number_cmd_send_and_read(RMEM_CMD_KEYWORD, result_reg[RESULT_TSENSOR_C0], &tsensor_c0) < 0) {
            return;
        }

        if (number_cmd_send_and_read(RMEM_CMD_KEYWORD, result_reg[RESULT_TSENSOR_C1], &tsensor_c1) < 0) {
            return;
        }

        tsensor_check(0, tsensor_c0); // tsensor 0
        tsensor_check(1, tsensor_c1); // tsensor 1
    }
}

static factory_wlan_memdump_t *get_wlan_memdump_cfg(void)
{
    return &g_factory_wlan_memdump_cfg;
}

int32_t get_device_test_mem(bool is_wifi)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    factory_wlan_memdump_t *wlan_memdump_storage = NULL;
    int32_t ret;
    char buff[DEVICE_MEM_DUMP_MAX_BUFF_SIZE];

    wlan_memdump_storage = get_wlan_memdump_cfg();
    if (wlan_memdump_storage == NULL) {
        ps_print_err("memdump cfg is NULL!\n");
        return -FAILURE;
    }
    ret = snprintf_s(buff, sizeof(buff), sizeof(buff) - 1, "0x%x,%d",
                     wlan_memdump_storage->addr, wlan_memdump_storage->len);
    if (ret < 0) {
        ps_print_err("log str format err line[%d]\n", __LINE__);
        return ret;
    }

    if (sdio_read_mem(RMEM_CMD_KEYWORD, buff, is_wifi) >= 0) {
        ps_print_warning("read device mem succ\n");
    } else {
        ps_print_warning("read device mem fail\n");
        return -FAILURE;
    }
#endif
    return OAL_SUCC;
}

uint32_t set_wlan_mem_check_mdelay(uint32_t mdelay)
{
    g_wlan_mem_check_mdelay = mdelay;
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "set_wlan_mem_check_mdelay::set delay:%dms!!", g_wlan_mem_check_mdelay);
    return OAL_SUCC;
}
uint32_t set_bfgx_mem_check_mdelay(uint32_t mdelay)
{
    g_bfgx_mem_check_mdelay = mdelay;
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "g_bfgx_mem_check_mdelay::set delay:%dms!!", g_bfgx_mem_check_mdelay);
    return OAL_SUCC;
}

EXPORT_SYMBOL_GPL(set_wlan_mem_check_mdelay);

uint32_t set_wlan_mem_check_memdump(int32_t addr, int32_t len)
{
    g_factory_wlan_memdump_cfg.addr = addr;
    g_factory_wlan_memdump_cfg.len = len;
    g_factory_wlan_memdump_cfg.en = 1;
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "set_wlan_mem_check_memdump set ok: addr:0x%x,len:%d", addr, len);
    return OAL_SUCC;
}

EXPORT_SYMBOL_GPL(set_wlan_mem_check_memdump);

int32_t wlan_device_mem_check(int32_t l_runing_test_mode)
{
    struct wlan_pm_s *pst_wlan_pm = wlan_pm_get_drv();
    int32_t        l_chip_type = get_hi110x_subchip_type();
    uint32_t       ret;

    if (oal_warn_on(pst_wlan_pm == NULL)) {
        oam_error_log0(0, OAM_SF_PWR, "wlan_device_mem_check: pst_wlan_pm is null \n");
        return -OAL_FAIL;
    }

    g_ram_reg_test_result = OAL_SUCC;
    g_ram_reg_test_time = 0;
    hcc_bus_wake_lock(pst_wlan_pm->pst_bus);

    /* 03&05共host，后续项目调试时间时可注掉这部分 */
    if (l_chip_type == BOARD_VERSION_HI1105) {
        ret = set_wlan_mem_check_mdelay(5000);  // wifi 5000ms
        ret += set_bfgx_mem_check_mdelay(4000); // bfg 4000ms
    } else if (l_chip_type == BOARD_VERSION_SHENKUO) {
        ret = set_wlan_mem_check_mdelay(30000); // debug延迟时间，后续asci需要调整
        ret += set_bfgx_mem_check_mdelay(40000);
    } else {
        ret = set_wlan_mem_check_mdelay(3000);  // wifi 3000ms
        ret += set_bfgx_mem_check_mdelay(5000); // bfg 5000ms
    }

    if (ret != 0) {
        oam_warning_log0(0, OAM_SF_PWR, "st_ram_reg_test_work set mdelay fail !\n");
    }
    if (l_runing_test_mode == 0) {
        /* dbc工位，低压memcheck */
        g_ram_test_run_voltage_bias_sel = RAM_TEST_RUN_VOLTAGE_BIAS_LOW;
    } else {
        /* 老化工位，高压memcheck */
        g_ram_test_run_voltage_bias_sel = RAM_TEST_RUN_VOLTAGE_BIAS_HIGH;
    }

    if (wlan_pm_work_submit(pst_wlan_pm, &pst_wlan_pm->st_ram_reg_test_work) != 0) {
        hcc_bus_wake_unlock(pst_wlan_pm->pst_bus);
        oam_warning_log0(0, OAM_SF_PWR, "st_ram_reg_test_work submit work fail !\n");
    }

    return OAL_SUCC;
}

int32_t wlan_device_mem_check_result(unsigned long long *time)
{
    *time = g_ram_reg_test_time;
    return g_ram_reg_test_result;
}

void wlan_device_mem_check_work(oal_work_stru *pst_worker)
{
    struct wlan_pm_s *pst_wlan_pm = wlan_pm_get_drv();
    struct pm_top* pm_top_data = pm_get_top();

    mutex_lock(&(pm_top_data->host_mutex));

    hcc_bus_disable_state(pst_wlan_pm->pst_bus, OAL_BUS_STATE_ALL);
    g_ram_reg_test_result = device_mem_check(&g_ram_reg_test_time);
    hcc_bus_enable_state(pst_wlan_pm->pst_bus, OAL_BUS_STATE_ALL);

    hcc_bus_wake_unlock(pst_wlan_pm->pst_bus);

    mutex_unlock(&(pm_top_data->host_mutex));
}

EXPORT_SYMBOL_GPL(wlan_device_mem_check);
EXPORT_SYMBOL_GPL(wlan_device_mem_check_result);

#ifdef PLATFORM_DEBUG_ENABLE
static uint32_t g_wlan_soc_check_mdelay = 12000;
static uint32_t g_bfgx_soc_check_mdelay = 25000;
static uint16_t g_wlan_soc_test_mode = 0xf7ff; // 默认不测IPC。IPC有单独的用例测
static uint16_t g_bfgx_soc_test_mode = 0xb7ff; // 默认不测IPC,I3C

void set_wlan_soc_test_mdelay(uint32_t mdelay)
{
    g_wlan_soc_check_mdelay = mdelay;
    ps_print_info("set wlan_soc_check_mdelay = [%u]ms \n", g_wlan_soc_check_mdelay);
}

void set_bfgx_soc_test_mdelay(uint32_t mdelay)
{
    g_bfgx_soc_check_mdelay = mdelay;
    ps_print_info("set bfgx_soc_check_mdelay = [%u]ms \n", g_bfgx_soc_check_mdelay);
}

void set_wlan_soc_test_mode(uint16_t test_mode)
{
    g_wlan_soc_test_mode = test_mode;
    ps_print_info("set soc_test_mode = [0x%x] \n", g_wlan_soc_test_mode);
}

void set_bfgx_soc_test_mode(uint16_t test_mode)
{
    g_bfgx_soc_test_mode = test_mode;
    ps_print_info("set soc_test_mode = [0x%x] \n", g_bfgx_soc_test_mode);
}

void soc_test_help(void)
{
    ps_print_info("TIMER_TEST_FLAG          BIT0 \n");
    ps_print_info("REG_MEM_CHECK_TEST_FLAG  BIT1 \n");
    ps_print_info("INTERRUPT_TEST_FLAG      BIT2 \n");
    ps_print_info("MEM_MONITOR_TEST_FLAG    BIT3 \n");
    ps_print_info("DMA_TEST_FLAG            BIT4 \n");
    ps_print_info("CPU_TRACE_TEST_FLAG      BIT5 \n");
    ps_print_info("PATCH_TEST_FLAG          BIT6 \n");
    ps_print_info("SSI_MASTER_TEST_FLAG     BIT7 \n");
    ps_print_info("EFUSE_TEST_FLAG(null)    BIT8 \n");
    ps_print_info("WDT_TEST_FLAG            BIT9 \n");
    ps_print_info("MPU_TEST_FLAG            BIT10 \n");
    ps_print_info("IPC_TEST_FLAG            BIT11 \n");
    ps_print_info("MBIST_TEST_FLAG(null)    BIT12 \n");
    ps_print_info("SCHEDULE_FPU_TEST_FLAG(b)BIT13 \n");
    ps_print_info("I3C_SLAVE_TEST_FLAG(b)   BIT14 \n");
    ps_print_info("ecall set_(wlan or bfgx)_soc_test_mode_hi110x with a mode can set test mode\n");
    ps_print_info("ecall set_(wlan or bfgx)_soc_test_mdelay_hi110x with a mode can set test mdelay\n");
}

static int32_t get_device_soc_test_result(void)
{
    int32_t ret;
    int32_t test_flag = 0;

    ret = number_type_cmd_send(RMEM_CMD_KEYWORD, SOC_TEST_RST_FLAG);
    if (ret < 0) {
        ps_print_warning("send cmd %s:%s fail,ret = %d\n", RMEM_CMD_KEYWORD, SOC_TEST_RST_FLAG, ret);
        return -FAILURE;
    }

    ret = read_msg((uint8_t *)&test_flag, sizeof(test_flag));
    if (ret < 0) {
        ps_print_warning("read device test flag fail, read_len = %d, return = %d\n", (int32_t)sizeof(test_flag), ret);
        return -FAILURE;
    }
    ps_print_warning("get device test flag: [0x%x] \n", test_flag);
    return test_flag;
}

static int32_t wlan_soc_test_init(void)
{
    int32_t ret;

    /* 对应device逻辑，保证每次用例都得到正确的初始化 */
    ret = write_device_reg16(SOC_TEST_STATUS_REG, SOC_TEST_RUN_FINISH);
    if (ret) {
        ps_print_err("SOC-TEST status set failed ret=%d\n", ret);
        board_power_off(W_SYS);
        return -FAILURE;
    }

    ret = write_device_reg16(SOC_TEST_MODE_REG, g_wlan_soc_test_mode);
    if (ret) {
        ps_print_err("SOC-TEST mode set failed ret=%d\n", ret);
        board_power_off(W_SYS);
        return -FAILURE;
    }
    ps_print_suc("SOC-TEST mode [0x%x] set succ \n", g_wlan_soc_test_mode);
    return 0;
}

static int32_t soc_ipc_test_init(void)
{
    int32_t ret;

    /* 对应device逻辑，保证每次用例都得到正确的初始化 */
    ret = write_device_reg16(SOC_TEST_STATUS_REG, SOC_TEST_RUN_FINISH);
    if (ret) {
        ps_print_err("SOC-TEST status set failed ret=%d\n", ret);
        board_power_off(W_SYS);
        return -FAILURE;
    }

    ret = write_device_reg16(SOC_TEST_MODE_REG, IPC_TEST_FLAG);
    if (ret) {
        ps_print_err("SOC-TEST mode set failed ret=%d\n", ret);
        board_power_off(W_SYS);
        return -FAILURE;
    }
    ps_print_suc("SOC-TEST mode [0x%x] set succ \n", IPC_TEST_FLAG);
    return 0;
}

static int32_t bfgx_soc_test_init(void)
{
    int32_t ret;

    /* 对应device逻辑，保证每次用例都得到正确的初始化 */
    ret = write_device_reg16(SOC_TEST_STATUS_REG, SOC_TEST_RUN_FINISH);
    if (ret) {
        ps_print_err("SOC-TEST status set failed ret=%d\n", ret);
        board_power_off(W_SYS);
        return -FAILURE;
    }

    ret = write_device_reg16(SOC_TEST_MODE_REG, g_bfgx_soc_test_mode);
    if (ret) {
        ps_print_err("SOC-TEST mode set failed ret=%d\n", ret);
        board_power_off(W_SYS);
        return -FAILURE;
    }
    ps_print_suc("SOC-TEST mode [0x%x] set succ \n", g_bfgx_soc_test_mode);
    return 0;
}

static void print_soc_err_msg(uint16_t us_err_rst)
{
    printk("[plat] / ");
    if (us_err_rst & TIMER_TEST_FLAG) {
        printk("timer / ");
    }
    if (us_err_rst & REG_MEM_CHECK_TEST_FLAG) {
        printk("reg mem check / ");
    }
    if (us_err_rst & INTERRUPT_TEST_FLAG) {
        printk("interrupt / ");
    }
    if (us_err_rst & MEM_MONITOR_TEST_FLAG) {
        printk("mem monitor / ");
    }
    if (us_err_rst & DMA_TEST_FLAG) {
        printk("dma / ");
    }
    if (us_err_rst & CPU_TRACE_TEST_FLAG) {
        printk("cpu trace / ");
    }
    if (us_err_rst & PATCH_TEST_FLAG) {
        printk("patch / ");
    }
    if (us_err_rst & SSI_MASTER_TEST_FLAG) {
        printk("ssi master / ");
    }
    if (us_err_rst & WDT_TEST_FLAG) {
        printk("wdt / ");
    }
    if (us_err_rst & MPU_TEST_FLAG) {
        printk("mpu / ");
    }
    if (us_err_rst & IPC_TEST_FLAG) {
        printk("ipc / ");
    }
    if (us_err_rst & SCHEDULE_FPU_TEST_FLAG) {
        printk("fpu / ");
    }
    if (us_err_rst & I3C_SLAVE_TEST_FLAG) {
        printk("i3c / ");
    }
    printk("test failed \r\n");
}

static void check_soc_test_result(uint16_t us_expect_val, uint16_t us_reg_val, bool is_wifi)
{
    uint16_t us_err_rst;

    if (us_reg_val == us_expect_val) {
        ps_print_suc("device %s SOC-TEST success!\n", is_wifi ? "WCPU" : "BCPU");
    } else {
        ps_print_err("device %s SOC-TEST failed!\n", is_wifi ? "WCPU" : "BCPU");
        if (us_expect_val > us_reg_val) {
            us_err_rst = (uint16_t)(us_expect_val - us_reg_val);
        } else {
            ps_print_err("ut result not accord with expect, please check code\n");
            ps_print_err("expect result = [0x%x], real result = [0x%x]\n", us_expect_val, us_reg_val);
            return;
        }
        print_soc_err_msg(us_err_rst);
    }
}

static void device_soc_ipc_test(void)
{
    int32_t ret;
    int32_t ipc_test;
    const int32_t ipc_test_mdelay = 2000;

    ps_print_info("start ipc SOC-TEST!\n");
    ret = board_power_on(W_SYS);
    if (ret) {
        ps_print_err("W_SYS on failed ret=%d\n", ret);
        return;
    }

    ret = firmware_download_function_priv(BFGX_AND_WIFI_CFG, soc_ipc_test_init,
                                          hcc_get_bus(HCC_EP_WIFI_DEV));
    if (ret == SUCCESS) {
        /* 等待device信息处理，ipc用例2s足够 */
        mdelay(ipc_test_mdelay);
        ret = get_device_soc_test_result();
    }
    board_power_off(W_SYS);

    ipc_test = (int32_t)IPC_TEST_FLAG;
    if (ret == ipc_test) {
        ps_print_info("device ipc SOC-TEST success!\n");
    } else {
        ps_print_info("device ipc SOC-TEST failed!\n");
    }
}

static int32_t device_bfgx_soc_test(void)
{
    int ret;

    ps_print_info("start bcpu SOC-TEST!\n");
    ret = board_power_on(W_SYS);
    if (ret) {
        ps_print_err("W_SYS on failed ret=%d\n", ret);
        return -FAILURE;
    }

    ret = firmware_download_function_priv(BFGX_CFG, bfgx_soc_test_init,
                                          hcc_get_bus(HCC_EP_WIFI_DEV));
    if (ret == SUCCESS) {
        /* 等待device信息处理 */
        mdelay(g_bfgx_soc_check_mdelay);
        ret = get_device_soc_test_result();
    }
    board_power_off(W_SYS);

    return ret;
}

/*
 * 因rom same版本soc test用例bin文件过大，在单device测试时下bin文件时间过长，而采取host的方式下bin。
 * 编译时1105请用build_test.sh asic/fpga (rom same版本)方式取bin；
 *       1103请用build_test_gcc.sh pilot asic/fpga (rom same版本)方式取bin。
 * 测试时，替换bin文件之后，直接调用到device_soc_test() 函数即可(ecall)
 * 默认情况下除i3c之外其他的用例都会测到。
 * 注:1105 build_soc_test.sh asic/fpga 编出的是不保证rom same的版本，此版本bin较小，可以用单device方式测试；
 *    1103 build_test_gcc.sh mpw2 编出的是不保证rom same的版本，但是patch和ssi master用例测不了。
 */
void device_soc_test(void)
{
    int32_t w_ret;
    int32_t b_ret;

    if (!bfgx_is_shutdown()) {
        ps_print_err("SOC-TEST need bfgx shut down!\n");
        bfgx_print_subsys_state();
        return;
    }
    if (!wlan_is_shutdown()) {
        ps_print_err("SOC-TEST need wlan shut down!\n");
        return;
    }

    ps_print_info("start wcpu SOC-TEST!\n");
    w_ret = board_power_on(W_SYS);
    if (w_ret) {
        ps_print_err("W_SYS on failed ret=%d\n", w_ret);
        return;
    }

    w_ret = firmware_download_function_priv(WIFI_CFG, wlan_soc_test_init,
                                            hcc_get_bus(HCC_EP_WIFI_DEV));
    if (w_ret == SUCCESS) {
        /* 等待device信息处理 */
        mdelay(g_wlan_soc_check_mdelay);
        w_ret = get_device_soc_test_result();
    }
    board_power_off(W_SYS);

    b_ret = device_bfgx_soc_test();

    device_soc_ipc_test();

    if (w_ret == -FAILURE) {
        ps_print_info("device wcpu SOC-TEST failed! read device result reg fail!\n");
    } else {
        check_soc_test_result(g_wlan_soc_test_mode, (uint16_t)w_ret, true);
    }

    if (b_ret == -FAILURE) {
        ps_print_info("device bcpu SOC-TEST failed! read device result reg fail!\n");
    } else {
        check_soc_test_result(g_bfgx_soc_test_mode, (uint16_t)b_ret, false);
    }
    ps_print_info("ecall soc_test_help_hi110x can get some help\n");
}
#endif // endif of PLATFORM_DEBUG_ENABLE

static int32_t g_pro_memcheck_en = 0;
static struct completion g_pro_memcheck_finish;
int32_t memcheck_is_working(void)
{
    if (g_pro_memcheck_en) {
        complete(&g_pro_memcheck_finish);
        ps_print_info("is in product mem check test !bfg_wakeup_host=%d\n",
                      oal_gpio_get_value(g_st_board_info.dev_wakeup_host[B_SYS]));
        return 0;
    }
    return -1;
}

void memcheck_bfgx_init(void)
{
    struct pm_drv_data *pm_data = pm_get_drvdata(BUART);
    bfgx_gpio_intr_enable(pm_data, OAL_TRUE);
    ps_print_info("memcheck_bfgx_init\n");
    if (get_hi110x_subchip_type() == BOARD_VERSION_SHENKUO) {
        pm_data = pm_get_drvdata(GUART);
        bfgx_gpio_intr_enable(pm_data, OAL_TRUE);
        ps_print_info("memcheck_gcpu_init\n");
    }
    g_pro_memcheck_en = 1;
    init_completion(&g_pro_memcheck_finish);
}
void memcheck_bfgx_exit(void)
{
    struct pm_drv_data *pm_data = pm_get_drvdata(BUART);
    g_pro_memcheck_en = 0;
    bfgx_gpio_intr_enable(pm_data, OAL_FALSE);
    if (get_hi110x_subchip_type() == BOARD_VERSION_SHENKUO) {
        pm_data = pm_get_drvdata(GUART);
        bfgx_gpio_intr_enable(pm_data, OAL_FALSE);
    }
}

int32_t wait_gpio_level(int32_t gpio_num, int32_t gpio_level, unsigned long timeout)
{
    int32_t gpio_value;
    if ((gpio_num < 0) || (timeout == 0)) {
        oal_warn_on(1);
        return -FAILURE;
    }
    gpio_level = (gpio_level == GPIO_LOWLEVEL) ? GPIO_LOWLEVEL : GPIO_HIGHLEVEL;
    forever_loop() {
        gpio_value = oal_gpio_get_value(gpio_num);
        gpio_value = (gpio_value == GPIO_LOWLEVEL) ? GPIO_LOWLEVEL : GPIO_HIGHLEVEL;
        if (gpio_value == gpio_level) {
            return SUCCESS;
        }

        if (time_after(jiffies, timeout)) {
            return -FAILURE;
        } else {
            oal_msleep(SLEEP_10_MSEC);
        }
    }
}

int32_t memcheck_bfgx_is_succ(int32_t is_bcpu)
{
    unsigned long timeout;
    unsigned long timeout_hold;
    uint32_t hold_time = 100; /* 拉高维持100ms */
    int32_t gpio_num;
    declare_time_cost_stru(cost);

    if (is_bcpu) {
        gpio_num = g_st_board_info.dev_wakeup_host[B_SYS];
    } else {
        gpio_num = g_st_board_info.dev_wakeup_host[G_SYS];
    }

    /* 中断改成电平判断，WLAN POWERON拉高瞬间存在毛刺会误报中断 */
    timeout = jiffies + msecs_to_jiffies(g_bfgx_mem_check_mdelay);
    ps_print_info("bfgx memcheck gpio level check start,timeout=%d ms\n", g_bfgx_mem_check_mdelay);
    oal_get_time_cost_start(cost);

    while (1) {
        // wait gpio high
        if (wait_gpio_level(gpio_num, GPIO_HIGHLEVEL, timeout) != SUCCESS) {
            ps_print_err("[E]wait wakeup gpio to high timeout [%u] ms[%lu:%lu]\n",
                         g_bfgx_mem_check_mdelay, jiffies, timeout);
            return -FAILURE;
        }
        oal_get_time_cost_end(cost);
        oal_calc_time_cost_sub(cost);
        ps_print_info("%s wake up host gpio to high %llu us",
                      (is_bcpu == true) ? "bcpu" : "gcpu", time_cost_var_sub(cost));

        // high level hold time
        timeout_hold = jiffies + msecs_to_jiffies(hold_time);
        oal_msleep(SLEEP_10_MSEC);

        // wait high level hold time
        if (wait_gpio_level(gpio_num, GPIO_LOWLEVEL, timeout_hold) == SUCCESS) {
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "[E]gpio pull down again, retry");
            oal_msleep(SLEEP_10_MSEC);
            continue;
        }

        // gpio high and hold enough time.
        oal_get_time_cost_end(cost);
        oal_calc_time_cost_sub(cost);
        ps_print_info("%s hold high level %u ms,check succ, test cost %llu us",
                      (is_bcpu == true) ? "bcpu" : "gcpu", hold_time, time_cost_var_sub(cost));
        break;
    }

    return SUCCESS;
}

static int32_t device_mem_check_priv_init(void)
{
    int32_t ret;
    uint32_t ram_test_voltage_addr;
    uint32_t process_sel_addr;
    if (get_hi110x_subchip_type() == BOARD_VERSION_SHENKUO) {
        ram_test_voltage_addr = RAM_TEST_RUN_VOLTAGE_REG_ADDR06;
        process_sel_addr = RAM_TEST_RUN_PROCESS_SEL_REG_ADDR06;
    } else {
        ram_test_voltage_addr = RAM_TEST_RUN_VOLTAGE_REG_ADDR;
        process_sel_addr = RAM_TEST_RUN_PROCESS_SEL_REG_ADDR;
    }
    ret = write_device_reg16(ram_test_voltage_addr,
                             (g_ram_test_run_voltage_bias_sel == RAM_TEST_RUN_VOLTAGE_BIAS_HIGH) ?
                              RAM_TEST_RUN_VOLTAGE_BIAS_HIGH : RAM_TEST_RUN_VOLTAGE_BIAS_LOW);
    if (ret) {
        ps_print_err("write reg=0x%x value=0x%x failed, high bias\n",
                     ram_test_voltage_addr, g_ram_test_run_voltage_bias_sel);
        return ret;
    }

    ps_print_info("g_ram_test_run_voltage_bias_sel=%d, [%s]\n",
                  g_ram_test_run_voltage_bias_sel,
                  (g_ram_test_run_voltage_bias_sel == RAM_TEST_RUN_VOLTAGE_BIAS_HIGH) ?
                   "high voltage bias" : "low voltage bias");

    if (g_ram_test_run_process_sel) {
        ret = write_device_reg16(process_sel_addr, g_ram_test_run_process_sel);
        if (ret) {
            ps_print_err("write reg=0x%x value=0x%x failed, process sel\n",
                         process_sel_addr, g_ram_test_run_process_sel);
            return ret; /* just test, if failed, we don't return */
        }
        ps_print_info("ram_test_run_process_sel=%d\n", g_ram_test_run_process_sel);
    }
    return ret;
}

static void factory_cfg_init(bool is_wifi)
{
    if (get_hi110x_subchip_type() == BOARD_VERSION_HI1103) {
        g_hi1103_pilot_cfg_patch_in_vendor[RAM_REG_TEST_CFG] = is_wifi ?
            RAM_CHECK_CFG_HI1103_PILOT_PATH : RAM_BCPU_CHECK_CFG_HI1103_PILOT_PATH;
    } else if (get_hi110x_subchip_type() == BOARD_VERSION_HI1105) {
        g_hi1105_asic_cfg_patch_in_vendor[RAM_REG_TEST_CFG] = is_wifi ?
            RAM_CHECK_CFG_HI1105_ASIC_PATH : RAM_BCPU_CHECK_CFG_HI1105_ASIC_PATH;
    } else if (get_hi110x_subchip_type() == BOARD_VERSION_SHENKUO) {
        if (hi110x_is_asic()) {
            g_shenkuo_asic_cfg_patch_in_vendor[RAM_REG_TEST_CFG] = is_wifi ?
                RAM_WIFI_CHECK_CFG_SHENKUO_ASIC_PATH : RAM_BGCPU_CHECK_CFG_SHENKUO_ASIC_PATH;
        } else {
            g_shenkuo_fpga_cfg_patch_in_vendor[RAM_REG_TEST_CFG] = is_wifi ?
                RAM_WIFI_CHECK_CFG_SHENKUO_FPGA_PATH : RAM_BGCPU_CHECK_CFG_SHENKUO_FPGA_PATH;
        }
    } else {
        ps_print_err("board subchip type err, only support 1103&1105&shenkuo, now is [%d] ", get_hi110x_subchip_type());
    }
}

static void device_mem_check_wcpu_debug(int32_t ret, uint32_t *wcost)
{
    if (g_ram_test_detail_result_dump) {
        get_device_ram_test_result(true, wcost);
    }

    if (g_ram_test_wifi_hold_time) {
        oal_msleep(g_ram_test_wifi_hold_time);
    }

    if (!ret) {
        ps_print_info("==device wcpu ram reg test success!\n");
        if (g_ram_test_ssi_pass_dump)
            ssi_dump_device_regs(SSI_MODULE_MASK_AON | SSI_MODULE_MASK_ARM_REG |
                                    SSI_MODULE_MASK_WCTRL | SSI_MODULE_MASK_BCTRL);
        if (g_ram_test_mem_pass_dump) {
            get_device_test_mem(true);
        }
    } else {
        ps_print_info("==device wcpu ram reg test failed!\n");
        if (g_ram_test_ssi_error_dump) {
            ssi_dump_device_regs(SSI_MODULE_MASK_AON | SSI_MODULE_MASK_ARM_REG |
                                    SSI_MODULE_MASK_WCTRL | SSI_MODULE_MASK_BCTRL);
        } else {
            ssi_dump_device_regs(SSI_MODULE_MASK_ARM_REG);
        }
        get_device_test_mem(true);
    }
}

static int32_t device_mem_check_wcpu(uint32_t *wcost)
{
    int32_t ret;

    ret = board_power_on(W_SYS);
    if (ret) {
        ps_print_err("W_SYS on failed ret=%d\n", ret);
        return -FAILURE;
    }

    ret = firmware_download_function_priv(RAM_REG_TEST_CFG, device_mem_check_priv_init,
                                          hcc_get_bus(HCC_EP_WIFI_DEV));
    if (ret == SUCCESS) {
        if (get_hi110x_subchip_type() != BOARD_VERSION_SHENKUO) {
            /* 之前的项目不更改，仍然使用delay的方式 */
            mdelay(g_wlan_mem_check_mdelay);
        }
        ret = is_device_mem_test_succ();
        device_mem_check_wcpu_debug(ret, wcost);
        if (ret) {
            board_power_off(W_SYS);
            return -OAL_EFAIL;
        }
    }
    board_power_off(W_SYS);
    return OAL_SUCC;
}

static void  device_mem_check_bcpu_debug(int32_t ret, uint32_t *bcost, int32_t is_bcpu)
{
    if (g_ram_test_detail_result_dump) {
        get_device_ram_test_result(false, bcost);
    }

    if (g_ram_test_bfgx_hold_time) {
        oal_msleep(g_ram_test_bfgx_hold_time);
    }

    if (!ret) {
        ps_print_info("==device %s ram reg test success!\n", (is_bcpu == true) ? "bcpu" : "gcpu");
        if (g_ram_test_ssi_pass_dump) {
            ssi_dump_device_regs(SSI_MODULE_MASK_AON | SSI_MODULE_MASK_ARM_REG |
                                 SSI_MODULE_MASK_WCTRL | SSI_MODULE_MASK_BCTRL);
        }

        if (g_ram_test_mem_pass_dump) {
            get_device_test_mem(false);
        }

        if (is_bcpu) {
            ps_print_info("[memcheck]bcpu_wkup_host=%d\n", oal_gpio_get_value(g_st_board_info.dev_wakeup_host[B_SYS]));
        } else {
            ps_print_info("[memcheck]gcpu_wkup_host=%d\n", oal_gpio_get_value(g_st_board_info.dev_wakeup_host[G_SYS]));
        }
    } else {
        ps_print_info("==device %s ram reg test failed!\n", (is_bcpu == true) ? "bcpu" : "gcpu");
        firmware_get_cfg(g_cfg_path[RAM_REG_TEST_CFG], RAM_REG_TEST_CFG);
        if (g_ram_test_ssi_error_dump) {
            ssi_dump_device_regs(SSI_MODULE_MASK_AON | SSI_MODULE_MASK_ARM_REG |
                                 SSI_MODULE_MASK_WCTRL | SSI_MODULE_MASK_BCTRL);
        } else {
            ssi_dump_device_regs(SSI_MODULE_MASK_ARM_REG);
        }
        get_device_test_mem(false);
    }
}

static int32_t device_mem_check_bcpu(uint32_t *bcost)
{
    int32_t ret;

    board_power_on(W_SYS);

    factory_cfg_init(false);

    ret = firmware_get_cfg(g_cfg_path[RAM_REG_TEST_CFG], RAM_REG_TEST_CFG);
    if (ret) {
        ps_print_info("ini analysis fail!\n");
        board_power_off(W_SYS);
        return -OAL_EFAIL;
    }

    memcheck_bfgx_init();

    if (get_hi110x_subchip_type() == BOARD_VERSION_SHENKUO) {
        ret = firmware_download_function_priv(RAM_REG_TEST_CFG + 1, device_mem_check_priv_init,
                                              hcc_get_bus(HCC_EP_WIFI_DEV));
    } else {
        ret = firmware_download_function_priv(RAM_REG_TEST_CFG, device_mem_check_priv_init,
                                              hcc_get_bus(HCC_EP_WIFI_DEV));
    }

    factory_cfg_init(true);

    if (ret == SUCCESS) {
        /* 等待device信息处理 */
        ret = memcheck_bfgx_is_succ(true);
        device_mem_check_bcpu_debug(ret, bcost, true);
    }

    firmware_get_cfg(g_cfg_path[RAM_REG_TEST_CFG], RAM_REG_TEST_CFG);
    memcheck_bfgx_exit();
    if (get_hi110x_subchip_type() != BOARD_VERSION_SHENKUO) {
        board_power_off(W_SYS);
    }

    return ret;
}

static int32_t device_mem_check_gcpu(uint32_t *gcost)
{
    int32_t ret;

    /* 等待device信息处理 */
    ret = memcheck_bfgx_is_succ(false);
    device_mem_check_bcpu_debug(ret, gcost, false);
    board_power_off(W_SYS);
    return ret;
}

static int32_t device_mem_check_prepare(unsigned long long *time)
{
    int32_t ret = 0;
    if (time == NULL) {
        ps_print_err("param time is  NULL!\n");
        return -FAILURE;
    }

    if (!bfgx_is_shutdown()) {
        ps_print_suc("factory ram reg test need bfgx shut down!\n");
        bfgx_print_subsys_state();
        return -FAILURE;
    }
    if (!wlan_is_shutdown()) {
        ps_print_suc("factory ram reg test need wlan shut down!\n");
        return -FAILURE;
    }

    return ret;
}
int32_t device_mem_check(unsigned long long *time)
{
    int32_t ret;
    uint32_t wcost = 0;
    uint32_t bcost = 0;
    uint32_t gcost = 0;

    unsigned long long total_time;
    ktime_t start_time, end_time, trans_time;

    start_time = ktime_get();

    ps_print_info("device ram reg test!\n");
    ret = device_mem_check_prepare(time);
    if (ret != 0) {
        return ret;
    }

    ps_print_info("start wcpu ram reg test!\n");
    ret = device_mem_check_wcpu(&wcost);
    if (ret != OAL_SUCC) {
        return ret;
    }

    ps_print_info("start bcpu ram reg test!\n");
    ret = device_mem_check_bcpu(&bcost);
    if (ret != OAL_SUCC) {
        return ret;
    }

    if (get_hi110x_subchip_type() == BOARD_VERSION_SHENKUO) {
        ps_print_info("start gcpu ram reg test!\n");
        ret = device_mem_check_gcpu(&gcost);
        if (ret != OAL_SUCC) {
            return ret;
        }
    }

    end_time = ktime_get();
    trans_time = ktime_sub(end_time, start_time);
    total_time = (unsigned long long)ktime_to_us(trans_time);

    *time = total_time;

    if (wcost + bcost + gcost) {
        ps_print_suc("device mem reg test time [%llu]us, actual cost=%u us\n", total_time, wcost + bcost + gcost);
    } else {
        ps_print_suc("device mem reg test time [%llu]us\n", total_time);
    }

    return ret;
}

EXPORT_SYMBOL(device_mem_check);
