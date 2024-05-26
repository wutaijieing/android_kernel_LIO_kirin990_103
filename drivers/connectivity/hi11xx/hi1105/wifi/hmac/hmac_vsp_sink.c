

#include "hmac_vsp.h"
#include "hmac_vsp_sink.h"
#include "hmac_host_rx.h"
#include "hmac_user.h"
#include "hmac_rx_data.h"
#include "host_hal_device.h"
#include "host_hal_dscr.h"
#include "host_hal_ext_if.h"
#include "mac_mib.h"
#include "mac_data.h"
#include "oal_net.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_VSP_SINK_C

#ifdef _PRE_WLAN_FEATURE_VSP
static void hmac_vsp_sink_init_rx_slice_priv(
    vsp_rx_slice_priv_stru *priv, hmac_vsp_info_stru *vsp_info, vsp_msdu_hdr_stru *vsp_hdr, rx_slice_mgmt *rx_slice)
{
    memset_s(priv, sizeof(vsp_rx_slice_priv_stru), 0, sizeof(vsp_rx_slice_priv_stru));
    oal_netbuf_head_init(&priv->rx_queue);
    priv->frm_id = vsp_hdr->frame_id;
    priv->slice_id = vsp_hdr->slice_num;
    priv->owner = vsp_info;
    priv->alloc_ts = hmac_vsp_get_timestamp(vsp_info);
    priv->vsync_vld = vsp_hdr->ts_type == VSP_TIMESTAMP_WIFI;
    priv->vsync_ts = vsp_hdr->timestamp;

    rx_slice->priv[0] = priv;
}

static rx_slice_mgmt *hmac_vsp_sink_alloc_rx_slice(hmac_vsp_info_stru *vsp_info, vsp_msdu_hdr_stru *vsp_hdr)
{
    rx_slice_mgmt *rx_slice = vsp_vcodec_alloc_slice_mgmt(sizeof(rx_slice_mgmt));
    vsp_rx_slice_priv_stru *priv = NULL;

    if (!rx_slice) {
        oam_error_log0(0, 0, "{hmac_vsp_alloc_rx_slice_mgmt::slice buffer alloc failed!}");
        return NULL;
    }

    memset_s(rx_slice, sizeof(rx_slice_mgmt), 0, sizeof(rx_slice_mgmt));

    /* 改为静态分配? */
    priv = oal_memalloc(sizeof(vsp_rx_slice_priv_stru));
    if (!priv) {
        oam_error_log0(0, 0, "{hmac_vsp_alloc_rx_slice_mgmt::alloc private data failed!}");
        return rx_slice;
    }

    hmac_vsp_sink_init_rx_slice_priv(priv, vsp_info, vsp_hdr, rx_slice);
    hmac_vsp_sink_inc_slice_total(vsp_info);

    oam_warning_log0(0, 0, "{hmac_vsp_alloc_rx_slice_mgmt::frame[%d] slice[%d] alloc new slice mgmt}");

    return rx_slice;
}

/* 没有收到同步的vsync时间戳时，估算当前接收帧的vsync。 */
OAL_STATIC uint32_t hmac_vsp_rx_estimate_vsync(hmac_vsp_info_stru *vsp_info, uint8_t slice_id)
{
    uint32_t runtime = hmac_vsp_calc_runtime(vsp_info->timer_ref_vsync, hmac_vsp_get_timestamp(vsp_info));
    uint32_t rmd = runtime % vsp_info->param.vsync_interval_us;
    uint32_t dly_rmd = vsp_info->param.max_transmit_dly_us + vsp_info->param.slice_interval_us * slice_id %
        vsp_info->param.vsync_interval_us;
    uint32_t adjust_count = runtime / vsp_info->param.vsync_interval_us;
    if (rmd < dly_rmd && adjust_count > 0) {
        adjust_count -= 1;
    }
    if (adjust_count == 0) {
        adjust_count = 1;
    }
    return vsp_info->timer_ref_vsync + adjust_count * vsp_info->param.vsync_interval_us;
}

