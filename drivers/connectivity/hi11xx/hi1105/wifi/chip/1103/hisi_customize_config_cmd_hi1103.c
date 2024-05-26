

/* 头文件包含 */
#include "hisi_customize_wifi_hi1103.h"
#include "hisi_customize_config_dts_hi1103.h"
#include "hisi_customize_config_priv_hi1103.h"
#include "hisi_customize_config_cmd_hi1103.h"

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
#define THIS_FILE_ID OAM_FILE_ID_HISI_CUSTOMIZE_CONFIG_CMD_HI1103_C

/* 全局变量定义 */
OAL_STATIC int32_t g_host_init_params[WLAN_CFG_INIT_BUTT] = {0}; /* ini定制化参数数组 */

OAL_STATIC wlan_cfg_cmd g_wifi_config_cmds[] = {
    /* ROAM */
    { "roam_switch",         WLAN_CFG_INIT_ROAM_SWITCH },
    { "scan_orthogonal",     WLAN_CFG_INIT_SCAN_ORTHOGONAL },
    { "trigger_b",           WLAN_CFG_INIT_TRIGGER_B },
    { "trigger_a",           WLAN_CFG_INIT_TRIGGER_A },
    { "delta_b",             WLAN_CFG_INIT_DELTA_B },
    { "delta_a",             WLAN_CFG_INIT_DELTA_A },
    { "dense_env_trigger_b", WLAN_CFG_INIT_DENSE_ENV_TRIGGER_B },
    { "dense_env_trigger_a", WLAN_CFG_INIT_DENSE_ENV_TRIGGER_A },
    { "scenario_enable",     WLAN_CFG_INIT_SCENARIO_ENABLE },
    { "candidate_good_rssi", WLAN_CFG_INIT_CANDIDATE_GOOD_RSSI },
    { "candidate_good_num",  WLAN_CFG_INIT_CANDIDATE_GOOD_NUM },
    { "candidate_weak_num",  WLAN_CFG_INIT_CANDIDATE_WEAK_NUM },
    { "interval_variable",   WLAN_CFG_INIT_INTERVAL_VARIABLE },

    /* 性能 */
    { "ampdu_tx_max_num",        WLAN_CFG_INIT_AMPDU_TX_MAX_NUM },
    { "used_mem_for_start",      WLAN_CFG_INIT_USED_MEM_FOR_START },
    { "used_mem_for_stop",       WLAN_CFG_INIT_USED_MEM_FOR_STOP },
    { "rx_ack_limit",            WLAN_CFG_INIT_RX_ACK_LIMIT },
    { "sdio_d2h_assemble_count", WLAN_CFG_INIT_SDIO_D2H_ASSEMBLE_COUNT },
    { "sdio_h2d_assemble_count", WLAN_CFG_INIT_SDIO_H2D_ASSEMBLE_COUNT },
    /* LINKLOSS */
    { "link_loss_threshold_bt",     WLAN_CFG_INIT_LINK_LOSS_THRESHOLD_BT },
    { "link_loss_threshold_dbac",   WLAN_CFG_INIT_LINK_LOSS_THRESHOLD_DBAC },
    { "link_loss_threshold_normal", WLAN_CFG_INIT_LINK_LOSS_THRESHOLD_NORMAL },
    /* 自动调频 */
    { "pss_threshold_level_0",  WLAN_CFG_INIT_PSS_THRESHOLD_LEVEL_0 },
    { "cpu_freq_limit_level_0", WLAN_CFG_INIT_CPU_FREQ_LIMIT_LEVEL_0 },
    { "ddr_freq_limit_level_0", WLAN_CFG_INIT_DDR_FREQ_LIMIT_LEVEL_0 },
    { "pss_threshold_level_1",  WLAN_CFG_INIT_PSS_THRESHOLD_LEVEL_1 },
    { "cpu_freq_limit_level_1", WLAN_CFG_INIT_CPU_FREQ_LIMIT_LEVEL_1 },
    { "ddr_freq_limit_level_1", WLAN_CFG_INIT_DDR_FREQ_LIMIT_LEVEL_1 },
    { "pss_threshold_level_2",  WLAN_CFG_INIT_PSS_THRESHOLD_LEVEL_2 },
    { "cpu_freq_limit_level_2", WLAN_CFG_INIT_CPU_FREQ_LIMIT_LEVEL_2 },
    { "ddr_freq_limit_level_2", WLAN_CFG_INIT_DDR_FREQ_LIMIT_LEVEL_2 },
    { "pss_threshold_level_3",  WLAN_CFG_INIT_PSS_THRESHOLD_LEVEL_3 },
    { "cpu_freq_limit_level_3", WLAN_CFG_INIT_CPU_FREQ_LIMIT_LEVEL_3 },
    { "ddr_freq_limit_level_3", WLAN_CFG_INIT_DDR_FREQ_LIMIT_LEVEL_3 },
    { "device_type_level_0",    WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_0 },
    { "device_type_level_1",    WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_1 },
    { "device_type_level_2",    WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_2 },
    { "device_type_level_3",    WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_3 },
    /* 修改DMA latency,避免cpu进入过深的idle state */
    { "lock_dma_latency",  WLAN_CFG_PRIV_DMA_LATENCY },
    { "lock_cpu_th_high",           WLAN_CFG_PRIV_LOCK_CPU_TH_HIGH },
    { "lock_cpu_th_low",            WLAN_CFG_PRIV_LOCK_CPU_TH_LOW },
    /* 收发中断动态绑核 */
    { "irq_affinity",       WLAN_CFG_INIT_IRQ_AFFINITY },
    { "cpu_id_th_low",      WLAN_CFG_INIT_IRQ_TH_LOW },
    { "cpu_id_th_high",     WLAN_CFG_INIT_IRQ_TH_HIGH },
    { "cpu_id_pps_th_low",  WLAN_CFG_INIT_IRQ_PPS_TH_LOW },
    { "cpu_id_pps_th_high", WLAN_CFG_INIT_IRQ_PPS_TH_HIGH },
#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
    /* 硬件聚合使能 */
    { "hw_ampdu",      WLAN_CFG_INIT_HW_AMPDU },
    { "hw_ampdu_th_l", WLAN_CFG_INIT_HW_AMPDU_TH_LOW },
    { "hw_ampdu_th_h", WLAN_CFG_INIT_HW_AMPDU_TH_HIGH },
#endif
#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU
    { "tx_amsdu_ampdu",      WLAN_CFG_INIT_AMPDU_AMSDU_SKB },
    { "tx_amsdu_ampdu_th_l", WLAN_CFG_INIT_AMSDU_AMPDU_TH_LOW },
    { "tx_amsdu_ampdu_th_m", WLAN_CFG_INIT_AMSDU_AMPDU_TH_MIDDLE },
    { "tx_amsdu_ampdu_th_h", WLAN_CFG_INIT_AMSDU_AMPDU_TH_HIGH },
#endif
#ifdef _PRE_WLAN_TCP_OPT
    { "en_tcp_ack_filter",      WLAN_CFG_INIT_TCP_ACK_FILTER },
    { "rx_tcp_ack_filter_th_l", WLAN_CFG_INIT_TCP_ACK_FILTER_TH_LOW },
    { "rx_tcp_ack_filter_th_h", WLAN_CFG_INIT_TCP_ACK_FILTER_TH_HIGH },
#endif

    { "small_amsdu_switch",   WLAN_CFG_INIT_TX_SMALL_AMSDU },
    { "small_amsdu_th_h",     WLAN_CFG_INIT_SMALL_AMSDU_HIGH },
    { "small_amsdu_th_l",     WLAN_CFG_INIT_SMALL_AMSDU_LOW },
    { "small_amsdu_pps_th_h", WLAN_CFG_INIT_SMALL_AMSDU_PPS_HIGH },
    { "small_amsdu_pps_th_l", WLAN_CFG_INIT_SMALL_AMSDU_PPS_LOW },

    { "tcp_ack_buf_switch",    WLAN_CFG_INIT_TX_TCP_ACK_BUF },
    { "tcp_ack_buf_th_h",      WLAN_CFG_INIT_TCP_ACK_BUF_HIGH },
    { "tcp_ack_buf_th_l",      WLAN_CFG_INIT_TCP_ACK_BUF_LOW },
    { "tcp_ack_buf_th_h_40M",  WLAN_CFG_INIT_TCP_ACK_BUF_HIGH_40M },
    { "tcp_ack_buf_th_l_40M",  WLAN_CFG_INIT_TCP_ACK_BUF_LOW_40M },
    { "tcp_ack_buf_th_h_80M",  WLAN_CFG_INIT_TCP_ACK_BUF_HIGH_80M },
    { "tcp_ack_buf_th_l_80M",  WLAN_CFG_INIT_TCP_ACK_BUF_LOW_80M },
    { "tcp_ack_buf_th_h_160M", WLAN_CFG_INIT_TCP_ACK_BUF_HIGH_160M },
    { "tcp_ack_buf_th_l_160M", WLAN_CFG_INIT_TCP_ACK_BUF_LOW_160M },

    { "tcp_ack_buf_userctl_switch", WLAN_CFG_INIT_TX_TCP_ACK_BUF_USERCTL },
    { "tcp_ack_buf_userctl_h",      WLAN_CFG_INIT_TCP_ACK_BUF_USERCTL_HIGH },
    { "tcp_ack_buf_userctl_l",      WLAN_CFG_INIT_TCP_ACK_BUF_USERCTL_LOW },

    { "dyn_bypass_extlna_th_switch", WLAN_CFG_INIT_RX_DYN_BYPASS_EXTLNA },
    { "dyn_bypass_extlna_th_h",      WLAN_CFG_INIT_RX_DYN_BYPASS_EXTLNA_HIGH },
    { "dyn_bypass_extlna_th_l",      WLAN_CFG_INIT_RX_DYN_BYPASS_EXTLNA_LOW },

    { "rx_ampdu_amsdu", WLAN_CFG_INIT_RX_AMPDU_AMSDU_SKB },
    { "rx_ampdu_bitmap", WLAN_CFG_INIT_RX_AMPDU_BITMAP },

    /* 低功耗 */
    { "powermgmt_switch", WLAN_CFG_INIT_POWERMGMT_SWITCH },

    { "ps_mode",                        WLAN_CFG_INIT_PS_MODE },
    { "min_fast_ps_idle",               WLAN_CFG_INIT_MIN_FAST_PS_IDLE },
    { "max_fast_ps_idle",               WLAN_CFG_INIT_MAX_FAST_PS_IDLE },
    { "auto_fast_ps_thresh_screen_on",  WLAN_CFG_INIT_AUTO_FAST_PS_THRESH_SCREENON },
    { "auto_fast_ps_thresh_screen_off", WLAN_CFG_INIT_AUTO_FAST_PS_THRESH_SCREENOFF },
    /* 可维可测 */
    { "loglevel", WLAN_CFG_INIT_LOGLEVEL },
    /* 2G RF前端插损 */
    { "rf_rx_insertion_loss_2g_b1", WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_2G_BAND1 },
    { "rf_rx_insertion_loss_2g_b2", WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_2G_BAND2 },
    { "rf_rx_insertion_loss_2g_b3", WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_2G_BAND3 },
    /* 5G RF前端插损 */
    { "rf_rx_insertion_loss_5g_b1", WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND1 },
    { "rf_rx_insertion_loss_5g_b2", WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND2 },
    { "rf_rx_insertion_loss_5g_b3", WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND3 },
    { "rf_rx_insertion_loss_5g_b4", WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND4 },
    { "rf_rx_insertion_loss_5g_b5", WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND5 },
    { "rf_rx_insertion_loss_5g_b6", WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND6 },
    { "rf_rx_insertion_loss_5g_b7", WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND7 },
    /* 用于定制化计算PWR RF值的偏差 */
    { "rf_line_rf_pwr_ref_rssi_db_2g_c0_mult4",  WLAN_CFG_INIT_RF_PWR_REF_RSSI_2G_C0_MULT4 },
    { "rf_line_rf_pwr_ref_rssi_db_2g_c1_mult4",  WLAN_CFG_INIT_RF_PWR_REF_RSSI_2G_C1_MULT4 },
    { "rf_line_rf_pwr_ref_rssi_db_5g_c0_mult4",  WLAN_CFG_INIT_RF_PWR_REF_RSSI_5G_C0_MULT4 },
    { "rf_line_rf_pwr_ref_rssi_db_5g_c1_mult4",  WLAN_CFG_INIT_RF_PWR_REF_RSSI_5G_C1_MULT4 },

    {"rf_rssi_amend_2g_c0", WLAN_CFG_INIT_RF_AMEND_RSSI_2G_C0},
    {"rf_rssi_amend_2g_c1", WLAN_CFG_INIT_RF_AMEND_RSSI_2G_C1},
    {"rf_rssi_amend_5g_c0", WLAN_CFG_INIT_RF_AMEND_RSSI_5G_C0},
    {"rf_rssi_amend_5g_c1", WLAN_CFG_INIT_RF_AMEND_RSSI_5G_C1},

    /* fem */
    { "rf_lna_bypass_gain_db_2g", WLAN_CFG_INIT_RF_LNA_BYPASS_GAIN_DB_2G },
    { "rf_lna_gain_db_2g",        WLAN_CFG_INIT_RF_LNA_GAIN_DB_2G },
    { "rf_pa_db_b0_2g",           WLAN_CFG_INIT_RF_PA_GAIN_DB_B0_2G },
    { "rf_pa_db_b1_2g",           WLAN_CFG_INIT_RF_PA_GAIN_DB_B1_2G },
    { "rf_pa_db_lvl_2g",          WLAN_CFG_INIT_RF_PA_GAIN_LVL_2G },
    { "ext_switch_isexist_2g",    WLAN_CFG_INIT_EXT_SWITCH_ISEXIST_2G },
    { "ext_pa_isexist_2g",        WLAN_CFG_INIT_EXT_PA_ISEXIST_2G },
    { "ext_lna_isexist_2g",       WLAN_CFG_INIT_EXT_LNA_ISEXIST_2G },
    { "lna_on2off_time_ns_2g",    WLAN_CFG_INIT_LNA_ON2OFF_TIME_NS_2G },
    { "lna_off2on_time_ns_2g",    WLAN_CFG_INIT_LNA_OFF2ON_TIME_NS_2G },
    { "rf_lna_bypass_gain_db_5g", WLAN_CFG_INIT_RF_LNA_BYPASS_GAIN_DB_5G },
    { "rf_lna_gain_db_5g",        WLAN_CFG_INIT_RF_LNA_GAIN_DB_5G },
    { "rf_pa_db_b0_5g",           WLAN_CFG_INIT_RF_PA_GAIN_DB_B0_5G },
    { "rf_pa_db_b1_5g",           WLAN_CFG_INIT_RF_PA_GAIN_DB_B1_5G },
    { "rf_pa_db_lvl_5g",          WLAN_CFG_INIT_RF_PA_GAIN_LVL_5G },
    { "ext_switch_isexist_5g",    WLAN_CFG_INIT_EXT_SWITCH_ISEXIST_5G },
    { "ext_pa_isexist_5g",        WLAN_CFG_INIT_EXT_PA_ISEXIST_5G },
    { "ext_lna_isexist_5g",       WLAN_CFG_INIT_EXT_LNA_ISEXIST_5G },
    { "lna_on2off_time_ns_5g",    WLAN_CFG_INIT_LNA_ON2OFF_TIME_NS_5G },
    { "lna_off2on_time_ns_5g",    WLAN_CFG_INIT_LNA_OFF2ON_TIME_NS_5G },
    /* SCAN */
    { "random_mac_addr_scan", WLAN_CFG_INIT_RANDOM_MAC_ADDR_SCAN },
    /* 11AC2G */
    { "11ac2g_enable",        WLAN_CFG_INIT_11AC2G_ENABLE },
    { "disable_capab_2ght40", WLAN_CFG_INIT_DISABLE_CAPAB_2GHT40 },
    { "dual_antenna_enable",  WLAN_CFG_INIT_DUAL_ANTENNA_ENABLE }, /* 双天线开关 */

    { "probe_resp_mode", WLAN_CFG_INIT_PROBE_RESP_MODE},
    { "miracast_enable", WLAN_CFG_INIT_MIRACAST_SINK_ENABLE},
    { "reduce_miracast_log", WLAN_CFG_INIT_REDUCE_MIRACAST_LOG},
    { "bind_lowest_load_cpu", WLAN_CFG_INIT_BIND_LOWEST_LOAD_CPU},
    { "core_bind_cap", WLAN_CFG_INIT_CORE_BIND_CAP},
    {"only_fast_mode", WLAN_CFG_INIT_FAST_MODE},

    /* sta keepalive cnt th */
    { "sta_keepalive_cnt_th", WLAN_CFG_INIT_STA_KEEPALIVE_CNT_TH }, /* 动态功率校准 */

    { "far_dist_pow_gain_switch", WLAN_CFG_INIT_FAR_DIST_POW_GAIN_SWITCH },
    { "far_dist_dsss_scale_promote_switch", WLAN_CFG_INIT_FAR_DIST_DSSS_SCALE_PROMOTE_SWITCH },
    { "chann_radio_cap", WLAN_CFG_INIT_CHANN_RADIO_CAP },

    { "lte_gpio_check_switch",    WLAN_CFG_LTE_GPIO_CHECK_SWITCH }, /* lte?????? */
    { "ism_priority",             WLAN_ATCMDSRV_ISM_PRIORITY },
    { "lte_rx",                   WLAN_ATCMDSRV_LTE_RX },
    { "lte_tx",                   WLAN_ATCMDSRV_LTE_TX },
    { "lte_inact",                WLAN_ATCMDSRV_LTE_INACT },
    { "ism_rx_act",               WLAN_ATCMDSRV_ISM_RX_ACT },
    { "bant_pri",                 WLAN_ATCMDSRV_BANT_PRI },
    { "bant_status",              WLAN_ATCMDSRV_BANT_STATUS },
    { "want_pri",                 WLAN_ATCMDSRV_WANT_PRI },
    { "want_status",              WLAN_ATCMDSRV_WANT_STATUS },
    { "tx5g_upc_mix_gain_ctrl_1", WLAN_TX5G_UPC_MIX_GAIN_CTRL_1 },
    { "tx5g_upc_mix_gain_ctrl_2", WLAN_TX5G_UPC_MIX_GAIN_CTRL_2 },
    { "tx5g_upc_mix_gain_ctrl_3", WLAN_TX5G_UPC_MIX_GAIN_CTRL_3 },
    { "tx5g_upc_mix_gain_ctrl_4", WLAN_TX5G_UPC_MIX_GAIN_CTRL_4 },
    { "tx5g_upc_mix_gain_ctrl_5", WLAN_TX5G_UPC_MIX_GAIN_CTRL_5 },
    { "tx5g_upc_mix_gain_ctrl_6", WLAN_TX5G_UPC_MIX_GAIN_CTRL_6 },
    { "tx5g_upc_mix_gain_ctrl_7", WLAN_TX5G_UPC_MIX_GAIN_CTRL_7 },
    /* 定制化RF部分PA偏置寄存器 */
    { "tx2g_pa_gate_236", WLAN_TX2G_PA_GATE_VCTL_REG236 },
    { "tx2g_pa_gate_237", WLAN_TX2G_PA_GATE_VCTL_REG237 },
    { "tx2g_pa_gate_238", WLAN_TX2G_PA_GATE_VCTL_REG238 },
    { "tx2g_pa_gate_239", WLAN_TX2G_PA_GATE_VCTL_REG239 },
    { "tx2g_pa_gate_240", WLAN_TX2G_PA_GATE_VCTL_REG240 },
    { "tx2g_pa_gate_241", WLAN_TX2G_PA_GATE_VCTL_REG241 },
    { "tx2g_pa_gate_242", WLAN_TX2G_PA_GATE_VCTL_REG242 },
    { "tx2g_pa_gate_243", WLAN_TX2G_PA_GATE_VCTL_REG243 },
    { "tx2g_pa_gate_244", WLAN_TX2G_PA_GATE_VCTL_REG244 },

    { "tx2g_pa_gate_253",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG253 },
    { "tx2g_pa_gate_254",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG254 },
    { "tx2g_pa_gate_255",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG255 },
    { "tx2g_pa_gate_256",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG256 },
    { "tx2g_pa_gate_257",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG257 },
    { "tx2g_pa_gate_258",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG258 },
    { "tx2g_pa_gate_259",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG259 },
    { "tx2g_pa_gate_260",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG260 },
    { "tx2g_pa_gate_261",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG261 },
    { "tx2g_pa_gate_262",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG262 },
    { "tx2g_pa_gate_263",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG263 },
    { "tx2g_pa_gate_264",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG264 },
    { "tx2g_pa_gate_265",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG265 },
    { "tx2g_pa_gate_266",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG266 },
    { "tx2g_pa_gate_267",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG267 },
    { "tx2g_pa_gate_268",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG268 },
    { "tx2g_pa_gate_269",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG269 },
    { "tx2g_pa_gate_270",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG270 },
    { "tx2g_pa_gate_271",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG271 },
    { "tx2g_pa_gate_272",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG272 },
    { "tx2g_pa_gate_273",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG273 },
    { "tx2g_pa_gate_274",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG274 },
    { "tx2g_pa_gate_275",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG275 },
    { "tx2g_pa_gate_276",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG276 },
    { "tx2g_pa_gate_277",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG277 },
    { "tx2g_pa_gate_278",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG278 },
    { "tx2g_pa_gate_279",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG279 },
    { "tx2g_pa_gate_280_band1", WLAN_TX2G_PA_VRECT_GATE_THIN_REG280_BAND1 },
    { "tx2g_pa_gate_281",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG281 },
    { "tx2g_pa_gate_282",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG282 },
    { "tx2g_pa_gate_283",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG283 },
    { "tx2g_pa_gate_284",       WLAN_TX2G_PA_VRECT_GATE_THIN_REG284 },
    { "tx2g_pa_gate_280_band2", WLAN_TX2G_PA_VRECT_GATE_THIN_REG280_BAND2 },
    { "tx2g_pa_gate_280_band3", WLAN_TX2G_PA_VRECT_GATE_THIN_REG280_BAND3 },
    { "delta_cca_ed_high_20th_2g", WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_20TH_2G },
    { "delta_cca_ed_high_40th_2g", WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_40TH_2G },
    { "delta_cca_ed_high_20th_5g", WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_20TH_5G },
    { "delta_cca_ed_high_40th_5g", WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_40TH_5G },
    { "delta_cca_ed_high_80th_5g", WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_80TH_5G },
    { "voe_switch_mask",           WLAN_CFG_INIT_VOE_SWITCH },
    { "11ax_switch_mask",          WLAN_CFG_INIT_11AX_SWITCH },
    { "htc_switch_mask",           WLAN_CFG_INIT_HTC_SWITCH },
    { "multi_bssid_switch_mask",   WLAN_CFG_INIT_MULTI_BSSID_SWITCH},
    /* ldac m2s rssi */
    { "ldac_threshold_m2s", WLAN_CFG_INIT_LDAC_THRESHOLD_M2S },
    { "ldac_threshold_s2m", WLAN_CFG_INIT_LDAC_THRESHOLD_S2M },
    /* btcoex mcm rssi */
    { "btcoex_threshold_mcm_down", WLAN_CFG_INIT_BTCOEX_THRESHOLD_MCM_DOWN },
    { "btcoex_threshold_mcm_up",   WLAN_CFG_INIT_BTCOEX_THRESHOLD_MCM_UP },
#ifdef _PRE_WLAN_FEATURE_NRCOEX
    /* 5g nr coex */
    {"nrcoex_enable",                   WLAN_CFG_INIT_NRCOEX_ENABLE},
    {"nrcoex_rule0_freq",               WLAN_CFG_INIT_NRCOEX_RULE0_FREQ},
    {"nrcoex_rule0_40m_20m_gap0",       WLAN_CFG_INIT_NRCOEX_RULE0_40M_20M_GAP0},
    {"nrcoex_rule0_160m_80m_gap0",      WLAN_CFG_INIT_NRCOEX_RULE0_160M_80M_GAP0},
    {"nrcoex_rule0_40m_20m_gap1",       WLAN_CFG_INIT_NRCOEX_RULE0_40M_20M_GAP1},
    {"nrcoex_rule0_160m_80m_gap1",      WLAN_CFG_INIT_NRCOEX_RULE0_160M_80M_GAP1},
    {"nrcoex_rule0_40m_20m_gap2",       WLAN_CFG_INIT_NRCOEX_RULE0_40M_20M_GAP2},
    {"nrcoex_rule0_160m_80m_gap2",      WLAN_CFG_INIT_NRCOEX_RULE0_160M_80M_GAP2},
    {"nrcoex_rule0_smallgap0_act",      WLAN_CFG_INIT_NRCOEX_RULE0_SMALLGAP0_ACT},
    {"nrcoex_rule0_gap01_act",          WLAN_CFG_INIT_NRCOEX_RULE0_GAP01_ACT},
    {"nrcoex_rule0_gap12_act",          WLAN_CFG_INIT_NRCOEX_RULE0_GAP12_ACT},
    {"nrcoex_rule0_rxslot_rssi",        WLAN_CFG_INIT_NRCOEX_RULE0_RXSLOT_RSSI},
    {"nrcoex_rule1_freq",               WLAN_CFG_INIT_NRCOEX_RULE1_FREQ},
    {"nrcoex_rule1_40m_20m_gap0",       WLAN_CFG_INIT_NRCOEX_RULE1_40M_20M_GAP0},
    {"nrcoex_rule1_160m_80m_gap0",      WLAN_CFG_INIT_NRCOEX_RULE1_160M_80M_GAP0},
    {"nrcoex_rule1_40m_20m_gap1",       WLAN_CFG_INIT_NRCOEX_RULE1_40M_20M_GAP1},
    {"nrcoex_rule1_160m_80m_gap1",      WLAN_CFG_INIT_NRCOEX_RULE1_160M_80M_GAP1},
    {"nrcoex_rule1_40m_20m_gap2",       WLAN_CFG_INIT_NRCOEX_RULE1_40M_20M_GAP2},
    {"nrcoex_rule1_160m_80m_gap2",      WLAN_CFG_INIT_NRCOEX_RULE1_160M_80M_GAP2},
    {"nrcoex_rule1_smallgap0_act",      WLAN_CFG_INIT_NRCOEX_RULE1_SMALLGAP0_ACT},
    {"nrcoex_rule1_gap01_act",          WLAN_CFG_INIT_NRCOEX_RULE1_GAP01_ACT},
    {"nrcoex_rule1_gap12_act",          WLAN_CFG_INIT_NRCOEX_RULE1_GAP12_ACT},
    {"nrcoex_rule1_rxslot_rssi",        WLAN_CFG_INIT_NRCOEX_RULE1_RXSLOT_RSSI},
    {"nrcoex_rule2_freq",               WLAN_CFG_INIT_NRCOEX_RULE2_FREQ},
    {"nrcoex_rule2_40m_20m_gap0",       WLAN_CFG_INIT_NRCOEX_RULE2_40M_20M_GAP0},
    {"nrcoex_rule2_160m_80m_gap0",      WLAN_CFG_INIT_NRCOEX_RULE2_160M_80M_GAP0},
    {"nrcoex_rule2_40m_20m_gap1",       WLAN_CFG_INIT_NRCOEX_RULE2_40M_20M_GAP1},
    {"nrcoex_rule2_160m_80m_gap1",      WLAN_CFG_INIT_NRCOEX_RULE2_160M_80M_GAP1},
    {"nrcoex_rule2_40m_20m_gap2",       WLAN_CFG_INIT_NRCOEX_RULE2_40M_20M_GAP2},
    {"nrcoex_rule2_160m_80m_gap2",      WLAN_CFG_INIT_NRCOEX_RULE2_160M_80M_GAP2},
    {"nrcoex_rule2_smallgap0_act",      WLAN_CFG_INIT_NRCOEX_RULE2_SMALLGAP0_ACT},
    {"nrcoex_rule2_gap01_act",          WLAN_CFG_INIT_NRCOEX_RULE2_GAP01_ACT},
    {"nrcoex_rule2_gap12_act",          WLAN_CFG_INIT_NRCOEX_RULE2_GAP12_ACT},
    {"nrcoex_rule2_rxslot_rssi",        WLAN_CFG_INIT_NRCOEX_RULE2_RXSLOT_RSSI},
    {"nrcoex_rule3_freq",               WLAN_CFG_INIT_NRCOEX_RULE3_FREQ},
    {"nrcoex_rule3_40m_20m_gap0",       WLAN_CFG_INIT_NRCOEX_RULE3_40M_20M_GAP0},
    {"nrcoex_rule3_160m_80m_gap0",      WLAN_CFG_INIT_NRCOEX_RULE3_160M_80M_GAP0},
    {"nrcoex_rule3_40m_20m_gap1",       WLAN_CFG_INIT_NRCOEX_RULE3_40M_20M_GAP1},
    {"nrcoex_rule3_160m_80m_gap1",      WLAN_CFG_INIT_NRCOEX_RULE3_160M_80M_GAP1},
    {"nrcoex_rule3_40m_20m_gap2",       WLAN_CFG_INIT_NRCOEX_RULE3_40M_20M_GAP2},
    {"nrcoex_rule3_160m_80m_gap2",      WLAN_CFG_INIT_NRCOEX_RULE3_160M_80M_GAP2},
    {"nrcoex_rule3_smallgap0_act",      WLAN_CFG_INIT_NRCOEX_RULE3_SMALLGAP0_ACT},
    {"nrcoex_rule3_gap01_act",          WLAN_CFG_INIT_NRCOEX_RULE3_GAP01_ACT},
    {"nrcoex_rule3_gap12_act",          WLAN_CFG_INIT_NRCOEX_RULE3_GAP12_ACT},
    {"nrcoex_rule3_rxslot_rssi",        WLAN_CFG_INIT_NRCOEX_RULE3_RXSLOT_RSSI},
#endif
#ifdef _PRE_WLAN_FEATURE_MBO
    {"mbo_switch_mask",                 WLAN_CFG_INIT_MBO_SWITCH},
#endif
    { "dynamic_dbac_adjust_mask",       WLAN_CFG_INIT_DYNAMIC_DBAC_SWITCH},
    { "ddr_freq",                       WLAN_CFG_INIT_DDR_FREQ},
    { "hiex_cap",                       WLAN_CFG_INIT_HIEX_CAP},
    { "ftm_cap",                        WLAN_CFG_INIT_FTM_CAP},
    { "rf_filter_narrow_rssi_amend_2g_c0", WLAN_CFG_INIT_RF_FILTER_NARROW_RSSI_AMEND_2G_C0},
    { "rf_filter_narrow_rssi_amend_2g_c1", WLAN_CFG_INIT_RF_FILTER_NARROW_RSSI_AMEND_2G_C1},
    { "rf_filter_narrow_rssi_amend_5g_c0", WLAN_CFG_INIT_RF_FILTER_NARROW_RSSI_AMEND_5G_C0},
    { "rf_filter_narrow_rssi_amend_5g_c1", WLAN_CFG_INIT_RF_FILTER_NARROW_RSSI_AMEND_5G_C1},

#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
    { "mcast_ampdu_enable",             WLAN_CFG_INIT_MCAST_AMPDU_ENABLE},
#endif
    { "pt_mcast_enable",             WLAN_CFG_INIT_PT_MCAST_ENABLE},
    { NULL,         0 }
};


