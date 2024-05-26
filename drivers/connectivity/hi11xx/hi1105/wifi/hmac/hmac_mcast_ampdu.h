

#ifndef __HMAC_MCAST_AMPDU_H__
#define __HMAC_MCAST_AMPDU_H__

/* 1 ����ͷ�ļ����� */
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
    /* ͳ����Ϣ */
    oal_bool_enum_uint8 pack_enable; /* PACK��������ʹ�ܱ�� */
    uint8_t rx_stats_count; /* ���ն�ͳ�Ƽ�ʱ��,��ʱ��0����ͳ����Ϣ */
    uint8_t rx_data_rssi; /* �յ�������֡ƽ��rssi */
    uint8_t resv;
    uint32_t rx_all_pkts; /* ��֡ͳ�� */
    uint32_t rx_non_retry_pkts; /* ���ظ�֡��֡ͳ�� */

    /* device���մ��� */
    uint16_t reorder_start; /* ��һ��δ�յ���MPDU�����к�,��reorder����ͷ,Ҳ������device���մ��ڵ���ʼλ�� */
    uint16_t reorder_end; /* ���һ���յ���MDPU���к�+1,��reorder����β,��head��end֮�们�� */
    uint16_t window_size; /* ����reorder�������ڵ�����device���մ��ڴ�С */
    uint16_t window_end;  /* device���մ��ڿ��Խ��յ����MPDU���к�,������ֵʱ��Ҫǿ���ƴ� */
    uint32_t rx_window_bitmap[BYTE_OFFSET_8]; /* rx buffer bitmap ��256λ */
} hmac_mcast_ampdu_rx_stats_stru; /* �鲥�ۺ���֡ͳ�ƽṹ�� */

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
