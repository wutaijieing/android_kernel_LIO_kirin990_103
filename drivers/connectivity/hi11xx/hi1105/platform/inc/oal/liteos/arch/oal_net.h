

#ifndef __OAL_LITEOS_NET_H__
#define __OAL_LITEOS_NET_H__


#include "oal_list.h"
#include "oal_util.h"
#include "oal_mutex.h"
#include <arch/oal_skbuff.h>
#include <arch/oal_types.h>
#include <arch/oal_schedule.h>
#include "platform_spec.h"
#include "los_task.h"
#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define OAL_BITFIELD_LITTLE_ENDIAN      0
#define OAL_BITFIELD_BIG_ENDIAN         1

#define OAL_WLAN_SA_QUERY_TR_ID_LEN    2
/* wiphy  */
#define IEEE80211_HT_MCS_MASK_LEN   10
#define OAL_IF_NAME_SIZE   16
#define ETHER_ADDR_LEN  6   /* length of an Ethernet address */
#define OAL_IEEE80211_MAX_SSID_LEN          32 /* 最大SSID长度 */

#define ETHER_HDR_LEN   14
#define LWIP_PBUF_STRUCT_SIZE OAL_GET_4BYTE_ALIGN_VALUE(sizeof(oal_lwip_buf)) /* struct pbuf 结构体大小 */
/*****************************************************************************
  2.10 IP宏定义
*****************************************************************************/
#define IPV6_ADDR_MULTICAST     0x0002U
#define IPV6_ADDR_UNICAST       0x0001U
#define IPV6_ADDR_SCOPE_TYPE(scope) ((scope) << 16)
#define IPV6_ADDR_SCOPE_NODELOCAL   0x01
#define IPV6_ADDR_SCOPE_LINKLOCAL   0x02
#define IPV6_ADDR_SCOPE_SITELOCAL   0x05
#define IPV6_ADDR_SCOPE_ORGLOCAL    0x08
#define IPV6_ADDR_SCOPE_GLOBAL      0x0e
#define IPV6_ADDR_LOOPBACK  0x0010U
#define IPV6_ADDR_LINKLOCAL 0x0020U
#define IPV6_ADDR_SITELOCAL 0x0040U
#define IPV6_ADDR_RESERVED  0x2000U /* reserved address space */

#define IPV6_ADDR_MC_SCOPE(a)   \
    ((a)->s6_addr[1] & 0x0f)    /* nonstandard */

/* ether type */
#define UEVENT_NUM_ENVP         32  /* number of env pointers */
#define UEVENT_BUFFER_SIZE      2048    /* buffer for the variables */

#define ETHER_TYPE_RARP  0x8035
#define ETHER_TYPE_PAE   0x888e  /* EAPOL PAE/802.1x */
#define ETHER_TYPE_IP    0x0800  /* IP protocol */
#define ETHER_TYPE_AARP  0x80f3  /* Appletalk AARP protocol */
#define ETHER_TYPE_IPX   0x8137  /* IPX over DIX protocol */
#define ETHER_TYPE_ARP   0x0806  /* ARP protocol */
#define ETHER_TYPE_IPV6  0x86dd  /* IPv6 */
#define ETHER_TYPE_TDLS  0x890d  /* TDLS */
#define ETHER_TYPE_VLAN  0x8100  /* VLAN TAG protocol */
#define ETHER_TYPE_WAI   0x88b4  /* WAI/WAPI */
#define ETHER_LLTD_TYPE  0x88D9  /* LLTD */
#define ETHER_ONE_X_TYPE 0x888E  /* 802.1x Authentication */
#define ETHER_TUNNEL_TYPE 0x88bd  /* 自定义tunnel协议 */
#define ETHER_TYPE_PPP_DISC 0x8863      /* PPPoE discovery messages */
#define ETHER_TYPE_PPP_SES  0x8864      /* PPPoE session messages */

#define ETH_SENDER_IP_ADDR_LEN       4  /* length of an Ethernet send ip address */
#define ETH_TARGET_IP_ADDR_LEN       4  /* length of an Ethernet target ip address */


#define oal_netif_set_up       netifapi_netif_set_up
#define oal_netif_set_down     netifapi_netif_set_down

#define oal_dhcp_start         netifapi_dhcp_start
#define oal_dhcp_stop          netifapi_dhcp_stop
#define oal_dhcp_succ_check    netifapi_dhcp_is_bound

#define oal_netif_running(_pst_net_dev)             0

#define oal_smp_mb()
#define OAL_CONTAINER_OF(_member_ptr, _stru_type, _stru_member_name) \
    container_of(_member_ptr, _stru_type, _stru_member_name)

#define OAL_NETBUF_LIST_NUM(_pst_head)              ((_pst_head)->qlen)
#define OAL_NET_DEV_PRIV(_pst_dev)                  ((_pst_dev)->ml_priv)
#define OAL_NET_DEV_WIRELESS_PRIV(_pst_dev)         (netdev_priv(_pst_dev))
#define OAL_NET_DEV_WIRELESS_DEV(_pst_dev)          ((_pst_dev)->ieee80211_ptr)
#define oal_netbuf_next(_pst_buf)                   ((_pst_buf)->next)
#define oal_netbuf_prev(_pst_buf)                   ((_pst_buf)->prev)
#define oal_netbuf_head_next(_pst_buf_head)         ((_pst_buf_head)->next)
#define OAL_NETBUF_HEAD_PREV(_pst_buf_head)         ((_pst_buf_head)->prev)

#define OAL_NETBUF_PROTOCOL(_pst_buf)               ((_pst_buf)->protocol)
#define OAL_NETBUF_LAST_RX(_pst_buf)                ((_pst_buf)->last_rx)
#define OAL_NETBUF_DATA(_pst_buf)                   ((_pst_buf)->data)
#define OAL_NETBUF_HEADER(_pst_buf)                 ((_pst_buf)->data)
#define OAL_NETBUF_PAYLOAD(_pst_buf)                 ((_pst_buf)->data)

#define oal_netbuf_cb(_pst_buf)                     ((_pst_buf)->cb)
#define oal_netbuf_cb_size()                        (OAL_SIZEOF(((oal_netbuf_stru*)0)->cb))
#define oal_netbuf_len(_pst_buf)                    ((_pst_buf)->len)
#define OAL_NETBUF_TAIL                              skb_tail_pointer
#define OAL_NETBUF_QUEUE_TAIL                        skb_queue_tail
#define OAL_NETBUF_QUEUE_HEAD_INIT                   skb_queue_head_init
#define OAL_NETBUF_DEQUEUE                           skb_dequeue
#define OAL_NETBUF_QUEUE_TAIL                        skb_queue_tail
#define get_netbuf_payload(_pst_buf)                 ((_pst_buf)->data)

#define OAL_NETDEVICE_OPS(_pst_dev)                         ((_pst_dev)->netdev_ops)
#define OAL_NETDEVICE_OPS_OPEN(_pst_netdev_ops)             ((_pst_netdev_ops)->ndo_open)
#define OAL_NETDEVICE_OPS_STOP(_pst_netdev_ops)             ((_pst_netdev_ops)->ndo_stop)
#define OAL_NETDEVICE_OPS_START_XMIT(_pst_netdev_ops)       ((_pst_netdev_ops)->ndo_start_xmit)
#define OAL_NETDEVICE_OPS_SET_MAC_ADDR(_pst_netdev_ops)     ((_pst_netdev_ops)->ndo_set_mac_address)
#define OAL_NETDEVICE_OPS_TX_TIMEOUT(_pst_netdev_ops)       ((_pst_netdev_ops)->ndo_tx_timeout)
#define OAL_NETDEVICE_OPS_SET_MC_LIST(_pst_netdev_ops)      ((_pst_netdev_ops)->ndo_set_multicast_list)
#define OAL_NETDEVICE_OPS_GET_STATS(_pst_netdev_ops)        ((_pst_netdev_ops)->ndo_get_stats)
#define OAL_NETDEVICE_OPS_DO_IOCTL(_pst_netdev_ops)         ((_pst_netdev_ops)->ndo_do_ioctl)
#define OAL_NETDEVICE_OPS_CHANGE_MTU(_pst_netdev_ops)       ((_pst_netdev_ops)->ndo_change_mtu)

#define OAL_NETDEVICE_LAST_RX(_pst_dev)                     ((_pst_dev)->last_rx)
#define OAL_NETDEVICE_WIRELESS_HANDLERS(_pst_dev)           ((_pst_dev)->wireless_handlers)
#define OAL_NETDEVICE_RTNL_LINK_OPS(_pst_dev)               ((_pst_dev)->rtnl_link_ops)
#define OAL_NETDEVICE_RTNL_LINK_STATE(_pst_dev)             ((_pst_dev)->rtnl_link_state)
#define OAL_NETDEVICE_MAC_ADDR(_pst_dev)                    ((_pst_dev)->dev_addr)
#define OAL_NETDEVICE_TX_QUEUE_LEN(_pst_dev)                ((_pst_dev)->tx_queue_len)
#define OAL_NETDEVICE_TX_QUEUE_NUM(_pst_dev)                ((_pst_dev)->num_tx_queues)
#define OAL_NETDEVICE_TX_QUEUE(_pst_dev, _index)            ((_pst_dev)->_tx[_index])
#define OAL_NETDEVICE_DESTRUCTOR(_pst_dev)                  ((_pst_dev)->destructor)
#define OAL_NETDEVICE_TYPE(_pst_dev)                        ((_pst_dev)->type)
#define OAL_NETDEVICE_NAME(_pst_dev)                        ((_pst_dev)->name)
#define OAL_NETDEVICE_MASTER(_pst_dev)                      ((_pst_dev)->master)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#define OAL_NETDEVICE_QDISC(_pst_dev, pst_val)              ((_pst_dev)->qdisc = pst_val)
#else
#define OAL_NETDEVICE_QDISC(_pst_dev, pst_val)
#endif
#define OAL_NETDEVICE_IFALIAS(_pst_dev)                     ((_pst_dev)->ifalias)
#define OAL_NETDEVICE_WDEV(_pst_dev)                        ((_pst_dev)->ieee80211_ptr)
#define OAL_NETDEVICE_HEADROOM(_pst_dev)                    ((_pst_dev)->needed_headroom)
#define OAL_NETDEVICE_TAILROOM(_pst_dev)                    ((_pst_dev)->needed_tailroom)
#define OAL_NETDEVICE_FLAGS(_pst_dev)                       ((_pst_dev)->flags)
#define OAL_NETDEVICE_ADDR_LEN(_pst_dev)                    ((_pst_dev)->addr_len)
#define OAL_NETDEVICE_WATCHDOG_TIMEO(_pst_dev)              ((_pst_dev)->watchdog_timeo)
#define OAL_NETDEVICE_HARD_HEADER_LEN(_pst_dev)             ((_pst_dev)->hard_header_len)

#define OAL_WIRELESS_DEV_NET_DEV(_pst_wireless_dev)         ((_pst_wireless_dev)->netdev)
#define OAL_WIRELESS_DEV_WIPHY(_pst_wireless_dev)           ((_pst_wireless_dev)->wiphy)
#define OAL_WIRELESS_DEV_IF_TYPE(_pst_wireless_dev)         ((_pst_wireless_dev)->iftype)

#define NL80211_RRF_DFS             (1<<4)

#define SIOCIWFIRSTPRIV 0x8BE0

#define OAL_IFF_RUNNING         IFF_RUNNING
#define OAL_SIOCIWFIRSTPRIV     SIOCIWFIRSTPRIV

#define IW_PRIV_TYPE_BYTE   0x1000  /* Char as number */
#define IW_PRIV_TYPE_CHAR   0x2000  /* Char as character */
#define IW_PRIV_TYPE_INT    0x4000  /* 32 bits int */
#define IW_PRIV_TYPE_ADDR   0x6000  /* struct sockaddr */

#define IW_PRIV_SIZE_FIXED  0x0800  /* Variable or fixed number of args */

/* iw_priv参数类型OAL封装 */
#define OAL_IW_PRIV_TYPE_BYTE   IW_PRIV_TYPE_BYTE       /* Char as number */
#define OAL_IW_PRIV_TYPE_CHAR   IW_PRIV_TYPE_CHAR       /* Char as character */
#define OAL_IW_PRIV_TYPE_INT    IW_PRIV_TYPE_INT        /* 32 bits int */
#define OAL_IW_PRIV_TYPE_FLOAT  IW_PRIV_TYPE_FLOAT      /* struct iw_freq */
#define OAL_IW_PRIV_TYPE_ADDR   IW_PRIV_TYPE_ADDR       /* struct sockaddr */
#define OAL_IW_PRIV_SIZE_FIXED  IW_PRIV_SIZE_FIXED      /* Variable or fixed number of args */

/* iwconfig mode oal封装 */
#define OAL_IW_MODE_AUTO    IW_MODE_AUTO    /* Let the driver decides */
#define OAL_IW_MODE_ADHOC   IW_MODE_ADHOC   /* Single cell network */
#define OAL_IW_MODE_INFRA   IW_MODE_INFRA   /* Multi cell network, roaming, ... */
#define OAL_IW_MODE_MASTER  IW_MODE_MASTER  /* Synchronisation master or Access Point */
#define OAL_IW_MODE_REPEAT  IW_MODE_REPEAT  /* Wireless Repeater (forwarder) */
#define OAL_IW_MODE_SECOND  IW_MODE_SECOND  /* Secondary master/repeater (backup) */
#define OAL_IW_MODE_MONITOR IW_MODE_MONITOR /* Passive monitor (listen only) */
#define OAL_IW_MODE_MESH    IW_MODE_MESH    /* Mesh (IEEE 802.11s) network */

/* Transmit Power flags available */
#define OAL_IW_TXPOW_TYPE       IW_TXPOW_TYPE           /* Type of value */
#define OAL_IW_TXPOW_DBM        IW_TXPOW_DBM            /* Value is in dBm */
#define OAL_IW_TXPOW_MWATT      IW_TXPOW_MWATT          /* Value is in mW */
#define OAL_IW_TXPOW_RELATIVE   IW_TXPOW_RELATIVE       /* Value is in arbitrary units */
#define OAL_IW_TXPOW_RANGE      IW_TXPOW_RANGE          /* Range of value between min/max */

/* 主机与网络字节序转换 */
#define oal_host2net_short(_x)  oal_swap_byteorder_16(_x)
#define oal_net2host_short(_x)  oal_swap_byteorder_16(_x)
#define OAL_HOST2NET_LONG(_x)   OAL_SWAP_BYTEORDER_32(_x)
#define oal_net2host_long(_x)   OAL_SWAP_BYTEORDER_32(_x)

#define OAL_INET_ECN_NOT_ECT    INET_ECN_NOT_ECT
#define OAL_INET_ECN_ECT_1      INET_ECN_ECT_1
#define OAL_INET_ECN_ECT_0      INET_ECN_ECT_0
#define OAL_INET_ECN_CE         INET_ECN_CE
#define OAL_INET_ECN_MASK       INET_ECN_MASK

/* 提取vlan信息 */
#define oal_vlan_tx_tag_present(_skb)   vlan_tx_tag_present(_skb)
#define oal_vlan_tx_tag_get(_skb)       vlan_tx_tag_get(_skb)

/* vlan宏定义 */
#define OAL_VLAN_VID_MASK       VLAN_VID_MASK       /* VLAN Identifier */
#define OAL_VLAN_PRIO_MASK      VLAN_PRIO_MASK      /* Priority Code Point */


#define OAL_VLAN_PRIO_SHIFT     13

/* ARP protocol opcodes. */
#define OAL_ARPOP_REQUEST      ARPOP_REQUEST         /* ARP request          */
#define OAL_ARPOP_REPLY        ARPOP_REPLY           /* ARP reply            */
#define OAL_ARPOP_RREQUEST     ARPOP_RREQUEST        /* RARP request         */
#define OAL_ARPOP_RREPLY       ARPOP_RREPLY          /* RARP reply           */
#define OAL_ARPOP_InREQUEST    ARPOP_InREQUEST       /* InARP request        */
#define OAL_ARPOP_InREPLY      ARPOP_InREPLY         /* InARP reply          */
#define OAL_ARPOP_NAK          ARPOP_NAK             /* (ATM)ARP NAK         */

