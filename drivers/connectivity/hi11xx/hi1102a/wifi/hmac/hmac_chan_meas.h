
#ifndef __HMAC_CHAN_MEAS_H__
#define __HMAC_CHAN_MEAS_H__
#include "hmac_device.h"
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHAN_MEAS_H

/* ɨ����Ϣ�ϱ��ں� */
typedef struct {
    hmac_info_report_type_enum acs_info_type;    /* �ں�����������Ϣ���� */
    uint8_t scan_chan_sbs;                            /* ��ǰɨ���ŵ���ɨ��list�е��±� */
    uint8_t scan_chan_idx;                            /* ��ǰɨ���ŵ����ŵ��� */
    uint32_t total_stats_time_us;
    uint32_t total_free_time_20m_us;
    uint32_t total_free_time_40m_us;
    uint32_t total_free_time_80m_us;
    uint32_t total_send_time_us;
    uint32_t total_recv_time_us;
    uint8_t free_power_cnt;                           /* �ŵ����й��ʲ������� */
    int16_t free_power_stats_20m;
    int16_t free_power_stats_40m;
    int16_t free_power_stats_80m;
} hmac_chba_chan_stat_report_stru;

typedef struct {
    uint8_t idx;                            /* ��� */
    uint8_t uc_chan_idx;                    /* ��20MHz�ŵ������� */
    uint8_t uc_chan_number;                 /* ��20MHz�ŵ��� */
    wlan_channel_bandwidth_enum_uint8 en_bandwidth; /* ����ģʽ */
} hmac_chan_stru;

#define HMAC_CHANNEL_NUM_2G                  32     /* 2G��ȫ�����ŵ� */
#define HMAC_CHANNEL_NUM_5G                  73     /* 5G��ȫ�����ŵ� */
extern hmac_chan_stru g_aus_channel_candidate_list_5g[HMAC_CHANNEL_NUM_5G];
extern hmac_chan_stru g_aus_channel_candidate_list_2g[HMAC_CHANNEL_NUM_2G];

void hmac_chan_meas_scan_complete_handler(hmac_scan_record_stru *pst_scan_record,
    uint8_t uc_scan_idx);
uint32_t hmac_chan_meas_scan_chan_once(mac_vap_stru *pst_mac_vap, mac_cfg_link_meas_stru *meas_cmd);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif
