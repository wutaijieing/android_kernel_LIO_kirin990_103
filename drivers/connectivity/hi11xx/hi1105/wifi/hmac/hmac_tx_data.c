
#include "hmac_vap.h"
#include "oal_net.h"
#include "frw_ext_if.h"
#include "hmac_tx_data.h"
#include "oam_stat_wifi.h"
#include "hmac_tx_amsdu.h"
#include "mac_frame.h"
#include "mac_data.h"
#include "hmac_frag.h"
#include "hmac_11i.h"
#include "wlan_chip.h"
#include "hmac_hid2d.h"
#include "hmac_wapi.h"
#include "hmac_edca_opt.h"
#include "hmac_traffic_classify.h"
#include "hmac_user.h"
#include "hmac_device.h"
#include "hmac_resource.h"
#include "hmac_stat.h"
#include "hmac_tcp_opt.h"
// ��ͷ�ļ�UT��Ҫ����ʹ�ã�����ɾ��
#include "hisi_customize_wifi.h"
#include "hmac_wifi_delay.h"
#include "hmac_auto_adjust_freq.h"  // Ϊ����ͳ������׼��

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "plat_pm_wlan.h"
#endif

#ifdef _PRE_WLAN_PKT_TIME_STAT
#include <hwnet/ipv4/wifi_delayst.h>
#endif

#ifdef _PRE_WLAN_FEATURE_SNIFFER
#ifdef CONFIG_HW_SNIFFER
#include <hwnet/ipv4/sysctl_sniffer.h>
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_HIEX
#include "hmac_hiex.h"
#include "hmac_tx_msdu_dscr.h"
#endif

#ifdef _PRE_WLAN_RR_PERFORMENCE_DEBUG
#include "hmac_rr_performance.h"
#endif

#include "hmac_host_tx_data.h"
#include "hmac_ht_self_cure.h"

#include "securec.h"
#ifdef _PRE_WLAN_FEATURE_TCP_ACK_BUFFER
#include "hmac_tcp_ack_buf.h"
#endif
#if defined(_PRE_WLAN_FEATURE_WMMAC)
#include "hmac_wmmac.h"
#endif
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
#include "hmac_mcast_ampdu.h"
#endif
#ifdef _PRE_WLAN_FEATURE_VSP
#include "hmac_vsp.h"
#endif
#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_TX_DATA_C
/*
 * definitions of king of games feature
 */
#ifdef CONFIG_NF_CONNTRACK_MARK
#define VIP_APP_VIQUE_TID WLAN_TIDNO_VIDEO
#define VIP_APP_MARK      0x5a
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
#define pktmark(p)       (((struct sk_buff *)(p))->mark)
#define pktsetmark(p, m) ((struct sk_buff *)(p))->mark = (m)
#else /* !2.6.0 */
#define pktmark(p)       (((struct sk_buff *)(p))->nfmark)
#define pktsetmark(p, m) ((struct sk_buff *)(p))->nfmark = (m)
#endif /* 2.6.0 */
#else  /* CONFIG_NF_CONNTRACK_MARK */
#define pktmark(p) 0
#define pktsetmark(p, m)
#endif /* CONFIG_NF_CONNTRACK_MARK */

#define MAC_KEY_FRAME_SOFT_RETRY_CNT 5

/* 2 ȫ�ֱ������� */
OAL_STATIC uint8_t g_uc_ac_new = 0;

#ifdef _PRE_WLAN_FEATURE_TCP_ACK_BUFFER
OAL_STATIC void hmac_tcp_ack_buf_switch_judge(mac_vap_stru *pst_mac_vap,
                                              uint32_t limit_throughput_high,
                                              uint32_t limit_throughput_low,
                                              uint32_t rx_throughput_mbps,
                                              uint8_t  uc_vap_idx);
OAL_STATIC void hmac_tcp_ack_buf_set_throughput_threshould(uint32_t high_throughput_level,
                                                           uint32_t *ul_limit_throughput_high,
                                                           uint32_t *ul_limit_throughput_low,
                                                           oal_bool_enum_uint8  en_mu_vap);
#endif

#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
hmac_tx_ampdu_hw_stru g_st_ampdu_hw = { 0 };
#endif

wlan_trx_switch_stru g_trx_switch = { 0 };

/* 3 ����ʵ�� */

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 hmac_tx_is_dhcp(mac_ether_header_stru *pst_ether_hdr)
{
    mac_ip_header_stru *puc_ip_hdr;

    puc_ip_hdr = (mac_ip_header_stru *)(pst_ether_hdr + 1);

    return mac_is_dhcp_port(puc_ip_hdr, oal_net2host_short(puc_ip_hdr->us_tot_len));
}


OAL_STATIC void hmac_tx_report_dhcp_and_arp(mac_vap_stru *pst_mac_vap,
    mac_ether_header_stru *pst_ether_hdr, uint16_t us_ether_len)
{
    oal_bool_enum_uint8 en_flg;

    switch (oal_host2net_short(pst_ether_hdr->us_ether_type)) {
        case ETHER_TYPE_ARP:
            en_flg = OAL_TRUE;
            break;

        case ETHER_TYPE_IP:
            en_flg = hmac_tx_is_dhcp(pst_ether_hdr);
            break;

        default:
            en_flg = OAL_FALSE;
            break;
    }

    if (en_flg && oam_report_dhcp_arp_get_switch()) {
        if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
            oam_report_eth_frame(pst_ether_hdr->auc_ether_dhost, (uint8_t *)pst_ether_hdr, us_ether_len,
                OAM_OTA_FRAME_DIRECTION_TYPE_TX);
        } else if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
            oam_report_eth_frame(pst_mac_vap->auc_bssid, (uint8_t *)pst_ether_hdr, us_ether_len,
                OAM_OTA_FRAME_DIRECTION_TYPE_TX);
        } else {
        }
    }
}


void hmac_tx_report_eth_frame(hmac_vap_stru *pst_hmac_vap, oal_netbuf_stru *pst_netbuf)
{
    uint16_t us_user_idx = 0;
    mac_ether_header_stru *pst_ether_hdr = NULL;
    uint32_t ret;
    uint8_t auc_user_macaddr[WLAN_MAC_ADDR_LEN];
    oal_switch_enum_uint8 en_eth_switch = oam_ota_get_switch(OAM_OTA_SWITCH_TX_ETH_FRAME);
    mac_vap_stru *pst_mac_vap = &pst_hmac_vap->st_vap_base_info;

    /* ͳ����̫�����������ݰ�ͳ�� */
    hmac_stat_vap_tx_netbuf(pst_hmac_vap, pst_netbuf);

    if (oal_likely(en_eth_switch != OAL_SWITCH_ON)) {
        return;
    }
    /* ��ȡĿ���û���Դ��id���û�MAC��ַ�����ڹ��� */
    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
        pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_netbuf);
        if (oal_unlikely(pst_ether_hdr == NULL)) {
            oam_error_log0(0, OAM_SF_TX, "{hmac_tx_report_eth_frame::ether_hdr is null!\r\n");
            return;
        }

        ret = mac_vap_find_user_by_macaddr(pst_mac_vap, pst_ether_hdr->auc_ether_dhost, &us_user_idx);
        if (ret == OAL_ERR_CODE_PTR_NULL) {
            oam_error_log1(0, OAM_SF_TX, "{hmac_tx_report_eth_frame::find user return ptr null!!\r\n", ret);
            return;
        }

        if (ret == OAL_FAIL) {
            /* ����Ҳ����û�����֡������dhcp����arp request����Ҫ�ϱ� */
            hmac_tx_report_dhcp_and_arp(pst_mac_vap, pst_ether_hdr, (uint16_t)oal_netbuf_len(pst_netbuf));
            return;
        }

        oal_set_mac_addr(&auc_user_macaddr[0], pst_ether_hdr->auc_ether_dhost);
    } else if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
        if (pst_mac_vap->us_user_nums == 0) {
            return;
        }
        pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_netbuf);
        if (oal_unlikely(pst_ether_hdr == NULL)) {
            oam_error_log0(0, OAM_SF_TX, "{hmac_tx_report_eth_frame::ether_hdr is null!\r\n");
            return;
        }
        /* ����Ҳ����û�����֡������dhcp����arp request����Ҫ�ϱ� */
        hmac_tx_report_dhcp_and_arp(pst_mac_vap, pst_ether_hdr, (uint16_t)oal_netbuf_len(pst_netbuf));
        us_user_idx = pst_mac_vap->us_assoc_vap_id;
        oal_set_mac_addr(&auc_user_macaddr[0], pst_mac_vap->auc_bssid);
    }

    /* ����̫��������֡�ϱ� */
    ret = oam_report_eth_frame(&auc_user_macaddr[0], oal_netbuf_data(pst_netbuf), (uint16_t)oal_netbuf_len(pst_netbuf),
        OAM_OTA_FRAME_DIRECTION_TYPE_TX);
    if (ret != OAL_SUCC) {
        oam_warning_log1(0, OAM_SF_TX, "{hmac_tx_report_eth_frame::oam_report_eth_frame return err: 0x%x.}\r\n", ret);
    }
}


uint16_t hmac_free_netbuf_list(oal_netbuf_stru *pst_buf)
{
    oal_netbuf_stru *pst_buf_tmp = NULL;
    mac_tx_ctl_stru *pst_tx_cb = NULL;
    uint16_t us_buf_num = 0;

    if (pst_buf != NULL) {
        pst_tx_cb = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_buf);

        while (pst_buf != NULL) {
            pst_buf_tmp = oal_netbuf_list_next(pst_buf);
            us_buf_num++;

            pst_tx_cb = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_buf);
            
            if ((oal_netbuf_headroom(pst_buf) < MAC_80211_QOS_HTC_4ADDR_FRAME_LEN) &&
                (MAC_GET_CB_FRAME_HEADER_ADDR(pst_tx_cb) != NULL)) {
                oal_mem_free_m(MAC_GET_CB_FRAME_HEADER_ADDR(pst_tx_cb), OAL_TRUE);
                MAC_GET_CB_FRAME_HEADER_ADDR(pst_tx_cb) = NULL;
            }

            oal_netbuf_free(pst_buf);

            pst_buf = pst_buf_tmp;
        }
    } else {
        oam_error_log0(0, OAM_SF_ANY, "{hmac_free_netbuf_list::pst_buf is null}");
    }

    return us_buf_num;
}

#ifdef _PRE_WLAN_FEATURE_HS20

static void hmac_tx_set_qos_map(
    hmac_vap_stru *hmac_vap, mac_ip_header_stru *ip_header, mac_tx_ctl_stru *pst_tx_ctl, uint8_t *tid)
{
    uint8_t uc_dscp;
    uint8_t uc_idx;

    /* ��IP TOS�ֶ�Ѱ��DSCP���ȼ� */
    /* ---------------------------------
      tosλ����
      ---------------------------------
    |    bit7~bit2      | bit1~bit0 |
    |    DSCP���ȼ�     | ����      |
    --------------------------------- */
    uc_dscp = ip_header->uc_tos >> WLAN_DSCP_PRI_SHIFT;
    if ((hmac_vap->st_cfg_qos_map_param.uc_num_dscp_except > 0) &&
        (hmac_vap->st_cfg_qos_map_param.uc_num_dscp_except <= MAX_DSCP_EXCEPT) &&
        (hmac_vap->st_cfg_qos_map_param.uc_valid)) {
        for (uc_idx = 0; uc_idx < hmac_vap->st_cfg_qos_map_param.uc_num_dscp_except; uc_idx++) {
            if (uc_dscp == hmac_vap->st_cfg_qos_map_param.auc_dscp_exception[uc_idx]) {
                *tid = hmac_vap->st_cfg_qos_map_param.auc_dscp_exception_up[uc_idx];
                MAC_GET_CB_FRAME_TYPE(pst_tx_ctl) = WLAN_CB_FRAME_TYPE_DATA;
                MAC_GET_CB_FRAME_SUBTYPE(pst_tx_ctl) = MAC_DATA_HS20;
                MAC_GET_CB_IS_NEEDRETRY(pst_tx_ctl) = OAL_TRUE;
                hmac_vap->st_cfg_qos_map_param.uc_valid = 0;
                return;
            }
        }
    }

    for (uc_idx = 0; uc_idx < MAX_QOS_UP_RANGE; uc_idx++) {
        if ((uc_dscp < hmac_vap->st_cfg_qos_map_param.auc_up_high[uc_idx]) &&
            (uc_dscp > hmac_vap->st_cfg_qos_map_param.auc_up_low[uc_idx])) {
            *tid = uc_idx;
            MAC_GET_CB_FRAME_TYPE(pst_tx_ctl) = WLAN_CB_FRAME_TYPE_DATA;
            MAC_GET_CB_FRAME_SUBTYPE(pst_tx_ctl) = MAC_DATA_HS20;
            MAC_GET_CB_IS_NEEDRETRY(pst_tx_ctl) = OAL_TRUE;
            hmac_vap->st_cfg_qos_map_param.uc_valid = 0;
            return;
        } else {
            *tid = 0;
        }
    }
    pst_hmac_vap->st_cfg_qos_map_param.uc_valid = 0;
}
#endif  // _PRE_WLAN_FEATURE_HS20


