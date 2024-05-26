

#ifndef __WLAN_CHIP_I_H__
#define __WLAN_CHIP_I_H__

#include "wlan_chip.h"
#include "oneimage.h"
#include "oal_main.h"
#include "hd_event.h"
uint32_t wlan_chip_ops_init(void);

OAL_STATIC OAL_INLINE uint32_t wlan_chip_custom_cali(void)
{
    if (g_wlan_chip_ops->custom_cali != NULL) {
        return g_wlan_chip_ops->custom_cali();
    }
    return OAL_FAIL;
}
OAL_STATIC OAL_INLINE void wlan_chip_custom_host_cali_data_init(void)
{
    if (g_wlan_chip_ops->custom_cali_data_host_addr_init != NULL) {
        g_wlan_chip_ops->custom_cali_data_host_addr_init();
    }
}
OAL_STATIC OAL_INLINE uint32_t wlan_chip_custom_host_read_cfg_init(void)
{
    if (g_wlan_chip_ops->custom_host_read_cfg_init != NULL) {
        return g_wlan_chip_ops->custom_host_read_cfg_init();
    }
    return OAL_FAIL;
}

OAL_STATIC OAL_INLINE uint32_t wlan_chip_hcc_customize_h2d_data_cfg(void)
{
    if (g_wlan_chip_ops->hcc_customize_h2d_data_cfg != NULL) {
        return g_wlan_chip_ops->hcc_customize_h2d_data_cfg();
    }
    return OAL_FAIL;
}

OAL_STATIC OAL_INLINE void wlan_chip_show_customize_info(void)
{
    if (g_wlan_chip_ops->show_customize_info != NULL) {
        g_wlan_chip_ops->show_customize_info();
    }
}

OAL_STATIC OAL_INLINE uint32_t wlan_chip_get_sar_ctrl_params(uint8_t lvl_num,
    uint8_t *data_addr, uint16_t *data_len, uint16_t dest_len)
{
    if (g_wlan_chip_ops->get_sar_ctrl_params != NULL) {
        return g_wlan_chip_ops->get_sar_ctrl_params(lvl_num, data_addr, data_len, dest_len);
    }
    return OAL_FAIL;
}

OAL_STATIC OAL_INLINE uint32_t wlan_chip_get_11ax_switch_mask(void)
{
    if (g_wlan_chip_ops->get_11ax_switch_mask != NULL) {
        return g_wlan_chip_ops->get_11ax_switch_mask();
    }
    return 0;
}

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 wlan_chip_get_11ac2g_enable(void)
{
    if (g_wlan_chip_ops->get_11ac2g_enable != NULL) {
        return g_wlan_chip_ops->get_11ac2g_enable();
    }
    return OAL_FALSE;
}

OAL_STATIC OAL_INLINE uint32_t wlan_chip_get_probe_resp_mode(void)
{
    if (g_wlan_chip_ops->get_probe_resp_mode != NULL) {
        return g_wlan_chip_ops->get_probe_resp_mode();
    }
    return 0;
}
OAL_STATIC OAL_INLINE uint32_t wlan_chip_get_trx_switch(void)
{
    if (g_wlan_chip_ops->get_trx_switch != NULL) {
        return g_wlan_chip_ops->get_trx_switch();
    }
    return DEVICE_RX;
}
OAL_STATIC OAL_INLINE uint8_t wlan_chip_get_d2h_access_ddr(void)
{
    if (g_wlan_chip_ops->get_d2h_access_ddr != NULL) {
        return g_wlan_chip_ops->get_d2h_access_ddr();
    }
    return 0;
}

OAL_STATIC OAL_INLINE uint8_t wlan_chip_get_chn_radio_cap(void)
{
    if (g_wlan_chip_ops->get_chn_radio_cap != NULL) {
        return g_wlan_chip_ops->get_chn_radio_cap();
    }
    return 0;
}

OAL_STATIC OAL_INLINE uint32_t wlan_chip_update_cfg80211_mgmt_tx_wait_time(uint32_t wait_time)
{
    if (g_wlan_chip_ops->update_cfg80211_mgmt_tx_wait_time != NULL) {
        return g_wlan_chip_ops->update_cfg80211_mgmt_tx_wait_time(wait_time);
    }
    return wait_time;
}

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 wlan_chip_check_need_setup_ba_session(void)
{
    if (g_wlan_chip_ops->check_need_setup_ba_session != NULL) {
        return g_wlan_chip_ops->check_need_setup_ba_session();
    }
    return OAL_FALSE;
}

