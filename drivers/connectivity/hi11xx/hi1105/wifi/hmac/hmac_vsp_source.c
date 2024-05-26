
#include "hmac_vsp_source.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_VSP_SOURCE_C

#ifdef _PRE_WLAN_FEATURE_VSP
static void hmac_vsp_set_timer_for_tx_slice(hmac_vsp_info_stru *vsp_info, vsp_tx_slice_stru *slice)
{
    /* slice0需要刷新定时参照的vsync */
    if (slice->slice_id == 0) {
        vsp_info->timer_ref_vsync = slice->vsync_ts;
    }

    hmac_vsp_set_timeout_for_slice(slice->slice_id, vsp_info);
}

static void hmac_vsp_source_timeout_handler(hmac_vsp_info_stru *vsp_info)
{
    hmac_tid_info_stru *tid_info = vsp_info->tid_info;
    uintptr_t rptr_addr = tid_info->tx_ring_device_info->word_addr[BYTE_OFFSET_3];

    oam_warning_log0(0, 0, "hmac_vsp_source_timeout_handler::timeout!");
    vsp_info->timer_running = OAL_FALSE;

    if (oal_unlikely(rptr_addr == 0)) {
        return;
    }

    wlan_pm_wakeup_dev_lock();
    tid_info->tx_ring.base_ring_info.read_index = (uint16_t)oal_readl(rptr_addr);

    hmac_vsp_set_evt_flag(vsp_info, VSP_SRC_EVT_TX_COMP);
}

static struct {
    vsp_tx_slice_stru *slice;
} g_vsp_tx_pkt_info[4096];

static vsp_tx_slice_stru *hmac_vsp_get_slice_of_packet(uint16_t idx)
{
    return g_vsp_tx_pkt_info[idx].slice;
}

static void hmac_vsp_record_packet_info(uint16_t idx, vsp_tx_slice_stru *s)
{
    g_vsp_tx_pkt_info[idx].slice = s;
    s->pkts_in_ring++;
}

static uint8_t hmac_vsp_get_tx_slice_id(tx_layer_ctrl *layer)
{
    return (layer->qos_type >> 4) & 0xf;
}

static uint8_t hmac_vsp_get_tx_frame_id(tx_layer_ctrl *layer)
{
    return (layer->qos_type >> 8) & VSP_FRAME_ID_MASK;
}

static inline uint8_t hmac_vsp_source_cfg_pkt_tx_succ(tx_layer_ctrl *layer)
{
    uintptr_t msdu_dscr_addr = hmac_vsp_source_get_pkt_msdu_dscr(layer->data_addr);
    hal_tx_msdu_dscr_info_stru msdu_dscr = { 0 };

    hal_vsp_msdu_dscr_info_get((uint8_t *)msdu_dscr_addr, &msdu_dscr);

    return msdu_dscr.tx_count < 120;
}

static inline void hmac_vsp_source_cfg_pkt_tx_succ_proc(hmac_vsp_info_stru *vsp_info, vsp_tx_slice_stru *slice)
{
    hmac_vsp_slice_enqueue(&vsp_info->comp_queue, slice);
}

static inline void hmac_vsp_source_cfg_pkt_retrans(hmac_vsp_info_stru *vsp_info, vsp_tx_slice_stru *slice)
{
    slice->retrans_cnt--;
    hmac_vsp_slice_enqueue_head(&vsp_info->free_queue, slice);
    hmac_vsp_set_evt_flag(vsp_info, VSP_SRC_EVT_NEW_SLICE);
}

static inline void hmac_vsp_source_slice_tx_done(vsp_tx_slice_stru *slice)
{
    oal_dlist_delete_entry(&slice->entry);
    vsp_vcodec_tx_pkt_done(&slice->tx_result);
    oal_free(slice);
}

static inline void hmac_vsp_source_cfg_pkt_tx_fail_proc(hmac_vsp_info_stru *vsp_info, vsp_tx_slice_stru *slice)
{
    if (slice->retrans_cnt) {
        hmac_vsp_source_cfg_pkt_retrans(vsp_info, slice);
    } else {
        hmac_vsp_source_slice_tx_done(slice);
    }
}

