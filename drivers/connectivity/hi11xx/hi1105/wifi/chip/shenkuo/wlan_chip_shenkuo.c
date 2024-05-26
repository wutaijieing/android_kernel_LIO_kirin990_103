

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

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
void hwifi_cfg_host_global_switch_init_shenkuo(void)
{
#ifndef _PRE_LINUX_TEST
    /*************************** 低功耗定制化 *****************************/
    /* 由于device 低功耗开关不是true false,而host是,先取定制化的值赋给device开关,再根据此值给host低功耗开关赋值 */
    g_wlan_device_pm_switch = g_cust_cap.wlan_device_pm_switch;
    g_wlan_ps_mode = g_cust_host.wlan_ps_mode;
    g_wlan_fast_ps_dyn_ctl = ((MAX_FAST_PS == g_wlan_ps_mode) ? 1 : 0);
    g_wlan_min_fast_ps_idle = g_cust_cap.fast_ps.wlan_min_fast_ps_idle;
    g_wlan_max_fast_ps_idle = g_cust_cap.fast_ps.wlan_max_fast_ps_idle;
    g_wlan_auto_ps_screen_on = g_cust_cap.fast_ps.wlan_auto_ps_thresh_screen_on;
    g_wlan_auto_ps_screen_off = g_cust_cap.fast_ps.wlan_auto_ps_thresh_screen_off;
    g_wlan_host_pm_switch = (g_wlan_device_pm_switch == WLAN_DEV_ALL_ENABLE ||
        g_wlan_device_pm_switch == WLAN_DEV_LIGHT_SLEEP_SWITCH_EN) ? OAL_TRUE : OAL_FALSE;

    g_feature_switch[HMAC_MIRACAST_SINK_SWITCH] =
        (g_cust_host.en_hmac_feature_switch[CUST_MIRACAST_SINK_SWITCH] == 1);
    g_feature_switch[HMAC_MIRACAST_REDUCE_LOG_SWITCH] =
        (g_cust_host.en_hmac_feature_switch[CUST_MIRACAST_REDUCE_LOG_SWITCH] == 1);
    g_feature_switch[HMAC_CORE_BIND_SWITCH] =
        (g_cust_host.en_hmac_feature_switch[CUST_CORE_BIND_SWITCH] == 1);
#endif
}

void hwifi_cfg_host_global_init_sounding_shenkuo(void)
{
    g_pst_mac_device_capability[0].en_rx_stbc_is_supp = g_cust_cap.en_rx_stbc_is_supp;
    g_pst_mac_device_capability[0].en_tx_stbc_is_supp = g_cust_cap.en_tx_stbc_is_supp;
    g_pst_mac_device_capability[0].en_su_bfmer_is_supp = g_cust_cap.en_su_bfmer_is_supp;
    g_pst_mac_device_capability[0].en_su_bfmee_is_supp = g_cust_cap.en_su_bfmee_is_supp;
    g_pst_mac_device_capability[0].en_mu_bfmer_is_supp = g_cust_cap.en_mu_bfmer_is_supp;
    g_pst_mac_device_capability[0].en_mu_bfmee_is_supp = g_cust_cap.en_mu_bfmee_is_supp;
}

