

#ifndef __HMAC_TX_RING_ALLOC_H__
#define __HMAC_TX_RING_ALLOC_H__

#include "hmac_user.h"

uint32_t hmac_init_tx_ring(hmac_user_stru *hmac_user, uint8_t tid);
uint32_t hmac_alloc_tx_ring(hmac_msdu_info_ring_stru *tx_ring);
void hmac_user_tx_ring_deinit(hmac_user_stru *hmac_user);
uint32_t hmac_set_tx_ring_device_base_addr(frw_event_mem_stru *frw_mem);
void hmac_tx_host_ring_release(hmac_msdu_info_ring_stru *tx_ring);
void hmac_wait_tx_ring_empty(hmac_tid_info_stru *tid_info);
void hmac_tx_ring_release_all_netbuf(hmac_msdu_info_ring_stru *tx_ring);

#endif