int32_t *hwifi_get_g_host_init_params(void)
{
    return (int32_t *)g_host_init_params;
}

wlan_cfg_cmd *hwifi_get_g_wifi_config_cmds(void)
{
    return (wlan_cfg_cmd *)g_wifi_config_cmds;
}

/*
 * 函 数 名  : host_auto_freq_params_init
 * 功能描述  : 给定制化参数全局数组 g_host_init_params中自动调频相关参数赋初值
 *             ini文件读取失败时用初值
 */
OAL_STATIC void host_auto_freq_params_init(void)
{
    g_host_init_params[WLAN_CFG_INIT_PSS_THRESHOLD_LEVEL_0] = PPS_VALUE_0;
    g_host_init_params[WLAN_CFG_INIT_CPU_FREQ_LIMIT_LEVEL_0] = CPU_MIN_FREQ_VALUE_0;
    g_host_init_params[WLAN_CFG_INIT_DDR_FREQ_LIMIT_LEVEL_0] = DDR_MIN_FREQ_VALUE_0;
    g_host_init_params[WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_0] = FREQ_IDLE;
    g_host_init_params[WLAN_CFG_INIT_PSS_THRESHOLD_LEVEL_1] = PPS_VALUE_1;
    g_host_init_params[WLAN_CFG_INIT_CPU_FREQ_LIMIT_LEVEL_1] = CPU_MIN_FREQ_VALUE_1;
    g_host_init_params[WLAN_CFG_INIT_DDR_FREQ_LIMIT_LEVEL_1] = DDR_MIN_FREQ_VALUE_1;
    g_host_init_params[WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_1] = FREQ_MIDIUM;
    g_host_init_params[WLAN_CFG_INIT_PSS_THRESHOLD_LEVEL_2] = PPS_VALUE_2;
    g_host_init_params[WLAN_CFG_INIT_CPU_FREQ_LIMIT_LEVEL_2] = CPU_MIN_FREQ_VALUE_2;
    g_host_init_params[WLAN_CFG_INIT_DDR_FREQ_LIMIT_LEVEL_2] = DDR_MIN_FREQ_VALUE_2;
    g_host_init_params[WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_2] = FREQ_HIGHER;
    g_host_init_params[WLAN_CFG_INIT_PSS_THRESHOLD_LEVEL_3] = PPS_VALUE_3;
    g_host_init_params[WLAN_CFG_INIT_CPU_FREQ_LIMIT_LEVEL_3] = CPU_MIN_FREQ_VALUE_3;
    g_host_init_params[WLAN_CFG_INIT_DDR_FREQ_LIMIT_LEVEL_3] = DDR_MIN_FREQ_VALUE_3;
    g_host_init_params[WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_3] = FREQ_HIGHEST;
}

