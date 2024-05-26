

#ifndef __HMAC_VSP_SINK_H__
#define __HMAC_VSP_SINK_H__

#include "hmac_vsp.h"

#ifdef _PRE_WLAN_FEATURE_VSP
typedef struct {
    uint8_t frm_id;
    uint8_t slice_id;
    uint32_t tbl_idx;
    uint32_t total_pkts[MAX_LAYER_NUM];
    oal_netbuf_head_stru rx_queue;
    uint32_t alloc_ts; /* 结构体申请的时间点，即第一个数据包收到的时间 */
    uint32_t vsync_ts; /* 发送端同步的vsync时间 */
    uint8_t vsync_vld; /* vsync_ts是否有效 */
    hmac_vsp_info_stru *owner;
} vsp_rx_slice_priv_stru;

void hmac_vsp_sink_rx_amsdu_proc(hmac_vsp_info_stru *vsp_info, oal_netbuf_stru *netbuf);
void hmac_vsp_sink_rx_slice_timeout(hmac_vsp_info_stru *vsp_info);

static inline uint8_t hmac_is_vsp_sink(hmac_user_stru *hmac_user)
{
    hmac_vsp_info_stru *vsp_info = hmac_user->vsp_hdl;

    return vsp_info && vsp_info->enable && vsp_info->mode == VSP_MODE_SINK;
}

static inline uint8_t hmac_is_vsp_sink_tid(hmac_tid_info_stru *tid_info)
{
    hmac_user_stru *hmac_user = mac_res_get_hmac_user(tid_info->user_index);

    return hmac_user && hmac_is_vsp_sink(hmac_user) && tid_info->tid_no == WLAN_TIDNO_VSP;
}

static inline uint8_t hmac_is_vsp_sink_cfg_pkt(vsp_msdu_hdr_stru *vsp_hdr)
{
    return vsp_hdr->cfg_pkt;
}

static inline void hmac_vsp_sink_inc_layer_rx_succ_num(rx_slice_mgmt *rx_slice, uint8_t layer_id)
{
    rx_slice->packet_num[layer_id]++;
}

static inline void hmac_vsp_sink_reset_layer_rx_succ_num(rx_slice_mgmt *rx_slice, uint8_t layer_id)
{
    rx_slice->packet_num[layer_id] = 0;
}

static inline uint32_t hmac_vsp_sink_get_layer_rx_succ_num(rx_slice_mgmt *rx_slice, uint8_t layer_id)
{
    return rx_slice->packet_num[layer_id];
}

static inline vsp_rx_slice_priv_stru *hmac_vsp_sink_get_rx_slice_priv(rx_slice_mgmt *rx_slice)
{
    return (vsp_rx_slice_priv_stru *)rx_slice->priv[0];
}

static inline uint32_t hmac_vsp_sink_get_rx_slice_tbl_idx(rx_slice_mgmt *rx_slice)
{
    return hmac_vsp_sink_get_rx_slice_priv(rx_slice)->tbl_idx;
}

static inline void hmac_vsp_sink_update_rx_slice_tbl_idx(rx_slice_mgmt *rx_slice, uint32_t idx)
{
    hmac_vsp_sink_get_rx_slice_priv(rx_slice)->tbl_idx = idx;
}

static inline hmac_vsp_info_stru *hmac_vsp_sink_get_rx_slice_vsp_info(rx_slice_mgmt *rx_slice)
{
    return hmac_vsp_sink_get_rx_slice_priv(rx_slice)->owner;
}

static inline oal_netbuf_head_stru *hmac_vsp_sink_rx_slice_netbuf_queue(rx_slice_mgmt *rx_slice)
{
    return &hmac_vsp_sink_get_rx_slice_priv(rx_slice)->rx_queue;
}

static inline void hmac_vsp_sink_rx_slice_netbuf_enqueue(rx_slice_mgmt *rx_slice, oal_netbuf_stru *netbuf)
{
    oal_netbuf_addlist(hmac_vsp_sink_rx_slice_netbuf_queue(rx_slice), netbuf);
}