static inline uint8_t hmac_vsp_sink_rx_pkt_len_invalid(vsp_msdu_hdr_stru *vsp_hdr, oal_netbuf_stru *netbuf)
{
    return oal_netbuf_len(netbuf) < sizeof(vsp_msdu_hdr_stru) + vsp_hdr->len;
}

static inline uint8_t hmac_vsp_sink_rx_pkt_hdr_invalid(vsp_msdu_hdr_stru *vsp_hdr, hmac_vsp_info_stru *vsp_info)
{
    return vsp_hdr->number >= MAX_PAKET_NUM || vsp_hdr->layer_num >= MAX_LAYER_NUM ||
           vsp_hdr->slice_num >= vsp_info->param.slices_per_frm;
}

static inline uint8_t hmac_vsp_sink_rx_expired_pkt(hmac_vsp_info_stru *vsp_info, vsp_msdu_hdr_stru *vsp_hdr)
{
    if (oal_unlikely(hmac_vsp_info_last_rx_comp_info_invalid(vsp_info))) {
        return OAL_FALSE;
    }

    return (vsp_info->last_rx_comp_frm == vsp_hdr->frame_id) && (vsp_info->last_rx_comp_slice == vsp_hdr->slice_num);
}

static uint8_t hmac_vsp_sink_rx_pkt_invalid(
    hmac_vsp_info_stru *vsp_info, vsp_msdu_hdr_stru *vsp_hdr, oal_netbuf_stru *netbuf)
{
    return hmac_vsp_sink_rx_pkt_len_invalid(vsp_hdr, netbuf) || hmac_vsp_sink_rx_pkt_hdr_invalid(vsp_hdr, vsp_info);
}

static inline void hmac_vsp_sink_init_timer_conext(hmac_vsp_info_stru *vsp_info, vsp_rx_slice_priv_stru *priv)
{
    vsp_info->next_timeout_frm = priv->frm_id;
    vsp_info->next_timeout_slice = priv->slice_id;
    vsp_info->timer_ref_vsync = priv->vsync_vld ? priv->vsync_ts : hmac_vsp_rx_estimate_vsync(vsp_info, priv->slice_id);

    oam_warning_log4(0, 0, "{hmac_vsp_sink_init_timer_conext::next_timeout frame[%d] slice[%d] vsync[%d] valid[%d]}",
        vsp_info->next_timeout_frm, vsp_info->next_timeout_slice, vsp_info->timer_ref_vsync, priv->vsync_vld);
}

static inline void hmac_vsp_sink_init_rx_slice(hmac_vsp_info_stru *vsp_info, vsp_rx_slice_priv_stru *priv)
{
    hmac_vsp_sink_init_timer_conext(vsp_info, priv);
    if (vsp_info->timer_running) {
        oam_warning_log0(0, 0, "{hmac_vsp_sink_init_rx_slice::timer stop}");
        hmac_vsp_stop_timer(vsp_info);
    }
    hmac_vsp_set_timeout_for_slice(priv->slice_id, vsp_info);
}

static inline uint32_t hmac_vsp_sink_get_pkt_rx_slice_index(hmac_vsp_info_stru *vsp_info, vsp_msdu_hdr_stru *vsp_hdr)
{
    return vsp_hdr->frame_id * vsp_info->param.slices_per_frm + vsp_hdr->slice_num;
}

static rx_slice_mgmt *hmac_vsp_sink_get_pkt_rx_slice(hmac_vsp_info_stru *vsp_info, vsp_msdu_hdr_stru *vsp_hdr)
{
    uint32_t index = hmac_vsp_sink_get_pkt_rx_slice_index(vsp_info, vsp_hdr);
    rx_slice_mgmt *rx_slice = hmac_vsp_sink_get_rx_slice_from_tbl(vsp_info, index);

    if (oal_unlikely(!rx_slice)) {
        rx_slice = hmac_vsp_sink_alloc_rx_slice(vsp_info, vsp_hdr);
        if (rx_slice) {
            hmac_vsp_sink_set_rx_slice_to_tbl(vsp_info, index, rx_slice);
            hmac_vsp_sink_init_rx_slice(vsp_info, hmac_vsp_sink_get_rx_slice_priv(rx_slice));
        }
    }

    return rx_slice;
}

static inline void hmac_vsp_sink_update_rx_slice_last_pkt_info(
    rx_slice_mgmt *rx_slice, vsp_rx_slice_priv_stru *priv, vsp_msdu_hdr_stru *vsp_hdr)
{
    if (oal_likely(!vsp_hdr->last)) {
        return;
    }

    priv->total_pkts[vsp_hdr->layer_num] = vsp_hdr->number + 1;
    rx_slice->last_len[vsp_hdr->layer_num] = vsp_hdr->len;
}

static uint32_t hmac_vsp_sink_update_rx_slice_pkt_info(
    rx_slice_mgmt *rx_slice, vsp_rx_slice_priv_stru *priv, vsp_msdu_hdr_stru *vsp_hdr)
{
    if (oal_unlikely(hmac_vsp_sink_get_rx_slice_pkt_addr(rx_slice, vsp_hdr->layer_num, vsp_hdr->number))) {
        oam_warning_log3(0, OAM_SF_ANY, "{hmac_vsp_rx_slice_process::slice[%d] layer[%d] paket[%d] DUPLICATE!}",
            vsp_hdr->slice_num, vsp_hdr->layer_num, vsp_hdr->number);
        return OAL_FAIL;
    }

    hmac_vsp_sink_update_rx_slice_pkt_addr(rx_slice, vsp_hdr->layer_num, vsp_hdr->number, (uintptr_t)vsp_hdr);
    hmac_vsp_sink_update_rx_slice_last_pkt_info(rx_slice, priv, vsp_hdr);

    return OAL_SUCC;
}

static inline uint8_t hmac_vsp_sink_rx_layer_complete(rx_slice_mgmt *rx_slice, uint8_t layer_id)
{
    vsp_rx_slice_priv_stru *priv = hmac_vsp_sink_get_rx_slice_priv(rx_slice);

    return priv->total_pkts[layer_id] && (priv->total_pkts[layer_id] == rx_slice->packet_num[layer_id]);
}

static uint8_t hmac_vsp_sink_rx_slice_complete(hmac_vsp_info_stru *vsp_info, rx_slice_mgmt *rx_slice)
{
    uint8_t layer_id;

    for (layer_id = 0; layer_id < vsp_info->param.layer_num; layer_id++) {
        if (!hmac_vsp_sink_rx_layer_complete(rx_slice, layer_id)) {
            return OAL_FALSE;
        }
    }

    return OAL_TRUE;
}

#define VSP_FIX_PACKET_LEN (VSP_MSDU_CB_LEN + 1400)
static void hmac_vsp_fill_rx_slice_info(rx_slice_mgmt *rx_slice, hmac_vsp_info_stru *vsp_info)
{
    uint32_t layer_id;
    vsp_rx_slice_priv_stru *priv = hmac_vsp_sink_get_rx_slice_priv(rx_slice);

    for (layer_id = 0; layer_id < MAX_LAYER_NUM; layer_id++) {
        if (!hmac_vsp_sink_rx_layer_complete(rx_slice, layer_id)) {
            break;
        }
        oam_warning_log4(0, 0, "{hmac_vsp_fill_rx_slice_info::frame[%d] slice[%d] layer[%d] rx comp, pktnum[%d]}",
            priv->frm_id, priv->slice_id, layer_id, hmac_vsp_sink_get_layer_rx_succ_num(rx_slice, layer_id));
    }

    vsp_info->sink_stat.rx_detail[layer_id]++;

    for (; layer_id < vsp_info->param.layer_num; layer_id++) {
        oam_warning_log4(0, 0, "{hmac_vsp_fill_rx_slice_info::frame[%d] slice[%d] layer[%d] rx partial, pktnum[%d]}",
            priv->frm_id, priv->slice_id, layer_id, hmac_vsp_sink_get_layer_rx_succ_num(rx_slice, layer_id));
    }

    /* 某个layer没收全，则后续的layer都不需要 */
    for (; layer_id < MAX_LAYER_NUM; layer_id++) {
        hmac_vsp_sink_reset_layer_rx_succ_num(rx_slice, layer_id);
    }

    /* fix_len后续FL配置? */
    rx_slice->fix_len = VSP_FIX_PACKET_LEN;
    rx_slice->payload_offset = sizeof(vsp_msdu_hdr_stru);

    /* 其他参数填写 */
}

