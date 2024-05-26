

/* 1 其他头文件包含 */

#include "hmac_tx_data.h"
#include "hmac_host_tx_data.h"
#include "hmac_mcast_ampdu.h"
#include "hmac_resource.h"
#include "hmac_tx_ring_alloc.h"
#include "hmac_blockack.h"
#include "oam_stat_wifi.h"
#include "hmac_wapi.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_MCAST_AMPDU_C

#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
/* CHBA可靠组播收帧统计信息 */
hmac_mcast_ampdu_rx_stats_stru g_mcast_ampdu_rx_info_stru[WLAN_VAP_SUPPORT_MAX_NUM_LIMIT];

void hmac_mcast_ampdu_netbuf_process(mac_vap_stru *mac_vap, oal_netbuf_stru *netbuf, mac_tx_ctl_stru *tx_ctl,
    mac_ether_header_stru *eth_hdr, oal_bool_enum_uint8 *multicast_need_host_tx)
{
    hmac_user_stru *hmac_user;
    if (mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_AP) {
        return;
    }
    /* 单播数据不处理，走正常发送流程 */
    if (oal_likely(!ether_is_multicast(eth_hdr->auc_ether_dhost))) {
        return;
    }
    /* 设置组播标识位 */
    MAC_GET_CB_IS_MCAST(tx_ctl) = OAL_TRUE;

    /* 更新ACK策略 */
    MAC_GET_CB_ACK_POLACY(tx_ctl) = WLAN_TX_NO_ACK;

    /* 获取组播用户 */
    hmac_user = mac_res_get_hmac_user(mac_vap->us_multi_user_idx);
    if (oal_unlikely(hmac_user == NULL)) {
        oam_warning_log1(mac_vap->uc_vap_id, OAM_SF_TX,
            "{hmac_mcast_ampdu_netbuf_process::get multi user failed[%d].}", mac_vap->us_multi_user_idx);
        oam_stat_vap_incr(mac_vap->uc_vap_id, tx_abnormal_msdu_dropped, 1);
        return;
    }

    MAC_GET_CB_TX_USER_IDX(tx_ctl) = mac_vap->us_multi_user_idx;
    /* 组播数据用tid3发送 */
    MAC_GET_CB_WME_TID_TYPE(tx_ctl) = WLAN_TIDNO_MCAST_AMPDU;
    MAC_GET_CB_WME_AC_TYPE(tx_ctl) = WLAN_WME_TID_TO_AC(WLAN_TIDNO_MCAST_AMPDU);
    /* 设置支持host tx */
    *multicast_need_host_tx = OAL_TRUE;
}

void hmac_mcast_ampdu_user_tid_init(mac_vap_stru *mac_vap, hmac_user_stru *hmac_user, uint8_t *multi_user_lut_idx)
{
    mac_chip_stru *mac_chip;
    if (mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_AP) {
        return;
    }
    if (hmac_ring_tx_enabled() != OAL_TRUE) {
        oam_warning_log0(0, 0, "{hmac_mcast_ampdu_user_tid_init::return due to tx_switch != HOST_TX}");
        return;
    }
    mac_chip = hmac_res_get_mac_chip(mac_vap->uc_chip_id);
    if (mac_chip == NULL) {
        oam_warning_log0(mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_mcast_ampdu_user_tid_init::mac_chip null.}");
        return;
    }
    /* 组播用户也申请lut index, 最大用户数是否需要新增一个？ */
    if (hmac_alloc_user_lut_index(hmac_user, mac_chip, MAC_AP_TYPE_NORMAL) != OAL_SUCC) {
        oam_error_log0(mac_vap->uc_vap_id, OAM_SF_TX,
            "{hmac_mcast_ampdu_user_tid_init::get multi user lut failed.}");
        hmac_user_free(hmac_user->st_user_base_info.us_assoc_id);
        return;
    }
    /* 配置lut index，同步到device */
    *multi_user_lut_idx = hmac_user->lut_index;
    oam_warning_log1(mac_vap->uc_vap_id, OAM_SF_TX,
        "{MCAST AMPDU::hmac_mcast_ampdu_user_tid_init::multi user lut idx = [%d].}", hmac_user->lut_index);
    /* 初始化ring信息 */
    hmac_init_tx_ring(hmac_user, WLAN_TIDNO_MCAST_AMPDU);
    oal_atomic_set(&hmac_user->netdev_tx_suspend, OAL_FALSE);
}