#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU
/*
 * 函 数 名  : host_amsdu_th_params_init
 * 功能描述  : 给定制化参数全局数组 g_host_init_params中amsdu聚合门限相关参数赋初值
 *             ini文件读取失败时用初值
 */
OAL_STATIC void host_amsdu_th_params_init(void)
{
    g_host_init_params[WLAN_CFG_INIT_AMPDU_AMSDU_SKB] = OAL_FALSE;
    g_host_init_params[WLAN_CFG_INIT_AMSDU_AMPDU_TH_HIGH]   = WLAN_AMSDU_AMPDU_TH_HIGH;
    g_host_init_params[WLAN_CFG_INIT_AMSDU_AMPDU_TH_MIDDLE] = WLAN_AMSDU_AMPDU_TH_MIDDLE;
    g_host_init_params[WLAN_CFG_INIT_AMSDU_AMPDU_TH_LOW]    = WLAN_AMSDU_AMPDU_TH_LOW;
}
#endif


/*
 * 函 数 名  : host_params_init_first
 * 功能描述  : 给定制化参数全局数组 g_host_init_params 附初值
 *             ini文件读取失败时用初值
 */
OAL_STATIC void host_params_performance_init(void)
{
    /* 动态绑PCIE中断 */
    g_host_init_params[WLAN_CFG_PRIV_DMA_LATENCY] = OAL_FALSE;
    g_host_init_params[WLAN_CFG_PRIV_LOCK_CPU_TH_HIGH] = 0;
    g_host_init_params[WLAN_CFG_PRIV_LOCK_CPU_TH_LOW] = 0;
    g_host_init_params[WLAN_CFG_INIT_IRQ_AFFINITY] = OAL_FALSE;
    g_host_init_params[WLAN_CFG_INIT_IRQ_TH_HIGH] = WLAN_IRQ_TH_HIGH;
    g_host_init_params[WLAN_CFG_INIT_IRQ_TH_LOW] = WLAN_IRQ_TH_LOW;
    g_host_init_params[WLAN_CFG_INIT_IRQ_PPS_TH_HIGH] = WLAN_IRQ_PPS_TH_HIGH;
    g_host_init_params[WLAN_CFG_INIT_IRQ_PPS_TH_LOW] = WLAN_IRQ_PPS_TH_LOW;
#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
    /* 硬件聚合定制化项 */
    g_host_init_params[WLAN_CFG_INIT_HW_AMPDU] = OAL_FALSE;
    g_host_init_params[WLAN_CFG_INIT_HW_AMPDU_TH_HIGH] = WLAN_HW_AMPDU_TH_HIGH;
    g_host_init_params[WLAN_CFG_INIT_HW_AMPDU_TH_LOW] = WLAN_HW_AMPDU_TH_LOW;
#endif

#ifdef _PRE_WLAN_TCP_OPT
    g_host_init_params[WLAN_CFG_INIT_TCP_ACK_FILTER] = OAL_FALSE;
    g_host_init_params[WLAN_CFG_INIT_TCP_ACK_FILTER_TH_HIGH] = WLAN_TCP_ACK_FILTER_TH_HIGH;
    g_host_init_params[WLAN_CFG_INIT_TCP_ACK_FILTER_TH_LOW] = WLAN_TCP_ACK_FILTER_TH_LOW;
#endif

    g_host_init_params[WLAN_CFG_INIT_TX_SMALL_AMSDU] = OAL_TRUE;
    g_host_init_params[WLAN_CFG_INIT_SMALL_AMSDU_HIGH] = WLAN_SMALL_AMSDU_HIGH;
    g_host_init_params[WLAN_CFG_INIT_SMALL_AMSDU_LOW] = WLAN_SMALL_AMSDU_LOW;
    g_host_init_params[WLAN_CFG_INIT_SMALL_AMSDU_PPS_HIGH] = WLAN_SMALL_AMSDU_PPS_HIGH;
    g_host_init_params[WLAN_CFG_INIT_SMALL_AMSDU_PPS_LOW] = WLAN_SMALL_AMSDU_PPS_LOW;

    g_host_init_params[WLAN_CFG_INIT_TX_TCP_ACK_BUF] = OAL_TRUE;
    g_host_init_params[WLAN_CFG_INIT_TCP_ACK_BUF_HIGH] = WLAN_TCP_ACK_BUF_HIGH;
    g_host_init_params[WLAN_CFG_INIT_TCP_ACK_BUF_LOW] = WLAN_TCP_ACK_BUF_LOW;
    g_host_init_params[WLAN_CFG_INIT_TCP_ACK_BUF_HIGH_40M] = WLAN_TCP_ACK_BUF_HIGH_40M;
    g_host_init_params[WLAN_CFG_INIT_TCP_ACK_BUF_LOW_40M] = WLAN_TCP_ACK_BUF_LOW_40M;
    g_host_init_params[WLAN_CFG_INIT_TCP_ACK_BUF_HIGH_80M] = WLAN_TCP_ACK_BUF_HIGH_80M;
    g_host_init_params[WLAN_CFG_INIT_TCP_ACK_BUF_LOW_80M] = WLAN_TCP_ACK_BUF_LOW_80M;
    g_host_init_params[WLAN_CFG_INIT_TCP_ACK_BUF_HIGH_160M] = WLAN_TCP_ACK_BUF_HIGH_160M;
    g_host_init_params[WLAN_CFG_INIT_TCP_ACK_BUF_LOW_160M] = WLAN_TCP_ACK_BUF_LOW_160M;

    g_host_init_params[WLAN_CFG_INIT_TX_TCP_ACK_BUF_USERCTL] = OAL_FALSE;
    g_host_init_params[WLAN_CFG_INIT_TCP_ACK_BUF_USERCTL_HIGH] = WLAN_TCP_ACK_BUF_USERCTL_HIGH;
    g_host_init_params[WLAN_CFG_INIT_TCP_ACK_BUF_USERCTL_LOW] = WLAN_TCP_ACK_BUF_USERCTL_LOW;

    g_host_init_params[WLAN_CFG_INIT_RX_DYN_BYPASS_EXTLNA] = OAL_FALSE;
    g_host_init_params[WLAN_CFG_INIT_RX_DYN_BYPASS_EXTLNA_HIGH] = WLAN_RX_DYN_BYPASS_EXTLNA_HIGH;
    g_host_init_params[WLAN_CFG_INIT_RX_DYN_BYPASS_EXTLNA_LOW] = WLAN_RX_DYN_BYPASS_EXTLNA_LOW;
    g_host_init_params[WLAN_CFG_INIT_RX_AMPDU_AMSDU_SKB] = OAL_FALSE;
    g_host_init_params[WLAN_CFG_INIT_RX_AMPDU_BITMAP] = OAL_FALSE;
    g_host_init_params[WLAN_CFG_INIT_HIEX_CAP] = WLAN_HIEX_DEV_CAP;
    g_host_init_params[WLAN_CFG_INIT_SDIO_D2H_ASSEMBLE_COUNT] = HISDIO_DEV2HOST_SCATT_MAX;
    g_host_init_params[WLAN_CFG_INIT_SDIO_H2D_ASSEMBLE_COUNT] = WLAN_SDIO_H2D_ASSEMBLE_COUNT_VAL;
    g_host_init_params[WLAN_CFG_INIT_FTM_CAP] = OAL_FALSE;
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
    g_host_init_params[WLAN_CFG_INIT_MCAST_AMPDU_ENABLE] = OAL_FALSE;
#endif
    g_host_init_params[WLAN_CFG_INIT_PT_MCAST_ENABLE] = OAL_FALSE;
}

