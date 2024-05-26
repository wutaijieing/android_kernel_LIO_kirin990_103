

#ifndef __OAL_LITEOS_SPINLOCK_H__
#define __OAL_LITEOS_SPINLOCK_H__

#include <los_task.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_OAL_SPINLOCK_H
typedef struct _oal_spin_lock_stru_ {
    uint32_t  lock;
} oal_spin_lock_stru;

#define oal_spin_lock_init(lock) do {}while (0)
/*
锁线程调度，支持嵌套
*/
#define oal_spin_lock(lock) do { LOS_TaskLock(); }while (0)

#define oal_spin_unlock(lock) do { LOS_TaskUnlock(); }while (0)

/*
锁线程调度，支持嵌套
*/
#define oal_spin_lock_bh(lock) do { LOS_TaskLock(); }while (0)

#define oal_spin_unlock_bh(lock) do { LOS_TaskUnlock(); }while (0)

/*
锁硬件中断，不支持嵌套
*/
#define oal_spin_lock_irq(lock) do { LOS_IntLock(); }while (0)

#define oal_spin_unlock_irq(lock) do { LOS_IntUnLock(); }while (0)


/*
锁硬件中断，支持嵌套
*/
#define oal_spin_lock_irq_save(lock, flags)  do { *(flags) = LOS_IntLock(); } while (0)

#define oal_spin_unlock_irq_restore(lock, flags)  do { LOS_IntRestore(*(flags)); } while (0)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_spinlock.h */