static inline void hmac_vsp_source_cfg_pkt_tx_comp_proc(hmac_vsp_info_stru *vsp_info, vsp_tx_slice_stru *slice)
{
    if (oal_likely(hmac_vsp_source_cfg_pkt_tx_succ(slice->head))) {
        hmac_vsp_source_cfg_pkt_tx_succ_proc(vsp_info, slice);
    } else {
        hmac_vsp_source_cfg_pkt_tx_fail_proc(vsp_info, slice);
    }
}

static inline void hmac_vsp_source_data_pkt_tx_comp_proc(hmac_vsp_info_stru *vsp_info, vsp_tx_slice_stru *slice)
{
    hmac_vsp_slice_enqueue(&vsp_info->comp_queue, slice);
    hmac_vsp_set_evt_flag(vsp_info, VSP_SRC_EVT_NEW_SLICE);
}

static void hmac_vsp_slice_tx_complete_proc(hmac_vsp_info_stru *vsp_info, vsp_tx_slice_stru *slice)
{
    if (vsp_info->timer_running) {
        oam_warning_log2(0, 0, "hmac_vsp_slice_tx_complete_proc::frame[%d] slice[%d] stop timer!",
            slice->frame_id, slice->slice_id);
        hmac_vsp_stop_timer(vsp_info);
    }

    if (oal_likely(!hmac_is_vsp_source_cfg_pkt(slice->head))) {
        hmac_vsp_source_data_pkt_tx_comp_proc(vsp_info, slice);
    } else {
        hmac_vsp_source_cfg_pkt_tx_comp_proc(vsp_info, slice);
    }
}

static void hmac_tx_complete_packet_process(hmac_vsp_info_stru *vsp_info, uint16_t idx)
{
    vsp_tx_slice_stru *slice = NULL;
    vsp_tx_slice_stru *dequeue_slice = NULL;

    slice = hmac_vsp_get_slice_of_packet(idx);
    if (!slice) {
        oam_error_log0(0, 0, "{hmac_tx_complete_packet_process::record slice is null!}");
        return;
    }

    if (--slice->pkts_in_ring != 0) {
        return;
    }

    dequeue_slice = hmac_vsp_slice_dequeue(&vsp_info->free_queue);
    if (!dequeue_slice) {
        oam_error_log0(0, 0, "{hmac_vsp_slice_tx_complete_proc::dequeue_slice NULL!}");
    }

    if (dequeue_slice != slice) {
        oam_error_log0(0, 0, "{hmac_vsp_slice_tx_complete_proc::slice mismatch!}");
    }

    hmac_vsp_slice_tx_complete_proc(vsp_info, slice);
}

void hmac_vsp_tx_complete_process(void)
{
    hmac_vsp_info_stru *vsp_info = hmac_get_vsp_source();
    hmac_vsp_set_evt_flag(vsp_info, VSP_SRC_EVT_TX_COMP);
}

static void hmac_vsp_tx_complete_handler(hmac_vsp_info_stru *vsp_info)
{
    hmac_msdu_info_ring_stru *tx_ring = &vsp_info->tid_info->tx_ring;
    un_rw_ptr release_ptr, target_ptr;
    unsigned long flags = 0;

    oal_spin_lock_irq_save(&vsp_info->tx_lock, &flags);

    release_ptr.rw_ptr = tx_ring->release_index;
    target_ptr.rw_ptr = tx_ring->base_ring_info.read_index;
    while (hmac_tx_rw_ptr_compare(release_ptr, target_ptr) == RING_PTR_SMALLER) {
        hmac_tx_complete_packet_process(vsp_info, release_ptr.st_rw_ptr.bit_rw_ptr);

        hmac_tx_reset_msdu_info(tx_ring, release_ptr);
        hmac_tx_msdu_ring_inc_release_ptr(tx_ring);
        release_ptr.rw_ptr = tx_ring->release_index;
    }

    oal_spin_unlock_irq_restore(&vsp_info->tx_lock, &flags);
}


