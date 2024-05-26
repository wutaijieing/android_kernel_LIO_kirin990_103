/*
 * 版权所有 (c) 华为技术有限公司 2018-2020
 * 功能说明 : WIFI 芯片差异接口文件
 * 作    者 : duankaiyong
 * 创建日期 : 2020年6月19日
 */

#include "wlan_chip.h"
#include "oal_main.h"

#ifdef _PRE_WLAN_FEATURE_HIEX
#include "mac_hiex.h"
#endif
#ifdef _PRE_WLAN_FEATURE_FTM
#include "mac_ftm.h"
#endif
#include "hmac_ext_if.h"
#include "hmac_auto_adjust_freq.h"
#include "hmac_blockack.h"
#include "hmac_cali_mgmt.h"
#include "hmac_tx_data.h"
#include "hmac_host_tx_data.h"
#include "hmac_tx_amsdu.h"
#include "hmac_hcc_adapt.h"
#include "hmac_stat.h"
#include "hmac_scan.h"


#include "hisi_customize_wifi.h"
#include "hisi_customize_wifi_shenkuo.h"

#include "hmac_config.h"
#ifdef _PRE_WLAN_FEATURE_TCP_ACK_BUFFER
#include "hmac_tcp_ack_buf.h"
#endif
#ifdef _PRE_WLAN_FEATURE_CSI
#include "hmac_csi.h"
#endif
#ifdef _PRE_WLAN_FEATURE_FTM
#include "hmac_ftm.h"
#endif
#ifdef _PRE_WLAN_FEATURE_WMMAC
#include "hmac_wmmac.h"
#endif

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#include "plat_pm_wlan.h"
#endif
#ifdef _PRE_WLAN_FEATURE_DFS
#include "hmac_dfs.h"
#endif
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
#include "hmac_mcast_ampdu.h"
#endif
#include "mac_mib.h"
#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WLAN_CHIP_1106_C

