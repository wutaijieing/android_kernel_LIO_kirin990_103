

#ifndef __MAC_CB_TX_H__
#define __MAC_CB_TX_H__

/* 1 其他头文件包含 */
#include "oal_types.h"
#include "wlan_types.h"
#include "mac_frame_common.h"

// 此处不加extern "C" UT编译不过
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/* 专用于CB字段自定义帧类型、帧子类型枚举值 */
typedef enum {
    WLAN_CB_FRAME_TYPE_START = 0,  /* cb默认初始化为0 */
    WLAN_CB_FRAME_TYPE_ACTION = 1, /* action帧 */
    WLAN_CB_FRAME_TYPE_DATA = 2,   /* 数据帧 */
    WLAN_CB_FRAME_TYPE_MGMT = 3,   /* 管理帧，用于p2p等需要上报host */

    WLAN_CB_FRAME_TYPE_BUTT
} wlan_cb_frame_type_enum;
typedef uint8_t wlan_cb_frame_type_enum_uint8;

/* cb字段action帧子类型枚举定义 */
typedef enum {
    WLAN_ACTION_BA_ADDBA_REQ = 0, /* 聚合action */
    WLAN_ACTION_BA_ADDBA_RSP = 1,
    WLAN_ACTION_BA_DELBA = 2,
#ifdef _PRE_WLAN_FEATURE_WMMAC
    WLAN_ACTION_BA_WMMAC_ADDTS_REQ = 3,
    WLAN_ACTION_BA_WMMAC_ADDTS_RSP = 4,
    WLAN_ACTION_BA_WMMAC_DELTS = 5,
#endif
    WLAN_ACTION_SMPS_FRAME_SUBTYPE = 6,   /* SMPS节能action */
    WLAN_ACTION_OPMODE_FRAME_SUBTYPE = 7, /* 工作模式通知action */
    WLAN_ACTION_P2PGO_FRAME_SUBTYPE = 8,  /* host发送的P2P go帧，主要是discoverability request */

#ifdef _PRE_WLAN_FEATURE_TWT
    WLAN_ACTION_TWT_SETUP_REQ = 9,
    WLAN_ACTION_TWT_TEARDOWN_REQ = 10,
#endif
#ifdef _PRE_WLAN_FEATURE_HIEX
    WLAN_ACTION_HIEX = 11,
#endif
#ifdef _PRE_WLAN_FEATURE_NAN
    WLAN_ACTION_NAN_PUBLISH = 12,
    WLAN_ACTION_NAN_FLLOWUP = 13,
    WLAN_ACTION_NAN_SUBSCRIBE = 14,
#endif
#ifdef _PRE_WLAN_FEATURE_FTM
    WLAN_ACTION_FTM_REQUEST = 15,
    WLAN_ACTION_FTM_RESPONE = 16,
#endif
    WLAN_FRAME_TYPE_ACTION_BUTT
} wlan_cb_action_subtype_enum;
typedef uint8_t wlan_cb_frame_subtype_enum_uint8;

/* 1字节对齐 */
#pragma pack(push, 1)
typedef struct {
    mac_ieee80211_frame_stru *pst_frame_header; /* 该MPDU的帧头指针 */
    uint16_t us_seqnum;                       /* 记录软件分配的seqnum */
    wlan_frame_type_enum_uint8 en_frame_type;   /* 该帧是控制针、管理帧、数据帧 */
    uint8_t bit_80211_mac_head_type : 1;      /* 0: 802.11 mac头不在skb中,另外申请了内存存放;1:802.11 mac头在skb中 */
    uint8_t en_res : 7;                       /* 是否使用4地址，由WDS特性决定 */
} mac_tx_expand_cb_stru;

