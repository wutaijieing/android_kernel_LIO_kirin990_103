

#ifndef __DRIVER_HISI_COMMON_H__
#define __DRIVER_HISI_COMMON_H__

// 1 其他头文件包含
#include "oal_types.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

// 3 宏定义
#if ((_PRE_TEST_MODE_UT == _PRE_TEST_MODE) || \
    (_PRE_TEST_MODE_ST == _PRE_TEST_MODE) || \
    defined(_PRE_WIFI_DMT)) || defined(_PRE_PC_LINT)
#define OAL_STATIC
#define OAL_VOLATILE
#ifdef INLINE_TO_FORCEINLINE
#define OAL_INLINE      __forceinline
#else
#define OAL_INLINE      __inline
#endif
#else
#define OAL_STATIC          static
#define OAL_VOLATILE        volatile
#ifdef INLINE_TO_FORCEINLINE
#define OAL_INLINE      __forceinline
#else
#define OAL_INLINE      inline
#endif
#endif

#define OAL_CONST           const
#define HISI_SUCC            0
#define HISI_EFAIL           1
#define HISI_EINVAL         22

#define HISI_BITRATE_MAX_NUM 12
#define HISI_CHANNEL_MAX_NUM 14

#ifndef ETH_ADDR_LEN
#define ETH_ADDR_LEN        6
#endif

#ifndef MAX_SSID_LEN
#define MAX_SSID_LEN        32
#endif

#ifndef HISI_MAX_NR_CIPHER_SUITES
#define HISI_MAX_NR_CIPHER_SUITES    5
#endif

#ifndef HISI_MAX_NR_AKM_SUITES
#define HISI_MAX_NR_AKM_SUITES       2
#endif

#ifndef HISI_WPAS_MAX_SCAN_SSIDS
#define HISI_WPAS_MAX_SCAN_SSIDS     16
#endif

#ifndef    HISI_PTR_NULL
#define    HISI_PTR_NULL  NULL
#endif

#ifndef    SCAN_AP_LIMIT
#define    SCAN_AP_LIMIT            64
#endif

#ifndef   NETDEV_UP
#define   NETDEV_UP              0x0001
#endif
#ifndef   NETDEV_DOWN
#define   NETDEV_DOWN            0x0002
#endif

#ifndef   NOTIFY_DONE
#define   NOTIFY_DONE            0x0000
#endif

// 4 枚举定义
typedef enum {
    HISI_FALSE   = 0,
    HISI_TRUE    = 1,

    HISI_BUTT
} hisi_bool_enum;

typedef enum {
    HISI_IFTYPE_UNSPECIFIED,
    HISI_IFTYPE_ADHOC,
    HISI_IFTYPE_STATION,
    HISI_IFTYPE_AP,
    HISI_IFTYPE_AP_VLAN,
    HISI_IFTYPE_WDS,
    HISI_IFTYPE_MONITOR,
    HISI_IFTYPE_MESH_POINT,
    HISI_IFTYPE_P2P_CLIENT,
    HISI_IFTYPE_P2P_GO,
    HISI_IFTYPE_P2P_DEVICE,

    /* keep last */
    NUM_HISI_IFTYPES,
    HISI_IFTYPE_MAX = NUM_HISI_IFTYPES - 1
} hisi_iftype_enum;

typedef enum {
    HISI_KEYTYPE_GROUP,
    HISI_KEYTYPE_PAIRWISE,
    HISI_KEYTYPE_PEERKEY,

    NUM_HISI_KEYTYPES
} hisi_key_type_enum;

typedef enum {
    __HISI_KEY_DEFAULT_TYPE_INVALID,
    HISI_KEY_DEFAULT_TYPE_UNICAST,
    HISI_KEY_DEFAULT_TYPE_MULTICAST,

    NUM_HISI_KEY_DEFAULT_TYPES
} hisi_key_default_types_enum;

typedef enum {
    HISI_NO_SSID_HIDING,
    HISI_HIDDEN_SSID_ZERO_LEN,
    HISI_HIDDEN_SSID_ZERO_CONTENTS
} hisi_hidden_ssid_enum;

