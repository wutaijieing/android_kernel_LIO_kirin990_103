

#ifndef __OAL_LITEOS_WAIT_H__
#define __OAL_LITEOS_WAIT_H__

#include <linux/wait.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
typedef wait_queue_head_t    oal_wait_queue_head_stru;


extern void __init_waitqueue_head(wait_queue_head_t *wait);


extern void __wake_up_interruptible(wait_queue_head_t *wait);
#ifndef wake_up_interruptible
#define wake_up_interruptible(p_wait) __wake_up_interruptible(p_wait)
#endif

#define oal_wait_queue_wake_up_interrupt(_pst_wq)     wake_up_interruptible(_pst_wq)

#define OAL_WAIT_QUEUE_WAKE_UP(_pst_wq)     wake_up(_pst_wq)

#define oal_wait_queue_init_head(_pst_wq)   init_waitqueue_head(_pst_wq)

#define oal_wait_event_interruptible_timeout_m(_st_wq, _condition, _timeout) \
    wait_event_interruptible_timeout(_st_wq, _condition, _timeout)

#define OAL_WAIT_EVENT_TIMEOUT(_st_wq, _condition, _timeout) \
    wait_event_interruptible_timeout(_st_wq, _condition, _timeout)

#define oal_wait_event_interruptible_m(_st_wq, _condition) \
    wait_event_interruptible(_st_wq, _condition)
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_wait.h */

