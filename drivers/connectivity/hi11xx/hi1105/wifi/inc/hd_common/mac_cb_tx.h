

#ifndef __MAC_CB_TX_H__
#define __MAC_CB_TX_H__

/* 1 ����ͷ�ļ����� */
#include "oal_types.h"
#include "wlan_types.h"
#include "mac_frame_common.h"

// �˴�����extern "C" UT���벻��
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/* ר����CB�ֶ��Զ���֡���͡�֡������ö��ֵ */
typedef enum {
    WLAN_CB_FRAME_TYPE_START = 0,  /* cbĬ�ϳ�ʼ��Ϊ0 */
    WLAN_CB_FRAME_TYPE_ACTION = 1, /* action֡ */
    WLAN_CB_FRAME_TYPE_DATA = 2,   /* ����֡ */
    WLAN_CB_FRAME_TYPE_MGMT = 3,   /* ����֡������p2p����Ҫ�ϱ�host */

    WLAN_CB_FRAME_TYPE_BUTT
} wlan_cb_frame_type_enum;
typedef uint8_t wlan_cb_frame_type_enum_uint8;

/* cb�ֶ�action֡������ö�ٶ��� */
typedef enum {
    WLAN_ACTION_BA_ADDBA_REQ = 0, /* �ۺ�action */
    WLAN_ACTION_BA_ADDBA_RSP = 1,
    WLAN_ACTION_BA_DELBA = 2,
#ifdef _PRE_WLAN_FEATURE_WMMAC
    WLAN_ACTION_BA_WMMAC_ADDTS_REQ = 3,
    WLAN_ACTION_BA_WMMAC_ADDTS_RSP = 4,
    WLAN_ACTION_BA_WMMAC_DELTS = 5,
#endif
    WLAN_ACTION_SMPS_FRAME_SUBTYPE = 6,   /* SMPS����action */
    WLAN_ACTION_OPMODE_FRAME_SUBTYPE = 7, /* ����ģʽ֪ͨaction */
    WLAN_ACTION_P2PGO_FRAME_SUBTYPE = 8,  /* host���͵�P2P go֡����Ҫ��discoverability request */

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

/* 1�ֽڶ��� */
#pragma pack(push, 1)
typedef struct {
    mac_ieee80211_frame_stru *pst_frame_header; /* ��MPDU��֡ͷָ�� */
    uint16_t us_seqnum;                       /* ��¼��������seqnum */
    wlan_frame_type_enum_uint8 en_frame_type;   /* ��֡�ǿ����롢����֡������֡ */
    uint8_t bit_80211_mac_head_type : 1;      /* 0: 802.11 macͷ����skb��,�����������ڴ���;1:802.11 macͷ��skb�� */
    uint8_t en_res : 7;                       /* �Ƿ�ʹ��4��ַ����WDS���Ծ��� */
} mac_tx_expand_cb_stru;

/* ��ϵͳcb�ֶ� ֻ��20�ֽڿ���, ��ǰʹ��19�ֽ�; HCC[5]PAD[1]FRW[3]+CB[19]+MAC HEAD[36] */
struct mac_tx_ctl {
    /* byte1 */
    /* ȡֵ:FRW_EVENT_TYPE_WLAN_DTX��FRW_EVENT_TYPE_HOST_DRX������:���ͷ�ʱ�������ڴ�ص�netbuf����ԭ��̬�� */
    uint8_t bit_event_type : 5;
    uint8_t bit_event_sub_type : 3;
    /* byte2-3 */
    wlan_cb_frame_type_enum_uint8 uc_frame_type;       /* �Զ���֡���� */
    wlan_cb_frame_subtype_enum_uint8 uc_frame_subtype; /* �Զ���֡������ */
    /* byte4 */
    uint8_t bit_mpdu_num : 7;   /* ampdu�а�����MPDU����,ʵ����������д��ֵΪ��ֵ-1 */
    uint8_t bit_netbuf_num : 1; /* ÿ��MPDUռ�õ�netbuf��Ŀ */
    /* ��ÿ��MPDU�ĵ�һ��NETBUF����Ч */
    /* byte5-6 */
    uint16_t us_mpdu_payload_len; /* ÿ��MPDU�ĳ��Ȳ�����mac header length */
    /* byte7 */
    uint8_t bit_frame_header_length : 6; /* 51�ĵ�ַ32 */ /* ��MPDU��802.11ͷ���� */
    uint8_t bit_is_amsdu : 1;                             /* �Ƿ�AMSDU: OAL_FALSE���ǣ�OAL_TRUE�� */
    uint8_t bit_is_first_msdu : 1;                        /* �Ƿ��ǵ�һ����֡��OAL_FALSE���� OAL_TRUE�� */
    /* byte8 */
    uint8_t bit_tid : 4;                  /* dmac tx �� tx complete ���ݵ�user�ṹ�壬Ŀ���û���ַ */
    wlan_wme_ac_type_enum_uint8 bit_ac : 3; /* ac */
    uint8_t bit_ismcast : 1;              /* ��MPDU�ǵ������Ƕಥ:OAL_FALSE������OAL_TRUE�ಥ */
    /* byte9 */
    uint8_t bit_retried_num : 4;   /* �ش����� */
    uint8_t bit_mgmt_frame_id : 4; /* wpas ���͹���֡��frame id */
    /* byte10 */
    uint8_t bit_tx_user_idx : 6;          /* ����������userindex��һ��bit���ڱ�ʶ��Чindex */
    uint8_t bit_roam_data : 1;            /* �����ڼ�֡���ͱ�� */
    uint8_t bit_is_get_from_ps_queue : 1; /* ���������ã���ʶһ��MPDU�Ƿ�ӽ��ܶ�����ȡ������ */
    /* byte11 */
    uint8_t bit_tx_vap_index : 3;
    wlan_tx_ack_policy_enum_uint8 en_ack_policy : 3;
    uint8_t bit_is_needretry : 1;
    uint8_t bit_need_rsp : 1; /* WPAS send mgmt,need dmac response tx status */
    /* byte12 */
    uint8_t en_is_probe_data : 3; /* �Ƿ�̽��֡ */
    uint8_t bit_is_eapol_key_ptk : 1;                  /* 4 �����ֹ��������õ�����ԿEAPOL KEY ֡��ʶ */
    uint8_t bit_is_m2u_data : 1;                       /* �Ƿ����鲥ת���������� */
    uint8_t bit_is_tcp_ack : 1;                        /* ���ڱ��tcp ack */
    uint8_t bit_is_rtsp : 1;
    uint8_t en_use_4_addr : 1; /* �Ƿ�ʹ��4��ַ����WDS���Ծ��� */
    /* byte13-16 */
    uint16_t us_timestamp_ms;   /* ά��ʹ����TID���е�ʱ���, ��λ1ms���� */
    uint8_t bit_is_qosnull : 1; /* ���ڱ��qos null֡ */
    uint8_t bit_is_himit : 1; /* ���ڱ��himit֡ */
    uint8_t bit_hiex_traced : 1; /* ���ڱ��hiex֡ */
    uint8_t bit_is_game_marked : 1; /* ���ڱ����Ϸ֡ */
    uint8_t bit_is_hid2d_pkt : 1; /* ���ڱ��hid2dͶ������֡ */
    uint8_t bit_is_802_3_snap : 1; /* �Ƿ���802.3 snap */
    uint8_t uc_reserved1 : 2;
    uint8_t uc_data_type : 4; /* ����֡����, ring txʹ��, ��Ӧdata_type_enum */
    uint8_t csum_type : 3;
    uint8_t bit_is_pt_mcast : 1;  /* ��ʶ֡�Ƿ�Ϊ�鲥ͼ��picture transmissionЭ������ */
    /* byte17-18 */
    uint8_t uc_alg_pktno;     /* �㷨�õ����ֶΣ�Ψһ��ʾ�ñ��� */
    uint8_t uc_alg_frame_tag : 2; /* �����㷨��֡���б�� */
    uint8_t uc_hid2d_tx_delay_time : 6; /* ���ڱ���hid2d����֡��dmac��������ʱ�� */
    /* byte19 */
    uint8_t bit_large_amsdu_level : 2;  /* offload�´��AMSDU�ۺ϶� */
    uint8_t bit_align_padding : 2;      /* SKB ͷ������ETHER HEADERʱ,4�ֽڶ�����Ҫ��padding */
    uint8_t bit_msdu_num_in_amsdu : 2;  /* ����ۺ�ʱÿ��AMSDU��֡�� */
    uint8_t bit_is_large_skb_amsdu : 1; /* �Ƿ��Ƕ���֡����ۺ� */
    uint8_t bit_htc_flag : 1;           /* ���ڱ��htc */

#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
    /* OFFLOAD�ܹ��£�HOST���DEVICE��CB���� */
    mac_tx_expand_cb_stru st_expand_cb;
#endif
} __OAL_DECLARE_PACKED;
typedef struct mac_tx_ctl mac_tx_ctl_stru;
#pragma pack(pop)

/* ������offload�ܹ���CB�ֶ� */
#define MAC_GET_CB_IS_4ADDRESS(_pst_tx_ctrl)        ((_pst_tx_ctrl)->en_use_4_addr)
/* ����Ƿ���С��AMSDU֡ */
#define MAC_GET_CB_IS_AMSDU(_pst_tx_ctrl)           ((_pst_tx_ctrl)->bit_is_amsdu)
/* ����Ӧ�㷨�����ľۺ϶� */
#define MAC_GET_CB_AMSDU_LEVEL(_pst_tx_ctrl)        ((_pst_tx_ctrl)->bit_large_amsdu_level)
/* MSDU֡ͷ�������ֽ� */
#define MAC_GET_CB_ETHER_HEAD_PADDING(_pst_tx_ctrl) ((_pst_tx_ctrl)->bit_align_padding)
/* ʵ����ɵĴ��AMSDU֡��������֡�� */
#define MAC_GET_CB_HAS_MSDU_NUMBER(_pst_tx_ctrl)    ((_pst_tx_ctrl)->bit_msdu_num_in_amsdu)
/* ����Ƿ�����˴��AMSDU֡ */
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
/* VIP����֡ */
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

/* ģ�鷢�����̿�����Ϣ�ṹ�����ϢԪ�ػ�ȡ */
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
#define MAC_GET_CB_80211_MAC_HEAD_TYPE(_pst_tx_ctrl) 1 /* offload�ܹ�,MAC HEAD��netbuf index����,����Ҫ�ͷ� */

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
    /* ע��! �������mac header���Ȳ���24�ֽڵĲ�Ҫʹ�øú��� */
    return get_netbuf_payload(netbuf) + MAC_80211_FRAME_LEN;
#endif
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of mac_cb.h */
