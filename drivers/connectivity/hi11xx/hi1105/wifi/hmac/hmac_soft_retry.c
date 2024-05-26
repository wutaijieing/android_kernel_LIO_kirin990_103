

#include "hmac_soft_retry.h"
#include "hmac_tx_msdu_dscr.h"
#include "hmac_host_tx_data.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_SOFT_RETRY_C


uint32_t hmac_tx_soft_retry_process(hmac_tid_info_stru *tid_info, oal_netbuf_stru *netbuf)
{
    mac_tx_ctl_stru *tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(netbuf);

    if (MAC_GET_CB_RETRIED_NUM(tx_ctl) == 0) {
        return OAL_FAIL;
    }

    MAC_GET_CB_RETRIED_NUM(tx_ctl)--;
    hmac_tx_msdu_update_free_cnt(&tid_info->tx_ring);

    oam_warning_log4(0, OAM_SF_TX, "{hmac_tx_soft_retry_process::user[%d] tid[%d] retry frame type[%d] subtype[%d]",
        tid_info->user_index, tid_info->tid_no, MAC_GET_CB_FRAME_TYPE(tx_ctl), MAC_GET_CB_FRAME_SUBTYPE(tx_ctl));

    hmac_host_tx_tid_enqueue(tid_info, netbuf);

    return OAL_SUCC;
}