void hwifi_cfg_host_global_init_param_extend_shenkuo(void)
{
#ifdef _PRE_FEATURE_PLAT_LOCK_CPUFREQ
    if (hwifi_get_cust_read_status(CUS_TAG_HOST, WLAN_CFG_HOST_LOCK_CPU_FREQ)) {
        g_freq_lock_control.uc_lock_max_cpu_freq = g_cust_host.lock_max_cpu_freq;
    }
#endif

#ifdef _PRE_WLAN_FEATURE_HIEX
    if (hwifi_get_cust_read_status(CUS_TAG_CAP, WLAN_CFG_CAP_HIEX_CAP)) {
        if (memcpy_s(&g_st_default_hiex_cap, sizeof(g_st_default_hiex_cap), &g_cust_cap.hiex_cap,
            sizeof(g_cust_cap.hiex_cap)) != EOK) {
            oal_io_print("hwifi_cfg_host_global_init_param_extend:hiex cap memcpy_s fail!");
        }
    }

    oam_warning_log1(0, 0, "host_global_init::hiex cap[0x%X]!", *(uint32_t *)&g_st_default_hiex_cap);
#endif
    if (hwifi_get_cust_read_status(CUS_TAG_HOST, WLAN_CFG_HOST_OPTIMIZED_FEATURE_SWITCH)) {
        g_optimized_feature_switch_bitmap = g_cust_cap.optimized_feature_mask;
    }

    if (hwifi_get_cust_read_status(CUS_TAG_CAP, WLAN_CFG_CAP_TRX_SWITCH)) {
        hmac_set_trx_switch(g_cust_cap.trx_switch);
    }
    oam_warning_log1(0, 0, "host_global_init::trx 0x%x!", g_cust_cap.trx_switch);
#ifdef _PRE_WLAN_CHBA_MGMT
    if (hwifi_get_cust_read_status(CUS_TAG_HOST, WLAN_CFG_HOST_CHBA_INIT_SOCIAL_CHAN)) {
        g_chba_init_social_channel = g_cust_cap.chba_init_social_channel;
    }

    if (hwifi_get_cust_read_status(CUS_TAG_HOST, WLAN_CFG_HOST_CHBA_SOCIAL_FOLLOW_WORK)) {
        g_chba_social_chan_cap = g_cust_cap.chba_social_follow_work_disable;
    }
#endif
#ifdef _PRE_WLAN_FEATURE_FTM
    if (hwifi_get_cust_read_status(CUS_TAG_CAP, WLAN_CFG_CAP_FTM)) {
        g_mac_ftm_cap = g_cust_cap.ftm_cap;
    }
#endif
}

void hwifi_cfg_host_global_init_mcm_shenkuo(void)
{
    uint8_t chan_radio_cap, chan_num_2g, chan_num_5g;

    chan_radio_cap = g_cust_cap.chn_radio_cap;
    /* 获取通道数目,刷新nss信息到mac_device */
    chan_num_2g = chan_radio_cap & 0x0F;
    chan_num_5g = chan_radio_cap & 0xF0;

    g_pst_mac_device_capability[0].en_nss_num =
        oal_max(oal_bit_get_num_one_byte(chan_num_2g), oal_bit_get_num_one_byte(chan_num_5g)) - 1;

    g_mcm_mask_custom = g_cust_cap.mcm_custom_func_mask;
}

/* 将定制化数据配置到host业务使用的全局变量 */
void hwifi_cfg_host_global_init_param_shenkuo(void)
{
    uint8_t device_idx;

    hwifi_cfg_host_global_switch_init_shenkuo();
    /*************************** 私有定制化 *******************************/
    for (device_idx = 0; device_idx < WLAN_SERVICE_DEVICE_MAX_NUM_PER_CHIP; device_idx++) {
        /* 定制化 RADIO_0 bit4 给dbdc软件开关用 */
        g_wlan_priv_dbdc_radio_cap = (uint8_t)((g_cust_cap.radio_cap[device_idx] & 0x10) >> 4);
        g_wlan_service_device_per_chip[device_idx] = g_cust_cap.radio_cap[device_idx] & 0x0F;
    }
    /* 同步host侧业务device */
    if (memcpy_s(g_auc_mac_device_radio_cap, WLAN_SERVICE_DEVICE_MAX_NUM_PER_CHIP,
        g_wlan_service_device_per_chip, WLAN_SERVICE_DEVICE_MAX_NUM_PER_CHIP) != EOK) {
        oal_io_print("hwifi_cfg_host_global_init_param_shenkuo:device_radio cap memcpy_s fail!");
    }

    g_pst_mac_device_capability[0].en_channel_width = g_cust_cap.en_channel_width;
    g_pst_mac_device_capability[0].en_ldpc_is_supp = g_cust_cap.en_ldpc_is_supp;

    hwifi_cfg_host_global_init_sounding_shenkuo();

    if (memcpy_s(&g_pst_mac_device_capability[0].hisi_priv_cap, sizeof(mac_hisi_cap_vendor_ie_stru),
        &g_cust_cap.hisi_priv_cap, sizeof(uint32_t)) != EOK) {
        oal_io_print("hwifi_cfg_host_global_init_param_shenkuo:hisi_priv cap memcpy_s fail!");
    }

    g_pst_mac_device_capability[0].en_1024qam_is_supp = !!(g_cust_cap.hisi_priv_cap & BIT4);

    g_download_rate_limit_pps = g_cust_cap.download_rate_limit_pps;

    hwifi_cfg_host_global_init_mcm_shenkuo();

    hwifi_cfg_host_global_init_param_extend_shenkuo();
}

oal_bool_enum_uint8 wlan_first_powon_mark_shenkuo(void)
{
    oal_bool_enum_uint8 cali_first_pwr_on = OAL_TRUE;

    if (hwifi_get_cust_read_status(CUS_TAG_CAP, WLAN_CFG_CAP_CALI_DATA_MASK)) {
        cali_first_pwr_on = !!(CALI_FIST_POWER_ON_MASK & g_cust_cap.cali_data_mask);
        oal_io_print("wlan_first_powon_mark_shenkuo:cali_first_pwr_on [%d]\r\n",
                     cali_first_pwr_on);
    }
    return cali_first_pwr_on;
}

oal_bool_enum_uint8 wlan_chip_is_aput_support_160m_shenkuo(void)
{
    if (hwifi_get_cust_read_status(CUS_TAG_CAP, WLAN_CFG_CAP_APUT_160M_ENABLE)) {
        return !!g_cust_cap.aput_160m_switch;
    }
    return OAL_FALSE;
}

void wlan_chip_get_flow_ctrl_used_mem_shenkuo(struct wlan_flow_ctrl_params *flow_ctrl)
{
    flow_ctrl->start = g_cust_cap.used_mem_for_start;
    flow_ctrl->stop  = g_cust_cap.used_mem_for_stop;
}

uint32_t hwifi_force_update_rf_params_shenkuo(void)
{
    return hwifi_config_init_shenkuo(CUS_TAG_POW);
}

uint8_t wlan_chip_get_selfstudy_country_flag_shenkuo(void)
{
    return g_cust_cap.country_self_study;
}

uint32_t wlan_chip_get_11ax_switch_mask_shenkuo(void)
{
    return g_cust_cap.wifi_11ax_switch_mask;
}

oal_bool_enum_uint8 wlan_chip_get_11ac2g_enable_shenkuo(void)
{
    return g_cust_host.wlan_11ac2g_switch;
}
uint32_t wlan_chip_get_probe_resp_mode_shenkuo(void)
{
    return g_cust_host.wlan_probe_resp_mode;
}
uint32_t wlan_chip_get_trx_switch_shenkuo(void)
{
    return g_cust_cap.trx_switch;
}
uint8_t wlan_chip_get_d2h_access_ddr_shenkuo(void)
{
    return g_cust_cap.device_access_ddr;
}
#endif // _PRE_PLAT_FEATURE_CUSTOMIZE
uint8_t wlan_chip_get_chn_radio_cap_shenkuo(void)
{
    return g_cust_cap.chn_radio_cap;
}

