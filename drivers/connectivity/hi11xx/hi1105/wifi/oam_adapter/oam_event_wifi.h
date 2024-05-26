
#ifndef __OAM_EVENT_WIFI_H__
#define __OAM_EVENT_WIFI_H__

#include "oal_types.h"
#include "oam_ext_if.h"
#include "wlan_oneimage_define.h"

#define MAC_ADDR_LEN 6

extern uint8_t g_bcast_addr[MAC_ADDR_LEN];
#define BROADCAST_MACADDR g_bcast_addr
uint32_t oam_report_80211_frame(uint8_t *puc_user_macaddr,
                                uint8_t *puc_mac_hdr_addr,
                                uint8_t uc_mac_hdr_len,
                                uint8_t *puc_mac_body_addr,
                                uint16_t us_mac_frame_len,
                                uint8_t en_frame_direction);
uint32_t oam_report_beacon(uint8_t *puc_beacon_hdr_addr,
                           uint8_t uc_beacon_hdr_len,
                           uint8_t *puc_beacon_body_addr,
                           uint16_t us_beacon_len,
                           uint8_t en_beacon_direction);
#endif