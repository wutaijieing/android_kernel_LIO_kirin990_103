

/* 1 头文件包含 */
#include "oal_net.h"
#include "wlan_spec.h"
#include "wlan_types.h"
#include "mac_vap.h"
#include "mac_device.h"
#include "mac_data.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_DATA_C

/* 2 全局变量定义 */
/* 3 函数实现 */

uint8_t mac_get_data_type_from_80211(oal_netbuf_stru *pst_netbuff, uint16_t us_mac_hdr_len, uint32_t payload_len)
{
    uint8_t uc_datatype;
    mac_llc_snap_stru *pst_snap = NULL;

    if (pst_netbuff == NULL) {
        return MAC_DATA_BUTT;
    }
    pst_snap = (mac_llc_snap_stru *)(oal_netbuf_data(pst_netbuff) + us_mac_hdr_len);

    uc_datatype = mac_get_data_type_from_8023((uint8_t *)pst_snap, MAC_NETBUFF_PAYLOAD_SNAP, payload_len);

    return uc_datatype;
}


mac_netbuff_payload_type mac_get_eth_data_subtype(oal_netbuf_stru *netbuff,  uint32_t frame_len)
{
    mac_ether_header_stru *ether_hdr = NULL;

    /* len校验 */
    if (frame_len < sizeof(mac_ether_header_stru)) {
        return MAC_NETBUFF_PAYLOAD_BUTT;
    }

    ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(netbuff);
    if (oal_host2net_short(ether_hdr->us_ether_type) <= MAC_DATA_TYPE_INVALID_MIN_VALUE) {
        return MAC_NETBUFF_PAYLOAD_SNAP;
    }

    return MAC_NETBUFF_PAYLOAD_ETH;
}


oal_bool_enum_uint8 mac_is_eapol_key_ptk(mac_eapol_header_stru *pst_eapol_header)
{
    mac_eapol_key_stru *pst_key = NULL;

    if (pst_eapol_header->uc_type == IEEE802_1X_TYPE_EAPOL_KEY) {
        if ((uint16_t)(oal_net2host_short(pst_eapol_header->us_length)) >=
            (uint16_t)sizeof(mac_eapol_key_stru)) {
            pst_key = (mac_eapol_key_stru *)(pst_eapol_header + 1);

            if (pst_key->auc_key_info[1] & WPA_KEY_INFO_KEY_TYPE) {
                return OAL_TRUE;
            }
        }
    }
    return OAL_FALSE;
}


uint8_t mac_get_data_type_by_snap(oal_netbuf_stru *netbuff)
{
    uint8_t          datatype;
    mac_llc_snap_stru *snap = NULL;
    dmac_rx_ctl_stru *cb_ctrl = NULL;

    if (netbuff == NULL) {
        return MAC_DATA_BUTT;
    }
    /* 对没有snap头的数据帧进行保护 */
    cb_ctrl = (dmac_rx_ctl_stru *)oal_netbuf_cb(netbuff);
    if (cb_ctrl->st_rx_info.us_frame_len == cb_ctrl->st_rx_info.uc_mac_header_len) {
        return MAC_DATA_BUTT;
    }

    snap = (mac_llc_snap_stru *)oal_netbuf_payload(netbuff);
    if (snap == NULL) {
        return MAC_DATA_BUTT;
    }

    datatype = mac_get_data_type_from_8023((uint8_t *)snap, MAC_NETBUFF_PAYLOAD_SNAP,
        cb_ctrl->st_rx_info.us_frame_len - cb_ctrl->st_rx_info.uc_mac_header_len);

    return datatype;
}


oal_bool_enum_uint8 mac_is_eapol_key_ptk_4_4(oal_netbuf_stru *pst_netbuff)
{
    mac_eapol_header_stru *pst_eapol_header = NULL;
    mac_eapol_key_stru *pst_eapol_key = NULL;

    if ((mac_get_data_type_by_snap(pst_netbuff) == MAC_DATA_EAPOL)) {
        pst_eapol_header = (mac_eapol_header_stru *)(oal_netbuf_payload(pst_netbuff) + sizeof(mac_llc_snap_stru));
        if (mac_is_eapol_key_ptk(pst_eapol_header) == OAL_TRUE) {
            pst_eapol_key = (mac_eapol_key_stru *)(pst_eapol_header + 1);
            if (pst_eapol_key->auc_key_data_length[0] == 0 &&
                pst_eapol_key->auc_key_data_length[1] == 0) {
                return OAL_TRUE;
            }
        }
    }

    return OAL_FALSE;
}


