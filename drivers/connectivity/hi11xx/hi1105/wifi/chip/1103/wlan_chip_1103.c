

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
#include "hmac_tx_amsdu.h"
#include "hmac_hcc_adapt.h"
#include "hmac_scan.h"
#include "wal_linux_ioctl.h"
#include "hmac_config.h"
#include "mac_mib.h"
#ifdef _PRE_WLAN_FEATURE_TCP_ACK_BUFFER
#include "hmac_tcp_ack_buf.h"
#endif
#ifdef _PRE_WLAN_FEATURE_WMMAC
#include "hmac_wmmac.h"
#endif

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#include "plat_pm_wlan.h"
#endif
#include "hisi_customize_wifi.h"
#include "hisi_customize_wifi_hi1103.h"
#include "hisi_customize_config_dts_hi1103.h"
#include "hisi_customize_config_priv_hi1103.h"
#include "hisi_customize_config_cmd_hi1103.h"
#ifdef _PRE_WLAN_FEATURE_DFS
#include "hmac_dfs.h"
#endif


#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WLAN_CHIP_1103_C

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
OAL_STATIC void hwifi_cfg_host_global_switch_init(void)
{
#ifndef _PRE_LINUX_TEST
    /*************************** �͹��Ķ��ƻ� *****************************/
    /* ����device �͹��Ŀ��ز���true false,��host��,��ȡ���ƻ���ֵ����device����,�ٸ��ݴ�ֵ��host�͹��Ŀ��ظ�ֵ */
    g_wlan_device_pm_switch = (uint8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_POWERMGMT_SWITCH);
    g_wlan_ps_mode = (uint8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_PS_MODE);
    g_wlan_fast_ps_dyn_ctl = ((MAX_FAST_PS == g_wlan_ps_mode) ? 1 : 0);
    g_wlan_min_fast_ps_idle = (uint8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_MIN_FAST_PS_IDLE);
    g_wlan_max_fast_ps_idle = (uint8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_MAX_FAST_PS_IDLE);
    g_wlan_auto_ps_screen_on = (uint8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_AUTO_FAST_PS_THRESH_SCREENON);
    g_wlan_auto_ps_screen_off = (uint8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_AUTO_FAST_PS_THRESH_SCREENOFF);
    g_wlan_host_pm_switch = (g_wlan_device_pm_switch == WLAN_DEV_ALL_ENABLE ||
        g_wlan_device_pm_switch == WLAN_DEV_LIGHT_SLEEP_SWITCH_EN) ? OAL_TRUE : OAL_FALSE;

    g_feature_switch[HMAC_MIRACAST_SINK_SWITCH] =
        (hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_MIRACAST_SINK_ENABLE) == 1);
    g_feature_switch[HMAC_MIRACAST_REDUCE_LOG_SWITCH] =
        (hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_REDUCE_MIRACAST_LOG) == 1);
    g_feature_switch[HMAC_BIND_LOWEST_LOAD_CPU] =
        (hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_BIND_LOWEST_LOAD_CPU) == 1);
    g_feature_switch[HMAC_CORE_BIND_SWITCH] =
        (hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_CORE_BIND_CAP) == 1);
#endif
}

OAL_STATIC void hwifi_cfg_host_global_init_sounding(void)
{
    int32_t l_priv_value = 0;
    int32_t l_ret;

    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_RX_STBC, &l_priv_value);
    oal_io_print("hwifi_cfg_host_global_init_param: rx stbc[%d] ret[%d]\r\n", l_priv_value, l_ret);
    if (l_ret == OAL_SUCC) {
        g_pst_mac_device_capability[0].en_rx_stbc_is_supp = (oal_bool_enum_uint8)l_priv_value;
    }

    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_TX_STBC, &l_priv_value);
    oal_io_print("hwifi_cfg_host_global_init_param: tx stbc[%d] ret[%d]\r\n", l_priv_value, l_ret);
    if (l_ret == OAL_SUCC) {
        g_pst_mac_device_capability[0].en_tx_stbc_is_supp = (oal_bool_enum_uint8)l_priv_value;
    }

    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_SU_BFER, &l_priv_value);
    oal_io_print("hwifi_cfg_host_global_init_param: su bfer[%d] ret[%d]\r\n", l_priv_value, l_ret);
    if (l_ret == OAL_SUCC) {
        g_pst_mac_device_capability[0].en_su_bfmer_is_supp = (oal_bool_enum_uint8)l_priv_value;
    }

    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_SU_BFEE, &l_priv_value);
    oal_io_print("hwifi_cfg_host_global_init_param: su bfee[%d] ret[%d]\r\n", l_priv_value, l_ret);
    if (l_ret == OAL_SUCC) {
        g_pst_mac_device_capability[0].en_su_bfmee_is_supp = (oal_bool_enum_uint8)l_priv_value;
    }

    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_MU_BFER, &l_priv_value);
    oal_io_print("hwifi_cfg_host_global_init_param: mu bfer[%d] ret[%d]\r\n", l_priv_value, l_ret);
    if (l_ret == OAL_SUCC) {
        g_pst_mac_device_capability[0].en_mu_bfmer_is_supp = (oal_bool_enum_uint8)l_priv_value;
    }

    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_MU_BFEE, &l_priv_value);
    oal_io_print("hwifi_cfg_host_global_init_param: mu bfee[%d] ret[%d]\r\n", l_priv_value, l_ret);
    if (l_ret == OAL_SUCC) {
        g_pst_mac_device_capability[0].en_mu_bfmee_is_supp = (oal_bool_enum_uint8)l_priv_value;
    }
}


