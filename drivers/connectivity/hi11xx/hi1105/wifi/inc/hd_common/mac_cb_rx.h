

#ifndef __MAC_CB_RX_H__
#define __MAC_CB_RX_H__

/* 1 其他头文件包含 */
#include "oal_types.h"
#include "wlan_types.h"

// 此处不加extern "C" UT编译不过
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/* HOST专属 */
typedef struct {
    uint32_t *pul_mac_hdr_start_addr; /* 对应的帧的帧头地址,虚拟地址 */
    uint16_t us_da_user_idx;          /* 目的地址用户索引 */
    uint16_t us_rsv;                  /* 对齐 */
} mac_rx_expand_cb_stru;

/* 裸系统下需要传输给HMAC模块的信息 */
/* hal_rx_ctl_stru结构的修改要考虑hi110x_rx_get_info_dscr函数中的优化 */
/* 1字节对齐 */
/* 对于06及以后产品，扫描结果上报hmac时会将extend info放到cb后面的部分，
   如果扩充cb大小，请同步检查mac_scanned_result_extend_info_stru大小（当前为18字节），
   保证sizeof(hal_rx_ctl_stru) + sizeof(mac_scanned_result_extend_info_stru)不超过55字节，否则cb部分会被覆盖！ */
#pragma pack(push, 1)
typedef struct {
    /* byte 0 */
    uint8_t bit_vap_id : 5;
    uint8_t bit_amsdu_enable : 1;    /* 是否为amsdu帧,每个skb标记 */
    uint8_t bit_is_first_buffer : 1; /* 当前skb是否为amsdu的首个skb */
    uint8_t bit_is_fragmented : 1;

    /* byte 1 */
    uint8_t uc_msdu_in_buffer; /* 每个skb包含的msdu数,amsdu用,每帧标记 */

    /* byte 2 */
    /* host add macro then open */
    uint8_t bit_ta_user_idx : 5;
    uint8_t rx_priv_trans   : 1;
    uint8_t bit_nan_flag : 1;
    uint8_t bit_is_key_frame : 1;
    /* byte 3 */
    uint8_t uc_mac_header_len : 6; /* mac header帧头长度 */
    uint8_t bit_is_beacon : 1;
    uint8_t bit_is_last_buffer : 1;

    /* byte 4-5 */
    uint16_t us_frame_len; /* 帧头与帧体的总长度,AMSDU非首帧不填 */

    /* byte 6 */
    /* host add macro then open 修改原因:host与device不一致导致uc_mac_vap_id获取错误，出现HOST死机 */
    uint8_t uc_mac_vap_id : 4; /* 业务侧vap id号 */
    uint8_t bit_buff_nums : 4; /* 每个MPDU占用的SKB数,AMSDU帧占多个 */
    /* byte 7 */
    uint8_t uc_channel_number; /* 接收帧的信道 */

#if (defined(_PRE_PRODUCT_ID_HI110X_HOST) || defined(_PRE_WLAN_FEATURE_MAC_HEADER_IN_PAYLOAD))
    /* 适配ring接收帧, 80211 head与payload一起; 非ring接收帧head与payload分离
       对于03/05 host skb cb 48字节，hcc层copy会有冗余，不会有功能问题 */
    /* byte 8-9 */
    uint16_t   us_frame_control;
    /* byte 10-11 */
    uint16_t   bit_start_seqnum   : 12;
    uint16_t   bit_cipher_type    : 4;
    /* byte 12-13 */
    uint16_t   bit_release_end_sn : 12;
    uint16_t   bit_fragment_num   : 4;
    /* byte 14-15 */
    uint16_t   bit_ipv4cs_valid   : 1;
    uint16_t   bit_ipv4cs_pass    : 1;
    uint16_t   bit_ptlcs_pass     : 1;
    uint16_t   bit_ptlcs_valid    : 1;
    uint16_t   bit_rx_user_id     : 8;
    uint16_t   bit_frame_format   : 2;
    uint16_t   bit_dst_is_valid   : 1;
    uint16_t   bit_mcast_bcast    : 1;
    /* byte 16-17 */
    uint16_t   us_seq_num         : 12;
    uint16_t   bit_process_flag   : 3;
    uint16_t   bit_release_valid  : 1;
    /* byte 18-19 */
    uint8_t    bit_data_type      : 5;
    uint8_t    bit_eth_flag       : 2;
    uint8_t    last_msdu_in_amsdu : 1;
    uint8_t    dst_user_id;
    /* byte 20-21 */
    uint16_t   bit_release_start_sn   : 12;
    uint16_t   bit_is_reo_timeout     : 1;
    uint16_t   is_before_last_release : 1;
    uint16_t   bit_is_ampdu           : 1;
    uint16_t   is_6ghz_flag           : 1;
    /* byte 22-23 */
    uint8_t    dst_hal_vap_id;
    uint8_t    bit_band_id          : 2;
    uint8_t    bit_dst_band_id      : 2;
    uint8_t    bit_rx_tid           : 4;
    /* byte 24 */
    uint8_t    rx_status;

    /* byte 25-28 */
    uint32_t   rx_lsb_pn;
    /* byte 29-30 */
    uint16_t   us_rx_msb_pn;
#endif
#ifdef _PRE_PRODUCT_ID_HI110X_HOST
    /* OFFLOAD架构下，HOST相对DEVICE的CB增量 */
    mac_rx_expand_cb_stru st_expand_cb;
#endif
} __OAL_DECLARE_PACKED hal_rx_ctl_stru;
#pragma pack(pop)

