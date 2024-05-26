

#ifndef __HAL_COMMON_H__
#define __HAL_COMMON_H__

/* 1 ͷ�ļ����� */
#include "oal_types.h"
#include "wlan_types.h"
#ifdef _PRE_WLAN_FEATURE_HIEX
#include "hiex_msg.h"
#endif
#include "mac_delay.h"
#include "hal_common_descriptor.h"
/*****************************************************************************/
/*****************************************************************************/
/*                        �����궨�塢ö�١��ṹ��                           */
/*****************************************************************************/
/*****************************************************************************/
#define MAX_MAC_AGGR_MPDU_NUM  256
#define AL_TX_MSDU_NETBUF_MAX_LEN 1500
#define HAL_INVALID_LUT_USER 0xFF

#define HAL_POW_CUSTOM_24G_11B_RATE_NUM        2 /* ���ƻ�11b������Ŀ */
#define HAL_POW_CUSTOM_11G_11A_RATE_NUM        5 /* ���ƻ�11g/11a������Ŀ */
#define HAL_POW_CUSTOM_HT20_VHT20_RATE_NUM     6 /* ���ƻ�HT20_VHT20������Ŀ */
#define HAL_POW_CUSTOM_24G_HT40_VHT40_RATE_NUM 8
#define HAL_POW_CUSTOM_5G_HT40_VHT40_RATE_NUM  7
#define HAL_POW_CUSTOM_5G_VHT80_RATE_NUM       6
/* ���ƻ�ȫ������ */
#define HAL_POW_CUSTOM_MCS9_10_11_RATE_NUM     3
#define HAL_POW_CUSTOM_MCS10_11_RATE_NUM       2
#define HAL_POW_CUSTOM_5G_VHT160_RATE_NUM      12 /* ���ƻ�5G_11ac_VHT160������Ŀ */
#define HAL_POW_CUSTOM_HT20_VHT20_DPD_RATE_NUM 5  /* ���ƻ�DPD������Ŀ */
#define HAL_POW_CUSTOM_HT40_VHT40_DPD_RATE_NUM 5
/* ���ƻ���غ� */
/* NVRAM�д洢�ĸ�Э����������书�ʲ����ĸ��� From:24G_11b_1M To:5G_VHT80_MCS7 */
#define NUM_OF_NV_NORMAL_MAX_TXPOWER (HAL_POW_CUSTOM_24G_11B_RATE_NUM +                                            \
                                      HAL_POW_CUSTOM_11G_11A_RATE_NUM + HAL_POW_CUSTOM_HT20_VHT20_RATE_NUM +       \
                                      HAL_POW_CUSTOM_24G_HT40_VHT40_RATE_NUM + HAL_POW_CUSTOM_11G_11A_RATE_NUM +   \
                                      HAL_POW_CUSTOM_HT20_VHT20_RATE_NUM + HAL_POW_CUSTOM_5G_HT40_VHT40_RATE_NUM + \
                                      HAL_POW_CUSTOM_5G_VHT80_RATE_NUM)
#define NUM_OF_NV_MAX_TXPOWER (NUM_OF_NV_NORMAL_MAX_TXPOWER + \
                               HAL_POW_CUSTOM_MCS9_10_11_RATE_NUM * 4 + \
                               HAL_POW_CUSTOM_5G_VHT160_RATE_NUM + \
                               HAL_POW_CUSTOM_MCS10_11_RATE_NUM)

#define NUM_OF_NV_DPD_MAX_TXPOWER (HAL_POW_CUSTOM_HT20_VHT20_DPD_RATE_NUM + HAL_POW_CUSTOM_HT40_VHT40_DPD_RATE_NUM)
#define NUM_OF_NV_11B_DELTA_TXPOWER      2
#define NUM_OF_NV_5G_UPPER_UPC           4
#define NUM_OF_IQ_CAL_POWER              2
#define NUM_OF_NV_2G_LOW_POW_DELTA_VAL   4
#define NUM_OF_NV_5G_LPF_LVL             4
/* TPC��λ���� */
#define HAL_POW_LEVEL_NUM          5                              /* �㷨�ܵ�λ��Ŀ */

