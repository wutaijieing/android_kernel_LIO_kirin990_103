

/* 头文件包含 */
#include "hisi_customize_wifi_hi1103.h"
#include "hisi_customize_config_priv_hi1103.h"

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/etherdevice.h>
#endif

#include "oal_hcc_host_if.h"
#include "oal_main.h"
#include "hisi_ini.h"
#include "plat_cali.h"
#include "plat_pm_wlan.h"
#include "plat_firmware.h"
#include "oam_ext_if.h"

#include "wlan_spec.h"
#include "wlan_chip_i.h"
#include "hisi_customize_wifi.h"

#include "mac_hiex.h"

#include "wal_config.h"
#include "wal_linux_ioctl.h"

#include "hmac_auto_adjust_freq.h"
#include "hmac_tx_data.h"
#include "hmac_cali_mgmt.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HISI_CUSTOMIZE_CONFIG_PRIV_HI1103_C
OAL_STATIC wlan_customize_private_stru g_priv_cust_params[WLAN_CFG_PRIV_BUTT] = {{ 0, 0 }}; /* 私有定制化参数数组 */

OAL_STATIC wlan_cfg_cmd g_wifi_config_priv[] = {
    /* 校准开关 */
    { "cali_mask", WLAN_CFG_PRIV_CALI_MASK },
    /*
     * #bit0:开wifi重新校准 bit1:开wifi重新上传 bit2:开机校准 bit3:动态校准调平Debug
     * #bit4:不读取NV区域的数据(1:不读取 0：读取)
     */
    { "cali_data_mask", WLAN_CFG_PRIV_CALI_DATA_MASK },
    { "cali_auto_cali_mask", WLAN_CFG_PRIV_CALI_AUTOCALI_MASK },
    { "bw_max_width", WLAN_CFG_PRIV_BW_MAX_WITH },
    { "ldpc_coding",  WLAN_CFG_PRIV_LDPC_CODING },
    { "rx_stbc",      WLAN_CFG_PRIV_RX_STBC },
    { "tx_stbc",      WLAN_CFG_PRIV_TX_STBC },
    { "su_bfer",      WLAN_CFG_PRIV_SU_BFER },
    { "su_bfee",      WLAN_CFG_PRIV_SU_BFEE },
    { "mu_bfer",      WLAN_CFG_PRIV_MU_BFER },
    { "mu_bfee",      WLAN_CFG_PRIV_MU_BFEE },

    { "11n_txbf", WLAN_CFG_PRIV_11N_TXBF },

    { "1024qam_en", WLAN_CFG_PRIV_1024_QAM },
    /* DBDC */
    { "radio_cap_0",     WLAN_CFG_PRIV_DBDC_RADIO_0 },
    { "radio_cap_1",     WLAN_CFG_PRIV_DBDC_RADIO_1 },
    { "fastscan_switch", WLAN_CFG_PRIV_FASTSCAN_SWITCH },

    /* RSSI天线切换 */
    { "rssi_ant_switch", WLAN_CFG_ANT_SWITCH },

     /* 国家码自学习功能开关 */
    { "countrycode_selfstudy", WLAN_CFG_PRIV_COUNRTYCODE_SELFSTUDY_CFG},

    { "m2s_function_mask_ext", WLAN_CFG_PRIV_M2S_FUNCTION_EXT_MASK},
    { "m2s_function_mask", WLAN_CFG_PRIV_M2S_FUNCTION_MASK },

    { "mcm_custom_function_mask", WLAN_CFG_PRIV_MCM_CUSTOM_FUNCTION_MASK},
    { "mcm_function_mask", WLAN_CFG_PRIV_MCM_FUNCTION_MASK },
    { "linkloss_threshold_fixed", WLAN_CFG_PRRIV_LINKLOSS_THRESHOLD_FIXED },

    { "aput_support_160m", WLAN_CFG_APUT_160M_ENABLE},
    { "radar_isr_forbid", WLAN_CFG_RADAR_ISR_FORBID},

    { "download_rate_limit_pps", WLAN_CFG_PRIV_DOWNLOAD_RATE_LIMIT_PPS },
#ifdef _PRE_WLAN_FEATURE_TXOPPS
    { "txopps_switch", WLAN_CFG_PRIV_TXOPPS_SWITCH },
#endif
    { "over_temper_protect_threshold",   WLAN_CFG_PRIV_OVER_TEMPER_PROTECT_THRESHOLD },
    { "over_temp_pro_enable",            WLAN_CFG_PRIV_OVER_TEMP_PRO_ENABLE },
    { "over_temp_pro_reduce_pwr_enable", WLAN_CFG_PRIV_OVER_TEMP_PRO_REDUCE_PWR_ENABLE },
    { "over_temp_pro_safe_th",           WLAN_CFG_PRIV_OVER_TEMP_PRO_SAFE_TH },
    { "over_temp_pro_over_th",           WLAN_CFG_PRIV_OVER_TEMP_PRO_OVER_TH },
    { "over_temp_pro_pa_off_th",         WLAN_CFG_PRIV_OVER_TEMP_PRO_PA_OFF_TH },

    { "dsss2ofdm_dbb_pwr_bo_val",   WLAN_DSSS2OFDM_DBB_PWR_BO_VAL },
    { "evm_fail_pll_reg_fix",       WLAN_CFG_PRIV_EVM_PLL_REG_FIX },
    { "voe_switch_mask",            WLAN_CFG_PRIV_VOE_SWITCH },
    { "11ax_switch_mask",           WLAN_CFG_PRIV_11AX_SWITCH },
    { "htc_switch_mask",            WLAN_CFG_PRIV_HTC_SWITCH },
    { "multi_bssid_switch_mask",    WLAN_CFG_PRIV_MULTI_BSSID_SWITCH},
    { "ac_priv_mask",               WLAN_CFG_PRIV_AC_SUSPEND},
    { "dyn_bypass_extlna_enable",   WLAN_CFG_PRIV_DYN_BYPASS_EXTLNA },
    { "ctrl_frame_tx_chain",        WLAN_CFG_PRIV_CTRL_FRAME_TX_CHAIN },
    { "upc_cali_code_for_18dBm_c0", WLAN_CFG_PRIV_CTRL_UPC_FOR_18DBM_CO },
    { "upc_cali_code_for_18dBm_c1", WLAN_CFG_PRIV_CTRL_UPC_FOR_18DBM_C1 },
    { "11b_double_chain_bo_pow",    WLAN_CFG_PRIV_CTRL_11B_DOUBLE_CHAIN_BO_POW },
    { "hcc_flowctrl_type",          WLAN_CFG_PRIV_HCC_FLOWCTRL_TYPE },
    { "lock_cpu_freq",              WLAN_CFG_PRIV_LOCK_CPU_FREQ },
    { "mbo_switch_mask",            WLAN_CFG_PRIV_MBO_SWITCH},
    { "dynamic_dbac_adjust_mask",   WLAN_CFG_PRIV_DYNAMIC_DBAC_SWITCH},
    { "dc_flowctl_switch",          WLAN_CFG_PRIV_DC_FLOWCTL_SWITCH },
    { "phy_cap_mask",               WLAN_CFG_PRIV_PHY_CAP_SWITCH},
    { "hal_ps_rssi_param",          WLAN_CFG_PRIV_HAL_PS_RSSI_PARAM},
    { "hal_ps_pps_param",           WLAN_CFG_PRIV_HAL_PS_PPS_PARAM},
    { "optimized_feature_mask",     WLAN_CFG_PRIV_OPTIMIZED_FEATURE_SWITCH},
    { "ddr_switch_mask",            WLAN_CFG_PRIV_DDR_SWITCH},

    { "fem_backoff_pow",              WLAN_CFG_PRIV_FEM_DELT_POW},
    { "tpc_adj_pow_start_idx_by_fem", WLAN_CFG_PRIV_FEM_ADJ_TPC_TBL_START_IDX},
    { "hiex_cap",                     WLAN_CFG_PRIV_HIEX_CAP},
    { "ftm_cap",                      WLAN_CFG_PRIV_FTM_CAP},
    { "wlan_feature_miracast_sink",   WLAN_CFG_PRIV_MIRACAST_SINK },
    { "disable_w58_channel",          WLAN_CFG_PRIV_DISABLE_W58_CHANNEL },
    { "wlan_go_assoc_user_max_num", WLAN_CFG_PRIV_P2P_GO_ASSOC_USER_MAX_NUM },
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
    { "mcast_ampdu_enable",           WLAN_CFG_PRIV_MCAST_AMPDU_ENABLE},
#endif
    { "pt_mcast_enable",             WLAN_CFG_PRIV_PT_MCAST_ENABLE},
    { "close_filter_switch",         WLAN_CFG_PRIV_CLOSE_FILTER_SWITCH},
    { NULL,                 0 }
};

