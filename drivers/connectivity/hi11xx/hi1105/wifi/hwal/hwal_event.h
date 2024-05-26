

#ifndef __HWAL_EVENT_H__
#define __HWAL_EVENT_H__

// 1 其他头文件包含
#include "driver_hisi_common.h"
#include "oam_ext_if.h"
#include "oal_net.h"
#include "driver_hisi_lib_api.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

// 2 函数声明
void cfg80211_new_sta(oal_net_device_stru *pst_dev,
                      const uint8_t *puc_macaddr,
                      oal_station_info_stru *pst_sinfo,
                      oal_gfp_enum_uint8 en_gfp);
void cfg80211_del_sta(oal_net_device_stru *pst_dev, const uint8_t *puc_mac_addr, oal_gfp_enum_uint8 en_gfp);
oal_bool_enum_uint8 cfg80211_rx_mgmt(oal_wireless_dev_stru *pst_wdev,
                                     int32_t l_freq,
                                     int32_t l_sig_mbm,
                                     const uint8_t *puc_buf,
                                     size_t ul_len,
                                     oal_gfp_enum_uint8 en_gfp);
oal_bool_enum_uint8 cfg80211_mgmt_tx_status(struct wireless_dev *pst_wdev,
                                            uint64_t ull_cookie,
                                            const uint8_t *puc_buf,
                                            size_t ul_len,
                                            oal_bool_enum_uint8 en_ack,
                                            oal_gfp_enum_uint8 en_gfp);
void cfg80211_inform_bss_frame(oal_wiphy_stru *pst_wiphy,
                               oal_ieee80211_channel_stru *pst_ieee80211_channel,
                               oal_ieee80211_mgmt_stru *pst_mgmt,
                               uint32_t ul_len,
                               int32_t l_signal,
                               oal_gfp_enum_uint8 en_ftp);
void cfg80211_connect_result(oal_net_device_stru *pst_dev,
                             const uint8_t *puc_bssid,
                             const uint8_t *puc_req_ie,
                             size_t ul_req_ie_len,
                             const uint8_t *puc_resp_ie,
                             size_t ul_resp_ie_len,
                             uint16_t us_status,
                             oal_gfp_enum_uint8 en_gfp);
uint32_t cfg80211_disconnected(oal_net_device_stru *pst_net_device,
                               uint16_t us_reason,
                               uint8_t *puc_ie,
                               uint32_t ul_ie_len,
                               oal_gfp_enum_uint8 en_gfp);
void cfg80211_scan_done(oal_cfg80211_scan_request_stru *pst_request, uint8_t uc_aborted);
void cfg80211_rx_exception(oal_net_device_stru *netdev, uint8_t *puc_data, uint32_t ul_data_len);


#ifdef _PRE_WLAN_FEATURE_HILINK
uint32_t hisi_wlan_register_upload_frame_cb(hisi_upload_frame_cb func);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif

