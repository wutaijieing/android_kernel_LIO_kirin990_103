

#ifndef __HMAC_MCAST_AMPDU_H__
#define __HMAC_MCAST_AMPDU_H__

/* 1 其他头文件包含 */
#include "hmac_user.h"
#include "hmac_vap.h"
#include "hmac_rx_data.h"
#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_MCAST_AMPDU_H
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
typedef struct {
    /* 统计信息 */
    oal_bool_enum_uint8 pack_enable; /* PACK反馈机制使能标记 */
    uint8_t rx_stats_count; /* 接收端统计计时器,计时到0则反馈统计信息 */
    uint8_t rx_data_rssi; /* 收到的数据帧平均rssi */
    uint8_t resv;
    uint32_t rx_all_pkts; /* 收帧统计 */
    uint32_t rx_non_retry_pkts; /* 非重复帧收帧统计 */

    /* device接收窗口 */
    uint16_t reorder_start; /* 第一个未收到的MPDU的序列号,是reorder队列头,也是整个device接收窗口的起始位置 */
    uint16_t reorder_end; /* 最后一个收到的MDPU序列号+1,是reorder队列尾,在head和end之间滑动 */
    uint16_t window_size; /* 包括reorder队列在内的整个device接收窗口大小 */
    uint16_t window_end;  /* device接收窗口可以接收的最大MPDU序列号,超出此值时需要强制移窗 */
    uint32_t rx_window_bitmap[BYTE_OFFSET_8]; /* rx buffer bitmap 共256位 */
} hmac_mcast_ampdu_rx_stats_stru; /* 组播聚合收帧统计结构体 */

void hmac_mcast_ampdu_netbuf_process(mac_vap_stru *mac_vap, oal_netbuf_stru *netbuf, mac_tx_ctl_stru *tx_ctl,
    mac_ether_header_stru *eth_hdr, oal_bool_enum_uint8 *multicast_need_host_tx);
void hmac_mcast_ampdu_user_tid_init(mac_vap_stru *mac_vap, hmac_user_stru *hmac_user, uint8_t *multi_user_lut_idx);
void hmac_mcast_ampdu_rx_win_init(hmac_user_stru *hmac_user, uint8_t tidno);
void hmac_mcast_ampdu_set_pack_switch(mac_vap_stru *mac_vap, uint8_t en);
hmac_rx_frame_ctrl_enum hmac_mcast_ampdu_rx_proc(hmac_host_rx_context_stru *rx_context);
void hmac_mcast_ampdu_stats_stru_init(hmac_vap_stru *hmac_vap);
#endif
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