/*
 * 函 数 名  : hwifi_config_init_private_custom
 * 功能描述  : 初始化私有定制全局变量数组
 */
int32_t hwifi_config_init_private_custom(void)
{
    int32_t l_cfg_id;
    int32_t l_ret;

    for (l_cfg_id = 0; l_cfg_id < WLAN_CFG_PRIV_BUTT; l_cfg_id++) {
        /* 获取 private 的配置值 */
        l_ret = get_cust_conf_int32(INI_MODU_WIFI, g_wifi_config_priv[l_cfg_id].name,
                                    &(g_priv_cust_params[l_cfg_id].l_val));
        if (l_ret == INI_FAILED) {
            g_priv_cust_params[l_cfg_id].en_value_state = OAL_FALSE;
            continue;
        }
        g_priv_cust_params[l_cfg_id].en_value_state = OAL_TRUE;
    }

    oam_warning_log0(0, OAM_SF_CFG, "hwifi_config_init_private_custom read from ini success!");

    return INI_SUCC;
}

int32_t hwifi_get_init_priv_value(int32_t l_cfg_id, int32_t *pl_priv_value)
{
    if (l_cfg_id < 0 || l_cfg_id >= WLAN_CFG_PRIV_BUTT) {
        oam_error_log2(0, OAM_SF_ANY, "hwifi_get_init_priv_value cfg id[%d] out of range, max[%d]",
                       l_cfg_id, WLAN_CFG_PRIV_BUTT - 1);
        return OAL_FAIL;
    }

    if (g_priv_cust_params[l_cfg_id].en_value_state == OAL_FALSE) {
        return OAL_FAIL;
    }

    *pl_priv_value = g_priv_cust_params[l_cfg_id].l_val;

    return OAL_SUCC;
}

OAL_STATIC OAL_INLINE void hwifi_custom_adapt_hcc_flowctrl_type(hmac_to_dmac_cfg_custom_data_stru *pst_syn_msg,
                                                                uint8_t *puc_priv_cfg_value)
{
    if (hcc_bus_flowctrl_init(*puc_priv_cfg_value) != OAL_SUCC) {
        /* GPIO流控中断注册失败，强制device使用SDIO流控(type = 0) */
        *puc_priv_cfg_value = 0;
    }
    pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_HCC_FLOWCTRL_TYPE_ID;
    oal_io_print("hwifi_custom_adapt_mac_device_priv_ini_param::sdio_flow_ctl_type[0x%x].\r\n",
                 *puc_priv_cfg_value);
}

OAL_STATIC void hwifi_custom_adapt_priv_ini_param_extend_etc(
    hmac_to_dmac_cfg_custom_data_stru *pst_syn_msg, wlan_cfg_priv_id_uint8 uc_cfg_id)
{
    switch (uc_cfg_id) {
        case WLAN_CFG_PRIV_PHY_CAP_SWITCH:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_PHY_CAP_SWITCH_ID;
            break;
        case WLAN_CFG_PRIV_OPTIMIZED_FEATURE_SWITCH:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_OPTIMIZED_FEATURE_SWITCH_ID;
            break;
        case WLAN_CFG_PRIV_DDR_SWITCH:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_DDR_SWITCH_ID;
            break;
        case WLAN_CFG_PRIV_FTM_CAP:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_FTM_CAP_ID;
            break;
        case WLAN_CFG_PRIV_CLOSE_FILTER_SWITCH:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_CLOSE_FILTER_SWITCH_ID;
            break;
        default:
            break;
    }
}

OAL_STATIC void hwifi_custom_adapt_priv_ini_param_extend(hmac_to_dmac_cfg_custom_data_stru *pst_syn_msg,
                                                         wlan_cfg_priv_id_uint8 uc_cfg_id,
                                                         uint8_t *puc_priv_cfg_value)
{
    switch (uc_cfg_id) {
        case WLAN_CFG_PRIV_DYN_BYPASS_EXTLNA:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_DYN_BYPASS_EXTLNA_ID;
            break;
        case WLAN_CFG_PRIV_CTRL_FRAME_TX_CHAIN:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_CTRL_FRAME_TX_CHAIN_ID;
            break;
        case WLAN_CFG_PRIV_CTRL_UPC_FOR_18DBM_CO:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_CTRL_UPC_FOR_18DBM_C0_ID;
            break;
        case WLAN_CFG_PRIV_CTRL_UPC_FOR_18DBM_C1:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_CTRL_UPC_FOR_18DBM_C1_ID;
            break;
        case WLAN_CFG_PRIV_CTRL_11B_DOUBLE_CHAIN_BO_POW:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_CTRL_11B_DOUBLE_CHAIN_BO_POW_ID;
            break;
        case WLAN_CFG_RADAR_ISR_FORBID:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_RADAR_ISR_FORBID_ID;
            break;
        case WLAN_CFG_PRIV_HCC_FLOWCTRL_TYPE:
            hwifi_custom_adapt_hcc_flowctrl_type(pst_syn_msg, puc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_MBO_SWITCH:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_MBO_SWITCH_ID;
            break;
        case WLAN_CFG_PRIV_DYNAMIC_DBAC_SWITCH:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_DYNAMIC_DBAC_ID;
            break;
        case WLAN_CFG_PRIV_DC_FLOWCTL_SWITCH:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_DC_FLOWCTRL_ID;
            break;
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
        case WLAN_CFG_PRIV_MCAST_AMPDU_ENABLE:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_MCAST_AMPDU_ENABLE_ID;
            break;
#endif
        case WLAN_CFG_PRIV_PT_MCAST_ENABLE:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_PT_MCAST_ENABLE_ID;
            break;
        case WLAN_CFG_PRIV_MIRACAST_SINK:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_MIRACAST_SINK;
            break;

        default:
            break;
    }
    oal_io_print("hwifi_custom_adapt_priv_ini_param_extend::syn_id[%d] val[%d].\r\n",
        pst_syn_msg->en_syn_id, *puc_priv_cfg_value);
    hwifi_custom_adapt_priv_ini_param_extend_etc(pst_syn_msg, uc_cfg_id);
}

OAL_STATIC void hwifi_custom_adapt_priv_11ax_feature_ini_param(hmac_to_dmac_cfg_custom_data_stru *pst_syn_msg,
                                                               wlan_cfg_priv_id_uint8 uc_cfg_id,
                                                               uint8_t uc_priv_cfg_value)
{
    switch (uc_cfg_id) {
        case WLAN_CFG_PRIV_VOE_SWITCH:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_VOE_SWITCH_ID;
            oal_io_print("hwifi_custom_adapt_mac_device_priv_ini_param::voe switch[%d].\r\n", uc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_11AX_SWITCH:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_11AX_SWITCH_ID;
            break;
        case WLAN_CFG_PRIV_HTC_SWITCH:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_HTC_SWITCH_ID;
            break;
        case WLAN_CFG_PRIV_MULTI_BSSID_SWITCH:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_MBSSID_SWITCH_ID;
            break;
        case WLAN_CFG_PRIV_AC_SUSPEND:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_AC_SUSPEND_ID;
            break;
        case WLAN_CFG_PRIV_DYNAMIC_DBAC_SWITCH:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_DYNAMIC_DBAC_ID;
            break;
        default:
            break;
    }
}

OAL_STATIC void hwifi_custom_adapt_priv_feature_ini_param(hmac_to_dmac_cfg_custom_data_stru *pst_syn_msg,
                                                          wlan_cfg_priv_id_uint8 uc_cfg_id, uint8_t *puc_priv_cfg_value)
{
    switch (uc_cfg_id) {
        case WLAN_CFG_PRIV_COUNRTYCODE_SELFSTUDY_CFG:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_COUNTRYCODE_SELFSTUDY_CFG_ID;
            oal_io_print("hwifi_custom_adapt_mac_device_priv_ini_param::country code self study[%d].\r\n",
                         *puc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_M2S_FUNCTION_MASK:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_M2S_FUNCTION_MASK_ID;
            oal_io_print("hwifi_custom_adapt_mac_device_priv_ini_param::m2s_mask[0x%x].\r\n", *puc_priv_cfg_value);
            break;

        case WLAN_CFG_PRIV_M2S_FUNCTION_EXT_MASK:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_M2S_FUNCTION_EXT_MASK_ID;
            oal_io_print("hwifi_custom_adapt_mac_device_priv_ini_param::m2s_mask_ext[0x%x].\r\n", *puc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_MCM_FUNCTION_MASK:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_MCM_FUNCTION_MASK_ID;
            oal_io_print("hwifi_custom_adapt_mac_device_priv_ini_param::mcm_msk[0x%x].\r\n", *puc_priv_cfg_value);
            break;

        case WLAN_CFG_PRIV_MCM_CUSTOM_FUNCTION_MASK:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_MCM_CUSTOM_FUNCTION_MASK_ID;
            oal_io_print("hwifi_custom_adapt_mac_device_priv_ini_param::mcm_custom[0x%x].\r\n", *puc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_FASTSCAN_SWITCH:
            pst_syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_FASTSCAN_SWITCH_ID;
            oal_io_print("hwifi_custom_adapt_mac_device_priv_ini_param::fastcan [0x%x].\r\n", *puc_priv_cfg_value);
            break;
        default:
            break;
    }
}

/*
 * 函 数 名  : hwifi_custom_adapt_priv_ini_param_extend
 * 功能描述  : 解圈复杂度拆分函数
 */
OAL_STATIC void hwifi_custom_adapt_priv_ini_extend(hmac_to_dmac_cfg_custom_data_stru *pst_syn_msg,
    wlan_cfg_priv_id_uint8 uc_cfg_id, uint8_t *puc_priv_cfg_value)
{
    switch (uc_cfg_id) {
        case WLAN_CFG_PRIV_DYN_BYPASS_EXTLNA:
        case WLAN_CFG_PRIV_CTRL_FRAME_TX_CHAIN:
        case WLAN_CFG_PRIV_CTRL_UPC_FOR_18DBM_CO:
        case WLAN_CFG_PRIV_CTRL_UPC_FOR_18DBM_C1:
        case WLAN_CFG_PRIV_CTRL_11B_DOUBLE_CHAIN_BO_POW:
        case WLAN_CFG_RADAR_ISR_FORBID:
        case WLAN_CFG_PRIV_HCC_FLOWCTRL_TYPE:
        case WLAN_CFG_PRIV_MBO_SWITCH:
        case WLAN_CFG_PRIV_DC_FLOWCTL_SWITCH:
        case WLAN_CFG_PRIV_PHY_CAP_SWITCH:
        case WLAN_CFG_PRIV_OPTIMIZED_FEATURE_SWITCH:
        case WLAN_CFG_PRIV_DDR_SWITCH:
        case WLAN_CFG_PRIV_CLOSE_FILTER_SWITCH:
        case WLAN_CFG_PRIV_FTM_CAP:
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
        case WLAN_CFG_PRIV_MCAST_AMPDU_ENABLE:
#endif
        case WLAN_CFG_PRIV_PT_MCAST_ENABLE:
        case WLAN_CFG_PRIV_MIRACAST_SINK:
            hwifi_custom_adapt_priv_ini_param_extend(pst_syn_msg, uc_cfg_id, puc_priv_cfg_value);
            break;
        default:
            break;
    }
}
static void hwifi_priv_ini_param_set_ldpc_codeing_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                                     uint8_t cfg_id,
                                                     uint8_t *priv_cfg_value)
{
    syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_LDPC_CODING_ID;
    oal_io_print("hwifi_custom_adapt_priv_ini_param::ldpc coding[%d].\r\n", *priv_cfg_value);
}

static void hwifi_priv_ini_param_set_bw_max_with_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                                    uint8_t cfg_id,
                                                    uint8_t *priv_cfg_value)
{
    syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_BW_MAX_WITH_ID;
    oal_io_print("hwifi_custom_adapt_priv_ini_param::max_bw[%d].\r\n", *priv_cfg_value);
}

static void hwifi_priv_ini_param_set_rx_stbc_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                                uint8_t cfg_id,
                                                uint8_t *priv_cfg_value)
{
    syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_RX_STBC_ID;
    oal_io_print("hwifi_custom_adapt_priv_ini_param::rx_stbc[%d].\r\n", *priv_cfg_value);
}

static void hwifi_priv_ini_param_set_tx_stbc_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                                uint8_t cfg_id,
                                                uint8_t *priv_cfg_value)
{
    syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_TX_STBC_ID;
    oal_io_print("hwifi_custom_adapt_priv_ini_param::tx_stbc[%d].\r\n", *priv_cfg_value);
}

static void hwifi_priv_ini_param_set_su_bfer_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                                uint8_t cfg_id,
                                                uint8_t *priv_cfg_value)
{
    syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_SU_BFER_ID;
    oal_io_print("hwifi_custom_adapt_priv_ini_param::su bfer[%d].\r\n", *priv_cfg_value);
}

