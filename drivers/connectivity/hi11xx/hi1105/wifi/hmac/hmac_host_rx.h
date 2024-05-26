

#ifndef __HMAC_HOST_RX_H__
#define __HMAC_HOST_RX_H__

#include "hmac_rx_data.h"
#include "frw_ext_if.h"

#ifdef __cplusplus // windows ut编译不过，后续下线清理
#if __cplusplus
extern "C" {
#endif
#endif
#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_HOST_RX_H

void hmac_rx_common_proc(hmac_host_rx_context_stru *rx_context, oal_netbuf_head_stru *rpt_list);
uint32_t hmac_host_rx_ring_data_event(frw_event_mem_stru *event_mem);
void hmac_rx_rpt_netbuf(oal_netbuf_head_stru *list);
uint32_t hmac_rx_proc_feature(hmac_host_rx_context_stru *rx_context, oal_netbuf_head_stru *rpt_list,
    oal_netbuf_head_stru *w2w_list);
uint32_t hmac_host_build_rx_context(hal_host_device_stru *hal_dev, oal_netbuf_stru *netbuf,
    hmac_host_rx_context_stru *rx_context);
uint32_t hmac_rx_proc_feature(hmac_host_rx_context_stru *rx_context, oal_netbuf_head_stru *rpt_list,
    oal_netbuf_head_stru *w2w_list);
uint32_t hmac_host_rx_mpdu_build_cb(hal_host_device_stru *hal_dev, oal_netbuf_stru *netbuf);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif
