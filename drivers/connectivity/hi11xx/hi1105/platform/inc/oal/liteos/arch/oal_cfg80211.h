

#ifndef __OAL_LITEOS_CFG80211_H__
#define __OAL_LITEOS_CFG80211_H__

#include "los_typedef.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
/* hostapd和supplicant事件上报需要用到宏 */
#define OAL_NLMSG_GOODSIZE   1920
#define OAL_ETH_ALEN_SIZE         6
#define OAL_NLMSG_DEFAULT_SIZE (OAL_NLMSG_GOODSIZE - OAL_NLMSG_HDRLEN)
#define OAL_IEEE80211_MIN_ACTION_SIZE 1000

#define OAL_NLA_PUT_U32(skb, attrtype, value)      NLA_PUT_U32(skb, attrtype, value)
#define OAL_NLA_PUT(skb, attrtype, attrlen, data)  NLA_PUT(skb, attrtype, attrlen, data)
#define OAL_NLA_PUT_U16(skb, attrtype, value)      NLA_PUT_U16(skb, attrtype, value)
#define OAL_NLA_PUT_U8(skb, attrtype, value)       NLA_PUT_U8(skb, attrtype, value)
#define OAL_NLA_PUT_FLAG(skb, attrtype)            NLA_PUT_FLAG(skb, attrtype)

#define WLAN_CAPABILITY_ESS         (1 << 0)
#define WLAN_CAPABILITY_IBSS        (1 << 1)

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0))
typedef enum rate_info_flags {
    RATE_INFO_FLAGS_MCS          = BIT(0),
    RATE_INFO_FLAGS_40_MHZ_WIDTH = BIT(1),
    RATE_INFO_FLAGS_SHORT_GI     = BIT(2),
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0))
    RATE_INFO_FLAGS_60G          = BIT(3),
#endif
} oal_rate_info_flags;
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0))
typedef enum rate_info_flags {
    RATE_INFO_FLAGS_MCS             = BIT(0),
    RATE_INFO_FLAGS_VHT_MCS         = BIT(1),
    RATE_INFO_FLAGS_40_MHZ_WIDTH    = BIT(2),
    RATE_INFO_FLAGS_80_MHZ_WIDTH    = BIT(3),
    RATE_INFO_FLAGS_80P80_MHZ_WIDTH = BIT(4),
    RATE_INFO_FLAGS_160_MHZ_WIDTH   = BIT(5),
    RATE_INFO_FLAGS_SHORT_GI        = BIT(6),
    RATE_INFO_FLAGS_60G             = BIT(7),
} oal_rate_info_flags;
#else
typedef enum rate_info_flags {
    RATE_INFO_FLAGS_MCS      = BIT(0),
    RATE_INFO_FLAGS_VHT_MCS  = BIT(1),
    RATE_INFO_FLAGS_SHORT_GI = BIT(2),
    RATE_INFO_FLAGS_60G      = BIT(3),
} oal_rate_info_flags;

enum rate_info_bw {
    RATE_INFO_BW_5,
    RATE_INFO_BW_10,
    RATE_INFO_BW_20,
    RATE_INFO_BW_40,
    RATE_INFO_BW_80,
    RATE_INFO_BW_160,
} oal_rate_info_bw;
#endif
struct cfg80211_external_auth_params {
    enum nl80211_external_auth_action action;
    uint8_t                           bssid[OAL_ETH_ALEN_SIZE];
    oal_cfg80211_ssid_stru            ssid;
    unsigned int                      key_mgmt_suite;
    uint16_t                          status;
};
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_cfg80211.h */
