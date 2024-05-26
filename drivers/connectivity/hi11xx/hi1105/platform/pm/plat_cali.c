

/* 头文件包含 */
#include "plat_cali.h"
#include "plat_firmware.h"
#include "plat_debug.h"
#include "bfgx_dev.h"
#include "plat_pm.h"
#include "pcie_linux.h"
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
#ifndef _PRE_NO_HISI_NVRAM
#define HISI_NVRAM_SUPPORT
#endif
#endif

/* 保存校准数据的buf */
STATIC uint8_t *g_cali_data_buf = NULL; /* wifi校准数据(仅SDIO使用) */
STATIC uint8_t *g_cali_data_buf_dma = NULL; /* wifi校准数据(仅PCIE使用) */
STATIC uint64_t g_cali_data_buf_phy_addr = 0; /* 06 wifi校准数据DMA物理地址 */
uint8_t g_netdev_is_open = OAL_FALSE;
uint32_t g_cali_update_channel_info = 0;

/* add for hi1103 bfgx */
struct completion g_cali_recv_done;
STATIC uint8_t *g_bfgx_cali_data_buf = NULL;

/* 定义不能超过BFGX_BT_CUST_INI_SIZE/4 (128) 最后使用 { NULL, 0 } 结尾 */
STATIC bfgx_ini_cmd g_bfgx_ini_config_cmd_1103[BFGX_BT_CUST_INI_SIZE / 4] = {
    { "bt_maxpower",                       0 },
    { "bt_edrpow_offset",                  0 },
    { "bt_blepow_offset",                  0 },
    { "bt_cali_txpwr_pa_ref_num",          0 },
    { "bt_cali_txpwr_pa_ref_band1",        0 },
    { "bt_cali_txpwr_pa_ref_band2",        0 },
    { "bt_cali_txpwr_pa_ref_band3",        0 },
    { "bt_cali_txpwr_pa_ref_band4",        0 },
    { "bt_cali_txpwr_pa_ref_band5",        0 },
    { "bt_cali_txpwr_pa_ref_band6",        0 },
    { "bt_cali_txpwr_pa_ref_band7",        0 },
    { "bt_cali_txpwr_pa_ref_band8",        0 },
    { "bt_cali_txpwr_pa_fre1",             0 },
    { "bt_cali_txpwr_pa_fre2",             0 },
    { "bt_cali_txpwr_pa_fre3",             0 },
    { "bt_cali_txpwr_pa_fre4",             0 },
    { "bt_cali_txpwr_pa_fre5",             0 },
    { "bt_cali_txpwr_pa_fre6",             0 },
    { "bt_cali_txpwr_pa_fre7",             0 },
    { "bt_cali_txpwr_pa_fre8",             0 },
    { "bt_cali_bt_tone_amp_grade",         0 },
    { "bt_rxdc_band",                      0 },
    { "bt_dbb_scaling_saturation",         0 },
    { "bt_productline_upccode_search_max", 0 },
    { "bt_productline_upccode_search_min", 0 },
    { "bt_dynamicsarctrl_bt",              0 },
    { "bt_powoffsbt",                      0 },
    { "bt_elna_2g_bt",                     0 },
    { "bt_rxisobtelnabyp",                 0 },
    { "bt_rxgainbtelna",                   0 },
    { "bt_rxbtextloss",                    0 },
    { "bt_elna_on2off_time_ns",            0 },
    { "bt_elna_off2on_time_ns",            0 },
    { "bt_hipower_mode",                   0 },
    { "bt_fem_control",                    0 },
    { "bt_feature_32k_clock",              0 },
    { "bt_feature_log",                    0 },
    { "bt_cali_swtich_all",                0 },
    { "bt_ant_num_bt",                     0 },
    { "bt_power_level_control",            0 },
    { "bt_country_code",                   0 },
    { "bt_reserved1",                      0 },
    { "bt_reserved2",                      0 },
    { "bt_reserved3",                      0 },
    { "bt_dedicated_antenna",              0 },
    { "bt_reserved5",                      0 },
    { "bt_reserved6",                      0 },
    { "bt_reserved7",                      0 },
    { "bt_reserved8",                      0 },
    { "bt_reserved9",                      0 },
    { "bt_reserved10",                     0 },

    { NULL, 0 }
};