OAL_STATIC uint32_t hmac_set_tid_based_pktmark(oal_netbuf_stru *pst_buf, uint8_t *puc_tid,
    mac_tx_ctl_stru *pst_tx_ctl)
{
#ifdef CONFIG_NF_CONNTRACK_MARK
    /*
     * the king of game feature will mark packets
     * and we will use VI queue to send these packets.
     */
    if (pktmark(pst_buf) == VIP_APP_MARK) {
        *puc_tid = VIP_APP_VIQUE_TID;
        pst_tx_ctl->bit_is_needretry = OAL_TRUE;
        return OAL_TRUE;
    }
#endif
    return OAL_FALSE;
}
uint8_t hmac_tx_classify_tcp(hmac_vap_stru *hmac_vap, mac_ip_header_stru *ip_header, uint16_t ip_len,
    mac_tx_ctl_stru *tx_ctl, uint8_t tid_num)
{
    uint8_t curr_tid = tid_num;
    uint16_t ip_header_len = IP_HEADER_LEN(ip_header->ip_header_len);
    mac_tcp_header_stru *tcp_hdr = NULL;
    if (ip_len < ip_header_len + sizeof(mac_tcp_header_stru)) {
        oam_error_log1(0, OAM_SF_ANY, "hmac_tx_classify_tcp:ip_len=%d", ip_len);
        return curr_tid;
    }
    tcp_hdr = (mac_tcp_header_stru *)(((uint8_t *)ip_header) + ip_header_len);
#ifdef _PRE_WLAN_FEATURE_SCHEDULE
    if (oal_value_eq_any2(MAC_CHARIOT_NETIF_PORT, tcp_hdr->us_dport, tcp_hdr->us_sport)) {
        curr_tid = WLAN_TIDNO_BCAST;
        MAC_GET_CB_FRAME_TYPE(tx_ctl) = WLAN_CB_FRAME_TYPE_DATA;
        MAC_GET_CB_FRAME_SUBTYPE(tx_ctl) = MAC_DATA_CHARIOT_SIG;
        MAC_GET_CB_IS_NEEDRETRY(tx_ctl) = OAL_TRUE;
        return curr_tid;
    }
#endif
    if (oal_netbuf_is_tcp_ack((oal_ip_header_stru *)ip_header) == OAL_TRUE) {
        /* option3:SYN FIN RST URG��Ϊ1��ʱ�򲻻��� */
        if ((tcp_hdr->uc_flags) & (FIN_FLAG_BIT | RESET_FLAG_BIT | URGENT_FLAG_BIT)) {
            MAC_GET_CB_FRAME_SUBTYPE(tx_ctl) = MAC_DATA_URGENT_TCP_ACK;
        } else if ((tcp_hdr->uc_flags) & SYN_FLAG_BIT) {
            MAC_GET_CB_IS_NEEDRETRY(tx_ctl) = OAL_TRUE;
            MAC_GET_CB_FRAME_SUBTYPE(tx_ctl) = MAC_DATA_TCP_SYN;
            MAC_GET_CB_RETRIED_NUM(tx_ctl) = MAC_KEY_FRAME_SOFT_RETRY_CNT;
        } else {
            MAC_GET_CB_FRAME_SUBTYPE(tx_ctl) = MAC_DATA_NORMAL_TCP_ACK;
        }
    } else if ((!IS_LEGACY_VAP(&(hmac_vap->st_vap_base_info))) &&
       (oal_ntoh_16(tcp_hdr->us_sport) == MAC_WFD_RTSP_PORT)) {
        MAC_GET_CB_FRAME_TYPE(tx_ctl) = WLAN_DATA_BASICTYPE;
        MAC_GET_CB_IS_NEEDRETRY(tx_ctl) = OAL_TRUE;
        MAC_GET_CB_FRAME_SUBTYPE(tx_ctl) = MAC_DATA_RTSP;
        curr_tid = WLAN_TIDNO_BCAST;
    } else {
        MAC_GET_CB_FRAME_SUBTYPE(tx_ctl) = MAC_DATA_BUTT;
    }

    return curr_tid;
}
#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
void hmac_adjust_edca_opt_tx_ptks(hmac_user_stru *hmac_user,
    uint8_t curr_tid, mac_ip_header_stru *ip_header, uint16_t ip_len)
{
    if (((ip_header->uc_protocol == MAC_UDP_PROTOCAL) &&
        (hmac_user->aaul_txrx_data_stat[WLAN_WME_TID_TO_AC(curr_tid)][WLAN_TX_UDP_DATA] <
        (HMAC_EDCA_OPT_PKT_NUM + BYTE_OFFSET_10))) ||
        ((ip_header->uc_protocol == MAC_TCP_PROTOCAL) &&
        (hmac_user->aaul_txrx_data_stat[WLAN_WME_TID_TO_AC(curr_tid)][WLAN_TX_TCP_DATA] <
        (HMAC_EDCA_OPT_PKT_NUM + BYTE_OFFSET_10)))) {
        hmac_edca_opt_tx_pkts_stat(hmac_user, curr_tid, ip_header, ip_len);
    }
}
#endif
OAL_STATIC void hmac_tx_data_set_dhcp_cb(mac_tx_ctl_stru *tx_ctl)
{
    MAC_GET_CB_FRAME_TYPE(tx_ctl) = WLAN_CB_FRAME_TYPE_DATA;
    MAC_GET_CB_FRAME_SUBTYPE(tx_ctl) = MAC_DATA_DHCP;
    MAC_GET_CB_IS_NEEDRETRY(tx_ctl) = OAL_TRUE;
}
OAL_STATIC void hmac_tx_data_set_dns_cb(mac_tx_ctl_stru *tx_ctl)
{
    MAC_GET_CB_FRAME_TYPE(tx_ctl) = WLAN_CB_FRAME_TYPE_DATA;
    MAC_GET_CB_FRAME_SUBTYPE(tx_ctl) = MAC_DATA_DNS;
    MAC_GET_CB_IS_NEEDRETRY(tx_ctl) = OAL_TRUE;
    MAC_GET_CB_RETRIED_NUM(tx_ctl) = MAC_KEY_FRAME_SOFT_RETRY_CNT;
}
/*lint -e778*/
static uint8_t hmac_tx_classify_lan_to_wlan_ip_handle(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user,
    mac_ether_header_stru *ether_header, mac_tx_ctl_stru *tx_ctl, uint16_t netbuf_len)
{
    /* ��IP TOS�ֶ�Ѱ�����ȼ� */
    /* ----------------------------------------------------------------------
                tosλ����
       ----------------------------------------------------------------------
            | bit7~bit5 | bit4 |  bit3  |  bit2  |   bit1   | bit0 |
            | �����ȼ�  | ʱ�� | ������ | �ɿ��� | ����ɱ� | ���� |
       ---------------------------------------------------------------------- */
    mac_ip_header_stru *ip_header = NULL;
    uint8_t curr_tid = 0;
    uint16_t remain_len = netbuf_len - sizeof(mac_ether_header_stru); /* �ϲ㺯����֤�������ᷭת */
    uint16_t ip_len;
    if (remain_len < sizeof(mac_ip_header_stru)) {
        oam_error_log1(0, OAM_SF_ANY, "hmac_tx_classify_lan_to_wlan_ip_handle:netbuf_len=%d", netbuf_len);
        return curr_tid;
    }
    ip_header = (mac_ip_header_stru *)(ether_header + 1); /* ƫ��һ����̫��ͷ��ȡipͷ */
    ip_len = oal_net2host_short(ip_header->us_tot_len);
    if (remain_len < ip_len) {
        oam_error_log2(0, OAM_SF_ANY, "hmac_tx_classify_lan_to_wlan_ip_handle:remain_len=%d, ip_len=%d",
            remain_len, ip_len);
        return curr_tid;
    }
#ifdef _PRE_WLAN_FEATURE_HS20
    if (hmac_vap->st_cfg_qos_map_param.uc_valid) {
        hmac_tx_set_qos_map(hmac_vap, ip_header, tx_ctl, &curr_tid);
        return curr_tid;
    }
#endif  // _PRE_WLAN_FEATURE_HS20
    if ((ip_header->uc_tos >> WLAN_DSCP_PRI_SHIFT) >= HMAC_MAX_DSCP_VALUE_NUM) {
        return curr_tid;
    }
    curr_tid = ip_header->uc_tos >> WLAN_IP_PRI_SHIFT;
    if (OAL_SWITCH_ON == mac_mib_get_TxTrafficClassifyFlag(&hmac_vap->st_vap_base_info)) {
        if (curr_tid != 0) {
            return curr_tid;
        }
        /* RTP RTSP ����ֻ�� P2P�ϲſ���ʶ���� */
        if (!IS_LEGACY_VAP(&(hmac_vap->st_vap_base_info))) {
            hmac_tx_traffic_classify(hmac_user, tx_ctl, ip_header, ip_len, &curr_tid);
        }
    }
    /* �����DHCP֡�������VO���з��� */
    if (mac_is_dhcp_port(ip_header, ip_len) == OAL_TRUE) {
        curr_tid = WLAN_TIDNO_BCAST;
        hmac_tx_data_set_dhcp_cb(tx_ctl);
    } else if (mac_is_dns(ip_header, ip_len) == OAL_TRUE) {
        hmac_tx_data_set_dns_cb(tx_ctl);
    } else if (ip_header->uc_protocol == MAC_TCP_PROTOCAL) { /* ����chariot����Ľ������⴦����ֹ���� */
        curr_tid = hmac_tx_classify_tcp(hmac_vap, ip_header, ip_len, tx_ctl, curr_tid);
    }
#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
    if ((hmac_vap->uc_edca_opt_flag_ap == OAL_TRUE) &&
        (hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_AP)) {
        /* mips�Ż�:�������ҵ��ͳ�����ܲ�10M���� */
        hmac_adjust_edca_opt_tx_ptks(hmac_user, curr_tid, ip_header, ip_len);
    }
#endif
    return curr_tid;
}

static uint8_t hmac_tx_classify_lan_to_wlan_ipv6_handle(mac_ether_header_stru *ether_header, mac_tx_ctl_stru *tx_ctl)
{
    /* ��IPv6 traffic class�ֶλ�ȡ���ȼ� */
    /* ----------------------------------------------------------------------
        IPv6��ͷ ǰ32Ϊ����
     -----------------------------------------------------------------------
    | �汾�� | traffic class   | ������ʶ |
    | 4bit   | 8bit(ͬipv4 tos)|  20bit   |
    ----------------------------------------------------------------------- */
    uint32_t ipv6_hdr = *((uint32_t *)(ether_header + 1)); /* ƫ��һ����̫��ͷ��ȡipͷ */
    uint32_t ipv6_pri = (oal_net2host_long(ipv6_hdr) & WLAN_IPV6_PRIORITY_MASK) >> WLAN_IPV6_PRIORITY_SHIFT;
    uint8_t curr_tid = (uint8_t)(ipv6_pri >> WLAN_IP_PRI_SHIFT);
    /* �����DHCPV6֡�������VO���з��� */
    oal_ipv6hdr_stru *ipv6_header = (oal_ipv6hdr_stru *)(ether_header + 1); /* ƫ��һ����̫��ͷ��ȡipͷ */
    uint32_t payload_len = oal_net2host_short(ipv6_header->payload_len);
    if (mac_is_dhcp6((oal_ipv6hdr_stru *)(ether_header + 1), payload_len) == OAL_TRUE) {
        curr_tid = WLAN_TIDNO_BCAST;
        MAC_GET_CB_FRAME_TYPE(tx_ctl) = WLAN_CB_FRAME_TYPE_DATA;
        MAC_GET_CB_FRAME_SUBTYPE(tx_ctl) = MAC_DATA_DHCPV6;
        MAC_GET_CB_IS_NEEDRETRY(tx_ctl) = OAL_TRUE;
    }
    return curr_tid;
}

static uint8_t hmac_tx_classify_lan_to_wlan_pae_handle(hmac_vap_stru *hmac_vap,
    mac_ether_header_stru *ether_header, mac_tx_ctl_stru *tx_ctl)
{
    /* �����EAPOL֡�������VO���з��� */
    uint8_t curr_tid = WLAN_TIDNO_BCAST;
    MAC_GET_CB_FRAME_TYPE(tx_ctl) = WLAN_CB_FRAME_TYPE_DATA;
    MAC_GET_CB_FRAME_SUBTYPE(tx_ctl) = MAC_DATA_EAPOL;
    MAC_GET_CB_IS_NEEDRETRY(tx_ctl) = OAL_TRUE;
    /* �����4 ���������õ�����Կ��������tx cb ��bit_is_eapol_key_ptk ��һ��dmac ���Ͳ����� */
    if (OAL_TRUE == mac_is_eapol_key_ptk((mac_eapol_header_stru *)(ether_header + 1))) {
        MAC_GET_CB_IS_EAPOL_KEY_PTK(tx_ctl) = OAL_TRUE;
    }
    oam_warning_log2(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CONN,
                     "{hmac_tx_classify_lan_to_wlan:: EAPOL tx : uc_tid=%d,IS_EAPOL_KEY_PTK=%d.}",
                     curr_tid, MAC_GET_CB_IS_EAPOL_KEY_PTK(tx_ctl));
    return curr_tid;
}

static uint8_t hmac_tx_classify_lan_to_wlan_pppoe_handle(mac_tx_ctl_stru *tx_ctl)
{
    uint8_t curr_tid = WLAN_TIDNO_BCAST;
    MAC_GET_CB_FRAME_TYPE(tx_ctl) = WLAN_CB_FRAME_TYPE_DATA;
    MAC_GET_CB_FRAME_SUBTYPE(tx_ctl) = MAC_DATA_PPPOE;
    MAC_GET_CB_IS_NEEDRETRY(tx_ctl) = OAL_TRUE;
    return curr_tid;
}

static uint8_t hmac_tx_classify_lan_to_wlan_wapi_handle(mac_tx_ctl_stru *tx_ctl)
{
    uint8_t curr_tid = WLAN_TIDNO_BCAST;
    MAC_GET_CB_FRAME_TYPE(tx_ctl) = WLAN_CB_FRAME_TYPE_DATA;
    MAC_GET_CB_FRAME_SUBTYPE(tx_ctl) = MAC_DATA_WAPI;
    MAC_GET_CB_IS_NEEDRETRY(tx_ctl) = OAL_TRUE;
    return curr_tid;
}

static uint8_t hmac_tx_classify_lan_to_wlan_eth_type_handle(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user,
    oal_netbuf_stru *netbuf, mac_tx_ctl_stru *tx_ctl)
{
    mac_ether_header_stru *ether_header = (mac_ether_header_stru *)oal_netbuf_data(netbuf);
    oal_vlan_ethhdr_stru *vlan_ethhdr = NULL;
    uint16_t netbuf_len = oal_netbuf_get_len(netbuf);
    uint16_t vlan_tci;
    uint8_t curr_tid = 0;

    if (netbuf_len < sizeof(mac_ether_header_stru)) {
        oam_error_log1(0, OAM_SF_ANY, "hmac_tx_classify_lan_to_wlan_eth_type_handle:netbuf_len=%d", netbuf_len);
        return curr_tid;
    }
    /* ��ȡtid ����data type���и�ֵ���� */
    MAC_GET_CB_FRAME_SUBTYPE(tx_ctl) = MAC_DATA_NUM;
    switch (ether_header->us_ether_type) {
        /*lint -e778*/ /* ����Info-- Constant expression evaluates to 0 in operation '&' */
        case oal_host2net_short(ETHER_TYPE_IP):
            curr_tid = hmac_tx_classify_lan_to_wlan_ip_handle(hmac_vap, hmac_user, ether_header, tx_ctl, netbuf_len);
            break;
        case oal_host2net_short(ETHER_TYPE_IPV6):
            curr_tid = hmac_tx_classify_lan_to_wlan_ipv6_handle(ether_header, tx_ctl);
            break;
        case oal_host2net_short(ETHER_TYPE_PAE):
            curr_tid = hmac_tx_classify_lan_to_wlan_pae_handle(hmac_vap, ether_header, tx_ctl);
            break;
        case oal_host2net_short(ETHER_TYPE_ARP):
        /* �����ARP֡�������VO���з��� */
            wlan_chip_update_arp_tid(&curr_tid);
            MAC_GET_CB_FRAME_SUBTYPE(tx_ctl) = mac_get_arp_type_by_arphdr((oal_eth_arphdr_stru *)(ether_header + 1),
                netbuf_len - sizeof(mac_ether_header_stru));
            MAC_GET_CB_IS_NEEDRETRY(tx_ctl) = OAL_TRUE;
            break;
        /* TDLS֡��������������������ȼ�TID���� */
        case oal_host2net_short(ETHER_TYPE_TDLS):
            curr_tid = WLAN_TIDNO_BCAST;
            MAC_GET_CB_FRAME_SUBTYPE(tx_ctl) = MAC_DATA_TDLS;
            break;
        /* PPPOE֡������������(���ֽ׶�, �Ự�׶�)��������ȼ�TID���� */
        case oal_host2net_short(ETHER_TYPE_PPP_DISC):
        case oal_host2net_short(ETHER_TYPE_PPP_SES):
            curr_tid = hmac_tx_classify_lan_to_wlan_pppoe_handle(tx_ctl);
            break;
#ifdef _PRE_WLAN_FEATURE_WAPI
        case oal_host2net_short(ETHER_TYPE_WAI):
            curr_tid = hmac_tx_classify_lan_to_wlan_wapi_handle(tx_ctl);
            break;
#endif
        case oal_host2net_short(ETHER_TYPE_VLAN):
            /* ��ȡvlan tag�����ȼ� */
            vlan_ethhdr = (oal_vlan_ethhdr_stru *)oal_netbuf_data(netbuf);
            /* ------------------------------------------------------------------
                802.1Q(VLAN) TCI(tag control information)λ����
             -------------------------------------------------------------------
            |Priority | DEI  | Vlan Identifier |
            | 3bit    | 1bit |      12bit      |
             ------------------------------------------------------------------ */
            vlan_tci = oal_net2host_short(vlan_ethhdr->h_vlan_TCI);
            curr_tid = vlan_tci >> OAL_VLAN_PRIO_SHIFT; /* ����13λ����ȡ��3λ���ȼ� */
            break;
        /*lint +e778*/
        default:
            break;
    }
    return curr_tid;
}
/*lint +e778*/

void hmac_tx_classify_lan_to_wlan(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user,
    oal_netbuf_stru *netbuf, uint8_t *tid_num)
{
    mac_tx_ctl_stru *tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(netbuf);
    uint8_t curr_tid;

    if (hmac_set_tid_based_pktmark(netbuf, tid_num, tx_ctl) == OAL_TRUE) {
        return;
    }
    curr_tid = hmac_tx_classify_lan_to_wlan_eth_type_handle(hmac_vap, hmac_user, netbuf, tx_ctl);

    /* ���θ�ֵ */
    *tid_num = curr_tid;
}


OAL_STATIC OAL_INLINE void hmac_tx_update_tid(oal_bool_enum_uint8 en_wmm, uint8_t *puc_tid)
{
    if (oal_likely(en_wmm == OAL_TRUE)) { /* wmmʹ�� */
        *puc_tid = (*puc_tid < WLAN_TIDNO_BUTT) ? wlan_tos_to_tid(*puc_tid) : WLAN_TIDNO_BCAST;
    } else { /* wmm��ʹ�� */
        *puc_tid = MAC_WMM_SWITCH_TID;
    }
}

uint8_t hmac_tx_wmm_acm(oal_bool_enum_uint8 en_wmm, hmac_vap_stru *pst_hmac_vap, uint8_t *puc_tid)
{
    uint8_t uc_ac;

    if (oal_any_null_ptr2(pst_hmac_vap, puc_tid)) {
        return OAL_FALSE;
    }

    if (en_wmm == OAL_FALSE) {
        return OAL_FALSE;
    }

    uc_ac = WLAN_WME_TID_TO_AC(*puc_tid);
    g_uc_ac_new = uc_ac;
    while ((g_uc_ac_new != WLAN_WME_AC_BK) &&
           (OAL_TRUE == mac_mib_get_QAPEDCATableMandatory(&pst_hmac_vap->st_vap_base_info, g_uc_ac_new))) {
        switch (g_uc_ac_new) {
            case WLAN_WME_AC_VO:
                g_uc_ac_new = WLAN_WME_AC_VI;
                break;

            case WLAN_WME_AC_VI:
                g_uc_ac_new = WLAN_WME_AC_BE;
                break;

            default:
                g_uc_ac_new = WLAN_WME_AC_BK;
                break;
        }
    }

    if (g_uc_ac_new != uc_ac) {
        *puc_tid = WLAN_WME_AC_TO_TID(g_uc_ac_new);
    }

    return OAL_TRUE;
}
#ifdef _PRE_WLAN_FEATURE_VSP
static inline void hmac_tx_classify_vsp_change_tidno(hmac_user_stru *hmac_user, uint8_t *tidno)
{
    if (hmac_is_vsp_tid(hmac_user, *tidno)) {
        *tidno = WLAN_TIDNO_VIDEO;
    }
}
#endif
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
/* ����鲥�ۺϴ򿪣����鲥������Ҫ��tid3, Ϊ��reorder��������be֡��tid0 */
OAL_INLINE void hmac_tx_classify_mcast_ampdu_remove_tidno(hmac_vap_stru *hmac_vap, uint8_t *tidno)
{
    if (mac_get_mcast_ampdu_switch() == OAL_TRUE && (hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_AP) &&
        (*tidno == WLAN_TIDNO_ANT_TRAINING_LOW_PRIO)) {
        *tidno = WLAN_TIDNO_BEST_EFFORT;
    }
}
#endif
OAL_STATIC OAL_INLINE void hmac_tx_classify_get_tid(mac_device_stru *mac_dev,
    hmac_vap_stru *hmac_vap, mac_tx_ctl_stru *tx_ctl, uint8_t *tidno)
{
    /* ���ʹ����vap���ȼ�����������õ�vap���ȼ� */
    if (mac_dev->en_vap_classify == OAL_TRUE) {
        *tidno = mac_mib_get_VAPClassifyTidNo(&hmac_vap->st_vap_base_info);
    }

    /* ���ap�ر���WMM�������б�����BE ���� */
    if (!hmac_vap->st_vap_base_info.en_vap_wmm) {
        *tidno = WLAN_TIDNO_BEST_EFFORT;
    }

    /* tid7��VIP֡ʹ�ã�������VO�ۺϣ���ͨ����֡tid��Ϊtid6 */
    if ((*tidno == WLAN_TIDNO_BCAST) && (!MAC_GET_CB_IS_VIPFRAME(tx_ctl))) {
        *tidno = WLAN_TIDNO_VOICE;
    }
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
    hmac_tx_classify_mcast_ampdu_remove_tidno(hmac_vap, tidno);
#endif
}