const struct wlan_chip_ops g_wlan_chip_ops_1161 = {
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    .host_global_init_param = hwifi_cfg_host_global_init_param_shenkuo,
    .first_power_on_mark = wlan_first_powon_mark_shenkuo,
    .first_powon_cali_completed = hmac_first_powon_cali_completed,
    .is_aput_support_160M = wlan_chip_is_aput_support_160m_shenkuo,
    .get_flow_ctrl_used_mem = wlan_chip_get_flow_ctrl_used_mem_shenkuo,
    .force_update_custom_params = hwifi_force_update_rf_params_shenkuo,
    .init_nvram_main = hwifi_config_init_fcc_ce_params_shenkuo,
    .cpu_freq_ini_param_init = hwifi_config_cpu_freq_ini_param_shenkuo,
    .host_global_ini_param_init = hwifi_config_host_global_ini_param_shenkuo,
    .get_selfstudy_country_flag = wlan_chip_get_selfstudy_country_flag_shenkuo,
    .custom_cali = wal_custom_cali_shenkuo,
    .custom_cali_data_host_addr_init = hwifi_rf_cali_data_host_addr_init_shenkuo,
    .send_cali_data = wal_send_cali_data_shenkuo,
    .send_20m_all_chn_cali_data = wal_send_cali_data_shenkuo,
    .send_dbdc_scan_all_chn_cali_data = hmac_send_dbdc_scan_cali_data_shenkuo,
    .custom_host_read_cfg_init = hwifi_custom_host_read_cfg_init_shenkuo,
    .hcc_customize_h2d_data_cfg = hwifi_hcc_customize_h2d_data_cfg_shenkuo,
    .show_customize_info = hwifi_show_customize_info_shenkuo,
    .get_sar_ctrl_params = hwifi_get_sar_ctrl_params_shenkuo,
    .get_11ax_switch_mask = wlan_chip_get_11ax_switch_mask_shenkuo,
    .get_11ac2g_enable = wlan_chip_get_11ac2g_enable_shenkuo,
    .get_probe_resp_mode = wlan_chip_get_probe_resp_mode_shenkuo,
    .get_trx_switch = wlan_chip_get_trx_switch_shenkuo,
    .get_d2h_access_ddr = wlan_chip_get_d2h_access_ddr_shenkuo,
#endif
    .h2d_cmd_need_filter = wlan_chip_h2d_cmd_need_filter_shenkuo,
    .update_cfg80211_mgmt_tx_wait_time = wlan_chip_update_cfg80211_mgmt_tx_wait_time_shenkuo,
    // 收发和聚合相关
    .ba_rx_hdl_init = hmac_ba_rx_win_init,
    .check_need_setup_ba_session = wlan_chip_check_need_setup_ba_session_shenkuo,
    .tx_update_amsdu_num = hmac_update_amsdu_num_shenkuo,
    .check_need_process_bar = wlan_chip_check_need_process_bar_shenkuo,
    .ba_send_reorder_timeout = hmac_ba_send_ring_reorder_timeout,
    .ba_need_check_lut_idx = wlan_chip_ba_need_check_lut_idx_shenkuo,
#ifdef _PRE_WLAN_FEATURE_TCP_ACK_BUFFER
    .tcp_ack_buff_config = hmac_tcp_ack_buff_config_shenkuo,
#endif
#ifdef _PRE_WLAN_FEATURE_WMMAC
    .wmmac_need_degrade_for_ts = hmac_need_degrade_for_ts_shenkuo,
#endif
    .update_arp_tid = hmac_update_arp_tid_shenkuo,
    .get_6g_flag = mac_get_rx_6g_flag_shenkuo,
    // 校准相关
    .send_cali_matrix_data = hmac_send_cali_matrix_data_shenkuo,
    .save_cali_event = hmac_save_cali_event_shenkuo,
    .update_cur_chn_cali_data = hmac_update_cur_chn_cali_data_shenkuo,
    .get_chn_radio_cap = wlan_chip_get_chn_radio_cap_shenkuo,
#ifdef _PRE_WLAN_FEATURE_11AX
    .mac_vap_init_mib_11ax = mac_vap_init_mib_11ax_shenkuo,
    .tx_set_frame_htc = hmac_tx_set_qos_frame_htc,
#endif
    .mac_mib_set_auth_rsp_time_out = wlan_chip_mac_mib_set_auth_rsp_time_out_shenkuo,
    .mac_vap_need_set_user_htc_cap = mac_vap_need_set_user_htc_cap_shenkuo,
#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU
    .tx_encap_large_skb_amsdu = hmac_tx_encap_large_skb_amsdu_shenkuo, /* 大包AMDPU+大包AMSDU入口 shenkuo 不生效 */
#endif
    .check_headroom_len = check_headroom_length,
    .adjust_netbuf_data = hmac_format_netbuf_header,
    .proc_query_station_packets = wlan_chip_proc_query_station_packets_shenkuo,
    .scan_req_alloc_and_fill_netbuf = wlan_chip_scan_req_alloc_and_fill_netbuf_shenkuo,
#ifdef _PRE_WLAN_FEATURE_SNIFFER
    .set_sniffer_config = hmac_config_set_sniffer_shenkuo,
#endif
#ifdef _PRE_WLAN_FEATURE_CSI
    .dcsi_config = hmac_device_csi_config,
    .hcsi_config = hmac_host_csi_config,
#endif
#ifdef _PRE_WLAN_FEATURE_FTM
    .dftm_config = hmac_device_ftm_config,
    .hftm_config = hmac_host_ftm_config,
    .rrm_proc_rm_request = hmac_rrm_proc_rm_request_shenkuo,
    .dconfig_wifi_rtt_config = hmac_device_wifi_rtt_config,
    .hconfig_wifi_rtt_config = hmac_host_wifi_rtt_config,
    .ftm_vap_init = hmac_ftm_vap_init_shenkuo,
#endif
#ifdef _PRE_WLAN_FEATURE_DFS
    .start_zero_wait_dfs = hmac_config_start_zero_wait_dfs_handle,
#endif
    .update_rxctl_data_type = NULL,
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
    .mcast_ampdu_rx_ba_init = hmac_mcast_ampdu_rx_win_init,
    .mcast_stats_stru_init = hmac_mcast_ampdu_stats_stru_init,
#endif
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU_HW
    .mcast_ampdu_sta_add_multi_user = hmac_mcast_ampdu_sta_add_multi_user,
#endif
    .is_dbdc_ini_en = wlan_chip_is_dbdc_ini_en_shenkuo,
#if defined(CONFIG_ARCH_HISI) &&  defined(CONFIG_NR_CPUS)
#if CONFIG_NR_CPUS > OAL_BUS_HPCPU_NUM
    .frw_task_bind_cpu = hmac_rx_frw_task_bind_cpu,
#endif
#endif

    .send_connect_security_params = wlan_chip_send_connect_security_params_shenkuo,
    .defrag_is_pn_abnormal = wlan_chip_defrag_is_pn_abnormal_shenkuo,
    .is_support_hw_wapi = wlan_chip_is_support_hw_wapi_bisheng,
    .get_scaned_result_extend_info = wlan_chip_get_scaned_result_extend_info_shenkuo,
    .get_scaned_payload_extend_len = wlan_chip_get_scaned_payload_extend_len_shenkuo,
    .tx_pt_mcast_set_cb = NULL,
};
