

/* 头文件包含 */
#include "hisi_customize_wifi_hi1103.h"
#include "hisi_customize_config_dts_hi1103.h"
#include "hisi_customize_nvram_config_hi1103.h"

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
#define THIS_FILE_ID OAM_FILE_ID_HISI_CUSTOMIZE_NVRAM_CONFIG_HI1103_C

OAL_STATIC int32_t g_nvram_init_params[NVRAM_PARAMS_PWR_INDEX_BUTT] = {0};         /* ini文件中NV参数数组 */

OAL_STATIC wlan_init_cust_nvram_params g_cust_nv_params = {{{0}}}; /* 最大发送功率定制化数组 */

OAL_STATIC wlan_cfg_cmd g_nvram_config_ini[NVRAM_PARAMS_PWR_INDEX_BUTT] = {
    { "nvram_params0",  NVRAM_PARAMS_INDEX_0 },
    { "nvram_params1",  NVRAM_PARAMS_INDEX_1 },
    { "nvram_params2",  NVRAM_PARAMS_INDEX_2 },
    { "nvram_params3",  NVRAM_PARAMS_INDEX_3 },
    { "nvram_params4",  NVRAM_PARAMS_INDEX_4 },
    { "nvram_params5",  NVRAM_PARAMS_INDEX_5 },
    { "nvram_params6",  NVRAM_PARAMS_INDEX_6 },
    { "nvram_params7",  NVRAM_PARAMS_INDEX_7 },
    { "nvram_params8",  NVRAM_PARAMS_INDEX_8 },
    { "nvram_params9",  NVRAM_PARAMS_INDEX_9 },
    { "nvram_params10", NVRAM_PARAMS_INDEX_10 },
    { "nvram_params11", NVRAM_PARAMS_INDEX_11 },
    { "nvram_params12", NVRAM_PARAMS_INDEX_12 },
    { "nvram_params13", NVRAM_PARAMS_INDEX_13 },
    { "nvram_params14", NVRAM_PARAMS_INDEX_14 },
    { "nvram_params15", NVRAM_PARAMS_INDEX_15 },
    { "nvram_params16", NVRAM_PARAMS_INDEX_16 },
    { "nvram_params17", NVRAM_PARAMS_INDEX_17 },
    { "nvram_params59", NVRAM_PARAMS_INDEX_DPD_0 },
    { "nvram_params60", NVRAM_PARAMS_INDEX_DPD_1 },
    { "nvram_params61", NVRAM_PARAMS_INDEX_DPD_2 },
    /* 11B & OFDM delta power */
    { "nvram_params62", NVRAM_PARAMS_INDEX_11B_OFDM_DELT_POW },
    /* 5G cali upper upc limit */
    { "nvram_params63", NVRAM_PARAMS_INDEX_IQ_MAX_UPC },
    /* 2G low pow amend */
    { "nvram_params64",                  NVRAM_PARAMS_INDEX_2G_LOW_POW_AMEND },
    { NULL,                      NVRAM_PARAMS_TXPWR_INDEX_BUTT },
    { "nvram_max_txpwr_base_2p4g",       NVRAM_PARAMS_INDEX_19 },
    { "nvram_max_txpwr_base_5g",         NVRAM_PARAMS_INDEX_20 },
    { "nvram_max_txpwr_base_2p4g_slave", NVRAM_PARAMS_INDEX_21 },
    { "nvram_max_txpwr_base_5g_slave",   NVRAM_PARAMS_INDEX_22 },
    { NULL,                      NVRAM_PARAMS_BASE_INDEX_BUTT },
    { "nvram_delt_max_base_txpwr",       NVRAM_PARAMS_INDEX_DELT_BASE_POWER_23 },
    { NULL,                      NVRAM_PARAMS_INDEX_24_RSV },
    /* FCC */
    { "fcc_side_band_txpwr_limit_5g_20m_0", NVRAM_PARAMS_INDEX_25 },
    { "fcc_side_band_txpwr_limit_5g_20m_1", NVRAM_PARAMS_INDEX_26 },
    { "fcc_side_band_txpwr_limit_5g_40m_0", NVRAM_PARAMS_INDEX_27 },
    { "fcc_side_band_txpwr_limit_5g_40m_1", NVRAM_PARAMS_INDEX_28 },
    { "fcc_side_band_txpwr_limit_5g_80m_0", NVRAM_PARAMS_INDEX_29 },
    { "fcc_side_band_txpwr_limit_5g_80m_1", NVRAM_PARAMS_INDEX_30 },
    { "fcc_side_band_txpwr_limit_5g_160m",  NVRAM_PARAMS_INDEX_31 },
    { "fcc_side_band_txpwr_limit_24g_ch1",  NVRAM_PARAMS_INDEX_32 },
    { "fcc_side_band_txpwr_limit_24g_ch2",  NVRAM_PARAMS_INDEX_33 },
    { "fcc_side_band_txpwr_limit_24g_ch3",  NVRAM_PARAMS_INDEX_34 },
    { "fcc_side_band_txpwr_limit_24g_ch4",  NVRAM_PARAMS_INDEX_35 },
    { "fcc_side_band_txpwr_limit_24g_ch5",  NVRAM_PARAMS_INDEX_36 },
    { "fcc_side_band_txpwr_limit_24g_ch6",  NVRAM_PARAMS_INDEX_37 },
    { "fcc_side_band_txpwr_limit_24g_ch7",  NVRAM_PARAMS_INDEX_38 },
    { "fcc_side_band_txpwr_limit_24g_ch8",  NVRAM_PARAMS_INDEX_39 },
    { "fcc_side_band_txpwr_limit_24g_ch9",  NVRAM_PARAMS_INDEX_40 },
    { "fcc_side_band_txpwr_limit_24g_ch10", NVRAM_PARAMS_INDEX_41 },
    { "fcc_side_band_txpwr_limit_24g_ch11", NVRAM_PARAMS_INDEX_42 },
    { "fcc_side_band_txpwr_limit_24g_ch12", NVRAM_PARAMS_INDEX_43 },
    { "fcc_side_band_txpwr_limit_24g_ch13", NVRAM_PARAMS_INDEX_44 },
    { NULL,                         NVRAM_PARAMS_FCC_END_INDEX_BUTT },
    /* CE */
    { "ce_side_band_txpwr_limit_5g_20m_0", NVRAM_PARAMS_INDEX_CE_0 },
    { "ce_side_band_txpwr_limit_5g_20m_1", NVRAM_PARAMS_INDEX_CE_1 },
    { "ce_side_band_txpwr_limit_5g_40m_0", NVRAM_PARAMS_INDEX_CE_2 },
    { "ce_side_band_txpwr_limit_5g_40m_1", NVRAM_PARAMS_INDEX_CE_3 },
    { "ce_side_band_txpwr_limit_5g_80m_0", NVRAM_PARAMS_INDEX_CE_4 },
    { "ce_side_band_txpwr_limit_5g_80m_1", NVRAM_PARAMS_INDEX_CE_5 },
    { "ce_side_band_txpwr_limit_5g_160m",  NVRAM_PARAMS_INDEX_CE_6 },
    { "ce_side_band_txpwr_limit_24g_ch1",  NVRAM_PARAMS_INDEX_CE_7 },
    { "ce_side_band_txpwr_limit_24g_ch2",  NVRAM_PARAMS_INDEX_CE_8 },
    { "ce_side_band_txpwr_limit_24g_ch3",  NVRAM_PARAMS_INDEX_CE_9 },
    { "ce_side_band_txpwr_limit_24g_ch4",  NVRAM_PARAMS_INDEX_CE_10 },
    { "ce_side_band_txpwr_limit_24g_ch5",  NVRAM_PARAMS_INDEX_CE_11 },
    { "ce_side_band_txpwr_limit_24g_ch6",  NVRAM_PARAMS_INDEX_CE_12 },
    { "ce_side_band_txpwr_limit_24g_ch7",  NVRAM_PARAMS_INDEX_CE_13 },
    { "ce_side_band_txpwr_limit_24g_ch8",  NVRAM_PARAMS_INDEX_CE_14 },
    { "ce_side_band_txpwr_limit_24g_ch9",  NVRAM_PARAMS_INDEX_CE_15 },
    { "ce_side_band_txpwr_limit_24g_ch10", NVRAM_PARAMS_INDEX_CE_16 },
    { "ce_side_band_txpwr_limit_24g_ch11", NVRAM_PARAMS_INDEX_CE_17 },
    { "ce_side_band_txpwr_limit_24g_ch12", NVRAM_PARAMS_INDEX_CE_18 },
    { "ce_side_band_txpwr_limit_24g_ch13", NVRAM_PARAMS_INDEX_CE_19 },
    { NULL,                        NVRAM_PARAMS_CE_END_INDEX_BUTT },
    /* FCC */
    { "fcc_side_band_txpwr_limit_5g_20m_0_c1", NVRAM_PARAMS_INDEX_25_C1 },
    { "fcc_side_band_txpwr_limit_5g_20m_1_c1", NVRAM_PARAMS_INDEX_26_C1 },
    { "fcc_side_band_txpwr_limit_5g_40m_0_c1", NVRAM_PARAMS_INDEX_27_C1 },
    { "fcc_side_band_txpwr_limit_5g_40m_1_c1", NVRAM_PARAMS_INDEX_28_C1 },
    { "fcc_side_band_txpwr_limit_5g_80m_0_c1", NVRAM_PARAMS_INDEX_29_C1 },
    { "fcc_side_band_txpwr_limit_5g_80m_1_c1", NVRAM_PARAMS_INDEX_30_C1 },
    { "fcc_side_band_txpwr_limit_5g_160m_c1",  NVRAM_PARAMS_INDEX_31_C1 },
    { "fcc_side_band_txpwr_limit_24g_ch1_c1",  NVRAM_PARAMS_INDEX_32_C1 },
    { "fcc_side_band_txpwr_limit_24g_ch2_c1",  NVRAM_PARAMS_INDEX_33_C1 },
    { "fcc_side_band_txpwr_limit_24g_ch3_c1",  NVRAM_PARAMS_INDEX_34_C1 },
    { "fcc_side_band_txpwr_limit_24g_ch4_c1",  NVRAM_PARAMS_INDEX_35_C1 },
    { "fcc_side_band_txpwr_limit_24g_ch5_c1",  NVRAM_PARAMS_INDEX_36_C1 },
    { "fcc_side_band_txpwr_limit_24g_ch6_c1",  NVRAM_PARAMS_INDEX_37_C1 },
    { "fcc_side_band_txpwr_limit_24g_ch7_c1",  NVRAM_PARAMS_INDEX_38_C1 },
    { "fcc_side_band_txpwr_limit_24g_ch8_c1",  NVRAM_PARAMS_INDEX_39_C1 },
    { "fcc_side_band_txpwr_limit_24g_ch9_c1",  NVRAM_PARAMS_INDEX_40_C1 },
    { "fcc_side_band_txpwr_limit_24g_ch10_c1", NVRAM_PARAMS_INDEX_41_C1 },
    { "fcc_side_band_txpwr_limit_24g_ch11_c1", NVRAM_PARAMS_INDEX_42_C1 },
    { "fcc_side_band_txpwr_limit_24g_ch12_c1", NVRAM_PARAMS_INDEX_43_C1 },
    { "fcc_side_band_txpwr_limit_24g_ch13_c1", NVRAM_PARAMS_INDEX_44_C1 },
    { NULL,                            NVRAM_PARAMS_FCC_C1_END_INDEX_BUTT },
    /* CE */
    { "ce_side_band_txpwr_limit_5g_20m_0_c1", NVRAM_PARAMS_INDEX_CE_0_C1 },
    { "ce_side_band_txpwr_limit_5g_20m_1_c1", NVRAM_PARAMS_INDEX_CE_1_C1 },
    { "ce_side_band_txpwr_limit_5g_40m_0_c1", NVRAM_PARAMS_INDEX_CE_2_C1 },
    { "ce_side_band_txpwr_limit_5g_40m_1_c1", NVRAM_PARAMS_INDEX_CE_3_C1 },
    { "ce_side_band_txpwr_limit_5g_80m_0_c1", NVRAM_PARAMS_INDEX_CE_4_C1 },
    { "ce_side_band_txpwr_limit_5g_80m_1_c1", NVRAM_PARAMS_INDEX_CE_5_C1 },
    { "ce_side_band_txpwr_limit_5g_160m_c1",  NVRAM_PARAMS_INDEX_CE_6_C1 },
    { "ce_side_band_txpwr_limit_24g_ch1_c1",  NVRAM_PARAMS_INDEX_CE_7_C1 },
    { "ce_side_band_txpwr_limit_24g_ch2_c1",  NVRAM_PARAMS_INDEX_CE_8_C1 },
    { "ce_side_band_txpwr_limit_24g_ch3_c1",  NVRAM_PARAMS_INDEX_CE_9_C1 },
    { "ce_side_band_txpwr_limit_24g_ch4_c1",  NVRAM_PARAMS_INDEX_CE_10_C1 },
    { "ce_side_band_txpwr_limit_24g_ch5_c1",  NVRAM_PARAMS_INDEX_CE_11_C1 },
    { "ce_side_band_txpwr_limit_24g_ch6_c1",  NVRAM_PARAMS_INDEX_CE_12_C1 },
    { "ce_side_band_txpwr_limit_24g_ch7_c1",  NVRAM_PARAMS_INDEX_CE_13_C1 },
    { "ce_side_band_txpwr_limit_24g_ch8_c1",  NVRAM_PARAMS_INDEX_CE_14_C1 },
    { "ce_side_band_txpwr_limit_24g_ch9_c1",  NVRAM_PARAMS_INDEX_CE_15_C1 },
    { "ce_side_band_txpwr_limit_24g_ch10_c1", NVRAM_PARAMS_INDEX_CE_16_C1 },
    { "ce_side_band_txpwr_limit_24g_ch11_c1", NVRAM_PARAMS_INDEX_CE_17_C1 },
    { "ce_side_band_txpwr_limit_24g_ch12_c1", NVRAM_PARAMS_INDEX_CE_18_C1 },
    { "ce_side_band_txpwr_limit_24g_ch13_c1", NVRAM_PARAMS_INDEX_CE_19_C1 },
    { NULL,                           NVRAM_PARAMS_CE_C1_END_INDEX_BUTT },
    /* SAR */
    { "sar_txpwr_ctrl_5g_band1_0", NVRAM_PARAMS_INDEX_45 },
    { "sar_txpwr_ctrl_5g_band2_0", NVRAM_PARAMS_INDEX_46 },
    { "sar_txpwr_ctrl_5g_band3_0", NVRAM_PARAMS_INDEX_47 },
    { "sar_txpwr_ctrl_5g_band4_0", NVRAM_PARAMS_INDEX_48 },
    { "sar_txpwr_ctrl_5g_band5_0", NVRAM_PARAMS_INDEX_49 },
    { "sar_txpwr_ctrl_5g_band6_0", NVRAM_PARAMS_INDEX_50 },
    { "sar_txpwr_ctrl_5g_band7_0", NVRAM_PARAMS_INDEX_51 },
    { "sar_txpwr_ctrl_2g_0",       NVRAM_PARAMS_INDEX_52 },
    { "sar_txpwr_ctrl_5g_band1_1", NVRAM_PARAMS_INDEX_53 },
    { "sar_txpwr_ctrl_5g_band2_1", NVRAM_PARAMS_INDEX_54 },
    { "sar_txpwr_ctrl_5g_band3_1", NVRAM_PARAMS_INDEX_55 },
    { "sar_txpwr_ctrl_5g_band4_1", NVRAM_PARAMS_INDEX_56 },
    { "sar_txpwr_ctrl_5g_band5_1", NVRAM_PARAMS_INDEX_57 },
    { "sar_txpwr_ctrl_5g_band6_1", NVRAM_PARAMS_INDEX_58 },
    { "sar_txpwr_ctrl_5g_band7_1", NVRAM_PARAMS_INDEX_59 },
    { "sar_txpwr_ctrl_2g_1",       NVRAM_PARAMS_INDEX_60 },
    { "sar_txpwr_ctrl_5g_band1_2", NVRAM_PARAMS_INDEX_61 },
    { "sar_txpwr_ctrl_5g_band2_2", NVRAM_PARAMS_INDEX_62 },
    { "sar_txpwr_ctrl_5g_band3_2", NVRAM_PARAMS_INDEX_63 },
    { "sar_txpwr_ctrl_5g_band4_2", NVRAM_PARAMS_INDEX_64 },
    { "sar_txpwr_ctrl_5g_band5_2", NVRAM_PARAMS_INDEX_65 },
    { "sar_txpwr_ctrl_5g_band6_2", NVRAM_PARAMS_INDEX_66 },
    { "sar_txpwr_ctrl_5g_band7_2", NVRAM_PARAMS_INDEX_67 },
    { "sar_txpwr_ctrl_2g_2",       NVRAM_PARAMS_INDEX_68 },
    { "sar_txpwr_ctrl_5g_band1_3", NVRAM_PARAMS_INDEX_69 },
    { "sar_txpwr_ctrl_5g_band2_3", NVRAM_PARAMS_INDEX_70 },
    { "sar_txpwr_ctrl_5g_band3_3", NVRAM_PARAMS_INDEX_71 },
    { "sar_txpwr_ctrl_5g_band4_3", NVRAM_PARAMS_INDEX_72 },
    { "sar_txpwr_ctrl_5g_band5_3", NVRAM_PARAMS_INDEX_73 },
    { "sar_txpwr_ctrl_5g_band6_3", NVRAM_PARAMS_INDEX_74 },
    { "sar_txpwr_ctrl_5g_band7_3", NVRAM_PARAMS_INDEX_75 },
    { "sar_txpwr_ctrl_2g_3",       NVRAM_PARAMS_INDEX_76 },
    { "sar_txpwr_ctrl_5g_band1_4", NVRAM_PARAMS_INDEX_77 },
    { "sar_txpwr_ctrl_5g_band2_4", NVRAM_PARAMS_INDEX_78 },
    { "sar_txpwr_ctrl_5g_band3_4", NVRAM_PARAMS_INDEX_79 },
    { "sar_txpwr_ctrl_5g_band4_4", NVRAM_PARAMS_INDEX_80 },
    { "sar_txpwr_ctrl_5g_band5_4", NVRAM_PARAMS_INDEX_81 },
    { "sar_txpwr_ctrl_5g_band6_4", NVRAM_PARAMS_INDEX_82 },
    { "sar_txpwr_ctrl_5g_band7_4", NVRAM_PARAMS_INDEX_83 },
    { "sar_txpwr_ctrl_2g_4",       NVRAM_PARAMS_INDEX_84 },
    { NULL,                NVRAM_PARAMS_SAR_END_INDEX_BUTT },
    {"sar_txpwr_ctrl_5g_band1_0_c1", NVRAM_PARAMS_INDEX_45_C1},
    {"sar_txpwr_ctrl_5g_band2_0_c1", NVRAM_PARAMS_INDEX_46_C1},
    {"sar_txpwr_ctrl_5g_band3_0_c1", NVRAM_PARAMS_INDEX_47_C1},
    {"sar_txpwr_ctrl_5g_band4_0_c1", NVRAM_PARAMS_INDEX_48_C1},
    {"sar_txpwr_ctrl_5g_band5_0_c1", NVRAM_PARAMS_INDEX_49_C1},
    {"sar_txpwr_ctrl_5g_band6_0_c1", NVRAM_PARAMS_INDEX_50_C1},
    {"sar_txpwr_ctrl_5g_band7_0_c1", NVRAM_PARAMS_INDEX_51_C1},
    {"sar_txpwr_ctrl_2g_0_c1",       NVRAM_PARAMS_INDEX_52_C1},
    {"sar_txpwr_ctrl_5g_band1_1_c1", NVRAM_PARAMS_INDEX_53_C1},
    {"sar_txpwr_ctrl_5g_band2_1_c1", NVRAM_PARAMS_INDEX_54_C1},
    {"sar_txpwr_ctrl_5g_band3_1_c1", NVRAM_PARAMS_INDEX_55_C1},
    {"sar_txpwr_ctrl_5g_band4_1_c1", NVRAM_PARAMS_INDEX_56_C1},
    {"sar_txpwr_ctrl_5g_band5_1_c1", NVRAM_PARAMS_INDEX_57_C1},
    {"sar_txpwr_ctrl_5g_band6_1_c1", NVRAM_PARAMS_INDEX_58_C1},
    {"sar_txpwr_ctrl_5g_band7_1_c1", NVRAM_PARAMS_INDEX_59_C1},
    {"sar_txpwr_ctrl_2g_1_c1",       NVRAM_PARAMS_INDEX_60_C1},
    {"sar_txpwr_ctrl_5g_band1_2_c1", NVRAM_PARAMS_INDEX_61_C1},
    {"sar_txpwr_ctrl_5g_band2_2_c1", NVRAM_PARAMS_INDEX_62_C1},
    {"sar_txpwr_ctrl_5g_band3_2_c1", NVRAM_PARAMS_INDEX_63_C1},
    {"sar_txpwr_ctrl_5g_band4_2_c1", NVRAM_PARAMS_INDEX_64_C1},
    {"sar_txpwr_ctrl_5g_band5_2_c1", NVRAM_PARAMS_INDEX_65_C1},
    {"sar_txpwr_ctrl_5g_band6_2_c1", NVRAM_PARAMS_INDEX_66_C1},
    {"sar_txpwr_ctrl_5g_band7_2_c1", NVRAM_PARAMS_INDEX_67_C1},
    {"sar_txpwr_ctrl_2g_2_c1",       NVRAM_PARAMS_INDEX_68_C1},
    {"sar_txpwr_ctrl_5g_band1_3_c1", NVRAM_PARAMS_INDEX_69_C1},
    {"sar_txpwr_ctrl_5g_band2_3_c1", NVRAM_PARAMS_INDEX_70_C1},
    {"sar_txpwr_ctrl_5g_band3_3_c1", NVRAM_PARAMS_INDEX_71_C1},
    {"sar_txpwr_ctrl_5g_band4_3_c1", NVRAM_PARAMS_INDEX_72_C1},
    {"sar_txpwr_ctrl_5g_band5_3_c1", NVRAM_PARAMS_INDEX_73_C1},
    {"sar_txpwr_ctrl_5g_band6_3_c1", NVRAM_PARAMS_INDEX_74_C1},
    {"sar_txpwr_ctrl_5g_band7_3_c1", NVRAM_PARAMS_INDEX_75_C1},
    {"sar_txpwr_ctrl_2g_3_c1",       NVRAM_PARAMS_INDEX_76_C1},
    {"sar_txpwr_ctrl_5g_band1_4_c1", NVRAM_PARAMS_INDEX_77_C1},
    {"sar_txpwr_ctrl_5g_band2_4_c1", NVRAM_PARAMS_INDEX_78_C1},
    {"sar_txpwr_ctrl_5g_band3_4_c1", NVRAM_PARAMS_INDEX_79_C1},
    {"sar_txpwr_ctrl_5g_band4_4_c1", NVRAM_PARAMS_INDEX_80_C1},
    {"sar_txpwr_ctrl_5g_band5_4_c1", NVRAM_PARAMS_INDEX_81_C1},
    {"sar_txpwr_ctrl_5g_band6_4_c1", NVRAM_PARAMS_INDEX_82_C1},
    {"sar_txpwr_ctrl_5g_band7_4_c1", NVRAM_PARAMS_INDEX_83_C1},
    {"sar_txpwr_ctrl_2g_4_c1",       NVRAM_PARAMS_INDEX_84_C1},
    {NULL,                   NVRAM_PARAMS_SAR_C1_END_INDEX_BUTT },

#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
    { "tas_ant_switch_en", NVRAM_PARAMS_TAS_ANT_SWITCH_EN },
    { "tas_txpwr_ctrl_params", NVRAM_PARAMS_TAS_PWR_CTRL },
#endif
    { "5g_max_pow_high_band_fcc_ce", NVRAM_PARAMS_5G_FCC_CE_HIGH_BAND_MAX_PWR },
    { "5g_iq_cali_lpf_lvl",          NVRAM_PARAMS_INDEX_IQ_LPF_LVL},

    { "cfg_tpc_ru_pow_5g_0",         NVRAM_CFG_TPC_RU_POWER_20M_5G},
    { "cfg_tpc_ru_pow_5g_1",         NVRAM_CFG_TPC_RU_POWER_40M_5G},
    { "cfg_tpc_ru_pow_5g_2",         NVRAM_CFG_TPC_RU_POWER_80M_L_5G},
    { "cfg_tpc_ru_pow_5g_3",         NVRAM_CFG_TPC_RU_POWER_80M_H_5G},
    { "cfg_tpc_ru_pow_5g_4",         NVRAM_CFG_TPC_RU_POWER_160M_L_5G},
    { "cfg_tpc_ru_pow_5g_5",         NVRAM_CFG_TPC_RU_POWER_160M_H_5G},
    { "cfg_tpc_ru_pow_2g_0",         NVRAM_CFG_TPC_RU_POWER_20M_2G},
    { "cfg_tpc_ru_pow_2g_1",         NVRAM_CFG_TPC_RU_POWER_40M_0_2G},
    { "cfg_tpc_ru_pow_2g_2",         NVRAM_CFG_TPC_RU_POWER_40M_1_2G},

    { "cfg_tpc_ru_max_pow_242_106_52_26_mimo_2g",      NVRAM_CFG_TPC_RU_MAX_POWER_0_2G_MIMO },
    { "cfg_tpc_ru_max_pow_484_mimo_2g",                NVRAM_CFG_TPC_RU_MAX_POWER_1_2G_MIMO },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_2g_c0",   NVRAM_CFG_TPC_RU_MAX_POWER_0_2G_C0 },
    { "cfg_tpc_ru_max_pow_484_siso_2g_c0",             NVRAM_CFG_TPC_RU_MAX_POWER_1_2G_C0 },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_2g_c1",   NVRAM_CFG_TPC_RU_MAX_POWER_0_2G_C1 },
    { "cfg_tpc_ru_max_pow_484_siso_2g_c1",             NVRAM_CFG_TPC_RU_MAX_POWER_1_2G_C1 },

    { "cfg_tpc_ru_max_pow_242_106_52_26_mimo_5g_b1",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_MIMO_B1 },
    { "cfg_tpc_ru_max_pow_1992_996_484_mimo_5g_b1",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_MIMO_B1 },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c0_b1",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C0_B1 },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c0_b1",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C0_B1 },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c1_b1",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C1_B1 },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c1_b1",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C1_B1 },
    { "cfg_tpc_ru_max_pow_242_106_52_26_mimo_5g_b2",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_MIMO_B2 },
    { "cfg_tpc_ru_max_pow_1992_996_484_mimo_5g_b2",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_MIMO_B2 },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c0_b2",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C0_B2 },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c0_b2",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C0_B2 },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c1_b2",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C1_B2 },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c1_b2",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C1_B2 },
    { "cfg_tpc_ru_max_pow_242_106_52_26_mimo_5g_b3",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_MIMO_B3 },
    { "cfg_tpc_ru_max_pow_1992_996_484_mimo_5g_b3",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_MIMO_B3 },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c0_b3",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C0_B3 },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c0_b3",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C0_B3 },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c1_b3",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C1_B3 },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c1_b3",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C1_B3 },
    { "cfg_tpc_ru_max_pow_242_106_52_26_mimo_5g_b4",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_MIMO_B4 },
    { "cfg_tpc_ru_max_pow_1992_996_484_mimo_5g_b4",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_MIMO_B4 },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c0_b4",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C0_B4 },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c0_b4",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C0_B4 },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c1_b4",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C1_B4 },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c1_b4",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C1_B4 },
    { "cfg_tpc_ru_max_pow_242_106_52_26_mimo_5g_b5",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_MIMO_B5 },
    { "cfg_tpc_ru_max_pow_1992_996_484_mimo_5g_b5",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_MIMO_B5 },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c0_b5",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C0_B5 },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c0_b5",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C0_B5 },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c1_b5",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C1_B5 },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c1_b5",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C1_B5 },
    { "cfg_tpc_ru_max_pow_242_106_52_26_mimo_5g_b6",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_MIMO_B6 },
    { "cfg_tpc_ru_max_pow_1992_996_484_mimo_5g_b6",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_MIMO_B6 },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c0_b6",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C0_B6 },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c0_b6",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C0_B6 },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c1_b6",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C1_B6 },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c1_b6",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C1_B6 },
    { "cfg_tpc_ru_max_pow_242_106_52_26_mimo_5g_b7",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_MIMO_B7 },
    { "cfg_tpc_ru_max_pow_1992_996_484_mimo_5g_b7",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_MIMO_B7 },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c0_b7",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C0_B7 },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c0_b7",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C0_B7 },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c1_b7",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C1_B7 },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c1_b7",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C1_B7 },
    { "cfg_tpc_ru_max_pow_242_106_52_26_mimo_5g_b1_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_MIMO_B1_CE },
    { "cfg_tpc_ru_max_pow_1992_996_484_mimo_5g_b1_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_MIMO_B1_CE },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c0_b1_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C0_B1_CE },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c0_b1_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C0_B1_CE },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c1_b1_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C1_B1_CE },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c1_b1_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C1_B1_CE },
    { "cfg_tpc_ru_max_pow_242_106_52_26_mimo_5g_b2_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_MIMO_B2_CE },
    { "cfg_tpc_ru_max_pow_1992_996_484_mimo_5g_b2_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_MIMO_B2_CE },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c0_b2_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C0_B2_CE },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c0_b2_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C0_B2_CE },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c1_b2_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C1_B2_CE },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c1_b2_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C1_B2_CE },
    { "cfg_tpc_ru_max_pow_242_106_52_26_mimo_5g_b3_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_MIMO_B3_CE },
    { "cfg_tpc_ru_max_pow_1992_996_484_mimo_5g_b3_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_MIMO_B3_CE },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c0_b3_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C0_B3_CE },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c0_b3_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C0_B3_CE },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c1_b3_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C1_B3_CE },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c1_b3_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C1_B3_CE },
    { "cfg_tpc_ru_max_pow_242_106_52_26_mimo_5g_b4_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_MIMO_B4_CE },
    { "cfg_tpc_ru_max_pow_1992_996_484_mimo_5g_b4_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_MIMO_B4_CE },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c0_b4_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C0_B4_CE },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c0_b4_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C0_B4_CE },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c1_b4_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C1_B4_CE },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c1_b4_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C1_B4_CE },
    { "cfg_tpc_ru_max_pow_242_106_52_26_mimo_5g_b5_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_MIMO_B5_CE },
    { "cfg_tpc_ru_max_pow_1992_996_484_mimo_5g_b5_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_MIMO_B5_CE },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c0_b5_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C0_B5_CE },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c0_b5_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C0_B5_CE },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c1_b5_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C1_B5_CE },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c1_b5_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C1_B5_CE },
    { "cfg_tpc_ru_max_pow_242_106_52_26_mimo_5g_b6_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_MIMO_B6_CE },
    { "cfg_tpc_ru_max_pow_1992_996_484_mimo_5g_b6_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_MIMO_B6_CE },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c0_b6_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C0_B6_CE },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c0_b6_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C0_B6_CE },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c1_b6_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C1_B6_CE },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c1_b6_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C1_B6_CE },
    { "cfg_tpc_ru_max_pow_242_106_52_26_mimo_5g_b7_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_MIMO_B7_CE },
    { "cfg_tpc_ru_max_pow_1992_996_484_mimo_5g_b7_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_MIMO_B7_CE },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c0_b7_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C0_B7_CE },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c0_b7_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C0_B7_CE },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c1_b7_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C1_B7_CE },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c1_b7_ce",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C1_B7_CE },
    { "cfg_tpc_ru_max_pow_242_106_52_26_mimo_5g_b1_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_MIMO_B1_FCC },
    { "cfg_tpc_ru_max_pow_1992_996_484_mimo_5g_b1_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_MIMO_B1_FCC },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c0_b1_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C0_B1_FCC },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c0_b1_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C0_B1_FCC },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c1_b1_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C1_B1_FCC },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c1_b1_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C1_B1_FCC },
    { "cfg_tpc_ru_max_pow_242_106_52_26_mimo_5g_b2_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_MIMO_B2_FCC },
    { "cfg_tpc_ru_max_pow_1992_996_484_mimo_5g_b2_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_MIMO_B2_FCC },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c0_b2_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C0_B2_FCC },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c0_b2_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C0_B2_FCC },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c1_b2_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C1_B2_FCC },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c1_b2_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C1_B2_FCC },
    { "cfg_tpc_ru_max_pow_242_106_52_26_mimo_5g_b3_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_MIMO_B3_FCC },
    { "cfg_tpc_ru_max_pow_1992_996_484_mimo_5g_b3_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_MIMO_B3_FCC },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c0_b3_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C0_B3_FCC },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c0_b3_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C0_B3_FCC },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c1_b3_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C1_B3_FCC },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c1_b3_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C1_B3_FCC },
    { "cfg_tpc_ru_max_pow_242_106_52_26_mimo_5g_b4_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_MIMO_B4_FCC },
    { "cfg_tpc_ru_max_pow_1992_996_484_mimo_5g_b4_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_MIMO_B4_FCC },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c0_b4_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C0_B4_FCC },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c0_b4_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C0_B4_FCC },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c1_b4_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C1_B4_FCC },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c1_b4_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C1_B4_FCC },
    { "cfg_tpc_ru_max_pow_242_106_52_26_mimo_5g_b5_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_MIMO_B5_FCC },
    { "cfg_tpc_ru_max_pow_1992_996_484_mimo_5g_b5_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_MIMO_B5_FCC },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c0_b5_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C0_B5_FCC },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c0_b5_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C0_B5_FCC },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c1_b5_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C1_B5_FCC },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c1_b5_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C1_B5_FCC },
    { "cfg_tpc_ru_max_pow_242_106_52_26_mimo_5g_b6_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_MIMO_B6_FCC },
    { "cfg_tpc_ru_max_pow_1992_996_484_mimo_5g_b6_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_MIMO_B6_FCC },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c0_b6_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C0_B6_FCC },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c0_b6_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C0_B6_FCC },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c1_b6_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C1_B6_FCC },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c1_b6_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C1_B6_FCC },
    { "cfg_tpc_ru_max_pow_242_106_52_26_mimo_5g_b7_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_MIMO_B7_FCC },
    { "cfg_tpc_ru_max_pow_1992_996_484_mimo_5g_b7_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_MIMO_B7_FCC },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c0_b7_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C0_B7_FCC },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c0_b7_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C0_B7_FCC },
    { "cfg_tpc_ru_max_pow_242_106_52_26_siso_5g_c1_b7_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_C1_B7_FCC },
    { "cfg_tpc_ru_max_pow_1992_996_484_siso_5g_c1_b7_fcc",    NVRAM_CFG_TPC_RU_MAX_POWER_1_5G_C1_B7_FCC },

    { "cfg_tpc_ru_max_pow_2g",      NVRAM_CFG_TPC_RU_MAX_POWER_2G },
    { "cfg_tpc_ru_max_pow_5g",      NVRAM_CFG_TPC_RU_MAX_POWER_5G },
};

wlan_cfg_cmd *hwifi_get_nvram_config(void)
{
    return (wlan_cfg_cmd *)g_nvram_config_ini;
}

/*
 * 函 数 名  : hwifi_get_init_nvram_params
 * 功能描述  : 获取定制化NV数据结构体
 */
void *hwifi_get_init_nvram_params(void)
{
    return &g_cust_nv_params;
}

/*
 * 函 数 名  : hwifi_get_nvram_params
 * 功能描述  : 获取定制化最大发射功率和对应的scaling值
 */
void *hwifi_get_nvram_params(void)
{
    return &g_cust_nv_params.st_pow_ctrl_custom_param;
}

/*
 * 函 数 名  : original_value_for_nvram_params
 * 功能描述  : 最大发射功率定制化参数初值处理
 */
void original_value_for_nvram_params(void)
{
    uint32_t uc_param_idx;
    g_nvram_init_params[NVRAM_PARAMS_INDEX_0] = 0x0000F6F6;
    g_nvram_init_params[NVRAM_PARAMS_INDEX_1] = 0xFBE7F1FB;
    g_nvram_init_params[NVRAM_PARAMS_INDEX_2] = 0xE7F1F1FB;
    g_nvram_init_params[NVRAM_PARAMS_INDEX_3] = 0xECF6F6D8;
    g_nvram_init_params[NVRAM_PARAMS_INDEX_4] = 0xD8D8E2EC;
    g_nvram_init_params[NVRAM_PARAMS_INDEX_5] = 0x000000E2;
    g_nvram_init_params[NVRAM_PARAMS_INDEX_6] = 0x0000F1F6;
    g_nvram_init_params[NVRAM_PARAMS_INDEX_7] = 0xE2ECF600;
    g_nvram_init_params[NVRAM_PARAMS_INDEX_8] = 0xF1FBFBFB;
    g_nvram_init_params[NVRAM_PARAMS_INDEX_9] = 0x00F1D3EA;
    g_nvram_init_params[NVRAM_PARAMS_INDEX_10] = 0xE7EC0000;
    g_nvram_init_params[NVRAM_PARAMS_INDEX_11] = 0xC9CED3CE;
    /*  2.4g 5g 20M mcs9 */
    g_nvram_init_params[NVRAM_PARAMS_INDEX_12] = 0xD8DDCED3;
    g_nvram_init_params[NVRAM_PARAMS_INDEX_13] = 0xC9C9CED3;
    g_nvram_init_params[NVRAM_PARAMS_INDEX_14] = 0x000000C4;
    g_nvram_init_params[NVRAM_PARAMS_INDEX_15] = 0xEC000000;
    g_nvram_init_params[NVRAM_PARAMS_INDEX_16] = 0xC9CECEE7;
    g_nvram_init_params[NVRAM_PARAMS_INDEX_17] = 0x000000C4;
    /* DPD 打开时高阶速率功率 */
    g_nvram_init_params[NVRAM_PARAMS_INDEX_DPD_0] = 0xE2ECEC00;
    g_nvram_init_params[NVRAM_PARAMS_INDEX_DPD_1] = 0xE2E200E2;
    g_nvram_init_params[NVRAM_PARAMS_INDEX_DPD_2] = 0x0000C4C4;
    /* 11B和OFDM功率差 */
    g_nvram_init_params[NVRAM_PARAMS_INDEX_11B_OFDM_DELT_POW] = 0xA0A00000;
    /* 5G功率和IQ校准UPC上限值 */
    g_nvram_init_params[NVRAM_PARAMS_INDEX_IQ_MAX_UPC] = 0xD8D83030;
    for (uc_param_idx = NVRAM_PARAMS_FCC_START_INDEX;
         uc_param_idx < NVRAM_PARAMS_SAR_C1_END_INDEX_BUTT; uc_param_idx++) {
        /* FCC/CE/SAR功率认证 */
        g_nvram_init_params[uc_param_idx] = 0xFFFFFFFF;
    }
#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
    g_nvram_init_params[NVRAM_PARAMS_TAS_ANT_SWITCH_EN] = 0x0;
    g_nvram_init_params[NVRAM_PARAMS_TAS_PWR_CTRL] = 0x0;
#endif
    g_nvram_init_params[NVRAM_PARAMS_5G_FCC_CE_HIGH_BAND_MAX_PWR] = 0x00FA00FA;
    g_nvram_init_params[NVRAM_PARAMS_INDEX_IQ_LPF_LVL] = 0x00001111;
}

OAL_STATIC void hwifi_config_init_default_max_txpwr_base(uint8_t *base_params, uint8_t param_num)
{
    uint8_t param_idx;
    /* 失败默认使用初始值 */
    for (param_idx = 0; param_idx < param_num; param_idx++) {
        *(base_params + param_idx) = CUS_MAX_BASE_TXPOWER_VAL;
    }
}

/*
 * 函 数 名  : hwifi_get_max_txpwr_base
 * 功能描述  : 获取定制化基准发射功率
 */
OAL_STATIC void hwifi_get_max_txpwr_base(const char *tag, uint8_t base_idx, uint8_t *base_params, uint8_t param_num)
{
    uint8_t param_idx;
    int32_t l_ret;
    uint8_t *base_pwr_params = NULL;
    uint16_t per_param_num = 0;
    int32_t l_nv_params[DY_CALI_NUM_5G_BAND] = {0};
    uint8_t val;

    base_pwr_params = (uint8_t *)os_kzalloc_gfp(CUS_PARAMS_LEN_MAX * sizeof(uint8_t));
    if (base_pwr_params == NULL) {
        oam_error_log0(0, OAM_SF_CUSTOM, "hwifi_get_max_txpwr_base::base_pwr_params mem alloc fail!");
        hwifi_config_init_default_max_txpwr_base(base_params, param_num);
        return;
    }

    memset_s(base_pwr_params, CUS_PARAMS_LEN_MAX * sizeof(uint8_t), 0, CUS_PARAMS_LEN_MAX * sizeof(uint8_t));
    l_ret = get_cust_conf_string(tag, g_nvram_config_ini[base_idx].name, base_pwr_params, CUS_PARAMS_LEN_MAX - 1);
    if (l_ret != INI_SUCC) {
        oam_error_log2(0, OAM_SF_CUSTOM, "hwifi_get_max_txpwr_base::read %dth failed ret[%d]!", base_idx, l_ret);
        hwifi_config_init_default_max_txpwr_base(base_params, param_num);
        os_mem_kfree(base_pwr_params);
        return;
    }

    if ((hwifi_config_sepa_coefficient_from_param(base_pwr_params, l_nv_params, &per_param_num, param_num) == OAL_SUCC)
        && (per_param_num == param_num)) {
        /* 参数合理性检查 */
        for (param_idx = 0; param_idx < param_num; param_idx++) {
            if (cus_val_invalid(l_nv_params[param_idx], CUS_MAX_BASE_TXPOWER_VAL, CUS_MIN_BASE_TXPOWER_VAL)) {
                oam_error_log3(0, OAM_SF_CUSTOM,
                    "hwifi_get_max_txpwr_base read %dth from ini val[%d] out of range replaced by [%d]!",
                    base_idx, l_nv_params[param_idx], CUS_MAX_BASE_TXPOWER_VAL);
                val = CUS_MAX_BASE_TXPOWER_VAL;
            } else {
                val = (uint8_t)l_nv_params[param_idx];
            }
            *(base_params + param_idx) = val;
        }
    }

    os_mem_kfree(base_pwr_params);
}

/*
 * 函 数 名  : hwifi_config_fcc_ce_5g_high_band_txpwr_nvram
 * 功能描述  : FCC/CE 5G高band认证
 */
OAL_STATIC void hwifi_config_fcc_ce_5g_high_band_txpwr_nvram(regdomain_enum regdomain_type)
{
    uint8_t uc_5g_max_pwr_for_high_band;
    int32_t l_val = g_nvram_init_params[NVRAM_PARAMS_5G_FCC_CE_HIGH_BAND_MAX_PWR];
    wlan_cust_nvram_params *pst_g_cust_nv_params = hwifi_get_nvram_params();

    /* FCC/CE 5G 高band的最大发射功率 */
    if (get_cust_conf_int32(INI_MODU_WIFI,
                            g_nvram_config_ini[NVRAM_PARAMS_5G_FCC_CE_HIGH_BAND_MAX_PWR].name,
                            &l_val) != INI_SUCC) {
        /* 读取失败时,使用初始值 */
        l_val = g_nvram_init_params[NVRAM_PARAMS_5G_FCC_CE_HIGH_BAND_MAX_PWR];
    }

    uc_5g_max_pwr_for_high_band = (uint8_t)((regdomain_type == REGDOMAIN_ETSI) ?
                                              cus_get_low_16bits(l_val) : cus_get_high_16bits(l_val));
    /* 参数有效性检查 */
    if (cus_val_invalid(uc_5g_max_pwr_for_high_band, CUS_MAX_BASE_TXPOWER_VAL, CUS_MIN_BASE_TXPOWER_VAL)) {
        oam_warning_log1(0, OAM_SF_CFG, "hwifi_config_init_nvram read 5g_max_pow_high_band[%d] failed!", l_val);
        uc_5g_max_pwr_for_high_band = CUS_MAX_BASE_TXPOWER_VAL;
    }

    pst_g_cust_nv_params->uc_5g_max_pwr_fcc_ce_for_high_band = uc_5g_max_pwr_for_high_band;
}

OAL_STATIC void hwifi_config_get_fcc_ce_nvram_idx(uint8_t chain_idx, regdomain_enum regdomain_type,
    uint8_t *start_idx, uint8_t *end_idx)
{
    if (chain_idx == WLAN_RF_CHANNEL_ZERO) {
        if (regdomain_type == REGDOMAIN_ETSI) {
            *start_idx = NVRAM_PARAMS_CE_START_INDEX;
            *end_idx = NVRAM_PARAMS_CE_END_INDEX_BUTT;
        } else {
            *start_idx = NVRAM_PARAMS_FCC_START_INDEX;
            *end_idx = NVRAM_PARAMS_FCC_END_INDEX_BUTT;
        }
    } else {
        if (regdomain_type == REGDOMAIN_ETSI) {
            *start_idx = NVRAM_PARAMS_CE_C1_START_INDEX;
            *end_idx = NVRAM_PARAMS_CE_C1_END_INDEX_BUTT;
        } else {
            *start_idx = NVRAM_PARAMS_FCC_C1_START_INDEX;
            *end_idx = NVRAM_PARAMS_FCC_C1_END_INDEX_BUTT;
        }
    }
}

OAL_STATIC int32_t hwifi_config_init_fcc_ce_txpwr_nvram_5g(int32_t *nv_val, uint8_t chn_idx)
{
    wlan_cust_nvram_params *cust_nv_params = hwifi_get_nvram_params();  /* 最大发送功率定制化数组 */
    wlan_cust_cfg_custom_fcc_ce_txpwr_limit_stru *fcc_ce_param = &cust_nv_params->ast_fcc_ce_param[chn_idx];
    int32_t l_ret;

    /* 5g */
    l_ret = memcpy_s(fcc_ce_param->auc_5g_fcc_txpwr_limit_params_20m,
        sizeof(fcc_ce_param->auc_5g_fcc_txpwr_limit_params_20m), nv_val, sizeof(int32_t));
    nv_val++;
    /* 偏移已经拷贝过数据的4字节，剩余长度是buff总长度减去已拷贝过数据的6-4=2字节 */
    l_ret += memcpy_s(fcc_ce_param->auc_5g_fcc_txpwr_limit_params_20m + sizeof(int32_t),
        (sizeof(fcc_ce_param->auc_5g_fcc_txpwr_limit_params_20m) - sizeof(int32_t)),
        nv_val, 2 * sizeof(uint8_t)); /* 2字节 */
    nv_val++;
    l_ret += memcpy_s(fcc_ce_param->auc_5g_fcc_txpwr_limit_params_40m,
        sizeof(fcc_ce_param->auc_5g_fcc_txpwr_limit_params_40m), nv_val, sizeof(int32_t));
    nv_val++;
    /* 偏移已经拷贝过数据的4字节，剩余长度是buff总长度减去已拷贝过数据的6-4=2字节 */
    l_ret += memcpy_s(fcc_ce_param->auc_5g_fcc_txpwr_limit_params_40m + sizeof(int32_t),
        (sizeof(fcc_ce_param->auc_5g_fcc_txpwr_limit_params_40m) - sizeof(int32_t)),
        nv_val, 2 * sizeof(uint8_t)); /* 2字节 */
    nv_val++;
    l_ret += memcpy_s(fcc_ce_param->auc_5g_fcc_txpwr_limit_params_80m,
        sizeof(fcc_ce_param->auc_5g_fcc_txpwr_limit_params_80m), nv_val, sizeof(int32_t));
    nv_val++;
    l_ret += memcpy_s(fcc_ce_param->auc_5g_fcc_txpwr_limit_params_80m + sizeof(int32_t),
        (sizeof(fcc_ce_param->auc_5g_fcc_txpwr_limit_params_80m) - sizeof(int32_t)), nv_val, sizeof(uint8_t));
    nv_val++;
    l_ret += memcpy_s(fcc_ce_param->auc_5g_fcc_txpwr_limit_params_160m,
        sizeof(fcc_ce_param->auc_5g_fcc_txpwr_limit_params_160m), nv_val, CUS_NUM_5G_160M_SIDE_BAND * sizeof(uint8_t));
    return l_ret;
}

OAL_STATIC int32_t hwifi_config_init_fcc_ce_txpwr_nvram_2g(int32_t *nv_val, uint8_t chn_idx)
{
    wlan_cust_nvram_params *cust_nv_params = hwifi_get_nvram_params();  /* 最大发送功率定制化数组 */
    wlan_cust_cfg_custom_fcc_ce_txpwr_limit_stru *fcc_ce_param = &cust_nv_params->ast_fcc_ce_param[chn_idx];
    int32_t l_ret;
    uint8_t cfg_id;

    for (cfg_id = 0; cfg_id < MAC_2G_CHANNEL_NUM; cfg_id++) {
        nv_val++;
        l_ret = memcpy_s(fcc_ce_param->auc_2g_fcc_txpwr_limit_params[cfg_id],
            CUS_NUM_FCC_CE_2G_PRO * sizeof(uint8_t), nv_val, CUS_NUM_FCC_CE_2G_PRO * sizeof(uint8_t));
        if (l_ret != EOK) {
            return l_ret;
        }
    }
    return EOK;
}

/*
 * 函 数 名  : hwifi_config_init_fcc_ce_txpwr_nvram
 * 功能描述  : FCC/CE认证
 */
OAL_STATIC int32_t hwifi_config_init_fcc_ce_txpwr_nvram(uint8_t chn_idx)
{
    int32_t l_ret;
    uint8_t cfg_id;
    uint8_t param_idx = 0;
    int32_t *nv_val = NULL;
    regdomain_enum regdomain_type = hwifi_get_regdomain_from_country_code((uint8_t *)g_wifi_country_code);
    uint8_t param_len;
    uint8_t start_idx = 0;
    uint8_t end_idx = 0;

    /* 根据管制域信息选择下发FCC还是CE参数 */
    hwifi_config_fcc_ce_5g_high_band_txpwr_nvram(regdomain_type);

    hwifi_config_get_fcc_ce_nvram_idx(chn_idx, regdomain_type, &start_idx, &end_idx);

    param_len = (end_idx - start_idx) * sizeof(int32_t);
    nv_val = (int32_t *)os_kzalloc_gfp(param_len);
    if (nv_val == NULL) {
        oam_error_log0(0, OAM_SF_CUSTOM, "hwifi_config_init_fcc_txpwr_nvram::nv_val mem alloc fail!");
        return INI_FAILED;
    }
    memset_s(nv_val, param_len, 0, param_len);

    for (cfg_id = start_idx; cfg_id < end_idx; cfg_id++) {
        l_ret = get_cust_conf_int32(INI_MODU_WIFI, g_nvram_config_ini[cfg_id].name, nv_val + param_idx);
        oal_io_print("{hwifi_config_init_fcc_txpwr_nvram params[%d]=0x%x!\r\n}", param_idx, nv_val[param_idx]);

        if (l_ret != INI_SUCC) {
            oam_warning_log1(0, OAM_SF_CFG, "hwifi_config_init_nvram read id[%d] from ini failed!", cfg_id);
            /* 读取失败时,使用初始值 */
            nv_val[param_idx] = g_nvram_init_params[cfg_id];
        }
        param_idx++;
    }

    l_ret = hwifi_config_init_fcc_ce_txpwr_nvram_5g(nv_val, chn_idx);
    if (l_ret != EOK) {
        oam_error_log1(0, OAM_SF_CUSTOM, "hwifi_config_init_fcc_txpwr_nvram::5G memcpy_s fail[%d]!", l_ret);
        os_mem_kfree(nv_val);
        return INI_FAILED;
    }

    // 5G fcc para[0] - para[6], 2G fcc para[7] - para[19]
    l_ret = hwifi_config_init_fcc_ce_txpwr_nvram_2g(nv_val + (NVRAM_PARAMS_INDEX_CE_6 - NVRAM_PARAMS_INDEX_CE_0),
        chn_idx);
    if (l_ret != EOK) {
        oam_error_log1(0, OAM_SF_CUSTOM, "hwifi_config_init_fcc_txpwr_nvram::2G memcpy_s fail[%d]!", l_ret);
        os_mem_kfree(nv_val);
        return INI_FAILED;
    }

    os_mem_kfree(nv_val);
    return INI_SUCC;
}

/*
 * 函 数 名  : hwifi_config_check_sar_ctrl_nvram
 * 功能描述  : 降SAR参数检查
 */
OAL_STATIC void hwifi_config_check_sar_ctrl_nvram(uint8_t *puc_nvram_params, uint8_t uc_cfg_id,
                                                  uint8_t uc_band_id, uint8_t uc_chn_idx)
{
    uint8_t uc_sar_lvl_idx;
    wlan_init_cust_nvram_params *pst_g_cust_nv_params = hwifi_get_init_nvram_params();

    for (uc_sar_lvl_idx = 0; uc_sar_lvl_idx < CUS_NUM_OF_SAR_ONE_PARAM_NUM; uc_sar_lvl_idx++) {
        /* 定制项检查 */
        if (puc_nvram_params[uc_sar_lvl_idx] <= CUS_MIN_OF_SAR_VAL) {
            oam_error_log4(0, OAM_SF_CUSTOM, "hwifi_config_check_sar_ctrl_nvram::uc_cfg_id[%d]uc_band_id[%d] val[%d] \
                abnormal check ini file for chn[%d]!", uc_cfg_id, uc_band_id, puc_nvram_params[uc_sar_lvl_idx],
                uc_chn_idx);
            puc_nvram_params[uc_sar_lvl_idx] = 0xFF;
        }

        if (uc_chn_idx == WLAN_RF_CHANNEL_ZERO) {
            pst_g_cust_nv_params->st_sar_ctrl_params[uc_sar_lvl_idx + uc_cfg_id *
                CUS_NUM_OF_SAR_ONE_PARAM_NUM][uc_band_id].auc_sar_ctrl_params_c0 = puc_nvram_params[uc_sar_lvl_idx];
        } else {
            pst_g_cust_nv_params->st_sar_ctrl_params[uc_sar_lvl_idx + uc_cfg_id *
                CUS_NUM_OF_SAR_ONE_PARAM_NUM][uc_band_id].auc_sar_ctrl_params_c1 = puc_nvram_params[uc_sar_lvl_idx];
        }
    }
}

/*
 * 函 数 名  : hwifi_config_init_sar_ctrl_nvram
 * 功能描述  : 降SAR
 */
OAL_STATIC int32_t hwifi_config_init_sar_ctrl_nvram(uint8_t uc_chn_idx)
{
    int32_t  l_ret = INI_FAILED;
    uint8_t  uc_cfg_id;
    uint8_t  uc_band_id;
    uint8_t  uc_cus_id = uc_chn_idx == WLAN_RF_CHANNEL_ZERO ? NVRAM_PARAMS_SAR_START_INDEX :
                                                                NVRAM_PARAMS_SAR_C1_START_INDEX;
    uint32_t nvram_params = 0;
    uint8_t  auc_nvram_params[CUS_NUM_OF_SAR_ONE_PARAM_NUM];

    for (uc_cfg_id = 0; uc_cfg_id < CUS_NUM_OF_SAR_PER_BAND_PAR_NUM; uc_cfg_id++) {
        for (uc_band_id = 0; uc_band_id < CUS_NUM_OF_SAR_PARAMS; uc_band_id++) {
            l_ret = get_cust_conf_int32(INI_MODU_WIFI, g_nvram_config_ini[uc_cus_id].name, &nvram_params);
            if (l_ret != INI_SUCC) {
                oam_warning_log1(0, OAM_SF_CUSTOM, "hwifi_config_init_sar_ctrl_nvram read id[%d] from ini failed!",
                                 uc_cus_id);
                /* 读取失败时,使用初始值 */
                nvram_params = 0xFFFFFFFF;
            }
            oal_io_print("{hwifi_config_init_sar_ctrl_nvram::chn[%d] params %s 0x%x!\r\n}",
                         uc_chn_idx, g_nvram_config_ini[uc_cus_id].name, nvram_params);
            if (memcpy_s(auc_nvram_params, sizeof(auc_nvram_params),
                         &nvram_params, sizeof(nvram_params)) != EOK) {
                oam_error_log3(0, OAM_SF_CUSTOM,
                               "hwifi_config_init_sar_ctrl_nvram::uc_cfg_id[%d]band_id[%d]param[%d] set failed!",
                               uc_cfg_id, uc_band_id, nvram_params);
                return INI_FAILED;
            }

            /* 定制项检查 */
            hwifi_config_check_sar_ctrl_nvram(auc_nvram_params, uc_cfg_id, uc_band_id, uc_chn_idx);
            uc_cus_id++;
        }
    }
    return INI_SUCC;
}

/*
 * 函 数 名  : hwifi_get_max_txpwr_base_delt_val
 * 功能描述  : 获取mimo定制化基准发射功率差
 */
static void hwifi_get_max_txpwr_base_delt_val(uint8_t uc_cus_id,
                                              int8_t *pc_txpwr_base_delt_params,
                                              uint8_t uc_param_num)
{
    uint8_t  uc_param_idx;
    int32_t  nvram_params  = 0;

    memset_s(pc_txpwr_base_delt_params, uc_param_num, 0, uc_param_num);
    if (INI_SUCC == get_cust_conf_int32(INI_MODU_WIFI, g_nvram_config_ini[uc_cus_id].name, &nvram_params)) {
        if (memcpy_s(pc_txpwr_base_delt_params, uc_param_num, &nvram_params, sizeof(nvram_params)) != EOK) {
            oam_error_log0(0, OAM_SF_CUSTOM, "hwifi_get_max_txpwr_base_delt_val::copy failed!");
            return;
        }

        /* 参数合理性检查 */
        for (uc_param_idx = 0; uc_param_idx < uc_param_num; uc_param_idx++) {
            if (cus_val_invalid(pc_txpwr_base_delt_params[uc_param_idx], 0, CUS_MAX_BASE_TXPOWER_DELT_VAL_MIN)) {
                        oam_error_log1(0, OAM_SF_CUSTOM,
                                       "hwifi_get_max_txpwr_base_delt_val::ini val[%d] out of range replaced by zero!",
                                       pc_txpwr_base_delt_params[uc_param_idx]);
                        pc_txpwr_base_delt_params[uc_param_idx] = 0;
            }
        }
    }

    return;
}

/*
 * 函 数 名  : hwifi_config_init_iq_lpf_nvram_param
 * 功能描述  : iq校准lpf档位
 */
OAL_STATIC void hwifi_config_init_iq_lpf_nvram_param(void)
{
    int32_t l_nvram_params = 0;
    int32_t l_cfg_id = NVRAM_PARAMS_INDEX_IQ_LPF_LVL;
    int32_t l_ret;
    wlan_cust_nvram_params *pst_g_cust_nv_params = hwifi_get_nvram_params();

    l_ret = get_cust_conf_int32(INI_MODU_WIFI, g_nvram_config_ini[l_cfg_id].name, &l_nvram_params);
    if (l_ret != INI_SUCC) {
        oam_warning_log1(0, OAM_SF_CFG, "hwifi_config_init_iq_lpf_nvram_param read id[%d] from ini failed!", l_cfg_id);

        /* 读取失败时,使用初始值 */
        l_nvram_params = g_nvram_init_params[l_cfg_id];
    }

    /*  5g iq cali lvl  */
    l_ret += memcpy_s(pst_g_cust_nv_params->auc_5g_iq_cali_lpf_params,
                      NUM_OF_NV_5G_LPF_LVL, &l_nvram_params, sizeof(l_nvram_params));
    if (l_ret != EOK) {
        oam_error_log0(0, OAM_SF_CFG, "hwifi_config_init_iq_lpf_nvram_param memcpy failed!");
        return;
    }
}

#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
/*
 * 函 数 名  : hwifi_config_init_tas_ctrl_nvram
 * 功能描述  : tas发射功率
 */
OAL_STATIC int32_t hwifi_config_init_tas_ctrl_nvram(void)
{
    int32_t l_ret;
    uint8_t uc_band_idx;
    uint8_t uc_rf_idx;
    uint32_t nvram_params = 0;
    int8_t ac_tas_ctrl_params[WLAN_BAND_BUTT][WLAN_RF_CHANNEL_NUMS] = {{0}};
    wlan_cust_nvram_params *pst_g_cust_nv_params = hwifi_get_nvram_params();

    l_ret = get_cust_conf_int32(INI_MODU_WIFI, g_nvram_config_ini[NVRAM_PARAMS_TAS_ANT_SWITCH_EN].name,
                                &nvram_params);
    if (l_ret == INI_SUCC) {
        oam_warning_log1(0, OAM_SF_CUSTOM, "hwifi_config_init_tas_ctrl_nvram g_tas_switch_en[%d]!", nvram_params);
        g_tas_switch_en[WLAN_RF_CHANNEL_ZERO] = (oal_bool_enum_uint8)cus_get_low_16bits(nvram_params);
        g_tas_switch_en[WLAN_RF_CHANNEL_ONE] = (oal_bool_enum_uint8)cus_get_high_16bits(nvram_params);
    }

    l_ret = get_cust_conf_int32(INI_MODU_WIFI, g_nvram_config_ini[NVRAM_PARAMS_TAS_PWR_CTRL].name, &nvram_params);
    oal_io_print("{hwifi_config_init_tas_ctrl_nvram params[%d]=0x%x!\r\n}", NVRAM_PARAMS_TAS_PWR_CTRL, nvram_params);
    if (l_ret != INI_SUCC) {
        oam_warning_log1(0, OAM_SF_CUSTOM, "hwifi_config_init_tas_ctrl_nvram read id[%d] from ini failed!",
                         NVRAM_PARAMS_TAS_PWR_CTRL);
        /* 读取失败时,使用初始值 */
        nvram_params = (uint32_t)g_nvram_init_params[NVRAM_PARAMS_TAS_PWR_CTRL];
    }

    if (memcpy_s(ac_tas_ctrl_params, sizeof(ac_tas_ctrl_params),
                 &nvram_params, sizeof(nvram_params)) != EOK) {
        return INI_FAILED;
    }

    for (uc_band_idx = 0; uc_band_idx < WLAN_BAND_BUTT; uc_band_idx++) {
        for (uc_rf_idx = 0; uc_rf_idx < WLAN_RF_CHANNEL_NUMS; uc_rf_idx++) {
            if ((ac_tas_ctrl_params[uc_band_idx][uc_rf_idx] > CUS_MAX_OF_TAS_PWR_CTRL_VAL) ||
                (ac_tas_ctrl_params[uc_rf_idx][uc_rf_idx] < 0)) {
                oam_error_log4(0, OAM_SF_CUSTOM, "hwifi_config_init_tas_ctrl_nvram::band[%d] rf[%d] nvram_params[%d],\
                    val[%d] out of the \normal check ini file!", uc_band_idx, uc_rf_idx, nvram_params,
                    ac_tas_ctrl_params[uc_rf_idx][uc_rf_idx]);
                ac_tas_ctrl_params[uc_band_idx][uc_rf_idx] = 0;
            }
            pst_g_cust_nv_params->auc_tas_ctrl_params[uc_rf_idx][uc_band_idx] =
                (uint8_t)ac_tas_ctrl_params[uc_band_idx][uc_rf_idx];
        }
    }

    return INI_SUCC;
}
#endif

OAL_STATIC void hwifi_check_delt_ru_pow_val_5g(void)
{
    int32_t idx_one;
    int32_t idx_two;
    wlan_cust_nvram_params *nv_val = hwifi_get_nvram_params();

    for (idx_one = 0; idx_one < WLAN_BW_CAP_BUTT; idx_one++) {
        for (idx_two = 0; idx_two < WLAN_HE_RU_SIZE_BUTT; idx_two++) {
            if (cus_val_invalid(nv_val->ac_fullbandwidth_to_ru_power_5g[idx_one][idx_two],
                CUS_RU_DELT_POW_MAX, CUS_RU_DELT_POW_MIN)) {
                oam_error_log4(0, OAM_SF_CUSTOM,
                    "hwifi_get_delt_ru_pow_val_5g:cfg bw[%d]ru[%d] val[%d] invalid! max[%d]",
                    idx_one, idx_two, nv_val->ac_fullbandwidth_to_ru_power_5g[idx_one][idx_two], CUS_RU_DELT_POW_MAX);
                nv_val->ac_fullbandwidth_to_ru_power_5g[idx_one][idx_two] = 0;
            }
        }
    }
}


OAL_STATIC void hwifi_get_delt_ru_pow_val_5g(void)
{
    int32_t val = 0;
    int32_t ret;
    wlan_cust_nvram_params *nv_val = hwifi_get_nvram_params();

    /* 20m 26~242tone ru_power */
    if (get_cust_conf_int32(INI_MODU_WIFI, g_nvram_config_ini[NVRAM_CFG_TPC_RU_POWER_20M_5G].name, &val) != INI_SUCC) {
        val = g_nvram_init_params[NVRAM_CFG_TPC_RU_POWER_20M_5G];
    }
    ret = memcpy_s(nv_val->ac_fullbandwidth_to_ru_power_5g[WLAN_BW_CAP_20M], WLAN_HE_RU_SIZE_BUTT, &val, sizeof(val));

    /* 40m 26~242tone ru_power */
    if (get_cust_conf_int32(INI_MODU_WIFI, g_nvram_config_ini[NVRAM_CFG_TPC_RU_POWER_40M_5G].name, &val) != INI_SUCC) {
        val = g_nvram_init_params[NVRAM_CFG_TPC_RU_POWER_40M_5G];
    }
    ret += memcpy_s(nv_val->ac_fullbandwidth_to_ru_power_5g[WLAN_BW_CAP_40M], WLAN_HE_RU_SIZE_BUTT, &val, sizeof(val));

    /* 40m 484tone/80m 26~106tone ru_power */
    if (get_cust_conf_int32(INI_MODU_WIFI, g_nvram_config_ini[NVRAM_CFG_TPC_RU_POWER_80M_L_5G].name, &val)
        != INI_SUCC) {
        val = g_nvram_init_params[NVRAM_CFG_TPC_RU_POWER_80M_L_5G];
    }
    nv_val->ac_fullbandwidth_to_ru_power_5g[WLAN_BW_CAP_40M][WLAN_HE_RU_SIZE_484] = cus_get_first_byte(val);
    nv_val->ac_fullbandwidth_to_ru_power_5g[WLAN_BW_CAP_80M][WLAN_HE_RU_SIZE_26] = cus_get_second_byte(val);
    nv_val->ac_fullbandwidth_to_ru_power_5g[WLAN_BW_CAP_80M][WLAN_HE_RU_SIZE_52] = cus_get_third_byte(val);
    nv_val->ac_fullbandwidth_to_ru_power_5g[WLAN_BW_CAP_80M][WLAN_HE_RU_SIZE_106] = cus_get_fourth_byte(val);

    /* 80m 242~996tone/160m 26tone ru_power */
    if (get_cust_conf_int32(INI_MODU_WIFI, g_nvram_config_ini[NVRAM_CFG_TPC_RU_POWER_80M_H_5G].name, &val)
        != INI_SUCC) {
        val = g_nvram_init_params[NVRAM_CFG_TPC_RU_POWER_80M_H_5G];
    }
    nv_val->ac_fullbandwidth_to_ru_power_5g[WLAN_BW_CAP_80M][WLAN_HE_RU_SIZE_242] = cus_get_first_byte(val);
    nv_val->ac_fullbandwidth_to_ru_power_5g[WLAN_BW_CAP_80M][WLAN_HE_RU_SIZE_484] = cus_get_second_byte(val);
    nv_val->ac_fullbandwidth_to_ru_power_5g[WLAN_BW_CAP_80M][WLAN_HE_RU_SIZE_996] = cus_get_third_byte(val);
    nv_val->ac_fullbandwidth_to_ru_power_5g[WLAN_BW_CAP_160M][WLAN_HE_RU_SIZE_26] = cus_get_fourth_byte(val);

    /* 160m 52~996tone ru_power */
    if (get_cust_conf_int32(INI_MODU_WIFI, g_nvram_config_ini[NVRAM_CFG_TPC_RU_POWER_160M_L_5G].name, &val)
        != INI_SUCC) {
        val = g_nvram_init_params[NVRAM_CFG_TPC_RU_POWER_160M_L_5G];
    }
    ret += memcpy_s(&(nv_val->ac_fullbandwidth_to_ru_power_5g[WLAN_BW_CAP_160M][WLAN_HE_RU_SIZE_52]),
        (WLAN_HE_RU_SIZE_BUTT - WLAN_HE_RU_SIZE_52), &val, sizeof(val));
    if (get_cust_conf_int32(INI_MODU_WIFI, g_nvram_config_ini[NVRAM_CFG_TPC_RU_POWER_160M_H_5G].name, &val)
        != INI_SUCC) {
        val = g_nvram_init_params[NVRAM_CFG_TPC_RU_POWER_160M_H_5G];
    }
    ret += memcpy_s(&(nv_val->ac_fullbandwidth_to_ru_power_5g[WLAN_BW_CAP_160M][WLAN_HE_RU_SIZE_996]),
        (WLAN_HE_RU_SIZE_BUTT - WLAN_HE_RU_SIZE_996), &val, sizeof(uint8_t)*2); /* 2字节 */
    if (ret != EOK) {
        oam_error_log0(0, OAM_SF_CUSTOM, "hwifi_get_delt_ru_pow_val_5g::memcpy failed!");
        memset_s(nv_val->ac_fullbandwidth_to_ru_power_5g, sizeof(int8_t) * WLAN_BW_CAP_BUTT * WLAN_HE_RU_SIZE_BUTT, 0,
            sizeof(int8_t)*WLAN_BW_CAP_BUTT * WLAN_HE_RU_SIZE_BUTT);
        return;
    }

    /* 参数有效性检查 */
    hwifi_check_delt_ru_pow_val_5g();
}


static void hwifi_get_delt_ru_pow_val_2g(void)
{
    int32_t l_nvram_params = 0;
    int32_t l_ret;
    int32_t l_cfg_idx_one;
    int32_t l_cfg_idx_two;
    wlan_cust_nvram_params *pst_g_cust_nv_params = hwifi_get_nvram_params();

    /* 20m 26~242tone ru_power */
    if (get_cust_conf_int32(INI_MODU_WIFI, g_nvram_config_ini[NVRAM_CFG_TPC_RU_POWER_20M_2G].name, &l_nvram_params)
        != INI_SUCC) {
        l_nvram_params = g_nvram_init_params[NVRAM_CFG_TPC_RU_POWER_20M_2G];
    }
    l_ret = memcpy_s(pst_g_cust_nv_params->ac_fullbandwidth_to_ru_power_2g[WLAN_BW_CAP_20M], WLAN_HE_RU_SIZE_996,
                     &l_nvram_params, sizeof(l_nvram_params));

    /* 40m 26~242tone ru_power */
    if (get_cust_conf_int32(INI_MODU_WIFI, g_nvram_config_ini[NVRAM_CFG_TPC_RU_POWER_40M_0_2G].name, &l_nvram_params)
        != INI_SUCC) {
        l_nvram_params = g_nvram_init_params[NVRAM_CFG_TPC_RU_POWER_40M_0_2G];
    }
    l_ret += memcpy_s(pst_g_cust_nv_params->ac_fullbandwidth_to_ru_power_2g[WLAN_BW_CAP_40M], WLAN_HE_RU_SIZE_996,
                      &l_nvram_params, sizeof(l_nvram_params));

    /* 40m 484tone ru_power */
    if (get_cust_conf_int32(INI_MODU_WIFI, g_nvram_config_ini[NVRAM_CFG_TPC_RU_POWER_40M_1_2G].name, &l_nvram_params)
        != INI_SUCC) {
        l_nvram_params = g_nvram_init_params[NVRAM_CFG_TPC_RU_POWER_40M_1_2G];
    }
    pst_g_cust_nv_params->ac_fullbandwidth_to_ru_power_2g[WLAN_BW_CAP_40M][WLAN_HE_RU_SIZE_484]
        = cus_get_first_byte(l_nvram_params);

    if (l_ret != EOK) {
        oam_error_log0(0, OAM_SF_CUSTOM, "hwifi_get_delt_ru_pow_val_2g::memcpy failed!");
        memset_s(pst_g_cust_nv_params->ac_fullbandwidth_to_ru_power_5g,
                 sizeof(int8_t) * WLAN_BW_CAP_80M * WLAN_HE_RU_SIZE_996,
                 0, sizeof(int8_t) * WLAN_BW_CAP_80M * WLAN_HE_RU_SIZE_996);
        return;
    }

    /* 参数有效性检查 */
    for (l_cfg_idx_one = 0; l_cfg_idx_one < WLAN_BW_CAP_80M; l_cfg_idx_one++) {
        for (l_cfg_idx_two = 0; l_cfg_idx_two < WLAN_HE_RU_SIZE_996; l_cfg_idx_two++) {
            if (cus_val_invalid(pst_g_cust_nv_params->ac_fullbandwidth_to_ru_power_2g[l_cfg_idx_one][l_cfg_idx_two],
                                CUS_RU_DELT_POW_MAX, CUS_RU_DELT_POW_MIN)) {
                oam_error_log4(0, OAM_SF_CUSTOM,
                               "hwifi_get_delt_ru_pow_val_2g:cfg bw[%d]ru[%d] val[%d] invalid! max[%d]",
                               l_cfg_idx_one, l_cfg_idx_two,
                               pst_g_cust_nv_params->ac_fullbandwidth_to_ru_power_2g[l_cfg_idx_one][l_cfg_idx_two],
                               CUS_RU_DELT_POW_MAX);
                pst_g_cust_nv_params->ac_fullbandwidth_to_ru_power_2g[l_cfg_idx_one][l_cfg_idx_two] = 0;
            }
        }
    }

    return;
}
#if defined(_PRE_WLAN_FEATURE_11AX)

static int32_t hwifi_get_5g_ru_max_ru_pow_val(wlan_cust_nvram_params *g_cust_nv_params)
{
    int32_t nvram_params = 0;
    int32_t ret = 0;
    uint32_t idx;
    uint32_t band_idx;
    uint32_t nvram_idx;
    regdomain_enum_uint8 regdomain_type;

    /* 根据管制域信息选择下发FCC还是CE参数 */
    regdomain_type = hwifi_get_regdomain_from_country_code((uint8_t *)g_wifi_country_code);
    nvram_idx = (regdomain_type == REGDOMAIN_ETSI) ? NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_MIMO_B1_CE : \
                (regdomain_type == REGDOMAIN_FCC) ? NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_MIMO_B1_FCC : \
                NVRAM_CFG_TPC_RU_MAX_POWER_0_5G_MIMO_B1;

    for (band_idx = 0; band_idx < MAC_NUM_5G_BAND; band_idx++) {
        for (idx = CUS_POW_TX_CHAIN_MIMO; idx < CUS_POW_TX_CHAIN_BUTT; idx++) {
            if (get_cust_conf_int32(INI_MODU_WIFI, g_nvram_config_ini[nvram_idx].name, &nvram_params) != INI_SUCC) {
                nvram_params = 0xFFFFFFFF;
            }
            ret += memcpy_s(g_cust_nv_params->auc_tpc_tb_ru_5g_max_power[band_idx].auc_tb_ru_5g_max_power[idx],
                WLAN_HE_RU_SIZE_BUTT, &nvram_params, sizeof(nvram_params));
            nvram_idx++;
            if (get_cust_conf_int32(INI_MODU_WIFI, g_nvram_config_ini[nvram_idx].name, &nvram_params) != INI_SUCC) {
                nvram_params = 0xFFFFFF;
            }
            ret += memcpy_s(&g_cust_nv_params->auc_tpc_tb_ru_5g_max_power[band_idx].auc_tb_ru_5g_max_power\
                // 1992_996_484,3个ru的定制化项
                [idx][WLAN_HE_RU_SIZE_484], (WLAN_HE_RU_SIZE_BUTT - WLAN_HE_RU_SIZE_484),
                &nvram_params, 3 * sizeof(uint8_t));
            nvram_idx++;
        }
    }

    return ret;
}


static void hwifi_get_total_max_ru_pow_val(wlan_cust_nvram_params *pst_g_cust_nv_params)
{
    uint32_t idx;
    uint32_t nvram_idx;
    int32_t l_nvram_params = 0;

    nvram_idx = NVRAM_CFG_TPC_RU_MAX_POWER_2G;
    for (idx = WLAN_BAND_2G; idx < WLAN_BAND_BUTT; idx++) {
        if (get_cust_conf_int32(INI_MODU_WIFI, g_nvram_config_ini[nvram_idx].name, &l_nvram_params) != INI_SUCC) {
            l_nvram_params = 0xFF;
        }
        pst_g_cust_nv_params->auc_tpc_tb_ru_max_power[idx] = cus_get_first_byte(l_nvram_params);
        nvram_idx++;
    }
}


static void hwifi_get_max_ru_pow_val(void)
{
    int32_t l_nvram_params = 0;
    int32_t l_ret = 0;
    wlan_cust_nvram_params *cust_nvram_params = hwifi_get_nvram_params();
    uint32_t idx;
    uint32_t nvram_idx = NVRAM_CFG_TPC_RU_MAX_POWER_0_2G_MIMO;

    for (idx = CUS_POW_TX_CHAIN_MIMO; idx < CUS_POW_TX_CHAIN_BUTT; idx++) {
        if (get_cust_conf_int32(INI_MODU_WIFI, g_nvram_config_ini[nvram_idx].name, &l_nvram_params) != INI_SUCC) {
            l_nvram_params = 0xFFFFFFFF;
        }
        l_ret += memcpy_s(cust_nvram_params->auc_tpc_tb_ru_2g_max_power[idx], WLAN_HE_RU_SIZE_996,
                          &l_nvram_params, sizeof(l_nvram_params));
        nvram_idx++;
        if (get_cust_conf_int32(INI_MODU_WIFI, g_nvram_config_ini[nvram_idx].name, &l_nvram_params) != INI_SUCC) {
            l_nvram_params = 0xFF;
        }
        cust_nvram_params->auc_tpc_tb_ru_2g_max_power[idx][WLAN_HE_RU_SIZE_484] = cus_get_first_byte(l_nvram_params);
        nvram_idx++;
    }
    hwifi_get_5g_ru_max_ru_pow_val(cust_nvram_params);
    if (l_ret != EOK) {
        oam_error_log0(0, OAM_SF_CUSTOM, "hwifi_get_max_ru_pow_val::memcpy failed!");
        memset_s(cust_nvram_params->auc_tpc_tb_ru_2g_max_power, sizeof(cust_nvram_params->auc_tpc_tb_ru_2g_max_power),
                 0xFF, sizeof(cust_nvram_params->auc_tpc_tb_ru_2g_max_power));
        memset_s(cust_nvram_params->auc_tpc_tb_ru_5g_max_power, sizeof(cust_nvram_params->auc_tpc_tb_ru_5g_max_power),
                 0xFF, sizeof(cust_nvram_params->auc_tpc_tb_ru_5g_max_power));
    }

    hwifi_get_total_max_ru_pow_val(cust_nvram_params);
}
#endif


OAL_STATIC OAL_INLINE void hwifi_config_init_ru_pow_nvram(void)
{
    hwifi_get_delt_ru_pow_val_5g();
    hwifi_get_delt_ru_pow_val_2g();

#if defined(_PRE_WLAN_FEATURE_11AX)
    hwifi_get_max_ru_pow_val();
#endif
}
OAL_STATIC void hwifi_config_get_max_txpwr_base(void)
{
    wlan_cust_nvram_params *cust_nv_params = hwifi_get_nvram_params();

    hwifi_get_max_txpwr_base(INI_MODU_WIFI, NVRAM_PARAMS_INDEX_19,
        cust_nv_params->auc_2g_txpwr_base_params[WLAN_RF_CHANNEL_ZERO], CUS_BASE_PWR_NUM_2G);
    hwifi_get_max_txpwr_base(INI_MODU_WIFI, NVRAM_PARAMS_INDEX_20,
        cust_nv_params->auc_5g_txpwr_base_params[WLAN_RF_CHANNEL_ZERO], CUS_BASE_PWR_NUM_5G);
    hwifi_get_max_txpwr_base(INI_MODU_WIFI, NVRAM_PARAMS_INDEX_21,
        cust_nv_params->auc_2g_txpwr_base_params[WLAN_RF_CHANNEL_ONE], CUS_BASE_PWR_NUM_2G);
    hwifi_get_max_txpwr_base(INI_MODU_WIFI, NVRAM_PARAMS_INDEX_22,
        cust_nv_params->auc_5g_txpwr_base_params[WLAN_RF_CHANNEL_ONE], CUS_BASE_PWR_NUM_5G);
}

OAL_STATIC int32_t hwifi_config_init_max_txpwr_base(int32_t *nvram_params, uint32_t nv_len)
{
    wlan_cust_nvram_params *cust_nv_params = hwifi_get_nvram_params();
    int32_t l_ret;
    int32_t l_val;

    l_ret = memcpy_s(cust_nv_params->ac_delt_txpwr_params,
                     NUM_OF_NV_MAX_TXPOWER,
                     nvram_params,
                     NUM_OF_NV_MAX_TXPOWER);
    l_ret += memcpy_s(cust_nv_params->ac_dpd_delt_txpwr_params, NUM_OF_NV_DPD_MAX_TXPOWER,
        nvram_params + NVRAM_PARAMS_INDEX_DPD_0, NUM_OF_NV_DPD_MAX_TXPOWER);

    l_val = cus_get_low_16bits(*(nvram_params + NVRAM_PARAMS_INDEX_11B_OFDM_DELT_POW));
    l_ret += memcpy_s(cust_nv_params->ac_11b_delt_txpwr_params, NUM_OF_NV_11B_DELTA_TXPOWER,
                      &l_val, NUM_OF_NV_11B_DELTA_TXPOWER);
    /* FEM OFF IQ CALI POW */
    l_val = cus_get_high_16bits(*(nvram_params + NVRAM_PARAMS_INDEX_11B_OFDM_DELT_POW));
    l_ret += memcpy_s(cust_nv_params->auc_fem_off_iq_cal_pow_params,
                      sizeof(cust_nv_params->auc_fem_off_iq_cal_pow_params),
                      &l_val, sizeof(cust_nv_params->auc_fem_off_iq_cal_pow_params));

    l_ret += memcpy_s(cust_nv_params->auc_5g_upper_upc_params, NUM_OF_NV_5G_UPPER_UPC,
                      nvram_params + NVRAM_PARAMS_INDEX_IQ_MAX_UPC,
                      NUM_OF_NV_5G_UPPER_UPC);

    l_ret += memcpy_s(cust_nv_params->ac_2g_low_pow_amend_params, NUM_OF_NV_2G_LOW_POW_DELTA_VAL,
                      nvram_params + NVRAM_PARAMS_INDEX_2G_LOW_POW_AMEND,
                      NUM_OF_NV_2G_LOW_POW_DELTA_VAL);
    if (l_ret != EOK) {
        oam_error_log1(0, OAM_SF_CFG, "hwifi_config_init_nvram read from ini failed[%d]!", l_ret);
        return INI_FAILED;
    }
    return OAL_SUCC;
}

/*
 * 函 数 名  : hwifi_config_init_nvram
 * 功能描述  : handle nvram customize params
 */
int32_t hwifi_config_init_nvram(void)
{
    int32_t l_ret;
    int32_t l_cfg_id;
    int32_t nvram_params[NVRAM_PARAMS_TXPWR_INDEX_BUTT] = {0};
    wlan_cust_nvram_params *cust_nv_params = hwifi_get_nvram_params();

    memset_s(&g_cust_nv_params, sizeof(g_cust_nv_params), 0, sizeof(g_cust_nv_params));

    /* read nvm failed or data not exist or country_code updated, read ini:cust_spec > cust_common > default */
    /* find plat tag */
    for (l_cfg_id = NVRAM_PARAMS_INDEX_0; l_cfg_id < NVRAM_PARAMS_TXPWR_INDEX_BUTT; l_cfg_id++) {
        l_ret = get_cust_conf_int32(INI_MODU_WIFI, g_nvram_config_ini[l_cfg_id].name, &nvram_params[l_cfg_id]);
        oam_info_log2(0, OAM_SF_CFG, "{hwifi_config_init_nvram aul_nvram_params[%d]=0x%x!}",
                      l_cfg_id, nvram_params[l_cfg_id]);

        if (l_ret != INI_SUCC) {
            oam_warning_log1(0, OAM_SF_CFG, "hwifi_config_init_nvram read id[%d] from ini failed!", l_cfg_id);

            /* 读取失败时,使用初始值 */
            nvram_params[l_cfg_id] = g_nvram_init_params[l_cfg_id];
        }
    }

    l_ret = hwifi_config_init_max_txpwr_base(nvram_params, NVRAM_PARAMS_TXPWR_INDEX_BUTT);
    if (l_ret != OAL_SUCC) {
        return l_ret;
    }

    for (l_cfg_id = 0; l_cfg_id < NUM_OF_NV_2G_LOW_POW_DELTA_VAL; l_cfg_id++) {
        if (cus_abs(cust_nv_params->ac_2g_low_pow_amend_params[l_cfg_id]) > CUS_2G_LOW_POW_AMEND_ABS_VAL_MAX) {
            cust_nv_params->ac_2g_low_pow_amend_params[l_cfg_id] = 0;
        }
    }

    /* 基准功率 */
    hwifi_config_get_max_txpwr_base();

   /* FCC/CE/SAR */
    for (l_cfg_id = 0; l_cfg_id < WLAN_RF_CHANNEL_NUMS; l_cfg_id++) {
        hwifi_config_init_fcc_ce_txpwr_nvram(l_cfg_id);
        hwifi_config_init_sar_ctrl_nvram(l_cfg_id);
    }
    hwifi_get_max_txpwr_base_delt_val(NVRAM_PARAMS_INDEX_DELT_BASE_POWER_23,
        &cust_nv_params->ac_delt_txpwr_base_params[0][0], sizeof(cust_nv_params->ac_delt_txpwr_base_params));

    /* extend */
    hwifi_config_init_iq_lpf_nvram_param();

#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
    /* TAS */
    hwifi_config_init_tas_ctrl_nvram();
#endif
    hwifi_config_init_ru_pow_nvram();

    oam_info_log0(0, OAM_SF_CFG, "hwifi_config_init_nvram read from ini success!");
    return INI_SUCC;
}

