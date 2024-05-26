

#ifndef __HMAC_TID_LIST_RCU_H__
#define __HMAC_TID_LIST_RCU_H__

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "linux/rculist.h"
#include "hmac_tid.h"

typedef struct {
    struct list_head list;
    oal_spin_lock_stru lock;
    const hmac_tid_list_ops *ops;
} hmac_tid_rcu_list_stru;

static inline void hmac_tid_list_init_rcu(hmac_tid_rcu_list_stru *tid_list, const hmac_tid_list_ops *ops)
{
    INIT_LIST_HEAD_RCU(&tid_list->list);
    oal_spin_lock_init(&tid_list->lock);
    tid_list->ops = ops;
}

static inline void hmac_tid_list_enqueue_rcu(hmac_tid_rcu_list_stru *tid_list, hmac_tid_info_stru *tid_info)
{
    if (oal_unlikely(!tid_list->ops->entry_getter)) {
        return;
    }

    oal_spin_lock_bh(&tid_list->lock);
    list_add_rcu((struct list_head *)tid_list->ops->entry_getter(tid_info), &tid_list->list);
    oal_spin_unlock_bh(&tid_list->lock);
}

static inline void hmac_tid_list_delete_rcu(hmac_tid_rcu_list_stru *tid_list, hmac_tid_info_stru *tid_info)
{
    if (oal_unlikely(!tid_list->ops->entry_getter)) {
        return;
    }

    oal_spin_lock_bh(&tid_list->lock);
    list_del_rcu((struct list_head *)tid_list->ops->entry_getter(tid_info));
    oal_spin_unlock_bh(&tid_list->lock);
    synchronize_rcu();
}

static inline uint8_t hmac_tid_list_for_each_rcu(
    hmac_tid_rcu_list_stru *tid_list, hmac_tid_list_func func, void *param)
{
    uint8_t ret;

    if (oal_unlikely(!tid_list->ops->for_each_rcu)) {
        return OAL_FALSE;
    }

    rcu_read_lock();
    ret = tid_list->ops->for_each_rcu(func, param);
    rcu_read_unlock();

    return ret;
}

#endif
#endif
