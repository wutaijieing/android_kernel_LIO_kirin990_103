

#ifndef __OAL_LITEOS_THREAD_H__
#define __OAL_LITEOS_THREAD_H__

#include <los_task.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#if (HW_LITEOS_OPEN_VERSION_NUM >= KERNEL_VERSION(3,2,3))
#define LOS_TASK_CB         LosTaskCB
#define pcTaskName          taskName
#define uwTaskID            taskID
#define pstRunTask          runTask
#define g_pstTaskCBArray    g_taskCBArray
#define g_stLosTask         g_losTask
#endif
typedef  LOS_TASK_CB oal_kthread_stru;
typedef struct task_struct oal_task_stru;

typedef struct _kthread_param_ {
    uint32_t         ul_stacksize;
    int32_t           l_prio;
    int32_t           l_policy;
    int32_t           l_cpuid;
    int32_t           l_nice;
} oal_kthread_param_stru;


#define OAL_CURRENT     oal_get_current()

#define OAL_SCHED_FIFO      1
#define OAL_SCHED_RR        2

#define NOT_BIND_CPU        (-1)

typedef int (*oal_thread_func)(void *);
OAL_STATIC OAL_INLINE oal_kthread_stru* oal_kthread_create(char                   *pc_thread_name,
                                                           oal_thread_func        pf_threadfn,
                                                           void                   *p_data,
                                                           oal_kthread_param_stru *pst_thread_param)
{
    int32_t uwRet;
    TSK_INIT_PARAM_S stSdTask;
    uint32_t ul_taskid;
    oal_kthread_stru *pst_kthread = NULL;

    memset_s(&stSdTask, sizeof(stSdTask), 0, sizeof(stSdTask));
    stSdTask.pfnTaskEntry  = (TSK_ENTRY_FUNC)pf_threadfn;
    stSdTask.auwArgs[0]    = (int32_t)p_data;
    stSdTask.uwStackSize   = pst_thread_param->ul_stacksize;
    stSdTask.pcName        = pc_thread_name;
    stSdTask.usTaskPrio    = (uint16_t)pst_thread_param->l_prio;
    stSdTask.uwResved      = LOS_TASK_STATUS_DETACHED;

    uwRet = LOS_TaskCreate(&ul_taskid, &stSdTask);
    if (uwRet != LOS_OK) {
        dprintf("Failed to create %s thread\n", pc_thread_name);
        return NULL;
    }

    pst_kthread = (oal_kthread_stru *)&g_pstTaskCBArray[ul_taskid];
    return pst_kthread;
}


OAL_STATIC OAL_INLINE void oal_kthread_stop(oal_kthread_stru *pst_kthread)
{
    UINT32 uwErrRet;
    if (oal_unlikely(pst_kthread == NULL)) {
        dprintf("thread can't stop\n");
        return;
    }
    dprintf("%s thread stop\n", pst_kthread->pcTaskName);
    uwErrRet = LOS_TaskDelete(pst_kthread->uwTaskID);
    if (uwErrRet != LOS_OK) {
        dprintf("LOS_TaskDelete fail!\n");
    }
}

OAL_STATIC OAL_INLINE  int32_t oal_kthread_should_stop(void)
{
    return 0;
}

OAL_STATIC OAL_INLINE oal_kthread_stru *oal_get_current(void)
{
    return g_stLosTask.pstRunTask;
}

OAL_INLINE static char* oal_get_current_task_name(void)
{
    return g_stLosTask.pstRunTask->pcTaskName;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
