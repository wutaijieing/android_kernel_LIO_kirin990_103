

#ifndef __OAL_LITEOS_RW_LOCK_H__
#define __OAL_LITEOS_RW_LOCK_H__

#include "oal_util.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

typedef int                oal_rwlock_stru;

OAL_STATIC OAL_INLINE void  oal_rw_lock_init(oal_rwlock_stru *pst_lock)
{
    OAL_UNUSED_PARAM(pst_lock);
}


OAL_STATIC OAL_INLINE void  oal_rw_lock_read_lock(oal_rwlock_stru *pst_lock)
{
    OAL_UNUSED_PARAM(pst_lock);
}


OAL_STATIC OAL_INLINE void  oal_rw_lock_read_unlock(oal_rwlock_stru *pst_lock)
{
    OAL_UNUSED_PARAM(pst_lock);
}


OAL_STATIC OAL_INLINE void  oal_rw_lock_write_lock(oal_rwlock_stru *pst_lock)
{
    OAL_UNUSED_PARAM(pst_lock);
}


OAL_STATIC OAL_INLINE void  oal_rw_lock_write_unlock(oal_rwlock_stru *pst_lock)
{
    OAL_UNUSED_PARAM(pst_lock);
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_rw_lock.h */