#define  OAL_IPPROTO_UDP     IPPROTO_UDP         /* User Datagram Protocot */
#define  OAL_IPPROTO_ICMPV6  IPPROTO_ICMPV6      /* ICMPv6 */

#define OAL_IEEE80211_EXTRA_SSID_LEN 4  /* 因上层逻辑需要增加的额外SSID长度 */
#define OAL_INIT_NET            init_net
#define OAL_THIS_MODULE         THIS_MODULE
#define OAL_MSG_DONTWAIT        MSG_DONTWAIT


#define OAL_NL80211_MAX_NR_CIPHER_SUITES    5
#define OAL_NL80211_MAX_NR_AKM_SUITES       2

#define ETH_P_CONTROL   0x0016      /* Card specific control frames */
#undef  IFF_RUNNING
#define IFF_RUNNING        0x40     /* interface RFC2863 OPER_UP    */
#define IPPROTO_ICMPV6  58  /* ICMPv6 */
#define OAL_MAX_SCAN_CHANNELS               40  /* 内核下发的最大扫描信道个数 */

#define WLAN_SA_QUERY_TR_ID_LEN 2

typedef gfp_t        oal_gfp_enum_uint8;

#define OAL_NETDEV_TX_OK     NETDEV_TX_OK
#define OAL_NETDEV_TX_BUSY   NETDEV_TX_BUSY
#define OAL_NETDEV_TX_LOCKED NETDEV_TX_LOCKED
/**
 * oal_is_broadcast_ether_addr - Determine if the Ethernet address is broadcast
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Return true if the address is the broadcast address.
 */
static inline int32_t check_broadcast_ether_addr(const int8_t *a)
{
    return (a[0] & a[1] & a[2] & a[3] & a[4] & a[5]) == 0xff;
}
#define oal_is_broadcast_ether_addr(addr) check_broadcast_ether_addr(addr)

typedef enum {
    OAL_NETDEV_TX_OK     = 0x00,
    OAL_NETDEV_TX_BUSY   = 0x10,
    OAL_NETDEV_TX_LOCKED = 0x20,
} oal_net_dev_tx_enum;
enum nl80211_band {
    NL80211_BAND_2GHZ,
    NL80211_BAND_5GHZ,
    NL80211_BAND_60GHZ,
};
enum ieee80211_band {
    IEEE80211_BAND_2GHZ = NL80211_BAND_2GHZ,
    IEEE80211_BAND_5GHZ = NL80211_BAND_5GHZ,

    /* keep last */
    IEEE80211_NUM_BANDS
};


enum nl80211_channel_type {
    NL80211_CHAN_NO_HT,
    NL80211_CHAN_HT20,
    NL80211_CHAN_HT40MINUS,
    NL80211_CHAN_HT40PLUS
};

enum kobject_action {
    KOBJ_ADD,
    KOBJ_REMOVE,
    KOBJ_CHANGE,
    KOBJ_MOVE,
    KOBJ_ONLINE,
    KOBJ_OFFLINE,
    KOBJ_MAX
};

enum nl80211_iftype {
    NL80211_IFTYPE_UNSPECIFIED,
    NL80211_IFTYPE_ADHOC,
    NL80211_IFTYPE_STATION,
    NL80211_IFTYPE_AP,
    NL80211_IFTYPE_AP_VLAN,
    NL80211_IFTYPE_WDS,
    NL80211_IFTYPE_MONITOR,
    NL80211_IFTYPE_MESH_POINT,
    NL80211_IFTYPE_P2P_CLIENT,
    NL80211_IFTYPE_P2P_GO,
    NL80211_IFTYPE_P2P_DEVICE,

    /* keep last */
    NUM_NL80211_IFTYPES,
    NL80211_IFTYPE_MAX = NUM_NL80211_IFTYPES - 1
};

typedef enum nl80211_channel_type oal_nl80211_channel_type;

/* 由于02 device是裸系统，需要使用uint8，所有02用uint8类型 51不改变 */
typedef uint8_t  oal_nl80211_auth_type_enum_uint8;

typedef enum {
    NETIF_FLOW_CTRL_OFF,      // stop flow_ctrl, continue to transfer data to driver
    NETIF_FLOW_CTRL_ON,       // start flow_ctrl, stop transferring data to driver

    NETIF_FLOW_CTRL_BUTT
} netif_flow_ctrl_enum;
typedef uint8_t netif_flow_ctrl_enum_uint8;
/*
 * nl80211_external_auth_action - Action to perform with external
 *     authentication request. Used by NL80211_ATTR_EXTERNAL_AUTH_ACTION.
 * @NL80211_EXTERNAL_AUTH_START: Start the authentication.
 * @NL80211_EXTERNAL_AUTH_ABORT: Abort the ongoing authentication.
 */
enum nl80211_external_auth_action {
    NL80211_EXTERNAL_AUTH_START,
    NL80211_EXTERNAL_AUTH_ABORT,
};

/**
 * enum nl80211_chan_width - channel width definitions
 *
 * These values are used with the %NL80211_ATTR_CHANNEL_WIDTH
 * attribute.
 *
 * @NL80211_CHAN_WIDTH_20_NOHT: 20 MHz, non-HT channel
 * @NL80211_CHAN_WIDTH_20: 20 MHz HT channel
 * @NL80211_CHAN_WIDTH_40: 40 MHz channel, the %NL80211_ATTR_CENTER_FREQ1
 *	attribute must be provided as well
 * @NL80211_CHAN_WIDTH_80: 80 MHz channel, the %NL80211_ATTR_CENTER_FREQ1
 *	attribute must be provided as well
 * @NL80211_CHAN_WIDTH_80P80: 80+80 MHz channel, the %NL80211_ATTR_CENTER_FREQ1
 *	and %NL80211_ATTR_CENTER_FREQ2 attributes must be provided as well
 * @NL80211_CHAN_WIDTH_160: 160 MHz channel, the %NL80211_ATTR_CENTER_FREQ1
 *	attribute must be provided as well
 * @NL80211_CHAN_WIDTH_5: 5 MHz OFDM channel
 * @NL80211_CHAN_WIDTH_10: 10 MHz OFDM channel
 */
enum nl80211_chan_width {
    NL80211_CHAN_WIDTH_20_NOHT,
    NL80211_CHAN_WIDTH_20,
    NL80211_CHAN_WIDTH_40,
    NL80211_CHAN_WIDTH_80,
    NL80211_CHAN_WIDTH_80P80,
    NL80211_CHAN_WIDTH_160,
    NL80211_CHAN_WIDTH_5,
    NL80211_CHAN_WIDTH_10,
};

#define LL_ALLOCATED_SPACE(dev) \
    ((((dev)->hard_header_len+(dev)->needed_headroom+(dev)->needed_tailroom)&~(15)) + 16)

#define LL_RESERVED_SPACE(dev) \
    ((((dev)->hard_header_len+(dev)->needed_headroom)&~(15)) + 16)

#define OAL_LL_ALLOCATED_SPACE  LL_ALLOCATED_SPACE

/* 管制域相关结构体定义 */
#define MHZ_TO_KHZ(freq) ((freq) * 1000)
#define KHZ_TO_MHZ(freq) ((freq) / 1000)
#define DBI_TO_MBI(gain) ((gain) * 100)
#define MBI_TO_DBI(gain) ((gain) / 100)
#define DBM_TO_MBM(gain) ((gain) * 100)
#define MBM_TO_DBM(gain) ((gain) / 100)

#define REG_RULE(start, end, bw, gain, eirp, reg_flags) \
    {                           \
        .freq_range.start_freq_khz = MHZ_TO_KHZ(start), \
        .freq_range.end_freq_khz = MHZ_TO_KHZ(end), \
        .freq_range.max_bandwidth_khz = MHZ_TO_KHZ(bw), \
        .power_rule.max_antenna_gain = DBI_TO_MBI(gain), \
        .power_rule.max_eirp = DBM_TO_MBM(eirp),    \
        .flags = (reg_flags),             \
    }

enum cfg80211_signal_type {
    CFG80211_SIGNAL_TYPE_NONE,
    CFG80211_SIGNAL_TYPE_MBM,
    CFG80211_SIGNAL_TYPE_UNSPEC,
};

#define IEEE80211_MAX_SSID_LEN      32
#define NL80211_MAX_NR_CIPHER_SUITES        5
#define NL80211_MAX_NR_AKM_SUITES       2
#define NL80211_RRF_NO_OUTDOOR      (1<<3)
#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

enum station_info_flags {
    STATION_INFO_INACTIVE_TIME = 1<<0,
    STATION_INFO_RX_BYTES      = 1<<1,
    STATION_INFO_TX_BYTES      = 1<<2,
    STATION_INFO_LLID          = 1<<3,
    STATION_INFO_PLID          = 1<<4,
    STATION_INFO_PLINK_STATE   = 1<<5,
    STATION_INFO_SIGNAL        = 1<<6,
    STATION_INFO_TX_BITRATE    = 1<<7,
    STATION_INFO_RX_PACKETS    = 1<<8,
    STATION_INFO_TX_PACKETS    = 1<<9,
    STATION_INFO_TX_RETRIES     = 1<<10,
    STATION_INFO_TX_FAILED      = 1<<11,
    STATION_INFO_RX_DROP_MISC   = 1<<12,
    STATION_INFO_SIGNAL_AVG     = 1<<13,
    STATION_INFO_RX_BITRATE     = 1<<14,
    STATION_INFO_BSS_PARAM          = 1<<15,
    STATION_INFO_CONNECTED_TIME = 1<<16,
    STATION_INFO_ASSOC_REQ_IES  = 1<<17,
    STATION_INFO_STA_FLAGS      = 1<<18,
    STATION_INFO_BEACON_LOSS_COUNT  = 1<<19,
    STATION_INFO_T_OFFSET       = 1<<20,
    STATION_INFO_LOCAL_PM       = 1<<21,
    STATION_INFO_PEER_PM        = 1<<22,
    STATION_INFO_NONPEER_PM     = 1<<23,
    STATION_INFO_RX_BYTES64     = 1<<24,
    STATION_INFO_TX_BYTES64     = 1<<25,
};

enum nl80211_auth_type {
    NL80211_AUTHTYPE_OPEN_SYSTEM,
    NL80211_AUTHTYPE_SHARED_KEY,
    NL80211_AUTHTYPE_FT,
    NL80211_AUTHTYPE_NETWORK_EAP,
    NL80211_AUTHTYPE_SAE,

    /* keep last */
    __NL80211_AUTHTYPE_NUM,
    NL80211_AUTHTYPE_MAX = __NL80211_AUTHTYPE_NUM - 1,
    NL80211_AUTHTYPE_AUTOMATIC
};

enum nl80211_hidden_ssid {
    NL80211_HIDDEN_SSID_NOT_IN_USE,
    NL80211_HIDDEN_SSID_ZERO_LEN,
    NL80211_HIDDEN_SSID_ZERO_CONTENTS
};

enum nl80211_mfp {
    NL80211_MFP_NO,
    NL80211_MFP_REQUIRED,
};

enum nl80211_acl_policy {
    NL80211_ACL_POLICY_ACCEPT_UNLESS_LISTED,
    NL80211_ACL_POLICY_DENY_UNLESS_LISTED,
};
struct ieee80211_hdr {
    uint16_t frame_control;
    uint16_t duration_id;
    uint8_t addr1[6];
    uint8_t addr2[6];
    uint8_t addr3[6];
    uint16_t seq_ctrl;
    /* followed by 'u8 addr4[6];' if ToDS and FromDS is set in data frame
     */
};

struct ieee80211_freq_range {
    uint32_t start_freq_khz;
    uint32_t end_freq_khz;
    uint32_t max_bandwidth_khz;
};

struct ieee80211_power_rule {
    uint32_t max_antenna_gain;
    uint32_t max_eirp;
};

struct ieee80211_reg_rule {
    struct ieee80211_freq_range freq_range;
    struct ieee80211_power_rule power_rule;
    uint32_t flags;
};

struct ieee80211_msrment_ie {
    uint8_t token;
    uint8_t mode;
    uint8_t type;
    uint8_t request[0];
};

struct ieee80211_ext_chansw_ie {
    uint8_t mode;
    uint8_t new_operating_class;
    uint8_t new_ch_num;
    uint8_t count;
};

struct ieee80211_mgmt {
    uint16_t frame_control;
    uint16_t duration;
    uint8_t da[6];
    uint8_t sa[6];
    uint8_t bssid[6];
    uint16_t seq_ctrl;
    union {
        struct {
            uint16_t auth_alg;
            uint16_t auth_transaction;
            uint16_t status_code;
            /* possibly followed by Challenge text */
            uint8_t variable[0];
        }  auth;
        struct {
            uint16_t reason_code;
        }  deauth;
        struct {
            uint16_t capab_info;
            uint16_t listen_interval;
            /* followed by SSID and Supported rates */
            uint8_t variable[0];
        }  assoc_req;
        struct {
            uint16_t capab_info;
            uint16_t status_code;
            uint16_t aid;
            /* followed by Supported rates */
            uint8_t variable[0];
        }  assoc_resp, reassoc_resp;
        struct {
            uint16_t capab_info;
            uint16_t listen_interval;
            uint8_t current_ap[6];
            /* followed by SSID and Supported rates */
            uint8_t variable[0];
        }  reassoc_req;
        struct {
            uint16_t reason_code;
        }  disassoc;
        struct {
            uint64_t timestamp;
            uint16_t beacon_int;
            uint16_t capab_info;
            /* followed by some of SSID, Supported rates,
             * FH Params, DS Params, CF Params, IBSS Params, TIM */
            uint8_t variable[0];
        }  beacon;
        struct {
            /* only variable items: SSID, Supported rates */
            uint8_t variable[0];
        }  probe_req;
        struct {
            uint64_t timestamp;
            uint16_t beacon_int;
            uint16_t capab_info;
            /* followed by some of SSID, Supported rates,
             * FH Params, DS Params, CF Params, IBSS Params */
            uint8_t variable[0];
        }  probe_resp;
        struct {
            uint8_t category;
            union {
                struct {
                    uint8_t action_code;
                    uint8_t dialog_token;
                    uint8_t status_code;
                    uint8_t variable[0];
                }  wme_action;
                struct {
                    uint8_t action_code;
                    uint8_t variable[0];
                }  chan_switch;
                struct {
                    uint8_t action_code;
                    struct ieee80211_ext_chansw_ie data;
                    uint8_t variable[0];
                }  ext_chan_switch;
                struct {
                    uint8_t action_code;
                    uint8_t dialog_token;
                    uint8_t element_id;
                    uint8_t length;
                    struct ieee80211_msrment_ie msr_elem;
                }  measurement;
                struct {
                    uint8_t action_code;
                    uint8_t dialog_token;
                    uint16_t capab;
                    uint16_t timeout;
                    uint16_t start_seq_num;
                }  addba_req;
                struct {
                    uint8_t action_code;
                    uint8_t dialog_token;
                    uint16_t status;
                    uint16_t capab;
                    uint16_t timeout;
                }  addba_resp;
                struct {
                    uint8_t action_code;
                    uint16_t params;
                    uint16_t reason_code;
                }  delba;
                struct {
                    uint8_t action_code;
                    uint8_t variable[0];
                }  self_prot;
                struct {
                    uint8_t action_code;
                    uint8_t variable[0];
                }  mesh_action;
                struct {
                    uint8_t action;
                    uint8_t trans_id[WLAN_SA_QUERY_TR_ID_LEN];
                }  sa_query;
                struct {
                    uint8_t action;
                    uint8_t smps_control;
                }  ht_smps;
                struct {
                    uint8_t action_code;
                    uint8_t chanwidth;
                }  ht_notify_cw;
                struct {
                    uint8_t action_code;
                    uint8_t dialog_token;
                    uint16_t capability;
                    uint8_t variable[0];
                }  tdls_discover_resp;
                struct {
                    uint8_t action_code;
                    uint8_t operating_mode;
                }  vht_opmode_notif;
            } u;
        }  action;
    } u;
};