OAL_STATIC OAL_INLINE uint8_t wlan_chip_tx_update_amsdu_num(mac_vap_stru *mac_vap,
    hmac_performance_stat_stru *performance_stat_params, oal_bool_enum_uint8 mu_vap_flag)
{
    if (g_wlan_chip_ops->tx_update_amsdu_num != NULL) {
        return g_wlan_chip_ops->tx_update_amsdu_num(mac_vap, performance_stat_params, mu_vap_flag);
    }
    return WLAN_TX_AMSDU_BUTT;
}

OAL_STATIC OAL_INLINE void wlan_chip_ba_rx_hdl_init(hmac_vap_stru *pst_hmac_vap,
    hmac_user_stru *pst_hmac_user, uint8_t uc_tid)
{
    if (g_wlan_chip_ops->ba_rx_hdl_init != NULL) {
        g_wlan_chip_ops->ba_rx_hdl_init(pst_hmac_vap, pst_hmac_user, uc_tid);
    }
}
#ifdef _PRE_WLAN_FEATURE_TCP_ACK_BUFFER
OAL_STATIC OAL_INLINE uint32_t wlan_chip_config_tcp_ack_buf(mac_vap_stru *mac_vap, uint16_t len, uint8_t *param)
{
    if (g_wlan_chip_ops->tcp_ack_buff_config != NULL) {
        return g_wlan_chip_ops->tcp_ack_buff_config(mac_vap, len, param);
    }
    return OAL_FAIL;
}
#endif
#ifdef _PRE_WLAN_FEATURE_CSI
OAL_STATIC OAL_INLINE uint32_t wlan_chip_device_csi_config(mac_vap_stru *mac_vap, uint16_t len, uint8_t *param)
{
    if (g_wlan_chip_ops->dcsi_config != NULL) {
        return g_wlan_chip_ops->dcsi_config(mac_vap, len, param);
    }
    return OAL_FAIL;
}
OAL_STATIC OAL_INLINE uint32_t wlan_chip_host_csi_config(mac_vap_stru *mac_vap, uint16_t len, uint8_t *param)
{
    if (g_wlan_chip_ops->hcsi_config != NULL) {
        return g_wlan_chip_ops->hcsi_config(mac_vap, len, param);
    }
    return OAL_FAIL;
}
#endif
#ifdef _PRE_WLAN_FEATURE_FTM
OAL_STATIC OAL_INLINE uint32_t wlan_chip_config_wifi_rtt_config(mac_vap_stru *mac_vap, uint16_t len, uint8_t *param)
{
    uint8_t val;
    val = wlan_chip_get_d2h_access_ddr();
    if (val) {
        if (g_wlan_chip_ops->hconfig_wifi_rtt_config != NULL) {
            return g_wlan_chip_ops->hconfig_wifi_rtt_config(mac_vap, len, param);
        }
        return OAL_FAIL;
    } else {
        if (g_wlan_chip_ops->dconfig_wifi_rtt_config != NULL) {
            return g_wlan_chip_ops->dconfig_wifi_rtt_config(mac_vap, len, param);
        }
        return OAL_FAIL;
    }
}

OAL_STATIC OAL_INLINE uint32_t wlan_chip_device_ftm_config(mac_vap_stru *mac_vap, uint16_t len, uint8_t *param)
{
    if (g_wlan_chip_ops->dftm_config != NULL) {
        return g_wlan_chip_ops->dftm_config(mac_vap, len, param);
    }
    return OAL_FAIL;
}
OAL_STATIC OAL_INLINE uint32_t wlan_chip_host_ftm_config(mac_vap_stru *mac_vap, uint16_t len, uint8_t *param)
{
    if (g_wlan_chip_ops->hftm_config != NULL) {
        return g_wlan_chip_ops->hftm_config(mac_vap, len, param);
    }
    return OAL_FAIL;
}

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 wlan_chip_rrm_proc_rm_request(hmac_vap_stru *hmac_vap_sta,
    oal_netbuf_stru *netbuf)
{
    if (g_wlan_chip_ops->rrm_proc_rm_request != NULL) {
        return g_wlan_chip_ops->rrm_proc_rm_request(hmac_vap_sta, netbuf);
    }
    return OAL_FALSE;
}

OAL_STATIC OAL_INLINE void wlan_chip_ftm_vap_init(hmac_vap_stru *hmac_vap)
{
    if (g_wlan_chip_ops->ftm_vap_init != NULL) {
        g_wlan_chip_ops->ftm_vap_init(hmac_vap);
    }
}
#endif
OAL_STATIC OAL_INLINE oal_bool_enum_uint8 wlan_chip_check_need_process_bar(void)
{
    if (g_wlan_chip_ops->check_need_process_bar != NULL) {
        return g_wlan_chip_ops->check_need_process_bar();
    }
    return OAL_FALSE;
}

