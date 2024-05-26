

#ifndef __WLAN_CHIP_H__
#define __WLAN_CHIP_H__

#include "wlan_types.h"
#include "wlan_cali.h"
#include "mac_vap.h"
#include "hmac_vap.h"
#include "hmac_user.h"

/* hostʹ�õ�������߹�� */
#define HAL_HOST_MAX_RF_NUM          4
#define HAL_HOST_MAX_SLAVE_RF_NUM    2

/* VHTЭ���б�ʾ������NDP Nsts���� */
#define VHT_BFEE_NTX_SUPP_STS_CAP 4

#ifdef _PRE_WLAN_FEATURE_160M
#define WLAN_HAL0_BW_MAX_WIDTH WLAN_BW_CAP_160M
#else
#define WLAN_HAL0_BW_MAX_WIDTH WLAN_BW_CAP_80M
#endif

struct wlan_flow_ctrl_params {
    uint16_t start;
    uint16_t stop;
};

typedef struct {
    uint8_t auc_sar_ctrl_params_c0;
    uint8_t auc_sar_ctrl_params_c1;
} wlan_cust_sar_ctrl_stru;

struct wlan_chip_ops {
    // ���ƻ����
    /* ����hostȫ�ֱ���ֵ */
    void (*host_global_init_param)(void);

    /* �����״δ��ϵ��־λˢ�� */
    oal_bool_enum_uint8 (*first_power_on_mark)(void);
    /* �����״��ϵ�У׼��� */
    void (*first_powon_cali_completed)(void);

    /* ��ȡӲ���Ƿ�֧��160M APUT */
    oal_bool_enum_uint8 (*is_aput_support_160M)(void);

    /* ��ȡ���ƻ����ز��� */
    void (*get_flow_ctrl_used_mem)(struct wlan_flow_ctrl_params *flow_ctrl);

    /* hmac�����·���dmac�����ж� */
    /* ����ֵ��TRUE:���˸�������·�dmac��FALSE:�����·�dmac */
    oal_bool_enum_uint8 (*h2d_cmd_need_filter)(uint32_t cmd_id);

    /* ��ȡini�ļ������¶��ƻ���Ա��ֵ */
    uint32_t (*force_update_custom_params)(void);

    /* �·�host���ƻ�nv������device */
    uint32_t (*init_nvram_main)(oal_net_device_stru *cfg_net_dev);

    /* ���ݶ��ƻ�����CPU��Ƶ���� */
    void (*cpu_freq_ini_param_init)(void);

    /* ��ȡȫ�����ƻ����� */
    void (*host_global_ini_param_init)(void);

    /* ��ȡ���ƻ���������ѧϰ��־ */
    uint8_t (*get_selfstudy_country_flag)(void);

    /* host���ϵ�У׼�¼���У׼�����·��ӿ� */
    uint32_t (*custom_cali)(void);
    /* host��У׼�����ڴ�����ӿ� */
    void (*custom_cali_data_host_addr_init)(void);

    /* �״ζ�ȡ���ƻ������ļ������ */
    uint32_t (*custom_host_read_cfg_init)(void);

    /* Э��ջ��ʼ��ǰ���ƻ�������� */
    uint32_t (*hcc_customize_h2d_data_cfg)(void);

    /* host��ӡ���ƻ���Ϣ */
    void (*show_customize_info)(void);

    /* ��ȡ��sar���� */
    uint32_t (*get_sar_ctrl_params)(uint8_t lvl_num, uint8_t *data_addr, uint16_t *data_len, uint16_t dest_len);

    /* ��ȡ11ax�������ò��� */
    uint32_t (*get_11ax_switch_mask)(void);

    /* ��ȡ2G 11AC �����Ƿ�ʹ�ܿ��� */
    /* ����ֵ��TRUE:2G  11AC����ʹ�ܣ�FALSE:2G 11AC���ܹر� */
    oal_bool_enum_uint8 (*get_11ac2g_enable)(void);

    /* ��ȡ���ƻ��ļ�probe_respģʽ��ʶ */
    uint32_t (*get_probe_resp_mode)(void);
    /* ��ȡ���ƻ��ļ�trx switch��ʶ */
    uint32_t (*get_trx_switch)(void);
    /* ��ȡ���ƻ��ļ�trx switch��ʶ */
    uint8_t (*get_d2h_access_ddr)(void);
    /* ���µ���wal_cfg80211_mgmt_tx ����ʱ�� */
    uint32_t (*update_cfg80211_mgmt_tx_wait_time)(uint32_t wait_time);
    /* ���¹���������Ϣ��device */
    uint32_t (*send_connect_security_params)(mac_vap_stru *mac_vap, mac_conn_security_stru *connect_security_params);

    // �շ��;ۺ����
    /* �ж��Ƿ���Ҫ����BA�Ự */
    /* ����ֵ��TRUE:������BA�Ự��FALSE:��������BA �Ự */
    oal_bool_enum_uint8 (*check_need_setup_ba_session)(void);

    /* ���ݵ�ǰ���¾���amsdu�ۺϸ��� */
    uint8_t (*tx_update_amsdu_num)(mac_vap_stru *pst_mac_vap, hmac_performance_stat_stru *performance_stat_params,
        oal_bool_enum_uint8 en_mu_vap_flag);

    /* ��ʼ��ba��ز��� */
    void (*ba_rx_hdl_init)(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user, uint8_t uc_tid);

    /* �������Ƿ���Ҫ����BAR֡ */
    /* ����ֵ��TRUE:��Ҫ����BAR֡��FALSE:����Ҫ����BAR֡ */
    oal_bool_enum_uint8 (*check_need_process_bar)(void);

    /* �ϱ�����������г�ʱ�ı��� */
    uint32_t (*ba_send_reorder_timeout)(hmac_ba_rx_stru *rx_ba, hmac_vap_stru *hmac_vap,
        hmac_ba_alarm_stru *alarm_data, uint32_t *pus_timeout);

    /* ������յ�ADDBA REQ,�Ƿ���BA LUT index */
    /* ����ֵ��TRUE:��Ҫ���BA LUT index;FALSE:����Ҫ���BA LUT index */
    oal_bool_enum_uint8 (*ba_need_check_lut_idx)(void);

#ifdef _PRE_WLAN_FEATURE_TCP_ACK_BUFFER
    /* tcp ack�������06ֱ�Ӹ�ֵ��0305�������ݵ�device */
    /* ����ֵ��OAL_SUCC:���γɹ�;OAL_FAIL:����ʧ�� */
    uint32_t (*tcp_ack_buff_config)(mac_vap_stru *mac_vap, uint16_t len, uint8_t *param);
#endif

#ifdef _PRE_WLAN_FEATURE_CSI
    /* set_csi�������06ֱ��host���ã�0305�������ݵ�device */
    /* ����ֵ��OAL_SUCC:���γɹ�;OAL_FAIL:����ʧ�� */
    uint32_t (*dcsi_config)(mac_vap_stru *mac_vap, uint16_t len, uint8_t *param);
    /* set_csi�������06ֱ��host���ã�0305�������ݵ�device */
    /* ����ֵ��OAL_SUCC:���γɹ�;OAL_FAIL:����ʧ�� */
    uint32_t (*hcsi_config)(mac_vap_stru *mac_vap, uint16_t len, uint8_t *param);
#endif
#ifdef _PRE_WLAN_FEATURE_FTM
    uint32_t (*dftm_config)(mac_vap_stru *mac_vap, uint16_t len, uint8_t *param);
    uint32_t (*hftm_config)(mac_vap_stru *mac_vap, uint16_t len, uint8_t *param);
    oal_bool_enum_uint8 (*rrm_proc_rm_request)(hmac_vap_stru *hmac_vap_sta, oal_netbuf_stru *netbuf);
    uint32_t (*dconfig_wifi_rtt_config)(mac_vap_stru *mac_vap, uint16_t len, uint8_t *param);
    uint32_t (*hconfig_wifi_rtt_config)(mac_vap_stru *mac_vap, uint16_t len, uint8_t *param);
    void (*ftm_vap_init)(hmac_vap_stru *hmac_vap);
#endif

#ifdef _PRE_WLAN_FEATURE_WMMAC
    oal_bool_enum_uint8 (*wmmac_need_degrade_for_ts)(uint8_t tid, uint8_t need_degrade, uint8_t wmmac_auth_flag);
#endif
    void (*update_arp_tid)(uint8_t *tid);
    oal_bool_enum_uint8 (*get_6g_flag)(dmac_rx_ctl_stru *rx_ctrl);
    // У׼���
    /* �·�У׼�õ��ľ�����Ϣ */
    uint32_t (*send_cali_matrix_data)(mac_vap_stru *mac_vap);