struct cfg80211_scan_request {
    struct cfg80211_ssid *ssids;
    int32_t n_ssids;
    uint32_t n_channels;
    const uint8_t *ie;
    size_t ie_len;
    uint32_t flags;

    uint32_t rates[IEEE80211_NUM_BANDS];

    struct wireless_dev *wdev;

    /* internal */
    struct wiphy *wiphy;
    unsigned long scan_start;
    bool aborted;
    bool no_cck;

    /* keep last */
    struct ieee80211_channel *channels[0];
};

struct cfg80211_crypto_settings {
    uint32_t wpa_versions;
    uint32_t cipher_group;
    int32_t n_ciphers_pairwise;
    uint32_t ciphers_pairwise[NL80211_MAX_NR_CIPHER_SUITES];
    int32_t n_akm_suites;
    uint32_t akm_suites[NL80211_MAX_NR_AKM_SUITES];
    bool control_port;
    uint16_t control_port_ethertype;
    bool control_port_no_encrypt;
};

struct ieee80211_vht_mcs_info {
    uint16_t rx_mcs_map;
    uint16_t rx_highest;
    uint16_t tx_mcs_map;
    uint16_t tx_highest;
};

struct ieee80211_mcs_info {
    uint8_t   rx_mask[IEEE80211_HT_MCS_MASK_LEN];
    uint16_t  rx_highest;
    uint8_t   tx_params;
    uint8_t   reserved[3];
};

struct ieee80211_ht_cap {
    uint16_t cap_info;
    uint8_t ampdu_params_info;

    /* 16 bytes MCS information */
    struct ieee80211_mcs_info mcs;

    uint16_t extended_ht_cap_info;
    uint32_t tx_BF_cap_info;
    uint8_t antenna_selection_info;
};

struct ieee80211_vht_cap {
    uint32_t vht_cap_info;
    struct ieee80211_vht_mcs_info supp_mcs;
};

struct cfg80211_connect_params {
    struct ieee80211_channel *channel;
    uint8_t *bssid;
    uint8_t *ssid;
    size_t ssid_len;
    enum nl80211_auth_type auth_type;
    uint8_t *ie;
    size_t ie_len;
    bool privacy;
    enum nl80211_mfp mfp;
    struct cfg80211_crypto_settings crypto;
    const uint8_t *key;
    uint8_t key_len, key_idx;
    uint32_t flags;
    int32_t bg_scan_period;
    struct ieee80211_ht_cap ht_capa;
    struct ieee80211_ht_cap ht_capa_mask;
    struct ieee80211_vht_cap vht_capa;
    struct ieee80211_vht_cap vht_capa_mask;
};

struct cfg80211_beacon_data {
    const uint8_t *head, *tail;
    const uint8_t *beacon_ies;
    const uint8_t *proberesp_ies;
    const uint8_t *assocresp_ies;
    const uint8_t *probe_resp;

    size_t head_len, tail_len;
    size_t beacon_ies_len;
    size_t proberesp_ies_len;
    size_t assocresp_ies_len;
    size_t probe_resp_len;
};

struct oal_mac_address {
    uint8_t addr[ETH_ALEN];
};

struct cfg80211_acl_data {
    enum nl80211_acl_policy acl_policy;
    int32_t n_acl_entries;

    /* Keep it last */
    struct oal_mac_address mac_addrs[];
};

typedef struct ieee80211_channel {
    enum ieee80211_band band;
    uint16_t          center_freq;
    uint16_t          hw_value;
    uint32_t          flags;
    int32_t           max_antenna_gain;
    int32_t           max_power;
    uint32_t          orig_flags;
#ifndef _PRE_LITEOS_WLAN_FEATURE_MEM_REDUCE
    oal_bool_enum       beacon_found;
    int32_t           orig_mag;
    int32_t           orig_mpwr;
#endif
} oal_ieee80211_channel;

struct cfg80211_chan_def {
    oal_ieee80211_channel   *chan;
    oal_nl80211_channel_type width;
    int32_t                center_freq1;
    int32_t                center_freq2;
};

struct cfg80211_ap_settings {
    struct cfg80211_chan_def chandef;

    struct cfg80211_beacon_data beacon;

    int32_t beacon_interval, dtim_period;
    const uint8_t *ssid;
    size_t ssid_len;
    enum nl80211_hidden_ssid hidden_ssid;
    struct cfg80211_crypto_settings crypto;
    bool privacy;
    enum nl80211_auth_type auth_type;
    int32_t inactivity_timeout;
    uint8_t p2p_ctwindow;
    bool p2p_opp_ps;
    const struct cfg80211_acl_data *acl;
    bool radar_required;
};

struct cfg80211_ssid {
    uint8_t ssid[IEEE80211_MAX_SSID_LEN];
    uint8_t ssid_len;
};

struct cfg80211_match_set {
    struct cfg80211_ssid ssid;
    int32_t rssi_thold;
};

struct cfg80211_sched_scan_request {
    struct cfg80211_ssid *ssids;
    int32_t n_ssids;
    uint32_t n_channels;
    uint32_t interval;
    const uint8_t *ie;
    size_t ie_len;
    uint32_t flags;
    struct cfg80211_match_set *match_sets;
    int32_t n_match_sets;
    int32_t min_rssi_thold;
    int32_t rssi_thold; /* just for backward compatible */

    /* internal */
    struct wiphy *wiphy;
    struct oal_net_device_stru *dev;
    unsigned long scan_start;

    /* keep last */
    struct ieee80211_channel *channels[0];
};

typedef struct oal_cpu_usage_stat {
    uint64_t ull_user;
    uint64_t ull_nice;
    uint64_t ull_system;
    uint64_t ull_softirq;
    uint64_t ull_irq;
    uint64_t ull_idle;
    uint64_t ull_iowait;
    uint64_t ull_steal;
    uint64_t ull_guest;
} oal_cpu_usage_stat_stru;
/* 加密类型linux-2.6.34内核和linux-2.6.30内核不同 */
struct oal_ether_header {
    uint8_t    auc_ether_dhost[ETHER_ADDR_LEN];
    uint8_t    auc_ether_shost[ETHER_ADDR_LEN];
    uint16_t   us_ether_type;
} __OAL_DECLARE_PACKED;
typedef struct oal_ether_header oal_ether_header_stru;

#if (HW_LITEOS_OPEN_VERSION_NUM >= KERNEL_VERSION(1,4,2))
typedef struct ifreq oal_ifreq_stru;
#else
typedef struct {
    uint32_t   ul_fake;
    uint8_t   *ifr_data;
} oal_ifreq_stru;
#endif

typedef struct {
    uint32_t  ul_handle;
} oal_qdisc_stru;

/* iw_handler_def结构体封装 */
typedef struct {
    uint16_t       cmd;        /* Wireless Extension command */
    uint16_t       flags;      /* More to come ;-) */
} oal_iw_request_info_stru;

typedef struct {
    void       *pointer;    /* Pointer to the data  (in user space) */
    uint16_t  length;     /* number of fields or size in bytes */
    uint16_t  flags;      /* Optional params */
} oal_iw_point_stru;

typedef struct {
    int32_t      value;      /* The value of the parameter itself */
    uint8_t      fixed;      /* Hardware should not use auto select */
    uint8_t      disabled;   /* Disable the feature */
    uint16_t     flags;      /* Various specifc flags (if any) */
} oal_iw_param_stru;

typedef struct {
    int32_t        m;      /* Mantissa */
    int16_t        e;      /* Exponent */
    uint8_t        i;      /* List index (when in range struct) */
    uint8_t        flags;  /* Flags (fixed/auto) */
} oal_iw_freq_stru;

typedef struct {
    uint8_t        qual;       /* link quality (%retries, SNR, %missed beacons or better...) */
    uint8_t        level;      /* signal level (dBm) */
    uint8_t        noise;      /* noise level (dBm) */
    uint8_t        updated;    /* Flags to know if updated */
} oal_iw_quality_stru;

typedef struct {
    uint16_t      sa_family;      /* address family, AF_xxx   */
    int8_t        sa_data[14];    /* 14 bytes of protocol address */
} oal_sockaddr_stru;

typedef union {
    /* Config - generic */
    char                name[OAL_IF_NAME_SIZE];
    oal_iw_point_stru   essid;      /* Extended network name */
    oal_iw_param_stru   nwid;       /* network id (or domain - the cell) */
    oal_iw_freq_stru    freq;       /* frequency or channel : * 0-1000 = channel * > 1000 = frequency in Hz */
    oal_iw_param_stru   sens;       /* signal level threshold */
    oal_iw_param_stru   bitrate;    /* default bit rate */
    oal_iw_param_stru   txpower;    /* default transmit power */
    oal_iw_param_stru   rts;        /* RTS threshold threshold */
    oal_iw_param_stru   frag;       /* Fragmentation threshold */
    uint32_t          mode;       /* Operation mode */
    oal_iw_param_stru   retry;      /* Retry limits & lifetime */
    oal_iw_point_stru   encoding;   /* Encoding stuff : tokens */
    oal_iw_param_stru   power;      /* PM duration/timeout */
    oal_iw_quality_stru qual;       /* Quality part of statistics */
    oal_sockaddr_stru   ap_addr;    /* Access point address */
    oal_sockaddr_stru   addr;       /* Destination address (hw/mac) */
    oal_iw_param_stru   param;      /* Other small parameters */
    oal_iw_point_stru   data;       /* Other large parameters */
} oal_iwreq_data_union;

struct oal_net_device;
struct cfg80211_bitrate_mask;

typedef int32_t (*oal_iw_handler)(struct oal_net_device *dev, oal_iw_request_info_stru *info,
                                  oal_iwreq_data_union *wrqu, char *extra);
typedef uint8_t en_cfg80211_signal_type_uint8;

/* 此结构体成员命名是为了保持跟linux一致 */
typedef struct ieee80211_rate {
    uint32_t flags;
    uint16_t bitrate;
    uint16_t hw_value;
#ifndef _PRE_LITEOS_WLAN_FEATURE_MEM_REDUCE
    uint16_t hw_value_short;
    uint8_t  uc_rsv[2];
#endif
} oal_ieee80211_rate;

typedef struct ieee80211_sta_ht_cap {
    uint16_t          cap; /* use IEEE80211_HT_CAP_ */
    oal_bool_enum_uint8 ht_supported;
    uint8_t           ampdu_factor;
    uint8_t           ampdu_density;
    uint8_t           auc_rsv[3];
    struct ieee80211_mcs_info mcs;
} oal_ieee80211_sta_ht_cap;

typedef struct ieee80211_sta_vht_cap {
    bool vht_supported;
    uint32_t cap; /* use IEEE80211_VHT_CAP_ */
    struct ieee80211_vht_mcs_info vht_mcs;
} oal_ieee80211_sta_vht_cap;


typedef struct ieee80211_supported_band {
    oal_ieee80211_channel   *channels;
    oal_ieee80211_rate      *bitrates;
    enum ieee80211_band      band;
    int32_t                n_channels;
    int32_t                n_bitrates;
    oal_ieee80211_sta_ht_cap ht_cap;
    oal_ieee80211_sta_vht_cap vht_cap;
} oal_ieee80211_supported_band;
typedef struct oal_wiphy_tag {
    uint8_t   perm_addr[6];
    uint8_t   addr_mask[6];
    uint32_t  flags;
    en_cfg80211_signal_type_uint8        signal_type;
    uint8_t                            max_scan_ssids;
    uint16_t                           interface_modes;
    uint16_t                           max_scan_ie_len;
    uint8_t                            auc_rsv[2];
    int32_t                            n_cipher_suites;

    const uint32_t                *cipher_suites;
    uint32_t                           frag_threshold;
    uint32_t                           rts_threshold;
    oal_ieee80211_supported_band        *bands[IEEE80211_NUM_BANDS];
    uint8_t                            priv[4];

    const struct ieee80211_txrx_stypes *mgmt_stypes;
    const struct ieee80211_iface_combination *iface_combinations;
    int32_t n_iface_combinations;
    uint16_t max_remain_on_channel_duration;
    uint8_t max_sched_scan_ssids;
    uint8_t max_match_sets;
    uint16_t max_sched_scan_ie_len;
    void    *ctx;
} oal_wiphy_stru;
typedef struct {
    uint32_t  ul_fake;
} oal_iw_statistics_stru;
/* 私有IOCTL接口信息 */
typedef struct {
    uint32_t       cmd;                       /* ioctl命令号 */
    uint16_t       set_args;                  /* 类型和参数字符个数 */
    uint16_t       get_args;                  /* 类型和参数字符个数 */
    int8_t         name[OAL_IF_NAME_SIZE];    /* 私有命令名 */
} oal_iw_priv_args_stru;

typedef struct {
    const oal_iw_handler    *standard;
    uint16_t                   num_standard;
    uint16_t                   num_private;

    /* FangBaoshun For Hi1131 Compile */
    const oal_iw_handler    *private;
    /* FangBaoshun For Hi1131 Compile */
    uint8_t                    auc_resv[2];
    uint16_t                   num_private_args;

    const oal_iw_handler    *private_win32;

    const oal_iw_priv_args_stru *private_args;
    oal_iw_statistics_stru*    (*get_wireless_stats)(struct oal_net_device *dev);
} oal_iw_handler_def_stru;

typedef struct {
    uint32_t   rx_packets;     /* total packets received   */
    uint32_t   tx_packets;     /* total packets transmitted    */
    uint32_t   rx_bytes;       /* total bytes received     */
    uint32_t   tx_bytes;       /* total bytes transmitted  */
    uint32_t   rx_errors;      /* bad packets received     */
    uint32_t   tx_errors;      /* packet transmit problems */
    uint32_t   rx_dropped;     /* no space in linux buffers    */
    uint32_t   tx_dropped;     /* no space available in linux  */
    uint32_t   multicast;      /* multicast packets received   */
    uint32_t   collisions;

    /* detailed rx_errors: */
    uint32_t   rx_length_errors;
    uint32_t   rx_over_errors;     /* receiver ring buff overflow  */
    uint32_t   rx_crc_errors;      /* recved pkt with crc error    */
    uint32_t   rx_frame_errors;    /* recv'd frame alignment error */
    uint32_t   rx_fifo_errors;     /* recv'r fifo overrun      */
    uint32_t   rx_missed_errors;   /* receiver missed packet   */

    /* detailed tx_errors */
    uint32_t   tx_aborted_errors;
    uint32_t   tx_carrier_errors;
    uint32_t   tx_fifo_errors;
    uint32_t   tx_heartbeat_errors;
    uint32_t   tx_window_errors;

    /* for cslip etc */
    uint32_t   rx_compressed;
    uint32_t   tx_compressed;
} oal_net_device_stats_stru;

typedef struct work_struct                  oal_work_struct_stru;
typedef struct ieee80211_mgmt               oal_ieee80211_mgmt_stru;
typedef struct ieee80211_channel            oal_ieee80211_channel_stru;
typedef struct cfg80211_bss                 oal_cfg80211_bss_stru;
typedef struct rate_info                    oal_rate_info_stru;
typedef struct station_parameters           oal_station_parameters_stru;
typedef enum station_info_flags             oal_station_info_flags;

typedef struct nlattr                       oal_nlattr_stru;
typedef struct genl_multicast_group         oal_genl_multicast_group_stru;
typedef struct cfg80211_registered_device   oal_cfg80211_registered_device_stru;
typedef enum cfg80211_signal_type           oal_cfg80211_signal_type;
typedef enum nl80211_channel_type           oal_nl80211_channel_type;

typedef struct oal_genl_family {
    uint32_t      id;
    uint32_t      hdrsize;
    uint8_t       name[16];
    uint32_t      version;
    uint32_t      maxattr;
    oal_nlattr_stru  **attrbuf;    /* private */
    oal_list_head_stru  ops_list;   /* private */
    oal_list_head_stru  family_list;    /* private */
    oal_list_head_stru  mcast_groups;   /* private */
} oal_genl_family_stru;