OAL_STATIC OAL_INLINE uint32_t wlan_chip_ba_send_reorder_timeout(hmac_ba_rx_stru *rx_ba, hmac_vap_stru *hmac_vap,
    hmac_ba_alarm_stru *alarm_data, uint32_t *timeout)
{
    if (g_wlan_chip_ops->ba_send_reorder_timeout != NULL) {
        return g_wlan_chip_ops->ba_send_reorder_timeout(rx_ba, hmac_vap, alarm_data, timeout);
    }
    return OAL_FAIL;
}

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 wlan_chip_ba_need_check_lut_idx(void)
{
    if (g_wlan_chip_ops->ba_need_check_lut_idx != NULL) {
        return g_wlan_chip_ops->ba_need_check_lut_idx();
    }
    return OAL_TRUE;
}

OAL_STATIC OAL_INLINE uint32_t wlan_chip_send_cali_matrix_data(mac_vap_stru *mac_vap)
{
    if (g_wlan_chip_ops->send_cali_matrix_data != NULL) {
        return g_wlan_chip_ops->send_cali_matrix_data(mac_vap);
    }
    return OAL_FAIL;
}

OAL_STATIC OAL_INLINE void wlan_chip_send_cali_data(oal_net_device_stru *cfg_net_dev)
{
    if (g_wlan_chip_ops->send_cali_data != NULL) {
        return g_wlan_chip_ops->send_cali_data(cfg_net_dev);
    }
}

OAL_STATIC OAL_INLINE void wlan_chip_send_20m_all_chn_cali_data(oal_net_device_stru *cfg_net_dev)
{
    if (g_wlan_chip_ops->send_20m_all_chn_cali_data != NULL) {
        return g_wlan_chip_ops->send_20m_all_chn_cali_data(cfg_net_dev);
    }
}
OAL_STATIC OAL_INLINE void wlan_chip_send_dbdc_scan_cali_data(mac_vap_stru *mac_vap,
    oal_bool_enum_uint8 dbdc_running)
{
    if (g_wlan_chip_ops->send_dbdc_scan_all_chn_cali_data != NULL) {
        return g_wlan_chip_ops->send_dbdc_scan_all_chn_cali_data(mac_vap, dbdc_running);
    }
}
OAL_STATIC OAL_INLINE uint32_t wlan_chip_save_cali_event(frw_event_mem_stru *event_mem)
{
    if (g_wlan_chip_ops->save_cali_event != NULL) {
        return g_wlan_chip_ops->save_cali_event(event_mem);
    }
    return OAL_FAIL;
}
OAL_STATIC OAL_INLINE void wlan_chip_update_cur_chn_cali_data(cali_data_req_stru *cali_data_get)
{
    if (g_wlan_chip_ops->update_cur_chn_cali_data != NULL) {
        g_wlan_chip_ops->update_cur_chn_cali_data(cali_data_get);
    }
}

OAL_STATIC OAL_INLINE void wlan_chip_mac_vap_init_mib_11ax(mac_vap_stru *mac_vap, uint32_t nss_num)
{
    if (g_wlan_chip_ops->mac_vap_init_mib_11ax != NULL) {
        g_wlan_chip_ops->mac_vap_init_mib_11ax(mac_vap, nss_num);
    }
}

OAL_STATIC OAL_INLINE void wlan_chip_mac_mib_set_auth_rsp_time_out(mac_vap_stru *mac_vap)
{
    if (g_wlan_chip_ops->mac_mib_set_auth_rsp_time_out != NULL) {
        g_wlan_chip_ops->mac_mib_set_auth_rsp_time_out(mac_vap);
    }
}

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 wlan_chip_mac_vap_need_set_user_htc_cap(mac_vap_stru *mac_vap)
{
    if (g_wlan_chip_ops->mac_vap_need_set_user_htc_cap != NULL) {
        return g_wlan_chip_ops->mac_vap_need_set_user_htc_cap(mac_vap);
    }
    return OAL_FALSE;
}

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 wlan_chip_mac_get_6g_flag(dmac_rx_ctl_stru *rx_ctrl)
{
    if (g_wlan_chip_ops->get_6g_flag != NULL) {
        return g_wlan_chip_ops->get_6g_flag(rx_ctrl);
    }
    return OAL_FALSE;
}