#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
static void hwifi_priv_ini_param_set_mcast_ampdu_enable_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                                           uint8_t cfg_id,
                                                           uint8_t *priv_cfg_value)
{
    syn_msg->en_syn_id = WLAN_CFG_PRIV_MCAST_AMPDU_ENABLE;
    oal_io_print("hwifi_priv_ini_param_set_mcast_ampdu_enable_id::mcast_ampdu_enable[%d]", *priv_cfg_value);
}
#endif

static void hwifi_priv_ini_param_set_pt_mcast_enable_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
    uint8_t cfg_id, uint8_t *priv_cfg_value)
{
    syn_msg->en_syn_id = WLAN_CFG_PRIV_PT_MCAST_ENABLE;
    oal_io_print("hwifi_priv_ini_param_set_pt_mcast_enable_id::pt_mcast_enable[%d]", *priv_cfg_value);
}

static void hwifi_priv_ini_param_set_su_bfee_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                                uint8_t cfg_id,
                                                uint8_t *priv_cfg_value)
{
    syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_SU_BFEE_ID;
    oal_io_print("hwifi_custom_adapt_priv_ini_param::su bfee[%d].\r\n", *priv_cfg_value);
}

static void hwifi_priv_ini_param_set_mu_bfer_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                                uint8_t cfg_id,
                                                uint8_t *priv_cfg_value)
{
    syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_MU_BFER_ID;
    oal_io_print("hwifi_custom_adapt_priv_ini_param::mu bfer[%d].\r\n", *priv_cfg_value);
}

static void hwifi_priv_ini_param_set_mu_bfee_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                                uint8_t cfg_id,
                                                uint8_t *priv_cfg_value)
{
    syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_MU_BFEE_ID;
    oal_io_print("hwifi_custom_adapt_priv_ini_param::mu bfee[%d].\r\n", *priv_cfg_value);
}

static void hwifi_priv_ini_param_set_11n_txbf_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                                 uint8_t cfg_id,
                                                 uint8_t *priv_cfg_value)
{
    syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_11N_TXBF_ID;
    oal_io_print("hwifi_custom_adapt_priv_ini_param::11n txbf[%d].\r\n", *priv_cfg_value);
}

static void hwifi_priv_ini_param_set_1024_qam_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                                 uint8_t cfg_id,
                                                 uint8_t *priv_cfg_value)
{
    syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_1024_QAM_ID;
    oal_io_print("hwifi_custom_adapt_priv_ini_param::1024qam[%d].\r\n", *priv_cfg_value);
}

static void hwifi_priv_ini_param_set_cali_data_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                                  uint8_t cfg_id,
                                                  uint8_t *priv_cfg_value)
{
    /* 开机默认打开校准数据上传下发 */
    *priv_cfg_value = hwifi_custom_cali_ini_param(*priv_cfg_value);
    syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_CALI_DATA_MASK_ID;
    oal_io_print("hwifi_custom_adapt_priv_ini_param::g_wlan_open_cnt[%d] \
        priv_cali_data_up_down[0x%x].", g_wlan_open_cnt, *priv_cfg_value);
}

static void hwifi_priv_ini_param_set_autocali_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                                 uint8_t cfg_id,
                                                 uint8_t *priv_cfg_value)
{
    /* 开机默认不打开开机校准 */
    *priv_cfg_value = (g_custom_cali_done == OAL_FALSE) ? OAL_FALSE : *priv_cfg_value;
    syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_AUTOCALI_MASK_ID;
    oal_io_print("hwifi_custom_adapt_priv_ini_param::g_custom_cali_done[%d]\
        auto_cali_mask[0x%x].\r\n", g_custom_cali_done, *priv_cfg_value);
}

static void hwifi_priv_ini_param_set_priv_feature_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                                     uint8_t cfg_id,
                                                     uint8_t *priv_cfg_value)
{
    hwifi_custom_adapt_priv_feature_ini_param(syn_msg, cfg_id, priv_cfg_value);
}