static void hmac_vsp_sink_encap_feedback(oal_netbuf_stru *netbuf, rx_slice_mgmt *rx_slice)
{
    vsp_rx_slice_priv_stru *priv = hmac_vsp_sink_get_rx_slice_priv(rx_slice);
    hmac_vsp_info_stru *vsp_info = hmac_vsp_sink_get_rx_slice_vsp_info(rx_slice);
    vsp_feedback_frame *feedback = (vsp_feedback_frame *)oal_netbuf_data(netbuf);
    uint32_t layer_id;

    oal_set_mac_addr(feedback->ra, hmac_vsp_info_get_mac_ra(vsp_info));
    oal_set_mac_addr(feedback->sa, hmac_vsp_info_get_mac_sa(vsp_info));

    feedback->type = oal_host2net_short(0x0800); // 0x88A7 <--- what's this?
    feedback->frm_id = priv->frm_id;
    feedback->slice_id = priv->slice_id;
    feedback->layer_num = vsp_info->param.layer_num;

    oam_warning_log3(0, 0, "{hmac_vsp_sink_encap_feedback::frame[%d] slice[%d] encap feedback, layer num[%d]}",
        feedback->frm_id, feedback->slice_id, feedback->layer_num);

    for (layer_id = 0; layer_id < feedback->layer_num; layer_id++) {
        /* bit15标记layer是不是全部收到了 */
        feedback->layer_succ_num[layer_id] = hmac_vsp_sink_get_layer_rx_succ_num(rx_slice, layer_id);
        feedback->layer_succ_num[layer_id] |= (uint16_t)hmac_vsp_sink_rx_layer_complete(rx_slice, layer_id) << 15;
        oam_warning_log3(0, 0, "{hmac_vsp_sink_encap_feedback::layer[%d] succ_num[%d] rx_comp[%d]}",
            layer_id, hmac_vsp_sink_get_layer_rx_succ_num(rx_slice, layer_id),
            hmac_vsp_sink_rx_layer_complete(rx_slice, layer_id));
    }
}

static void hmac_vsp_sink_set_feedback_cb(oal_netbuf_stru *netbuf, hmac_vsp_info_stru *vsp_info)
{
    mac_tx_ctl_stru *tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(netbuf);

    memset_s(tx_ctl, sizeof(mac_tx_ctl_stru), 0, sizeof(mac_tx_ctl_stru));
    MAC_GET_CB_MPDU_NUM(tx_ctl) = 1;
    MAC_GET_CB_NETBUF_NUM(tx_ctl) = 1;
    MAC_GET_CB_WLAN_FRAME_TYPE(tx_ctl) = WLAN_DATA_BASICTYPE;
    MAC_GET_CB_ACK_POLACY(tx_ctl) = WLAN_TX_NORMAL_ACK;
    MAC_GET_CB_TX_VAP_INDEX(tx_ctl) = hmac_vsp_info_get_vap_id(vsp_info);
    MAC_GET_CB_TX_USER_IDX(tx_ctl) = hmac_vsp_info_get_user_id(vsp_info);
    MAC_GET_CB_WME_TID_TYPE(tx_ctl) = WLAN_TIDNO_VSP;
    MAC_GET_CB_WME_AC_TYPE(tx_ctl) = WLAN_WME_TID_TO_AC(WLAN_TIDNO_VSP);
    MAC_GET_CB_FRAME_TYPE(tx_ctl) = WLAN_CB_FRAME_TYPE_DATA;
    MAC_GET_CB_FRAME_SUBTYPE(tx_ctl) = MAC_DATA_NUM;
}