static uint32_t hmac_vsp_tx_set_msdu_info(msdu_info_stru *msdu_info, uint64_t iova, uint16_t msdu_len, uint8_t to_ds)
{
    uint32_t ret;
    uint64_t devva = 0;

    ret = (uint32_t)pcie_if_hostca_to_devva(HCC_EP_WIFI_DEV, iova, &devva);
    if (ret != OAL_SUCC) {
        oam_error_log1(0, OAM_SF_TX, "{hmac_vsp_tx_set_msdu_info::hostca to devva failed[%d]}", ret);
        return ret;
    }

    msdu_info->msdu_addr_lsb = get_low_32_bits(devva);
    msdu_info->msdu_addr_msb = get_high_32_bits(devva);
    msdu_info->msdu_len = msdu_len;
    msdu_info->data_type = DATA_TYPE_ETH;
    if (to_ds) {
        msdu_info->from_ds = 0;
        msdu_info->to_ds = 1;
    } else {
        msdu_info->from_ds = 1;
        msdu_info->to_ds = 0;
    }
    return OAL_SUCC;
}

/* 成功返回ring idx， 失败返回-1 */
static int32_t hmac_vsp_tx_ring_push_msdu(hmac_vsp_info_stru *vsp_info, uint64_t va, uint64_t iova, uint16_t msdu_len)
{
    hmac_msdu_info_ring_stru *tx_msdu_ring = &vsp_info->tid_info->tx_ring;
    un_rw_ptr write_ptr = { .rw_ptr = tx_msdu_ring->base_ring_info.write_index };
    msdu_info_stru *ring_msdu_info = NULL;
    uint8_t to_ds;
    if (hmac_tid_ring_full(tx_msdu_ring)) {
        return -1;
    }
    to_ds = vsp_info->hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_STA;
    ring_msdu_info = hmac_tx_get_ring_msdu_info(tx_msdu_ring, write_ptr.st_rw_ptr.bit_rw_ptr);
    memset_s(ring_msdu_info, sizeof(msdu_info_stru), 0, sizeof(msdu_info_stru));
    if (hmac_vsp_tx_set_msdu_info(ring_msdu_info, iova, msdu_len, to_ds) != OAL_SUCC) {
        return -1;
    }
    memset_s((uint8_t *)va, HAL_TX_MSDU_DSCR_LEN, 0, HAL_TX_MSDU_DSCR_LEN);
    hmac_tx_msdu_ring_inc_write_ptr(tx_msdu_ring);
    return write_ptr.st_rw_ptr.bit_rw_ptr;
}


static int32_t hmac_vsp_tx_packet(hmac_vsp_info_stru *vsp_info, uint64_t va, uint64_t iova, uint16_t msdu_len)
{
    hmac_tid_info_stru *hmac_tid_info = vsp_info->tid_info;
    hmac_msdu_info_ring_stru *tx_msdu_ring = &hmac_tid_info->tx_ring;
    int32_t idx;

    if (tx_msdu_ring->host_ring_buf == NULL || tx_msdu_ring->netbuf_list == NULL) {
        oam_error_log2(0, OAM_SF_TX, "{hmac_vsp_tx_slice_paket::ptr NULL host_ring_buf[0x%x] netbuf_list[0x%x]}",
            (uintptr_t)tx_msdu_ring->host_ring_buf, (uintptr_t)tx_msdu_ring->netbuf_list);
        return -1;
    }
    idx = hmac_vsp_tx_ring_push_msdu(vsp_info, va, iova, msdu_len);
    if (idx < 0) {
        oam_warning_log0(0, OAM_SF_TX, "hmac_vsp_tx_slice_paket: tx ring push fail");
        return -1;
    }
    return idx;
}

static void hmac_vsp_source_slice_update_vsync_ts(hmac_vsp_info_stru *vsp_info, vsp_tx_slice_stru *slice)
{
    uint32_t vsync = vsp_info->last_vsync;
    uint32_t now;

    if (oal_unlikely(hmac_vsp_calc_runtime(vsync, slice->arrive_ts) > 2 * vsp_info->param.slice_interval_us)) {
        oam_warning_log2(0, 0, "{hmac_vsp_build_tx_slice_struct::slice maybe late, vsync %d, arrive %d}",
            vsync, slice->arrive_ts);
    }

    now = hmac_vsp_get_timestamp(vsp_info);
    if (oal_unlikely(hmac_vsp_calc_runtime(vsync, now) < vsp_info->param.slice_interval_us)) {
        vsync -= vsp_info->param.vsync_interval_us;
        oam_warning_log2(0, 0, "{hmac_vsp_build_tx_slice_struct::vsync[%u] is too close to now[%u], "
            "should adjust to prev vsync}", vsync, now);
        /* 单sice场景会出现slice0到驱动时，下一帧的vsync已经产生。多slice场景为异常 */
        if (vsp_info->param.slices_per_frm != 1) {
            oam_error_log1(0, 0, "{hmac_vsp_build_tx_slice_struct::should not happend when [%d] slices per frame!}",
                vsp_info->param.slices_per_frm);
        }
    }

    slice->vsync_ts = vsync;
    vsp_info->pkt_ts_vsync = vsync;
}

