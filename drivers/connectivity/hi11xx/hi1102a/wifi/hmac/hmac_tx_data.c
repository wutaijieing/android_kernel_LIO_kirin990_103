

/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "oal_net.h"
#include "frw_ext_if.h"
#include "hmac_tx_data.h"
#include "hmac_tx_amsdu.h"
#include "mac_frame.h"
#include "mac_data.h"
#include "hmac_frag.h"
#include "hmac_11i.h"

#ifdef _PRE_WLAN_FEATURE_WAPI
#include "hmac_wapi.h"
#endif

#include "hmac_traffic_classify.h"

#include "hmac_device.h"
#include "hmac_resource.h"

#include "hmac_tcp_opt.h"

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#include "hisi_customize_wifi.h"
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

#include "hmac_statistic_data_flow.h"
#include "plat_pm_wlan.h"

#ifdef _PRE_WLAN_FEATURE_SNIFFER
#include <hwnet/ipv4/sysctl_sniffer.h>
#endif
#include "hmac_config.h"
#include "securec.h"
#include "hmac_tx_opt.h"

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

#define PSM_PPS_VALUE_LOW 50

#define PPS_LEVEL_IDLE                    0
#define PPS_LEVEL_BUSY                    1
#define PSM_PPS_LEVEL_SUCCESSIVE_DOWN_CNT 3
/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/
#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
OAL_STATIC oal_uint8 g_uc_ac_new = 0;
#endif
mac_rx_dyn_bypass_extlna_stru g_st_rx_dyn_bypass_extlna_switch = { 0 };

mac_small_amsdu_switch_stru g_st_small_amsdu_switch = { 0 };

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
/* ���ڲ����������� */
/* STAģʽ��ֵΪOAL_TRUEʱ����BA�������Ǵӷ��͵�һ����������֡(�ؼ�����֡����)��ʼ�յ�5��֡�����ۺ�
   ֵΪOAL_FALSEʱ��100ms����������5����������֡(�ؼ�����֡����)��ſ�ʼ�����ۺ� */
oal_uint32 g_ul_tx_ba_policy_select = OAL_TRUE;
#endif

#ifdef WIN32
oal_uint8 g_wlan_fast_check_cnt;
oal_uint8 wlan_pm_get_fast_check_cnt(void)
{
    return g_wlan_fast_check_cnt;
}
void wlan_pm_set_fast_check_cnt(oal_uint8 fast_check_cnt)
{
    g_wlan_fast_check_cnt = fast_check_cnt;
}
#endif

/* �����������ͳ�ƴ�����pps�Ƿ���ϴδ���������ͬһ���ȼ� */
OAL_STATIC oal_uint8 g_uc_pps_level = 0;
/* ������¼�Ƿ�����3�����ڻή�͵���pps,��������˯ģʽ */
OAL_STATIC oal_uint8 g_uc_pps_level_adjust_cnt = 0;

/* deviceÿ20ms���һ��������g_wlan_fast_check_cnt�����������շ������͹���ģʽ */
oal_bool_enum_uint8 g_en_force_pass_filter = OAL_FALSE;
#ifdef _PRE_WLAN_FEATURE_APF
oal_uint16 g_us_apf_program_len = 0;
#endif
/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 hmac_tx_is_dhcp(mac_ether_header_stru *pst_ether_hdr)
{
    mac_ip_header_stru *puc_ip_hdr;

    puc_ip_hdr = (mac_ip_header_stru *)(pst_ether_hdr + 1);

    return mac_is_dhcp_port(puc_ip_hdr);
}


OAL_STATIC oal_void hmac_tx_report_dhcp_and_arp(mac_vap_stru *pst_mac_vap,
                                                mac_ether_header_stru *ether_hdr,
                                                oal_uint16 us_ether_len)
{
    oal_bool_enum_uint8 en_flg = OAL_FALSE;

    switch (OAL_HOST2NET_SHORT(ether_hdr->us_ether_type)) {
        case ETHER_TYPE_ARP:
            en_flg = OAL_TRUE;
            break;

        case ETHER_TYPE_IP:
            en_flg = hmac_tx_is_dhcp(ether_hdr);
            break;

        default:
            en_flg = OAL_FALSE;
            break;
    }

    if (en_flg && oam_report_dhcp_arp_get_switch()) {
        if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
            oam_report_eth_frame(ether_hdr->auc_ether_dhost, WLAN_MAC_ADDR_LEN, (oal_uint8 *)ether_hdr, us_ether_len,
                                 OAM_OTA_FRAME_DIRECTION_TYPE_TX);
        } else if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
            oam_report_eth_frame(pst_mac_vap->auc_bssid, WLAN_MAC_ADDR_LEN, (oal_uint8 *)ether_hdr, us_ether_len,
                                 OAM_OTA_FRAME_DIRECTION_TYPE_TX);
        } else {
        }
    }
}


oal_uint32 hmac_tx_report_eth_frame(mac_vap_stru *pst_mac_vap,
                                    oal_netbuf_stru *pst_netbuf)
{
    oal_uint16 us_user_idx = 0;
    mac_ether_header_stru *pst_ether_hdr = OAL_PTR_NULL;
    oal_uint32 ul_ret;
    oal_uint8 auc_user_macaddr[WLAN_MAC_ADDR_LEN] = { 0 };
    oal_switch_enum_uint8 en_eth_switch = 0;
#ifdef _PRE_WLAN_DFT_STAT
    hmac_vap_stru *pst_hmac_vap;
#endif

    if ((oal_unlikely(pst_mac_vap == OAL_PTR_NULL)) || (oal_unlikely(pst_netbuf == OAL_PTR_NULL))) {
        oam_error_log2(0, OAM_SF_TX, "{hmac_tx_report_eth_frame::input null %x %x}", (uintptr_t)pst_mac_vap,
                       (uintptr_t)pst_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }
#ifdef _PRE_WLAN_DFT_STAT
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (oal_unlikely(pst_hmac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(0, OAM_SF_TX, "{hmac_tx_report_eth_frame::mac_res_get_hmac_vap fail. vap_id = %u}",
                       pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

#endif

    /* ͳ����̫�����������ݰ�ͳ�� */
    /* ��ȡĿ���û���Դ��id���û�MAC��ַ�����ڹ��� */
    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
        pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_netbuf);
        if (oal_unlikely(pst_ether_hdr == OAL_PTR_NULL)) {
            OAM_ERROR_LOG0(0, OAM_SF_TX, "{hmac_tx_report_eth_frame::ether_hdr is null!\r\n");
            return OAL_ERR_CODE_PTR_NULL;
        }

        ul_ret = mac_vap_find_user_by_macaddr(pst_mac_vap,
            pst_ether_hdr->auc_ether_dhost, WLAN_MAC_ADDR_LEN, &us_user_idx);
        if (ul_ret == OAL_ERR_CODE_PTR_NULL) {
            OAM_ERROR_LOG1(0, OAM_SF_TX, "{hmac_tx_report_eth_frame::find user return ptr null!!\r\n", ul_ret);
            return OAL_ERR_CODE_PTR_NULL;
        }

        if (ul_ret == OAL_FAIL) {
            /* ����Ҳ����û�����֡������dhcp����arp request����Ҫ�ϱ� */
            hmac_tx_report_dhcp_and_arp(pst_mac_vap, pst_ether_hdr, (oal_uint16)oal_netbuf_len(pst_netbuf));
            return OAL_SUCC;
        }

        oal_set_mac_addr(auc_user_macaddr, pst_ether_hdr->auc_ether_dhost);
    }

    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
        if (pst_mac_vap->us_user_nums == 0) {
            return OAL_SUCC;
        }
#if 1
        pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_netbuf);
        if (oal_unlikely(pst_ether_hdr == OAL_PTR_NULL)) {
            OAM_ERROR_LOG0(0, OAM_SF_TX, "{hmac_tx_report_eth_frame::ether_hdr is null!\r\n");
            return OAL_ERR_CODE_PTR_NULL;
        }
        /* ����Ҳ����û�����֡������dhcp����arp request����Ҫ�ϱ� */
        hmac_tx_report_dhcp_and_arp(pst_mac_vap, pst_ether_hdr, (oal_uint16)oal_netbuf_len(pst_netbuf));

#endif
        us_user_idx = pst_mac_vap->uc_assoc_vap_id;
        oal_set_mac_addr(auc_user_macaddr, pst_mac_vap->auc_bssid);
    }

    /* ����ӡ��̫��֡�Ŀ��� */
    ul_ret = oam_report_eth_frame_get_switch(us_user_idx, OAM_OTA_FRAME_DIRECTION_TYPE_TX, &en_eth_switch);
    if (ul_ret != OAL_SUCC) {
        oam_warning_log0(0, OAM_SF_TX, "{hmac_tx_report_eth_frame::get tx eth frame switch fail!\r\n");
        return ul_ret;
    }

    if (en_eth_switch == OAL_SWITCH_ON) {
        /* ����̫��������֡�ϱ� */
        ul_ret = oam_report_eth_frame(auc_user_macaddr, WLAN_MAC_ADDR_LEN,
            oal_netbuf_data(pst_netbuf), (oal_uint16)oal_netbuf_len(pst_netbuf),
            OAM_OTA_FRAME_DIRECTION_TYPE_TX);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(0, OAM_SF_TX, "{hmac_tx_report_eth_frame::oam_report_eth_frame return err:0x%x.}", ul_ret);
        }
    }

    return OAL_SUCC;
}


oal_uint16 hmac_free_netbuf_list(oal_netbuf_stru *pst_buf)
{
    oal_netbuf_stru *pst_buf_tmp = OAL_PTR_NULL;
    mac_tx_ctl_stru *pst_tx_cb = OAL_PTR_NULL;
    oal_uint16 us_buf_num = 0;

    if (pst_buf != OAL_PTR_NULL) {
        pst_tx_cb = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_buf);

        while (pst_buf != OAL_PTR_NULL) {
            pst_buf_tmp = oal_netbuf_list_next(pst_buf);
            us_buf_num++;

            pst_tx_cb = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_buf);
            
            if ((oal_netbuf_headroom(pst_buf) < MAC_80211_QOS_HTC_4ADDR_FRAME_LEN) &&
                (pst_tx_cb->pst_frame_header != OAL_PTR_NULL)) {
                oal_mem_free_m(pst_tx_cb->pst_frame_header, OAL_TRUE);
                pst_tx_cb->pst_frame_header = OAL_PTR_NULL;
            }

            oal_netbuf_free(pst_buf);

            pst_buf = pst_buf_tmp;
        }
    } else {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_free_netbuf_list::pst_buf is null}");
    }

    return us_buf_num;
}

#ifdef _PRE_WLAN_FEATURE_HS20

oal_void hmac_tx_set_qos_map(oal_netbuf_stru *pst_buf, oal_uint8 *puc_tid)
{
    mac_ether_header_stru *pst_ether_header;
    mac_ip_header_stru *pst_ip;
    oal_uint8 uc_dscp;
    mac_tx_ctl_stru *pst_tx_ctl;
    hmac_vap_stru *pst_hmac_vap = OAL_PTR_NULL;
    oal_uint8 uc_idx;

    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_buf);
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(MAC_GET_CB_TX_VAP_INDEX(pst_tx_ctl));

    /* ��ȡ��̫��ͷ */
    pst_ether_header = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);
    /* �����Ϸ��Լ�� */
    if ((pst_hmac_vap == OAL_PTR_NULL) || (pst_ether_header == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_HS20, "{hmac_tx_set_qos_map::The input parameter is OAL_PTR_NULL.}");
        return;
    }

    /* ��IP TOS�ֶ�Ѱ��DSCP���ȼ� */
    /* ---------------------------------
      tosλ����
      ---------------------------------
    |    bit7~bit2      | bit1~bit0 |
    |    DSCP���ȼ�     | ����      |
    --------------------------------- */
    /* ƫ��һ����̫��ͷ��ȡipͷ */
    pst_ip = (mac_ip_header_stru *)(pst_ether_header + 1);
    uc_dscp = pst_ip->uc_tos >> WLAN_DSCP_PRI_SHIFT;

    if ((pst_hmac_vap->st_cfg_qos_map_param.uc_num_dscp_except > 0)
        && (pst_hmac_vap->st_cfg_qos_map_param.uc_num_dscp_except <= MAX_DSCP_EXCEPT) &&
        (pst_hmac_vap->st_cfg_qos_map_param.uc_valid)) {
        for (uc_idx = 0; uc_idx < pst_hmac_vap->st_cfg_qos_map_param.uc_num_dscp_except; uc_idx++) {
            if (uc_dscp == pst_hmac_vap->st_cfg_qos_map_param.auc_dscp_exception[uc_idx]) {
                *puc_tid = pst_hmac_vap->st_cfg_qos_map_param.auc_dscp_exception_up[uc_idx];
                pst_tx_ctl->bit_is_vipframe = OAL_TRUE;
                pst_tx_ctl->bit_is_needretry = OAL_TRUE;
                pst_hmac_vap->st_cfg_qos_map_param.uc_valid = 0;
                return;
            }
        }
    }

    for (uc_idx = 0; uc_idx < MAX_QOS_UP_RANGE; uc_idx++) {
        if ((uc_dscp < pst_hmac_vap->st_cfg_qos_map_param.auc_up_high[uc_idx]) &&
            (uc_dscp > pst_hmac_vap->st_cfg_qos_map_param.auc_up_low[uc_idx])) {
            *puc_tid = uc_idx;
            pst_tx_ctl->bit_is_vipframe = OAL_TRUE;
            pst_tx_ctl->bit_is_needretry = OAL_TRUE;
            pst_hmac_vap->st_cfg_qos_map_param.uc_valid = 0;
            return;
        } else {
            *puc_tid = 0;
        }
    }
    pst_hmac_vap->st_cfg_qos_map_param.uc_valid = 0;
    return;
}
#endif  // _PRE_WLAN_FEATURE_HS20

#ifdef _PRE_WLAN_FEATURE_CLASSIFY

