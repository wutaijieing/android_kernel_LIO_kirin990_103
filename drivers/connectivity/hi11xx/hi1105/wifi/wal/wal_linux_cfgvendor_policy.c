/*
 * 版权所有 (c) 华为技术有限公司 2016-2018
 * 功能说明 : Linux cfgvendor policy 适配
 * 创建日期 : 2022年02月22日
 */

#include "oal_net.h"
#include "oal_cfg80211.h"
#include "wlan_types.h"
#include "mac_device_common.h"
#include "wal_linux_cfgvendor.h"
#include "wal_linux_cfgvendor_attributes.h"


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
const struct nla_policy wal_andr_wifi_attr_policy[ANDR_WIFI_ATTRIBUTE_MAX + 1] = {
    [ANDR_WIFI_ATTRIBUTE_RANDOM_MAC_OUI] = { .type = NLA_BINARY,
        .len = sizeof(NLA_U8) * WLAN_RANDOM_MAC_OUI_LEN },
    [ANDR_WIFI_ATTRIBUTE_COUNTRY] = { .type = NLA_BINARY,
        .len = sizeof(NLA_U8) * WLAN_COUNTRY_STR_LEN },
};

const struct nla_policy wal_gscan_attr_policy[GSCAN_ATTRIBUTE_MAX] = {
    [GSCAN_ATTRIBUTE_BAND] = { .type = NLA_U32 },
    [GSCAN_ATTRIBUTE_NUM_BSSID] = { .type = NLA_U32 },
    [GSCAN_ATTRIBUTE_BSSID_BLACKLIST_FLUSH] = { .type = NLA_U32},
    [GSCAN_ATTRIBUTE_BLACKLIST_BSSID] = { .type = NLA_BINARY,
        .len = sizeof(NLA_U8) * OAL_MAC_ADDR_LEN * MAX_BLACKLIST_BSSID },
    [GSCAN_ATTRIBUTE_ROAM_STATE_SET] = { .type = NLA_U32},
};

const struct nla_policy wal_mkeep_alive_attr_policy[MKEEP_ALIVE_ATTRIBUTE_MAX + 1] = {
    [MKEEP_ALIVE_ATTRIBUTE_ID] = { .type = NLA_U8 },
    [MKEEP_ALIVE_ATTRIBUTE_IP_PKT] = { .type = NLA_BINARY,
        .len = sizeof(NLA_U8) * MKEEP_ALIVE_IP_PKT_MAXLEN },
    [MKEEP_ALIVE_ATTRIBUTE_IP_PKT_LEN] = { .type = NLA_U16 },
    [MKEEP_ALIVE_ATTRIBUTE_SRC_MAC_ADDR] = { .type = NLA_BINARY,
        .len = sizeof(NLA_U8) * WLAN_MAC_ADDR_LEN },
    [MKEEP_ALIVE_ATTRIBUTE_DST_MAC_ADDR] = { .type = NLA_BINARY,
        .len = sizeof(NLA_U8) * WLAN_MAC_ADDR_LEN },
    [MKEEP_ALIVE_ATTRIBUTE_PERIOD_MSEC] = { .type = NLA_U32 },
    [MKEEP_ALIVE_ATTRIBUTE_ETHER_TYPE] = { .type = NLA_U16 },
};

#ifdef _PRE_WLAN_FEATURE_APF
const struct nla_policy wal_apf_attr_policy[APF_ATTRIBUTE_MAX + 1] = {
    [APF_ATTRIBUTE_PROGRAM] =  { .type = NLA_BINARY,
        .len = sizeof(NLA_U8) * APF_PROGRAM_MAX_LEN },
    [APF_ATTRIBUTE_PROGRAM_LEN] ={ .type = NLA_U32 },
};
#endif