static inline void hmac_vsp_source_cfg_pkt_update_tx_slice(vsp_tx_slice_stru *slice)
{
    slice->retrans_cnt = 3;
    hmac_vsp_source_set_tx_result_cfg_pkt(slice);
}

static void hmac_vsp_source_init_tx_slice(vsp_tx_slice_stru *slice, tx_layer_ctrl *layer, hmac_vsp_info_stru *vsp_info)
{
    slice->arrive_ts = hmac_vsp_get_timestamp(vsp_info);
    slice->head = layer;
    slice->curr = layer;
    slice->tx_pos = 0;
    slice->pkts_in_ring = 0;
    slice->frame_id = hmac_vsp_get_tx_frame_id(layer);
    slice->slice_id = hmac_vsp_get_tx_slice_id(layer);
    slice->feedback = OAL_FALSE;
    slice->is_rpt = OAL_FALSE;
    slice->retrans_cnt = 0;

    if (slice->slice_id == 0) {
        hmac_vsp_source_slice_update_vsync_ts(vsp_info, slice);
    }

    hmac_vsp_source_init_slice_tx_result(slice, layer);

    if (oal_likely(!hmac_is_vsp_source_cfg_pkt(layer))) {
        hmac_vsp_source_set_vsp_hdr_cfg_pkt(slice, OAL_FALSE);
    } else {
        hmac_vsp_source_cfg_pkt_update_tx_slice(slice);
        hmac_vsp_source_set_vsp_hdr_cfg_pkt(slice, OAL_TRUE);
    }
}

static inline void hmac_tx_sched_trigger(hmac_vsp_info_stru *vsp_info)
{
    if (hmac_tx_ring_soft_irq_sched_enabled()) {
        /* WCPU软中断调度 */
        oal_writel(1, (uintptr_t)vsp_info->host_hal_device->soc_regs.w_soft_int_set_aon_reg);
    } else {
        /* 下发事件调度 */
        hmac_tx_sync_ring_info(vsp_info->tid_info, TID_CMDID_ENQUE);
    }
}

// 发送slice中指定数量的packet
static uint16_t hmac_vsp_slice_send_packets(hmac_vsp_info_stru *vsp_info, vsp_tx_slice_stru *slice, uint16_t pkt_cnt)
{
    uint16_t pkt_len, send_pkts;
    tx_layer_ctrl *curr_layer = NULL;
    uint64_t buffer_iova, buffer_va;
    uint32_t buffer_offset;
    unsigned long flags = 0;
    int32_t idx;

    send_pkts = 0;
    while (send_pkts < pkt_cnt && (curr_layer = slice->curr) != NULL) {
        if (curr_layer->normal_pack_length < 96 || curr_layer->last_paket_len < 96) {
            slice->curr = NULL;
            oam_warning_log2(0, 0, "{hmac_vsp_slice_send_packets::normal packet_length %d or last len %d is invalid}",
                curr_layer->normal_pack_length, curr_layer->last_paket_len);
            return send_pkts;
        }
        buffer_offset = slice->tx_pos * curr_layer->normal_pack_length;
        buffer_iova = curr_layer->iova_addr;
        buffer_va = curr_layer->data_addr;
        for (; slice->tx_pos < curr_layer->paket_number && send_pkts < pkt_cnt; slice->tx_pos++, send_pkts++) {
            if (slice->tx_pos != curr_layer->paket_number - 1) {
                pkt_len = curr_layer->normal_pack_length;
            } else {
                pkt_len = curr_layer->last_paket_len;
            }

            oal_spin_lock_irq_save(&vsp_info->tx_lock, &flags);
            idx = hmac_vsp_tx_packet(vsp_info, buffer_va + buffer_offset + VSP_MSDU_CB_LEN - HAL_TX_MSDU_DSCR_LEN,
                buffer_iova + buffer_offset + VSP_MSDU_CB_LEN - HAL_TX_MSDU_DSCR_LEN,
                pkt_len - VSP_MSDU_CB_LEN);
            if (idx < 0) {
                oal_spin_unlock_irq_restore(&vsp_info->tx_lock, &flags);
                oam_error_log0(0, OAM_SF_ANY, "hmac_vsp_tx_slice::set msdu info error");
                return send_pkts;
            }
            hmac_vsp_record_packet_info((uint16_t)idx, slice);
            buffer_offset += pkt_len;
            /* 挂接Ring发送 */
            hmac_tx_reg_write_ring_info(vsp_info->tid_info, TID_CMDID_ENQUE);
            oal_spin_unlock_irq_restore(&vsp_info->tx_lock, &flags);
            hmac_tx_sched_trigger(vsp_info);
            hmac_vsp_source_update_bytes_total(vsp_info, pkt_len);
        }

        if (slice->tx_pos == curr_layer->paket_number) {
            slice->curr = (tx_layer_ctrl*)curr_layer->next;
            slice->tx_pos = 0;
        }
    }

    return send_pkts;
}