OAL_STATIC oal_void hmac_tx_classify_tcp(hmac_vap_stru *pst_hmac_vap,
                                         mac_ip_header_stru *pst_ip,
                                         mac_tx_ctl_stru *pst_tx_ctl,
                                         oal_uint8 *puc_tid)
{
    mac_tcp_header_stru *pst_tcp_hdr;

    pst_tcp_hdr = (mac_tcp_header_stru *)(pst_ip + 1);

#ifdef _PRE_WLAN_FEATURE_SCHEDULE
    if ((oal_ntoh_16(pst_tcp_hdr->us_dport) == MAC_CHARIOT_NETIF_PORT) ||
        (oal_ntoh_16(pst_tcp_hdr->us_sport) == MAC_CHARIOT_NETIF_PORT)) {
        /* ����chariot����Ľ������⴦������ֹ���� */
        oam_info_log0(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::chariot netif tcp pkt.}");
        *puc_tid = WLAN_DATA_VIP_TID;
        pst_tx_ctl->bit_is_vipframe = OAL_TRUE;
        pst_tx_ctl->bit_is_needretry = OAL_TRUE;
    } else
#endif /* _PRE_WLAN_FEATURE_SCHEDULE */
        if (oal_netbuf_is_tcp_ack((oal_ip_header_stru *)pst_ip) == OAL_TRUE) {
            /* option3:SYN FIN RST URG��Ϊ1��ʱ�򲻻��� */
            if ((pst_tcp_hdr->uc_flags) & (FIN_FLAG_BIT | RESET_FLAG_BIT | URGENT_FLAG_BIT)) {
                pst_tx_ctl->bit_data_frame_type = MAC_DATA_URGENT_TCP_ACK;
            } else if ((pst_tcp_hdr->uc_flags) & SYN_FLAG_BIT) {
                MAC_GET_CB_IS_NEEDRETRY(pst_tx_ctl) = OAL_TRUE;
                pst_tx_ctl->bit_data_frame_type = MAC_DATA_TCP_SYN;
            } else {
                pst_tx_ctl->bit_data_frame_type = MAC_DATA_NORMAL_TCP_ACK;
            }
        } else if ((!IS_LEGACY_VAP(&(pst_hmac_vap->st_vap_base_info))) &&
                   (oal_ntoh_16(pst_tcp_hdr->us_sport) == MAC_WFD_RTSP_PORT)) {
            oam_info_log0(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::tx rtsp.}");
            MAC_GET_CB_FRAME_TYPE(pst_tx_ctl) = WLAN_DATA_BASICTYPE;
            MAC_GET_CB_IS_NEEDRETRY(pst_tx_ctl) = OAL_TRUE;
            pst_tx_ctl->bit_data_frame_type = MAC_DATA_RTSP;
            *puc_tid = WLAN_TIDNO_VOICE;
        } else {
            pst_tx_ctl->bit_data_frame_type = MAC_DATA_UNSPEC;
        }
}


OAL_STATIC OAL_INLINE oal_void hmac_tx_classify_lan_to_wlan(oal_netbuf_stru *pst_buf, oal_uint8 *puc_tid)
{
    mac_ether_header_stru *pst_ether_header = OAL_PTR_NULL;
    mac_ip_header_stru *pst_ip = OAL_PTR_NULL;
    oal_vlan_ethhdr_stru *pst_vlan_ethhdr = OAL_PTR_NULL;
    oal_uint32 ul_ipv6_hdr;
    oal_uint32 ul_pri;
    oal_uint16 us_vlan_tci;
    oal_uint8 uc_tid = 0;
    mac_tx_ctl_stru *pst_tx_ctl;
    mac_data_type_enum_uint8 uc_arp_type;
    hmac_vap_stru *pst_hmac_vap = OAL_PTR_NULL;
#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
    hmac_user_stru *pst_hmac_user = OAL_PTR_NULL;
#endif

    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_buf);

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(MAC_GET_CB_TX_VAP_INDEX(pst_tx_ctl));
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_WARNING_LOG1(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::mac_res_get_hmac_vap fail.vap_index[%u]}",
                         MAC_GET_CB_TX_VAP_INDEX(pst_tx_ctl));
        return;
    }

#ifdef CONFIG_NF_CONNTRACK_MARK
    /* the king of game feature will mark packets
 *and we will use VI queue to send these packets.
 */
    if (pktmark(pst_buf) == VIP_APP_MARK) {
        *puc_tid = VIP_APP_VIQUE_TID;
        pst_tx_ctl->bit_is_needretry = OAL_TRUE;
        return;
    }
#endif

    /* ��ȡ��̫��ͷ */
    pst_ether_header = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);

    switch (pst_ether_header->us_ether_type) {
            /* lint -e778 */ /* ����Info -- Constant expression evaluates to 0 in operation '&' */
        case OAL_HOST2NET_SHORT(ETHER_TYPE_IP):

#ifdef _PRE_WLAN_FEATURE_HS20
            if (pst_hmac_vap->st_cfg_qos_map_param.uc_valid) {
                hmac_tx_set_qos_map(pst_buf, &uc_tid);
                *puc_tid = uc_tid;
                return;
            }
#endif  // _PRE_WLAN_FEATURE_HS20

            /* ��IP TOS�ֶ�Ѱ�����ȼ� */
            /* ----------------------------------------------------------------------
                tosλ����
             ----------------------------------------------------------------------
            | bit7~bit5 | bit4 |  bit3  |  bit2  |   bit1   | bit0 |
            | �����ȼ�  | ʱ�� | ������ | �ɿ��� | ����ɱ� | ���� |
             ---------------------------------------------------------------------- */
            pst_ip = (mac_ip_header_stru *)(pst_ether_header + 1); /* ƫ��һ����̫��ͷ��ȡipͷ */

            uc_tid = pst_ip->uc_tos >> WLAN_IP_PRI_SHIFT;

            if (uc_tid != 0) {
                break;
            }
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
            /* RTP RTSP ����ֻ�� P2P�ϲſ���ʶ���� */
            if (!IS_LEGACY_VAP(&(pst_hmac_vap->st_vap_base_info)))
#endif
            {
                hmac_tx_traffic_classify(pst_tx_ctl, pst_ip, &uc_tid);
            }

            /* �����DHCP֡�������VO���з��� */
            if (mac_is_dhcp_port(pst_ip) == OAL_TRUE) {
                uc_tid = WLAN_DATA_VIP_TID;
                pst_tx_ctl->bit_is_vipframe = OAL_TRUE;
                pst_tx_ctl->bit_is_needretry = OAL_TRUE;
                pst_tx_ctl->bit_data_frame_type = MAC_DATA_DHCP;
            } else if (mac_is_dns_frame(pst_ip) == OAL_TRUE) {
                pst_tx_ctl->bit_is_needretry = OAL_TRUE;
                pst_tx_ctl->bit_data_frame_type = MAC_DATA_DNS;
            } else if (pst_ip->uc_protocol == MAC_TCP_PROTOCAL) {
                hmac_tx_classify_tcp(pst_hmac_vap, pst_ip, pst_tx_ctl, &uc_tid);
            }
#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
            pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(MAC_GET_CB_TX_USER_IDX(pst_tx_ctl));
            if (oal_unlikely(pst_hmac_user == OAL_PTR_NULL)) {
                OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_edca_opt_rx_pkts_stat::null param,pst_hmac_user[%d].}",
                                 MAC_GET_CB_TX_USER_IDX(pst_tx_ctl));
                break;
            }

            if ((pst_hmac_vap->uc_edca_opt_flag_ap == OAL_TRUE) &&
                (pst_hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_AP)) {
                /* mips�Ż�:�������ҵ��ͳ�����ܲ�10M���� */
                if (((pst_ip->uc_protocol == MAC_UDP_PROTOCAL) &&
                     (pst_hmac_user->aaul_txrx_data_stat[WLAN_WME_TID_TO_AC(uc_tid)][WLAN_TX_UDP_DATA] <
                      (HMAC_EDCA_OPT_PKT_NUM + 10))) || /* ƽ��ÿ���뱨�ĸ�����10 */
                    ((pst_ip->uc_protocol == MAC_TCP_PROTOCAL) &&
                     (pst_hmac_user->aaul_txrx_data_stat[WLAN_WME_TID_TO_AC(uc_tid)][WLAN_TX_TCP_DATA] <
                      (HMAC_EDCA_OPT_PKT_NUM + 10)))) { /* ƽ��ÿ���뱨�ĸ�����10 */
                    hmac_edca_opt_tx_pkts_stat(pst_tx_ctl, uc_tid, pst_ip);
                }
            }
#endif

            break;

        case OAL_HOST2NET_SHORT(ETHER_TYPE_IPV6):
            oam_info_log0(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::ETHER_TYPE_IPV6.}");
            /* ��IPv6 traffic class�ֶλ�ȡ���ȼ� */
            /* ----------------------------------------------------------------------
                IPv6��ͷ ǰ32Ϊ����
             -----------------------------------------------------------------------
            | �汾�� | traffic class   | ������ʶ |
            | 4bit   | 8bit(ͬipv4 tos)|  20bit   |
            ----------------------------------------------------------------------- */
            ul_ipv6_hdr = *((oal_uint32 *)(pst_ether_header + 1)); /* ƫ��һ����̫��ͷ��ȡipͷ */

            ul_pri = (OAL_NET2HOST_LONG(ul_ipv6_hdr) & WLAN_IPV6_PRIORITY_MASK) >> WLAN_IPV6_PRIORITY_SHIFT;

            uc_tid = (oal_uint8)(ul_pri >> WLAN_IP_PRI_SHIFT);
            oam_info_log1(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::uc_tid=%d.}", uc_tid);

            /* �����ND֡�������VO���з��� */
            if (mac_is_nd((oal_ipv6hdr_stru *)(pst_ether_header + 1)) == OAL_TRUE) {
                uc_tid = WLAN_DATA_VIP_TID;
                pst_tx_ctl->bit_is_vipframe = OAL_TRUE;
                pst_tx_ctl->bit_is_needretry = OAL_TRUE;
            } else if (mac_is_dhcp6((oal_ipv6hdr_stru *)(pst_ether_header + 1)) == OAL_TRUE) {
                oam_info_log0(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::ETHER_TYPE_DHCP6.}");
                uc_tid = WLAN_DATA_VIP_TID;
                pst_tx_ctl->bit_is_vipframe = OAL_TRUE;
                pst_tx_ctl->bit_is_needretry = OAL_TRUE;
                pst_tx_ctl->bit_data_frame_type = MAC_DATA_DHCPV6;
            }
            break;

        case OAL_HOST2NET_SHORT(ETHER_TYPE_PAE):
            /* �����EAPOL֡�������VO���з��� */
            oam_info_log0(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::ETHER_TYPE_PAE.}");
            uc_tid = WLAN_DATA_VIP_TID;
            pst_tx_ctl->bit_is_vipframe = OAL_TRUE;
            pst_tx_ctl->bit_is_needretry = OAL_TRUE;

            pst_tx_ctl->bit_data_frame_type = MAC_DATA_EAPOL;
            /* �����4 ���������õ�����Կ��������tx cb ��bit_is_eapol_key_ptk ��һ��dmac ���Ͳ����� */
            if (mac_is_eapol_key_ptk((mac_eapol_header_stru *)(pst_ether_header + 1)) == OAL_TRUE) {
                pst_tx_ctl->bit_is_eapol_key_ptk = OAL_TRUE;
            }

            oam_info_log1(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::uc_tid=%d.}", uc_tid);
            break;

        /* TDLS֡����������������������ȼ�TID���� */
        case OAL_HOST2NET_SHORT(ETHER_TYPE_TDLS):
            oam_info_log0(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::ETHER_TYPE_TDLS.}");
            uc_tid = WLAN_DATA_VIP_TID;
            oam_info_log1(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::uc_tid=%d.}", uc_tid);
            break;

        /* PPPOE֡��������������(���ֽ׶�, �Ự�׶�)��������ȼ�TID���� */
        case OAL_HOST2NET_SHORT(ETHER_TYPE_PPP_DISC):
        case OAL_HOST2NET_SHORT(ETHER_TYPE_PPP_SES):
            oam_info_log0(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::ETHER_TYPE_PPP_DISC, ETHER_TYPE_PPP_SES.}");
            uc_tid = WLAN_DATA_VIP_TID;
            pst_tx_ctl->bit_is_vipframe = OAL_TRUE;
            pst_tx_ctl->bit_is_needretry = OAL_TRUE;

            oam_info_log1(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::uc_tid=%d.}", uc_tid);
            break;

#ifdef _PRE_WLAN_FEATURE_WAPI
        case OAL_HOST2NET_SHORT(ETHER_TYPE_WAI):
            oam_info_log0(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::ETHER_TYPE_WAI.}");
            uc_tid = WLAN_DATA_VIP_TID;
            pst_tx_ctl->bit_is_vipframe = OAL_TRUE;
            pst_tx_ctl->bit_is_needretry = OAL_TRUE;
            break;
#endif

        case OAL_HOST2NET_SHORT(ETHER_TYPE_VLAN):
            oam_info_log0(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::ETHER_TYPE_VLAN.}");
            /* ��ȡvlan tag�����ȼ� */
            pst_vlan_ethhdr = (oal_vlan_ethhdr_stru *)oal_netbuf_data(pst_buf);

            /* ------------------------------------------------------------------
                802.1Q(VLAN) TCI(tag control information)λ����
             -------------------------------------------------------------------
            |Priority | DEI  | Vlan Identifier |
            | 3bit    | 1bit |      12bit      |
             ------------------------------------------------------------------ */
            us_vlan_tci = OAL_NET2HOST_SHORT(pst_vlan_ethhdr->h_vlan_TCI);

            uc_tid = us_vlan_tci >> OAL_VLAN_PRIO_SHIFT; /* ����13λ����ȡ��3λ���ȼ� */
            oam_info_log1(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::uc_tid=%d.}", uc_tid);

            break;

        case OAL_HOST2NET_SHORT(ETHER_TYPE_ARP):
            /* �����ARP֡�������VO���з��� */
            uc_tid = WLAN_DATA_VIP_TID;
            pst_tx_ctl->bit_is_vipframe = OAL_TRUE;
            uc_arp_type = mac_get_arp_type_by_arphdr((oal_eth_arphdr_stru *)(pst_ether_header + 1));
            pst_tx_ctl->bit_data_frame_type = uc_arp_type;
            oam_info_log1(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::ETHER_TYPE_ARP,uc_tid=%d.}", uc_tid);
            break;

        /*lint +e778*/
        default:
            oam_info_log1(0, OAM_SF_TX, "{hmac_tx_classify_lan_to_wlan::default us_ether_type[%d].}",
                          pst_ether_header->us_ether_type);
            break;
    }

    /* ���θ�ֵ */
    *puc_tid = uc_tid;
}


