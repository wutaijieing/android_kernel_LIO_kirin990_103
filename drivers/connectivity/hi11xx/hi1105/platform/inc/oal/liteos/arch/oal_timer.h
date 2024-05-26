

#ifndef __OAL_LITEOS_TIMER_H__
#define __OAL_LITEOS_TIMER_H__

#include <linux/timer.h>
#include <los_swtmr.h>
#include "oal_time.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

typedef struct timer_list              oal_timer_list_stru;


typedef void (*oal_timer_func)(uintptr_t);

OAL_STATIC OAL_INLINE void  oal_timer_init(oal_timer_list_stru *pst_timer, uint32_t ul_delay,
                                           oal_timer_func p_func, uint32_t ui_arg)
{
    init_timer(pst_timer);
    pst_timer->expires = oal_msecs_to_jiffies(ul_delay);
    pst_timer->function = (void (*)(unsigned long))p_func;
    pst_timer->data = ui_arg;
}


OAL_STATIC OAL_INLINE int32_t  oal_timer_delete(oal_timer_list_stru *pst_timer)
{
    return del_timer(pst_timer);
}


OAL_STATIC OAL_INLINE int32_t  oal_timer_delete_sync(oal_timer_list_stru *pst_timer)
{
    return del_timer_sync(pst_timer);
}


OAL_STATIC OAL_INLINE void  oal_timer_add(oal_timer_list_stru *pst_timer)
{
    add_timer(pst_timer);
}


OAL_STATIC OAL_INLINE int32_t  oal_timer_start(oal_timer_list_stru *pst_timer, uint32_t ui_delay)
{
    if (pst_timer->flag == TIMER_UNVALID) {
        pst_timer->expires = oal_msecs_to_jiffies(ui_delay);
        add_timer(pst_timer);
        return OAL_SUCC;
    } else {
        return mod_timer(pst_timer, oal_msecs_to_jiffies(ui_delay));
    }
}


OAL_STATIC OAL_INLINE void  oal_timer_start_on(oal_timer_list_stru *pst_timer, uint32_t ui_delay, int32_t cpu)
{
    pst_timer->expires = oal_msecs_to_jiffies(ui_delay);
    add_timer(pst_timer);
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_timer.h */