static void hmac_vsp_source_update_pkt_timestamp(hmac_vsp_info_stru *vsp_info, vsp_msdu_hdr_stru *vsp_hdr)
{
    if (oal_unlikely(vsp_hdr->ts_type != VSP_TIMESTAMP_NOT_USED)) {
        oam_error_log1(0, 0, "{hmac_vsp_write_timestamp::unexpected timestamp type[%d]!}", vsp_hdr->ts_type);
        return;
    }

    vsp_hdr->timestamp = vsp_info->pkt_ts_vsync;
    vsp_hdr->ts_type = VSP_TIMESTAMP_WIFI;
}

static void hmac_vsp_source_update_slice_timestamp(hmac_vsp_info_stru *vsp_info, vsp_tx_slice_stru *slice)
{
    tx_layer_ctrl *layer = slice->head;
    uintptr_t va;
    uint16_t idx;

    while (layer) {
        for (idx = 0; idx < layer->paket_number; idx++) {
            va = hmac_vsp_source_get_pkt_vsp_hdr(layer->data_addr + idx * layer->normal_pack_length);
            hmac_vsp_source_update_pkt_timestamp(vsp_info, (vsp_msdu_hdr_stru *)va);
        }
        layer = layer->next;
    }
}

static uint32_t hmac_vsp_tx_slice(hmac_vsp_info_stru *vsp_info, tx_layer_ctrl *layer_head)
{
    vsp_tx_slice_stru *slice = oal_memalloc(sizeof(vsp_tx_slice_stru));

    if (!slice) {
        oam_error_log0(0, 0, "hmac_vsp_tx_slice::alloc slice queue entry failed");
        return OAL_FAIL;
    }

    /* vsp simulate vsync, del after vsync intr enabled */
    if (hmac_vsp_get_tx_slice_id(layer_head) == 0) {
        vsp_info->last_vsync = hmac_vsp_get_timestamp(vsp_info) - (vsp_info->param.slice_interval_us + 1000);
    }

    hmac_vsp_source_inc_slice_total(vsp_info);
    hmac_vsp_source_init_tx_slice(slice, layer_head, vsp_info);
    hmac_vsp_source_update_slice_timestamp(vsp_info, slice);
    hmac_vsp_slice_enqueue(&vsp_info->free_queue, slice);

    hmac_vsp_set_evt_flag(vsp_info, VSP_SRC_EVT_NEW_SLICE);

    return OAL_SUCC;
}

