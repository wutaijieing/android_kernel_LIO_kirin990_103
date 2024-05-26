

#ifndef __OAL_LITEOS_SEMAPHORE_H__
#define __OAL_LITEOS_SEMAPHORE_H__

#if (HW_LITEOS_OPEN_VERSION_NUM >= KERNEL_VERSION(3,2,3))
    #include <los_sem_pri.h>
#else
    #include <los_sem.h>
#endif
#include <los_sem.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#if (HW_LITEOS_OPEN_VERSION_NUM >= KERNEL_VERSION(3,2,3))
#define SEM_CB_S    LosSemCB
#define usSemID     semID
#endif
typedef SEM_CB_S oal_semaphore_stru;
OAL_STATIC OAL_INLINE void oal_sema_init(oal_semaphore_stru *pst_sem, uint16_t us_val)
{
    uint32_t ul_semhandle;
    if (pst_sem == NULL) {
        oal_io_print("[%s]pst_sem is null!\n", __func__);
        return;
    }

    if (LOS_SemCreate(us_val, &ul_semhandle) != LOS_OK) {
        oal_io_print("LOS_SemCreate failed!\n");
        return;
    }
    *pst_sem = *(GET_SEM(ul_semhandle));
}


OAL_STATIC OAL_INLINE void oal_up(oal_semaphore_stru *pst_sem)
{
    if (pst_sem == NULL) {
        oal_io_print("[%s]pst_sem is null!\n", __func__);
        return;
    }

    if (LOS_SemPost(pst_sem->usSemID) != LOS_OK) {
        oal_io_print("LOS_SemPost failed!\n");
        return;
    }
}

OAL_STATIC OAL_INLINE void oal_down(oal_semaphore_stru *pst_sem)
{
    if (pst_sem == NULL) {
        oal_io_print("[%s]pst_sem is null!\n", __func__);
        return;
    }
    if (LOS_SemPend(pst_sem->usSemID, LOS_WAIT_FOREVER) != LOS_OK) {
        oal_io_print("LOS_SemPend failed!\n");
        return;
    }
}

OAL_STATIC OAL_INLINE int32_t oal_down_timeout(oal_semaphore_stru *pst_sem, uint32_t ul_timeout)
{
    uint32_t ul_reval;
    if (pst_sem == NULL) {
        oal_io_print("[%s]pst_sem is null!\n", __func__);
        return -OAL_EFAIL;
    }

    ul_reval = LOS_SemPend(pst_sem->usSemID, ul_timeout);
    if (ul_reval != LOS_OK) {
        return ul_reval;
    }
    return OAL_SUCC;
}

OAL_STATIC OAL_INLINE int32_t oal_down_interruptible(oal_semaphore_stru *pst_sem)
{
    uint32_t ul_reval;
    if (pst_sem == NULL) {
        oal_io_print("[%s]pst_sem is null!\n", __func__);
        return -OAL_EFAIL;
    }
    ul_reval = LOS_SemPend(pst_sem->usSemID, LOS_WAIT_FOREVER);
    if (ul_reval != LOS_OK) {
        return ul_reval;
    }
    return OAL_SUCC;
}

OAL_STATIC OAL_INLINE int32_t oal_down_trylock(oal_semaphore_stru *pst_sem)
{
    uint32_t ul_reval;
    if (pst_sem == NULL) {
        oal_io_print("[%s]pst_sem is null!\n", __func__);
        return -OAL_EFAIL;
    }

    ul_reval = LOS_SemPend(pst_sem->usSemID, LOS_NO_WAIT);
    if (ul_reval != LOS_OK) {
        return ul_reval;
    }
    return OAL_SUCC;
}

OAL_STATIC OAL_INLINE int32_t oal_sema_destroy(oal_semaphore_stru *pst_sem)
{
    uint32_t ul_reval;
    if (pst_sem == NULL) {
        oal_io_print("[%s]pst_sem is null!\n", __func__);
        return -OAL_EFAIL;
    }

    ul_reval = LOS_SemDelete(pst_sem->usSemID);
    if (ul_reval != LOS_OK) {
        return ul_reval;
    }
    return OAL_SUCC;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_semaphore.h */