#define VSP_NETBUF_RESERVE (MAC_80211_QOS_HTC_4ADDR_FRAME_LEN + SNAP_LLC_FRAME_LEN)
static oal_netbuf_stru *hmac_vsp_sink_alloc_feedback_netbuf(hmac_vsp_info_stru *vsp_info)
{
    uint32_t len = hmac_vsp_info_get_feedback_pkt_len(vsp_info);
    oal_netbuf_stru *netbuf = oal_netbuf_alloc(len + VSP_NETBUF_RESERVE, VSP_NETBUF_RESERVE, 4); // 4字节对齐

    if (!netbuf) {
        return NULL;
    }

    oal_netbuf_prev(netbuf) = NULL;
    oal_netbuf_next(netbuf) = NULL;
    oal_netbuf_put(netbuf, len);

    return netbuf;
}

static uint32_t hmac_vsp_sink_send_feedback(rx_slice_mgmt *rx_slice, hmac_vsp_info_stru *vsp_info)
{
    oal_netbuf_stru *netbuf = hmac_vsp_sink_alloc_feedback_netbuf(vsp_info);

    if (!netbuf) {
        oam_error_log0(0, 0, "{hmac_vsp_rx_feedback::netbuf alloc failed!}");
        return OAL_FAIL;
    }

    hmac_vsp_sink_encap_feedback(netbuf, rx_slice);
    hmac_vsp_sink_set_feedback_cb(netbuf, vsp_info);

    if (oal_unlikely(hmac_tx_lan_to_wlan_no_tcp_opt(hmac_vsp_info_get_mac_vap(vsp_info), netbuf) != OAL_SUCC)) {
        oam_error_log0(0, 0, "{hmac_vsp_rx_feedback::send feedback failed!");
        oal_netbuf_free(netbuf);
        return OAL_FAIL;
    }

    oam_warning_log0(0, 0, "{hmac_vsp_rx_feedback::send feedback succ!");

    return OAL_SUCC;
}

static void hmac_vsp_sink_update_last_rx_comp_info(rx_slice_mgmt *rx_slice, hmac_vsp_info_stru *vsp_info)
{
    vsp_rx_slice_priv_stru *priv = hmac_vsp_sink_get_rx_slice_priv(rx_slice);

    vsp_info->last_rx_comp_frm = priv->frm_id;
    vsp_info->last_rx_comp_slice = priv->slice_id;
}

static void hmac_vsp_sink_rx_slice_complete_proc(hmac_vsp_info_stru *vsp_info, rx_slice_mgmt *rx_slice)
{
    if (vsp_info->timer_running) {
        oam_warning_log0(0, 0, "{hmac_vsp_sink_rx_slice_complete_proc::timer stop}");
        hmac_vsp_stop_timer(vsp_info);
    }

    hmac_vsp_fill_rx_slice_info(rx_slice, vsp_info);
    hmac_vsp_sink_send_feedback(rx_slice, vsp_info);

    hmac_vsp_sink_update_last_rx_comp_info(rx_slice, vsp_info);
    hmac_vsp_inc_next_timeout_slice(vsp_info);
    hmac_vsp_sink_reset_rx_slice_in_tbl(vsp_info, hmac_vsp_sink_get_rx_slice_tbl_idx(rx_slice));

    vsp_vcodec_rx_slice_done(rx_slice);
    hmac_vsp_sink_calc_rate(vsp_info);
}

static uint32_t hmac_vsp_sink_update_rx_slice(
    hmac_vsp_info_stru *vsp_info, rx_slice_mgmt *rx_slice, vsp_msdu_hdr_stru *vsp_hdr, oal_netbuf_stru *netbuf)
{
    vsp_rx_slice_priv_stru *priv = hmac_vsp_sink_get_rx_slice_priv(rx_slice);

    if (hmac_vsp_sink_update_rx_slice_pkt_info(rx_slice, priv, vsp_hdr) != OAL_SUCC) {
        return OAL_FAIL;
    }

    hmac_vsp_sink_rx_slice_netbuf_enqueue(rx_slice, netbuf);
    hmac_vsp_sink_update_bytes_total(vsp_info, vsp_hdr->len);

    return OAL_SUCC;
}

static inline uint8_t hmac_vsp_sink_rx_current_slice_pkt(hmac_vsp_info_stru *vsp_info, vsp_msdu_hdr_stru *vsp_hdr)
{
    if (oal_unlikely(hmac_vsp_info_next_timeout_info_invalid(vsp_info))) {
        /* first rx */
        return OAL_TRUE;
    }

    return (vsp_hdr->frame_id == vsp_info->next_timeout_frm) && (vsp_hdr->slice_num == vsp_info->next_timeout_slice);
}

static uint32_t hmac_vsp_sink_rx_current_slice_pkt_proc(
    hmac_vsp_info_stru *vsp_info, vsp_msdu_hdr_stru *vsp_hdr, oal_netbuf_stru *netbuf)
{
    rx_slice_mgmt *rx_slice = hmac_vsp_sink_get_pkt_rx_slice(vsp_info, vsp_hdr);

    if (oal_unlikely(!rx_slice)) {
        return OAL_FAIL;
    }

    if (oal_unlikely(hmac_vsp_sink_update_rx_slice(vsp_info, rx_slice, vsp_hdr, netbuf) != OAL_SUCC)) {
        return OAL_FAIL;
    }

    if (hmac_vsp_sink_rx_slice_complete(vsp_info, rx_slice)) {
        hmac_vsp_sink_rx_slice_complete_proc(vsp_info, rx_slice);
    }

    return OAL_SUCC;
}

void hmac_vsp_sink_rx_slice_partial_complete_proc(hmac_vsp_info_stru *vsp_info)
{
    rx_slice_mgmt *rx_slice = hmac_vsp_sink_get_next_timeout_rx_slice(vsp_info);

    if (!rx_slice) {
        oam_error_log2(0, 0, "{hmac_vsp_sink_partial_comp::rx_slice NULL! next_timeout frame[%d] slice[%d]}",
            vsp_info->next_timeout_frm, vsp_info->next_timeout_slice);
        return;
    }

    hmac_vsp_sink_rx_slice_complete_proc(vsp_info, rx_slice);
}

static uint32_t hmac_vsp_sink_rx_non_current_slice_pkt_proc(
    hmac_vsp_info_stru *vsp_info, vsp_msdu_hdr_stru *vsp_hdr, oal_netbuf_stru *netbuf)
{
    hmac_vsp_sink_rx_slice_partial_complete_proc(vsp_info);

    return hmac_vsp_sink_rx_current_slice_pkt_proc(vsp_info, vsp_hdr, netbuf);
}

static void hmac_vsp_sink_rx_cfg_pkt_complete_proc(hmac_vsp_info_stru *vsp_info, rx_slice_mgmt *rx_slice)
{
    hmac_vsp_fill_rx_slice_info(rx_slice, vsp_info);
    hmac_vsp_sink_send_feedback(rx_slice, vsp_info);

    hmac_vsp_sink_reset_rx_slice_in_tbl(vsp_info, hmac_vsp_sink_get_rx_slice_tbl_idx(rx_slice));
    vsp_vcodec_rx_slice_done(rx_slice);
}

static uint32_t hmac_vsp_sink_rx_cfg_pkt_proc(
    hmac_vsp_info_stru *vsp_info, vsp_msdu_hdr_stru *vsp_hdr, oal_netbuf_stru *netbuf)
{
    rx_slice_mgmt *rx_slice = hmac_vsp_sink_get_pkt_rx_slice(vsp_info, vsp_hdr);

    if (oal_unlikely(!rx_slice)) {
        return OAL_FAIL;
    }

    if (oal_unlikely(hmac_vsp_sink_update_rx_slice(vsp_info, rx_slice, vsp_hdr, netbuf) != OAL_SUCC)) {
        return OAL_FAIL;
    }

    hmac_vsp_sink_rx_cfg_pkt_complete_proc(vsp_info, rx_slice);

    return OAL_SUCC;
}