static void hwifi_priv_ini_param_set_ant_switch_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                                   uint8_t cfg_id,
                                                   uint8_t *priv_cfg_value)
{
    syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_ANT_SWITCH_ID;
    oal_io_print("hwifi_custom_adapt_priv_ini_param::ant switch[%d].\r\n", *priv_cfg_value);
}

static void hwifi_priv_ini_param_set_linkloss_threshold_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                                           uint8_t cfg_id,
                                                           uint8_t *priv_cfg_value)
{
    syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_LINKLOSS_THRESHOLD_FIXED_ID;
    oal_io_print("hwifi_custom_adapt_priv_ini_param::linkloss threshold fixed[%d].\r\n",
                 *priv_cfg_value);
}

static void hwifi_priv_ini_param_set_txopps_switch_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                                      uint8_t cfg_id,
                                                      uint8_t *priv_cfg_value)
{
#ifdef _PRE_WLAN_FEATURE_TXOPPS
    syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_TXOPPS_SWITCH_ID;
    oal_io_print("hwifi_custom_adapt_priv_ini_param::uc_priv_cfg_value[0x%x].\r\n", *priv_cfg_value);
#endif
}

static void hwifi_priv_ini_param_set_pro_enable_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                                   uint8_t cfg_id,
                                                   uint8_t *priv_cfg_value)
{
    syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_TEMP_PRO_ENABLE_ID;
    oal_io_print("hwifi_custom_adapt_priv_ini_param::temp pro enable[%d].\r\n", *priv_cfg_value);
}

static void hwifi_priv_ini_param_set_pro_reduce_enable_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                                          uint8_t cfg_id,
                                                          uint8_t *priv_cfg_value)
{
    syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_TEMP_PRO_REDUCE_PWR_ENABLE_ID;
    oal_io_print("hwifi_custom_adapt_priv_ini_param::temp pro reduce pwr enable[%d].\r\n",
                 *priv_cfg_value);
}
static void hwifi_priv_ini_param_set_pro_safe_th_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                                    uint8_t cfg_id,
                                                    uint8_t *priv_cfg_value)
{
    syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_TEMP_PRO_SAFE_TH_ID;
    oal_io_print("hwifi_custom_adapt_priv_ini_param::temp pro safe th[%d].\r\n", *priv_cfg_value);
}
static void hwifi_priv_ini_param_set_pro_over_th_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                                    uint8_t cfg_id,
                                                    uint8_t *priv_cfg_value)
{
    syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_TEMP_PRO_OVER_TH_ID;
    oal_io_print("hwifi_custom_adapt_priv_ini_param::temp pro over th[%d].\r\n", *priv_cfg_value);
}
static void hwifi_priv_ini_param_set_pro_pa_off_th_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                                      uint8_t cfg_id,
                                                      uint8_t *priv_cfg_value)
{
    syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_TEMP_PRO_PA_OFF_TH_ID;
    oal_io_print("hwifi_custom_adapt_priv_ini_param::temp pro pa off th[%d].\r\n", *priv_cfg_value);
}
static void hwifi_priv_ini_param_set_evm_pll_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                                uint8_t cfg_id,
                                                uint8_t *priv_cfg_value)
{
    syn_msg->en_syn_id = CUSTOM_CFGID_PRIV_INI_EVM_PLL_REG_FIX_ID;
    oal_io_print("hwifi_custom_adapt_priv_ini_param::temp pro safe th[%d].\r\n", *priv_cfg_value);
}
static void hwifi_priv_ini_param_set_11ax_feature_id(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                                     uint8_t cfg_id,
                                                     uint8_t *priv_cfg_value)
{
    hwifi_custom_adapt_priv_11ax_feature_ini_param(syn_msg, cfg_id, *priv_cfg_value);
}

typedef struct {
    uint8_t cfg_id;
    void (*hwifi_priv_ini_param_set_id)(hmac_to_dmac_cfg_custom_data_stru *syn_msg,
                                        uint8_t cfg_id,
                                        uint8_t *priv_cfg_value);
} hwifi_priv_ini_param_ops;

const hwifi_priv_ini_param_ops g_hwifi_ini_param_ops_tbl[] = {
    { WLAN_CFG_PRIV_LDPC_CODING,                     hwifi_priv_ini_param_set_ldpc_codeing_id },
    { WLAN_CFG_PRIV_BW_MAX_WITH,                     hwifi_priv_ini_param_set_bw_max_with_id },
    { WLAN_CFG_PRIV_RX_STBC,                         hwifi_priv_ini_param_set_rx_stbc_id },
    { WLAN_CFG_PRIV_TX_STBC,                         hwifi_priv_ini_param_set_tx_stbc_id },
    { WLAN_CFG_PRIV_SU_BFER,                         hwifi_priv_ini_param_set_su_bfer_id },
    { WLAN_CFG_PRIV_SU_BFEE,                         hwifi_priv_ini_param_set_su_bfee_id },
    { WLAN_CFG_PRIV_MU_BFER,                         hwifi_priv_ini_param_set_mu_bfer_id },
    { WLAN_CFG_PRIV_MU_BFEE,                         hwifi_priv_ini_param_set_mu_bfee_id },
    { WLAN_CFG_PRIV_11N_TXBF,                        hwifi_priv_ini_param_set_11n_txbf_id },
    { WLAN_CFG_PRIV_1024_QAM,                        hwifi_priv_ini_param_set_1024_qam_id },
    { WLAN_CFG_PRIV_CALI_DATA_MASK,                  hwifi_priv_ini_param_set_cali_data_id },
    { WLAN_CFG_PRIV_CALI_AUTOCALI_MASK,              hwifi_priv_ini_param_set_autocali_id },
    { WLAN_CFG_PRIV_M2S_FUNCTION_MASK,               hwifi_priv_ini_param_set_priv_feature_id },
    { WLAN_CFG_PRIV_M2S_FUNCTION_EXT_MASK,           hwifi_priv_ini_param_set_priv_feature_id },
    { WLAN_CFG_PRIV_MCM_FUNCTION_MASK,               hwifi_priv_ini_param_set_priv_feature_id },
    { WLAN_CFG_PRIV_MCM_CUSTOM_FUNCTION_MASK,        hwifi_priv_ini_param_set_priv_feature_id },
    { WLAN_CFG_PRIV_COUNRTYCODE_SELFSTUDY_CFG,       hwifi_priv_ini_param_set_priv_feature_id },
    { WLAN_CFG_PRIV_FASTSCAN_SWITCH,                 hwifi_priv_ini_param_set_priv_feature_id },
    { WLAN_CFG_ANT_SWITCH,                           hwifi_priv_ini_param_set_ant_switch_id },
    { WLAN_CFG_PRRIV_LINKLOSS_THRESHOLD_FIXED,       hwifi_priv_ini_param_set_linkloss_threshold_id },
    { WLAN_CFG_PRIV_TXOPPS_SWITCH,                   hwifi_priv_ini_param_set_txopps_switch_id },
    { WLAN_CFG_PRIV_OVER_TEMP_PRO_ENABLE,            hwifi_priv_ini_param_set_pro_enable_id },
    { WLAN_CFG_PRIV_OVER_TEMP_PRO_REDUCE_PWR_ENABLE, hwifi_priv_ini_param_set_pro_reduce_enable_id },
    { WLAN_CFG_PRIV_OVER_TEMP_PRO_SAFE_TH,           hwifi_priv_ini_param_set_pro_safe_th_id },
    { WLAN_CFG_PRIV_OVER_TEMP_PRO_OVER_TH,           hwifi_priv_ini_param_set_pro_over_th_id },
    { WLAN_CFG_PRIV_OVER_TEMP_PRO_PA_OFF_TH,         hwifi_priv_ini_param_set_pro_pa_off_th_id },
    { WLAN_CFG_PRIV_EVM_PLL_REG_FIX,                 hwifi_priv_ini_param_set_evm_pll_id },
    { WLAN_CFG_PRIV_VOE_SWITCH,                      hwifi_priv_ini_param_set_11ax_feature_id },
    { WLAN_CFG_PRIV_11AX_SWITCH,                     hwifi_priv_ini_param_set_11ax_feature_id },
    { WLAN_CFG_PRIV_HTC_SWITCH,                      hwifi_priv_ini_param_set_11ax_feature_id },
    { WLAN_CFG_PRIV_MULTI_BSSID_SWITCH,              hwifi_priv_ini_param_set_11ax_feature_id },
    { WLAN_CFG_PRIV_AC_SUSPEND,                      hwifi_priv_ini_param_set_11ax_feature_id },
    { WLAN_CFG_PRIV_DYNAMIC_DBAC_SWITCH,             hwifi_priv_ini_param_set_11ax_feature_id },
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
    { WLAN_CFG_PRIV_MCAST_AMPDU_ENABLE,              hwifi_priv_ini_param_set_mcast_ampdu_enable_id},
#endif
    { WLAN_CFG_PRIV_PT_MCAST_ENABLE,                 hwifi_priv_ini_param_set_pt_mcast_enable_id},
};