    /* У׼�����·� */
    void (*send_cali_data)(oal_net_device_stru *cfg_net_dev);
    /* ɨ��ȫ�ŵ�20MУ׼�����·� */
    void (*send_20m_all_chn_cali_data)(oal_net_device_stru *cfg_net_dev);
    /* DBDCģʽ�л�ʱɨ��ȫ�ŵ�20MУ׼�����·� */
    void (*send_dbdc_scan_all_chn_cali_data)(mac_vap_stru *mac_vap, oal_bool_enum_uint8 dbdc_running);
    /* ����device�ϱ���У׼���� */
    uint32_t (*save_cali_event)(frw_event_mem_stru *event_mem);
    /* ���µ�ǰ�ŵ�У׼���� */
    void (*update_cur_chn_cali_data)(cali_data_req_stru *get_cali_data_param);

    /* ��ȡ���ƻ��ļ�chann_radio cap */
    uint8_t (*get_chn_radio_cap)(void);
    /* 11ax mibֵ��ʼ�� */
    void (*mac_vap_init_mib_11ax)(mac_vap_stru *mac_vap, uint32_t nss_num);

    /* ����auth��ʱʱ�� */
    void (*mac_mib_set_auth_rsp_time_out)(mac_vap_stru *mac_vap);

    /* ��ǰvapģʽ�Ƿ�����user htc cap */
    oal_bool_enum_uint8 (*mac_vap_need_set_user_htc_cap)(mac_vap_stru *mac_vap);

    /* ���AMPDU+AMSDU���03 05��Ч */
    void (*tx_encap_large_skb_amsdu)(hmac_vap_stru *hmac_vap, hmac_user_stru *user,
        oal_netbuf_stru *buf, mac_tx_ctl_stru *tx_ctl);
    /* hcc headroom���ȼ�� */
    uint32_t (*check_headroom_len)(mac_tx_ctl_stru *pst_tx_ctl,
        uint8_t en_nest_type, uint8_t uc_nest_sub_type, uint8_t uc_cb_length);
    /* ����hccͷ��ʽ */
    void (*adjust_netbuf_data)(oal_netbuf_stru *netbuf, mac_tx_ctl_stru *tx_ctrl,
        uint8_t en_nest_type, uint8_t uc_nest_sub_type);
    /* ����get_station��ѯ�ӿ����ݰ�ͳ����Ϣ */
    void (*proc_query_station_packets)(hmac_vap_stru *hmac_vap, dmac_query_station_info_response_event *reponse_event);
    /* ���벢���ɨ���·���netbuf */
    uint32_t (*scan_req_alloc_and_fill_netbuf)(frw_event_mem_stru *event_mem, hmac_vap_stru *hmac_vap,
                                               oal_netbuf_stru **netbuf_scan_req, void *params);
    uint32_t (*set_sniffer_config)(mac_vap_stru *mac_vap, uint16_t len, uint8_t *param);

#ifdef _PRE_WLAN_FEATURE_DFS
    /* zero wait dfs���̣�Ŀǰ��1106֧�֣�1103/05��֧�� */
    void (*start_zero_wait_dfs)(mac_vap_stru *mac_vap, mac_cfg_channel_param_stru *channel_info);
#endif
    void (*update_rxctl_data_type)(oal_netbuf_stru *netbuf, mac_rx_ctl_stru *rx_ctl);

    /* �����ܵ�PN���Ƿ��쳣��OAL_TRUE:PN�쳣��OAL_FALSE:PN���� */
    oal_bool_enum_uint8 (*defrag_is_pn_abnormal)(hmac_user_stru *hmac_user,
        oal_netbuf_stru *netbuf, oal_netbuf_stru *last_netbuf);