OAL_STATIC OAL_INLINE oal_void hmac_tx_update_tid(oal_bool_enum_uint8 en_wmm, oal_uint8 *puc_tid)
{
    if (oal_likely(en_wmm == OAL_TRUE)) {
        *puc_tid = (*puc_tid < WLAN_TIDNO_BUTT) ? wlan_tos_to_tid(*puc_tid) : WLAN_TIDNO_BCAST;
    } else { /* wmm��ʹ�� */
        *puc_tid = MAC_WMM_SWITCH_TID;
    }
}

#if defined(_PRE_PRODUCT_ID_HI110X_HOST)

oal_uint8 hmac_tx_wmm_acm(oal_bool_enum_uint8 en_wmm, hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_tid)
{
    oal_uint8 uc_ac;
    mac_vap_stru *pst_mac_vap;

    if ((pst_hmac_vap == OAL_PTR_NULL) || (puc_tid == OAL_PTR_NULL)) {
        return OAL_FALSE;
    }

    if (en_wmm == OAL_FALSE) {
        return OAL_FALSE;
    }

    uc_ac = WLAN_WME_TID_TO_AC(*puc_tid);
    g_uc_ac_new = uc_ac;
    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);
    while ((g_uc_ac_new != WLAN_WME_AC_BK) &&
           (pst_mac_vap->pst_mib_info->st_wlan_mib_qap_edac[g_uc_ac_new].en_dot11QAPEDCATableMandatory == OAL_TRUE)) {
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
#endif /* defined(_PRE_PRODUCT_ID_HI110X_HOST) */


OAL_STATIC OAL_INLINE oal_void hmac_tx_classify(hmac_vap_stru *pst_hmac_vap,
                                                mac_user_stru *pst_user,
                                                oal_netbuf_stru *pst_buf)
{
    oal_uint8 uc_tid = 0;
    mac_tx_ctl_stru *pst_tx_ctl = OAL_PTR_NULL;
    mac_device_stru *pst_mac_dev = OAL_PTR_NULL;
    mac_vap_stru *pst_vap = &(pst_hmac_vap->st_vap_base_info);
    hmac_tx_classify_lan_to_wlan(pst_buf, &uc_tid);

    /* ��QoSվ�㣬ֱ�ӷ��� */
    if (oal_unlikely(pst_user->st_cap_info.bit_qos != OAL_TRUE)) {
        oam_info_log0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
                      "{hmac_tx_classify::user is a none QoS station.}");
        return;
    }

    pst_mac_dev = mac_res_get_dev(pst_user->uc_device_id);
    if (oal_unlikely(pst_mac_dev == OAL_PTR_NULL)) {
        oam_warning_log0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX, "{hmac_tx_classify::pst_mac_dev null.}");
        return;
    }
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_buf);

#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
#ifdef _PRE_WLAN_FEATURE_WMMAC
    if (g_en_wmmac_switch) {
        oal_uint8 uc_ac_num;
        uc_ac_num = WLAN_WME_TID_TO_AC(uc_tid);
        /* ���ACMλΪ1���Ҷ�ӦAC��TSû�н����ɹ����򽫸�AC������ȫ���ŵ�BE���з��� */
        if ((pst_vap->pst_mib_info->st_wlan_mib_qap_edac[uc_ac_num].en_dot11QAPEDCATableMandatory == OAL_TRUE) &&
            (pst_user->st_ts_info[uc_ac_num].en_ts_status != MAC_TS_SUCCESS)) {
            uc_tid = WLAN_WME_AC_BE;
            if (pst_tx_ctl->bit_is_vipframe == OAL_TRUE) {
                uc_tid = WLAN_TIDNO_BEST_EFFORT;
                pst_tx_ctl->bit_is_vipframe = OAL_FALSE;
                pst_tx_ctl->bit_is_needretry = OAL_FALSE;
            }
        }
    } else
#endif  // _PRE_WLAN_FEATURE_WMMAC
    {
        hmac_tx_wmm_acm(pst_mac_dev->en_wmm, pst_hmac_vap, &uc_tid);
    }
#endif  // _PRE_PRODUCT_ID_HI110X_HOST
#ifdef _PRE_WLAN_FEATURE_WMMAC
    if (!g_en_wmmac_switch)
#endif  // _PRE_WLAN_FEATURE_WMMAC
    {
        
        if ((pst_tx_ctl->bit_is_vipframe != OAL_TRUE) || (pst_mac_dev->en_wmm == OAL_FALSE)) {
            hmac_tx_update_tid(pst_mac_dev->en_wmm, &uc_tid);
        }
    }

    /* ���ʹ����vap���ȼ�����������õ�vap���ȼ� */
    if (pst_mac_dev->en_vap_classify == OAL_TRUE) {
        uc_tid = pst_hmac_vap->uc_classify_tid;
    }

    /* ����ac��tid��cb�ֶ� */
    pst_tx_ctl->uc_tid = uc_tid;
    pst_tx_ctl->uc_ac = WLAN_WME_TID_TO_AC(uc_tid);

    oam_info_log2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX, "{hmac_tx_classify::uc_ac=%d uc_tid=%d.}",
                  pst_tx_ctl->uc_ac, pst_tx_ctl->uc_tid);

    return;
}
#endif


OAL_STATIC OAL_INLINE oal_uint32 hmac_tx_filter_security(hmac_vap_stru *pst_hmac_vap,
                                                         oal_netbuf_stru *pst_buf,
                                                         hmac_user_stru *pst_hmac_user)
{
    mac_ether_header_stru *pst_ether_header = OAL_PTR_NULL;
    mac_user_stru *pst_mac_user = OAL_PTR_NULL;
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    oal_uint32 ul_ret = OAL_SUCC;
    oal_uint16 us_ether_type;

    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);
    pst_mac_user = &(pst_hmac_user->st_user_base_info);

    if (mac_mib_get_rsnaactivated(pst_mac_vap) == OAL_TRUE) {
        if (pst_mac_user->en_port_valid != OAL_TRUE) {
            /* ��ȡ��̫��ͷ */
            pst_ether_header = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);
            /* ��������ʱ����Է�EAPOL ������֡������ */
            if (oal_byteorder_host_to_net_uint16(ETHER_TYPE_PAE) != pst_ether_header->us_ether_type) {
                us_ether_type = oal_byteorder_host_to_net_uint16(pst_ether_header->us_ether_type);
                oam_warning_log2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
                                 "{hmac_tx_filter_security::TYPE 0x%04x, 0x%04x.}",
                                 us_ether_type, ETHER_TYPE_PAE);
                ul_ret = OAL_FAIL;
            }
        }
    }

    return ul_ret;
}


oal_void hmac_tx_ba_setup(hmac_vap_stru *pst_hmac_vap,
                          hmac_user_stru *pst_user,
                          oal_uint8 uc_tidno)
{
    mac_action_mgmt_args_stru st_action_args; /* ������дACTION֡�Ĳ��� */
    oal_bool_enum_uint8 en_ampdu_support;

    /* ����BA�Ự���Ƿ���Ҫ�ж�VAP��AMPDU��֧���������Ϊ��Ҫʵ�ֽ���BA�Ựʱ��һ����AMPDU */
    en_ampdu_support = hmac_user_xht_support(pst_user);
    if (en_ampdu_support) {
        /*
        ����BA�Ựʱ��st_action_args�ṹ������Ա��������
        (1)uc_category:action�����
        (2)uc_action:BA action�µ����
        (3)ul_arg1:BA�Ự��Ӧ��TID
        (4)ul_arg2:BUFFER SIZE��С
        (5)ul_arg3:BA�Ự��ȷ�ϲ���
        (6)ul_arg4:TIMEOUTʱ��
 */
        st_action_args.uc_category = MAC_ACTION_CATEGORY_BA;
        st_action_args.uc_action = MAC_BA_ACTION_ADDBA_REQ;
        st_action_args.ul_arg1 = uc_tidno; /* ������֡��Ӧ��TID�� */
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
        st_action_args.ul_arg2 = (oal_uint32)g_wlan_customize.ul_ampdu_tx_max_num;
        OAM_WARNING_LOG1(0, OAM_SF_TX, "hisi_customize_wifi::[ba buffer size:%d]", st_action_args.ul_arg2);
#else
        st_action_args.ul_arg2 = WLAN_AMPDU_TX_MAX_BUF_SIZE; /* ADDBA_REQ�У�buffer_size��Ĭ�ϴ�С */
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

        st_action_args.ul_arg3 = MAC_BA_POLICY_IMMEDIATE; /* BA�Ự��ȷ�ϲ��� */
        st_action_args.ul_arg4 = 0;                       /* BA�Ự�ĳ�ʱʱ������Ϊ0 */

        /* ����BA�Ự */
        hmac_mgmt_tx_action(pst_hmac_vap, pst_user, &st_action_args);
    } else {
        if (pst_user->ast_tid_info[uc_tidno].st_ba_tx_info.en_ba_status != DMAC_BA_INIT) {
            st_action_args.uc_category = MAC_ACTION_CATEGORY_BA;
            st_action_args.uc_action = MAC_BA_ACTION_DELBA;
            st_action_args.ul_arg1 = uc_tidno;                                       /* ������֡��Ӧ��TID�� */
            st_action_args.ul_arg2 = MAC_ORIGINATOR_DELBA;                           /* ���Ͷ�ɾ��ba */
            st_action_args.ul_arg3 = MAC_UNSPEC_REASON;                              /* BA�Ựɾ��ԭ�� */
            st_action_args.puc_arg5 = pst_user->st_user_base_info.auc_user_mac_addr; /* �û�mac��ַ */

            /* ɾ��BA�Ự */
            hmac_mgmt_tx_action(pst_hmac_vap, pst_user, &st_action_args);
        }
    }
}


oal_uint32 hmac_tx_ucast_process(hmac_vap_stru *pst_hmac_vap,
                                 oal_netbuf_stru *pst_buf,
                                 hmac_user_stru *pst_user,
                                 mac_tx_ctl_stru *pst_tx_ctl)
{
    oal_uint32 ul_ret = HMAC_TX_PASS;

    /* ��ȫ���� */
#if defined(_PRE_WLAN_FEATURE_WPA) || defined(_PRE_WLAN_FEATURE_WPA2)
    if (oal_unlikely(hmac_tx_filter_security(pst_hmac_vap, pst_buf, pst_user) != OAL_SUCC)) {
        oam_stat_vap_incr(pst_hmac_vap->st_vap_base_info.uc_vap_id, tx_security_check_faild, 1);
        return HMAC_TX_DROP_SECURITY_FILTER;
    }
#endif

    /* ��̫��ҵ��ʶ�� */
#ifdef _PRE_WLAN_FEATURE_CLASSIFY
    hmac_tx_classify(pst_hmac_vap, &(pst_user->st_user_base_info), pst_buf);
#endif

    /* �����EAPOL��DHCP֡����������������BA�Ự */
    if (pst_tx_ctl->bit_is_vipframe == OAL_FALSE) {
#ifdef _PRE_WLAN_FEATURE_AMPDU
        if (hmac_tid_need_ba_session(pst_hmac_vap, pst_user, pst_tx_ctl->uc_tid, pst_buf) == OAL_TRUE) {
            /* �Զ���������BA�Ự������AMPDU�ۺϲ�����Ϣ��DMACģ��Ĵ���addba rsp֡��ʱ�̺��� */
            hmac_tx_ba_setup(pst_hmac_vap, pst_user, pst_tx_ctl->uc_tid);
        }
#endif

#ifdef _PRE_WLAN_FEATURE_AMSDU
        ul_ret = hmac_amsdu_notify(pst_hmac_vap, pst_user, pst_buf);
        if (oal_unlikely(ul_ret != HMAC_TX_PASS)) {
            return ul_ret;
        }
#endif
    }

    return ul_ret;
}


OAL_STATIC oal_bool_enum_uint8 hmac_tx_is_need_frag(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user,
                                                    oal_netbuf_stru *pst_netbuf, mac_tx_ctl_stru *pst_tx_ctl)
{
    oal_uint32 ul_threshold;
    /* �жϱ����Ƿ���Ҫ���з�Ƭ */
    /* 1�����ȴ������� */
    /* 2����legacЭ��ģʽ */
    /* 3�����ǹ㲥֡ */
    /* 4�����Ǿۺ�֡ */
    /* 6��DHCP֡�����з�Ƭ */
    if (pst_tx_ctl->bit_is_vipframe == OAL_TRUE) {
        return OAL_FALSE;
    }
    ul_threshold = pst_hmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_operation.ul_dot11FragmentationThreshold;
    ul_threshold = (ul_threshold & (~(BIT0 | BIT1))) + 2; /* ��ȡthreshold��ֵ��threshold����0��1bit��ȡ���ټ�2�� */

    return (ul_threshold < (oal_netbuf_len(pst_netbuf)) &&
            (!pst_tx_ctl->en_ismcast) && (!pst_tx_ctl->en_is_amsdu) &&
            ((pst_hmac_user->st_user_base_info.en_avail_protocol_mode < WLAN_HT_MODE) ||
             (pst_hmac_vap->st_vap_base_info.en_protocol < WLAN_HT_MODE))
            && (hmac_vap_ba_is_setup(pst_hmac_user, pst_tx_ctl->uc_tid) == OAL_FALSE));
}

#ifdef _PRE_WLAN_FEATURE_APF