/* 定义不能超过BFGX_BT_CUST_INI_SIZE/4 (128) 最后使用 { NULL, 0 } 结尾 */
STATIC bfgx_ini_cmd g_bfgx_ini_config_cmd_1105[BFGX_BT_CUST_INI_SIZE / 4] = {
    { "bt_maxpower",                       0 },
    { "bt_edrpow_offset",                  0 },
    { "bt_blepow_offset",                  0 },
    { "bt_cali_txpwr_pa_ref_num",          0 },
    { "bt_cali_txpwr_pa_ref_band1",        0 },
    { "bt_cali_txpwr_pa_ref_band2",        0 },
    { "bt_cali_txpwr_pa_ref_band3",        0 },
    { "bt_cali_txpwr_pa_ref_band4",        0 },
    { "bt_cali_txpwr_pa_ref_band5",        0 },
    { "bt_cali_txpwr_pa_ref_band6",        0 },
    { "bt_cali_txpwr_pa_ref_band7",        0 },
    { "bt_cali_txpwr_pa_ref_band8",        0 },
    { "bt_cali_txpwr_pa_fre1",             0 },
    { "bt_cali_txpwr_pa_fre2",             0 },
    { "bt_cali_txpwr_pa_fre3",             0 },
    { "bt_cali_txpwr_pa_fre4",             0 },
    { "bt_cali_txpwr_pa_fre5",             0 },
    { "bt_cali_txpwr_pa_fre6",             0 },
    { "bt_cali_txpwr_pa_fre7",             0 },
    { "bt_cali_txpwr_pa_fre8",             0 },
    { "bt_cali_bt_tone_amp_grade",         0 },
    { "bt_rxdc_band",                      0 },
    { "bt_dbb_scaling_saturation",         0 },
    { "bt_productline_upccode_search_max", 0 },
    { "bt_productline_upccode_search_min", 0 },
    { "bt_dynamicsarctrl_bt",              0 },
    { "bt_powoffsbt",                      0 },
    { "bt_elna_2g_bt",                     0 },
    { "bt_rxisobtelnabyp",                 0 },
    { "bt_rxgainbtelna",                   0 },
    { "bt_rxbtextloss",                    0 },
    { "bt_elna_on2off_time_ns",            0 },
    { "bt_elna_off2on_time_ns",            0 },
    { "bt_hipower_mode",                   0 },
    { "bt_fem_control",                    0 },
    { "bt_feature_32k_clock",              0 },
    { "bt_feature_log",                    0 },
    { "bt_cali_switch_all",                0 },
    { "bt_ant_num_bt",                     0 },
    { "bt_power_level_control",            0 },
    { "bt_country_code",                   0 },
    { "bt_power_idle_voltage",             0 },
    { "bt_power_tx_voltage",               0 },
    { "bt_power_rx_voltage",               0 },
    { "bt_power_sleep_voltage",            0 },
    { "bt_power_idle_current",             0 },
    { "bt_power_tx_current",               0 },
    { "bt_power_rx_current",               0 },
    { "bt_power_sleep_current",            0 },
    { "bt_20dbm_txpwr_cali_num",           0 },
    { "bt_20dbm_txpwr_cali_freq_ref1",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref2",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref3",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref4",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref5",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref6",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref7",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref8",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref9",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref10",    0 },
    { "bt_20dbm_txpwr_cali_freq_ref11",    0 },
    { "bt_20dbm_txpwr_cali_freq_ref12",    0 },
    { "bt_20dbm_txpwr_cali_freq_ref13",    0 },
    { "bt_reserved1",                      0 },
    { "bt_20dbm_txpwr_adjust",             0 },
    { "bt_feature_config",                 0 },
    { "bt_dedicated_antenna",              0 },
    { "bt_reserved5",                      0 },
    { "bt_reserved6",                      0 },
    { "bt_reserved7",                      0 },
    { "bt_reserved8",                      0 },
    { "bt_reserved9",                      0 },
    { "bt_reserved10",                     0 },

    { NULL, 0 }
};

/* 定义不能超过BFGX_BT_CUST_INI_SIZE/4 (128) 最后使用 { NULL, 0 } 结尾 */
STATIC bfgx_ini_cmd g_bfgx_ini_config_cmd_shenkuo[BFGX_BT_CUST_INI_SIZE / 4] = {
    { "bt_maxpower",                       0 },
    { "bt_edrpow_offset",                  0 },
    { "bt_blepow_offset",                  0 },
    { "bt_txpwr_channel_num",              0 },
    /* BT tx pwr cali normal power Ad */
    { "bt_txpwr_normal_ad_c_offset1",      0 },
    { "bt_txpwr_normal_ad_c_offset2",      0 },
    { "bt_txpwr_normal_ad_c_offset3",      0 },
    { "bt_txpwr_normal_ad_c_offset4",      0 },
    /* BT tx pwr cali normal power A0 */
    { "bt_txpwr_normal_a0_c_offset1",      0 },
    { "bt_txpwr_normal_a0_c_offset2",      0 },
    { "bt_txpwr_normal_a0_c_offset3",      0 },
    { "bt_txpwr_normal_a0_c_offset4",      0 },
    /* BT tx pwr cali high power AD */
    { "bt_txpwr_high_ad_c_offset1",        0 },
    { "bt_txpwr_high_ad_c_offset2",        0 },
    { "bt_txpwr_high_ad_c_offset3",        0 },
    { "bt_txpwr_high_ad_c_offset4",        0 },
    /* BT tx pwr cali high power A0 */
    { "bt_txpwr_high_a0_c_offset1",        0 },
    { "bt_txpwr_high_a0_c_offset2",        0 },
    { "bt_txpwr_high_a0_c_offset3",        0 },
    { "bt_txpwr_high_a0_c_offset4",        0 },
    { "bt_txpwr_channel1",                 0 },
    { "bt_txpwr_channel2",                 0 },
    { "bt_txpwr_channel3",                 0 },
    { "bt_txpwr_channel4",                 0 },
    { "bt_txpwr_channel5",                 0 },
    { "bt_txpwr_channel6",                 0 },
    { "bt_txpwr_channel7",                 0 },
    { "bt_txpwr_channel8",                 0 },
    { "bt_cali_bt_tone_amp_grade",         0 },
    { "bt_rxdc_band",                      0 },
    { "bt_dbb_scaling_saturation",         0 },
    { "bt_productline_upccode_search_max", 0 },
    { "bt_productline_upccode_search_min", 0 },
    { "bt_dynamicsarctrl_bt",              0 },
    { "bt_powoffsbt",                      0 },
    { "bt_elna_2g_bt",                     0 },
    { "bt_rxisobtelnabyp",                 0 },
    { "bt_rxgainbtelna",                   0 },
    { "bt_rxbtextloss",                    0 },
    { "bt_elna_on2off_time_ns",            0 },
    { "bt_elna_off2on_time_ns",            0 },
    { "bt_hipower_mode",                   0 },
    { "bt_fem_control",                    0 },
    { "bt_feature_32k_clock",              0 },
    { "bt_feature_log",                    0 },
    { "bt_cali_switch_all",                0 },
    { "bt_ant_num_bt",                     0 },
    { "bt_power_level_control",            0 },
    { "bt_country_code",                   0 },
    { "bt_power_idle_voltage",             0 },
    { "bt_power_tx_voltage",               0 },
    { "bt_power_rx_voltage",               0 },
    { "bt_power_sleep_voltage",            0 },
    { "bt_power_idle_current",             0 },
    { "bt_power_tx_current",               0 },
    { "bt_power_rx_current",               0 },
    { "bt_power_sleep_current",            0 },
    { "bt_20dbm_txpwr_cali_num",           0 },
    { "bt_20dbm_txpwr_cali_freq_ref1",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref2",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref3",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref4",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref5",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref6",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref7",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref8",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref9",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref10",    0 },
    { "bt_20dbm_txpwr_cali_freq_ref11",    0 },
    { "bt_20dbm_txpwr_cali_freq_ref12",    0 },
    { "bt_20dbm_txpwr_cali_freq_ref13",    0 },
    { "bt_reserved1",                      0 },
    { "bt_20dbm_txpwr_adjust",             0 },
    { "bt_feature_config",                 0 },
    { "bt_dedicated_antenna",              0 },
    { "bt_reserved5",                      0 },
    { "bt_reserved6",                      0 },
    { "bt_reserved7",                      0 },
    { "bt_reserved8",                      0 },
    { "bt_reserved9",                      0 },
    { "bt_reserved10",                     0 },

    { NULL, 0 }
};

/* 定义不能超过BFGX_BT_CUST_INI_SIZE/4 (128) 最后使用 { NULL, 0 } 结尾 */
STATIC bfgx_ini_cmd g_bfgx_ini_config_cmd_bisheng[BFGX_BT_CUST_INI_SIZE / 4] = {
    { "bt_maxpower",                       0 },
    { "bt_edrpow_offset",                  0 },
    { "bt_blepow_offset",                  0 },
    { "bt_elna_2g_bt",                     0 },
    { "rf_trx_features",                   0 },
    { "bt_cali_switch_all",                0 },
    { "bt_power_idle_voltage",             0 },
    { "bt_power_tx_voltage",               0 },
    { "bt_power_rx_voltage",               0 },
    { "bt_power_sleep_voltage",            0 },
    { "bt_power_idle_current",             0 },
    { "bt_power_tx_current",               0 },
    { "bt_power_rx_current",               0 },
    { "bt_power_sleep_current",            0 },
    { "bt_feature_config",                 0 },
    { "bt_dedicated_antenna",              0 },
    { "bt_priv_config",                    0 },
    { "bt_reserved10",                     0 },

    { NULL, 0 }
};

/* 定义不能超过BFGX_BT_CUST_INI_SIZE/4 (128) 最后使用 { NULL, 0 } 结尾 */
STATIC bfgx_ini_cmd g_bfgx_ini_config_cmd_1161[BFGX_BT_CUST_INI_SIZE / 4] = {
    { "bt_maxpower",                       0 },
    { "bt_edrpow_offset",                  0 },
    { "bt_blepow_offset",                  0 },
    { "bt_txpwr_channel_num",              0 },
    /* BT tx pwr cali normal power Ad */
    { "bt_txpwr_normal_ad_c_offset1",      0 },
    { "bt_txpwr_normal_ad_c_offset2",      0 },
    { "bt_txpwr_normal_ad_c_offset3",      0 },
    { "bt_txpwr_normal_ad_c_offset4",      0 },
    /* BT tx pwr cali normal power A0 */
    { "bt_txpwr_normal_a0_c_offset1",      0 },
    { "bt_txpwr_normal_a0_c_offset2",      0 },
    { "bt_txpwr_normal_a0_c_offset3",      0 },
    { "bt_txpwr_normal_a0_c_offset4",      0 },
    /* BT tx pwr cali high power AD */
    { "bt_txpwr_high_ad_c_offset1",        0 },
    { "bt_txpwr_high_ad_c_offset2",        0 },
    { "bt_txpwr_high_ad_c_offset3",        0 },
    { "bt_txpwr_high_ad_c_offset4",        0 },
    /* BT tx pwr cali high power A0 */
    { "bt_txpwr_high_a0_c_offset1",        0 },
    { "bt_txpwr_high_a0_c_offset2",        0 },
    { "bt_txpwr_high_a0_c_offset3",        0 },
    { "bt_txpwr_high_a0_c_offset4",        0 },
    { "bt_txpwr_channel1",                 0 },
    { "bt_txpwr_channel2",                 0 },
    { "bt_txpwr_channel3",                 0 },
    { "bt_txpwr_channel4",                 0 },
    { "bt_txpwr_channel5",                 0 },
    { "bt_txpwr_channel6",                 0 },
    { "bt_txpwr_channel7",                 0 },
    { "bt_txpwr_channel8",                 0 },
    { "bt_cali_bt_tone_amp_grade",         0 },
    { "bt_rxdc_band",                      0 },
    { "bt_dbb_scaling_saturation",         0 },
    { "bt_productline_upccode_search_max", 0 },
    { "bt_productline_upccode_search_min", 0 },
    { "bt_dynamicsarctrl_bt",              0 },
    { "bt_powoffsbt",                      0 },
    { "bt_elna_2g_bt",                     0 },
    { "bt_rxisobtelnabyp",                 0 },
    { "bt_rxgainbtelna",                   0 },
    { "bt_rxbtextloss",                    0 },
    { "bt_elna_on2off_time_ns",            0 },
    { "bt_elna_off2on_time_ns",            0 },
    { "bt_hipower_mode",                   0 },
    { "bt_fem_control",                    0 },
    { "bt_feature_32k_clock",              0 },
    { "bt_feature_log",                    0 },
    { "bt_cali_switch_all",                0 },
    { "bt_ant_num_bt",                     0 },
    { "bt_power_level_control",            0 },
    { "bt_country_code",                   0 },
    { "bt_power_idle_voltage",             0 },
    { "bt_power_tx_voltage",               0 },
    { "bt_power_rx_voltage",               0 },
    { "bt_power_sleep_voltage",            0 },
    { "bt_power_idle_current",             0 },
    { "bt_power_tx_current",               0 },
    { "bt_power_rx_current",               0 },
    { "bt_power_sleep_current",            0 },
    { "bt_20dbm_txpwr_cali_num",           0 },
    { "bt_20dbm_txpwr_cali_freq_ref1",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref2",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref3",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref4",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref5",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref6",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref7",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref8",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref9",     0 },
    { "bt_20dbm_txpwr_cali_freq_ref10",    0 },
    { "bt_20dbm_txpwr_cali_freq_ref11",    0 },
    { "bt_20dbm_txpwr_cali_freq_ref12",    0 },
    { "bt_20dbm_txpwr_cali_freq_ref13",    0 },
    { "bt_reserved1",                      0 },
    { "bt_20dbm_txpwr_adjust",             0 },
    { "bt_feature_config",                 0 },
    { "bt_dedicated_antenna",              0 },
    { "bt_reserved5",                      0 },
    { "bt_reserved6",                      0 },
    { "bt_reserved7",                      0 },
    { "bt_reserved8",                      0 },
    { "bt_reserved9",                      0 },
    { "bt_reserved10",                     0 },

    { NULL, 0 }
};

/* 定义不能超过BFGX_BT_CUST_INI_SIZE/4 (128) */
STATIC int32_t g_bfgx_cust_ini_data[BFGX_BT_CUST_INI_SIZE / 4] = {0};

/*
 * 函 数 名  : get_cali_count
 * 功能描述  : 返回校准的次数，首次开机校准时为0，以后递增
 * 输入参数  : uint32 *count:调用函数保存校准次数的地址
 * 输出参数  : count:自开机以来，已经校准的次数
 * 返 回 值  : 0表示成功，-1表示失败
 */
int32_t get_cali_count(uint32_t *count)
{
    if (count == NULL) {
        ps_print_err("count is NULL\n");
        return -EFAIL;
    }

    *count = g_cali_update_channel_info;

    ps_print_warning("cali update info is [%d]\r\n", g_cali_update_channel_info);

    return SUCC;
}

#ifdef HISI_NVRAM_SUPPORT
/*
 * 函 数 名  : bfgx_get_nv_data_buf
 * 功能描述  : 返回保存bfgx nv数据的内存地址
 * 输出参数  : nv buf的长度
 * 返 回 值  : bfgx nv数据buf的地址，也可能是NULL
 */
STATIC void *bfgx_get_nv_data_buf(uint32_t *pul_len)
{
    bfgx_cali_data_stru *pst_bfgx_cali_data_buf = NULL;

    if (g_bfgx_cali_data_buf == NULL) {
        return NULL;
    }

    pst_bfgx_cali_data_buf = (bfgx_cali_data_stru *)g_bfgx_cali_data_buf;

    *pul_len = sizeof(pst_bfgx_cali_data_buf->auc_nv_data);

    ps_print_info("bfgx nv buf size is %d\n", *pul_len);

    return pst_bfgx_cali_data_buf->auc_nv_data;
}

/*
 * 函 数 名  : bfgx_nv_data_init
 * 功能描述  : bt 校准NV读取
 */
STATIC int32_t bfgx_nv_data_init(void)
{
#ifdef _PRE_WINDOWS_SUPPORT
    return INI_SUCC;
#else
    int32_t l_ret;
    char *pst_buf;
    uint32_t len;

    uint8_t bt_cal_nvram_tmp[OAL_BT_NVRAM_DATA_LENGTH];

    l_ret = read_conf_from_nvram(bt_cal_nvram_tmp, OAL_BT_NVRAM_DATA_LENGTH, OAL_BT_NVRAM_NUMBER, OAL_BT_NVRAM_NAME);
    if (l_ret != INI_SUCC) {
        ps_print_err("bfgx_nv_data_init::BT read NV error!");
        // last byte of NV ram is used to mark if NV ram is failed to read.
        bt_cal_nvram_tmp[OAL_BT_NVRAM_DATA_LENGTH - 1] = OAL_TRUE;
    } else {
        // last byte of NV ram is used to mark if NV ram is failed to read.
        bt_cal_nvram_tmp[OAL_BT_NVRAM_DATA_LENGTH - 1] = OAL_FALSE;
    }

    pst_buf = bfgx_get_nv_data_buf(&len);
    if (pst_buf == NULL) {
        ps_print_err("get bfgx nv buf fail!");
        return INI_FAILED;
    }
    if (len < OAL_BT_NVRAM_DATA_LENGTH) {
        ps_print_err("get bfgx nv buf size %d, NV data size is %d!", len, OAL_BT_NVRAM_DATA_LENGTH);
        return INI_FAILED;
    }

    l_ret = memcpy_s(pst_buf, len, bt_cal_nvram_tmp, OAL_BT_NVRAM_DATA_LENGTH);
    if (l_ret != EOK) {
        ps_print_err("bfgx_nv_data_init FAILED!");
        return INI_FAILED;
    }
    ps_print_info("bfgx_nv_data_init SUCCESS");
    return INI_SUCC;
#endif
}
#endif

/*
 * 函 数 名  : hi1103_get_bfgx_cali_data
 * 功能描述  : 返回保存bfgx校准数据的内存首地址以及长度
 * 输入参数  : uint8  *buf:调用函数保存bfgx校准数据的首地址
 *             uint32_t *len:调用函数保存bfgx校准数据内存长度的地址
 *             uint32_t buf_len:buf的长度
 * 返 回 值  : 0表示成功，-1表示失败
 */
STATIC int32_t hi1103_get_bfgx_cali_data(uint8_t *buf, uint32_t *len, uint32_t buf_len)
{
    uint32_t bfgx_cali_data_len;

    ps_print_info("%s\n", __func__);

    if (unlikely(buf == NULL)) {
        ps_print_err("buf is NULL\n");
        return -EFAIL;
    }

    if (unlikely(len == NULL)) {
        ps_print_err("len is NULL\n");
        return -EFAIL;
    }

    if (unlikely(g_bfgx_cali_data_buf == NULL)) {
        ps_print_err("g_bfgx_cali_data_buf is NULL\n");
        return -EFAIL;
    }

#ifdef HISI_NVRAM_SUPPORT
    if (bfgx_nv_data_init() != OAL_SUCC) {
        ps_print_err("bfgx nv data init fail!\n");
    }
#endif

    bfgx_cali_data_len = sizeof(bfgx_cali_data_stru);
    if (buf_len < bfgx_cali_data_len) {
        ps_print_err("bfgx cali buf len[%d] is smaller than struct size[%d]\n", buf_len, bfgx_cali_data_len);
        return -EFAIL;
    }

    if (memcpy_s(buf, buf_len, g_bfgx_cali_data_buf, bfgx_cali_data_len) != EOK) {
        ps_print_err("bfgx cali data copy fail, buf len = [%d], bfgx_cali_data_len = [%d]\n",
                     buf_len, bfgx_cali_data_len);
        return -EFAIL;
    }
    *len = bfgx_cali_data_len;

    return SUCC;
}

/*
 * 函 数 名  : get_bfgx_cali_data
 * 功能描述  : 返回保存bfgx校准数据的内存首地址以及长度
 * 输入参数  : uint8  *buf:调用函数保存bfgx校准数据的首地址
 *             uint32_t *len:调用函数保存bfgx校准数据内存长度的地址
 *             uint32_t buf_len:buf的长度
 * 返 回 值  : 0表示成功，-1表示失败
 */
int32_t get_bfgx_cali_data(uint8_t *buf, uint32_t *len, uint32_t buf_len)
{
    int32_t l_subchip_type = get_hi110x_subchip_type();

    switch (l_subchip_type) {
        case BOARD_VERSION_HI1103:
        case BOARD_VERSION_HI1105:
        case BOARD_VERSION_SHENKUO:
        case BOARD_VERSION_BISHENG:
        case BOARD_VERSION_HI1161:
            return hi1103_get_bfgx_cali_data(buf, len, buf_len);
        default:
            ps_print_err("subchip type error! subchip=%d\n", l_subchip_type);
            return -EFAIL;
    }
}

/*
 * 函 数 名  : get_cali_data_buf_addr
 * 功能描述  : 返回保存校准数据的内存地址
 */
void *get_cali_data_buf_addr(void)
{
    int32_t type = get_hi110x_subchip_type();
    uint8_t *buffer = NULL;
    uint32_t ul_buffer_len = SHENKUO_MIMO_CALI_DATA_STRU_LEN;

    /* 仅针对shenkuo、bisheng、1161 PCIE */
    if (hcc_is_sdio() == OAL_TRUE ||
        (type != BOARD_VERSION_SHENKUO && type != BOARD_VERSION_BISHENG && type != BOARD_VERSION_HI1161)) {
        return g_cali_data_buf;
    }

    /* 已申请直接返回 */
    if (g_cali_data_buf_dma != NULL) {
        return g_cali_data_buf_dma;
    }

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
    buffer = dma_alloc_coherent(&oal_get_wifi_pcie_dev()->dev, ul_buffer_len,
        &g_cali_data_buf_phy_addr, GFP_KERNEL);
#elif (defined _PRE_PLAT_FEATURE_HI116X_PCIE)
    buffer = dma_alloc_coherent(&(oal_get_default_pcie_handler())->comm_res->pcie_dev->dev, ul_buffer_len,
        (dma_addr_t *)&g_cali_data_buf_phy_addr, GFP_KERNEL);
#endif
    if (buffer == NULL) {
        ps_print_err("get_cali_data_buf_addr:: malloc for g_cali_data_buf_dma fail\n");
        return NULL;
    }

    g_cali_data_buf_dma = buffer;
    memset_s(g_cali_data_buf_dma, ul_buffer_len, 0, ul_buffer_len);
    ps_print_info("get_cali_data_buf_addr:: buf(%p) size %d alloc succ", g_cali_data_buf_dma, ul_buffer_len);
    return g_cali_data_buf_dma;
}

/*
 * 函 数 名  : get_cali_data_buf_phy_addr
 * 功能描述  : 返回保存校准数据的DMA物理内存地址
 */
uint64_t get_cali_data_buf_phy_addr(void)
{
    return g_cali_data_buf_phy_addr;
}

EXPORT_SYMBOL(get_cali_data_buf_phy_addr);
EXPORT_SYMBOL(get_cali_data_buf_addr);
EXPORT_SYMBOL(g_netdev_is_open);
EXPORT_SYMBOL(g_cali_update_channel_info);

/*
 * 函 数 名  : wifi_get_bfgx_rc_data_buf_addr
 * 功能描述  : 返回保存wifi rc code校准数据的内存地址
 */
void *wifi_get_bfgx_rc_data_buf_addr(uint32_t *pul_len)
{
    bfgx_cali_data_stru *pst_bfgx_cali_buf = NULL;

    if (g_bfgx_cali_data_buf == NULL) {
        return NULL;
    }

    pst_bfgx_cali_buf = (bfgx_cali_data_stru *)g_bfgx_cali_data_buf;
    *pul_len = sizeof(pst_bfgx_cali_buf->auc_wifi_rc_code_data);

    ps_print_info("wifi cali size is %d\n", *pul_len);

    return pst_bfgx_cali_buf->auc_wifi_rc_code_data;
}

EXPORT_SYMBOL(wifi_get_bfgx_rc_data_buf_addr);

/*
 * 函 数 名  : wifi_get_bt_cali_data_buf
 * 功能描述  : 返回保存wifi cali data for bt数据的内存地址
 * 输出参数  : wifi cali data for bt数据buf的长度
 * 返 回 值  : wifi cali data for bt数据buf的地址，也可能是NULL
 */
void *wifi_get_bt_cali_data_buf(uint32_t *pul_len)
{
    bfgx_cali_data_stru *pst_bfgx_cali_data_buf = NULL;

    if (g_bfgx_cali_data_buf == NULL) {
        return NULL;
    }

    pst_bfgx_cali_data_buf = (bfgx_cali_data_stru *)g_bfgx_cali_data_buf;

    *pul_len = sizeof(pst_bfgx_cali_data_buf->auc_wifi_cali_for_bt_data);

    ps_print_info("bfgx wifi cali data for bt buf size is %d\n", *pul_len);

    return pst_bfgx_cali_data_buf->auc_wifi_cali_for_bt_data;
}

EXPORT_SYMBOL(wifi_get_bt_cali_data_buf);

/*
 * 函 数 名  : bfgx_get_cali_data_buf
 * 功能描述  : 返回保存bfgx bt校准数据的内存地址
 * 输出参数  : bt 校准数据 buf的长度
 * 返 回 值  : bfgx bt校准buf的地址，也可能是NULL
 */
void *bfgx_get_cali_data_buf(uint32_t *pul_len)
{
    bfgx_cali_data_stru *pst_bfgx_cali_data_buf = NULL;

    if (g_bfgx_cali_data_buf == NULL) {
        return NULL;
    }

    pst_bfgx_cali_data_buf = (bfgx_cali_data_stru *)g_bfgx_cali_data_buf;

    *pul_len = sizeof(pst_bfgx_cali_data_buf->auc_bfgx_data);

    ps_print_info("bfgx bt cali data buf size is %d\n", *pul_len);

    return pst_bfgx_cali_data_buf->auc_bfgx_data;
}

/*
 * 函 数 名  : bfgx_get_cust_ini_data_buf
 * 功能描述  : 返回保存bfgx ini定制化数据的内存地址
 * 输出参数  : bfgx ini定制化数据buf的长度
 * 返 回 值  : bfgx ini数据buf的地址，也可能是NULL
 */
STATIC void *bfgx_get_cust_ini_data_buf(uint32_t *pul_len)
{
    bfgx_cali_data_stru *pst_bfgx_cali_data_buf = NULL;

    if (g_bfgx_cali_data_buf == NULL) {
        return NULL;
    }

    pst_bfgx_cali_data_buf = (bfgx_cali_data_stru *)g_bfgx_cali_data_buf;

    *pul_len = sizeof(pst_bfgx_cali_data_buf->auc_bt_cust_ini_data);

    ps_print_info("bfgx cust ini buf size is %d\n", *pul_len);

    return pst_bfgx_cali_data_buf->auc_bt_cust_ini_data;
}

void plat_bfgx_cali_data_test(void)
{
    bfgx_cali_data_stru *pst_cali_data = NULL;
    uint32_t *p_test = NULL;
    uint32_t count;
    uint32_t i;

    pst_cali_data = (bfgx_cali_data_stru *)bfgx_get_cali_data_buf(&count);
    if (pst_cali_data == NULL) {
        ps_print_err("get_cali_data_buf_addr failed\n");
        return;
    }

    p_test = (uint32_t *)pst_cali_data;
    count = count / sizeof(uint32_t);

    for (i = 0; i < count; i++) {
        p_test[i] = i;
    }

    return;
}

#ifndef BFGX_UART_DOWNLOAD_SUPPORT
STATIC int32_t wifi_cali_buf_malloc(void)
{
    uint8_t *buffer = NULL;
    uint32_t ul_buffer_len;
    int32_t type = get_hi110x_subchip_type();
    if ((type == BOARD_VERSION_SHENKUO) ||
        (type == BOARD_VERSION_BISHENG) ||
        (type == BOARD_VERSION_HI1161)) {
        ul_buffer_len = SHENKUO_MIMO_CALI_DATA_STRU_LEN;
    } else if (type == BOARD_VERSION_HI1105) {
        ul_buffer_len = OAL_MIMO_CALI_DATA_STRU_LEN;
    } else {
        ul_buffer_len = OAL_DOUBLE_CALI_DATA_STRU_LEN;
    }

    if (g_cali_data_buf == NULL) {
        buffer = (uint8_t *)os_kzalloc_gfp(ul_buffer_len);
        if (buffer == NULL) {
            ps_print_err("malloc for g_cali_data_buf fail\n");
            return -ENOMEM;
        }
        g_cali_data_buf = buffer;
        memset_s(g_cali_data_buf, ul_buffer_len, 0, ul_buffer_len);
        ps_print_info("g_cali_data_buf(%p) size %d alloc in sysfs init", g_cali_data_buf, ul_buffer_len);
    } else {
        ps_print_info("g_cali_data_buf size %d alloc in sysfs init\n", ul_buffer_len);
    }

    return SUCC;
}
#endif

STATIC int32_t bfgx_cali_buf_malloc(void)
{
    uint8_t *buffer = NULL;

    if (g_bfgx_cali_data_buf == NULL) {
        buffer = (uint8_t *)os_kzalloc_gfp(BFGX_CALI_DATA_BUF_LEN);
        if (buffer == NULL) {
            os_mem_kfree(g_cali_data_buf);
            g_cali_data_buf = NULL;
            ps_print_err("malloc for g_bfgx_cali_data_buf fail\n");
            return -ENOMEM;
        }
        g_bfgx_cali_data_buf = buffer;
    } else {
        ps_print_info("g_bfgx_cali_data_buf alloc in sysfs init\n");
    }

    return SUCC;
}

/*
 * 函 数 名  : cali_data_buf_malloc
 * 功能描述  : 分配保存校准数据的内存
 * 返 回 值  : 0表示分配成功，-1表示分配失败
 */
int32_t cali_data_buf_malloc(void)
{
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    if (wifi_cali_buf_malloc() < 0) {
        return -ENOMEM;
    }
#endif

    if (bfgx_cali_buf_malloc() < 0) {
        return -ENOMEM;
    }

    return SUCC;
}

/*
 * 函 数 名  : cali_data_buf_free
 * 功能描述  : 释放保存校准数据的内存
 */
void cali_data_buf_free(void)
{
    if (g_cali_data_buf != NULL) {
        os_mem_kfree(g_cali_data_buf);
    }

    if (g_cali_data_buf_dma != NULL) {
#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
        dma_free_coherent(&oal_get_wifi_pcie_dev()->dev,
            SHENKUO_MIMO_CALI_DATA_STRU_LEN, g_cali_data_buf, g_cali_data_buf_phy_addr);
#elif (defined _PRE_PLAT_FEATURE_HI116X_PCIE)
        dma_free_coherent(&(oal_get_default_pcie_handler())->comm_res->pcie_dev->dev,
            SHENKUO_MIMO_CALI_DATA_STRU_LEN, g_cali_data_buf, g_cali_data_buf_phy_addr);
#endif
    }

    g_cali_data_buf = NULL;
    g_cali_data_buf_dma = NULL;
    g_cali_data_buf_phy_addr = 0;
    if (g_bfgx_cali_data_buf != NULL) {
        os_mem_kfree(g_bfgx_cali_data_buf);
    }
    g_bfgx_cali_data_buf = NULL;
}

/*
 * 函 数 名  : wait_bfgx_cali_data
 * 功能描述  : 等待接受device发送的校准数据
 * 返 回 值  : 0表示成功，-1表示失败
 */
STATIC int32_t wait_bfgx_cali_data(void)
{
#define WAIT_BFGX_CALI_DATA_TIME 2000
    unsigned long timeleft;

    timeleft = wait_for_completion_timeout(&g_cali_recv_done, msecs_to_jiffies(WAIT_BFGX_CALI_DATA_TIME));
    if (!timeleft) {
        ps_print_err("wait bfgx cali data timeout\n");
        return -ETIMEDOUT;
    }

    return 0;
}

STATIC bfgx_ini_cmd *bfgx_cust_get_ini(void)
{
    int32_t l_subchip_type = get_hi110x_subchip_type();
    bfgx_ini_cmd *p_bfgx_ini_cmd = NULL;

    switch (l_subchip_type) {
        case BOARD_VERSION_HI1103:
            p_bfgx_ini_cmd = (bfgx_ini_cmd *)&g_bfgx_ini_config_cmd_1103;
            ps_print_dbg("bfgx load ini 1103");
            break;
        case BOARD_VERSION_HI1105:
            p_bfgx_ini_cmd = (bfgx_ini_cmd *)&g_bfgx_ini_config_cmd_1105;
            ps_print_dbg("bfgx load ini 1105");
            break;
        case BOARD_VERSION_SHENKUO:
            p_bfgx_ini_cmd = (bfgx_ini_cmd *)&g_bfgx_ini_config_cmd_shenkuo;
            ps_print_dbg("bfgx load ini shenkuo");
            break;
        case BOARD_VERSION_BISHENG:
            p_bfgx_ini_cmd = (bfgx_ini_cmd *)&g_bfgx_ini_config_cmd_bisheng;
            ps_print_dbg("bfgx load ini bisheng");
            break;
        case BOARD_VERSION_HI1161:
            p_bfgx_ini_cmd = (bfgx_ini_cmd *)&g_bfgx_ini_config_cmd_1161;
            ps_print_dbg("bfgx load ini shenkuo");
            break;
        default:
            ps_print_err("bfgx load ini error: No corresponding chip was found, chiptype %d", l_subchip_type);
            break;
    }
    return p_bfgx_ini_cmd;
}

/*
 * 函 数 名  : bfgx_cust_ini_init
 * 功能描述  : bt校准定制化项初始化
 * 返 回 值  : 0表示成功，-1表示失败
 */
STATIC int32_t bfgx_cust_ini_init(void)
{
    int32_t i;
    int32_t l_ret = INI_FAILED;
    int32_t l_cfg_value = 0;
    int32_t l_ori_val;
    char *pst_buf = NULL;
    uint32_t len;
    bfgx_ini_cmd *p_bfgx_ini_cmd = bfgx_cust_get_ini();
    if (p_bfgx_ini_cmd == NULL) {
        return INI_FAILED;
    }

    for (i = 0; i < (BFGX_BT_CUST_INI_SIZE / 4); i++) { /* 512/4转换为字节 */
        /* 定制化项目列表名称以 null 结尾 */
        if (p_bfgx_ini_cmd[i].name == NULL) {
            ps_print_info("bfgx ini load end count: %d", i);
            break;
        }

        l_ori_val = p_bfgx_ini_cmd[i].init_value;

        /* 获取ini的配置值 */
        l_ret = get_cust_conf_int32(INI_MODU_DEV_BT, p_bfgx_ini_cmd[i].name, &l_cfg_value);
        if (l_ret == INI_FAILED) {
            g_bfgx_cust_ini_data[i] = l_ori_val;
            ps_print_dbg("bfgx read ini file failed cfg_id[%d],default value[%d]!", i, l_ori_val);
            continue;
        }

        g_bfgx_cust_ini_data[i] = l_cfg_value;

        ps_print_info("bfgx ini init [id:%d] [%s] changed from [%d]to[%d]",
                      i, p_bfgx_ini_cmd[i].name, l_ori_val, l_cfg_value);
    }

    pst_buf = bfgx_get_cust_ini_data_buf(&len);
    if (pst_buf == NULL) {
        ps_print_err("get cust ini buf fail!");
        return INI_FAILED;
    }

    memcpy_s(pst_buf, len, g_bfgx_cust_ini_data, sizeof(g_bfgx_cust_ini_data));

    return INI_SUCC;
}

/*
 * 函 数 名  : bfgx_customize_init
 * 功能描述  : bfgx定制化项初始化，读取ini配置文件，读取nv配置项
 * 返 回 值  : 0表示成功，-1表示失败
 */
int32_t bfgx_customize_init(void)
{
    int32_t ret;

    /* 申请用于保存校准数据的buffer */
    ret = cali_data_buf_malloc();
    if (ret != OAL_SUCC) {
        ps_print_err("alloc cali data buf fail\n");
        return INI_FAILED;
    }

    init_completion(&g_cali_recv_done);

    ret = bfgx_cust_ini_init();
    if (ret != OAL_SUCC) {
        ps_print_err("bfgx ini init fail!\n");
        cali_data_buf_free();
        return INI_FAILED;
    }

#ifdef HISI_NVRAM_SUPPORT
    ret = bfgx_nv_data_init();
    if (ret != OAL_SUCC) {
        ps_print_err("bfgx nv data init fail!\n");
        cali_data_buf_free();
        return INI_FAILED;
    }
#endif

    return INI_SUCC;
}

/*
 * 函 数 名  : bfgx_cali_data_init
 * 功能描述  : 开机打开bt，进行bt校准
 * 返 回 值  : 0表示成功，-1表示失败
 */
int32_t bfgx_cali_data_init(void)
{
    int32_t ret = 0;
    static uint32_t cali_flag = 0;
    struct ps_core_s *ps_core_d = NULL;
    struct st_bfgx_data *pst_bfgx_data = NULL;
    struct pm_top* pm_top_data = pm_get_top();

    ps_print_info("%s\n", __func__);

    /* 校准只在开机时执行一次，OAM可能被杀掉重启，所以加标志保护 */
    if (cali_flag != 0) {
        ps_print_info("bfgx cali data has inited\n");
        return 0;
    }

    cali_flag++;

    ps_core_d = ps_get_core_reference(service_get_bus_id(BFGX_BT));
    if (unlikely(ps_core_d == NULL)) {
        ps_print_err("ps_core_d is NULL\n");
        return OAL_FAIL;
    }

    mutex_lock(&(pm_top_data->host_mutex));

    pst_bfgx_data = &ps_core_d->bfgx_info[BFGX_BT];
    if (atomic_read(&pst_bfgx_data->subsys_state) == POWER_STATE_OPEN) {
        ps_print_warning("%s has opened! ignore bfgx cali!\n", service_get_name(BFGX_BT));
        goto open_fail;
    }

    ret = hw_bfgx_open(BFGX_BT);
    if (ret != SUCC) {
        ps_print_err("bfgx cali, open bt fail\n");
        goto open_fail;
    }

    ret = wait_bfgx_cali_data();
    if (ret != SUCC) {
        goto timeout;
    }

    ret = hw_bfgx_close(BFGX_BT);
    if (ret != SUCC) {
        ps_print_err("bfgx cali, clsoe bt fail\n");
        goto close_fail;
    }

    mutex_unlock(&(pm_top_data->host_mutex));

    return ret;

timeout:
    hw_bfgx_close(BFGX_BT);
close_fail:
open_fail:
    mutex_unlock(&(pm_top_data->host_mutex));
    return ret;
}

void bt_txbf_get_wl_cali_data(void)
{
    unsigned stream, channel, level;
    bt_txbf_cali_param_stru *bt_txbf_cali_data = NULL;
    uint32_t cali_data_size;
    wlan_cali_data_para_stru *wl_cali_data = (wlan_cali_data_para_stru *)get_cali_data_buf_addr();

    if (wl_cali_data == NULL || get_hi110x_subchip_type() != BOARD_VERSION_SHENKUO) {
        return;
    }

    bt_txbf_cali_data = (bt_txbf_cali_param_stru *)wifi_get_bt_cali_data_buf(&cali_data_size);
    if (bt_txbf_cali_data == NULL) {
        return;
    }
    ps_print_warning("[BT TxBF]get required Tx calibration result from WLAN.");

    for (stream = 0; stream < BT_TXBF_CALI_STREAMS; stream++) {
        bt_txbf_cali_stream_stru *current_stream = &bt_txbf_cali_data->stream_cali_data[stream];
        wlan_cali_2g_20m_save_stru *wl_cali_2g = &wl_cali_data->rf_cali_data[stream].cali_data_2g_20m;

        for (channel = 0; channel < WLAN_2G_CALI_BAND_NUM; channel++) {
            bt_txbf_cali_channel_stru *current_channel = &current_stream->channel_cali_data[channel];
            wlan_cali_basic_para_stru *wl_cali_band = &wl_cali_2g->cali_data[channel];

            // use WL calibration result of level 0 to compensate
            level = 0;
            current_channel->txdc_comp_i = wl_cali_band->txdc_cali_data.txdc_cmp_i[level];
            current_channel->txdc_comp_q = wl_cali_band->txdc_cali_data.txdc_cmp_q[level];
            memcpy_s(&current_channel->txiq_comp_siso, sizeof(wlan_cali_iq_coef_stru),
                     &wl_cali_band->txiq_cali_data.qmc_data[level].qmc_siso, sizeof(wlan_cali_iq_coef_stru));
        }
    }
}
