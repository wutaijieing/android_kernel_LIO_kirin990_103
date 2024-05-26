
#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP

#include "hmac_edca_opt.h"
#include "hmac_vap.h"
#include "oam_wdk.h"
#include "securec.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_EDCA_OPT_C

#define HMAC_EDCA_OPT_ADJ_STEP 2

/* (3-a)/3*X + a/3*Y */
#define WLAN_EDCA_OPT_MOD(X, Y, a) \
    (((X) * (WLAN_EDCA_OPT_MAX_WEIGHT_STA - a) + (Y) * (a)) / WLAN_EDCA_OPT_MAX_WEIGHT_STA);

OAL_STATIC oal_bool_enum_uint8 hmac_edca_opt_check_is_tcp_data(mac_ip_header_stru *pst_ip);
OAL_STATIC void hmac_edca_opt_stat_traffic_num(hmac_vap_stru *pst_hmac_vap,
    uint8_t (*ppuc_traffic_num)[WLAN_TXRX_DATA_BUTT]);


OAL_STATIC oal_bool_enum_uint8 hmac_edca_opt_check_is_tcp_data(mac_ip_header_stru *pst_ip)
{
    uint16_t ip_len;
    uint16_t ip_header_len = IP_HEADER_LEN(pst_ip->ip_header_len);
    mac_tcp_header_stru *tcp_hdr = NULL;
    uint16_t tcp_header_len;

    /* 获取ip报文长度 */
    ip_len = oal_net2host_short(pst_ip->us_tot_len);
    if (ip_len < ip_header_len + sizeof(mac_tcp_header_stru)) {
        return OAL_FALSE;
    }
    tcp_hdr = (mac_tcp_header_stru *)(((uint8_t *)pst_ip) + ip_header_len);
    /* 获取tcp header长度 */
    tcp_header_len = TCP_HEADER_LEN(tcp_hdr->uc_offset);
    if ((ip_len - ip_header_len) == tcp_header_len) {
        return OAL_FALSE;
    }
    return OAL_TRUE;
}

OAL_STATIC void  hmac_edca_opt_traffic_num_update(hmac_user_stru *hmac_user,
    uint8_t (*ppuc_traffic_num)[WLAN_TXRX_DATA_BUTT])
{
    uint8_t ac_idx;
    uint8_t data_idx;
    for (ac_idx = 0; ac_idx < WLAN_WME_AC_BUTT; ac_idx++) {
        for (data_idx = 0; data_idx < WLAN_TXRX_DATA_BUTT; data_idx++) {
            if (hmac_user->aaul_txrx_data_stat[ac_idx][data_idx] > HMAC_EDCA_OPT_PKT_NUM) {
                ppuc_traffic_num[ac_idx][data_idx]++;
            }

            /* 统计完毕置0 */
            hmac_user->aaul_txrx_data_stat[ac_idx][data_idx] = 0;
        }
    }
}

OAL_STATIC void hmac_edca_opt_stat_traffic_num(hmac_vap_stru *pst_hmac_vap,
    uint8_t (*ppuc_traffic_num)[WLAN_TXRX_DATA_BUTT])
{
    mac_user_stru *pst_user;
    hmac_user_stru *pst_hmac_user;
    mac_vap_stru *pst_vap = &(pst_hmac_vap->st_vap_base_info);
    oal_dlist_head_stru *pst_list_pos = NULL;

    pst_list_pos = pst_vap->st_mac_user_list_head.pst_next;

    for (; pst_list_pos != &(pst_vap->st_mac_user_list_head); pst_list_pos = pst_list_pos->pst_next) {
        pst_user = oal_dlist_get_entry(pst_list_pos, mac_user_stru, st_user_dlist);
        pst_hmac_user = mac_res_get_hmac_user(pst_user->us_assoc_id);
        if (pst_hmac_user == NULL) {
            oam_error_log1(pst_vap->uc_vap_id, OAM_SF_CFG,
                           "hmac_edca_opt_stat_traffic_num: pst_hmac_user[%d] is null ptr!", pst_user->us_assoc_id);
            continue;
        }

        hmac_edca_opt_traffic_num_update(pst_hmac_user, ppuc_traffic_num);
    }

    return;
}