oal_void hmac_auto_set_apf_switch_in_suspend(oal_uint32 ul_total_pps)
{
    mac_device_stru *pst_mac_device;
    oal_bool_enum_uint8 en_apf_switch = OAL_FALSE;
    mac_vap_stru *pst_mac_vap;
    mac_cfg_suspend_stru st_suspend;
    oal_uint32 ul_ret;

    pst_mac_device = mac_res_get_dev(0);
    /* ����ǵ�VAP,�򲻴��� */
    if (mac_device_calc_up_vap_num(pst_mac_device) != 1) {
        return;
    }

    if ((g_en_force_pass_filter == OAL_FALSE) || pst_mac_device->uc_in_suspend == OAL_FALSE
#ifdef _PRE_WLAN_FEATURE_WAPI
        || (pst_mac_device->uc_wapi == OAL_TRUE)
#endif
) {
        return;
    }

    /* ����ģʽ��ֻ�е�����ʱ�ſ���apf���ˣ�����رմ˹��� */
    if ((ul_total_pps < g_st_thread_bindcpu.us_throughput_pps_irq_low) &&
        (g_us_apf_program_len > OAL_SIZEOF(oal_uint8))) {
        en_apf_switch = OAL_TRUE;
    }

    if (en_apf_switch == pst_mac_device->en_apf_switch) {
        return;
    }
    /* ��Ҫ�л�ʱ������������֪ͨdevice���� */
    pst_mac_device->en_apf_switch = en_apf_switch;

    ul_ret = mac_device_find_up_vap(pst_mac_device, &pst_mac_vap);
    if ((ul_ret != OAL_SUCC) || (pst_mac_vap == OAL_PTR_NULL)) {
        return;
    }
    oam_warning_log3(0, OAM_SF_ANY, "{hmac_auto_set_apf_switch_in_suspend:1.en_apf_switch = [%d],2.cur_txrx_pps = [%d],\
                     3.pps_low_th = [%d]! send to device!}",
                     en_apf_switch, ul_total_pps, g_st_thread_bindcpu.us_throughput_pps_irq_low);

    st_suspend.uc_apf_switch = en_apf_switch;
    st_suspend.uc_in_suspend = pst_mac_device->uc_in_suspend;
    st_suspend.uc_arpoffload_switch = pst_mac_device->uc_arpoffload_switch;
    /***************************************************************************
        ���¼���DMAC��, ͬ����Ļ����״̬��DMAC
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_APF_SWITCH_SYN, OAL_SIZEOF(mac_cfg_suspend_stru),
                                    (oal_uint8 *)&st_suspend);
    if (oal_unlikely(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_auto_set_apf_switch_in_suspend::send_event failed[%d]}", ul_ret);
    }
}
#endif


oal_void hmac_set_psm_activity_timer(oal_uint32 ul_total_pps)
{
    oal_uint8 uc_current_pps_level = PPS_LEVEL_IDLE;
    oal_uint32 ul_pm_timer_restart_cnt = HMAC_PSM_TIMER_MIDIUM_CNT;
    oal_uint32 ul_ret;
    mac_device_stru *pst_mac_device = OAL_PTR_NULL;
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    mac_cfg_ps_params_stru st_ps_params;

    /* ֻ������æ���У����⵵λ̫�࣬���ض��� */
    if (ul_total_pps < PSM_PPS_VALUE_LOW) {
        uc_current_pps_level = PPS_LEVEL_IDLE;
#ifdef _PRE_WLAN_DOWNLOAD_PM
        ul_pm_timer_restart_cnt = wlan_pm_get_download_rate_limit_pps() ? 1 : wlan_pm_get_fast_check_cnt();
#else
        ul_pm_timer_restart_cnt = wlan_pm_get_fast_check_cnt();
#endif
    } else {
        uc_current_pps_level = PPS_LEVEL_BUSY;
        ul_pm_timer_restart_cnt = HMAC_PSM_TIMER_BUSY_CNT;
    }

    /* ���Ŀǰ�������ʺ���һ��ͳ�Ƶ�����������ͬһ���ȼ�������Ҫ������·� */
    if (uc_current_pps_level == g_uc_pps_level) {
        g_uc_pps_level_adjust_cnt = 0;
        return;
    }

    /* Ϊ��֤���ܣ�pps�Ӹߵ���ʱ��ֻ������3�����ڶ��������ޣ���������˯ģʽ */
    if (uc_current_pps_level < g_uc_pps_level) {
        g_uc_pps_level_adjust_cnt++;
        if (g_uc_pps_level_adjust_cnt < PSM_PPS_LEVEL_SUCCESSIVE_DOWN_CNT) {
            return;
        }
    }
    /* Ϊ��֤���ܣ����pps���߳������ޣ���������pm��ʱ���������� */
    g_uc_pps_level_adjust_cnt = 0; /* �����������pps�Ӹߵ������޳���3�Σ��Լ��ӵ͵��������·�ʱ�ļ������� */
    g_uc_pps_level = uc_current_pps_level;

    /* ��Ҫ�л�ʱ������������֪ͨdevice���� */
    pst_mac_device = mac_res_get_dev(0);
    /* ����ǵ�VAP,�򲻴��� */
    if (mac_device_calc_up_vap_num(pst_mac_device) != 1) {
        return;
    }
    ul_ret = mac_device_find_up_vap(pst_mac_device, &pst_mac_vap);
    if ((ul_ret != OAL_SUCC) || (pst_mac_vap == OAL_PTR_NULL)) {
        return;
    }

    st_ps_params.en_cmd = MAC_PS_PARAMS_RESTART_COUNT;
    st_ps_params.uc_restart_count = ul_pm_timer_restart_cnt;
    ul_ret = hmac_config_set_ps_params(pst_mac_vap, OAL_SIZEOF(mac_cfg_ps_params_stru),
                                       (oal_uint8 *)(&st_ps_params));
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "{hmac_set_psm_activity_timer:: send failed!}");
        return;
    }
}


oal_void hmac_tx_small_amsdu_switch(oal_uint32 ul_rx_throughput_mbps, oal_uint32 ul_tx_pps)
{
    mac_device_stru *pst_mac_device = OAL_PTR_NULL;
    hmac_vap_stru *pst_hmac_vap = OAL_PTR_NULL;
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    oal_uint32 ul_limit_throughput_high;
    oal_uint32 ul_limit_throughput_low;
    oal_uint32 ul_limit_pps_high;
    oal_uint32 ul_limit_pps_low;
    oal_uint32 ul_ret;
    oal_bool_enum_uint8 en_small_amsdu;

    /* ������ƻ���֧��С���ۺ� */
    if (g_st_small_amsdu_switch.uc_ini_small_amsdu_en == OAL_FALSE) {
        return;
    }

    /* ÿ������������ */
    ul_limit_throughput_high = g_st_small_amsdu_switch.us_small_amsdu_throughput_high;
    ul_limit_throughput_low = g_st_small_amsdu_switch.us_small_amsdu_throughput_low;

    /* ÿ��PPS���� */
    ul_limit_pps_high = g_st_small_amsdu_switch.us_small_amsdu_pps_high;
    ul_limit_pps_low = g_st_small_amsdu_switch.us_small_amsdu_pps_low;

    if ((ul_rx_throughput_mbps > ul_limit_throughput_high) || (ul_tx_pps > ul_limit_pps_high)) {
        /* rx����������150M����tx pps����12500,��С��amsdu�ۺ� */
        en_small_amsdu = OAL_TRUE;
    } else if ((ul_rx_throughput_mbps < ul_limit_throughput_low) && (ul_tx_pps < ul_limit_pps_low)) {
        /* rx����������100M��tx ppsС��7500,�ر�С��amsdu�ۺϣ����������л� */
        en_small_amsdu = OAL_FALSE;
    } else {
        /* ����100M-150M֮��,�����л� */
        return;
    }

    /* ��ǰ�ۺϷ�ʽ��ͬ,������ */
    if (g_st_small_amsdu_switch.uc_cur_small_amsdu_en == en_small_amsdu) {
        return;
    }

    pst_mac_device = mac_res_get_dev(0);
    /* ����ǵ�VAP,���л� */
    if (mac_device_calc_up_vap_num(pst_mac_device) != 1) {
        return;
    }

    ul_ret = mac_device_find_up_vap(pst_mac_device, &pst_mac_vap);
    if ((ul_ret != OAL_SUCC) || (pst_mac_vap == OAL_PTR_NULL)) {
        return;
    }

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "hmac_tx_small_amsdu_switch:pst_hmac_vap is null");
        return;
    }
    g_st_small_amsdu_switch.uc_cur_small_amsdu_en = en_small_amsdu;
    pst_hmac_vap->en_small_amsdu_switch = en_small_amsdu;
    oam_warning_log3(0, OAM_SF_ANY, "{hmac_tx_small_amsdu_switch: 1.ul_rx_throughput_mbps = [%d],\
                     2.ul_tx_pps = [%d], 3.en_small_amsdu= [%d]!}",
                     ul_rx_throughput_mbps, ul_tx_pps, en_small_amsdu);
}

#ifdef _PRE_WLAN_FEATURE_TCP_ACK_BUFFER
oal_uint32 hmac_set_tx_throughput_threshold(oal_uint32 *pul_limit_throughput_high,
    oal_uint32 *pul_limit_throughput_low,
    mac_vap_stru *pst_mac_vap)
{
    mac_tcp_ack_buf_switch_stru *pst_tcp_ack_buf_switch = mac_get_tcp_ack_buf_switch_addr();

    if (pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_20M) {
        /* ÿ������������ */
        *pul_limit_throughput_high = pst_tcp_ack_buf_switch->us_tcp_ack_buf_throughput_high;
        *pul_limit_throughput_low = pst_tcp_ack_buf_switch->us_tcp_ack_buf_throughput_low;
    } else if ((pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_40PLUS) ||
               (pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_40MINUS)) {
        /* ÿ������������ */
        *pul_limit_throughput_high = pst_tcp_ack_buf_switch->us_tcp_ack_buf_throughput_high_40M;
        *pul_limit_throughput_low = pst_tcp_ack_buf_switch->us_tcp_ack_buf_throughput_low_40M;
    } else if ((pst_mac_vap->st_channel.en_bandwidth >= WLAN_BAND_WIDTH_80PLUSPLUS) &&
               (pst_mac_vap->st_channel.en_bandwidth < WLAN_BAND_WIDTH_40M)) {
        *pul_limit_throughput_high = pst_tcp_ack_buf_switch->us_tcp_ack_buf_throughput_high_80M;
        *pul_limit_throughput_low = pst_tcp_ack_buf_switch->us_tcp_ack_buf_throughput_low_80M;
    } else {
        return OAL_FAIL;
    }
    return OAL_SUCC;
}

oal_void hmac_tx_tcp_ack_buf_switch(oal_uint32 ul_rx_throughput_mbps)
{
    mac_device_stru *pst_mac_device;
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    mac_cfg_tcp_ack_buf_stru st_tcp_ack_param = { 0 };
    oal_uint32 ul_limit_throughput_high;
    oal_uint32 ul_limit_throughput_low;
    oal_uint32 ul_ret;
    oal_bool_enum_uint8 en_tcp_ack_buf;
    mac_tcp_ack_buf_switch_stru *pst_tcp_ack_buf_switch = mac_get_tcp_ack_buf_switch_addr();
    /* ������ƻ���֧��tcp_ack_buf */
    if (pst_tcp_ack_buf_switch->uc_ini_tcp_ack_buf_en == OAL_FALSE) {
        return;
    }

    pst_mac_device = mac_res_get_dev(0);
    /* ����ǵ�VAP,���л� */
    if (mac_device_calc_up_vap_num(pst_mac_device) != 1) {
        return;
    }

    ul_ret = mac_device_find_up_vap(pst_mac_device, &pst_mac_vap);
    if ((ul_ret != OAL_SUCC) || (pst_mac_vap == OAL_PTR_NULL)) {
        return;
    }

    /* ��STAģʽ���л� */
    if (pst_mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_STA) {
        return;
    }

    if (hmac_set_tx_throughput_threshold(&ul_limit_throughput_high,
        &ul_limit_throughput_low, pst_mac_vap) != OAL_SUCC) {
        return;
    }

    if (ul_rx_throughput_mbps > ul_limit_throughput_high) {
        en_tcp_ack_buf = !hmac_get_tx_opt_tcp_ack();
    } else if (ul_rx_throughput_mbps < ul_limit_throughput_low) {
        en_tcp_ack_buf = OAL_FALSE;
    } else {
        /* ����low-high֮��,�����л� */
        return;
    }

    /* ��ǰ�ۺϷ�ʽ��ͬ,������ */
    if (pst_tcp_ack_buf_switch->uc_cur_tcp_ack_buf_en == en_tcp_ack_buf) {
        return;
    }

    oam_warning_log4(0, OAM_SF_ANY, "{hmac_tx_tcp_ack_buf_switch: limit_high = [%d],limit_low = [%d],\
                     rx_throught= [%d]! en_tcp_ack_buf=%d}",
                     ul_limit_throughput_high, ul_limit_throughput_low, ul_rx_throughput_mbps, en_tcp_ack_buf);

    pst_tcp_ack_buf_switch->uc_cur_tcp_ack_buf_en = en_tcp_ack_buf;

    st_tcp_ack_param.en_cmd = MAC_TCP_ACK_BUF_ENABLE;
    st_tcp_ack_param.en_enable = en_tcp_ack_buf;

    hmac_config_tcp_ack_buf(pst_mac_vap, OAL_SIZEOF(mac_cfg_tcp_ack_buf_stru), (oal_uint8 *)&st_tcp_ack_param);
}
#endif