void hmac_mcast_ampdu_rx_win_init(hmac_user_stru *hmac_user, uint8_t tid)
{
    uint16_t index;
    hmac_ba_rx_stru *ba_rx_stru = NULL;
    /* 申请内存 */
    ba_rx_stru = (hmac_ba_rx_stru *)oal_mem_alloc_m(OAL_MEM_POOL_ID_LOCAL, (uint16_t)sizeof(hmac_ba_rx_stru), OAL_TRUE);
    if (ba_rx_stru == NULL) {
        oam_error_log1(0, OAM_SF_BA,
            "{hmac_mcast_ampdu_rx_win_init::alloc fail.tid[%d]}", tid);
        return;
    }

    hmac_user->ast_tid_info[tid].pst_ba_rx_info = ba_rx_stru;
    /* Ba会话参数初始化 */
    ba_rx_stru->en_ba_status = DMAC_BA_COMPLETE;
    ba_rx_stru->uc_ba_policy = MAC_BA_POLICY_IMMEDIATE;
    /* 初始化接收窗口 */
    ba_rx_stru->us_baw_start = 0;
    ba_rx_stru->us_baw_size = WLAN_AMPDU_RX_BUFFER_SIZE;

    /* 1.1 BA接收窗口参数(SSN与窗口大小)由管理面收到BA请求帧后获取并同步到数据面 */
    ba_rx_stru->us_last_relseq = DMAC_BA_SEQNO_SUB(ba_rx_stru->us_baw_start, 1);

    /* Ba会话参数初始化 */
    ba_rx_stru->us_baw_end = DMAC_BA_SEQ_ADD(ba_rx_stru->us_baw_start, (ba_rx_stru->us_baw_size - 1));
    ba_rx_stru->us_baw_tail = DMAC_BA_SEQNO_SUB(ba_rx_stru->us_baw_start, 1);
    ba_rx_stru->us_baw_head = DMAC_BA_SEQNO_SUB(ba_rx_stru->us_baw_start, HMAC_BA_BMP_SIZE);
    ba_rx_stru->uc_mpdu_cnt = 0;

    oal_spin_lock_init(&ba_rx_stru->st_ba_lock);
    oal_atomic_set(&ba_rx_stru->ref_cnt, 1);

    for (index = 0; index < WLAN_AMPDU_RX_HE_BUFFER_SIZE; index++) {
        ba_rx_stru->ast_re_order_list[index].in_use = 0;
        ba_rx_stru->ast_re_order_list[index].us_seq_num = 0;
        ba_rx_stru->ast_re_order_list[index].uc_num_buf  = 0;
        oal_netbuf_list_head_init(&(ba_rx_stru->ast_re_order_list[index].st_netbuf_head));
    }

    ba_rx_stru->en_back_var = MAC_BACK_COMPRESSED;
    ba_rx_stru->puc_transmit_addr = hmac_user->st_user_base_info.auc_user_mac_addr;
    ba_rx_stru->en_is_ba = OAL_TRUE;  // Ba session is processing
    ba_rx_stru->en_amsdu_supp = OAL_FALSE;
    /* 初始化定时器资源 */
    ba_rx_stru->st_alarm_data.pst_ba = ba_rx_stru;
    ba_rx_stru->st_alarm_data.us_mac_user_idx = hmac_user->st_user_base_info.us_assoc_id;
    ba_rx_stru->st_alarm_data.uc_vap_id = hmac_user->st_user_base_info.uc_vap_id;
    ba_rx_stru->st_alarm_data.uc_tid = tid;
    ba_rx_stru->st_alarm_data.us_timeout_times = 0;
    ba_rx_stru->st_alarm_data.en_direction = MAC_RECIPIENT_DELBA;
    ba_rx_stru->en_timer_triggered = OAL_FALSE;

    memset_s(ba_rx_stru->aul_rx_buf_bitmap, sizeof(ba_rx_stru->aul_rx_buf_bitmap),
        0, sizeof(ba_rx_stru->aul_rx_buf_bitmap));
}

