

#ifndef __HMAC_CHBA_COEX_MGMT_H__
#define __HMAC_CHBA_COEX_MGMT_H__

#include "hmac_chba_common_function.h"
#include "hmac_chba_mgmt.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHBA_COEX_H

#ifdef __cplusplus
extern "C" {
#endif
#ifdef _PRE_WLAN_CHBA_MGMT
// 上报CHBA设备的共存信道信息
typedef struct {
    hmac_info_report_type_enum report_type;
    uint8_t dev_mac_addr[WLAN_MAC_ADDR_LEN];
    uint8_t coex_chan_cnt;
    uint8_t coex_chan_lists[MAX_CHANNEL_NUM_FREQ_5G];
} hmac_chba_report_coex_chan_info;

// 上报CHBA设备的共存信道信息
typedef struct {
    hmac_info_report_type_enum report_type;
    uint8_t island_dev_cnt;
    uint8_t island_dev_lists[MAC_CHBA_MAX_ISLAND_DEVICE_NUM][WLAN_MAC_ADDR_LEN];
} hmac_chba_report_island_info;
int32_t wal_ioctl_chba_set_coex_start_check(oal_net_device_stru *net_dev);
int32_t wal_ioctl_chba_set_coex_cmd_para_check(mac_chba_set_coex_chan_info *cfg_coex_chan_info);
uint32_t hmac_chba_coex_info_changed_report(mac_vap_stru *mac_vap, oal_uint8 len, oal_uint8 *param);
void hmac_chba_coex_island_info_report(mac_chba_self_conn_info_stru *self_conn_info);
uint32_t hmac_config_chba_set_coex_chan(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params);
oal_bool_enum_uint8 hmac_chba_coex_chan_is_in_list(uint8_t *coex_chan_lists,
    uint8_t chan_idx, uint8_t chan_number);
#endif
#ifdef __cplusplus
}
#endif

#endif