#ifdef _PRE_WLAN_FEATURE_DYN_BYPASS_EXTLNA
oal_void hmac_rx_dyn_bypass_extlna_switch(oal_uint32 ul_tx_throughput_mbps, oal_uint32 ul_rx_throughput_mbps)
{
    mac_device_stru *pst_mac_device;
    mac_vap_stru *pst_mac_vap;
    oal_uint32 ul_limit_throughput_high;
    oal_uint32 ul_limit_throughput_low;
    oal_uint32 ul_throughput_mbps = oal_max(ul_tx_throughput_mbps, ul_rx_throughput_mbps);
    oal_uint32 ul_ret;
    oal_bool_enum_uint8 en_is_pm_test;

    /* ������ƻ���֧�ָ�������bypass����LNA */
    if (g_st_rx_dyn_bypass_extlna_switch.uc_ini_en == OAL_FALSE) {
        return;
    }

    /* ÿ������������ */
    ul_limit_throughput_high = g_st_rx_dyn_bypass_extlna_switch.us_throughput_high;
    ul_limit_throughput_low = g_st_rx_dyn_bypass_extlna_switch.us_throughput_low;

    if (ul_throughput_mbps > ul_limit_throughput_high) {
        /* ����100M,�ǵ͹��Ĳ��Գ��� */
        en_is_pm_test = OAL_FALSE;
    } else if (ul_throughput_mbps < ul_limit_throughput_low) {
        /* ����50M,�͹��Ĳ��Գ��� */
        en_is_pm_test = OAL_TRUE;
    } else {
        /* ����50M-100M֮��,�����л� */
        return;
    }

    /* ��Ҫ�л�ʱ������������֪ͨdevice���� */
    pst_mac_device = mac_res_get_dev(0);
    /* ����ǵ�VAP,�򲻴��� */
    if (mac_device_calc_up_vap_num(pst_mac_device) != 1) {
        return;
    }

    ul_ret = mac_device_find_up_vap(pst_mac_device, &pst_mac_vap);
    if ((ul_ret != OAL_SUCC) || (pst_mac_vap == OAL_PTR_NULL)) {
        return;
    }

    /* ��ǰ��ʽ��ͬ,������ */
    if (g_st_rx_dyn_bypass_extlna_switch.uc_cur_status == en_is_pm_test) {
        return;
    }

    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_DYN_EXTLNA_BYPASS_SWITCH, OAL_SIZEOF(oal_uint8),
                                    (oal_uint8 *)(&en_is_pm_test));
    if (ul_ret == OAL_SUCC) {
        g_st_rx_dyn_bypass_extlna_switch.uc_cur_status = en_is_pm_test;
    }

    oam_warning_log4(0, OAM_SF_ANY,
                     "{hmac_rx_dyn_bypass_extlna_switch: limit_high[%d],limit_low[%d],\
                     throughput[%d], uc_cur_status[%d](0:not pm, 1:pm))!}",
                     ul_limit_throughput_high, ul_limit_throughput_low, ul_throughput_mbps, en_is_pm_test);
}
#endif


OAL_INLINE oal_uint32 hmac_tx_encap(hmac_vap_stru *pst_vap,
                                    hmac_user_stru *pst_user,
                                    oal_netbuf_stru *pst_buf)
{
    mac_ieee80211_qos_htc_frame_addr4_stru *pst_hdr = OAL_PTR_NULL; /* 802.11ͷ */
    mac_ether_header_stru *pst_ether_hdr = OAL_PTR_NULL;
    oal_uint32 ul_qos = HMAC_TX_BSS_NOQOS;
    mac_tx_ctl_stru *pst_tx_ctl = OAL_PTR_NULL;
    oal_uint16 us_ether_type = 0;
    oal_uint8 auc_saddr[ETHER_ADDR_LEN]; /* ԭ��ַָ�� */
    oal_uint8 *saddr_ptr = auc_saddr; /* ԭ��ַָ�� */
    oal_uint8 auc_daddr[ETHER_ADDR_LEN]; /* Ŀ�ĵ�ַָ�� */
    oal_uint8 *daddr_ptr = auc_daddr;
    oal_uint32 ul_ret;
    mac_llc_snap_stru *pst_snap_hdr = OAL_PTR_NULL;

    /* ��ȡCB */
    pst_tx_ctl = (mac_tx_ctl_stru *)(oal_netbuf_cb(pst_buf));
    pst_tx_ctl->bit_align_padding_offset = 0;

#ifdef _PRE_WLAN_NARROW_BAND
    if (g_hitalk_status & NARROW_BAND_ON_MASK) {
        if (!MAC_GET_CB_IS_VIPFRAME(pst_tx_ctl) && (oal_netbuf_len(pst_buf) > 200)) { /* �ж�netbuf len�Ƿ����200 */
            oam_warning_log0(0, 0, "drop more than 200 byte non vip frame packet");
            return OAL_ERR_CODE_PTR_NULL;
        }
    }
#endif

    /* ���skb��dataָ��ǰԤ���Ŀռ����802.11 mac head len������Ҫ���������ڴ���802.11ͷ,3Ϊpading���ֵ */
    if (oal_netbuf_headroom(pst_buf) >= MAC_80211_QOS_HTC_4ADDR_FRAME_LEN + SNAP_LLC_FRAME_LEN +
        MAC_MAX_MSDU_ADDR_ALIGN) {
        pst_hdr = (mac_ieee80211_qos_htc_frame_addr4_stru *)(oal_netbuf_header(pst_buf) -
                    /* 802.11ͷ����Ϊnetbuf header��ȥQOS_HTC_4ADDR_FRAME_LEN��ȥsnap lcc֡��������ټ�ȥ2 */
                    MAC_80211_QOS_HTC_4ADDR_FRAME_LEN - SNAP_LLC_FRAME_LEN - 2);
        pst_tx_ctl->bit_80211_mac_head_type = 1; /* ָʾmacͷ����skb�� */
    } else {
        /* ��������80211ͷ */
        pst_hdr = (mac_ieee80211_qos_htc_frame_addr4_stru *)oal_mem_alloc_m(OAL_MEM_POOL_ID_SHARED_DATA_PKT,
                                                                            MAC_80211_QOS_HTC_4ADDR_FRAME_LEN,
                                                                            OAL_FALSE);
        if (oal_unlikely(pst_hdr == OAL_PTR_NULL)) {
            OAM_ERROR_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX, "{hmac_tx_encap::pst_hdr null.}");
            return OAL_ERR_CODE_PTR_NULL;
        }

        pst_tx_ctl->bit_80211_mac_head_type = 0; /* ָʾmacͷ������skb�У������˶����ڴ��ŵ� */
    }

    /* ��ȡ��̫��ͷ, ԭ��ַ��Ŀ�ĵ�ַ, ��̫������ */
    pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);
    oal_set_mac_addr((oal_uint8 *)auc_daddr, pst_ether_hdr->auc_ether_dhost);
    oal_set_mac_addr((oal_uint8 *)auc_saddr, pst_ether_hdr->auc_ether_shost);

    /* ��amsdu֡ */
    if (pst_tx_ctl->en_is_amsdu == OAL_FALSE) {
        us_ether_type = pst_ether_hdr->us_ether_type;
    } else {
        /* �����AMSDU�ĵ�һ����֡����Ҫ��snapͷ�л�ȡ��̫�����ͣ��������̫��֡������
          ֱ�Ӵ���̫��ͷ�л�ȡ */
        pst_snap_hdr = (mac_llc_snap_stru *)((oal_uint8 *)pst_ether_hdr + ETHER_HDR_LEN);
        us_ether_type = pst_snap_hdr->us_ether_type;
    }

    /* ���鲥֡����ȡ�û���QOS����λ��Ϣ */
    if (pst_tx_ctl->en_ismcast == OAL_FALSE) {
        /* �����û��ṹ���cap_info���ж��Ƿ���QOSվ�� */
        ul_qos = pst_user->st_user_base_info.st_cap_info.bit_qos;
        pst_tx_ctl->en_is_qosdata = pst_user->st_user_base_info.st_cap_info.bit_qos;
    }

    /* ����֡���� */
    hmac_tx_set_frame_ctrl(ul_qos, pst_tx_ctl, pst_hdr);

    /* ���õ�ַ */
    ul_ret = hmac_tx_set_addresses(pst_vap, pst_user, pst_tx_ctl, saddr_ptr, daddr_ptr, pst_hdr, us_ether_type);
    if (oal_unlikely(ul_ret != OAL_SUCC)) {
        if (pst_tx_ctl->bit_80211_mac_head_type == 0) {
            oal_mem_free_m(pst_hdr, OAL_TRUE);
        }
        OAM_ERROR_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
                       "{hmac_tx_encap::hmac_tx_set_addresses failed[%d].}", ul_ret);
        return ul_ret;
    }

    /* ����LAN to WLAN�ķ�AMSDU�ۺ�֡�����LLC SNAPͷ */
    if (pst_tx_ctl->en_is_amsdu == OAL_FALSE) {
#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU
            hmac_tx_encap_large_skb_amsdu(pst_vap, pst_user, pst_buf, pst_tx_ctl);

            if (MAC_GET_CB_HAS_EHTER_HEAD(pst_tx_ctl)) {
                /* �ָ�dataָ�뵽ETHER HEAD - LLC HEAD */
                oal_netbuf_pull(pst_buf, SNAP_LLC_FRAME_LEN);
            }
#endif

            mac_set_snap(pst_buf, us_ether_type, (ETHER_HDR_LEN - SNAP_LLC_FRAME_LEN));
#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU
            if (MAC_GET_CB_HAS_EHTER_HEAD(pst_tx_ctl)) {
                /* �ָ�dataָ�뵽ETHER HEAD */
                oal_netbuf_push(pst_buf, ETHER_HDR_LEN);
                /* ��֤4bytes���� */
                if ((oal_uint)(uintptr_t)oal_netbuf_data(pst_buf) !=
                    OAL_ROUND_DOWN((oal_uint)(uintptr_t)oal_netbuf_data(pst_buf), 4)) {  /* ����ȡ���ĳ���Ϊ4 */
                    pst_tx_ctl->bit_align_padding_offset = (oal_uint)(uintptr_t)oal_netbuf_data(pst_buf) -
                        OAL_ROUND_DOWN ((oal_uint)(uintptr_t)oal_netbuf_data(pst_buf), 4); /* ����ȡ���ĳ���Ϊ4 */
                    oal_netbuf_push(pst_buf, pst_tx_ctl->bit_align_padding_offset);
                }
            }
#endif
        /* ����frame���� */
        pst_tx_ctl->us_mpdu_len = (oal_uint16)oal_netbuf_get_len(pst_buf);

        /* ��amsdu�ۺ�֡����¼mpdu�ֽ�����������snap */
        pst_tx_ctl->us_mpdu_bytes = pst_tx_ctl->us_mpdu_len - SNAP_LLC_FRAME_LEN;
#ifdef _PRE_WLAN_FEATURE_SNIFFER
        proc_sniffer_write_file((const oal_uint8 *)pst_hdr, MAC_80211_QOS_FRAME_LEN,
                                (const oal_uint8 *)oal_netbuf_data(pst_buf), pst_tx_ctl->us_mpdu_len, 1);
#endif
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* macͷ����skb��ʱ��netbuf��dataָ��ָ��macͷ������mac_set_snap�������Ѿ���dataָ��ָ����llcͷ���������Ҫ����push��macͷ�� */
    if (pst_tx_ctl->bit_80211_mac_head_type == 1) {
        oal_netbuf_push(pst_buf, MAC_80211_QOS_HTC_4ADDR_FRAME_LEN);
    }
#endif

    /* �ҽ�802.11ͷ */
    pst_tx_ctl->pst_frame_header = (mac_ieee80211_frame_stru *)pst_hdr;

    /* ��Ƭ���� */
    if (hmac_tx_is_need_frag(pst_vap, pst_user, pst_buf, pst_tx_ctl) == OAL_TRUE) {
        ul_ret = hmac_frag_process_proc(pst_vap, pst_user, pst_buf, pst_tx_ctl);
    }

    return ul_ret;
}


OAL_STATIC oal_uint32 hmac_tx_lan_mpdu_process_sta(hmac_vap_stru *pst_vap,
                                                   oal_netbuf_stru *pst_buf,
                                                   mac_tx_ctl_stru *pst_tx_ctl)
{
    hmac_user_stru *pst_user;             /* Ŀ��STA�ṹ�� */
    mac_ether_header_stru *pst_ether_hdr; /* ��̫��ͷ */
    oal_uint32 ul_ret;
    oal_uint16 us_user_idx;
    oal_uint8 *puc_ether_payload = OAL_PTR_NULL;

    pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);
    pst_tx_ctl->uc_tx_vap_index = pst_vap->st_vap_base_info.uc_vap_id;

    us_user_idx = pst_vap->st_vap_base_info.uc_assoc_vap_id;

    pst_user = (hmac_user_stru *)mac_res_get_hmac_user(us_user_idx);
    if (pst_user == OAL_PTR_NULL) {
        oam_stat_vap_incr(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
        return HMAC_TX_DROP_USER_NULL;
    }

    if (oal_byteorder_host_to_net_uint16(ETHER_TYPE_ARP) == pst_ether_hdr->us_ether_type) {
        pst_ether_hdr++;
        puc_ether_payload = (oal_uint8 *)pst_ether_hdr;
        /* The source MAC address is modified only if the packet is an */
        /* ARP Request or a Response. The appropriate bytes are checked. */
        /* Type field (2 bytes): ARP Request (1) or an ARP Response (2) */
        if ((puc_ether_payload[6] == 0x00) && /* puc_ether_payload��6byte�Ƿ����0 */
            ((puc_ether_payload[7] == 0x02) || (puc_ether_payload[7] == 0x01))) { /* puc_ether_payload��7byte����1��2 */
            /* Set Address2 field in the WLAN Header with source address */
            oal_set_mac_addr(puc_ether_payload + 8, /* ��auc_dot11StationID��ַ��ֵ��puc_ether_payload + 8 */
                             pst_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);
        }
    }

    pst_tx_ctl->us_tx_user_idx = us_user_idx;

    ul_ret = hmac_tx_ucast_process(pst_vap, pst_buf, pst_user, pst_tx_ctl);
    if (oal_unlikely(ul_ret != HMAC_TX_PASS)) {
        return ul_ret;
    }

    /* ��װ802.11ͷ */
    ul_ret = hmac_tx_encap(pst_vap, pst_user, pst_buf);
    if (oal_unlikely((ul_ret != OAL_SUCC))) {
        OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
                         "{hmac_tx_lan_mpdu_process_sta::hmac_tx_encap failed[%d].}", ul_ret);
        oam_stat_vap_incr(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
        return HMAC_TX_DROP_80211_ENCAP_FAIL;
    }

    return HMAC_TX_PASS;
}