OAL_STATIC void hwifi_cfg_host_global_init_param_extend(void)
{
    int32_t priv_value = 0;
    int32_t l_ret;

#ifdef _PRE_FEATURE_PLAT_LOCK_CPUFREQ
    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_LOCK_CPU_FREQ, &priv_value);
    if (l_ret == OAL_SUCC) {
        g_freq_lock_control.uc_lock_max_cpu_freq = (oal_bool_enum_uint8) !!priv_value;
    }
    oal_io_print("hwifi_cfg_host_global_init_param_extend:lock_max_cpu_freq[%d]\r\n",
        g_freq_lock_control.uc_lock_max_cpu_freq);
#endif

#ifdef _PRE_WLAN_FEATURE_HIEX
    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_HIEX_CAP, &priv_value);
    if (l_ret == OAL_SUCC) {
        if (memcpy_s(&g_st_default_hiex_cap, sizeof(mac_hiex_cap_stru), &priv_value,
            sizeof(priv_value)) != EOK) {
            oal_io_print("hwifi_cfg_host_global_init_param_extend:hiex cap memcpy_s fail!");
        }
    }
    oal_io_print("hwifi_cfg_host_global_init_param_extend:hiex cap[0x%X]\r\n", *(uint32_t *)&g_st_default_hiex_cap);
#endif
    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_OPTIMIZED_FEATURE_SWITCH, &priv_value);
    if (l_ret == OAL_SUCC) {
        g_optimized_feature_switch_bitmap = (uint8_t)priv_value;
    }

    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_P2P_GO_ASSOC_USER_MAX_NUM, &priv_value);
    if (l_ret == OAL_SUCC && (uint8_t)priv_value > 0 && (uint8_t)priv_value <= 8) { /* 8Ϊ����û��� */
        g_p2p_go_max_user_num = (uint8_t)priv_value;
    } else {
        oam_warning_log2(0, OAM_SF_INI,
            "{hwifi_cfg_host_global_init_param_extend::wlan_go_assoc_user_max_num read fail, ret[%d], value[%d].}",
            l_ret, (uint8_t)priv_value);
    }
#ifdef _PRE_WLAN_FEATURE_FTM
        l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_FTM_CAP, &priv_value);
        if (l_ret == OAL_SUCC) {
            g_mac_ftm_cap = (uint8_t)priv_value;
        }
#endif
}

void hwifi_cfg_host_global_init_dbdc_param(void)
{
    uint8_t uc_cmd_idx;
    uint8_t uc_device_idx;
    int32_t l_priv_value = 0;
    int32_t l_ret;

    uc_cmd_idx = WLAN_CFG_PRIV_DBDC_RADIO_0;
    for (uc_device_idx = 0; uc_device_idx < WLAN_SERVICE_DEVICE_MAX_NUM_PER_CHIP; uc_device_idx++) {
        l_ret = hwifi_get_init_priv_value(uc_cmd_idx, &l_priv_value);
        if (l_ret == OAL_SUCC) {
            /* ���ƻ� RADIO_0 bit4 ��dbdc��������� */
            g_wlan_priv_dbdc_radio_cap = (uint8_t)(((uint32_t)l_priv_value & 0x10) >> 4);
            l_priv_value = (int32_t)((uint32_t)l_priv_value & 0x0F);
            g_wlan_service_device_per_chip[uc_device_idx] = (uint8_t)(uint32_t)l_priv_value;
        }

        uc_cmd_idx++;
    }
    /* ͬ��host��ҵ��device */
    memcpy_s(g_auc_mac_device_radio_cap, WLAN_SERVICE_DEVICE_MAX_NUM_PER_CHIP,
             g_wlan_service_device_per_chip, WLAN_SERVICE_DEVICE_MAX_NUM_PER_CHIP);
}