static inline void hmac_vsp_sink_rx_slice_netbuf_queue_purge(rx_slice_mgmt *rx_slice)
{
    oal_netbuf_queue_purge(hmac_vsp_sink_rx_slice_netbuf_queue(rx_slice));
}

static inline void hmac_vsp_sink_update_netbuf_recycle_cnt(rx_slice_mgmt *rx_slice)
{
    hmac_vsp_info_stru *vsp_info = hmac_vsp_sink_get_rx_slice_vsp_info(rx_slice);
    oal_netbuf_head_stru *queue = hmac_vsp_sink_rx_slice_netbuf_queue(rx_slice);

    vsp_info->sink_stat.netbuf_recycle_cnt += hal_host_rx_buff_recycle(vsp_info->host_hal_device, queue);
}

static inline rx_slice_mgmt *hmac_vsp_sink_get_rx_slice_from_tbl(hmac_vsp_info_stru *vsp_info, uint32_t idx)
{
    return vsp_info->rx_slice_tbl[idx];
}

static inline void hmac_vsp_sink_set_rx_slice_to_tbl(hmac_vsp_info_stru *vsp_info, uint32_t idx, rx_slice_mgmt *slice)
{
    vsp_info->rx_slice_tbl[idx] = slice;
    vsp_info->slices_in_tbl++;
    hmac_vsp_sink_update_rx_slice_tbl_idx(slice, idx);
}

static inline void hmac_vsp_sink_reset_rx_slice_in_tbl(hmac_vsp_info_stru *vsp_info, uint32_t idx)
{
    vsp_info->rx_slice_tbl[idx] = NULL;
    vsp_info->slices_in_tbl--;
}

static inline void hmac_vsp_sink_deinit_rx_slice_priv(rx_slice_mgmt *rx_slice)
{
    oal_free(hmac_vsp_sink_get_rx_slice_priv(rx_slice));
}

static inline void hmac_vsp_sink_update_rx_slice_pkt_addr(
    rx_slice_mgmt *rx_slice, uint8_t layer_id, uint32_t pkt_id, uintptr_t addr)
{
    rx_slice->base_addr[layer_id][pkt_id] = addr;
    hmac_vsp_sink_inc_layer_rx_succ_num(rx_slice, layer_id);
}

static inline uintptr_t hmac_vsp_sink_get_rx_slice_pkt_addr(rx_slice_mgmt *rx_slice, uint8_t layer_id, uint32_t pkt_id)
{
    return rx_slice->base_addr[layer_id][pkt_id];
}

static inline uint32_t hmac_vsp_sink_get_next_timeout_rx_slice_index(hmac_vsp_info_stru *vsp_info)
{
    return vsp_info->next_timeout_frm * vsp_info->param.slices_per_frm + vsp_info->next_timeout_slice;
}

static inline rx_slice_mgmt *hmac_vsp_sink_get_next_timeout_rx_slice(hmac_vsp_info_stru *vsp_info)
{
    return hmac_vsp_sink_get_rx_slice_from_tbl(vsp_info, hmac_vsp_sink_get_next_timeout_rx_slice_index(vsp_info));
}

static inline void hmac_vsp_sink_update_bytes_total(hmac_vsp_info_stru *vsp_info, uint32_t bytes)
{
    hmac_vsp_stat_update_bytes_total(&vsp_info->sink_stat.rate, bytes);
}

static inline void hmac_vsp_sink_inc_slice_total(hmac_vsp_info_stru *vsp_info)
{
    hmac_vsp_stat_inc_slice_total(&vsp_info->sink_stat.rate);
}

static inline void hmac_vsp_sink_calc_rate(hmac_vsp_info_stru *vsp_info)
{
    hmac_vsp_stat_calc_rate(&vsp_info->sink_stat.rate);
}

#endif
#endif