/*
 * 函 数 名  : hwifi_custom_adapt_priv_ini_param
 * 功能描述  : 下发私有开机device配置定制化项到device
 */
OAL_STATIC int32_t hwifi_custom_adapt_priv_ini_param(uint8_t uc_cfg_id, uint8_t *puc_data, uint32_t *pul_len)
{
    int32_t l_ret;
    hmac_to_dmac_cfg_custom_data_stru st_syn_msg = {0};
    int32_t l_priv_val = 0;
    uint8_t uc_priv_cfg_value;
    uint8_t idx;

    if (puc_data == NULL) {
        oam_error_log1(0, OAM_SF_CFG, "{hwifi_custom_adapt_priv_ini_param::puc_data NULL data_len[%d]}", *pul_len);
        return OAL_FAIL;
    }

    l_ret = hwifi_get_init_priv_value(uc_cfg_id, &l_priv_val);
    if (l_ret != OAL_SUCC) {
        oam_warning_log1(0, OAM_SF_CFG, "hwifi_custom_adapt_priv_ini_param::get_init_priv fail cfg_id[%d]", uc_cfg_id);
        return OAL_FAIL;
    }

    uc_priv_cfg_value = (uint8_t)(uint32_t)l_priv_val;

    for (idx = 0; idx < (sizeof(g_hwifi_ini_param_ops_tbl) / sizeof(hwifi_priv_ini_param_ops)); ++idx) {
        if (g_hwifi_ini_param_ops_tbl[idx].cfg_id == uc_cfg_id) {
            g_hwifi_ini_param_ops_tbl[idx].hwifi_priv_ini_param_set_id(&st_syn_msg, uc_cfg_id, &uc_priv_cfg_value);
            break;
        }
    }

    hwifi_custom_adapt_priv_ini_extend(&st_syn_msg, uc_cfg_id, &uc_priv_cfg_value);

    st_syn_msg.len = sizeof(uc_priv_cfg_value);
    l_ret = memcpy_s(puc_data, (WLAN_LARGE_NETBUF_SIZE - *pul_len), &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    l_ret += memcpy_s(puc_data + CUSTOM_MSG_DATA_HDR_LEN, (WLAN_LARGE_NETBUF_SIZE - *pul_len -
        CUSTOM_MSG_DATA_HDR_LEN), &uc_priv_cfg_value, sizeof(uc_priv_cfg_value));
    *pul_len += (sizeof(uc_priv_cfg_value) + CUSTOM_MSG_DATA_HDR_LEN);

    if (l_ret != EOK) {
        oam_error_log2(0, OAM_SF_CFG,
            "{hwifi_custom_adapt_priv_ini_param::memcpy_s fail[%d]. data_len[%d]}", l_ret, *pul_len);
        return OAL_FAIL;
    }
    return OAL_SUCC;
}
/*
 * 函 数 名  : hwifi_custom_adapt_device_priv_ini_temper_thread_param
 * 功能描述  : 下发过温保护配置定制化项到device
 */
OAL_STATIC int32_t hwifi_custom_adapt_device_priv_ini_temper_thread_param(uint8_t *puc_data,
                                                                          uint32_t *pul_data_len)
{
    int32_t l_ret;
    hmac_to_dmac_cfg_custom_data_stru st_syn_msg = {0};
    int32_t l_priv_val = 0;
    uint32_t over_temp_protect_thread;

    if (puc_data == NULL) {
        oam_error_log1(0, OAM_SF_CFG,
            "{hwifi_custom_adapt_device_priv_ini_temper_thread_param::puc_data is NULL data_len[%d].}", *pul_data_len);
        return OAL_FAIL;
    }

    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_OVER_TEMPER_PROTECT_THRESHOLD, &l_priv_val);
    if (l_ret == OAL_SUCC) {
        over_temp_protect_thread = (uint32_t)l_priv_val;
        oal_io_print("hwifi_custom_adapt_device_priv_ini_temper_thread_param::read over_temp_protect_thread[%d]\r\n",
                     over_temp_protect_thread);
    } else {
        return OAL_FAIL;
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_OVER_TEMPER_PROTECT_THRESHOLD_ID;
    st_syn_msg.len = sizeof(over_temp_protect_thread);

    l_ret = memcpy_s(puc_data, (WLAN_LARGE_NETBUF_SIZE - *pul_data_len), &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    l_ret += memcpy_s(puc_data + CUSTOM_MSG_DATA_HDR_LEN,
                      (WLAN_LARGE_NETBUF_SIZE - *pul_data_len - CUSTOM_MSG_DATA_HDR_LEN),
                      &over_temp_protect_thread,
                      sizeof(over_temp_protect_thread));
    if (l_ret != EOK) {
        oam_error_log2(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_priv_ini_temper_thread_param::memcpy_s fail[%d]. \
            data_len[%d]}", l_ret, *pul_data_len);
        *pul_data_len += (sizeof(over_temp_protect_thread) + CUSTOM_MSG_DATA_HDR_LEN);
        return OAL_FAIL;
    }

    *pul_data_len += (sizeof(over_temp_protect_thread) + CUSTOM_MSG_DATA_HDR_LEN);

    oam_warning_log2(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_priv_ini_temper_thread_param::da_len[%d] \
        over_temp_protect_thread[0x%x].}", *pul_data_len, over_temp_protect_thread);

    return OAL_SUCC;
}
/*
 * 函 数 名  : hwifi_custom_adapt_device_priv_ini_dsss2ofdm_dbb_pwr_bo_param
 * 功能描述  : 下发私有定制11b的回退功率值到device
 */
OAL_STATIC int32_t hwifi_custom_adapt_device_priv_ini_dsss2ofdm_dbb_pwr_bo_param(uint8_t *puc_data,
                                                                                 uint32_t *pul_data_len)
{
    int32_t l_ret;
    hmac_to_dmac_cfg_custom_data_stru st_syn_msg = {0};
    int32_t l_priv_val = 0;
    int16_t l_dsss2ofdm_dbb_pwr_bo;

    if (puc_data == NULL) {
        oam_error_log1(0, OAM_SF_CFG,
            "{hwifi_custom_adapt_device_priv_ini_dsss2ofdm_dbb_pwr_bo_param::puc_data is NULL data_len[%d].}",
            *pul_data_len);
        return OAL_FAIL;
    }

    l_ret = hwifi_get_init_priv_value(WLAN_DSSS2OFDM_DBB_PWR_BO_VAL, &l_priv_val);
    if (l_ret != OAL_SUCC) {
        return OAL_FAIL;
    }

    l_dsss2ofdm_dbb_pwr_bo = (int16_t)l_priv_val;
    st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_DSSS2OFDM_DBB_PWR_BO_VAL_ID;
    st_syn_msg.len = sizeof(l_dsss2ofdm_dbb_pwr_bo);

    l_ret = memcpy_s(puc_data, (WLAN_LARGE_NETBUF_SIZE - *pul_data_len), &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    l_ret += memcpy_s(puc_data + CUSTOM_MSG_DATA_HDR_LEN,
                      (WLAN_LARGE_NETBUF_SIZE - *pul_data_len - CUSTOM_MSG_DATA_HDR_LEN),
                      &l_dsss2ofdm_dbb_pwr_bo,
                      sizeof(l_dsss2ofdm_dbb_pwr_bo));
    if (l_ret != EOK) {
        oam_error_log2(0, OAM_SF_CFG,
            "{hwifi_custom_adapt_device_priv_ini_dsss2ofdm_dbb_pwr_bo_param::memcpy_s fail[%d]. data_len[%d]}",
            l_ret, *pul_data_len);
        *pul_data_len += (sizeof(l_dsss2ofdm_dbb_pwr_bo) + CUSTOM_MSG_DATA_HDR_LEN);
        return OAL_FAIL;
    }

    *pul_data_len += (sizeof(l_dsss2ofdm_dbb_pwr_bo) + CUSTOM_MSG_DATA_HDR_LEN);

    oam_warning_log2(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_priv_ini_dsss2ofdm_dbb_pwr_bo_param::da_len[%d] \
        l_dsss2ofdm_dbb_pwr_bo[0x%x].}", *pul_data_len, l_dsss2ofdm_dbb_pwr_bo);
    return OAL_SUCC;
}
#ifdef _PRE_WLAN_RX_LISTEN_POWER_SAVING
/*
 * 函 数 名  : hwifi_custom_adapt_device_priv_ini_download_pm_param
 * 功能描述  : 下发私有开机rx listen ps rssi定制化项到device
 */
OAL_STATIC int32_t hwifi_custom_adapt_device_priv_ini_hal_ps_rssi_param(uint8_t *puc_data,
                                                                        uint32_t *pul_data_len)
{
    hmac_to_dmac_cfg_custom_data_stru st_syn_msg = {0};
    int32_t l_ret;
    int32_t l_priv_val = 0;
    uint32_t hal_ps_rssi_params = 0;

    if (puc_data == NULL) {
        oam_error_log1(0, OAM_SF_CFG,
            "{hwifi_custom_adapt_device_priv_ini_hal_ps_rssi_param::puc_data NULL data_len[%d].}", *pul_data_len);
        return OAL_FAIL;
    }

    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_HAL_PS_RSSI_PARAM, &l_priv_val);
    if (l_ret == OAL_SUCC) {
        hal_ps_rssi_params = (uint32_t)l_priv_val;
        oal_io_print("hwifi_custom_adapt_device_priv_ini_hal_ps_rssi_param::read hal_ps_rssi_params[%d]l_ret[%d]\r\n",
                     hal_ps_rssi_params, l_ret);
    } else {
        return OAL_FAIL;
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_HAL_PS_RSSI_ID;
    st_syn_msg.len = sizeof(hal_ps_rssi_params);

    l_ret = memcpy_s(puc_data, (WLAN_LARGE_NETBUF_SIZE - *pul_data_len), &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    l_ret += memcpy_s(puc_data + CUSTOM_MSG_DATA_HDR_LEN,
                      (WLAN_LARGE_NETBUF_SIZE - *pul_data_len - CUSTOM_MSG_DATA_HDR_LEN),
                      &hal_ps_rssi_params, sizeof(hal_ps_rssi_params));
    if (l_ret != EOK) {
        oam_error_log2(0, OAM_SF_CFG,
                       "{hwifi_custom_adapt_device_priv_ini_hal_ps_rssi_param::memcpy_s fail[%d]. data_len[%d]}",
                       l_ret, *pul_data_len);
        *pul_data_len += (sizeof(hal_ps_rssi_params) + CUSTOM_MSG_DATA_HDR_LEN);

        return OAL_FAIL;
    }

    *pul_data_len += (sizeof(hal_ps_rssi_params) + CUSTOM_MSG_DATA_HDR_LEN);

    oam_warning_log2(0, OAM_SF_CFG,
                     "{hwifi_custom_adapt_device_priv_ini_hal_ps_rssi_param::da_len[%d] hal_pps_rssi_params [%X].}",
                     *pul_data_len, hal_ps_rssi_params);

    return OAL_SUCC;
}

/*
 * 函 数 名  : hwifi_custom_adapt_device_priv_ini_download_pm_param
 * 功能描述  : 下发私有开机rx listen ps pps定制化项到device
 */
OAL_STATIC int32_t hwifi_custom_adapt_device_priv_ini_hal_ps_pps_param(uint8_t *puc_data,
                                                                       uint32_t *pul_data_len)
{
    hmac_to_dmac_cfg_custom_data_stru st_syn_msg = {0};
    int32_t l_ret;
    int32_t l_priv_val = 0;
    uint32_t hal_ps_pps_params = 0;

    if (puc_data == NULL) {
        oam_error_log1(0, OAM_SF_CFG,
            "{hwifi_custom_adapt_device_priv_ini_hal_ps_pps_param::puc_data NULL data_len[%d].}", *pul_data_len);
        return OAL_FAIL;
    }

    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_HAL_PS_PPS_PARAM, &l_priv_val);
    if (l_ret == OAL_SUCC) {
        hal_ps_pps_params = (uint32_t)l_priv_val;
        oal_io_print("hwifi_custom_adapt_device_priv_ini_hal_ps_pps_param::read hal_ps_pps_params[%d]l_ret[%d]\r\n",
                     hal_ps_pps_params, l_ret);
    } else {
        return OAL_FAIL;
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_HAL_PS_PPS_ID;
    st_syn_msg.len = sizeof(hal_ps_pps_params);

    l_ret = memcpy_s(puc_data, (WLAN_LARGE_NETBUF_SIZE - *pul_data_len), &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    l_ret += memcpy_s(puc_data + CUSTOM_MSG_DATA_HDR_LEN,
                      (WLAN_LARGE_NETBUF_SIZE - *pul_data_len - CUSTOM_MSG_DATA_HDR_LEN),
                      &hal_ps_pps_params, sizeof(hal_ps_pps_params));
    if (l_ret != EOK) {
        oam_error_log2(0, OAM_SF_CFG,
                       "{hwifi_custom_adapt_device_priv_ini_hal_ps_pps_param::memcpy_s fail[%d]. data_len[%d]}",
                       l_ret, *pul_data_len);
        *pul_data_len += (sizeof(hal_ps_pps_params) + CUSTOM_MSG_DATA_HDR_LEN);

        return OAL_FAIL;
    }

    *pul_data_len += (sizeof(hal_ps_pps_params) + CUSTOM_MSG_DATA_HDR_LEN);

    oam_warning_log2(0, OAM_SF_CFG,
                     "{hwifi_custom_adapt_device_priv_ini_hal_ps_pps_param::da_len[%d] hal_ps_pps_params [%X].}",
                     *pul_data_len, hal_ps_pps_params);

    return OAL_SUCC;
}

#endif
/*
 * 函 数 名  : hwifi_custom_adapt_device_priv_ini_hiex_cap
 * 功能描述  : 下发私有开机hiex cap定制化项到device
 */
OAL_STATIC int32_t hwifi_custom_adapt_device_priv_ini_hiex_cap(uint8_t *puc_data,
    uint32_t *pul_data_len)
{
    hmac_to_dmac_cfg_custom_data_stru st_syn_msg = {0};
    int32_t l_ret;
    int32_t l_priv_val = 0;
    uint32_t hiex_cap = 0;

    if (puc_data == NULL) {
        oam_error_log1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_priv_ini_hiex_cap::puc_data NULL data_len[%d].}",
            *pul_data_len);
        return OAL_FAIL;
    }
    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_HIEX_CAP, &l_priv_val);
    if (l_ret == OAL_SUCC) {
        hiex_cap = (uint32_t)l_priv_val;
        oal_io_print("hwifi_custom_adapt_device_priv_ini_hiex_cap::read hiex_cap[%d]l_ret[%d]\r\n", hiex_cap, l_ret);
    } else {
        return OAL_FAIL;
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_HIEX_CAP_ID;
    st_syn_msg.len = sizeof(hiex_cap);
    l_ret = memcpy_s(puc_data, (WLAN_LARGE_NETBUF_SIZE - *pul_data_len), &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    l_ret += memcpy_s(puc_data + CUSTOM_MSG_DATA_HDR_LEN,
        (WLAN_LARGE_NETBUF_SIZE - *pul_data_len - CUSTOM_MSG_DATA_HDR_LEN), &hiex_cap, sizeof(hiex_cap));
    if (l_ret != EOK) {
        oam_error_log2(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_priv_ini_hiex_cap::memcpy_s fail[%d] data_len[%d]}",
            l_ret, *pul_data_len);
        *pul_data_len += (sizeof(hiex_cap) + CUSTOM_MSG_DATA_HDR_LEN);
        return OAL_FAIL;
    }

    *pul_data_len += (sizeof(hiex_cap) + CUSTOM_MSG_DATA_HDR_LEN);
    oam_warning_log2(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_priv_ini_hiex_cap::data_len[%d] hiex_cap[0x%X]}",
        *pul_data_len, hiex_cap);

    return OAL_SUCC;
}
/*
 * 函 数 名  : hwifi_custom_adapt_device_priv_ini_param_extend
 * 功能描述  : ini device侧上电前定制化参数适配extend
 */
OAL_STATIC void hwifi_custom_adapt_device_priv_ini_param_extend(uint8_t *puc_data, uint32_t data_len, uint32_t *pul_len)
{
    int32_t l_ret;

    hwifi_custom_adapt_device_priv_ini_temper_thread_param(puc_data + data_len, &data_len);
    hwifi_custom_adapt_device_priv_ini_dsss2ofdm_dbb_pwr_bo_param(puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_EVM_PLL_REG_FIX, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_VOE_SWITCH, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_11AX_SWITCH, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_HTC_SWITCH, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_MULTI_BSSID_SWITCH, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_AC_SUSPEND, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_DYNAMIC_DBAC_SWITCH, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_DYN_BYPASS_EXTLNA, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_CTRL_FRAME_TX_CHAIN, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_CTRL_UPC_FOR_18DBM_CO, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_CTRL_UPC_FOR_18DBM_C1, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_CTRL_11B_DOUBLE_CHAIN_BO_POW, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_HCC_FLOWCTRL_TYPE, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_MBO_SWITCH, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_DC_FLOWCTL_SWITCH, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_PHY_CAP_SWITCH, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_OPTIMIZED_FEATURE_SWITCH, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_CLOSE_FILTER_SWITCH, puc_data + data_len, &data_len);
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_MCAST_AMPDU_ENABLE, puc_data + data_len, &data_len);
#endif
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_PT_MCAST_ENABLE, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_MIRACAST_SINK, puc_data + data_len, &data_len);

    l_ret = hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_FTM_CAP, puc_data + data_len, &data_len);
    if (l_ret != OAL_SUCC) {
        oam_warning_log0(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_priv_ini_param_extend::FTM ini fail.}");
    }