OAL_STATIC OAL_INLINE oal_uint32 hmac_tx_lan_mpdu_process_ap(hmac_vap_stru *pst_vap,
                                                             oal_netbuf_stru *pst_buf,
                                                             mac_tx_ctl_stru *pst_tx_ctl)
{
    hmac_user_stru *pst_user = OAL_PTR_NULL; /* Ŀ��STA�ṹ�� */
    mac_ether_header_stru *pst_ether_hdr;    /* ��̫��ͷ */
    oal_uint8 *puc_addr;                     /* Ŀ�ĵ�ַ */
    oal_uint32 ul_ret;
    oal_uint16 us_user_idx;

    /* �ж����鲥�򵥲�,����lan to wlan�ĵ���֡��������̫����ַ */
    pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);
    puc_addr = pst_ether_hdr->auc_ether_dhost;

    /* ��������֡ */
    if (oal_likely(!ether_is_multicast(puc_addr))) {
        ul_ret = mac_vap_find_user_by_macaddr(&(pst_vap->st_vap_base_info), puc_addr, WLAN_MAC_ADDR_LEN, &us_user_idx);
        if (oal_unlikely(ul_ret != OAL_SUCC)) {
            oam_warning_log4(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
                             "{hmac_tx_lan_mpdu_process_ap::hmac_tx_find_user failed %2x:%2x:%2x:%2x}",
                             puc_addr[2], puc_addr[3], puc_addr[4], puc_addr[5]); /* puc_addr��2��3��4��5byte�����ӡ */
            oam_stat_vap_incr(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
            return HMAC_TX_DROP_USER_UNKNOWN;
        }

        /* ת��HMAC��USER�ṹ�� */
        pst_user = (hmac_user_stru *)mac_res_get_hmac_user(us_user_idx);
        if (oal_unlikely(pst_user == OAL_PTR_NULL)) {
            oam_stat_vap_incr(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
            return HMAC_TX_DROP_USER_NULL;
        }

        /* �û�״̬�ж� */
        if (oal_unlikely(pst_user->st_user_base_info.en_user_asoc_state != MAC_USER_STATE_ASSOC)) {
            oam_stat_vap_incr(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
            return HMAC_TX_DROP_USER_INACTIVE;
        }

        /* Ŀ��userָ�� */
        pst_tx_ctl->us_tx_user_idx = us_user_idx;

        ul_ret = hmac_tx_ucast_process(pst_vap, pst_buf, pst_user, pst_tx_ctl);
        if (oal_unlikely(ul_ret != HMAC_TX_PASS)) {
            return ul_ret;
        }
    } else { /* �鲥 or �㲥 */
        /* �����鲥��ʶλ */
        pst_tx_ctl->en_ismcast = OAL_TRUE;

        /* ����ACK���� */
        pst_tx_ctl->en_ack_policy = WLAN_TX_NO_ACK;

        /* ��ȡ�鲥�û� */
        pst_user = mac_res_get_hmac_user(pst_vap->st_vap_base_info.us_multi_user_idx);
        if (oal_unlikely(pst_user == OAL_PTR_NULL)) {
            OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
                             "{hmac_tx_lan_mpdu_process_ap::get multi user failed[%d].}",
                             pst_vap->st_vap_base_info.us_multi_user_idx);
            oam_stat_vap_incr(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
            return HMAC_TX_DROP_MUSER_NULL;
        }

        pst_tx_ctl->us_tx_user_idx = pst_vap->st_vap_base_info.us_multi_user_idx;
        pst_tx_ctl->uc_tid = WLAN_TIDNO_BCAST;

        pst_tx_ctl->uc_ac = WLAN_WME_TID_TO_AC(pst_tx_ctl->uc_tid);
    }

    /* ��װ802.11ͷ */
    ul_ret = hmac_tx_encap(pst_vap, pst_user, pst_buf);
    if (oal_unlikely((ul_ret != OAL_SUCC))) {
        OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
                         "{hmac_tx_lan_mpdu_process_ap::hmac_tx_encap failed[%d].}", ul_ret);
        oam_stat_vap_incr(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
        return HMAC_TX_DROP_80211_ENCAP_FAIL;
    }

    return HMAC_TX_PASS;
}

OAL_STATIC oal_void hmac_tx_vip_info(mac_vap_stru *pst_vap, oal_uint8 uc_data_type,
    oal_netbuf_stru *pst_buf, mac_tx_ctl_stru *pst_tx_ctl)
{
#ifndef WIN32
    mac_eapol_type_enum_uint8 en_eapol_type = MAC_EAPOL_PTK_BUTT;
    oal_uint8 uc_dhcp_type;
    mac_ether_header_stru *pst_eth = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);
    oal_ip_header_stru *pst_tx_ip_hdr = OAL_PTR_NULL;
    oal_eth_arphdr_stru *pst_arp_head = OAL_PTR_NULL;
    oal_int32 l_ret = EOK;
    oal_uint8 auc_ar_sip[ETH_SENDER_IP_ADDR_LEN]; /* sender IP address */
    oal_uint8 auc_ar_dip[ETH_SENDER_IP_ADDR_LEN]; /* sender IP address */

    if (uc_data_type == MAC_DATA_EAPOL) {
        en_eapol_type = mac_get_eapol_key_type((oal_uint8 *)(pst_eth + 1)); /* 1��ʾ����llcͷ */
        oam_warning_log2(pst_vap->uc_vap_id, OAM_SF_ANY,
                         "{hmac_tx_vip_info::EAPOL type=%u, len==%u}[1:1/4 2:2/4 3:3/4 4:4/4]",
                         en_eapol_type, oal_netbuf_len(pst_buf));
    } else if (uc_data_type == MAC_DATA_DHCP) {
        pst_tx_ip_hdr = (oal_ip_header_stru *)(pst_eth + 1); /* 1��ʾ����llcͷ */

        l_ret += memcpy_s((oal_uint8 *)auc_ar_sip, ETH_SENDER_IP_ADDR_LEN,
                          (oal_uint8 *)&pst_tx_ip_hdr->ul_saddr, OAL_SIZEOF(oal_uint32));
        l_ret += memcpy_s((oal_uint8 *)auc_ar_dip, ETH_SENDER_IP_ADDR_LEN,
                          (oal_uint8 *)&pst_tx_ip_hdr->ul_daddr, OAL_SIZEOF(oal_uint32));
        if (l_ret != EOK) {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "hmac_tx_vip_info::memcpy fail!");
            return;
        }

        uc_dhcp_type = mac_get_dhcp_frame_type(pst_tx_ip_hdr);
        OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_tx_vip_info::DHCP type=%d[1:discovery 2:offer 3:request 4:decline 5:ack 6:nack 7:release 8:inform.]",
            uc_dhcp_type);
        oam_warning_log4(pst_vap->uc_vap_id, OAM_SF_ANY, "{hmac_tx_vip_info:: DHCP sip: %d.%d, dip: %d.%d.",
                         auc_ar_sip[2], auc_ar_sip[3], auc_ar_dip[2], auc_ar_dip[3]); /* 2���±꣬3���±� */
    } else {
        pst_arp_head = (oal_eth_arphdr_stru *)(pst_eth + 1); /* 1��ʾ����llcͷ */
        OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_ANY, "{hmac_tx_vip_info:: ARP type=%d.[2:arp resp 3:arp req.]",
                         uc_data_type);
        oam_warning_log4(pst_vap->uc_vap_id, OAM_SF_ANY, "{hmac_tx_vip_info:: ARP sip: %d.%d, dip: %d.%d",
                         pst_arp_head->auc_ar_sip[2], pst_arp_head->auc_ar_sip[3], /* 2���±꣬3���±� */
                         pst_arp_head->auc_ar_tip[2], pst_arp_head->auc_ar_tip[3]); /* 2���±꣬3���±� */
    }

    oam_warning_log4(pst_vap->uc_vap_id, OAM_SF_ANY,
                     "{hmac_tx_vip_info::send to wlan smac: %x:%x, dmac: %x:%x]",
                     pst_eth->auc_ether_shost[4], pst_eth->auc_ether_shost[5],  /* 4���±꣬5���±� */
                     pst_eth->auc_ether_dhost[4], pst_eth->auc_ether_dhost[5]); /* 4���±꣬5���±� */
#endif
}


OAL_INLINE oal_uint32 hmac_tx_lan_to_wlan_no_tcp_opt(mac_vap_stru *pst_vap, oal_netbuf_stru *pst_buf)
{
    frw_event_stru *pst_event = OAL_PTR_NULL; /* �¼��ṹ�� */
    frw_event_mem_stru *pst_event_mem = OAL_PTR_NULL;
    hmac_vap_stru *pst_hmac_vap;                /* VAP�ṹ�� */
    mac_tx_ctl_stru *pst_tx_ctl = OAL_PTR_NULL; /* SKB CB */
    oal_uint32 ul_ret = HMAC_TX_PASS;
    dmac_tx_event_stru *pst_dtx_stru = OAL_PTR_NULL;
    oal_uint8 uc_data_type;
#ifdef _PRE_WLAN_FEATURE_WAPI
    hmac_user_stru *pst_user;
    hmac_wapi_stru *pst_wapi;
    mac_ieee80211_frame_stru *pst_mac_hdr;
    oal_bool_enum_uint8 en_is_mcast = OAL_FALSE;
#endif

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_vap->uc_vap_id);
    if (oal_unlikely(pst_hmac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_TX, "{hmac_tx_lan_to_wlan::pst_hmac_vap null.}");
        oam_stat_vap_incr(pst_vap->uc_vap_id, tx_abnormal_msdu_dropped, 1);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* VAPģʽ�ж� */
    if (!IS_LEGACY_AP_OR_STA(pst_vap)) {
        OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_TX, "{hmac_tx_lan_to_wlan::en_vap_mode=%d.}", pst_vap->en_vap_mode);
        oam_stat_vap_incr(pst_vap->uc_vap_id, tx_abnormal_msdu_dropped, 1);
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    /* ��������û�����Ϊ0���������� */
    if (oal_unlikely(pst_hmac_vap->st_vap_base_info.us_user_nums == 0)) {
        oam_stat_vap_incr(pst_vap->uc_vap_id, tx_abnormal_msdu_dropped, 1);
        return OAL_FAIL;
    }
#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
    /* �����ݣ�ֻ��һ�Σ����ⷴ������tx��������ַ */
    if (pst_vap->bit_al_tx_flag == OAL_SWITCH_ON) {
        if (pst_vap->bit_first_run != OAL_FALSE) {
            return OAL_SUCC;
        }
        mac_vap_set_al_tx_first_run(pst_vap, OAL_TRUE);
    }
#endif

    /* ��ʼ��CB tx rx�ֶ� , CB�ֶ���ǰ���Ѿ������㣬 �����ﲻ��Ҫ�ظ���ĳЩ�ֶθ���ֵ */
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_buf);
    pst_tx_ctl->uc_mpdu_num = 1;
    pst_tx_ctl->uc_netbuf_num = 1;
    pst_tx_ctl->en_frame_type = WLAN_DATA_BASICTYPE;
    pst_tx_ctl->en_is_probe_data = DMAC_USER_ALG_NON_PROBE;
    pst_tx_ctl->en_ack_policy = WLAN_TX_NORMAL_ACK;
#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
    pst_tx_ctl->en_ack_policy = pst_hmac_vap->bit_ack_policy;
#endif
    pst_tx_ctl->uc_tx_vap_index = pst_vap->uc_vap_id;
    pst_tx_ctl->us_tx_user_idx = 0xffff;
    pst_tx_ctl->uc_ac = WLAN_WME_AC_BE; /* ��ʼ����BE���� */

    /* ����LAN TO WLAN��WLAN TO WLAN��netbuf�������������Ϊ�����֣���Ҫ���ж�
       ��������������netbufȻ���ٶ�CB���¼������ֶθ�ֵ
 */
    if (pst_tx_ctl->en_event_type != FRW_EVENT_TYPE_WLAN_DTX) {
        pst_tx_ctl->en_event_type = FRW_EVENT_TYPE_HOST_DRX;
        pst_tx_ctl->uc_event_sub_type = DMAC_TX_HOST_DRX;
    }

    /* �˴����ݿ��ܴ��ں˶�����Ҳ�п�����dev��������ͨ���տ�ת��ȥ��ע��һ�� */
    uc_data_type = mac_get_data_type_from_8023((oal_uint8 *)oal_netbuf_data(pst_buf), MAC_NETBUFF_PAYLOAD_ETH);
    /* ά�⣬���һ���ؼ�֡��ӡ */
    if ((uc_data_type != MAC_DATA_UNSPEC) && (uc_data_type <= MAC_DATA_VIP)) {
        pst_tx_ctl->bit_is_vipframe = OAL_TRUE;
        hmac_tx_vip_info(pst_vap, uc_data_type, pst_buf, pst_tx_ctl);
    }
    oal_spin_lock_bh(&pst_hmac_vap->st_lock_state);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* �����ݰ�����ͳ�� */
    hmac_wifi_statistic_tx_packets(oal_netbuf_len(pst_buf));

    hmac_wfd_statistic_tx_packets(pst_vap);
#endif

    if (pst_hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
        /* ������ǰ MPDU */
        if (pst_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11QosOptionImplemented == OAL_FALSE) {
            pst_tx_ctl->uc_ac = WLAN_WME_AC_VO; /* APģʽ ��WMM ��VO���� */
            pst_tx_ctl->uc_tid = WLAN_WME_AC_TO_TID(pst_tx_ctl->uc_ac);
        }

        ul_ret = hmac_tx_lan_mpdu_process_ap(pst_hmac_vap, pst_buf, pst_tx_ctl);
    } else if (pst_hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
        /* ������ǰMPDU */
        pst_tx_ctl->uc_ac = WLAN_WME_AC_VO; /* STAģʽ ��qos֡��VO���� */
        pst_tx_ctl->uc_tid = WLAN_WME_AC_TO_TID(pst_tx_ctl->uc_ac);

        ul_ret = hmac_tx_lan_mpdu_process_sta(pst_hmac_vap, pst_buf, pst_tx_ctl);
#ifdef _PRE_WLAN_FEATURE_WAPI
        if (ul_ret == HMAC_TX_PASS) {
            // && oal_unlikely(wapi_is_work(pst_hmac_vap)))
            pst_user = (hmac_user_stru *)mac_res_get_hmac_user(pst_vap->uc_assoc_vap_id);
            if (pst_user == OAL_PTR_NULL) {
                oam_stat_vap_incr(pst_vap->uc_vap_id, tx_abnormal_msdu_dropped, 1);
                OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_tx_lan_to_wlan_no_tcp_opt::usrid==%u no usr!}",
                                 pst_vap->uc_assoc_vap_id);
                oal_spin_unlock_bh(&pst_hmac_vap->st_lock_state);
                return HMAC_TX_DROP_USER_NULL;
            }

            /* ��ȡwapi���� �鲥/���� */
            pst_mac_hdr = ((mac_tx_ctl_stru *)oal_netbuf_cb(pst_buf))->pst_frame_header;
            en_is_mcast = ether_is_multicast(pst_mac_hdr->auc_address1);
            /*lint -e730*/
            pst_wapi = hmac_user_get_wapi_ptr(pst_vap,
                                              !en_is_mcast,
                                              pst_user->st_user_base_info.us_assoc_id);
            /*lint +e730*/
            if (pst_wapi == OAL_PTR_NULL) {
                OAM_ERROR_LOG1(pst_vap->uc_vap_id, OAM_SF_ANY, "{hmac_tx_lan_to_wlan_no_tcp_opt::pst_wapi null, %x}",
                               (uintptr_t)pst_wapi);
                oal_spin_unlock_bh(&pst_hmac_vap->st_lock_state);
                return OAL_ERR_CODE_PTR_NULL;
            }
            if ((wapi_is_port_valid(pst_wapi) == OAL_TRUE) && (pst_wapi->wapi_netbuff_txhandle != OAL_PTR_NULL)) {
                pst_buf = pst_wapi->wapi_netbuff_txhandle(pst_wapi, pst_buf);
                if (pst_buf == OAL_PTR_NULL) {
                    oam_stat_vap_incr(pst_vap->uc_vap_id, tx_abnormal_msdu_dropped, 1);
                    oam_warning_log0(pst_vap->uc_vap_id, OAM_SF_ANY,
                                     "{hmac_tx_lan_to_wlan_no_tcp_opt_etc:: wapi_netbuff_txhandle fail!}");
                    oal_spin_unlock_bh(&pst_hmac_vap->st_lock_state);
                    return OAL_ERR_CODE_PTR_NULL;
                }
                /* ����wapi�����޸�netbuff���˴���Ҫ���»�ȡһ��cb */
                pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_buf);
            }
        }

