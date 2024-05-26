

#ifndef __FRW_TIMER_H__
#define __FRW_TIMER_H__

/* 其他头文件包含 */
#include "frw_ext_if.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_FRW_TIMER_H

/* 宏定义 */
#define FRW_TIME_UL_MAX       0xFFFFFFFF
#define FRW_TIMER_MAX_TIMEOUT (FRW_TIME_UL_MAX >> 1) /* 32位最大值的1/2 */

/* 函数声明 */
oal_uint32 frw_timer_timeout_proc(frw_event_mem_stru *pst_timeout_event);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 18, 0))
oal_void frw_timer_init(oal_uint32 ul_delay, oal_timer_func p_func, oal_ulong arg);
#else
oal_void frw_timer_init(oal_uint32 ul_delay, oal_timer_func p_func, oal_uint arg);
#endif
oal_void frw_timer_exit(oal_void);
oal_void frw_timer_timeout_proc_event(oal_timeout_func_para_t arg);
oal_void frw_timer_restart(oal_void);
oal_void frw_timer_stop(oal_void);
#if defined(_PRE_FRW_TIMER_BIND_CPU) && defined(CONFIG_NR_CPUS)
extern oal_uint32 g_frw_timer_cpu_count[];
#endif

/* return true if the time a is after time b,in case of overflow and wrap around to zero */
OAL_STATIC OAL_INLINE oal_int32 frw_time_after(oal_uint32 a, oal_uint32 b)
{
    return ((oal_int32)((b) - (a)) <= 0);
}

#endif /* end of frw_timer.h */