#ifdef _PRE_WLAN_RX_LISTEN_POWER_SAVING
    hwifi_custom_adapt_device_priv_ini_hal_ps_rssi_param(puc_data + data_len, &data_len);
    hwifi_custom_adapt_device_priv_ini_hal_ps_pps_param(puc_data + data_len, &data_len);
#endif
    l_ret = hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_DDR_SWITCH, puc_data + data_len, &data_len);
    if (l_ret != OAL_SUCC) {
        oam_warning_log0(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_priv_ini_param_extend::ini fail.}");
    }
    l_ret = hwifi_custom_adapt_device_priv_ini_hiex_cap(puc_data + data_len, &data_len);
    if (l_ret != OAL_SUCC) {
        oam_warning_log0(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_priv_ini_param_extend::hiex cap ini fail}");
    }

    *pul_len = data_len;
}
/*
 * 函 数 名  : hwifi_custom_adapt_device_priv_ini_fem_pow_ctl_param
 * 功能描述  : 下发私有fem pow ctl配置定制化项到device
 * 修改历史      :
 *  1.日    期   : 2019年11月2日
 *    作    者   : wulei
 *    修改内容   : 新生成函数
 */
OAL_STATIC int32_t hwifi_custom_adapt_device_priv_ini_fem_pow_ctl_param(uint8_t *puc_data,
    uint32_t *pul_data_len)
{
    int32_t l_ret;
    hmac_to_dmac_cfg_custom_data_stru st_syn_msg = {0};
    int32_t l_priv_fem_ctl_value = 0;
    int32_t l_priv_fem_adj_value = 0;
    dmac_ax_fem_pow_ctl_stru st_fem_pow_ctl = {0};

    if (puc_data == NULL) {
        oam_error_log1(0, OAM_SF_CUSTOM, "{hwifi_custom_adapt_device_priv_ini_fem_pow_ctl_param::puc_data is \
        NULL data_len[%d].}", *pul_data_len);
        return OAL_FAIL;
    }

    /* 为了私有定制化值 */
    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_FEM_DELT_POW, &l_priv_fem_ctl_value);
    l_ret += hwifi_get_init_priv_value(WLAN_CFG_PRIV_FEM_ADJ_TPC_TBL_START_IDX, &l_priv_fem_adj_value);
    if (l_ret != OAL_SUCC) {
        return OAL_FAIL;
    }
    st_fem_pow_ctl.uc_fem_delt_pow = (uint8_t)l_priv_fem_ctl_value;
    st_fem_pow_ctl.uc_tpc_adj_pow_start_idx = (uint8_t)l_priv_fem_adj_value;
    st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_FEM_POW_CTL_ID;
    st_syn_msg.len = sizeof(st_fem_pow_ctl);

    l_ret = memcpy_s(puc_data, WLAN_LARGE_NETBUF_SIZE - *pul_data_len, &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    l_ret += memcpy_s(puc_data + CUSTOM_MSG_DATA_HDR_LEN,
                      (WLAN_LARGE_NETBUF_SIZE - *pul_data_len) - CUSTOM_MSG_DATA_HDR_LEN,
                      &st_fem_pow_ctl, sizeof(st_fem_pow_ctl));
    if (l_ret != EOK) {
        oam_error_log3(0, OAM_SF_CUSTOM, "{hwifi_custom_adapt_device_priv_ini_fem_pow_ctl_param::memcpy_s fail[%d]. \
        data_len[%d] MAX_len[%d]}", l_ret, *pul_data_len, WLAN_LARGE_NETBUF_SIZE);
        return OAL_FAIL;
    }

    *pul_data_len += sizeof(st_fem_pow_ctl) + CUSTOM_MSG_DATA_HDR_LEN;
    oam_warning_log3(0, OAM_SF_CFG,
                     "{hwifi_custom_adapt_device_priv_ini_fem_pow_ctl_param::len[%d]delt_pow[%d] start_idx[%d].}",
                     *pul_data_len, st_fem_pow_ctl.uc_fem_delt_pow, st_fem_pow_ctl.uc_tpc_adj_pow_start_idx);

    return OAL_SUCC;
}