#endif /* #ifdef _PRE_WLAN_FEATURE_WAPI */
    }

    oal_spin_unlock_bh(&pst_hmac_vap->st_lock_state);

    if (oal_likely(ul_ret == HMAC_TX_PASS)) {
        /* ���¼�������DMAC */
        pst_event_mem = frw_event_alloc_m(OAL_SIZEOF(dmac_tx_event_stru));
        if (oal_unlikely(pst_event_mem == OAL_PTR_NULL)) {
            OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_TX, "{hmac_tx_lan_to_wlan::frw_event_alloc failed.}");
            return OAL_ERR_CODE_ALLOC_MEM_FAIL;
        }

        pst_event = (frw_event_stru *)pst_event_mem->puc_data;

        /* ��д�¼�ͷ */
        frw_event_hdr_init(&(pst_event->st_event_hdr),
                           FRW_EVENT_TYPE_HOST_DRX,
                           DMAC_TX_HOST_DRX,
                           OAL_SIZEOF(dmac_tx_event_stru),
                           FRW_EVENT_PIPELINE_STAGE_1,
                           pst_vap->uc_chip_id,
                           pst_vap->uc_device_id,
                           pst_vap->uc_vap_id);

        pst_dtx_stru = (dmac_tx_event_stru *)pst_event->auc_event_data;
        pst_dtx_stru->pst_netbuf = pst_buf;

        pst_dtx_stru->us_frame_len = pst_tx_ctl->us_mpdu_len;

        /* �����¼� */
        ul_ret = frw_event_dispatch_event(pst_event_mem);
        if (oal_unlikely(ul_ret != OAL_SUCC)) {
            OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_TX, "{hmac_tx_lan_to_wlan::dispatch_event failed[%d].}",
                             ul_ret);
            oam_stat_vap_incr(pst_vap->uc_vap_id, tx_abnormal_msdu_dropped, 1);
        }

        /* �ͷ��¼� */
        frw_event_free_m(pst_event_mem);
    } else if (oal_unlikely(ul_ret == HMAC_TX_BUFF)) {
        ul_ret = OAL_SUCC;
    } else if (ul_ret == HMAC_TX_DONE) {
        ul_ret = OAL_SUCC;
    } else if (ul_ret == HMAC_TX_DROP_MTOU_FAIL) {
        /* �鲥����û�ж�Ӧ��STA����ת�ɵ��������Զ�������������Ϊ */
        oam_info_log1(pst_vap->uc_vap_id, OAM_SF_TX, "{hmac_tx_lan_to_wlan_no_tcp_opt::TX_DROP.reason[%d]!}", ul_ret);
    } else {
        OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_TX, "{hmac_tx_lan_to_wlan_no_tcp_opt::HMAC_TX_DROP.reason[%d]!}",
                         ul_ret);
    }

    return ul_ret;
}

#ifdef _PRE_WLAN_TCP_OPT
OAL_STATIC oal_uint32 hmac_transfer_tx_handler(hmac_device_stru *hmac_device, hmac_vap_stru *hmac_vap,
                                               oal_netbuf_stru *netbuf)
{
    oal_uint32 ul_ret = OAL_SUCC;

    if (oal_netbuf_select_queue(netbuf) == WLAN_TCP_ACK_QUEUE) {
        oal_spin_lock_bh(&hmac_vap->ast_hmac_tcp_ack[HCC_TX].data_queue_lock[HMAC_TCP_ACK_QUEUE]);
        oal_netbuf_list_tail_nolock(&hmac_vap->ast_hmac_tcp_ack[HCC_TX].data_queue[HMAC_TCP_ACK_QUEUE], netbuf);

        /* ����TCP ACK�ȴ�����, ���ⱨ�����Ϸ��� */
        if (hmac_judge_tx_netbuf_is_tcp_ack((oal_ether_header_stru *)oal_netbuf_data(netbuf))) {
            oal_spin_unlock_bh(&hmac_vap->ast_hmac_tcp_ack[HCC_TX].data_queue_lock[HMAC_TCP_ACK_QUEUE]);
            hmac_sched_transfer();
        } else {
            oal_spin_unlock_bh(&hmac_vap->ast_hmac_tcp_ack[HCC_TX].data_queue_lock[HMAC_TCP_ACK_QUEUE]);
            hmac_tcp_ack_process();
        }
    } else {
        ul_ret = hmac_tx_lan_to_wlan_no_tcp_opt(&(hmac_vap->st_vap_base_info), netbuf);
    }

    return ul_ret;
}
#endif


oal_uint32 hmac_tx_wlan_to_wlan_ap(oal_mem_stru *pst_event_mem)
{
    frw_event_stru *pst_event = OAL_PTR_NULL; /* �¼��ṹ�� */
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    oal_netbuf_stru *pst_buf = OAL_PTR_NULL;     /* ��netbuf����ȡ������ָ��netbuf��ָ�� */
    oal_netbuf_stru *pst_buf_tmp = OAL_PTR_NULL; /* �ݴ�netbufָ�룬����whileѭ�� */
    mac_tx_ctl_stru *pst_tx_ctl = OAL_PTR_NULL;
    oal_uint32 ul_ret;

    /* ����ж� */
    if (oal_unlikely(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_TX, "{hmac_tx_wlan_to_wlan_ap::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* ��ȡ�¼� */
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    if (oal_unlikely(pst_event == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_TX, "{hmac_tx_wlan_to_wlan_ap::pst_event null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡPAYLOAD�е�netbuf�� */
    pst_buf = (oal_netbuf_stru *)(uintptr_t)(*((oal_uint *)(pst_event->auc_event_data)));

    ul_ret = hmac_tx_get_mac_vap(pst_event->st_event_hdr.uc_vap_id, &pst_mac_vap);
    if (oal_unlikely(ul_ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(pst_event->st_event_hdr.uc_vap_id, OAM_SF_TX,
                       "{hmac_tx_wlan_to_wlan_ap::hmac_tx_get_mac_vap failed[%d].}", ul_ret);
        hmac_free_netbuf_list(pst_buf);
        return ul_ret;
    }

    /* ѭ������ÿһ��netbuf��������̫��֡�ķ�ʽ���� */
    while (pst_buf != OAL_PTR_NULL) {
        pst_buf_tmp = oal_netbuf_next(pst_buf);

        oal_netbuf_next(pst_buf) = OAL_PTR_NULL;
        oal_netbuf_prev(pst_buf) = OAL_PTR_NULL;

        
        pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_buf);
        memset_s(pst_tx_ctl, OAL_SIZEOF(mac_tx_ctl_stru), 0, OAL_SIZEOF(mac_tx_ctl_stru));

        pst_tx_ctl->en_event_type = FRW_EVENT_TYPE_WLAN_DTX;
        pst_tx_ctl->uc_event_sub_type = DMAC_TX_WLAN_DTX;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        /* set the queue map id when wlan to wlan */
        oal_skb_set_queue_mapping(pst_buf, WLAN_NORMAL_QUEUE);
#endif

        ul_ret = hmac_tx_lan_to_wlan(pst_mac_vap, pst_buf);
        /* ����ʧ�ܣ��Լ������Լ��ͷ�netbuff�ڴ� */
        if (ul_ret != OAL_SUCC) {
            hmac_free_netbuf_list(pst_buf);
        }

        pst_buf = pst_buf_tmp;
    }

    return OAL_SUCC;
}


OAL_INLINE oal_uint32 hmac_tx_lan_to_wlan(mac_vap_stru *pst_vap, oal_netbuf_stru *pst_buf)
{
    oal_uint32 ul_ret = HMAC_TX_PASS;
#ifdef _PRE_WLAN_TCP_OPT
    hmac_device_stru *pst_hmac_device = OAL_PTR_NULL;
    hmac_vap_stru *pst_hmac_vap; /* VAP�ṹ�� */
#endif

#ifdef _PRE_WLAN_TCP_OPT
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_vap->uc_vap_id);
    if (oal_unlikely(pst_hmac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_TX, "{hmac_tx_lan_to_wlan_tcp_opt::pst_dmac_vap null.}\r\n");
        return OAL_FAIL;
    }
    pst_hmac_device = hmac_res_get_mac_dev(pst_vap->uc_device_id);
    if (oal_unlikely(pst_hmac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
                       "{hmac_tx_lan_to_wlan_tcp_opt::pst_hmac_device null.}\r\n");
        return OAL_FAIL;
    }
    if (pst_hmac_device->sys_tcp_tx_ack_opt_enable == OAL_TRUE) {
        ul_ret = hmac_transfer_tx_handler(pst_hmac_device, pst_hmac_vap, pst_buf);
    } else
#endif
    {
        ul_ret = hmac_tx_lan_to_wlan_no_tcp_opt(pst_vap, pst_buf);
    }
    return ul_ret;
}


oal_bool_enum_uint8 hmac_bridge_vap_should_drop(oal_netbuf_stru *pst_buf, mac_vap_stru *pst_vap)
{
    oal_bool_enum_uint8 en_drop_frame = OAL_FALSE;
    oal_uint8 uc_data_type;
    /* ����������Ѿ��пգ����ﲻ���п� */
    /* ���������д���p2pɨ�費��dhcp��eapol֡����ֹ����ʧ�� */
    if (pst_vap->en_vap_state != MAC_VAP_STATE_STA_LISTEN) {
        en_drop_frame = OAL_TRUE;
    } else {
        uc_data_type = mac_get_data_type_from_8023((oal_uint8 *)oal_netbuf_payload(pst_buf), MAC_NETBUFF_PAYLOAD_ETH);
        if ((uc_data_type != MAC_DATA_EAPOL) && (uc_data_type != MAC_DATA_DHCP)) {
            en_drop_frame = OAL_TRUE;
        } else {
            oam_warning_log2(pst_vap->uc_vap_id, OAM_SF_TX, "{hmac_bridge_vap_should_drop::donot drop [%d]\
                frame[EAPOL:1, DHCP:0]. vap state[%d].}",
                             uc_data_type, pst_vap->en_vap_state);
        }
    }
    return en_drop_frame;
}

#define MAC_VAP_IS_NOT_WORK (oal_unlikely(!((pst_vap->en_vap_state == MAC_VAP_STATE_UP) || (pst_vap->en_vap_state == MAC_VAP_STATE_PAUSE))))
OAL_STATIC oal_void hmac_stats_packet_from_ether(mac_vap_stru *pst_vap, hmac_vap_stru *pst_hmac_vap,
    oal_netbuf_stru *pst_buf)
{
    /* ͳ����̫�����������ݰ�ͳ�� */
    hmac_vap_dft_stats_pkt_incr(pst_hmac_vap->st_query_stats.ul_tx_pkt_num_from_lan, 1);
    hmac_vap_dft_stats_pkt_incr(pst_hmac_vap->st_query_stats.ul_tx_bytes_from_lan, oal_netbuf_len(pst_buf));
    oam_stat_vap_incr(pst_vap->uc_vap_id, tx_pkt_num_from_lan, 1);
    oam_stat_vap_incr(pst_vap->uc_vap_id, tx_bytes_from_lan, oal_netbuf_len(pst_buf));
}
OAL_STATIC oal_void hmac_bridge_filter_xmit_print(mac_vap_stru *pst_vap)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* filter the tx xmit pkts print */
    if ((pst_vap->en_vap_state == MAC_VAP_STATE_INIT) || (pst_vap->en_vap_state == MAC_VAP_STATE_BUTT)) {
        OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_TX, "{hmac_bridge_vap_xmit::vap state[%d] !=\
                         MAC_VAP_STATE_{UP|PAUSE}!}",
                         pst_vap->en_vap_state);
    } else {
        oam_info_log1(pst_vap->uc_vap_id, OAM_SF_TX, "{hmac_bridge_vap_xmit::vap state[%d] !=\
                      MAC_VAP_STATE_{UP|PAUSE}!}\r\n",
                      pst_vap->en_vap_state);
    }
#else
    OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_TX, "{hmac_bridge_vap_xmit::vap state[%d] !=\
                     MAC_VAP_STATE_{UP|PAUSE}!}\r\n",
                     pst_vap->en_vap_state);
#endif
}

