

#ifndef __HMAC_CHBA_USER_H__
#define __HMAC_CHBA_USER_H__

#include "mac_chba_common.h"
#include "hmac_user.h"
#include "hmac_chba_mgmt.h"
#include "hmac_chba_function.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHBA_USER_H

#ifdef _PRE_WLAN_CHBA_MGMT

/* º¯ÊýÉùÃ÷ */
void hmac_chba_user_set_connect_role(hmac_user_stru *hmac_user, chba_connect_role_enum connect_role);
void hmac_chba_user_set_ssid(hmac_user_stru *hmac_user, uint8_t *ssid, uint8_t ssid_len);
uint32_t hmac_chba_add_user(mac_vap_stru *mac_vap, uint8_t *peer_addr, uint16_t *user_idx);
void hmac_chba_del_user_proc(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user);
uint32_t hmac_chba_user_mgmt_conn_prepare(mac_vap_stru *mac_vap,
    hmac_chba_conn_param_stru *chba_conn_params, int16_t *user_idx, uint8_t connect_role);
void hmac_chba_save_update_beacon_pnf(uint8_t *bssid);
void hmac_chba_kick_user(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user);
#endif
#endif