/*
 * 功能描述  : 给定制化参数全局数组 g_host_init_params 附初值
 *             ini文件读取失败时用初值
 */
OAL_STATIC void host_btcoex_rssi_th_params_init(void)
{
    /* ldac m2s rssi */
    g_host_init_params[WLAN_CFG_INIT_LDAC_THRESHOLD_M2S] = WLAN_BTCOEX_THRESHOLD_MCM_DOWN; /* 默认最大门限，不支持 */
    g_host_init_params[WLAN_CFG_INIT_LDAC_THRESHOLD_S2M] = WLAN_BTCOEX_THRESHOLD_MCM_UP;

    /* mcm btcoex rssi */
    g_host_init_params[WLAN_CFG_INIT_BTCOEX_THRESHOLD_MCM_DOWN] = WLAN_BTCOEX_THRESHOLD_MCM_DOWN; /* 默认最大门限，不支持 */
    g_host_init_params[WLAN_CFG_INIT_BTCOEX_THRESHOLD_MCM_UP] = WLAN_BTCOEX_THRESHOLD_MCM_UP;
}

static void host_param_init_netaccess_param(void)
{
    /* ROAM */
    g_host_init_params[WLAN_CFG_INIT_ROAM_SWITCH] = WLAN_ROAM_SWITCH_MODE;
    g_host_init_params[WLAN_CFG_INIT_SCAN_ORTHOGONAL] = WLAN_SCAN_ORTHOGONAL_VAL;
    g_host_init_params[WLAN_CFG_INIT_TRIGGER_B] = WLAN_TRIGGER_B_VAL;
    g_host_init_params[WLAN_CFG_INIT_TRIGGER_A] = WLAN_TRIGGER_A_VAL;
    g_host_init_params[WLAN_CFG_INIT_DELTA_B] = WLAN_DELTA_B_VAL;
    g_host_init_params[WLAN_CFG_INIT_DELTA_A] = WLAN_DELTA_A_VAL;
}