enum wiphy_flags {
    WIPHY_FLAG_CUSTOM_REGULATORY        = BIT(0),
    WIPHY_FLAG_STRICT_REGULATORY        = BIT(1),
    WIPHY_FLAG_DISABLE_BEACON_HINTS     = BIT(2),
    WIPHY_FLAG_NETNS_OK         = BIT(3),
    WIPHY_FLAG_PS_ON_BY_DEFAULT     = BIT(4),
    WIPHY_FLAG_4ADDR_AP         = BIT(5),
    WIPHY_FLAG_4ADDR_STATION        = BIT(6),
    WIPHY_FLAG_CONTROL_PORT_PROTOCOL    = BIT(7),
    WIPHY_FLAG_IBSS_RSN         = BIT(8),
    WIPHY_FLAG_MESH_AUTH            = BIT(10),
    WIPHY_FLAG_SUPPORTS_SCHED_SCAN      = BIT(11),
    /* use hole at 12 */
    WIPHY_FLAG_SUPPORTS_FW_ROAM     = BIT(13),
    WIPHY_FLAG_AP_UAPSD         = BIT(14),
    WIPHY_FLAG_SUPPORTS_TDLS        = BIT(15),
    WIPHY_FLAG_TDLS_EXTERNAL_SETUP      = BIT(16),
    WIPHY_FLAG_HAVE_AP_SME          = BIT(17),
    WIPHY_FLAG_REPORTS_OBSS         = BIT(18),
    WIPHY_FLAG_AP_PROBE_RESP_OFFLOAD    = BIT(19),
    WIPHY_FLAG_OFFCHAN_TX           = BIT(20),
    WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL    = BIT(21),
};

typedef enum wiphy_params_flags             oal_wiphy_params_flags;
typedef enum wiphy_flags                    oal_wiphy_flags;

typedef struct sk_buff                      oal_netbuf_stru;
typedef struct sk_buff_head                 oal_netbuf_head_stru;
typedef ip4_addr_t                          oal_ip4_addr_t;
#define OAL_IP4_ADDR                        IP4_ADDR
typedef struct netif                        oal_lwip_netif;
typedef struct pbuf                         oal_lwip_buf;

typedef struct _oal_hisi_eapol_stru {
    oal_bool_enum    en_register;
    uint8_t        auc_reserve[3];
    void            *context;
    void           (*notify_callback)(void *, void *context);
    oal_netbuf_head_stru     st_eapol_skb_head;
} oal_hisi_eapol_stru;

typedef struct oal_net_device {
    int8_t                    name[OAL_IF_NAME_SIZE];
    void                   *ml_priv;
    struct oal_net_device_ops  *netdev_ops;
    uint32_t                    last_rx;
    uint32_t                    flags;
    oal_iw_handler_def_stru    *wireless_handlers;
    int8_t                    dev_addr[6];
    uint8_t                   auc_resv[2];
    int32_t                   tx_queue_len;
    int32_t                   ifindex;
    int16_t                   hard_header_len;
    int16_t                   type;
    int16_t                   needed_headroom;
    int16_t                   needed_tailroom;
    struct oal_net_device      *master;
    struct wireless_dev        *ieee80211_ptr;
    oal_qdisc_stru             *qdisc;
    uint8_t                  *ifalias;
    uint8_t                   addr_len;
    uint8_t                   auc_resv2[3];
    int32_t                     watchdog_timeo;
    oal_net_device_stats_stru   stats;
    uint32_t                  mtu;
    void                  (*destructor)(struct oal_net_device *);
    void                    *priv;
    uint32_t                  num_tx_queues;
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    oal_net_bridge_port        *br_port;
#endif

    oal_hisi_eapol_stru hisi_eapol;  /* EAPOL报文收发数据结构 */
    oal_lwip_netif *lwip_netif; /* LWIP协议栈数据结构 */
} oal_net_device_stru;

typedef struct oal_net_notify {
    uint32_t ul_ip_addr;
    uint32_t ul_notify_type;
} oal_net_notify_stru;

typedef struct cfg80211_chan_def oal_cfg80211_chan_def;
typedef struct wireless_dev {
    struct oal_net_device       *netdev;
    oal_wiphy_stru              *wiphy;
    enum nl80211_iftype          iftype;
    /* 1102新内核新增字段 add by lm */
    oal_cfg80211_chan_def        preset_chandef;
} oal_wireless_dev_stru;

typedef struct oal_net_device_ops {
    int32_t                   (*ndo_init)(oal_net_device_stru *);
    int32_t                   (*ndo_open)(struct oal_net_device*);
    int32_t                   (*ndo_stop)(struct oal_net_device*);
    oal_net_dev_tx_enum         (*ndo_start_xmit) (oal_netbuf_stru*, struct oal_net_device*);
    void                    (*ndo_set_multicast_list)(struct oal_net_device*);
    oal_net_device_stats_stru*  (*ndo_get_stats)(oal_net_device_stru *);
    int32_t                   (*ndo_do_ioctl)(struct oal_net_device*, oal_ifreq_stru *, int32_t);
    int32_t                   (*ndo_change_mtu)(struct oal_net_device*, int32_t);
    int32_t                   (*ndo_set_mac_address)(struct oal_net_device *, void *);
    uint16_t                  (*ndo_select_queue)(oal_net_device_stru *pst_dev, oal_netbuf_stru *);
    int32_t                   (*ndo_netif_notify)(oal_net_device_stru *, oal_net_notify_stru *);
} oal_net_device_ops_stru;

typedef struct rate_info {
    uint8_t    flags;
    uint8_t    mcs;
    uint16_t   legacy;
    uint8_t    nss;
    uint8_t      bw;
    uint8_t      rsv[3];
} oal_rate_info_stru;
typedef struct {
    union {
        uint8_t        u6_addr8[16];
        uint16_t       u6_addr16[8];
        uint32_t       u6_addr32[4];
    } in6_u;
#define s6_addr         in6_u.u6_addr8
#define s6_addr16       in6_u.u6_addr16
#define s6_addr32       in6_u.u6_addr32
} oal_in6_addr;

typedef struct {
    uint32_t      reserved : 5,
                    override : 1,
                    solicited : 1,
                    router : 1,
                    reserved2 : 24;
} icmpv6_nd_advt;
typedef struct {
    uint8_t           version : 4,
                        priority : 4;
    uint8_t          flow_lbl[3];
    uint16_t         payload_len;

    uint8_t           nexthdr;
    uint8_t           hop_limit;

    oal_in6_addr       saddr;
    oal_in6_addr       daddr;
} oal_ipv6hdr_stru;
typedef struct {
    uint8_t          icmp6_type;
    uint8_t          icmp6_code;
    uint16_t         icmp6_cksum;
    union {
        uint32_t          un_data32[1];
        uint16_t          un_data16[2];
        uint8_t           un_data8[4];
        icmpv6_nd_advt      u_nd_advt;
    } icmp6_dataun;

#define icmp6_solicited     icmp6_dataun.u_nd_advt.solicited
#define icmp6_override      icmp6_dataun.u_nd_advt.override
} oal_icmp6hdr_stru;

/* 多了4字节，记得减去4 */
typedef struct {
    oal_icmp6hdr_stru   icmph;
    oal_in6_addr        target;
    uint8_t           opt[1];
    uint8_t           rsv[3];
} oal_nd_msg_stru;

typedef struct {
    uint8_t       nd_opt_type;
    uint8_t       nd_opt_len;
} oal_nd_opt_hdr;
typedef struct  oal_cfg80211_crypto_settings_tag {
    uint32_t              wpa_versions;
    uint32_t              cipher_group;
    int32_t               n_ciphers_pairwise;
    uint32_t              ciphers_pairwise[OAL_NL80211_MAX_NR_CIPHER_SUITES];
    int32_t               n_akm_suites;
    uint32_t              akm_suites[OAL_NL80211_MAX_NR_AKM_SUITES];

    oal_bool_enum_uint8     control_port;
    uint8_t               auc_arry[3];
} oal_cfg80211_crypto_settings_stru;
typedef struct {
    int32_t         sk_wmem_queued;
} oal_sock_stru;
struct sta_bss_parameters {
    uint8_t  flags;
    uint8_t  dtim_period;
    uint16_t beacon_interval;
};

struct nl80211_sta_flag_update {
    uint32_t mask;
    uint32_t set;
};

enum nl80211_mesh_power_mode {
    NL80211_MESH_POWER_UNKNOWN,
    NL80211_MESH_POWER_ACTIVE,
    NL80211_MESH_POWER_LIGHT_SLEEP,
    NL80211_MESH_POWER_DEEP_SLEEP,

    __NL80211_MESH_POWER_AFTER_LAST,
    NL80211_MESH_POWER_MAX = __NL80211_MESH_POWER_AFTER_LAST - 1
};

typedef struct oal_station_info_tag {
    uint32_t filled;
    uint32_t connected_time;
    uint32_t inactive_time;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint16_t llid;
    uint16_t plid;
    uint8_t  plink_state;
    int8_t   signal;
    int8_t   signal_avg;
    oal_rate_info_stru txrate;
    oal_rate_info_stru rxrate;
    uint32_t rx_packets;
    uint32_t tx_packets;
    uint32_t tx_retries;
    uint32_t tx_failed;
    uint32_t rx_dropped_misc;
    struct sta_bss_parameters      bss_param;
    struct nl80211_sta_flag_update sta_flags;

    int32_t generation;

    const uint8_t *assoc_req_ies;
    uint32_t       assoc_req_ies_len;

    uint32_t beacon_loss_count;
    int64_t  t_offset;
    enum nl80211_mesh_power_mode local_pm;
    enum nl80211_mesh_power_mode peer_pm;
    enum nl80211_mesh_power_mode nonpeer_pm;
} oal_station_info_stru;
typedef struct oal_key_params_tag {
    uint8_t *key;
    uint8_t *seq;
    int32_t  key_len;
    int32_t  seq_len;
    uint32_t cipher;
} oal_key_params_stru;
/* VLAN以太网头 liteos封装 */
typedef struct {
    uint8_t       h_dest[6];
    uint8_t       h_source[6];
    uint16_t      h_vlan_proto;
    uint16_t      h_vlan_TCI;
    uint16_t      h_vlan_encapsulated_proto;
} oal_vlan_ethhdr_stru;
/* linux 结构体 */
typedef struct rtnl_link_ops                oal_rtnl_link_ops_stru;
typedef struct wireless_dev                 oal_wireless_dev_stru;
/* liwp 结构体 */
struct kobj_uevent_env {
    char *envp[UEVENT_NUM_ENVP];
    int envp_idx;
    char buf[UEVENT_BUFFER_SIZE];
    int buflen;
};

typedef struct kobj_uevent_env              oal_kobj_uevent_env_stru;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34)) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
typedef struct cfg80211_pmksa               oal_cfg80211_pmksa_stru;
#endif
typedef struct oal_cfg80211_ssid_tag {
    uint8_t   ssid[OAL_IEEE80211_MAX_SSID_LEN];
    uint8_t   ssid_len;
    uint8_t   auc_arry[3];
} oal_cfg80211_ssid_stru;

typedef struct cfg80211_sched_scan_request  oal_cfg80211_sched_scan_request_stru;
/* linux-2.6.34内核才有以下两个结构体，加密相关 */
typedef enum nl80211_mfp oal_nl80211_mfp_enum_uint8;

typedef struct oal_cfg80211_connect_params_tag {
    oal_ieee80211_channel_stru          *channel;

    uint8_t                           *bssid;
    uint8_t                           *ssid;
    uint8_t                           *ie;

    uint32_t                          ssid_len;
    uint32_t                          ie_len;
    oal_cfg80211_crypto_settings_stru   crypto;
    const uint8_t                     *key;

    oal_nl80211_auth_type_enum_uint8    auth_type;
    oal_bool_enum_uint8                 privacy;
    uint8_t                           key_len;
    uint8_t                           key_idx;
    oal_nl80211_mfp_enum_uint8          mfp;
    uint8_t                           auc_resv[3];
} oal_cfg80211_connect_params_stru;

struct vif_params {
    uint8_t    *mesh_id;
    int32_t     mesh_id_len;
    int32_t     use_4addr;
    uint8_t    *macaddr;
};

typedef struct vif_params                   oal_vif_params_stru;

typedef struct oal_cfg80211_scan_request_tag {
    oal_cfg80211_ssid_stru *ssids;
    int32_t               n_ssids;
    uint32_t              n_channels;
    uint32_t              ie_len;

    /* internal */
    oal_wiphy_stru         *wiphy;
    oal_net_device_stru    *dev;

    struct wireless_dev *wdev;

    oal_bool_enum_uint8     aborted;
    uint8_t               auc_arry[3];

    const uint8_t        *ie;

    /* keep last */
    oal_ieee80211_channel_stru  *channels[OAL_MAX_SCAN_CHANNELS];
} oal_cfg80211_scan_request_stru;

enum nl80211_key_type {
    NL80211_KEYTYPE_GROUP,
    NL80211_KEYTYPE_PAIRWISE,
    NL80211_KEYTYPE_PEERKEY,
    NUM_NL80211_KEYTYPES
};

typedef enum nl80211_key_type oal_nl80211_key_type;

typedef struct ieee80211_regdomain          oal_ieee80211_regdomain_stru;
typedef struct cfg80211_update_ft_ies_params    oal_cfg80211_update_ft_ies_stru;
typedef struct cfg80211_ft_event_params         oal_cfg80211_ft_event_stru;

/* To be implement! */
typedef struct cfg80211_beacon_data         oal_beacon_data_stru;
typedef struct cfg80211_ap_settings         oal_ap_settings_stru;
typedef struct bss_parameters               oal_bss_parameters;

struct beacon_parameters {
    uint8_t *head, *tail;
    int32_t interval, dtim_period;
    int32_t head_len, tail_len;
};

typedef struct beacon_parameters            oal_beacon_parameters;

typedef struct ieee80211_channel_sw_ie      oal_ieee80211_channel_sw_ie;
typedef struct ieee80211_msrment_ie         oal_ieee80211_msrment_ie;
typedef struct ieee80211_mgmt               oal_ieee80211_mgmt;


typedef struct {
    oal_proc_dir_entry_stru   *proc_net;
} oal_net_stru;

typedef struct module            oal_module_stru;
typedef struct {
    uint32_t      nlmsg_len;      /* 消息长度，包括首部在内 */
    uint16_t      nlmsg_type;     /* 消息内容的类型 */
    uint16_t      nlmsg_flags;    /* 附加的标志 */
    uint32_t      nlmsg_seq;      /* 序列号 */
    uint32_t      nlmsg_pid;      /* 发送进程的端口ID */
} oal_nlmsghdr_stru;

typedef struct ethhdr            oal_ethhdr;
typedef struct nf_hook_ops       oal_nf_hook_ops_stru;
typedef struct net_bridge_port   oal_net_bridge_port;


#define OAL_LL_ALLOCATED_SPACE  LL_ALLOCATED_SPACE

/* netlink相关 */
#define OAL_NLMSG_ALIGNTO               4
#define OAL_NLMSG_ALIGN(_len)           (((_len)+OAL_NLMSG_ALIGNTO - 1) & ~(OAL_NLMSG_ALIGNTO - 1))
#define OAL_NLMSG_HDRLEN                ((int32_t) OAL_NLMSG_ALIGN(sizeof(oal_nlmsghdr_stru)))
#define OAL_NLMSG_LENGTH(_len)          ((_len)+OAL_NLMSG_ALIGN(OAL_NLMSG_HDRLEN))
#define OAL_NLMSG_SPACE(_len)           OAL_NLMSG_ALIGN(OAL_NLMSG_LENGTH(_len))
#define OAL_NLMSG_DATA(_nlh)            ((void*)(((char*)(_nlh)) + OAL_NLMSG_LENGTH(0)))
#define OAL_NLMSG_PAYLOAD(_nlh, _len)   ((_nlh)->nlmsg_len - OAL_NLMSG_SPACE((_len)))
#define OAL_NLMSG_OK(_nlh, _len)        ((_len) >= (int)(sizeof(oal_nlmsghdr_stru)) && \
                                         (_nlh)->nlmsg_len >= (sizeof(oal_nlmsghdr_stru)) && \
                                         (_nlh)->nlmsg_len <= (_len))