OAL_STATIC void hmac_tx_wmmac_classify(hmac_vap_stru *pst_hmac_vap, mac_tx_ctl_stru *tx_ctl,
    mac_device_stru *pst_mac_dev, mac_user_stru *pst_user, uint8_t *tid_num)
{
#if defined(_PRE_WLAN_FEATURE_WMMAC)
    if (g_en_wmmac_switch) {
        uint8_t uc_ac_num;
        uc_ac_num = WLAN_WME_TID_TO_AC(*tid_num);
        /* ���ACMλΪ1���Ҷ�ӦAC��TSû�н����ɹ����򽫸�AC������ȫ���ŵ�BE���з��� */
        if (hmac_need_degrade_for_wmmac(pst_hmac_vap, tx_ctl, pst_user, uc_ac_num, *tid_num) == OAL_TRUE) {
            *tid_num = WLAN_TIDNO_BEST_EFFORT;
        }
    } else {
#endif  // defined(_PRE_WLAN_FEATURE_WMMAC)
        hmac_tx_wmm_acm(pst_mac_dev->en_wmm, pst_hmac_vap, tid_num);

        
        /* 1102��������ֻʹ����4��tid:0 1 5 6 */
        if ((!MAC_GET_CB_IS_VIPFRAME(tx_ctl)) || (pst_mac_dev->en_wmm == OAL_FALSE)) {
            hmac_tx_update_tid(pst_mac_dev->en_wmm, tid_num);
        }
#if defined(_PRE_WLAN_FEATURE_WMMAC)
    }
#endif
}
void hmac_update_arp_tid_shenkuo(uint8_t *tid)
{
    *tid = WLAN_TIDNO_BCAST;
}

void hmac_tx_classify(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *hmac_user,
    oal_netbuf_stru *pst_buf, mac_tx_ctl_stru *tx_ctl)
{
    uint8_t uc_tid = 0;
    mac_device_stru *pst_mac_dev = NULL;
    mac_ether_header_stru *ether_header = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);

    /* ��qos��ͬ����Ҫ��EAPOL����ҵ��ʶ�� */
    hmac_tx_classify_lan_to_wlan(pst_hmac_vap, hmac_user, pst_buf, &uc_tid);
    /* ��QoSվ�㣬ֱ�ӷ��أ�Ĭ����TID6 */
    if (oal_unlikely(hmac_user->st_user_base_info.st_cap_info.bit_qos != OAL_TRUE)) {
        if ((ether_header->us_ether_type == oal_host2net_short(ETHER_TYPE_WAI)) ||
            (ether_header->us_ether_type == oal_host2net_short(ETHER_TYPE_PAE))) {
            /* ����ac��tid��cb�ֶ� */
            MAC_GET_CB_WME_TID_TYPE(tx_ctl) = uc_tid;
            MAC_GET_CB_WME_AC_TYPE(tx_ctl) = WLAN_WME_TID_TO_AC(uc_tid);
        }
        oam_info_log0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX, "{hmac_tx_classify::user isn't a QoS sta.}");
        return;
    }

    pst_mac_dev = mac_res_get_dev(hmac_user->st_user_base_info.uc_device_id);
    if (oal_unlikely(pst_mac_dev == NULL)) {
        oam_warning_log0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX, "{hmac_tx_classify::pst_mac_dev null.}");
        return;
    }

    hmac_tx_wmmac_classify(pst_hmac_vap, tx_ctl, pst_mac_dev, &(hmac_user->st_user_base_info), &uc_tid);

#ifdef _PRE_WLAN_FEATURE_HIEX
    if (g_wlan_spec_cfg->feature_hiex_is_open) {
        /* �������Ϸ������ݣ���֧��������Ƭ���У���NS���У����⣬��VO���� */
        hmac_hiex_judge_is_game_marked_enter_to_vo(pst_hmac_vap, hmac_user, pst_buf, &uc_tid);
    }
#endif
    hmac_tx_classify_get_tid(pst_mac_dev, pst_hmac_vap, tx_ctl, &uc_tid);
#ifdef _PRE_WLAN_FEATURE_VSP
    hmac_tx_classify_vsp_change_tidno(hmac_user, &uc_tid);
#endif

    /* ����ac��tid��cb�ֶ� */
    MAC_GET_CB_WME_TID_TYPE(tx_ctl) = uc_tid;
    MAC_GET_CB_WME_AC_TYPE(tx_ctl) = WLAN_WME_TID_TO_AC(uc_tid);
}


void hmac_rx_dyn_bypass_extlna_switch(uint32_t tx_throughput_mbps, uint32_t rx_throughput_mbps)
{
    mac_device_stru *pst_mac_device = NULL;
    mac_vap_stru *pst_mac_vap = NULL;
    uint32_t limit_throughput_high, limit_throughput_low, ret;
    uint32_t throughput_mbps = oal_max(tx_throughput_mbps, rx_throughput_mbps);
    oal_bool_enum_uint8 en_is_pm_test;
    mac_rx_dyn_bypass_extlna_stru *rx_extlna = mac_vap_get_rx_dyn_bypass_extlna_switch();

    /* ������ƻ���֧�ָ�������bypass����LNA */
    if (rx_extlna->uc_ini_en == OAL_FALSE) {
        return;
    }

    /* ÿ������������ */
    if ((rx_extlna->us_throughput_high != 0) && (rx_extlna->us_throughput_low != 0)) {
        limit_throughput_high = rx_extlna->us_throughput_high;
        limit_throughput_low = rx_extlna->us_throughput_low;
    } else {
        limit_throughput_high = WLAN_DYN_BYPASS_EXTLNA_THROUGHPUT_THRESHOLD_HIGH;
        limit_throughput_low = WLAN_DYN_BYPASS_EXTLNA_THROUGHPUT_THRESHOLD_LOW;
    }
    if (throughput_mbps > limit_throughput_high) {
        /* ����100M,�ǵ͹��Ĳ��Գ��� */
        en_is_pm_test = OAL_FALSE;
    } else if (throughput_mbps < limit_throughput_low) {
        /* ����50M,�͹��Ĳ��Գ��� */
        en_is_pm_test = OAL_TRUE;
    } else {
        /* ����50M-100M֮��,�����л� */
        return;
    }

    /* ��Ҫ�л�ʱ������������֪ͨdevice���� */
    pst_mac_device = mac_res_get_dev(0);
    ret = mac_device_find_up_vap(pst_mac_device, &pst_mac_vap);
    if ((ret != OAL_SUCC) || (pst_mac_vap == NULL)) {
        return;
    }

    /* ��ǰ��ʽ��ͬ,������ */
    if (rx_extlna->uc_cur_status == en_is_pm_test) {
        return;
    }

    ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_DYN_EXTLNA_BYPASS_SWITCH,
        sizeof(uint8_t), (uint8_t *)(&en_is_pm_test));
    if (ret == OAL_SUCC) {
        rx_extlna->uc_cur_status = en_is_pm_test;
    }

    oam_warning_log4(0, OAM_SF_ANY, "{hmac_rx_dyn_bypass_extlna_switch: limit_high:%d,limit_low:%d,throughput:%d,\
        cur_status:%d(0:not pm, 1:pm))!}", limit_throughput_high, limit_throughput_low, throughput_mbps, en_is_pm_test);
}
void hmac_tx_small_amsdu_get_limit_throughput(uint32_t *limit_throughput_high,
    uint32_t *limit_throughput_low)
{
    mac_small_amsdu_switch_stru *small_amsdu_switch = mac_vap_get_small_amsdu_switch();

    if ((small_amsdu_switch->us_small_amsdu_throughput_high != 0) &&
        (small_amsdu_switch->us_small_amsdu_throughput_low != 0)) {
        *limit_throughput_high = small_amsdu_switch->us_small_amsdu_throughput_high;
        *limit_throughput_low = small_amsdu_switch->us_small_amsdu_throughput_low;
    } else {
        *limit_throughput_high = WLAN_SMALL_AMSDU_THROUGHPUT_THRESHOLD_HIGH;
        *limit_throughput_low = WLAN_SMALL_AMSDU_THROUGHPUT_THRESHOLD_LOW;
    }
}