/* 裸系统cb字段 只有20字节可用, 当前使用19字节; HCC[5]PAD[1]FRW[3]+CB[19]+MAC HEAD[36] */
struct mac_tx_ctl {
    /* byte1 */
    /* 取值:FRW_EVENT_TYPE_WLAN_DTX和FRW_EVENT_TYPE_HOST_DRX，作用:在释放时区分是内存池的netbuf还是原生态的 */
    uint8_t bit_event_type : 5;
    uint8_t bit_event_sub_type : 3;
    /* byte2-3 */
    wlan_cb_frame_type_enum_uint8 uc_frame_type;       /* 自定义帧类型 */
    wlan_cb_frame_subtype_enum_uint8 uc_frame_subtype; /* 自定义帧子类型 */
    /* byte4 */
    uint8_t bit_mpdu_num : 7;   /* ampdu中包含的MPDU个数,实际描述符填写的值为此值-1 */
    uint8_t bit_netbuf_num : 1; /* 每个MPDU占用的netbuf数目 */
    /* 在每个MPDU的第一个NETBUF中有效 */
    /* byte5-6 */
    uint16_t us_mpdu_payload_len; /* 每个MPDU的长度不包括mac header length */
    /* byte7 */
    uint8_t bit_frame_header_length : 6; /* 51四地址32 */ /* 该MPDU的802.11头长度 */
    uint8_t bit_is_amsdu : 1;                             /* 是否AMSDU: OAL_FALSE不是，OAL_TRUE是 */
    uint8_t bit_is_first_msdu : 1;                        /* 是否是第一个子帧，OAL_FALSE不是 OAL_TRUE是 */
    /* byte8 */
    uint8_t bit_tid : 4;                  /* dmac tx 到 tx complete 传递的user结构体，目标用户地址 */
    wlan_wme_ac_type_enum_uint8 bit_ac : 3; /* ac */
    uint8_t bit_ismcast : 1;              /* 该MPDU是单播还是多播:OAL_FALSE单播，OAL_TRUE多播 */
    /* byte9 */
    uint8_t bit_retried_num : 4;   /* 重传次数 */
    uint8_t bit_mgmt_frame_id : 4; /* wpas 发送管理帧的frame id */
    /* byte10 */
    uint8_t bit_tx_user_idx : 6;          /* 比描述符中userindex多一个bit用于标识无效index */
    uint8_t bit_roam_data : 1;            /* 漫游期间帧发送标记 */
    uint8_t bit_is_get_from_ps_queue : 1; /* 节能特性用，标识一个MPDU是否从节能队列中取出来的 */
    /* byte11 */
    uint8_t bit_tx_vap_index : 3;
    wlan_tx_ack_policy_enum_uint8 en_ack_policy : 3;
    uint8_t bit_is_needretry : 1;
    uint8_t bit_need_rsp : 1; /* WPAS send mgmt,need dmac response tx status */
    /* byte12 */
    uint8_t en_is_probe_data : 3; /* 是否探测帧 */
    uint8_t bit_is_eapol_key_ptk : 1;                  /* 4 次握手过程中设置单播密钥EAPOL KEY 帧标识 */
    uint8_t bit_is_m2u_data : 1;                       /* 是否是组播转单播的数据 */
    uint8_t bit_is_tcp_ack : 1;                        /* 用于标记tcp ack */
    uint8_t bit_is_rtsp : 1;
    uint8_t en_use_4_addr : 1; /* 是否使用4地址，由WDS特性决定 */
    /* byte13-16 */
    uint16_t us_timestamp_ms;   /* 维测使用入TID队列的时间戳, 单位1ms精度 */
    uint8_t bit_is_qosnull : 1; /* 用于标记qos null帧 */
    uint8_t bit_is_himit : 1; /* 用于标记himit帧 */
    uint8_t bit_hiex_traced : 1; /* 用于标记hiex帧 */
    uint8_t bit_is_game_marked : 1; /* 用于标记游戏帧 */
    uint8_t bit_is_hid2d_pkt : 1; /* 用于标记hid2d投屏数据帧 */
    uint8_t bit_is_802_3_snap : 1; /* 是否是802.3 snap */
    uint8_t uc_reserved1 : 2;
    uint8_t uc_data_type : 4; /* 数据帧类型, ring tx使用, 对应data_type_enum */
    uint8_t csum_type : 3;
    uint8_t bit_is_pt_mcast : 1;  /* 标识帧是否为组播图传picture transmission协议类型 */
    /* byte17-18 */
    uint8_t uc_alg_pktno;     /* 算法用到的字段，唯一标示该报文 */
    uint8_t uc_alg_frame_tag : 2; /* 用于算法对帧进行标记 */
    uint8_t uc_hid2d_tx_delay_time : 6; /* 用于保存hid2d数据帧在dmac的允许传输时间 */
    /* byte19 */
    uint8_t bit_large_amsdu_level : 2;  /* offload下大包AMSDU聚合度 */
    uint8_t bit_align_padding : 2;      /* SKB 头部包含ETHER HEADER时,4字节对齐需要的padding */
    uint8_t bit_msdu_num_in_amsdu : 2;  /* 大包聚合时每个AMSDU子帧数 */
    uint8_t bit_is_large_skb_amsdu : 1; /* 是否是多子帧大包聚合 */
    uint8_t bit_htc_flag : 1;           /* 用于标记htc */

#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
    /* OFFLOAD架构下，HOST相对DEVICE的CB增量 */
    mac_tx_expand_cb_stru st_expand_cb;
#endif
} __OAL_DECLARE_PACKED;
typedef struct mac_tx_ctl mac_tx_ctl_stru;
#pragma pack(pop)