#define MAC_TX_CTL_SIZE oal_netbuf_cb_size()
typedef hal_rx_ctl_stru mac_rx_ctl_stru;

#pragma pack(push, 1)
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
typedef struct {
    /* byte 0 */
    uint8_t bit_cipher_protocol_type : 4; /* 接收帧加密类型 */
    uint8_t bit_dscr_status : 4;          /* 接收状态 */

    /* byte 1 */
    uint8_t bit_fec_coding : 1;
    uint8_t bit_last_mpdu_flag : 1;
    uint8_t bit_resv : 1;
    uint8_t bit_gi_type : 2;
    uint8_t bit_AMPDU : 1;
    uint8_t bit_sounding_mode : 2;

    /* byte 2 */
    uint8_t bit_ext_spatial_streams : 2;
    uint8_t bit_smoothing : 1;
    uint8_t bit_freq_bandwidth_mode : 4;
    uint8_t bit_preabmle : 1;

    /* byte 3-4 */
    union {
        struct {
            uint8_t bit_rate_mcs : 4;
            uint8_t bit_nss_mode : 2;
            uint8_t bit_protocol_mode : 2;

            uint8_t bit_rsp_flag : 1;
            uint8_t bit_STBC : 2;
            uint8_t bit_he_flag : 1;
            uint8_t bit_is_rx_vip : 1;
            uint8_t bit_he_ltf_type : 2;
            uint8_t bit_reserved3 : 1;
        } st_rate; /* 11a/b/g/11ac/11ax的速率集定义 */
        struct {
            uint8_t bit_ht_mcs : 6;
            uint8_t bit_protocol_mode : 2;
            uint8_t bit_rsp_flag : 1;
            uint8_t bit_STBC : 2;
            uint8_t bit_he_flag : 1;
            uint8_t bit_is_rx_vip : 1;
            uint8_t bit_he_ltf_type : 2;
            uint8_t bit_reserved3 : 1;
        } st_ht_rate; /* 11n的速率集定义 */
    } un_nss_rate;
} hal_rx_status_stru;
#else
typedef struct {
    /* byte 0 */
    uint8_t bit_cipher_protocol_type : 4; /* 接收帧加密类型 */
    uint8_t bit_dscr_status : 4;          /* 接收状态 */

    /* byte 1 */
    uint8_t bit_AMPDU : 1;
    uint8_t bit_last_mpdu_flag : 1; /* 固定位置 */
    uint8_t bit_gi_type : 2;
    uint8_t bit_he_ltf_type : 2;
    uint8_t bit_sounding_mode : 2;

    /* byte 2 */
    uint8_t bit_freq_bandwidth_mode : 3;
    uint8_t bit_rx_himit_flag : 1; /* rx himit flag */
    uint8_t bit_ext_spatial_streams : 2;
    uint8_t bit_smoothing : 1;
    uint8_t bit_fec_coding : 1; /* channel code */

    /* byte 3-4 */
    union {
        struct {
            uint16_t bit_rate_mcs : 4;
            uint16_t bit_nss_mode : 2;
            uint16_t bit_protocol_mode : 4;
            uint16_t bit_is_rx_vip : 1; /* place dcm bit */
            uint16_t bit_rsp_flag : 1;  /* beaforming时，是否上报信道矩阵的标识，0:上报，1:不上报 */
            uint16_t bit_mu_mimo_flag : 1;
            uint16_t bit_ofdma_flag : 1;
            uint16_t bit_beamforming_flag : 1; /* 接收帧是否开启了beamforming */
            uint16_t bit_STBC : 1;
        } st_rate; /* 11a/b/g/11ac/11ax的速率集定义 */
        struct {
            uint16_t bit_ht_mcs : 6;
            uint16_t bit_protocol_mode : 4;
            uint16_t bit_is_rx_vip : 1; /* place dcm bit */
            uint16_t bit_rsp_flag : 1;  /* beaforming时，是否上报信道矩阵的标识，0:上报，1:不上报 */
            uint16_t bit_mu_mimo_flag : 1;
            uint16_t bit_ofdma_flag : 1;
            uint16_t bit_beamforming_flag : 1; /* 接收帧是否开启了beamforming */
            uint16_t bit_STBC : 1;
        } st_ht_rate; /* 11n的速率集定义 */
    } un_nss_rate;
} hal_rx_status_stru;
#endif

typedef struct {
    /* byte 0 */
    int8_t c_rssi_dbm;

    /* byte 1 */
    uint8_t bit_resv4;

#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1105_DEV))
    int8_t snr[2];  /* 2天线的SNR */
    int8_t rssi[2]; /* 2天线的RSSI */
#else
    int8_t snr[4];  /* 4天线的SNR */
    int8_t rssi[4]; /* 4天线的RSSI */
#endif
} hal_rx_statistic_stru;
#pragma pack(pop)

/* DMAC模块接收流程控制信息结构，存放于对应的netbuf的CB字段中，最大值为48字节,
   如果修改，一定要通知sdt同步修改，否则解析会有错误!!!!!!!!!!!!!!!!!!!!!!!!! */
typedef struct {
    hal_rx_ctl_stru st_rx_info;            /* dmac需要传给hmac的数据信息 */
    hal_rx_status_stru st_rx_status;       /* 保存加密类型及帧长信息 */
    hal_rx_statistic_stru st_rx_statistic; /* 保存接收描述符的统计信息 */
} dmac_rx_ctl_stru;

#define MAC_GET_RX_CB_FRAME_LEN(_pst_rx_ctl)         ((_pst_rx_ctl)->us_frame_len)
#define MAC_GET_RX_CB_MAC_HEADER_LEN(_pst_rx_ctl)    ((_pst_rx_ctl)->uc_mac_header_len)
#define MAC_GET_RX_CB_MAC_VAP_ID(_pst_rx_ctl)        ((_pst_rx_ctl)->uc_mac_vap_id)
#define MAC_GET_RX_CB_HAL_VAP_IDX(_pst_rx_ctl)       ((_pst_rx_ctl)->bit_vap_id)
#define MAC_RXCB_VAP_ID(_pst_rx_ctl)                 ((_pst_rx_ctl)->uc_mac_vap_id)

#define MAC_GET_RX_CB_SEQ_NUM(_pst_rx_ctl)           ((_pst_rx_ctl)->us_seq_num)
#define MAC_GET_RX_CB_TID(_pst_rx_ctl)               ((_pst_rx_ctl)->bit_rx_tid)
#define MAC_GET_RX_CB_PROC_FLAG(_pst_rx_ctl)         ((_pst_rx_ctl)->bit_process_flag)
#define MAC_GET_RX_CB_REL_IS_VALID(_pst_rx_ctl)      ((_pst_rx_ctl)->bit_release_valid)
#define MAC_GET_RX_CB_REL_END_SEQNUM(_pst_rx_ctl)    ((_pst_rx_ctl)->bit_release_end_sn)
#define MAC_GET_RX_CB_SSN(_pst_rx_ctl)               ((_pst_rx_ctl)->bit_start_seqnum)
#define MAC_GET_RX_CB_REL_START_SEQNUM(_pst_rx_ctl)  ((_pst_rx_ctl)->bit_release_start_sn)
#define MAC_GET_RX_CB_DST_IS_VALID(_pst_rx_ctl)      ((_pst_rx_ctl)->bit_dst_is_valid)
#define MAC_GET_RX_DST_USER_ID(_pst_rx_ctl)          ((_pst_rx_ctl)->dst_user_id)

#define MAC_RXCB_IS_AMSDU(_pst_rx_ctl)               ((_pst_rx_ctl)->bit_amsdu_enable)
#define MAC_GET_RX_CB_IS_FIRST_SUB_MSDU(_pst_rx_ctl) ((_pst_rx_ctl)->bit_is_first_buffer)
#define MAC_RX_CB_IS_DEST_CURR_BSS(_pst_rx_ctl)     \
    (((_pst_rx_ctl)->bit_dst_is_valid) && (((_pst_rx_ctl)->bit_band_id) == ((_pst_rx_ctl)->bit_dst_band_id)) && \
    (((_pst_rx_ctl)->bit_vap_id) == ((_pst_rx_ctl)->dst_hal_vap_id)))