void hmac_tx_small_amsdu_get_limit_pps(uint32_t *limit_pps_high, uint32_t *limit_pps_low)
{
    mac_small_amsdu_switch_stru *small_amsdu_switch = mac_vap_get_small_amsdu_switch();

    if ((small_amsdu_switch->us_small_amsdu_pps_high != 0) &&
        (small_amsdu_switch->us_small_amsdu_pps_low != 0)) {
        *limit_pps_high = small_amsdu_switch->us_small_amsdu_pps_high;
        *limit_pps_low = small_amsdu_switch->us_small_amsdu_pps_low;
    } else {
        *limit_pps_high = WLAN_SMALL_AMSDU_PPS_THRESHOLD_HIGH;
        *limit_pps_low = WLAN_SMALL_AMSDU_PPS_THRESHOLD_LOW;
    }
}
#ifdef _PRE_WLAN_TCP_OPT
static void hmac_tcp_ack_update(mac_device_stru *mac_device, oal_bool_enum_uint8 en_tcp_ack_filter)
{
    uint8_t  uc_vap_idx;
    hmac_vap_stru *hmac_vap = NULL;
    hmac_device_stru *pst_hmac_device = NULL;

    if (mac_device == NULL) {
        return;
    }
    pst_hmac_device = hmac_res_get_mac_dev(mac_device->uc_device_id);
    if (pst_hmac_device == NULL) {
        return;
    }
    pst_hmac_device->sys_tcp_tx_ack_opt_enable = en_tcp_ack_filter;

    for (uc_vap_idx = 0; uc_vap_idx < mac_device->uc_vap_num; uc_vap_idx++) {
        hmac_vap = mac_res_get_hmac_vap(mac_device->auc_vap_id[uc_vap_idx]);
        if (oal_unlikely(hmac_vap == NULL)) {
            oam_warning_log1(0, OAM_SF_SCAN, "hmac_tcp_ack_update::vap is null! vap id %d",
                mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        hmac_vap->sys_tcp_tx_ack_opt_enable = en_tcp_ack_filter;
    }
}

void hmac_tcp_ack_filter_switch(uint32_t rx_throughput_mbps)
{
    mac_device_stru *pst_mac_device = mac_res_get_dev(0);
    uint32_t limit_throughput_high, limit_throughput_low;
    oal_bool_enum_uint8 en_tcp_ack_filter;
    oal_bool_enum_uint8 en_mu_vap_flag = (mac_device_calc_up_vap_num(pst_mac_device) > 1);
    /* ������ƻ���֧��tcp ack���˶�̬���� */
    if (g_st_tcp_ack_filter.uc_tcp_ack_filter_en == OAL_FALSE) {
        return;
    }

    /* ÿ������������ */
    if ((g_st_tcp_ack_filter.us_rx_filter_throughput_high != 0) &&
        (g_st_tcp_ack_filter.us_rx_filter_throughput_low != 0)) {
        limit_throughput_high = g_st_tcp_ack_filter.us_rx_filter_throughput_high >> en_mu_vap_flag;
        limit_throughput_low = g_st_tcp_ack_filter.us_rx_filter_throughput_low >> en_mu_vap_flag;
    } else {
        limit_throughput_high = WLAN_TCP_ACK_FILTER_THROUGHPUT_TH_HIGH >> en_mu_vap_flag;
        limit_throughput_low = WLAN_TCP_ACK_FILTER_THROUGHPUT_TH_LOW >> en_mu_vap_flag;
    }
    if (rx_throughput_mbps > limit_throughput_high) {
        /* ����60M, ��tcp ack���� */
        en_tcp_ack_filter = OAL_TRUE;
    } else if (rx_throughput_mbps < limit_throughput_low) {
        /* ����20M,�ر�tcp ack���� */
        en_tcp_ack_filter = OAL_FALSE;
    } else {
        /* ����20M-60M֮��,�����л� */
        return;
    }
    if (g_st_tcp_ack_filter.uc_cur_filter_status == en_tcp_ack_filter) {
        return;
    }
    g_st_tcp_ack_filter.uc_cur_filter_status = en_tcp_ack_filter;
    hmac_tcp_ack_update(pst_mac_device, en_tcp_ack_filter);
    oam_warning_log3(0, OAM_SF_ANY, "{hmac_tcp_ack_filter_switch: limit_high = [%d],limit_low = [%d],\
        rx_throught= [%d]!}", limit_throughput_high, limit_throughput_low, rx_throughput_mbps);
}
#endif


void hmac_tx_small_amsdu_switch(uint32_t rx_throughput_mbps, uint32_t tx_pps)
{
    mac_device_stru *pst_mac_device = NULL;
    mac_vap_stru *pst_mac_vap = NULL;
    mac_cfg_ampdu_tx_on_param_stru st_ampdu_tx_on = { 0 };
    uint32_t limit_throughput_high, limit_throughput_low;
    uint32_t limit_pps_high;
    uint32_t limit_pps_low;
    uint32_t ret;
    oal_bool_enum_uint8 en_small_amsdu;
    mac_small_amsdu_switch_stru *small_amsdu_switch = mac_vap_get_small_amsdu_switch();

    /* ������ƻ���֧��Ӳ���ۺ� */
    if (small_amsdu_switch->uc_ini_small_amsdu_en == OAL_FALSE) {
        return;
    }

    /* ÿ������������ */
    hmac_tx_small_amsdu_get_limit_throughput(&limit_throughput_high, &limit_throughput_low);
    /* ÿ��PPS���� */
    hmac_tx_small_amsdu_get_limit_pps(&limit_pps_high, &limit_pps_low);

    if ((rx_throughput_mbps > limit_throughput_high) || (tx_pps > limit_pps_high)) {
        /* rx����������300M����tx pps����25000,��С��amsdu�ۺ� */
        en_small_amsdu = OAL_TRUE;
    } else if ((rx_throughput_mbps < limit_throughput_low) && (tx_pps < limit_pps_low)) {
        /* rx����������200M��tx ppsС��15000,�ر�С��amsdu�ۺϣ����������л� */
        en_small_amsdu = OAL_FALSE;
    } else {
        /* ����200M-300M֮��,�����л� */
        return;
    }

    /* ��ǰ�ۺϷ�ʽ��ͬ,������ */
    if (small_amsdu_switch->uc_cur_small_amsdu_en == en_small_amsdu) {
        return;
    }

    pst_mac_device = mac_res_get_dev(0);
    /* ����ǵ�VAP,���л� */
    if (mac_device_calc_up_vap_num(pst_mac_device) != 1) {
        return;
    }

    ret = mac_device_find_up_vap(pst_mac_device, &pst_mac_vap);
    if ((ret != OAL_SUCC) || (pst_mac_vap == NULL)) {
        return;
    }

    oam_warning_log3(0, OAM_SF_ANY, "{hmac_tx_small_amsdu_switch: limit_high = [%d],limit_low = [%d],\
        rx_throught= [%d]!}", limit_throughput_high, limit_throughput_low, rx_throughput_mbps);
    oam_warning_log3(0, OAM_SF_ANY, "{hmac_tx_small_amsdu_switch: PPS limit_high = [%d],PPS limit_low = [%d],\
        tx_pps = %d!}", limit_pps_high, limit_pps_low, tx_pps);

    st_ampdu_tx_on.uc_aggr_tx_on = en_small_amsdu;

    small_amsdu_switch->uc_cur_small_amsdu_en = en_small_amsdu;

    hmac_config_set_amsdu_tx_on(pst_mac_vap, sizeof(mac_cfg_ampdu_tx_on_param_stru), (uint8_t *)&st_ampdu_tx_on);
}

#ifdef _PRE_WLAN_FEATURE_TCP_ACK_BUFFER
void hmac_set_vap_throughput_threshould_by_bw(mac_vap_stru *mac_vap[], uint32_t *limit_throughput_high,
    uint32_t *limit_throughput_low, uint32_t rx_throughput_mbps, uint8_t vap_num)
{
    uint8_t  vap_idx;
    oal_bool_enum_uint8 en_mu_vap = (vap_num > 1);
#ifdef _PRE_WLAN_CHBA_MGMT
    /* ���vap upʱֻ֧������vap���㷨 */
    if ((g_optimized_feature_switch_bitmap & BIT(CUSTOM_OPTIMIZE_CHBA)) && (vap_num > TWO_UP_VAP_NUMS)) {
        vap_num = TWO_UP_VAP_NUMS;
    }
#endif
    for (vap_idx = 0; vap_idx < vap_num; vap_idx++) {
        if (mac_vap[vap_idx]->st_channel.en_bandwidth == WLAN_BAND_WIDTH_20M) {
            /* ����20M��ÿ������������ */
            hmac_tcp_ack_buf_set_throughput_threshould(WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_HIGH,
                limit_throughput_high, limit_throughput_low, en_mu_vap);
        } else if ((mac_vap[vap_idx]->st_channel.en_bandwidth == WLAN_BAND_WIDTH_40PLUS) ||
                   (mac_vap[vap_idx]->st_channel.en_bandwidth == WLAN_BAND_WIDTH_40MINUS)) {
            /* ����40M��ÿ������������ */
            hmac_tcp_ack_buf_set_throughput_threshould(WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_HIGH_40M,
                limit_throughput_high, limit_throughput_low, en_mu_vap);
        } else if ((mac_vap[vap_idx]->st_channel.en_bandwidth >= WLAN_BAND_WIDTH_80PLUSPLUS) &&
                   (mac_vap[vap_idx]->st_channel.en_bandwidth <= WLAN_BAND_WIDTH_80MINUSMINUS)) {
            /* ����80M��ÿ������������ */
            hmac_tcp_ack_buf_set_throughput_threshould(WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_HIGH_80M,
                limit_throughput_high, limit_throughput_low, en_mu_vap);
#ifdef _PRE_WLAN_FEATURE_160M
        } else if ((mac_vap[vap_idx]->st_channel.en_bandwidth >= WLAN_BAND_WIDTH_160PLUSPLUSPLUS) &&
                   (mac_vap[vap_idx]->st_channel.en_bandwidth <= WLAN_BAND_WIDTH_160MINUSMINUSMINUS)) {
            /* ����160M��ÿ������������ */
            hmac_tcp_ack_buf_set_throughput_threshould(WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_HIGH_160M,
                limit_throughput_high, limit_throughput_low, en_mu_vap);
#endif
        }
        hmac_tcp_ack_buf_switch_judge(mac_vap[vap_idx], *limit_throughput_high,
                                      *limit_throughput_low, rx_throughput_mbps, vap_idx);
    }
}


void hmac_tx_tcp_ack_buf_switch(uint32_t rx_throughput_mbps)
{
    mac_device_stru *mac_device = mac_res_get_dev(0);
    mac_vap_stru *mac_vap[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE] = {NULL};
    hmac_vap_stru *hmac_vap = NULL;
    uint32_t limit_throughput_high = 550;
    uint32_t limit_throughput_low = 450;
    uint8_t  vap_num = mac_device_calc_up_vap_num(mac_device);
    uint32_t ret;
    oal_bool_enum_uint8 en_mu_vap = (vap_num > 1);
    mac_tcp_ack_buf_switch_stru *tcp_ack_buf_switch = mac_vap_get_tcp_ack_buf_switch();

    /* ������ƻ����ز�֧���л���ֱ�ӷ��� */
    if (tcp_ack_buf_switch->uc_ini_tcp_ack_buf_en == OAL_FALSE) {
        return;
    }

    if (en_mu_vap) {
        ret = mac_device_find_up_vaps(mac_device, mac_vap, WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE);
    } else {
        ret = mac_device_find_up_vap(mac_device, &mac_vap[0]);
    }
    if ((ret != OAL_SUCC) || (mac_vap[0] == NULL) || (en_mu_vap && (mac_vap[1] == NULL))) {
        return;
    }

    tcp_ack_buf_switch->us_tcp_ack_smooth_throughput >>= 1;
    tcp_ack_buf_switch->us_tcp_ack_smooth_throughput += (rx_throughput_mbps >> 1);

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap[0]->uc_vap_id);
    if (!en_mu_vap && IS_LEGACY_STA(mac_vap[0]) && hmac_vap != NULL &&
        ((hmac_vap->tcp_ack_buf_use_ctl_switch == OAL_SWITCH_ON) ||
        (tcp_ack_buf_switch->uc_ini_tcp_ack_buf_userctl_test_en == OAL_TRUE))) {
        /* �ϲ��´�ָ�� ָ������������ */
        limit_throughput_high = tcp_ack_buf_switch->us_tcp_ack_buf_userctl_high;
        limit_throughput_low = tcp_ack_buf_switch->us_tcp_ack_buf_userctl_low;
        hmac_tcp_ack_buf_switch_judge(mac_vap[0], limit_throughput_high,
            limit_throughput_low, rx_throughput_mbps, 0); // 0����ֻ��һ��vap
        return;
    }

    /* �ϲ�δ�´�ָ�����������޻�֧�ָù��� */
    hmac_set_vap_throughput_threshould_by_bw(mac_vap, &limit_throughput_high,
                                             &limit_throughput_low, rx_throughput_mbps, vap_num);
}


OAL_STATIC void hmac_tcp_ack_buf_set_throughput_threshould(uint32_t high_throughput_level,
    uint32_t *ul_limit_throughput_high, uint32_t *ul_limit_throughput_low, oal_bool_enum_uint8  en_mu_vap)
{
    mac_tcp_ack_buf_switch_stru *tcp_ack_buf_switch = mac_vap_get_tcp_ack_buf_switch();
    switch (high_throughput_level) {
        case WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_HIGH:
            *ul_limit_throughput_high = WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_HIGH >> en_mu_vap;
            *ul_limit_throughput_low = WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_LOW >> en_mu_vap;
            if ((tcp_ack_buf_switch->us_tcp_ack_buf_throughput_high != 0) &&
                (tcp_ack_buf_switch->us_tcp_ack_buf_throughput_low != 0)) {
                *ul_limit_throughput_high = tcp_ack_buf_switch->us_tcp_ack_buf_throughput_high >> en_mu_vap;
                *ul_limit_throughput_low = tcp_ack_buf_switch->us_tcp_ack_buf_throughput_low >> en_mu_vap;
            }
            break;
        case WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_HIGH_40M:
            *ul_limit_throughput_high = WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_HIGH_40M >> en_mu_vap;
            *ul_limit_throughput_low = WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_LOW_40M >> en_mu_vap;
            if ((tcp_ack_buf_switch->us_tcp_ack_buf_throughput_high_40M != 0) &&
                (tcp_ack_buf_switch->us_tcp_ack_buf_throughput_low_40M != 0)) {
                *ul_limit_throughput_high = tcp_ack_buf_switch->us_tcp_ack_buf_throughput_high_40M >> en_mu_vap;
                *ul_limit_throughput_low = tcp_ack_buf_switch->us_tcp_ack_buf_throughput_low_40M >> en_mu_vap;
            }
            break;
        case WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_HIGH_80M:
            *ul_limit_throughput_high = WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_HIGH_80M >> en_mu_vap;
            *ul_limit_throughput_low = WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_LOW_80M >> en_mu_vap;
            if ((tcp_ack_buf_switch->us_tcp_ack_buf_throughput_high_80M != 0) &&
                (tcp_ack_buf_switch->us_tcp_ack_buf_throughput_low_80M != 0)) {
                *ul_limit_throughput_high = tcp_ack_buf_switch->us_tcp_ack_buf_throughput_high_80M >> en_mu_vap;
                *ul_limit_throughput_low = tcp_ack_buf_switch->us_tcp_ack_buf_throughput_low_80M >> en_mu_vap;
            }
            break;
#ifdef _PRE_WLAN_FEATURE_160M
        case WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_HIGH_160M:
            *ul_limit_throughput_high = WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_HIGH_160M >> en_mu_vap;
            *ul_limit_throughput_low = WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_LOW_160M >> en_mu_vap;
            if ((tcp_ack_buf_switch->us_tcp_ack_buf_throughput_high_160M != 0) &&
                (tcp_ack_buf_switch->us_tcp_ack_buf_throughput_low_160M != 0)) {
                *ul_limit_throughput_high = tcp_ack_buf_switch->us_tcp_ack_buf_throughput_high_160M >> en_mu_vap;
                *ul_limit_throughput_low = tcp_ack_buf_switch->us_tcp_ack_buf_throughput_low_160M;
            }
            break;
#endif
        default:
            break;
    }
}

OAL_STATIC void hmac_tcp_ack_buf_switch_judge(mac_vap_stru *pst_mac_vap, uint32_t limit_throughput_high,
    uint32_t limit_throughput_low, uint32_t rx_throughput_mbps, uint8_t  uc_vap_idx)
{
    mac_cfg_tcp_ack_buf_stru st_tcp_ack_param = { 0 };
    oal_bool_enum_uint8 en_tcp_ack_buf;
    mac_tcp_ack_buf_switch_stru *tcp_ack_buf_switch = mac_vap_get_tcp_ack_buf_switch();

    if (tcp_ack_buf_switch->us_tcp_ack_smooth_throughput > limit_throughput_high) {
        /* ����������,��tcp ack buf */
        en_tcp_ack_buf = OAL_TRUE;
    } else if (tcp_ack_buf_switch->us_tcp_ack_smooth_throughput < limit_throughput_low) {
        /* ����������,�ر�tcp ack buf */
        en_tcp_ack_buf = OAL_FALSE;
    } else {
        /* ������������֮�䣬 �����л� */
        return;
    }

    /* δ�����仯 ,������ */
    if (tcp_ack_buf_switch->uc_cur_tcp_ack_buf_en[uc_vap_idx] == en_tcp_ack_buf) {
        return;
    }

    oam_warning_log4(0, OAM_SF_ANY,
        "{hmac_tx_tcp_ack_buf_switch: limit_high = [%d],limit_low = [%d],rx_throught= [%d]! en_tcp_ack_buf=%d}",
        limit_throughput_high, limit_throughput_low, rx_throughput_mbps, en_tcp_ack_buf);

    tcp_ack_buf_switch->uc_cur_tcp_ack_buf_en[uc_vap_idx] = en_tcp_ack_buf;

    st_tcp_ack_param.en_cmd = MAC_TCP_ACK_BUF_ENABLE;
    st_tcp_ack_param.en_enable = en_tcp_ack_buf;

    hmac_config_tcp_ack_buf(pst_mac_vap, sizeof(mac_cfg_tcp_ack_buf_stru), (uint8_t *)&st_tcp_ack_param);
}
#endif

#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
OAL_STATIC uint32_t hmac_tx_ampdu_hw_switch(mac_vap_stru *pst_mac_vap, uint32_t tx_throughput_mbps,
    uint32_t limit_throughput_high, uint32_t limit_throughput_low, oal_bool_enum_uint8 *pen_ampdu_hw)
{
    if ((tx_throughput_mbps > limit_throughput_high)
#ifdef _PRE_WLAN_FEATURE_11AX
        || (g_wlan_spec_cfg->feature_11ax_is_open &&
        (pst_mac_vap->bit_ofdma_aggr_num == MAC_VAP_AMPDU_HW) &&
        (tx_throughput_mbps > (limit_throughput_high >> 2))) /* 2�������޳���4 */
#endif
    ) {
        /* ����350M����UL OFDMA��������Ҫ�ۺ��ֽ����ر𳤲�����һ��������,�л�Ӳ���ۺ� */
        *pen_ampdu_hw = OAL_TRUE;
    } else if (tx_throughput_mbps < limit_throughput_low) {
        /* ����200M,�л�Ӳ���ۺ� */
        *pen_ampdu_hw = OAL_FALSE;
    } else {
        /* ����200M-300M֮��,�����л� */
        return OAL_FAIL;
    }

    return OAL_SUCC;
}

OAL_STATIC uint32_t hmac_tx_ampdu_close_hw_switch(mac_vap_stru *pst_mac_vap)
{
    if (oal_value_ne_all2(pst_mac_vap->en_protocol, WLAN_VHT_MODE, WLAN_VHT_ONLY_MODE)) {
        if (g_wlan_spec_cfg->feature_11ax_is_open) {
#ifdef _PRE_WLAN_FEATURE_11AX
            if (pst_mac_vap->en_protocol != WLAN_HE_MODE) {
                return OAL_FAIL;
            }
#endif
        } else {
            return OAL_FAIL;
        }
    }

    return OAL_SUCC;
}


#define HMAC_TX_AMPDU_AGGR_HW_ENABLE 4
#define HMAC_TX_AMPDU_AGGR_HW_DISABLE 8
OAL_STATIC void hmac_tx_ampdu_hw_cfg_send(mac_vap_stru *pst_mac_vap, oal_bool_enum_uint8 en_ampdu_hw)
{
    mac_cfg_ampdu_tx_on_param_stru st_ampdu_tx_on = { 0 };

    if (en_ampdu_hw == OAL_TRUE) {
        st_ampdu_tx_on.uc_aggr_tx_on = HMAC_TX_AMPDU_AGGR_HW_ENABLE;
    } else {
        st_ampdu_tx_on.uc_aggr_tx_on = HMAC_TX_AMPDU_AGGR_HW_DISABLE;
    }

    st_ampdu_tx_on.uc_snd_type = OAL_TRUE;
#ifdef _PRE_WLAN_FEATURE_11AX
    if (pst_mac_vap->bit_ofdma_aggr_num == MAC_VAP_AMPDU_HW) {
        st_ampdu_tx_on.uc_snd_type = OAL_FALSE;
    }
#endif
    st_ampdu_tx_on.en_aggr_switch_mode = AMPDU_SWITCH_BY_BA_LUT;
    hmac_config_set_ampdu_tx_on(pst_mac_vap, sizeof(mac_cfg_ampdu_tx_on_param_stru), (uint8_t *)&st_ampdu_tx_on);
}


void hmac_tx_ampdu_switch(uint32_t tx_throughput_mbps)
{
    mac_device_stru *pst_mac_device = NULL;
    mac_vap_stru *pst_mac_vap = NULL;
    uint32_t limit_throughput_high = WLAN_AMPDU_THROUGHPUT_THRESHOLD_HIGH;
    uint32_t limit_throughput_low = WLAN_AMPDU_THROUGHPUT_THRESHOLD_LOW;
    uint32_t ret;
    oal_bool_enum_uint8 en_ampdu_hw = OAL_TRUE;
    hmac_user_stru *pst_hmac_user = NULL;
    /* ������ƻ���֧��Ӳ���ۺ� */
    if (g_st_ampdu_hw.uc_ampdu_hw_en == OAL_FALSE) {
        return;
    }
    /* ÿ������������ */
    if ((g_st_ampdu_hw.us_throughput_high != 0) && (g_st_ampdu_hw.us_throughput_low != 0)) {
        limit_throughput_high = g_st_ampdu_hw.us_throughput_high;
        limit_throughput_low = g_st_ampdu_hw.us_throughput_low;
    }

    pst_mac_device = mac_res_get_dev(0);
    /* ����ǵ�VAP,�򲻿���Ӳ���ۺ� */
    if (mac_device_calc_up_vap_num(pst_mac_device) != 1) {
        en_ampdu_hw = OAL_FALSE;
    } else {
        ret = mac_device_find_up_vap(pst_mac_device, &pst_mac_vap);
        if ((ret != OAL_SUCC) || (pst_mac_vap == NULL)) {
            return;
        }
        /* Ƕ������Ż���װ */
        if (hmac_tx_ampdu_hw_switch(pst_mac_vap, tx_throughput_mbps, limit_throughput_high,
            limit_throughput_low, &en_ampdu_hw) != OAL_SUCC) {
            return;
        }
    }

    /* ��ǰ�ۺϷ�ʽ��ͬ,������ */
    if (g_st_ampdu_hw.uc_ampdu_hw_enable == en_ampdu_hw) {
        return;
    }

    /* �л���Ӳ���ۺ�ʱ,��Ҫ�ж��Ƿ�����л����� */
    if (en_ampdu_hw == OAL_TRUE) {
        g_st_ampdu_hw.us_remain_hw_cnt = 0;

        /* VHT/HE��֧���л� */ /* Ƕ������Ż���װ */
        if (hmac_tx_ampdu_close_hw_switch(pst_mac_vap) != OAL_SUCC) {
            return;
        }

        /* ����:���鹤��Ƶ�ʵ���160Mhz(80M���´���)��֧���л� */
        if (WLAN_BAND_WIDTH_80PLUSPLUS > pst_mac_vap->st_channel.en_bandwidth) {
            return;
        }
        /* ��������AP������Ӳ���ۺ��л� */
        if (IS_LEGACY_STA(pst_mac_vap)) {
            pst_hmac_user = mac_res_get_hmac_user(pst_mac_vap->us_assoc_vap_id);
            if (pst_hmac_user == NULL) {
                oam_error_log0(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "hmac_tx_ampdu_switch: pst_hmac_user is null ptr.");
                return;
            }
            if (pst_hmac_user->en_user_ap_type & MAC_AP_TYPE_AGGR_BLACKLIST) {
                return;
            }
        }
    } else {
        /* ��������Ƶ���л�,��Ӳ���ۺ��л�����ۺ�,�������ܽ������л���ֵ,
           ����Ҫ��Ӳ���ۺ�����������һ��ʱ�� */
        /* �ǵ�vap,ֱ���л� */
        if (mac_device_calc_up_vap_num(pst_mac_device) == 1) {
            g_st_ampdu_hw.us_remain_hw_cnt++;
            if (g_st_ampdu_hw.us_remain_hw_cnt < WLAN_AMPDU_HW_SWITCH_PERIOD) {
                return;
            }
            g_st_ampdu_hw.us_remain_hw_cnt = 0;
        }
        pst_mac_vap = mac_res_get_mac_vap(0);
        if (pst_mac_vap == NULL) {
            return;
        }
    }

    g_st_ampdu_hw.uc_ampdu_hw_enable = en_ampdu_hw;

    oam_warning_log3(0, OAM_SF_ANY, "{hmac_tx_ampdu_switch: limit_high = [%d],limit_low = [%d],tx_throught= [%d]!}",
                     limit_throughput_high, limit_throughput_low, tx_throughput_mbps);

    hmac_tx_ampdu_hw_cfg_send(pst_mac_vap, en_ampdu_hw);
}
#endif


uint32_t hmac_tx_filter_security(hmac_vap_stru *pst_hmac_vap, oal_netbuf_stru *pst_buf, hmac_user_stru *pst_hmac_user)
{
    mac_ether_header_stru *pst_ether_header = NULL;
    mac_user_stru *pst_mac_user = NULL;
    mac_vap_stru *pst_mac_vap = NULL;
    uint32_t ret = OAL_SUCC;
    uint16_t us_ether_type;

    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);
    pst_mac_user = &(pst_hmac_user->st_user_base_info);

    if (OAL_TRUE == mac_mib_get_rsnaactivated(pst_mac_vap)) { /* �ж��Ƿ�ʹ��WPA/WPA2 */
        if (mac_user_get_port(pst_mac_user) != OAL_TRUE) { /* �ж϶˿��Ƿ�� */
            /* ��ȡ��̫��ͷ */
            pst_ether_header = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);
            /* ��������ʱ����Է�EAPOL ������֡������ */
            if (oal_byteorder_host_to_net_uint16(ETHER_TYPE_PAE) != pst_ether_header->us_ether_type) {
                us_ether_type = oal_byteorder_host_to_net_uint16(pst_ether_header->us_ether_type);
                oam_warning_log2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
                                 "{hmac_tx_filter_security::TYPE 0x%04x, 0x%04x.}",
                                 us_ether_type, ETHER_TYPE_PAE);
                ret = OAL_FAIL;
            }
        }
    }

    return ret;
}


void hmac_tx_ba_setup(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_user, uint8_t uc_tidno)
{
    mac_action_mgmt_args_stru st_action_args; /* ������дACTION֡�Ĳ��� */

    /*
    ����BA�Ựʱ��st_action_args�ṹ������Ա��������
    (1)uc_category:action�����
    (2)uc_action:BA action�µ����
    (3)arg1:BA�Ự��Ӧ��TID
    (4)arg2:BUFFER SIZE��С
    (5)arg3:BA�Ự��ȷ�ϲ���
    (6)arg4:TIMEOUTʱ��
    */
    st_action_args.uc_category = MAC_ACTION_CATEGORY_BA;
    st_action_args.uc_action = MAC_BA_ACTION_ADDBA_REQ;
    st_action_args.arg1 = uc_tidno; /* ������֡��Ӧ��TID�� */
    /* ADDBA_REQ�У�buffer_size��Ĭ�ϴ�С */
    st_action_args.arg2 = (uint32_t)g_wlan_cust.ampdu_tx_max_num;

    st_action_args.arg3 = MAC_BA_POLICY_IMMEDIATE; /* BA�Ự��ȷ�ϲ��� */
    st_action_args.arg4 = 0;                       /* BA�Ự�ĳ�ʱʱ������Ϊ0 */

    /* ����BA�Ự */
    hmac_mgmt_tx_action(pst_hmac_vap, pst_user, &st_action_args);
}


uint32_t hmac_tx_ucast_process(hmac_vap_stru *pst_hmac_vap,
    oal_netbuf_stru *pst_buf, hmac_user_stru *pst_user, mac_tx_ctl_stru *pst_tx_ctl)
{
    uint32_t ret;
    /* ��ȫ���� */
    if (oal_unlikely(OAL_SUCC != hmac_tx_filter_security(pst_hmac_vap, pst_buf, pst_user))) {
        oam_stat_vap_incr(pst_hmac_vap->st_vap_base_info.uc_vap_id, tx_security_check_faild, 1);
        return HMAC_TX_DROP_SECURITY_FILTER;
    }

    if (hmac_ring_tx_enabled() == OAL_TRUE) {
        MAC_GET_CB_WME_TID_TYPE(pst_tx_ctl) = WLAN_TIDNO_BCAST;
        MAC_GET_CB_WME_AC_TYPE(pst_tx_ctl) = WLAN_WME_TID_TO_AC(WLAN_TIDNO_BCAST);
    }

    /* �����EAPOL��DHCP֡����������������BA�Ự */
    if (MAC_GET_CB_IS_VIPFRAME(pst_tx_ctl)) {
        return HMAC_TX_PASS;
    }

    if (hmac_tid_need_ba_session(pst_hmac_vap, pst_user, MAC_GET_CB_WME_TID_TYPE(pst_tx_ctl), pst_buf) == OAL_TRUE) {
        /* �Զ���������BA�Ự������AMPDU�ۺϲ�����Ϣ��DMACģ��Ĵ���addba rsp֡��ʱ�̺��� */
        hmac_tx_ba_setup(pst_hmac_vap, pst_user, MAC_GET_CB_WME_TID_TYPE(pst_tx_ctl));
    }

    ret = hmac_amsdu_notify(pst_hmac_vap, pst_user, pst_buf);
    if (oal_unlikely(ret != HMAC_TX_PASS)) {
        return ret;
    }
    return HMAC_TX_PASS;
}


uint32_t hmac_tx_need_frag(
    hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, oal_netbuf_stru *netbuf, mac_tx_ctl_stru *tx_ctl)
{
    uint32_t threshold;
    oal_bool_enum_uint8 need_frag = OAL_TRUE;
    /* �жϱ����Ƿ���Ҫ���з�Ƭ */
    /* 1�����ȴ�������          */
    /* 2����legacЭ��ģʽ       */
    /* 3�����ǹ㲥֡            */
    /* 4�����Ǿۺ�֡            */
    /* 6��DHCP֡�����з�Ƭ      */
    threshold = mac_mib_get_FragmentationThreshold(&hmac_vap->st_vap_base_info);
#ifdef _PRE_WLAN_FEATURE_ERSRU
    if (hmac_user->st_ersru.bit_enable) {
        threshold = oal_min(threshold, hmac_user->st_ersru.bit_frag_len);
    }
#endif
    threshold = (threshold & (~(BIT0 | BIT1))) + BYTE_OFFSET_2;

    /* ��ǰ֡�����ڷ�Ƭ����,����Ƭ */
    if (threshold >= (oal_netbuf_len(netbuf) + MAC_GET_CB_FRAME_HEADER_LENGTH(tx_ctl))) {
        return 0;
    }

    /* ��LegacyЭ��/VIP֡/�㲥֡/����Ƭ,˽�ж�ͨ��Ƭ */
    if ((hmac_user->st_user_base_info.en_cur_protocol_mode >= WLAN_HT_MODE) ||
        (hmac_vap->st_vap_base_info.en_protocol >= WLAN_HT_MODE) ||
        (MAC_GET_CB_IS_VIPFRAME(tx_ctl) == OAL_TRUE) ||
        (MAC_GET_CB_IS_MCAST(tx_ctl) == OAL_TRUE)) {
        need_frag = OAL_FALSE;
    }
#ifdef _PRE_WLAN_FEATURE_ERSRU
    if (hmac_user->st_ersru.bit_enable) {
        need_frag = OAL_TRUE;
    }
#endif
    /* �ۺϲ���Ƭ */
    if ((need_frag == OAL_TRUE) &&
        (MAC_GET_CB_IS_AMSDU(tx_ctl) == OAL_FALSE)
#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU
        && (MAC_GET_CB_AMSDU_LEVEL(tx_ctl) == WLAN_TX_AMSDU_NONE)
#endif
        && (OAL_FALSE == hmac_tid_ba_is_setup(hmac_user, MAC_GET_CB_WME_TID_TYPE(tx_ctl)))) {
        return threshold;
    }

    return 0;
}


static uint32_t hmac_tx_encap_mac_hdr(hmac_vap_stru *hmac_vap,
    mac_tx_ctl_stru *tx_ctl, oal_netbuf_stru *netbuf, uint8_t **mac_hdr)
{
    uint32_t ret;
    uint32_t headroom_size = oal_netbuf_headroom(netbuf);
    /* �����Ring����802.11֡, ���Ƭ֡��WAPI֡��MAC HEADER��������netbuf��,���뱣֤��������֡ͷ */
    if ((hmac_ring_tx_enabled() == OAL_TRUE) && headroom_size < MAC_80211_QOS_HTC_4ADDR_FRAME_LEN) {
        oam_warning_log1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,
            "{hmac_tx_encap_mac_hdr::headroom[%d] too short}", headroom_size);
        ret = (uint32_t)oal_netbuf_expand_head(netbuf, MAC_80211_QOS_HTC_4ADDR_FRAME_LEN, 0, GFP_ATOMIC);
        if (ret != OAL_SUCC) {
            oam_error_log0(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,
                "{hmac_tx_encap_mac_hdr::expand headroom failed}");
            return OAL_ERR_CODE_ALLOC_MEM_FAIL;
        }
    }

    /* device��������dataָ��ǰԤ���Ŀռ����802.11 mac head len������Ҫ���������ڴ���802.11ͷ */
    if (headroom_size >= MAC_80211_QOS_HTC_4ADDR_FRAME_LEN) {
        *mac_hdr = (oal_netbuf_header(netbuf) - MAC_80211_QOS_HTC_4ADDR_FRAME_LEN);
        MAC_GET_CB_80211_MAC_HEAD_TYPE(tx_ctl) = 1; /* ָʾmacͷ����skb�� */
    } else {
        /* ��������80211ͷ */
        *mac_hdr = oal_mem_alloc_m(OAL_MEM_POOL_ID_SHARED_DATA_PKT, MAC_80211_QOS_HTC_4ADDR_FRAME_LEN, OAL_FALSE);
        if (oal_unlikely(*mac_hdr == NULL)) {
            oam_error_log0(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX, "{hmac_tx_encap_mac_hdr::pst_hdr null.}");
            return OAL_ERR_CODE_PTR_NULL;
        }
        MAC_GET_CB_80211_MAC_HEAD_TYPE(tx_ctl) = 0; /* ָʾmacͷ������skb�У������˶����ڴ��ŵ� */
    }
    /* �ҽ�802.11ͷ */
    MAC_GET_CB_FRAME_HEADER_ADDR(tx_ctl) = (mac_ieee80211_frame_stru *)(*mac_hdr);
    return OAL_SUCC;
}

void hmac_tx_set_frame_htc(hmac_vap_stru *pst_hmac_vap,
    mac_tx_ctl_stru *pst_tx_ctl, hmac_user_stru *pst_hmac_user, mac_ieee80211_qos_htc_frame_addr4_stru *pst_hdr_addr4)
{
    mac_ieee80211_qos_htc_frame_stru *pst_hdr = NULL;
    uint32_t                        htc_val = 0;

    if (MAC_GET_CB_WME_TID_TYPE(pst_tx_ctl) == WLAN_TIDNO_BCAST) {
        return;
    }
    if (pst_hmac_vap->uc_htc_order_flag == HAL_HE_HTC_CONFIG_NO_HTC) {
        return;
    }

    /* ��������ʱ��host���himit֡����htc�ֶ�����ΪȫF,��dmac�ж�OM��ǰ�Ƿ�ʹ��HTC�ֶ�,OM���Բ���ʱ,himit��������HTC */
#ifdef _PRE_WLAN_FEATURE_HIEX
    if ((g_wlan_spec_cfg->feature_hiex_is_open) && (hmac_tx_enable_himit_htc(pst_hmac_user, pst_tx_ctl) == OAL_TRUE)) {
        htc_val = HTC_INVALID_VALUE;
        MAC_GET_CB_IS_HIMIT_FRAME(pst_tx_ctl) = OAL_TRUE;
    } else if (MAC_USER_TX_DATA_INCLUDE_HTC(&pst_hmac_user->st_user_base_info) ||
               pst_hmac_vap->st_vap_base_info.uc_himit_set_htc != 0) {
        htc_val = HTC_INVALID_VALUE;
    } else {
        return;
    }
#endif

    MAC_GET_CB_HTC_FLAG(pst_tx_ctl) = 1;  // ���Ϊ����HTC�ֶ�
    /* ����֡ͷ���� */
    if (OAL_FALSE == MAC_GET_CB_IS_4ADDRESS(pst_tx_ctl)) {
        pst_hdr = (mac_ieee80211_qos_htc_frame_stru *)pst_hdr_addr4;
        pst_hdr->st_frame_control.bit_order = 1;
        MAC_GET_CB_FRAME_HEADER_LENGTH(pst_tx_ctl) = MAC_80211_QOS_HTC_FRAME_LEN;
        pst_hdr->htc = htc_val;
    } else {
        /* ����QOS�����ֶ� */
        pst_hdr_addr4->st_frame_control.bit_order = 1;
        MAC_GET_CB_FRAME_HEADER_LENGTH(pst_tx_ctl) = MAC_80211_QOS_HTC_4ADDR_FRAME_LEN;
        pst_hdr_addr4->htc = htc_val;
    }
}
void hmac_tx_set_qos_frame_htc(hmac_vap_stru *hmac_vap,
    mac_tx_ctl_stru *pst_tx_ctl, hmac_user_stru *pst_hmac_user, mac_ieee80211_qos_htc_frame_addr4_stru *pst_hdr_addr4)
{
    mac_ieee80211_qos_htc_frame_stru *pst_hdr = NULL;

    /* tid7 no ring nor not support */
    if (MAC_GET_CB_WME_TID_TYPE(pst_tx_ctl) == WLAN_TIDNO_BCAST) {
        return;
    }
    if (MAC_USER_NOT_SUPPORT_HTC(&pst_hmac_user->st_user_base_info)) {
        return;
    }
    /* ��������ʱ��host���himit֡����htc�ֶ�����ΪȫF,��dmac�ж�OM��ǰ�Ƿ�ʹ��HTC�ֶ�,OM���Բ���ʱ,himit��������HTC */
#ifdef _PRE_WLAN_FEATURE_HIEX
    if ((g_wlan_spec_cfg->feature_hiex_is_open) && (hmac_tx_enable_himit_htc(pst_hmac_user, pst_tx_ctl) == OAL_TRUE)) {
        MAC_GET_CB_IS_HIMIT_FRAME(pst_tx_ctl) = OAL_TRUE;
    }
#endif

    // ���Ϊ����HTC�ֶ�
    MAC_GET_CB_HTC_FLAG(pst_tx_ctl) = 1;
    /* ����֡ͷ���� */
    if (OAL_FALSE == MAC_GET_CB_IS_4ADDRESS(pst_tx_ctl)) {
        pst_hdr = (mac_ieee80211_qos_htc_frame_stru *)pst_hdr_addr4;
        pst_hdr->st_frame_control.bit_order = 1;
        MAC_GET_CB_FRAME_HEADER_LENGTH(pst_tx_ctl) = MAC_80211_QOS_HTC_FRAME_LEN;
        pst_hdr->htc = HTC_INVALID_VALUE;
    } else {
        /* ����QOS�����ֶ� */
        pst_hdr_addr4->st_frame_control.bit_order = 1;
        MAC_GET_CB_FRAME_HEADER_LENGTH(pst_tx_ctl) = MAC_80211_QOS_HTC_4ADDR_FRAME_LEN;
        pst_hdr_addr4->htc = HTC_INVALID_VALUE;
    }
}

void hmac_tx_encap_do_non_amsdu(hmac_vap_stru *pst_vap, hmac_user_stru *pst_user, oal_netbuf_stru *pst_buf,
    mac_tx_ctl_stru *pst_tx_ctl, uint16_t us_ether_type)
{
    /* len = EHTER HEAD LEN + PAYLOAD LEN */
    uint16_t us_mpdu_len = (uint16_t)oal_netbuf_get_len(pst_buf);

    /* ����frame���ȣ�ָ��skb payload--LLC HEAD */
    MAC_GET_CB_MPDU_LEN(pst_tx_ctl) = (us_mpdu_len - ETHER_HDR_LEN + SNAP_LLC_FRAME_LEN);

    MAC_GET_CB_ETHER_HEAD_PADDING(pst_tx_ctl) = 0;

#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU
    wlan_chip_tx_encap_large_skb_amsdu(pst_vap, pst_user, pst_buf, pst_tx_ctl);

    if (MAC_GET_CB_AMSDU_LEVEL(pst_tx_ctl)) {
        /* �ָ�dataָ�뵽ETHER HEAD - LLC HEAD */
        oal_netbuf_pull(pst_buf, SNAP_LLC_FRAME_LEN);
    }
#endif
    /* ����netbuf->dataָ��LLC HEAD */
    mac_set_snap(pst_buf, us_ether_type);

#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU
    if (MAC_GET_CB_AMSDU_LEVEL(pst_tx_ctl)) {
        /* �ָ�dataָ�뵽ETHER HEAD */
        oal_netbuf_push(pst_buf, ETHER_HDR_LEN);

        /* ��֤4bytes���� */
        if ((unsigned long)(uintptr_t)oal_netbuf_data(pst_buf) !=
            OAL_ROUND_DOWN((unsigned long)(uintptr_t)oal_netbuf_data(pst_buf), 4)) { // 4����4�ֽڶ���
            MAC_GET_CB_ETHER_HEAD_PADDING(pst_tx_ctl) = (unsigned long)(uintptr_t)oal_netbuf_data(pst_buf) -
                OAL_ROUND_DOWN((unsigned long)(uintptr_t)oal_netbuf_data(pst_buf), 4); // 4����4�ֽڶ���
            oal_netbuf_push(pst_buf, MAC_GET_CB_ETHER_HEAD_PADDING(pst_tx_ctl));
        }
    }
#endif
}
/*
 * ��������  : 802.11֡ͷ��װ APģʽ
 */
/*lint -e695*/
uint32_t hmac_tx_encap_ieee80211_header(hmac_vap_stru *pst_vap, hmac_user_stru *pst_user, oal_netbuf_stru *pst_buf)
{
    uint8_t *puc_80211_hdr = NULL; /* 802.11ͷ */
    uint32_t qos = HMAC_TX_BSS_NOQOS;
    mac_tx_ctl_stru *pst_tx_ctl = (mac_tx_ctl_stru *)(oal_netbuf_cb(pst_buf));
    uint32_t ret, threshold;
    uint8_t uc_buf_is_amsdu = MAC_GET_CB_IS_AMSDU(pst_tx_ctl);
    uint8_t uc_ic_header = 0;
    mac_ether_header_stru st_ether_hdr;

    memset_s(&st_ether_hdr, sizeof(mac_ether_header_stru), 0, sizeof(mac_ether_header_stru));

    /* ��ȡ��̫��ͷ, ԭ��ַ��Ŀ�ĵ�ַ, ��̫������ */
    if (EOK != memcpy_s(&st_ether_hdr, sizeof(st_ether_hdr), oal_netbuf_data(pst_buf), ETHER_HDR_LEN)) {
        oam_error_log0(0, OAM_SF_TX, "hmac_tx_encap_ieee80211_header::memcpy fail!");
    }

    /* ��amsdu֡ */
    if (uc_buf_is_amsdu == OAL_FALSE) {
        hmac_tx_encap_do_non_amsdu(pst_vap, pst_user, pst_buf, pst_tx_ctl, st_ether_hdr.us_ether_type);
    }

    ret = hmac_tx_encap_mac_hdr(pst_vap, pst_tx_ctl, pst_buf, &puc_80211_hdr);
    if (oal_unlikely(ret != OAL_SUCC)) {
        return ret;
    }

    /* �鲥�ۺ�֡������鲥֡����ȡ�û���QOS����λ��Ϣ */
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
    if ((mac_get_mcast_ampdu_switch() == OAL_TRUE) || (MAC_GET_CB_IS_MCAST(pst_tx_ctl) == OAL_FALSE)) {
#else
    if (MAC_GET_CB_IS_MCAST(pst_tx_ctl) == OAL_FALSE) {
#endif
        /* �����û��ṹ���cap_info���ж��Ƿ���QOSվ�� */
        qos = pst_user->st_user_base_info.st_cap_info.bit_qos;
    }

    /* ����֡���� */
    hmac_tx_set_frame_ctrl(qos, pst_tx_ctl, (mac_ieee80211_qos_htc_frame_addr4_stru *)puc_80211_hdr);
    /* ����֡HTC */
    if ((qos == HMAC_TX_BSS_QOS) && (g_wlan_spec_cfg->feature_11ax_is_open)) {
        wlan_chip_tx_set_frame_htc(pst_vap, pst_tx_ctl, pst_user,
            (mac_ieee80211_qos_htc_frame_addr4_stru *)puc_80211_hdr);
    }
    /* ���õ�ַ */
    hmac_tx_set_addresses(pst_vap, pst_user, pst_tx_ctl, &st_ether_hdr,
                          (mac_ieee80211_qos_htc_frame_addr4_stru *)puc_80211_hdr);

    /* ��Ƭ���� */
    threshold = hmac_tx_need_frag(pst_vap, pst_user, pst_buf, pst_tx_ctl);
    if (threshold != 0) {
        /* ���ü��ܽӿ���ʹ��TKIPʱ��MSDU���м��ܺ��ڽ��з�Ƭ */
        ret = hmac_en_mic(pst_vap, pst_user, pst_buf, &uc_ic_header);
        if (ret != OAL_SUCC) {
            oam_error_log1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_en_mic failed[%d].}", ret);
            return ret;
        }

        /* ���з�Ƭ���� */
        ret = hmac_frag_process(pst_vap, pst_buf, pst_tx_ctl, (uint32_t)uc_ic_header, threshold);
    }

#if ((defined (PRE_WLAN_FEATURE_SNIFFER)) && (defined (CONFIG_HW_SNIFFER)))
    proc_sniffer_write_file((const uint8_t *)puc_80211_hdr, MAC_80211_QOS_FRAME_LEN,
                            (const uint8_t *)oal_netbuf_data(pst_buf), oal_netbuf_len(pst_buf), 1);
#endif
    return ret;
}

/*lint +e695*/

OAL_STATIC uint32_t hmac_tx_lan_mpdu_process_sta(hmac_vap_stru *pst_vap,
    oal_netbuf_stru *pst_buf, mac_tx_ctl_stru *pst_tx_ctl)
{
    hmac_user_stru *pst_user = NULL; /* Ŀ��STA�ṹ�� */
    mac_ether_header_stru *pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);    /* ��̫��ͷ */
    uint32_t ret;
    uint8_t *puc_ether_payload = NULL;

    MAC_GET_CB_TX_VAP_INDEX(pst_tx_ctl) = pst_vap->st_vap_base_info.uc_vap_id;

    pst_user = (hmac_user_stru *)mac_res_get_hmac_user(MAC_GET_CB_TX_USER_IDX(pst_tx_ctl));
    if (pst_user == NULL) {
        oam_stat_vap_incr(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
        return HMAC_TX_DROP_USER_NULL;
    }

    if (oal_byteorder_host_to_net_uint16(ETHER_TYPE_ARP) == pst_ether_hdr->us_ether_type) {
        pst_ether_hdr++;
        puc_ether_payload = (uint8_t *)pst_ether_hdr;
        /* The source MAC address is modified only if the packet is an   */
        /* ARP Request or a Response. The appropriate bytes are checked. */
        /* Type field (2 bytes): ARP Request (1) or an ARP Response (2)  */
        if ((puc_ether_payload[BYTE_OFFSET_6] == 0x00) &&
            (puc_ether_payload[BYTE_OFFSET_7] == 0x02 || puc_ether_payload[BYTE_OFFSET_7] == 0x01)) {
            /* Set Address2 field in the WLAN Header with source address */
            oal_set_mac_addr(puc_ether_payload + BYTE_OFFSET_8, mac_mib_get_StationID(&pst_vap->st_vap_base_info));
        }
    }

    ret = hmac_tx_ucast_process(pst_vap, pst_buf, pst_user, pst_tx_ctl);
    if (oal_unlikely(ret != HMAC_TX_PASS)) {
        return ret;
    }

    /* ��װ802.11ͷ */
    ret = hmac_tx_encap_ieee80211_header(pst_vap, pst_user, pst_buf);
    if (oal_unlikely((ret != OAL_SUCC))) {
        oam_warning_log1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
                         "{hmac_tx_lan_mpdu_process_sta::failed[%d].}", ret);
        oam_stat_vap_incr(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
        return HMAC_TX_DROP_80211_ENCAP_FAIL;
    }

    return HMAC_TX_PASS;
}

#ifdef _PRE_WINDOWS_SUPPORT
/*
 * �� �� ��  : hmac_tx_encap_fips
 * ��������  : ��ȡwdi 80211ͷ��Ϣ���췢��80211macͷ
 * 1.��    ��  : 2021��01��28��
 *   �޸�����  : �����ɺ���
 */
/*lint -e695*/
static uint32_t hmac_tx_encap_fips(hmac_vap_stru *hmac_vap, hmac_user_stru *user, oal_netbuf_stru *netbuf)
{
    uint8_t *mac80211_hdr = NULL;
    mac_tx_ctl_stru *tx_ctl = NULL;
    mac_ieee80211_frame_stru *head;
    uint8_t mac80211_hdr_old[MAC_80211_QOS_HTC_4ADDR_FRAME_LEN] = { 0 };
    uint32_t qos = HMAC_TX_BSS_NOQOS;
    uint32_t ret;

    /* FIPSģʽ��ȡ802.11ͷ */
    if (memcpy_s(&mac80211_hdr_old, MAC_80211_QOS_HTC_4ADDR_FRAME_LEN,
                 oal_netbuf_data(netbuf), netbuf->head_len) != EOK) {
        oam_error_log0(0, OAM_SF_TX, "hmac_tx_encap_fips:: MAC_80211 memcpy fail!");
    }
    tx_ctl = (mac_tx_ctl_stru *)(oal_netbuf_cb(netbuf));

    /* �ָ�dataָ�뵽80211 macͷ�󣬻���llc header�����Ǽ���CCMP key */
    oal_netbuf_pull(netbuf, netbuf->head_len);
    MAC_GET_CB_MPDU_LEN(tx_ctl) = (uint16_t)oal_netbuf_get_len(netbuf) - netbuf->head_len;

    ret = hmac_tx_encap_mac_hdr(hmac_vap, tx_ctl, netbuf, &mac80211_hdr);
    if (oal_unlikely(ret != OAL_SUCC)) {
        return ret;
    }

    /* ���鲥֡����ȡ�û���QOS����λ��Ϣ */
    if (MAC_GET_CB_IS_MCAST(tx_ctl) == FALSE) {
        /* �����û��ṹ���cap_info���ж��Ƿ���QOSվ�� */
        qos = user->st_user_base_info.st_cap_info.bit_qos;
    }
    /* ����֡���� */
    hmac_tx_set_frame_ctrl(qos, tx_ctl, (mac_ieee80211_qos_htc_frame_addr4_stru *)mac80211_hdr);
    hmac_tx_switch_addresses_fips(tx_ctl,
        (mac_ieee80211_qos_htc_frame_addr4_stru *)mac80211_hdr_old,
        (mac_ieee80211_qos_htc_frame_addr4_stru *)mac80211_hdr);
    ((mac_ieee80211_qos_htc_frame_addr4_stru *)mac80211_hdr)->st_frame_control.bit_protected_frame =
    ((mac_ieee80211_qos_htc_frame_addr4_stru *)mac80211_hdr_old)->st_frame_control.bit_protected_frame;

#if ((defined (PRE_WLAN_FEATURE_SNIFFER)) && (defined (CONFIG_HW_SNIFFER)))
    proc_sniffer_write_file((const uint8_t *)head, MAC_80211_QOS_FRAME_LEN,
                            (const uint8_t *)oal_netbuf_data(netbuf), oal_netbuf_len(netbuf), 1);
#endif
    return ret;
}
/*lint +e695*/
/*
 * �� �� ��  : hmac_tx_lan_mpdu_process_fips
 * ��������  : hmac ����һ��mpdu�Ĺؼ�֡ʶ��ά��
 * 1.��    ��  : 2018��9��19��
 *   �޸�����  : �����ɺ���
 */
/*lint -e695*/
static uint32_t hmac_tx_lan_mpdu_process_fips(hmac_vap_stru *hmac_vap, oal_netbuf_stru *netbuf,
    mac_tx_ctl_stru *tx_ctl)
{
    hmac_user_stru *user = NULL;
    uint16_t user_idx = hmac_vap->st_vap_base_info.us_assoc_vap_id;
    uint32_t ret;
    uint8_t tid;

    user = (hmac_user_stru *)mac_res_get_hmac_user(user_idx);
    if (user == NULL) {
        oam_stat_vap_incr(hmac_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
        return HMAC_TX_DROP_USER_NULL;
    }

    MAC_GET_CB_TX_VAP_INDEX(tx_ctl) = hmac_vap->st_vap_base_info.uc_vap_id;
    MAC_GET_CB_FRAME_SUBTYPE(tx_ctl) = MAC_DATA_BUTT;
    MAC_GET_CB_TX_USER_IDX(tx_ctl) = user_idx;
    tid = WLAN_TIDNO_BEST_EFFORT;
    MAC_GET_CB_WME_TID_TYPE(tx_ctl) = tid;
    MAC_GET_CB_WME_AC_TYPE(tx_ctl) = WLAN_WME_TID_TO_AC(tid);

    /* ��װ802.11ͷ */
    ret = hmac_tx_encap_fips(hmac_vap, user, netbuf);
    if (oal_unlikely((ret != OAL_SUCC))) {
        oam_warning_log1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
                         "{hmac_tx_lan_mpdu_process_fips::failed[%d].}", ret);
        oam_stat_vap_incr(hmac_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
        return HMAC_TX_DROP_80211_ENCAP_FAIL;
    }

    return HMAC_TX_PASS;
}
#endif

OAL_STATIC OAL_INLINE uint32_t hmac_tx_data_mcast_process_ap(hmac_vap_stru *hmac_vap, mac_tx_ctl_stru *tx_ctl,
    hmac_user_stru **hmac_user)
{
    /* �����鲥��ʶλ */
    MAC_GET_CB_IS_MCAST(tx_ctl) = OAL_TRUE;

    /* ����ACK���� */
    MAC_GET_CB_ACK_POLACY(tx_ctl) = WLAN_TX_NO_ACK;

    /* ��ȡ�鲥�û� */
    *hmac_user = mac_res_get_hmac_user(hmac_vap->st_vap_base_info.us_multi_user_idx);
    if (oal_unlikely(*hmac_user == NULL)) {
        oam_warning_log1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
            "{hmac_tx_lan_mpdu_process_ap::get multi user failed[%d].}",
            hmac_vap->st_vap_base_info.us_multi_user_idx);
        oam_stat_vap_incr(hmac_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
        return HMAC_TX_DROP_MUSER_NULL;
    }

    MAC_GET_CB_WME_TID_TYPE(tx_ctl) = WLAN_TIDNO_BCAST;
    MAC_GET_CB_WME_AC_TYPE(tx_ctl) = WLAN_WME_TID_TO_AC(WLAN_TIDNO_BCAST);
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
    /* ����鲥�ۺϴ򿪣��鲥֡��tid2 */
    if (mac_get_mcast_ampdu_switch() == OAL_TRUE) {
        MAC_GET_CB_WME_TID_TYPE(tx_ctl) = WLAN_TIDNO_MCAST_AMPDU;
        MAC_GET_CB_WME_AC_TYPE(tx_ctl) = WLAN_WME_TID_TO_AC(WLAN_TIDNO_MCAST_AMPDU);
    }
#endif
    return OAL_SUCC;
}

OAL_STATIC OAL_INLINE uint32_t hmac_tx_lan_mpdu_process_ap(hmac_vap_stru *pst_vap,
    oal_netbuf_stru *pst_buf, mac_tx_ctl_stru *pst_tx_ctl)
{
    hmac_user_stru *pst_user = NULL; /* Ŀ��STA�ṹ�� */
    mac_ether_header_stru *pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_buf); /* ��̫��ͷ */
    uint8_t *puc_addr = pst_ether_hdr->auc_ether_dhost; /* Ŀ�ĵ�ַ */
    uint32_t ret;
    uint16_t us_user_idx = MAC_GET_CB_TX_USER_IDX(pst_tx_ctl);

    /* ��������֡ */
    if (oal_likely(!ether_is_multicast(puc_addr))) {
        if (oal_unlikely(us_user_idx == g_wlan_spec_cfg->invalid_user_id)) {
            oam_stat_vap_incr(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
            return HMAC_TX_DROP_USER_UNKNOWN;
        }

        /* ת��HMAC��USER�ṹ�� */
        pst_user = (hmac_user_stru *)mac_res_get_hmac_user(us_user_idx);
        if (oal_unlikely(pst_user == NULL)) {
            oam_stat_vap_incr(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
            return HMAC_TX_DROP_USER_NULL;
        }

        /* �û�״̬�ж� */
        if (oal_unlikely(pst_user->st_user_base_info.en_user_asoc_state != MAC_USER_STATE_ASSOC)) {
            oam_stat_vap_incr(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
            return HMAC_TX_DROP_USER_INACTIVE;
        }

        ret = hmac_tx_ucast_process(pst_vap, pst_buf, pst_user, pst_tx_ctl);
        if (oal_unlikely(ret != HMAC_TX_PASS)) {
            return ret;
        }
    } else { /* �鲥 or �㲥 */
        ret = hmac_tx_data_mcast_process_ap(pst_vap, pst_tx_ctl, &pst_user);
        if (ret != OAL_SUCC) {
            return ret;
        }
        wlan_chip_tx_pt_mcast_set_cb(pst_vap, pst_ether_hdr, pst_tx_ctl);
    }

    /* ��װ802.11ͷ */
    ret = hmac_tx_encap_ieee80211_header(pst_vap, pst_user, pst_buf);
    if (oal_unlikely((ret != OAL_SUCC))) {
        oam_warning_log1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
                         "{hmac_tx_lan_mpdu_process_ap::failed[%d].}", ret);
        oam_stat_vap_incr(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
        return HMAC_TX_DROP_80211_ENCAP_FAIL;
    }

    return HMAC_TX_PASS;
}


void hmac_tx_vip_info(mac_vap_stru *pst_vap, uint8_t uc_data_type,
    oal_netbuf_stru *pst_buf, mac_tx_ctl_stru *pst_tx_ctl)
{
    mac_eapol_type_enum_uint8 en_eapol_type;
    uint8_t uc_dhcp_type;
    mac_ieee80211_qos_htc_frame_addr4_stru *pst_mac_header = NULL;
    mac_llc_snap_stru *pst_llc = NULL;
    oal_ip_header_stru *pst_rx_ip_hdr = NULL;
    oal_eth_arphdr_stru *puc_arp_head = NULL;
    int32_t l_ret = EOK;

    uint8_t auc_ar_sip[ETH_SENDER_IP_ADDR_LEN]; /* sender IP address */
    uint8_t auc_ar_dip[ETH_SENDER_IP_ADDR_LEN]; /* sender IP address */

    /* ����skb�Ѿ���װ80211ͷ */
    /* ��ȡLLC SNAP */
    pst_llc = (mac_llc_snap_stru *)oal_netbuf_data(pst_buf);
    pst_mac_header = (mac_ieee80211_qos_htc_frame_addr4_stru *)MAC_GET_CB_FRAME_HEADER_ADDR(pst_tx_ctl);
    if (pst_mac_header == NULL) {
        return;
    }
    if (uc_data_type == MAC_DATA_EAPOL) {
        en_eapol_type = mac_get_eapol_key_type((uint8_t *)(pst_llc + 1));
        oam_warning_log2(pst_vap->uc_vap_id, OAM_SF_ANY, "{hmac_tx_vip_info::EAPOL type=%u, len==%u} \
            [1:1/4 2:2/4 3:3/4 4:4/4]", en_eapol_type, oal_netbuf_len(pst_buf));
    } else if (uc_data_type == MAC_DATA_DHCP) {
        pst_rx_ip_hdr = (oal_ip_header_stru *)(pst_llc + 1);

        l_ret += memcpy_s((uint8_t *)auc_ar_sip, ETH_SENDER_IP_ADDR_LEN,
                          (uint8_t *)&pst_rx_ip_hdr->saddr, sizeof(uint32_t));
        l_ret += memcpy_s((uint8_t *)auc_ar_dip, ETH_SENDER_IP_ADDR_LEN,
                          (uint8_t *)&pst_rx_ip_hdr->daddr, sizeof(uint32_t));
        if (l_ret != EOK) {
            oam_error_log0(0, OAM_SF_ANY, "hmac_tx_vip_info::memcpy fail!");
            return;
        }

        uc_dhcp_type = mac_get_dhcp_frame_type(pst_rx_ip_hdr, MAC_GET_CB_MPDU_LEN(pst_tx_ctl) - SNAP_LLC_FRAME_LEN);
        hmac_ht_self_cure_event_set(pst_vap, pst_mac_header->auc_address1, HMAC_HT_SELF_CURE_EVENT_TX_DHCP_FRAME);
        oam_warning_log1(pst_vap->uc_vap_id, OAM_SF_ANY, "{hmac_tx_vip_info::DHCP type=%d.[1:discovery 2:offer \
            3:request 4:decline 5:ack 6:nack 7:release 8:inform]", uc_dhcp_type);
        oam_warning_log4(pst_vap->uc_vap_id, OAM_SF_ANY, "{hmac_tx_vip_info:: DHCP sip: %d.%d, dip: %d.%d.",
            auc_ar_sip[BYTE_OFFSET_2], auc_ar_sip[BYTE_OFFSET_3], auc_ar_dip[BYTE_OFFSET_2], auc_ar_dip[BYTE_OFFSET_3]);
    } else {
        puc_arp_head = (oal_eth_arphdr_stru *)(pst_llc + 1);
        oam_warning_log1(pst_vap->uc_vap_id, OAM_SF_ANY, "{hmac_tx_vip_info:: ARP type=%d.[2:arp resp 3:arp req.]",
                         uc_data_type);
        oam_warning_log4(pst_vap->uc_vap_id, OAM_SF_ANY, "{hmac_tx_lan_mpdu_info:: ARP sip: %d.%d, dip: %d.%d",
                         puc_arp_head->auc_ar_sip[BYTE_OFFSET_2], puc_arp_head->auc_ar_sip[BYTE_OFFSET_3],
                         puc_arp_head->auc_ar_tip[BYTE_OFFSET_2], puc_arp_head->auc_ar_tip[BYTE_OFFSET_3]);
    }

    oam_warning_log4(pst_vap->uc_vap_id, OAM_SF_ANY, "{hmac_tx_vip_info::send to wlan smac: %x:%x, dmac: %x:%x]",
        pst_mac_header->auc_address2[MAC_ADDR_4], pst_mac_header->auc_address2[MAC_ADDR_5],
        pst_mac_header->auc_address1[MAC_ADDR_4], pst_mac_header->auc_address1[MAC_ADDR_5]);
}
void hmac_tx_rtsp_info(hmac_vap_stru *hmac_vap, uint8_t data_type, oal_netbuf_stru *pst_buf)
{
    if (pst_buf == NULL || hmac_vap == NULL) {
        return;
    }
    if (data_type == MAC_DATA_RTSP) {
        oam_warning_log1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_tx_rtsp_info::rtsp len==%d}",
            oal_netbuf_len(pst_buf));
    }
}


OAL_STATIC void hmac_tx_cb_field_init(mac_tx_ctl_stru *pst_tx_ctl, mac_vap_stru *pst_vap, uint16_t us_user_idx)
{
    MAC_GET_CB_MPDU_NUM(pst_tx_ctl) = 1;
    MAC_GET_CB_NETBUF_NUM(pst_tx_ctl) = 1;
    MAC_GET_CB_WLAN_FRAME_TYPE(pst_tx_ctl) = WLAN_DATA_BASICTYPE;
    MAC_GET_CB_ACK_POLACY(pst_tx_ctl) = WLAN_TX_NORMAL_ACK;
    MAC_GET_CB_TX_VAP_INDEX(pst_tx_ctl) = pst_vap->uc_vap_id;
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl) = us_user_idx;
    MAC_GET_CB_WME_AC_TYPE(pst_tx_ctl) = WLAN_WME_AC_VO;
    MAC_GET_CB_WME_TID_TYPE(pst_tx_ctl) = WLAN_TIDNO_VOICE;
    MAC_GET_CB_FRAME_TYPE(pst_tx_ctl) = WLAN_CB_FRAME_TYPE_DATA;
    MAC_GET_CB_FRAME_SUBTYPE(pst_tx_ctl) = MAC_DATA_NUM; /* ����ʶ��ģ��ˢ�� */
}