    /*
     * �Ƿ�֧��WAPIӲ���ӽ��ܡ�
     * ����ֵ��OAL_TRUE:֧��Ӳ���ӽ���WAPI; OAL_FALSE:��֧��Ӳ���ӽ���
     */
    oal_bool_enum_uint8 (*is_support_hw_wapi)(void);

#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
    void (*mcast_ampdu_rx_ba_init)(hmac_user_stru *hmac_user, uint8_t tidno);
    void (*mcast_stats_stru_init)(hmac_vap_stru *hmac_vap);
#endif
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU_HW
    void (*mcast_ampdu_sta_add_multi_user)(mac_vap_stru *mac_vap, uint8_t *mac_addr);
#endif
    oal_bool_enum (*is_dbdc_ini_en)(void);
#if defined(CONFIG_ARCH_HISI) &&  defined(CONFIG_NR_CPUS)
#if CONFIG_NR_CPUS > OAL_BUS_HPCPU_NUM
    void (*frw_task_bind_cpu)(struct cpumask *fast_cpus);
#endif
#endif
    /* ��ȡbeacon/probe rsp֡����չ�ֶβ��� */
    mac_scanned_result_extend_info_stru *(*get_scaned_result_extend_info)(oal_netbuf_stru *netbuf, uint16_t frame_len);
    uint16_t (*get_scaned_payload_extend_len)(void);
    void (*tx_pt_mcast_set_cb)(hmac_vap_stru *pst_vap, mac_ether_header_stru *ether_hdr, mac_tx_ctl_stru *tx_ctl);
#ifdef _PRE_WLAN_FEATURE_11AX
    void (*tx_set_frame_htc)(hmac_vap_stru *pst_hmac_vap,
    mac_tx_ctl_stru *pst_tx_ctl, hmac_user_stru *pst_hmac_user, mac_ieee80211_qos_htc_frame_addr4_stru *pst_hdr_addr4);
#endif
};

extern const struct wlan_chip_ops *g_wlan_chip_ops;
extern const struct wlan_chip_ops g_wlan_chip_dummy_ops;
extern const struct wlan_chip_ops g_wlan_chip_ops_1103;
extern const struct wlan_chip_ops g_wlan_chip_ops_1105;
extern const struct wlan_chip_ops g_wlan_chip_ops_shenkuo;
extern const struct wlan_chip_ops g_wlan_chip_ops_bisheng;
extern const struct wlan_chip_ops g_wlan_chip_ops_1161;

void wlan_chip_ops_register(struct wlan_chip_ops *ops);
oal_bool_enum_uint8 wlan_chip_is_support_hw_wapi_bisheng(void);
void hwifi_cfg_host_global_init_sounding_shenkuo(void);
void hwifi_cfg_host_global_init_param_extend_shenkuo(void);
void hwifi_cfg_host_global_init_mcm_shenkuo(void);
void hwifi_cfg_host_global_init_param_shenkuo(void);
oal_bool_enum_uint8 wlan_first_powon_mark_shenkuo(void);
oal_bool_enum_uint8 wlan_chip_is_aput_support_160m_shenkuo(void);
void wlan_chip_get_flow_ctrl_used_mem_shenkuo(struct wlan_flow_ctrl_params *flow_ctrl);
uint32_t hwifi_force_update_rf_params_shenkuo(void);
uint8_t wlan_chip_get_selfstudy_country_flag_shenkuo(void);
uint32_t wlan_chip_get_11ax_switch_mask_shenkuo(void);
oal_bool_enum_uint8 wlan_chip_get_11ac2g_enable_shenkuo(void);
uint32_t wlan_chip_get_probe_resp_mode_shenkuo(void);
uint32_t wlan_chip_get_trx_switch_shenkuo(void);
uint8_t wlan_chip_get_d2h_access_ddr_shenkuo(void);
uint8_t wlan_chip_get_chn_radio_cap_shenkuo(void);
oal_bool_enum_uint8 wlan_chip_h2d_cmd_need_filter_shenkuo(uint32_t cmd_id);
uint32_t wlan_chip_update_cfg80211_mgmt_tx_wait_time_shenkuo(uint32_t wait_time);
oal_bool_enum_uint8 wlan_chip_check_need_setup_ba_session_shenkuo(void);
oal_bool_enum_uint8 wlan_chip_check_need_process_bar_shenkuo(void);
oal_bool_enum_uint8 wlan_chip_ba_need_check_lut_idx_shenkuo(void);
void wlan_chip_mac_mib_set_auth_rsp_time_out_shenkuo(mac_vap_stru *mac_vap);
void wlan_chip_proc_query_station_packets_shenkuo(hmac_vap_stru *hmac_vap,
    dmac_query_station_info_response_event *response_event);