OAL_STATIC OAL_INLINE void wlan_chip_tx_encap_large_skb_amsdu(hmac_vap_stru *hmac_vap,
    hmac_user_stru *user, oal_netbuf_stru *buf, mac_tx_ctl_stru *tx_ctl)
{
    if (g_wlan_chip_ops->tx_encap_large_skb_amsdu != NULL) {
        g_wlan_chip_ops->tx_encap_large_skb_amsdu(hmac_vap, user, buf, tx_ctl);
    }
}

OAL_STATIC OAL_INLINE uint32_t wlan_chip_check_headroom_len(mac_tx_ctl_stru *tx_ctl,
    uint8_t nest_type, uint8_t nest_sub_type, uint8_t cb_len)
{
    if (g_wlan_chip_ops->check_headroom_len != NULL) {
        return g_wlan_chip_ops->check_headroom_len(tx_ctl, nest_type, nest_sub_type, cb_len);
    }
    return 0;
}

OAL_STATIC OAL_INLINE void wlan_chip_adjust_netbuf_data(oal_netbuf_stru *netbuf, mac_tx_ctl_stru *tx_ctrl,
    uint8_t nest_type, uint8_t nest_sub_type)
{
    if (g_wlan_chip_ops->adjust_netbuf_data != NULL) {
        g_wlan_chip_ops->adjust_netbuf_data(netbuf, tx_ctrl, nest_type, nest_sub_type);
    }
}
OAL_STATIC OAL_INLINE oal_bool_enum_uint8 wlan_chip_wmmac_need_degrade_for_ts(uint8_t tid,
    uint8_t need_degrade, uint8_t wmmac_auth_flag)
{
    if (g_wlan_chip_ops->wmmac_need_degrade_for_ts != NULL) {
        return g_wlan_chip_ops->wmmac_need_degrade_for_ts(tid, need_degrade, wmmac_auth_flag);
    }
    return OAL_FALSE;
}
OAL_STATIC OAL_INLINE void wlan_chip_update_arp_tid(uint8_t *tid)
{
    if (g_wlan_chip_ops->update_arp_tid != NULL) {
        g_wlan_chip_ops->update_arp_tid(tid);
    }
}
OAL_STATIC OAL_INLINE void wlan_chip_proc_query_station_packets(hmac_vap_stru *hmac_vap,
    dmac_query_station_info_response_event *response_event)
{
    if (g_wlan_chip_ops->proc_query_station_packets != NULL) {
        g_wlan_chip_ops->proc_query_station_packets(hmac_vap, response_event);
    }
}
OAL_STATIC OAL_INLINE uint32_t wlan_chip_scan_req_alloc_and_fill_netbuf(frw_event_mem_stru *event_mem,
    hmac_vap_stru *hmac_vap, oal_netbuf_stru **netbuf_scan_req, void *params)
{
    if (g_wlan_chip_ops->scan_req_alloc_and_fill_netbuf != NULL) {
        return g_wlan_chip_ops->scan_req_alloc_and_fill_netbuf(event_mem, hmac_vap, netbuf_scan_req, params);
    }
    return OAL_FAIL;
}
#ifdef _PRE_WLAN_FEATURE_SNIFFER
OAL_STATIC OAL_INLINE uint32_t wlan_chip_set_sniffer_config(mac_vap_stru *mac_vap, uint16_t len, uint8_t *param)
{
    if (g_wlan_chip_ops->set_sniffer_config != NULL) {
        return g_wlan_chip_ops->set_sniffer_config(mac_vap, len, param);
    }
    return OAL_FAIL;
}
#endif

OAL_STATIC OAL_INLINE void wlan_chip_start_zero_wait_dfs(mac_vap_stru *mac_vap,
    mac_cfg_channel_param_stru *channel_info)
{
    if (g_wlan_chip_ops->start_zero_wait_dfs != NULL) {
        return g_wlan_chip_ops->start_zero_wait_dfs(mac_vap, channel_info);
    }
}

OAL_STATIC OAL_INLINE void wlan_chip_update_rxctl_data_type(oal_netbuf_stru *netbuf, mac_rx_ctl_stru *rx_ctl)
{
    if (g_wlan_chip_ops->update_rxctl_data_type != NULL) {
        return g_wlan_chip_ops->update_rxctl_data_type(netbuf, rx_ctl);
    }
}
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
OAL_STATIC OAL_INLINE void wlan_chip_mcast_ampdu_rx_ba_init(hmac_user_stru *hmac_user, uint8_t tidno)
{
    if (g_wlan_chip_ops->mcast_ampdu_rx_ba_init != NULL) {
        return g_wlan_chip_ops->mcast_ampdu_rx_ba_init(hmac_user, tidno);
    }
}