/* 不区分offload架构的CB字段 */
#define MAC_GET_CB_IS_4ADDRESS(_pst_tx_ctrl)        ((_pst_tx_ctrl)->en_use_4_addr)
/* 标记是否是小包AMSDU帧 */
#define MAC_GET_CB_IS_AMSDU(_pst_tx_ctrl)           ((_pst_tx_ctrl)->bit_is_amsdu)
/* 自适应算法决定的聚合度 */
#define MAC_GET_CB_AMSDU_LEVEL(_pst_tx_ctrl)        ((_pst_tx_ctrl)->bit_large_amsdu_level)
/* MSDU帧头部对齐字节 */
#define MAC_GET_CB_ETHER_HEAD_PADDING(_pst_tx_ctrl) ((_pst_tx_ctrl)->bit_align_padding)
/* 实际组成的大包AMSDU帧包含的子帧数 */
#define MAC_GET_CB_HAS_MSDU_NUMBER(_pst_tx_ctrl)    ((_pst_tx_ctrl)->bit_msdu_num_in_amsdu)
/* 标记是否组成了大包AMSDU帧 */
#define MAC_GET_CB_IS_LARGE_SKB_AMSDU(_pst_tx_ctrl) ((_pst_tx_ctrl)->bit_is_large_skb_amsdu)
#define MAC_GET_CB_IS_FIRST_MSDU(_pst_tx_ctrl)      ((_pst_tx_ctrl)->bit_is_first_msdu)
#define MAC_GET_CB_IS_NEED_RESP(_pst_tx_ctrl)       ((_pst_tx_ctrl)->bit_need_rsp)
#define MAC_GET_CB_IS_EAPOL_KEY_PTK(_pst_tx_ctrl)   ((_pst_tx_ctrl)->bit_is_eapol_key_ptk)
#define MAC_GET_CB_IS_ROAM_DATA(_pst_tx_ctrl)       ((_pst_tx_ctrl)->bit_roam_data)
#define MAC_GET_CB_IS_FROM_PS_QUEUE(_pst_tx_ctrl)   ((_pst_tx_ctrl)->bit_is_get_from_ps_queue)
#define MAC_GET_CB_IS_MCAST(_pst_tx_ctrl)           ((_pst_tx_ctrl)->bit_ismcast)
#define MAC_GET_CB_IS_PT_MCAST(_pst_tx_ctrl)        ((_pst_tx_ctrl)->bit_is_pt_mcast)
#define MAC_GET_CB_IS_NEEDRETRY(_pst_tx_ctrl)       ((_pst_tx_ctrl)->bit_is_needretry)
#define MAC_GET_CB_IS_PROBE_DATA(_pst_tx_ctrl)      ((_pst_tx_ctrl)->en_is_probe_data)
#define MAC_GET_CB_IS_RTSP(_pst_tx_ctrl)            ((_pst_tx_ctrl)->bit_is_rtsp)
#define MAC_GET_CB_ALG_TAGS(_pst_tx_ctrl)           ((_pst_tx_ctrl)->uc_alg_frame_tag)