/*
 * 函 数 名  : hwifi_custom_adapt_device_priv_ini_radio_cap_param
 * 功能描述  : 下发私有动态/静态dbdc配置定制化项到device
 */
OAL_STATIC int32_t hwifi_custom_adapt_device_priv_ini_radio_cap_param(uint8_t *puc_data, uint32_t *pul_data_len)
{
    int32_t ret;
    hmac_to_dmac_cfg_custom_data_stru st_syn_msg = {0};
    int32_t l_priv_value = 0;
    uint8_t uc_cmd_idx, uc_device_idx;
    uint8_t auc_wlan_service_device_per_chip[WLAN_SERVICE_DEVICE_MAX_NUM_PER_CHIP] = { WLAN_INIT_DEVICE_RADIO_CAP };

    if (puc_data == NULL) {
        oam_error_log1(0, OAM_SF_CFG,
            "{hwifi_custom_adapt_device_priv_ini_radio_cap_param::puc_data is NULL data_len[%d].}", *pul_data_len);
        return OAL_FAIL;
    }

    /* 为了不影响host device初始化，这里重新获取定制化文件读到的值 */
    uc_cmd_idx = WLAN_CFG_PRIV_DBDC_RADIO_0;
    for (uc_device_idx = 0; uc_device_idx < WLAN_SERVICE_DEVICE_MAX_NUM_PER_CHIP; uc_device_idx++) {
        ret = hwifi_get_init_priv_value(uc_cmd_idx++, &l_priv_value);
        if (ret == OAL_SUCC) {
            oam_warning_log1(0, OAM_SF_ANY,
                "{hwifi_custom_adapt_device_priv_ini_radio_cap_param::WLAN_CFG_PRIV_DBDC_RADIO_0 [%d]}", l_priv_value);
            auc_wlan_service_device_per_chip[uc_device_idx] = (uint8_t)(uint32_t)l_priv_value;
        }
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_RADIO_CAP_ID;
    st_syn_msg.len = sizeof(auc_wlan_service_device_per_chip);

    ret = memcpy_s(puc_data, WLAN_LARGE_NETBUF_SIZE, &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    ret += memcpy_s(puc_data + CUSTOM_MSG_DATA_HDR_LEN, (WLAN_LARGE_NETBUF_SIZE - CUSTOM_MSG_DATA_HDR_LEN),
        auc_wlan_service_device_per_chip, sizeof(auc_wlan_service_device_per_chip));
    if (ret != EOK) {
        oam_error_log2(0, OAM_SF_CFG,
            "{hwifi_custom_adapt_device_priv_ini_radio_cap_param:memcpy_s fail[%d].data_len[%d]}", ret, *pul_data_len);
        *pul_data_len += (sizeof(auc_wlan_service_device_per_chip) + CUSTOM_MSG_DATA_HDR_LEN);
        return OAL_FAIL;
    }

    *pul_data_len += (sizeof(auc_wlan_service_device_per_chip) + CUSTOM_MSG_DATA_HDR_LEN);

    oam_warning_log2(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_priv_ini_radio_cap_param::da_len[%d] radio_cap_0[%d].}",
                     *pul_data_len, auc_wlan_service_device_per_chip[0]);

    return OAL_SUCC;
}


/*
 * 函 数 名  : hwifi_custom_adapt_device_priv_ini_cali_mask_param
 * 功能描述  : 下发私有开机校准配置定制化项到device
 */
OAL_STATIC int32_t hwifi_custom_adapt_device_priv_ini_cali_mask_param(uint8_t *puc_data, uint32_t *pul_data_len)
{
    int32_t l_ret;
    hmac_to_dmac_cfg_custom_data_stru st_syn_msg = {0};
    int32_t l_priv_val = 0;
    uint32_t cali_mask;
#ifdef _PRE_PHONE_RUNMODE_FACTORY
    int32_t l_chip_type = get_hi110x_subchip_type();
#endif

    if (puc_data == NULL) {
        oam_error_log1(0, OAM_SF_CFG,
            "{hwifi_custom_adapt_device_priv_ini_cali_mask_param::puc_data is NULL data_len[%d].}", *pul_data_len);
        return OAL_FAIL;
    }

    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_CALI_MASK, &l_priv_val);
    if (l_ret == OAL_SUCC) {
        cali_mask = (uint32_t)l_priv_val;
#ifdef _PRE_PHONE_RUNMODE_FACTORY
        if (l_chip_type == BOARD_VERSION_HI1105) {
            cali_mask = (cali_mask & (~CALI_MIMO_MASK)) | CALI_MUL_TIME_CALI_MASK;
        }
#endif
        oal_io_print("hwifi_custom_adapt_device_priv_ini_cali_mask_param::read cali_mask[%d]l_ret[%d]\r\n",
                     cali_mask, l_ret);
    } else {
        return OAL_FAIL;
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_CALI_MASK_ID;
    st_syn_msg.len = sizeof(cali_mask);

    l_ret = memcpy_s(puc_data, (WLAN_LARGE_NETBUF_SIZE - *pul_data_len), &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    l_ret += memcpy_s(puc_data + CUSTOM_MSG_DATA_HDR_LEN,
        (WLAN_LARGE_NETBUF_SIZE - *pul_data_len - CUSTOM_MSG_DATA_HDR_LEN), &cali_mask, sizeof(cali_mask));
    if (l_ret != EOK) {
        oam_error_log2(0, OAM_SF_CFG,
                       "{hwifi_custom_adapt_device_priv_ini_cali_mask_param::memcpy_s fail[%d]. data_len[%d]}",
                       l_ret, *pul_data_len);
        *pul_data_len += (sizeof(cali_mask) + CUSTOM_MSG_DATA_HDR_LEN);

        return OAL_FAIL;
    }

    *pul_data_len += (sizeof(cali_mask) + CUSTOM_MSG_DATA_HDR_LEN);

    oam_warning_log2(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_priv_ini_cali_mask_param::da_len[%d] cali_mask[0x%x].}",
                     *pul_data_len, cali_mask);

    return OAL_SUCC;
}

/*
 * 函 数 名  : hwifi_custom_adapt_device_priv_ini_download_pm_param
 * 功能描述  : 下发私有开机校准配置定制化项到device
 */
OAL_STATIC int32_t hwifi_custom_adapt_device_priv_ini_download_pm_param(uint8_t *puc_data, uint32_t *pul_data_len)
{
    int32_t l_ret;
    hmac_to_dmac_cfg_custom_data_stru st_syn_msg = {0};
    int32_t l_priv_val = 0;
    uint16_t us_download_rate_limit_pps;

    if (puc_data == NULL) {
        oam_error_log1(0, OAM_SF_CFG,
            "{hwifi_custom_adapt_device_priv_ini_download_pm_param::puc_data is NULL data_len[%d]}", *pul_data_len);
        return OAL_FAIL;
    }

    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_DOWNLOAD_RATE_LIMIT_PPS, &l_priv_val);
    if (l_ret == OAL_SUCC) {
        us_download_rate_limit_pps = (uint16_t)(uint32_t)l_priv_val;
        oal_io_print("hwifi_custom_adapt_device_priv_ini_download_pm_param::read download_rate_limit_pps[%d] \
            l_ret[%d]\r\n", us_download_rate_limit_pps, l_ret);
    } else {
        return OAL_FAIL;
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_DOWNLOAD_RATELIMIT_PPS;
    st_syn_msg.len = sizeof(us_download_rate_limit_pps);

    l_ret = memcpy_s(puc_data, (WLAN_LARGE_NETBUF_SIZE - *pul_data_len), &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    l_ret += memcpy_s(puc_data + CUSTOM_MSG_DATA_HDR_LEN,
                      (WLAN_LARGE_NETBUF_SIZE - *pul_data_len - CUSTOM_MSG_DATA_HDR_LEN),
                      &us_download_rate_limit_pps, sizeof(us_download_rate_limit_pps));
    if (l_ret != EOK) {
        oam_error_log2(0, OAM_SF_CFG,
                       "{hwifi_custom_adapt_device_priv_ini_download_pm_param::memcpy_s fail[%d]. data_len[%d]}",
                       l_ret, *pul_data_len);
        *pul_data_len += (sizeof(us_download_rate_limit_pps) + CUSTOM_MSG_DATA_HDR_LEN);

        return OAL_FAIL;
    }

    *pul_data_len += (sizeof(us_download_rate_limit_pps) + CUSTOM_MSG_DATA_HDR_LEN);

    oam_warning_log2(0, OAM_SF_CFG,
                     "{hwifi_custom_adapt_device_priv_ini_download_pm_param::da_len[%d] download_rate_limit [%d]pps.}",
                     *pul_data_len, us_download_rate_limit_pps);

    return OAL_SUCC;
}


/*
 * 函 数 名  : hwifi_custom_adapt_device_priv_ini_param
 * 功能描述  : ini device侧上电前定制化参数适配
 */
int32_t hwifi_custom_adapt_device_priv_ini_param(uint8_t *puc_data)
{
    uint32_t data_len = 0;

    if (puc_data == NULL) {
        oam_error_log0(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_priv_ini_param::puc_data is NULL.}");
        return INI_FAILED;
    }

    /*
     * 发送消息的格式如下:
     * +-------------------------------------------------------------------+
     * | CFGID0    |DATA0 Length| DATA0 Value | ......................... |
     * +-------------------------------------------------------------------+
     * | 4 Bytes   |4 Byte      | DATA  Length| ......................... |
     * +-------------------------------------------------------------------+
     */
    /* 私有定制化 */
    hwifi_custom_adapt_device_priv_ini_radio_cap_param(puc_data, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_BW_MAX_WITH, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_LDPC_CODING, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_RX_STBC, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_TX_STBC, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_SU_BFER, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_SU_BFEE, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_MU_BFER, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_MU_BFEE, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_11N_TXBF, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_1024_QAM, puc_data + data_len, &data_len);
    hwifi_custom_adapt_device_priv_ini_cali_mask_param(puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_CALI_DATA_MASK, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_CALI_AUTOCALI_MASK, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_OVER_TEMP_PRO_ENABLE, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_OVER_TEMP_PRO_REDUCE_PWR_ENABLE,
                                      puc_data + data_len, &data_len);

    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_OVER_TEMP_PRO_SAFE_TH, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_OVER_TEMP_PRO_OVER_TH, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_OVER_TEMP_PRO_PA_OFF_TH, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_FASTSCAN_SWITCH, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_ANT_SWITCH, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_M2S_FUNCTION_MASK, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_M2S_FUNCTION_EXT_MASK, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_MCM_FUNCTION_MASK, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_MCM_CUSTOM_FUNCTION_MASK, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRRIV_LINKLOSS_THRESHOLD_FIXED, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_COUNRTYCODE_SELFSTUDY_CFG, puc_data + data_len, &data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_RADAR_ISR_FORBID, puc_data + data_len, &data_len);
    hwifi_custom_adapt_device_priv_ini_download_pm_param(puc_data + data_len, &data_len);
#ifdef _PRE_WLAN_FEATURE_TXOPPS
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_TXOPPS_SWITCH, puc_data + data_len, &data_len);
#endif
    hwifi_custom_adapt_device_priv_ini_param_extend(puc_data, data_len, &data_len);
    hwifi_custom_adapt_device_priv_ini_fem_pow_ctl_param(puc_data + data_len, &data_len);
    oam_warning_log2(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_priv_ini_param::da_len[%d]MAX[%d].}",
                     data_len, WLAN_LARGE_NETBUF_SIZE);
    return data_len;
}