void hmac_mcast_ampdu_stats_stru_init(hmac_vap_stru *hmac_vap)
{
    hmac_mcast_ampdu_rx_stats_stru *rx_info = NULL;

    hmac_vap->mcast_ampdu_rx_info = (void *)(&g_mcast_ampdu_rx_info_stru[hmac_vap->st_vap_base_info.uc_vap_id]);
    memset_s((void *)hmac_vap->mcast_ampdu_rx_info, sizeof(hmac_mcast_ampdu_rx_stats_stru),
        0, sizeof(hmac_mcast_ampdu_rx_stats_stru));

    rx_info = (hmac_mcast_ampdu_rx_stats_stru *)hmac_vap->mcast_ampdu_rx_info;
    /* 初始化不开启PACK反馈功能 */
    rx_info->pack_enable = OAL_FALSE;

    /* 初始化device接收窗口 */
    memset_s((void *)rx_info->rx_window_bitmap, sizeof(uint32_t) * BYTE_OFFSET_8,
        0, sizeof(uint32_t) * BYTE_OFFSET_8);
    rx_info->reorder_start = 0;
    rx_info->reorder_end = DMAC_BA_SEQNO_ADD(rx_info->reorder_start, 1);
    rx_info->window_size = WLAN_AMPDU_RX_HE_BUFFER_SIZE;
    rx_info->window_end =
        DMAC_BA_SEQNO_ADD(rx_info->reorder_start, rx_info->window_size - 1);
}

#define dmac_get_bitmap_index(_seqno, _bitmap_size) ((_seqno) & ((_bitmap_size) -1))
#define dmac_tx_buf_bitmap_is_set(_bitmap, _idx) \
    (((_bitmap)[((_idx) >> 5)] & \
    ((uint32_t)(1U << ((_idx) & 31)))) != 0)

static void hmac_flush_mcast_ampdu_rx_window(hmac_mcast_ampdu_rx_stats_stru *rx_reorder)
{
    uint16_t idx;

    /* device接收窗口头部右移 */
    while (hmac_ba_rx_seqno_lt(rx_reorder->reorder_start, rx_reorder->reorder_end)) {
        idx = dmac_get_bitmap_index(rx_reorder->reorder_start, rx_reorder->window_size);
        if (!dmac_tx_buf_bitmap_is_set(rx_reorder->rx_window_bitmap, idx)) {
            break;
        }
        /* reorder窗口起始位置右移一个MPDU */
        rx_reorder->reorder_start = DMAC_BA_SEQNO_ADD(rx_reorder->reorder_start, 1);
    }

    /* device接收窗口尾部右移 */
    while (rx_reorder->window_end != DMAC_BA_SEQNO_ADD(rx_reorder->reorder_start, rx_reorder->window_size - 1)) {
        rx_reorder->window_end = DMAC_BA_SEQNO_ADD(rx_reorder->window_end, 1);
        /* 清理device接收窗口 */
        idx = dmac_get_bitmap_index(rx_reorder->window_end, rx_reorder->window_size);
        hmac_tx_buf_bitmap_clr(rx_reorder->rx_window_bitmap, idx);
    }

    /* 更新device接收窗口中reorder窗口 */
    if (hmac_ba_rx_seqno_geq(rx_reorder->reorder_start, rx_reorder->reorder_end)) {
        rx_reorder->reorder_end = DMAC_BA_SEQNO_ADD(rx_reorder->reorder_start, 1);
    }
}

static void hmac_move_mcast_ampdu_rx_window_one_mpdu(hmac_mcast_ampdu_rx_stats_stru *rx_reorder)
{
    uint16_t idx;

    /* 窗口强制向右移动一个MPDU */
    rx_reorder->reorder_start = DMAC_BA_SEQNO_ADD(rx_reorder->reorder_start, 1);
    if (hmac_ba_rx_seqno_geq(rx_reorder->reorder_start, rx_reorder->reorder_end)) {
        rx_reorder->reorder_end = DMAC_BA_SEQNO_ADD(rx_reorder->reorder_start, 1);
    }
    rx_reorder->window_end = DMAC_BA_SEQNO_ADD(rx_reorder->window_end, 1);
    idx = dmac_get_bitmap_index(rx_reorder->window_end, rx_reorder->window_size);
    hmac_tx_buf_bitmap_clr(rx_reorder->rx_window_bitmap, idx);

    /* 强制移窗后刷新device接收窗口 */
    hmac_flush_mcast_ampdu_rx_window(rx_reorder);
}