static uint32_t hmac_tx_lan_to_wlan_no_tcp_opt_tx_pass_handle(mac_vap_stru *pst_vap,
    oal_netbuf_stru *pst_buf, mac_tx_ctl_stru *pst_tx_ctl)
{
    frw_event_stru *pst_event = NULL; /* �¼��ṹ�� */
    frw_event_mem_stru *pst_event_mem = NULL;
    dmac_tx_event_stru *pst_dtx_stru = NULL;
    uint8_t data_type = pst_tx_ctl->uc_frame_subtype;
    hmac_vap_stru *pst_hmac_vap = NULL;
    uint32_t ret;

    if (pst_buf == NULL || pst_vap == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_vap->uc_vap_id); /* VAP�ṹ�� */
    /* ά�⣬���һ���ؼ�֡��ӡ */
    if (data_type <= MAC_DATA_ARP_REQ) {
        hmac_tx_vip_info(pst_vap, data_type, pst_buf, pst_tx_ctl);
    }
    hmac_tx_rtsp_info(pst_hmac_vap, data_type, pst_buf);
#ifdef _PRE_WLAN_PKT_TIME_STAT
    if (DELAY_STATISTIC_SWITCH_ON && IS_NEED_RECORD_DELAY(pst_buf, TP_SKB_HMAC_XMIT)) {
        skbprobe_record_time(pst_buf, TP_SKB_HMAC_TX);
    }
#endif
    /* ���¼�������DMAC */
    pst_event_mem = frw_event_alloc_m(sizeof(dmac_tx_event_stru));
    if (oal_unlikely(pst_event_mem == NULL)) {
        oam_error_log0(pst_vap->uc_vap_id, OAM_SF_TX, "{hmac_tx_lan_to_wlan::frw_event_alloc_m failed.}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);

    /* ��д�¼�ͷ */
    frw_event_hdr_init(&(pst_event->st_event_hdr), FRW_EVENT_TYPE_HOST_DRX,
                       DMAC_TX_HOST_DRX, sizeof(dmac_tx_event_stru),
                       FRW_EVENT_PIPELINE_STAGE_1, pst_vap->uc_chip_id,
                       pst_vap->uc_device_id, pst_vap->uc_vap_id);

    pst_dtx_stru = (dmac_tx_event_stru *)pst_event->auc_event_data;
    pst_dtx_stru->pst_netbuf = pst_buf;
    pst_dtx_stru->us_frame_len = MAC_GET_CB_MPDU_LEN(pst_tx_ctl);

    /* �����¼� */
    ret = frw_event_dispatch_event(pst_event_mem);
    if (oal_unlikely(ret != OAL_SUCC)) {
        oam_warning_log1(pst_vap->uc_vap_id, OAM_SF_TX,
                         "{hmac_tx_lan_to_wlan::frw_event_dispatch_event failed[%d].}", ret);
        oam_stat_vap_incr(pst_vap->uc_vap_id, tx_abnormal_msdu_dropped, 1);
    }

    /* �ͷ��¼� */
    frw_event_free_m(pst_event_mem);

#ifdef _PRE_WLAN_RR_PERFORMENCE_DEBUG
    /* RR���ܼ�ⷢ������hmac to dmac λ�ô�� */
    hmac_rr_tx_h2d_timestamp();
#endif
    return ret;
}


/*lint -e695*/
uint32_t hmac_tx_lan_to_wlan_no_tcp_opt(mac_vap_stru *pst_vap, oal_netbuf_stru *pst_buf)
{
    uint32_t ret = HMAC_TX_PASS;
    mac_tx_ctl_stru *pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_buf);
    hmac_vap_stru *pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_vap->uc_vap_id);
    if (oal_unlikely(pst_hmac_vap == NULL)) {
        oam_error_log0(pst_vap->uc_vap_id, OAM_SF_TX, "{hmac_tx_lan_to_wlan_no_tcp_opt::vap/user null.}");
        oam_stat_vap_incr(pst_vap->uc_vap_id, tx_abnormal_msdu_dropped, 1);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* VAPģʽ�ж� */
    if (oal_unlikely(pst_vap->en_vap_mode != WLAN_VAP_MODE_BSS_AP && pst_vap->en_vap_mode != WLAN_VAP_MODE_BSS_STA)) {
        oam_warning_log1(pst_vap->uc_vap_id, OAM_SF_TX, "{no_tcp_opt::en_vap_mode=%d.}", pst_vap->en_vap_mode);
        oam_stat_vap_incr(pst_vap->uc_vap_id, tx_abnormal_msdu_dropped, 1);
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    /* ��������û�����Ϊ0���������� */
    if (oal_unlikely(pst_hmac_vap->st_vap_base_info.us_user_nums == 0)) {
        oam_stat_vap_incr(pst_vap->uc_vap_id, tx_abnormal_msdu_dropped, 1);
        return OAL_FAIL;
    }

    if (hmac_is_ring_tx(pst_vap, pst_buf, pst_tx_ctl)) {
        return hmac_ring_tx(pst_hmac_vap, pst_buf, pst_tx_ctl);
    }

#ifndef _PRE_WINDOWS_SUPPORT
    /* csumӲ����host��������֧�֣�device������Ҫ���������� */
    if (pst_buf->ip_summed == CHECKSUM_PARTIAL) {
        oal_skb_checksum_help(pst_buf);
    }
#endif

#ifdef _PRE_WLAN_FEATURE_SPECIAL_PKT_LOG
    hmac_parse_special_ipv4_packet(oal_netbuf_data(pst_buf), oal_netbuf_get_len(pst_buf), HMAC_PKT_DIRECTION_TX);
#endif

    /* ����LAN TO WLAN��WLAN TO WLAN��netbuf�������������Ϊ�����֣���Ҫ���ж�
       ��������������netbufȻ���ٶ�CB���¼������ֶθ�ֵ */
    if (MAC_GET_CB_EVENT_TYPE(pst_tx_ctl) != FRW_EVENT_TYPE_WLAN_DTX) {
        MAC_GET_CB_EVENT_TYPE(pst_tx_ctl) = FRW_EVENT_TYPE_HOST_DRX;
        MAC_GET_CB_EVENT_SUBTYPE(pst_tx_ctl) = DMAC_TX_HOST_DRX;
    }

    /* ����host�ඪ������ڴ���ͳ�� */
    hmac_tx_pkts_stat(1);
    /* �����ݰ�����ͳ�� */
    hmac_stat_device_tx_netbuf(oal_netbuf_len(pst_buf));

    oal_spin_lock_bh(&pst_hmac_vap->st_lock_state);
    if (pst_hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
        /*  ����ǰ MPDU */
        ret = hmac_tx_lan_mpdu_process_ap(pst_hmac_vap, pst_buf, pst_tx_ctl);
    } else if (pst_hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
        /* ����ǰMPDU */
#ifdef _PRE_WINDOWS_SUPPORT
        if (pst_hmac_vap->fips == TRUE) {
            ret = hmac_tx_lan_mpdu_process_fips(pst_hmac_vap, pst_buf, pst_tx_ctl);
        } else {
#endif
        ret = hmac_tx_lan_mpdu_process_sta(pst_hmac_vap, pst_buf, pst_tx_ctl);
#ifdef _PRE_WINDOWS_SUPPORT
        }
#endif

#ifdef _PRE_WLAN_FEATURE_WAPI
        if (ret == HMAC_TX_PASS && wlan_chip_is_support_hw_wapi() == OAL_FALSE) {
            hmac_wapi_stru *pst_wapi = NULL;
            mac_ieee80211_frame_stru *pst_mac_hdr = NULL;
            oal_bool_enum_uint8 en_is_mcast = OAL_FALSE;

            /* ��ȡwapi���� �鲥/���� */
            pst_mac_hdr = MAC_GET_CB_FRAME_HEADER_ADDR((mac_tx_ctl_stru *)oal_netbuf_cb(pst_buf));
            en_is_mcast = ether_is_multicast(pst_mac_hdr->auc_address1);
            /*lint -e730*/
            pst_wapi = hmac_user_get_wapi_ptr(pst_vap, !en_is_mcast, pst_vap->us_assoc_vap_id);
            if (pst_wapi == NULL) {
                oam_stat_vap_incr(pst_vap->uc_vap_id, tx_abnormal_msdu_dropped, 1);
                oam_warning_log0(pst_vap->us_assoc_vap_id, OAM_SF_ANY, "no_tcp_opt::get_wapi_ptr fail!}");
                oal_spin_unlock_bh(&pst_hmac_vap->st_lock_state);
                return OAL_ERR_CODE_PTR_NULL;
            }

            /*lint +e730*/
            if ((OAL_TRUE == WAPI_PORT_FLAG(pst_wapi)) &&
                (pst_wapi->wapi_netbuff_txhandle != NULL)) {
                pst_buf = pst_wapi->wapi_netbuff_txhandle(pst_wapi, pst_buf);
                if (pst_buf == NULL) {
                    oam_stat_vap_incr(pst_vap->uc_vap_id, tx_abnormal_msdu_dropped, 1);
                    oam_warning_log0(pst_vap->uc_vap_id, OAM_SF_ANY, "{hmac wapi_netbuff_txhandle fail!}");
                    oal_spin_unlock_bh(&pst_hmac_vap->st_lock_state);
                    return OAL_ERR_CODE_PTR_NULL;
                }
                /*  ����wapi�����޸�netbuff���˴���Ҫ���»�ȡһ��cb */
                pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_buf);
            }
        }
#endif /* #ifdef _PRE_WLAN_FEATURE_WAPI */
    }
    oal_spin_unlock_bh(&pst_hmac_vap->st_lock_state);

    if (oal_likely(ret == HMAC_TX_PASS)) {
        return hmac_tx_lan_to_wlan_no_tcp_opt_tx_pass_handle(pst_vap, pst_buf, pst_tx_ctl);
    } else if (oal_unlikely((ret == HMAC_TX_BUFF) || (ret == HMAC_TX_DONE))) {
        return OAL_SUCC;
    }
    oam_warning_log1(pst_vap->uc_vap_id, OAM_SF_TX, "{::HMAC_TX_DROP.reason[%d]!}", ret);
    return ret;
}
/*lint +e695*/
OAL_STATIC uint32_t hmac_transfer_tx_handler(hmac_vap_stru *hmac_vap, oal_netbuf_stru *netbuf)
{
#ifdef _PRE_WLAN_TCP_OPT
    mac_tx_ctl_stru *pst_tx_ctl = NULL;

#ifdef _PRE_WLAN_TCP_OPT_DEBUG
    oam_warning_log0(0, OAM_SF_TX, "{hmac_transfer_tx_handler::netbuf is tcp ack.}\r\n");
#endif
    oal_spin_lock_bh(&hmac_vap->st_hmac_tcp_ack[HCC_TX].data_queue_lock[HMAC_TCP_ACK_QUEUE]);
    oal_netbuf_list_tail(&hmac_vap->st_hmac_tcp_ack[HCC_TX].data_queue[HMAC_TCP_ACK_QUEUE], netbuf);

    /* ����TCP ACK�ȴ�����, ���ⱨ�����Ϸ��� */
    if (hmac_judge_tx_netbuf_is_tcp_ack((oal_ether_header_stru *)oal_netbuf_data(netbuf))) {
        pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(netbuf);
        MAC_GET_CB_TCP_ACK(pst_tx_ctl) = OAL_TRUE;
        oal_spin_unlock_bh(&hmac_vap->st_hmac_tcp_ack[HCC_TX].data_queue_lock[HMAC_TCP_ACK_QUEUE]);
        hmac_sched_transfer();
    } else {
        oal_spin_unlock_bh(&hmac_vap->st_hmac_tcp_ack[HCC_TX].data_queue_lock[HMAC_TCP_ACK_QUEUE]);
        hmac_tcp_ack_process();
    }
    return OAL_SUCC;
#endif
}


uint32_t hmac_tx_wlan_to_wlan_ap(oal_mem_stru *pst_event_mem)
{
    frw_event_stru *pst_event = NULL; /* �¼��ṹ�� */
    mac_vap_stru *pst_mac_vap = NULL;
    oal_netbuf_stru *pst_buf = NULL;     /* ��netbuf����ȡ������ָ��netbuf��ָ�� */
    oal_netbuf_stru *pst_buf_tmp = NULL; /* �ݴ�netbufָ�룬����whileѭ�� */
    mac_tx_ctl_stru *pst_tx_ctl = NULL;
    uint32_t ret;
    /* ����ж� */
    if (oal_unlikely(pst_event_mem == NULL)) {
        oam_error_log0(0, OAM_SF_TX, "{hmac_tx_wlan_to_wlan_ap::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* ��ȡ�¼� */
    pst_event = frw_get_event_stru(pst_event_mem);
    if (oal_unlikely(pst_event == NULL)) {
        oam_error_log0(0, OAM_SF_TX, "{hmac_tx_wlan_to_wlan_ap::pst_event null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡPAYLOAD�е�netbuf�� */
    pst_buf = (oal_netbuf_stru *)(uintptr_t)(*((unsigned long *)(pst_event->auc_event_data)));

    ret = hmac_tx_get_mac_vap(pst_event->st_event_hdr.uc_vap_id, &pst_mac_vap);
    if (oal_unlikely(ret != OAL_SUCC)) {
        oam_error_log1(pst_event->st_event_hdr.uc_vap_id, OAM_SF_TX,
                       "{hmac_tx_wlan_to_wlan_ap::hmac_tx_get_mac_vap failed[%d].}", ret);
        hmac_free_netbuf_list(pst_buf);
        return ret;
    }

    /* ѭ������ÿһ��netbuf��������̫��֡�ķ�ʽ���� */
    while (pst_buf != NULL) {
        pst_buf_tmp = oal_netbuf_next(pst_buf);

        oal_netbuf_next(pst_buf) = NULL;
        oal_netbuf_prev(pst_buf) = NULL;

        
        pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_buf);
        memset_s(pst_tx_ctl, sizeof(mac_tx_ctl_stru), 0, sizeof(mac_tx_ctl_stru));

        MAC_GET_CB_EVENT_TYPE(pst_tx_ctl) = FRW_EVENT_TYPE_WLAN_DTX;
        MAC_GET_CB_EVENT_SUBTYPE(pst_tx_ctl) = DMAC_TX_WLAN_DTX;

        /* set the queue map id when wlan to wlan */
        oal_skb_set_queue_mapping(pst_buf, WLAN_NORMAL_QUEUE);

        ret = hmac_tx_lan_to_wlan(pst_mac_vap, pst_buf);
        /* ����ʧ�ܣ��Լ������Լ��ͷ�netbuff�ڴ� */
        if (ret != OAL_SUCC) {
            hmac_free_netbuf_list(pst_buf);
        }

        pst_buf = pst_buf_tmp;
    }

    return OAL_SUCC;
}


void hmac_set_trx_switch(uint32_t trx_switch_ini_param)
{
    memset_s(&g_trx_switch, sizeof(g_trx_switch), 0, sizeof(g_trx_switch));
    /* �����Ƿ�֧��ring�л� */
    g_trx_switch.rx_switch = trx_switch_ini_param & BIT0;
    /* �����Ƿ�֧��ring */
    g_trx_switch.host_ring_tx_switch = (trx_switch_ini_param & BIT1) >> NUM_1_BITS;
    g_trx_switch.device_ring_tx_switch = (trx_switch_ini_param & BIT2) >> NUM_2_BITS;
    g_trx_switch.tx_ring_switch = (trx_switch_ini_param & BIT3) >> NUM_3_BITS;
    /* �ڴ��Ƿ�֧������Ӧ���� */
    g_trx_switch.netbuf_alloc_opt = (trx_switch_ini_param & BIT4) >> NUM_4_BITS;
    g_trx_switch.tx_netbuf_limit = (trx_switch_ini_param & BIT5) >> NUM_5_BITS;

    /* if support host ring, then set true */
    if (g_trx_switch.host_ring_tx_switch) {
        g_trx_switch.ring_switch_independent = OAL_TRUE; // ֧��host ring tx,��Ĭ��TRX�����л�
        g_trx_switch.host_update_wptr = OAL_TRUE; // ֧��host ring tx,��ʹ��pcie����
        g_trx_switch.soft_irq_sched = OAL_TRUE;   // ֧��host ring tx,��ʹ��softirq����; �����¼�����
        g_trx_switch.device_loop_sched = OAL_TRUE;  // ֧��host ring tx,��ʹ��loop����; �����¼�����
    }

    oam_warning_log3(0, OAM_SF_CFG, "{hmac_set_trx_switch::ini[0x%x], ring switch[%d] independent[%d]}",
        trx_switch_ini_param, g_trx_switch.tx_ring_switch, g_trx_switch.ring_switch_independent);
    oam_warning_log3(0, OAM_SF_CFG, "{hmac_set_trx_switch::wptr[%d] softirq[%d] loop[%d]}",
        g_trx_switch.host_update_wptr, g_trx_switch.soft_irq_sched, g_trx_switch.device_loop_sched);
}


oal_bool_enum_uint8 hmac_is_ring_tx(mac_vap_stru *mac_vap, oal_netbuf_stru *netbuf, mac_tx_ctl_stru *tx_ctl)
{
    mac_ether_header_stru *eth_hdr = NULL;
    oal_bool_enum_uint8 need_ring_tx = OAL_TRUE;

    if (hmac_ring_tx_enabled() != OAL_TRUE) {
        return OAL_FALSE;
    }

    eth_hdr = (mac_ether_header_stru *)oal_netbuf_data(netbuf);
    if (mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_STA) {
        /* APģʽ�ĵ���֧֡��ring tx, �鲥֡��֧��ring tx; STAģʽû���鲥֡ */
        need_ring_tx = !ether_is_multicast(eth_hdr->auc_ether_dhost);
    }
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
    /* ����鲥�ۺϴ򿪣����鲥������Ҫ��ring tx���̣�����tid6 */
    if (mac_get_mcast_ampdu_switch() == OAL_TRUE) {
        hmac_mcast_ampdu_netbuf_process(mac_vap, netbuf, tx_ctl, eth_hdr, &need_ring_tx);
    }
#endif
    return (MAC_GET_CB_WME_TID_TYPE(tx_ctl) != WLAN_TIDNO_BCAST) && need_ring_tx;
}


uint16_t hmac_tx_get_user_id(mac_vap_stru *mac_vap, mac_ether_header_stru *ether_hdr)
{
    uint16_t user_idx = g_wlan_spec_cfg->invalid_user_id;
    uint8_t *addr = NULL;                     /* Ŀ�ĵ�ַ */
    uint32_t ret;

    if (mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
        return mac_vap->us_assoc_vap_id;
    }

    if (mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
        /* �ж����鲥�򵥲�,����lan to wlan�ĵ���֡��������̫����ַ */
        addr = ether_hdr->auc_ether_dhost;
        /* ��������֡ */
        if (oal_likely(!ether_is_multicast(addr))) {
            ret = mac_vap_find_user_by_macaddr(mac_vap, addr, &user_idx);
            if (oal_unlikely(ret != OAL_SUCC)) {
                oam_warning_log4(mac_vap->uc_vap_id, OAM_SF_TX, "{hmac_get_tx_user_id::hmac_tx_find_user failed \
                    %2x:%2x:%2x:%2x}", addr[MAC_ADDR_2], addr[MAC_ADDR_3], addr[MAC_ADDR_4], addr[MAC_ADDR_5]);
                return g_wlan_spec_cfg->invalid_user_id;
            }
        } else { /* �鲥 or �㲥 */
            user_idx = mac_vap->us_multi_user_idx;
        }
    }

    return user_idx;
}


/*lint -e695*/
uint32_t hmac_tx_lan_to_wlan(mac_vap_stru *mac_vap, oal_netbuf_stru *netbuf)
{
    mac_ether_header_stru *ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(netbuf);
    mac_tx_ctl_stru *tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(netbuf);
    uint16_t us_user_idx = hmac_tx_get_user_id(mac_vap, ether_hdr);
    hmac_user_stru *hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(us_user_idx);
    hmac_vap_stru *hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id); /* VAP�ṹ�� */
    if (oal_unlikely((hmac_vap == NULL) || (hmac_user == NULL))) {
        oam_warning_log0(0, OAM_SF_TX, "{hmac_tx_lan_to_wlan::dmac_vap  or user null.}\r\n");
        return OAL_FAIL;
    }

    /* ��ʼ��CB tx rx�ֶ� , CB�ֶ���ǰ���Ѿ������㣬 �����ﲻ��Ҫ�ظ���ĳЩ�ֶθ���ֵ */
    hmac_tx_cb_field_init(tx_ctl, mac_vap, us_user_idx);
#ifdef _PRE_WLAN_CHBA_MGMT
    /* �û�֡ͳ�� */
    if (hmac_chba_vap_start_check(hmac_vap) == OAL_TRUE) {
        hmac_stat_user_tx_netbuf(hmac_user, netbuf);
    }
#endif
    /* ��̫��ҵ��ʶ�� tid/datatypeˢ�µ�cb�� */
    hmac_tx_classify(hmac_vap, hmac_user, netbuf, tx_ctl);

    if ((hmac_vap->sys_tcp_tx_ack_opt_enable == OAL_TRUE) &&
        (oal_netbuf_select_queue(netbuf) == WLAN_TCP_ACK_QUEUE)) {
        return hmac_transfer_tx_handler(hmac_vap, netbuf);
    } else {
        return hmac_tx_lan_to_wlan_no_tcp_opt(mac_vap, netbuf);
    }
}
/*lint +e695*/
static oal_net_dev_tx_enum hmac_bridge_vap_xmit_pre_get_res_check(oal_netbuf_stru *pst_buf,
    oal_net_device_stru *pst_dev, mac_vap_stru **pst_vap, hmac_vap_stru **pst_hmac_vap)
{
    /* ��ȡVAP�ṹ�� */
    *pst_vap = (mac_vap_stru *)oal_net_dev_priv(pst_dev);
    /* ���VAP�ṹ�岻���ڣ��������� */
    if (oal_unlikely(*pst_vap == NULL)) {
        /* will find vap fail when receive a pkt from
         * kernel while vap is deleted, return OAL_NETDEV_TX_OK is so. */
        oam_warning_log0(0, OAM_SF_TX, "{hmac_bridge_vap_xmit::pst_vap = NULL!}\r\n");
        oal_netbuf_free(pst_buf);
        oam_stat_vap_incr(0, tx_abnormal_msdu_dropped, 1);
        return OAL_NETDEV_TX_OK;
    }

    *pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap((*pst_vap)->uc_vap_id);
    if (*pst_hmac_vap == NULL) {
        oam_error_log0((*pst_vap)->uc_vap_id, OAM_SF_CFG, "{hmac_bridge_vap_xmit::pst_hmac_vap null.}");
        oal_netbuf_free(pst_buf);
        return OAL_NETDEV_TX_OK;
    }

    /* ��������ģ�pst_device_struָ���л�δ״̬, ���������л�δ����״̬ */
    if ((*pst_vap)->bit_al_tx_flag == OAL_SWITCH_ON) {
        oal_netbuf_free(pst_buf);
        return OAL_NETDEV_TX_OK;
    }
    return OAL_NETDEV_TX_BUSY;
}

static oal_bool_enum_uint8 hmac_bridge_vap_xmit_check_drop_frame(mac_vap_stru *vap,
    oal_netbuf_stru *buf)
{
    oal_bool_enum_uint8 drop_frame = OAL_FALSE;
    uint8_t data_type;

    if (vap->en_vap_state != MAC_VAP_STATE_STA_LISTEN) {
        drop_frame = OAL_TRUE;
    } else {
        data_type = mac_get_data_type_from_8023((uint8_t *)oal_netbuf_payload(buf),
            MAC_NETBUFF_PAYLOAD_ETH, oal_netbuf_get_len(buf));
        if ((data_type != MAC_DATA_EAPOL) && (data_type != MAC_DATA_DHCP)) {
            drop_frame = OAL_TRUE;
        } else {
            oam_warning_log2(vap->uc_vap_id, OAM_SF_TX,
                "{hmac_bridge_vap_xmit::donot drop [%d]frame[EAPOL:1,DHCP:0]. vap state[%d].}",
                data_type, vap->en_vap_state);
        }
    }

    return drop_frame;
}
#ifdef _PRE_WLAN_CHBA_MGMT
void hmac_bridge_vap_xmit_chba(oal_netbuf_stru *pst_buf, oal_net_device_stru *pst_dev,
    hmac_vap_stru *pst_hmac_vap, mac_vap_stru *pst_vap)
{
    hmac_user_stru *hmac_user = NULL;
    hmac_chba_vap_stru *chba_vap_info = hmac_get_up_chba_vap_info();
    oal_netbuf_stru *copy_buf = NULL;
    uint8_t *puc_addr; /* Ŀ�ĵ�ַ */
    mac_ether_header_stru *pst_ether_hdr = NULL;
    uint8_t user_bitmap_idx;

    if (chba_vap_info == NULL) {
        oal_netbuf_free(pst_buf);
        return;
    }
    for (user_bitmap_idx = 0; user_bitmap_idx < g_wlan_spec_cfg->max_user_limit; user_bitmap_idx++) {
        if ((pst_hmac_vap->user_id_bitmap & BIT(user_bitmap_idx)) == 0) {
            continue;
        }

        hmac_user = mac_res_get_hmac_user(user_bitmap_idx);
        if (hmac_user == NULL) {
            continue;
        }
        if (hmac_user->st_user_base_info.en_is_multi_user == OAL_TRUE) {
            continue;
        }
        /* ����netbuf */
        copy_buf = oal_netbuf_copy(pst_buf, GFP_ATOMIC);
        if (copy_buf == NULL) {
            oam_warning_log0(0, 0, "CHBA: pst_copy_buf is null");
            continue;
        }
        pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(copy_buf);
        puc_addr = pst_ether_hdr->auc_ether_dhost;
        /* ��user��mac��ַ���Ƹ�dhost��mac��ַ */
        memcpy_s(puc_addr, WLAN_MAC_ADDR_LEN, hmac_user->st_user_base_info.auc_user_mac_addr, WLAN_MAC_ADDR_LEN);

        if (oal_unlikely(hmac_tx_lan_to_wlan(pst_vap, copy_buf) != OAL_SUCC)) {
            /* ����ʧ�ܣ�Ҫ�ͷ��ں������netbuff�ڴ�� */
            oal_netbuf_free(copy_buf);
        }
    }
    /* �ͷ�netbuf */
    oal_netbuf_free(pst_buf);
}
#endif

static uint32_t hmac_bridge_vap_state_check(hmac_vap_stru *pst_hmac_vap, oal_netbuf_stru *pst_buf)
{
    uint8_t uc_data_type;
    mac_vap_stru *pst_vap = &(pst_hmac_vap->st_vap_base_info);
    /* ����VAP״̬������滥�⣬��Ҫ�������� */
    oal_spin_lock_bh(&pst_hmac_vap->st_lock_state);
    /* �ж�VAP��״̬�����ROAM���������� MAC_DATA_DHCP/MAC_DATA_ARP */
    if (pst_vap->en_vap_state == MAC_VAP_STATE_ROAMING) {
        uc_data_type = mac_get_data_type_from_8023((uint8_t *)oal_netbuf_payload(pst_buf),
            MAC_NETBUFF_PAYLOAD_ETH, oal_netbuf_get_len(pst_buf));
        if (uc_data_type != MAC_DATA_EAPOL) {
            if (uc_data_type != MAC_DATA_BUTT) {
                oam_warning_log1(pst_vap->uc_vap_id, OAM_SF_TX, "{vap_xmit::drop skb type %d!}", uc_data_type);
            }
            oal_netbuf_free(pst_buf);
            oam_stat_vap_incr(pst_vap->uc_vap_id, tx_abnormal_msdu_dropped, 1);
            oal_spin_unlock_bh(&pst_hmac_vap->st_lock_state);
            return OAL_FAIL;
        }
    } else if ((pst_vap->en_vap_state != MAC_VAP_STATE_UP) && (pst_vap->en_vap_state != MAC_VAP_STATE_PAUSE)) {
        /* ���������д���p2pɨ�費��dhcp��eapol֡����ֹ����ʧ�� */
        if (hmac_bridge_vap_xmit_check_drop_frame(pst_vap, pst_buf) == OAL_TRUE) {
            /* filter the tx xmit pkts print */
            if (pst_vap->en_vap_state == MAC_VAP_STATE_INIT || pst_vap->en_vap_state == MAC_VAP_STATE_BUTT) {
                oam_warning_log1(pst_vap->uc_vap_id, OAM_SF_TX,
                    "{hmac_bridge_vap_xmit::vap state[%d] != MAC_VAP_STATE_{UP|PAUSE}!}", pst_vap->en_vap_state);
            }
            oal_netbuf_free(pst_buf);
            oam_stat_vap_incr(pst_vap->uc_vap_id, tx_abnormal_msdu_dropped, 1);
            oal_spin_unlock_bh(&pst_hmac_vap->st_lock_state);
            return OAL_FAIL;
        }
    }
    oal_spin_unlock_bh(&pst_hmac_vap->st_lock_state);

    return OAL_SUCC;
}

/*
 * ��������  : �ҽӵ�VAP��Ӧnet_device�ṹ���µķ��ͺ���
 */
oal_net_dev_tx_enum hmac_bridge_vap_xmit(oal_netbuf_stru *pst_buf, oal_net_device_stru *pst_dev)
{
    mac_vap_stru *pst_vap = NULL;
    hmac_vap_stru *pst_hmac_vap = NULL;
    mac_ether_header_stru *ether_hdr = NULL;
#ifdef _PRE_SKB_TRACE
    mem_trace_add_node((uintptr_t)pst_buf);
#endif
    if (hmac_bridge_vap_xmit_pre_get_res_check(pst_buf, pst_dev, &pst_vap, &pst_hmac_vap) == OAL_NETDEV_TX_OK) {
        return OAL_NETDEV_TX_OK;
    }

    pst_buf = oal_netbuf_unshare(pst_buf, GFP_ATOMIC);
    if (oal_unlikely(pst_buf == NULL)) {
        return OAL_NETDEV_TX_OK;
    }

    if (hmac_bridge_vap_state_check(pst_hmac_vap, pst_buf) != OAL_SUCC) {
        return OAL_NETDEV_TX_OK;
    }

    oal_netbuf_next(pst_buf) = NULL;
    oal_netbuf_prev(pst_buf) = NULL;
#ifdef _PRE_WLAN_PKT_TIME_STAT
    if (DELAY_STATISTIC_SWITCH_ON && IS_NEED_RECORD_DELAY(pst_buf, TP_SKB_IP)) {
        skbprobe_record_time(pst_buf, TP_SKB_HMAC_XMIT);
    }
#endif
    memset_s(oal_netbuf_cb(pst_buf), oal_netbuf_cb_size(), 0, oal_netbuf_cb_size());
#ifdef _PRE_DELAY_DEBUG
    hmac_wifi_delay_tx_notify(pst_hmac_vap, pst_buf);
#endif
    if (IS_STA(pst_vap)) {
        /* ���ͷ����arp_req ͳ�ƺ��ع����Ĵ��� */
        hma_arp_probe_timer_start(pst_buf, pst_hmac_vap);
    }

    /* ����̫��������֡�ϱ�SDT */
    hmac_tx_report_eth_frame(pst_hmac_vap, pst_buf);

    /* ���鲥���chba mode */
#ifdef _PRE_WLAN_CHBA_MGMT
    ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);
    if (!(ether_is_multicast(ether_hdr->auc_ether_dhost)) || (!hmac_chba_vap_start_check(pst_hmac_vap))) {
#endif
        if (oal_unlikely(hmac_tx_lan_to_wlan(pst_vap, pst_buf) != OAL_SUCC)) {
            oal_netbuf_free(pst_buf);
        }
#ifdef _PRE_WLAN_CHBA_MGMT
    } else {
        hmac_bridge_vap_xmit_chba(pst_buf, pst_dev, pst_hmac_vap, pst_vap);
    }
#endif
    g_hisi_softwdt_check.xmit_cnt++;
    return OAL_NETDEV_TX_OK;
}