#define MAC_GET_CB_MGMT_FRAME_ID(_pst_tx_ctrl) ((_pst_tx_ctrl)->bit_mgmt_frame_id)
#define MAC_GET_CB_MPDU_LEN(_pst_tx_ctrl)      ((_pst_tx_ctrl)->us_mpdu_payload_len)
#define MAC_GET_CB_FRAME_TYPE(_pst_tx_ctrl)    ((_pst_tx_ctrl)->uc_frame_type)
#define MAC_GET_CB_FRAME_SUBTYPE(_pst_tx_ctrl) ((_pst_tx_ctrl)->uc_frame_subtype)
#define MAC_GET_CB_DATA_TYPE(_pst_tx_ctrl)     ((_pst_tx_ctrl)->uc_data_type)
#define MAC_GET_CB_IS_802_3_SNAP(_pst_tx_ctrl)    ((_pst_tx_ctrl)->bit_is_802_3_snap)
#define MAC_GET_CB_TIMESTAMP(_pst_tx_ctrl)     ((_pst_tx_ctrl)->us_timestamp_ms)
#define MAC_GET_CB_IS_QOSNULL(_pst_tx_ctrl)    ((_pst_tx_ctrl)->bit_is_qosnull)
#ifdef _PRE_WLAN_FEATURE_HIEX
#define MAC_GET_CB_HIEX_TRACED(_pst_tx_ctrl) ((_pst_tx_ctrl)->bit_hiex_traced)
#define MAC_GET_RX_CB_HIEX_TRACED(_pst_rx_ctl) ((_pst_rx_ctl)->bit_hiex_traced)
#endif
/* VIP数据帧 */
#define MAC_GET_CB_IS_EAPOL(ptx)                           \
    ((MAC_GET_CB_FRAME_TYPE(ptx) == WLAN_CB_FRAME_TYPE_DATA) && (MAC_GET_CB_FRAME_SUBTYPE(ptx) == MAC_DATA_EAPOL))
#define MAC_GET_CB_IS_ARP(ptx)                             \
    ((MAC_GET_CB_FRAME_TYPE(ptx) == WLAN_CB_FRAME_TYPE_DATA) && \
     ((MAC_GET_CB_FRAME_SUBTYPE(ptx) == MAC_DATA_ARP_REQ) || (MAC_GET_CB_FRAME_SUBTYPE(ptx) == MAC_DATA_ARP_RSP)))
#define MAC_GET_CB_IS_WAI(ptx)                             \
    ((MAC_GET_CB_FRAME_TYPE(ptx) == WLAN_CB_FRAME_TYPE_DATA) && (MAC_GET_CB_FRAME_SUBTYPE(ptx) == MAC_DATA_WAPI))
#define MAC_GET_CB_IS_VIPFRAME(ptx)                             \
    ((MAC_GET_CB_FRAME_TYPE(ptx) == WLAN_CB_FRAME_TYPE_DATA) && (MAC_GET_CB_FRAME_SUBTYPE(ptx) <= MAC_DATA_VIP_FRAME))

#define MAC_GET_CB_IS_SMPS_FRAME(_pst_tx_ctrl)                             \
    ((WLAN_CB_FRAME_TYPE_ACTION == MAC_GET_CB_FRAME_TYPE(_pst_tx_ctrl)) && \
        (WLAN_ACTION_SMPS_FRAME_SUBTYPE == MAC_GET_CB_FRAME_SUBTYPE(_pst_tx_ctrl)))
#define MAC_GET_CB_IS_OPMODE_FRAME(_pst_tx_ctrl)                           \
    ((WLAN_CB_FRAME_TYPE_ACTION == MAC_GET_CB_FRAME_TYPE(_pst_tx_ctrl)) && \
        (WLAN_ACTION_OPMODE_FRAME_SUBTYPE == MAC_GET_CB_FRAME_SUBTYPE(_pst_tx_ctrl)))
#define MAC_GET_CB_IS_P2PGO_FRAME(_pst_tx_ctrl)                          \
    ((WLAN_CB_FRAME_TYPE_MGMT == MAC_GET_CB_FRAME_TYPE(_pst_tx_ctrl)) && \
        (WLAN_ACTION_P2PGO_FRAME_SUBTYPE == MAC_GET_CB_FRAME_SUBTYPE(_pst_tx_ctrl)))
#ifdef _PRE_WLAN_FEATURE_TWT
#define MAC_GET_CB_IS_TWT_SETUP_FRAME(_pst_tx_ctrl)                        \
    ((WLAN_CB_FRAME_TYPE_ACTION == MAC_GET_CB_FRAME_TYPE(_pst_tx_ctrl)) && \
        (WLAN_ACTION_TWT_SETUP_REQ == MAC_GET_CB_FRAME_SUBTYPE(_pst_tx_ctrl)))
#endif
#define mac_dbdc_send_m2s_action(mac_device, tx_ctl) \
    (mac_is_dbdc_running((mac_device)) && (MAC_GET_CB_IS_SMPS_FRAME((tx_ctl)) || MAC_GET_CB_IS_OPMODE_FRAME((tx_ctl))))