oal_net_dev_tx_enum hmac_bridge_vap_xmit(oal_netbuf_stru *pst_buf, oal_net_device_stru *pst_dev)
{
    mac_vap_stru *pst_vap = OAL_PTR_NULL;
    hmac_vap_stru *pst_hmac_vap = OAL_PTR_NULL;
    oal_uint32 ul_ret;
#ifdef _PRE_WLAN_FEATURE_ROAM
    oal_uint8 uc_data_type;
#endif

#if defined(_PRE_WLAN_FEATURE_ALWAYS_TX)
    mac_device_stru *pst_mac_device;
#endif
    oal_bool_enum_uint8 en_drop_frame = OAL_FALSE;
#ifdef _PRE_WLAN_CHBA_MGMT
    uint16_t user_idx = 0;
    hmac_user_stru *hmac_user = NULL;
    hmac_chba_vap_stru *chba_vap_info = NULL;
    oal_netbuf_stru *copy_buf = NULL;
    mac_ether_header_stru *pst_ether_hdr; /* ��̫��ͷ */
    uint8_t *puc_addr; /* Ŀ�ĵ�ַ */
    uint8_t user_bitmap_idx;
#endif

    if (oal_unlikely(pst_buf == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_TX, "{hmac_bridge_vap_xmit::pst_buf = OAL_PTR_NULL!}\r\n");
        return OAL_NETDEV_TX_OK;
    }

    if (oal_unlikely(pst_dev == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_TX, "{hmac_bridge_vap_xmit::pst_dev = OAL_PTR_NULL!}\r\n");
        oal_netbuf_free(pst_buf);
        oam_stat_vap_incr(0, tx_abnormal_msdu_dropped, 1);
        return OAL_NETDEV_TX_OK;
    }

    /* ��ȡVAP�ṹ�� */
    pst_vap = (mac_vap_stru *)oal_net_dev_priv(pst_dev);
    /* ���VAP�ṹ�岻���ڣ��������� */
    if (oal_unlikely(pst_vap == OAL_PTR_NULL)) {
        /* will find vap fail when receive a pkt from
         * kernel while vap is deleted, return OAL_NETDEV_TX_OK is so. */
        oam_warning_log0(0, OAM_SF_TX, "{hmac_bridge_vap_xmit::pst_vap = OAL_PTR_NULL!}\r\n");
        oal_netbuf_free(pst_buf);
        oam_stat_vap_incr(0, tx_abnormal_msdu_dropped, 1);
        return OAL_NETDEV_TX_OK;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_bridge_vap_xmit::pst_hmac_vap null.}");
        oal_netbuf_free(pst_buf);
        return OAL_NETDEV_TX_OK;
    }

#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX

    pst_mac_device = mac_res_get_dev(pst_vap->uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_PROXYSTA, "{hmac_bridge_vap_xmit::mac_res_get_dev is null!}");
        oal_netbuf_free(pst_buf);

        return OAL_NETDEV_TX_OK;
    }

    /* ��������ģ�pst_device_struָ���л�δ״̬, ���������л�δ����״̬ */
    if ((pst_vap->bit_al_tx_flag == OAL_SWITCH_ON) ||
        ((pst_mac_device->pst_device_stru != OAL_PTR_NULL) &&
         (pst_mac_device->pst_device_stru->bit_al_tx_flag == HAL_ALWAYS_TX_AMPDU_ENABLE))) {
        oal_netbuf_free(pst_buf);
        return OAL_NETDEV_TX_OK;
    }
#endif

    pst_buf = oal_netbuf_unshare(pst_buf, GFP_ATOMIC);
    if (oal_unlikely(pst_buf == OAL_PTR_NULL)) {
        oam_info_log0(pst_vap->uc_vap_id, OAM_SF_TX, "{hmac_bridge_vap_xmit::the unshare netbuf = OAL_PTR_NULL!}\r\n");
        return OAL_NETDEV_TX_OK;
    }

    /* ͳ����̫�����������ݰ�ͳ�� */
    hmac_stats_packet_from_ether(pst_vap, pst_hmac_vap, pst_buf);

    /* ����VAP״̬������滥�⣬��Ҫ�������� */
    oal_spin_lock_bh(&pst_hmac_vap->st_lock_state);

    /* �ж�VAP��״̬�����ROAM���������� MAC_DATA_DHCP/MAC_DATA_ARP */
#ifdef _PRE_WLAN_FEATURE_ROAM
    if (pst_vap->en_vap_state == MAC_VAP_STATE_ROAMING) {
        uc_data_type = mac_get_data_type_from_8023((oal_uint8 *)oal_netbuf_payload(pst_buf), MAC_NETBUFF_PAYLOAD_ETH);
        if (uc_data_type != MAC_DATA_EAPOL) {
            oal_netbuf_free(pst_buf);
            oal_spin_unlock_bh(&pst_hmac_vap->st_lock_state);
            return OAL_NETDEV_TX_OK;
        }
    } else {
#endif  // _PRE_WLAN_FEATURE_ROAM
        /* �ж�VAP��״̬�����û��UP/PAUSE���������� */
        if (MAC_VAP_IS_NOT_WORK) {
            en_drop_frame = hmac_bridge_vap_should_drop(pst_buf, pst_vap);
            if (en_drop_frame == OAL_TRUE) {
                hmac_bridge_filter_xmit_print(pst_vap);
                oal_netbuf_free(pst_buf);
                oam_stat_vap_incr(pst_vap->uc_vap_id, tx_abnormal_msdu_dropped, 1);
                oal_spin_unlock_bh(&pst_hmac_vap->st_lock_state);
                return OAL_NETDEV_TX_OK;
            }
        }
#ifdef _PRE_WLAN_FEATURE_ROAM
    }
#endif

    oal_netbuf_next(pst_buf) = OAL_PTR_NULL;
    oal_netbuf_prev(pst_buf) = OAL_PTR_NULL;

    memset_s(oal_netbuf_cb(pst_buf), oal_netbuf_cb_size(), 0, oal_netbuf_cb_size());

#ifdef _PRE_WLAN_FEATURE_BTCOEX
    /* ���ͷ����arp_req ͳ�ƺ�ɾba�Ĵ��� */
    hmac_btcoex_arp_fail_delba_process(pst_buf, &(pst_hmac_vap->st_vap_base_info));
#endif

    oal_spin_unlock_bh(&pst_hmac_vap->st_lock_state);
#ifdef _PRE_WLAN_CHBA_MGMT
    pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);
    if ((!hmac_chba_vap_start_check(pst_hmac_vap)) || !(ether_is_multicast(pst_ether_hdr->auc_ether_dhost))) {
        ul_ret = mac_vap_find_user_by_macaddr(pst_vap, pst_ether_hdr->auc_ether_dhost, WLAN_MAC_ADDR_LEN, &user_idx);
        hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(user_idx);
        hmac_stat_user_tx_netbuf(hmac_user, pst_buf);
        ul_ret = hmac_tx_lan_to_wlan(pst_vap, pst_buf);
        if (oal_unlikely(ul_ret != OAL_SUCC)) {
            /* ����ʧ�ܣ�Ҫ�ͷ��ں������netbuff�ڴ�� */
            oal_netbuf_free(pst_buf);
        }
        return OAL_NETDEV_TX_OK;
    }

    chba_vap_info = hmac_get_up_chba_vap_info();
    if (chba_vap_info == NULL) {
        oal_netbuf_free(pst_buf);
        return OAL_NETDEV_TX_OK;
    }

    for (user_bitmap_idx = 0; user_bitmap_idx < MAC_RES_MAX_USER_NUM; user_bitmap_idx++) {
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
        /* ��ȡ��user�Ľṹ�� */
        /* ��user��mac��ַ���Ƹ�dhost��mac��ַ */
        if (memcpy_s(puc_addr, WLAN_MAC_ADDR_LEN, hmac_user->st_user_base_info.auc_user_mac_addr,
            WLAN_MAC_ADDR_LEN) != EOK) {
            OAM_ERROR_LOG0(0, 0, "{hmac_bridge_vap_xmit::memcpy_s failed.}");
        }

        hmac_stat_user_tx_netbuf(hmac_user, copy_buf);
        ul_ret = hmac_tx_lan_to_wlan(pst_vap, copy_buf);
        if (oal_unlikely(ul_ret != OAL_SUCC)) {
            /* ����ʧ�ܣ�Ҫ�ͷ��ں������netbuff�ڴ�� */
            oal_netbuf_free(copy_buf);
        }
    }
    /* �ͷ�netbuf */
    oal_netbuf_free(pst_buf);
#else
    ul_ret = hmac_tx_lan_to_wlan(pst_vap, pst_buf);
    if (oal_unlikely(ul_ret != OAL_SUCC)) {
        /* ����ʧ�ܣ�Ҫ�ͷ��ں������netbuff�ڴ�� */
        oal_netbuf_free(pst_buf);
    }
#endif
    return OAL_NETDEV_TX_OK;
}

oal_void hmac_tx_ba_cnt_vary(hmac_vap_stru *pst_hmac_vap,
                             hmac_user_stru *pst_hmac_user,
                             oal_uint8 uc_tidno,
                             oal_netbuf_stru *pst_buf)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_uint32 ul_current_timestamp;
    if ((pst_hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_STA) &&
        (g_ul_tx_ba_policy_select == OAL_TRUE)) {
        
        pst_hmac_user->auc_ba_flag[uc_tidno]++;
    } else {
        ul_current_timestamp = (oal_uint32)oal_time_get_stamp_ms();
        /* ��һ����ֱ�Ӽ�����
           ��ʱ����������ʱ����ʼ����BA;
           TCP ACK�ظ�����������ʱ�����ơ� */
        if ((pst_hmac_user->auc_ba_flag[uc_tidno] == 0) ||
            (oal_netbuf_is_tcp_ack((oal_ip_header_stru *)(oal_netbuf_data(pst_buf) + ETHER_HDR_LEN))) ||
            ((oal_uint32)oal_time_get_runtime(ul_current_timestamp, pst_hmac_user->aul_last_timestamp[uc_tidno]) <
             WLAN_BA_CNT_INTERVAL)) {
            pst_hmac_user->auc_ba_flag[uc_tidno]++;
        } else {
            pst_hmac_user->auc_ba_flag[uc_tidno] = 0;
        }

        pst_hmac_user->aul_last_timestamp[uc_tidno] = ul_current_timestamp;
    }
#else
    pst_hmac_user->auc_ba_flag[uc_tidno]++;
#endif
}

#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU
OAL_STATIC oal_uint32 hmac_get_amsdu_judge_result(oal_uint32 ul_ret,
                                                  oal_bool_enum_uint8 en_mu_vap_flag,
                                                  mac_vap_stru *pst_vap1,
                                                  mac_vap_stru *pst_vap2)
{
    if ((ul_ret != OAL_SUCC) || (pst_vap1 == OAL_PTR_NULL) || (en_mu_vap_flag && (pst_vap2 == OAL_PTR_NULL))) {
        return OAL_FAIL;;
    }
    return OAL_SUCC;
}


oal_void hmac_tx_amsdu_ampdu_switch(oal_uint32 ul_tx_throughput_mbps)
{
    mac_device_stru *pst_mac_device = mac_res_get_dev(0);
    oal_uint32 ul_limit_throughput_high;
    oal_uint32 ul_limit_throughput_low;
    oal_uint32 ul_ret;
    oal_bool_enum_uint8 en_large_amsdu_ampdu;
    oal_uint32 ul_up_ap_num  = mac_device_calc_up_vap_num(pst_mac_device);
    oal_bool_enum_uint8 en_mu_vap_flag = (ul_up_ap_num > 1);
    oal_uint8  uc_idx;
    mac_vap_stru *pst_vap[2] = {OAL_PTR_NULL}; // 2����2��vap
    oal_bool_enum_uint8 en_mu_vap = (ul_up_ap_num > 1);
    mac_tx_large_amsdu_ampdu_stru *tx_large_amsdu = mac_get_tx_large_amsdu_addr();

    /* ������ƻ���֧��amsdu_ampdu���Ͼۺ� */
    if (tx_large_amsdu->uc_tx_amsdu_ampdu_en == OAL_FALSE) {
        return;
    }

    if (en_mu_vap) {
        ul_ret = mac_device_find_2up_vap(pst_mac_device, &pst_vap[0], &pst_vap[1]);
    } else {
        ul_ret = mac_device_find_up_vap(pst_mac_device, &pst_vap[0]);
    }

    if (hmac_get_amsdu_judge_result(ul_ret, en_mu_vap, pst_vap[0], pst_vap[1]) == OAL_FAIL) {
        return;
    }

    /* ÿ������������ */
    ul_limit_throughput_high = (tx_large_amsdu->us_amsdu_ampdu_throughput_high >> en_mu_vap_flag);
    ul_limit_throughput_low = (tx_large_amsdu->us_amsdu_ampdu_throughput_low >> en_mu_vap_flag);
    for (uc_idx = 0; uc_idx < ul_up_ap_num; uc_idx++) {
        /* ����м��������⣬�ص�amsdu_ampdu���Ͼۺ� */
        if (tx_large_amsdu->uc_compability_en == OAL_TRUE && IS_LEGACY_VAP(pst_vap[uc_idx])) {
            tx_large_amsdu->uc_cur_amsdu_ampdu_enable[pst_vap[uc_idx]->uc_vap_id] = OAL_FALSE;
            continue;
        }

        if (ul_tx_throughput_mbps > ul_limit_throughput_high) {
            /* ����100M,�л�amsdu����ۺϣ���vapʱ����� */
            en_large_amsdu_ampdu = OAL_TRUE;
        } else if (ul_tx_throughput_mbps < ul_limit_throughput_low) {
            /* ����50M,�ر�amsdu����ۺϣ���vapʱ����� */
            en_large_amsdu_ampdu = OAL_FALSE;
        } else {
            /* ����50M-100M֮��,�����л� */
            continue;
        }

        /* ��ǰ�ۺϷ�ʽ��ͬ,������ */
        if (tx_large_amsdu->uc_cur_amsdu_ampdu_enable[pst_vap[uc_idx]->uc_vap_id] == en_large_amsdu_ampdu) {
            continue;
        }

        tx_large_amsdu->uc_cur_amsdu_ampdu_enable[pst_vap[uc_idx]->uc_vap_id] = en_large_amsdu_ampdu;

        oam_warning_log4(0, OAM_SF_ANY, "{hmac_tx_amsdu_ampdu_switch:vap%d enabled[%d], limit_high[%d], limit_low[%d]}",
                         pst_vap[uc_idx]->uc_vap_id, en_large_amsdu_ampdu, ul_limit_throughput_high,
                         ul_limit_throughput_low);
    }
}
#endif

oal_module_symbol(hmac_tx_wlan_to_wlan_ap);
oal_module_symbol(hmac_tx_lan_to_wlan);
oal_module_symbol(hmac_free_netbuf_list);

oal_module_symbol(hmac_tx_report_eth_frame);
oal_module_symbol(hmac_bridge_vap_xmit);