OAL_STATIC OAL_INLINE void wlan_chip_mcast_stats_stru_init(hmac_vap_stru *hmac_vap)
{
    if (g_wlan_chip_ops->mcast_stats_stru_init != NULL) {
        return g_wlan_chip_ops->mcast_stats_stru_init(hmac_vap);
    }
}
#endif
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU_HW
OAL_STATIC OAL_INLINE void wlan_chip_mcast_ampdu_sta_add_multi_user(mac_vap_stru *mac_vap, uint8_t *mac_addr)
{
    if (g_wlan_chip_ops->mcast_ampdu_sta_add_multi_user != NULL) {
        return g_wlan_chip_ops->mcast_ampdu_sta_add_multi_user(mac_vap, mac_addr);
    }
}
#endif
OAL_STATIC OAL_INLINE oal_bool_enum wlan_chip_is_dbdc_ini_en(void)
{
    if (g_wlan_chip_ops->is_dbdc_ini_en != NULL) {
        return g_wlan_chip_ops->is_dbdc_ini_en();
    }
    return OAL_FALSE;
}

#if defined(CONFIG_ARCH_HISI) &&  defined(CONFIG_NR_CPUS)
#if CONFIG_NR_CPUS > OAL_BUS_HPCPU_NUM
OAL_STATIC OAL_INLINE void wlan_chip_frw_task_bind_cpu(struct cpumask *cpus)
{
    if (g_wlan_chip_ops->frw_task_bind_cpu != NULL) {
        g_wlan_chip_ops->frw_task_bind_cpu(cpus);
    }
}
#endif
#endif

OAL_STATIC OAL_INLINE uint32_t wlan_chip_send_connect_security_params(mac_vap_stru *mac_vap,
    mac_conn_security_stru *connect_security_params)
{
    if (g_wlan_chip_ops->send_connect_security_params != NULL) {
        return g_wlan_chip_ops->send_connect_security_params(mac_vap, connect_security_params);
    }
    return OAL_FAIL;
}

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 wlan_chip_defrag_is_pn_abnormal(hmac_user_stru *hmac_user,
    oal_netbuf_stru *netbuf, oal_netbuf_stru *last_netbuf)
{
    if (g_wlan_chip_ops->defrag_is_pn_abnormal != NULL) {
        return g_wlan_chip_ops->defrag_is_pn_abnormal(hmac_user, netbuf, last_netbuf);
    }
    return OAL_FALSE;
}

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 wlan_chip_is_support_hw_wapi(void)
{
    if (g_wlan_chip_ops->is_support_hw_wapi != NULL) {
        return g_wlan_chip_ops->is_support_hw_wapi();
    }
    return OAL_FALSE;
}

OAL_STATIC OAL_INLINE mac_scanned_result_extend_info_stru *wlan_chip_get_scaned_result_extend_info(
    oal_netbuf_stru *netbuf, uint16_t frame_len)
{
    if (g_wlan_chip_ops->get_scaned_result_extend_info != NULL) {
        return g_wlan_chip_ops->get_scaned_result_extend_info(netbuf, frame_len);
    }
    return NULL;
}

OAL_STATIC OAL_INLINE uint16_t wlan_chip_get_scaned_payload_extend_len(void)
{
    if (g_wlan_chip_ops->get_scaned_payload_extend_len != NULL) {
        return g_wlan_chip_ops->get_scaned_payload_extend_len();
    }
    return 0;
}

OAL_STATIC OAL_INLINE void wlan_chip_tx_pt_mcast_set_cb(hmac_vap_stru *pst_vap,
    mac_ether_header_stru *ether_hdr, mac_tx_ctl_stru *tx_ctl)
{
    if (g_pt_mcast_enable && g_wlan_chip_ops->tx_pt_mcast_set_cb != NULL) {
        g_wlan_chip_ops->tx_pt_mcast_set_cb(pst_vap, ether_hdr, tx_ctl);
    }
}

#ifdef _PRE_WLAN_FEATURE_11AX
OAL_STATIC OAL_INLINE void wlan_chip_tx_set_frame_htc(hmac_vap_stru *hmac_vap,
    mac_tx_ctl_stru *tx_ctl, hmac_user_stru *hmac_user, mac_ieee80211_qos_htc_frame_addr4_stru *hdr_addr4)
{
    if (g_wlan_chip_ops->tx_set_frame_htc != NULL) {
        g_wlan_chip_ops->tx_set_frame_htc(hmac_vap, tx_ctl, hmac_user, hdr_addr4);
    }
}
#endif

#endif /* end of wlan_chip_i.h */