static void host_param_init_performance_param(void)
{
    /* 性能 */
    g_host_init_params[WLAN_CFG_INIT_AMPDU_TX_MAX_NUM] = 64; // win size 64
    g_host_init_params[WLAN_CFG_INIT_USED_MEM_FOR_START] = WLAN_MEM_FOR_START;
    g_host_init_params[WLAN_CFG_INIT_USED_MEM_FOR_STOP] = WLAN_MEM_FOR_STOP;
    g_host_init_params[WLAN_CFG_INIT_RX_ACK_LIMIT] = WLAN_RX_ACK_LIMIT_VAL;
    /* LINKLOSS */
    g_host_init_params[WLAN_CFG_INIT_LINK_LOSS_THRESHOLD_BT] = WLAN_LOSS_THRESHOLD_WLAN_BT;
    g_host_init_params[WLAN_CFG_INIT_LINK_LOSS_THRESHOLD_DBAC] = WLAN_LOSS_THRESHOLD_WLAN_DBAC;
    g_host_init_params[WLAN_CFG_INIT_LINK_LOSS_THRESHOLD_NORMAL] = WLAN_LOSS_THRESHOLD_WLAN_NORMAL;
}

static void host_param_init_rf_param(void)
{
    uint8_t param_idx;

    /* 2G RF前端 */
    g_host_init_params[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_2G_BAND1] = 0xF4F4;
    g_host_init_params[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_2G_BAND2] = 0xF4F4;
    g_host_init_params[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_2G_BAND3] = 0xF4F4;
    /* 5G RF前端 */
    g_host_init_params[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND1] = 0xF8F8;
    g_host_init_params[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND2] = 0xF8F8;
    g_host_init_params[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND3] = 0xF8F8;
    g_host_init_params[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND4] = 0xF8F8;
    g_host_init_params[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND5] = 0xF8F8;
    g_host_init_params[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND6] = 0xF8F8;
    g_host_init_params[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND7] = 0xF8F8;
    /* fem */
    g_host_init_params[WLAN_CFG_INIT_RF_LNA_BYPASS_GAIN_DB_2G] = 0xFFF4FFF4;
    g_host_init_params[WLAN_CFG_INIT_RF_LNA_GAIN_DB_2G] = 0x00140014;
    g_host_init_params[WLAN_CFG_INIT_RF_PA_GAIN_DB_B0_2G] = 0xFFF4FFF4;
    g_host_init_params[WLAN_CFG_INIT_RF_PA_GAIN_DB_B1_2G] = 0xFFF4FFF4;
    g_host_init_params[WLAN_CFG_INIT_RF_PA_GAIN_LVL_2G] = 0x00010001;
    g_host_init_params[WLAN_CFG_INIT_EXT_SWITCH_ISEXIST_2G] = 0x00010001;
    g_host_init_params[WLAN_CFG_INIT_EXT_PA_ISEXIST_2G] = 0x00010001;
    g_host_init_params[WLAN_CFG_INIT_EXT_LNA_ISEXIST_2G] = 0x00010001;
    g_host_init_params[WLAN_CFG_INIT_LNA_ON2OFF_TIME_NS_2G] = 0x02760276;
    g_host_init_params[WLAN_CFG_INIT_LNA_OFF2ON_TIME_NS_2G] = 0x01400140;
    g_host_init_params[WLAN_CFG_INIT_RF_LNA_BYPASS_GAIN_DB_5G] = 0xFFF4FFF4;
    g_host_init_params[WLAN_CFG_INIT_RF_LNA_GAIN_DB_5G] = 0x00140014;
    g_host_init_params[WLAN_CFG_INIT_RF_PA_GAIN_DB_B0_5G] = 0xFFF4FFF4;
    g_host_init_params[WLAN_CFG_INIT_RF_PA_GAIN_DB_B1_5G] = 0xFFF4FFF4;
    g_host_init_params[WLAN_CFG_INIT_RF_PA_GAIN_LVL_5G] = 0x00010001;
    g_host_init_params[WLAN_CFG_INIT_EXT_SWITCH_ISEXIST_5G] = 0x00010001;
    g_host_init_params[WLAN_CFG_INIT_EXT_PA_ISEXIST_5G] = 0x00010001;
    g_host_init_params[WLAN_CFG_INIT_EXT_LNA_ISEXIST_5G] = 0x00010001;
    g_host_init_params[WLAN_CFG_INIT_LNA_ON2OFF_TIME_NS_5G] = 0x02760276;
    g_host_init_params[WLAN_CFG_INIT_LNA_OFF2ON_TIME_NS_5G] = 0x01400140;
    /* 用于定制化计算PWR RF值的偏差 */
    for (param_idx = WLAN_CFG_INIT_RF_PWR_REF_RSSI_2G_C0_MULT4;
         param_idx <= WLAN_CFG_INIT_RF_AMEND_RSSI_5G_C1; param_idx++) {
        g_host_init_params[param_idx] = 0;
    }
}

static void host_param_init_keepalive_param(void)
{
    /* sta keepalive cnt th */
    g_host_init_params[WLAN_CFG_INIT_STA_KEEPALIVE_CNT_TH] = WLAN_STA_KEEPALIVE_CNT_TH;
    g_host_init_params[WLAN_CFG_INIT_FAR_DIST_POW_GAIN_SWITCH] = 1;
    g_host_init_params[WLAN_CFG_INIT_FAR_DIST_DSSS_SCALE_PROMOTE_SWITCH] = 1;
    g_host_init_params[WLAN_CFG_INIT_CHANN_RADIO_CAP] = 0xF;

    g_host_init_params[WLAN_CFG_LTE_GPIO_CHECK_SWITCH] = 0;
    g_host_init_params[WLAN_ATCMDSRV_ISM_PRIORITY] = 0;
    g_host_init_params[WLAN_ATCMDSRV_LTE_RX] = 0;
    g_host_init_params[WLAN_ATCMDSRV_LTE_TX] = 0;
    g_host_init_params[WLAN_ATCMDSRV_LTE_INACT] = 0;
    g_host_init_params[WLAN_ATCMDSRV_ISM_RX_ACT] = 0;
    g_host_init_params[WLAN_ATCMDSRV_BANT_PRI] = 0;
    g_host_init_params[WLAN_ATCMDSRV_BANT_STATUS] = 0;
    g_host_init_params[WLAN_ATCMDSRV_WANT_PRI] = 0;
    g_host_init_params[WLAN_ATCMDSRV_WANT_STATUS] = 0;
    g_host_init_params[WLAN_TX5G_UPC_MIX_GAIN_CTRL_1] = 0;
    g_host_init_params[WLAN_TX5G_UPC_MIX_GAIN_CTRL_2] = 0;
    g_host_init_params[WLAN_TX5G_UPC_MIX_GAIN_CTRL_3] = 0;
    g_host_init_params[WLAN_TX5G_UPC_MIX_GAIN_CTRL_4] = 0;
    g_host_init_params[WLAN_TX5G_UPC_MIX_GAIN_CTRL_5] = 0;
    g_host_init_params[WLAN_TX5G_UPC_MIX_GAIN_CTRL_6] = 0;
    g_host_init_params[WLAN_TX5G_UPC_MIX_GAIN_CTRL_7] = 0;
}

static void host_param_init_pa_gate_param(void)
{
    /* PA bias */
    g_host_init_params[WLAN_TX2G_PA_GATE_VCTL_REG236] = 0x12081208;
    g_host_init_params[WLAN_TX2G_PA_GATE_VCTL_REG237] = 0x2424292D;
    g_host_init_params[WLAN_TX2G_PA_GATE_VCTL_REG238] = 0x24242023;
    g_host_init_params[WLAN_TX2G_PA_GATE_VCTL_REG239] = 0x24242020;
    g_host_init_params[WLAN_TX2G_PA_GATE_VCTL_REG240] = 0x24242020;
    g_host_init_params[WLAN_TX2G_PA_GATE_VCTL_REG241] = 0x24241B1B;
    g_host_init_params[WLAN_TX2G_PA_GATE_VCTL_REG242] = 0x24241B1B;
    g_host_init_params[WLAN_TX2G_PA_GATE_VCTL_REG243] = 0x24241B1B;
    g_host_init_params[WLAN_TX2G_PA_GATE_VCTL_REG244] = 0x24241B1B;

    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG253] = 0x14141414;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG254] = 0x13131313;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG255] = 0x12121212;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG256] = 0x12121212;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG257] = 0x12121212;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG258] = 0x12121212;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG259] = 0x12121212;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG260] = 0x12121212;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG261] = 0x0F0F0F0F;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG262] = 0x0D0D0D0D;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG263] = 0x0A0B0A0B;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG264] = 0x0A0A0A0A;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG265] = 0x0A0A0A0A;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG266] = 0x0A0A0A0A;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG267] = 0x0A0A0A0A;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG268] = 0x0A0A0A0A;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG269] = 0x0F0F0F0F;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG270] = 0x0D0D0D0D;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG271] = 0x0A0B0A0B;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG272] = 0x0A0A0A0A;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG273] = 0x0A0A0A0A;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG274] = 0x0A0A0A0A;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG275] = 0x0A0A0A0A;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG276] = 0x0A0A0A0A;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG277] = 0x0D0D0D0D;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG278] = 0x0D0D0D0D;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG279] = 0x0D0D0A0B;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG280_BAND1] = 0x0D0D0A0A;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG280_BAND2] = 0x0D0D0A0A;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG280_BAND3] = 0x0D0D0A0A;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG281] = 0x0D0D0A0A;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG282] = 0x0D0D0A0A;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG283] = 0x0D0D0A0A;
    g_host_init_params[WLAN_TX2G_PA_VRECT_GATE_THIN_REG284] = 0x0D0D0A0A;
}

