

#ifndef __HMAC_TX_DATA_MACRO_H__
#define __HMAC_TX_DATA_MACRO_H__


/* 1 其他头文件包含 */
#include "mac_frame.h"
#include "mac_frame_inl.h"
#include "mac_common.h"
#include "hmac_vap.h"
#include "hmac_user.h"
#include "hmac_main.h"
#include "hmac_mgmt_classifier.h"
#include "mac_resource.h"
#include "mac_mib.h"
#include "wlan_chip_i.h"
#include "hmac_multi_netbuf_amsdu.h"
#ifdef _PRE_WLAN_CHBA_MGMT
#include "hmac_chba_function.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_TX_DATA_MACRO_H

/* 2 宏定义 */
/* 基本能力信息中关于是否是QOS的能力位 */
#define HMAC_CAP_INFO_QOS_MASK 0x0200

#define wlan_tos_to_tid(_tos) ( \
    (((_tos) == 0) || ((_tos) == 3)) ? WLAN_TIDNO_BEST_EFFORT : (((_tos) == 1) || ((_tos) == 2)) ?   \
    WLAN_TIDNO_BACKGROUND : (((_tos) == 4) || ((_tos) == 5)) ? WLAN_TIDNO_VIDEO : WLAN_TIDNO_VOICE)

#define WLAN_BA_CNT_INTERVAL 100

#define HMAC_TXQUEUE_DROP_LIMIT_LOW  600
#define HMAC_TXQUEUE_DROP_LIMIT_HIGH 800

#define hmac_tx_pkts_stat(_pkt) (g_host_tx_pkts.snd_pkts += (_pkt))

#define WLAN_AMPDU_THROUGHPUT_THRESHOLD_HIGH            300 /* 300Mb/s */
#define WLAN_AMPDU_THROUGHPUT_THRESHOLD_MIDDLE          250 /* 250Mb/s */
#define WLAN_AMPDU_THROUGHPUT_THRESHOLD_LOW             200 /* 200Mb/s */
#define WLAN_SMALL_AMSDU_THROUGHPUT_THRESHOLD_HIGH      300 /* 300Mb/s */
#define WLAN_SMALL_AMSDU_THROUGHPUT_THRESHOLD_LOW       200 /* 200Mb/s */
#define WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_HIGH      90  /* 100Mb/s */
#define WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_LOW       30  /* 30Mb/s */
#define WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_HIGH_40M  300 /* 300Mb/s */
#define WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_LOW_40M   150 /* 150Mb/s */
#define WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_HIGH_80M  550 /* 500Mb/s */
#define WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_LOW_80M   450 /* 300Mb/s */
#define WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_HIGH_160M 800 /* 500Mb/s */
#define WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_LOW_160M  700 /* 300Mb/s */

#define WLAN_DYN_BYPASS_EXTLNA_THROUGHPUT_THRESHOLD_HIGH 100   /* 100Mb/s */
#define WLAN_DYN_BYPASS_EXTLNA_THROUGHPUT_THRESHOLD_LOW  50    /* 50Mb/s */
#define WLAN_SMALL_AMSDU_PPS_THRESHOLD_HIGH              25000 /* pps 25000 */
#define WLAN_SMALL_AMSDU_PPS_THRESHOLD_LOW               5000  /* pps 5000 */
#define WLAN_TCP_ACK_FILTER_THROUGHPUT_TH_HIGH           50
#define WLAN_TCP_ACK_FILTER_THROUGHPUT_TH_LOW            20

#define WLAN_AMSDU_THROUGHPUT_TH_HE_VHT_DIFF             100 /* 100Mb/s */

#define WLAN_AMPDU_HW_SWITCH_PERIOD 100 /* 硬件聚合切回软件聚合时间：10s */

#define CSUM_TYPE_IPV4_TCP 1
#define CSUM_TYPE_IPV4_UDP 2
#define CSUM_TYPE_IPV6_TCP 3
#define CSUM_TYPE_IPV6_UDP 4

/* 3 枚举定义 */
typedef enum {
    HMAC_TX_BSS_NOQOS = 0,
    HMAC_TX_BSS_QOS = 1,

    HMAC_TX_BSS_QOS_BUTT
} hmac_tx_bss_qos_type_enum;

/* 4 消息头定义 */
/* 5 消息定义 */
/* 6 STRUCT定义 */
typedef struct {
    uint32_t pkt_len;    /* HOST来帧量统计 */
    uint32_t snd_pkts;   /* 驱动实际发送帧统计 */
    uint32_t start_time; /* 均速统计开始时间 */
} hmac_tx_pkts_stat_stru;
extern hmac_tx_pkts_stat_stru g_host_tx_pkts;

#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
typedef struct {
    /* 定制化硬件聚合是否生效,默认为软件聚合 */
    uint8_t uc_ampdu_hw_en;
    /* 当前聚合是硬件聚合还是软件聚合 */
    uint8_t uc_ampdu_hw_enable;
    uint16_t us_remain_hw_cnt;
    uint16_t us_throughput_high;
    uint16_t us_throughput_low;
} hmac_tx_ampdu_hw_stru;
extern hmac_tx_ampdu_hw_stru g_st_ampdu_hw;
#endif

/* 8 UNION定义 */
/* 10 函数声明 */
uint32_t hmac_tx_encap_ieee80211_header(hmac_vap_stru *pst_vap, hmac_user_stru *pst_user, oal_netbuf_stru *pst_buf);
uint32_t hmac_tx_ucast_process(hmac_vap_stru *pst_vap, oal_netbuf_stru *pst_buf, hmac_user_stru *pst_user,
    mac_tx_ctl_stru *pst_tx_ctl);

void hmac_tx_ba_setup(hmac_vap_stru *pst_vap, hmac_user_stru *pst_user, uint8_t uc_tidno);
uint8_t hmac_tx_wmm_acm(oal_bool_enum_uint8 en_wmm, hmac_vap_stru *pst_hmac_vap, uint8_t *puc_tid);
#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
void hmac_tx_ampdu_switch(uint32_t tx_throughput_mbps);
#endif
void hmac_rx_dyn_bypass_extlna_switch(uint32_t tx_throughput_mbps, uint32_t rx_throughput_mbps);

#ifdef _PRE_WLAN_TCP_OPT
void hmac_tcp_ack_filter_switch(uint32_t rx_throughput_mbps);
#endif

void hmac_tx_small_amsdu_switch(uint32_t rx_throughput_mbps, uint32_t pps);

#ifdef _PRE_WLAN_FEATURE_TCP_ACK_BUFFER
void hmac_tx_tcp_ack_buf_switch(uint32_t rx_throughput_mbps);
#endif
void hmac_update_arp_tid_shenkuo(uint8_t *tid);

uint32_t hmac_tx_filter_security(hmac_vap_stru *hmac_vap, oal_netbuf_stru *netbuf, hmac_user_stru *hmac_user);
void hmac_tx_classify(hmac_vap_stru *hmac_vap,
    hmac_user_stru *hmac_user, oal_netbuf_stru *netbuf, mac_tx_ctl_stru *tx_ctl);
uint32_t hmac_tx_need_frag(
    hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, oal_netbuf_stru *netbuf, mac_tx_ctl_stru *tx_ctl);
void hmac_tx_classify_lan_to_wlan(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user,
    oal_netbuf_stru *pst_buf, uint8_t *puc_tid);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of hmac_tx_data.h */