/* 模块发送流程控制信息结构体的信息元素获取 */
#define MAC_GET_CB_MPDU_NUM(_pst_tx_ctrl)            ((_pst_tx_ctrl)->bit_mpdu_num)
#define MAC_GET_CB_NETBUF_NUM(_pst_tx_ctrl)          ((_pst_tx_ctrl)->bit_netbuf_num)
#define MAC_GET_CB_FRAME_HEADER_LENGTH(_pst_tx_ctrl) ((_pst_tx_ctrl)->bit_frame_header_length)
#define MAC_GET_CB_ACK_POLACY(_pst_tx_ctrl)          ((_pst_tx_ctrl)->en_ack_policy)
#define MAC_GET_CB_TX_VAP_INDEX(_pst_tx_ctrl)        ((_pst_tx_ctrl)->bit_tx_vap_index)
#define MAC_GET_CB_TX_USER_IDX(_pst_tx_ctrl)         ((_pst_tx_ctrl)->bit_tx_user_idx)
#define MAC_GET_CB_WME_AC_TYPE(_pst_tx_ctrl)         ((_pst_tx_ctrl)->bit_ac)
#define MAC_GET_CB_WME_TID_TYPE(_pst_tx_ctrl)        ((_pst_tx_ctrl)->bit_tid)
#define MAC_GET_CB_EVENT_TYPE(_pst_tx_ctrl)          ((_pst_tx_ctrl)->bit_event_type)
#define MAC_GET_CB_EVENT_SUBTYPE(_pst_tx_ctrl)       ((_pst_tx_ctrl)->bit_event_sub_type)
#define MAC_GET_CB_RETRIED_NUM(_pst_tx_ctrl)         ((_pst_tx_ctrl)->bit_retried_num)
#define MAC_GET_CB_ALG_PKTNO(_pst_tx_ctrl)           ((_pst_tx_ctrl)->uc_alg_pktno)
#define MAC_GET_CB_TCP_ACK(_pst_tx_ctrl)             ((_pst_tx_ctrl)->bit_is_tcp_ack)
#define MAC_GET_CB_HTC_FLAG(_pst_tx_ctrl)            ((_pst_tx_ctrl)->bit_htc_flag)
#ifdef _PRE_WLAN_FEATURE_HIEX
#define MAC_GET_CB_IS_HIMIT_FRAME(_pst_tx_ctrl)      ((_pst_tx_ctrl)->bit_is_himit)
#define MAC_GET_CB_IS_GAME_MARKED_FRAME(_pst_tx_ctrl)((_pst_tx_ctrl)->bit_is_game_marked)
#endif

#define MAC_SET_CB_IS_QOS_DATA(_pst_tx_ctrl, _flag)
#define MAC_GET_CB_IS_QOS_DATA(_pst_tx_ctrl) \
    ((MAC_GET_CB_WLAN_FRAME_TYPE(_pst_tx_ctrl) == WLAN_DATA_BASICTYPE) &&  \
     ((MAC_GET_CB_WLAN_FRAME_SUBTYPE(_pst_tx_ctrl) == WLAN_QOS_DATA) || \
      (MAC_GET_CB_WLAN_FRAME_SUBTYPE(_pst_tx_ctrl) == WLAN_QOS_NULL_FRAME)))

#define MAC_GET_CB_IS_DATA_FRAME(_pst_tx_ctrl)                                \
    (MAC_GET_CB_IS_QOS_DATA(_pst_tx_ctrl) || \
     ((MAC_GET_CB_WLAN_FRAME_TYPE(_pst_tx_ctrl) == WLAN_DATA_BASICTYPE) &&     \
      (MAC_GET_CB_WLAN_FRAME_SUBTYPE(_pst_tx_ctrl) == WLAN_DATA)))

#ifdef _PRE_WLAN_FEATURE_TXBF_HW
#define MAC_FRAME_TYPE_IS_VHT_NDPA(_uc_type, _uc_sub_type) \
        ((WLAN_CONTROL == (_uc_type)) && (WLAN_VHT_NDPA == (_uc_sub_type)))
#endif

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
#ifdef _PRE_WLAN_FEATURE_MAC_HEADER_IN_PAYLOAD
#define MAC_CB_FRM_OFFSET (OAL_MAX_CB_LEN + MAX_MAC_HEAD_LEN)
#else
#define MAC_CB_FRM_OFFSET OAL_MAX_CB_LEN
#endif

