

#ifndef __HMAC_TID_UPDATE_H__
#define __HMAC_TID_UPDATE_H__

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "hmac_tid_list_rcu.h"
#endif
#include "hmac_tid_list.h"
#include "frw_ext_if.h"

#define HMAC_TID_UPDATE_LVL_CNT 4
typedef struct {
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    hmac_tid_rcu_list_stru tid_list;
#else
    hmac_tid_list_stru tid_list;
#endif
    uint32_t tid_update_th[HMAC_TID_UPDATE_LVL_CNT];
    uint32_t tid_sche_th[HMAC_TID_UPDATE_LVL_CNT];
    uint32_t ring_sync_th[HMAC_TID_UPDATE_LVL_CNT];
    uint32_t ring_tx_opt[HMAC_TID_UPDATE_LVL_CNT];
    frw_timeout_stru tid_update_timer;
} hmac_tid_update_stru;

extern hmac_tid_update_stru g_tid_update_list;

static inline hmac_tid_update_stru *hmac_get_tid_update_list(void)
{
    return &g_tid_update_list;
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
static inline uint32_t hmac_tid_update_list_enqueue(hmac_tid_info_stru *tid_info)
{
    if (oal_atomic_read(&tid_info->tid_updated)) {
        return OAL_SUCC;
    }

    hmac_tid_list_enqueue_rcu(&g_tid_update_list.tid_list, tid_info);
    oal_atomic_set(&tid_info->tid_updated, OAL_TRUE);

    return OAL_SUCC;
}

static inline void hmac_tid_update_list_delete(hmac_tid_info_stru *tid_info)
{
    if (!oal_atomic_read(&tid_info->tid_updated)) {
        return;
    }

    hmac_tid_list_delete_rcu(&g_tid_update_list.tid_list, tid_info);
    oal_atomic_set(&tid_info->tid_updated, OAL_FALSE);
}

static inline uint8_t hmac_tid_update_list_for_each(hmac_tid_list_func func, void *param)
{
    return hmac_tid_list_for_each_rcu(&g_tid_update_list.tid_list, func, param);
}
#else
static inline uint32_t hmac_tid_update_list_enqueue(hmac_tid_info_stru *tid_info)
{
    return hmac_tid_list_enqueue(&g_tid_update_list.tid_list, tid_info);
}

static inline void hmac_tid_update_list_delete(hmac_tid_info_stru *tid_info)
{
    hmac_tid_list_delete(&g_tid_update_list.tid_list, tid_info);
}

static inline uint8_t hmac_tid_update_list_for_each(hmac_tid_list_func func, void *param)
{
    return hmac_tid_list_for_each(&g_tid_update_list.tid_list, func, param);
}
#endif

void hmac_tid_update_init(void);
void hmac_tid_update_timer_init(void);
void hmac_tid_update_timer_deinit(void);
#endif
