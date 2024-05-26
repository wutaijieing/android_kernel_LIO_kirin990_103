

#ifndef __OAL_LITEOS_SCHEDULE_H__
#define __OAL_LITEOS_SCHEDULE_H__
/*lint -e322*/
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/jiffies.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#endif
/*lint +e322*/
#include "oal_util.h"
#include "time.h"
#include "oal_types.h"
#include "linux/completion.h"
#include "oal_mm.h"
#include "los_swtmr.h"
#include "oal_time.h"
#include "oal_timer.h"
#include "oal_wakelock.h"
#include "oal_rw_lock.h"
#include "oal_spinlock.h"
#include "oal_atomic.h"
#include "oal_file.h"
#include "oal_wait.h"
#include "oal_semaphore.h"
#include "oal_completion.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#define MAX_ERRNO   4095

#define EXPORT_SYMBOL_GPL(x)
#define OAL_EXPORT_SYMBOL(x)

#define IS_ERR_VALUE(x) unlikely((x) >= (unsigned long) - MAX_ERRNO)

// pedef struct tasklet_struct       oal_tasklet_stru;
typedef void                    (*oal_defer_func)(uint32_t);

/* tasklet声明 */
#define OAL_DECLARE_TASK    DECLARE_TASKLET

#define oal_in_interrupt()  in_interrupt()
#define in_atomic()        0
#define oal_in_atomic()     in_atomic()
typedef void (*oal_timer_func)(uintptr_t);

typedef uint32_t (*oal_module_func_t)(void);

/* 模块入口 */
#define oal_module_init(_module_name)   module_init(_module_name)

#define oal_module_license(_license_name)

#define oal_module_param(_symbol, _type, _name)

#define OAL_S_IRUGO         S_IRUGO

/* 模块出口 */
#define oal_module_exit(_module_name)   module_exit(_module_name)

/* 模块符号导出 */
#define oal_module_symbol(_symbol)      EXPORT_SYMBOL(_symbol)  //lint !e19
#define OAL_MODULE_DEVICE_TABLE(_type, _name) MODULE_DEVICE_TABLE(_type, _name)

#define oal_smp_call_function_single(core, task, info, wait) smp_call_function_single(core, task, info, wait)

enum {
    TASKLET_STATE_SCHED,    /* Tasklet is scheduled for execution */
    TASKLET_STATE_RUN   /* Tasklet is running (SMP only) */
};

typedef struct proc_dir_entry       oal_proc_dir_entry_stru;
typedef struct _oal_task_lock_stru_ {
    oal_wait_queue_head_stru    wq;
    struct task_struct  *claimer;   /* task that has host claimed */
    oal_spin_lock_stru      lock;       /* lock for claim and bus ops */
    uint32_t            claim_addr;
    uint32_t           claimed;
    int32_t            claim_cnt;
} oal_task_lock_stru;
OAL_STATIC OAL_INLINE uint32_t IS_ERR_OR_NULL(const void *ptr)
{
    return !ptr;
}

OAL_STATIC OAL_INLINE uint32_t  oal_copy_from_user(void *p_to, const void *p_from, uint32_t ul_size)
{
    errno_t l_ret;
    if ((p_to == NULL) || (p_from == NULL) || (ul_size <= 0)) {
        return OAL_FAIL;
    }
    l_ret = memcpy_s(p_to, ul_size, p_from, ul_size);
    if (l_ret != EOK) {
        return OAL_FAIL;
    }

    return 0;
}

OAL_STATIC OAL_INLINE uint32_t  oal_copy_to_user(void *p_to, const void *p_from, uint32_t ul_size)
{
    errno_t l_ret;
    if ((p_to == NULL) || (p_from == NULL) || (ul_size <= 0)) {
        return OAL_FAIL;
    }
    l_ret = memcpy_s(p_to, ul_size, p_from, ul_size);
    if (l_ret != EOK) {
        return OAL_FAIL;
    }

    return 0;
}


OAL_STATIC OAL_INLINE oal_proc_dir_entry_stru* oal_create_proc_entry(const int8_t *pc_name, uint16_t us_mode,
                                                                     oal_proc_dir_entry_stru *pst_parent)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,44)) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    return NULL;
#else
    return create_proc_entry(pc_name, us_mode, pst_parent);
#endif
}


OAL_STATIC OAL_INLINE void oal_remove_proc_entry(const int8_t *pc_name, oal_proc_dir_entry_stru *pst_parent)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,44)) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#else
    return remove_proc_entry(pc_name, pst_parent);
#endif
}

OAL_STATIC OAL_INLINE int oal_allow_signal(int sig)
{
    return 0;
}


#ifdef _PRE_OAL_FEATURE_TASK_NEST_LOCK
/*****************************************************************************
 函 数 名  : oal_smp_task_lock
 功能描述  : lock the task, the lock can be double locked by the same process

 输出参数  : 无

 修改历史      :
  1.日    期   : 2015年9月28日
    修改内容   : 新生成函数

*****************************************************************************/
extern void _oal_smp_task_lock_(oal_task_lock_stru* pst_lock, uint32_t  claim_addr);
#define oal_smp_task_lock(lock)    _oal_smp_task_lock_(lock, (uintptr_t)_THIS_IP_)


OAL_STATIC OAL_INLINE void oal_smp_task_unlock(oal_task_lock_stru* pst_lock)
{
    uint32_t flags;

    if (oal_warn_on(in_interrupt() || oal_in_atomic())) {
        return;
    }

    if (oal_unlikely(!pst_lock->claimed)) {
        oal_warn_on(1);
        return;
    }

    oal_spin_lock_irq_save(&pst_lock->lock, &flags);
    if (--pst_lock->claim_cnt) {
        oal_spin_unlock_irq_restore(&pst_lock->lock, &flags);
    } else {
        pst_lock->claimed = 0;
        pst_lock->claimer = NULL;
        oal_spin_unlock_irq_restore(&pst_lock->lock, &flags);
        wake_up(&pst_lock->wq);
    }
}


OAL_STATIC OAL_INLINE void oal_smp_task_lock_init(oal_task_lock_stru* pst_lock)
{
    memset_s((void*)pst_lock, sizeof(oal_task_lock_stru), 0, sizeof(oal_task_lock_stru));

    oal_spin_lock_init(&pst_lock->lock);
    oal_wait_queue_init_head(&pst_lock->wq);
    pst_lock->claimed = 0;
    pst_lock->claim_cnt = 0;
}
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_schedule.h */