typedef enum {
    HISI_IOCTL_SET_AP = 0,
    HISI_IOCTL_NEW_KEY,
    HISI_IOCTL_DEL_KEY,
    HISI_IOCTL_SET_KEY,
    HISI_IOCTL_SEND_MLME,
    HISI_IOCTL_SEND_EAPOL,
    HISI_IOCTL_RECEIVE_EAPOL,
    HISI_IOCTL_ENALBE_EAPOL,
    HISI_IOCTL_DISABLE_EAPOL,
    HIIS_IOCTL_GET_ADDR,
    HISI_IOCTL_SET_POWER,
    HISI_IOCTL_SET_MODE,
    HIIS_IOCTL_GET_HW_FEATURE,
    HISI_IOCTL_STOP_AP,
    HISI_IOCTL_DEL_VIRTUAL_INTF,
    HISI_IOCTL_SCAN,
    HISI_IOCTL_DISCONNET,
    HISI_IOCTL_ASSOC,
    HISI_IOCTL_SET_NETDEV,
#ifdef _PRE_WLAN_CONFIG_WPS
    HISI_IOCTL_SET_AP_WPS_P2P_IE,
#endif
    HISI_IOCTL_CHANGE_BEACON,
    HISI_IOCTL_DHCP_START,
    HISI_IOCTL_DHCP_STOP,
    HISI_IOCTL_DHCP_SUCC_CHECK,
    HISI_IOCTL_SET_PM_ON,
    HISI_IOCTL_IP_NOTIFY_DRIVER,
    HISI_IOCTL_SET_MAX_STA,
    HISI_IOCTL_STA_REMOVE,
    HWAL_EVENT_BUTT
} hisi_event_enum;

typedef enum {
    HISI_ELOOP_EVENT_NEW_STA = 0,
    HISI_ELOOP_EVENT_DEL_STA,
    HISI_ELOOP_EVENT_RX_MGMT,
    HISI_ELOOP_EVENT_TX_STATUS,
    HISI_ELOOP_EVENT_SCAN_DONE,
    HISI_ELOOP_EVENT_SCAN_RESULT,
    HISI_ELOOP_EVENT_CONNECT_RESULT,
    HISI_ELOOP_EVENT_DISCONNECT,
    HISI_ELOOP_EVENT_DHCP_SUCC,
    HISI_ELOOP_EVENT_RX_DFX,

    HISI_ELOOP_EVENT_BUTT
} hisi_eloop_event_enum;

typedef enum {
    HISI_MFP_NO,
    HISI_MFP_REQUIRED,
} hisi_mfp_enum;

typedef enum {
    HISI_AUTHTYPE_OPEN_SYSTEM = 0,
    HISI_AUTHTYPE_SHARED_KEY,
    HISI_AUTHTYPE_FT,
    HISI_AUTHTYPE_NETWORK_EAP,
    HISI_AUTHTYPE_SAE,

    /* keep last */
    __HISI_AUTHTYPE_NUM,
    HISI_AUTHTYPE_MAX = __HISI_AUTHTYPE_NUM - 1,
    HISI_AUTHTYPE_AUTOMATIC,

    HISI_AUTHTYPE_BUTT
} hisi_auth_type_enum;

typedef struct _hisi_ioctl_command_stru {
    unsigned int cmd;
    void *buf;
} hisi_ioctl_command_stru;
typedef signed int (*hisi_send_event_cb)(signed int, unsigned char *, unsigned int);

// 8 STRUCT定义
typedef struct _hisi_new_sta_info_stru {
    int32_t reassoc;
    size_t ie_len;
    uint8_t *ie;
    uint8_t macaddr[ETH_ADDR_LEN];
} hisi_new_sta_info_stru;

typedef struct _hisi_rx_mgmt_stru {
    uint8_t *buf;
    uint32_t len;
    int32_t sig_mbm;
} hisi_rx_mgmt_stru;

typedef struct _hisi_tx_status_stru {
    uint8_t *buf;
    uint32_t len;
    uint8_t ack;
} hisi_tx_status_stru;

typedef struct _hisi_mlme_data_stru {
    int32_t noack;
    uint32_t freq;
    size_t data_len;
    uint8_t *data;
    uint64_t *send_action_cookie;
} hisi_mlme_data_stru;

typedef struct _hisi_beacon_data_stru {
    size_t head_len;
    size_t tail_len;
    uint8_t *head;
    uint8_t *tail;
} hisi_beacon_data_stru;

typedef struct _hisi_freq_params_stru {
    int32_t mode;
    int32_t freq;
    int32_t channel;

    /* for HT */
    int32_t ht_enabled;

    /* 0 = HT40 disabled, -1 = HT40 enabled,
 * secondary channel below primary, 1 = HT40
 * enabled, secondary channel above primary */
    int32_t sec_channel_offset;

    /* for VHT */
    int32_t vht_enabled;

    /* valid for both HT and VHT, center_freq2 is non-zero
 * only for bandwidth 80 and an 80+80 channel */
    int32_t center_freq1;
    int32_t center_freq2;
    int32_t bandwidth;
} hisi_freq_params_stru;

/* 密钥信息设置数据传递结构体 */
typedef struct _hisi_key_ext_stru {
    int32_t type;
    uint32_t key_idx;
    uint32_t key_len;
    uint32_t seq_len;
    uint32_t set_tx;
    uint32_t cipher;
    uint8_t *addr;
    uint8_t *key;
    uint8_t *seq;
    uint8_t def;
    uint8_t defmgmt;
    uint8_t nlmode;
    uint8_t default_types;
} hisi_key_ext_stru;