static void hmac_update_mcast_ampdu_rx_window(hmac_mcast_ampdu_rx_stats_stru *rx_reorder, uint16_t seqno)
{
    uint16_t idx;

    /* MPDU在reorder区中 */
    if (hmac_ba_rx_seqno_geq(seqno, rx_reorder->reorder_start) && hmac_ba_seqno_lt(seqno, rx_reorder->reorder_end)) {
        idx = dmac_get_bitmap_index(seqno, rx_reorder->window_size);
        hmac_tx_buf_bitmap_set(rx_reorder->rx_window_bitmap, idx);
        hmac_flush_mcast_ampdu_rx_window(rx_reorder);
        return;
    }

    /* MPDU在reorder区外的接收窗口中,需更新reorder队尾 */
    if (hmac_ba_rx_seqno_geq(seqno, rx_reorder->reorder_end) && hmac_ba_rx_seqno_lt(seqno, rx_reorder->window_end)) {
        idx = dmac_get_bitmap_index(seqno, rx_reorder->window_size);
        hmac_tx_buf_bitmap_set(rx_reorder->rx_window_bitmap, idx);
        rx_reorder->reorder_end = DMAC_BA_SEQNO_ADD(seqno, 1);
        if (hmac_ba_rx_seqno_geq(rx_reorder->reorder_end, rx_reorder->window_end)) {
            /* device接收窗口已满,需强制移窗 */
            oam_warning_log0(0, OAM_SF_RX, "{hmac_update_mcast_ampdu_rx_window:rx windows overflow!}");
            hmac_move_mcast_ampdu_rx_window_one_mpdu(rx_reorder);
        }
        return;
    }

    /* MPDU落在device接收窗口外,需强制移窗 */
    if (hmac_ba_rx_seqno_geq(seqno, rx_reorder->window_end)) {
        while (hmac_ba_rx_seqno_geq(seqno, rx_reorder->window_end)) {
            idx = dmac_get_bitmap_index(rx_reorder->reorder_start, rx_reorder->window_size);
            hmac_tx_buf_bitmap_clr(rx_reorder->rx_window_bitmap, idx);
            rx_reorder->window_end = DMAC_BA_SEQNO_ADD(rx_reorder->window_end, 1);
            rx_reorder->reorder_start = DMAC_BA_SEQNO_SUB(rx_reorder->window_end, rx_reorder->window_size - 1);
        }
        /* 更新MPDU信息 */
        idx = dmac_get_bitmap_index(seqno, rx_reorder->window_size);
        hmac_tx_buf_bitmap_set(rx_reorder->rx_window_bitmap, idx);
        rx_reorder->reorder_end = DMAC_BA_SEQNO_ADD(seqno, 1);
        if (hmac_ba_rx_seqno_geq(rx_reorder->reorder_end, rx_reorder->window_end)) {
            /* device接收窗口已满,需强制移窗 */
            hmac_move_mcast_ampdu_rx_window_one_mpdu(rx_reorder);
        }
        hmac_flush_mcast_ampdu_rx_window(rx_reorder);
    }
}