bool wifi_tx_venc_pkg(tx_layer_ctrl *layer_list)
{
    hmac_vsp_info_stru *vsp_info = hmac_get_vsp_source();
    mac_user_stru *mac_user;

    if (oal_unlikely(!vsp_info || !vsp_info->enable || !vsp_info->hmac_user)) {
        oam_error_log0(0, 0, "{wifi_tx_venc_pkg::vsp not enable, cannot send packet!}");
        return OAL_FALSE;
    }

    mac_user = hmac_vsp_info_get_mac_user(vsp_info);
    if (oal_unlikely(oal_compare_mac_addr(layer_list->mac_ra_address, mac_user->auc_user_mac_addr))) {
        oam_error_log4(0, 0, "{wifi_tx_venc_pkg::slice ra %02x:%02x, user mac %02x:%02x mismatch!}",
            layer_list->mac_ra_address[MAC_ADDR_4], layer_list->mac_ra_address[MAC_ADDR_5],
            mac_user->auc_user_mac_addr[MAC_ADDR_4], mac_user->auc_user_mac_addr[MAC_ADDR_5]);
        return OAL_FALSE;
    }

    return hmac_vsp_tx_slice(vsp_info, layer_list) == OAL_SUCC;
}

static uint64_t g_tx_sync_stage_addr;
uint32_t hmac_set_vsp_info_addr(frw_event_mem_stru *frw_mem)
{
    frw_event_stru *frw_event = NULL;
    mac_d2h_device_vspinfo_addr *d2h_device_vspinfo_addr = NULL;
    uint32_t ret;

    frw_event = frw_get_event_stru(frw_mem);
    d2h_device_vspinfo_addr = (mac_d2h_device_vspinfo_addr *)frw_event->auc_event_data;
    ret = (uint32_t)oal_pcie_devca_to_hostva(HCC_EP_WIFI_DEV, d2h_device_vspinfo_addr->device_vspinfo_addr,
        &g_tx_sync_stage_addr);
    if (ret != OAL_SUCC) {
        oam_error_log0(0, 0, "devca to hostca fail");
        return ret;
    }
    return OAL_SUCC;
}

static uint32_t hmac_vsp_clear_tx_ring_sync(hmac_vsp_info_stru *vsp_info)
{
    uint32_t sync_res;
    uint32_t check_cnt = 1001;
    oal_spin_lock(&vsp_info->tx_lock);
    oal_writel(VSP_TX_SYNC_LOCK_ACQUIRE, g_tx_sync_stage_addr);
    do {
        oal_udelay(1);
        check_cnt--;
    } while ((sync_res = oal_readl(g_tx_sync_stage_addr)) == VSP_TX_SYNC_LOCK_ACQUIRE && check_cnt);
    oal_writel(VSP_TX_SYNC_LOCK_RELEASE, g_tx_sync_stage_addr);
    oal_spin_unlock(&vsp_info->tx_lock);
    oam_warning_log2(0, 0, "{hmac_vsp_clear_tx_ring_sync::sync res %d, check %d}", sync_res, check_cnt);
    if (sync_res == VSP_TX_SYNC_CLEAR_DONE) {
        return OAL_SUCC;
    }
    return OAL_FAIL;
}

void hmac_vsp_timer_intr_callback(void *data)
{
    hmac_vsp_info_stru *vsp_info = hmac_get_vsp_source();
    bool change;

    hmac_vsp_clear_tx_ring_sync(vsp_info);
    change = hmac_vsp_set_evt_flag(vsp_info, VSP_SRC_EVT_TIMEOUT);
    if (oal_unlikely(!change)) {
        oam_error_log0(0, 0, "{hmac_vsp_timer_intr_callback::last timeout event not process yet!!}");
    }
}

static uint32_t hmac_vsp_fill_feedback_info(
    hmac_vsp_info_stru *vsp_info, vsp_tx_slice_stru *slice, vsp_feedback_frame *fb, uint32_t len)
{
    int layer_idx;
    if (slice->feedback) {
        oam_warning_log2(0, 0, "{hmac_vsp_source_rx_process::frm %d, slice %d, dup feedback!}",
            fb->frm_id, fb->slice_id);
        return OAL_FAIL;
    }
    slice->feedback = OAL_TRUE;
    if (len < sizeof(vsp_feedback_frame) + VSP_LAYER_FEEDBACK_SIZE * fb->layer_num) {
        oam_error_log1(0, 0, "{hmac_vsp_source_rx_process::frame is too short for feedback pkt, len %d!}", len);
        return OAL_FAIL;
    }
    /* 每个layer 2字节记录发送情况 */
    if (memcpy_s(slice->tx_result.layer_suc_num, MAX_LAYER_NUM * VSP_LAYER_FEEDBACK_SIZE,
        fb->layer_succ_num, fb->layer_num * VSP_LAYER_FEEDBACK_SIZE) != EOK) {
        oam_error_log1(0, 0, "{hmac_vsp_source_rx_process::memcpy failed, layer num %d}", fb->layer_num);
        return OAL_FAIL;
    }

    for (layer_idx = 0; layer_idx < MAX_LAYER_NUM; layer_idx++) {
        if ((slice->tx_result.layer_suc_num[layer_idx] & BIT15) == 0) {
            break;
        }
    }
    if (layer_idx != 0) {
        vsp_info->source_stat.feedback_detail[layer_idx - 1]++;
    }

    return OAL_SUCC;
}