/*
 * �� �� ��  : hwifi_cfg_host_global_init_param_1103
 * ��������  : ��ʼ�����ƻ�ini�ļ�host��ȫ�ֱ���
 */
OAL_STATIC void hwifi_cfg_host_global_init_param_1103(void)
{
    int32_t l_priv_value = 0;
    int32_t l_ret;

    hwifi_cfg_host_global_switch_init();
    /*************************** ˽�ж��ƻ� *******************************/
    hwifi_cfg_host_global_init_dbdc_param();
    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_BW_MAX_WITH, &l_priv_value);
    oal_io_print("hwifi_cfg_host_global_init_param: bw max with[%d] ret[%d]\r\n", l_priv_value, l_ret);
    if (l_ret == OAL_SUCC) {
        g_pst_mac_device_capability[0].en_channel_width = (wlan_bw_cap_enum_uint8)l_priv_value;
    }

    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_LDPC_CODING, &l_priv_value);
    oal_io_print("hwifi_cfg_host_global_init_param: ldpc coding[%d] ret[%d]\r\n", l_priv_value, l_ret);
    if (l_ret == OAL_SUCC) {
        g_pst_mac_device_capability[0].en_ldpc_is_supp = (oal_bool_enum_uint8)l_priv_value;
    }

    hwifi_cfg_host_global_init_sounding();

    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_1024_QAM, &l_priv_value);
    oal_io_print("hwifi_cfg_host_global_init_param: 1024 qam[%d] ret[%d]\r\n", l_priv_value, l_ret);
    if (l_ret == OAL_SUCC) {
        g_pst_mac_device_capability[0].en_1024qam_is_supp = (oal_bool_enum_uint8)l_priv_value;
    }
    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_DOWNLOAD_RATE_LIMIT_PPS, &l_priv_value);
    oal_io_print("hwifi_cfg_host_global_init_param:download rx drop threshold[%d] ret[%d]\r\n", l_priv_value, l_ret);
    if (l_ret == OAL_SUCC) {
        g_download_rate_limit_pps = (uint16_t)l_priv_value;
    }

    hwifi_cfg_host_global_init_param_extend();

    return;
}

OAL_STATIC oal_bool_enum_uint8 wlan_first_powon_mark_1103(void)
{
    oal_bool_enum_uint8 cali_first_pwr_on = OAL_TRUE;
    int32_t l_priv_value = 0;

    if (hwifi_get_init_priv_value(WLAN_CFG_PRIV_CALI_DATA_MASK, &l_priv_value) == OAL_SUCC) {
        cali_first_pwr_on = !!(CALI_FIST_POWER_ON_MASK & (uint32_t)l_priv_value);
        oal_io_print("host_module_init:cali_first_pwr_on pri_val[0x%x]first_pow[%d]\r\n",
            l_priv_value, cali_first_pwr_on);
    }
    return cali_first_pwr_on;
}

OAL_STATIC oal_bool_enum_uint8 wlan_chip_is_aput_support_160m_1103(void)
{
    int32_t l_priv_value;
    if (hwifi_get_init_priv_value(WLAN_CFG_APUT_160M_ENABLE, &l_priv_value) == OAL_SUCC) {
        return !!l_priv_value;
    }
    return OAL_FALSE;
}

OAL_STATIC void wlan_chip_get_flow_ctrl_used_mem_1103(struct wlan_flow_ctrl_params *flow_ctrl)
{
    flow_ctrl->start = (uint16_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_USED_MEM_FOR_START);
    flow_ctrl->stop  = (uint16_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_USED_MEM_FOR_STOP);
}

OAL_STATIC uint32_t hwifi_force_update_rf_params_1103(void)
{
    return hwifi_config_init(CUS_TAG_NV);
}

OAL_STATIC uint8_t wlan_chip_get_selfstudy_country_flag_1103(void)
{
    int32_t l_priv_value = 0;
    hwifi_get_init_priv_value(WLAN_CFG_PRIV_COUNRTYCODE_SELFSTUDY_CFG, &l_priv_value);
    return (uint8_t)l_priv_value;
}

OAL_STATIC uint32_t wlan_chip_get_11ax_switch_mask_1103(void)
{
    int32_t l_priv_value = 0;
    hwifi_get_init_priv_value(WLAN_CFG_PRIV_11AX_SWITCH, &l_priv_value);
    return l_priv_value;
}

OAL_STATIC oal_bool_enum_uint8 wlan_chip_get_11ac2g_enable_1103(void)
{
    return !!hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_11AC2G_ENABLE);
}