#define HAL_MU_MAX_USER_NUM 1

/* ÿ���û�֧�ֵ�������ʼ����� */
#define HAL_TX_RATE_MAX_NUM 4

#define HAL_MULTI_TID_MAX_NUM      2  /* ֧��Multi-TID�����TID��Ŀ */
#define HAL_MAX_ANT_NUM            2  /* 2G/5G�µ������� */

#define HAL_TX_MSDU_DSCR_LEN 24 /* mac tx msdu��������С */
#define HAL_TX_MSDU_MAX_PADDING_LEN 8

#define HAL_WF_SUB_BW_NUM 16 /* �Ӵ�עˮ�Ӵ����� */
/* �Ӵ��Ĵ���ģʽ���� */
typedef enum {
    HAL_SUB_WF_MODE_10M = 0,
    HAL_SUB_WF_MODE_20M = 1,
    HAL_SUB_WF_MODE_40M = 2,
    HAL_SUB_WF_MODE_FULL = 3,

    HAL_SUB_WF_MODE_BUTT
} hal_sub_wf_mode_enum;
/* 3.4 �ж����ö�ٶ��� */
/* ��Ϊmac error��dmac misc���ȼ�һ�£�03��high prio��ʵʱ�¼�������������mac error����dmac misc */
/* 3.4.1 ʵʱ�¼��ж����� */
typedef enum {
    HAL_EVENT_DMAC_HIGH_PRIO_BTCOEX_PS,   /* BTCOEX ps�ж�, ��Ϊrom����Ŀǰֻ�ܷ���һ�� */
    HAL_EVENT_DMAC_HIGH_PRIO_BTCOEX_LDAC, /* BTCOEX LDAC�ж� */
    HAL_EVENT_DMAC_HIGH_PRIO_SWIRQ_SCHE,

    HAL_EVENT_DMAC_HIGH_PRIO_SUB_TYPE_BUTT
} hal_event_dmac_high_prio_sub_type_enum;
typedef uint8_t hal_event_dmac_high_prio_sub_type_enum_uint8;

/* ���ܲ������ */
typedef enum {
    HAL_ALWAYS_TX_DISABLE,      /* ���ó��� */
    HAL_ALWAYS_TX_RF,           /* ������RF���Թ㲥���� */
    HAL_ALWAYS_TX_AMPDU_ENABLE, /* ʹ��AMPDU�ۺϰ����� */
    HAL_ALWAYS_TX_MPDU,         /* ʹ�ܷǾۺϰ����� */
    HAL_ALWAYS_TX_BUTT
} hal_device_always_tx_state_enum;
typedef uint8_t hal_device_always_tx_enum_uint8;

typedef enum {
    HAL_ALWAYS_RX_DISABLE,      /* ���ó��� */
    HAL_ALWAYS_RX_RESERVED,     /* ������RF���Թ㲥���� */
    HAL_ALWAYS_RX_AMPDU_ENABLE, /* ʹ��AMPDU�ۺϰ����� */
    HAL_ALWAYS_RX_ENABLE,       /* ʹ�ܷǾۺϰ����� */
    HAL_ALWAYS_RX_BUTT
} hal_device_always_rx_state_enum;
typedef uint8_t hal_device_always_rx_enum_uint8;

typedef enum {
    HAL_TXBF_PROTOCOL_LEGACY = 0,  // ������11b
    HAL_TXBF_PROTOCOL_HT = 1,
    HAL_TXBF_PROTOCOL_VHT = 2,
    HAL_TXBF_PROTOCOL_HE = 3,

    HAL_TXBF_PROTOCOL_TYPE_BUTT
} hal_txbf_protocol_type_enum;
typedef uint8_t hal_txbf_protocol_type_enum_uint8;

/* 3.6 �������ö�ٶ��� */
/* 3.6.1  оƬ��Կ���Ͷ��� */
typedef enum {
    HAL_KEY_TYPE_TX_GTK = 0, /* Hi1102:HAL_KEY_TYPE_TX_IGTK */
    HAL_KEY_TYPE_PTK = 1,
    HAL_KEY_TYPE_RX_GTK = 2,
    HAL_KEY_TYPE_RX_GTK2 = 3, /* 02ʹ�ã�03��51��ʹ�� */
    HAL_KEY_TYPE_BUTT
} hal_cipher_key_type_enum;
typedef uint8_t hal_cipher_key_type_enum_uint8;

/* dmac_pkt_capturesʹ��,tx rx����ʹ�� */
typedef struct {
    union {
        uint16_t rate_value;
        /* ��HT������ʼ����� */
        struct {
            uint16_t bit_rate_mcs : 4;
            uint16_t bit_nss_mode : 2;
            uint16_t bit_protocol_mode : 4;  // wlan_phy_protocol_mode_enum_uint8
            uint16_t bit_reserved2 : 6;
        } rate_stru;
        struct {
            uint16_t bit_ht_mcs : 6;
            uint16_t bit_protocol_mode : 4;
            uint16_t bit_reserved2 : 6;
        } st_ht_rate; /* 11n�����ʼ����� */
    } un_nss_rate;

    uint8_t uc_short_gi;
    uint8_t uc_bandwidth;

    uint8_t bit_preamble : 1,
            bit_channel_code : 1,
            bit_stbc : 2,
            bit_reserved2 : 4;
} hal_statistic_stru;

/*****************************************************************************
  �ṹ��  : hal_rx_dscr_queue_header_stru
  �ṹ˵��: ��������������ͷ�Ľṹ��
*****************************************************************************/
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
typedef struct {
    oal_dlist_head_stru st_header;                    /* ��������������ͷ��� */
    uint16_t us_element_cnt;                        /* ����������������Ԫ�صĸ��� */
    hal_dscr_queue_status_enum_uint8 uc_queue_status; /* �������������е�״̬ */
    uint8_t uc_available_res_cnt;                   /* ��ǰ������Ӳ���������������� */
} hal_rx_dscr_queue_header_stru;
#else
typedef struct {
    uint32_t *pul_element_head;                     /* ָ����������������ĵ�һ��Ԫ�� */
    uint32_t *pul_element_tail;                     /* ָ��������������������һ��Ԫ�� */
    uint16_t us_element_cnt;                        /* ����������������Ԫ�صĸ��� */
    hal_dscr_queue_status_enum_uint8 uc_queue_status; /* �������������е�״̬ */
    uint8_t auc_resv[1]; /* ���� 1 */
} hal_rx_dscr_queue_header_stru;
#endif

/* ɨ��״̬��ͨ���жϵ�ǰɨ���״̬���ж϶��ɨ������Ĵ��������Լ��ϱ�ɨ�����Ĳ��� */
typedef enum {
    MAC_SCAN_STATE_IDLE,
    MAC_SCAN_STATE_RUNNING,

    MAC_SCAN_STATE_BUTT
} mac_scan_state_enum;
typedef uint8_t mac_scan_state_enum_uint8;

#ifdef _PRE_WLAN_FEATURE_11AX
typedef enum {
    HAL_HE_HTC_CONFIG_UPH_AUTO = 0,                   /* Ĭ��auto, mac ����ͼ���uph */
    HAL_HE_HTC_CONFIG_NO_HTC = 1,                     /* ����֡������htc ͷ */
    HAL_HE_HTC_CONFIG_SOFT_INSERT_SOFT_ADD = 2,       /* �������� htcͷ */
    HAL_HE_HTC_CONFIG_UPH_MAC_INSERT_TRIGGER_CAL = 3, /* mac����htc,����ͨ��trigger֡����uph */
    HAL_HE_HTC_CONFIG_MAC_INSERT_SOFT_ADD = 4,        /* mac���������̶���htcֵ */

    HAL_HE_HTC_CONFIG_BUTT
} hal_he_htc_config_enum;
#endif

/* ���շ���֡���ͣ�ֵ�����޸� */
typedef enum {
    MAC_FRAME_TYPE_RTH   = 0,   /* Ethernet II֡��ʽ */
    MAC_FRAME_TYPE_RSV0  = 1,
    MAC_FRAME_TYPE_RSV1  = 2,
    MAC_FRAME_TYPE_80211 = 3,   /* 802.11֡��ʽ */

    FRAME_TYPE_RESEREVD         /* ���� */
} mac_frame_type_enum;
typedef uint8_t mac_frame_type_enum_uint8;