#ifdef _PRE_WLAN_FEATURE_NAN
const struct nla_policy wal_nan_attr_policy[NAN_ATTRIBUTE_MAX + 1] = {
    [NAN_ATTRIBUTE_DURATION_CONFIGURE] = { .type = NLA_U32 },
    [NAN_ATTRIBUTE_PERIOD_CONFIGURE] = { .type = NLA_U32 },
    [NAN_ATTRIBUTE_TYPE_CONFIGURE] = { .type = NLA_U8 },
    [NAN_ATTRIBUTE_BAND_CONFIGURE] = { .type = NLA_U8 },
    [NAN_ATTRIBUTE_CHANNEL_CONFIGURE] = { .type = NLA_U8 },
    [NAN_ATTRIBUTE_DATATYPE] = { .type = NLA_U8 },
    [NAN_ATTRIBUTE_DATALEN] = { .type = NLA_U16 },
    [NAN_ATTRIBUTE_DATA_FRAME] = { .type = NLA_BINARY,
        .len = 1024 }, /* 上层下发data_frame长度为1024 */
    [NAN_ATTRIBUTE_TRANSACTION_ID] = { .type = NLA_U16 },
};
#endif

#ifdef _PRE_WLAN_FEATURE_FTM
const struct nla_policy wal_rtt_attr_policy[RTT_ATTRIBUTE_MAX + 1] = {
    [RTT_ATTRIBUTE_TARGET_CNT] = { .type = NLA_U8 },
    [RTT_ATTRIBUTE_TARGET_INFO] = { .type = NLA_BINARY,
        .len = OAL_UINT16_MAX },
    [RTT_ATTRIBUTE_TARGET_MAC] = { .type = NLA_BINARY,
        .len = sizeof(NLA_U8) * WLAN_MAC_ADDR_LEN },
    [RTT_ATTRIBUTE_TARGET_TYPE] = { .type = NLA_U32 },
    [RTT_ATTRIBUTE_TARGET_PEER] = { .type = NLA_U32 },
    [RTT_ATTRIBUTE_TARGET_CHAN] = { .type = NLA_BINARY,
        .len = 32 }, // sizeof(mac_ftm_wifi_channel_info)大小为32字节
    [RTT_ATTRIBUTE_TARGET_PERIOD] = { .type = NLA_U32 },
    [RTT_ATTRIBUTE_TARGET_NUM_BURST] = { .type = NLA_U32 },
    [RTT_ATTRIBUTE_TARGET_NUM_FTM_BURST] = { .type = NLA_U32 },
    [RTT_ATTRIBUTE_TARGET_LCI] = { .type = NLA_U8 },
    [RTT_ATTRIBUTE_TARGET_LCR] = { .type = NLA_U8 },
    [RTT_ATTRIBUTE_TARGET_BURST_DURATION] = { .type = NLA_U32 },
    [RTT_ATTRIBUTE_TARGET_PREAMBLE] = { .type = NLA_U32 },
    [RTT_ATTRIBUTE_TARGET_BW] = { .type = NLA_U32 },
};
#endif

#ifdef _PRE_WLAN_CHBA_MGMT
const struct nla_policy wal_chba_conn_ntf_attr_policy[MAC_CHBA_CONN_NTF_ATTR_MAX + 1] = {
    [MAC_CHBA_CONN_NTF_ATTR_ID] = { .type = NLA_U8 },
    [MAC_CHBA_CONN_NTF_ATTR_MAC_ADDR] = { .type = NLA_BINARY,
        .len = sizeof(NLA_U8) * WLAN_MAC_ADDR_LEN },
    [MAC_CHBA_CONN_NTF_ATTR_CENTER_FREQ_20M] = { .type = NLA_U32 },
    [MAC_CHBA_CONN_NTF_ATTR_CENTER_FREQ_1] = { .type = NLA_U32 },
    [MAC_CHBA_CONN_NTF_ATTR_CENTER_FREQ_2] = { .type = NLA_U32 },
    [MAC_CHBA_CONN_NTF_ATTR_CENTER_BANDWIDTH] = { .type = NLA_U8 },
    [MAC_CHBA_CONN_NTF_ATTR_EXPIRE_TIME] = { .type = NLA_U16 },
};
#endif
#endif