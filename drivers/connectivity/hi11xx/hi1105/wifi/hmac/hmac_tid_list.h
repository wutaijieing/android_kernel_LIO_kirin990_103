

#ifndef __HMAC_TID_LIST_H__
#define __HMAC_TID_LIST_H__

#include "hmac_tid.h"

typedef struct {
    oal_dlist_head_stru list;
    oal_spin_lock_stru lock;
    const hmac_tid_list_ops *ops;
} hmac_tid_list_stru;

static inline void hmac_tid_list_init(hmac_tid_list_stru *tid_list, const hmac_tid_list_ops *ops)
{
    oal_dlist_init_head(&tid_list->list);
    oal_spin_lock_init(&tid_list->lock);
    tid_list->ops = ops;
}

static inline uint8_t __hmac_tid_list_for_each(hmac_tid_list_stru *tid_list, hmac_tid_list_func func, void *param)
{
    oal_dlist_head_stru *entry = NULL;
    oal_dlist_head_stru *entry_tmp = NULL;

    if (oal_unlikely(!tid_list->ops->tid_info_getter)) {
        return OAL_FALSE;
    }

    oal_dlist_search_for_each_safe(entry, entry_tmp, &tid_list->list) {
        if (func(tid_list->ops->tid_info_getter(entry), param) == OAL_RETURN) {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}

static inline uint8_t hmac_tid_list_for_each(hmac_tid_list_stru *tid_list, hmac_tid_list_func func, void *param)
{
    uint8_t ret;

    oal_spin_lock_bh(&tid_list->lock);
    ret = __hmac_tid_list_for_each(tid_list, func, param);
    oal_spin_unlock_bh(&tid_list->lock);

    return ret;
}

static inline uint8_t hmac_tid_list_entry_count(hmac_tid_list_stru *tid_list)
{
    uint8_t count = 0;
    oal_dlist_head_stru *entry = NULL;
    oal_dlist_head_stru *entry_tmp = NULL;

    oal_spin_lock_bh(&tid_list->lock);
    oal_dlist_search_for_each_safe(entry, entry_tmp, &tid_list->list) {
        count++;
    }
    oal_spin_unlock_bh(&tid_list->lock);
    return count;
}

static inline uint8_t hmac_tid_entry_equal(hmac_tid_info_stru *tid_info_entry, void *tid_info)
{
    if (tid_info_entry->user_index == ((hmac_tid_info_stru *)tid_info)->user_index &&
        tid_info_entry->tid_no == ((hmac_tid_info_stru *)tid_info)->tid_no) {
        return OAL_RETURN;
    }

    return OAL_CONTINUE;
}

static inline uint8_t __hmac_tid_entry_exists(hmac_tid_list_stru *tid_list, hmac_tid_info_stru *tid_info)
{
    return __hmac_tid_list_for_each(tid_list, hmac_tid_entry_equal, (void *)tid_info);
}

static inline uint8_t hmac_tid_entry_exists(hmac_tid_list_stru *tid_list, hmac_tid_info_stru *tid_info)
{
    uint8_t ret;

    oal_spin_lock_bh(&tid_list->lock);
    ret = __hmac_tid_entry_exists(tid_list, tid_info);
    oal_spin_unlock_bh(&tid_list->lock);

    return ret;
}

static inline hmac_tid_info_stru *__hmac_tid_list_dequeue(hmac_tid_list_stru *tid_list)
{
    if (oal_dlist_is_empty(&tid_list->list) || oal_unlikely(!tid_list->ops->tid_info_getter)) {
        return NULL;
    }

    return tid_list->ops->tid_info_getter(oal_dlist_delete_head(&tid_list->list));
}

static inline hmac_tid_info_stru *hmac_tid_list_dequeue(hmac_tid_list_stru *tid_list)
{
    hmac_tid_info_stru *tid_info = NULL;

    oal_spin_lock_bh(&tid_list->lock);
    tid_info = __hmac_tid_list_dequeue(tid_list);
    oal_spin_unlock_bh(&tid_list->lock);

    return tid_info;
}

static inline uint32_t __hmac_tid_list_enqueue(hmac_tid_list_stru *tid_list, hmac_tid_info_stru *tid_info)
{
    if (__hmac_tid_entry_exists(tid_list, tid_info) || oal_unlikely(!tid_list->ops->entry_getter)) {
        return OAL_FAIL;
    }

    oal_dlist_add_tail(tid_list->ops->entry_getter(tid_info), &tid_list->list);
    return OAL_SUCC;
}

static inline uint32_t hmac_tid_list_enqueue(hmac_tid_list_stru *tid_list, hmac_tid_info_stru *tid_info)
{
    uint32_t ret;

    oal_spin_lock_bh(&tid_list->lock);
    ret = __hmac_tid_list_enqueue(tid_list, tid_info);
    oal_spin_unlock_bh(&tid_list->lock);

    return ret;
}

static inline void __hmac_tid_list_delete(hmac_tid_list_stru *tid_list, hmac_tid_info_stru *tid_info)
{
    if (oal_unlikely(!tid_list->ops->entry_getter)) {
        return;
    }

    oal_dlist_delete_entry(tid_list->ops->entry_getter(tid_info));
}

/*
功能: 删除list特定节点
*/
static inline void hmac_tid_list_delete(hmac_tid_list_stru *tid_list, hmac_tid_info_stru *tid_info)
{
    oal_spin_lock_bh(&tid_list->lock);
    __hmac_tid_list_delete(tid_list, tid_info);
    oal_spin_unlock_bh(&tid_list->lock);
}


static inline void hmac_tid_list_search_delete(hmac_tid_list_stru *tid_list, hmac_tid_info_stru *tid_info)
{
    oal_dlist_head_stru *entry = NULL;
    oal_dlist_head_stru *entry_tmp = NULL;

    if (oal_unlikely(!tid_list->ops->tid_info_getter)) {
        return;
    }

    oal_spin_lock_bh(&tid_list->lock);
    oal_dlist_search_for_each_safe(entry, entry_tmp, &tid_list->list) {
        if (tid_list->ops->tid_info_getter(entry) == tid_info) {
            __hmac_tid_list_delete(tid_list, tid_info);
            break;
        }
    }
    oal_spin_unlock_bh(&tid_list->lock);
}

static inline void __hmac_tid_list_clear(hmac_tid_list_stru *tid_list)
{
    while (!oal_dlist_is_empty(&tid_list->list)) {
        oal_dlist_delete_head(&tid_list->list);
    }
}

static inline void hmac_tid_list_clear(hmac_tid_list_stru *tid_list)
{
    oal_spin_lock_bh(&tid_list->lock);
    __hmac_tid_list_clear(tid_list);
    oal_spin_unlock_bh(&tid_list->lock);
}

#endif