const uint32_t g_cmd_need_filter_shenkuo[] = {
    WLAN_CFGID_ETH_SWITCH,
    WLAN_CFGID_80211_UCAST_SWITCH,
    WLAN_CFGID_80211_MCAST_SWITCH,
    WLAN_CFGID_PROBE_SWITCH,
    WLAN_CFGID_OTA_BEACON_SWITCH,
    WLAN_CFGID_OTA_RX_DSCR_SWITCH,
    WLAN_CFGID_VAP_STAT,
    WLAN_CFGID_REPORT_VAP_INFO,
    WLAN_CFGID_USER_INFO,
    WLAN_CFGID_ALL_STAT,
    WLAN_CFGID_BSS_TYPE,
    WLAN_CFGID_GET_HIPKT_STAT,
    WLAN_CFGID_PROT_MODE,
    WLAN_CFGID_BEACON_INTERVAL,
    WLAN_CFGID_NO_BEACON,
    WLAN_CFGID_DTIM_PERIOD,
    WLAN_CFGID_SET_POWER_TEST,
    WLAN_CFGID_GET_ANT,
    WALN_CFGID_SET_P2P_SCENES,
    WLAN_CFGID_QUERY_RSSI,
    WLAN_CFGID_QUERY_PSST,
    WLAN_CFGID_QUERY_RATE,
    WLAN_CFGID_QUERY_ANI,
    WLAN_CFGID_SET_P2P_MIRACAST_STATUS,
    WLAN_CFGID_SET_CUS_DYN_CALI_PARAM,
    WLAN_CFGID_SET_ALL_LOG_LEVEL,
    WLAN_CFGID_SET_CUS_RF,
    WLAN_CFGID_SET_CUS_DTS_CALI,
    WLAN_CFGID_SET_CUS_NVRAM_PARAM,
    WLAN_CFGID_SHOW_DEV_CUSTOMIZE_INFOS,
    WLAN_CFGID_SEND_BAR,
    WLAN_CFGID_RESET_HW,
    WLAN_CFGID_GET_USER_RSSBW,
    WLAN_CFGID_ENABLE_ARP_OFFLOAD,
    WLAN_CFGID_SHOW_ARPOFFLOAD_INFO,
    WLAN_CFGID_SET_AUTO_PROTECTION,
    WLAN_CFGID_SET_BW_FIXED,
    WLAN_CFGID_SET_FEATURE_LOG,
    WLAN_CFGID_WMM_SWITCH,
    WLAN_CFGID_NARROW_BW,
};

oal_bool_enum_uint8 wlan_chip_h2d_cmd_need_filter_shenkuo(uint32_t cmd_id)
{
    uint32_t idx;

    for (idx = 0; idx < oal_array_size(g_cmd_need_filter_shenkuo); idx++) {
        if (cmd_id == g_cmd_need_filter_shenkuo[idx]) {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE; /* 不用过滤 */
}

uint32_t wlan_chip_update_cfg80211_mgmt_tx_wait_time_shenkuo(uint32_t wait_time)
{
    if (!hi110x_is_asic() && wait_time <= 100) { // 100 shenkuo FPGA 切换信道时间较长，对于等待时间小于100ms 发帧，修改为150ms
        wait_time = 150;    // 小于100ms 监听时间，增加为150ms，增加发送时间。避免管理帧未来得及发送，超时定时器就结束。
    }
    return wait_time;
}

oal_bool_enum_uint8 wlan_chip_check_need_setup_ba_session_shenkuo(void)
{
    /* shenkuo device tx由于硬件约束不允许建立聚合 */
    return hmac_ring_tx_enabled();
}

oal_bool_enum_uint8 wlan_chip_check_need_process_bar_shenkuo(void)
{
    /* shenkuo 硬件处理bar，软件不需要处理 */
    return OAL_FALSE;
}

oal_bool_enum_uint8 wlan_chip_ba_need_check_lut_idx_shenkuo(void)
{
    /* shenkuo无lut限制，不用检查LUT idx */
    return OAL_FALSE;
}

void wlan_chip_mac_mib_set_auth_rsp_time_out_shenkuo(mac_vap_stru *mac_vap)
{
    if (mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
        mac_mib_set_AuthenticationResponseTimeOut(mac_vap, WLAN_AUTH_AP_TIMEOUT);
    } else {
        /* shenkuo FPGA阶段 入网auth时间加长为1105一倍 */
        mac_mib_set_AuthenticationResponseTimeOut(mac_vap, WLAN_AUTH_TIMEOUT * 2); /* 入网时间增加2倍 */
    }
}

void wlan_chip_proc_query_station_packets_shenkuo(hmac_vap_stru *hmac_vap,
    dmac_query_station_info_response_event *response_event)
{
    hmac_vap_stat_stru *hmac_vap_stat = hmac_stat_get_vap_stat(hmac_vap);
    /* shenkuo tx/rx帧统计从host获取 */
    hmac_vap->station_info.rx_packets = hmac_vap_stat->rx_packets;
    hmac_vap->station_info.rx_bytes = hmac_vap_stat->rx_bytes;
    hmac_vap->station_info.rx_dropped_misc = response_event->rx_dropped_misc + hmac_vap_stat->rx_dropped_misc;

    hmac_vap->station_info.tx_packets = hmac_vap_stat->tx_packets;
    hmac_vap->station_info.tx_bytes = hmac_vap_stat->tx_bytes;
    hmac_vap->station_info.tx_retries = response_event->tx_retries + hmac_vap_stat->tx_retries;
    hmac_vap->station_info.tx_failed = response_event->tx_failed + hmac_vap_stat->tx_failed;
}

uint32_t wlan_chip_scan_req_alloc_and_fill_netbuf_shenkuo(frw_event_mem_stru *event_mem, hmac_vap_stru *hmac_vap,
    oal_netbuf_stru **netbuf_scan_req, void *params)
{
    frw_event_stru *event = NULL;
    dmac_tx_event_stru *scan_req_event = NULL;
    mac_scan_req_h2d_stru *scan_req_h2d = (mac_scan_req_h2d_stru *)params;
    mac_scan_req_ex_stru *scan_req_ex = NULL;

    /* 申请netbuf内存  */
    (*netbuf_scan_req) = oal_mem_netbuf_alloc(OAL_NORMAL_NETBUF,
        (sizeof(mac_scan_req_ex_stru)), OAL_NETBUF_PRIORITY_MID);
    if ((*netbuf_scan_req) == NULL) {
        oam_error_log0(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
            "{wlan_chip_scan_req_alloc_and_fill_netbuf_shenkuo::netbuf_scan_req alloc failed.}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }
    /* 填写事件 */
    event = frw_get_event_stru(event_mem);

    frw_event_hdr_init(&(event->st_event_hdr), FRW_EVENT_TYPE_WLAN_CTX, DMAC_WLAN_CTX_EVENT_SUB_TYPE_SCAN_REQ,
        sizeof(dmac_tx_event_stru), FRW_EVENT_PIPELINE_STAGE_1, hmac_vap->st_vap_base_info.uc_chip_id,
        hmac_vap->st_vap_base_info.uc_device_id, hmac_vap->st_vap_base_info.uc_vap_id);

    /***************************** copy data **************************/
    memset_s(oal_netbuf_cb(*netbuf_scan_req), OAL_TX_CB_LEN, 0, OAL_TX_CB_LEN);
    scan_req_ex = (mac_scan_req_ex_stru *)(oal_netbuf_data(*netbuf_scan_req));
    /* 拷贝扫描请求参数到netbuf data区域 */
    scan_req_ex->scan_flag = scan_req_h2d->scan_flag;
    hmac_scan_param_convert_ex(scan_req_ex, &(scan_req_h2d->st_scan_params));

    /* 拷贝netbuf 到事件数据区域 */
    scan_req_event = (dmac_tx_event_stru *)event->auc_event_data;
    scan_req_event->pst_netbuf = (*netbuf_scan_req);
    scan_req_event->us_frame_len = sizeof(mac_scan_req_ex_stru);
    scan_req_event->us_remain = 0;
    return OAL_SUCC;
}

oal_bool_enum wlan_chip_is_dbdc_ini_en_shenkuo(void)
{
    if ((g_cust_cap.radio_cap[0] & BIT4) == 0) {
        return OAL_FALSE;
    }
    return OAL_TRUE;
}

uint32_t wlan_chip_send_connect_security_params_shenkuo(mac_vap_stru *mac_vap,
    mac_conn_security_stru *connect_security_params)
{
    return hmac_config_send_event(mac_vap, WLAN_CFGID_CONNECT_REQ,
        sizeof(*connect_security_params), (uint8_t *)connect_security_params);
}

/*
 * 检查加密的PN号是否异常。
 * 加密场景下，需要后一个分片帧PN严格比前一个分片帧PN+1，否则丢弃分片帧
 * 参考资料:80211-2016 12.5.3.3.2 PN processing
 * 返回值：OAL_TRUE:PN异常；OAL_FALSE:PN正常
 */
oal_bool_enum_uint8 wlan_chip_defrag_is_pn_abnormal_shenkuo(hmac_user_stru *hmac_user,
    oal_netbuf_stru *netbuf, oal_netbuf_stru *last_netbuf)
{
    uint64_t                last_pn_val;
    uint64_t                rx_pn_val;
    mac_rx_ctl_stru        *rx_ctl = NULL;
    mac_rx_ctl_stru        *last_rx_ctl = NULL;

    wlan_ciper_protocol_type_enum_uint8 cipher_type = hmac_user->st_user_base_info.st_key_info.en_cipher_type;

    if (cipher_type != WLAN_80211_CIPHER_SUITE_CCMP && cipher_type != WLAN_80211_CIPHER_SUITE_GCMP &&
        cipher_type != WLAN_80211_CIPHER_SUITE_CCMP_256 && cipher_type != WLAN_80211_CIPHER_SUITE_GCMP_256) {
        return OAL_FALSE;
    }

    rx_ctl = (mac_rx_ctl_stru *)oal_netbuf_cb(netbuf);
    last_rx_ctl = (mac_rx_ctl_stru *)oal_netbuf_cb(last_netbuf);

    rx_pn_val = ((uint64_t)rx_ctl->us_rx_msb_pn << 32) | (rx_ctl->rx_lsb_pn);
    last_pn_val = ((uint64_t)last_rx_ctl->us_rx_msb_pn << 32) | (last_rx_ctl->rx_lsb_pn);
    if (rx_pn_val != (last_pn_val + 1)) {
        oam_warning_log3(0, OAM_SF_ANY,
            "{wlan_chip_defrag_is_pn_abnormal_shenkuo::cipher_type[%u], rx_pn_val[%u], last_pn_val[%u].}",
            cipher_type, rx_pn_val, last_pn_val);
        return OAL_TRUE;
    }
    return OAL_FALSE;
}

oal_bool_enum_uint8 wlan_chip_is_support_hw_wapi_shenkuo(void)
{
    return g_wlan_spec_cfg->feature_hw_wapi;
}

mac_scanned_result_extend_info_stru *wlan_chip_get_scaned_result_extend_info_shenkuo(oal_netbuf_stru *netbuf,
    uint16_t frame_len)
{
    return (mac_scanned_result_extend_info_stru *)(oal_netbuf_data(netbuf) -
        sizeof(mac_scanned_result_extend_info_stru));
}

uint16_t wlan_chip_get_scaned_payload_extend_len_shenkuo(void)
{
    return 0;
}

const struct wlan_chip_ops g_wlan_chip_ops_shenkuo = {
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
    .update_rxctl_data_type = hmac_rx_netbuf_update_rxctl_data_type,
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
    .mcast_ampdu_rx_ba_init = hmac_mcast_ampdu_rx_win_init,
    .mcast_stats_stru_init = hmac_mcast_ampdu_stats_stru_init,
#endif
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU_HW
    .mcast_ampdu_sta_add_multi_user = NULL,
#endif
    .is_dbdc_ini_en = wlan_chip_is_dbdc_ini_en_shenkuo,
#if defined(CONFIG_ARCH_HISI) &&  defined(CONFIG_NR_CPUS)
#if CONFIG_NR_CPUS > OAL_BUS_HPCPU_NUM
    .frw_task_bind_cpu = hmac_rx_frw_task_bind_cpu,
#endif
#endif

    .send_connect_security_params = wlan_chip_send_connect_security_params_shenkuo,
    .defrag_is_pn_abnormal = wlan_chip_defrag_is_pn_abnormal_shenkuo,
    .is_support_hw_wapi = wlan_chip_is_support_hw_wapi_shenkuo,
    .get_scaned_result_extend_info = wlan_chip_get_scaned_result_extend_info_shenkuo,
    .get_scaned_payload_extend_len = wlan_chip_get_scaned_payload_extend_len_shenkuo,
    .tx_pt_mcast_set_cb = NULL,
};