uint8_t mac_get_dhcp_type(uint8_t *puc_pos, uint8_t *puc_packet_end)
{
    uint8_t *puc_opt = NULL;
    while ((puc_pos < puc_packet_end) && (*puc_pos != 0xFF)) {
        puc_opt = puc_pos++;
        if (*puc_opt == 0) {
            continue; /* Padding */
        }
        puc_pos += *puc_pos + 1;
        if (puc_pos >= puc_packet_end) {
            break;
        }
        if ((*puc_opt == 53) && // 53表示dhcp option id, 为Message type
            (puc_opt[1] != 0)) {
            return puc_opt[BYTE_OFFSET_2];
        }
    }
    return 0xFF;  // unknow type
}


uint8_t mac_get_dhcp_frame_type(oal_ip_header_stru *pst_rx_ip_hdr, uint32_t payload_len)
{
    uint8_t *puc_pos = NULL;
    oal_dhcp_packet_stru *pst_rx_dhcp_hdr = NULL;
    uint8_t *puc_packet_end = NULL;

    /* len本身校验 */
    if (payload_len < sizeof(oal_ip_header_stru) + sizeof(oal_udp_header_stru) + sizeof(oal_dhcp_packet_stru)) {
        oam_warning_log1(0, OAM_SF_RX, "mac_get_dhcp_frame_type::DHCP tot_len[%d].", payload_len);
        return MAC_DHCP_UNKNOWN;
    }

    puc_pos = (uint8_t *)pst_rx_ip_hdr;
    puc_pos += (puc_pos[0] & 0x0F) << BIT_OFFSET_2; /* point udp header */
    pst_rx_dhcp_hdr = (oal_dhcp_packet_stru *)(puc_pos + BYTE_OFFSET_8);

    puc_packet_end = (uint8_t *)pst_rx_ip_hdr + payload_len;
    puc_pos = &(pst_rx_dhcp_hdr->options[BYTE_OFFSET_4]);

    return mac_get_dhcp_type(puc_pos, puc_packet_end);
}


mac_eapol_type_enum_uint8 mac_get_eapol_key_type(uint8_t *pst_payload)
{
    mac_eapol_header_stru *pst_eapol_header;
    mac_eapol_key_stru *pst_eapol_key = NULL;
    oal_bool_enum_uint8 en_key_ack_set = OAL_FALSE;
    oal_bool_enum_uint8 en_key_mic_set = OAL_FALSE;
    oal_bool_enum_uint8 en_key_len_set = OAL_FALSE;
    mac_eapol_type_enum_uint8 en_eapol_type = MAC_EAPOL_PTK_BUTT;

    /* 调用此接口,需保证已默认识别Ethertype为0x888E, EAPOL */
    /* 入参payload为LLC或者Ether头后 */
    pst_eapol_header = (mac_eapol_header_stru *)(pst_payload);

    if (mac_is_eapol_key_ptk(pst_eapol_header) == OAL_TRUE) {
        pst_eapol_key = (mac_eapol_key_stru *)(pst_eapol_header + 1);

        if (pst_eapol_key->auc_key_info[1] & WPA_KEY_INFO_KEY_ACK) {
            en_key_ack_set = OAL_TRUE;
        }

        if (pst_eapol_key->auc_key_info[0] & WPA_KEY_INFO_KEY_MIC) {
            en_key_mic_set = OAL_TRUE;
        }

        if (pst_eapol_key->auc_key_data_length[0] != 0 || pst_eapol_key->auc_key_data_length[1] != 0) {
            en_key_len_set = OAL_TRUE;
        }

        /* ack设置,表明STA发起期望响应 */
        if (en_key_ack_set == OAL_TRUE) {
            if ((en_key_mic_set == OAL_FALSE) && (en_key_len_set == OAL_FALSE)) {
                /* 1/4识别:ACK=1,MIC=0,LEN=0 */
                en_eapol_type = MAC_EAPOL_PTK_1_4;
            } else if ((en_key_mic_set == OAL_TRUE) && (en_key_len_set == OAL_TRUE)) {
                /* 3/4识别:ACK=1,MIC=1,LEN=1 */
                en_eapol_type = MAC_EAPOL_PTK_3_4;
            }
        } else {
            if ((en_key_mic_set == OAL_TRUE) && (en_key_len_set == OAL_TRUE)) {
                /* 2/4识别:ACK=0,MIC=0,LEN=1 */
                en_eapol_type = MAC_EAPOL_PTK_2_4;
            } else if ((en_key_mic_set == OAL_TRUE) && (en_key_len_set == OAL_FALSE)) {
                /* 4/4识别:ACK=0,MIC=1,LEN=0 */
                en_eapol_type = MAC_EAPOL_PTK_4_4;
            }
        }
    }

    return en_eapol_type;
}

