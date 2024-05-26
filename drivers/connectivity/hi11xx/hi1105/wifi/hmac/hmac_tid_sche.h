

#ifndef __HMAC_TID_SCHE_H__
#define __HMAC_TID_SCHE_H__

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "hmac_tid_list_rcu.h"
#endif
#include "hmac_tid_list.h"

typedef struct {
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    hmac_tid_rcu_list_stru tid_list;
#else
    hmac_tid_list_stru tid_list;
#endif
    hmac_tid_list_stru tid_tx_list; /* 本轮调度需要发帧的tid */
    struct semaphore tid_sche_sema;
    oal_task_stru *tid_schedule_thread;
    oal_atomic ring_mpdu_num;
} hmac_tid_schedule_stru;

extern hmac_tid_schedule_stru g_tid_schedule_list;

static inline hmac_tid_schedule_stru *hmac_get_tid_schedule_list(void)
{
    return &g_tid_schedule_list;
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
static inline uint32_t hmac_tid_schedule_list_enqueue(hmac_tid_info_stru *tid_info)
{
    if (oal_atomic_read(&tid_info->tid_scheduled)) {
        return OAL_SUCC;
    }

    hmac_tid_list_enqueue_rcu(&g_tid_schedule_list.tid_list, tid_info);
    oal_atomic_set(&tid_info->tid_scheduled, OAL_TRUE);

    return OAL_SUCC;
}

static inline void hmac_tid_schedule_list_delete(hmac_tid_info_stru *tid_info)
{
    if (!oal_atomic_read(&tid_info->tid_scheduled)) {
        return;
    }

    hmac_tid_list_delete_rcu(&g_tid_schedule_list.tid_list, tid_info);
    oal_atomic_set(&tid_info->tid_scheduled, OAL_FALSE);
}

static inline uint8_t hmac_tid_schedule_list_for_each(hmac_tid_list_func func, void *param)
{
    return hmac_tid_list_for_each_rcu(&g_tid_schedule_list.tid_list, func, param);
}
#else
static inline uint32_t hmac_tid_schedule_list_enqueue(hmac_tid_info_stru *tid_info)
{
    return hmac_tid_list_enqueue(&g_tid_schedule_list.tid_list, tid_info);
}

static inline void hmac_tid_schedule_list_delete(hmac_tid_info_stru *tid_info)
{
    hmac_tid_list_delete(&g_tid_schedule_list.tid_list, tid_info);
}

static inline uint8_t hmac_tid_schedule_list_for_each(hmac_tid_list_func func, void *param)
{
    return hmac_tid_list_for_each(&g_tid_schedule_list.tid_list, func, param);
}
#endif

static inline uint32_t hmac_tid_tx_list_enqueue(hmac_tid_info_stru *tid_info)
{
    return __hmac_tid_list_enqueue(&g_tid_schedule_list.tid_tx_list, tid_info);
}

static inline void hmac_tid_tx_list_clear(void)
{
    __hmac_tid_list_clear(&g_tid_schedule_list.tid_tx_list);
}

static inline uint8_t hmac_tid_tx_list_for_each(hmac_tid_list_func func, void *param)
{
    return __hmac_tid_list_for_each(&g_tid_schedule_list.tid_tx_list, func, param);
}

static inline void hmac_tid_schedule(void)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    up(&g_tid_schedule_list.tid_sche_sema);
#endif
}

static inline oal_bool_enum_uint8 hmac_tid_sche_allowed(hmac_tid_info_stru *tid_info)
{
    return (oal_netbuf_list_len(&tid_info->tid_queue) >=
        (uint32_t)oal_atomic_read(&tid_info->tid_sche_th)) ? OAL_TRUE : OAL_FALSE;
}

void hmac_tid_schedule_init(void);
int32_t hmac_tid_schedule_thread(void *p_data);
uint8_t hmac_is_tid_empty(void);

#endif