OAL_STATIC uint32_t wlan_chip_get_probe_resp_mode_1103(void)
{
    return hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_PROBE_RESP_MODE);
}
#endif // _PRE_PLAT_FEATURE_CUSTOMIZE

OAL_STATIC oal_bool_enum_uint8 wlan_chip_h2d_cmd_need_filter_1103(uint32_t cmd_id)
{
    /* ��1103/1105��Ʒ��shenkuo�����¼�����Ҫ�·�device */
    return oal_value_in_valid_range(cmd_id, WLAN_CFGID_SHENKUO_PRIV_START, WLAN_CFGID_SHENKUO_PRIV_END);
}

OAL_STATIC uint32_t wlan_chip_update_cfg80211_mgmt_tx_wait_time_1103(uint32_t wait_time)
{
    return wait_time;
}

OAL_STATIC oal_bool_enum_uint8 wlan_chip_check_need_setup_ba_session_1103(void)
{
    return OAL_TRUE;
}

OAL_STATIC oal_bool_enum_uint8 wlan_chip_check_need_process_bar_1103(void)
{
    /* 1103/1105��Ҫ�������bar */
    return OAL_TRUE;
}

OAL_STATIC oal_bool_enum_uint8 wlan_chip_ba_need_check_lut_idx_1103(void)
{
    /* 1103/1105��Ҫ���LUT idx */
    return OAL_TRUE;
}

OAL_STATIC void wlan_chip_mac_mib_set_auth_rsp_time_out_1103(mac_vap_stru *mac_vap)
{
    if (mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
        mac_mib_set_AuthenticationResponseTimeOut(mac_vap, WLAN_AUTH_AP_TIMEOUT);
    } else {
        mac_mib_set_AuthenticationResponseTimeOut(mac_vap, WLAN_AUTH_TIMEOUT);
    }
}

static void wlan_chip_proc_query_station_packets_1103(hmac_vap_stru *hmac_vap,
    dmac_query_station_info_response_event *response_event)
{
    /* 1103 tx/rx֡ͳ�ƴ�host��ȡ */
    hmac_vap->station_info.rx_packets = response_event->rx_packets;
    hmac_vap->station_info.rx_bytes = response_event->rx_bytes;
    hmac_vap->station_info.rx_dropped_misc = response_event->rx_dropped_misc;

    hmac_vap->station_info.tx_packets = response_event->tx_packets;
    hmac_vap->station_info.tx_bytes = response_event->tx_bytes;
    hmac_vap->station_info.tx_retries = response_event->tx_retries;
    hmac_vap->station_info.tx_failed = response_event->tx_failed;
}
static uint32_t wlan_chip_scan_req_alloc_and_fill_netbuf_1103(frw_event_mem_stru *event_mem, hmac_vap_stru *hmac_vap,
                                                              oal_netbuf_stru **netbuf_scan_req, void *params)
{
    int32_t ret;
    uint8_t *netbuf_data = NULL;
    frw_event_stru *event = NULL;
    dmac_tx_event_stru *scan_req_event = NULL;
    mac_scan_req_stru *scan_params = NULL;

    /* ����netbuf�ڴ�  */
    (*netbuf_scan_req) = oal_mem_netbuf_alloc(OAL_NORMAL_NETBUF,
        (sizeof(mac_scan_req_h2d_stru)), OAL_NETBUF_PRIORITY_MID);
    if ((*netbuf_scan_req) == NULL) {
        oam_error_log0(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_req_post_event_msg_info_set_1103::netbuf_scan_req alloc failed.}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }
    /* ��д�¼� */
    event = frw_get_event_stru(event_mem);

    frw_event_hdr_init(&(event->st_event_hdr), FRW_EVENT_TYPE_WLAN_CTX, DMAC_WLAN_CTX_EVENT_SUB_TYPE_SCAN_REQ,
        sizeof(dmac_tx_event_stru), FRW_EVENT_PIPELINE_STAGE_1, hmac_vap->st_vap_base_info.uc_chip_id,
        hmac_vap->st_vap_base_info.uc_device_id, hmac_vap->st_vap_base_info.uc_vap_id);

    /***************************** copy data **************************/
    memset_s(oal_netbuf_cb(*netbuf_scan_req), OAL_TX_CB_LEN, 0, OAL_TX_CB_LEN);
    netbuf_data = (uint8_t *)(oal_netbuf_data(*netbuf_scan_req));
    /* ����ɨ�����������netbuf data���� */
    ret = memcpy_s(netbuf_data, sizeof(mac_scan_req_h2d_stru), (uint8_t *)params, sizeof(mac_scan_req_h2d_stru));
    if (ret != EOK) {
        oal_netbuf_free(*netbuf_scan_req);
        oam_error_log0(0, OAM_SF_ANY, "wlan_chip_scan_req_alloc_and_fill_netbuf_1103::memcpy fail!");
        return OAL_FAIL;
    }
    scan_params = &(((mac_scan_req_h2d_stru *)netbuf_data)->st_scan_params);
    scan_params->p_fn_cb = NULL;

    /* ����netbuf ���¼��������� */
    scan_req_event = (dmac_tx_event_stru *)event->auc_event_data;
    scan_req_event->pst_netbuf = (*netbuf_scan_req);
    scan_req_event->us_frame_len = sizeof(mac_scan_req_h2d_stru);
    scan_req_event->us_remain = 0;
    return OAL_SUCC;
}

