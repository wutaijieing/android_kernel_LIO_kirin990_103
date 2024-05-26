

#ifndef __OAL_LITEOS_MUTEX_H__
#define __OAL_LITEOS_MUTEX_H__

#include <pthread.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

typedef pthread_mutex_t oal_mutex_stru;

# define    OAL_MUTEX_INIT(mutex)        pthread_mutex_init(mutex, NULL)
# define    OAL_MUTEX_DESTROY(mutex)        pthread_mutex_destroy(mutex)
OAL_STATIC OAL_INLINE void oal_mutex_lock(oal_mutex_stru *lock)
{
    int32_t ret;
    ret = pthread_mutex_lock(lock);
    if (ret != ENOERR) {
        oal_io_print("ERROR:mutex lock fail! error no = %d \n", ret);
    }
}

OAL_STATIC OAL_INLINE int32_t oal_mutex_trylock(oal_mutex_stru *lock)
{
    return pthread_mutex_trylock(lock);
}

OAL_STATIC OAL_INLINE void oal_mutex_unlock(oal_mutex_stru *lock)
{
    int32_t ret;
    ret = pthread_mutex_unlock(lock);
    if (ret != ENOERR) {
        oal_io_print("ERROR:mutex unlock fail! error no = %d \n", ret);
    }
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_mutex.h */

