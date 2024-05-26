

#include "hmac_tid_sche.h"
#include "hmac_tx_msdu_dscr.h"
#include "hmac_host_tx_data.h"
#include "oam_stat_wifi.h"
#include "hmac_tid_update.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_TID_SCHE_C

/* tid调度管理结构体, 主要用于保存待调度的user tid链表, 在调度线程中处理 */
hmac_tid_schedule_stru g_tid_schedule_list;

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
static inline uint8_t hmac_tid_sche_list_for_each_rcu(hmac_tid_list_func func, void *param)
{
    hmac_tid_info_stru *tid_info = NULL;
    hmac_tid_rcu_list_stru *tid_list = &g_tid_schedule_list.tid_list;

    list_for_each_entry_rcu(tid_info, &tid_list->list, tid_sche_entry) {
        if (func(tid_info, param) == OAL_RETURN) {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}
#endif

static inline hmac_tid_info_stru *hmac_sche_tid_info_getter(void *entry)
{
    return oal_container_of(entry, hmac_tid_info_stru, tid_sche_entry);
}

static inline void *hmac_sche_entry_getter(hmac_tid_info_stru *tid_info)
{
    return &tid_info->tid_sche_entry;
}

const hmac_tid_list_ops g_tid_sche_list_ops = {
    .tid_info_getter = hmac_sche_tid_info_getter,
    .entry_getter = hmac_sche_entry_getter,
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    .for_each_rcu = hmac_tid_sche_list_for_each_rcu,
#endif
};

static inline hmac_tid_info_stru *hmac_tx_tid_info_getter(void *entry)
{
    return oal_container_of(entry, hmac_tid_info_stru, tid_tx_entry);
}

static inline void *hmac_tx_entry_getter(hmac_tid_info_stru *tid_info)
{
    return &tid_info->tid_tx_entry;
}

const hmac_tid_list_ops g_tid_tx_list_ops = {
    .tid_info_getter = hmac_tx_tid_info_getter,
    .entry_getter = hmac_tx_entry_getter,
};

void hmac_tid_schedule_init(void)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    hmac_tid_list_init_rcu(&g_tid_schedule_list.tid_list, &g_tid_sche_list_ops);
#else
    hmac_tid_list_init(&g_tid_schedule_list.tid_list, &g_tid_sche_list_ops);
#endif
    hmac_tid_list_init(&g_tid_schedule_list.tid_tx_list, &g_tid_tx_list_ops);
    oal_atomic_set(&g_tid_schedule_list.ring_mpdu_num, 0);
}

static inline oal_bool_enum_uint8 hmac_tid_push_ring_allowed(hmac_tid_info_stru *tid_info)
{
    return oal_atomic_read(&tid_info->tx_ring.enabled) && !hmac_tid_ring_full(&tid_info->tx_ring);
}

static inline void hmac_tid_get_netbuf_list(
    oal_netbuf_head_stru *tid_queue, oal_netbuf_head_stru *netbuf_list, uint32_t delist_limit)
{
    oal_netbuf_head_init(netbuf_list);

    oal_spin_lock_head_bh(tid_queue);

    while (!oal_netbuf_list_empty(tid_queue) && oal_netbuf_list_len(netbuf_list) < delist_limit) {
        oal_netbuf_list_tail_nolock(netbuf_list, oal_netbuf_delist_nolock(tid_queue));
    }

    oal_spin_unlock_head_bh(tid_queue);
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && (!defined _PRE_WINDOWS_SUPPORT)
static void hmac_tx_set_cb_csum_info(mac_tx_ctl_stru *tx_ctl, oal_netbuf_stru *netbuf)
{
    mac_ether_header_stru *ether_header = (mac_ether_header_stru *)oal_netbuf_data(netbuf);
    if (ether_header->us_ether_type == oal_host2net_short(ETHER_TYPE_IP)) {
        struct iphdr *iph = (struct iphdr *)(netbuf->head + netbuf->network_header);
        if (iph->protocol == OAL_IPPROTO_TCP) {
            tx_ctl->csum_type = CSUM_TYPE_IPV4_TCP;
        } else if (iph->protocol == OAL_IPPROTO_UDP) {
            tx_ctl->csum_type = CSUM_TYPE_IPV4_UDP;
        }
    } else if (ether_header->us_ether_type == oal_host2net_short(ETHER_TYPE_IPV6)) {
        struct ipv6hdr *ipv6h = (struct ipv6hdr *)(netbuf->head + netbuf->network_header);
        if (ipv6h->nexthdr == OAL_IPPROTO_TCP) {
            tx_ctl->csum_type = CSUM_TYPE_IPV6_TCP;
        } else if (ipv6h->nexthdr == OAL_IPPROTO_UDP) {
            tx_ctl->csum_type = CSUM_TYPE_IPV6_UDP;
        }
    }
}
#else
static void hmac_tx_set_cb_csum_info(mac_tx_ctl_stru *tx_ctl, oal_netbuf_stru *netbuf)
{
}
#endif

uint32_t hmac_tid_push_netbuf_to_device_ring(
    hmac_tid_info_stru *tid_info, hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user)
{
    uint32_t ret;
    oal_netbuf_head_stru netbuf_list;
    oal_netbuf_stru *netbuf = NULL;
    frw_event_stru *frw_event = NULL; /* 事件结构体 */
    frw_event_mem_stru *frw_event_mem = NULL;
    dmac_tx_event_stru *dtx_event = NULL;
    mac_tx_ctl_stru *tx_ctl = NULL;
    mac_vap_stru *mac_vap = &hmac_vap->st_vap_base_info;

    hmac_tid_get_netbuf_list(&tid_info->tid_queue, &netbuf_list, oal_netbuf_list_len(&tid_info->tid_queue));
    if (oal_unlikely(oal_netbuf_list_empty(&netbuf_list))) {
        return OAL_SUCC;
    }

    /* 优化成netbuf链, 直接在一个事件中赋值dtx_event->pst_netbuf, 不需要循环抛多个事件 */
    while ((netbuf = oal_netbuf_delist_nolock(&netbuf_list)) != NULL) {
        frw_event_mem = frw_event_alloc_m(sizeof(dmac_tx_event_stru));
        if (oal_unlikely(frw_event_mem == NULL)) {
            oam_error_log0(mac_vap->uc_vap_id, OAM_SF_TX, "{hmac_tx_lan_to_wlan::frw_event_alloc_m failed.}");
            return OAL_ERR_CODE_ALLOC_MEM_FAIL;
        }

        frw_event = frw_get_event_stru(frw_event_mem);
        frw_event_hdr_init(&(frw_event->st_event_hdr), FRW_EVENT_TYPE_HOST_DRX,
                           DMAC_TX_HOST_DTX, sizeof(dmac_tx_event_stru),
                           FRW_EVENT_PIPELINE_STAGE_1, mac_vap->uc_chip_id,
                           mac_vap->uc_device_id, mac_vap->uc_vap_id);

        dtx_event = (dmac_tx_event_stru *)frw_event->auc_event_data;
        dtx_event->pst_netbuf = netbuf;

        tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(netbuf);

        /* 设置8023帧长, 用于设置device ring msdu info的msdu_len字段 */
        MAC_GET_CB_MPDU_LEN(tx_ctl) = oal_netbuf_len(netbuf);
#ifndef _PRE_WINDOWS_SUPPORT
        if (netbuf->ip_summed == CHECKSUM_PARTIAL) {
            hmac_tx_set_cb_csum_info(tx_ctl, netbuf);
        }
#endif
        ret = frw_event_dispatch_event(frw_event_mem);
        if (oal_unlikely(ret != OAL_SUCC)) {
            oam_warning_log1(mac_vap->uc_vap_id, OAM_SF_TX,
                             "{hmac_tid_trans_netbuf_to_device::frw_event_dispatch_event failed[%d]}", ret);
            oam_stat_vap_incr(mac_vap->uc_vap_id, tx_abnormal_msdu_dropped, 1);
        }

        frw_event_free_m(frw_event_mem);
    }

    return OAL_SUCC;
}

static inline uint32_t hmac_tid_get_ring_avail_size(hmac_msdu_info_ring_stru *tx_ring)
{
    return hal_host_tx_tid_ring_depth_get(tx_ring->base_ring_info.size) - (uint32_t)oal_atomic_read(&tx_ring->msdu_cnt);
}

static inline void hmac_tx_trig_device_softirq(hmac_vap_stru *hmac_vap)
{
    hal_host_device_stru *hal_device = hal_get_host_device(hmac_vap->hal_dev_id);

    if (hal_device == NULL) {
        return;
    }

    oal_writel(1, (uintptr_t)hal_device->soc_regs.w_soft_int_set_aon_reg);
}

uint32_t hmac_tid_push_netbuf_to_host_ring(
    hmac_tid_info_stru *tid_info, hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user)
{
    oal_netbuf_stru *netbuf = NULL;
    oal_netbuf_head_stru netbuf_list;

    host_cnt_init_record_performance(TX_RING_PROC);
    host_start_record_performance(TX_RING_PROC);

    if (oal_unlikely(!hmac_tid_push_ring_allowed(tid_info))) {
        return OAL_FAIL;
    }

    hmac_tid_get_netbuf_list(&tid_info->tid_queue, &netbuf_list, hmac_tid_get_ring_avail_size(&tid_info->tx_ring));
    if (oal_unlikely(oal_netbuf_list_empty(&netbuf_list))) {
        return OAL_SUCC;
    }

    wlan_pm_wakeup_dev_lock();

    while ((netbuf = oal_netbuf_delist_nolock(&netbuf_list)) != NULL) {
        if (hmac_host_ring_tx(hmac_vap, hmac_user, netbuf) != OAL_SUCC) {
            oal_netbuf_free(netbuf);
            continue;
        }

        if (++tid_info->ring_sync_cnt >= oal_atomic_read(&tid_info->ring_sync_th)) {
            tid_info->ring_sync_cnt = 0;
            hmac_tx_reg_write_ring_info(tid_info, TID_CMDID_ENQUE);

            if (hmac_tx_ring_soft_irq_sched_enabled() == OAL_TRUE) {
                /* WCPU软中断调度 */
                hmac_tx_trig_device_softirq(hmac_vap);
            } else {
                /* 下发事件调度 */
                hmac_tx_sync_ring_info(tid_info, TID_CMDID_ENQUE);
            }
        }
    }

    hmac_tx_reg_write_ring_info(tid_info, TID_CMDID_ENQUE);

    host_end_record_performance(host_cnt_get_record_performance(TX_RING_PROC), TX_RING_PROC);

    return OAL_SUCC;
}

typedef uint32_t (* hmac_tid_push_ring)(hmac_tid_info_stru *, hmac_vap_stru *, hmac_user_stru *);
hmac_tid_push_ring hmac_get_tid_push_ring_func(hmac_tid_info_stru *tid_info)
{
    uint8_t ring_tx_mode = (uint8_t)oal_atomic_read(&tid_info->ring_tx_mode);
    hmac_tid_push_ring func = NULL;

    if (oal_likely(ring_tx_mode == HOST_RING_TX_MODE)) {
        func = hmac_tid_push_netbuf_to_host_ring;
    } else if (ring_tx_mode == DEVICE_RING_TX_MODE) {
        func = hmac_tid_push_netbuf_to_device_ring;
    } else {
        oam_warning_log3(0, OAM_SF_TX, "{hmac_get_tid_push_ring_func::user[%d] tid[%d] push ring[%d] not allowed!}",
            tid_info->user_index, tid_info->tid_no, ring_tx_mode);
    }

    return func;
}

static uint8_t hmac_tid_push_ring_process(hmac_tid_info_stru *tid_info, void *param)
{
    hmac_user_stru *hmac_user = mac_res_get_hmac_user(tid_info->user_index);
    hmac_vap_stru *hmac_vap = NULL;
    hmac_tid_push_ring push_ring_func;

    if (oal_unlikely(hmac_user == NULL ||
        hmac_user->st_user_base_info.en_user_asoc_state != MAC_USER_STATE_ASSOC)) {
        return OAL_CONTINUE;
    }

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(hmac_user->st_user_base_info.uc_vap_id);
    if (oal_unlikely(hmac_vap == NULL || hmac_vap->st_vap_base_info.en_vap_state != MAC_VAP_STATE_UP)) {
        return OAL_CONTINUE;
    }

    push_ring_func = hmac_get_tid_push_ring_func(tid_info);
    if (oal_unlikely(push_ring_func == NULL)) {
        return OAL_CONTINUE;
    }
    push_ring_func(tid_info, hmac_vap, hmac_user);

    g_pm_wlan_pkt_statis.ring_tx_pkt += (uint32_t)oal_atomic_read(&g_tid_schedule_list.ring_mpdu_num);

    return OAL_CONTINUE;
}

static uint8_t hmac_get_tx_tid_info(hmac_tid_info_stru *tid_info, void *param)
{
    if (oal_unlikely(tid_info == NULL || !oal_netbuf_list_len(&tid_info->tid_queue))) {
        return OAL_CONTINUE;
    }

    hmac_tid_tx_list_enqueue(tid_info);

    return OAL_CONTINUE;
}

int32_t hmac_tid_schedule_thread(void *p_data)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    struct sched_param param;

    param.sched_priority = 97; /* 调度优先级97 */
    oal_set_thread_property(current, SCHED_FIFO, &param, -10); /* set nice -10 */

    allow_signal(SIGTERM);

    while (oal_likely(!down_interruptible(&g_tid_schedule_list.tid_sche_sema))) {
#ifdef _PRE_WINDOWS_SUPPORT
        if (oal_kthread_should_stop((PRT_THREAD)p_data)) {
#else
        if (oal_kthread_should_stop()) {
#endif
            break;
        }

        /* rcu protected */
        hmac_tid_schedule_list_for_each(hmac_get_tx_tid_info, NULL);

        /* local */
        hmac_tid_tx_list_for_each(hmac_tid_push_ring_process, NULL);
        hmac_tid_tx_list_clear();
    }
#endif

    return OAL_SUCC;
}

static inline uint8_t hmac_tid_queue_empty(hmac_tid_info_stru *tid_info, void *param)
{
    return oal_netbuf_list_len(&tid_info->tid_queue) ? OAL_RETURN : OAL_CONTINUE;
}

uint8_t hmac_is_tid_empty(void)
{
    return !hmac_tid_schedule_list_for_each(hmac_tid_queue_empty, NULL);
}
