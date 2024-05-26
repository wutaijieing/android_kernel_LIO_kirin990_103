

#ifndef __HMAC_CHBA_FRAME_H__
#define __HMAC_CHBA_FRAME_H__
/* 1 其他头文件包含 */
#include "hmac_device.h"
#include "hmac_vap.h"
#include "hmac_user.h"
#include "hmac_chba_function.h"
#include "mac_chba_common.h"
#include "hmac_chba_common_function.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHBA_FRAME_H

#ifdef _PRE_WLAN_CHBA_MGMT

/* 2 宏定义 */
/* 3 枚举定义 */
/* request、response类型枚举 */
typedef enum {
    MAC_CHBA_REQUEST_ACTION = 0,
    MAC_CHBA_RESPONSE_ACTION = 1,
    MAC_CHBA_FINISH_ACTION = 2,

    MAC_CHBA_ACTION_SUBTYPE_BUTT,
} hmac_chba_action_subtype_enum;

/* 4 全局变量声明 */
/* 5 消息头定义 */
/* 6 消息定义 */
/* 7 STRUCT定义 */
typedef struct {
    uint8_t master_election;
    uint8_t link_info;
    uint8_t channel_measurement;
    uint8_t qoe;
    uint8_t rrm_list;
    uint8_t channel_switch;
    uint8_t power_save_bitmap;
    uint8_t coex_status;
    uint8_t island_owner;
    uint8_t ie_container;
} attr_ctrl_info;

/* 8 UNION定义 */
/* 9 OTHERS定义 */
/* 10 函数声明 */
uint8_t *hmac_find_chba_attr(uint8_t attr_id, uint8_t *payload, int32_t len);
void mac_set_domain_bssid(uint8_t *domain_bssid, mac_vap_stru *mac_vap, hmac_chba_vap_stru *chba_vap_info);
void mac_set_chba_action_hdr(uint8_t *action_buf, uint8_t action_type);
void hmac_chba_assoc_set_ssid(hmac_vap_stru *hmac_vap, oal_uint8 *req_frame, oal_uint8 *ie_len, oal_uint8 *dest_addr);
void hmac_chba_set_assoc_scan_bss(hmac_vap_stru *hmac_sta, hmac_scanned_bss_info *scaned_bss);
uint32_t hmac_rx_chba_mgmt(hmac_vap_stru *hmac_vap, dmac_wlan_crx_event_stru *rx_mgmt_event, oal_netbuf_stru *netbuf);
uint32_t hmac_send_chba_sync_request_action(mac_vap_stru *mac_vap, void *chba_vap,
    uint8_t *peer_addr, uint8_t *payload, uint16_t payload_len);
void hmac_chba_encap_sync_beacon_frame(mac_vap_stru *mac_vap, uint8_t *beacon_buf, uint16_t *beacon_len,
    uint8_t *domain_bssid, uint8_t *payload, uint16_t payload_len);
void hmac_chba_encap_pnf_action_frame(mac_vap_stru *mac_vap, uint8_t *pnf_buf, uint16_t *pnf_len);

void hmac_chba_save_mgmt_frame(hmac_chba_vap_stru *chba_vap_info, uint8_t *payload,
    uint16_t payload_len, uint8_t mgmt_type);
uint32_t hmac_chba_set_master_election_attr(uint8_t *attr_buf);
void hmac_chba_encap_sync_beacon(hmac_chba_vap_stru *chba_vap_info, uint8_t *beacon_buf, uint16_t *beacon_len,
    attr_ctrl_info *attr_ctrl);
void hmac_chba_encap_pnf_action(hmac_chba_vap_stru *chba_vap_info, uint8_t *pnf_buf, uint16_t *pnf_len);
uint32_t hmac_chba_encape_assoc_attribute(uint8_t *buffer, uint32_t buffer_len);
void hmac_chba_send_sync_beacon(hmac_vap_stru *hmac_vap, uint8_t *bssid, uint8_t addr_len);
void hmac_chba_send_sync_action(uint8_t *sa_addr, uint8_t *bssid, uint8_t action_type, uint8_t request_type);
void hmac_chba_save_update_beacon_pnf(uint8_t *bssid);
uint32_t hmac_chba_decap_power_save_attr(uint8_t *payload, uint16_t len);
void hmac_chba_get_master_info_from_frame(uint8_t *payload, uint16_t payload_len, uint8_t *sa_addr,
    master_info *master_info);
uint32_t hmac_chba_tx_deauth_disassoc_complete(mac_vap_stru *mac_vap, oal_uint8 len, oal_uint8 *param);
uint32_t hmac_chba_encap_channel_sequence(hmac_chba_vap_stru *chba_vap_info, uint8_t *buffer);
#endif
#endif /* end of this file */