#define OAL_AF_BRIDGE   AF_BRIDGE   /* Multiprotocol bridge     */
#define OAL_PF_BRIDGE   OAL_AF_BRIDGE

/* Bridge Hooks */
/* After promisc drops, checksum checks. */
#define OAL_NF_BR_PRE_ROUTING   NF_BR_PRE_ROUTING
/* If the packet is destined for this box. */
#define OAL_NF_BR_LOCAL_IN      NF_BR_LOCAL_IN
/* If the packet is destined for another interface. */
#define OAL_NF_BR_FORWARD       NF_BR_FORWARD
/* Packets coming from a local process. */
#define OAL_NF_BR_LOCAL_OUT     NF_BR_LOCAL_OUT
/* Packets about to hit the wire. */
#define OAL_NF_BR_POST_ROUTING  NF_BR_POST_ROUTING
/* Not really a hook, but used for the ebtables broute table */
#define OAL_NF_BR_BROUTING      NF_BR_BROUTING
#define OAL_NF_BR_NUMHOOKS      NF_BR_NUMHOOKS

/* Responses from hook functions. */
#define OAL_NF_DROP             NF_DROP
#define OAL_NF_ACCEPT           NF_ACCEPT
#define OAL_NF_STOLEN           NF_STOLEN
#define OAL_NF_QUEUE            NF_QUEUE
#define OAL_NF_REPEAT           NF_REPEAT
#define OAL_NF_STOP             NF_STOP
#define OAL_NF_MAX_VERDICT      NF_STOP

typedef struct {
    uint16_t us_ar_hrd;   /* format of hardware address */
    uint16_t us_ar_pro;   /* format of protocol address */

    uint8_t  uc_ar_hln;   /* length of hardware address */
    uint8_t  uc_ar_pln;   /* length of protocol address */
    uint16_t us_ar_op;    /* ARP opcode (command) */

    uint8_t  auc_ar_sha[ETHER_ADDR_LEN];           /* sender hardware address */
    uint8_t  auc_ar_sip[ETH_SENDER_IP_ADDR_LEN];   /* sender IP address */
    uint8_t  auc_ar_tha[ETHER_ADDR_LEN];           /* target hardware address */
    uint8_t  auc_ar_tip[ETH_TARGET_IP_ADDR_LEN];   /* target IP address */
} oal_eth_arphdr_stru;


struct ieee80211_txrx_stypes {
    uint16_t tx, rx;
};

struct ieee80211_iface_limit {
    uint16_t max;
    uint16_t types;
};

struct ieee80211_iface_combination {
    const struct ieee80211_iface_limit *limits;
    uint32_t num_different_channels;
    uint16_t max_interfaces;
    uint8_t n_limits;
    bool beacon_int_infra_match;
    uint8_t radar_detect_widths;
};

typedef struct ieee80211_iface_limit        oal_ieee80211_iface_limit;
typedef struct ieee80211_iface_combination  oal_ieee80211_iface_combination;

typedef struct oal_cfg80211_ops_tag {
    int32_t (*add_key)(oal_wiphy_stru *wiphy, oal_net_device_stru *netdev,
                         uint8_t key_index,
                         bool            en_pairwise,
                         const uint8_t *mac_addr, oal_key_params_stru *params);
    int32_t (*get_key)(oal_wiphy_stru *wiphy, oal_net_device_stru *netdev,
                         uint8_t key_index,
                         bool            en_pairwise,
                         const uint8_t *mac_addr, void *cookie,
                         void (*callback)(void *cookie, oal_key_params_stru *key));
    int32_t (*del_key)(oal_wiphy_stru *wiphy, oal_net_device_stru *netdev,
                         uint8_t key_index,
                         bool            en_pairwise,
                         const uint8_t *mac_addr);
    int32_t (*set_default_key)(oal_wiphy_stru *wiphy, oal_net_device_stru *netdev, uint8_t key_index,
                                 bool            en_unicast,
                                 bool            en_multicast);
    int32_t (*set_default_mgmt_key)(oal_wiphy_stru *wiphy, oal_net_device_stru *netdev, uint8_t key_index);
    int32_t (*scan)(oal_wiphy_stru *pst_wiphy,  oal_cfg80211_scan_request_stru *pst_request);
    int32_t (*connect)(oal_wiphy_stru *pst_wiphy, oal_net_device_stru *pst_dev,
                         oal_cfg80211_connect_params_stru *pst_sme);
    int32_t (*disconnect)(oal_wiphy_stru *pst_wiphy, oal_net_device_stru *pst_dev, uint16_t us_reason_code);
    int32_t (*set_channel)(oal_wiphy_stru *pst_wiphy, oal_ieee80211_channel *pst_chan,
                             oal_nl80211_channel_type en_channel_type);
    int32_t (*set_wiphy_params)(oal_wiphy_stru *pst_wiphy, uint32_t ul_changed);
    int32_t (*add_beacon)(oal_wiphy_stru *pst_wiphy, oal_net_device_stru *pst_dev, oal_beacon_parameters *pst_info);
    int32_t (*change_virtual_intf)(oal_wiphy_stru *pst_wiphy, oal_net_device_stru *pst_dev,
                                     enum nl80211_iftype en_type, uint32_t *pul_flags,
                                     oal_vif_params_stru *pst_params);
    int32_t (*add_station)(oal_wiphy_stru *pst_wiphy, oal_net_device_stru *pst_dev, uint8_t *puc_mac,
                             oal_station_parameters_stru *pst_sta_parms);
    int32_t (*del_station)(oal_wiphy_stru *pst_wiphy, oal_net_device_stru *pst_dev, const uint8_t *puc_mac);
    int32_t (*change_station)(oal_wiphy_stru *pst_wiphy, oal_net_device_stru *pst_dev,
                                uint8_t *puc_mac, oal_station_parameters_stru *pst_sta_parms);
    int32_t (*get_station)(oal_wiphy_stru *pst_wiphy, oal_net_device_stru *pst_dev,
                             uint8_t *puc_mac, oal_station_info_stru *pst_sta_info);
    int32_t (*dump_station)(oal_wiphy_stru *wiphy, oal_net_device_stru *dev, int32_t idx,
                              uint8_t *mac, oal_station_info_stru *pst_sta_info);
    /* lm add new code begin */
    int32_t (*change_beacon)(oal_wiphy_stru  *pst_wiphy, oal_net_device_stru *pst_netdev,
                               oal_beacon_data_stru *pst_beacon_info);
    int32_t (*start_ap)(oal_wiphy_stru *pst_wiphy, oal_net_device_stru *pst_netdev,
                          oal_ap_settings_stru *pst_ap_settings);
    int32_t (*stop_ap)(oal_wiphy_stru *pst_wiphy, oal_net_device_stru *pst_netdev);
    int32_t (*change_bss)(oal_wiphy_stru *pst_wiphy, oal_net_device_stru *pst_netdev,
                            oal_bss_parameters *pst_bss_params);
    int32_t (*set_power_mgmt)(oal_wiphy_stru  *pst_wiphy, oal_net_device_stru *pst_ndev,
                                bool  enabled, int32_t ul_timeout);
    /* lm add new code end */
    int32_t   (*sched_scan_start)(oal_wiphy_stru *wiphy,
                                    oal_net_device_stru *dev,
                                    struct cfg80211_sched_scan_request *request);
    int32_t   (*sched_scan_stop)(oal_wiphy_stru *wiphy,  oal_net_device_stru *dev);
    int32_t   (*remain_on_channel)(oal_wiphy_stru *wiphy,
                                     struct wireless_dev *wdev,
                                     struct ieee80211_channel *chan,
                                     uint32_t duration,
                                     uint64_t *cookie);
    int32_t   (*cancel_remain_on_channel)(oal_wiphy_stru *wiphy,
                                            struct wireless_dev *wdev,
                                            uint64_t cookie);

    int32_t   (*mgmt_tx)(oal_wiphy_stru *wiphy, struct wireless_dev *wdev,
                           struct ieee80211_channel *chan, bool offchan,
                           uint32_t wait, const uint8_t *buf, size_t len,
                           bool no_cck, bool dont_wait_for_ack, uint64_t *cookie);
    void    (*mgmt_frame_register)(struct wiphy *wiphy,
                                   struct wireless_dev *wdev,
                                   uint16_t frame_type, bool reg);
    int32_t   (*set_bitrate_mask)(struct wiphy *wiphy,
                                    oal_net_device_stru *dev,
                                    const uint8_t *peer,
                                    const struct cfg80211_bitrate_mask *mask);
    struct wireless_dev *(*add_virtual_intf)(oal_wiphy_stru *wiphy,
                                              const char *name,
                                              enum nl80211_iftype type,
                                              uint32_t *flags,
                                              struct vif_params *params);
    int32_t   (*del_virtual_intf)(oal_wiphy_stru *wiphy,
                                    struct  wireless_dev *wdev);
    int32_t   (*mgmt_tx_cancel_wait)(oal_wiphy_stru *wiphy,
                                       struct wireless_dev *wdev,
                                       uint64_t cookie);
    int32_t   (*start_p2p_device)(oal_wiphy_stru *wiphy,
                                    struct wireless_dev *wdev);
    void    (*stop_p2p_device)(oal_wiphy_stru *wiphy,
                               struct wireless_dev *wdev);
} oal_cfg80211_ops_stru;
/*****************************************************************************
  8 UNION定义
*****************************************************************************/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0))
/* WiFi 驱动适配linux4.9 */
/* Linux 4.7 删除enum ieee80211_band，用enum nl80211_band 替换，
   WiFi 驱动新增enum ieee80211_band 定义 */
enum ieee80211_band {
    IEEE80211_BAND_2GHZ = NL80211_BAND_2GHZ,
    IEEE80211_BAND_5GHZ = NL80211_BAND_5GHZ,
    IEEE80211_BAND_60GHZ = NL80211_BAND_60GHZ,

    /* keep last */
    IEEE80211_NUM_BANDS
};
#define HISI_IEEE80211_BAND_2GHZ    NL80211_BAND_2GHZ
#define HISI_IEEE80211_BAND_5GHZ    NL80211_BAND_5GHZ
#else
#define HISI_IEEE80211_BAND_2GHZ    IEEE80211_BAND_2GHZ
#define HISI_IEEE80211_BAND_5GHZ    IEEE80211_BAND_5GHZ
#endif  /* (LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0)) */
extern oal_sock_stru g_st_sock;
extern oal_net_device_stru *g_past_net_device[];
extern oal_wiphy_stru* g_pst_wiphy;
extern bool g_init_network;

extern int32_t hwal_netif_rx(oal_net_device_stru *pst_net_dev, oal_netbuf_stru *pst_netbuf);
extern int32_t hwal_lwip_init(oal_net_device_stru *pst_net_dev);
extern void hwal_lwip_deinit(oal_net_device_stru *pst_net_dev);
extern void  netif_set_flow_ctrl_status(oal_lwip_netif *pst_netif, netif_flow_ctrl_enum_uint8 en_status);

typedef enum nl80211_external_auth_action oal_nl80211_external_auth_action;
typedef enum nl80211_chan_width oal_nl80211_chan_width;
typedef struct ethtool_ops oal_ethtool_ops_stru;

#if (HW_LITEOS_OPEN_VERSION_NUM < KERNEL_VERSION(3,1,3))
OAL_STATIC OAL_INLINE int32_t mutex_destroy(pthread_mutex_t *mutex)
{
    return pthread_mutex_destroy(mutex);
}
#endif
OAL_STATIC OAL_INLINE void oal_netbuf_copy_queue_mapping(oal_netbuf_stru  *to, const oal_netbuf_stru *from)
{
    skb_copy_queue_mapping(to, from);
}


OAL_STATIC OAL_INLINE uint8_t* oal_netbuf_put(oal_netbuf_stru *pst_netbuf, uint32_t ul_len)
{
    return skb_put(pst_netbuf, ul_len);
}


OAL_STATIC OAL_INLINE uint8_t  *oal_netbuf_push(oal_netbuf_stru *pst_netbuf, uint32_t ul_len)
{
    return skb_push(pst_netbuf, ul_len);
}


OAL_STATIC OAL_INLINE uint8_t* oal_netbuf_pull(oal_netbuf_stru *pst_netbuf, uint32_t ul_len)
{
    if (ul_len > pst_netbuf->len) {
        return NULL;
    }

    pst_netbuf->len -= ul_len;

    return (pst_netbuf->data += ul_len);
}

OAL_STATIC OAL_INLINE int32_t oal_ieee80211_channel_to_frequency(int32_t l_channel, enum ieee80211_band band)
{
    /* see 802.11 17.3.8.3.2 and Annex J
        * there are overlapping channel numbers in 5GHz and 2GHz bands */
    if (l_channel <= 0) {
        return 0; /* not supported */
    }

    switch (band) {
        case IEEE80211_BAND_2GHZ:
        {
            if (l_channel == 14) {
                return 2484;
            } else if (l_channel < 14) {
                return 2407 + l_channel * 5;
            }
            break;
        }

        case IEEE80211_BAND_5GHZ: // 此case 完成后直接返回
        {
            if (l_channel >= 182 && l_channel <= 196) {
                return 4000 + l_channel * 5;
            } else {
                return 5000 + l_channel * 5;
            }
        }
        default:
            /* not supported other BAND */
            return 0;
    }

    /* not supported */
    return 0;
}

OAL_STATIC OAL_INLINE int32_t  oal_ieee80211_frequency_to_channel(int32_t l_center_freq)
{
    int32_t l_channel;

    /* see 802.11 17.3.8.3.2 and Annex J */
    if (l_center_freq == 2484) {
        l_channel = 14;
    } else if (l_center_freq < 2484) {
        l_channel = (l_center_freq - 2407) / 5;
    } else if (l_center_freq >= 4910 && l_center_freq <= 4980) {
        l_channel = (l_center_freq - 4000) / 5;
    } else if (l_center_freq <= 45000) { /* DMG band lower limit */
        l_channel = (l_center_freq - 5000) / 5;
    } else if (l_center_freq >= 58320 && l_center_freq <= 64800) {
        l_channel = (l_center_freq - 56160) / 2160;
    } else {
        l_channel = 0;
    }
    return l_channel;
}


OAL_STATIC OAL_INLINE uint8_t oal_netbuf_get_bitfield(void)
{
    union bitfield {
        uint8_t uc_byte;
        struct  {
            uint8_t high : 4,
                      low  : 4;
        } bits;
    } un_bitfield;

    un_bitfield.uc_byte = 0x12;
    if (un_bitfield.bits.low == 0x2) {
        return OAL_BITFIELD_LITTLE_ENDIAN;
    } else {
        return OAL_BITFIELD_BIG_ENDIAN;
    }
}


OAL_STATIC OAL_INLINE void oal_set_netbuf_prev(oal_netbuf_stru *pst_buf, oal_netbuf_stru *pst_prev)
{
    pst_buf->prev = pst_prev;
}

OAL_STATIC OAL_INLINE oal_netbuf_stru *oal_get_netbuf_prev(oal_netbuf_stru *pst_buf)
{
    return pst_buf->prev;
}

/* arm64 尾指针变成了偏移 */
#ifndef CONFIG_ARM64
/* tail指针操作请使用skb_put，避免使用该函数 */
OAL_STATIC OAL_INLINE void oal_set_netbuf_tail(oal_netbuf_stru *pst_buf,  uint8_t  *tail)
{
    pst_buf->tail = (sk_buff_data_t)tail;
}
#endif

OAL_STATIC OAL_INLINE void oal_set_netbuf_next(oal_netbuf_stru *pst_buf,  oal_netbuf_stru  *next)
{
    if (pst_buf == NULL) {
        return;
    } else {
        pst_buf->next = next;
    }
}

OAL_STATIC OAL_INLINE oal_netbuf_stru *oal_get_netbuf_next(oal_netbuf_stru *pst_buf)
{
    return pst_buf->next;
}

/* arm64 尾指针变成了偏移 */
#ifndef CONFIG_ARM64
/* tail指针操作请使用skb_put，避免使用该函数 */
OAL_STATIC OAL_INLINE void oal_set_single_netbuf_tail(oal_netbuf_stru *pst_netbuf, uint8_t *puc_tail)
{
    pst_netbuf->tail = (sk_buff_data_t)puc_tail;
}
#endif


OAL_STATIC OAL_INLINE void  oal_get_cpu_stat(oal_cpu_usage_stat_stru *pst_cpu_stat)
{
    if (pst_cpu_stat == NULL) {
        return;
    }
#if (defined(_PRE_BOARD_SD5610) || defined(_PRE_BOARD_SD5115))
    if (memcpy_s(pst_cpu_stat, sizeof(oal_cpu_usage_stat_stru),
                 &kstat_cpu(0).cpustat, sizeof(oal_cpu_usage_stat_stru)) != EOK) {
        return;
    }
#endif
}

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
#define oal_br_should_route_hook     br_should_route_hook


OAL_STATIC OAL_INLINE int32_t  oal_nf_register_hooks(oal_nf_hook_ops_stru *pst_nf_hook_ops, uint32_t ul_num)
{
    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE void  oal_nf_unregister_hooks(oal_nf_hook_ops_stru *pst_nf_hook_ops, uint32_t ul_num)
{
    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE uint32_t oal_br_get_port_type(oal_net_bridge_port *pst_port)
{
#if (defined(_PRE_BOARD_SD5610) || defined(_PRE_BOARD_SD5115))
    return (pst_port->type);
#else
    return ~0;
#endif
}


OAL_STATIC OAL_INLINE  oal_ethhdr *oal_eth_hdr(const oal_netbuf_stru *pst_netbuf)
{
    return (oal_ethhdr *)pst_netbuf->mac_header;
}

#define OAL_RCU_ASSIGN_POINTER(_p, _v)


#endif



OAL_STATIC OAL_INLINE oal_ieee80211_channel_stru *oal_ieee80211_get_channel(oal_wiphy_stru *pst_wiphy,
                                                                            int32_t ul_freq)
{
    enum ieee80211_band band;
    struct ieee80211_supported_band *sband = NULL;
    int i;

    for (band = 0; band < IEEE80211_NUM_BANDS; band++) {
        sband = pst_wiphy->bands[band];

        if (sband == NULL)
            continue;

        for (i = 0; i < sband->n_channels; i++) {
            if (sband->channels[i].center_freq == ul_freq)
                return &sband->channels[i];
        }
    }

    return NULL;
}

/* BEGIN : Linux wiphy 结构相关的处理函数 */

OAL_STATIC OAL_INLINE oal_wiphy_stru *oal_wiphy_new(oal_cfg80211_ops_stru *ops, uint32_t sizeof_priv)
{
    return (oal_wiphy_stru *)oal_memalloc(sizeof_priv + sizeof(oal_wiphy_stru));
}


OAL_STATIC OAL_INLINE int32_t oal_wiphy_register(oal_wiphy_stru *pst_wiphy)
{
    if (pst_wiphy == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    } else {
        g_pst_wiphy = pst_wiphy;
        return OAL_SUCC;
    }
}

OAL_STATIC OAL_INLINE oal_wiphy_stru *oal_wiphy_get(void)
{
    return g_pst_wiphy;
}

OAL_STATIC OAL_INLINE void  oal_wiphy_unregister(oal_wiphy_stru *pst_wiphy)
{
    return;
}


OAL_STATIC OAL_INLINE void oal_wiphy_free(oal_wiphy_stru *pst_wiphy)
{
    oal_free(pst_wiphy);
}

OAL_STATIC OAL_INLINE void *oal_wiphy_priv(oal_wiphy_stru *pst_wiphy)
{
    return pst_wiphy->priv;
}

OAL_STATIC OAL_INLINE void oal_wiphy_apply_custom_regulatory(oal_wiphy_stru *pst_wiphy,
    const oal_ieee80211_regdomain_stru *regd)
{
    return;
}

/* END : Linux wiphy 结构相关的处理函数 */
/* 添加wiphy结构体rts门限赋值 */
OAL_STATIC OAL_INLINE void oal_wiphy_set_rts(oal_wiphy_stru *pst_wiphy, uint32_t ul_rts_threshold)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34)) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    pst_wiphy->rts_threshold  =  ul_rts_threshold;
#endif
    return;
}

/* 添加wiphy结构体分片门限赋值 */
OAL_STATIC OAL_INLINE void oal_wiphy_set_frag(oal_wiphy_stru *pst_wiphy, uint32_t ul_frag_threshold)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34)) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    pst_wiphy->frag_threshold  =  ul_frag_threshold;
#endif
    return;
}


OAL_STATIC OAL_INLINE uint16_t  oal_eth_type_trans(oal_netbuf_stru *pst_netbuf, oal_net_device_stru *pst_device)
{
    /* 将netbuf的data指针指向mac frame的payload处 --fix by fangbaoshun for liteos */
    oal_netbuf_pull(pst_netbuf, sizeof(oal_ether_header_stru));
    return 0;
}


OAL_STATIC OAL_INLINE void oal_ether_setup(oal_net_device_stru *p_net_device)
{
    return;
}


OAL_STATIC OAL_INLINE oal_net_device_stru* oal_dev_get_by_name(const char *pc_name)
{
    uint32_t   ul_netdev_index;

    for (ul_netdev_index = 0; ul_netdev_index < WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; ul_netdev_index++) {
        if ((g_past_net_device[ul_netdev_index] != NULL) &&
            (!(oal_strcmp((const char *)(g_past_net_device[ul_netdev_index]->name), pc_name)))) {
            return g_past_net_device[ul_netdev_index];
        }
    }

    return NULL;
}



#define oal_dev_put(_pst_dev)


OAL_STATIC OAL_INLINE void  oal_net_close_dev(oal_net_device_stru *pst_netdev)
{
    return;
}


OAL_STATIC OAL_INLINE oal_net_device_stru *oal_net_alloc_netdev(uint32_t ul_sizeof_priv, int8_t *puc_name,
                                                                void *p_set_up)
{
    oal_net_device_stru *pst_net_dev;
    uint32_t           ul_size;
    errno_t l_ret;

    ul_size = OAL_STRLEN(puc_name) + 1; /* 包括'\0' */

    pst_net_dev = (oal_net_device_stru *)oal_memalloc(sizeof(oal_net_device_stru));
    if (pst_net_dev == NULL) {
        return NULL;
    }

    memset_s(pst_net_dev, sizeof(oal_net_device_stru), 0, sizeof(oal_net_device_stru));

    /* 将name保存到netdeivce */
    l_ret = memcpy_s(pst_net_dev->name, OAL_IF_NAME_SIZE, puc_name, ul_size);
    if (l_ret != EOK) {
        oal_io_print("oal_net_alloc_netdev: memcpy_s failed!\n");
        return NULL;
    }

    return pst_net_dev;
}


OAL_STATIC OAL_INLINE oal_net_device_stru *oal_net_alloc_netdev_mqs(uint32_t ul_sizeof_priv, const int8_t *puc_name,
                                                                    void *p_set_up, uint32_t ul_txqs,
                                                                    uint32_t ul_rxqs)
{
    oal_net_device_stru *pst_net_dev;
    uint32_t           ul_size;
    errno_t l_ret;

    ul_size = OAL_STRLEN(puc_name) + 1; /* 包括'\0' */

    pst_net_dev = (oal_net_device_stru *)oal_memalloc(sizeof(oal_net_device_stru));
    if (pst_net_dev == NULL) {
        return NULL;
    }

    memset_s(pst_net_dev, sizeof(oal_net_device_stru), 0, sizeof(oal_net_device_stru));

    /* 将name保存到netdeivce */
    l_ret = memcpy_s(pst_net_dev->name, OAL_IF_NAME_SIZE, puc_name, ul_size);
    if (l_ret != EOK) {
        oal_io_print("oal_net_alloc_netdev_mqs: memcpy_s failed!\n");
        return NULL;
    }

    return pst_net_dev;
}


OAL_STATIC OAL_INLINE void oal_net_tx_wake_all_queues(oal_net_device_stru *pst_dev)
{
    return;
}



OAL_STATIC OAL_INLINE void oal_net_tx_stop_all_queues(oal_net_device_stru *pst_dev)
{
    return;
}

extern netif_flow_ctrl_enum            g_en_netif_flow_ctrl;

OAL_STATIC OAL_INLINE void oal_net_wake_subqueue(oal_net_device_stru *pst_dev, uint16_t us_queue_idx)
{
    if (pst_dev == NULL) {
        printf("oal_net_wake_subqueue::pst_dev = NULL!!! \r\n");
        return;
    }

    netif_set_flow_ctrl_status(pst_dev->lwip_netif, NETIF_FLOW_CTRL_OFF);
    return;
}



OAL_STATIC OAL_INLINE void oal_net_stop_subqueue(oal_net_device_stru *pst_dev, uint16_t us_queue_idx)
{
    if (pst_dev == NULL) {
        printf("oal_net_stop_subqueue::pst_dev = NULL!!! \r\n");
        return;
    }

    netif_set_flow_ctrl_status(pst_dev->lwip_netif, NETIF_FLOW_CTRL_ON);
    return;
}


OAL_STATIC OAL_INLINE void oal_net_free_netdev(oal_net_device_stru *pst_netdev)
{
    if (pst_netdev == NULL) {
        return;
    }

    if (pst_netdev->priv != NULL) {
        oal_free((void *)pst_netdev->priv);
    }

    oal_free((void *)pst_netdev);
}


OAL_INLINE int32_t  oal_net_register_netdev(oal_net_device_stru *pst_net_dev)
{
    int32_t               ul_netdev_index;
    oal_bool_enum_uint8     en_dev_register = OAL_FALSE;

    if (pst_net_dev == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 注册驱动数据结构 */
    for (ul_netdev_index = 0; ul_netdev_index < WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; ul_netdev_index++) {
        if (g_past_net_device[ul_netdev_index] == NULL) {
            g_past_net_device[ul_netdev_index] = pst_net_dev;

            en_dev_register = OAL_TRUE;
            break;
        }
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* HCC层用 */
    OAL_NETDEVICE_HEADROOM(pst_net_dev) = 64;
    OAL_NETDEVICE_TAILROOM(pst_net_dev) = 32;
#endif

    if (en_dev_register != OAL_TRUE) {
        return OAL_FAIL;
    }

    /* 注册LWIP协议栈 */
    if (hwal_lwip_init(pst_net_dev) != OAL_SUCC) {
        g_past_net_device[ul_netdev_index] = NULL;

        return OAL_FAIL;
    }

    return OAL_SUCC;
}



OAL_STATIC OAL_INLINE void oal_net_unregister_netdev(oal_net_device_stru *pst_net_dev)
{
    uint32_t    ul_netdev_index;

    if (pst_net_dev == NULL) {
        return;
    }

    for (ul_netdev_index = 0; ul_netdev_index < WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; ul_netdev_index++) {
        if (g_past_net_device[ul_netdev_index] == pst_net_dev) {
            g_past_net_device[ul_netdev_index] = NULL;

            /* 先解注册LWIP协议栈 */
            hwal_lwip_deinit(pst_net_dev);

            /* linux下操作系统会释放netdev，liteos下需自己释放 */
            oal_net_free_netdev(pst_net_dev);

            return;
        }
    }
}


OAL_STATIC OAL_INLINE void* oal_net_device_priv(oal_net_device_stru *pst_net_dev)
{
    return pst_net_dev->ml_priv;
}



OAL_STATIC OAL_INLINE int32_t  oal_net_device_open(oal_net_device_stru *pst_dev)
{
    /* netdevice相关接口函数后续统一优化 */
    pst_dev->flags |= OAL_IFF_RUNNING;

    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE int32_t oal_net_device_close(oal_net_device_stru *pst_dev)
{
    /* netdevice相关接口函数后续统一优化 */
    pst_dev->flags &= ~OAL_IFF_RUNNING;

    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE int32_t oal_net_device_set_macaddr(oal_net_device_stru *pst_dev, void *pst_addr)
{
    errno_t l_ret;
    /* netdevice相关接口函数后续统一优化 */
    oal_sockaddr_stru *pst_mac;

    pst_mac = (oal_sockaddr_stru *)pst_addr;

    l_ret = memcpy_s(pst_dev->dev_addr, sizeof(pst_dev->dev_addr), pst_mac->sa_data, ETHER_ADDR_LEN);
    if (l_ret != EOK) {
        oal_io_print("oal_net_device_set_macaddr: memcpy_s failed!\n");
        return OAL_FAIL;
    }
    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE int32_t oal_net_device_init(oal_net_device_stru *pst_dev)
{
    /* netdevice相关接口函数后续统一优化 */
    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE oal_net_device_stats_stru *oal_net_device_get_stats(oal_net_device_stru *pst_dev)
{
    /* netdevice相关接口函数后续统一优化 */
    oal_net_device_stats_stru *pst_stats;

    pst_stats = &pst_dev->stats;

    pst_stats->tx_errors     = 0;
    pst_stats->tx_dropped    = 0;
    pst_stats->tx_packets    = 0;
    pst_stats->rx_packets    = 0;
    pst_stats->rx_errors     = 0;
    pst_stats->rx_dropped    = 0;
    pst_stats->rx_crc_errors = 0;

    return pst_stats;
}


OAL_STATIC OAL_INLINE int32_t oal_net_device_ioctl(oal_net_device_stru *pst_dev, oal_ifreq_stru *pst_ifr,
                                                   int32_t ul_cmd)
{
    /* netdevice相关接口函数后续统一优化 */
    return -OAL_EINVAL;
}


OAL_STATIC OAL_INLINE int32_t oal_net_device_multicast_list(oal_net_device_stru *pst_dev)
{
    /* netdevice相关接口函数后续统一优化 */
    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE int32_t oal_net_device_change_mtu(oal_net_device_stru *pst_dev, int32_t ul_mtu)
{
    /* 需要优化 */
    pst_dev->mtu = ul_mtu;
    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE int32_t oal_net_device_hardstart(oal_netbuf_stru *pst_skb, oal_net_device_stru *pst_dev)
{
    /* netdevice相关接口函数后续统一优化 */
    return OAL_SUCC;
}


/* 在dev.c中定义，用来在中断上下文或者非中断上下文中释放skb */
extern void dev_kfree_skb_any(struct sk_buff *skb);


OAL_STATIC OAL_INLINE void  oal_netbuf_reserve(oal_netbuf_stru *pst_netbuf, int32_t l_len)
{
    skb_reserve(pst_netbuf, l_len);
}


OAL_STATIC OAL_INLINE oal_netbuf_stru* oal_netbuf_alloc(uint32_t ul_size, int32_t l_reserve, int32_t l_align)
{
    oal_netbuf_stru *pst_netbuf = NULL;
    uint32_t       ul_offset;

    l_align = CACHE_ALIGNED_SIZE;

    /* 保证data部分的size不会再字节对齐后小于预先想分配的大小 */
    if (l_align) {
        ul_size += (l_align - 1);
    }

    pst_netbuf = dev_alloc_skb(ul_size);
    if (oal_unlikely(pst_netbuf == NULL)) {
        return NULL;
    }

    skb_reserve(pst_netbuf, l_reserve);

    if (l_align) {
        /* 计算为了能使4字节对齐的偏移量 */
        ul_offset = (int32_t)(((uint32_t)pst_netbuf->data) % (uint32_t)l_align);
        if (ul_offset) {
            skb_reserve(pst_netbuf, l_align - ul_offset);
        }
    }

    return pst_netbuf;
}

#ifdef _PRE_LWIP_ZERO_COPY
OAL_STATIC oal_netbuf_stru *oal_pbuf_netbuf_alloc(uint32_t ul_len)
{
    oal_netbuf_stru *pst_sk_buff = NULL;
    oal_lwip_buf    *pst_lwip_buf = NULL;

    /*
    |---PBUF_STRU---|-------HEADROOM-------|------ul_len-------|----TAILROOM-----|
    |    16BYTE     |         64BYTE       |      ul_len       |      32BYTE     |
    p_mem_head      head                   data                tail              end
    */
    pst_lwip_buf = pbuf_alloc(PBUF_RAW, ul_len, PBUF_RAM);
    if (oal_unlikely(pst_lwip_buf == NULL)) {
        return NULL;
    }

    pst_sk_buff = (oal_netbuf_stru *)oal_memalloc(SKB_DATA_ALIGN(OAL_SIZEOF(oal_netbuf_stru)));
    if (oal_unlikely(pst_sk_buff == NULL)) {
        pbuf_free(pst_lwip_buf);
        return NULL;
    }

    memset_s(pst_sk_buff, sizeof(oal_netbuf_stru), 0, sizeof(oal_netbuf_stru));
    oal_atomic_set(&pst_sk_buff->users, 1);

    pst_sk_buff->p_mem_head = (uint8_t *)pst_lwip_buf;
    pst_sk_buff->head       = pst_sk_buff->p_mem_head + LWIP_PBUF_STRUCT_SIZE;
    pst_sk_buff->data       = pst_sk_buff->head + PBUF_NETDEV_HEADROOM_RESERVE;
    skb_reset_tail_pointer(pst_sk_buff);
    pst_sk_buff->end        = pst_sk_buff->tail + ul_len + PBUF_NETDEV_TAILROOM_RESERVE;

    return pst_sk_buff;
}
#endif


OAL_STATIC OAL_INLINE uint32_t  oal_netbuf_free(oal_netbuf_stru *pst_netbuf)
{
    /* 释放调用alloc_skb接口申请的内存 */
    if ((pst_netbuf->p_mem_head == NULL) && (pst_netbuf->head != NULL)) {
        dev_kfree_skb(pst_netbuf);
    } else { /* 释放调用pbuf_alloc接口申请的内存 */
        pbuf_free((struct pbuf*)(pst_netbuf->p_mem_head));
        oal_free(pst_netbuf);
    }

    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE void  oal_netbuf_free_any(oal_netbuf_stru *pst_netbuf)
{
    /* 释放调用alloc_skb接口申请的内存 */
    if ((pst_netbuf->p_mem_head == NULL) && (pst_netbuf->head != NULL)) {
        dev_kfree_skb_any(pst_netbuf);
    } else { /* 释放调用pbuf_alloc接口申请的内存 */
        pbuf_free((struct pbuf*)(pst_netbuf->p_mem_head));
        oal_free(pst_netbuf);
    }
}


OAL_STATIC OAL_INLINE oal_netbuf_stru* oal_netbuf_copy(oal_netbuf_stru *pst_netbuf, oal_gfp_enum_uint8 en_priority)
{
    return skb_copy(pst_netbuf, en_priority);
}

OAL_STATIC OAL_INLINE oal_netbuf_stru* oal_netbuf_unshare(oal_netbuf_stru *pst_netbuf, oal_gfp_enum_uint8 en_priority)
{
    return skb_unshare(pst_netbuf, en_priority);
}



OAL_STATIC OAL_INLINE uint8_t* oal_netbuf_data(oal_netbuf_stru *pst_netbuf)
{
    return pst_netbuf->data;
}


OAL_STATIC OAL_INLINE uint8_t* oal_netbuf_header(oal_netbuf_stru *pst_netbuf)
{
    return pst_netbuf->data;
}


OAL_STATIC OAL_INLINE uint8_t* oal_netbuf_payload(oal_netbuf_stru *pst_netbuf)
{
    return pst_netbuf->data;
}



OAL_STATIC OAL_INLINE uint8_t *oal_netbuf_end(oal_netbuf_stru *pst_netbuf)
{
    return skb_end_pointer(pst_netbuf);
}


OAL_STATIC OAL_INLINE uint32_t  oal_netbuf_get_len(oal_netbuf_stru *pst_netbuf)
{
    return pst_netbuf->len;
}


OAL_STATIC OAL_INLINE uint32_t  oal_netbuf_headroom(const oal_netbuf_stru *pst_netbuf)
{
    return skb_headroom(pst_netbuf);
}


OAL_STATIC OAL_INLINE uint32_t  oal_netbuf_tailroom(const oal_netbuf_stru *pst_netbuf)
{
    return (uint32_t)skb_tailroom(pst_netbuf);
}


OAL_STATIC OAL_INLINE oal_netbuf_stru* oal_netbuf_realloc_headroom(oal_netbuf_stru *pst_netbuf, uint32_t ul_headroom)
{
    if (pskb_expand_head(pst_netbuf, ul_headroom, 0, GFP_ATOMIC)) {
        pst_netbuf = NULL;
    }

    return pst_netbuf;
}


OAL_STATIC OAL_INLINE oal_netbuf_stru* oal_netbuf_realloc_tailroom(oal_netbuf_stru *pst_netbuf, uint32_t ul_tailroom)
{
    if (pskb_expand_head(pst_netbuf, 0, ul_tailroom, GFP_ATOMIC)) {
        pst_netbuf = NULL;
    }

    return pst_netbuf;
}


OAL_STATIC OAL_INLINE uint8_t* oal_netbuf_cb(oal_netbuf_stru *pst_netbuf)
{
    return pst_netbuf->cb;
}


OAL_STATIC OAL_INLINE void  oal_netbuf_add_to_list(oal_netbuf_stru *pst_buf, oal_netbuf_stru *pst_prev,
                                                   oal_netbuf_stru *pst_next)
{
    pst_buf->next   = pst_next;
    pst_buf->prev   = pst_prev;
    pst_next->prev  = pst_buf;
    pst_prev->next  = pst_buf;
}


OAL_STATIC OAL_INLINE void  oal_netbuf_add_to_list_tail(oal_netbuf_stru *pst_buf, oal_netbuf_head_stru *pst_head)
{
    skb_queue_tail(pst_head, pst_buf);
}


OAL_STATIC OAL_INLINE uint32_t  oal_netbuf_list_len(oal_netbuf_head_stru *pst_head)
{
    return skb_queue_len(pst_head);
}



OAL_STATIC OAL_INLINE void  oal_netbuf_delete(oal_netbuf_stru *pst_buf, oal_netbuf_head_stru *pst_list_head)
{
    skb_unlink(pst_buf, pst_list_head);
}

OAL_STATIC OAL_INLINE void __netbuf_unlink(struct sk_buff *skb, struct sk_buff_head *list)
{
    struct sk_buff *next = NULL, *prev = NULL;

    list->qlen--;
    next       = skb->next;
    prev       = skb->prev;
    skb->next  = skb->prev = NULL;
    next->prev = prev;
    prev->next = next;
}


OAL_STATIC OAL_INLINE oal_netbuf_stru* oal_netbuf_delist(oal_netbuf_head_stru *pst_list_head)
{
    return skb_dequeue(pst_list_head);
}

/*
 *  * 函 数 名  : oal_netbuf_delist_nolock
 *   * 功能描述  : skb链表出队
 *    */
OAL_STATIC OAL_INLINE oal_netbuf_stru *oal_netbuf_delist_nolock(oal_netbuf_head_stru *pst_list_head)
{
    return __skb_dequeue(pst_list_head);
}


OAL_STATIC OAL_INLINE void oal_netbuf_addlist(oal_netbuf_head_stru *pst_list_head,
                                              oal_netbuf_stru* netbuf)
{
    return skb_queue_head(pst_list_head, netbuf);
}



OAL_STATIC OAL_INLINE void oal_netbuf_list_purge(oal_netbuf_head_stru *pst_list_head)
{
    skb_queue_purge(pst_list_head);
}


OAL_STATIC OAL_INLINE int32_t  oal_netbuf_list_empty(const oal_netbuf_head_stru *pst_list_head)
{
    return skb_queue_empty(pst_list_head);
}


OAL_STATIC OAL_INLINE void  oal_netbuf_list_head_init(oal_netbuf_head_stru *pst_list_head)
{
    skb_queue_head_init(pst_list_head);
}


OAL_STATIC OAL_INLINE oal_netbuf_stru* oal_netbuf_list_next(const oal_netbuf_stru *pst_buf)
{
    return pst_buf->next;
}


OAL_STATIC OAL_INLINE void oal_netbuf_list_tail(oal_netbuf_head_stru *list, oal_netbuf_stru *newsk)
{
    skb_queue_tail(list, newsk);
}

/*
 *  * 函 数 名  : oal_netbuf_list_tail_nolock
 *   * 功能描述  : add a netbuf to skb list
 *    */
OAL_STATIC OAL_INLINE void oal_netbuf_list_tail_nolock(oal_netbuf_head_stru *list, oal_netbuf_stru *newsk)
{
    __skb_queue_tail(list, newsk);
}


OAL_STATIC OAL_INLINE void oal_netbuf_splice_init(oal_netbuf_head_stru *list, oal_netbuf_head_stru *head)
{
    skb_queue_splice_init(list, head);
}

OAL_STATIC OAL_INLINE void oal_netbuf_queue_splice_tail_init(oal_netbuf_head_stru *list, oal_netbuf_head_stru *head)
{
    skb_queue_splice_tail_init(list, head);
}


OAL_STATIC OAL_INLINE oal_netbuf_stru* oal_netbuf_delist_tail(oal_netbuf_head_stru *head)
{
    return skb_dequeue_tail(head);
}


OAL_STATIC OAL_INLINE void oal_netbuf_splice_sync(oal_netbuf_head_stru *list, oal_netbuf_head_stru *head)
{
    oal_netbuf_stru* netbuf = NULL;
    for (;;) {
        netbuf = oal_netbuf_delist_tail(head);
        if (netbuf == NULL) {
            break;
        }
        oal_netbuf_addlist(list, netbuf);
    }
}


OAL_STATIC OAL_INLINE void oal_netbuf_head_init(oal_netbuf_head_stru *list)
{
    skb_queue_head_init(list);
}


OAL_STATIC OAL_INLINE oal_netbuf_stru* oal_netbuf_peek(oal_netbuf_head_stru *pst_head)
{
    return skb_peek(pst_head);
}
OAL_STATIC OAL_INLINE oal_netbuf_stru *oal_netbuf_peek_next(oal_netbuf_stru *skb, oal_netbuf_head_stru *pst_head)
{
    return skb_peek_next(skb, pst_head);
}

OAL_STATIC OAL_INLINE oal_netbuf_stru* oal_netbuf_tail(oal_netbuf_head_stru *pst_head)
{
    return skb_peek_tail(pst_head);
}


OAL_STATIC OAL_INLINE uint32_t  oal_netbuf_free_list(oal_netbuf_head_stru *pst_head, uint32_t ul_num)
{
    uint32_t ul_index;
    uint32_t ul_ret;

    for (ul_index = 0; ul_index < ul_num; ul_index++) {
        ul_ret = oal_netbuf_free(oal_netbuf_delist(pst_head));
        if (ul_ret != OAL_SUCC) {
            return ul_ret;
        }
    }

    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE uint32_t  oal_netbuf_get_appointed_netbuf(oal_netbuf_stru *pst_netbuf, uint8_t uc_num,
                                                                oal_netbuf_stru **pst_expect_netbuf)
{
    uint8_t   uc_buf_num;

    if (oal_unlikely((pst_netbuf == NULL) || (pst_expect_netbuf == NULL))) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    *pst_expect_netbuf = NULL;

    for (uc_buf_num = 0; uc_buf_num < uc_num; uc_buf_num++) {
        *pst_expect_netbuf = oal_get_netbuf_next(pst_netbuf);

        if (*pst_expect_netbuf == NULL) {
            break;
        }

        pst_netbuf = *pst_expect_netbuf;
    }

    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE oal_netbuf_stru* oal_netbuf_clone(oal_netbuf_stru *pst_buf)
{
    int32_t   l_flags = 0;

    if (oal_unlikely(pst_buf == NULL)) {
        return NULL;
    }

    return skb_clone(pst_buf, l_flags);
}

/*
 * 函 数 名  : oal_netbuf_read_user
 * 功能描述  : 读取netbuf引用计数
 */
OAL_STATIC OAL_INLINE uint32_t oal_netbuf_read_user(oal_netbuf_stru *pst_buf)
{
    if (oal_unlikely(pst_buf == NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    return (uint32_t)oal_atomic_read(&(pst_buf->users));
}

/*
 * 函 数 名  : oal_netbuf_set_user
 * 功能描述  : 设置netbuf引用计数
 */
OAL_STATIC OAL_INLINE void oal_netbuf_set_user(oal_netbuf_stru *pst_buf, uint32_t refcount)
{
    if (oal_unlikely(pst_buf == NULL)) {
        return;
    }

    oal_atomic_set(&(pst_buf->users), (int32_t)refcount);
}



OAL_STATIC OAL_INLINE uint32_t  oal_netbuf_decrease_user(oal_netbuf_stru *pst_buf)
{
    if (oal_unlikely(pst_buf == NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_atomic_dec(&(pst_buf->users));

    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE uint32_t  oal_netbuf_increase_user(oal_netbuf_stru *pst_buf)
{
    if (oal_unlikely(pst_buf == NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_atomic_inc(&(pst_buf->users));

    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE uint32_t  oal_netbuf_get_buf_num(oal_netbuf_head_stru *pst_netbuf_head)
{
    return pst_netbuf_head->qlen;
}


OAL_STATIC OAL_INLINE oal_netbuf_stru* oal_netbuf_get(oal_netbuf_stru *pst_netbuf)
{
    return skb_get(pst_netbuf);
}


OAL_STATIC OAL_INLINE void  oal_netbuf_queue_purge(oal_netbuf_head_stru  *pst_netbuf_head)
{
    skb_queue_purge(pst_netbuf_head);
}


OAL_STATIC OAL_INLINE oal_netbuf_stru*  oal_netbuf_copy_expand(oal_netbuf_stru *pst_netbuf, int32_t ul_headroom,
                                                               int32_t ul_tailroom, oal_gfp_enum_uint8 en_gfp_mask)
{
    return skb_copy_expand(pst_netbuf, ul_headroom, ul_tailroom, en_gfp_mask);
}

OAL_STATIC OAL_INLINE int32_t  oal_netif_rx_hw(oal_netbuf_stru *pst_netbuf)
{
    return hwal_netif_rx(pst_netbuf->dev, pst_netbuf);
}



OAL_STATIC OAL_INLINE int32_t  oal_netif_rx(oal_netbuf_stru *pst_netbuf)
{
    return hwal_netif_rx(pst_netbuf->dev, pst_netbuf);
}


OAL_STATIC OAL_INLINE int32_t  oal_netif_rx_ni(oal_netbuf_stru *pst_netbuf)
{
    return hwal_netif_rx(pst_netbuf->dev, pst_netbuf);
}


OAL_STATIC OAL_INLINE void  oal_local_bh_disable(void)
{
    return;
}

OAL_STATIC OAL_INLINE void  oal_local_bh_enable(void)
{
    return;
}


OAL_STATIC OAL_INLINE uint64_t  oal_cpu_clock(void)
{
    return OAL_SUCC;
}

OAL_STATIC OAL_INLINE int32_t oal_netbuf_expand_head(oal_netbuf_stru *netbuf, int32_t nhead, int32_t ntail,
                                                     int32_t gfp_mask)
{
    return pskb_expand_head(netbuf, nhead, ntail, gfp_mask);
}


OAL_STATIC OAL_INLINE oal_sock_stru* oal_netlink_kernel_create(oal_net_stru *pst_net, int32_t l_unit,
    uint32_t ul_groups, void (*input)(oal_netbuf_stru *pst_netbuf),
    oal_mutex_stru *pst_cb_mutex, oal_module_stru *pst_module)
{
    return &g_st_sock;
}


OAL_STATIC OAL_INLINE void  oal_netlink_kernel_release(oal_sock_stru *pst_sock)
{
    return;
}


OAL_STATIC OAL_INLINE oal_nlmsghdr_stru* oal_nlmsg_hdr(const oal_netbuf_stru *pst_netbuf)
{
    return (oal_nlmsghdr_stru *)OAL_NETBUF_HEADER(pst_netbuf);
}


OAL_STATIC OAL_INLINE int32_t  oal_nlmsg_msg_size(int32_t l_payload)
{
    return OAL_NLMSG_HDRLEN + l_payload;
}


OAL_STATIC OAL_INLINE int32_t  oal_nlmsg_total_size(int32_t l_payload)
{
    return OAL_NLMSG_ALIGN(oal_nlmsg_msg_size(l_payload));
}


OAL_STATIC OAL_INLINE oal_nlmsghdr_stru* oal_nlmsg_put(
    oal_netbuf_stru *pst_netbuf, uint32_t ul_pid,
    uint32_t ul_seq, int32_t l_type, int32_t l_payload, int32_t l_flags)
{
    oal_nlmsghdr_stru   *pst_nlmsghdr = NULL;
    int32_t            l_size;

    if (oal_unlikely((int32_t)oal_netbuf_tailroom(pst_netbuf) < oal_nlmsg_total_size(l_payload))) {
        return NULL;
    }

    l_size = OAL_NLMSG_LENGTH(l_payload);

    pst_nlmsghdr = (oal_nlmsghdr_stru *)oal_netbuf_put(pst_netbuf, OAL_NLMSG_ALIGN(l_size));
    pst_nlmsghdr->nlmsg_type = (uint16_t)l_type;
    pst_nlmsghdr->nlmsg_len = (uint32_t)l_size;
    pst_nlmsghdr->nlmsg_flags = (uint16_t)l_flags;
    pst_nlmsghdr->nlmsg_pid = ul_pid;
    pst_nlmsghdr->nlmsg_seq = ul_seq;
    if (OAL_NLMSG_ALIGN(l_size) - l_size != 0) {
        if (memset_s((uint8_t *)OAL_NLMSG_DATA(pst_nlmsghdr) + l_payload,
                     OAL_NLMSG_ALIGN(l_size) - l_size,
                     0, OAL_NLMSG_ALIGN(l_size) - l_size) != EOK) {
            oal_io_print("oal_nlmsg_put::memset fail!");
            return NULL;
        }
    }

    return pst_nlmsghdr;
}


OAL_STATIC OAL_INLINE int32_t oal_nla_put_u32(oal_netbuf_stru *pst_skb, int32_t l_attrtype, uint32_t ul_value)
{
    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE int32_t  oal_nla_put(oal_netbuf_stru *pst_skb, int32_t l_attrtype, int32_t l_attrlen,
                                           const void *p_data)
{
    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE  oal_netbuf_stru *oal_nlmsg_new(int32_t payload, oal_gfp_enum_uint8 flags)
{
    return NULL;
}


OAL_STATIC OAL_INLINE void oal_nlmsg_free(oal_netbuf_stru *pst_skb)
{
    return;
}


OAL_STATIC OAL_INLINE int32_t  oal_genlmsg_multicast(oal_netbuf_stru *pst_skb, uint32_t ul_pid,
    uint32_t ul_group, oal_gfp_enum_uint8 flags)
{
    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE void *oal_genlmsg_put(oal_netbuf_stru *pst_skb, uint32_t ul_pid, uint32_t ul_seq,
    oal_genl_family_stru *pst_family, int32_t flags, uint8_t cmd)
{
    return (void *)NULL;
}


OAL_STATIC OAL_INLINE oal_nlattr_stru *oal_nla_nest_start(oal_netbuf_stru *pst_skb, int32_t l_attrtype)
{
    oal_nlattr_stru *pst_nla_nest_start = NULL;

    return pst_nla_nest_start;
}


OAL_STATIC OAL_INLINE void  oal_genlmsg_cancel(oal_netbuf_stru *pst_skb, void *pst_hdr)
{
    return;
}


OAL_STATIC OAL_INLINE int32_t  oal_nla_nest_end(oal_netbuf_stru *pst_skb, oal_nlattr_stru *pst_start)
{
    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE int32_t  oal_genlmsg_end(oal_netbuf_stru *pst_skb, void *pst_hdr)
{
    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE void oal_netbuf_reset(oal_netbuf_stru *pst_netbuf, uint32_t ul_data_offset)
{
#if (defined(_PRE_BOARD_SD5610) || defined(_PRE_BOARD_SD5115))
    struct skb_shared_info *shinfo;

    /* tail之前的成员初始化为0 */
    memset_s(pst_netbuf, offsetof(struct sk_buff, tail), 0, offsetof(struct sk_buff, tail));

    /* 初始化skb的share info */
    shinfo = skb_shinfo(pst_netbuf);
    shinfo->nr_frags  = 0;
    shinfo->gso_size = 0;
    shinfo->gso_segs = 0;
    shinfo->gso_type = 0;
    shinfo->ip6_frag_id = 0;
    shinfo->tx_flags.flags = 0;
    shinfo->frag_list = NULL;
    memset_s(&shinfo->hwtstamps, sizeof(shinfo->hwtstamps), 0, sizeof(shinfo->hwtstamps));

    /* data tail指针复位 */
    pst_netbuf->data = pst_netbuf->head + ul_data_offset;
    pst_netbuf->tail = pst_netbuf->data;
    pst_netbuf->len  = 0;
#endif
}


OAL_STATIC OAL_INLINE oal_cfg80211_registered_device_stru *oal_wiphy_to_dev(oal_wiphy_stru *pst_wiphy)
{
    return NULL;
}


OAL_STATIC OAL_INLINE int32_t  oal_netlink_unicast(oal_sock_stru *pst_sock, oal_netbuf_stru *pst_netbuf,
    uint32_t ul_pid, int32_t l_nonblock)
{
    oal_netbuf_free(pst_netbuf);

    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE int32_t  oal_netlink_broadcast(oal_sock_stru *pst_sock, oal_netbuf_stru *pst_netbuf,
    uint32_t ul_pid, int32_t ul_group, oal_gfp_enum_uint8  en_gfp)
{
    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE uint32_t  oal_netbuf_copydata(oal_netbuf_stru *pst_netbuf_sc, uint32_t ul_offset,
                                                    void *p_dst, uint32_t ul_len)
{
    return (uint32_t)skb_copy_bits(pst_netbuf_sc, ul_offset, p_dst, ul_len);
}


OAL_STATIC OAL_INLINE void  oal_netbuf_trim(oal_netbuf_stru *pst_netbuf, uint32_t ul_len)
{
    return skb_trim(pst_netbuf, pst_netbuf->len - ul_len);
}


OAL_STATIC OAL_INLINE void  oal_netbuf_concat(oal_netbuf_stru *pst_netbuf_head, oal_netbuf_stru *pst_netbuf)
{
    if (skb_is_nonlinear(pst_netbuf_head)) {
        oal_io_print("oal_netbuf_concat:pst_netbuf_head not linear ");
    }

    if (skb_tailroom(pst_netbuf_head) < pst_netbuf->len) {
        oal_io_print("not enough space for concat");
    }

    if (memcpy_s(skb_tail_pointer(pst_netbuf_head), pst_netbuf_head->len, pst_netbuf->data, pst_netbuf->len) != EOK) {
        oal_io_print("oal_netbuf_concat::memcpy_s failed !");
    }

    skb_put(pst_netbuf_head, pst_netbuf->len);

    dev_kfree_skb(pst_netbuf);
}



OAL_STATIC OAL_INLINE void  oal_netbuf_set_len(oal_netbuf_stru *pst_netbuf, uint32_t ul_len)
{
    if (pst_netbuf->len > ul_len) {
        skb_trim(pst_netbuf, ul_len);
    } else {
        skb_put(pst_netbuf, (ul_len - pst_netbuf->len));
    }
}


OAL_STATIC OAL_INLINE void  oal_netbuf_init(oal_netbuf_stru *pst_netbuf, uint32_t ul_len)
{
    oal_netbuf_set_len(pst_netbuf, ul_len);
    pst_netbuf->protocol = ETH_P_CONTROL;
}


OAL_STATIC OAL_INLINE void  oal_hi_kernel_wdt_clear(void)
{
#if defined(_PRE_BOARD_SD5115)
    hi_kernel_wdt_clear();
#endif
}


OAL_STATIC OAL_INLINE uint32_t oal_in_aton(uint8_t *pul_str)
{
    uint32_t ul_l;
    uint32_t ul_val;
    uint8_t  uc_i;

    ul_l = 0;

    for (uc_i = 0; uc_i < 4; uc_i++) {
        ul_l <<= 8;
        if (*pul_str != '\0') {
            ul_val = 0;
            while (*pul_str != '\0' && *pul_str != '.' && *pul_str != '\n') {
                ul_val *= 10;
                ul_val += *pul_str - '0';
                pul_str++;
            }

            ul_l |= ul_val;

            if (*pul_str != '\0') {
                pul_str++;
            }
        }
    }

    return(oal_byteorder_host_to_net_uint32(ul_l));
}


OAL_STATIC OAL_INLINE void  oal_ipv6_addr_copy(oal_in6_addr *pst_ipv6_dst, oal_in6_addr *pst_ipv6_src)
{
    return;
}


OAL_STATIC OAL_INLINE int32_t  oal_dev_hard_header(oal_netbuf_stru *pst_nb, oal_net_device_stru *pst_net_dev,
    uint16_t us_type, uint8_t *puc_addr_d, uint8_t *puc_addr_s, uint32_t ul_len)
{
    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE uint16_t  oal_csum_ipv6_magic(oal_in6_addr *pst_ipv6_s, oal_in6_addr *pst_ipv6_d,
    uint32_t ul_len, uint16_t us_proto, uint32_t ul_sum)
{
    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE uint32_t  oal_csum_partial(const void *p_buff, int32_t  l_len, uint32_t ul_sum)
{
    return OAL_SUCC;
}
/*lint +e778*/ /*lint +e572*/ /*lint +e778*/ /*lint +e713*/

OAL_STATIC OAL_INLINE int32_t ipv6_addr_type(oal_in6_addr *pst_addr6)
{
    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE int32_t  oal_ipv6_addr_type(oal_in6_addr *pst_ipv6)
{
    return ipv6_addr_type(pst_ipv6);
}


OAL_STATIC OAL_INLINE int32_t  oal_pskb_may_pull(oal_netbuf_stru *pst_nb, uint32_t ul_len)
{
    return pskb_may_pull(pst_nb, ul_len);
}


OAL_STATIC OAL_INLINE int32_t arp_hdr_len(oal_net_device_stru *pst_dev)
{
    /* ARP header, plus 2 device addresses, plus 2 IP addresses. */
    return sizeof(oal_eth_arphdr_stru) + (pst_dev->addr_len + sizeof(uint32_t)) * 2;
}


OAL_STATIC OAL_INLINE int32_t  eth_header(oal_netbuf_stru *skb, oal_net_device_stru *dev, uint16_t type,
                                          void *daddr, void *saddr, uint32_t len)
{
    oal_ether_header_stru *eth = (oal_ether_header_stru *)oal_netbuf_push(skb, 14);

    if (type != 0x0001 && type != 0x0004) {
        eth->us_ether_type = oal_host2net_short(type);
    } else {
        eth->us_ether_type = oal_host2net_short(len);
    }

    if (saddr == NULL) {
        saddr = dev->dev_addr;
    }

    if (memcpy_s(eth->auc_ether_shost, ETHER_ADDR_LEN, saddr, 6) != EOK) {
        oal_io_print("eth_header::memcpy_s fail!");
    }

    if (daddr != NULL) {
        if (memcpy_s(eth->auc_ether_dhost, ETHER_ADDR_LEN, daddr, 6) != EOK) {
            oal_io_print("eth_header::memcpy_s fail!");
        }
        return 14;
    }

    return -14;
}


OAL_STATIC OAL_INLINE oal_netbuf_stru  *oal_arp_create(int32_t l_type, int32_t l_ptype, uint32_t ul_dest_ip,
                                                       oal_net_device_stru *pst_dev, uint32_t ul_src_ip,
                                                       uint8_t *puc_dest_hw, uint8_t *puc_src_hw,
                                                       uint8_t *puc_target_hw)
{
    oal_netbuf_stru             *pst_skb = NULL;
    oal_eth_arphdr_stru         *pst_arp = NULL;
    uint8_t                   *pst_arp_ptr = NULL;
    int8_t                    ac_bcast[6] = { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33 };
    int32_t                   l_ret       = EOK;
    int len = OAL_SIZEOF(ac_bcast);

    pst_skb = oal_netbuf_alloc(arp_hdr_len(pst_dev) + LL_ALLOCATED_SPACE(pst_dev), 0, 4);
    if (pst_skb == NULL) {
        return NULL;
    }

    skb_reserve(pst_skb, LL_RESERVED_SPACE(pst_dev)); /* reserve 16 */
    pst_arp = (oal_eth_arphdr_stru *) oal_netbuf_put(pst_skb, (uint32_t)arp_hdr_len(pst_dev));
    pst_skb->dev = pst_dev;
    pst_skb->protocol = oal_host2net_short(ETHER_TYPE_ARP);
    if (strlen(puc_src_hw) == 0) {
        l_ret += memcpy_s(puc_src_hw, ETHER_ADDR_LEN, pst_dev->dev_addr, len);
    }

    if (strlen(puc_dest_hw) == 0) {
        l_ret += memcpy_s(puc_dest_hw, ETHER_ADDR_LEN, ac_bcast, len);
    }

    /*lint -e734*/
    if (eth_header(pst_skb, pst_dev, l_ptype, puc_dest_hw, puc_src_hw, pst_skb->len) < 0) {
        oal_netbuf_free(pst_skb);
        return NULL;
    }/*lint +e734*/

    /*lint -e778*/
    pst_arp->us_ar_hrd = oal_host2net_short(pst_dev->type);
    pst_arp->us_ar_pro = oal_host2net_short(ETHER_TYPE_IP);
    /*lint +e778*/
    pst_arp->uc_ar_hln = 6;
    pst_arp->uc_ar_pln = 4;
    pst_arp->us_ar_op = oal_host2net_short(l_type);

    pst_arp_ptr = (uint8_t *)pst_arp + 8; // 偏移8长字
    l_ret += memcpy_s(pst_arp_ptr, ETHER_ADDR_LEN, puc_src_hw, len);

    pst_arp_ptr += 6; // 偏移uc_ar_hln 6
    l_ret += memcpy_s(pst_arp_ptr, ETH_SENDER_IP_ADDR_LEN, &ul_src_ip, sizeof(ul_src_ip));

    pst_arp_ptr += 4; // 偏移uc_ar_pln 4
    if (puc_target_hw != NULL) {
        l_ret += memcpy_s(pst_arp_ptr, ETHER_ADDR_LEN, puc_target_hw, len);
    } else {
        if (memset_s(pst_arp_ptr, len, 0, len) != EOK) {
            oal_io_print("oal_arp_create::memset fail!");
            oal_netbuf_free(pst_skb);
            return NULL;
        }
    }

    pst_arp_ptr += 6; // 偏移uc_ar_hln 6
    l_ret += memcpy_s(pst_arp_ptr, ETH_TARGET_IP_ADDR_LEN, &ul_dest_ip, sizeof(ul_src_ip));
    if (l_ret != EOK) {
        oal_io_print("oal_arp_create::memcpy_s fail!");
        oal_netbuf_free(pst_skb);
        return NULL;
    }
    return pst_skb;
}

/* get the queue index of the input skbuff */
OAL_STATIC OAL_INLINE uint16_t oal_skb_get_queue_mapping(oal_netbuf_stru  *pst_skb)
{
    return skb_get_queue_mapping(pst_skb);
}

OAL_STATIC OAL_INLINE void oal_skb_set_queue_mapping(oal_netbuf_stru  *pst_skb, uint16_t queue_mapping)
{
    skb_set_queue_mapping(pst_skb, queue_mapping);
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_net.h */