#ifdef _PRE_WLAN_FEATURE_NRCOEX
static void host_param_init_nrcoex_param(void)
{
    /* 5g nr coex */
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_ENABLE]                  = 0;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE0_FREQ]              = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE0_40M_20M_GAP0]      = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE0_160M_80M_GAP0]     = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE0_40M_20M_GAP1]      = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE0_160M_80M_GAP1]     = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE0_40M_20M_GAP2]      = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE0_160M_80M_GAP2]     = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE0_SMALLGAP0_ACT]     = 0xFFFFFFFF;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE0_GAP01_ACT]         = 0xFFFFFFFF;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE0_GAP12_ACT]         = 0xFFFFFFFF;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE0_RXSLOT_RSSI]       = 0xFFFFFFFF;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE1_FREQ]              = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE1_40M_20M_GAP0]      = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE1_160M_80M_GAP0]     = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE1_40M_20M_GAP1]      = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE1_160M_80M_GAP1]     = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE1_40M_20M_GAP2]      = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE1_160M_80M_GAP2]     = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE1_SMALLGAP0_ACT]     = 0xFFFFFFFF;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE1_GAP01_ACT]         = 0xFFFFFFFF;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE1_GAP12_ACT]         = 0xFFFFFFFF;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE1_RXSLOT_RSSI]       = 0xFFFFFFFF;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE2_FREQ]              = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE2_40M_20M_GAP0]      = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE2_160M_80M_GAP0]     = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE2_40M_20M_GAP1]      = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE2_160M_80M_GAP1]     = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE2_40M_20M_GAP2]      = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE2_160M_80M_GAP2]     = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE2_SMALLGAP0_ACT]     = 0xFFFFFFFF;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE2_GAP01_ACT]         = 0xFFFFFFFF;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE2_GAP12_ACT]         = 0xFFFFFFFF;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE2_RXSLOT_RSSI]       = 0xFFFFFFFF;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE3_FREQ]              = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE3_40M_20M_GAP0]      = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE3_160M_80M_GAP0]     = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE3_40M_20M_GAP1]      = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE3_160M_80M_GAP1]     = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE3_40M_20M_GAP2]      = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE3_160M_80M_GAP2]     = 0x00000000;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE3_SMALLGAP0_ACT]     = 0xFFFFFFFF;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE3_GAP01_ACT]         = 0xFFFFFFFF;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE3_GAP12_ACT]         = 0xFFFFFFFF;
    g_host_init_params[WLAN_CFG_INIT_NRCOEX_RULE3_RXSLOT_RSSI]       = 0xFFFFFFFF;
}
#endif


/*
 * 函 数 名  : host_params_init_first
 * 功能描述  : 给定制化参数全局数组 g_host_init_params 附初值
 *             ini文件读取失败时用初值
 */
void host_params_init_first(void)
{
    host_param_init_netaccess_param();

    host_param_init_performance_param();
    /* 自动调频 */
    host_auto_freq_params_init();

#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU
    host_amsdu_th_params_init();
#endif

    host_params_performance_init();

    /* 低功耗 */
    g_host_init_params[WLAN_CFG_INIT_POWERMGMT_SWITCH] = OAL_TRUE;
    g_host_init_params[WLAN_CFG_INIT_PS_MODE] = WLAN_PS_MODE;
    g_host_init_params[WLAN_CFG_INIT_MIN_FAST_PS_IDLE] = WLAN_MIN_FAST_PS_IDLE;
    g_host_init_params[WLAN_CFG_INIT_MAX_FAST_PS_IDLE] = WLAN_MAX_FAST_PS_IDLE;
    g_host_init_params[WLAN_CFG_INIT_AUTO_FAST_PS_THRESH_SCREENON] = WLAN_AUTO_FAST_PS_SCREENON;
    g_host_init_params[WLAN_CFG_INIT_AUTO_FAST_PS_THRESH_SCREENOFF] = WLAN_AUTO_FAST_PS_SCREENOFF;

    /* 可维可测 */
    /* 日志级别 */
    g_host_init_params[WLAN_CFG_INIT_LOGLEVEL] = OAM_LOG_LEVEL_WARNING;

    host_param_init_rf_param();

    /* SCAN */
    g_host_init_params[WLAN_CFG_INIT_RANDOM_MAC_ADDR_SCAN] = 1;
    /* 11AC2G */
    g_host_init_params[WLAN_CFG_INIT_11AC2G_ENABLE] = 1;
    g_host_init_params[WLAN_CFG_INIT_DISABLE_CAPAB_2GHT40] = 0;
    g_host_init_params[WLAN_CFG_INIT_DUAL_ANTENNA_ENABLE] = 0;

    /* miracast */
    g_host_init_params[WLAN_CFG_INIT_PROBE_RESP_MODE] = 0x10;
    g_host_init_params[WLAN_CFG_INIT_MIRACAST_SINK_ENABLE] = 0;
    g_host_init_params[WLAN_CFG_INIT_REDUCE_MIRACAST_LOG] = 0;
    g_host_init_params[WLAN_CFG_INIT_BIND_LOWEST_LOAD_CPU] = 0;
    g_host_init_params[WLAN_CFG_INIT_CORE_BIND_CAP] = 1;
    g_host_init_params[WLAN_CFG_INIT_FAST_MODE] = 0;

    host_param_init_keepalive_param();

    host_param_init_pa_gate_param();

    host_btcoex_rssi_th_params_init();
    /* connect */
    g_host_init_params[WLAN_CFG_INIT_DDR_FREQ] = WLAN_DDR_CAHNL_FREQ;

#ifdef _PRE_WLAN_FEATURE_NRCOEX
    host_param_init_nrcoex_param();
#endif
}


/*
 * 函 数 名  : hwifi_custom_adapt_device_ini_freq_param
 * 功能描述  : 初始化device侧定制化ini频率相关的配置项
 */
OAL_STATIC int32_t hwifi_custom_adapt_device_ini_freq_param(uint8_t *puc_data, uint32_t *pul_data_len)
{
    config_device_freq_h2d_stru st_device_freq_data = {0};
    uint8_t uc_index;
    int32_t l_val, l_ret;
    uint32_t cfg_id;
    hmac_to_dmac_cfg_custom_data_stru st_syn_msg = {0};
    const uint32_t dev_data_buff_size = sizeof(st_device_freq_data.st_device_data) /
                                             sizeof(st_device_freq_data.st_device_data[0]);

    if (puc_data == NULL) {
        oam_error_log1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_freq_param puc_data is NULL last data_len[%d].}",
                       *pul_data_len);
        return INI_FAILED;
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_INI_FREQ_ID;

    for (uc_index = 0, cfg_id = WLAN_CFG_INIT_PSS_THRESHOLD_LEVEL_0; uc_index < dev_data_buff_size; uc_index++) {
        l_val = hwifi_get_init_value(CUS_TAG_INI, cfg_id);
        if (PPS_VALUE_0 <= l_val && l_val <= PPS_VALUE_3) {
            st_device_freq_data.st_device_data[uc_index].speed_level = (uint32_t)l_val;
            cfg_id += WLAN_CFG_ID_OFFSET;
        } else {
            oam_error_log1(0, OAM_SF_CFG,
                           "{hwifi_custom_adapt_device_ini_freq_param get wrong PSS_THRESHOLD_LEVEL[%d]!}", l_val);
            return OAL_FALSE;
        }
    }

    for (uc_index = 0, cfg_id = WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_0; uc_index < dev_data_buff_size; uc_index++) {
        l_val = hwifi_get_init_value(CUS_TAG_INI, cfg_id);
        if (l_val >= FREQ_IDLE && l_val <= FREQ_HIGHEST) {
            st_device_freq_data.st_device_data[uc_index].cpu_freq_level = (uint32_t)l_val;
            cfg_id++;
        } else {
            oam_error_log1(0, OAM_SF_CFG,
                           "{hwifi_custom_adapt_device_ini_freq_param get wrong DEVICE_TYPE_LEVEL [%d]!}", l_val);
            return OAL_FALSE;
        }
    }
    st_device_freq_data.uc_set_type = FREQ_SYNC_DATA;

    st_syn_msg.len = sizeof(st_device_freq_data);

    l_ret = memcpy_s(puc_data, (WLAN_LARGE_NETBUF_SIZE - *pul_data_len), &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    l_ret += memcpy_s(puc_data + CUSTOM_MSG_DATA_HDR_LEN,
                      (WLAN_LARGE_NETBUF_SIZE - *pul_data_len - CUSTOM_MSG_DATA_HDR_LEN),
                      &st_device_freq_data,
                      sizeof(st_device_freq_data));
    if (l_ret != EOK) {
        oam_error_log2(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_freq_param::memcpy_s fail[%d]. data_len[%d]}",
                       l_ret, *pul_data_len);
        *pul_data_len += (st_syn_msg.len + CUSTOM_MSG_DATA_HDR_LEN);
        return OAL_FAIL;
    }

    *pul_data_len += (st_syn_msg.len + CUSTOM_MSG_DATA_HDR_LEN);
    oam_warning_log1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_freq_param da_len[%d].}", *pul_data_len);

    return OAL_SUCC;
}
/*
 * 函 数 名  : hwifi_custom_adapt_ini_device_perf_param
 * 功能描述  : 性能device定制化参数初始化
 */
OAL_STATIC void hwifi_custom_adapt_device_ini_perf_param(uint8_t *puc_data, uint32_t *pul_data_len)
{
    const uint32_t itoa_len = 5; /* 整数转字符串之后的长度 */
    int8_t ac_tmp[8];            /* 整数转字符串之后的buff为最大8B */
    uint8_t uc_sdio_assem_h2d, uc_sdio_assem_d2h;
    int32_t l_ret;
    config_device_perf_h2d_stru st_device_perf;
    hmac_to_dmac_cfg_custom_data_stru st_syn_msg = {0};

    if (puc_data == NULL) {
        oam_error_log1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_perf_param puc_data is NULL last data_len[%d].}",
                       *pul_data_len);
        return;
    }

    memset_s(ac_tmp, sizeof(ac_tmp), 0, sizeof(ac_tmp));
    memset_s(&st_device_perf, sizeof(st_device_perf), 0, sizeof(st_device_perf));

    st_syn_msg.en_syn_id = CUSTOM_CFGID_INI_PERF_ID;

    /* SDIO FLOWCTRL */
    // device侧做合法性判断
    oal_itoa(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_USED_MEM_FOR_START),
             st_device_perf.ac_used_mem_param, itoa_len);
    oal_itoa(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_USED_MEM_FOR_STOP), ac_tmp, itoa_len);
    st_device_perf.ac_used_mem_param[OAL_STRLEN(st_device_perf.ac_used_mem_param)] = ' ';
    l_ret = memcpy_s(st_device_perf.ac_used_mem_param + OAL_STRLEN(st_device_perf.ac_used_mem_param),
                     (sizeof(st_device_perf.ac_used_mem_param) - OAL_STRLEN(st_device_perf.ac_used_mem_param)),
                     ac_tmp, OAL_STRLEN(ac_tmp));
    if (l_ret != EOK) {
        oam_error_log1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_perf_param memcpy_s failed[%d].}", l_ret);
        return;
    }

    st_device_perf.ac_used_mem_param[OAL_STRLEN(st_device_perf.ac_used_mem_param)] = '\0';

    /* SDIO ASSEMBLE COUNT:H2D */
    uc_sdio_assem_h2d = (uint8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_SDIO_H2D_ASSEMBLE_COUNT);
    // 判断值的合法性
    if (uc_sdio_assem_h2d >= 1 && uc_sdio_assem_h2d <= HISDIO_HOST2DEV_SCATT_MAX) {
        g_hcc_assemble_count = uc_sdio_assem_h2d;
    } else {
        oam_error_log2(0, OAM_SF_ANY, "{hwifi_custom_adapt_device_ini_perf_param::sdio_assem_h2d[%d] out of range(0,%d]\
            check value in ini file!}\r\n", uc_sdio_assem_h2d, HISDIO_HOST2DEV_SCATT_MAX);
    }

    /* SDIO ASSEMBLE COUNT:D2H */
    uc_sdio_assem_d2h = (uint8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_SDIO_D2H_ASSEMBLE_COUNT);
    // 判断值的合法性
    if (uc_sdio_assem_d2h >= 1 && uc_sdio_assem_d2h <= HISDIO_DEV2HOST_SCATT_MAX) {
        st_device_perf.uc_sdio_assem_d2h = uc_sdio_assem_d2h;
    } else {
        st_device_perf.uc_sdio_assem_d2h = HISDIO_DEV2HOST_SCATT_MAX;
        oam_error_log2(0, OAM_SF_ANY, "{hwifi_custom_adapt_device_ini_perf_param::sdio_assem_d2h[%d] out of range(0,%d]\
            check value in ini file!}\r\n", uc_sdio_assem_d2h, HISDIO_DEV2HOST_SCATT_MAX);
    }

    st_syn_msg.len = sizeof(st_device_perf);

    l_ret = memcpy_s(puc_data, (WLAN_LARGE_NETBUF_SIZE - *pul_data_len), &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    l_ret += memcpy_s(puc_data + CUSTOM_MSG_DATA_HDR_LEN,
                      (WLAN_LARGE_NETBUF_SIZE - *pul_data_len - CUSTOM_MSG_DATA_HDR_LEN),
                      &st_device_perf, sizeof(st_device_perf));
    if (l_ret != EOK) {
        oam_error_log2(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_perf_param::memcpy_s fail[%d]. data_len[%d]}",
                       l_ret, *pul_data_len);
        *pul_data_len += (sizeof(st_device_perf) + CUSTOM_MSG_DATA_HDR_LEN);
        return;
    }

    *pul_data_len += (sizeof(st_device_perf) + CUSTOM_MSG_DATA_HDR_LEN);

    oam_warning_log1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_perf_param::da_len[%d].}", *pul_data_len);
}

