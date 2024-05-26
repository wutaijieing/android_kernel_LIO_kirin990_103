

#ifndef __HMAC_TX_SWITCH_H__
#define __HMAC_TX_SWITCH_H__

#include "mac_vap.h"
#include "frw_ext_if.h"
#include "hmac_user.h"
#include "hmac_tid_ring_switch.h"
#include "hmac_tx_switch_fsm.h"

void hmac_release_avail_tx_ring(hmac_tid_info_stru *tid_info);
uint32_t hmac_tx_mode_switch(mac_vap_stru *mac_vap, uint32_t *params);
uint32_t hmac_tx_mode_switch_process(frw_event_mem_stru *event_mem);
uint32_t hmac_tx_mode_h2d_switch(hmac_tid_info_stru *tid_info);
uint32_t hmac_tx_mode_d2h_switch(hmac_tid_info_stru *tid_info);
void hmac_tx_mode_switch_dump_process(hmac_tid_info_stru *tid_info);
void hmac_tx_mode_h2d_switch_process(hmac_tid_info_stru *tid_info);
void hmac_tx_switch(uint8_t new_rx_mode);
void hmac_tx_pps_switch(uint32_t tx_pps);
oal_bool_enum_uint8 hmac_tx_keep_host_switch(void);
hmac_psm_tx_switch_event_enum hmac_psm_tx_switch_throughput_to_event(uint32_t tx_pps);
#endif