oal_bool_enum mac_snap_is_protocol_type(uint8_t uc_snap_type)
{
    if (oal_likely(uc_snap_type == SNAP_LLC_LSAP)) {
        return OAL_TRUE;
    } else {
        oam_warning_log1(0, OAM_SF_RX, "mac_snap_is_protocol_type:: rx a frame not 0xaa snap type[%x]", uc_snap_type);
        if (uc_snap_type == SNAP_BRIDGE_SPANNING_TREE || uc_snap_type == SNAP_IBM_LAN_MANAGEMENT) {
            return OAL_FALSE;
        }
    }

    /* 无法识别的类型默认protocl type形式，与原逻辑保持一致 */
    return OAL_TRUE;
}


#define MAC_DHCP_PORT_67 67
#define MAC_DHCP_PORT_68 68
#define IP_HEADER_FRAGMENT_MASK 0xFF1F
oal_bool_enum_uint8 mac_is_dhcp_port(mac_ip_header_stru *pst_ip_hdr, uint32_t payload_len)
{
    udp_hdr_stru *pst_udp_hdr = NULL;
    uint16_t ip_header_len = IP_HEADER_LEN(pst_ip_hdr->ip_header_len);
    if (payload_len < ip_header_len + sizeof(udp_hdr_stru)) {
        return OAL_FALSE;
    }

    /* DHCP判断标准: udp协议，ip头部fragment offset字段为0，目的端口号为67或68 */
    if (pst_ip_hdr->uc_protocol == MAC_UDP_PROTOCAL && ((pst_ip_hdr->us_frag_off & IP_HEADER_FRAGMENT_MASK) == 0)) {
        pst_udp_hdr = (udp_hdr_stru *)(((uint8_t *)pst_ip_hdr) + ip_header_len);
        if (oal_net2host_short(pst_udp_hdr->us_des_port) == MAC_DHCP_PORT_67 ||
            oal_net2host_short(pst_udp_hdr->us_des_port) == MAC_DHCP_PORT_68) {
            return OAL_TRUE;
        }
    }
    return OAL_FALSE;
}
oal_bool_enum_uint8 mac_is_rtsp_port(mac_ip_header_stru *pst_ip_hdr, uint32_t payload_len)
{
    mac_tcp_header_stru *tcp_hdr = (mac_tcp_header_stru *)(pst_ip_hdr + 1);
    if (payload_len < sizeof(mac_ip_header_stru) + sizeof(mac_tcp_header_stru)) {
        return OAL_FALSE;
    }

    /* DHCP判断标准: udp协议，ip头部fragment offset字段为0，目的端口号为67或68 */
    if (pst_ip_hdr->uc_protocol == MAC_TCP_PROTOCAL) {
        if ((oal_net2host_short(tcp_hdr->us_dport) == MAC_WFD_RTSP_PORT)) {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}
#define DNS_SERVER_PORT 53
/*
 * 函 数 名   : mac_is_dns_frame
 * 功能描述   : 判断是否是dns帧:满足udp协议，且不是分片报文，以及端口号限制
 */
oal_bool_enum_uint8 mac_is_dns(mac_ip_header_stru *ip_hdr, uint16_t ip_len)
{
    udp_hdr_stru *udp_hdr = NULL;
    uint16_t ip_header_len = IP_HEADER_LEN(ip_hdr->ip_header_len);
    /* DNS判断标准: udp协议，ip头部fragment offset字段为0，目的端口号为53 */
    if ((ip_hdr->uc_protocol != MAC_UDP_PROTOCAL) || ((ip_hdr->us_frag_off & IP_HEADER_FRAGMENT_MASK) != 0)) {
        return OAL_FALSE;
    }
    /* 判断IP包长度是否正常 */
    if (ip_len < ip_header_len + sizeof(udp_hdr_stru)) {
        return OAL_FALSE;
    }
    udp_hdr = (udp_hdr_stru *)(((uint8_t *)ip_hdr) + ip_header_len);
    if (oal_net2host_short(udp_hdr->us_des_port) != DNS_SERVER_PORT) {
        return OAL_FALSE;
    }
    return OAL_TRUE;
}


oal_bool_enum_uint8 mac_is_nd(oal_ipv6hdr_stru *pst_ipv6hdr, uint32_t payload_len)
{
    oal_icmp6hdr_stru *pst_icmp6hdr = NULL;
    if (payload_len < sizeof(oal_ipv6hdr_stru) + sizeof(oal_icmp6hdr_stru)) {
        return OAL_FALSE;
    }

    if (pst_ipv6hdr->nexthdr == OAL_IPPROTO_ICMPV6) {
        pst_icmp6hdr = (oal_icmp6hdr_stru *)(pst_ipv6hdr + 1);

        if ((pst_icmp6hdr->icmp6_type == MAC_ND_RSOL) ||
            (pst_icmp6hdr->icmp6_type == MAC_ND_RADVT) ||
            (pst_icmp6hdr->icmp6_type == MAC_ND_NSOL) ||
            (pst_icmp6hdr->icmp6_type == MAC_ND_NADVT) ||
            (pst_icmp6hdr->icmp6_type == MAC_ND_RMES)) {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}


oal_bool_enum_uint8 mac_is_dhcp6(oal_ipv6hdr_stru *pst_ipv6hdr, uint32_t payload_len)
{
    udp_hdr_stru *pst_udp_hdr = NULL;
    if (payload_len < sizeof(oal_ipv6hdr_stru) + sizeof(udp_hdr_stru)) {
        return OAL_FALSE;
    }

    if (pst_ipv6hdr->nexthdr == MAC_UDP_PROTOCAL) {
        pst_udp_hdr = (udp_hdr_stru *)(pst_ipv6hdr + 1);

        if (pst_udp_hdr->us_des_port == oal_host2net_short(MAC_IPV6_UDP_DES_PORT) ||
            pst_udp_hdr->us_des_port == oal_host2net_short(MAC_IPV6_UDP_SRC_PORT)) {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}

mac_data_type_enum_uint8 mac_get_arp_type_by_arphdr(oal_eth_arphdr_stru *pst_rx_arp_hdr, uint32_t payload_len)
{
    if (payload_len < sizeof(oal_eth_arphdr_stru)) {
        return MAC_DATA_BUTT;
    }
    if (MAC_ARP_REQUEST == oal_net2host_short(pst_rx_arp_hdr->us_ar_op)) {
        return MAC_DATA_ARP_REQ;
    } else if (MAC_ARP_RESPONSE == oal_net2host_short(pst_rx_arp_hdr->us_ar_op)) {
        return MAC_DATA_ARP_RSP;
    }

    return MAC_DATA_BUTT;
}

static uint8_t mac_get_data_type_by_ether_type(uint16_t ether_type, uint8_t *frame_body, uint32_t payload_len)
{
    mac_ip_header_stru  *ip = NULL;
    uint8_t              datatype = MAC_DATA_BUTT;

    switch (ether_type) {
            /* lint -e778 */ /* 屏蔽Info-- Constant expression evaluates to 0 in operation '&' */
        case oal_host2net_short(ETHER_TYPE_IP):
            /* 从IP TOS字段寻找优先级 */
            /* ----------------------------------------------------------------------
                tos位定义
             ----------------------------------------------------------------------
            | bit7~bit5 | bit4 |  bit3  |  bit2  |   bit1   | bit0 |
            | 包优先级  | 时延 | 吞吐量 | 可靠性 | 传输成本 | 保留 |
             ---------------------------------------------------------------------- */
            ip = (mac_ip_header_stru *)frame_body; /* 偏移一个以太网头，取ip头 */

            if (mac_is_dhcp_port(ip, payload_len) == OAL_TRUE) {
                datatype = MAC_DATA_DHCP;
            }
            if (mac_is_rtsp_port(ip, payload_len) == OAL_TRUE) {
                datatype = MAC_DATA_RTSP;
            }
            break;
        case oal_host2net_short(ETHER_TYPE_IPV6):
            /* 从IPv6 traffic class字段获取优先级 */
            /* ----------------------------------------------------------------------
                IPv6包头 前32为定义
             -----------------------------------------------------------------------
            | 版本号 | traffic class   | 流量标识 |
            | 4bit   | 8bit(同ipv4 tos)|  20bit   |
            ----------------------------------------------------------------------- */
            /* 如果是ND帧，则进入VO队列发送 */
            if (mac_is_nd((oal_ipv6hdr_stru *)frame_body, payload_len) == OAL_TRUE) {
                datatype = MAC_DATA_ND;
            } else if (mac_is_dhcp6((oal_ipv6hdr_stru *)frame_body, payload_len) == OAL_TRUE) { /* 如果是DHCPV6帧 */
                datatype = MAC_DATA_DHCPV6;
            }
            break;
        case oal_host2net_short(ETHER_TYPE_PAE):
            /* 如果是EAPOL帧，则进入VO队列发送 */
            datatype = MAC_DATA_EAPOL; /* eapol */
            break;
        /* TDLS帧处理，建链保护，入高优先级TID队列 */
        case oal_host2net_short(ETHER_TYPE_TDLS):
            datatype = MAC_DATA_TDLS;
            break;
        /* PPPOE帧处理，建链保护(发现阶段, 会话阶段)，入高优先级TID队列 */
        case oal_host2net_short(ETHER_TYPE_PPP_DISC):
        case oal_host2net_short(ETHER_TYPE_PPP_SES):
            datatype = MAC_DATA_PPPOE;
            break;
        case oal_host2net_short(ETHER_TYPE_WAI):
            datatype = MAC_DATA_WAPI;
            break;
        case oal_host2net_short(ETHER_TYPE_VLAN):
        case oal_host2net_short(ETHER_TYPE_VLAN_88A8):
        case oal_host2net_short(ETHER_TYPE_VLAN_9100):
            datatype = MAC_DATA_VLAN;
            break;
        case oal_host2net_short(ETHER_TYPE_ARP):
            /* 如果是ARP帧，则进入VO队列发送 */
            datatype = (uint8_t)mac_get_arp_type_by_arphdr((oal_eth_arphdr_stru *)frame_body, payload_len);
            break;
        /* lint +e778 */
        default:
            datatype = MAC_DATA_BUTT;
            break;
    }

    return datatype;
}

uint8_t mac_get_data_type_from_8023(uint8_t *frame_hdr, mac_netbuff_payload_type hdr_type, uint32_t payload_len)
{
    uint8_t           *frame_body = NULL;
    uint16_t           ether_type;
    uint8_t            datatype = MAC_DATA_BUTT;
    uint32_t           shift_len;

    if (frame_hdr == NULL) {
        return datatype;
    }

    if (hdr_type == MAC_NETBUFF_PAYLOAD_ETH) {
        shift_len = ETHER_HDR_LEN;
    } else if (hdr_type == MAC_NETBUFF_PAYLOAD_SNAP) {
        shift_len = SNAP_LLC_FRAME_LEN;
    } else {
        return datatype;
    }
    if (payload_len < shift_len) {
        oam_error_log2(0, OAM_SF_ANY, "mac_get_data_type_from_8023:payload_len=%d, shift_len=%d",
            payload_len, shift_len);
        return datatype;
    }
    if (hdr_type == MAC_NETBUFF_PAYLOAD_ETH) {
        ether_type = ((mac_ether_header_stru *)frame_hdr)->us_ether_type;
    } else {
        ether_type = ((mac_llc_snap_stru *)frame_hdr)->us_ether_type;
    }
    frame_body = frame_hdr + shift_len;
    datatype = mac_get_data_type_by_ether_type(ether_type, frame_body, payload_len - shift_len);
    return datatype;
}