/*
 * 函 数 名  : hwifi_custom_adapt_device_ini_linkloss_param
 * 功能描述  : linkloss门限定制化
 */
OAL_STATIC void hwifi_custom_adapt_device_ini_linkloss_param(uint8_t *puc_data, uint32_t *pul_data_len)
{
    uint8_t ast_threshold[WLAN_LINKLOSS_MODE_BUTT] = {0};
    int32_t  l_ret;

    hmac_to_dmac_cfg_custom_data_stru st_syn_msg = {0};

    if (puc_data == NULL) {
        oam_error_log1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_linkloss_param::puc_data is NULL data_len[%d].}",
                       *pul_data_len);
        return;
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_INI_LINKLOSS_ID;

    ast_threshold[WLAN_LINKLOSS_MODE_BT] =
        (uint8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_LINK_LOSS_THRESHOLD_BT);
    ast_threshold[WLAN_LINKLOSS_MODE_DBAC] =
        (uint8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_LINK_LOSS_THRESHOLD_DBAC);
    ast_threshold[WLAN_LINKLOSS_MODE_NORMAL] =
        (uint8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_LINK_LOSS_THRESHOLD_NORMAL);

    st_syn_msg.len = sizeof(ast_threshold);

    l_ret = memcpy_s(puc_data, (WLAN_LARGE_NETBUF_SIZE - *pul_data_len), &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    l_ret += memcpy_s(puc_data + CUSTOM_MSG_DATA_HDR_LEN,
                      (WLAN_LARGE_NETBUF_SIZE - *pul_data_len - CUSTOM_MSG_DATA_HDR_LEN),
                      &ast_threshold, sizeof(ast_threshold));
    if (l_ret != EOK) {
        oam_error_log2(0, OAM_SF_CFG,
            "{hwifi_custom_adapt_device_ini_linkloss_param::memcpy_s fail[%d]. data_len[%d]}", l_ret, *pul_data_len);
        *pul_data_len += (sizeof(ast_threshold) + CUSTOM_MSG_DATA_HDR_LEN);
        return;
    }

    *pul_data_len += (sizeof(ast_threshold) + CUSTOM_MSG_DATA_HDR_LEN);

    oam_warning_log1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_linkloss_param::da_len[%d].}", *pul_data_len);
}
/*
 * 函 数 名  : hwifi_custom_adapt_device_ini_pm_switch_param
 * 功能描述  : 低功耗定制化
 */
OAL_STATIC void hwifi_custom_adapt_device_ini_pm_switch_param(uint8_t *puc_data, uint32_t *pul_data_len)
{
    hmac_to_dmac_cfg_custom_data_stru st_syn_msg;
    int32_t  l_ret;

    if (puc_data == NULL) {
        oam_error_log1(0, OAM_SF_CFG,
            "{hwifi_custom_adapt_device_ini_pm_switch_param::puc_data is NULL data_len[%d].}", *pul_data_len);
        return;
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_INI_PM_SWITCH_ID;

    g_wlan_device_pm_switch = (uint8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_POWERMGMT_SWITCH);
    g_wlan_host_pm_switch = (g_wlan_device_pm_switch == WLAN_DEV_ALL_ENABLE ||
                          g_wlan_device_pm_switch == WLAN_DEV_LIGHT_SLEEP_SWITCH_EN) ? OAL_TRUE : OAL_FALSE;

    st_syn_msg.auc_msg_body[0] = g_wlan_device_pm_switch;
    st_syn_msg.len = sizeof(st_syn_msg) - CUSTOM_MSG_DATA_HDR_LEN;

    l_ret = memcpy_s(puc_data, (WLAN_LARGE_NETBUF_SIZE - *pul_data_len), &st_syn_msg, sizeof(st_syn_msg));
    if (l_ret != EOK) {
        oam_error_log2(0, OAM_SF_CFG,
            "{hwifi_custom_adapt_device_ini_pm_switch_param::memcpy_s fail[%d]. data_len[%d]}", l_ret, *pul_data_len);
        *pul_data_len += sizeof(st_syn_msg);
        return;
    }

    *pul_data_len += sizeof(st_syn_msg);

    oam_warning_log3(0, OAM_SF_CFG,
                     "{hwifi_custom_adapt_device_ini_pm_switch_param::da_len[%d].device[%d]host[%d]pm switch}",
                     *pul_data_len, g_wlan_device_pm_switch, g_wlan_host_pm_switch);
}

/*
 * 函 数 名  : hwifi_custom_adapt_device_ini_fast_ps_check_cnt
 * 功能描述  : max ps mode check cnt定制化，20ms定时器检查几次，即调整idle超时时间
 */
OAL_STATIC void hwifi_custom_adapt_device_ini_fast_ps_check_cnt(uint8_t *puc_data, uint32_t *pul_data_len)
{
    hmac_to_dmac_cfg_custom_data_stru st_syn_msg;
    int32_t  l_ret;

    if (puc_data == NULL) {
        oam_error_log1(0, OAM_SF_CFG,
            "{hwifi_custom_adapt_device_ini_fast_ps_check_cnt::puc_data is NULL data_len[%d].}", *pul_data_len);
        return;
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_INI_PS_FAST_CHECK_CNT_ID;

    st_syn_msg.auc_msg_body[BYTE_OFFSET_0] = g_wlan_min_fast_ps_idle;
    st_syn_msg.auc_msg_body[BYTE_OFFSET_1] = g_wlan_max_fast_ps_idle;
    st_syn_msg.auc_msg_body[BYTE_OFFSET_2] = g_wlan_auto_ps_screen_on;
    st_syn_msg.auc_msg_body[BYTE_OFFSET_3] = g_wlan_auto_ps_screen_off;
    st_syn_msg.len = sizeof(st_syn_msg) - CUSTOM_MSG_DATA_HDR_LEN;

    l_ret = memcpy_s(puc_data, (WLAN_LARGE_NETBUF_SIZE - *pul_data_len), &st_syn_msg, sizeof(st_syn_msg));
    if (l_ret != EOK) {
        oam_error_log2(0, OAM_SF_CFG,
            "{hwifi_custom_adapt_device_ini_fast_ps_check_cnt::memcpy_s fail[%d]. data_len[%d]}", l_ret, *pul_data_len);
        *pul_data_len += sizeof(st_syn_msg);
        return;
    }

    *pul_data_len += sizeof(st_syn_msg);

    oam_warning_log4(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_fast_ps_check_cnt:fast_ps idle min/max[%d/%d], \
        auto_ps thresh screen on/off[%d/%d]}", g_wlan_min_fast_ps_idle, g_wlan_max_fast_ps_idle,
        g_wlan_auto_ps_screen_on, g_wlan_auto_ps_screen_off);
}

/*
 * BUCK stay in fastmode
 */
OAL_STATIC void hwifi_custom_adapt_device_ini_fast_mode(uint8_t *data, uint32_t *data_len)
{
    errno_t ret;
    hmac_to_dmac_cfg_custom_data_stru syn_msg = {0};

    if (data == NULL) {
        oam_error_log1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_fast_mode:: data_len[%d].}", *data_len);
        return;
    }

    syn_msg.en_syn_id = CUSTOM_CFGID_INI_FAST_MODE;
    syn_msg.auc_msg_body[0] = (uint8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_FAST_MODE);
    syn_msg.len = sizeof(syn_msg) - CUSTOM_MSG_DATA_HDR_LEN;
    ret = memcpy_s(data, (WLAN_LARGE_NETBUF_SIZE - *data_len), &syn_msg, sizeof(syn_msg));
    if (ret != EOK) {
        oam_error_log2(0, OAM_SF_CFG,
            "{hwifi_custom_adapt_device_ini_fast_mode:: memcpy_s fail[%d]. data_len[%d]}", ret, *data_len);
        *data_len += sizeof(syn_msg);
        return;
    }

    *data_len += sizeof(syn_msg);

    oam_warning_log2(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_fast_mode:fast_mode = [%d], data_len = [%d]}",
                     syn_msg.auc_msg_body[0], *data_len);
}
/*
 * 函 数 名  : hwifi_custom_adapt_device_ini_ldac_m2s_rssi_param
 * 功能描述  : ldac m2s rssi门限定制化
 */
