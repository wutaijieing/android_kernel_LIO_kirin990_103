

#ifndef __HMAC_HOST_TX_DATA_H__
#define __HMAC_HOST_TX_DATA_H__

#include "hmac_vap.h"
#include "hmac_tid_sche.h"

uint32_t hmac_ring_tx(hmac_vap_stru *hmac_vap, oal_netbuf_stru *netbuf, mac_tx_ctl_stru *tx_ctl);
uint32_t hmac_host_tx_tid_enqueue(hmac_tid_info_stru *hmac_tid_info, oal_netbuf_stru *netbuf);
uint32_t hmac_host_ring_tx(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, oal_netbuf_stru *netbuf);
void hmac_host_ring_tx_suspend(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, hmac_tid_info_stru *tid_info);
void hmac_host_ring_tx_resume(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, hmac_tid_info_stru *tid_info);
oal_bool_enum_uint8 hmac_is_ring_tx(mac_vap_stru *mac_vap, oal_netbuf_stru *netbuf, mac_tx_ctl_stru *tx_ctl);
static inline uint32_t hmac_host_tx_data(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, oal_netbuf_stru *netbuf)
{
    uint8_t tid;
    uint32_t ret;

    if (oal_likely(hmac_get_tid_schedule_list()->tid_schedule_thread)) {
        tid = MAC_GET_CB_WME_TID_TYPE((mac_tx_ctl_stru *)oal_netbuf_cb(netbuf));
        ret = hmac_host_tx_tid_enqueue(&hmac_user->tx_tid_info[tid], netbuf);

        host_end_record_performance(1, TX_XMIT_PROC);
        return ret;
    } else {
        return hmac_host_ring_tx(hmac_vap, hmac_user, netbuf);
    }
}

#endif