#define MAC_GET_CB_SEQ_NUM(_pst_tx_ctrl)             \
    (((mac_ieee80211_frame_stru *)((uint8_t *)_pst_tx_ctrl + OAL_MAX_CB_LEN))->bit_seq_num)
#define MAC_SET_CB_80211_MAC_HEAD_TYPE(_pst_tx_ctrl, _flag)
#define MAC_GET_CB_80211_MAC_HEAD_TYPE(_pst_tx_ctrl) 1 /* offload架构,MAC HEAD由netbuf index管理,不需要释放 */

#define MAC_GET_CB_WLAN_FRAME_TYPE(_pst_tx_ctrl) \
    (((mac_ieee80211_frame_stru *)((uint8_t *)(_pst_tx_ctrl) + MAC_CB_FRM_OFFSET))->st_frame_control.bit_type)
#define MAC_GET_CB_WLAN_FRAME_SUBTYPE(_pst_tx_ctrl) \
    (((mac_ieee80211_frame_stru *)((uint8_t *)(_pst_tx_ctrl) + MAC_CB_FRM_OFFSET))->st_frame_control.bit_sub_type)

#define MAC_SET_CB_FRAME_HEADER_ADDR(_pst_tx_ctrl, _addr)
#define MAC_GET_CB_FRAME_HEADER_ADDR(_pst_tx_ctrl) \
    ((mac_ieee80211_frame_stru *)((uint8_t *)_pst_tx_ctrl + OAL_MAX_CB_LEN))

#define MAC_SET_CB_IS_BAR(_pst_tx_ctrl, _flag)
#define MAC_GET_CB_IS_BAR(_pst_tx_ctrl) ((WLAN_CONTROL == MAC_GET_CB_WLAN_FRAME_TYPE(_pst_tx_ctrl)) && \
        (WLAN_BLOCKACK_REQ == MAC_GET_CB_WLAN_FRAME_SUBTYPE(_pst_tx_ctrl)))

#else
#define MAC_GET_RX_CB_DA_USER_IDX(_pst_rx_ctl)              ((_pst_rx_ctl)->st_expand_cb.us_da_user_idx)
#define MAC_GET_CB_WLAN_FRAME_TYPE(_pst_tx_ctrl)            ((_pst_tx_ctrl)->st_expand_cb.en_frame_type)
#define MAC_GET_CB_WLAN_FRAME_SUBTYPE(_pst_tx_ctrl)         \
    (((_pst_tx_ctrl)->st_expand_cb.pst_frame_header)->st_frame_control.bit_sub_type)
#define MAC_GET_CB_SEQ_NUM(_pst_tx_ctrl)                    ((_pst_tx_ctrl)->st_expand_cb.us_seqnum)
#define MAC_SET_CB_FRAME_HEADER_ADDR(_pst_tx_ctrl, _addr)   ((_pst_tx_ctrl)->st_expand_cb.pst_frame_header = (_addr))
#define MAC_GET_CB_FRAME_HEADER_ADDR(_pst_tx_ctrl)          ((_pst_tx_ctrl)->st_expand_cb.pst_frame_header)
#define MAC_SET_CB_80211_MAC_HEAD_TYPE(_pst_tx_ctrl, _flag) \
    ((_pst_tx_ctrl)->st_expand_cb.bit_80211_mac_head_type = (_flag))
#define MAC_GET_CB_80211_MAC_HEAD_TYPE(_pst_tx_ctrl)        ((_pst_tx_ctrl)->st_expand_cb.bit_80211_mac_head_type)
#endif  // #if defined(_PRE_PRODUCT_ID_HI110X_DEV)

#define ALG_SCHED_PARA(tx_ctl) \
    ((MAC_GET_CB_IS_DATA_FRAME(tx_ctl)) && (!MAC_GET_CB_IS_VIPFRAME(tx_ctl)) && (!MAC_GET_CB_IS_MCAST(tx_ctl)))


static inline uint8_t *mac_netbuf_get_payload(oal_netbuf_stru *netbuf)
{
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
#ifdef _PRE_WLAN_FEATURE_MAC_HEADER_IN_PAYLOAD
    return get_netbuf_payload(netbuf) + MAC_80211_FRAME_LEN;
#else
    return get_netbuf_payload(netbuf);
#endif
#else
    /* 注意! 所以如果mac header长度不是24字节的不要使用该函数 */
    return get_netbuf_payload(netbuf) + MAC_80211_FRAME_LEN;
#endif
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of mac_cb.h */
