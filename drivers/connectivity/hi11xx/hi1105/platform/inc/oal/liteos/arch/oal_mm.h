

#ifndef __OAL_LITEOS_MM_H__
#define __OAL_LITEOS_MM_H__


/*lint -e322*/
#include <linux/hardirq.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <stdlib.h>
/*lint +e322*/
#include "oal_types.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define OAL_MEMCMP                                  memcmp

#define OAL_GFP_KERNEL                          0L
#define OAL_GFP_ATOMIC                          0L

OAL_STATIC OAL_INLINE void* oal_memalloc(uint32_t ul_size)
{
    if (ul_size == 0) {
        return NULL;
    }
    return malloc(ul_size);
}

OAL_STATIC OAL_INLINE void*  oal_kzalloc(uint32_t ul_size, int32_t l_flags)
{
    (void)(l_flags);
    return calloc(1, ul_size);
}

OAL_STATIC OAL_INLINE void*  oal_vmalloc(uint32_t ul_size)
{
    return malloc(ul_size);
}


OAL_STATIC OAL_INLINE void  oal_free(void *p_buf)
{
    if (p_buf == NULL) {
        return;
    }
    free(p_buf);
}

OAL_STATIC OAL_INLINE void  oal_vfree(void *p_buf)
{
    free(p_buf);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_mm.h */
