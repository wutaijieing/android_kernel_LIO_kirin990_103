

#ifndef __HAL_COMMON_H__
#define __HAL_COMMON_H__

/* 1 头文件包含 */
#include "oal_types.h"
#include "wlan_types.h"
#ifdef _PRE_WLAN_FEATURE_HIEX
#include "hiex_msg.h"
#endif
#include "mac_delay.h"
#include "hal_common_descriptor.h"
/*****************************************************************************/
/*****************************************************************************/
/*                        公共宏定义、枚举、结构体                           */
/*****************************************************************************/
/*****************************************************************************/
#define MAX_MAC_AGGR_MPDU_NUM  256
#define AL_TX_MSDU_NETBUF_MAX_LEN 1500
#define HAL_INVALID_LUT_USER 0xFF

#define HAL_POW_CUSTOM_24G_11B_RATE_NUM        2 /* 定制化11b速率数目 */
#define HAL_POW_CUSTOM_11G_11A_RATE_NUM        5 /* 定制化11g/11a速率数目 */
#define HAL_POW_CUSTOM_HT20_VHT20_RATE_NUM     6 /* 定制化HT20_VHT20速率数目 */
#define HAL_POW_CUSTOM_24G_HT40_VHT40_RATE_NUM 8
#define HAL_POW_CUSTOM_5G_HT40_VHT40_RATE_NUM  7
#define HAL_POW_CUSTOM_5G_VHT80_RATE_NUM       6
/* 定制化全部速率 */
#define HAL_POW_CUSTOM_MCS9_10_11_RATE_NUM     3
#define HAL_POW_CUSTOM_MCS10_11_RATE_NUM       2
#define HAL_POW_CUSTOM_5G_VHT160_RATE_NUM      12 /* 定制化5G_11ac_VHT160速率数目 */
#define HAL_POW_CUSTOM_HT20_VHT20_DPD_RATE_NUM 5  /* 定制化DPD速率数目 */
#define HAL_POW_CUSTOM_HT40_VHT40_DPD_RATE_NUM 5
/* 定制化相关宏 */
/* NVRAM中存储的各协议速率最大发射功率参数的个数 From:24G_11b_1M To:5G_VHT80_MCS7 */
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
/* TPC档位设置 */
#define HAL_POW_LEVEL_NUM          5                              /* 算法总档位数目 */

#define HAL_MU_MAX_USER_NUM 1

/* 每个用户支持的最大速率集个数 */
#define HAL_TX_RATE_MAX_NUM 4

#define HAL_MULTI_TID_MAX_NUM      2  /* 支持Multi-TID的最大TID数目 */
#define HAL_MAX_ANT_NUM            2  /* 2G/5G下的天线数 */

#define HAL_TX_MSDU_DSCR_LEN 24 /* mac tx msdu描述符大小 */
#define HAL_TX_MSDU_MAX_PADDING_LEN 8

#define HAL_WF_SUB_BW_NUM 16 /* 子带注水子带个数 */
/* 子带的带宽模式定义 */
typedef enum {
    HAL_SUB_WF_MODE_10M = 0,
    HAL_SUB_WF_MODE_20M = 1,
    HAL_SUB_WF_MODE_40M = 2,
    HAL_SUB_WF_MODE_FULL = 3,

    HAL_SUB_WF_MODE_BUTT
} hal_sub_wf_mode_enum;
/* 3.4 中断相关枚举定义 */
/* 因为mac error和dmac misc优先级一致，03将high prio做实时事件队列来处理，mac error并入dmac misc */
/* 3.4.1 实时事件中断类型 */
typedef enum {
    HAL_EVENT_DMAC_HIGH_PRIO_BTCOEX_PS,   /* BTCOEX ps中断, 因为rom化，目前只能放置一个 */
    HAL_EVENT_DMAC_HIGH_PRIO_BTCOEX_LDAC, /* BTCOEX LDAC中断 */
    HAL_EVENT_DMAC_HIGH_PRIO_SWIRQ_SCHE,

    HAL_EVENT_DMAC_HIGH_PRIO_SUB_TYPE_BUTT
} hal_event_dmac_high_prio_sub_type_enum;
typedef uint8_t hal_event_dmac_high_prio_sub_type_enum_uint8;

/* 性能测试相关 */
typedef enum {
    HAL_ALWAYS_TX_DISABLE,      /* 禁用常发 */
    HAL_ALWAYS_TX_RF,           /* 保留给RF测试广播报文 */
    HAL_ALWAYS_TX_AMPDU_ENABLE, /* 使能AMPDU聚合包常发 */
    HAL_ALWAYS_TX_MPDU,         /* 使能非聚合包常发 */
    HAL_ALWAYS_TX_BUTT
} hal_device_always_tx_state_enum;
typedef uint8_t hal_device_always_tx_enum_uint8;