static uint32_t hmac_vsp_sink_rx_data_pkt_proc(
    hmac_vsp_info_stru *vsp_info, vsp_msdu_hdr_stru *vsp_hdr, oal_netbuf_stru *netbuf)
{
    if (oal_unlikely(hmac_vsp_sink_rx_expired_pkt(vsp_info, vsp_hdr))) {
        return OAL_FAIL;
    }

    if (oal_likely(hmac_vsp_sink_rx_current_slice_pkt(vsp_info, vsp_hdr))) {
        return hmac_vsp_sink_rx_current_slice_pkt_proc(vsp_info, vsp_hdr, netbuf);
    } else {
        oam_warning_log4(0, 0, "{hmac_vsp_sink_rx_msdu_proc::next_timeout frame[%d] slice[%d], rx frame[%d] slice[%d]}",
            vsp_info->next_timeout_frm, vsp_info->next_timeout_slice, vsp_hdr->frame_id, vsp_hdr->slice_num);
        return hmac_vsp_sink_rx_non_current_slice_pkt_proc(vsp_info, vsp_hdr, netbuf);
    }
}

static uint32_t hmac_vsp_sink_rx_msdu_proc(hmac_vsp_info_stru *vsp_info, oal_netbuf_stru *netbuf)
{
    vsp_msdu_hdr_stru *vsp_hdr = (vsp_msdu_hdr_stru *)oal_netbuf_data(netbuf);

    if (oal_unlikely(hmac_vsp_sink_rx_pkt_invalid(vsp_info, vsp_hdr, netbuf))) {
        return OAL_FAIL;
    }

    if (oal_likely(!hmac_is_vsp_sink_cfg_pkt(vsp_hdr))) {
        return hmac_vsp_sink_rx_data_pkt_proc(vsp_info, vsp_hdr, netbuf);
    } else {
        return hmac_vsp_sink_rx_cfg_pkt_proc(vsp_info, vsp_hdr, netbuf);
    }
}

void hmac_vsp_sink_rx_amsdu_proc(hmac_vsp_info_stru *vsp_info, oal_netbuf_stru *netbuf)
{
    oal_netbuf_stru *current_msdu = netbuf;
    oal_netbuf_stru *next_msdu = NULL;

    while (current_msdu) {
        next_msdu = oal_netbuf_next(current_msdu);
        if (hmac_vsp_sink_rx_msdu_proc(vsp_info, current_msdu) != OAL_SUCC) {
            /*
             * 1. rx_slice_mgmt是否需要做相应处理?
             * 2. 此处释放是否合理, 需不需要等vdec处理完当前slice, 再一次性释放所有netbuf?
             */
            oal_netbuf_next(current_msdu) = NULL;
            oal_netbuf_free(current_msdu);
        }
        current_msdu = next_msdu;
    }
}

/* 解码器处理完数据后，通知Wi-Fi */
void vdec_rx_slice_done(rx_slice_mgmt *rx_slice)
{
    if (oal_unlikely(!rx_slice || !hmac_vsp_sink_get_rx_slice_priv(rx_slice))) {
        oam_error_log0(0, 0, "{vdec_rx_slice_done::rx_slice_mgmt/priv NULL}");
        return;
    }

    if (oal_unlikely(!hmac_vsp_sink_get_rx_slice_vsp_info(rx_slice))) {
        oam_error_log0(0, 0, "{vdec_rx_slice_done::vap_info NULL}");
        hmac_vsp_sink_rx_slice_netbuf_queue_purge(rx_slice);
        return;
    }

    hmac_vsp_sink_update_netbuf_recycle_cnt(rx_slice);
    hmac_vsp_sink_deinit_rx_slice_priv(rx_slice);
}

void hmac_vsp_sink_rx_slice_timeout(hmac_vsp_info_stru *vsp_info)
{
    hmac_vsp_info_set_last_timeout_ts(vsp_info, hmac_vsp_get_timestamp(vsp_info));
    hmac_vsp_sink_rx_slice_partial_complete_proc(vsp_info);
}

#endif