typedef struct {
    hmac_vsp_info_stru *vsp_info;
    vsp_feedback_frame *fb;
    uint32_t fb_len;
} hmac_vsp_feedback_context_stru;

static inline uint8_t hmac_vsp_source_fill_feedback_info(vsp_tx_slice_stru *slice, void *param)
{
    hmac_vsp_feedback_context_stru *context = (hmac_vsp_feedback_context_stru *)param;

    if (slice->frame_id == context->fb->frm_id || slice->slice_id == context->fb->slice_id) {
        hmac_vsp_fill_feedback_info(context->vsp_info, slice, context->fb, context->fb_len);
        oam_error_log2(0, 0, "hmac_vsp_source_fill_feedback_info::frame[%d] slice[%d] receive feedback!",
            slice->frame_id, slice->slice_id);
        return OAL_RETURN;
    }

    return OAL_CONTINUE;
}

static inline void hmac_vsp_source_foreach_fill_feedback_info(
    hmac_vsp_info_stru *vsp_info, hmac_vsp_feedback_context_stru *context)
{
    /* search free queue & comp queue */
    hmac_vsp_slice_queue_foreach_safe(&vsp_info->comp_queue, hmac_vsp_source_fill_feedback_info, (void *)context);
}

static inline uint8_t hmac_vsp_source_check_feedback_timeout(hmac_vsp_info_stru *vsp_info, vsp_tx_slice_stru *slice)
{
    if (hmac_vsp_calc_runtime(slice->arrive_ts, hmac_vsp_get_timestamp(vsp_info)) >= vsp_info->param.feedback_dly_us) {
        oam_error_log2(0, 0, "{hmac_vsp_source_check_feedback_timeout::frame[%d] slice[%d] feedback timeout!}",
            slice->frame_id, slice->slice_id);
        return OAL_TRUE;
    }

    return OAL_FALSE;
}

static inline uint8_t hmac_vsp_source_cfg_pkt_feedback_fail(vsp_tx_slice_stru *slice)
{
    return hmac_is_vsp_source_cfg_pkt(slice->head) && !hmac_vsp_source_get_tx_result_layer_succ_num(slice, 0);
}

static inline uint8_t hmac_vsp_rpt_slice(vsp_tx_slice_stru *slice, void *param)
{
    hmac_vsp_info_stru *vsp_info = param;

    if (!slice->feedback && !hmac_vsp_source_check_feedback_timeout(vsp_info, slice)) {
        return OAL_CONTINUE;
    }

    if (oal_likely(!hmac_vsp_source_cfg_pkt_feedback_fail(slice))) {
        hmac_vsp_source_slice_tx_done(slice);
    } else {
        hmac_vsp_source_cfg_pkt_tx_fail_proc(vsp_info, slice);
    }

    return OAL_CONTINUE;
}

static inline void hmac_vsp_source_foreach_rpt_slice(hmac_vsp_info_stru *vsp_info)
{
    hmac_vsp_slice_queue_foreach_safe(&vsp_info->comp_queue, hmac_vsp_rpt_slice, vsp_info);
}

static void hmac_vsp_process_one_feedback(hmac_vsp_info_stru *vsp_info, oal_netbuf_stru *netbuf)
{
    hmac_vsp_feedback_context_stru context = {
        .vsp_info = vsp_info,
        .fb = (vsp_feedback_frame *)oal_netbuf_data(netbuf),
        .fb_len = oal_netbuf_len(netbuf),
    };
    if (oal_unlikely(context.fb_len < sizeof(vsp_feedback_frame))) {
        oal_netbuf_free(netbuf);
        oam_error_log1(0, 0, "{hmac_vsp_process_one_feedback::frame len[%d] too short!}", context.fb_len);
        return;
    }

    hmac_vsp_source_foreach_fill_feedback_info(vsp_info, &context);
    hmac_vsp_source_foreach_rpt_slice(vsp_info);

    oal_netbuf_free(netbuf);
}

