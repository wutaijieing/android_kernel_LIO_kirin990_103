

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
#define HMAC_CHBA_INVALID_COEX_VAP_CNT 1
// 上报CHBA设备的共存信道信息
typedef struct {
    hmac_info_report_type_enum report_type;
    uint8_t dev_mac_addr[WLAN_MAC_ADDR_LEN];
    uint8_t coex_chan_cnt;
    uint8_t coex_chan_lists[WLAN_5G_CHANNEL_NUM];
} hmac_chba_report_coex_chan_info;

// 上报CHBA设备的共存信道信息
typedef struct {
    hmac_info_report_type_enum report_type;
    uint8_t island_dev_cnt;
    uint8_t island_dev_lists[MAC_CHBA_MAX_ISLAND_DEVICE_NUM][WLAN_MAC_ADDR_LEN];
} hmac_chba_report_island_info;

typedef enum {
    HMAC_CHBA_COEX_CHAN_SWITCH_STA_ROAM_RPT = 0, /* 因STA漫游避免DBAC触发信道切换上报 */
    HMAC_CHBA_COEX_CHAN_SWITCH_STA_CSA_RPT = 1, /* 因dmac STA避免DBAC触发信道切换上报 */
    HMAC_CHBA_COEX_CHAN_SWITCH_CHBA_CHAN_RPT = 2, /* 因CHBA本岛切信道避免DBAC上层触发信道切换 */
    HMAC_CHBA_COEX_CHAN_SWITCH_BUTT,
} hmac_chba_coex_chan_switch_dbac_rpt_enum;

int32_t wal_ioctl_chba_set_coex_start_check(oal_net_device_stru *net_dev);
int32_t wal_ioctl_chba_set_coex_cmd_para_check(mac_chba_set_coex_chan_info *cfg_coex_chan_info);
uint32_t hmac_chba_coex_info_changed_report(mac_vap_stru *mac_vap, uint8_t len, uint8_t *param);
uint32_t hmac_chba_coex_d2h_sta_csa_dbac_rpt(mac_vap_stru *mac_vap, uint8_t len, uint8_t *param);
void hmac_chba_coex_island_info_report(mac_chba_self_conn_info_stru *self_conn_info);
uint32_t hmac_config_chba_set_coex_chan(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params);
oal_bool_enum_uint8 hmac_chba_coex_chan_is_in_list(uint8_t *coex_chan_lists,
    uint8_t chan_idx, uint8_t chan_number);
oal_bool_enum_uint8 hmac_chba_coex_other_is_dbac(mac_device_stru *mac_device,
    mac_channel_stru *chan_info);
void hmac_chba_coex_switch_chan_dbac_rpt(mac_channel_stru *mac_channel, uint8_t rpt_type);
oal_bool_enum_uint8 hmac_chba_coex_go_csa_chan_is_dbac(mac_device_stru *mac_device,
    uint8_t new_channel, wlan_channel_bandwidth_enum_uint8 go_new_bw);
oal_bool_enum_uint8 hmac_chba_coex_channel_is_dbac(mac_vap_stru *start_vap,
    mac_channel_stru *new_channel);
oal_bool_enum_uint8 hmac_chba_coex_start_is_dbac(mac_vap_stru *start_vap, mac_channel_stru *new_channel);
#endif
#ifdef __cplusplus
}
#endif

#endif