OAL_STATIC void hwifi_custom_adapt_device_ini_ldac_m2s_rssi_param(uint8_t *puc_data, uint32_t *pul_data_len)
{
    int8_t ast_ldac_m2s_rssi_threshold[WLAN_M2S_LDAC_RSSI_BUTT] = { 0, 0 }; /* 当前m2s和s2m门限，后续扩展再添加枚举 */
    int32_t l_ret;

    hmac_to_dmac_cfg_custom_data_stru st_syn_msg = {0};

    if (puc_data == NULL) {
        oam_error_log1(0, OAM_SF_CFG,
            "{hwifi_custom_adapt_device_ini_ldac_m2s_rssi_param::puc_data is NULL data_len[%d].}", *pul_data_len);
        return;
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_LDAC_M2S_TH_ID;

    ast_ldac_m2s_rssi_threshold[WLAN_M2S_LDAC_RSSI_TO_SISO] =
        (int8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_LDAC_THRESHOLD_M2S);
    ast_ldac_m2s_rssi_threshold[WLAN_M2S_LDAC_RSSI_TO_MIMO] =
        (int8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_LDAC_THRESHOLD_S2M);

    st_syn_msg.len = sizeof(ast_ldac_m2s_rssi_threshold);

    l_ret = memcpy_s(puc_data, (WLAN_LARGE_NETBUF_SIZE - *pul_data_len), &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    l_ret += memcpy_s(puc_data + CUSTOM_MSG_DATA_HDR_LEN,
                      (WLAN_LARGE_NETBUF_SIZE - *pul_data_len - CUSTOM_MSG_DATA_HDR_LEN),
                      &ast_ldac_m2s_rssi_threshold,
                      sizeof(ast_ldac_m2s_rssi_threshold));
    if (l_ret != EOK) {
        oam_error_log2(0, OAM_SF_CFG,
            "{hwifi_custom_adapt_device_ini_ldac_m2s_rssi_param:memcpy_s fail[%d],data_len[%d]}", l_ret, *pul_data_len);
        *pul_data_len += (sizeof(ast_ldac_m2s_rssi_threshold) + CUSTOM_MSG_DATA_HDR_LEN);
        return;
    }

    *pul_data_len += (sizeof(ast_ldac_m2s_rssi_threshold) + CUSTOM_MSG_DATA_HDR_LEN);

    oam_warning_log3(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_ldac_m2s_rssi_param::da_len[%d], \
        m2s[%d], s2m[%d].}", *pul_data_len,
        ast_ldac_m2s_rssi_threshold[WLAN_M2S_LDAC_RSSI_TO_SISO],
        ast_ldac_m2s_rssi_threshold[WLAN_M2S_LDAC_RSSI_TO_MIMO]);
}
/*
 * 函 数 名  : hwifi_custom_adapt_device_ini_btcoex_mcm_rssi_param
 * 功能描述  : btcoex mcm rssi门限定制化
 */
OAL_STATIC void hwifi_custom_adapt_device_ini_btcoex_mcm_rssi_param(uint8_t *puc_data, uint32_t *pul_data_len)
{
    int8_t ast_ldac_m2s_rssi_threshold[WLAN_BTCOEX_RSSI_MCM_BUTT] = { 0, 0 }; /* 当前m2s和s2m门限，后续扩展再添加枚举 */
    int32_t ret;

    hmac_to_dmac_cfg_custom_data_stru st_syn_msg = {0};

    if (puc_data == NULL) {
        oam_error_log1(0, OAM_SF_CFG,
            "{hwifi_custom_adapt_device_ini_btcoex_mcm_rssi_param::puc_data is NULL data_len[%d].}", *pul_data_len);
        return;
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_BTCOEX_MCM_TH_ID;

    ast_ldac_m2s_rssi_threshold[WLAN_BTCOEX_RSSI_MCM_DOWN] =
        (int8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_BTCOEX_THRESHOLD_MCM_DOWN);
    ast_ldac_m2s_rssi_threshold[WLAN_BTCOEX_RSSI_MCM_UP] =
        (int8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_BTCOEX_THRESHOLD_MCM_UP);

    st_syn_msg.len = sizeof(ast_ldac_m2s_rssi_threshold);

    ret = memcpy_s(puc_data, (WLAN_LARGE_NETBUF_SIZE - *pul_data_len), &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    ret += memcpy_s(puc_data + CUSTOM_MSG_DATA_HDR_LEN,
        (WLAN_LARGE_NETBUF_SIZE - *pul_data_len - CUSTOM_MSG_DATA_HDR_LEN),
        &ast_ldac_m2s_rssi_threshold, sizeof(ast_ldac_m2s_rssi_threshold));
    if (ret != EOK) {
        oam_error_log2(0, OAM_SF_CFG,
            "{hwifi_custom_adapt_device_ini_btcoex_mcm_rssi_param:memcpy_s fail[%d],data_len[%d]}", ret, *pul_data_len);
        *pul_data_len += (sizeof(ast_ldac_m2s_rssi_threshold) + CUSTOM_MSG_DATA_HDR_LEN);
        return;
    }

    *pul_data_len += (sizeof(ast_ldac_m2s_rssi_threshold) + CUSTOM_MSG_DATA_HDR_LEN);

    oam_warning_log3(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_btcoex_mcm_rssi_param::da_len[%d], \
        mcm down[%d], mcm up[%d].}", *pul_data_len, ast_ldac_m2s_rssi_threshold[WLAN_BTCOEX_RSSI_MCM_DOWN],
        ast_ldac_m2s_rssi_threshold[WLAN_BTCOEX_RSSI_MCM_UP]);
}


#ifdef _PRE_WLAN_FEATURE_NRCOEX
/*
 * 函 数 名  : hwifi_custom_adapt_device_ini_nrcoex_param
 * 功能描述  : nr coex干扰表定制化
*/
OAL_STATIC void hwifi_custom_adapt_device_ini_nrcoex_param(uint8_t *puc_data, uint32_t *pul_data_len)
{
    hmac_to_dmac_cfg_custom_data_stru   st_syn_msg = {0};
    nrcoex_cfg_info_stru ini_info = {0};
    int32_t*           freq = &ini_info.un_nrcoex_rule_data[0].st_nrcoex_rule_data.freq;
    uint32_t           cfg_id;
    int32_t            l_ret;
    uint8_t idx;

    if (puc_data == NULL) {
        oam_error_log1(0, OAM_SF_CFG,
            "{hwifi_custom_adapt_device_ini_nrcoex_param::puc_data is NULL data_len[%d].}", *pul_data_len);
        return;
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_NRCOEX_ID;

    ini_info.uc_nrcoex_enable = (uint8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_NRCOEX_ENABLE);

    idx = DMAC_WLAN_NRCOEX_INTERFERE_RULE_NUM - 1;
    for (cfg_id = WLAN_CFG_INIT_NRCOEX_RULE0_FREQ; cfg_id <= WLAN_CFG_INIT_NRCOEX_RULE3_RXSLOT_RSSI; cfg_id++) {
        if (freq > (&ini_info.un_nrcoex_rule_data[idx].st_nrcoex_rule_data.l_rxslot_rssi)) {
            oam_error_log1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_nrcoex_param:overflow, cfg_id[%d]}", cfg_id);
            return;
        }
        *freq = hwifi_get_init_value(CUS_TAG_INI, cfg_id);
        freq++;
    }

    st_syn_msg.len = sizeof(ini_info);

    l_ret = memcpy_s(puc_data, (WLAN_LARGE_NETBUF_SIZE - *pul_data_len), &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    l_ret += memcpy_s(puc_data + CUSTOM_MSG_DATA_HDR_LEN,
        (WLAN_LARGE_NETBUF_SIZE - *pul_data_len - CUSTOM_MSG_DATA_HDR_LEN),
        &ini_info, sizeof(ini_info));
    if (l_ret != EOK) {
        oam_error_log2(0, OAM_SF_CFG,
            "{hwifi_custom_adapt_device_ini_nrcoex_param::memcpy_s fail[%d]. data_len[%d]}", l_ret, *pul_data_len);
        *pul_data_len += (sizeof(ini_info) + CUSTOM_MSG_DATA_HDR_LEN);
        return;
    }

    *pul_data_len += (sizeof(ini_info) + CUSTOM_MSG_DATA_HDR_LEN);

    oam_warning_log1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_nrcoex_param::da_len[%d].}", *pul_data_len);
}
#endif

/*
 * 函 数 名  : hwifi_custom_adapt_device_ini_end_param
 * 功能描述  : 配置定制化参数结束标志
 */
OAL_STATIC void hwifi_custom_adapt_device_ini_end_param(uint8_t *puc_data, uint32_t *pul_data_len)
{
    int32_t l_ret;
    hmac_to_dmac_cfg_custom_data_stru st_syn_msg = {0};

    if (puc_data == NULL) {
        oam_error_log1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_end_param:NULL data_len[%d].}", *pul_data_len);
        return;
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_INI_ENDING_ID;
    st_syn_msg.len = 0;

    l_ret = memcpy_s(puc_data, (WLAN_LARGE_NETBUF_SIZE - *pul_data_len), &st_syn_msg, sizeof(st_syn_msg));
    if (l_ret != EOK) {
        oam_error_log2(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_end_param::memcpy_s fail[%d]. data_len[%d]}",
                       l_ret, *pul_data_len);
        *pul_data_len += sizeof(st_syn_msg);
        return;
    }

    *pul_data_len += sizeof(st_syn_msg);

    oam_warning_log1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_end_param::da_len[%d].}", *pul_data_len);
}


/*
 * 函 数 名  : hwifi_custom_adapt_device_ini_param
 * 功能描述  : ini device侧上电前定制化参数适配
 */
int32_t hwifi_custom_adapt_device_ini_param(uint8_t *puc_data)
{
    uint32_t data_len = 0;

    if (puc_data == NULL) {
        oam_error_log0(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_param::puc_data is NULL.}");
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
    /* 自动调频 */
    if (hwifi_custom_adapt_device_ini_freq_param(puc_data + data_len, &data_len) != OAL_SUCC) {
        oam_error_log0(0, OAM_SF_CFG, "hwifi_custom_adapt_device_ini_param::custom_adapt_device_ini_freq_param fail");
    }

    /* 性能 */
    hwifi_custom_adapt_device_ini_perf_param(puc_data + data_len, &data_len);

    /* linkloss */
    hwifi_custom_adapt_device_ini_linkloss_param(puc_data + data_len, &data_len);

    /* 低功耗 */
    hwifi_custom_adapt_device_ini_pm_switch_param(puc_data + data_len, &data_len);

    /* fast ps mode 检查次数 */
    hwifi_custom_adapt_device_ini_fast_ps_check_cnt(puc_data + data_len, &data_len);

    /* BUCK stay in fastmode */
    hwifi_custom_adapt_device_ini_fast_mode(puc_data + data_len, &data_len);

    /* ldac m2s rssi门限 */
    hwifi_custom_adapt_device_ini_ldac_m2s_rssi_param(puc_data + data_len, &data_len);

    /* ldac m2s rssi门限 */
    hwifi_custom_adapt_device_ini_btcoex_mcm_rssi_param(puc_data + data_len, &data_len);

#ifdef _PRE_WLAN_FEATURE_NRCOEX
    /* nr coex 定制化参数 */
    hwifi_custom_adapt_device_ini_nrcoex_param(puc_data + data_len, &data_len);
#endif

    /* 结束 */
    hwifi_custom_adapt_device_ini_end_param(puc_data + data_len, &data_len);

    return data_len;
}