typedef enum {
    HAL_ALWAYS_RX_DISABLE,      /* 禁用常收 */
    HAL_ALWAYS_RX_RESERVED,     /* 保留给RF测试广播报文 */
    HAL_ALWAYS_RX_AMPDU_ENABLE, /* 使能AMPDU聚合包常收 */
    HAL_ALWAYS_RX_ENABLE,       /* 使能非聚合包常收 */
    HAL_ALWAYS_RX_BUTT
} hal_device_always_rx_state_enum;
typedef uint8_t hal_device_always_rx_enum_uint8;

typedef enum {
    HAL_TXBF_PROTOCOL_LEGACY = 0,  // 不包括11b
    HAL_TXBF_PROTOCOL_HT = 1,
    HAL_TXBF_PROTOCOL_VHT = 2,
    HAL_TXBF_PROTOCOL_HE = 3,

    HAL_TXBF_PROTOCOL_TYPE_BUTT
} hal_txbf_protocol_type_enum;
typedef uint8_t hal_txbf_protocol_type_enum_uint8;

/* 3.6 加密相关枚举定义 */
/* 3.6.1  芯片密钥类型定义 */
typedef enum {
    HAL_KEY_TYPE_TX_GTK = 0, /* Hi1102:HAL_KEY_TYPE_TX_IGTK */
    HAL_KEY_TYPE_PTK = 1,
    HAL_KEY_TYPE_RX_GTK = 2,
    HAL_KEY_TYPE_RX_GTK2 = 3, /* 02使用，03和51不使用 */
    HAL_KEY_TYPE_BUTT
} hal_cipher_key_type_enum;
typedef uint8_t hal_cipher_key_type_enum_uint8;

/* dmac_pkt_captures使用,tx rx均会使用 */
typedef struct {
    union {
        uint16_t rate_value;
        /* 除HT外的速率集定义 */
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
        } st_ht_rate; /* 11n的速率集定义 */
    } un_nss_rate;

    uint8_t uc_short_gi;
    uint8_t uc_bandwidth;

    uint8_t bit_preamble : 1,
            bit_channel_code : 1,
            bit_stbc : 2,
            bit_reserved2 : 4;
} hal_statistic_stru;

/*****************************************************************************
  结构名  : hal_rx_dscr_queue_header_stru
  结构说明: 接收描述符队列头的结构体
*****************************************************************************/
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
typedef struct {
    oal_dlist_head_stru st_header;                    /* 发送描述符队列头结点 */
    uint16_t us_element_cnt;                        /* 接收描述符队列中元素的个数 */
    hal_dscr_queue_status_enum_uint8 uc_queue_status; /* 接收描述符队列的状态 */
    uint8_t uc_available_res_cnt;                   /* 当前队列中硬件可用描述符个数 */
} hal_rx_dscr_queue_header_stru;
#else
typedef struct {
    uint32_t *pul_element_head;                     /* 指向接收描述符链表的第一个元素 */
    uint32_t *pul_element_tail;                     /* 指向接收描述符链表的最后一个元素 */
    uint16_t us_element_cnt;                        /* 接收描述符队列中元素的个数 */
    hal_dscr_queue_status_enum_uint8 uc_queue_status; /* 接收描述符队列的状态 */
    uint8_t auc_resv[1]; /* 对齐 1 */
} hal_rx_dscr_queue_header_stru;
#endif

/* 扫描状态，通过判断当前扫描的状态，判断多个扫描请求的处理策略以及上报扫描结果的策略 */
typedef enum {
    MAC_SCAN_STATE_IDLE,
    MAC_SCAN_STATE_RUNNING,

    MAC_SCAN_STATE_BUTT
} mac_scan_state_enum;
typedef uint8_t mac_scan_state_enum_uint8;

#ifdef _PRE_WLAN_FEATURE_11AX
typedef enum {
    HAL_HE_HTC_CONFIG_UPH_AUTO = 0,                   /* 默认auto, mac 插入和计算uph */
    HAL_HE_HTC_CONFIG_NO_HTC = 1,                     /* 数据帧不含有htc 头 */
    HAL_HE_HTC_CONFIG_SOFT_INSERT_SOFT_ADD = 2,       /* 软件添加 htc头 */
    HAL_HE_HTC_CONFIG_UPH_MAC_INSERT_TRIGGER_CAL = 3, /* mac插入htc,软件通过trigger帧计算uph */
    HAL_HE_HTC_CONFIG_MAC_INSERT_SOFT_ADD = 4,        /* mac插入软件固定的htc值 */

    HAL_HE_HTC_CONFIG_BUTT
} hal_he_htc_config_enum;
#endif

/* 接收方向帧类型，值不可修改 */
typedef enum {
    MAC_FRAME_TYPE_RTH   = 0,   /* Ethernet II帧格式 */
    MAC_FRAME_TYPE_RSV0  = 1,
    MAC_FRAME_TYPE_RSV1  = 2,
    MAC_FRAME_TYPE_80211 = 3,   /* 802.11帧格式 */

    FRAME_TYPE_RESEREVD         /* 保留 */
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
    /* MPDU缓存队列，中断上半部与下半部乒乓切换，以减少并发访问 */
    oal_netbuf_head_stru  ast_rx_mpdu_list[HAL_RX_MPDU_QNUM];
    oal_spin_lock_stru    st_spin_lock;
    /* 中断<下>半部当前使用的MPDU描述符队列 */
    uint8_t              cur_idx;
    uint8_t              auc_resv[3]; /* 3 resv */

    /* 每次最多出队列数目 */
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
    int32_t l_pow_par2; /* 二次项系数 */
    int32_t l_pow_par1; /* 一次 */
    int32_t l_pow_par0; /* 常数项 */
} hal_pwr_fit_para_stru;

#define HAL_BA_BITMAP_SIZE 8 /* MAC上报的ba info中ba bitmap大小为8个word */

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

/* hal device id 枚举 */
typedef enum {
    HAL_DEVICE_ID_MASTER        = 0,    /* master的hal device id */
    HAL_DEVICE_ID_SLAVE         = 1,    /* slave的hal device id */

    HAL_DEVICE_ID_BUTT                  /* 最大id */
} hal_device_id_enum;
typedef uint8_t hal_device_id_enum_enum_uint8;

typedef enum {
    HAL_DDR_RX = 0,  /* Host DDR接收数据 */
    HAL_RAM_RX = 1,  /* Device侧接收数据 */
    HAL_SWTICH_ING = 2,  /* DDR切换过程中 */

    HAL_RX_BUTT
} hal_rx_switch_enum;

typedef enum {
    WLAN_SINFFER_OFF, /* sniffer抓包功能关闭 */
    WLAN_SNIFFER_TRAVEL_CAP_ON, /* sniffer出行场景抓包功能开启 */
    WLAN_SINFFER_ON,  /* sniffer抓包功能开启 */
    WLAN_SNIFFER_STATE_BUTT
} wlan_sniffer_state_enum;
typedef uint8_t wlan_sniffer_state_enum_uint8;

typedef enum {
    WLAN_MONITOR_OTA_HOST_RPT, /* host ota上报 */
    WLAN_MONITOR_OTA_DEVICE_RPT,  /* device ota上报 */
    WLAN_MONITOR_OTA_ALL_RPT,

    WLAN_MONITOR_OTA_RPT_BUTT
} wlan_monitor_ota_state_enum;
typedef uint8_t wlan_monitor_ota_mode_enum_uint8;

/* 同1105的hal_rx_status_stru，与device要保持一致 */
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
            uint8_t bit_rsp_flag : 1;  /* beaforming时，是否上报信道矩阵的标识，0:上报，1:不上报 */
            uint8_t bit_mu_mimo_flag : 1;
            uint8_t bit_ofdma_flag : 1;
            uint8_t bit_beamforming_flag : 1; /* 接收帧是否开启了beamforming */
            uint8_t bit_STBC : 1;
        } st_rate; /* 11a/b/g/11ac/11ax的速率集定义 */
        struct {
            uint8_t bit_ht_mcs : 6;
            uint8_t bit_protocol_mode : 4;
            uint8_t bit_is_rx_vip : 1; /* place dcm bit */
            uint8_t bit_rsp_flag : 1;  /* beaforming时，是否上报信道矩阵的标识，0:上报，1:不上报 */
            uint8_t bit_mu_mimo_flag : 1;
            uint8_t bit_ofdma_flag : 1;
            uint8_t bit_beamforming_flag : 1; /* 接收帧是否开启了beamforming */
            uint8_t bit_STBC : 1;
        } st_ht_rate; /* 11n的速率集定义 */
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
    int8_t c_ant0_rssi; /* ANT0上报当前帧RSSI */
    int8_t c_ant1_rssi; /* ANT1上报当前帧RSSI */
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