/* AP信息设置相关数据传递结构体 */
typedef struct _hisi_ap_settings_stru {
    hisi_freq_params_stru freq_params;
    hisi_beacon_data_stru beacon_data;
    size_t ssid_len;
    int32_t beacon_interval;
    int32_t dtim_period;
    uint8_t *ssid;
    uint8_t hidden_ssid;
    uint8_t auth_type;
} hisi_ap_settings_stru;

/* 设置模式结构体 */
typedef struct _hisi_set_mode_stru {
    uint8_t bssid[ETH_ADDR_LEN];
    uint8_t iftype;
} hisi_set_mode_stru;

typedef struct _hisi_tx_eapol_stru {
    uint8_t src[ETH_ADDR_LEN];
    uint8_t dst[ETH_ADDR_LEN];
    uint8_t *buf;
    uint32_t len;
} hisi_tx_eapol_stru;

typedef struct _hisi_rx_eapol_stru {
    uint8_t *buf;
    uint32_t len;
} hisi_rx_eapol_stru;

typedef struct _hisi_enable_eapol_stru {
    void *callback;
    void *contex;
} hisi_enable_eapol_stru;

typedef struct _hisi_ieee80211_channel_stru {
    uint16_t channel;
    uint32_t freq;
    uint32_t flags;
} hisi_ieee80211_channel_stru;

typedef struct _hisi_hw_feature_data_stru {
    int32_t channel_num;
    uint16_t aus_bitrate[HISI_BITRATE_MAX_NUM];
    uint16_t ht_capab;
    hisi_ieee80211_channel_stru ast_iee80211_channel[HISI_CHANNEL_MAX_NUM];
} hisi_hw_feature_data_stru;

typedef struct _hisi_driver_scan_ssid_stru {
    uint8_t ssid[MAX_SSID_LEN];
    size_t ssid_len;
} hisi_driver_scan_ssid_stru;

typedef struct _hisi_scan_stru {
    hisi_driver_scan_ssid_stru ssids[HISI_WPAS_MAX_SCAN_SSIDS];
    int32_t *freqs;
    uint8_t *extra_ies;
    size_t num_ssids;
    size_t num_freqs;
    size_t extra_ies_len;
} hisi_scan_stru;

typedef struct  _hisi_crypto_settings_stru {
    uint32_t wpa_versions;
    uint32_t cipher_group;
    int32_t n_ciphers_pairwise;
    uint32_t ciphers_pairwise_arr[HISI_MAX_NR_CIPHER_SUITES];
    int32_t n_akm_suites;
    uint32_t akm_suites[HISI_MAX_NR_AKM_SUITES];
} hisi_crypto_settings_stru;

typedef struct _hisi_associate_params_stru {
    uint8_t *bssid;
    uint8_t *ssid;
    uint8_t *ie;
    uint8_t *key;
    uint8_t auth_type;
    uint8_t privacy;
    uint8_t key_len;
    uint8_t key_idx;
    uint8_t mfp;
    uint32_t freq;
    uint32_t ssid_len;
    uint32_t ie_len;
    hisi_crypto_settings_stru crypto;
} hisi_associate_params_stru;

typedef struct _hisi_connect_result_stru {
    uint8_t *req_ie;
    size_t req_ie_len;
    uint8_t *resp_ie;
    size_t resp_ie_len;
    uint8_t bssid[ETH_ADDR_LEN];
    uint16_t status;
    uint8_t rsv[2];
} hisi_connect_result_stru;

typedef struct _hisi_scan_result_stru {
    int32_t flags;
    uint8_t bssid[ETH_ADDR_LEN];
    uint8_t rsv[2];
    int32_t freq;
    int16_t beacon_int;
    int16_t caps;
    int32_t qual;
    int32_t noise;
    int32_t level;
    uint64_t tsf;
    uint32_t age;
    uint32_t ie_len;
    uint32_t beacon_ie_len;
    uint8_t *variable;
} hisi_scan_result_stru;

typedef struct _hisi_disconnect_stru {
    uint8_t *ie;
    uint16_t reason;
    uint8_t rsv[2];
    uint32_t ie_len;
} hisi_disconnect_stru;

#ifdef _PRE_WLAN_CONFIG_WPS
typedef struct _hisi_app_ie_stru {
    uint32_t ie_len;
    uint8_t app_ie_type;
    uint8_t rsv[3];
    /* ie 中保存信息元素 */
    uint8_t *ie;
} hisi_app_ie_stru;
#endif

// 11 函数声明
extern void hisi_register_send_event_cb(hisi_send_event_cb func);
extern int32_t hisi_hwal_wpa_ioctl(int8_t *ifname, hisi_ioctl_command_stru *cmd);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of driver_hisi_common.h */