static uint32_t wlan_chip_send_connect_security_params_1103(mac_vap_stru *mac_vap,
    mac_conn_security_stru *connect_security_params)
{
    mac_conn_security_stru_1103 connect_params_1103 = { 0 };

    /* �����������ݵ�1103/1105��Ӧ�ṹ�� */
    connect_params_1103.en_privacy = connect_security_params->en_privacy;
    connect_params_1103.en_auth_type = connect_security_params->en_auth_type;
    connect_params_1103.uc_wep_key_len = connect_security_params->uc_wep_key_len;
    connect_params_1103.uc_wep_key_index = connect_security_params->uc_wep_key_index;
    (void)memcpy_s(connect_params_1103.auc_wep_key, WLAN_WEP104_KEY_LEN,
        connect_security_params->auc_wep_key, WLAN_WEP104_KEY_LEN);
    connect_params_1103.en_mgmt_proteced = connect_security_params->en_mgmt_proteced;
    connect_params_1103.en_pmf_cap = connect_security_params->en_pmf_cap;
    connect_params_1103.en_wps_enable = connect_security_params->en_wps_enable;
    /* 1103/1105���������ṹ��ת�� */
    connect_params_1103.st_crypto.wpa_versions = connect_security_params->st_crypto.wpa_versions;
    connect_params_1103.st_crypto.group_suite = connect_security_params->st_crypto.group_suite;
    connect_params_1103.st_crypto.group_mgmt_suite = connect_security_params->st_crypto.group_mgmt_suite;
    connect_params_1103.st_crypto.aul_pair_suite[0] = connect_security_params->st_crypto.aul_pair_suite[0];
    connect_params_1103.st_crypto.aul_pair_suite[1] = connect_security_params->st_crypto.aul_pair_suite[1];
    connect_params_1103.st_crypto.aul_akm_suite[0] = connect_security_params->st_crypto.aul_akm_suite[0];
    connect_params_1103.st_crypto.aul_akm_suite[1] = connect_security_params->st_crypto.aul_akm_suite[1];

#ifdef _PRE_WLAN_FEATURE_11R
    (void)memcpy_s(connect_params_1103.auc_mde, NUM_8_BYTES,
        connect_security_params->auc_mde, NUM_8_BYTES);
#endif
    connect_params_1103.c_rssi = connect_security_params->c_rssi;
    (void)memcpy_s(connect_params_1103.rssi, HD_EVENT_RF_NUM, connect_security_params->rssi, HD_EVENT_RF_NUM);
    connect_params_1103.is_wapi_connect = connect_security_params->is_wapi_connect;

    return hmac_config_send_event(mac_vap, WLAN_CFGID_CONNECT_REQ,
        sizeof(connect_params_1103), (uint8_t *)(&connect_params_1103));
}

/*
 * �����ܵ�PN���Ƿ��쳣��03/05 оƬ���ϱ�PN,����FALSE
 * ����ֵ��OAL_TRUE:PN�쳣��OAL_FALSE:PN����
 */
static oal_bool_enum_uint8 wlan_chip_defrag_is_pn_abnormal_1103(hmac_user_stru *hmac_user,
    oal_netbuf_stru *netbuf, oal_netbuf_stru *last_netbuf)
{
    return OAL_FALSE;
}

static oal_bool_enum_uint8 wlan_chip_is_support_hw_wapi_1103(void)
{
    return g_wlan_spec_cfg->feature_hw_wapi;
}

static mac_scanned_result_extend_info_stru *wlan_chip_get_scaned_result_extend_info_1103(oal_netbuf_stru *netbuf,
    uint16_t frame_len)
{
    return (mac_scanned_result_extend_info_stru *)(oal_netbuf_data(netbuf) + frame_len);
}

