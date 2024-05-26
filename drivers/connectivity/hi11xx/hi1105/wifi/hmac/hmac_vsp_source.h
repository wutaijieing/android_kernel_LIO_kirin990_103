

#ifndef __HMAC_VSP_SOURCE_H__
#define __HMAC_VSP_SOURCE_H__

#if _PRE_OS_VERSION_LINUX == _PRE_OS_VERSION
#include <linux/dma-buf.h>
#endif
#include "hmac_vsp.h"

#ifdef _PRE_WLAN_FEATURE_VSP
#define VSP_RECIEVER_FEEDBACK 0
#define VSP_TRANSMITTER_FEEDBACK 1

enum vsp_source_event {
    VSP_SRC_EVT_TIMEOUT,
    VSP_SRC_EVT_TX_COMP,
    VSP_SRC_EVT_NEW_SLICE,
    VSP_SRC_EVT_RX_FEEDBACK,
    VSP_SRC_EVT_BUTT,
};

void hmac_vsp_source_rx_amsdu_proc(hmac_vsp_info_stru *vsp_info, oal_netbuf_stru *netbuf);
int hmac_vsp_source_evt_loop(void *data);
bool hmac_vsp_set_evt_flag(hmac_vsp_info_stru *vsp_info, enum vsp_source_event evt);
uint32_t hmac_vsp_source_map_dma_buf(struct dma_buf *dmabuf, uintptr_t *iova);
uint32_t hmac_vsp_source_unmap_dma_buf(struct dma_buf *dmabuf);

static inline uint8_t hmac_is_vsp_source(hmac_user_stru *hmac_user)
{
    hmac_vsp_info_stru *vsp_info = hmac_user->vsp_hdl;

    return vsp_info && vsp_info->enable && vsp_info->mode == VSP_MODE_SOURCE;
}

static inline uint8_t hmac_is_vsp_source_tid(hmac_tid_info_stru *tid_info)
{
    hmac_user_stru *hmac_user = mac_res_get_hmac_user(tid_info->user_index);

    return hmac_user && hmac_is_vsp_source(hmac_user) && tid_info->tid_no == WLAN_TIDNO_VSP;
}

static inline uint8_t hmac_is_vsp_source_cfg_pkt(tx_layer_ctrl *layer)
{
    return (layer->qos_type & BIT14) != 0;
}

static inline uint8_t hmac_vsp_source_set_tx_result_cfg_pkt(vsp_tx_slice_stru *slice)
{
    return slice->tx_result.slice_info |= BIT7;
}

static inline uint8_t hmac_vsp_source_get_tx_result_layer_succ_num(vsp_tx_slice_stru *slice, uint8_t layer_id)
{
    return slice->tx_result.layer_suc_num[layer_id];
}

static inline uintptr_t hmac_vsp_source_get_pkt_vsp_hdr(uintptr_t va)
{
    return va + VSP_MSDU_CB_LEN;
}

static inline uintptr_t hmac_vsp_source_get_pkt_msdu_dscr(uintptr_t va)
{
    return hmac_vsp_source_get_pkt_vsp_hdr(va) - HAL_TX_MSDU_DSCR_LEN;
}

static inline void hmac_vsp_source_init_slice_tx_result(vsp_tx_slice_stru *slice, tx_layer_ctrl *layer)
{
    memset_s(&slice->tx_result, sizeof(send_result), 0, sizeof(send_result));
    slice->tx_result.slice_layer = layer;
}

static inline void hmac_vsp_source_set_vsp_hdr_cfg_pkt(vsp_tx_slice_stru *slice, uint8_t cfg_pkt)
{
    vsp_msdu_hdr_stru *vsp_hdr = (vsp_msdu_hdr_stru *)hmac_vsp_source_get_pkt_vsp_hdr(slice->head->data_addr);

    vsp_hdr->cfg_pkt = cfg_pkt;
}

static inline void hmac_vsp_source_inc_slice_total(hmac_vsp_info_stru *vsp_info)
{
    hmac_vsp_stat_inc_slice_total(&vsp_info->source_stat.rate);
}

static inline void hmac_vsp_source_update_bytes_total(hmac_vsp_info_stru *vsp_info, uint64_t bytes)
{
    hmac_vsp_stat_update_bytes_total(&vsp_info->source_stat.rate, bytes);
}

#endif
#endif
