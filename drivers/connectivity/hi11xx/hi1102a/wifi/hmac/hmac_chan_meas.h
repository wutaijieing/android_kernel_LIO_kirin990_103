
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

/* 扫描信息上报内核 */
typedef struct {
    hmac_info_report_type_enum acs_info_type;    /* 内核用来区分消息类型 */
    uint8_t scan_chan_sbs;                            /* 当前扫描信道在扫描list中的下标 */
    uint8_t scan_chan_idx;                            /* 当前扫描信道的信道号 */
    uint32_t total_stats_time_us;
    uint32_t total_free_time_20m_us;
    uint32_t total_free_time_40m_us;
    uint32_t total_free_time_80m_us;
    uint32_t total_send_time_us;
    uint32_t total_recv_time_us;
    uint8_t free_power_cnt;                           /* 信道空闲功率测量次数 */
    int16_t free_power_stats_20m;
    int16_t free_power_stats_40m;
    int16_t free_power_stats_80m;
} hmac_chba_chan_stat_report_stru;

typedef struct {
    uint8_t idx;                            /* 序号 */
    uint8_t uc_chan_idx;                    /* 主20MHz信道索引号 */
    uint8_t uc_chan_number;                 /* 主20MHz信道号 */
    wlan_channel_bandwidth_enum_uint8 en_bandwidth; /* 带宽模式 */
} hmac_chan_stru;

#define HMAC_CHANNEL_NUM_2G                  32     /* 2G上全部的信道 */
#define HMAC_CHANNEL_NUM_5G                  73     /* 5G上全部的信道 */
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