static uint16_t wlan_chip_get_scaned_payload_extend_len_1103(void)
{
    return sizeof(mac_scanned_result_extend_info_stru);
}
// �鲥ͼ��Э�����ֶ����UDPͷƫ����
#define HMAC_PT_MCAST_PROTOCOL_OFFSET 12
static oal_bool_enum_uint8 wlan_chip_tx_is_pt_mcast_data(uint8_t *payload)
{
    return (payload[0] == 0x4e && payload[1] == 0x45 && payload[2] == 0x58 && payload[3] == 0x54);
}
static void wlan_chip_tx_pt_mcast_set_cb_1103(hmac_vap_stru *pst_vap,
    mac_ether_header_stru *ether_hdr, mac_tx_ctl_stru *tx_ctl)
{
    mac_ip_header_stru *ip_header = (mac_ip_header_stru *)(ether_hdr + 1);
    udp_hdr_stru *pst_udp_hdr = (udp_hdr_stru *)(ip_header + 1); /* ƫ��һ��IPͷ��ȡUDPͷ */
    uint8_t *payload = (uint8_t *)(pst_udp_hdr + 1);
    payload += HMAC_PT_MCAST_PROTOCOL_OFFSET;

    if (wlan_chip_tx_is_pt_mcast_data(payload) || pst_vap->en_pt_mcast_switch) {
        // ��ʶΪ�鲥ͼ������
        MAC_GET_CB_IS_PT_MCAST(tx_ctl) = OAL_TRUE;
    }
}

const struct wlan_chip_ops g_wlan_chip_ops_1103 = {
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    .host_global_init_param = hwifi_cfg_host_global_init_param_1103,
    .first_power_on_mark = wlan_first_powon_mark_1103,
    .first_powon_cali_completed = NULL,
    .is_aput_support_160M = wlan_chip_is_aput_support_160m_1103,
    .get_flow_ctrl_used_mem = wlan_chip_get_flow_ctrl_used_mem_1103,
    .force_update_custom_params = hwifi_force_update_rf_params_1103,
    .init_nvram_main = hwifi_config_init_nvram_main_1103,
    .cpu_freq_ini_param_init = hwifi_config_cpu_freq_ini_param_1103,
    .host_global_ini_param_init = hwifi_config_host_global_ini_param_1103,
    .get_selfstudy_country_flag = wlan_chip_get_selfstudy_country_flag_1103,
    .custom_cali = wal_custom_cali_1103,
    .custom_cali_data_host_addr_init = NULL,
    .send_cali_data = wal_send_cali_data_1103,
    .custom_host_read_cfg_init = hwifi_custom_host_read_cfg_init_1103,
    .hcc_customize_h2d_data_cfg = hwifi_hcc_customize_h2d_data_cfg_1103,
    .show_customize_info = hwifi_get_cfg_params,
    .get_sar_ctrl_params = hwifi_get_sar_ctrl_params_1103,
    .get_11ax_switch_mask = wlan_chip_get_11ax_switch_mask_1103,
    .get_11ac2g_enable = wlan_chip_get_11ac2g_enable_1103,
    .get_probe_resp_mode = wlan_chip_get_probe_resp_mode_1103,
    .get_d2h_access_ddr = NULL,
#endif
    .h2d_cmd_need_filter = wlan_chip_h2d_cmd_need_filter_1103,
    .update_cfg80211_mgmt_tx_wait_time = wlan_chip_update_cfg80211_mgmt_tx_wait_time_1103,

    // �շ��;ۺ����
    .ba_rx_hdl_init = hmac_ba_rx_hdl_init,
    .check_need_setup_ba_session = wlan_chip_check_need_setup_ba_session_1103,
    .tx_update_amsdu_num = hmac_update_amsdu_num_1103, /* ע�⣺�ú���1103/1105ʵ�ֲ�ͬ */
    .check_need_process_bar = wlan_chip_check_need_process_bar_1103,
    .ba_send_reorder_timeout = hmac_ba_send_reorder_timeout,
    .ba_need_check_lut_idx = wlan_chip_ba_need_check_lut_idx_1103,
#ifdef _PRE_WLAN_FEATURE_TCP_ACK_BUFFER
    .tcp_ack_buff_config = hmac_tcp_ack_buff_config_1103,
#endif
#ifdef _PRE_WLAN_FEATURE_WMMAC
    .wmmac_need_degrade_for_ts = hmac_need_degrade_for_ts_1103,
#endif
    .update_arp_tid = NULL,
    .get_6g_flag = mac_get_rx_6g_flag_1103,
    // У׼���
    .send_cali_matrix_data = hmac_send_cali_matrix_data_1103,
    .save_cali_event = hmac_save_cali_event_1103,
    .update_cur_chn_cali_data = NULL,
    .get_chn_radio_cap = NULL,
#ifdef _PRE_WLAN_FEATURE_11AX
    .mac_vap_init_mib_11ax = mac_vap_init_mib_11ax_1103,
    .tx_set_frame_htc = hmac_tx_set_frame_htc,
#endif
    .mac_mib_set_auth_rsp_time_out = wlan_chip_mac_mib_set_auth_rsp_time_out_1103,

    .mac_vap_need_set_user_htc_cap = mac_vap_need_set_user_htc_cap_1103,
#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU
    .tx_encap_large_skb_amsdu = hmac_tx_encap_large_skb_amsdu_1103, /* ���AMDPU+���AMSDU���03 05��Ч */
#endif
    .check_headroom_len = check_headroom_add_length,
    .adjust_netbuf_data = hmac_adjust_netbuf_data,
    .proc_query_station_packets = wlan_chip_proc_query_station_packets_1103,
    .scan_req_alloc_and_fill_netbuf = wlan_chip_scan_req_alloc_and_fill_netbuf_1103,
#ifdef _PRE_WLAN_FEATURE_SNIFFER
    .set_sniffer_config = hmac_config_set_sniffer_1103,
#endif
#ifdef _PRE_WLAN_FEATURE_CSI
    .dcsi_config = hmac_device_csi_config,
    .hcsi_config = NULL,
#endif
#ifdef _PRE_WLAN_FEATURE_FTM
    .dftm_config = hmac_device_ftm_config,
    .hftm_config = NULL,
    .rrm_proc_rm_request = NULL,
    .dconfig_wifi_rtt_config = hmac_device_wifi_rtt_config,
    .hconfig_wifi_rtt_config = NULL,
    .ftm_vap_init = NULL,
#endif
#ifdef _PRE_WLAN_FEATURE_DFS
    .start_zero_wait_dfs = NULL,
#endif
    .update_rxctl_data_type = hmac_rx_netbuf_update_rxctl_data_type,
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
    .mcast_ampdu_rx_ba_init = hmac_mcast_ampdu_rx_ba_init,
#endif
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU_HW
    .mcast_ampdu_sta_add_multi_user = NULL,
#endif
    .is_dbdc_ini_en = NULL,
#if defined(CONFIG_ARCH_HISI) &&  defined(CONFIG_NR_CPUS)
#if CONFIG_NR_CPUS > OAL_BUS_HPCPU_NUM
    .frw_task_bind_cpu = NULL,
#endif
#endif

    .send_connect_security_params = wlan_chip_send_connect_security_params_1103,
    .defrag_is_pn_abnormal = wlan_chip_defrag_is_pn_abnormal_1103,
    .is_support_hw_wapi = wlan_chip_is_support_hw_wapi_1103,
    .get_scaned_result_extend_info = wlan_chip_get_scaned_result_extend_info_1103,
    .get_scaned_payload_extend_len = wlan_chip_get_scaned_payload_extend_len_1103,
    .tx_pt_mcast_set_cb = wlan_chip_tx_pt_mcast_set_cb_1103,
};

