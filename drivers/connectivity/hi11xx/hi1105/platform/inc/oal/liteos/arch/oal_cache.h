

#ifndef __OAL_LITEOS_CACHE_H__
#define __OAL_LITEOS_CACHE_H__

#include "oal_types.h"
#include <linux/device.h>
#include <linux/compiler.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/fb.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define oal_cacheline_aligned   ____cacheline_aligned

extern void __iomem *g_l2cache_base;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of oal_cache.h */