typedef enum {
    MGMT_COMP_RING = 0,
    DATA_COMP_RING,
    RX_PPDU_COMP_RING,
    BA_INFO_RING,

    RX_COMP_RING_TYPE_BUTT
} RX_COMP_RING_TYPE;
typedef uint8_t hal_comp_ring_type_enum_uint8;

#define HAL_RX_MPDU_QNUM 2
#define HAL_DEV_BA_INFO_COUNT 64

typedef struct tag_hal_device_rx_mpdu_stru {
    /* MPDU������У��ж��ϰ벿���°벿ƹ���л����Լ��ٲ������� */
    oal_netbuf_head_stru  ast_rx_mpdu_list[HAL_RX_MPDU_QNUM];
    oal_spin_lock_stru    st_spin_lock;
    /* �ж�<��>�벿��ǰʹ�õ�MPDU���������� */
    uint8_t              cur_idx;
    uint8_t              auc_resv[3]; /* 3 resv */

    /* ÿ������������Ŀ */
    uint32_t             process_num_per_round;
} hal_rx_mpdu_que;

typedef struct {
    volatile uint32_t tx_msdu_info_ring_base_lsb;
    volatile uint32_t tx_msdu_info_ring_base_msb;

    volatile uint16_t write_ptr;
    volatile uint16_t tx_msdu_ring_depth : 4;
    volatile uint16_t max_aggr_amsdu_num : 4;
    volatile uint16_t reserved : 8;

    volatile uint16_t read_ptr;
    volatile uint16_t reserved1;
} hal_tx_msdu_info_ring_stru;

typedef struct hal_pwr_fit_para_stru {
    int32_t l_pow_par2; /* ������ϵ�� */
    int32_t l_pow_par1; /* һ�� */
    int32_t l_pow_par0; /* ������ */
} hal_pwr_fit_para_stru;

#define HAL_BA_BITMAP_SIZE 8 /* MAC�ϱ���ba info��ba bitmap��СΪ8��word */

typedef struct {
    oal_dlist_head_stru entry;
    uint16_t user_id;
    uint16_t ba_ssn;
    uint16_t rptr;
    uint8_t rptr_valid;
    uint8_t winsize;
    uint8_t tid_no;
    uint8_t ba_info_vld;
} hal_tx_ba_info_stru;

typedef struct {
    uint16_t seq_num;
    oal_bool_enum_uint8 sn_vld;
    uint8_t tx_count;
} hal_tx_msdu_dscr_info_stru;

#ifdef _PRE_WLAN_FEATURE_HIEX
typedef mac_hiex_cap_stru hal_hiex_cap_stru;
#endif

/* hal device id ö�� */
typedef enum {
    HAL_DEVICE_ID_MASTER        = 0,    /* master��hal device id */
    HAL_DEVICE_ID_SLAVE         = 1,    /* slave��hal device id */

    HAL_DEVICE_ID_BUTT                  /* ���id */
} hal_device_id_enum;
typedef uint8_t hal_device_id_enum_enum_uint8;

typedef enum {
    HAL_DDR_RX = 0,  /* Host DDR�������� */
    HAL_RAM_RX = 1,  /* Device��������� */
    HAL_SWTICH_ING = 2,  /* DDR�л������� */

    HAL_RX_BUTT
} hal_rx_switch_enum;

typedef enum {
    WLAN_SINFFER_OFF, /* snifferץ�����ܹر� */
    WLAN_SNIFFER_TRAVEL_CAP_ON, /* sniffer���г���ץ�����ܿ��� */
    WLAN_SINFFER_ON,  /* snifferץ�����ܿ��� */
    WLAN_SNIFFER_STATE_BUTT
} wlan_sniffer_state_enum;
typedef uint8_t wlan_sniffer_state_enum_uint8;

typedef enum {
    WLAN_MONITOR_OTA_HOST_RPT, /* host ota�ϱ� */
    WLAN_MONITOR_OTA_DEVICE_RPT,  /* device ota�ϱ� */
    WLAN_MONITOR_OTA_ALL_RPT,

    WLAN_MONITOR_OTA_RPT_BUTT
} wlan_monitor_ota_state_enum;
typedef uint8_t wlan_monitor_ota_mode_enum_uint8;

/* ͬ1105��hal_rx_status_stru����deviceҪ����һ�� */
typedef struct {
    /* byte 0 */
    uint8_t bit_cipher_protocol_type : 4; /* ����֡�������� */
    uint8_t bit_dscr_status : 4;          /* ����״̬ */

    /* byte 1 */
    uint8_t bit_AMPDU : 1;
    uint8_t bit_last_mpdu_flag : 1; /* �̶�λ�� */
    uint8_t bit_gi_type : 2;
    uint8_t bit_he_ltf_type : 2;
    uint8_t bit_sounding_mode : 2;

    /* byte 2 */
    uint8_t bit_freq_bandwidth_mode : 3;
    uint8_t bit_rx_himit_flag : 1; /* rx himit flag = 1103 bit_preabmle */
    uint8_t bit_ext_spatial_streams : 2;
    uint8_t bit_smoothing : 1;
    uint8_t bit_fec_coding : 1; /* channel code */

    /* byte 3-4 */
    union {
        struct {
            uint8_t bit_rate_mcs : 4;
            uint8_t bit_nss_mode : 2;

            uint8_t bit_protocol_mode : 4;
            uint8_t bit_is_rx_vip : 1; /* place dcm bit */
            uint8_t bit_rsp_flag : 1;  /* beaformingʱ���Ƿ��ϱ��ŵ�����ı�ʶ��0:�ϱ���1:���ϱ� */
            uint8_t bit_mu_mimo_flag : 1;
            uint8_t bit_ofdma_flag : 1;
            uint8_t bit_beamforming_flag : 1; /* ����֡�Ƿ�����beamforming */
            uint8_t bit_STBC : 1;
        } st_rate; /* 11a/b/g/11ac/11ax�����ʼ����� */
        struct {
            uint8_t bit_ht_mcs : 6;
            uint8_t bit_protocol_mode : 4;
            uint8_t bit_is_rx_vip : 1; /* place dcm bit */
            uint8_t bit_rsp_flag : 1;  /* beaformingʱ���Ƿ��ϱ��ŵ�����ı�ʶ��0:�ϱ���1:���ϱ� */
            uint8_t bit_mu_mimo_flag : 1;
            uint8_t bit_ofdma_flag : 1;
            uint8_t bit_beamforming_flag : 1; /* ����֡�Ƿ�����beamforming */
            uint8_t bit_STBC : 1;
        } st_ht_rate; /* 11n�����ʼ����� */
    } un_nss_rate;
} hal_sniffer_rx_status_stru;

typedef struct {
    /* byte 0 */
    int8_t c_rssi_dbm;

    /* byte 1-4 */
    uint8_t uc_code_book;
    uint8_t uc_grouping;
    uint8_t uc_row_number;

    /* byte 5-6 */
    int8_t c_snr_ant0; /* ant0 SNR */
    int8_t c_snr_ant1; /* ant1 SNR */

    /* byte 7-8 */
    int8_t c_ant0_rssi; /* ANT0�ϱ���ǰ֡RSSI */
    int8_t c_ant1_rssi; /* ANT1�ϱ���ǰ֡RSSI */
} hal_sniffer_rx_statistic_stru;

#define SNIFFER_RX_INFO_SIZE                                                              \
    (sizeof(hal_sniffer_rx_status_stru) + sizeof(hal_sniffer_rx_statistic_stru) + \
        sizeof(uint32_t) + sizeof(hal_statistic_stru))

#ifdef _PRE_WLAN_FEATURE_SNIFFER
/* sniffer extend info */
typedef struct {
    hal_sniffer_rx_status_stru sniffer_rx_status;
    hal_sniffer_rx_statistic_stru sniffer_rx_statistic;
    hal_statistic_stru per_rate;
    uint32_t rate_kbps;
} hal_sniffer_extend_info;
#endif

#endif /* end of hal_common.h */