oal_bool_enum hmac_filter_duplicate_mcast_ampdu_pkt(hmac_mcast_ampdu_rx_stats_stru *rx_reorder,
    uint16_t seqno)
{
    uint16_t idx;
    /* reorder区前的帧,认为是重复帧 */
    if (hmac_ba_seqno_lt(seqno, rx_reorder->reorder_start)) {
        return OAL_TRUE;
    }

    if (hmac_ba_rx_seqno_geq(seqno, rx_reorder->reorder_start) && hmac_ba_seqno_lt(seqno, rx_reorder->reorder_end)) {
        /* reorder区间的帧,通过bitmap确认是否已经收到 */
        idx = dmac_get_bitmap_index(seqno, rx_reorder->window_size);
        if (dmac_tx_buf_bitmap_is_set(rx_reorder->rx_window_bitmap, idx)) {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}

hmac_rx_frame_ctrl_enum hmac_mcast_ampdu_rx_proc(hmac_host_rx_context_stru *rx_context)
{
    hmac_mcast_ampdu_rx_stats_stru *client_info =
        (hmac_mcast_ampdu_rx_stats_stru *)(rx_context->hmac_vap->mcast_ampdu_rx_info);
    mac_ether_header_stru *ether_hdr = NULL;
    uint16_t seq_num;
    if (client_info == NULL) {
        oam_error_log0(rx_context->hmac_vap->st_vap_base_info.uc_vap_id, 0,
            "{hmac_mcast_ampdu_rx_proc::mcast rx info is null}");
        return HMAC_RX_FRAME_CTRL_GOON;
    }
    if ((mac_get_mcast_ampdu_switch() != OAL_TRUE) || (client_info->pack_enable != OAL_TRUE)) {
        return HMAC_RX_FRAME_CTRL_GOON;
    }

    ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(rx_context->netbuf);
    if ((rx_context->hmac_vap->st_vap_base_info.en_vap_mode != WLAN_VAP_MODE_BSS_STA) ||
        (ether_is_multicast(ether_hdr->auc_ether_dhost) == OAL_FALSE)) {
        return HMAC_RX_FRAME_CTRL_GOON;
    }

    /* 统计全部收到的组播聚合帧 */
    client_info->rx_all_pkts++;

    seq_num = rx_context->cb->us_seq_num;
    /* 过滤重复帧 */
    if (hmac_filter_duplicate_mcast_ampdu_pkt(client_info, seq_num) == OAL_TRUE) {
        return HMAC_RX_FRAME_CTRL_DROP;
    }

    /* 统计全部收到的非重复组播聚合帧 */
    client_info->rx_non_retry_pkts++;

    /* 更新device接收窗口 */
    hmac_update_mcast_ampdu_rx_window(client_info, seq_num);
    return HMAC_RX_FRAME_CTRL_GOON;
}

void hmac_mcast_ampdu_set_pack_switch(mac_vap_stru *mac_vap, uint8_t en)
{
    hmac_vap_stru *hmac_vap;
    hmac_mcast_ampdu_rx_stats_stru *client_info = NULL;
    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    if (hmac_vap == NULL) {
        oam_error_log0(mac_vap->uc_vap_id, 0, "{hmac_mcast_ampdu_set_pack_switch::hmac_vap null.}");
        return;
    }

    client_info = (hmac_mcast_ampdu_rx_stats_stru *)(hmac_vap->mcast_ampdu_rx_info);
    client_info->pack_enable = en;
    oam_warning_log1(0, 0, "{hmac_mcast_ampdu_set_pack_switch::pack switch %d}", en);
}
#endif

#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU_HW

OAL_STATIC uint32_t hmac_mcast_ampdu_user_add_post_event(mac_vap_stru *mac_vap, hmac_user_stru *hmac_user,
    uint8_t *mac_addr, uint16_t user_idx)
{
    frw_event_mem_stru *event_mem = NULL;
    frw_event_stru *frw_event = NULL;
    dmac_ctx_add_user_stru *add_user_payload = NULL;
    uint32_t ret;

    event_mem = frw_event_alloc_m(sizeof(dmac_ctx_add_user_stru));
    if (oal_unlikely(event_mem == NULL)) {
        oam_error_log0(mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add::event_mem null.}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    frw_event = frw_get_event_stru(event_mem);
    add_user_payload = (dmac_ctx_add_user_stru *)frw_event->auc_event_data;
    add_user_payload->us_user_idx = user_idx;
    add_user_payload->en_ap_type = hmac_user->en_user_ap_type;
    add_user_payload->lut_index = hmac_user->lut_index;
    oal_set_mac_addr(add_user_payload->auc_user_mac_addr, mac_addr);

    /* 填充事件头 */
    frw_event_hdr_init(&(frw_event->st_event_hdr), FRW_EVENT_TYPE_WLAN_CTX, DMAC_WLAN_CTX_EVENT_SUB_TYPE_ADD_MULTI_USER,
        sizeof(dmac_ctx_add_user_stru), FRW_EVENT_PIPELINE_STAGE_1, mac_vap->uc_chip_id,
        mac_vap->uc_device_id, mac_vap->uc_vap_id);

    ret = frw_event_dispatch_event(event_mem);

    frw_event_free_m(event_mem);
    return ret;
}


static void hmac_mcast_ampdu_add_user(mac_vap_stru *mac_vap, uint8_t *mac_addr)
{
    uint32_t ret;
    uint16_t user_index;
    mac_user_stru *mac_user = NULL;
#ifdef _PRE_WLAN_FEATURE_WAPI
    hmac_user_stru *hmac_user = NULL;
#endif
    uint8_t multi_user_lut_idx;

    ret = hmac_user_alloc(&user_index);
    if (ret != OAL_SUCC) {
        oam_error_log1(mac_vap->uc_vap_id, OAM_SF_UM,
            "{hmac_mcast_ampdu_add_user::hmac_user_alloc failed[%d].}", ret);
        return;
    }

    /* 初始化组播用户基本信息 */
    mac_user = (mac_user_stru *)mac_res_get_mac_user(user_index);
    if (mac_user == NULL) {
        oam_warning_log1(mac_vap->uc_vap_id, OAM_SF_UM,
                         "{hmac_mcast_ampdu_add_user::mac_user[%d] null.}", user_index);
        return;
    }

    mac_user_init(mac_user, user_index, mac_addr,
        mac_vap->uc_chip_id, mac_vap->uc_device_id, mac_vap->uc_vap_id);
    mac_user_init_rom(mac_user, user_index);

    mac_user->en_is_multi_user = OAL_TRUE;
    mac_user->en_user_asoc_state = MAC_USER_STATE_ASSOC;
    /* 设置mac地址 */
    oal_set_mac_addr(mac_user->auc_user_mac_addr, mac_addr);
    mac_user_set_qos(mac_user, OAL_TRUE);
#ifdef _PRE_WLAN_FEATURE_WAPI
    hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(user_index);
    if (hmac_user == NULL) {
        oam_error_log1(mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_mcast_ampdu_add_user::hmac_user[%d] null.}", user_index);
        return;
    }

    /* 初始化wapi对象 */
    hmac_wapi_init(&hmac_user->st_wapi, OAL_FALSE);
#endif
    /* 组播聚合场景初始化tid、ring信息 */
    hmac_mcast_ampdu_user_tid_init(mac_vap, hmac_user, &multi_user_lut_idx);

    /***************************************************************************
        抛事件到DMAC层, 创建dmac用户
    ***************************************************************************/
    ret = hmac_mcast_ampdu_user_add_post_event(mac_vap, hmac_user, mac_addr, user_index);
    if (ret != OAL_SUCC) {
        /* 异常处理，释放内存，device下关联用户数还没有++，这里不需要判断返回值做--操作 */
        hmac_user_free(user_index);
        /* 不应该出现用户添加失败，失败需要定位具体原因 */
        oam_error_log1(mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_mcast_ampdu_add_user::dispatch_event failed[%d].}", ret);
        return;
    }
    oam_warning_log2(mac_vap->uc_vap_id, OAM_SF_ANY,
        "{hmac_mcast_ampdu_add_user, user index[%d] lut idx[%d].}", user_index, multi_user_lut_idx);
}

static void hmac_mcast_ampdu_get_addr(uint8_t *multi_addr, uint8_t *mac_addr)
{
    oal_set_mac_addr(multi_addr, mac_addr);
    multi_addr[5] |= BIT0; /* addr[5]的bit0:bit40设置为1 */
}


void hmac_mcast_ampdu_sta_add_multi_user(mac_vap_stru *mac_vap, uint8_t *mac_addr)
{
    uint8_t multi_addr[OAL_MAC_ADDRESS_LEN] = {0};
    if (mac_get_mcast_ampdu_switch() == OAL_FALSE) {
        return;
    }
    if (mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_STA) {
        return;
    }
    hmac_mcast_ampdu_get_addr(multi_addr, mac_addr);
    oal_set_mac_addr(multi_addr, mac_addr);
    oam_warning_log4(0, OAM_SF_UM,
        "{hmac_mcast_ampdu_sta_add_multi_user::ucast user:mac addr:0x%x:0x%x...0x%x:0x%x}",
        mac_addr[0], mac_addr[1], mac_addr[4], mac_addr[5]);
    oam_warning_log4(0, OAM_SF_UM,
        "{hmac_mcast_ampdu_sta_add_multi_user::multi user:mac addr:0x%x:0x%x...0x%x:0x%x}",
        multi_addr[0], multi_addr[1], multi_addr[4], multi_addr[5]);
    hmac_mcast_ampdu_add_user(mac_vap, multi_addr);
}
#endif