uint32_t hmac_edca_opt_timeout_fn(void *p_arg)
{
    uint8_t aast_uc_traffic_num[WLAN_WME_AC_BUTT][WLAN_TXRX_DATA_BUTT];
    hmac_vap_stru *pst_hmac_vap = NULL;

    frw_event_mem_stru *pst_event_mem;
    frw_event_stru *pst_event;

    if (oal_unlikely(p_arg == NULL)) {
        oam_warning_log0(0, OAM_SF_ANY, "{hmac_edca_opt_timeout_fn::p_arg is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)p_arg;

    /* 计数初始化 */
    memset_s(aast_uc_traffic_num, sizeof(aast_uc_traffic_num), 0, sizeof(aast_uc_traffic_num));

    /* 统计device下所有用户上/下行 TPC/UDP条数目 */
    hmac_edca_opt_stat_traffic_num(pst_hmac_vap, aast_uc_traffic_num);

    /***************************************************************************
        抛事件到dmac模块,将统计信息报给dmac
    ***************************************************************************/
    pst_event_mem = frw_event_alloc_m(sizeof(aast_uc_traffic_num));
    if (oal_unlikely(pst_event_mem == NULL)) {
        oam_error_log0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANTI_INTF,
                       "{hmac_edca_opt_timeout_fn::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);

    /* 填写事件头 */
    frw_event_hdr_init(&(pst_event->st_event_hdr), FRW_EVENT_TYPE_WLAN_CTX, DMAC_WLAN_CTX_EVENT_SUB_TYPR_EDCA_OPT,
        sizeof(aast_uc_traffic_num), FRW_EVENT_PIPELINE_STAGE_1, pst_hmac_vap->st_vap_base_info.uc_chip_id,
        pst_hmac_vap->st_vap_base_info.uc_device_id, pst_hmac_vap->st_vap_base_info.uc_vap_id);

    /* 拷贝参数 */
    if (memcpy_s(frw_get_event_payload(pst_event_mem), sizeof(aast_uc_traffic_num),
        (uint8_t *)aast_uc_traffic_num, sizeof(aast_uc_traffic_num)) != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "hwifi_config_init_fcc_ce_txpwr_nvram::memcpy fail!");
        frw_event_free_m(pst_event_mem);
        return OAL_FAIL;
    }

    /* 分发事件 */
    frw_event_dispatch_event(pst_event_mem);
    frw_event_free_m(pst_event_mem);

    return OAL_SUCC;
}


void hmac_edca_opt_rx_pkts_stat(hmac_user_stru *hmac_user, uint8_t uc_tidno, mac_ip_header_stru *pst_ip)
{
    /* 过滤IP_LEN 小于 HMAC_EDCA_OPT_MIN_PKT_LEN的报文 */
    if (oal_net2host_short(pst_ip->us_tot_len) < HMAC_EDCA_OPT_MIN_PKT_LEN) {
        return;
    }

    if (pst_ip->uc_protocol == MAC_UDP_PROTOCAL) {
        hmac_user->aaul_txrx_data_stat[WLAN_WME_TID_TO_AC(uc_tidno)][WLAN_RX_UDP_DATA]++;
        oam_info_log4(0, OAM_SF_RX,
            "{hmac_edca_opt_rx_pkts_stat:is udp_data, assoc_id = %d, tidno = %d, type = %d, num = %d",
            hmac_user->st_user_base_info.us_assoc_id, uc_tidno, WLAN_RX_UDP_DATA,
            hmac_user->aaul_txrx_data_stat[WLAN_WME_TID_TO_AC(uc_tidno)][WLAN_RX_UDP_DATA]);
    } else if (pst_ip->uc_protocol == MAC_TCP_PROTOCAL) {
        if (hmac_edca_opt_check_is_tcp_data(pst_ip)) {
            hmac_user->aaul_txrx_data_stat[WLAN_WME_TID_TO_AC(uc_tidno)][WLAN_RX_TCP_DATA]++;
            oam_info_log4(0, OAM_SF_RX,
                "{hmac_edca_opt_rx_pkts_stat:is tcp_data, assoc_id = %d, tidno = %d, type = %d, num = %d",
                hmac_user->st_user_base_info.us_assoc_id, uc_tidno, WLAN_RX_TCP_DATA,
                hmac_user->aaul_txrx_data_stat[WLAN_WME_TID_TO_AC(uc_tidno)][WLAN_RX_TCP_DATA]);
        }
    } else {
        oam_info_log0(0, OAM_SF_RX, "{hmac_tx_pkts_stat_for_edca_opt: neither UDP nor TCP ");
    }
}


void hmac_edca_opt_tx_pkts_stat(hmac_user_stru *hmac_user, uint8_t tidno, mac_ip_header_stru *ip_hdr, uint16_t ip_len)
{
    /* 过滤IP_LEN 小于 HMAC_EDCA_OPT_MIN_PKT_LEN的报文 */
    if (ip_len < HMAC_EDCA_OPT_MIN_PKT_LEN) {
        return;
    }

    if (ip_hdr->uc_protocol == MAC_UDP_PROTOCAL) {
        hmac_user->aaul_txrx_data_stat[WLAN_WME_TID_TO_AC(tidno)][WLAN_TX_UDP_DATA]++;
        oam_info_log4(0, OAM_SF_TX,
            "{hmac_edca_opt_tx_pkts_stat:is udp_data, assoc_id = %d, tidno = %d, type = %d, num = %d",
            hmac_user->st_user_base_info.us_assoc_id, tidno, WLAN_TX_UDP_DATA,
            hmac_user->aaul_txrx_data_stat[WLAN_WME_TID_TO_AC(tidno)][WLAN_TX_UDP_DATA]);
    } else if (ip_hdr->uc_protocol == MAC_TCP_PROTOCAL) {
        if (hmac_edca_opt_check_is_tcp_data(ip_hdr)) {
            hmac_user->aaul_txrx_data_stat[WLAN_WME_TID_TO_AC(tidno)][WLAN_TX_TCP_DATA]++;
            oam_info_log4(0, OAM_SF_TX,
                "{hmac_edca_opt_tx_pkts_stat:is tcp_data, assoc_id = %d, tidno = %d, type = %d, num = %d",
                hmac_user->st_user_base_info.us_assoc_id, tidno, WLAN_TX_TCP_DATA,
                hmac_user->aaul_txrx_data_stat[WLAN_WME_TID_TO_AC(tidno)][WLAN_TX_TCP_DATA]);
        }
    } else {
        oam_info_log0(0, OAM_SF_TX, "{hmac_edca_opt_tx_pkts_stat: neither UDP nor TCP");
    }
}

#endif /* end of _PRE_WLAN_FEATURE_EDCA_OPT_AP */