/* DMAC模块接收流程控制信息结构体的信息元素获取 */
#define MAC_RXCB_TA_USR_ID(_pst_rx_ctl) ((_pst_rx_ctl)->bit_ta_user_idx)
#define MAC_GET_RX_CB_TA_USER_IDX(_pst_rx_ctl) ((_pst_rx_ctl)->bit_ta_user_idx)
#define MAC_GET_RX_CB_PAYLOAD_LEN(_pst_rx_ctl) ((_pst_rx_ctl)->us_frame_len - (_pst_rx_ctl)->uc_mac_header_len)
#define MAC_GET_RX_CB_NAN_FLAG(_pst_rx_ctl) ((_pst_rx_ctl)->bit_nan_flag)

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
#ifdef _PRE_WLAN_FEATURE_MAC_HEADER_IN_PAYLOAD
#define MAC_GET_RX_PAYLOAD_ADDR(_rx_ctl, _pst_buf)  \
    (get_netbuf_payload(_pst_buf) + ((_rx_ctl)->uc_mac_header_len))
#else
#define MAC_GET_RX_PAYLOAD_ADDR(_rx_ctl, _pst_buf)  \
    (get_netbuf_payload(_pst_buf) + ((_rx_ctl)->uc_mac_header_len) - ((_rx_ctl)->uc_mac_header_len))
#endif
#else
#define MAC_GET_RX_PAYLOAD_ADDR(_rx_ctl, _pst_buf) \
    ((uint8_t *)(mac_get_rx_cb_mac_hdr(_rx_ctl)) + MAC_GET_RX_CB_MAC_HEADER_LEN(_rx_ctl))
#endif

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
#define MAC_GET_RX_CB_MAC_HEADER_ADDR(_prx_cb_ctrl)    (mac_get_rx_cb_mac_hdr(_prx_cb_ctrl))
#else
#define MAC_GET_RX_CB_MAC_HEADER_ADDR(_pst_rx_ctl)     ((_pst_rx_ctl)->st_expand_cb.pul_mac_hdr_start_addr)
#endif

#if (defined(_PRE_PRODUCT_ID_HI110X_HOST) || defined(_PRE_WLAN_FEATURE_MAC_HEADER_IN_PAYLOAD))
#define MAC_RXCB_STATUS(_pst_rx_ctl)                 ((_pst_rx_ctl)->rx_status)
#define MAC_RX_CB_IS_MULTICAST(_pst_rx_ctl)          ((_pst_rx_ctl)->bit_mcast_bcast)
#define MAC_GET_RX_CB_IS_REO_TIMEOUT(_pst_rx_ctl)    ((_pst_rx_ctl)->bit_is_reo_timeout)
#define MAC_RXCB_IS_AMSDU_SUB_MSDU(_pst_rx_ctl) \
    ((MAC_RXCB_IS_AMSDU(_pst_rx_ctl) == OAL_TRUE) && (MAC_GET_RX_CB_IS_FIRST_SUB_MSDU(_pst_rx_ctl) == OAL_FALSE))
#define MAC_RXCB_IS_FIRST_AMSDU(_pst_rx_ctl) \
    ((MAC_RXCB_IS_AMSDU(_pst_rx_ctl) == OAL_TRUE) && (MAC_GET_RX_CB_IS_FIRST_SUB_MSDU(_pst_rx_ctl) == OAL_TRUE))
#define MAC_RXCB_IS_LAST_MSDU(_pst_rx_ctl)  (((_pst_rx_ctl)->last_msdu_in_amsdu) && ((_pst_rx_ctl)->bit_amsdu_enable))
#endif


OAL_STATIC OAL_INLINE uint32_t *mac_get_rx_cb_mac_hdr(mac_rx_ctl_stru *pst_cb_ctrl)
{
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
#ifdef _PRE_WLAN_FEATURE_MAC_HEADER_IN_PAYLOAD
    return (uint32_t *)((uint8_t *)pst_cb_ctrl + OAL_MAX_CB_LEN + MAX_MAC_HEAD_LEN);
#else
    return (uint32_t *)((uint8_t *)pst_cb_ctrl + OAL_MAX_CB_LEN);
#endif
#else
    return MAC_GET_RX_CB_MAC_HEADER_ADDR(pst_cb_ctrl);
#endif
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