static void hmac_vsp_feedback_handler(hmac_vsp_info_stru *vsp_info)
{
    oal_netbuf_stru *netbuf = NULL;
    uint32_t qlen = oal_netbuf_list_len(&vsp_info->feedback_q);
    if (qlen > 1) {
        oam_warning_log1(0, 0, "{hmac_vsp_feedback_handler::feedback queue has %d buff!}", qlen);
    }
    while ((netbuf = oal_netbuf_delist(&vsp_info->feedback_q)) != NULL) {
        hmac_vsp_process_one_feedback(vsp_info, netbuf);
        vsp_info->source_stat.feedback_cnt++;
    }
}

void hmac_vsp_source_rx_amsdu_proc(hmac_vsp_info_stru *vsp_info, oal_netbuf_stru *netbuf)
{
    oal_netbuf_add_to_list_tail(netbuf, &vsp_info->feedback_q);
    hmac_vsp_set_evt_flag(vsp_info, VSP_SRC_EVT_RX_FEEDBACK);
}

void hmac_vsp_tx_sched(void)
{
    hmac_vsp_info_stru *vsp_info = hmac_get_vsp_source();
    vsp_tx_slice_stru *slice = hmac_vsp_slice_queue_peek(&vsp_info->free_queue);

    if (!slice) {
        oam_error_log0(0, 0, "hmac_vsp_tx_sched::slice NULL!");
        return;
    }

    hmac_vsp_set_timer_for_tx_slice(vsp_info, slice);
    hmac_vsp_slice_send_packets(vsp_info, slice, 0xffff);
}

static void hmac_vsp_new_slice_handler(hmac_vsp_info_stru *vsp_info)
{
    /* 新的slice到来时加速上报已超时的slice, 避免未收到feedback导致slice堆积、vdec无内存可用 */
    hmac_vsp_source_foreach_rpt_slice(vsp_info);

    if (!hmac_vsp_tid_ring_empty(vsp_info) || hmac_vsp_slice_queue_empty(&vsp_info->free_queue)) {
        return;
    }

    hmac_vsp_tx_sched();
}

bool hmac_vsp_set_evt_flag(hmac_vsp_info_stru *vsp_info, enum vsp_source_event evt)
{
#if _PRE_OS_VERSION_LINUX == _PRE_OS_VERSION
    bool chg = !test_and_set_bit(evt, &vsp_info->evt_flags);
    oal_wait_queue_wake_up_interrupt(&vsp_info->evt_wq);
    return chg;
#else
    return OAL_TRUE;
#endif
}

static void (*g_event_handler[VSP_SRC_EVT_BUTT])(hmac_vsp_info_stru *) = {
    [VSP_SRC_EVT_TIMEOUT] = hmac_vsp_source_timeout_handler,
    [VSP_SRC_EVT_TX_COMP] = hmac_vsp_tx_complete_handler,
    [VSP_SRC_EVT_NEW_SLICE] = hmac_vsp_new_slice_handler,
    [VSP_SRC_EVT_RX_FEEDBACK] = hmac_vsp_feedback_handler,
};

int hmac_vsp_source_evt_loop(void *data)
{
#if _PRE_OS_VERSION_LINUX == _PRE_OS_VERSION
    int32_t ret;
    uint32_t i;
    hmac_vsp_info_stru *vsp_info = hmac_get_vsp_source();

    allow_signal(SIGTERM);
    while (!oal_kthread_should_stop()) {
        ret = oal_wait_event_interruptible_m(vsp_info->evt_wq, READ_ONCE(vsp_info->evt_flags));
        if (ret) {
            oam_error_log1(0, 0, "{hmac_vsp_source_evt_loop::interrupt by a signal, ret %d}", ret);
            break;
        }
        for (i = 0; i < VSP_SRC_EVT_BUTT; i++) {
            if (test_and_clear_bit(i, &vsp_info->evt_flags)) {
                g_event_handler[i](vsp_info);
            }
        }
    }
#endif
    return 0;
}

#endif
