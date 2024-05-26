

#include "hmac_tid_update.h"
#include "hmac_tid_sche.h"
#include "hmac_tx_msdu_dscr.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_TID_UPDATE_C

/*
 * tid更新管理结构体, 用于保存活跃的tid
 * pps定时器中根据上一周期的tx数据更新特性开关、门限
 */
hmac_tid_update_stru g_tid_update_list = {
    .tid_update_th = {
        500, 3000, 6000, 999999, /* tid当前统计周期内发送的帧数 */
    },
    .ring_sync_th = {
        9, 7, 5, 3, /* 调度事件下发频率 */
    },
    .tid_sche_th = {
        1, 1, 16, 64, /* tid调度频率 */
    },
    .ring_tx_opt = {
        OAL_FALSE, OAL_FALSE, OAL_TRUE, OAL_TRUE, /* ring tx MIPS优化开关 */
    }
};

static uint8_t hmac_tid_update_sync_threshold(hmac_tid_info_stru *tid_info, void *param)
{
    uint32_t index;
    uint32_t last_period_tx_msdu = (uint32_t)oal_atomic_read(&tid_info->tx_ring.last_period_tx_msdu);
    hmac_tid_update_stru *tid_update_list = &g_tid_update_list;

    for (index = 0; index < HMAC_TID_UPDATE_LVL_CNT; index++) {
        if (last_period_tx_msdu > tid_update_list->tid_update_th[index]) {
            continue;
        }

        /*
         * ring_sync_th不为1时, 高流量突然变成低流量的场景下,
         * ring中缓存的帧可能达到HMAC_TID_UPDATE_TIMER_PERIOD的发送时延
         */
        oal_atomic_set(&tid_info->ring_sync_th, last_period_tx_msdu >> tid_update_list->ring_sync_th[index]);
        oal_atomic_set(&tid_info->tid_sche_th, tid_update_list->tid_sche_th[index]);
        oal_atomic_set(&tid_info->ring_tx_opt, tid_update_list->ring_tx_opt[index]);
        break;
    }

    if (!hmac_tx_ring_switching(tid_info)) {
        hmac_tid_schedule();
    }

    if (last_period_tx_msdu || hmac_tx_ring_switching(tid_info)) {
        hmac_tx_sync_ring_info(tid_info, TID_CMDID_ENQUE);
    }

    oal_atomic_set(&tid_info->tx_ring.last_period_tx_msdu, 0);

    return OAL_CONTINUE;
}

static inline uint32_t hmac_tid_update_list_process(void *arg)
{
    hmac_tid_update_list_for_each(hmac_tid_update_sync_threshold, NULL);

    return OAL_SUCC;
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
static inline uint8_t hmac_tid_update_list_for_each_rcu(hmac_tid_list_func func, void *param)
{
    hmac_tid_info_stru *tid_info = NULL;
    hmac_tid_rcu_list_stru *tid_list = &g_tid_update_list.tid_list;

    list_for_each_entry_rcu(tid_info, &tid_list->list, tid_update_entry) {
        if (func(tid_info, param) == OAL_RETURN) {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}
#endif

static inline hmac_tid_info_stru *hmac_update_tid_info_getter(void *entry)
{
    return oal_container_of(entry, hmac_tid_info_stru, tid_update_entry);
}

static inline void *hmac_update_entry_getter(hmac_tid_info_stru *tid_info)
{
    return &tid_info->tid_update_entry;
}

const hmac_tid_list_ops g_tid_update_list_ops = {
    .tid_info_getter = hmac_update_tid_info_getter,
    .entry_getter = hmac_update_entry_getter,
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    .for_each_rcu = hmac_tid_update_list_for_each_rcu,
#endif
};

void hmac_tid_update_init(void)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    hmac_tid_list_init_rcu(&g_tid_update_list.tid_list, &g_tid_update_list_ops);
#else
    hmac_tid_list_init(&g_tid_update_list.tid_list, &g_tid_update_list_ops);
#endif
}

#define HMAC_TID_UPDATE_TIMER_PERIOD 25
void hmac_tid_update_timer_init(void)
{
    frw_timeout_stru *tid_update_timer = &g_tid_update_list.tid_update_timer;

    if (!hmac_host_ring_tx_enabled()) {
        return;
    }

    if (tid_update_timer->en_is_registerd) {
        return;
    }

    frw_timer_create_timer_m(tid_update_timer, hmac_tid_update_list_process, HMAC_TID_UPDATE_TIMER_PERIOD,
                             NULL, OAL_TRUE, OAM_MODULE_ID_HMAC, 0);
}

void hmac_tid_update_timer_deinit(void)
{
    if (!hmac_host_ring_tx_enabled()) {
        return;
    }

    if (g_freq_lock_control.hmac_freq_timer.en_is_registerd == OAL_FALSE) {
        return;
    }

    frw_timer_immediate_destroy_timer_m(&g_freq_lock_control.hmac_freq_timer);
}