const struct wlan_chip_ops g_wlan_chip_ops_1105 = {
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    .host_global_init_param = hwifi_cfg_host_global_init_param_1103,
    .first_power_on_mark = wlan_first_powon_mark_1103,
    .first_powon_cali_completed = NULL,
    .is_aput_support_160M = wlan_chip_is_aput_support_160m_1103,
    .get_flow_ctrl_used_mem = wlan_chip_get_flow_ctrl_used_mem_1103,
    .force_update_custom_params = hwifi_force_update_rf_params_1103,
    .init_nvram_main = hwifi_config_init_nvram_main_1103,
    .cpu_freq_ini_param_init = hwifi_config_cpu_freq_ini_param_1103,
    .host_global_ini_param_init = hwifi_config_host_global_ini_param_1103,
    .get_selfstudy_country_flag = wlan_chip_get_selfstudy_country_flag_1103,
    .custom_cali = wal_custom_cali_1103,
    .custom_cali_data_host_addr_init = NULL,
    .send_cali_data = wal_send_cali_data_1105,
    .custom_host_read_cfg_init = hwifi_custom_host_read_cfg_init_1103,
    .hcc_customize_h2d_data_cfg = hwifi_hcc_customize_h2d_data_cfg_1103,
    .show_customize_info = hwifi_get_cfg_params,
    .get_sar_ctrl_params = hwifi_get_sar_ctrl_params_1103,
    .get_11ax_switch_mask = wlan_chip_get_11ax_switch_mask_1103,
    .get_11ac2g_enable = wlan_chip_get_11ac2g_enable_1103,
    .get_probe_resp_mode = wlan_chip_get_probe_resp_mode_1103,
    .get_d2h_access_ddr = NULL,
#endif
    .h2d_cmd_need_filter = wlan_chip_h2d_cmd_need_filter_1103,
    .update_cfg80211_mgmt_tx_wait_time = wlan_chip_update_cfg80211_mgmt_tx_wait_time_1103,

    // �շ��;ۺ����
    .ba_rx_hdl_init = hmac_ba_rx_hdl_init,
    .check_need_setup_ba_session = wlan_chip_check_need_setup_ba_session_1103,
    .tx_update_amsdu_num = hmac_update_amsdu_num_1105, /* ע�⣺�ú���1103/1105ʵ�ֲ�ͬ */
    .check_need_process_bar = wlan_chip_check_need_process_bar_1103,
    .ba_send_reorder_timeout = hmac_ba_send_reorder_timeout,
    .ba_need_check_lut_idx = wlan_chip_ba_need_check_lut_idx_1103,
#ifdef _PRE_WLAN_FEATURE_TCP_ACK_BUFFER
    .tcp_ack_buff_config = hmac_tcp_ack_buff_config_1103,
#endif
#ifdef _PRE_WLAN_FEATURE_WMMAC
    .wmmac_need_degrade_for_ts = hmac_need_degrade_for_ts_1103,
#endif
    .update_arp_tid = NULL,
    .get_6g_flag = mac_get_rx_6g_flag_1103,
    // У׼���
    .send_cali_matrix_data = hmac_send_cali_matrix_data_1105,
    .save_cali_event = hmac_save_cali_event_1105,
    .update_cur_chn_cali_data = NULL,
    .get_chn_radio_cap = NULL,
#ifdef _PRE_WLAN_FEATURE_11AX
    .mac_vap_init_mib_11ax = mac_vap_init_mib_11ax_1105,
    .tx_set_frame_htc = hmac_tx_set_frame_htc,
#endif
    .mac_mib_set_auth_rsp_time_out = wlan_chip_mac_mib_set_auth_rsp_time_out_1103,

    .mac_vap_need_set_user_htc_cap = mac_vap_need_set_user_htc_cap_1103,
#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU
    .tx_encap_large_skb_amsdu = hmac_tx_encap_large_skb_amsdu_1103, /* ���AMDPU+���AMSDU���03 05��Ч */
#endif
    .check_headroom_len = check_headroom_add_length,
    .adjust_netbuf_data = hmac_adjust_netbuf_data,
    .proc_query_station_packets = wlan_chip_proc_query_station_packets_1103,
    .scan_req_alloc_and_fill_netbuf = wlan_chip_scan_req_alloc_and_fill_netbuf_1103,
#ifdef _PRE_WLAN_FEATURE_SNIFFER
    .set_sniffer_config = hmac_config_set_sniffer_1103,
#endif
#ifdef _PRE_WLAN_FEATURE_CSI
    .dcsi_config = hmac_device_csi_config,
    .hcsi_config = NULL,
#endif
#ifdef _PRE_WLAN_FEATURE_FTM
    .dftm_config = hmac_device_ftm_config,
    .hftm_config = NULL,
    .rrm_proc_rm_request = NULL,
    .dconfig_wifi_rtt_config = hmac_device_wifi_rtt_config,
    .hconfig_wifi_rtt_config = NULL,
    .ftm_vap_init = NULL,
#endif
#ifdef _PRE_WLAN_FEATURE_DFS
    .start_zero_wait_dfs = NULL,
#endif
    .update_rxctl_data_type = hmac_rx_netbuf_update_rxctl_data_type,
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
    .mcast_ampdu_rx_ba_init = hmac_mcast_ampdu_rx_ba_init,
#endif
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU_HW
    .mcast_ampdu_sta_add_multi_user = NULL,
#endif
    .is_dbdc_ini_en = NULL,
#if defined(CONFIG_ARCH_HISI) &&  defined(CONFIG_NR_CPUS)
#if CONFIG_NR_CPUS > OAL_BUS_HPCPU_NUM
    .frw_task_bind_cpu = NULL,
#endif
#endif

    .send_connect_security_params = wlan_chip_send_connect_security_params_1103,
    .defrag_is_pn_abnormal = wlan_chip_defrag_is_pn_abnormal_1103,
    .is_support_hw_wapi = wlan_chip_is_support_hw_wapi_1103,
    .get_scaned_result_extend_info = wlan_chip_get_scaned_result_extend_info_1103,
    .get_scaned_payload_extend_len = wlan_chip_get_scaned_payload_extend_len_1103,
    .tx_pt_mcast_set_cb = NULL,
};

