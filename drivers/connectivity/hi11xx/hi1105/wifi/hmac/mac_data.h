

#ifndef __MAC_DATA_H__
#define __MAC_DATA_H__

/* 1 其他头文件包含 */
#include "oal_ext_if.h"
#include "wlan_mib.h"
#include "mac_user.h"
#include "oam_ext_if.h"
#include "mac_regdomain.h"
// 此处不加extern "C" UT编译不过
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/* 2 宏定义 */
/* 暂时放在这里，需要放入oal_net.h */
#define OAL_EAPOL_INFO_POS 13
#define OAL_EAPOL_TYPE_POS 9
#define OAL_EAPOL_TYPE_KEY 3

#define MAC_DATA_TYPE_INVALID_MIN_VALUE        1535 /* 非法以太网报文类型的最小判断值，0x0600~0xFFFF为Type */

#define mac_get_snap_type(_pst_snap) (*(uint8_t *)(_pst_snap))

#define ETHER_TYPE_VLAN_88A8 0x88a8 /* VLAN TAG TPID */
#define ETHER_TYPE_VLAN_9100 0x9100 /* VLAN TAG TPID */

/* 3 枚举定义 */
/* 数据帧子类型枚举定义:5x与0x需保持一致 */
typedef enum {
    MAC_DATA_DHCP        = 0,
    MAC_DATA_EAPOL       = 1,
    MAC_DATA_ARP_RSP     = 2,
    MAC_DATA_ARP_REQ     = 3, /* 注意: 前4个vip帧类型顺序勿变 */
    MAC_DATA_DHCPV6      = 4,
    MAC_DATA_PPPOE       = 5,
    MAC_DATA_WAPI        = 6,
    MAC_DATA_MULTI_AP    = 7,
    MAC_DATA_LLDP        = 8,
    MAC_DATA_HS20        = 9,
    MAC_DATA_CHARIOT_SIG = 10,  /* chariot 信令报文 */

    MAC_DATA_RTSP        = 11,
    MAC_DATA_VIP_FRAME   = MAC_DATA_RTSP, /* 预留共14个 为VIP DATA FRAME */

    /* 非VIP报文 */
    MAC_DATA_TDLS           = 13,
    MAC_DATA_VLAN           = 14,
    MAC_DATA_ND             = 15,
    MAC_DATA_URGENT_TCP_ACK = 16,
    MAC_DATA_NORMAL_TCP_ACK = 17,
    MAC_DATA_TCP_SYN        = 18,
    MAC_DATA_DNS            = 19,
    MAC_DATA_VIP_RESV1      = 20,

    MAC_DATA_NO_VIP_RESV1   = 21,

    /* 私有 23~25 预留芯片后续按软件需求动态适配解析 */
    MAC_DATA_HILINK      = 23,    /* ont级联组网心跳报文 */
    MAC_DATA_PRIV_RESV1  = 24,
    MAC_DATA_PRIV_RESV2  = 25,

    MAC_DATA_INVALID     = 30,
    MAC_DATA_NUM         = 31,
    MAC_DATA_BUTT        = MAC_DATA_NUM
} mac_data_type_enum_uint8;

typedef enum {
    MAC_NETBUFF_PAYLOAD_ETH = 0,
    MAC_NETBUFF_PAYLOAD_SNAP,

    MAC_NETBUFF_PAYLOAD_BUTT
} mac_netbuff_payload_type;

typedef enum {
    PKT_TRACE_DATA_DHCP = 0,
    PKT_TRACE_DATA_ARP_REQ,
    PKT_TRACE_DATA_ARP_RSP,
    PKT_TRACE_DATA_EAPOL,
    PKT_TRACE_DATA_ICMP,
    PKT_TRACE_MGMT_ASSOC_REQ,
    PKT_TRACE_MGMT_ASSOC_RSP,
    PKT_TRACE_MGMT_REASSOC_REQ,
    PKT_TRACE_MGMT_REASSOC_RSP,
    PKT_TRACE_MGMT_DISASOC,
    PKT_TRACE_MGMT_AUTH,
    PKT_TRACE_MGMT_DEAUTH,
    PKT_TRACE_BUTT
} pkt_trace_type_enum;

uint8_t mac_get_dhcp_frame_type(oal_ip_header_stru *pst_rx_ip_hdr, uint32_t payload_len);
mac_eapol_type_enum_uint8 mac_get_eapol_key_type(uint8_t *pst_payload);
oal_bool_enum_uint8 mac_is_dhcp_port(mac_ip_header_stru *pst_ip_hdr, uint32_t payload_len);
oal_bool_enum_uint8 mac_is_rtsp_port(mac_ip_header_stru *pst_ip_hdr, uint32_t payload_len);
oal_bool_enum_uint8 mac_is_dns(mac_ip_header_stru *ip_hdr, uint16_t ip_len);
mac_data_type_enum_uint8 mac_get_arp_type_by_arphdr(oal_eth_arphdr_stru *pst_rx_arp_hdr, uint32_t payload_len);
oal_bool_enum_uint8 mac_is_nd(oal_ipv6hdr_stru *pst_ipv6hdr, uint32_t payload_len);
oal_bool_enum_uint8 mac_is_dhcp6(oal_ipv6hdr_stru *pst_ether_hdr, uint32_t payload_len);
uint8_t mac_get_data_type(oal_netbuf_stru *pst_netbuff);
uint8_t mac_get_data_type_from_8023(uint8_t *puc_frame_hdr,
    mac_netbuff_payload_type uc_hdr_type, uint32_t payload_len);
oal_bool_enum_uint8 mac_is_eapol_key_ptk(mac_eapol_header_stru *pst_eapol_header);
uint8_t mac_get_data_type_from_80211(oal_netbuf_stru *pst_netbuff, uint16_t us_mac_hdr_len, uint32_t payload_len);
oal_bool_enum mac_snap_is_protocol_type(uint8_t snap_type);
mac_netbuff_payload_type mac_get_eth_data_subtype(oal_netbuf_stru *netbuff,  uint32_t frame_len);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of mac_vap.h */