uint32_t wlan_chip_scan_req_alloc_and_fill_netbuf_shenkuo(frw_event_mem_stru *event_mem, hmac_vap_stru *hmac_vap,
    oal_netbuf_stru **netbuf_scan_req, void *params);
oal_bool_enum wlan_chip_is_dbdc_ini_en_shenkuo(void);
uint32_t wlan_chip_send_connect_security_params_shenkuo(mac_vap_stru *mac_vap,
    mac_conn_security_stru *connect_security_params);
oal_bool_enum_uint8 wlan_chip_defrag_is_pn_abnormal_shenkuo(hmac_user_stru *hmac_user,
    oal_netbuf_stru *netbuf, oal_netbuf_stru *last_netbuf);
oal_bool_enum_uint8 wlan_chip_is_support_hw_wapi_shenkuo(void);
mac_scanned_result_extend_info_stru *wlan_chip_get_scaned_result_extend_info_shenkuo(oal_netbuf_stru *netbuf,
    uint16_t frame_len);
uint16_t wlan_chip_get_scaned_payload_extend_len_shenkuo(void);

OAL_STATIC OAL_INLINE void wlan_chip_host_global_init_param(void)
{
    if (g_wlan_chip_ops->host_global_init_param != NULL) {
        g_wlan_chip_ops->host_global_init_param();
    }
}

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 wlan_chip_first_power_on_mark(void)
{
    if (g_wlan_chip_ops->first_power_on_mark != NULL) {
        return g_wlan_chip_ops->first_power_on_mark();
    }
    return OAL_TRUE;
}
OAL_STATIC OAL_INLINE void wlan_chip_first_powon_cali_completed(void)
{
    if (g_wlan_chip_ops->first_powon_cali_completed != NULL) {
        g_wlan_chip_ops->first_powon_cali_completed();
    }
}

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 wlan_chip_is_aput_support_160m(void)
{
    if (g_wlan_chip_ops->is_aput_support_160M != NULL) {
        return g_wlan_chip_ops->is_aput_support_160M();
    }
    return OAL_FALSE;
}

OAL_STATIC OAL_INLINE void wlan_chip_get_flow_ctrl_used_mem(struct wlan_flow_ctrl_params *flow_ctrl)
{
    if (g_wlan_chip_ops->get_flow_ctrl_used_mem != NULL) {
        g_wlan_chip_ops->get_flow_ctrl_used_mem(flow_ctrl);
    }
}

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 wlan_chip_h2d_cmd_need_filter(uint32_t cmd_id)
{
    if (g_wlan_chip_ops->h2d_cmd_need_filter != NULL) {
        return g_wlan_chip_ops->h2d_cmd_need_filter(cmd_id);
    }
    return OAL_TRUE;
}

OAL_STATIC OAL_INLINE uint32_t wlan_chip_force_update_custom_params(void)
{
    if (g_wlan_chip_ops->force_update_custom_params != NULL) {
        return g_wlan_chip_ops->force_update_custom_params();
    }
    return OAL_FAIL;
}

OAL_STATIC OAL_INLINE uint32_t wlan_chip_init_nvram_main(oal_net_device_stru *cfg_net_dev)
{
    if (g_wlan_chip_ops->init_nvram_main != NULL) {
        return g_wlan_chip_ops->init_nvram_main(cfg_net_dev);
    }
    return OAL_FAIL;
}

OAL_STATIC OAL_INLINE void wlan_chip_cpu_freq_ini_param_init(void)
{
    if (g_wlan_chip_ops->cpu_freq_ini_param_init != NULL) {
        g_wlan_chip_ops->cpu_freq_ini_param_init();
    }
}

OAL_STATIC OAL_INLINE void wlan_chip_host_global_ini_param_init(void)
{
    if (g_wlan_chip_ops->host_global_ini_param_init != NULL) {
        g_wlan_chip_ops->host_global_ini_param_init();
    }
}

OAL_STATIC OAL_INLINE uint8_t wlan_chip_get_selfstudy_country_flag(void)
{
    if (g_wlan_chip_ops->get_selfstudy_country_flag != NULL) {
        return g_wlan_chip_ops->get_selfstudy_country_flag();
    }
    return 0;
}
#endif /* end of wlan_chip.h */
